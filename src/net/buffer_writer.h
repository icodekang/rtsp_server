#ifndef _BUFFER_WRITER_H_
#define _BUFFER_WRITER_H_

#include <cstdint>
#include <memory>
#include <queue>
#include <string>
#include "vlog.h"
#include "socket.h"

void write_uint32BE(char *p, uint32_t value);
void write_uint32LE(char *p, uint32_t value);
void write_uint24BE(char *p, uint32_t value);
void write_uint24LE(char *p, uint32_t value);
void write_uint16BE(char *p, uint16_t value);
void write_uint16LE(char *p, uint16_t value);
	
class BufferWriter
{
public:
	BufferWriter(int capacity = m_MaxQueueLength);

	bool append(std::shared_ptr<char> data, uint32_t size, uint32_t index = 0);
	bool append(const char* data, uint32_t size, uint32_t index = 0);
	int  send(SOCKET sockfd, int timeout = 0);
	bool is_empty() const;
	bool is_full()  const;
	uint32_t size() const;

	~BufferWriter() = default;
	
private:
	typedef struct {
		std::shared_ptr<char> data;
		uint32_t size;
		uint32_t write_index;
	} Packet;

	std::queue<Packet> m_buffer;  		
	int m_max_queue_length = 0;
	 
	static const int m_MaxQueueLength = 10000;
};

#endif // _BUFFER_WRITER_H_

