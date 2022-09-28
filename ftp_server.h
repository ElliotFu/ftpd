#ifndef FTP_SERVER_H
#define FTP_SERVER_H

#include "ace/Event_Handler.h"
#include "ace/Reactor.h"
#include "ace/SOCK_Acceptor.h"

class Command_Handler;

class Ftp_Server : public ACE_Event_Handler 
{
public:
    /**
     * @brief Construct a new Ftp_Server
     * 
     * @param reactor the reactor that manage this Ftp_Server
     */
    Ftp_Server (ACE_Reactor *reactor): ACE_Event_Handler (reactor) {}

    /**
     * @brief Open ACE_SOCK_Acceptor with a specified address including port and
     *        register accept event to reactor
     * 
     * @param local_addr address for listening
     * @return int , 0 for success, -1 for failure
     */
    virtual int open (const ACE_INET_Addr &local_addr);

    /**
     * @brief The handler for accept event, establish command connection for new ftp client
     * 
     * @return int , 0 for success, 
     *               -1 for failure then trigger handle_close ()
     */
    virtual int handle_input (ACE_HANDLE = ACE_INVALID_HANDLE);

    /**
     * @brief Close the ftp server
     * 
     * @return int , return value doesn't matter, it will be ignored by reactor
     */
    virtual int handle_close (ACE_HANDLE = ACE_INVALID_HANDLE, 
                            ACE_Reactor_Mask = 0);

    /**
     * @brief Get the underlying handle/fd
     * 
     * @return ACE_HANDLE underlying handle/fd
     */
    virtual ACE_HANDLE get_handle () const { return acceptor_.get_handle (); }

    /**
     * @brief Get the ACE_SOCK_Acceptor
     * 
     * @return ACE_SOCK_Acceptor& 
     */
    ACE_SOCK_Acceptor &acceptor () { return acceptor_; }

    /**
     * @brief Destroy the Ftp_Server object
     */
    virtual ~Ftp_Server ();

    /**
     * @brief close command handler and collect garbage
     *        When program exits, the destruction order of Ftp_Server and Command_Handler is not always the same,
     *        so a static funciton makes sure this operation is always available.
     * 
     * @param com_p 
     */
    // static void close_command_handler (Command_Handler *com_p);

private:
    ACE_SOCK_Acceptor acceptor_;
};

#endif