#pragma once

#include "Common.h"
#include "SystemMemoryAlloc.h"

template<class T>
class FixedSizeMemoryPool
{
	/// <summary>
	/// 当前连续未使用内存(即大块内存)的起始地址
	/// </summary>
	char* _curContiguousMemory = nullptr;

	/// <summary>
	/// 当前整块连续未使用内存(即大块内存)的大小,
	/// </summary>
	size_t _curContiguousMemorySize = 0;

	/// <summary>
	/// 碎片内存链表,
	/// 还给定长内存池的碎片内存 链在这个链表上
	/// </summary>
	FragmentedMemoryList _fragmentedMemoryList;

public:
	/// <summary>
	/// 从定长内存池中new一个T类型对象
	/// </summary>
	/// <returns>返回一个T类型对象指针</returns>
	T* New()
	{
		T* obj = nullptr;

		if (!_fragmentedMemoryList.empty()) //优先使用碎片内存空间链表上的空间
		{
			obj = (T*)(_fragmentedMemoryList.pop());
		}
		else 
		{
			if (_curContiguousMemorySize < sizeof(T)) //如果连续内存空间的剩余大小 小于 T类型大小
			{
				size_t allocMemorySize = max((1 << PAGE_SHIFT), (sizeof(T) / (1 << PAGE_SHIFT) + 1) * (1 << PAGE_SHIFT));
				_curContiguousMemory = (char*)systemMemoryAlloc(allocMemorySize);
				if (nullptr == _curContiguousMemory) //如果申请内存失败
				{
					throw std::bad_alloc();
				}

				_curContiguousMemorySize = allocMemorySize;
			}

			obj = (T*)(_curContiguousMemory);
			//如果sizeof(T)<sizeof(void*),
			//则分配sizeof(void*)的大小,
			//确保_fragmentedMemoryList中的结点的地址能够存的下
			size_t objMemorySize = max(sizeof(T), sizeof(void*)); 
			_curContiguousMemory += objMemorySize;   //更新连续内存空间的起始地址
			_curContiguousMemorySize -= objMemorySize; //更新剩余连续内存空间的大小
		}

		//使用定位new初始化T类型对象
		new (obj) T;

		return obj;
	}

	/// <summary>
	/// 释放一个T类型对象
	/// </summary>
	/// <param name="obj">T类型对象的指针</param>
	void Delete(T* obj)
	{
		//调用T类型对象析构函数
		obj->~T();

		_fragmentedMemoryList.push(obj);
	}
};