/*
 * allocator.h
 *
 *  Created on: 19. 1. 2017
 *      Author: ondra
 */

#ifndef SRC_IMTJSON_ALLOCATOR_H_
#define SRC_IMTJSON_ALLOCATOR_H_

namespace json {
///Declaration of a structure which configures allocator for JSON values.
	struct Allocator {
		///Pointer to allocator.
		/** @param size size to allocate
		*/
		void *(*alloc)(std::size_t size);
		///Pointer to deallocator
		/** @param pointer to memory to deallocate
		 *  @param size count of bytes to deallocate
		 */
		void (*dealloc)(void *ptr);


		static const Allocator *getInstance();

	};

}


#endif /* SRC_IMTJSON_ALLOCATOR_H_ */

