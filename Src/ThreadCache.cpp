#include "../Header/ThreadCache.h"
#include "../Header/CentralCache.h"
#include <iostream>
void* ThreadCache::Allocate(size_t size) {//线程申请size大小的空间
	assert(size <= MAX_BYTES);
	size_t alignSize = SizeCompute::RoundUp(size);//对齐后的字节数
	size_t index = SizeCompute::index(size);//size对应在哈希表中的哪个桶

	if (!_freeLists[index].empty()) {
		return _freeLists[index].pop();//自由链表不为空，直接获取空间
	}
	else {
		return FetchFromCentralCache(index, alignSize);
	}
	return nullptr;
}


void ThreadCache::Deallocate(void* obj, size_t size) {//回收线程中的obj空间
	assert(obj);
	assert(size <= MAX_BYTES);

	size_t index = SizeCompute::index(size);
	_freeLists[index].push(obj);//对应的自由链表回收空间

	//当桶中的块数大于单批次申请的最大空间，需要归还空间
	if (_freeLists[index].size() >= _freeLists[index].MaxSize()) {
		ListTooLong(_freeLists[index], size);
	}
}



//ThreadCache空间不够时，向CentralCache申请空间的接口
void* ThreadCache::FetchFromCentralCache(size_t index, size_t alignSize) {
#ifdef  _WIN64//在windows中，会优先使用windows的宏定义min
	size_t batchNum = min(_freeLists[index].MaxSize(),
		SizeCompute::NumMoveSize(alignSize));
#else
	size_t batchNum = std::min(_freeLists[index].MaxSize(),
		SizeCompute::NumMoveSize(alignSize));
#endif
	if (batchNum == _freeLists[index].MaxSize()) {
		//如果没有到达	上限，那下次再申请的时候可以多给一块
		_freeLists[index].MaxSize()++;
		//这里就是慢开始反馈调节的核心
	}

	void* start = nullptr;
	void* end = nullptr;

	size_t actulNum = CentralCache::Getinstance()->FetchRangObj(
		start, end, batchNum, alignSize);

	assert(actulNum >= 1);

	if (actulNum == 1) {
		assert(start == end);
		
	}
	else {
		_freeLists[index].pushRang(objNext(start), end,actulNum -1);
	}
	return start;

}



void ThreadCache::ListTooLong(FreeList& list, size_t size) {
	void* start = nullptr;
	void* end = nullptr;

	list.popRang(start, end, list.MaxSize());
	//归还空间
	CentralCache::Getinstance()->ReleaseListToSpan(start,size);
}