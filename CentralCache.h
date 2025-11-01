#pragma once

#include "Common.h"
#include "PageCache.h"


class CentralCache
{
	/// <summary>
	/// span链表数组
	/// </summary>
	SpanList _spanList[FRAMENTED_MEMORY_LIST_NUM];


	/// <summary>
	/// CentralCache的唯一实例
	/// </summary>
	static CentralCache* _instance;

	/// <summary>
	/// 创造唯一单例的互斥锁
	/// </summary>
	static std::mutex _createInstanceMutex;

public:
	/// <summary>
	/// 默认构造
	/// </summary>
	CentralCache();

	/// <summary>
	/// 禁用拷贝构造
	/// </summary>
	/// <param name=""></param>
	CentralCache(const CentralCache&) = delete;

	/// <summary>
	/// 获取唯一的CentralCache实例,
	/// 该方法是线程安全的
	/// </summary>
	/// <returns>返回唯一的CentralCache实例</returns>
	static CentralCache* getInstance();


	SpanNode* getOneSpanNode(SpanList& spanList, size_t alignedSize);

	/// <summary>
	/// 获取一定数量的碎片内存,
	/// 得到的是一个begin开始,end结尾的链表
	/// </summary>
	/// <param name="alignedBytes">对于单个碎片内存,对齐后的字节数</param>
	/// <param name="expectedNum">期望获取的数量</param>
	/// <param name="start">输出型参数,碎片内存链表的第一个结点</param>
	/// <param name="end">输出型参数,碎片内存链表的最后一个结点</param>
	/// <returns>返回实际获取到的数量</returns>
	size_t fetchRangeFramentedMemory(size_t alignedBytes, size_t expectedNum, void*& start, void*& end);
};


