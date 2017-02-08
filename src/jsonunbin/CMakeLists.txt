cmake_minimum_required(VERSION 2.8)
add_compile_options(-std=c++11)
add_executable (jsonunbin jsonunbin.cpp) 
target_link_libraries (jsonunbin LINK_PUBLIC imtjson)

