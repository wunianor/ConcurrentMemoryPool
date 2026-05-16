#pragma once
//transcode to utf8
#include "Common.h"

/// <summary>
/// 计算工具类
/// </summary>
class CalculateTool
{
	/// <summary>
	/// 计算对于bytes对齐之后的字节数
	/// </summary>
	/// <param name="bytes">字节数</param>
	/// <param name="alignedSize">对齐字节数</param>
	/// <returns>返回对于bytes对齐之后的字节数</returns>
	static size_t _calculateAlignedBytes(size_t bytes, size_t alignedSize);

	/// <summary>
	/// 计算bytes字节数在哪个桶(链表)内,返回其索引
	/// </summary>
	/// <param name="bytes">字节数</param>
	/// <param name="alignedShift">log2(对齐字节数alignedSize)</param>
	/// <returns>返回bytes对应桶(链表)的索引</returns>
	static size_t _calculateIndex(size_t bytes, size_t alignedShift);

public:
	/*
		控制浪费在10%
		[1,128]                   8byte对齐               对应碎片内存链表数组的[0,15]
		[128+1,1024]              16byte对齐              对应碎片内存链表数组的[16,71]
		[1024+1,8*1024]           128byte对齐             对应碎片内存链表数组的[72,127]
		[8*1024+1,64*1024]        1024byte对齐            对应碎片内存链表数组的[128,183]
		[64*1024+1,256*1024]      8*1024byte对齐          对应碎片内存链表数组的[184,207]
	*/


	/// <summary>
	/// 计算对于bytes对齐之后的字节数
	/// </summary>
	/// <param name="bytes">字节数</param>
	/// <returns>返回对于bytes对齐之后的字节数</returns>
	static size_t calculateAlignedBytes(size_t bytes);

	/// <summary>
	/// 计算bytes对应桶(链表)的索引
	/// </summary>
	/// <param name="bytes">字节数</param>
	/// <returns>返回bytes对应桶(链表)的索引</returns>
	static size_t calculateIndex(size_t bytes);

	/// <summary>
	/// 根据对齐后的字节数,计算批量获取的碎片内存个数
	/// alignedBytes越大,预期获取到的个数就越少;
	///	alignedBytes越小,预期获取到的个数就越多;
	/// </summary>
	/// <param name="alignedBytes">对齐后的字节数</param>
	/// <returns></returns>
	static size_t calculateFetchFragmentedMemoryNum(size_t alignedBytes);

	/// <summary>
	/// 根据对齐后的字节数计算分配多少个Page(一个Page8KB)
	/// </summary>
	/// <param name="alignedBytes">对齐后的字节数</param>
	/// <returns>返回需要分配多少个Page</returns>
	static size_t calculateFetchPageNum(size_t alignedBytes);
};
