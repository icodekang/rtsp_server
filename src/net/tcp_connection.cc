#include "tcp_connection.h"
#include "socket_util.h"

TcpConnection::TcpConnection(TaskScheduler *task_scheduler, SOCKET sockfd)
	: m_task_scheduler(task_scheduler)
	, m_read_buffer(new BufferReader)
	, m_write_buffer(new BufferWriter(500))
	, m_channel(new Channel(sockfd))
{
	m_is_closed = false;

	m_channel->set_read_callback([this]()  { this->handle_read(); });
	m_channel->set_write_callback([this]() { this->handle_write(); });
	m_channel->set_close_callback([this]() { this->handle_close(); });
	m_channel->set_error_callback([this]() { this->handle_error(); });

	SocketUtil::set_non_block(sockfd);
	SocketUtil::set_send_buf_size(sockfd, 100 * 1024);
	SocketUtil::set_keep_alive(sockfd);

	m_channel->enable_reading();
	m_task_scheduler->update_channel(m_channel);
}

TaskScheduler* TcpConnection::get_task_scheduler() const 
{ 
	return m_task_scheduler; 
}

void TcpConnection::set_read_callback(const ReadCallback &cb)
{ 
	m_read_cb = cb; 
}

void TcpConnection::set_close_callback(const CloseCallback &cb)
{ 
	m_close_cb = cb; 
}

void TcpConnection::send(std::shared_ptr<char> data, uint32_t size)
{
	if (!m_is_closed) {
		m_mutex.lock();
		m_write_buffer->append(data, size);
		m_mutex.unlock();

		this->handle_write();
	}
}

void TcpConnection::send(const char *data, uint32_t size)
{
	if (!m_is_closed) {
		m_mutex.lock();
		m_write_buffer->append(data, size);
		m_mutex.unlock();

		this->handle_write();
	}
}

void TcpConnection::disconnect()
{
	std::lock_guard<std::mutex> lock(m_mutex);
	auto conn = shared_from_this();
	m_task_scheduler->add_trigger_event([conn]() {
		conn->close();
	});
}

bool TcpConnection::is_closed() const
{
	return m_is_closed;
}

SOCKET TcpConnection::get_socket() const
{
	return m_channel->get_socket();
}

uint16_t TcpConnection::get_port() const
{
	return SocketUtil::get_peer_port(m_channel->get_socket());
}

std::string TcpConnection::get_ip() const
{
	return SocketUtil::get_peer_ip(m_channel->get_socket());
}

void TcpConnection::handle_read()
{
	{
		std::lock_guard<std::mutex> lock(m_mutex);

		if (m_is_closed) {
			return;
		}
		
		int ret = m_read_buffer->read(m_channel->get_socket());
		if (ret <= 0) {
			this->close();
			return;
		}
	}

	if (m_read_cb) {
		bool ret = m_read_cb(shared_from_this(), *m_read_buffer);
		if (false == ret) {
			std::lock_guard<std::mutex> lock(m_mutex);
			this->close();
		}
	}
}

void TcpConnection::handle_write()
{
	if (m_is_closed) {
		return;
	}

	if (!m_mutex.try_lock()) {
		return;
	}

	int  ret = 0;
	bool empty = false;

	do {
		ret = m_write_buffer->send(m_channel->get_socket());
		if (ret < 0) {
			this->close();
			m_mutex.unlock();
			return;
		}
		empty = m_write_buffer->is_empty();
	} while (0);

	if (empty) {
		if (m_channel->is_writing()) {
			m_channel->disable_writing();
			m_task_scheduler->update_channel(m_channel);
		}
	} else if(!m_channel->is_writing()) {
		m_channel->enable_writing();
		m_task_scheduler->update_channel(m_channel);
	}

	m_mutex.unlock();
}

void TcpConnection::close()
{
	if (!m_is_closed) {
		m_is_closed = true;
		m_task_scheduler->remove_channel(m_channel);

		if (m_close_cb) {
			m_close_cb(shared_from_this());
		}			

		if (m_disconnect_cb) {
			m_disconnect_cb(shared_from_this());
		}	
	}
}

void TcpConnection::handle_close()
{
	std::lock_guard<std::mutex> lock(m_mutex);
	this->close();
}

void TcpConnection::handle_error()
{
	std::lock_guard<std::mutex> lock(m_mutex);
	this->close();
}

void TcpConnection::set_disconnect_callback(const DisconnectCallback &cb)
{ 
	m_disconnect_cb = cb;
}

TcpConnection::~TcpConnection()
{
	SOCKET fd = m_channel->get_socket();
	if (fd > 0) {
		SocketUtil::close(fd);
	}
}