#include <iostream>
#include <ctime>
#include <vector>
#include <thread>
using std::cout;
using std::endl;

#include "Common.h"
#include "FixedSizeMemoryPool.hpp"
#include "ThreadCache.h"
#include "ConcurrentMemoryPool.h"

using namespace std;


struct TreeNode
{
	int val = 100;
	TreeNode* left = nullptr;
	TreeNode* right = nullptr;
	TreeNode* right1 = nullptr;
	TreeNode* right2 = nullptr;

};

void CompareNewAndFixedSizeMemoryPool()
{
	int round = 10;
	int nodeNum = 100000;

	std::vector<TreeNode*> v1, v2;
	v1.resize(nodeNum);
	v2.resize(nodeNum);

	clock_t begin, end;

	FixedSizeMemoryPool<TreeNode> memPool;

	begin = clock();
	for (int i = 0; i < round; ++i)
	{
		for (int j = 0; j < nodeNum; ++j)
		{
			v1[j] = new TreeNode();
		}
		for (int j = 0; j < nodeNum; ++j)
		{
			delete v1[j];
		}
	}
	end = clock();

	cout << "new:" << end - begin << endl;

	begin = clock();
	for (int i = 0; i < round; ++i)
	{
		for (int j = 0; j < nodeNum; ++j)
		{
			v2[j] = memPool.New();
			//cout << "v2-" << j << ":" << v2[j] << " v2[j]->val=" << v2[j]->val << endl;
		}
		for (int j = 0; j < nodeNum; ++j)
		{
			memPool.Delete(v2[j]);
		}
	}
	end = clock();

	cout << "FixedSizeMemoryPool:" << end - begin << endl;
}




void allocate1()
{
	TLSThreadCache = new ThreadCache();
	cout << std::this_thread::get_id() << ":" << TLSThreadCache << endl;

	void* ptr1 = 0;
	size_t size1 = 0;
	for (int i = 1; i <= 1024*127; ++i)
	{
		int size = 8;
	
		if (i == 1024 * 127)
		{
			void* ptr = TLSThreadCache->allocate(size);
		}
		else
		{
			void* ptr = TLSThreadCache->allocate(size);
		}
	}



	TLSThreadCache->allocate(8);

}

void allocate2()
{
	TLSThreadCache = new ThreadCache();
	cout << std::this_thread::get_id() << ":" << TLSThreadCache << endl;
	for (int i = 0; i < 100000; ++i)
	{
		void* ptr = TLSThreadCache->allocate(rand() % 8 + 1);
	}

}


void testTLS()
{
	std::thread t1(allocate1);
	//std::thread t2(allocate2);


	t1.join();
	//t2.join();
}

#include <windows.h>
#include <random>
#include <chrono>

void testMallocVsVirtualalloc()
{
	clock_t begin, end;

	begin = clock();
	for (int i = 0; i < 10000; ++i)
	{
		malloc((1<<12));
	}
	end = clock();
	cout << "malloc:" << end - begin << endl;
	begin = clock();
	for (int i = 0; i < 10000; ++i)
	{
		VirtualAlloc(nullptr, (1 << 12), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	}
	end = clock();
	cout << "VirtualAlloc:" << end - begin << endl;
}

std::mutex pmutex;

void testAllocateAndDeallocate()
{
	srand(time(nullptr));

	int n = 10000;

	vector<size_t> vz(n);
	for (auto& e : vz)
	{
		e = rand() % (256 * 1024);
	}

	vector<pair<void*, size_t>> v;

	TLSThreadCache = new ThreadCache();

	int roundNum = 5;

	long long a = 0, b = 0;

	vector<void*> v1;

	long long newa = 0, newb = 0;

	while (roundNum--)
	{
		for (int i = 0; i < n; ++i)
		{
			int size = vz[i];


			auto start = std::chrono::high_resolution_clock::now();
			void* ptr = TLSThreadCache->allocate(size);
			auto end= std::chrono::high_resolution_clock::now();

			auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
			a += duration.count();


			v.emplace_back(ptr, size);
		}

		int randFree = rand() % v.size();
		
		for (int i = 0; i < randFree; ++i)
		{
			//pmutex.lock();
			//std::cout << this_thread::get_id() << " 正在-释放" << v.back().second << "字节" << std::endl;
			//pmutex.unlock();

			auto start = std::chrono::high_resolution_clock::now();
			TLSThreadCache->deallocate(v[i].first, v[i].second);
			auto end = std::chrono::high_resolution_clock::now();

			auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
			b += duration.count();
		}
		


		/*for (int i = 0; i < rand(); ++i)
		{
			int size = rand() % 8 + 1;
			void* ptr = TLSThreadCache->allocate(size);
			v.emplace_back(ptr, size);
		}*/


		
		for (int i = 0; i < n; ++i)
		{
			auto start = std::chrono::high_resolution_clock::now();
			v1.emplace_back(malloc(vz[i]));
			auto end = std::chrono::high_resolution_clock::now();

			auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
			newa += duration.count();

			
		}


		for (int i=0;i< randFree;++i)
		{
			auto& e = v1[i];
			auto start = std::chrono::high_resolution_clock::now();
			free(e);
			auto end = std::chrono::high_resolution_clock::now();

			auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
			newb += duration.count();
		}

	
	}

	
	pmutex.lock();
	std::cout << this_thread::get_id()
		<< " 申请总时间:" << a
		<< "ns 平均:" << a * 1.0 / n
		<< "ns 释放总时间:" << b << "ns"
		<< " 平均:" << b * 1.0 / n << " ns"
		<<endl;
	pmutex.unlock();

	pmutex.lock();
	cout << this_thread::get_id() << " malloc time:" << newa
		<< "ns 平均malloc:" << newa * 1.0 / n
		<< "ns free:" << newb << "ns "
		<< "平均 free:" << newb * 1.0 / n<<"ns"
		<< endl;
	pmutex.unlock();

	//int size = 1;
	//void* ptr = TLSThreadCache->allocate(size);

	//TLSThreadCache->deallocate(ptr, size);
}

int main()
{
	
	//CompareNewAndFixedSizeMemoryPool();
	//testTLS();

	//testMallocVsVirtualalloc();

	thread t1(testAllocateAndDeallocate);
	/*thread t2(testAllocateAndDeallocate);
	thread t3(testAllocateAndDeallocate);
	thread t4(testAllocateAndDeallocate);
	thread t5(testAllocateAndDeallocate);*/


	t1.join();
	//t2.join();
	//t3.join();
	//t4.join();
	//t5.join();



	return 0;
}








//#include <iostream>
//#include <thread>
//#include <vector>
//#include <random>
//#include <chrono>
//#include <mutex>
//#include <atomic>
//#include <cassert>
//#include <cstring>
//#include <utility>
//
//
//
//// -----------------------------------------------------------------------------
//// 测试配置参数（可根据测试需求调整）
//// -----------------------------------------------------------------------------
//const size_t kThreadCount = 20;                // 高并发线程数（模拟多线程竞争）
//const size_t kOperateCountPerThread = 50000;  // 每个线程的申请/释放操作次数
//const size_t kMinAllocateBytes = 1;            // 最小申请字节数
//const size_t kMaxAllocateBytes = MAX_ALLOCATE_BYTES;  // 最大申请字节数
//const uint32_t kMagicNumber = 0x12345678;      // 内存数据校验魔数（检测内存篡改）
//const size_t kAlignBoundary = 8;               // 内存对齐检查边界（根据内存池实际规则调整）
//
//// 线程安全输出锁（避免多线程输出乱码）
//std::mutex g_outputMutex;
//// 测试结果标记（原子变量保证线程安全）
//std::atomic<bool> g_testSuccess(true);
//
//// -----------------------------------------------------------------------------
//// 线程测试函数：模拟多线程并发申请/释放内存
//// -----------------------------------------------------------------------------
//void MemoryPoolTestThread(size_t threadId) {
//	// 1. 初始化线程局部ThreadCache（必须在每个线程内初始化）
//	if (TLSThreadCache == nullptr) {
//		TLSThreadCache = new ThreadCache;
//		if (TLSThreadCache == nullptr) {
//			std::lock_guard<std::mutex> lock(g_outputMutex);
//			std::cerr << "[ERROR] Thread " << threadId << " failed to create ThreadCache!" << std::endl;
//			g_testSuccess = false;
//			return;
//		}
//	}
//
//	// 2. 初始化线程私有随机数生成器（避免多线程随机数竞争）
//	std::random_device rd;
//	auto seed = rd() ^ (static_cast<unsigned int>(std::chrono::high_resolution_clock::now().time_since_epoch().count()) << threadId);
//	std::mt19937 gen(seed);
//	std::uniform_int_distribution<size_t> bytesDist(kMinAllocateBytes, kMaxAllocateBytes);
//	std::uniform_int_distribution<int> opDist(0, 3);  // 0-2: 申请后缓存, 3: 随机释放历史内存
//
//	// 存储已申请的内存块（指针+对应的字节数，用于后续释放）
//	std::vector<std::pair<void*, size_t>> allocatedPtrs;
//	allocatedPtrs.reserve(kOperateCountPerThread / 2);  // 预留空间，减少vector扩容
//
//	try {
//		for (size_t i = 0; i < kOperateCountPerThread; ++i) {
//			// 随机选择操作类型：申请新内存 或 释放历史内存
//			int opType = opDist(gen);
//
//			if (opType != 3) {
//				// 3. 随机申请内存（覆盖不同大小场景）
//				size_t reqBytes = bytesDist(gen);
//				void* ptr = TLSThreadCache->allocate(reqBytes);
//
//				// 检查申请结果有效性
//				if (ptr == nullptr) {
//					std::lock_guard<std::mutex> lock(g_outputMutex);
//					std::cout << "[ERROR] Thread " << threadId << " allocate failed! Bytes: " << reqBytes
//						<< ", Operate: " << i << std::endl;
//					g_testSuccess = false;
//					break;
//				}
//
//				// 4. 内存对齐检查（根据内存池实际对齐规则调整kAlignBoundary）
//				if (reinterpret_cast<uintptr_t>(ptr) % kAlignBoundary != 0) {
//					std::lock_guard<std::mutex> lock(g_outputMutex);
//					std::cout << "[ERROR] Thread " << threadId << " memory not aligned! Ptr: " << ptr
//						<< ", Bytes: " << reqBytes << ", Align: " << kAlignBoundary << std::endl;
//					g_testSuccess = false;
//					TLSThreadCache->deallocate(ptr, reqBytes);
//					break;
//				}
//
//				// 5. 写入校验数据（检测内存篡改/越界）
//				size_t writeSize = min(reqBytes, sizeof(kMagicNumber));
//				memcpy(ptr, &kMagicNumber, writeSize);
//
//				// 缓存指针，用于后续释放
//				allocatedPtrs.emplace_back(ptr, reqBytes);
//			}
//			else {
//				// 6. 随机释放历史内存（模拟真实场景中的内存回收）
//				if (!allocatedPtrs.empty()) {
//					std::uniform_int_distribution<size_t> idxDist(0, allocatedPtrs.size() - 1);
//					size_t idx = idxDist(gen);
//					auto [ptr, bytes] = allocatedPtrs[idx];
//					allocatedPtrs.erase(allocatedPtrs.begin() + idx);
//
//					// 释放前校验数据（确保内存未被篡改）
//					size_t checkSize = min(bytes, sizeof(kMagicNumber));
//					uint32_t checkMagic = 0;
//					memcpy(&checkMagic, ptr, checkSize);
//					if (memcmp(&checkMagic, &kMagicNumber, checkSize) != 0) {
//						std::lock_guard<std::mutex> lock(g_outputMutex);
//						std::cout << "[ERROR] Thread " << threadId << " memory corrupted! Ptr: " << ptr
//							<< ", Bytes: " << bytes << ", Expected: " << std::hex << kMagicNumber
//							<< ", Actual: " << checkMagic << std::endl;
//						g_testSuccess = false;
//						TLSThreadCache->deallocate(ptr, bytes);
//						break;
//					}
//
//					// 释放内存
//					TLSThreadCache->deallocate(ptr, bytes);
//				}
//			}
//
//			// 7. 偶尔测试边界场景（0字节申请、空指针释放）
//			std::uniform_int_distribution<int> edgeDist(0, 10000);
//			if (edgeDist(gen) == 0) {
//				// 测试0字节申请（应返回nullptr或无害）
//				void* nullPtr = TLSThreadCache->allocate(0);
//				if (nullPtr != nullptr) {
//					std::lock_guard<std::mutex> lock(g_outputMutex);
//					std::cout << "[WARNING] Thread " << threadId << " allocate(0) returned non-null: " << nullPtr << std::endl;
//				}
//			}
//			else if (edgeDist(gen) == 1) {
//				// 测试空指针释放（应无害）
//				TLSThreadCache->deallocate(nullptr, 1024);
//			}
//		}
//
//		// 8. 释放剩余所有内存（避免内存泄漏，确保内存池回收逻辑正常）
//		for (auto [ptr, bytes] : allocatedPtrs) {
//			TLSThreadCache->deallocate(ptr, bytes);
//		}
//
//	}
//	catch (const std::exception& e) {
//		std::lock_guard<std::mutex> lock(g_outputMutex);
//		std::cout << "[ERROR] Thread " << threadId << " threw exception: " << e.what() << std::endl;
//		g_testSuccess = false;
//	}
//	catch (...) {
//		std::lock_guard<std::mutex> lock(g_outputMutex);
//		std::cout << "[ERROR] Thread " << threadId << " threw unknown exception!" << std::endl;
//		g_testSuccess = false;
//	}
//
//	// 9. 释放线程局部ThreadCache（归还给CentralCache，避免测试代码内存泄漏）
//	delete TLSThreadCache;
//	TLSThreadCache = nullptr;
//
//	// 输出线程完成信息
//	std::lock_guard<std::mutex> lock(g_outputMutex);
//	std::cout << "[INFO] Thread " << threadId << " completed " << kOperateCountPerThread << " operations" << std::endl;
//}
//
//// -----------------------------------------------------------------------------
//// 主函数：启动多线程测试
//// -----------------------------------------------------------------------------
//int main() {
//	std::cout << "================================================" << std::endl;
//	std::cout << "High Concurrency Memory Pool Thread-Safe Test" << std::endl;
//	std::cout << "================================================" << std::endl;
//	std::cout << "Thread Count: " << kThreadCount << std::endl;
//	std::cout << "Ops Per Thread: " << kOperateCountPerThread << std::endl;
//	std::cout << "Max Allocate: " << kMaxAllocateBytes / 1024 << "KB" << std::endl;
//	std::cout << "Align Boundary: " << kAlignBoundary << " bytes" << std::endl;
//	std::cout << "================================================" << std::endl;
//
//	// 记录测试开始时间
//	auto startTime = std::chrono::high_resolution_clock::now();
//
//	// 启动所有测试线程
//	std::vector<std::thread> testThreads;
//	testThreads.reserve(kThreadCount);
//	for (size_t i = 0; i < kThreadCount; ++i) {
//		testThreads.emplace_back(MemoryPoolTestThread, i);
//	}
//
//	// 等待所有线程完成
//	for (auto& t : testThreads) {
//		if (t.joinable()) {
//			t.join();
//		}
//	}
//
//	// 计算测试耗时
//	auto endTime = std::chrono::high_resolution_clock::now();
//	auto duration = std::chrono::duration_cast<std::chrono::seconds>(endTime - startTime).count();
//
//	// 输出最终测试结果
//	std::cout << "================================================" << std::endl;
//	if (g_testSuccess) {
//		std::cout << "[PASS] All threads completed without thread-safe issues!" << std::endl;
//		std::cout << "Total Test Time: " << duration << "s" << std::endl;
//	}
//	else {
//		std::cout << "[FAIL] Thread-safe issues detected! Check error messages above." << std::endl;
//		return 1;
//	}
//
//	return 0;
//}
//
//

//#include <iostream>
//#include <thread>
//#include <vector>
//#include <chrono>
//#include <cstdlib>
//#include <cstring>
//#include <iomanip>
//#include <algorithm>
//
//
//
//// ====================== 测试工具函数 ======================
//// 高精度计时工具（毫秒级，支持平均耗时计算）
//using Clock = std::chrono::high_resolution_clock;
//using MsDuration = std::chrono::duration<double, std::milli>;
//
//// 线程函数模板：统一线程启动、等待逻辑（支持 ThreadCache/malloc 两种测试）
//template <typename AllocFunc, typename DeallocFunc>
//double run_thread_test(AllocFunc alloc, DeallocFunc dealloc, size_t thread_cnt, size_t bytes, size_t op_per_thread)
//{
//	std::vector<std::thread> threads;
//	threads.reserve(thread_cnt);
//
//	// 预热操作：消除线程缓存初始化、TLS创建等冷启动耗时
//	if (thread_cnt > 0)
//	{
//		void* temp = alloc(bytes);
//		if (temp) dealloc(temp, bytes);
//	}
//
//	// 开始计时
//	auto start = Clock::now();
//
//	// 启动所有线程：每个线程执行「申请→使用→释放」循环
//	for (size_t i = 0; i < thread_cnt; ++i)
//	{
//		threads.emplace_back([=]() {
//			for (size_t j = 0; j < op_per_thread; ++j)
//			{
//				void* ptr = alloc(bytes);
//				if (ptr)
//				{
//					memset(ptr, 0xAA, bytes);  // 模拟内存使用，防止编译器优化
//					dealloc(ptr, bytes);
//				}
//				else
//				{
//					std::cerr << "[ERROR] 内存申请失败！线程=" << i << " 大小=" << bytes << "B" << std::endl;
//				}
//			}
//			});
//	}
//
//	// 等待所有线程完成
//	for (auto& t : threads) t.join();
//
//	// 计算耗时（毫秒）
//	auto end = Clock::now();
//	return MsDuration(end - start).count();
//}
//
//// ====================== 测试用例实现 ======================
//int main()
//{
//	// 测试配置（可根据需求调整）
//	const std::vector<size_t> THREAD_COUNTS = { 1, 2, 4, 8, 16, 20 };  // 线程数组合
//	const std::vector<size_t> MEM_SIZES = { 16, 64, 256, 1024, 4096, 32768, 131072, 262144 };  // 内存大小（B）
//	const size_t OP_PER_THREAD = 50000;  // 每个线程操作次数（总操作数=线程数×50000≤1e6）
//	const int TEST_ROUNDS = 3;           // 每个用例运行轮数（取平均，提升稳定性）
//
//	// 输出表格表头
//	std::cout << std::fixed << std::setprecision(3);
//	std::cout << "==================================================== 高并发内存池性能测试 ====================================================" << std::endl;
//	std::cout << "内存大小(B)\t线程数\t总操作数\tThreadCache耗时(ms)\tmalloc/free耗时(ms)\tThreadCache吞吐量(ops/ms)\tmalloc/free吞吐量(ops/ms)\t性能提升(%)" << std::endl;
//	std::cout << "=============================================================================================================================" << std::endl;
//
//	// 遍历所有测试用例（内存大小×线程数）
//	for (size_t mem_size : MEM_SIZES)
//	{
//		for (size_t thread_cnt : THREAD_COUNTS)
//		{
//			const size_t TOTAL_OPS = thread_cnt * OP_PER_THREAD;
//			if (TOTAL_OPS > 20 * 50000)  // 严格遵守用户限制
//			{
//				std::cerr << "[SKIP] 线程数=" << thread_cnt << " 操作数/线程=" << OP_PER_THREAD << " 总操作数超出限制" << std::endl;
//				continue;
//			}
//
//			// -------------------------- 测试 ThreadCache --------------------------
//			auto tc_alloc = [](size_t bytes) -> void* {
//				if (!TLSThreadCache) TLSThreadCache = new ThreadCache;  // 懒初始化（用户代码若已实现可删除）
//				return TLSThreadCache->allocate(bytes);
//				};
//			auto tc_dealloc = [](void* ptr, size_t bytes) {
//				if (TLSThreadCache) TLSThreadCache->deallocate(ptr, bytes);
//				};
//
//			double tc_avg_time = 0.0;
//			for (int r = 0; r < TEST_ROUNDS; ++r)
//			{
//				tc_avg_time += run_thread_test(tc_alloc, tc_dealloc, thread_cnt, mem_size, OP_PER_THREAD);
//			}
//			tc_avg_time /= TEST_ROUNDS;
//			const double tc_throughput = TOTAL_OPS / tc_avg_time;
//
//			// -------------------------- 测试 malloc/free --------------------------
//			auto mf_alloc = [](size_t bytes) -> void* { return malloc(bytes); };
//			auto mf_dealloc = [](void* ptr, size_t) { if (ptr) free(ptr); };
//
//			double mf_avg_time = 0.0;
//			for (int r = 0; r < TEST_ROUNDS; ++r)
//			{
//				mf_avg_time += run_thread_test(mf_alloc, mf_dealloc, thread_cnt, mem_size, OP_PER_THREAD);
//			}
//			mf_avg_time /= TEST_ROUNDS;
//			const double mf_throughput = TOTAL_OPS / mf_avg_time;
//
//			// -------------------------- 计算性能提升 --------------------------
//			const double speedup = ((mf_avg_time - tc_avg_time) / mf_avg_time) * 100.0;
//
//			// -------------------------- 输出结果 --------------------------
//			std::cout << mem_size << "\t\t"
//				<< thread_cnt << "\t"
//				<< TOTAL_OPS << "\t"
//				<< tc_avg_time << "\t\t"
//				<< mf_avg_time << "\t\t"
//				<< tc_throughput << "\t\t"
//				<< mf_throughput << "\t\t"
//				<< (speedup > 0 ? speedup : 0.0) << std::endl;
//		}
//		std::cout << "-----------------------------------------------------------------------------------------------------------------------------" << std::endl;
//	}
//
//	// 清理主线程的 ThreadCache（避免内存泄漏）
//	if (TLSThreadCache)
//	{
//		delete TLSThreadCache;
//		TLSThreadCache = nullptr;
//	}
//
//	return 0;
//}




//#include <iostream>
//#include <vector>
//#include <thread>
//#include <chrono>
//#include <mutex>
//#include <iomanip>
//#include <random>
//#include <algorithm>
//#include <cstdlib>
//#include <cstring>
//#include <atomic>
//#include <cstdint>
//
//using namespace std;
//using namespace chrono;
//
//
//
//// ========================== 测试核心配置与工具函数 ==========================
//// 测试覆盖的内存大小（8B~256KB，符合单次最大256KB限制）
//const vector<size_t> TEST_MEM_SIZES = {
//    8, 16, 32, 64, 128, 256, 512,
//    1024, 4 * 1024, 8 * 1024, 16 * 1024,
//    32 * 1024, 64 * 1024, 128 * 1024, 256 * 1024
//};
//
//// 测试线程数（符合≤20的限制）
//const vector<size_t> TEST_THREAD_COUNTS = { 1, 4, 8, 12, 16, 20 };
//
//// 单线程操作次数（符合线程数×操作次数≤1e6的限制）
//const vector<size_t> TEST_OP_COUNTS = { 10000, 50000 };
//
//// 内存限制（7GB，预留1GB系统空间，避免OOM）
//const size_t TOTAL_MEM_LIMIT = 7ULL * 1024 * 1024 * 1024;
//
//// 计算安全操作次数（避免内存溢出）
//size_t GetSafeOpCount(size_t bytes, size_t opCount) {
//    if (bytes == 0) return 0;
//    return min(opCount, TOTAL_MEM_LIMIT / bytes);
//}
//
//// 高精度计时工具（返回毫秒）
//template <typename Func>
//double Timeit(Func&& func) {
//    auto start = high_resolution_clock::now();
//    func();
//    auto end = high_resolution_clock::now();
//    return duration<double, milli>(end - start).count();
//}
//
//// 结果格式化输出（对齐更美观）
//void PrintResultHeader() {
//    cout << left
//        << setw(14) << "测试类型"
//        << setw(12) << "内存大小"
//        << setw(10) << "线程数"
//        << setw(12) << "总操作数"
//        << setw(14) << "总耗时(ms)"
//        << setw(16) << "单次耗时(ns)"
//        << setw(14) << "吞吐量(ops/s)" << endl;
//    cout << string(92, '-') << endl;
//}
//
//void PrintResult(const string& type, size_t bytes, size_t threads, size_t totalOps, double timeMs) {
//    string sizeStr;
//    if (bytes == (size_t)-1) {
//        sizeStr = "随机大小";
//    }
//    else if (bytes < 1024) {
//        sizeStr = to_string(bytes) + "B";
//    }
//    else if (bytes < 1024 * 1024) {
//        sizeStr = to_string(bytes / 1024) + "KB";
//    }
//    else {
//        sizeStr = to_string(bytes / (1024 * 1024)) + "MB";
//    }
//
//    double avgNs = timeMs * 1e6 / totalOps;
//    double throughput = totalOps / (timeMs / 1000);
//
//    cout << left
//        << setw(14) << type
//        << setw(12) << sizeStr
//        << setw(10) << threads
//        << setw(12) << totalOps
//        << setw(14) << fixed << setprecision(2) << timeMs
//        << setw(16) << fixed << setprecision(2) << avgNs
//        << setw(14) << fixed << setprecision(0) << throughput << endl;
//}
//
//// ========================== 测试用例实现（已改为普通函数）==========================
//// 1. 单线程混合测试（分配后立即释放，模拟高频短生命周期场景）
//void SingleThreadMixTest(size_t bytes, size_t opCount) {
//    opCount = GetSafeOpCount(bytes, opCount);
//    if (opCount == 0) {
//        cout << "[警告] 内存大小" << bytes << "B 超出内存限制，跳过测试" << endl;
//        return;
//    }
//
//    // 初始化ThreadCache（首次调用前确保TLS有效）
//    if (!TLSThreadCache) {
//        TLSThreadCache = new ThreadCache;
//    }
//
//    // ThreadCache测试
//    double tcTime = Timeit([&]() {
//        for (size_t i = 0; i < opCount; ++i) {
//            void* ptr = TLSThreadCache->allocate(bytes);
//            TLSThreadCache->deallocate(ptr, bytes);
//        }
//        });
//    PrintResult("ThreadCache", bytes, 1, opCount, tcTime);
//
//    // malloc/free测试
//    double mfTime = Timeit([&]() {
//        for (size_t i = 0; i < opCount; ++i) {
//            void* ptr = malloc(bytes);
//            free(ptr);
//        }
//        });
//    PrintResult("malloc/free", bytes, 1, opCount, mfTime);
//    cout << endl;
//}
//
//// 2. 单线程批量测试（批量分配→批量释放，模拟批量任务场景）
//void SingleThreadBatchTest(size_t bytes, size_t opCount) {
//    opCount = GetSafeOpCount(bytes, opCount);
//    if (opCount == 0) {
//        cout << "[警告] 内存大小" << bytes << "B 超出内存限制，跳过测试" << endl;
//        return;
//    }
//    vector<void*> ptrs(opCount, nullptr);
//
//    // 初始化ThreadCache
//    if (!TLSThreadCache) {
//        TLSThreadCache = new ThreadCache;
//    }
//
//    // ThreadCache测试（批量分配+批量释放）
//    double tcAlloc = Timeit([&]() {
//        for (size_t i = 0; i < opCount; ++i) {
//            ptrs[i] = TLSThreadCache->allocate(bytes);
//        }
//        });
//    double tcFree = Timeit([&]() {
//        for (size_t i = 0; i < opCount; ++i) {
//            TLSThreadCache->deallocate(ptrs[i], bytes);
//            ptrs[i] = nullptr;
//        }
//        });
//    PrintResult("ThreadCache", bytes, 1, opCount, tcAlloc + tcFree);
//
//    // malloc/free测试（批量分配+批量释放）
//    double mfAlloc = Timeit([&]() {
//        for (size_t i = 0; i < opCount; ++i) {
//            ptrs[i] = malloc(bytes);
//        }
//        });
//    double mfFree = Timeit([&]() {
//        for (size_t i = 0; i < opCount; ++i) {
//            free(ptrs[i]);
//            ptrs[i] = nullptr;
//        }
//        });
//    PrintResult("malloc/free", bytes, 1, opCount, mfAlloc + mfFree);
//    cout << endl;
//}
//
//// 3. 多线程混合测试（分配后立即释放，测试并发性能）
//void MultiThreadMixTest(size_t bytes, size_t threadCount, size_t opPerThread) {
//    size_t totalOps = threadCount * opPerThread;
//    opPerThread = GetSafeOpCount(bytes, opPerThread);
//    totalOps = threadCount * opPerThread;
//
//    if (opPerThread == 0) {
//        cout << "[警告] 内存大小" << bytes << "B 超出内存限制，跳过测试" << endl;
//        return;
//    }
//
//    // ThreadCache线程函数（每个线程独立TLS缓存，无竞争）
//    auto tcThreadFunc = [](size_t bytes, size_t ops) {
//        // 线程内初始化TLS ThreadCache
//        if (!TLSThreadCache) {
//            TLSThreadCache = new ThreadCache;
//        }
//        for (size_t i = 0; i < ops; ++i) {
//            void* ptr = TLSThreadCache->allocate(bytes);
//            TLSThreadCache->deallocate(ptr, bytes);
//        }
//        // 线程结束释放ThreadCache（避免内存泄漏）
//        delete TLSThreadCache;
//        TLSThreadCache = nullptr;
//        };
//
//    // ThreadCache测试
//    vector<thread> tcThreads;
//    double tcTime = Timeit([&]() {
//        for (size_t i = 0; i < threadCount; ++i) {
//            tcThreads.emplace_back(tcThreadFunc, bytes, opPerThread);
//        }
//        for (auto& t : tcThreads) {
//            t.join();
//        }
//        });
//    PrintResult("ThreadCache", bytes, threadCount, totalOps, tcTime);
//
//    // malloc/free线程函数（存在全局锁竞争）
//    auto mfThreadFunc = [](size_t bytes, size_t ops) {
//        for (size_t i = 0; i < ops; ++i) {
//            void* ptr = malloc(bytes);
//            free(ptr);
//        }
//        };
//
//    // malloc/free测试
//    vector<thread> mfThreads;
//    double mfTime = Timeit([&]() {
//        for (size_t i = 0; i < threadCount; ++i) {
//            mfThreads.emplace_back(mfThreadFunc, bytes, opPerThread);
//        }
//        for (auto& t : mfThreads) {
//            t.join();
//        }
//        });
//    PrintResult("malloc/free", bytes, threadCount, totalOps, mfTime);
//    cout << endl;
//}
//
//// 4. 随机大小多线程测试（模拟实际业务随机内存需求）
//void RandomSizeMultiThreadTest(size_t threadCount, size_t opPerThread) {
//    // 取最小内存大小计算安全操作数
//    size_t minBytes = TEST_MEM_SIZES.front();
//    opPerThread = GetSafeOpCount(minBytes, opPerThread);
//    size_t totalOps = threadCount * opPerThread;
//
//    if (opPerThread == 0) {
//        cout << "[警告] 随机内存场景超出内存限制，跳过测试" << endl;
//        return;
//    }
//
//    // ThreadCache测试（每个线程独立随机数生成器，无竞争）
//    auto tcThreadFunc = [&]() {
//        if (!TLSThreadCache) {
//            TLSThreadCache = new ThreadCache;
//        }
//        // 线程局部随机数生成器（避免竞争）
//        random_device rd;
//        mt19937 gen(rd());
//        uniform_int_distribution<> dis(0, static_cast<int>(TEST_MEM_SIZES.size() - 1));
//
//        for (size_t i = 0; i < opPerThread; ++i) {
//            size_t idx = static_cast<size_t>(dis(gen));
//            size_t bytes = TEST_MEM_SIZES[idx];
//            void* ptr = TLSThreadCache->allocate(bytes);
//            TLSThreadCache->deallocate(ptr, bytes);
//        }
//
//        delete TLSThreadCache;
//        TLSThreadCache = nullptr;
//        };
//
//    vector<thread> tcThreads;
//    double tcTime = Timeit([&]() {
//        for (size_t i = 0; i < threadCount; ++i) {
//            tcThreads.emplace_back(tcThreadFunc);
//        }
//        for (auto& t : tcThreads) {
//            t.join();
//        }
//        });
//    PrintResult("ThreadCache", (size_t)-1, threadCount, totalOps, tcTime);
//
//    // malloc/free测试
//    auto mfThreadFunc = [&]() {
//        random_device rd;
//        mt19937 gen(rd());
//        uniform_int_distribution<> dis(0, static_cast<int>(TEST_MEM_SIZES.size() - 1));
//
//        for (size_t i = 0; i < opPerThread; ++i) {
//            size_t idx = static_cast<size_t>(dis(gen));
//            size_t bytes = TEST_MEM_SIZES[idx];
//            void* ptr = malloc(bytes);
//            free(ptr);
//        }
//        };
//
//    vector<thread> mfThreads;
//    double mfTime = Timeit([&]() {
//        for (size_t i = 0; i < threadCount; ++i) {
//            mfThreads.emplace_back(mfThreadFunc);
//        }
//        for (auto& t : mfThreads) {
//            t.join();
//        }
//        });
//    PrintResult("malloc/free", (size_t)-1, threadCount, totalOps, mfTime);
//    cout << endl;
//}
//
//// 5. 长时间稳定性测试（测试内存碎片与并发稳定性）
//void LongRunTest(size_t threadCount, size_t opPerLoop, size_t durationSec = 60) {
//    size_t bytes = 64; // 高频小内存场景（最能体现内存池优势）
//    opPerLoop = GetSafeOpCount(bytes, opPerLoop);
//
//    if (opPerLoop == 0) {
//        cout << "[警告] 长时间测试超出内存限制，跳过测试" << endl;
//        return;
//    }
//
//    cout << "=== 长时间稳定性测试（持续" << durationSec << "秒，内存大小：64B）===" << endl;
//    atomic<bool> running(true);
//    atomic<size_t> tcTotalOps(0), mfTotalOps(0);
//
//    // ThreadCache长时间测试
//    vector<thread> tcThreads;
//    for (size_t i = 0; i < threadCount; ++i) {
//        tcThreads.emplace_back([&]() {
//            if (!TLSThreadCache) {
//                TLSThreadCache = new ThreadCache;
//            }
//            while (running) {
//                for (size_t j = 0; j < opPerLoop && running; ++j) {
//                    void* ptr = TLSThreadCache->allocate(bytes);
//                    TLSThreadCache->deallocate(ptr, bytes);
//                    tcTotalOps.fetch_add(1, memory_order_relaxed);
//                }
//            }
//            delete TLSThreadCache;
//            TLSThreadCache = nullptr;
//            });
//    }
//
//    // malloc/free长时间测试
//    vector<thread> mfThreads;
//    for (size_t i = 0; i < threadCount; ++i) {
//        mfThreads.emplace_back([&]() {
//            while (running) {
//                for (size_t j = 0; j < opPerLoop && running; ++j) {
//                    void* ptr = malloc(bytes);
//                    free(ptr);
//                    mfTotalOps.fetch_add(1, memory_order_relaxed);
//                }
//            }
//            });
//    }
//
//    // 运行指定时间
//    this_thread::sleep_for(seconds(durationSec));
//    running = false;
//
//    // 等待所有线程结束
//    for (auto& t : tcThreads) {
//        t.join();
//    }
//    for (auto& t : mfThreads) {
//        t.join();
//    }
//
//    // 输出结果
//    double tcThroughput = static_cast<double>(tcTotalOps) / durationSec;
//    double mfThroughput = static_cast<double>(mfTotalOps) / durationSec;
//
//    cout << left
//        << setw(14) << "ThreadCache"
//        << setw(12) << "64B"
//        << setw(10) << threadCount
//        << setw(12) << tcTotalOps
//        << setw(14) << durationSec * 1000.0 << " (固定)"
//        << setw(16) << fixed << setprecision(2) << (durationSec * 1000.0 * 1e6) / tcTotalOps
//        << setw(14) << fixed << setprecision(0) << tcThroughput << endl;
//
//    cout << left
//        << setw(14) << "malloc/free"
//        << setw(12) << "64B"
//        << setw(10) << threadCount
//        << setw(12) << mfTotalOps
//        << setw(14) << durationSec * 1000.0 << " (固定)"
//        << setw(16) << fixed << setprecision(2) << (durationSec * 1000.0 * 1e6) / mfTotalOps
//        << setw(14) << fixed << setprecision(0) << mfThroughput << endl;
//
//    cout << endl;
//}
//
//// ========================== 主测试入口 ==========================
//int main() {
//    // 控制台输出优化（支持中文）
//    //system("chcp 65001");
//    cout << "=== 高并发内存池 vs malloc/free 性能测试 ===" << endl;
//    cout << "测试环境：CPU=i7-13650H | 物理内存=8GB | 线程数×操作次数≤20×50000" << endl;
//    cout << "测试范围：内存大小8B~256KB | 支持单线程/多线程/随机大小/长时间场景" << endl;
//    cout << "==========================================================" << endl << endl;
//
//    // 1. 单线程混合操作测试（分配→立即释放，高频短生命周期）
//    cout << "=== 1. 单线程混合操作测试（分配后立即释放）===" << endl;
//    PrintResultHeader();
//    for (size_t bytes : TEST_MEM_SIZES) {
//        for (size_t ops : TEST_OP_COUNTS) {
//            cout << "--- 测试配置：内存=" << (bytes < 1024 ? to_string(bytes) + "B" : to_string(bytes / 1024) + "KB")
//                << " | 单线程操作数=" << ops << " ---" << endl;
//            SingleThreadMixTest(bytes, ops);
//        }
//    }
//
//    // 2. 单线程批量操作测试（批量分配→批量释放）
//    cout << "=== 2. 单线程批量操作测试（批量分配→批量释放）===" << endl;
//    PrintResultHeader();
//    for (size_t bytes : TEST_MEM_SIZES) {
//        for (size_t ops : TEST_OP_COUNTS) {
//            cout << "--- 测试配置：内存=" << (bytes < 1024 ? to_string(bytes) + "B" : to_string(bytes / 1024) + "KB")
//                << " | 单线程操作数=" << ops << " ---" << endl;
//            SingleThreadBatchTest(bytes, ops);
//        }
//    }
//
//    // 3. 多线程混合操作测试（重点测试核心内存大小）
//    vector<size_t> coreMemSizes = { 64, 4 * 1024, 64 * 1024, 256 * 1024 }; // 64B/4KB/64KB/256KB（高频场景）
//    cout << "=== 3. 多线程混合操作测试（分配后立即释放）===" << endl;
//    PrintResultHeader();
//    for (size_t bytes : coreMemSizes) {
//        for (size_t threads : TEST_THREAD_COUNTS) {
//            // 256KB内存减少操作数，避免OOM（8GB内存限制）
//            size_t opsPerThread = (bytes == 256 * 1024) ? 1000 : 50000;
//            cout << "--- 测试配置：内存=" << (bytes < 1024 ? to_string(bytes) + "B" : to_string(bytes / 1024) + "KB")
//                << " | 线程数=" << threads << " | 单线程操作数=" << opsPerThread << " ---" << endl;
//            MultiThreadMixTest(bytes, threads, opsPerThread);
//        }
//    }
//
//    // 4. 随机大小多线程测试（模拟实际业务场景）
//    cout << "=== 4. 随机大小多线程测试（8B~256KB随机）===" << endl;
//    PrintResultHeader();
//    for (size_t threads : {8, 16, 20}) { // 重点测试高并发场景
//        cout << "--- 测试配置：线程数=" << threads << " | 单线程操作数=50000 ---" << endl;
//        RandomSizeMultiThreadTest(threads, 50000);
//    }
//
//    // 5. 长时间稳定性测试（验证内存碎片与并发稳定性）
//    cout << "=== 5. 长时间稳定性测试 ===" << endl;
//    PrintResultHeader();
//    LongRunTest(16, 50000, 120); // 16线程×50000次/轮 | 持续60秒
//
//    cout << "=== 所有测试完成！===" << endl;
//    return 0;
//}





//
//#include <iostream>
//#include <vector>
//#include <thread>
//#include <atomic>
//#include <random>
//#include <chrono>
//#include <iomanip>
//#include <windows.h>
//#include <cstdlib>
//#include <cstring>
//
//using namespace std;
//
//
//
//// ======================== 测试工具函数 ========================
//// 高精度计时器（ms级，支持纳秒转换）
//class HighResTimer {
//private:
//    LARGE_INTEGER _start;
//    double _freq; // 时钟频率（转换为ms）
//public:
//    HighResTimer() {
//        LARGE_INTEGER freq;
//        QueryPerformanceFrequency(&freq);
//        _freq = (double)freq.QuadPart / 1000.0;
//        QueryPerformanceCounter(&_start);
//    }
//
//    // 获取已流逝时间（ms）
//    double ElapsedMs() const {
//        LARGE_INTEGER end;
//        QueryPerformanceCounter(&end);
//        return (end.QuadPart - _start.QuadPart) / _freq;
//    }
//
//    // 获取已流逝时间（ns）
//    double ElapsedNs() const {
//        return ElapsedMs() * 1e6;
//    }
//};
//
//// 线程局部随机数生成（生成1B~maxBytes的随机内存大小）
//size_t RandomMemorySize(size_t maxBytes) {
//    static thread_local mt19937 gen(random_device{}());
//    uniform_int_distribution<size_t> dist(1, maxBytes);
//    return dist(gen);
//}
//
//// ======================== 测试用例 ========================
///**
// * 1. 单线程单次操作测试：对比不同大小内存的单次申请+释放耗时
// * 测试内存大小：1B、16B、64B、256B、1KB、8KB、64KB、256KB
// */
//void TestSingleOp() {
//    cout << "=== 【单线程单次操作测试】===" << endl;
//    cout << setw(8) << "内存大小"
//        << setw(18) << "内存池耗时(ns)"
//        << setw(18) << "malloc耗时(ns)"
//        << setw(12) << "提速倍数" << endl;
//
//    const vector<size_t> testSizes = { 1, 16, 64, 256, 1024, 8192, 65536, 262144 };
//    for (size_t bytes : testSizes) {
//        if (bytes > MAX_ALLOCATE_BYTES) {
//            cout << setw(8) << bytes << "B" << setw(18) << "超出限制" << setw(18) << "-" << setw(12) << "-" << endl;
//            continue;
//        }
//
//        // 内存池测试
//        HighResTimer poolTimer;
//        void* poolPtr = TLSThreadCache->allocate(bytes);
//        *(volatile char*)poolPtr = 0; // 防优化：写入内存
//        double poolNs = poolTimer.ElapsedNs();
//        TLSThreadCache->deallocate(poolPtr, bytes);
//
//        // malloc测试
//        HighResTimer mallocTimer;
//        void* mallocPtr = malloc(bytes);
//        *(volatile char*)mallocPtr = 0; // 防优化：写入内存
//        double mallocNs = mallocTimer.ElapsedNs();
//        free(mallocPtr);
//
//        // 计算提速倍数
//        double speedup = mallocNs / poolNs;
//        cout << setw(8) << bytes << "B"
//            << setw(18) << fixed << setprecision(2) << poolNs
//            << setw(18) << fixed << setprecision(2) << mallocNs
//            << setw(12) << fixed << setprecision(2) << speedup << "x" << endl;
//    }
//    cout << endl;
//}
//
///**
// * 2. 单线程批量操作测试：循环N次申请+释放，统计吞吐量
// * @param bytes 内存大小
// * @param opCount 操作次数（申请+释放为1次操作）
// */
//void TestBatchOp(size_t bytes, size_t opCount) {
//    cout << "=== 【单线程批量操作测试】（" << bytes << "B，" << opCount << "次）===" << endl;
//    if (bytes > MAX_ALLOCATE_BYTES) {
//        cout << "超出内存池最大限制，跳过" << endl << endl;
//        return;
//    }
//
//    // 内存池测试
//    HighResTimer poolTimer;
//    for (size_t i = 0; i < opCount; ++i) {
//        void* ptr = TLSThreadCache->allocate(bytes);
//        *(volatile char*)ptr = 0;
//        TLSThreadCache->deallocate(ptr, bytes);
//    }
//    double poolMs = poolTimer.ElapsedMs();
//    double poolOps = opCount / poolMs * 1000; // 每秒操作数
//
//    // malloc测试
//    HighResTimer mallocTimer;
//    for (size_t i = 0; i < opCount; ++i) {
//        void* ptr = malloc(bytes);
//        *(volatile char*)ptr = 0;
//        free(ptr);
//    }
//    double mallocMs = mallocTimer.ElapsedMs();
//    double mallocOps = opCount / mallocMs * 1000;
//
//    // 输出结果
//    cout << "内存池：总耗时=" << fixed << setprecision(3) << poolMs << "ms，吞吐量=" << (size_t)poolOps << " OPS" << endl;
//    cout << "malloc：总耗时=" << fixed << setprecision(3) << mallocMs << "ms，吞吐量=" << (size_t)mallocOps << " OPS" << endl;
//    cout << "吞吐量提升：" << fixed << setprecision(2) << (poolOps / mallocOps) << "x" << endl << endl;
//}
//
///**
// * 3. 多线程并发测试（固定内存大小）
// * @param threadNum 线程数（≤20）
// * @param opPerThread 每线程操作次数（≤50000）
// * @param bytes 内存大小
// */
//void TestMultiThreadFixed(size_t threadNum, size_t opPerThread, size_t bytes) {
//    cout << "=== 【多线程并发测试】（" << threadNum << "线程，每线程" << opPerThread << "次，" << bytes << "B）===" << endl;
//    if (threadNum > 20 || threadNum * opPerThread > 20 * 50000) {
//        cout << "违反限制（线程数×操作次数≤1e6），跳过" << endl << endl;
//        return;
//    }
//    if (bytes > MAX_ALLOCATE_BYTES) {
//        cout << "超出内存池最大限制，跳过" << endl << endl;
//        return;
//    }
//
//    atomic<size_t> poolSuccess(0); // 内存池成功操作数
//    atomic<size_t> mallocSuccess(0); // malloc成功操作数
//
//    // 内存池多线程测试
//    HighResTimer poolTimer;
//    vector<thread> poolThreads;
//    for (size_t i = 0; i < threadNum; ++i) {
//        poolThreads.emplace_back([&]() {
//            TLSThreadCache = new ThreadCache();
//            for (size_t j = 0; j < opPerThread; ++j) {
//                void* ptr = TLSThreadCache->allocate(bytes);
//                if (ptr) {
//                    *(volatile char*)ptr = 0;
//                    TLSThreadCache->deallocate(ptr, bytes);
//                    poolSuccess.fetch_add(1, memory_order_relaxed);
//                }
//            }
//            });
//    }
//    for (auto& t : poolThreads) t.join();
//    double poolMs = poolTimer.ElapsedMs();
//    double poolOps = (threadNum * opPerThread) / poolMs * 1000;
//
//    // malloc多线程测试
//    HighResTimer mallocTimer;
//    vector<thread> mallocThreads;
//    for (size_t i = 0; i < threadNum; ++i) {
//        mallocThreads.emplace_back([&]() {
//            for (size_t j = 0; j < opPerThread; ++j) {
//                void* ptr = malloc(bytes);
//                if (ptr) {
//                    *(volatile char*)ptr = 0;
//                    free(ptr);
//                    mallocSuccess.fetch_add(1, memory_order_relaxed);
//                }
//            }
//            });
//    }
//    for (auto& t : mallocThreads) t.join();
//    double mallocMs = mallocTimer.ElapsedMs();
//    double mallocOps = (threadNum * opPerThread) / mallocMs * 1000;
//
//    // 输出结果
//    cout << "内存池：总操作=" << threadNum * opPerThread << "，成功=" << poolSuccess
//        << "，耗时=" << fixed << setprecision(3) << poolMs << "ms，吞吐量=" << (size_t)poolOps << " OPS" << endl;
//    cout << "malloc：总操作=" << threadNum * opPerThread << "，成功=" << mallocSuccess
//        << "，耗时=" << fixed << setprecision(3) << mallocMs << "ms，吞吐量=" << (size_t)mallocOps << " OPS" << endl;
//    cout << "吞吐量提升：" << fixed << setprecision(2) << (poolOps / mallocOps) << "x" << endl << endl;
//}
//
///**
// * 4. 多线程混合大小测试（模拟真实场景）
// * @param threadNum 线程数
// * @param opPerThread 每线程操作次数
// * @param maxBytes 最大内存大小（≤256KB）
// */
//void TestMultiThreadMixed(size_t threadNum, size_t opPerThread, size_t maxBytes) {
//    cout << "=== 【多线程混合大小测试】（" << threadNum << "线程，每线程" << opPerThread << "次，1B~" << maxBytes << "B）===" << endl;
//    if (threadNum > 20 || threadNum * opPerThread > 20 * 50000) {
//        cout << "违反限制（线程数×操作次数≤1e6），跳过" << endl << endl;
//        return;
//    }
//    maxBytes = min(maxBytes, MAX_ALLOCATE_BYTES);
//
//    atomic<size_t> poolSuccess(0);
//    atomic<size_t> mallocSuccess(0);
//
//    // 内存池测试
//    HighResTimer poolTimer;
//    vector<thread> poolThreads;
//    for (size_t i = 0; i < threadNum; ++i) {
//        poolThreads.emplace_back([&]() {
//
//            TLSThreadCache = new ThreadCache();
//            thread_local vector<size_t> sizes;
//            sizes.reserve(opPerThread);
//            // 预生成随机大小（减少线程内随机数开销）
//            for (size_t j = 0; j < opPerThread; ++j) {
//                sizes.push_back(RandomMemorySize(maxBytes));
//            }
//            // 批量申请释放
//            for (size_t j = 0; j < opPerThread; ++j) {
//                size_t bytes = sizes[j];
//                void* ptr = TLSThreadCache->allocate(bytes);
//                if (ptr) {
//                    *(volatile char*)ptr = 0;
//                    TLSThreadCache->deallocate(ptr, bytes);
//                    poolSuccess.fetch_add(1, memory_order_relaxed);
//                }
//            }
//            });
//    }
//    for (auto& t : poolThreads) t.join();
//    double poolMs = poolTimer.ElapsedMs();
//    double poolOps = (threadNum * opPerThread) / poolMs * 1000;
//
//    // malloc测试
//    HighResTimer mallocTimer;
//    vector<thread> mallocThreads;
//    for (size_t i = 0; i < threadNum; ++i) {
//        mallocThreads.emplace_back([&]() {
//            thread_local vector<size_t> sizes;
//            sizes.reserve(opPerThread);
//            for (size_t j = 0; j < opPerThread; ++j) {
//                sizes.push_back(RandomMemorySize(maxBytes));
//            }
//            for (size_t j = 0; j < opPerThread; ++j) {
//                size_t bytes = sizes[j];
//                void* ptr = malloc(bytes);
//                if (ptr) {
//                    *(volatile char*)ptr = 0;
//                    free(ptr);
//                    mallocSuccess.fetch_add(1, memory_order_relaxed);
//                }
//            }
//            });
//    }
//    for (auto& t : mallocThreads) t.join();
//    double mallocMs = mallocTimer.ElapsedMs();
//    double mallocOps = (threadNum * opPerThread) / mallocMs * 1000;
//
//    // 输出结果
//    cout << "内存池：总操作=" << threadNum * opPerThread << "，成功=" << poolSuccess
//        << "，耗时=" << fixed << setprecision(3) << poolMs << "ms，吞吐量=" << (size_t)poolOps << " OPS" << endl;
//    cout << "malloc：总操作=" << threadNum * opPerThread << "，成功=" << mallocSuccess
//        << "，耗时=" << fixed << setprecision(3) << mallocMs << "ms，吞吐量=" << (size_t)mallocOps << " OPS" << endl;
//    cout << "吞吐量提升：" << fixed << setprecision(2) << (poolOps / mallocOps) << "x" << endl << endl;
//}
//
///**
// * 5. 长时间稳定性测试（300秒，多线程混合大小）
// * @param threadNum 线程数
// * @param maxBytes 最大内存大小
// * @param durationSec 测试时长（默认300秒）
// */
//void TestLongRunning(size_t threadNum, size_t maxBytes, double durationSec = 300.0) {
//    cout << "=== 【长时间稳定性测试】（" << threadNum << "线程，1B~" << maxBytes << "B，持续" << durationSec << "s）===" << endl;
//    if (threadNum > 20) threadNum = 20;
//    maxBytes = min(maxBytes, MAX_ALLOCATE_BYTES);
//
//    atomic<bool> stopFlag(false);
//    atomic<size_t> poolTotalOps(0);
//    atomic<size_t> mallocTotalOps(0);
//
//    // 内存池长时间测试
//    cout << "=== 内存池测试启动（开始计时）===" << endl;
//    HighResTimer poolTimer;
//    vector<thread> poolThreads;
//    for (size_t i = 0; i < threadNum; ++i) {
//        poolThreads.emplace_back([&]() {
//
//            TLSThreadCache = new ThreadCache();
//
//            while (!stopFlag.load(memory_order_relaxed)) {
//                size_t bytes = RandomMemorySize(maxBytes);
//                void* ptr = TLSThreadCache->allocate(bytes);
//                if (ptr) {
//                    *(volatile char*)ptr = 0;
//                    TLSThreadCache->deallocate(ptr, bytes);
//                    poolTotalOps.fetch_add(1, memory_order_relaxed);
//                }
//                // 微小延时（避免CPU 100%占用，可根据需求关闭）
//                this_thread::sleep_for(chrono::nanoseconds(10));
//            }
//            });
//    }
//
//    // 等待指定时长
//    this_thread::sleep_for(chrono::duration<double>(durationSec));
//    stopFlag.store(true, memory_order_relaxed);
//    for (auto& t : poolThreads) t.join();
//    double poolDur = poolTimer.ElapsedMs() / 1000.0;
//    double poolAvgOps = poolTotalOps / poolDur;
//
//    // malloc长时间测试
//    stopFlag.store(false);
//    cout << "=== malloc测试启动（开始计时）===" << endl;
//    HighResTimer mallocTimer;
//    vector<thread> mallocThreads;
//    for (size_t i = 0; i < threadNum; ++i) {
//        mallocThreads.emplace_back([&]() {
//            while (!stopFlag.load(memory_order_relaxed)) {
//                size_t bytes = RandomMemorySize(maxBytes);
//                void* ptr = malloc(bytes);
//                if (ptr) {
//                    *(volatile char*)ptr = 0;
//                    free(ptr);
//                    mallocTotalOps.fetch_add(1, memory_order_relaxed);
//                }
//                this_thread::sleep_for(chrono::nanoseconds(10));
//            }
//            });
//    }
//
//    this_thread::sleep_for(chrono::duration<double>(durationSec));
//    stopFlag.store(true, memory_order_relaxed);
//    for (auto& t : mallocThreads) t.join();
//    double mallocDur = mallocTimer.ElapsedMs() / 1000.0;
//    double mallocAvgOps = mallocTotalOps / mallocDur;
//
//    // 输出结果
//    cout << "=== 测试完成 ===" << endl;
//    cout << "内存池：总操作=" << poolTotalOps << "，实际时长=" << fixed << setprecision(2) << poolDur << "s，平均吞吐量=" << (size_t)poolAvgOps << " OPS" << endl;
//    cout << "malloc：总操作=" << mallocTotalOps << "，实际时长=" << fixed << setprecision(2) << mallocDur << "s，平均吞吐量=" << (size_t)mallocAvgOps << " OPS" << endl;
//    cout << "平均吞吐量提升：" << fixed << setprecision(2) << (poolAvgOps / mallocAvgOps) << "x" << endl;
//    cout << "✅ 测试无崩溃，内存池稳定性良好" << endl << endl;
//}
//
//// ======================== 主函数（执行所有测试）========================
//int main() {
//    // 初始化线程局部ThreadCache
//    if (TLSThreadCache == nullptr) {
//        TLSThreadCache = new ThreadCache;
//    }
//
//    // 测试配置（可根据需求调整）
//    const size_t THREAD_NUM = 20;                // 最大线程数（符合限制）
//    const size_t OP_PER_THREAD = 50000;          // 每线程操作数（20×5万=1e6，符合限制）
//    const size_t BATCH_OP_COUNT_SMALL = 1000000; // 小内存批量操作次数（1e6）
//    const size_t BATCH_OP_COUNT_LARGE = 10000;   // 大内存批量操作次数（1万）
//    const size_t MAX_MIXED_BYTES = 256 * 1024;   // 混合大小测试最大内存
//    const double LONG_RUN_DURATION = 300.0;      // 长时间测试时长（300秒）
//
//    // 执行测试用例
//    TestSingleOp();                                  // 单次操作测试
//    TestBatchOp(64, BATCH_OP_COUNT_SMALL);           // 64B批量测试
//    TestBatchOp(8192, BATCH_OP_COUNT_SMALL / 10);    // 8KB批量测试
//    TestBatchOp(262144, BATCH_OP_COUNT_LARGE);       // 256KB批量测试
//    TestMultiThreadFixed(THREAD_NUM, OP_PER_THREAD, 64); // 多线程固定64B测试
//    TestMultiThreadMixed(THREAD_NUM, OP_PER_THREAD / 10, MAX_MIXED_BYTES); // 多线程混合大小测试
//    TestLongRunning(THREAD_NUM, MAX_MIXED_BYTES, LONG_RUN_DURATION); // 长时间稳定性测试
//
//    // 清理资源
//    delete TLSThreadCache;
//    TLSThreadCache = nullptr;
//    return 0;
//}