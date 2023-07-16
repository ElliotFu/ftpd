# my_ftp_server

## Name
简易FTP服务器

## Description
使用ACE_TP_Reactor实现简易FTP服务器  
支持指令包括:  
user pass quit pwd cwd cdup port retr list type stor pasv rnfr rnto rmd dele mkd

## Installation
进入项目根目录  
mkdir build  
cd build  
cmake ..  
cmake --build .

## Usage
cd build  
./my_ftp_server PORT  
建议配合 filezilla 客户端使用

## Design

FTP服务器包括 Ftp_Server（服务器主程序类）、Command_Handler（FTP命令连接类）、Data_Handler（FTP数据链接类）和 User_Inf（用户信息类）  
