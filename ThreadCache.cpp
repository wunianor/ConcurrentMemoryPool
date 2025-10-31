#include "ThreadCache.h"


void* ThreadCache::fetchFromCentralCache(size_t index, size_t size)
{
	return nullptr;
}

void* ThreadCache::allocate(size_t size)
{
	assert(size <= MAX_SIZE);
	size_t alignedSize = CalculcateTool::calculateAlignedSize(size); //计算size对齐后的大小
	size_t index = CalculcateTool::calculateIndex(size); //计算size对应链表的索引

	if (!_fragmentedMemoryList[index].empty()) //优先从size对应的碎片内存链表申请
	{
		return _fragmentedMemoryList[index].pop();
	}
	else
	{
		return fetchFromCentralCache(index, size);
	}
}

void ThreadCache::deallocate(void* ptr, size_t size)
{
	assert(ptr);
	assert(size <= MAX_SIZE);

	size_t index = CalculcateTool::calculateIndex(size); //计算size对应链表的索引

	_fragmentedMemoryList[index].push(ptr);
}