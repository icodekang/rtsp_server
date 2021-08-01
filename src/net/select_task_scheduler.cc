#include "select_task_scheduler.h"
#include "timer.h"
#include <cstring>
#include <forward_list>

#define SELECT_CTL_ADD	0
#define SELECT_CTL_MOD  1
#define SELECT_CTL_DEL	2

SelectTaskScheduler::SelectTaskScheduler(int id) : TaskScheduler(id)
{
	FD_ZERO(&m_fd_read_backup);
	FD_ZERO(&m_fd_write_backup);
	FD_ZERO(&m_fd_exp_backup);

	this->update_channel(m_wakeup_channel);
}

void SelectTaskScheduler::update_channel(ChannelPtr channel)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	SOCKET socket = channel->get_socket();

	if (m_channels.find(socket) != m_channels.end()) {
		if(channel->is_none_event()) {
			m_is_fd_read_reset  = true;
			m_is_fd_write_reset = true;
			m_is_fd_exp_reset   = true;
			m_channels.erase(socket);
		} else {
			//is_fd_read_reset_ = true;
			m_is_fd_write_reset = true;
		}
	} else {
		if(!channel->is_none_event()) {
			m_channels.emplace(socket, channel);
			m_is_fd_read_reset  = true;
			m_is_fd_write_reset = true;
			m_is_fd_exp_reset   = true;
		}	
	}	
}

void SelectTaskScheduler::remove_channel(ChannelPtr& channel)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	SOCKET fd = channel->get_socket();

	if (m_channels.find(fd) != m_channels.end()) {
		m_is_fd_read_reset  = true;
		m_is_fd_write_reset = true;
		m_is_fd_exp_reset   = true;
		m_channels.erase(fd);
	}
}

bool SelectTaskScheduler::handle_event(int timeout)
{	
	if (m_channels.empty()) {
		if (timeout <= 0) {
			timeout = 10;
		}
         
		Timer::sleep(timeout);
		return true;
	}

	fd_set fd_read;
	fd_set fd_write;
	fd_set fd_exp;
	FD_ZERO(&fd_read);
	FD_ZERO(&fd_write);
	FD_ZERO(&fd_exp);
	bool fd_read_reset  = false;
	bool fd_write_reset = false;
	bool fd_exp_reset   = false;

	if (m_is_fd_read_reset || m_is_fd_write_reset || m_is_fd_exp_reset) {
		if (m_is_fd_exp_reset) {
			m_maxfd = 0;
		}
          
		std::lock_guard<std::mutex> lock(m_mutex);
		for(auto iter : m_channels) {
			int events = iter.second->get_events();
			SOCKET fd = iter.second->get_socket();

			if (m_is_fd_read_reset && (events & EVENT_IN)) {
				FD_SET(fd, &fd_read);
			}

			if (m_is_fd_write_reset && (events&EVENT_OUT)) {
				FD_SET(fd, &fd_write);
			}

			if (m_is_fd_exp_reset) {
				FD_SET(fd, &fd_exp);
				if(fd > m_maxfd) {
					m_maxfd = fd;
				}
			}		
		}
        
		fd_read_reset  = m_is_fd_read_reset;
		fd_write_reset = m_is_fd_write_reset;
		fd_exp_reset   = m_is_fd_exp_reset;
		m_is_fd_read_reset  = false;
		m_is_fd_write_reset = false;
		m_is_fd_exp_reset   = false;
	}
	
	if (fd_read_reset) {
		FD_ZERO(&m_fd_read_backup);
		memcpy(&m_fd_read_backup, &fd_read, sizeof(fd_set)); 
	} else {
		memcpy(&fd_read, &m_fd_read_backup, sizeof(fd_set));
	}
       

	if (fd_write_reset) {
		FD_ZERO(&m_fd_write_backup);
		memcpy(&m_fd_write_backup, &fd_write, sizeof(fd_set));
	} else {
		memcpy(&fd_write, &m_fd_write_backup, sizeof(fd_set));
	}
     

	if (fd_exp_reset) {
		FD_ZERO(&m_fd_exp_backup);
		memcpy(&m_fd_exp_backup, &fd_exp, sizeof(fd_set));
	} else {
		memcpy(&fd_exp, &m_fd_exp_backup, sizeof(fd_set));
	}

	if (timeout <= 0) {
		timeout = 10;
	}

	struct timeval tv = { timeout/1000, timeout%1000*1000 };
	int ret = select((int)m_maxfd + 1, &fd_read, &fd_write, &fd_exp, &tv); 	
	if (ret < 0) {
#if defined(__linux) || defined(__linux__) 
	if(errno == EINTR) {
		return true;
	}					
#endif 
		return false;
	}

	std::forward_list<std::pair<ChannelPtr, int>> event_list;
	if (ret > 0) {
		std::lock_guard<std::mutex> lock(m_mutex);
		for(auto iter : m_channels) {
			int events = 0;
			SOCKET socket = iter.second->get_socket();

			if (FD_ISSET(socket, &fd_read)) {
				events |= EVENT_IN;
			}

			if (FD_ISSET(socket, &fd_write)) {
				events |= EVENT_OUT;
			}

			if (FD_ISSET(socket, &fd_exp)) {
				events |= (EVENT_HUP); // close
			}

			if (events != 0) {
				event_list.emplace_front(iter.second, events);
			}
		}
	}	

	for(auto& iter: event_list) {
		iter.first->handle_event(iter.second);
	}

	return true;
}



