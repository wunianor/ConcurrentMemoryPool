#pragma once

#include <iostream>
#include <unordered_map>
#include <exception>
#include <mutex>


#include <cstdlib>
#include <cassert>
#include <errno.h>

#ifdef WIN32
	#include <windows.h>
#elif defined(__unix__) || defined(__APPLE__) 
	#include <unistd.h>
	#include <sys/mman.h>
#else
	#error "unknown platform"
#endif

#include "FragmentedMemoryList.h"


#define BAD_ALLOC 1

//#define DEBUG


/// <summary>
/// 碎片内存链表的个数
/// </summary>
const size_t FRAMENTED_MEMORY_LIST_NUM = 208;

/// <summary>
/// 最大可申请内存-256KB
/// </summary>
const size_t MAX_ALLOCATE_BYTES = 256 * 1024; 

/// <summary>
/// PageCache中spanList的个数
/// </summary>
const size_t PAGE_CACHE_SPAN_LIST_NUM = 128;

/// <summary>
/// 一个spanNode内最多能拥有的page的数量
/// </summary>
const size_t MAX_PAGENUM_IN_SPANNODE = PAGE_CACHE_SPAN_LIST_NUM;

/// <summary>
/// log2(一个页的大小(8KB))
/// </summary>
const size_t PAGE_SHIFT = 13;



/// <summary>
/// 向系统申请一块内存
/// </summary>
/// <param name="size">内存大小</param>
/// <returns>
/// 申请成功,返回内存起始地址;
/// 申请失败,返回nullptr
/// </returns>
static void* systemMemoryAlloc(size_t size)
{
	void* ptr = nullptr;

#ifdef WIN32
	ptr = VirtualAlloc(
		NULL,                          // 系统自动分配地址（天然页对齐）
		size,		                   // 申请的内存大小
		MEM_RESERVE | MEM_COMMIT,      // 核心参数：保留地址空间+提交物理内存
		PAGE_READWRITE                 // 仅读写权限（安全无执行）
	);

	if(NULL == ptr)
	{
		ptr = nullptr;
	}

#elif defined(__unix__) || defined(__APPLE__) 
	ptr = mmap(
		NULL,                          // 内核自动分配地址（天然页对齐）
		size,		                   // 申请的内存大小
		PROT_READ | PROT_WRITE,        // 仅读写权限（安全无执行）
		MAP_PRIVATE | MAP_ANONYMOUS,   // 匿名+私有映射（无文件IO，进程独占）
		-1,                            // 匿名映射无需文件描述符
		0                              // 偏移量必须为 0
	);

	if (MAP_FAILED == ptr)
	{
		ptr = nullptr;
	}

#else
	#error "unknown platform"
#endif

	return ptr;
}



/// <summary>
/// 计算工具类
/// </summary>
class CalculcateTool
{
	/// <summary>
	/// 计算对于bytes对齐之后的字节数
	/// </summary>
	/// <param name="bytes">字节数</param>
	/// <param name="alignedSize">对齐字节数</param>
	/// <returns>返回对于bytes对齐之后的字节数</returns>
	static size_t _calculateAlignedBytes(size_t bytes, size_t alignedSize)
	{
		//等价于((bytes-1)/aligendSize+1)*aligendSize;
		return (bytes - 1 + alignedSize) & (~(alignedSize - 1));
	}

	/// <summary>
	/// 计算bytes字节数在哪个桶(链表)内,返回其索引
	/// </summary>
	/// <param name="bytes">字节数</param>
	/// <param name="alignedShift">log2(对齐字节数alignedSize)</param>
	/// <returns>返回bytes对应桶(链表)的索引</returns>
	static size_t _calculateIndex(size_t bytes, size_t alignedShift)
	{
		return ((bytes + (1LL << alignedShift) - 1) >> alignedShift) - 1;
	}

public:
	/*
		控制浪费在10%
		[1,128]                   8byte对齐               对应碎片内存链表数组的[0,15]
		[128+1,1024]              16byte对齐              对应碎片内存链表数组的[16,71]
		[1024+1,8*1024]           128byte对齐             对应碎片内存链表数组的[72,127]
		[8*1024+1,64*1024]        1024byte对齐            对应碎片内存链表数组的[128,183]
		[64*1024+1,256*1024]      8*1024byte对齐          对应碎片内存链表数组的[184,207]
	*/


	/// <summary>
	/// 计算对于bytes对齐之后的字节数
	/// </summary>
	/// <param name="bytes">字节数</param>
	/// <returns>返回对于bytes对齐之后的字节数</returns>
	static size_t calculateAlignedBytes(size_t bytes)
	{
		if (bytes <= 128)
		{
			return _calculateAlignedBytes(bytes, 8);
		}
		else if (bytes <= 1024)
		{
			return _calculateAlignedBytes(bytes, 16);
		}
		else if (bytes <= 8 * 1024)
		{
			return _calculateAlignedBytes(bytes, 128);
		}
		else if (bytes <= 64 * 1024)
		{
			return _calculateAlignedBytes(bytes, 1024);
		}
		else if (bytes <= 256 * 1024)
		{
			return _calculateAlignedBytes(bytes, 8 * 1024);
		}
		else
		{
			assert(false);
		}

		return -1;
	}

	/// <summary>
	/// 计算bytes对应桶(链表)的索引
	/// </summary>
	/// <param name="bytes">字节数</param>
	/// <returns>返回bytes对应桶(链表)的索引</returns>
	static size_t calculateIndex(size_t bytes)
	{
		static size_t groupArray[4] = { 16, 56, 56, 56 };
		if (bytes <= 128)
		{
			return _calculateIndex(bytes, 3);
		}
		else if (bytes <= 1024)
		{
			return _calculateIndex(bytes - 128, 4) + groupArray[0];
		}
		else if (bytes <= 8 * 1024)
		{
			return _calculateIndex(bytes - 1024, 7) + groupArray[0] + groupArray[1];
		}
		else if (bytes <= 64 * 1024)
		{
			return _calculateIndex(bytes - 8 * 1024, 10) + groupArray[0] + groupArray[1] + groupArray[2];
		}
		else if (bytes <= 256 * 1024)
		{
			return _calculateIndex(bytes - 64 * 1024, 13) + groupArray[0] + groupArray[1] + groupArray[2] + groupArray[3];
		}
		else
		{
			assert(false);

		}
		return -1;
	}

	/// <summary>
	/// 根据对齐后的字节数,计算批量获取的碎片内存个数
	/// alignedBytes越大,预期获取到的个数就越少;
	///	alignedBytes越小,预期获取到的个数就越多;
	/// </summary>
	/// <param name="alignedBytes">对齐后的字节数</param>
	/// <returns></returns>
	static size_t calculateFetchFragmentedMemoryNum(size_t alignedBytes)
	{
		assert(alignedBytes <= MAX_ALLOCATE_BYTES);

		//对齐后的字节数越大,获取的碎片内存个数(fetchNum)就越少,
		//fetchNum的取值范围[2,512]
		size_t fetchNum = MAX_ALLOCATE_BYTES / alignedBytes; 
		if (fetchNum < 2)
		{
			fetchNum = 2;
		}
		else if (fetchNum > 512)
		{
			fetchNum = 512;
		}

		return fetchNum;
	}

	/// <summary>
	/// 根据对齐后的字节数计算分配多少个Page(一个Page8KB)
	/// </summary>
	/// <param name="alignedBytes">对齐后的字节数</param>
	/// <returns>返回需要分配多少个Page</returns>
	static size_t calculateFetchPageNum(size_t alignedBytes)
	{
		//申请一次会批量获取多少个碎片内存
		size_t fragmentedMemoryNum = calculateFetchFragmentedMemoryNum(alignedBytes);

		//批量获取到的碎片内存的总字节数
		size_t fragmentedMemoryNumBytes = fragmentedMemoryNum * alignedBytes;

		//计算需要申请的页数
		size_t pageNum = fragmentedMemoryNumBytes >>= PAGE_SHIFT;
		if (pageNum == 0)
		{
			pageNum = 1;
		}
		
		return pageNum;
	}
};

/// <summary>
/// Span节点类
/// </summary>
struct SpanNode
{
	/// <summary>
	/// 该Span内第一个page的页号,
	/// 即该Span内第一个page的起始地址>>13
	/// </summary>
	size_t _firstPageId = 0;

	/// <summary>
	/// 该Span内有多少个page
	/// </summary>
	size_t _pageNum = 0;


	/// <summary>
	/// 上一个SpanNode
	/// </summary>
	SpanNode* _prev = nullptr;

	/// <summary>
	/// 下一个SpanNode
	/// </summary>
	SpanNode* _next = nullptr;


	/// <summary>
	/// 有多少块碎片内存已经分配给ThreadCache
	/// </summary>
	size_t _useCount = 0;

	/// <summary>
	/// 表示该spanNode是否被CentralCache使用
	/// </summary>
	bool _isUse = false;

	/// <summary>
	/// 碎片内存链表
	/// </summary>
	FragmentedMemoryList _fragmentedMemoryList;

};

/// <summary>
/// Span链表类
/// </summary>
class SpanList
{
	/// <summary>
	/// 链表哨兵位头结点
	/// </summary>
	SpanNode* _head;

	/// <summary>
	/// 互斥锁
	/// </summary>
	std::mutex _mutex;

public:
	SpanList()
	{
		_head = new SpanNode;
		_head->_prev = _head;
		_head->_next = _head;
	}

	/// <summary>
	/// 申请锁
	/// </summary>
	void lock()
	{
		_mutex.lock();
	}

	/// <summary>
	/// 释放锁
	/// </summary>
	void unlock()
	{
		_mutex.unlock();
	}


	/// <summary>
	/// 在pos后面插入一个节点
	/// </summary>
	/// <param name="pos">插入位置</param>
	/// <param name="newNode">插入节点</param>
	void insert(SpanNode* pos, SpanNode* newNode)
	{
		assert(pos);
		assert(newNode);

		SpanNode* prev = pos->_prev;

		prev->_next = newNode;
		newNode->_prev = prev;

		newNode->_next = pos;
		pos->_prev = newNode;
	}

	/// <summary>
	/// 头插一个SpanNode
	/// </summary>
	/// <param name="newNode">待插入结点</param>
	void pushFront(SpanNode* newNode)
	{
		insert(begin(), newNode);
	}

	/// <summary>
	/// 删除pos位置的spanNode
	/// </summary>
	/// <param name="pos">需要删除的位置</param>
	void erase(SpanNode* pos)
	{
		assert(nullptr != pos); 
		assert(pos != _head);

		SpanNode* prev = pos->_prev;
		SpanNode* next = pos->_next;

		prev->_next = next;  
		next->_prev = prev;
	}

	/// <summary>
	/// 头删(获取)一个SpanNode
	/// </summary>
	/// <returns>返回一个SpanNode</returns>
	SpanNode* popFront()
	{
		assert(begin() != _head);

		SpanNode* front = begin();
		erase(begin());
		return front;
	}



	bool empty()
	{
		return begin() == end();
	}

	SpanNode* begin()
	{
		return _head->_next;
	}

	SpanNode* end()
	{
		return _head;
	}
};