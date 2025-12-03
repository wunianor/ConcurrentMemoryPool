#include "systemMemoryAlloc.h"

/// <summary>
/// 向系统申请一块内存
/// </summary>
/// <param name="size">内存大小</param>
/// <returns>
/// 申请成功,返回内存起始地址;
/// 申请失败,返回nullptr
/// </returns>
void* systemMemoryAlloc(size_t size)
{
	void* ptr = nullptr;

#ifdef _WIN32
	ptr = VirtualAlloc(
		NULL,                          // 系统自动分配地址（天然页对齐）
		size,		                   // 申请的内存大小
		MEM_RESERVE | MEM_COMMIT,      // 核心参数：保留地址空间+提交物理内存
		PAGE_READWRITE                 // 仅读写权限（安全无执行）
	);

	if (NULL == ptr)
	{
		ptr = nullptr;
	}

#elif defined(__unix__) || defined(__APPLE__) 
	ptr = mmap(
		NULL,                          // 内核自动分配地址（天然页对齐）
		size,		                   // 申请的内存大小
		PROT_READ | PROT_WRITE,        // 仅读写权限（安全无执行）
		MAP_PRIVATE | MAP_ANONYMOUS,   // 匿名+私有映射（无文件IO，进程独占）
		-1,                            // 匿名映射无需文件描述符
		0                              // 偏移量必须为 0
	);

	if (MAP_FAILED == ptr)
	{
		ptr = nullptr;
	}

#else
#error "unknown platform"
#endif

	return ptr;
}



/// <summary>
/// 释放由 systemMemoryAlloc() 申请的内存
/// </summary>
/// <param name="ptr">需要释放的内存块的起始地址</param>
/// <param name="size">内存块的大小</param>
/// <returns>
/// 释放成功返回 true(空指针释放视为成功);
/// 释放失败返回 false
/// </returns>
bool systemMemoryFree(void* ptr, size_t size)
{
	// 释放空指针视为成功
	if (ptr == nullptr)
	{
		return true;
	}

#ifdef _WIN32
	// Windows 平台：使用 VirtualFree 释放，对应 VirtualAlloc 的分配
	// 注意：释放时 size 设为 0，MEM_RELEASE 会同时释放保留地址空间和提交的物理内存
	BOOL releaseResult = VirtualFree(
		ptr,
		0,                  // 释放整个内存区域时，size 必须设为 0
		MEM_RELEASE         // 核心参数：释放保留+提交的内存（与分配时的 MEM_RESERVE|MEM_COMMIT 对应）
	);

	// BOOL 本质是 int，TRUE=1，FALSE=0，转换为 bool 返回
	return (releaseResult != FALSE);

#elif defined(__unix__) || defined(__APPLE__)
	// Unix/Linux/macOS 平台：使用 munmap 释放，对应 mmap 的分配
	// 关键：size 必须与 mmap 时的 size 完全一致，否则会导致释放失败或内存 corruption
	int unmapResult = munmap(ptr, size);

	// munmap 返回 0 表示成功，-1 表示失败
	return (unmapResult == 0);

#else
#error "unknown platform"
#endif
}