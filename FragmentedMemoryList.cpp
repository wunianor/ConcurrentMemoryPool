#include "FragmentedMemoryList.h"


/// <summary>
/// 返回碎片内存节点存储的下一个内存节点地址的引用
/// </summary>
/// <param name="node">一块碎片内存的地址</param>
/// <returns>返回碎片内存节点存储的下一个内存节点地址的引用</returns>
void*& nextMemoryNode(void* node)
{
	assert(nullptr != node);

	//把碎片内存的前sizeof(void*)字节用来存放下一个结点的地址
	//*(void**)可以避免32位平台和64位平台指针大小不一样的问题
	//并且逻辑上也符合:node存储另外一个内存块的地址,那么node本身也是一个二级指针
	return *((void**)node);
}



/// <summary>
/// 头插一个碎片内存
/// </summary>
/// <param name="fragmentedMemory">碎片内存的地址</param>
void FragmentedMemoryList::push(void* fragmentedMemory)
{
	assert(nullptr != fragmentedMemory);

	nextMemoryNode(fragmentedMemory) = _head;
	_head = fragmentedMemory;

	++_size;
}

/// <summary>
/// 头插一段内存碎片,
/// 这段内存碎片的结构是链表
/// </summary>
/// <param name="begin">链表的第一个结点</param>
/// <param name="end">链表的最后一个结点</param>
/// <param name="size">链表的大小</param>
void FragmentedMemoryList::pushRange(void* begin, void* end, size_t size)
{
	assert(nullptr != begin);
	assert(nullptr != end);

	nextMemoryNode(end) = _head;
	_head = begin;

	_size += size;
}

/// <summary>
/// 从链表中获取(头删)一个碎片内存
/// </summary>
/// <returns>
///  若头结点为nullptr,返回nullptr;
///  否则返回一块碎片内存的地址
/// </returns>
void* FragmentedMemoryList::pop()
{
	if (nullptr == _head || size() == 0)
	{
		return nullptr;
	}

	void* fragmentedMemory = _head;
	_head = nextMemoryNode(_head);

	--_size;

	return fragmentedMemory;
}

/// <summary>
/// 从链表中头删(获取)一定数量的碎片内存,
/// 并且end->next会置为nullptr
/// </summary>
/// <param name="expectedNum">期望头删(获取)的数量</param>
/// <param name="start">输出型参数,获取到的链表的第一个结点</param>
/// <param name="end">输出型参数,获取到的链表的最后一个结点</param>
/// <returns>返回实际获取到的数量</returns>
size_t FragmentedMemoryList::popRange(size_t expectedNum, void*& start, void*& end)
{
	assert(expectedNum > 0);

	start = _head;
	end = _head;

	if (nullptr == _head)
	{
		return 0;
	}

	size_t actualNum = 1; //实际获取到的个数

	while (actualNum < expectedNum && nullptr != nextMemoryNode(end))
	{
		end = nextMemoryNode(end);
		++actualNum;
	}

	_head = nextMemoryNode(end);//更新链表头结点

	nextMemoryNode(end) = nullptr;//将end->next置为nullptr

	_size -= actualNum; //更新链表内碎片内存的数量

	return actualNum;

}

/// <summary>
/// 获取链表是否为空
/// </summary>
/// <returns>为空,返回true;否则,返回false</returns>
bool FragmentedMemoryList::empty()
{
	return nullptr == _head;
}

/// <summary>
/// 将对象重置为空状态:将头指针置为 nullptr,大小置为 0,并将下次从CentralCache获取碎片化内存的计数器重置为 1
/// </summary>
void FragmentedMemoryList::setEmpty()
{
	_head = nullptr;
	_size = 0;
	_nextFetchFragmentedMemoryNumFromCentralCache = 1;
}

/// <summary>
/// 获取链表内碎片内存的数量
/// </summary>
/// <returns>返回链表内碎片内存的数量</returns>
size_t FragmentedMemoryList::size()
{
	return _size;
}

/// <summary>
/// 获取下一次从CentralCache批量获取碎片内存的数量的引用
/// </summary>
/// <returns>返回下一次从CentralCache批量获取碎片内存的数量的引用</returns>
size_t& FragmentedMemoryList::nextFetchFragmentedMemoryNumFromCentralCache()
{
	return _nextFetchFragmentedMemoryNumFromCentralCache;
}
