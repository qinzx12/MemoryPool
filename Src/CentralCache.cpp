#include "../Header/CentralCache.h"
#include "../Header/PageCache.h"
CentralCache CentralCache:: _sInst;//��̬��Ա���������������������ʼ��

span* CentralCache::GetOneSpace(SpanList& list, size_t size) {
	//��ȡһ������ռ�ǿյ�span
	//GetOneSpan��������cc��pc�����
	span* it = list.begin();
	while (it != list.end()) {
		if (it->_freelist != nullptr)//�ҵ�����ռ�ǿյ�span
			return it;
		else
			it = it->next;
	}
	list._mtx.unlock();//���Ͱ�������������ccͰ���в������߳��õ���
	//�ߵ��������CC��û���ҵ�����ռ�ǿյ�span
	size_t pagek = SizeCompute::NumMovePage(size);
	
	PageCache::Getinstance()->_pagemtx.lock();
	//����new span ��ȡһ��ȫ�µ�span
	span* new_span = PageCache::Getinstance()->NewSpan(pagek);
	PageCache::Getinstance()->_pagemtx.unlock();

	char* start = (char*)(new_span->_pageID << PAGE_SHIFT);
	char* end = (char*)(start + (new_span->_n << PAGE_SHIFT));
	/*��ʼ�з�span����Ŀռ�*/
	new_span->_freelist = start;

	void* tail = start;
	start += size;

	while (start < end) {//���Ӹ�����
		objNext(tail) = start;
		start += size;
		tail = objNext(tail);
	}
	objNext(tail) = nullptr;

	list._mtx.lock();//span����ȥ֮ǰ����������
	list.pushFront(new_span);

	return new_span;
}


size_t CentralCache::FetchRangObj(void*& start, void*& end,
	size_t batchNum, size_t size) {//�����������������CC�ó�һ��ռ��tc
	//start end��ʾcc�ṩ�Ŀռ�Ŀ�ʼ�ͽ�β
	//n��ʾ����ռ������size��ÿһ��Ĵ�С

	//��ȡsize����һ��spanlist��
	size_t index = SizeCompute::index(size);

	_spanLists[index]._mtx.lock();

	span* sp1 = GetOneSpace(_spanLists[index], size);
	assert(sp1);
	assert(sp1->_freelist);//���ԣ�����span�ռ�ǿ�

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