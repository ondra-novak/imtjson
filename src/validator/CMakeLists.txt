cmake_minimum_required(VERSION 2.8)
add_compile_options(-std=c++11)
add_executable (validator main.cpp) 
target_link_libraries (validator LINK_PUBLIC immujson)