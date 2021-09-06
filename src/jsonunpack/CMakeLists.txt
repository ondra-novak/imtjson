cmake_minimum_required(VERSION 3.0)
add_executable (jsonunpack jsonunpack.cpp) 
target_link_libraries (jsonunpack LINK_PUBLIC imtjson)

