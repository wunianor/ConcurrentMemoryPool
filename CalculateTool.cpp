#include "CalculateTool.h"



/// <summary>
/// 计算对于bytes对齐之后的字节数
/// </summary>
/// <param name="bytes">字节数</param>
/// <param name="alignedSize">对齐字节数</param>
/// <returns>返回对于bytes对齐之后的字节数</returns>
size_t CalculateTool::_calculateAlignedBytes(size_t bytes, size_t alignedSize)
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
size_t CalculateTool::_calculateIndex(size_t bytes, size_t alignedShift)
{
	return ((bytes + (1LL << alignedShift) - 1) >> alignedShift) - 1;
}



/// <summary>
/// 计算对于bytes对齐之后的字节数
/// </summary>
/// <param name="bytes">字节数</param>
/// <returns>返回对于bytes对齐之后的字节数</returns>
size_t CalculateTool::calculateAlignedBytes(size_t bytes)
{
	if (bytes <= 128)
	{
		return _calculateAlignedBytes(bytes, 8);
	}
	else if (bytes <= 1024)
	{
		return _calculateAlignedBytes(bytes, 16);
	}
	else if (bytes <= 8 * 1024)
	{
		return _calculateAlignedBytes(bytes, 128);
	}
	else if (bytes <= 64 * 1024)
	{
		return _calculateAlignedBytes(bytes, 1024);
	}
	else if (bytes <= 256 * 1024)
	{
		return _calculateAlignedBytes(bytes, 8 * 1024);
	}
	else
	{
		return _calculateAlignedBytes(bytes, 1 << PAGE_SHIFT);
	}
}




/// <summary>
/// 计算bytes对应桶(链表)的索引
/// </summary>
/// <param name="bytes">字节数</param>
/// <returns>返回bytes对应桶(链表)的索引</returns>
size_t CalculateTool::calculateIndex(size_t bytes)
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



/// <summary>
/// 根据对齐后的字节数,计算批量获取的碎片内存个数
/// alignedBytes越大,预期获取到的个数就越少;
///	alignedBytes越小,预期获取到的个数就越多;
/// </summary>
/// <param name="alignedBytes">对齐后的字节数</param>
/// <returns></returns>
size_t CalculateTool::calculateFetchFragmentedMemoryNum(size_t alignedBytes)
{
	assert(alignedBytes <= THREAD_CACHE_MAX_ALLOCATE_BYTES);

	//对齐后的字节数越大,获取的碎片内存个数(fetchNum)就越少,
	//fetchNum的取值范围[2,512]
	size_t fetchNum = THREAD_CACHE_MAX_ALLOCATE_BYTES / alignedBytes;
	if (fetchNum < 2)
	{
		fetchNum = 2;
	}
	else if (fetchNum > 512)
	{
		fetchNum = 512;
	}

	return fetchNum;
}


/// <summary>
/// 根据对齐后的字节数计算分配多少个Page(一个Page8KB)
/// </summary>
/// <param name="alignedBytes">对齐后的字节数</param>
/// <returns>返回需要分配多少个Page</returns>
size_t CalculateTool::calculateFetchPageNum(size_t alignedBytes)
{
	//申请一次会批量获取多少个碎片内存
	size_t fragmentedMemoryNum = calculateFetchFragmentedMemoryNum(alignedBytes);

	//批量获取到的碎片内存的总字节数
	size_t fragmentedMemoryNumBytes = fragmentedMemoryNum * alignedBytes;

	//计算需要申请的页数
	size_t pageNum = fragmentedMemoryNumBytes >> PAGE_SHIFT;
	if (pageNum == 0)
	{
		pageNum = 1;
	}

	return pageNum;
}