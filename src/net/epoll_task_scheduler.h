#ifndef _EPOLL_TASK_SCHEDULER_H_
#define _EPOLL_TASK_SCHEDULER_H_

#include "vlog.h"
#include "task_scheduler.h"
#include <mutex>
#include <unordered_map>

class EpollTaskScheduler : public TaskScheduler
{
public:
	EpollTaskScheduler(int id = 0);

	void update_channel(ChannelPtr  channel);
	void remove_channel(ChannelPtr &channel);
	bool handle_event(int timeout);

	virtual ~EpollTaskScheduler() = default;

private:
	void update(int operation, ChannelPtr &channel);

	int m_epollfd = -1;
	std::mutex m_mutex;
	std::unordered_map<int, ChannelPtr> m_channels;
};

#endif // _EPOLL_TASK_SCHEDULER_H_
