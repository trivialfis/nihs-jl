project(Json)
cmake_minimum_required(VERSION 3.2)

option(ENABLE_GTEST "Use googletest for unittesting." ON)

add_library(json SHARED json.cc)

if (ENABLE_GTEST)
  enable_testing()
  add_executable(test-json tests.cc)
  target_link_libraries(test-json json)
  find_package(GTest REQUIRED)

  target_include_directories(test-json PRIVATE ${GTEST_INCLUDE_DIRS})
  target_link_libraries(test-json ${GTEST_LIBRARIES})

  add_test(TestJson test-json)
endif(ENABLE_GTEST)