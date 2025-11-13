#include "SpanList.h"


FixedSizeMemoryPool<SpanNode> spanNodeObjPool;

SpanList::SpanList()
{
	_head = spanNodeObjPool.New();
	_head->_prev = _head;
	_head->_next = _head;
}


/// <summary>
/// 申请锁
/// 
/// 注意:
/// .h和.cpp分离会导致lock()不是inline的,对性能影响不大;
/// 但是,如果是inline的话,vs2022的性能监视器显示不出该函数的总执行时间
/// </summary>
void SpanList::lock() 
{
	_mutex.lock();
}


/// <summary>
/// 释放锁
/// 
/// 注意:
/// .h和.cpp分离会导致unlock()不是inline的,对性能影响不大;
/// 但是,如果是inline的话,vs2022的性能监视器显示不出该函数的总执行时间
/// </summary>
void SpanList::unlock()
{
	_mutex.unlock();
}


/// <summary>
/// 在pos后面插入一个节点
/// </summary>
/// <param name="pos">插入位置</param>
/// <param name="newNode">插入节点</param>
void SpanList::insert(SpanNode* pos, SpanNode* newNode)
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
void SpanList::pushFront(SpanNode* newNode)
{
	insert(begin(), newNode);
}



/// <summary>
/// 删除pos位置的spanNode
/// </summary>
/// <param name="pos">需要删除的位置</param>
void SpanList::erase(SpanNode* pos)
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
SpanNode* SpanList::popFront()
{
	assert(begin() != _head);

	SpanNode* front = begin();
	erase(begin());
	return front;
}


/// <summary>
	/// 获取对象是否为空
	/// </summary>
	/// <returns>
	/// true,表示为空;
	/// false,表示不为空
	/// </returns>
bool SpanList::empty()
{
	return begin() == end();
}

/// <summary>
/// 返回begin()迭代器
/// </summary>
/// <returns>返回begin()迭代器</returns>
SpanNode* SpanList::begin()
{
	return _head->_next;
}

/// <summary>
/// 返回end()迭代器
/// </summary>
/// <returns>返回end()迭代器</returns>
SpanNode* SpanList::end()
{
	return _head;
}
