cmake_minimum_required(VERSION 2.8)
add_compile_options(-std=c++11)
add_executable (jsonbin jsonbin.cpp) 
target_link_libraries (jsonbin LINK_PUBLIC imtjson)

