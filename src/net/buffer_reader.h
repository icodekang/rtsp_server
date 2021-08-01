#ifndef _BUFFER_READER_H_
#define _BUFFER_READER_H_

#include <cstdint>
#include <vector>
#include <string>
#include <algorithm>  
#include <memory>
#include "vlog.h"
#include "socket.h"

uint32_t read_uint32BE(char* data);
uint32_t read_uint32LE(char* data);
uint32_t read_uint24BE(char* data);
uint32_t read_uint24LE(char* data);
uint16_t read_uint16BE(char* data);
uint16_t read_uint16LE(char* data);
    
class BufferReader
{
public:	
	BufferReader(uint32_t initial_size = 2048);

	uint32_t readable_bytes() const;
	uint32_t writable_bytes() const;
	char       *peek();
	const char *peek() const;
	const char *find_first_crlf() const;
	const char *find_last_crlf() const;
	const char *find_last_crlf_crlf() const;
	void retrieve_all();
	void retrieve(size_t len);
	void retrieve_until(const char *end);
	int  read(SOCKET sockfd);
	uint32_t read_all(std::string &data);
	uint32_t read_until_crlf(std::string &data);
	uint32_t size() const;

	virtual ~BufferReader() = default;

private:
	char       *begin();
	const char *begin() const;
	char       *begin_write();
	const char *begin_write() const;

	std::vector<char> m_buffer;
	size_t            m_reader_index = 0;
	size_t            m_writer_index = 0;

	static const char     m_CRLF[];
	static const uint32_t m_MAX_BYTES_PER_READ = 4096;
	static const uint32_t m_MAX_BUFFER_SIZE    = 1024 * 100000;
};

#endif // _BUFFER_READER_H_


