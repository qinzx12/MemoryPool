#pragma once

#include "Common.h"

class ThreadCache {
private:
	FreeList _freeLists[FREE_LIST_NUM];//哈希，每个桶表示一个自由链表

public:
	void* Allocate(size_t size);	//线程申请size大小的空间
	void Deallocate(void* obj, size_t size);//回收线程中大小为size的obj空间
	//ThreadCache空间不够时，向CentralCache申请空间的接口
	void* FetchFromCentralCache(size_t index, size_t alignSize);
	void ListTooLong(FreeList& list, size_t size);//tc向cc归还空间list桶中的空间
};


//TLS的全局对象指针，这样每个线程都能有一个独立的全局对象
static _declspec(thread) ThreadCache* pTLSThreadCache = nullptr;
//TLS 是线程局部存储，每个线程数据独有，不会共享给其余线程



