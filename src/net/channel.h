#ifndef _CHANNEL_H_
#define _CHANNEL_H_

#include <functional>
#include <memory>
#include "vlog.h"
#include "socket.h"
  
enum EventType
{
	EVENT_NONE   = 0,
	EVENT_IN     = 1,
	EVENT_PRI    = 2,		
	EVENT_OUT    = 4,
	EVENT_ERR    = 8,
	EVENT_HUP    = 16,
	EVENT_RDHUP  = 8192
};

class Channel 
{
public:
	typedef std::function<void()> EventCallback;
    
	Channel() = delete;
	Channel(SOCKET sockfd);

	void set_read_callback (const EventCallback &cb);
	void set_write_callback(const EventCallback &cb);
	void set_close_callback(const EventCallback &cb);
	void set_error_callback(const EventCallback &cb);
	SOCKET get_socket() const;
	int  get_events() const;
	void set_events(int events);
	void enable_reading();
	void enable_writing();
	void disable_reading();
	void disable_writing();
	bool is_none_event() const;
	bool is_writing() const;
	bool is_reading() const;
	void handle_event(int events);

	virtual ~Channel() = default;

private:
	EventCallback m_read_callback  = []{};
	EventCallback m_write_callback = []{};
	EventCallback m_close_callback = []{};
	EventCallback m_error_callback = []{};
    
	SOCKET m_sockfd = 0;
	int    m_events = 0;    
};

typedef std::shared_ptr<Channel> ChannelPtr;

#endif  // _CHANNEL_H_

