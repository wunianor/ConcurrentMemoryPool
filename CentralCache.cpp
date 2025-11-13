#include "CentralCache.h"


//初始化CentralCache对象池
FixedSizeMemoryPool<CentralCache> CentralCache::_centralCacheObjPool;

//nullptr初始化唯一CentralCache实例
CentralCache* CentralCache::_instance = nullptr;

//初始化创建唯一CentralCache实例的互斥锁
std::mutex CentralCache::_createInstanceMutex;

CentralCache::CentralCache() {}

CentralCache* CentralCache::getInstance()
{
	if (nullptr == _instance)
	{
		std::unique_lock<std::mutex> lock(_createInstanceMutex);
		if (nullptr == _instance)
		{
			_instance = _centralCacheObjPool.New();
		}
	}

	return _instance;
}


SpanNode* CentralCache::getOneSpanNode(SpanList& spanList, size_t alignedBytes)
{
	assert(1 <= alignedBytes && alignedBytes <= THREAD_CACHE_MAX_ALLOCATE_BYTES);

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
	SpanNode* span = PageCache::getInstance()->fetchPageNumSpan(CalculateTool::calculateFetchPageNum(alignedBytes));
	PageCache::getInstance()->unlock();
	

	//将span内的多个page切成小块的碎片空间,并链接成链表
	char* begin = (char*)((span->_firstPageId) << PAGE_SHIFT);//多个page的起始地址
	char* end = begin + span->_pageNum * (1 << PAGE_SHIFT); //多个page的末尾地址,[begin,end)
	char* tail = begin; //当前链表的最后一个结点(的起始地址),[begin,tail],tail->next=nullptr
	char* tailNextNodeTailAddress = (tail + alignedBytes) + alignedBytes - 1; //tail结点的下一个结点的最后一个地址
	size_t count = (end - begin) / alignedBytes;//碎片内存的数量
	while (tailNextNodeTailAddress < end)
	{
		nextMemoryNode(tail) = tail + alignedBytes;
		tail = (char*)nextMemoryNode(tail); //更新tail结点
		tailNextNodeTailAddress = (tail + alignedBytes) + alignedBytes - 1;
	}
	nextMemoryNode(tail) = nullptr; //最后一个结点的next设为nullptr

	span->_fragmentedMemoryList.pushRange(begin, tail, count);
	span->_fragmentedMemorySize = alignedBytes;

	spanList.lock();
	spanList.pushFront(span);
	

	return span;
}

size_t CentralCache::fetchRangeFramentedMemory(size_t alignedBytes, size_t expectedNum,void*& start, void*& end)
{
	assert(1 <= alignedBytes && alignedBytes <= THREAD_CACHE_MAX_ALLOCATE_BYTES);
	assert(expectedNum > 0);

	size_t index = CalculateTool::calculateIndex(alignedBytes);


	_spanList[index].lock();

	SpanNode* spanNode = getOneSpanNode(_spanList[index], alignedBytes);
	size_t actualNum = spanNode->_fragmentedMemoryList.popRange(expectedNum, start, end);
	spanNode->_useCount += actualNum;

	_spanList[index].unlock();


	return actualNum;
}


void CentralCache::freeListToCentrealCacheSpans(size_t index, void* begin)
{
	_spanList[index].lock();
	
	void* cur = begin; //保存当前正在释放的内存碎片
	while (cur)
	{
		void* next = nextMemoryNode(cur);

		size_t pageId = (size_t)cur >> PAGE_SHIFT; //计算cur所在page的页号

		PageCache::getInstance()->lock();
		SpanNode* spanNode = PageCache::getInstance()->pageIdMapSpanNode(pageId); //计算page在哪个spanNode内
		PageCache::getInstance()->unlock();
		
		assert(nullptr != spanNode);
		

		spanNode->_fragmentedMemoryList.push(cur);//释放cur到对应的spanNode
		--(spanNode->_useCount);

		if (spanNode->_useCount == 0)
		{
			_spanList[index].erase(spanNode);

			
			spanNode->_prev = nullptr;
			spanNode->_next = nullptr;
			spanNode->_fragmentedMemoryList.setEmpty();
			
			_spanList[index].unlock();
			PageCache::getInstance()->lock();
			PageCache::getInstance()->freeSpanNodeToPageCache(spanNode);
			PageCache::getInstance()->unlock();
			_spanList[index].lock();
		}

		cur = next;
	}

	_spanList[index].unlock();
}
