#pragma once
//公共库，让其他文件包含这个文件。  Public Library
#include <iostream>
#include <vector>
#include <mutex>
#include <assert.h>
using std::vector;
using std::cout;
using std::endl;


static const size_t FREE_LIST_NUM = 208;//哈希表中的自由链表个数
static const size_t MAX_BYTES = 256 * 1024;//ThreadChche单次申请的最大字节数
static const size_t MAX_PAGE = 129;
static const size_t PAGE_SHIFT = 13;//一页多少位，这里一页8K，就是13位

/*****************/
//使用系统的内存空间申请接口  替代malloc
#ifdef  _WIN64
#include <Windows.h>
#else
#include <linux.h>
#endif
inline static void* SystemAlloc(size_t kpage) {
#ifdef _WIN64
	void* ptr = VirtualAlloc(0, kpage << PAGE_SHIFT, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
#else
	//linux下的brk  mmap等
	void* ptr = malloc(kpage << 13);
#endif
	if (ptr == nullptr)
		throw std::bad_alloc();
	return ptr;
}
/*****************/


static void*& objNext(void* obj) {
	return *(void**)obj;//返回指向obj的头4/8个字节
}


class FreeList {
public:
	void push(void *obj){//用来回收空间
		assert(obj);//检查obj是否非空
		++_size;
		objNext(obj) = _freelist;
		_freelist = obj;
	}

	void* pop() {//用来提供空间的
		assert(_freelist);//提供空间的前提是要有空间
		--_size;
		void* obj = _freelist;
		_freelist = objNext(obj);
		return obj;
	}
	bool empty() {
		return _freelist == nullptr;
	}
public:
	size_t& MaxSize() {
		return _maxSize;
	}
	size_t& size() {
		return _size;
	}
	void pushRang(void *start,void* end,size_t _size) {
		objNext(end) = _freelist;
		_freelist = start;
		this->_size += _size;
	}
	void popRang(void*& start, void*& end, size_t n) {
		assert(n <= _size);
		start = end = _freelist;

		for (size_t i = 0; i < n - 1; ++i) {
			end = objNext(end);
		}
		_freelist = objNext(end);
		objNext(end) = nullptr;
		_size -= n;
	}
private:
	void* _freelist = nullptr;//自由链表，初始为空
	size_t _maxSize = 1;//自由链表申请最大空间是多少，初始化为1
	size_t _size = 0;//当前自由链表中有多少块空间
};


class SizeCompute {
public:
	//辅助函数：向上对齐字节
	static size_t _RoundUp(size_t size, size_t alignNum) {
		size_t res = 0;
		if (size % alignNum != 0) {
			res = (size / alignNum + 1) * size;
		}
		else {
			res = size;
		}
		return res;
		//大佬写法：
		//return ((size + alignNum - 1)& ~(alignNum - 1));
	}
	static size_t RoundUp(size_t size) {//计算对齐字节数
		if (size <= 128) {
			return _RoundUp(size, 8);
		}
		else if (size <= 1024) {
			return _RoundUp(size, 16);
		}
		else if (size <= 8 * 1024) {
			return _RoundUp(size, 128);
		}
		else if (size <= 64 * 1024) {
			return _RoundUp(size, 1024);
		}
		else if (size <= 256 * 1024) {
			return _RoundUp(size, 8 * 1024);
		}
		else {
			assert(false);
			return -1;
		}
	}

	//求size对应在哈希表中的下标
	inline static size_t _index(size_t size, size_t align_shift) {
		return ((size + (1 << align_shift) - 1) >> align_shift) - 1;
	}
	inline static size_t index(size_t size) {
		assert(size <= MAX_BYTES);

		static int _array[4] = { 16,56,56,56 };

		if (size <= 128) {
			return _index(size, 3);
		}
		else if (size <= 1024) {
			return _index(size - 128, 4) + _array[0];
		}
		else if (size <= 8 * 1024) {
			return _index(size - 1024, 7) + _array[0] + _array[1];
		}
		else if (size <= 64 * 1024) {
			return _index(size - 8 * 1024, 10) + _array[0]
				+ _array[1] + _array[2];
		}
		else if (size <= 256 * 1024) {
			return _index(size - 64 * 1024, 13) + _array[0] + _array[1]
				+ _array[2] + _array[3];
		}
		else {
			assert(false);
			return -1;
		}
	}
	static size_t NumMoveSize(size_t size) {
		assert(size > 0);
		int num = MAX_BYTES / size;
		
		if (num > 512) num = 512;
		if (num < 2) num = 2;
		
		return num;
	}
	/*to PageCache*/
	static size_t NumMovePage(size_t size) {
		//NumMoveSize是算出tc向CC申请size大小的块时的单次最大申请块数
		size_t num = NumMoveSize(size);
		size_t npage = num * size;//单次申请最大空间大小
		npage >>= PAGE_SHIFT;//除以每页的大小，申请多少页

		if (npage == 0)npage = 1;//不足1页，给一页空间
		return npage;
	}
};


/*******************************************************************/
struct span {
	size_t _pageID; //页号
	size_t _n = 0;	//当前span管理的页数量

	void* _freelist = nullptr;	//每个span下面挂的小块空间头结点
	size_t _use_count = 0;

	span* prev = nullptr;	//前一个节点
	span* next = nullptr;	//后一个节点

};

class SpanList {
private:
	span* _head;//虚拟头结点
	

public:
	std::mutex _mtx;//每个CentralCache中的哈希桶都要有一个桶锁
	bool empty() {
		return _head == _head->next;
	}
	span* popFront() {//获取第一个span
		span* front = _head->next;
		erase(front);
		return front;
	}
	void pushFront(span* spa1) {
		insert(begin(), spa1);
	}
	span* begin() {
		return _head->next;
	}
	span* end() {
		return _head;
	}
	SpanList() {
		_head = new span;

		_head->next = _head;
		_head->prev = _head;
	}

	void insert(span* pos,span* ptr) {//在pos前面插入ptr
		assert(pos);
		assert(ptr);

		span* t = pos->prev;
		t->next = ptr;
		ptr->prev = t;

		ptr->next = pos;
		pos->prev = ptr;

	}

	void erase(span* pos) {//删除位置为pos的链表节点
		assert(pos);
		assert(pos != _head);

		span* pr = pos->prev;
		span* ne = pos->next;

		pr->next = ne;
		ne->prev = pr;
		/*删除的节点不用调用delete，需要调用span的回收逻辑*/
	}

};