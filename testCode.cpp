#include <iostream>
#include <ctime>
#include <vector>
using std::cout;
using std::endl;

#include "FixedSizeMemoryPool.hpp"
#include "ConcurrentMemoryPool.h"

using namespace std;


struct TreeNode
{
	int val = 0;
	TreeNode* left = nullptr;
	TreeNode* right = nullptr;
};

void CompareNewAndFixedSizeMemoryPool()
{
	int round = 10;
	int nodeNum = 1000000;

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
			//cout << "v2-" << j << ":" << v2[j] << endl;
		}
		for (int j = 0; j < nodeNum; ++j)
		{
			memPool.Delete(v2[j]);
		}
	}
	end = clock();

	cout << "FixedSizeMemoryPool:" << end - begin << endl;
}

int main()
{
	CompareNewAndFixedSizeMemoryPool();




	return 0;
}