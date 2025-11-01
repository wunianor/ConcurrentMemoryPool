#include "CentralCache.h"


CentralCache::CentralCache(){}




//nullptr初始化唯一CentralCache实例
CentralCache* CentralCache::_instance = nullptr;

//初始化创建唯一CentralCache实例的互斥锁
std::mutex CentralCache::_createInstanceMutex;

CentralCache* CentralCache::getInstance()
{
	if (nullptr == _instance)
	{
		std::unique_lock<std::mutex> lock(_createInstanceMutex);
		if (nullptr == _instance)
		{
			_instance = new CentralCache;
		}
	}

	return _instance;
}


SpanNode* CentralCache::getOneSpanNode(SpanList& spanList, size_t alignedBytes)
{
	assert(1 <= alignedBytes && alignedBytes <= MAX_ALLOCATE_BYTES);

	//先去spanList里面去寻找有未分配内存的Span
	SpanNode* it = spanList.begin();
	while (it != spanList.end())
	{
		if (!it->_fragmentedMemoryList.empty()) //如果it指向的SpanNode的_fragmentedMemoryList有碎片内存结点
		{
			return it;
		}
		else
		{
			it = it->_next;
		}
	}

	//走到这里,说明spanList内所有的Span的碎片内存都分配完毕 or 没有Span,
	//从pageCache获取一个span
	spanList.unlock(); //释放spanList的锁,让其他释放碎片内存的线程可以进来
	PageCache::getInstance()->lock();
	SpanNode* span = PageCache::getInstance()->fetchPageNumSpan(CalculcateTool::calculateFetchPageNum(alignedBytes));
	PageCache::getInstance()->unlock();
	

	//将span内的大块内存切成小块的碎片空间,并链接成链表
	char* begin = (char*)span->_firstPageStartAddress;//大块内存起始地址
	char* end = begin + span->_pageNum * (1 << PAGE_SHIFT); //大块内存末尾地址,[begin,end)
	char* tail = begin; //当前链表的最后一个结点
	while (tail + alignedBytes < end)
	{
		nextMemoryNode(tail) = tail + alignedBytes;
		tail = (char*)nextMemoryNode(tail);
	}

	span->_fragmentedMemoryList.pushRange(begin, tail);

	spanList.lock();
	spanList.pushFront(span);
	

	return span;
}

size_t CentralCache::fetchRangeFramentedMemory(size_t alignedBytes, size_t expectedNum,void*& start, void*& end)
{
	assert(1 <= alignedBytes && alignedBytes <= MAX_ALLOCATE_BYTES);
	assert(expectedNum > 0);

	size_t index = CalculcateTool::calculateIndex(alignedBytes);


	_spanList[index].lock();

	SpanNode* spanNode = getOneSpanNode(_spanList[index], alignedBytes);
	size_t actualNum = spanNode->_fragmentedMemoryList.popRange(expectedNum, start, end);

	_spanList[index].unlock();



	return actualNum;
}
