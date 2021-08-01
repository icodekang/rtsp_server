#include "epoll_task_scheduler.h"

#if defined(__linux) || defined(__linux__) 
#include <sys/epoll.h>
#include <errno.h>
#endif

EpollTaskScheduler::EpollTaskScheduler(int id) : TaskScheduler(id)
{
#if defined(__linux) || defined(__linux__) 
    m_epollfd = epoll_create(1024);
 #endif
    this->update_channel(m_wakeup_channel);
}

void EpollTaskScheduler::update_channel(ChannelPtr channel)
{
	std::lock_guard<std::mutex> lock(m_mutex);
#if defined(__linux) || defined(__linux__) 
	int fd = channel->get_socket();
	if(m_channels.find(fd) != m_channels.end()) {
		if(channel->is_none_event()) {
			update(EPOLL_CTL_DEL, channel);
			m_channels.erase(fd);
		} else {
			update(EPOLL_CTL_MOD, channel);
		}
	} else {
		if(!channel->is_none_event()) {
			m_channels.emplace(fd, channel);
			update(EPOLL_CTL_ADD, channel);
		}	
	}	
#endif
}

void EpollTaskScheduler::update(int operation, ChannelPtr& channel)
{
#if defined(__linux) || defined(__linux__) 
	struct epoll_event event = {0};

	if(operation != EPOLL_CTL_DEL) {
		event.data.ptr = channel.get();
		event.events = channel->get_events();
	}

	if(::epoll_ctl(m_epollfd, operation, channel->get_socket(), &event) < 0) {

	}
#endif
}

void EpollTaskScheduler::remove_channel(ChannelPtr& channel)
{
    std::lock_guard<std::mutex> lock(m_mutex);
#if defined(__linux) || defined(__linux__) 
	int fd = channel->get_socket();

	if(m_channels.find(fd) != m_channels.end()) {
		update(EPOLL_CTL_DEL, channel);
		m_channels.erase(fd);
	}
#endif
}

bool EpollTaskScheduler::handle_event(int timeout)
{
#if defined(__linux) || defined(__linux__) 
	struct epoll_event events[512] = {0};
	int num_events = -1;

	num_events = epoll_wait(m_epollfd, events, 512, timeout);
	if (num_events < 0)  {
		if(errno != EINTR) {
			return false;
		}								
	}

	for(int n = 0; n < num_events; n++) {
		if(events[n].data.ptr) {        
			((Channel *)events[n].data.ptr)->handle_event(events[n].events);
		}
	}		
	return true;
#else
    return false;
#endif
}


