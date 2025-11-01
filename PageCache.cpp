#include "PageCache.h"


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
		return _spanList->popFront();
	}

	//在有[pageNum+1,PAGE_CACHE_SPAN_LIST_NUM]个page的SpanNode的_spanList内寻找
	for (size_t i = pageNum + 1; i <= PAGE_CACHE_SPAN_LIST_NUM; ++i)
	{
		if (!_spanList[i].empty())
		{
			//把一个有i个page的SpanNode拆成
			//一个有 pageNum个page 的SpanNode 和
			//一个有 i-pageNum个page 的SpanNode
			SpanNode* nSpan = _spanList[i].popFront(); //有i个page的SpanNode

			SpanNode* kSpan = new SpanNode; //有pageNum个page的SpanNode
			kSpan->_firstPageStartAddress = nSpan->_firstPageStartAddress;			
			kSpan->_pageNum = pageNum;

			SpanNode* n_kSpan = nSpan; //有i-pageNum个page的SpanNode
			n_kSpan->_firstPageStartAddress = (char*)(n_kSpan->_firstPageStartAddress) + pageNum * (1 << PAGE_SHIFT);
			n_kSpan->_pageNum -= pageNum;

			_spanList[n_kSpan->_pageNum].pushFront(n_kSpan);
			
			return kSpan;
		}
	}

	//申请一个 有PAGE_CACHE_SPAN_LIST_NUM个page 的Span
	SpanNode* maxSpan = new SpanNode; 
	void* ptr = malloc(PAGE_CACHE_SPAN_LIST_NUM * (1 << PAGE_SHIFT));
	maxSpan->_firstPageStartAddress = ptr;
	maxSpan->_pageNum = PAGE_CACHE_SPAN_LIST_NUM;

	_spanList[maxSpan->_pageNum].pushFront(maxSpan);

	return fetchPageNumSpan(pageNum);
}


void PageCache::lock()
{
	_pageMutex.lock();
}

void PageCache::unlock()
{
	_pageMutex.unlock();
}