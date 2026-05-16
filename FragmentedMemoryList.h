#pragma once

#include "Common.h"



/// <summary>
/// 返回碎片内存节点存储的下一个内存节点地址的引用
/// </summary>
/// <param name="node">一块碎片内存的地址</param>
/// <returns>返回碎片内存节点存储的下一个内存节点地址的引用</returns>
void*& nextMemoryNode(void* node);



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
	/// 链表内碎片内存的数量
	/// </summary>
	size_t _size = 0;

	/// <summary>
	/// 该链表下一次从CentralCache批量获取碎片内存的数量
	/// </summary>
	size_t _nextFetchFragmentedMemoryNumFromCentralCache = 1;

public:
	
	/// <summary>
	/// 头插一个碎片内存
	/// </summary>
	/// <param name="fragmentedMemory">碎片内存的地址</param>
	void push(void* fragmentedMemory);

	/// <summary>
	/// 头插一段内存碎片,
	/// 这段内存碎片的结构是链表
	/// </summary>
	/// <param name="begin">链表的第一个结点</param>
	/// <param name="end">链表的最后一个结点</param>
	/// <param name="size">链表的大小</param>
	void pushRange(void* begin, void* end, size_t size);

	/// <summary>
	/// 从链表中获取(头删)一个碎片内存
	/// </summary>
	/// <returns>
	///  若头结点为nullptr,返回nullptr;
	///  否则返回一块碎片内存的地址
	/// </returns>
	void* pop();

	/// <summary>
	/// 从链表中头删(获取)一定数量的碎片内存,
	/// 并且end->next会置为nullptr
	/// </summary>
	/// <param name="expectedNum">期望头删(获取)的数量</param>
	/// <param name="start">输出型参数,获取到的链表的第一个结点</param>
	/// <param name="end">输出型参数,获取到的链表的最后一个结点</param>
	/// <returns>返回实际获取到的数量</returns>
	size_t popRange(size_t expectedNum, void*& start, void*& end);

	/// <summary>
	/// 获取链表是否为空
	/// </summary>
	/// <returns>为空,返回true;否则,返回false</returns>
	bool empty();

	/// <summary>
	/// 将对象重置为空状态:将头指针置为 nullptr,大小置为 0,并将下次从CentralCache获取碎片化内存的计数器重置为 1
	/// </summary>
	void setEmpty();

	/// <summary>
	/// 获取链表内碎片内存的数量
	/// </summary>
	/// <returns>返回链表内碎片内存的数量</returns>
	size_t size();

	/// <summary>
	/// 获取下一次从CentralCache批量获取碎片内存的数量的引用
	/// </summary>
	/// <returns>返回下一次从CentralCache批量获取碎片内存的数量的引用</returns>
	size_t& nextFetchFragmentedMemoryNumFromCentralCache();
};
