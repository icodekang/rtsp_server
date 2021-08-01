#include "buffer_reader.h"
#include "socket.h"
#include <cstring>
 
uint32_t read_uint32BE(char* data)
{
	uint8_t *p = (uint8_t*)data;
	uint32_t value = (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
	return value;
}

uint32_t read_uint32LE(char* data)
{
	uint8_t *p = (uint8_t*)data;
	uint32_t value = (p[3] << 24) | (p[2] << 16) | (p[1] << 8) | p[0];
	return value;
}

uint32_t read_uint24BE(char* data)
{
	uint8_t *p = (uint8_t*)data;
	uint32_t value = (p[0] << 16) | (p[1] << 8) | p[2];
	return value;
}

uint32_t read_uint24LE(char* data)
{
	uint8_t *p = (uint8_t*)data;
	uint32_t value = (p[2] << 16) | (p[1] << 8) | p[0];
	return value;
}

uint16_t read_uint16BE(char* data)
{
	uint8_t *p = (uint8_t*)data;
	uint16_t value = (p[0] << 8) | p[1];
	return value; 
}

uint16_t read_uint16LE(char* data)
{
	uint8_t *p = (uint8_t*)data;
	uint16_t value = (p[1] << 8) | p[0];
	return value; 
}

const char BufferReader::m_CRLF[] = "\r\n";

BufferReader::BufferReader(uint32_t initial_size)
{
	m_buffer.resize(initial_size);
}	

uint32_t BufferReader::readable_bytes() const
{
	return (uint32_t)(m_writer_index - m_reader_index); 
}

uint32_t BufferReader::writable_bytes() const
{ 
	return (uint32_t)(m_buffer.size() - m_writer_index);
}

char* BufferReader::peek() 
{ 
	return begin() + m_reader_index; 
}

const char *BufferReader::peek() const
{
	return begin() + m_reader_index;
}

const char *BufferReader::find_first_crlf() const 
{    
	const char* crlf = std::search(peek(), begin_write(), m_CRLF, m_CRLF + 2);
	return crlf == begin_write() ? nullptr : crlf;
}

const char *BufferReader::find_last_crlf() const 
{    
	const char *crlf = std::find_end(peek(), begin_write(), m_CRLF, m_CRLF + 2);
	return crlf == begin_write() ? nullptr : crlf;
}

const char *BufferReader::find_last_crlf_crlf() const {
	char crlf_crlf[] = "\r\n\r\n";
	const char *crlf = std::find_end(peek(), begin_write(), crlf_crlf, crlf_crlf + 4);
	return crlf == begin_write() ? nullptr : crlf;
}

void BufferReader::retrieve_all()  
{ 
	m_writer_index = 0; 
	m_reader_index = 0; 
}

void BufferReader::retrieve(size_t len) {
	if (len <= readable_bytes()) {
		m_reader_index += len;
		if (m_reader_index == m_writer_index) {
			m_reader_index = 0;
			m_writer_index = 0;
		}
	} else {
		retrieve_all();
	}
}

void BufferReader::retrieve_until(const char* end)
{ 
	retrieve(end - peek()); 
}

int BufferReader::read(SOCKET sockfd)
{	
	uint32_t size = writable_bytes();
	if(size < m_MAX_BYTES_PER_READ) {
		uint32_t buffer_reader_Size = (uint32_t)m_buffer.size();
		if(buffer_reader_Size > m_MAX_BUFFER_SIZE) {
			return 0; 
		}
        
		m_buffer.resize(buffer_reader_Size + m_MAX_BYTES_PER_READ);
	}

	int bytes_read = ::recv(sockfd, begin_write(), m_MAX_BYTES_PER_READ, 0);
	if(bytes_read > 0) {
		m_writer_index += bytes_read;
	}

	return bytes_read;
}


uint32_t BufferReader::read_all(std::string &data)
{
	uint32_t size = readable_bytes();
	if (size > 0)  {
		data.assign(peek(), size);
		m_writer_index = 0;
		m_reader_index = 0;
	}

	return size;
}

uint32_t BufferReader::read_until_crlf(std::string &data)
{
	const char *crlf = find_last_crlf();
	if (crlf == nullptr)  {
		return 0;
	}

	uint32_t size = (uint32_t)(crlf - peek() + 2);
	data.assign(peek(), size);
	retrieve(size);

	return size;
}

char *BufferReader::begin()
{ 
	return &*m_buffer.begin(); 
}

const char *BufferReader::begin() const
{ 
	return &*m_buffer.begin(); 
}

char *BufferReader::begin_write()
{ 
	return begin() + m_writer_index; 
}

const char *BufferReader::begin_write() const
{ 
	return begin() + m_writer_index; 
}