#include "ftp_server.h"

#include "ace/Log_Msg.h"
#include "ace/Reactor.h"
#include "ace/TP_Reactor.h"
#include "ace/Thread_Manager.h"

#include <iostream>
#include <memory>

static const size_t N_THREADS = 4;

/**
 * @brief Reactor event loop. Keep waiting for events.
 * 
 * @param arg a pointer to reactor
 * @return void* always be 0
 */
static void *event_loop (void *arg) 
{
    ACE_Reactor *reactor = static_cast<ACE_Reactor *> (arg);
    reactor->run_reactor_event_loop ();
    return 0;
}

static void *quit_controller (void *arg)
{
    ACE_Reactor *reactor = static_cast<ACE_Reactor *> (arg);

    while(1)
    {
        std::string input;
        std::getline (std::cin, input, '\n');
        if (input == "quit")
        {
            ACE_DEBUG ( (LM_DEBUG, "recv quit.\n"));
            reactor->end_reactor_event_loop ();
            break;
        }
    }
    return 0;
}

int main (int argc, char *argv[])
{
    if (argc == 1) {
        ACE_DEBUG ((LM_DEBUG, ACE_TEXT ("input port number")));
        return 0;
    }

    // choose TP_Reactor(Thread Pool Reactor)
    ACE_TP_Reactor tp_reactor;
    ACE_Reactor reactor (&tp_reactor);

    ACE_High_Res_Timer::global_scale_factor();
    reactor.timer_queue ()->gettimeofday (&ACE_High_Res_Timer::gettimeofday_hr);
    
    u_short port = ACE_OS::atoi (argv[1]);
    ACE_INET_Addr server_addr;
    int result = server_addr.set (port, (ACE_UINT32) INADDR_ANY);
    if(result == -1){
        ACE_DEBUG ((LM_DEBUG, ACE_TEXT ("addr set failed")));
        return 0;
    }

    std::unique_ptr<Ftp_Server> server = std::make_unique<Ftp_Server> (&reactor);
    if ( (server->open (server_addr)) == -1)
    {
        ACE_DEBUG ((LM_DEBUG, ACE_TEXT ("open ftp server failed.\n")));
        return 0;
    }

    ACE_Thread_Manager::instance ()->spawn_n (N_THREADS, event_loop, &reactor);
    ACE_Thread_Manager::instance ()->spawn (quit_controller, &reactor);

    return ACE_Thread_Manager::instance ()->wait ();
}