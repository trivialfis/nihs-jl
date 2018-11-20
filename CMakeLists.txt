project(Json)
add_library(json SHARED json.cc)
cmake_minimum_required(VERSION 3.2)

add_executable(test-json test.cc)
target_link_libraries(test-json json)

enable_testing()
add_test(TestJson test-json)
