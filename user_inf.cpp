#include "user_inf.h"

#include "ace/Log_Msg.h"

#include <fstream>
#include <iomanip>
#include <openssl/sha.h>
#include <sstream>

const std::string User_Inf::passwords_file_path_ = "./users.txt";

std::unordered_map<std::string, std::string> User_Inf::passwords_;

int User_Inf::read_passwords ()
{
    if (!passwords_.empty ())
        return 0;
    
    std::ifstream file_stream;
    file_stream.open (passwords_file_path_);
    if (!file_stream.is_open ())
    {
        ACE_DEBUG ( (LM_DEBUG, "user information init failed.\n"));
        return -1;
    }
    
    std::string str_line;
    while(std::getline (file_stream, str_line))
    {
        if (str_line.empty ())
            continue;
        int pos = str_line.find (' ', 0);
        std::string user = str_line.substr (0, pos);
        std::string password = str_line.substr (pos+1);
        ACE_DEBUG ( (LM_DEBUG, "%s\n", user.c_str ()));
        ACE_DEBUG ( (LM_DEBUG, "%s\n", password.c_str ()));
        passwords_[user] = password;
    }
    return 0;
}

int User_Inf::check_username (const std::string &username) 
{   
    auto ite = passwords_.find (username);
    if (ite == passwords_.end ())
        return -1;
    else
    {
        username_ = username;
        return 0;
    }
}

int User_Inf::check_password (const std::string &password) 
{
    if (username_.length () == 0)
        return -1;
    
    std::string pass_with_salt = password + SALT;
    std::string pass_sha256;
    sha256 (pass_with_salt, pass_sha256);
    
    if (pass_sha256 == passwords_[username_])
    {
        is_logged_in_ = true;
        return 0;
    }
    else 
        return -1;
}

void User_Inf::sha256 (const std::string &src, std::string &des)
{
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, src.c_str(), src.size());
    SHA256_Final(hash, &sha256);
    std::stringstream ss;
    for(int i = 0; i < SHA256_DIGEST_LENGTH; i++)
    {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    des = ss.str();
}