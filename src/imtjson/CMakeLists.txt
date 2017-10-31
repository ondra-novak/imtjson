cmake_minimum_required(VERSION 2.8) 
add_compile_options(-std=c++11 -fPIC)
file(GLOB imtjson_SRC "*.cpp")
file(GLOB imtjson_HDR "*.h" "*.tcc")
file(GLOB imtjson_SHR  "shared/refcnt.h" "shared/stringview.h")
add_library (imtjson ${imtjson_SRC})
# target_include_directories (imtjson PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

install(TARGETS imtjson
        DESTINATION "lib"
        ) 
INSTALL(FILES ${imtjson_HDR}
        DESTINATION "include/imtjson" 
        )    
INSTALL(FILES ${imtjson_SHR}
        DESTINATION "include/imtjson/shared" 
        )    