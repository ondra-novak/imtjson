cmake_minimum_required(VERSION 3.0)
add_executable (jsonbin jsonbin.cpp) 
target_link_libraries (jsonbin LINK_PUBLIC imtjson)

