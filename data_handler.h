#ifndef DATA_HANDLER_H
#define DATA_HANDLER_H

#include "ace/SOCK_Acceptor.h"
#include "ace/SOCK_Connector.h"
#include "ace/SOCK_Stream.h"
#include "ace/FILE_IO.h"

#include <string>

#define MAX_BUFFER_SIZE 2048

enum Data_Modes 
{
    STREAM = 1,
    BLOCK = 2,
    COMPRESSED = 3,
};

enum Data_Types 
{
    ASCII = 1,
    EBCDIC = 2,
    IMAGE = 3,
};

class Data_Handler{
public:
    /**
     * @brief Construct a new Data_Handler object, used by passive mode.
     * 
     * @param type data type for transfer
     */
    Data_Handler (const int &type) : 
        mode_ (Data_Modes::STREAM),
        type_ (type),
        is_lock_ (false),
        wfile_try_connection_ (nullptr)
    {}
    
    /**
     * @brief Construct a new Data_Handler object, used by active mode.
     * 
     * @param ip_addr client's address for data connection
     * @param port client's port for data connection
     * @param type data type for transfer
     */
    Data_Handler (const std::string &ip_addr, const int &port, const int &type);
    
    /**
     * @brief Establish active data connection
     * 
     * @return int , 0 for success, -1 for failure
     */
    virtual int data_link_init ();

    /**
     * @brief Establish local file connection
     * 
     * @param is_output whether this file connetion is for sending this file to client
     * @return int , 0 for success, -1 for failure
     */
    virtual int file_link_init (bool is_output);

    /**
     * @brief Send the linked file to client
     * 
     * @return int , 0 for success, -1 for failure
     */
    virtual int send_file ();

    /**
     * @brief Send the information of a list of files in specified directory to client
     * 
     * @param dir_path desired directory's path
     * @return int , 0 for success, -1 for failure
     */
    virtual int list_dir (const std::string &dir_path);

    /**
     * @brief Send specified file's information to client
     * 
     * @param file_path desired file's path
     * @return int , 0 for success, -1 for failure
     */
    virtual int list_file (const std::string &file_path);

    /**
     * @brief Receive file from client and store on server
     * 
     * @return int , 0 for success, -1 for failure
     */
    virtual int recv_file ();

    /**
     * @brief Get the data link object
     * 
     * @return ACE_SOCK_Stream& 
     */
    ACE_SOCK_Stream &get_data_link () { return data_link_; }

    /**
     * @brief Get the beginning of data buffer
     * 
     * @return char* 
     */
    char *get_data_buffer () { return data_buffer_; }

    /**
     * @brief Set the file path
     * 
     * @param file_path desired file's path
     */
    void set_file_path (const std::string &file_path) { file_path_ = file_path; }

    virtual ~Data_Handler();

private:
    ACE_SOCK_Stream data_link_;         // data connection with ftp client
    ACE_INET_Addr client_addr_;         // client address
    ACE_FILE_IO file_link_;             // file connection
    char data_buffer_[MAX_BUFFER_SIZE]; // buffer for send or receive
    int mode_;                          // transfer mode, only support STREAM
    int type_;                          // transfer data type, only support IMAGE
    std::string file_path_;             // file path for file connection
    bool is_lock_;                      // whether lock a file
    FILE *wfile_try_connection_;        // When client wants to upload a file,
                                        // use it to make a write lock

    /**
     * @brief Change a file's type and mode to readable string, stored in buf
     * 
     * @param mode file type and mode from "struct stat"
     * @param buf buffer to store result
     */
    void mode_to_letters (mode_t mode, char *buf);

    /**
     * @brief Find out uname according to uid, stored in buf
     * 
     * @param uid user id
     * @param buf buffer to store result
     */
    void uid_to_name (uid_t uid, char *buf);

    /**
     * @brief Find out group name according to gid, stored in buf
     * 
     * @param gid group id
     * @param buf buffer to store result
     */
    void gid_to_name (gid_t gid, char *buf);
};
#endif