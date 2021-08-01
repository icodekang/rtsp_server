#ifndef _RING_BUFFER_H_
#define _RING_BUFFER_H_

#include <vector>
#include <memory>
#include <atomic>
#include <iostream>
#include "vlog.h"

template <typename T>
class RingBuffer
{
public:
	RingBuffer(int capacity = 60);

	bool push(const T &data);	
	bool push(T &&data);   
	bool pop(T &data);
	bool is_full()  const;	
	bool is_empty() const;
	int  size()     const;

	virtual ~RingBuffer() = default;
	
private:		
	template <typename F>
	bool push_data(F &&data);
	void add(int &pos);

	int m_capacity = 0;
	int m_put_pos  = 0;
	int m_get_pos  = 0;

	std::atomic_int m_num_datas;     			
	std::vector<T>  m_buffer;
};

#define RINGBUFFER
#include "ring_buffer.cc"

#endif // _RING_BUFFER_H_


