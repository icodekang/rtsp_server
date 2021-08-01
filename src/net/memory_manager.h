#ifndef _MEMMORY_MANAGER_H_
#define _MEMMORY_MANAGER_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <mutex>
#include "vlog.h"

void *memory_alloc(uint32_t size);
void  memory_free(void *ptr);

class MemoryPool;

struct MemoryBlock
{
	uint32_t block_id = 0;
	MemoryPool  *pool = nullptr;
	MemoryBlock *next = nullptr;
};

class MemoryPool
{
public:
	MemoryPool() = default;

	void   init(uint32_t size, uint32_t n);
	void  *alloc(uint32_t size);
	void   free(void* ptr);
	size_t bolck_size() const;

	virtual ~MemoryPool();

private:
	char        *m_memory     = nullptr;
	uint32_t     m_block_size = 0;
	uint32_t     m_num_blocks = 0;
	MemoryBlock* m_head       = nullptr;
	std::mutex   m_mutex;
};

class MemoryManager
{
public:
	static MemoryManager &instance();

	void* alloc(uint32_t size);
	void  free(void* ptr);

	~MemoryManager() = default;

private:
	MemoryManager();

	static const int MaxMemoryPool = 3;
	MemoryPool m_memory_pools[MaxMemoryPool];
};

#endif // _MEMMORY_MANAGER_H_
