#pragma once

#include "Common.h"
#include <mutex>
class PageCache {
private:
	SpanList _spanlistPage[MAX_PAGE];
	
	static PageCache _sInst;//����ģʽ����һ������
public:
	static PageCache* Getinstance() {
		return &_sInst;
	}
	span* NewSpan(size_t k);
	std::mutex _pagemtx;//page�������
private:
	PageCache() {}
	PageCache(const PageCache& t) = delete;
	PageCache& operator=(const PageCache& t) = delete;
	PageCache(PageCache&& t) = delete;
	PageCache& operator=(PageCache&& t) = delete;

};