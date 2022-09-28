#include "ftp_server.h"
#include "command_handler.h"

#include "ace/Log_Msg.h"

#include "user_inf.h"

int Ftp_Server::open (const ACE_INET_Addr &local_addr) 
{
    if (acceptor_.open (local_addr) == -1)
        return -1;
    if ( User_Inf::read_passwords () == -1)
        return -1;
    return reactor ()->register_handler (this, ACE_Event_Handler::ACCEPT_MASK);
}

int Ftp_Server::handle_input (ACE_HANDLE) 
{
    Command_Handler *command_handler = 0;
    ACE_NEW_RETURN (command_handler,
                    Command_Handler (reactor ()),
                    -1);

    if (acceptor_.accept (command_handler->get_command_link ()) == -1) {
        ACE_DEBUG ((LM_DEBUG, ACE_TEXT ("accept failed\n")));
        command_handler->handle_close ();
        return -1;
    } else if (command_handler->init () == -1) {
        ACE_DEBUG ((LM_DEBUG, ACE_TEXT ("command_handler init failed\n")));
        command_handler->handle_close();
        return -1;
    }
    return 0;
}

int Ftp_Server::handle_close (ACE_HANDLE, ACE_Reactor_Mask)
{
    //reactor ()->end_reactor_event_loop ();
    return 0;
}

// void Ftp_Server::close_command_handler (Command_Handler *com_p)
// {
//     delete com_p;
// }

Ftp_Server::~Ftp_Server ()
{
    reactor ()->remove_handler (acceptor_.get_handle (),
                                ACE_Event_Handler::ACCEPT_MASK |
                                ACE_Event_Handler::DONT_CALL);
    acceptor_.close();
}