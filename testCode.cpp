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
			e = (rand() * rand()) % (130 * (1 << PAGE_SHIFT)) + 1;
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
					int x = ((long long)rand() * rand() * rand()) % (130 * 8 * 1024) + 1;
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
					int x = ((long long)rand() * rand()*rand()) % (130 * 8 * 1024) + 1;
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

	printf("%u个线程并发执行%u轮次，每轮次concurrent alloc %u次: 花费：%u ms\n",
		nworks, rounds, ntimes, malloc_costtime.load());

	printf("%u个线程并发执行%u轮次，每轮次concurrent dealloc %u次: 花费：%u ms\n",
		nworks, rounds, ntimes, free_costtime.load());

	printf("%u个线程并发concurrent alloc&dealloc %u次，总计花费：%u ms\n",
		nworks, nworks * rounds * ntimes, malloc_costtime.load() + free_costtime.load());
}

int main()
{
	srand(time(nullptr));
	size_t n = 1000;
	cout << "==========================================================" << endl;
	BenchmarkConcurrentMalloc(n, 4, 1);
	cout << endl << endl;

	BenchmarkMalloc(n, 4, 1);
	cout << "==========================================================" << endl;

	return 0;
}