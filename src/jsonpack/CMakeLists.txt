cmake_minimum_required(VERSION 2.8)
add_compile_options(-std=c++11)
add_executable (jsonpack jsonpack.cpp) 
target_link_libraries (jsonpack LINK_PUBLIC imtjson)

