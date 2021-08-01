#include "memory_manager.h"

void *memory_alloc(uint32_t size)
{
	return MemoryManager::instance().alloc(size);
}

void memory_free(void *ptr)
{
	return MemoryManager::instance().free(ptr);
}

MemoryPool::~MemoryPool()
{
	if (m_memory) {
		free(m_memory);
		m_memory = nullptr;
	}
}

void MemoryPool::init(uint32_t size, uint32_t n)
{
	if (m_memory) {
		return;
	}

	m_block_size = size;
	m_num_blocks = n;
	m_memory = (char*)malloc(m_num_blocks * (m_block_size + sizeof(MemoryBlock)));
	m_head   = (MemoryBlock*)m_memory;
	m_head->block_id = 1;
	m_head->pool     = this;
	m_head->next     = nullptr;

	MemoryBlock* current = m_head;
	for (uint32_t n = 1; n < m_num_blocks; n++) {
		MemoryBlock* next = (MemoryBlock*)(m_memory + (n * (m_block_size + sizeof(MemoryBlock))));
		next->block_id = n + 1;
		next->pool     = this;
		next->next     = nullptr;

		current->next = next;
		current       = next;
	}
}

void* MemoryPool::alloc(uint32_t size)
{
	std::lock_guard<std::mutex> locker(m_mutex);
	if (m_head != nullptr) {
		MemoryBlock* block = m_head;
		m_head = m_head->next;
		return ((char*)block + sizeof(MemoryBlock));
	}

	return nullptr;
}

void MemoryPool::free(void* ptr)
{
	MemoryBlock *block = (MemoryBlock*)((char*)ptr - sizeof(MemoryBlock));
	if (block->block_id != 0) {
		std::lock_guard<std::mutex> locker(m_mutex);
		block->next = m_head;
		m_head = block;
	}
}

size_t MemoryPool::bolck_size() const
{ 
	return m_block_size; 
}

MemoryManager::MemoryManager()
{
	m_memory_pools[0].init(4096,   50);
	m_memory_pools[1].init(40960,  10);
	m_memory_pools[2].init(102400, 5);
	//m_memory_pools[3].Init(204800, 2);
}

MemoryManager& MemoryManager::instance()
{
	static MemoryManager s_mgr;
	return s_mgr;
}

void* MemoryManager::alloc(uint32_t size)
{
	for (int n = 0; n < MaxMemoryPool; n++) {
		if (size <= m_memory_pools[n].bolck_size()) {
			void *ptr = m_memory_pools[n].alloc(size);
			if (ptr != nullptr) {
				return ptr;
			} else {
				break;
			}
		}
	} 

	MemoryBlock *block = (MemoryBlock*)malloc(size + sizeof(MemoryBlock));
	block->block_id = 0;
	block->pool = nullptr;
	block->next = nullptr;
	return ((char*)block + sizeof(MemoryBlock));
}

void MemoryManager::free(void* ptr)
{
	MemoryBlock *block = (MemoryBlock*)((char*)ptr - sizeof(MemoryBlock));
	MemoryPool  *pool = block->pool;
	
	if (pool != nullptr && block->block_id > 0) {
		pool->free(ptr);
	} else {
		::free(block);
	}
}
