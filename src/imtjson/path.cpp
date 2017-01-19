/*
 * path.cpp
 *
 *  Created on: 13. 10. 2016
 *      Author: ondra
 */

#define _SCL_SECURE_NO_WARNINGS
#include "path.h"
#include "array.h"
#include "allocator.h"

namespace json {


class RootPath: public PathRef {
public:
	RootPath():PathRef(Path::root,"<root>") {
		addRef();
	}
};

static RootPath rootPath;
const Path &Path::root = rootPath;


Value Value::operator[](const Path &path) const {
	if (path.isRoot()) return *this;
	else {
		path.checkInstance();
		Value sub = this->operator[](path.getParent());
		if (path.isIndex()) {
			return sub[path.getIndex()];
		} else {
			return sub[path.getKey()];
		}

	}
}




PPath Path::copy() const {
	//do not perform copy of root element
	if (isRoot()) return PPath(static_cast<const PathRef *>(&root));
	//calculate required size - first include pointer to allocator
	uintptr_t reqSize = 0;
	//calculate length
	uintptr_t len=0;
	for (const Path *p = this; p != &root; p = &p->parent) {
		p->checkInstance();
		//reserve space for element
		if (p == this) {
			reqSize += sizeof(PathRef);
			len += sizeof(PathRef);
		}
		else {
			reqSize +=sizeof(Path);
			len += sizeof(Path);
		}
		//if it is key, then reserve space for name
		if (p->isKey()) reqSize+=p->keyName.length+1;
	}

	void *buff = Value::allocator->alloc(reqSize+sizeof(std::uintptr_t));

	std::uintptr_t *cookie = reinterpret_cast<std::uintptr_t *>(
			reinterpret_cast<char *>(buff)+reqSize);

	*cookie = StackProtected::cookieValue;

	//get pointer to first item
	PathRef *start = reinterpret_cast<PathRef *>(buff);
	//get pointer to string buffer
	char *strbuff = reinterpret_cast<char *>(start)+len;
	//perform recursive copy
	PPath out(copyRecurse(start,strbuff));

	if (*cookie != StackProtected::cookieValue)
		throw std::runtime_error("FATAL: imtjson - Corrupted memory!");
	return out;
}


template<typename T>
T *Path::copyRecurse(T * trg, char  *strBuff) const {
	//stop on root
	if (isRoot()) return static_cast<T *>(const_cast<Path *>(this));
	//conv ptr
	Path *p = trg+1;
	//if it is key
	if (isKey()) {
		//pick first unused byte in string buffer
		char *c = strBuff;
		//copy name to buffer
		std::copy(keyName.data,keyName.data+keyName.length, strBuff);
/*		//copy name to buffer
		memcpy(strBuff, keyName.data,keyName.length);*/
		//advance pointer in buffer
		strBuff+=keyName.length;
		//write terminating zero for compatibility issues
		*strBuff++=0;
		//copy rest of path to retrieve new pointer
		Path *out = parent.copyRecurse(p,strBuff);
		//construct path item at give position, use returned pointer as parent
		//and use text from string buffer as name
		return new((void *)trg) T(*out, StringView<char>(c, keyName.length));
	} else {
		//if it is index, just construct rest of path
		Path *out = parent.copyRecurse(p,strBuff);
		//copy rest of path to retrieve new pointer

		return new((void *)trg) T(*out, index);
	}
}

void Path::operator delete(void *ptr) {
	//delete operator is used instead of destructor
	//because object is POD type, it has no destructor
	//destructor is called when need to delete object
	//because object is always allocated by copy()
	//we cannot perform cleanup of whole path

	Value::allocator->dealloc(ptr);

}
void *Path::operator new(size_t , void *p) {
	return p;
}

Value Path::toValue() const {
	Array a;
	toArray(a);
	return a;
}

void Path::toArray(Array& array) const {
	if (isRoot()) return;
	parent.toArray(array);
	if (isKey()) {
		array.add(getKey());
	} else {
		array.add(getIndex());
	}
}

void *Path::operator new(size_t) {
	throw std::runtime_error("Use copy() to allocate Path");
}
void Path::operator delete (void *, void *) {

}

int Path::compare(const Path &other) const {
	if (isRoot()) return other.isRoot()?0:-1;
	if (other.isRoot()) return 1;
	checkInstance();
	other.checkInstance();
	int z;
	if (isIndex()) {
		if (other.isIndex()) {
			if (getIndex() <  other.getIndex()) return -1;
			else if (getIndex() > other.getIndex()) return 1;
		} else {
			return -1;
		}
	} else if (other.isKey()) {
		z = getKey().compare(other.getKey());
		if (z != 0) return z;
	} else {
		return 1;
	}
	return  parent.compare(other.parent);
}



PPath::PPath(const PathRef* p):ref(p) {

}

PPath::~PPath() {
	ref = nullptr;
}

PPath PPath::fromValue(const Path& parent, const Value& v, uintptr_t offset) {
	if (offset >= v.size()) {
		return parent.copy();
	} else {
		Value itm = v[offset];
		if (itm.type() == string) {
			Path x(parent,itm.getString());
			return fromValue(x,v,offset+1);
		} else if (itm.type() == number) {
			Path x(parent,itm.getUInt());
			return fromValue(x,v,offset+1);
		} else {
			throw std::runtime_error("PPath::fromValue: Unsupported item type");
		}
	}
}

int PPath::compare(const Path& other) const {
	return ref->compare(other);
}

PPath::operator const Path&() const {
	return *ref;
}

PPath PPath::fromValue(const Value& v) {
	return fromValue(Path::root, v, 0);
}

Value json::PPath::toValue() const {
	return ref->toValue();
}

PPath::PPath(const Path p):ref(p.copy().ref) {
}



}

