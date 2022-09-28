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

设计中值得一提的细节有：
- 使用 ACE_TP_Reactor 实现多线程，一定程度提高并发能力，它实现了一种称为 Leader-Follower 的线程模型：一组线程中，有一个线程充当 Leader 角色，负责侦测事件，运行 select()，  
其它线程处在空闲待命状态。当事件到达，Leader 线程开始分派事件，在此之前，它选取线程池中的一个线程充当 Leader 角色，而自己在分派完事件后，加入空闲线程池，成为 Follower 角色，  
如此循环往复，不断推动 Reactor 运行；
- 为控制连接添加了超时检测，超过 30 分钟无命令发出即由服务器端主动断开控制连接；
- 因为运行在多线程环境下，必须为文件加读写锁；

## blog
