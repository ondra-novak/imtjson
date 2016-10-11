cmake_minimum_required(VERSION 3.5)
add_compile_options(-std=c++11)
add_library (immujson abstractValue.cpp  array.cpp  arrayValue.cpp  basicValues.cpp  edit.cpp  object.cpp  objectValue.cpp  stackProtection.cpp  value.cpp)
target_include_directories (immujson PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})