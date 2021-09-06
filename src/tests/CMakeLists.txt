cmake_minimum_required(VERSION 2.8)
add_executable (imtjson_test main.cpp rpc_tests.cpp jwt_test.cpp) 
target_link_libraries (imtjson_test LINK_PUBLIC imtjson ssl crypto) 