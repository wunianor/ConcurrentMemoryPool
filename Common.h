#pragma once

#include <cstdlib>
#include <cassert>

#include <exception>

#include "FragmentedMemoryList.h"


class CalculcateTool
{
	/// <summary>
	/// 计算对于bytes对齐之后的字节数
	/// </summary>
	/// <param name="bytes">字节数</param>
	/// <param name="alignedSize">对齐字节数</param>
	/// <returns>返回对于bytes对齐之后的字节数</returns>
	static size_t _calculateAlignedSize(size_t bytes, size_t alignedSize)
	{
		//等价于((bytes-1)/aligendSize+1)*aligendSize;
		return (bytes - 1 + alignedSize) & (~(alignedSize - 1));
	}

	/// <summary>
	/// 计算bytes字节数在哪个桶(链表)内,返回其索引
	/// </summary>
	/// <param name="bytes">字节数</param>
	/// <param name="alignedShift">log2(对齐字节数alignedSize)</param>
	/// <returns>返回bytes对应桶(链表)的索引</returns>
	static size_t _calculateIndex(size_t bytes, size_t alignedShift)
	{
		return ((bytes + (1 << alignedShift) - 1) >> alignedShift) - 1;
	}

public:
	/*
		大约会有10%的浪费
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
	static size_t calculateAlignedSize(size_t bytes)
	{
		if (bytes <= 128)
		{
			return _calculateAlignedSize(bytes, 8);
		}
		else if (bytes <= 1024)
		{
			return _calculateAlignedSize(bytes, 16);
		}
		else if (bytes <= 8 * 1024)
		{
			return _calculateAlignedSize(bytes, 128);
		}
		else if (bytes <= 64 * 1024)
		{
			return _calculateAlignedSize(bytes, 1024);
		}
		else if (bytes <= 256 * 1024)
		{
			return _calculateAlignedSize(bytes, 8 * 1024);
		}
		else
		{
			assert(false);
		}

		return -1;
	}

	/// <summary>
	/// 计算bytes对应桶(链表)的索引
	/// </summary>
	/// <param name="bytes">字节数</param>
	/// <returns>返回bytes对应桶(链表)的索引</returns>
	static size_t calculateIndex(size_t bytes)
	{
		static size_t groupArray[4] = { 16, 56, 56, 56 };
		if (bytes <= 128)
		{
			return _calculateIndex(bytes, 3);
		}
		else if (bytes <= 1024)
		{
			return _calculateIndex(bytes - 128, 4) + groupArray[0];
		}
		else if (bytes <= 8 * 1024)
		{
			return _calculateIndex(bytes - 1024, 7) + groupArray[0] + groupArray[1];
		}
		else if (bytes <= 64 * 1024)
		{
			return _calculateIndex(bytes - 8 * 1024, 10) + groupArray[0] + groupArray[1] + groupArray[2];
		}
		else if (bytes <= 256 * 1024)
		{
			return _calculateIndex(bytes - 64 * 1024, 13) + groupArray[0] + groupArray[1] + groupArray[2] + groupArray[3];
		}
		else
		{
			assert(false);

		}
		return -1;
	}

};