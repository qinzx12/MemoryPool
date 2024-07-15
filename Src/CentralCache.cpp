#include "../Header/CentralCache.h"
#include "../Header/PageCache.h"
CentralCache CentralCache:: _sInst;//静态成员变量，类内声明，类外初始化

span* CentralCache::GetOneSpace(SpanList& list, size_t size) {
	//获取一个管理空间非空的span
	//GetOneSpan是用来让cc向pc申请的
	span* it = list.begin();
	while (it != list.end()) {
		if (it->_freelist != nullptr)//找到管理空间非空的span
			return it;
		else
			it = it->next;
	}
	list._mtx.unlock();//解掉桶锁，让其他向该cc桶进行操作的线程拿到锁
	//走到这里就是CC中没有找到管理空间非空的span
	size_t pagek = SizeCompute::NumMovePage(size);
	
	PageCache::Getinstance()->_pagemtx.lock();
	//调用new span 获取一个全新的span
	span* new_span = PageCache::Getinstance()->NewSpan(pagek);
	PageCache::Getinstance()->_pagemtx.unlock();

	char* start = (char*)(new_span->_pageID << PAGE_SHIFT);
	char* end = (char*)(start + (new_span->_n << PAGE_SHIFT));
	/*开始切分span管理的空间*/
	new_span->_freelist = start;

	void* tail = start;
	start += size;

	while (start < end) {//链接各个块
		objNext(tail) = start;
		start += size;
		tail = objNext(tail);
	}
	objNext(tail) = nullptr;

	list._mtx.lock();//span挂上去之前，重新上锁
	list.pushFront(new_span);

	return new_span;
}


size_t CentralCache::FetchRangObj(void*& start, void*& end,
	size_t batchNum, size_t size) {//这个函数的作用是让CC拿出一块空间给tc
	//start end表示cc提供的空间的开始和结尾
	//n表示申请空间块数，size是每一块的大小

	//获取size在哪一个spanlist中
	size_t index = SizeCompute::index(size);

	_spanLists[index]._mtx.lock();

	span* sp1 = GetOneSpace(_spanLists[index], size);
	assert(sp1);
	assert(sp1->_freelist);//断言：管理span空间非空

	start = end = sp1->_freelist;
	size_t actualNum = 1;
	size_t i = 0;
	while (i < batchNum - 1 && objNext(end) != nullptr) {
		end = objNext(end);
		++actualNum;
		++i;
	}
	sp1->_freelist = objNext(end);
	sp1->_use_count += actualNum;
	objNext(end) = nullptr;

	_spanLists[index]._mtx.unlock();

	return actualNum;
}

void CentralCache::ReleaseListToSpan(void* start, size_t size) {
	size_t index = SizeCompute::index(size);

	_spanLists[index]._mtx.lock();


	_spanLists[index]._mtx.unlock();
}