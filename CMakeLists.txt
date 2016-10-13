cmake_minimum_required(VERSION 2.8)
project (immujson)
add_subdirectory (src/immujson)
add_subdirectory (src/tests)
  # The 'test' target runs all but the future tests
  cmake_policy(PUSH)
  if(POLICY CMP0037)
    cmake_policy(SET CMP0037 OLD)
  endif()
add_compile_options(-std=c++11)
add_custom_target( test src/tests/immujson_test DEPENDS src/tests/immujson_test)
cmake_policy(POP) 

