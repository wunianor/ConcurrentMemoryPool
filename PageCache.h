#pragma once

#include "Common.h"
#include "SpanList.h"
#include "SystemMemoryAlloc.h"
#include "PageMapSpanNode.hpp"


class PageCache
{
	/// <summary>
	/// _spanList数组,
	/// _spanList[i]为有i个page的Span组成的链表
	/// </summary>
	SpanList _spanList[PAGE_CACHE_SPAN_LIST_NUM + 1];

	/// <summary>
	/// pageId到SpanNode*的映射
	/// </summary>
	//std::unordered_map<size_t, SpanNode*> _pageIdMapSpanNode;
	TCMalloc_PageMap3<64 - PAGE_SHIFT> _pageIdMapSpanNode;


	/// <summary>
	/// pageCache层的互斥锁
	/// </summary>
	std::mutex _pageMutex;

	/// <summary>
	/// pageCache对象池
	/// 实际上这个对象池内只会有一个对象
	/// </summary>
	static FixedSizeMemoryPool<PageCache> _pageCacheObjPool;

	/// <summary>
	/// PageCache的唯一实例
	/// </summary>
	static PageCache* _instance;

	/// <summary>
	/// 创造唯一单例的互斥锁
	/// </summary>
	static std::mutex _createInstanceMutex;

public:

	/// <summary>
	/// 获取唯一PageCache实例
	/// </summary>
	/// <returns>返回唯一PageCache实例</returns>
	static PageCache* getInstance();

	/// <summary>
	/// 从PageCache内获取一个有pageNum个page的Span
	/// </summary>
	/// <param name="pageNum">页的数量</param>
	/// <returns>返回一个有pageNum个page的Span</returns>
	SpanNode* fetchPageNumSpan(size_t pageNum);

	/// <summary>
	/// 查找pageId所在的spanNode
	/// </summary>
	/// <param name="pageId">页号</param>
	/// <returns>返回pageId所在的spanNode</returns>
	SpanNode* getPageIdMapSpanNode(size_t pageId);


	/// <summary>
	/// 设置pageId所在的spanNode
	/// </summary>
	/// <param name="pageId">页号</param>
	/// <param name="spanNode">pageId所在的spanNode</param>
	/// <returns></returns>
	void setPageIdMapSpanNode(size_t pageId, SpanNode* spanNode);

	/// <summary>
	/// 向PageCache释放spanNode
	/// </summary>
	/// <param name="spanNode">需要释放的spanNode</param>
	void freeSpanNodeToPageCache(SpanNode* spanNode);

	/// <summary>
	/// 申请锁
	/// 
	/// 注意:
	/// .h和.cpp分离会导致lock()不是inline的,对性能影响不大;
	/// 但是,如果是inline的话,vs2022的性能监视器显示不出该函数的总执行时间
	/// </summary>
	void lock();

	/// <summary>
	/// 释放锁
	///
	/// 注意:
	/// .h和.cpp分离会导致unlock()不是inline的,对性能影响不大;
	/// 但是,如果是inline的话,vs2022的性能监视器显示不出该函数的总执行时间
	/// </summary>
	void unlock();
};




