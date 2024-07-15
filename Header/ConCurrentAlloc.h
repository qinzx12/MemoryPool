#pragma once

#include "ThreadCache.h"
#include <thread>
#include <iostream>
void* ConCurrentAlloc(size_t size) {//�߳�����ռ� malloc()
	std::cout << "Thread id: " << std::this_thread::get_id()
		<< " pTLSThreadCache : " << pTLSThreadCache << std::endl;
	//TLS���ֲ߳̾��洢��ÿ���̶߳���һ���Լ����еģ������ھ������̰߳�ȫ
	if (pTLSThreadCache == nullptr) {
		pTLSThreadCache = new ThreadCache;
	}
	return pTLSThreadCache->Allocate(size);
}


void ConCurrentFree(void *ptr) {//�����߳̿ռ�   free()
	assert(ptr);
	size_t size = sizeof(ptr);
	pTLSThreadCache->Deallocate(ptr,size);
}