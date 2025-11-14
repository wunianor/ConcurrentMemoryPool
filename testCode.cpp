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

	int n = 1000;



	TLSThreadCache = new ThreadCache();

	int roundNum = 10;

	long long a = 0, b = 0;

	long long newa = 0, newb = 0;

	while (roundNum--)
	{

		vector<size_t> vz(n);
		for (auto& e : vz)
		{
			e = (rand() * rand()) % (40 * (1 << PAGE_SHIFT)) + 1;
			int x = rand();
			if (x % 5 == 0)
			{
				e = 256 * 1024 + 1;
			}
			else if (x % 5 == 1)
			{
				e = 128 * 8 * 1024 + 1;
			}
		}

		vector<pair<void*, size_t>> v;

		vector<void*> v1;

		for (int i = 0; i < n; ++i)
		{
			int size = vz[i];


			//auto start = std::chrono::high_resolution_clock::now();
			void* ptr = ConcurrentAlloc(size);
			/*auto end= std::chrono::high_resolution_clock::now();

			auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
			a += duration.count();*/


			v.emplace_back(ptr, size);
		}

		int randFree = v.size();
		
		for (int i = 0; i < randFree; ++i)
		{
			//pmutex.lock();
			//std::cout << this_thread::get_id() << " 正在-释放" << v.back().second << "字节" << std::endl;
			//pmutex.unlock();

			//auto start = std::chrono::high_resolution_clock::now();
			ConcurrentFree(v[i].first);
			//auto end = std::chrono::high_resolution_clock::now();

			/*auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
			b += duration.count();*/
		}
		


		/*for (int i = 0; i < rand(); ++i)
		{
			int size = rand() % 8 + 1;
			void* ptr = TLSThreadCache->allocate(size);
			v.emplace_back(ptr, size);
		}*/


		
		/*for (int i = 0; i < n; ++i)
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
		}*/

	
	}

	
	
	pmutex.lock();
	std::cout << "TLSTCOBJpool:" << (void*)&TLSthreadCacheObjPool << std::endl;

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

//int main()
//{
//	
//	//CompareNewAndFixedSizeMemoryPool();
//	//testTLS();
//
//	//testMallocVsVirtualalloc();
//
//	thread t1(testAllocateAndDeallocate);
//	thread t2(testAllocateAndDeallocate);
//	thread t3(testAllocateAndDeallocate);
//	thread t4(testAllocateAndDeallocate);
//	thread t5(testAllocateAndDeallocate);
//
//
//	t1.join();
//	t2.join();
//	t3.join();
//	t4.join();
//	t5.join();
//
//
//
//	return 0;
//}



#include <iostream>
#include <cstdlib>
#include <vector>
#include <thread>
#include <atomic>
#include <ctime>
#include <cstdio>

using namespace std;

int getX()
{
	return ((long long)rand() * rand() * rand()) % (1024) + 1;
}

void BenchmarkMalloc(size_t ntimes, size_t nworks, size_t rounds)
{
	std::vector<std::thread> vthread(nworks);
	std::atomic<size_t> malloc_costtime = 0;
	std::atomic<size_t> free_costtime = 0;

	for (size_t k = 0; k < nworks; ++k)
	{
		vthread[k] = std::thread([&]() {
			std::vector<void*> v;
			v.reserve(ntimes);

			for (size_t j = 0; j < rounds; ++j)
			{
				size_t begin1 = clock();
				for (size_t i = 0; i < ntimes; i++)
				{
					//v.push_back(malloc(16));

					//v.push_back(malloc((16 + i) % 8192 + 1));

					int x = getX();
					v.push_back(malloc(x));
				}
				size_t end1 = clock();

				size_t begin2 = clock();
				for (size_t i = 0; i < ntimes; i++)
				{
					free(v[i]);
				}
				size_t end2 = clock();
				v.clear();

				malloc_costtime += (end1 - begin1);
				free_costtime += (end2 - begin2);
			}
			});
	}

	for (auto& t : vthread)
	{
		t.join();
	}

	printf("%u个线程并发执行%u轮次，每轮次malloc %u次: 花费：%u ms\n",
		nworks, rounds, ntimes, malloc_costtime.load());

	printf("%u个线程并发执行%u轮次，每轮次free %u次: 花费：%u ms\n",
		nworks, rounds, ntimes, free_costtime.load());

	printf("%u个线程并发malloc&free %u次，总计花费：%u ms\n",
		nworks, nworks * rounds * ntimes, malloc_costtime.load() + free_costtime.load());
}

// 单轮次申请释放次数 线程数 轮次
void BenchmarkConcurrentMalloc(size_t ntimes, size_t nworks, size_t rounds)
{

	std::vector<std::thread> vthread(nworks);
	std::atomic<size_t> malloc_costtime = 0;
	std::atomic<size_t> free_costtime = 0;

	for (size_t k = 0; k < nworks; ++k)
	{
		vthread[k] = std::thread([&]() {
			std::vector<void*> v;
			v.reserve(ntimes);

			for (size_t j = 0; j < rounds; ++j)
			{
				size_t begin1 = clock();
				for (size_t i = 0; i < ntimes; i++)
				{
					//v.push_back(ConcurrentAlloc(16));

					//v.push_back(ConcurrentAlloc((16 + i) % 8192 + 1));

					int x = getX();
					v.push_back(ConcurrentAlloc(x));
				}
				size_t end1 = clock();

				size_t begin2 = clock();
				for (size_t i = 0; i < ntimes; i++)
				{
					ConcurrentFree(v[i]);
				}
				size_t end2 = clock();
				v.clear();

				malloc_costtime += (end1 - begin1);
				free_costtime += (end2 - begin2);
			}
			});
	}

	for (auto& t : vthread)
	{
		t.join();
	}


	//cout << "sum1:" << sum1 << endl;
	//cout << "sum2:" << sum2 << endl;
	//cout << "sum3:" << sum3 << endl;



	printf("%u个线程并发执行%u轮次，每轮次concurrent alloc %u次: 花费：%u ms\n",
		nworks, rounds, ntimes, malloc_costtime.load());

	printf("%u个线程并发执行%u轮次，每轮次concurrent dealloc %u次: 花费：%u ms\n",
		nworks, rounds, ntimes, free_costtime.load());

	printf("%u个线程并发concurrent alloc&dealloc %u次，总计花费：%u ms\n",
		nworks, nworks * rounds * ntimes, malloc_costtime.load() + free_costtime.load());
}

//int main()
//{
//	srand(time(nullptr));
//	size_t n = 1000;
//	cout << "==========================================================" << endl;
//	BenchmarkConcurrentMalloc(n, 4, 10);
//	cout << endl << endl;
//
//	BenchmarkMalloc(n, 4, 10);
//	cout << "==========================================================" << endl;
//
//
//
//	return 0;
//}





#include <Windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <thread>
#include <vector>
#include <random>
#include <algorithm>
#include <numeric>
#include <iostream>
#include <iomanip>
#include <string>



// -------------------------- 测试配置参数 --------------------------
const size_t MAX_TOTAL_ALLOCS = 100000;    // 总申请次数（不超过10万）
const std::vector<int> THREAD_COUNTS = { 1,2,3,5,10 };  // 测试线程数范围
const size_t FIXED_SIZE = 64;             // 定长内存申请大小（字节）
const size_t MIN_RAND_SIZE = 1;           // 不定长内存最小大小（字节）
const size_t MAX_RAND_SIZE = 4096;        // 不定长内存最大大小（字节）
const int TEST_RUNS = 3;                  // 每个测试用例运行次数（取平均值）
const std::string TEST_ENV = "Windows | i7-13650H | 8GB RAM";

// -------------------------- 高精度计时器 --------------------------
class HighResTimer {
private:
	LARGE_INTEGER freq_;
	LARGE_INTEGER start_;
	double elapsed_ms_;

public:
	HighResTimer() {
		QueryPerformanceFrequency(&freq_);
		elapsed_ms_ = 0.0;
	}

	void Start() { QueryPerformanceCounter(&start_); }
	void Stop() {
		LARGE_INTEGER end;
		QueryPerformanceCounter(&end);
		elapsed_ms_ = (end.QuadPart - start_.QuadPart) * 1000.0 / freq_.QuadPart;
	}
	double GetElapsedMs() const { return elapsed_ms_; }
};

// -------------------------- 单线程测试函数 --------------------------
// 单线程定长内存测试（申请+释放）
double SingleThreadFixedTest(bool use_concurrent) {
	HighResTimer timer;
	std::vector<void*> ptrs;
	ptrs.reserve(MAX_TOTAL_ALLOCS);

	timer.Start();
	// 批量申请
	for (size_t i = 0; i < MAX_TOTAL_ALLOCS; ++i) {
		void* ptr = use_concurrent ? ConcurrentAlloc(FIXED_SIZE) : malloc(FIXED_SIZE);
		ptrs.push_back(ptr);
	}
	// 批量释放
	for (void* ptr : ptrs) {
		use_concurrent ? ConcurrentFree(ptr) : free(ptr);
	}
	timer.Stop();

	return timer.GetElapsedMs();
}

// 单线程不定长内存测试（申请+释放）
double SingleThreadRandomTest(bool use_concurrent) {
	HighResTimer timer;
	std::vector<void*> ptrs;
	ptrs.reserve(MAX_TOTAL_ALLOCS);

	// 线程安全的随机数生成器
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<size_t> dist(MIN_RAND_SIZE, MAX_RAND_SIZE);

	timer.Start();
	// 批量申请（随机大小）
	for (size_t i = 0; i < MAX_TOTAL_ALLOCS; ++i) {
		size_t size = dist(gen);
		void* ptr = use_concurrent ? ConcurrentAlloc(size) : malloc(size);
		ptrs.push_back(ptr);
	}
	// 批量释放
	for (void* ptr : ptrs) {
		use_concurrent ? ConcurrentFree(ptr) : free(ptr);
	}
	timer.Stop();

	return timer.GetElapsedMs();
}

// -------------------------- 多线程测试函数 --------------------------
// 多线程定长测试-工作线程函数
void MultiThreadFixedWorker(size_t alloc_count, bool use_concurrent,
	std::vector<void*>& ptrs, double& thread_time) {
	HighResTimer timer;
	ptrs.reserve(alloc_count);

	timer.Start();
	// 线程内申请
	for (size_t i = 0; i < alloc_count; ++i) {
		void* ptr = use_concurrent ? ConcurrentAlloc(FIXED_SIZE) : malloc(FIXED_SIZE);
		ptrs.push_back(ptr);
	}
	// 线程内释放
	for (void* ptr : ptrs) {
		use_concurrent ? ConcurrentFree(ptr) : free(ptr);
	}
	timer.Stop();

	thread_time = timer.GetElapsedMs();
}

// 多线程定长内存测试（总申请次数=线程数×单线程次数）
double MultiThreadFixedTest(int thread_count, bool use_concurrent) {
	if (thread_count <= 0) return 0.0;

	// 每个线程的申请次数（确保总次数≤MAX_TOTAL_ALLOCS）
	size_t alloc_per_thread = MAX_TOTAL_ALLOCS / thread_count;
	size_t total_allocs = alloc_per_thread * thread_count;

	// 存储每个线程的指针和耗时
	std::vector<std::thread> threads;
	std::vector<std::vector<void*>> thread_ptrs(thread_count);
	std::vector<double> thread_times(thread_count, 0.0);

	// 启动所有线程
	for (int i = 0; i < thread_count; ++i) {
		threads.emplace_back(MultiThreadFixedWorker, alloc_per_thread, use_concurrent,
			std::ref(thread_ptrs[i]), std::ref(thread_times[i]));
	}

	// 等待所有线程完成
	for (auto& t : threads) {
		t.join();
	}

	// 返回并行执行的总耗时（取最长线程耗时，即实际完成时间）
	return *std::max_element(thread_times.begin(), thread_times.end());
}

// 多线程不定长测试-工作线程函数
void MultiThreadRandomWorker(size_t alloc_count, bool use_concurrent,
	std::vector<void*>& ptrs, double& thread_time, int thread_id) {
	HighResTimer timer;
	ptrs.reserve(alloc_count);

	// 每个线程独立随机数生成器（避免竞争）
	std::random_device rd;
	std::mt19937 gen(rd() + thread_id);  // 种子偏移避免重复
	std::uniform_int_distribution<size_t> dist(MIN_RAND_SIZE, MAX_RAND_SIZE);

	timer.Start();
	// 线程内申请（随机大小）
	for (size_t i = 0; i < alloc_count; ++i) {
		size_t size = dist(gen);
		void* ptr = use_concurrent ? ConcurrentAlloc(size) : malloc(size);
		ptrs.push_back(ptr);
	}
	// 线程内释放
	for (void* ptr : ptrs) {
		use_concurrent ? ConcurrentFree(ptr) : free(ptr);
	}
	timer.Stop();

	thread_time = timer.GetElapsedMs();
}

// 多线程不定长内存测试
double MultiThreadRandomTest(int thread_count, bool use_concurrent) {
	if (thread_count <= 0) return 0.0;

	size_t alloc_per_thread = MAX_TOTAL_ALLOCS / thread_count;
	size_t total_allocs = alloc_per_thread * thread_count;

	std::vector<std::thread> threads;
	std::vector<std::vector<void*>> thread_ptrs(thread_count);
	std::vector<double> thread_times(thread_count, 0.0);

	// 启动所有线程
	for (int i = 0; i < thread_count; ++i) {
		threads.emplace_back(MultiThreadRandomWorker, alloc_per_thread, use_concurrent,
			std::ref(thread_ptrs[i]), std::ref(thread_times[i]), i);
	}

	// 等待所有线程完成
	for (auto& t : threads) {
		t.join();
	}

	return *std::max_element(thread_times.begin(), thread_times.end());
}

// -------------------------- 测试执行与结果输出 --------------------------
void RunAllTests() {
	std::cout << "===== 高并发内存池性能测试报告（vs malloc/free）=====" << std::endl;
	std::cout << "测试环境：" << TEST_ENV << std::endl;
	std::cout << "总申请次数：" << MAX_TOTAL_ALLOCS << " 次" << std::endl;
	std::cout << "定长内存：" << FIXED_SIZE << " B | 不定长内存：" << MIN_RAND_SIZE
		<< " - " << MAX_RAND_SIZE << " B" << std::endl;
	std::cout << "每个用例运行：" << TEST_RUNS << " 次（取平均值）" << std::endl;
	std::cout << "=====================================================" << std::endl << std::endl;

	// -------------------------- 1. 单线程测试 --------------------------
	std::cout << "[1] 单线程测试" << std::endl;
	std::vector<double> concur_fixed(TEST_RUNS), malloc_fixed(TEST_RUNS);
	std::vector<double> concur_rand(TEST_RUNS), malloc_rand(TEST_RUNS);

	// 运行测试
	for (int i = 0; i < TEST_RUNS; ++i) {
		concur_fixed[i] = SingleThreadFixedTest(true);
		malloc_fixed[i] = SingleThreadFixedTest(false);
		concur_rand[i] = SingleThreadRandomTest(true);
		malloc_rand[i] = SingleThreadRandomTest(false);
	}

	// 计算平均值
	double avg_concur_fixed = std::accumulate(concur_fixed.begin(), concur_fixed.end(), 0.0) / TEST_RUNS;
	double avg_malloc_fixed = std::accumulate(malloc_fixed.begin(), malloc_fixed.end(), 0.0) / TEST_RUNS;
	double avg_concur_rand = std::accumulate(concur_rand.begin(), concur_rand.end(), 0.0) / TEST_RUNS;
	double avg_malloc_rand = std::accumulate(malloc_rand.begin(), malloc_rand.end(), 0.0) / TEST_RUNS;

	// 输出结果
	std::cout << std::fixed << std::setprecision(2);
	std::cout << "  定长内存（" << FIXED_SIZE << "B）：" << std::endl;
	std::cout << "    ConcurrentAlloc: " << avg_concur_fixed << " ms" << std::endl;
	std::cout << "    malloc:          " << avg_malloc_fixed << " ms" << std::endl;
	std::cout << "    性能提升：" << ((avg_malloc_fixed - avg_concur_fixed) / avg_malloc_fixed) * 100
		<< "%" << std::endl << std::endl;

	std::cout << "  不定长内存（" << MIN_RAND_SIZE << "-" << MAX_RAND_SIZE << "B）：" << std::endl;
	std::cout << "    ConcurrentAlloc: " << avg_concur_rand << " ms" << std::endl;
	std::cout << "    malloc:          " << avg_malloc_rand << " ms" << std::endl;
	std::cout << "    性能提升：" << ((avg_malloc_rand - avg_concur_rand) / avg_malloc_rand) * 100
		<< "%" << std::endl << std::endl;

	// -------------------------- 2. 多线程测试 --------------------------
	std::cout << "[2] 多线程测试" << std::endl;
	std::cout << std::setw(10) << "线程数" << std::setw(22) << "测试类型"
		<< std::setw(22) << "ConcurrentAlloc(ms)" << std::setw(22) << "malloc(ms)"
		<< std::setw(20) << "性能提升(%)" << std::endl;
	std::cout << std::string(96, '-') << std::endl;

	for (int thread_cnt : THREAD_COUNTS) {
		// 多线程定长测试
		std::vector<double> mt_concur_fixed(TEST_RUNS), mt_malloc_fixed(TEST_RUNS);
		for (int i = 0; i < TEST_RUNS; ++i) {
			mt_concur_fixed[i] = MultiThreadFixedTest(thread_cnt, true);
			mt_malloc_fixed[i] = MultiThreadFixedTest(thread_cnt, false);
		}
		double avg_mt_concur_fixed = std::accumulate(mt_concur_fixed.begin(), mt_concur_fixed.end(), 0.0) / TEST_RUNS;
		double avg_mt_malloc_fixed = std::accumulate(mt_malloc_fixed.begin(), mt_malloc_fixed.end(), 0.0) / TEST_RUNS;
		double speedup_fixed = ((avg_mt_malloc_fixed - avg_mt_concur_fixed) / avg_mt_malloc_fixed) * 100;

		// 输出定长结果
		std::cout << std::setw(10) << thread_cnt
			<< std::setw(22) << "定长（" << FIXED_SIZE << "B）"
			<< std::setw(22) << avg_mt_concur_fixed
			<< std::setw(22) << avg_mt_malloc_fixed
			<< std::setw(20) << speedup_fixed << std::endl;

		// 多线程不定长测试
		std::vector<double> mt_concur_rand(TEST_RUNS), mt_malloc_rand(TEST_RUNS);
		for (int i = 0; i < TEST_RUNS; ++i) {
			mt_concur_rand[i] = MultiThreadRandomTest(thread_cnt, true);
			mt_malloc_rand[i] = MultiThreadRandomTest(thread_cnt, false);
		}
		double avg_mt_concur_rand = std::accumulate(mt_concur_rand.begin(), mt_concur_rand.end(), 0.0) / TEST_RUNS;
		double avg_mt_malloc_rand = std::accumulate(mt_malloc_rand.begin(), mt_malloc_rand.end(), 0.0) / TEST_RUNS;
		double speedup_rand = ((avg_mt_malloc_rand - avg_mt_concur_rand) / avg_mt_malloc_rand) * 100;

		// 输出不定长结果
		std::cout << std::setw(10) << thread_cnt
			<< std::setw(22) << "不定长（1-" << MAX_RAND_SIZE << "B）"
			<< std::setw(22) << avg_mt_concur_rand
			<< std::setw(22) << avg_mt_malloc_rand
			<< std::setw(20) << speedup_rand << std::endl;

		std::cout << std::string(96, '-') << std::endl;
	}

	std::cout << "===== 测试完成 =====" << std::endl;
}

int main() {
	// 设置控制台UTF-8编码，避免中文乱码
	//SetConsoleOutputCP(CP_UTF8);
	//SetConsoleCP(CP_UTF8);

	RunAllTests();

	system("pause");
	return 0;
}


