#include "event_loop.h"

#if defined(WIN32) || defined(_WIN32) 
#include<windows.h>
#endif

#if defined(WIN32) || defined(_WIN32) 
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib,"Iphlpapi.lib")
#endif 

EventLoop::EventLoop(uint32_t num_threads) : m_index(1)
{
	m_num_threads = 1;
	if (num_threads > 0) {
		m_num_threads = num_threads;
	}

	this->loop();
}

EventLoop::~EventLoop()
{
	this->quit();
}

std::shared_ptr<TaskScheduler> EventLoop::get_task_scheduler()
{
	std::lock_guard<std::mutex> locker(m_mutex);
	if (m_task_schedulers.size() == 1) {
		return m_task_schedulers.at(0);
	}
	else {
		auto task_scheduler = m_task_schedulers.at(m_index);
		m_index++;
		if (m_index >= m_task_schedulers.size()) {
			m_index = 1;
		}		
		return task_scheduler;
	}

	return nullptr;
}

void EventLoop::loop()
{
	std::lock_guard<std::mutex> locker(m_mutex);

	if (!m_task_schedulers.empty()) {
		return ;
	}

	for (uint32_t n = 0; n < m_num_threads; n++) 
	{
#if defined(__linux) || defined(__linux__) 
		std::shared_ptr<TaskScheduler> task_scheduler_ptr(new EpollTaskScheduler(n));
#elif defined(WIN32) || defined(_WIN32) 
		std::shared_ptr<TaskScheduler> task_scheduler_ptr(new SelectTaskScheduler(n));
#endif
		m_task_schedulers.push_back(task_scheduler_ptr);
		std::shared_ptr<std::thread> thread(new std::thread(&TaskScheduler::start, task_scheduler_ptr.get()));
		thread->native_handle();
		m_threads.push_back(thread);
	}

	const int priority = TASK_SCHEDULER_PRIORITY_REALTIME;

	for (auto iter : m_threads)  {

#if defined(__linux) || defined(__linux__) 

#elif defined(WIN32) || defined(_WIN32) 
		switch (priority) {
			
		case TASK_SCHEDULER_PRIORITY_LOW:
			SetThreadPriority(iter->native_handle(), THREAD_PRIORITY_BELOW_NORMAL);
			break;
		case TASK_SCHEDULER_PRIORITY_NORMAL:
			SetThreadPriority(iter->native_handle(), THREAD_PRIORITY_NORMAL);
			break;
		case TASK_SCHEDULER_PRIORITYO_HIGH:
			SetThreadPriority(iter->native_handle(), THREAD_PRIORITY_ABOVE_NORMAL);
			break;
		case TASK_SCHEDULER_PRIORITY_HIGHEST:
			SetThreadPriority(iter->native_handle(), THREAD_PRIORITY_HIGHEST);
			break;
		case TASK_SCHEDULER_PRIORITY_REALTIME:
			SetThreadPriority(iter->native_handle(), THREAD_PRIORITY_TIME_CRITICAL);
			break;
		}
#endif
	}
}

void EventLoop::quit()
{
	std::lock_guard<std::mutex> locker(m_mutex);

	for (auto iter : m_task_schedulers) {
		iter->stop();
	}

	for (auto iter : m_threads) {
		iter->join();
	}

	m_task_schedulers.clear();
	m_threads.clear();
}
	
void EventLoop::update_channel(ChannelPtr channel)
{
	std::lock_guard<std::mutex> locker(m_mutex);
	if (m_task_schedulers.size() > 0) {
		m_task_schedulers[0]->update_channel(channel);
	}	
}

void EventLoop::remove_channel(ChannelPtr &channel)
{
	std::lock_guard<std::mutex> locker(m_mutex);
	if (m_task_schedulers.size() > 0) {
		m_task_schedulers[0]->remove_channel(channel);
	}	
}

TimerId EventLoop::add_timer(TimerEvent timer_event, uint32_t msec)
{
	std::lock_guard<std::mutex> locker(m_mutex);
	if (m_task_schedulers.size() > 0) {
		return m_task_schedulers[0]->add_timer(timer_event, msec);
	}
	return 0;
}

void EventLoop::remove_timer(TimerId timer_id)
{
	std::lock_guard<std::mutex> locker(m_mutex);
	if (m_task_schedulers.size() > 0) {
		m_task_schedulers[0]->remove_timer(timer_id);
	}	
}

bool EventLoop::add_trigger_event(TriggerEvent callback)
{   
	std::lock_guard<std::mutex> locker(m_mutex);
	if (m_task_schedulers.size() > 0) {
		return m_task_schedulers[0]->add_trigger_event(callback);
	}
	return false;
}
