#include "ConcurrentMemoryPool.h"



void* ConcurrentAlloc(size_t size)
{
	void* ptr = nullptr;
	if (size > THREAD_CACHE_MAX_ALLOCATE_BYTES) //申请的内存大小大于通过threadCache最大可申请内存
	{
		size_t alignedBytes = CalculateTool::calculateAlignedBytes(size); //计算根据size对齐后的alignedBytes
		size_t pageNum = alignedBytes >> PAGE_SHIFT;//计算需要多少个page

		PageCache::getInstance()->lock();
		SpanNode* spanNode = PageCache::getInstance()->fetchPageNumSpan(pageNum); //获取一个含pageNum个page的spanNode
		spanNode->_fragmentedMemorySize = size;
		PageCache::getInstance()->unlock();

		ptr = (void*)((spanNode->_firstPageId) << PAGE_SHIFT); //计算内存块起始地址
	}
	else
	{
		//通过ThreadCache来申请
		if (nullptr == TLSThreadCache)
		{
			TLSThreadCache = TLSthreadCacheObjPool.New();
		}
		ptr = TLSThreadCache->allocate(size);
	}

	return ptr;
}



void ConcurrentFree(void* ptr)
{
	if(nullptr == ptr) return;
	assert(nullptr != TLSThreadCache);

	size_t pageId = ((size_t)ptr) >> PAGE_SHIFT;//计算ptr所在的page的id

	//pageId映射SpanNode的数据结构是不存在同时对一个地方进行读写操作的
	//什么情况下会读:
	//	1.释放内存的时候
	//什么情况下会写:
	//	1.申请内存时,在把内存交给用户之前(spanNode都没给用户,用户怎么会释放其包含的内存)
	//	2.释放内存合并SpanNode时,把SpanNode放入pageCache层前(该spanNode的所有内存都被用户释放了,正常情况下用户不会再释放该spanNode内的内存了)
	SpanNode* spanNode = PageCache::getInstance()->getPageIdMapSpanNode(pageId);//求pageId对应的spanNode

	size_t bytes = spanNode->_fragmentedMemorySize;//计算ptr内存块的大小

	if (bytes > THREAD_CACHE_MAX_ALLOCATE_BYTES) //如果ptr内存块的大小大于通过threadCache最大可申请内存
	{
		PageCache::getInstance()->lock();
		PageCache::getInstance()->freeSpanNodeToPageCache(spanNode);
		PageCache::getInstance()->unlock();
	}
	else
	{
		TLSThreadCache->deallocate(ptr, bytes);
	}
}
