#include "command_handler.h"
#include "msg.h"

#include "ace/Log_Msg.h"
#include "ace/FILE_Connector.h"
#include "ace/Timer_Queue.h"
#include "ace/Date_Time.h"

#include <algorithm>
#include <cstdio>
#include <dirent.h>
#include <sstream>
#include <sys/stat.h>
#include <grp.h>

const std::unordered_map<std::string, int (Command_Handler::*) ()> Command_Handler::commands_
(
    {
        { "user", &Command_Handler::handle_user },
        { "pass", &Command_Handler::handle_pass },
        { "quit", &Command_Handler::handle_quit },
        { "pwd", &Command_Handler::handle_pwd },
        { "cwd", &Command_Handler::handle_cwd },
        { "cdup", &Command_Handler::handle_cdup },
        { "port", &Command_Handler::handle_port },
        { "retr", &Command_Handler::handle_retr },
        { "list", &Command_Handler::handle_list },
        { "type", &Command_Handler::handle_type },
        { "stor", &Command_Handler::handle_stor },
        { "pasv", &Command_Handler::handle_pasv },
        { "rnfr", &Command_Handler::handle_rnfr },
        { "rnto", &Command_Handler::handle_rnto },
        { "rmd", &Command_Handler::handle_rmd },
        { "dele", &Command_Handler::handle_dele },
        { "mkd", &Command_Handler::handle_mkd },    
    }
);

Command_Handler::Command_Handler (ACE_Reactor *reactor) : 
    ACE_Event_Handler (reactor), 
    data_handler_ (nullptr),
    is_pasv_ (false),
    pasv_port_ (0),
    max_client_timeout_ (MAX_CLIENT_TIMEOUT)
{
    ACE_OS::memset (recv_buffer_, 0 , sizeof(recv_buffer_));
}

int Command_Handler::init () 
{
    int result = reactor ()->register_handler (command_link_.get_handle (),
                                                this, 
                                                ACE_Event_Handler::READ_MASK);
    if (result == -1)
    {
        ACE_DEBUG ( (LM_DEBUG, "register read event failed.\n"));
        return -1;
    }

    ACE_Time_Value reschedule (max_client_timeout_.sec () / 2);
    result = reactor ()->schedule_timer (this, 0, max_client_timeout_, reschedule);
    if (result == -1)
    {
        ACE_DEBUG ( (LM_DEBUG, "register timeout event failed.\n"));
        return -1;
    }

    time_of_last_command_ = 
        reactor ()->timer_queue ()->gettimeofday ();
    send_response (MSG_NEW_USER);
    return 0;
}

int Command_Handler::handle_input (ACE_HANDLE) 
{
    ssize_t recv_len = command_link_.recv (recv_buffer_, MAX_COMMAND_BUFFER_SIZE - 1);
    if (recv_len < 0)
    {
        if (errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN)
            return 1;
        else
            return -1;
    }
    else if (recv_len == 0)
        return -1;
    if (recv_buffer_[recv_len - 2] == '\r' && recv_buffer_[recv_len - 1] == '\n')
        recv_buffer_[recv_len - 2] = '\0';
    else
    {
        ACE_DEBUG ( (LM_DEBUG, "command is incomplete\n"));
        return Command_Consequences::COMMAND_CON_CLOSE;
    }
    int command_res = handle_command ();
    if (command_res == Command_Consequences::COMMAND_CON_CLOSE)
    {
        ACE_DEBUG ( (LM_DEBUG, "command failed, command connection closing\n"));
        send_response (MSG_CLOSE);
        return -1;
    }
    else if (command_res == Command_Consequences::DATA_CON_CLOSE && data_handler_)
    {   
        //data_handler_->close ();
        data_handler_.reset ();
        return 0;
    }
    return 0;
}

int Command_Handler::handle_close (ACE_HANDLE, ACE_Reactor_Mask)
{
    ACE_DEBUG ((LM_DEBUG, ACE_TEXT("handle command close\n")));
    delete this;
    return 0;
}

int Command_Handler::handle_timeout (const ACE_Time_Value &now, const void *)
{
    ACE_Date_Time now_date(now);
    ACE_DEBUG((LM_DEBUG,"now time %d-%d-%d %d:%d:%d:%d\n",
		now_date.year(),
		now_date.month(),
		now_date.day(),
		now_date.hour(),
		now_date.minute(),
		now_date.second(),
		now_date.microsec()));

    ACE_Date_Time date(time_of_last_command_);
    ACE_DEBUG((LM_DEBUG,"last time %d-%d-%d %d:%d:%d:%d\n",
		date.year(),
		date.month(),
		date.day(),
		date.hour(),
		date.minute(),
		date.second(),
		date.microsec()));    
    if (now - time_of_last_command_ >= max_client_timeout_)
        handle_close ();
    return 0;
}

int Command_Handler::handle_command ()
{
    ACE_DEBUG ((LM_DEBUG, ACE_TEXT("handle command\n")));
    
    char *space_addr = strchr (recv_buffer_, ' ');
    space_addr = (space_addr == nullptr ? recv_buffer_ + strlen (recv_buffer_) : space_addr);
    std::string recv_command (recv_buffer_, space_addr);
    ACE_DEBUG ( (LM_DEBUG, "%s\n", recv_command.c_str ()));
    std::transform (recv_command.begin (),
                    recv_command.end (),
                    recv_command.begin (),
                    ::tolower);
    auto command_ite = commands_.find (recv_command);
    if (command_ite != commands_.end ())
    {
        time_of_last_command_ = 
            reactor ()->timer_queue ()->gettimeofday ();
        int result = (this->*(command_ite->second)) ();
        return result;
    }
    else
    {
        send_response (MSG_INVALID_COMMAND);
        return Command_Consequences::CONTINUE;
    }
}

int Command_Handler::send_response (const std::string &msg) 
{
    int send_len = command_link_.send (msg.c_str (), msg.length ());
    return (send_len == msg.length ()) ? 0 : -1;
}

int Command_Handler::send_response (const char *format, const char *detail)
{
    char send_buf[MAX_COMMAND_BUFFER_SIZE];
    sprintf (send_buf, format, detail);
    int send_len = command_link_.send (send_buf, strlen (send_buf));
    return (send_len == strlen (send_buf)) ? 0 : -1;
}

int Command_Handler::handle_user () 
{
    if (user_.check_logged_in ()) 
    {
        send_response (MSG_LOGIN_SUCCESS);
        return Command_Consequences::CONTINUE;
    }

    CHECK_COMMAND_LENGTH(recv_buffer_, 6);

    std::string recv_name(recv_buffer_ + 5, recv_buffer_ + strlen (recv_buffer_));
    int result = user_.check_username (recv_name);
    if (result == 0)
    {
        send_response (MSG_REQUIRE_PASS);
        return Command_Consequences::OK;
    }
    else
    {
        send_response (MSG_LOGIN_FAIL);
        return Command_Consequences::CONTINUE;
    }
    
}

int Command_Handler::handle_pass ()
{
    if( user_.get_user_name ().length () == 0)
    {
        send_response (MSG_REQUIRE_USER);
        return Command_Consequences::CONTINUE;
    }

    CHECK_COMMAND_LENGTH(recv_buffer_, 6);
    std::string recv_password (recv_buffer_ + 5, recv_buffer_ + strlen (recv_buffer_));
    int result = user_.check_password (recv_password);
    if (result == 0)
    {
        send_response (MSG_LOGIN_SUCCESS);
        return Command_Consequences::OK;
    }
    else
    {
        send_response (MSG_LOGIN_FAIL);
        return Command_Consequences::CONTINUE;
    }
}

int Command_Handler::handle_pwd ()
{
    CHECK_LOGIN();

    send_response (MSG_CUR_PATH, user_.get_cur_dir ().c_str ());
    return Command_Consequences::OK;
}

int Command_Handler::handle_cwd ()
{
    CHECK_LOGIN();

    CHECK_COMMAND_LENGTH(recv_buffer_, 5);

    std::string dir (recv_buffer_ + 4, recv_buffer_ + strlen (recv_buffer_));
    relative_to_absolute (dir);
    
    ACE_DEBUG ( (LM_DEBUG, "dir:%s\n", dir.c_str ()));
    user_.set_cur_dir (dir);
    send_response (MSG_COMMON_SUCCESS);
    return Command_Consequences::OK;
}

int Command_Handler::handle_cdup ()
{
    CHECK_LOGIN();

    std::string cur_dir = user_.get_cur_dir ();
    if (cur_dir.length () == 1)
    {
        send_response (MSG_COMMON_SUCCESS);
        return Command_Consequences::OK;
    }
    size_t pos = cur_dir.rfind ('/');
    pos = (pos == 0 ? 1 : pos);
    user_.set_cur_dir (cur_dir.substr (0,pos));
    send_response (MSG_COMMON_SUCCESS);
    return Command_Consequences::OK;
}

int Command_Handler::handle_port ()
{
    CHECK_LOGIN();
    
    CHECK_COMMAND_LENGTH(recv_buffer_, 6);

    std::istringstream param (recv_buffer_ + 5);
    int h1, h2, h3, h4, p1, p2;
    char comma;
    param >> h1 >> comma >> h2 >> comma >> h3 >> comma >> h4 
          >> comma >> p1 >> comma >> p2;
    int port = (p1 << 8) + p2;
    std::stringstream ss;
    ss << h1 << '.' << h2 << '.' << h3 << '.' << h4;
    std::string ip_addr = ss.str ();

    if (is_pasv_)
    {
        pasv_acceptor_.close ();
        is_pasv_ = false;
    }
    if (data_handler_)
        data_handler_.reset ();
    data_handler_ = std::make_unique<Data_Handler> (ip_addr, port, data_type_);
    send_response (MSG_COMMON_SUCCESS);
    return Command_Consequences::OK;
}

int Command_Handler::handle_pasv ()
{
    CHECK_LOGIN();

    if (!is_pasv_)
    {
        ACE_INET_Addr local_addr;
        local_addr.set ((u_short)0, (ACE_UINT32) INADDR_ANY);
        if (pasv_acceptor_.open (local_addr) < 0)
        {
            ACE_DEBUG ( (LM_DEBUG, "open pasv port failed\n"));
            send_response (MSG_FAILED);
            return Command_Consequences::CONTINUE;
        }
        pasv_acceptor_.get_local_addr (local_addr);
        ACE_DEBUG ( (LM_DEBUG, "open port: %d\n", local_addr.get_port_number ()));
        pasv_port_ = local_addr.get_port_number ();
        is_pasv_ = true;
    }
    if (data_handler_)
        data_handler_.reset ();
    data_handler_ = std::make_unique<Data_Handler> (data_type_);
    u_short high_byte = pasv_port_ >> 8;
    u_short low_byte = pasv_port_ & 0x00ff;
    std::stringstream pasv_address_ss;
    pasv_address_ss << HOST_ADDRESS << ',' << high_byte << ',' << low_byte;
    send_response (MSG_PASV_SUCCESS, pasv_address_ss.str ().c_str ());

    return Command_Consequences::OK;
}

int Command_Handler::handle_type ()
{
    CHECK_LOGIN();

    CHECK_COMMAND_LENGTH(recv_buffer_, 6);

    char type = recv_buffer_[5];
    ACE_DEBUG ( (LM_DEBUG, "type is %c\n",type));
    switch (type)
    {
    // case 'a':
    // case 'A':
    //     data_type_ = Data_Types::ASCII;
    //     break;
    
    // case 'E':
    // case 'e':
    //     data_type_ = Data_Types::EBCDIC;
    //     break;
    
    case 'I':
    case 'i':
        data_type_ = Data_Types::IMAGE;
        break;

    default:
        send_response (MSG_INVALID_PARAM);
        return Command_Consequences::CONTINUE;
    }
    send_response (MSG_COMMON_SUCCESS);
    return Command_Consequences::OK;
}

int Command_Handler::handle_quit ()
{
    handle_close ();
    return 0;
}

int Command_Handler::make_data_connection ()
{
    if(is_pasv_)
    {
        if (pasv_acceptor_.accept (data_handler_->get_data_link ()) < 0)
        {
            ACE_DEBUG ( (LM_DEBUG, "pasv accept failed\n"));
            return -1;
        }
        ACE_DEBUG ( (LM_DEBUG, "pasv connection succeed.\n"));
        return 0;
    }
    else
        return data_handler_->data_link_init ();
}

int Command_Handler::handle_retr () 
{
    CHECK_DATA_LINK_VALID();

    CHECK_COMMAND_LENGTH(recv_buffer_, 6);

    std::string file_path (recv_buffer_ + 5, recv_buffer_ + strlen (recv_buffer_));
    relative_to_absolute (file_path);
    ACE_DEBUG( (LM_DEBUG, "file_path:%s\n", file_path.c_str ()));
    data_handler_->set_file_path (file_path);
    if (data_handler_->file_link_init (true) == -1)
    {  
        send_response (MSG_FAILED);
        return Command_Consequences::CONTINUE;
    }

    if (make_data_connection () == -1)
    {
        send_response (MSG_DATA_LINK_FAIL);
        return Command_Consequences::DATA_CON_CLOSE;
    }
    send_response (MSG_CONNECTION_READY);

    if (data_handler_->send_file () == -1)
    {
        ACE_DEBUG ( (LM_DEBUG, "send file failed\n"));
        send_response (MSG_FAILED);
        return Command_Consequences::DATA_CON_CLOSE;
    }
    //data_handler_->close ();
    data_handler_.reset ();
    time_of_last_command_ = 
        reactor ()->timer_queue ()->gettimeofday ();
    send_response (MSG_COMMON_SUCCESS);
    return Command_Consequences::OK;
}

int Command_Handler::handle_list ()
{
    CHECK_DATA_LINK_VALID();

    if (make_data_connection () == -1)
    {
        send_response (MSG_DATA_LINK_FAIL);
        return Command_Consequences::DATA_CON_CLOSE;
    }
    send_response (MSG_CONNECTION_READY);

    std::string path = user_.get_cur_dir ();
    if (strlen (recv_buffer_) > 5)
        path.assign (recv_buffer_ + 5);

    int list_dir_res = data_handler_->list_dir (path);
    if ( list_dir_res == -1)
    {
        int list_file_res = data_handler_->list_file (path);
        if ( list_file_res == -1)
        {
            send_response (MSG_INVALID_PARAM);
            return Command_Consequences::DATA_CON_CLOSE;
        }
        else if (list_file_res == -2)
        {
            send_response (MSG_FAILED);
            return Command_Consequences::DATA_CON_CLOSE;
        }
        //data_handler_->close ();
        data_handler_.reset ();
        send_response (MSG_COMMON_SUCCESS);
        return Command_Consequences::OK;
    }
    else if (list_dir_res == -2)
    {
        send_response (MSG_FAILED);
        return Command_Consequences::DATA_CON_CLOSE;
    }
    //data_handler_->close ();
    data_handler_.reset ();
    send_response (MSG_COMMON_SUCCESS);
    return Command_Consequences::OK;
}

int Command_Handler::handle_stor ()
{
    CHECK_DATA_LINK_VALID();

    CHECK_COMMAND_LENGTH(recv_buffer_, 6);

    std::string file_path (recv_buffer_ + 5, recv_buffer_ + strlen (recv_buffer_));
    relative_to_absolute (file_path);
    data_handler_->set_file_path (file_path);
    if(data_handler_->file_link_init (false) == -1)
    {
        send_response (MSG_FAILED);
        return Command_Consequences::DATA_CON_CLOSE;
    }

    if (make_data_connection () == -1)
    {
        send_response (MSG_DATA_LINK_FAIL);
        return Command_Consequences::DATA_CON_CLOSE;
    }
    send_response (MSG_CONNECTION_READY);

    if (data_handler_->recv_file () == -1)
    {
        send_response (MSG_FAILED);
        return Command_Consequences::DATA_CON_CLOSE;
    }
    //data_handler_->close ();
    data_handler_.reset ();
    time_of_last_command_ = 
        reactor ()->timer_queue ()->gettimeofday ();
    send_response (MSG_COMMON_SUCCESS);
    return Command_Consequences::OK;
}

int Command_Handler::handle_rnfr ()
{
    CHECK_LOGIN();

    CHECK_COMMAND_LENGTH(recv_buffer_, 6);

    std::string file_name (recv_buffer_ + 5, recv_buffer_ + strlen (recv_buffer_));
    std::string file_path = user_.get_cur_dir () + '/' + file_name;
    struct stat buffer;
    if(ACE_OS::stat (file_path.c_str(), &buffer) != 0)
    {
        send_response (MSG_INVALID_PARAM);
        return Command_Consequences::CONTINUE;
    }
    else
    {
        user_.set_old_file_name (file_name);
        send_response (MSG_COMMON_SUCCESS);
        return Command_Consequences::OK;
    }
}

int Command_Handler::handle_rnto ()
{
    CHECK_LOGIN();

    if (user_.get_old_file_name ().length () == 0)
    {
        send_response (MSG_BAD_SEQUENCE);
        return Command_Consequences::CONTINUE;
    }

    CHECK_COMMAND_LENGTH(recv_buffer_, 6);

    std::string new_name (recv_buffer_ + 5, recv_buffer_ + strlen (recv_buffer_));
    std::string new_file_path = user_.get_cur_dir () + '/' + new_name;
    std::string old_file_path = user_.get_cur_dir () + '/' + user_.get_old_file_name ();
    if (ACE_OS::rename (old_file_path.c_str (), new_file_path.c_str ()) == 0)
    {
        send_response (MSG_COMMON_SUCCESS);
        user_.get_old_file_name ().clear ();
        return Command_Consequences::OK;
    }
    else
    {
        send_response (MSG_FAILED);
        return Command_Consequences::CONTINUE;
    }
}


int Command_Handler::handle_rmd ()
{
    CHECK_LOGIN();

    CHECK_COMMAND_LENGTH(recv_buffer_, 5);

    std::string dir_path (recv_buffer_ + 4, recv_buffer_ + strlen (recv_buffer_));
    relative_to_absolute (dir_path);
    ACE_DEBUG ( (LM_DEBUG, "RMD %s\n", dir_path.c_str ()));
    struct stat buffer;
    if(ACE_OS::stat (dir_path.c_str(), &buffer) != 0 ||
        (buffer.st_mode & S_IFMT) != S_IFDIR)
    {
        send_response (MSG_INVALID_PARAM);
        return Command_Consequences::CONTINUE;
    }
    else if (ACE_OS::rmdir (dir_path.c_str ()) != 0)
    {
        send_response (MSG_FAILED);
        return Command_Consequences::CONTINUE;
    }
    else
    {
        send_response (MSG_FILE_SUCCESS);
        return Command_Consequences::OK;
    }
}

int Command_Handler::handle_dele ()
{
    CHECK_LOGIN();

    CHECK_COMMAND_LENGTH(recv_buffer_, 6);

    std::string file_path (recv_buffer_ + 5, recv_buffer_ + strlen (recv_buffer_));
    relative_to_absolute (file_path);
    struct stat buffer;
    if(ACE_OS::stat (file_path.c_str(), &buffer) != 0)
    {
        send_response (MSG_INVALID_PARAM);
        return Command_Consequences::CONTINUE;
    }
    else if (std::remove (file_path.c_str ()) != 0)
    {
        send_response (MSG_FAILED);
        return Command_Consequences::CONTINUE;
    }
    else
    {
        send_response (MSG_COMMON_SUCCESS);
        return Command_Consequences::OK;
    }
}

int Command_Handler::handle_mkd ()
{
    CHECK_LOGIN();

    CHECK_COMMAND_LENGTH(recv_buffer_, 5);

    std::string dir_path (recv_buffer_ + 4, recv_buffer_ + strlen (recv_buffer_));
    relative_to_absolute (dir_path);
    if (ACE_OS::mkdir (dir_path.c_str ()) < 0)
    {
        send_response (MSG_FAILED);
        return Command_Consequences::CONTINUE;
    }
    else
    {
        send_response (MSG_MKD_SUCCESS);
        return Command_Consequences::OK;
    }
}

inline void Command_Handler::relative_to_absolute (std::string &str)
{
    if (str[0] != '/')
    {
        std::string cur_dir = user_.get_cur_dir ();
        if (cur_dir.length () == 1)
            str = cur_dir + str;
        else
            str = cur_dir + '/' + str;
    }
}

Command_Handler::~Command_Handler ()
{
    reactor ()->remove_handler (command_link_.get_handle (), 
                                ACE_Event_Handler::READ_MASK |
                                ACE_Event_Handler::DONT_CALL);
    reactor ()->cancel_timer (this);
    if(data_handler_)
    {
        //data_handler_->close ();
        data_handler_.reset ();
    }
    pasv_acceptor_.close ();
    command_link_.close();
}