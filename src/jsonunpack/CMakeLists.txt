cmake_minimum_required(VERSION 2.8)
add_compile_options(-std=c++11)
add_executable (jsonunpack jsonunpack.cpp) 
target_link_libraries (jsonunpack LINK_PUBLIC imtjson)

