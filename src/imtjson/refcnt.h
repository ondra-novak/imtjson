#ifndef __ONDRA_IMTJSON_REFCNT_H_pkgqgqwdkuUvgvuywefgUIGuyqwteudvw
#define __ONDRA_IMTJSON_REFCNT_H_pkgqgqwdkuUvgvuywefgUIGuyqwteudvw

#include <atomic>

namespace json {


	template<typename T> class RefCntPtr;

	///Simple refcounting
	/** Because std::shared_ptr is too heavy a bloated and slow and wastes a lot memory */

	class RefCntObj {
	public:

		void addRef() const noexcept {
			++counter;
		}

		bool release() const noexcept {
			return --counter == 0;
		}

		RefCntObj():counter(0) {}

		bool isShared() const {
			return counter > 1;
		}

		long use_count() const noexcept {
			return counter;
		}

	protected:
		mutable std::atomic_long counter;


		template<typename T> friend class RefCntPtr;
	};


	///Simple refcounting smart pointer
	/** Because std::shared_ptr is too heavy a bloated and slow and wastes a lot memory */
	template<typename T>
	class RefCntPtr {
	public:

		RefCntPtr() :ptr(0)  {}
		RefCntPtr(T *ptr) :ptr(ptr)  { addRefPtr(); }
		RefCntPtr(const RefCntPtr &other)  :ptr(other.ptr) {
			addRefPtr();
		}
		~RefCntPtr()  {
			releaseRefPtr();
		}
		RefCntPtr(RefCntPtr &&other) :ptr(other.ptr)  {
			other.ptr = nullptr;
		}



		RefCntPtr &operator=(const RefCntPtr &other)  {
			if (other.ptr != ptr) {
				if (other.ptr) other.ptr->addRef();
				releaseRefPtr();
				ptr = other.ptr;
			}
			return *this;
		}

		RefCntPtr &operator=(RefCntPtr &&other) {
			if (this != &other) {
				releaseRefPtr();
				ptr = other.ptr;
				other.ptr = nullptr;
			}
			return *this;
		}

		operator T *() const noexcept { return ptr; }
		T &operator *() const noexcept { return *ptr; }
		T *operator->() const noexcept { return ptr; }

		template<typename X>
		operator X *() const noexcept { return ptr; }

		template<typename X>
		static RefCntPtr<T> dynamicCast(const RefCntPtr<X> &other) noexcept {
			return RefCntPtr<T>(dynamic_cast<T *>((X *)other));
		}
		template<typename X>
		static RefCntPtr<T> staticCast(const RefCntPtr<X> &other) noexcept {
			return RefCntPtr<T>(static_cast<T *>((X *)other));
		}

		bool operator==(std::nullptr_t) const noexcept { return ptr == 0; }
		bool operator!=(std::nullptr_t) const noexcept { return ptr != 0; }
		bool operator==(const RefCntPtr &other) const noexcept { return ptr == other.ptr; }
		bool operator!=(const RefCntPtr &other) const noexcept { return ptr != other.ptr; }

		long use_count() const noexcept {
			return ptr?ptr->use_count():0;
		}


	protected:
		T *ptr;

		void addRefPtr() noexcept {
			if (ptr) ptr->addRef();
		}
		void releaseRefPtr()  {
			if (ptr) {
				T *save = ptr;
				ptr = 0;
				if (save->release())
					delete save;
			}
		}
	};

}

#endif
