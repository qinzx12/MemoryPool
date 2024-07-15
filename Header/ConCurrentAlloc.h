#pragma once

#include "ThreadCache.h"
#include <thread>
#include <iostream>
void* ConCurrentAlloc(size_t size) {//线程申请空间 malloc()
	std::cout << "Thread id: " << std::this_thread::get_id()
		<< " pTLSThreadCache : " << pTLSThreadCache << std::endl;
	//TLS是线程局部存储，每个线程都有一个自己独有的，不存在竞争，线程安全
	if (pTLSThreadCache == nullptr) {
		pTLSThreadCache = new ThreadCache;
	}
	return pTLSThreadCache->Allocate(size);
}


void ConCurrentFree(void *ptr) {//回收线程空间   free()
	assert(ptr);
	size_t size = sizeof(ptr);
	pTLSThreadCache->Deallocate(ptr,size);
}