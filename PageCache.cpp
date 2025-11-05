#include "PageCache.h"

#ifdef DEBUG
	int systemAllocCnt = 0;
#endif 



//nullptr初始化PageCache实例
PageCache* PageCache::_instance = nullptr;

//初始化创建唯一PageCache实例的互斥锁
std::mutex PageCache::_createInstanceMutex;

PageCache* PageCache::getInstance()
{
	if (nullptr == _instance)
	{
		std::unique_lock<std::mutex> lock(_createInstanceMutex);
		if (nullptr == _instance)
		{
			_instance = new PageCache;
		}
	}

	return _instance;
}

SpanNode* PageCache::fetchPageNumSpan(size_t pageNum)
{
	assert(1 <= pageNum && pageNum <= PAGE_CACHE_SPAN_LIST_NUM);

	//_spanList[pageNum]内存在 有pageNum个page的SpanNode
	if (!_spanList[pageNum].empty())
	{
		SpanNode* pageNumSpan = _spanList[pageNum].popFront();

		//建立pageNumSpan内各个page与pageNumPage的映射,
		//这些page在CentralCache层内可能会被切分,
		//所以每个page都要映射
		for (size_t j = 0; j < pageNumSpan->_pageNum; ++j)
		{
			_pageIdMapSpanNode[pageNumSpan->_firstPageId + j] = pageNumSpan;
		}
		return pageNumSpan;
	}

	//在有[pageNum+1,PAGE_CACHE_SPAN_LIST_NUM]个page的SpanNode的_spanList内寻找
	for (size_t i = pageNum + 1; i <= PAGE_CACHE_SPAN_LIST_NUM; ++i)
	{
		if (!_spanList[i].empty())
		{
			//把一个有i个page的SpanNode拆成
			//一个有 pageNum个page 的SpanNode 和
			//一个有 i-pageNum个page 的SpanNode
			SpanNode* iSpan = _spanList[i].popFront(); //有i个page的SpanNode

			SpanNode* pageNumSpan = new SpanNode; //有pageNum个page的SpanNode
			pageNumSpan->_firstPageId = iSpan->_firstPageId;			
			pageNumSpan->_pageNum = pageNum;

			//建立pageNumSpan内各个page与pageNumPage的映射,
			//这些page在CentralCache层内可能会被切分,
			//所以每个page都要映射
			for (size_t j = 0; j < pageNumSpan->_pageNum; ++j)
			{
				_pageIdMapSpanNode[pageNumSpan->_firstPageId + j] = pageNumSpan;
			}

			SpanNode* i_pageNumSpan = iSpan; //有i-pageNum个page的SpanNode
			i_pageNumSpan->_firstPageId = iSpan->_firstPageId + pageNum;
			i_pageNumSpan->_pageNum -= pageNum;
			
			//建立i_pageNumSpan内第一个page和最后一个page与i_pageNumSpan的映射,
			//方便后续合并page用
			_pageIdMapSpanNode[i_pageNumSpan->_firstPageId] = i_pageNumSpan;
			_pageIdMapSpanNode[i_pageNumSpan->_firstPageId + i_pageNumSpan->_pageNum - 1] = i_pageNumSpan;



			_spanList[i_pageNumSpan->_pageNum].pushFront(i_pageNumSpan);
			
			return pageNumSpan;
		}
	}

	//申请一个 有PAGE_CACHE_SPAN_LIST_NUM个page 的Span
	SpanNode* maxSpan = new SpanNode; 
	void* ptr = systemMemoryAlloc(PAGE_CACHE_SPAN_LIST_NUM * (1 << PAGE_SHIFT));

	if (nullptr == ptr)//如果申请失败,直接退出程序
	{
#ifdef DEBUG
		std::cout << "[pageCache]systemMemoryAlloc申请失败,申请次数:" << (++systemAllocCnt) << std::endl;
#endif 


		exit(BAD_ALLOC);
	}

#ifdef DEBUG
	std::cout << "[pageCache]:ptr=" << ptr << "申请次数:" << (++systemAllocCnt) << std::endl;
#endif // DEBUG

	
	maxSpan->_firstPageId = (size_t)ptr >> PAGE_SHIFT;
	maxSpan->_pageNum = PAGE_CACHE_SPAN_LIST_NUM;

	_spanList[maxSpan->_pageNum].pushFront(maxSpan);

	return fetchPageNumSpan(pageNum);
}

SpanNode* PageCache::pageIdMapSpanNode(size_t pageId)
{
	auto it = _pageIdMapSpanNode.find(pageId);
	if (it == _pageIdMapSpanNode.end())
	{
		return nullptr;
	}
	return it->second;
}

void PageCache::freeSpanToPageCache(SpanNode* spanNode)
{
	assert(nullptr != spanNode);

	//这一句一定要放freeSpanToPageCache()内,
	//不能放PageCache加锁之前(调试2个半小时的痛)
	spanNode->_isUse = false;

	//将spanNode与前面能合并的span合并
	while (true)
	{
		//寻找spanNode在内存上连续的上一个spanNode
		auto it = _pageIdMapSpanNode.find(spanNode->_firstPageId - 1);
		if (it == _pageIdMapSpanNode.end()) //如果没找到
		{
			break;
		}

		//到这里就是找到了
		SpanNode* prevSpanNode = it->second;

		//如果prevSpanNode正在被使用,就不合并
		if (prevSpanNode->_isUse == true) 
		{
			break;
		}

		//如果合并后spanNode拥有的page的数量>MAX_PAGENUM_IN_SPANNODE,就不合并
		if (prevSpanNode->_pageNum + spanNode->_pageNum > MAX_PAGENUM_IN_SPANNODE) 
		{
			break;
		}

		_spanList[prevSpanNode->_pageNum].erase(prevSpanNode);

		spanNode->_firstPageId = prevSpanNode->_firstPageId;
		spanNode->_pageNum += prevSpanNode->_pageNum;
		
		delete prevSpanNode;
	}
	

	//将spanNode与后面能合并的span合并
	while (true)
	{
		//寻找spanNode在内存上连续的下一个spanNode
		auto it = _pageIdMapSpanNode.find(spanNode->_firstPageId + spanNode->_pageNum);
		if (it == _pageIdMapSpanNode.end())
		{
			break;
		}

		SpanNode* nextSpanNode = it->second;

		//如果nextSpanNode正在被使用,就不合并
		if (nextSpanNode->_isUse = true)
		{
			break;
		}

		//如果合并后spanNode拥有的page的数量>MAX_PAGENUM_IN_SPANNODE,就不合并
		if (spanNode->_pageNum + nextSpanNode->_pageNum > MAX_PAGENUM_IN_SPANNODE)
		{
			break;
		}

		_spanList[nextSpanNode->_pageNum].erase(nextSpanNode);

		spanNode->_pageNum += nextSpanNode->_pageNum;

		delete nextSpanNode;
	}


	_spanList[spanNode->_pageNum].pushFront(spanNode);

	//建立spanNode内第一个page和最后一个page与spanNode*指针的映射,
	//方便后续进行合并
	_pageIdMapSpanNode[spanNode->_firstPageId] = spanNode;
	_pageIdMapSpanNode[spanNode->_firstPageId + spanNode->_pageNum - 1] = spanNode;
}

void PageCache::lock()
{
	_pageMutex.lock();
}

void PageCache::unlock()
{
	_pageMutex.unlock();
}