cmake_minimum_required(VERSION 2.8)
add_compile_options(-std=c++11)
add_executable (immujson_test main.cpp) 
target_link_libraries (immujson_test LINK_PUBLIC immujson)