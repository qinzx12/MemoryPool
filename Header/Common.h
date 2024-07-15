#pragma once
//�����⣬�������ļ���������ļ���  Public Library
#include <iostream>
#include <vector>
#include <mutex>
#include <assert.h>
using std::vector;
using std::cout;
using std::endl;


static const size_t FREE_LIST_NUM = 208;//��ϣ���е������������
static const size_t MAX_BYTES = 256 * 1024;//ThreadChche�������������ֽ���
static const size_t MAX_PAGE = 129;
static const size_t PAGE_SHIFT = 13;//һҳ����λ������һҳ8K������13λ

/*****************/
//ʹ��ϵͳ���ڴ�ռ�����ӿ�  ���malloc
#ifdef  _WIN64
#include <Windows.h>
#else
#include <linux.h>
#endif
inline static void* SystemAlloc(size_t kpage) {
#ifdef _WIN64
	void* ptr = VirtualAlloc(0, kpage << PAGE_SHIFT, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
#else
	//linux�µ�brk  mmap��
	void* ptr = malloc(kpage << 13);
#endif
	if (ptr == nullptr)
		throw std::bad_alloc();
	return ptr;
}
/*****************/


static void*& objNext(void* obj) {
	return *(void**)obj;//����ָ��obj��ͷ4/8���ֽ�
}


class FreeList {
public:
	void push(void *obj){//�������տռ�
		assert(obj);//���obj�Ƿ�ǿ�
		++_size;
		objNext(obj) = _freelist;
		_freelist = obj;
	}

	void* pop() {//�����ṩ�ռ��
		assert(_freelist);//�ṩ�ռ��ǰ����Ҫ�пռ�
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
	void* _freelist = nullptr;//����������ʼΪ��
	size_t _maxSize = 1;//���������������ռ��Ƕ��٣���ʼ��Ϊ1
	size_t _size = 0;//��ǰ�����������ж��ٿ�ռ�
};


class SizeCompute {
public:
	//�������������϶����ֽ�
	static size_t _RoundUp(size_t size, size_t alignNum) {
		size_t res = 0;
		if (size % alignNum != 0) {
			res = (size / alignNum + 1) * size;
		}
		else {
			res = size;
		}
		return res;
		//����д����
		//return ((size + alignNum - 1)& ~(alignNum - 1));
	}
	static size_t RoundUp(size_t size) {//��������ֽ���
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

	//��size��Ӧ�ڹ�ϣ���е��±�
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
		//NumMoveSize�����tc��CC����size��С�Ŀ�ʱ�ĵ�������������
		size_t num = NumMoveSize(size);
		size_t npage = num * size;//�����������ռ��С
		npage >>= PAGE_SHIFT;//����ÿҳ�Ĵ�С���������ҳ

		if (npage == 0)npage = 1;//����1ҳ����һҳ�ռ�
		return npage;
	}
};


/*******************************************************************/
struct span {
	size_t _pageID; //ҳ��
	size_t _n = 0;	//��ǰspan�����ҳ����

	void* _freelist = nullptr;	//ÿ��span����ҵ�С��ռ�ͷ���
	size_t _use_count = 0;

	span* prev = nullptr;	//ǰһ���ڵ�
	span* next = nullptr;	//��һ���ڵ�

};

class SpanList {
private:
	span* _head;//����ͷ���
	

public:
	std::mutex _mtx;//ÿ��CentralCache�еĹ�ϣͰ��Ҫ��һ��Ͱ��
	bool empty() {
		return _head == _head->next;
	}
	span* popFront() {//��ȡ��һ��span
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

	void insert(span* pos,span* ptr) {//��posǰ�����ptr
		assert(pos);
		assert(ptr);

		span* t = pos->prev;
		t->next = ptr;
		ptr->prev = t;

		ptr->next = pos;
		pos->prev = ptr;

	}

	void erase(span* pos) {//ɾ��λ��Ϊpos������ڵ�
		assert(pos);
		assert(pos != _head);

		span* pr = pos->prev;
		span* ne = pos->next;

		pr->next = ne;
		ne->prev = pr;
		/*ɾ���Ľڵ㲻�õ���delete����Ҫ����span�Ļ����߼�*/
	}

};