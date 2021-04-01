cmake_minimum_required(VERSION 3.0) 
file(GLOB imtjson_SRC "*.cpp")
file(GLOB imtjson_HDR "*.h" "*.tcc")
add_library (imtjson ${imtjson_SRC})
