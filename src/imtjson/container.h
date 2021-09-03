/*
 * container.h
 *
 *  Created on: 19. 1. 2017
 *      Author: ondra
 */

#ifndef SRC_IMTJSON_CONTAINER_H_
#define SRC_IMTJSON_CONTAINER_H_

#pragma once

#include "allocator.h"

namespace json {


///Base class for containers (array and object)
/** Because std::vector sucks
 *
 * Container keeps items along with the object. It must be allocated with predefined capacity.
 * You cannot declare object at stack unless you provide sufficient space to store  items
 * Otherwise you can allocate object with create() function.
 *
 * The Container works similar as vector. It can push_back or pop_back items, perform
 * forward iteration and search for values through operator[]. It cannot insert or delete
 * items in middle or begin of the container
 *
 *
 * */
template<typename T>
class Container{
public:

	struct AllocInfo {
		T *items;
		std::size_t capacity;
		const std::size_t reqCapacity;
		AllocInfo():items(0), capacity(0),reqCapacity(0) {}
		AllocInfo(T *items,std::size_t capacity):items(items),capacity(capacity),reqCapacity(0) {}
		explicit AllocInfo(std::size_t reqCapacity):items(0), capacity(0),reqCapacity(reqCapacity) {}
	};

	Container(const AllocInfo &ainfo);
	///create empty R/O container (on stack)
	Container();
	~Container();

	static Container *create(std::size_t capacity);


	Container(const Container &other) = delete;
	Container &operator=(const Container &other);

	T &operator[](unsigned int pos);
	const PValue &operator[](unsigned int pos) const;
	T *begin() const;
	T *end() const;

	bool push_back(const T &v);
	bool push_back(T &&v);
	void pop_back();

	void clear();
	bool empty() const;
	std::size_t size() const;

	void *operator new(std::size_t obj, AllocInfo &ainfo);
	void operator delete(void *ptr, AllocInfo &ainfo);
	void operator delete(void *ptr, std::size_t sz);

	void shrink(std::size_t newSize);

protected:
	std::size_t capacity;
	std::size_t curSize;
	T *items;

};


template<typename T>
inline Container<T>::Container(const AllocInfo &allocInfo)
	:capacity(allocInfo.capacity)
	,curSize(0)
	,items(allocInfo.items)
{

}

template<typename T>
inline Container<T>::~Container() {
	clear();
}

template<typename T>
inline Container<T>& Container<T>::operator =(const Container& other) {
	clear();
	for ( const T &v : other )
		push_back(v);
	return *this;
}

template<typename T>
inline T& Container<T>::operator [](unsigned int pos) {
	return items[pos];
}

template<typename T>
inline const PValue& Container<T>::operator [](unsigned int pos) const {
	return items[pos];
}

template<typename T>
inline T* Container<T>::begin() const {
	return items;
}

template<typename T>
inline T* Container<T>::end() const {
	return items+curSize;
}

template<typename T>
inline bool Container<T>::push_back(const T& v) {
	if (curSize < capacity) {
		new(items+curSize) T(v);
		curSize++;
		return true;
	} else {
		return false;
	}
}

template<typename T>
inline bool Container<T>::push_back(T&& v) {
	if (curSize < capacity) {
		new(items+curSize) T(std::move(v));
		curSize++;
		return true;
	} else {
		return false;
	}
}

template<typename T>
inline void Container<T>::pop_back() {
	if (curSize > 0) {
		items[--curSize].~T();
	}
}

template<typename T>
inline void Container<T>::clear() {
	while (curSize>0) {
		items[--curSize].~T();
	}
}

template<typename T>
inline bool Container<T>::empty() const {
	return curSize == 0;
}

template<typename T>
inline std::size_t Container<T>::size() const {
	return curSize;
}

template<typename T>
inline void* Container<T>::operator new(std::size_t sz, AllocInfo &allocInfo) {
	std::size_t needsz = sz + allocInfo.reqCapacity*sizeof(T);
	void *p = Allocator::getInstance()->alloc(needsz);
	allocInfo.items = reinterpret_cast<T *>(reinterpret_cast<unsigned char *>(p)+sz);
	allocInfo.capacity = allocInfo.reqCapacity;
	return p;
}

template<typename T>
inline void Container<T>::operator delete(void* ptr, AllocInfo &) {
	Allocator::getInstance()->dealloc(ptr);
}

template<typename T>
inline void Container<T>::operator delete(void* ptr, std::size_t ) {
	Allocator::getInstance()->dealloc(ptr);
}

template<typename T>
Container<T> *Container<T>::create(std::size_t capacity) {
	AllocInfo cap(capacity);
	return new(cap) Container<T>(cap);
}
}

template<typename T>
inline json::Container<T>::Container() :capacity(0),curSize(0),items(nullptr) {}

template<typename T>
inline void json::Container<T>::shrink(std::size_t newSize) {
	while (newSize < curSize) pop_back();
}

#endif /* SRC_IMTJSON_CONTAINER_H_ */

