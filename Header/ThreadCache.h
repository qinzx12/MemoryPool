#pragma once

#include "Common.h"

class ThreadCache {
private:
	FreeList _freeLists[FREE_LIST_NUM];//��ϣ��ÿ��Ͱ��ʾһ����������

public:
	void* Allocate(size_t size);	//�߳�����size��С�Ŀռ�
	void Deallocate(void* obj, size_t size);//�����߳��д�СΪsize��obj�ռ�
	//ThreadCache�ռ䲻��ʱ����CentralCache����ռ�Ľӿ�
	void* FetchFromCentralCache(size_t index, size_t alignSize);
	void ListTooLong(FreeList& list, size_t size);//tc��cc�黹�ռ�listͰ�еĿռ�
};


//TLS��ȫ�ֶ���ָ�룬����ÿ���̶߳�����һ��������ȫ�ֶ���
static _declspec(thread) ThreadCache* pTLSThreadCache = nullptr;
//TLS ���ֲ߳̾��洢��ÿ���߳����ݶ��У����Ṳ��������߳�



