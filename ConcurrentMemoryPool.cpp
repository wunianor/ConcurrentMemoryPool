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
	assert(nullptr != TLSThreadCache);

	size_t pageId = ((size_t)ptr) >> PAGE_SHIFT;//计算ptr所在的page的id

	//pageId映射SpanNode的数据结构是读写分离的
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
