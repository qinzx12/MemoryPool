#pragma once

#include "Common.h"

class CentralCache {
private:
	SpanList _spanLists[FREE_LIST_NUM];
	static CentralCache _sInst;//����ģʽ����һ������
public:
	static CentralCache* Getinstance() {
		return &_sInst;
	}

private:
	CentralCache(){}
	CentralCache(const CentralCache& t) = delete;
	CentralCache& operator=(const CentralCache& t) = delete;
	CentralCache(CentralCache&& t) = delete;
	CentralCache& operator=(CentralCache&& t) = delete;
public:
	size_t FetchRangObj(void*& start, void*& end, size_t batchNum, size_t size);
	//start end��ʾcc�ṩ�Ŀռ�Ŀ�ʼ�ͽ�β
	//n��ʾ����ռ������size��ÿһ��Ĵ�С
	span* GetOneSpace(SpanList& list, size_t size);
	void ReleaseListToSpan(void* start, size_t size);
};