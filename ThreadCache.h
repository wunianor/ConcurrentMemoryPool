#pragma once
#include "Common.h"

static const int MAX_SIZE = 256 * 1024;

class ThreadCache
{
	/// <summary>
	/// 碎片内存链表数组
	/// </summary>
	FragmentedMemoryList _fragmentedMemoryList[208];


public:
	/// <summary>
	/// 分配指定大小的内存并返回指向该内存的指针。
	/// </summary>
	/// <param name="size">要分配的内存大小（以字节为单位）</param>
	/// <returns>指向已分配内存的指针（void*）</returns>
	void* allocate(size_t size);

	/// <summary>
	/// 释放先前分配的内存块。
	/// </summary>
	/// <param name="ptr">指向要释放的内存块的指针（要释放的起始地址）</param>
	/// <param name="size">要释放的内存块的大小，单位为字节，用于告知分配器要释放的内存范围。</param>
	void deallocate(void* ptr, size_t size);

	//从CentralCache获取内存
	void* fetchFromCentralCache(size_t index, size_t size);
};

/// <summary>
/// 每个线程独占的ThreadCache
/// </summary>
static _declspec(thread) ThreadCache* TLSThreadCache = nullptr;//static保证其只在当前文件可见



