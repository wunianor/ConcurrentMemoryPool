#pragma once

/// <summary>
/// 碎片内存链表类
/// </summary>
class FragmentedMemoryList
{
	/// <summary>
	/// 碎片内存链表的头结点
	/// </summary>
	void* _head = nullptr;

	/// <summary>
	/// 返回碎片内存节点存储的下一个内存节点地址的引用
	/// </summary>
	/// <param name="node">一块碎片内存的地址</param>
	/// <returns>返回碎片内存节点存储的下一个内存节点地址的引用</returns>
	void*& _nextMemoryNode(void* node)
	{
		//把碎片内存的前sizeof(void*)字节用来存放下一个结点的地址
		//*(void**)可以避免32位平台和64位平台指针大小不一样的问题
		return *((void**)node); 
	}

public:

	
	/// <summary>
	/// 向链表中头插一个碎片内存
	/// </summary>
	/// <typeparam name="T">原先使用碎片内存的类型</typeparam>
	/// <param name="fragmentedMemory">碎片内存的地址</param>
	/// <returns>
	///		插入成功,返回true;
	///		插入失败,返回false(碎片内存空间小于指针大小时插入失败)
	/// </returns>
	template<class T>
	bool push(T* fragmentedMemory)
	{
		if (sizeof(T) < sizeof(void*))
		{
			return false;
		}

		//*(void**)可以避免32位平台和64位平台指针大小不一样的问题,
		//把碎片内存的前sizeof(void*)字节用来存放下一个结点的地址
		_nextMemoryNode(fragmentedMemory) = _head;
		_head = fragmentedMemory;

		return true;
	}

	/// <summary>
	/// 从链表中头删(获取)一个碎片内存
	/// </summary>
	/// <returns>若头结点为nullptr,返回nullptr;否则返回一块碎片内存的地址</returns>
	void* pop()
	{
		if(nullptr == _head)
		{
			return nullptr;
		}

		void* fragmentedMemory = _head;
		_head = _nextMemoryNode(_head);

		return fragmentedMemory;
	}

	/// <summary>
	/// 获取链表是否为空
	/// </summary>
	/// <returns>为空,返回true;否则,返回false</returns>
	bool empty()
	{
		if (nullptr == _head) return true;
		return false;
	}
};