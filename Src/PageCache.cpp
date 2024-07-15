#include "../Header/PageCache.h"


PageCache PageCache::_sInst;//饿汉模式创建一个单例


span* PageCache::NewSpan(size_t k) {
	assert(k > 0 && k < MAX_PAGE);
	//1-k号桶中有span
	if (!_spanlistPage[k].empty()) {
			return _spanlistPage[k].popFront();
	}

	//2-k号桶没有span，但后面的桶中有span
	for (int i = k + 1; i < MAX_PAGE; ++i) {
		if (!_spanlistPage[i].empty()) {
			span* nspan = _spanlistPage[i].popFront();
			span* kspan = new span;

			kspan->_pageID = nspan->_pageID;
			kspan->_n = k;

			nspan->_pageID += k;
			nspan->_n -= k;

			_spanlistPage[nspan->_n].pushFront(nspan);
			return kspan;
		}
	}
	//3-k号桶以及后面的桶中都没有span
	void* ptr = SystemAlloc(MAX_PAGE - 1);
	span* bigSpan = new span;

	bigSpan->_pageID = ((size_t)ptr) >> PAGE_SHIFT;
	bigSpan->_n = MAX_PAGE - 1;

	_spanlistPage[MAX_PAGE - 1].pushFront(bigSpan);

	return NewSpan(k);
}