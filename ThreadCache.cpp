#include "ThreadCache.h"


void* ThreadCache::fetchFromCentralCache(size_t index, size_t alignedBytes)
{
	assert(0 <= index && index < FRAMENTED_MEMORY_LIST_NUM);
	assert(1 <= alignedBytes && alignedBytes <= MAX_ALLOCATE_BYTES);

	/*
		慢开始获取碎片内存算法
			第一次申请,获取到的碎片内存较少;
			后续会随着申请次数的增多,获取到的碎片内存也越多;
			alignedBytes越大,预期获取到的个数就越少;
			alignedBytes越小,预期获取到的个数就越多;
	*/
	//计算预期从CentralCache层批量获取的碎片内存的个数
	size_t expectedFragmentedMemoryNum = std::min(_fragmentedMemoryList[index].nextFetchFragmentedMemoryNumFromCentralCache(), CalculcateTool::calculateFetchFragmentedMemoryNum(alignedBytes));

	if (expectedFragmentedMemoryNum == _fragmentedMemoryList[index].nextFetchFragmentedMemoryNumFromCentralCache())
	{
		++_fragmentedMemoryList[index].nextFetchFragmentedMemoryNumFromCentralCache();//更新下一次从CentralCache获取碎片内存的数量
	}

	//从CentralCache层批量获取碎片内存,
	//得到的是一个begin开始,end结尾的链表
	void* begin = nullptr;
	void* end = nullptr;
	size_t actualFragmentedMemoryNum = CentralCache::getInstance()->fetchRangeFramentedMemory(alignedBytes, expectedFragmentedMemoryNum, begin, end);

	assert(actualFragmentedMemoryNum > 0);

	//如果实际获取的数量>1,将start后面的结点都放入ThreadCache层的_fragmentedMemoryList[index]内
	if (actualFragmentedMemoryNum > 1)
	{
		_fragmentedMemoryList[index].pushRange(nextMemoryNode(begin), end);
	}
	
	//返回获取到的碎片内存
	return begin;
}

void* ThreadCache::allocate(size_t bytes)
{
	assert(1 <= bytes && bytes <= MAX_ALLOCATE_BYTES);

	size_t alignedBytes = CalculcateTool::calculateAlignedBytes(bytes); //计算bytes对齐后的字节数
	size_t index = CalculcateTool::calculateIndex(bytes); //计算bytes对应链表的索引

	if (!_fragmentedMemoryList[index].empty()) //优先从size对应的碎片内存链表申请
	{
		return _fragmentedMemoryList[index].pop();
	}
	else //从CentralCache获取内存
	{
		return fetchFromCentralCache(index, alignedBytes);
	}
}

void ThreadCache::deallocate(void* ptr, size_t bytes)
{
	assert(nullptr != ptr);
	assert(1 <= bytes && bytes <= MAX_ALLOCATE_BYTES);

	size_t index = CalculcateTool::calculateIndex(bytes); //计算size对应链表的索引

	_fragmentedMemoryList[index].push(ptr);
}