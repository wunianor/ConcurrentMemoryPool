#pragma once
#include "Common.h"
#include "CalculateTool.h"
#include "CentralCache.h"
#include "PageCache.h"


class ThreadCache
{
	/// <summary>
	/// 碎片内存链表数组
	/// </summary>
	FragmentedMemoryList _fragmentedMemoryList[FRAMENTED_MEMORY_LIST_NUM];

public:
	
	/// <summary>
	/// 分配指定大小的内存并返回指向该内存的指针。
	/// </summary>
	/// <param name="bytes">要分配的内存大小</param>
	/// <returns>指向已分配内存的指针</returns>
	void* allocate(size_t bytes);

	/// <summary>
	/// 释放之前分配的内存块
	/// </summary>
	/// <param name="ptr">指向要释放的内存块的起始地址</param>
	/// <param name="bytes>要释放的内存块的大小</param>
	void deallocate(void* ptr, size_t bytes);


	/// <summary>
	/// 从CentralCache内获取碎片内存
	/// </summary>
	/// <param name="index">链表在数组内的索引</param>
	/// <param name="alignedbytes">对齐后的字节数</param>
	/// <returns></returns>
	void* fetchFromCentralCache(size_t index, size_t alignedbytes);
};

/// <summary>
/// 每个线程独占的ThreadCache(线程局部存储)
/// </summary>
static _declspec(thread) ThreadCache* TLSThreadCache = nullptr;//static保证其只在当前文件可见

/// <summary>
/// 每个线程独占的threadCache对象池(线程局部存储),
/// 实际上这个对象池内只会有一个对象
/// </summary>
static _declspec(thread) FixedSizeMemoryPool<ThreadCache> TLSthreadCacheObjPool;

