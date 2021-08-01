#ifndef _SELECT_TASK_SCHEDULER_H_
#define _SELECT_TASK_SCHEDULER_H_

#include "task_scheduler.h"
#include "vlog.h"
#include "socket.h"
#include <mutex>
#include <unordered_map>

#if defined(__linux) || defined(__linux__) 
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#endif

class SelectTaskScheduler : public TaskScheduler
{
public:
	SelectTaskScheduler(int id = 0);

	void update_channel(ChannelPtr  channel);
	void remove_channel(ChannelPtr &channel);
	bool handle_event(int timeout);

	virtual ~SelectTaskScheduler() = default;
	
private:
	fd_set m_fd_read_backup;
	fd_set m_fd_write_backup;
	fd_set m_fd_exp_backup;
	SOCKET m_maxfd = 0;

	bool m_is_fd_read_reset  = false;
	bool m_is_fd_write_reset = false;
	bool m_is_fd_exp_reset   = false;

	std::mutex m_mutex;
	std::unordered_map<SOCKET, ChannelPtr> m_channels;
};

#endif // _SELECT_TASK_SCHEDULER_H_
