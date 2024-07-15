#include "../Header/ThreadCache.h"
#include "../Header/CentralCache.h"
#include <iostream>
void* ThreadCache::Allocate(size_t size) {//�߳�����size��С�Ŀռ�
	assert(size <= MAX_BYTES);
	size_t alignSize = SizeCompute::RoundUp(size);//�������ֽ���
	size_t index = SizeCompute::index(size);//size��Ӧ�ڹ�ϣ���е��ĸ�Ͱ

	if (!_freeLists[index].empty()) {
		return _freeLists[index].pop();//��������Ϊ�գ�ֱ�ӻ�ȡ�ռ�
	}
	else {
		return FetchFromCentralCache(index, alignSize);
	}
	return nullptr;
}


void ThreadCache::Deallocate(void* obj, size_t size) {//�����߳��е�obj�ռ�
	assert(obj);
	assert(size <= MAX_BYTES);

	size_t index = SizeCompute::index(size);
	_freeLists[index].push(obj);//��Ӧ������������տռ�

	//��Ͱ�еĿ������ڵ�������������ռ䣬��Ҫ�黹�ռ�
	if (_freeLists[index].size() >= _freeLists[index].MaxSize()) {
		ListTooLong(_freeLists[index], size);
	}
}



//ThreadCache�ռ䲻��ʱ����CentralCache����ռ�Ľӿ�
void* ThreadCache::FetchFromCentralCache(size_t index, size_t alignSize) {
#ifdef  _WIN64//��windows�У�������ʹ��windows�ĺ궨��min
	size_t batchNum = min(_freeLists[index].MaxSize(),
		SizeCompute::NumMoveSize(alignSize));
#else
	size_t batchNum = std::min(_freeLists[index].MaxSize(),
		SizeCompute::NumMoveSize(alignSize));
#endif
	if (batchNum == _freeLists[index].MaxSize()) {
		//���û�е���	���ޣ����´��������ʱ����Զ��һ��
		_freeLists[index].MaxSize()++;
		//�����������ʼ�������ڵĺ���
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
	//�黹�ռ�
	CentralCache::Getinstance()->ReleaseListToSpan(start,size);
}