#pragma once

#include "Common.h"
#include "SpanList.h"
#include "CalculateTool.h"
#include "FixedSizeMemoryPool.hpp"
#include "PageCache.h"


class CentralCache
{
	/// <summary>
	/// span链表数组
	/// </summary>
	SpanList _spanList[FRAMENTED_MEMORY_LIST_NUM];

	/// <summary>
	/// centralCache对象池
	/// 实际上这个对象池内只会有一个对象
	/// </summary>
	static FixedSizeMemoryPool<CentralCache> _centralCacheObjPool;

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

	/// <summary>
	/// 从spanList中获取一个SpanNode
	/// </summary>
	/// <param name="spanList">表示从哪个spanList中获取SpanNode</param>
	/// <param name="alignedSize">对齐之后的字节数</param>
	/// <returns>返回一个SpanNode的地址</returns>
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

	/// <summary>
	/// 将链表中的每一个碎片内存还给对应的SpanNode
	/// </summary>
	/// <param name="index"></param>
	/// <param name="begin">链表头结点</param>
	void freeListToCentrealCacheSpans(size_t index, void* begin);
};


//extern std::atomic<size_t> sum1, sum2, sum3;