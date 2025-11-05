#pragma once

#include "Common.h"

#define BAD_ALLOC 1

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
	std::unordered_map<size_t, SpanNode*> _pageIdMapSpanNode;

	/// <summary>
	/// pageCache层的互斥锁
	/// </summary>
	std::mutex _pageMutex;

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


	SpanNode* pageIdMapSpanNode(size_t pageId);

	void freeSpanToPageCache(SpanNode* spanNode);

	/// <summary>
	/// 申请锁
	/// </summary>
	void lock();

	/// <summary>
	/// 释放锁
	/// </summary>
	void unlock();

};