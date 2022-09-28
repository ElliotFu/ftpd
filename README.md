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
<img src="https://gitlab.scutech.com/fuzh/my_ftp_server/-/raw/develop/pictures/classes.png" width="1000px">  

FTP服务器包括 Ftp_Server（服务器主程序类）、Command_Handler（FTP命令连接类）、Data_Handler（FTP数据链接类）和 User_Inf（用户信息类）  

<img src="https://gitlab.scutech.com/fuzh/my_ftp_server/-/raw/develop/pictures/timeline.png" width="1000px">  

- 服务器时序图如上，程序使用 ACE_TP_Reactor 作为反应器的实现方式，程序启动时创建一个 FTP_Server 实例，向 ACE_TP_Reactor 注册 accept 事件；  
- 当 accept 事件发生，触发 handle_input() 方法，建立 FTP 命令连接，产生一个 Command_Handler 实例并初始化，初始化过程会向 Reactor 注册 input 事件  
和 timeout 事件，随后等待客户命令；  
- 当接收到 stor, retr, list等需要使用数据连接的命令时，建立 FTP 数据连接，产生一个 Data_Handler 实例并建立数据连接，然后执行对应数据传输操作，  
数据传输结束后立即关闭数据连接；
- 当程序收到标准输入内的 quit 指令后，开始关闭服务器，ACE_TP_Reactor 会不固定顺序地调用所有已注册的事件处理器的 handle_close() 方法，试图关闭处理器；  
  - 原设计中会在 handle_close() 函数中使用 "delete this" 杀死自己回收空间，导师建议避免 "delete this" 代码，希望由创建者管理其创建实例的生命周期；
  - 后修改为使用智能指针管理程序中唯一的 Ftp_Server 实例，Command_Handler 在 handle_close() 中调用 Ftp_Server 的 close_command_handler() 方法；
  - 但是这样只做到了字面上的避免 "delete this" 代码，本质并未改变，存在设计缺陷；

设计中值得一提的细节有：
- 使用 ACE_TP_Reactor 实现多线程，一定程度提高并发能力，它实现了一种称为 Leader-Follower 的线程模型：一组线程中，有一个线程充当 Leader 角色，负责侦测事件，运行 select()，  
其它线程处在空闲待命状态。当事件到达，Leader 线程开始分派事件，在此之前，它选取线程池中的一个线程充当 Leader 角色，而自己在分派完事件后，加入空闲线程池，成为 Follower 角色，  
如此循环往复，不断推动 Reactor 运行；
- 为控制连接添加了超时检测，超过 30 分钟无命令发出即由服务器端主动断开控制连接；
- 因为运行在多线程环境下，必须为文件加读写锁；
