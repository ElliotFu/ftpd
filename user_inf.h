#ifndef USER_INF_H
#define USER_INF_H

#include <string>
#include <unordered_map>

#define SALT "scutech"

class User_Inf
{
public:
    /**
     * @brief Construct a new User_Inf object
     * 
     */
    User_Inf () : is_logged_in_ (false), current_dir_ ("/home/scutech") {}

    /**
     * @brief Check whether user has logged in.
     * 
     * @return true , user has logged in.
     * @return false , user hasn't logged in.
     */
    bool check_logged_in () { return is_logged_in_; }

    /**
     * @brief Check whether username is valid, if it's valid, set username.
     * 
     * @param username 
     * @return int , 0 for valid and set successfully,
     *               -1 for invalid
     */
    int check_username (const std::string &username);

    /**
     * @brief Check whether password is correct, if it's correct, log in.
     * 
     * @param password 
     * @return int , 0 for correct and log in successfully,
     *               -1 for incorrect
     */
    int check_password (const std::string &password);

    /**
     * @brief Get the current working directory
     * 
     * @return std::string current working directory
     */
    std::string get_cur_dir () { return current_dir_; }

    /**
     * @brief Get the username
     * 
     * @return std::string username
     */
    std::string get_user_name () { return username_; }

    /**
     * @brief Get the old file name, ftp RNFR command sets it
     * 
     * @return std::string& old file name
     */
    std::string &get_old_file_name () { return old_file_name_; }

    /**
     * @brief Set the current working directory 
     * 
     * @param current_dir current working directory 
     */
    void set_cur_dir (const std::string &current_dir) { current_dir_ = current_dir; }

    /**
     * @brief Set the old file name
     * 
     * @param old_file_name old file name
     */
    void set_old_file_name (const std::string &old_file_name) { old_file_name_ = old_file_name; }

    /**
     * @brief do sha256 with str, then turn 32 bytes hashcode into readable readable hex string
     *        result stored in des
     * 
     * @param str 
     * @param des 
     */
    void sha256(const std::string &str, std::string &des);

    /**
     * @brief Read valid username and password to passwords_
     * 
     * @return int , 0 for success, -1 for failure
     */
    static int read_passwords ();
private:
    bool is_logged_in_;         // whether the ftp client has logged in
    std::string username_;      // username
    std::string current_dir_;   // current working directory
    std::string old_file_name_; // Before renaming a file, store the origin name of the file

    // valid username and passwords, coming from a file
    static std::unordered_map<std::string, std::string> passwords_;
    static const std::string passwords_file_path_;  // path of the file storing username and passwords
};

#endif