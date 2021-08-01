#ifndef _EVENT_LOOP_H_
#define _EVENT_LOOP_H_

#include <memory>
#include <atomic>
#include <unordered_map>
#include <functional>
#include <queue>
#include <thread>
#include <mutex>

#include "select_task_scheduler.h"
#include "epoll_task_scheduler.h"
#include "vlog.h"
#include "pipe.h"
#include "timer.h"
#include "ring_buffer.h"

#define TASK_SCHEDULER_PRIORITY_LOW       0
#define TASK_SCHEDULER_PRIORITY_NORMAL    1
#define TASK_SCHEDULER_PRIORITYO_HIGH     2 
#define TASK_SCHEDULER_PRIORITY_HIGHEST   3
#define TASK_SCHEDULER_PRIORITY_REALTIME  4

class EventLoop 
{
public:
	EventLoop(const EventLoop&) = delete;
	EventLoop &operator = (const EventLoop&) = delete; 
	EventLoop(uint32_t num_threads =1);

	std::shared_ptr<TaskScheduler> get_task_scheduler();
	bool add_trigger_event(TriggerEvent callback);
	TimerId add_timer(TimerEvent timer_event, uint32_t msec);
	void remove_timer(TimerId timer_id);	
	void update_channel(ChannelPtr channel);
	void remove_channel(ChannelPtr &channel);
	void loop();
	void quit();

	virtual ~EventLoop();

private:
	std::mutex m_mutex;
	uint32_t   m_num_threads = 1;
	uint32_t   m_index       = 1;
	std::vector<std::shared_ptr<TaskScheduler>> m_task_schedulers;
	std::vector<std::shared_ptr<std::thread>>   m_threads;
};

#endif // _EVENT_LOOP_H_

