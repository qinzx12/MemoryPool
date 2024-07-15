#include<iostream>
#include <vector>
#include "../Header/Common.h"
#include "../Header/ObjectPool.h"
#include "../Header/ThreadCache.h"

/*参考：高并发内存池项目：
https://blog.csdn.net/m0_62782700/article/details/135443352?spm=1001.2014.3001.5502
*/

#ifndef _TEST
#define _TEST
#include "../Test/UnitTest.h"
#endif 

int main() {
	Test_ObjectPool();
	Test_ConCurrentAlloc();
	return 0;
}