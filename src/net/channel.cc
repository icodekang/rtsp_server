#include "channel.h"

Channel::Channel(SOCKET sockfd): m_sockfd(sockfd)
{

}

void Channel::set_read_callback(const EventCallback &cb)
{ 
    m_read_callback = cb; 
}

void Channel::set_write_callback(const EventCallback& cb)
{ 
    m_write_callback = cb; 
}

void Channel::set_close_callback(const EventCallback& cb)
{ 
    m_close_callback = cb; 
}

void Channel::set_error_callback(const EventCallback& cb)
{ 
    m_error_callback = cb; 
}

SOCKET Channel::get_socket() const 
{ 
    return m_sockfd; 
}

int  Channel::get_events() const 
{ 
    return m_events; 
}

void Channel::set_events(int events) 
{ 
    m_events = events; 
}

void Channel::enable_reading() 
{ 
    m_events |= EVENT_IN; 
}

void Channel::enable_writing() 
{ 
    m_events |= EVENT_OUT; 
}

void Channel::disable_reading() 
{ 
    m_events &= ~EVENT_IN; 
}

void Channel::disable_writing() 
{ 
    m_events &= ~EVENT_OUT; 
}
    
bool Channel::is_none_event() const 
{ 
    return m_events == EVENT_NONE; 
}

bool Channel::is_writing() const 
{ 
    return (m_events & EVENT_OUT) != 0; 
}

bool Channel::is_reading() const 
{ 
    return (m_events & EVENT_IN) != 0; 
}

void Channel::handle_event(int events)
{	
    if (events & (EVENT_PRI | EVENT_IN)) {
        m_read_callback();
    }

    if (events & EVENT_OUT) {
        m_write_callback();
    }
    
    if (events & EVENT_HUP) {
        m_close_callback();
        return ;
    }

    if (events & (EVENT_ERR)) {
        m_error_callback();
    }
}