#include "timer.h"
#include <iostream>

Timer::Timer(const TimerEvent &event, uint32_t msec) : m_event_callback(event) , m_interval(msec)
{
	if (m_interval == 0) {
		m_interval = 1;
	}
}

void Timer::sleep(uint32_t msec)
{ 
	std::this_thread::sleep_for(std::chrono::milliseconds(msec));
}

void Timer::set_event_callback(const TimerEvent &event)
{
	m_event_callback = event;
}

void Timer::start(int64_t microseconds, bool repeat)
{
	m_is_repeat = repeat;
	auto time_begin = std::chrono::high_resolution_clock::now();
	int64_t elapsed = 0;

	do {
		std::this_thread::sleep_for(std::chrono::microseconds(microseconds - elapsed));
		time_begin = std::chrono::high_resolution_clock::now();
		if (m_event_callback) {
			m_event_callback();
		}			
		elapsed = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - time_begin).count();
		if (elapsed < 0) {
			elapsed = 0;
		}
		
	} while (m_is_repeat);
}

void Timer::stop()
{
	m_is_repeat = false;
}	

void Timer::set_next_timeout(int64_t time_point)
{
	m_next_timeout = time_point + m_interval;
}

int64_t Timer::get_next_timeout() const
{
	return m_next_timeout;
}

/******************************TimerQueue*****************************/

TimerId TimerQueue::add_timer(const TimerEvent &event, uint32_t ms)
{    
	std::lock_guard<std::mutex> locker(m_mutex);

	int64_t timeout = get_time_now();
	TimerId timer_id = ++m_last_timer_id;

	auto timer = std::make_shared<Timer>(event, ms);	
	timer->set_next_timeout(timeout);
	m_timers.emplace(timer_id, timer);
	m_events.emplace(std::pair<int64_t, TimerId>(timeout + ms, timer_id), std::move(timer));

	return timer_id;
}

void TimerQueue::remove_timer(TimerId timer_id)
{
	std::lock_guard<std::mutex> locker(m_mutex);

	auto iter = m_timers.find(timer_id);
	if (iter != m_timers.end()) {
		int64_t timeout = iter->second->get_next_timeout();
		m_events.erase(std::pair<int64_t, TimerId>(timeout, timer_id));
		m_timers.erase(timer_id);
	}
}

int64_t TimerQueue::get_time_now()
{	
	auto time_point = std::chrono::steady_clock::now();	
	return std::chrono::duration_cast<std::chrono::milliseconds>(time_point.time_since_epoch()).count();
}

int64_t TimerQueue::get_time_remaining()
{	
	std::lock_guard<std::mutex> locker(m_mutex);

	if (m_timers.empty()) {
		return -1;
	}

	int64_t msec = m_events.begin()->first.first - get_time_now();
	if (msec < 0) {
		msec = 0;
	}

	return msec;
}

void TimerQueue::handle_timer_event()
{
	if (!m_timers.empty()) {
		std::lock_guard<std::mutex> locker(m_mutex);
		int64_t time_point = get_time_now();
		while (!m_timers.empty() && m_events.begin()->first.first <= time_point) {	
			TimerId timer_id = m_events.begin()->first.second;
			bool flag = m_events.begin()->second->m_event_callback();
			if (flag == true) {
				m_events.begin()->second->set_next_timeout(time_point);
				auto timer_ptr = std::move(m_events.begin()->second);
				m_events.erase(m_events.begin());
				m_events.emplace(std::pair<int64_t, TimerId>(timer_ptr->get_next_timeout(), timer_id), timer_ptr);
			} else {		
				m_events.erase(m_events.begin());
				m_timers.erase(timer_id);				
			}
		}	
	}
}

