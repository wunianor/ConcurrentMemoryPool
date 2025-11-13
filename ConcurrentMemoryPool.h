
#include "Common.h"
#include "ThreadCache.h"
#include "PageCache.h"





/// <summary>
/// 高并发申请内存
/// </summary>
/// <param name="size">申请内存的大小</param>
/// <returns>返回申请到的指针</returns>
void* ConcurrentAlloc(size_t size);



/// <summary>
/// 高并发释放内存
/// </summary>
/// <param name="ptr">释放的内存的地址</param>
void ConcurrentFree(void* ptr);