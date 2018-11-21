project(Json)
cmake_minimum_required(VERSION 3.2)

add_library(json SHARED json.cc)

add_executable(test-json tests.cc)
target_link_libraries(test-json json)

enable_testing()
add_test(TestJson test-json)