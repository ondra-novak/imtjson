cmake_minimum_required(VERSION 3.1)
add_compile_options(-std=c++11)
add_executable (jsonvalidator main.cpp) 
target_link_libraries (jsonvalidator LINK_PUBLIC imtjson)


install(TARGETS jsonvalidator
        DESTINATION "bin"
        ) 
