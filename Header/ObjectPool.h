// MemoryPool.h: 标准系统包含文件的包含文件
// 或项目特定的包含文件。
#pragma once

#include "Common.h"
// TODO: 在此处引用程序需要的其他标头。


/*Thanks url : https://blog.csdn.net/m0_62782700/article/details/135443352?spm=1001.2014.3001.5502 */





template<class T>
class objectPool {
private:
	char* _memory = nullptr;	//指向内存块的指针
	size_t _remanentBytes = 0;	//_memory指向空间中剩余多少字节
	void* _freelist = nullptr;	//自由链表，用来连接归还的空闲空间
public:
	T* New() {//申请一个T类型大小的空间
		T* obj = nullptr;//返回的最终空间
		if (_freelist) {
			//_freelist不为空，表示有大小为T的空间被回收，可以重复利用
			void* next = *(void**)_freelist;
			obj = (T*)_freelist;
			_freelist = next;
		}
		else {
			if (_remanentBytes < sizeof(T)) {//剩余空间小于T时，再申请空间
				_remanentBytes = 128 * 1024;	//128k空间
				_memory = (char*)SystemAlloc(_remanentBytes>>13);//(char*)malloc(_remanentBytes);
				if (_memory == nullptr) {
					throw std::bad_alloc();//开辟空间失败，抛出异常
				}
			}

			obj = (T*)_memory;	//给定一个T类型大小
			size_t objSize = sizeof(T) < sizeof(void*) ? sizeof(void*) : sizeof(T);
			_memory += objSize;	//指针后移
			_remanentBytes -= objSize;	//分配后，剩余空间减小
		}
		//使用定位new : void* operator new(size_t, void* p) noexcept;
		new(obj)T;//通过定位new调用构造函数进行初始化
		return obj;
	}

	void Delete(T* obj) {//回收换回来的小空间
		obj->~T();//显示调用析构函数，对对象清理
		//头插法
		*(void**)obj = _freelist;//新块指向旧块或空
		_freelist = obj;//头指针指向新块
	}
};