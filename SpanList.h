#pragma once

#include "Common.h"
#include "FixedSizeMemoryPool.hpp"


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
	/// 每一个碎片内存的大小
	/// </summary>
	size_t _fragmentedMemorySize = 0;

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
	SpanList();



	/// <summary>
	/// 申请锁
	/// 
	/// 注意:
	/// .h和.cpp分离会导致lock()不是inline的,对性能影响不大;
	/// 但是,如果是inline的话,vs2022的性能监视器显示不出该函数的总执行时间
	/// </summary>
	void lock();

	/// <summary>
	/// 释放锁
	///
	/// 注意:
	/// .h和.cpp分离会导致unlock()不是inline的,对性能影响不大;
	/// 但是,如果是inline的话,vs2022的性能监视器显示不出该函数的总执行时间
	/// </summary>
	void unlock();



	/// <summary>
	/// 在pos后面插入一个节点
	/// </summary>
	/// <param name="pos">插入位置</param>
	/// <param name="newNode">插入节点</param>
	void insert(SpanNode* pos, SpanNode* newNode);

	/// <summary>
	/// 头插一个SpanNode
	/// </summary>
	/// <param name="newNode">待插入结点</param>
	void pushFront(SpanNode* newNode);



	/// <summary>
	/// 删除pos位置的spanNode
	/// </summary>
	/// <param name="pos">需要删除的位置</param>
	void erase(SpanNode* pos);

	/// <summary>
	/// 头删(获取)一个SpanNode
	/// </summary>
	/// <returns>返回一个SpanNode</returns>
	SpanNode* popFront();




	/// <summary>
	/// 获取对象是否为空
	/// </summary>
	/// <returns>
	/// true,表示为空;
	/// false,表示不为空
	/// </returns>
	bool empty();

	/// <summary>
	/// 返回begin()迭代器
	/// </summary>
	/// <returns>返回begin()迭代器</returns>
	SpanNode* begin();

	/// <summary>
	/// 返回end()迭代器
	/// </summary>
	/// <returns>返回end()迭代器</returns>
	SpanNode* end();
};

/// <summary>
/// spanNode对象池
/// </summary>
extern FixedSizeMemoryPool<SpanNode> spanNodeObjPool;