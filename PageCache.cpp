#include "PageCache.h"

#ifdef DEBUG
	int systemAllocCnt = 0;
#endif 


//初始化pageCache对象池
FixedSizeMemoryPool<PageCache> PageCache::_pageCacheObjPool;

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
			_instance = _pageCacheObjPool.New();
		}
	}

	return _instance;
}

SpanNode* PageCache::fetchPageNumSpan(size_t pageNum)
{
	if (pageNum > PAGE_CACHE_SPAN_LIST_NUM) 
	{
		void* ptr = systemMemoryAlloc(pageNum << PAGE_SHIFT);
		if (nullptr == ptr)
		{
			exit(BAD_ALLOC);
		}

		SpanNode* spanNode = spanNodeObjPool.New();
		spanNode->_firstPageId = (size_t)ptr >> PAGE_SHIFT;
		spanNode->_pageNum = pageNum;

		setPageIdMapSpanNode(spanNode->_firstPageId, spanNode);
		
		return spanNode;
	}

	SpanNode* pageNumSpan = nullptr; //有pageNum个page的SpanNode

	//_spanList[pageNum]内存在 有pageNum个page的SpanNode
	if (!_spanList[pageNum].empty())
	{
		pageNumSpan = _spanList[pageNum].popFront();
	}
	else
	{
		//在有[pageNum+1,PAGE_CACHE_SPAN_LIST_NUM]个page的SpanNode的_spanList内寻找
		for (size_t i = pageNum + 1; i <= PAGE_CACHE_SPAN_LIST_NUM; ++i)
		{
			if (!_spanList[i].empty())
			{
				//把一个有i个page的SpanNode拆成
				//一个有 pageNum个page 的SpanNode 和
				//一个有 i-pageNum个page 的SpanNode
				SpanNode* iSpan = _spanList[i].popFront(); //有i个page的SpanNode

				pageNumSpan = spanNodeObjPool.New(); //有pageNum个page的SpanNode
				pageNumSpan->_firstPageId = iSpan->_firstPageId;
				pageNumSpan->_pageNum = pageNum;

				SpanNode* i_pageNumSpan = iSpan; //有i-pageNum个page的SpanNode
				i_pageNumSpan->_firstPageId = iSpan->_firstPageId + pageNum;
				i_pageNumSpan->_pageNum -= pageNum;

				//建立i_pageNumSpan内第一个page和最后一个page与i_pageNumSpan的映射,
				//方便后续合并span用
				setPageIdMapSpanNode(i_pageNumSpan->_firstPageId, i_pageNumSpan);
				setPageIdMapSpanNode(i_pageNumSpan->_firstPageId + i_pageNumSpan->_pageNum - 1, i_pageNumSpan);

				_spanList[i_pageNumSpan->_pageNum].pushFront(i_pageNumSpan);

				break;//找到了要break(调试了1个半小时才发现)
			}
		}
	}


	if (nullptr != pageNumSpan) //如果找到了就建立映射并返回
	{
		//建立pageNumSpan内各个page与pageNumPage的映射,
		//这些page在CentralCache层内可能会被切分,
		//所以每个page都要映射
		//pageIdMapSpanNodeMutexLock();
		for (size_t j = 0; j < pageNumSpan->_pageNum; ++j)
		{
			setPageIdMapSpanNode(pageNumSpan->_firstPageId + j, pageNumSpan);
		}
		//pageIdMapSpanNodeMutexUnlock();

		//将pageNumSpan的状态置为正在centralCache内使用
		pageNumSpan->_isUse = true;

		return pageNumSpan;
	}


	//申请一个 有PAGE_CACHE_SPAN_LIST_NUM个page 的Span
	SpanNode* maxSpan = spanNodeObjPool.New();
	void* ptr = systemMemoryAlloc(PAGE_CACHE_SPAN_LIST_NUM * (1 << PAGE_SHIFT));

	if (nullptr == ptr)//如果申请失败,直接退出程序
	{
#ifdef DEBUG
		std::cout << "[pageCache]systemMemoryAlloc申请失败,申请次数:" << (++systemAllocCnt) << std::endl;
#endif 


		exit(BAD_ALLOC);
	}

#ifdef DEBUG
	++systemAllocCnt;
	//std::cout << "[pageCache]:ptr=" << ptr << "申请次数:" << (systemAllocCnt) << std::endl;
#endif // DEBUG

	
	maxSpan->_firstPageId = (size_t)ptr >> PAGE_SHIFT;
	maxSpan->_pageNum = PAGE_CACHE_SPAN_LIST_NUM;

	//建立maxSpan内第一个page和最后一个page与maxSpan的映射,
	//方便后续合并span用
	setPageIdMapSpanNode(maxSpan->_firstPageId, maxSpan);
	setPageIdMapSpanNode(maxSpan->_firstPageId + maxSpan->_pageNum - 1, maxSpan);

	_spanList[maxSpan->_pageNum].pushFront(maxSpan);

	return fetchPageNumSpan(pageNum);
}

SpanNode* PageCache::getPageIdMapSpanNode(size_t pageId)
{
	/*auto it = _pageIdMapSpanNode.find(pageId);
	if (it == _pageIdMapSpanNode.end())
	{
		return nullptr;
	}
	return it->second;*/


	SpanNode* spanNode = (SpanNode*)_pageIdMapSpanNode.get(pageId);
	if (nullptr == spanNode)
	{
		return nullptr;
	}
	return spanNode;
}

void PageCache::setPageIdMapSpanNode(size_t pageId, SpanNode* spanNode)
{
	if (_pageIdMapSpanNode.Ensure(pageId, 1))
	{
		_pageIdMapSpanNode.set(pageId, spanNode);
	}
}

void PageCache::freeSpanNodeToPageCache(SpanNode* spanNode)
{
	assert(nullptr != spanNode);
	
	if (spanNode->_pageNum > PAGE_CACHE_SPAN_LIST_NUM)
	{
		void* ptr = (void*)((spanNode->_firstPageId) << PAGE_SHIFT); //计算spanNode的第1个page的起始地址
		size_t size = (spanNode->_pageNum) << PAGE_SHIFT; //计算spanNode包含的page的总大小

		if (systemMemoryFree(ptr, size) == false)//如果释放内存失败
		{
			exit(BAD_FREE);
		}

		spanNodeObjPool.Delete(spanNode);

		return;
	}

	//将spanNode与前面能合并的span合并
	while (true)
	{
		//寻找spanNode在内存上连续的上一个spanNode
		//pageIdMapSpanNodeMutexLock();
		SpanNode* prevSpanNode = getPageIdMapSpanNode(spanNode->_firstPageId - 1);
		//pageIdMapSpanNodeMutexUnlock();
		if (nullptr == prevSpanNode) //如果没找到
		{
			break;
		}

		//到这里就是找到了
		//正常情况prevSpanNode此时在_spanList[prevSpanNode->_pageNum]内

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
		
		spanNodeObjPool.Delete(prevSpanNode);
	}
	

	//将spanNode与后面能合并的span合并
	while (true)
	{
		//寻找spanNode在内存上连续的下一个spanNode
		//pageIdMapSpanNodeMutexLock();
		SpanNode* nextSpanNode = getPageIdMapSpanNode(spanNode->_firstPageId + spanNode->_pageNum);
		//pageIdMapSpanNodeMutexUnlock();
		if (nullptr == nextSpanNode)
		{
			break;
		}

		//到这里就是找到了
		//正常情况nextSpanNode在_spanList[nextSpanNode->_pageNum]内

		//如果nextSpanNode正在被使用,就不合并
		if (nextSpanNode->_isUse == true)
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

		spanNodeObjPool.Delete(nextSpanNode);
	}


	//建立spanNode内第一个page和最后一个page与spanNode*指针的映射,
	//方便后续进行合并
	setPageIdMapSpanNode(spanNode->_firstPageId, spanNode);
	setPageIdMapSpanNode(spanNode->_firstPageId + spanNode->_pageNum - 1, spanNode);

	_spanList[spanNode->_pageNum].pushFront(spanNode);

	//spanNode->_isUse = false;这一句一定要放freeSpanNodeToPageCache()内,
	//不能放PageCache加锁之前(调试2个半小时的痛),
	//不然会有线程安全问题(
	//	线程a刚把spanNode sa的_isUse置为false,
	//  另一个线程b就进来这个函数要把sa合并了(此时线程a会被锁阻塞了),
	//  合并的时候执行_spanList[sa->_pageNum].erase(sa)时,
	//  就发现sa->_prev为nullptr(因为已经被线程a置为nullptr了),
	//  然后报错崩溃
	// )
	spanNode->_isUse = false;
}

/// <summary>
/// 申请锁
/// 
/// 注意:
/// .h和.cpp分离会导致lock()不是inline的,对性能影响不大;
/// 但是,如果是inline的话,vs2022的性能监视器显示不出该函数的总执行时间
/// </summary>
void PageCache::lock()
{
	_pageMutex.lock();
}

/// <summary>
/// 释放锁
/// 
/// 注意:
/// .h和.cpp分离会导致unlock()不是inline的,对性能影响不大;
/// 但是,如果是inline的话,vs2022的性能监视器显示不出该函数的总执行时间
/// </summary>
void PageCache::unlock()
{
	_pageMutex.unlock();
}


