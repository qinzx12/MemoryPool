#pragma once

#include "Common.h"

class CentralCache {
private:
	SpanList _spanLists[FREE_LIST_NUM];
	static CentralCache _sInst;//饿汉模式创建一个单例
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
	//start end表示cc提供的空间的开始和结尾
	//n表示申请空间块数，size是每一块的大小
	span* GetOneSpace(SpanList& list, size_t size);
	void ReleaseListToSpan(void* start, size_t size);
};