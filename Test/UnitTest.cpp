#include "../Header/ConCurrentAlloc.h"
#include "../Header/ObjectPool.h"
#include "UnitTest.h"

//std
#include <iostream>
#include <thread>

/////////////////////////////// Test1 : test ConCurrentAlloc.h  ////////////
void Alloc1() {
	for (int i = 0; i < 10; ++i) {
		ConCurrentAlloc(10*1024);
		
	}
	std::cout << "Alloc1 Success!!!" << std::endl;
}

void Alloc2() {
	for (int i = 0; i < 10; ++i) {
		ConCurrentAlloc(5*1024);
	}
	std::cout << "Alloc2 Success!!!" << std::endl;
}


void Test_ConCurrentAlloc() {
	std::thread t1{ Alloc1 };
	t1.join();
	std::thread t2{Alloc2};

	t2.join();

}

/////////////////////////////// Test2 : test objectPool.h  ////////////
struct TreeNode // һ�����ṹ�Ľڵ㣬�Ȼ�����ռ��ʱ�����������ڵ�������
{
	int _val;
	TreeNode* _left;
	TreeNode* _right;

	TreeNode()
		:_val(0)
		, _left(nullptr)
		, _right(nullptr)
	{}
};

void Test_ObjectPool() // malloc�͵�ǰ�����ڴ�����ܶԱ�
{
	// �����ͷŵ��ִ�
	const size_t Rounds = 50;

	// ÿ�������ͷŶ��ٴ�
	const size_t N = 100000;

	// �����ܹ�������ͷŵĴ�������Rounds * N�Σ�������ôЩ��˭����

	std::vector<TreeNode*> v1;
	v1.reserve(N);

	// ����malloc������
	size_t begin1 = clock();
	for (size_t j = 0; j < Rounds; ++j)
	{
		for (int i = 0; i < N; ++i)
		{
			v1.push_back(new TreeNode); // ������Ȼ�õ���new������new�ײ��õ�Ҳ��malloc
		}
		for (int i = 0; i < N; ++i)
		{
			delete v1[i]; // ͬ���ģ�delete�ײ�Ҳ��free
		}
		v1.clear(); // ����clear���þ��ǽ�vector�е�������գ�size���㣬
		// ��capacity���ֲ��䣬��������ѭ����ȥ����push_back
	}
	size_t end1 = clock();


	std::vector<TreeNode*> v2;
	v2.reserve(N);

	// �����ڴ�أ�����������ͷŵ�T���;������ڵ�
	objectPool<TreeNode> TNPool;
	size_t begin2 = clock();
	for (size_t j = 0; j < Rounds; ++j)
	{
		for (int i = 0; i < N; ++i)
		{
			v2.push_back(TNPool.New()); // �����ڴ���е�����ռ�
		}
		for (int i = 0; i < N; ++i)
		{
			TNPool.Delete(v2[i]); // �����ڴ���еĻ��տռ�
		}
		v2.clear();// ����clear���þ��ǽ�vector�е�������գ�size���㣬
		// ��capacity���ֲ��䣬��������ѭ����ȥ����push_back
	}
	size_t end2 = clock();


	cout << "new cost time:" << end1 - begin1 << endl; // ���������Ϊʱ�䵥λ����ms
	cout << "object pool cost time:" << end2 - begin2 << endl;
}