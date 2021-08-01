#include "task_scheduler.h"
#if defined(__linux) || defined(__linux__) 
#include <signal.h>
#endif

TaskScheduler::TaskScheduler(int id)
	: m_id(id)
	, m_is_shutdown(false) 
	, m_wakeup_pipe(new Pipe())
	, m_trigger_events(new RingBuffer<TriggerEvent>(m_MaxTriggetEvents))
{
	static std::once_flag flag;
	std::call_once(flag, [] {
#if defined(WIN32) || defined(_WIN32) 
		WSADATA wsa_data;
		if (WSAStartup(MAKEWORD(2, 2), &wsa_data)) {
			WSACleanup();
		}
#endif
	});

	if (m_wakeup_pipe->create()) {
		m_wakeup_channel.reset(new Channel(m_wakeup_pipe->read()));
		m_wakeup_channel->enable_reading();
		m_wakeup_channel->set_read_callback([this]() { this->wake(); });		
	}        
}

void TaskScheduler::start()
{
#if defined(__linux) || defined(__linux__) 
	signal(SIGPIPE, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
	signal(SIGUSR1, SIG_IGN);
	signal(SIGTERM, SIG_IGN);
	signal(SIGKILL, SIG_IGN);
#endif     
	m_is_shutdown = false;
	while (!m_is_shutdown) {
		this->handle_trigger_event();
		this->m_timer_queue.handle_timer_event();
		int64_t timeout = this->m_timer_queue.get_time_remaining();
		this->handle_event((int)timeout);
	}
}

void TaskScheduler::stop()
{
	m_is_shutdown = true;
	char event    = m_TriggetEvent;
	m_wakeup_pipe->write(&event, 1);
}

TimerId TaskScheduler::add_timer(TimerEvent timer_event, uint32_t msec)
{
	TimerId id = m_timer_queue.add_timer(timer_event, msec);
	return id;
}

void TaskScheduler::remove_timer(TimerId timer_id)
{
	m_timer_queue.remove_timer(timer_id);
}

bool TaskScheduler::add_trigger_event(TriggerEvent callback)
{
	if (m_trigger_events->size() <m_MaxTriggetEvents) {
		std::lock_guard<std::mutex> lock(m_mutex);
		char event = m_TriggetEvent;
		m_trigger_events->push(std::move(callback));
		m_wakeup_pipe->write(&event, 1);
		return true;
	}

	return false;
}

void TaskScheduler::update_channel(ChannelPtr channel)
{
	return;
}

void TaskScheduler::remove_channel(ChannelPtr &channel)
{
	return;
}

bool TaskScheduler::handle_event(int timeout) 
{ 
	return false; 
}

int TaskScheduler::get_id() const 
{ 
	return m_id; 
}

void TaskScheduler::wake()
{
	char event[10] = { 0 };
	while (m_wakeup_pipe->read(event, 10) > 0);
}

void TaskScheduler::handle_trigger_event()
{
	do {
		TriggerEvent callback;
		if (m_trigger_events->pop(callback)) {
			callback();
		}
	} while (m_trigger_events->size() > 0);
}
