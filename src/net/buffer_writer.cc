#include "buffer_writer.h"
#include "socket.h"
#include "socket_util.h"

void write_uint32BE(char* p, uint32_t value)
{
	p[0] = value >> 24;
	p[1] = value >> 16;
	p[2] = value >> 8;
	p[3] = value & 0xff;
}

void write_uint32LE(char* p, uint32_t value)
{
	p[0] = value & 0xff;
	p[1] = value >> 8;
	p[2] = value >> 16;
	p[3] = value >> 24;
}

void write_uint24BE(char* p, uint32_t value)
{
	p[0] = value >> 16;
	p[1] = value >> 8;
	p[2] = value & 0xff;
}

void write_uint24LE(char* p, uint32_t value)
{
	p[0] = value & 0xff;
	p[1] = value >> 8;
	p[2] = value >> 16;
}

void write_uint16BE(char* p, uint16_t value)
{
	p[0] = value >> 8;
	p[1] = value & 0xff;
}

void write_uint16LE(char* p, uint16_t value)
{
	p[0] = value & 0xff;
	p[1] = value >> 8;
}

BufferWriter::BufferWriter(int capacity) 
	: m_max_queue_length(capacity)
{
	
}	

bool BufferWriter::append(std::shared_ptr<char> data, uint32_t size, uint32_t index)
{
	if (size <= index) {
		return false;
	}
   
	if ((int)m_buffer.size() >= m_max_queue_length) {
		return false;
	}
     
	Packet pkt = { data, size, index };
	m_buffer.emplace(std::move(pkt));

	return true;
}

bool BufferWriter::append(const char* data, uint32_t size, uint32_t index)
{
	if (size <= index) {
		return false;
	}
     
	if ((int)m_buffer.size() >= m_max_queue_length) {
		return false;
	}
     
	Packet pkt;
	pkt.data.reset(new char[size+512]);
	memcpy(pkt.data.get(), data, size);
	pkt.size = size;
	pkt.write_index = index;
	m_buffer.emplace(std::move(pkt));

	return true;
}

int BufferWriter::send(SOCKET sockfd, int timeout)
{		
	if (timeout > 0) {
		SocketUtil::set_block(sockfd, timeout); 
	}
      
	int ret = 0;
	int count = 1;

	do {
		if (m_buffer.empty()) {
			return 0;
		}
		
		count -= 1;
		Packet &pkt = m_buffer.front();
		ret = ::send(sockfd, pkt.data.get() + pkt.write_index, pkt.size - pkt.write_index, 0);
		if (ret > 0) {
			pkt.write_index += ret;
			if (pkt.size == pkt.write_index) {
				count += 1;
				m_buffer.pop();
			}
		}
		else if (ret < 0) {
#if defined(__linux) || defined(__linux__)
		if (errno == EINTR || errno == EAGAIN) 
#elif defined(WIN32) || defined(_WIN32)
			int error = WSAGetLastError();
			if (error == WSAEWOULDBLOCK || error == WSAEINPROGRESS || error == 0)
#endif
			{
				ret = 0;
			}
		}
	} while (count > 0);

	if (timeout > 0) {
		SocketUtil::set_non_block(sockfd);
	}
    
	return ret;
}

bool BufferWriter::is_empty() const 
{ 
	return m_buffer.empty(); 
}

bool BufferWriter::is_full() const 
{ 
	return ((int)m_buffer.size() >= m_max_queue_length ? true : false); 
}

uint32_t BufferWriter::size() const 
{ 
	return (uint32_t)m_buffer.size();
}