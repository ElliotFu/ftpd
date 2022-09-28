#ifndef COMMAND_HANDLER_H
#define COMMAND_HANDLER_H

#include "user_inf.h"
#include "data_handler.h"
#include "ftp_server.h"

#include "ace/Event_Handler.h"
#include "ace/Reactor.h"
#include "ace/SOCK_Stream.h"
#include "ace/Time_Value.h"

#include <string>
#include <memory>
#include <unordered_map>

#define MAX_COMMAND_BUFFER_SIZE 1024
#define HOST_ADDRESS "127,0,0,1"
#define MAX_CLIENT_TIMEOUT 1800

#define CHECK_LOGIN() \
    if (!user_.check_logged_in ()) \
    { \
        send_response (MSG_NOT_LOGIN); \
        return Command_Consequences::CONTINUE; \
    } \

#define CHECK_COMMAND_LENGTH(command_buffer, length) \
    if (strlen (command_buffer) < length) \
    { \
        send_response (MSG_INVALID_PARAM); \
        return Command_Consequences::CONTINUE; \
    } \

#define CHECK_DATA_LINK_VALID() \
    if (!data_handler_) \
    { \
        ACE_DEBUG ((LM_DEBUG, "data link not set.\n")); \
        send_response (MSG_BAD_SEQUENCE); \
        return Command_Consequences::CONTINUE; \
    } \

enum Command_Consequences
{
    OK = 0,
    COMMAND_CON_CLOSE = -1,
    DATA_CON_CLOSE = -2,
    CONTINUE = -3,
};

class Command_Handler : public ACE_Event_Handler 
{
public:
    /**
     * @brief Construct a new Command_Handler object
     * 
     * @param reactor the reactor that manage this Command_Handler
     */
    Command_Handler (ACE_Reactor *reactor);

    /**
     * @brief Register READ event on reactor and send welcome to client
     * 
     * @return int , 0 for success and -1 for failure
     */
    virtual int init ();

    /**
     * @brief The handler for input event, analyze received ftp commands then
     *        call handle_command ()
     * 
     * @return int , 0 for success or not fatal error, 
     *              -1 for fatal error then trigger handle_close
     */
    virtual int handle_input (ACE_HANDLE = ACE_INVALID_HANDLE);

    /**
     * @brief Close command connection
     * 
     * @return int , return value doesn't matter, it will be ignored by reactor
     */
    virtual int handle_close (ACE_HANDLE = ACE_INVALID_HANDLE,
                            ACE_Reactor_Mask = 0);

    /**
     * @brief The handler for timeout event, when last command is over [max_client_timeout_]
     *        ago, close this command connection and data connection.
     * 
     * @param now current time
     * @param act Asynchronous Completion Token
     * @return int , 0 for success
     */
    virtual int handle_timeout (const ACE_Time_Value &now, const void *act);
    /**
     * @brief The handler for ftp commands
     * 
     * @return int , same as each command's return value, 
     *               enum Data_Consequences show the meaning of command's return value,
     *               OK (0) means everything is OK,
     *               COMMAND_CON_CLOSE (-1) means something fatal and command connection need be closed
     *               DATA_CON_CLOSE (-2) means something wrong and data connetion need be closed
     *               CONTINUE (-3) means something wrong but the server can continue with nothing to do
     */
    virtual int handle_command ();

    /**
     * @brief The handler for ftp USER command, set the username
     * 
     * @return int , see the comment of handle_command ()
     */
    virtual int handle_user ();

    /**
     * @brief The handler for ftp PASS command, login with the received password
     * 
     * @return int , see the comment of handle_command ()
     */
    virtual int handle_pass ();

    /**
     * @brief The handler for ftp PWD command, send user's working directory
     * 
     * @return int , see the comment of handle_command ()
     */
    virtual int handle_pwd ();

    /**
     * @brief The handler for ftp CWD command,
     *        change user's working directory to received directory path
     * 
     * @return int , see the comment of handle_command ()
     */
    virtual int handle_cwd ();

    /**
     * @brief The handler for ftp CDUP command,
     *        change user's working directory to its parent directory
     * 
     * @return int , see the comment of handle_command ()
     */
    virtual int handle_cdup ();

    /**
     * @brief The handler for ftp QUIT command, call handle_close ()
     * 
     * @return int , 0 for success
     */
    virtual int handle_quit ();

    /**
     * @brief The handler for ftp PORT command,
     *        instantiate active data_handler with received address
     * 
     * @return int , see the comment of handle_command ()
     */
    virtual int handle_port ();

    /**
     * @brief The handler for PASV command,
     *        instantiate passive data_handler, use ACE_SOCK_Acceptor to 
     *        open a random port and send local address to client
     * 
     * @return int , see the comment of handle_command ()
     */
    virtual int handle_pasv ();

    /**
     * @brief The handler for TYPE command,
     *        change data type for transfer, RFC959 provides three types:
     *        ASCII, EBCDIC and IMAGE, but this server only support IMAGE.
     * 
     * @return int , see the comment of handle_command ()
     */
    virtual int handle_type ();

    /**
     * @brief The handler for LIST command, 
     *        reply directory/file information by data connection. If the pathname 
     *        specifies a directory, the server should transfer a list of files. 
     *        If the pathname specifies file then the server should send current 
     *        information of the file. A null argument implies the user's current 
     *        working directory.
     * 
     * @return int , see the comment of handle_command ()
     */
    virtual int handle_list ();

    /**
     * @brief The handler for RETR command,
     *        establish data connection and send the desired file to the client.
     *        When transfer is over, data connection will be closed immediately.
     * 
     * @return int , see the comment of handle_command ()
     */
    virtual int handle_retr ();

    /**
     * @brief The handler for STOR command,
     *        establish data connection and receive the desired file from client.
     *        If the file has existed on server, it will be overwritten from the beginning. 
     *        When transfer is over, data connection will be closed immediately.
     * 
     * @return int , see the comment of handle_command ()
     */
    virtual int handle_stor ();

    /**
     * @brief The handler for REFR command,
     *        check and record the file that client wants to rename.
     * 
     * @return int , see the comment of handle_command ()
     */
    virtual int handle_rnfr ();

    /**
     * @brief The handler for RETO command,
     *        rename the recorded file from REFR command to a new name.
     * 
     * @return int , see the comment of handle_command ()
     */
    virtual int handle_rnto ();

    /**
     * @brief The handler for RMD command,
     *        remove a specified directory and files in it.
     * 
     * @return int , see the comment of handle_command ()
     */
    virtual int handle_rmd ();

    /**
     * @brief The handler for DELE command, delete a specified file.
     * 
     * @return int , see the comment of handle_command ()
     */
    virtual int handle_dele ();

    /**
     * @brief The handler for mkd command, make a new directory.
     * 
     * @return int , see the comment of handle_command ()
     */
    virtual int handle_mkd ();

    /**
     * @brief Get the command link object
     * 
     * @return ACE_SOCK_Stream& 
     */
    ACE_SOCK_Stream &get_command_link () { return command_link_; }

    /**
     * @brief Destroy the Command_Handler object
     * 
     */
    ~Command_Handler ();

private:
    ACE_SOCK_Stream command_link_;              // command connection with ftp client
    ACE_SOCK_Acceptor pasv_acceptor_;           // acceptor for passive mode
    char recv_buffer_[MAX_COMMAND_BUFFER_SIZE]; // buffer for received command
    User_Inf user_;                             // user information
    std::unique_ptr<Data_Handler> data_handler_;// data connection with tp client
    int data_type_;                             // ftp data type, only support IMAGE
    bool is_pasv_;                              // whether in passive mode 
    u_short pasv_port_;                         // port number for passive mode
    ACE_Time_Value time_of_last_command_;       // time of last valid command
    const ACE_Time_Value max_client_timeout_;   // max interval for two commands

    // command to function map
    static const std::unordered_map<std::string, int (Command_Handler::*) ()> commands_;

    /**
     * @brief send specified response to client, see detailed response in msg.h
     * 
     * @param msg specified response from msg.h
     * @return int , 0 for success, -1 for failure
     */
    int send_response (const std::string &msg);

    /**
     * @brief send specified response to client, see detailed response in msg.h
     * 
     * @param format C-style format string 
     * @param detail the padding content for format
     * @return int , 0 for success, -1 for failure
     */
    int send_response (const char *format, const char *detail);

    /**
     * @brief establish passive or active data connection
     * 
     * @return int , 0 for success, -1 for failure
     */
    int make_data_connection ();

    /**
     * @brief change param from relative path to absolute path, if param 
     *        is absolute path, do nothing.
     * 
     * @param str path
     */
    inline void relative_to_absolute (std::string &str);
};

#endif