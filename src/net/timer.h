#ifndef _TIMER_H_
#define _TIMER_H_

#include <map>
#include <unordered_map>
#include <chrono>
#include <functional>
#include <cstdint>
#include <chrono>
#include <memory>
#include <mutex>
#include <thread>
#include "vlog.h"
   
typedef std::function<bool(void)> TimerEvent;
typedef uint32_t TimerId;

class Timer
{
public:
	Timer(const TimerEvent& event, uint32_t msec);

	static void sleep(uint32_t msec);
	void set_event_callback(const TimerEvent &event);
	void start(int64_t microseconds, bool repeat = false);
	void stop();

	~Timer() = default;	

private:
	friend class TimerQueue;

	void    set_next_timeout(int64_t time_point);
	int64_t get_next_timeout() const;

	bool       m_is_repeat      = false;
	TimerEvent m_event_callback = [] { return false; };
	uint32_t   m_interval       = 0;
	int64_t    m_next_timeout   = 0;
};

class TimerQueue
{
public:
	TimerId add_timer(const TimerEvent &event, uint32_t msec);
	void    remove_timer(TimerId timer_id);
	int64_t get_time_remaining();
	void    handle_timer_event();

private:
	int64_t get_time_now();

	std::mutex m_mutex;
	std::unordered_map<TimerId, std::shared_ptr<Timer>>           m_timers;
	std::map<std::pair<int64_t, TimerId>, std::shared_ptr<Timer>> m_events;
	uint32_t   m_last_timer_id = 0;
};

#endif // _TIMER_H_ 



