#include "data_handler.h"

#include "ace/Log_Msg.h"
#include "ace/FILE_Connector.h"

#include <grp.h>
#include <sys/file.h>
#include <sys/sendfile.h>

Data_Handler::Data_Handler (const std::string &ip_addr, const int &port, const int &type) :
    type_ (type),
    mode_ (Data_Modes::STREAM),
    is_lock_ (false),
    wfile_try_connection_ (nullptr)
{
    int set_addr_res = client_addr_.set (port, ip_addr.c_str ());
    if (set_addr_res == -1)
    {
        ACE_DEBUG ((LM_DEBUG, "set client address failed\n"));
    }
    memset (data_buffer_, 0, MAX_BUFFER_SIZE);
    data_link_.enable (ACE_NONBLOCK);
    file_link_.enable (ACE_NONBLOCK);
}

Data_Handler::~Data_Handler ()
{
    if (is_lock_ && flock (file_link_.get_handle(), LOCK_UN) != 0)
        ACE_DEBUG ( (LM_DEBUG, "shared unlock file failed\n"));
    if (is_lock_ && wfile_try_connection_ != nullptr && flock (fileno (wfile_try_connection_), LOCK_UN) != 0)
        ACE_DEBUG ( (LM_DEBUG, "exclusive unlock file failed\n"));
    is_lock_ = false;
    if (wfile_try_connection_ != nullptr)
        fclose (wfile_try_connection_);
    data_link_.close ();
    file_link_.close ();
    ACE_DEBUG ( (LM_DEBUG, "data connection destroyed.\n"));
}

int Data_Handler::data_link_init ()
{
    ACE_SOCK_Connector connector;
    if (connector.connect (data_link_, client_addr_) < 0)
    {
        ACE_DEBUG ((LM_DEBUG, "connect client's data failed\n"));
        return -1;
    }
    return 0;
}

int Data_Handler::file_link_init (bool is_output)
{
    ACE_FILE_Connector connector;
    int result = 0;
    if (is_output)
        result = connector.connect (file_link_,
                                    ACE_FILE_Addr (file_path_.c_str ()),
                                    0,
                                    ACE_Addr::sap_any,
                                    0,
                                    O_RDONLY,
                                    ACE_DEFAULT_FILE_PERMS);
    else
    {
        wfile_try_connection_ = ACE_OS::fopen (file_path_.c_str (), "a");
        if (wfile_try_connection_ == nullptr)
        {
            ACE_DEBUG ((LM_DEBUG, ACE_TEXT ("connect file failed.\n")));
            return -1;
        }
    }
    if (result < 0)
    {
        ACE_DEBUG ((LM_DEBUG, ACE_TEXT ("connect file failed.\n")));
        return -1;
    }

    return 0;
}

int Data_Handler::send_file ()
{
    if (type_ != Data_Types::IMAGE)
        return -1;
    
    if (!is_lock_ && flock (file_link_.get_handle (), LOCK_SH | LOCK_NB) != 0)
    {
        ACE_DEBUG ( (LM_DEBUG, "shared lock file failed\n"));
        return -1;
    }
    is_lock_ = true;
    ACE_DEBUG ( (LM_DEBUG, "lock done\n"));
    int send_count = 0;
    long offset = 0;
    while (1)
    {
        send_count = sendfile (data_link_.get_handle (), file_link_.get_handle (), &offset, 512);
        //ACE_DEBUG ( (LM_DEBUG, "offset: %d\n", offset));
        if (send_count < 0)
        {
            if (errno == EAGAIN || errno == EINTR || errno == EWOULDBLOCK)
                continue;
            else
                return -1;
        } 
        else if (send_count == 0)
            return 0;
    }
    return 0;
}

int Data_Handler::list_dir (const std::string &dir_path)
{
    std::string path = dir_path;
    if (path.back () != '/')
        path.push_back ('/');
    DIR *dir = opendir (path.c_str ());
    if (dir == nullptr)
    {
        return -1;
    }
    
    struct dirent *content;
    struct stat content_stat;
    while ( (content = readdir (dir)) != nullptr)
    {
        std::string content_path = path + content->d_name;
        ACE_OS::stat (content_path.c_str (), &content_stat);

        char mode[10] = {0};
        mode_to_letters (content_stat.st_mode, mode);

        char owner[8] = {0};
        uid_to_name (content_stat.st_uid, owner);

        char group[8]= {0};
        gid_to_name (content_stat.st_gid, group);

        int n = snprintf(data_buffer_,MAX_BUFFER_SIZE,"%s %4d %-8s %-8s %8d %.12s ",
                        mode,
                        (int)content_stat.st_nlink,
                        owner,
                        group,
                        (int)content_stat.st_size,
                        4+ctime(&content_stat.st_mtime));

        int name_len = snprintf (data_buffer_ + n, MAX_BUFFER_SIZE - n, "%s", content->d_name);
        data_buffer_[n + name_len] = '\r';
        data_buffer_[n + name_len + 1] = '\n';
        if (data_link_.send (data_buffer_, n + name_len + 2) < 0)
        {
            ACE_DEBUG ( (LM_DEBUG, "send list dir failed\n"));
            return -2;
        }
    }
    closedir (dir);
    return 0;
}

int Data_Handler::list_file (const std::string &file_path)
{
    struct stat file_stat;
    if (ACE_OS::stat (file_path.c_str (), &file_stat) == -1)
    {
        return -1;
    }

    char mode[10] = {0};
    mode_to_letters (file_stat.st_mode, mode);

    char owner[8] = {0};
    uid_to_name (file_stat.st_uid, owner);

    char group[8]= {0};
    gid_to_name (file_stat.st_gid, group);

    int n = snprintf(data_buffer_,MAX_BUFFER_SIZE,"%s %4d %-8s %-8s %8d %.12s ",
                    mode,
                    (int)file_stat.st_nlink,
                    owner,
                    group,
                    (int)file_stat.st_size,
                    4+ctime(&file_stat.st_mtime));
    int pos = file_path.rfind ('/');
    int name_len = snprintf (data_buffer_ + n, MAX_BUFFER_SIZE - n, "%s", file_path.c_str () + pos + 1);
    data_buffer_[n + name_len] = '\r';
    data_buffer_[n + name_len + 1] = '\n';
    if (data_link_.send (data_buffer_, n + name_len + 2) < 0)
    {
        ACE_DEBUG ( (LM_DEBUG, "send list file failed\n"));
        return -2;
    }
    return 0;
}

int Data_Handler::recv_file ()
{
    if (type_ != Data_Types::IMAGE)
        return -1;
    if (!is_lock_ && flock (fileno (wfile_try_connection_), LOCK_EX | LOCK_NB) != 0)
    {
        ACE_DEBUG ( (LM_DEBUG, "lock file failed\n"));
        return -1;
    }
    is_lock_ = true;

    ACE_FILE_Connector connector;
    connector.connect (file_link_,
                        ACE_FILE_Addr (file_path_.c_str ()),
                        0,
                        ACE_Addr::sap_any,
                        0,
                        O_RDWR | O_CREAT | O_TRUNC,
                        ACE_DEFAULT_FILE_PERMS);

    int recv_count = 0;
    while(1)
    {
        recv_count = data_link_.recv (data_buffer_, MAX_BUFFER_SIZE);
        if (recv_count < 0)
        {
            if (errno == EAGAIN || errno == EINTR || errno == EWOULDBLOCK)
                continue;
            else
                return -1;
        } 
        else if (recv_count == 0)
            return 0;
        else
        {
            int write_count = file_link_.send (data_buffer_, recv_count);
            if (write_count <= 0)
                return -1;
        }
    }
    return 0;
}

void Data_Handler::mode_to_letters(mode_t mode, char *buf)
{
    ACE_OS::memset (buf, '-', 10);
    switch (mode & S_IFMT) 
    {
        case S_IFBLK:   buf[0] = 'b'; break; // block device
        case S_IFCHR:   buf[0] = 'c'; break; // character device
        case S_IFDIR:   buf[0] = 'd'; break; // directory
        case S_IFIFO:   buf[0] = 'f'; break; // FIFO
        case S_IFLNK:   buf[0] = 'l'; break; // symbolic link
        case S_IFSOCK:  buf[0] = 's'; break; // socket
        default:        buf[0] = '-'; break;
    }
                                                                                               
    if(mode&S_IRUSR)    buf[1] = 'r';
    if(mode&S_IWUSR)    buf[2] = 'w';
    if(mode&S_IXUSR)    buf[3] = 'x';
    if(mode&S_IRGRP)    buf[4] = 'r';
    if(mode&S_IWGRP)    buf[5] = 'w';
    if(mode&S_IXGRP)    buf[6] = 'x';
    if(mode&S_IROTH)    buf[7] = 'r';
    if(mode&S_IWOTH)    buf[8] = 'w';
    if(mode&S_IXOTH)    buf[9] = 'x';
}

void Data_Handler::uid_to_name (uid_t uid, char *buf)
{
    struct passwd *pw_ptr;
    if ( (pw_ptr = getpwuid (uid)) == nullptr)
    {
        std::string uid_str = std::to_string (uid);
        ACE_OS::strncpy (buf, uid_str.c_str (), 8);
    }
    else
        ACE_OS::strncpy (buf, pw_ptr->pw_name, 8);
}

void Data_Handler::gid_to_name (gid_t gid, char *buf)
{
    struct group *grp_ptr;
    if ( (grp_ptr = getgrgid (gid)) == nullptr)
    {
        std::string gid_str = std::to_string (gid);
        ACE_OS::strncpy (buf, gid_str.c_str (), 8);
    }
    else
        ACE_OS::strncpy (buf, grp_ptr->gr_name, 8);
}