#pragma once

#include "Common.h"

class PageCache
{
	SpanList _spanList[PAGE_CACHE_SPAN_LIST_NUM + 1];


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

	/// <summary>
	/// 申请锁
	/// </summary>
	void lock();

	/// <summary>
	/// 释放锁
	/// </summary>
	void unlock();

};