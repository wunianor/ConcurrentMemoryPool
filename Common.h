#pragma once

//transcode to utf8

#include <iostream>
#include <unordered_map>
#include <exception>
#include <mutex>
#include <atomic>


#include <cstdlib>
#include <cassert>
#include <errno.h>


#if defined(_WIN32)
	#include <windows.h>
#elif defined(__unix__) || defined(__APPLE__) 
	#include <unistd.h>
	#include <sys/mman.h>
#else
	#error "unknown platform"
#endif

#include "FragmentedMemoryList.h"


#define BAD_ALLOC 1
#define BAD_FREE 2

//#define DEBUG


/// <summary>
/// 碎片内存链表的个数
/// </summary>
const size_t FRAMENTED_MEMORY_LIST_NUM = 208;

/// <summary>
/// 通过threadCache最大可申请内存->256KB
/// </summary>
const size_t THREAD_CACHE_MAX_ALLOCATE_BYTES = 256 * 1024; 

/// <summary>
/// PageCache中spanList的个数,
/// 同时也是PageCache能管理的spanNode的最大_pageNum(含有page的数量超过此值的spanNode,不由PageCache管理)
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

