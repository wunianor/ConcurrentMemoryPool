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
	for (int i = 0; i < 1024; ++i)
	{
		void* ptr = TLSThreadCache->allocate(rand() % 8 + 1);
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

int main()
{
	srand(time(nullptr));
	//CompareNewAndFixedSizeMemoryPool();
	testTLS();



	return 0;
}