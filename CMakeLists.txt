cmake_minimum_required(VERSION 2.8)
project (imtjson)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/)
add_subdirectory (src/imtjson)
add_subdirectory (src/tests)
add_subdirectory (src/jsonpack)
add_subdirectory (src/jsonunpack)
add_subdirectory (src/jsonbin)
add_subdirectory (src/jsonunbin)
add_subdirectory (src/validator)
  # The 'test' target runs all but the future tests
  cmake_policy(PUSH)
  if(POLICY CMP0037)
    cmake_policy(SET CMP0037 OLD)
  endif()
add_compile_options(-std=c++14)
add_compile_options(-Wall)
add_compile_options(-Werror)
add_custom_target( test bin/imtjson_test DEPENDS bin/imtjson_test)
cmake_policy(POP) 


install(TARGETS imtjson
        DESTINATION "lib"
        ) 
INSTALL(FILES ${imtjson_HDR}
        DESTINATION "include/imtjson" 
        )    
INSTALL(FILES "shared/vla.h" "shared/refcnt.h" "shared/stringview.h"
	DESTINATION "include/imtjson/shared"
	)