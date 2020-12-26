cmake_minimum_required(VERSION 2.8) 
file(GLOB imtjson_SRC "*.cpp")
file(GLOB imtjson_HDR "*.h" "*.tcc")
add_library (imtjson ${imtjson_SRC})
