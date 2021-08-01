#ifndef _TASK_SCHEDULER_H_
#define _TASK_SCHEDULER_H_

#include "vlog.h"
#include "channel.h"
#include "pipe.h"
#include "timer.h"
#include "ring_buffer.h"

typedef std::function<void(void)> TriggerEvent;

class TaskScheduler 
{
public:
	TaskScheduler(int id = 1);

	void start();
	void stop();
	TimerId add_timer(TimerEvent timer_event, uint32_t msec);
	void remove_timer(TimerId timerId);
	bool add_trigger_event(TriggerEvent callback);
	virtual void update_channel(ChannelPtr channel);
	virtual void remove_channel(ChannelPtr& channel);
	virtual bool handle_event(int timeout);
	int get_id() const;

	virtual ~TaskScheduler() = default;

protected:
	void wake();
	void handle_trigger_event();

	int m_id = 0;
	std::atomic_bool                          m_is_shutdown;
	std::unique_ptr<Pipe>                     m_wakeup_pipe;
	std::shared_ptr<Channel>                  m_wakeup_channel;
	std::unique_ptr<RingBuffer<TriggerEvent>> m_trigger_events;

	std::mutex m_mutex;
	TimerQueue m_timer_queue;

	static const char m_TriggetEvent = 1;
	static const char m_TimerEvent   = 2;
	static const int  m_MaxTriggetEvents = 50000;
};

#endif  // _TASK_SCHEDULER_H_

