#pragma once


#include "Common.h"

/// <summary>
/// 向系统申请一块内存
/// </summary>
/// <param name="size">内存大小</param>
/// <returns>
/// 申请成功,返回内存起始地址;
/// 申请失败,返回nullptr
/// </returns>
void* systemMemoryAlloc(size_t size);



/// <summary>
/// 释放由 systemMemoryAlloc() 申请的内存
/// </summary>
/// <param name="ptr">需要释放的内存块的起始地址</param>
/// <param name="size">内存块的大小</param>
/// <returns>
/// 释放成功返回 true(空指针释放视为成功);
/// 释放失败返回 false
/// </returns>
bool systemMemoryFree(void* ptr, size_t size);
