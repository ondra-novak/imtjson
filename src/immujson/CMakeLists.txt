cmake_minimum_required(VERSION 2.8)
add_compile_options(-std=c++11)
file(GLOB immujson_SRC "*.cpp")
file(GLOB immujson_HDR "*.h" "*.tcc")
add_library (immujson ${immujson_SRC})
target_include_directories (immujson PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

install(TARGETS immujson
        DESTINATION "lib"
        ) 
INSTALL(FILES ${immujson_HDR}
        DESTINATION "include/immujson"
        ) 