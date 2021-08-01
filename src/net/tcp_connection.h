#ifndef _TCP_CONNECTION_H_
#define _TCP_CONNECTION_H_

#include <atomic>
#include <mutex>
#include "task_scheduler.h"
#include "buffer_reader.h"
#include "buffer_writer.h"
#include "vlog.h"
#include "channel.h"
#include "socket_util.h"

class TcpConnection : public std::enable_shared_from_this<TcpConnection>
{
public:
	using Ptr                = std::shared_ptr<TcpConnection>;
	using DisconnectCallback = std::function<void(std::shared_ptr<TcpConnection> conn)> ;
	using CloseCallback      = std::function<void(std::shared_ptr<TcpConnection> conn)>;
	using ReadCallback       = std::function<bool(std::shared_ptr<TcpConnection> conn, BufferReader &buffer)>;

	TcpConnection(TaskScheduler *task_scheduler, SOCKET sockfd);

	TaskScheduler *get_task_scheduler() const;
	void set_read_callback(const ReadCallback &cb);
	void set_close_callback(const CloseCallback &cb);
	void send(std::shared_ptr<char> data, uint32_t size);
	void send(const char *data, uint32_t size);
	void disconnect();
	bool is_closed() const;
	SOCKET get_socket() const;
	uint16_t get_port() const;
	std::string get_ip() const;

	virtual ~TcpConnection();

protected:
	friend class TcpServer;

	virtual void handle_read();
	virtual void handle_write();
	virtual void handle_close();
	virtual void handle_error();	

	void set_disconnect_callback(const DisconnectCallback &cb);

	TaskScheduler                *m_task_scheduler;
	std::unique_ptr<BufferReader> m_read_buffer;
	std::unique_ptr<BufferWriter> m_write_buffer;
	std::atomic_bool              m_is_closed;

private:
	void close();

	std::shared_ptr<Channel> m_channel;
	std::mutex               m_mutex;
	DisconnectCallback       m_disconnect_cb;
	CloseCallback            m_close_cb;
	ReadCallback             m_read_cb;
};

#endif  // _TCP_CONNECTION_H_
