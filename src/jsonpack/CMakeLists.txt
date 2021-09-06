cmake_minimum_required(VERSION 3.0)
add_executable (jsonpack jsonpack.cpp) 
target_link_libraries (jsonpack LINK_PUBLIC imtjson)

