cmake_minimum_required(VERSION 3.16)

project(seuYacc LANGUAGES CXX)

if(NOT UNIX)
  message(FATAL_ERROR "seuYacc currently requires a UNIX/POSIX environment.")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

enable_testing()

add_executable(seuYacc
  src/yacc_parser.cpp
  src/lr1_pda.cpp
  src/parse_table.cpp
  src/lalr_converter.cpp
  src/symbol_table.cpp
  src/main.cpp
)

target_include_directories(seuYacc PRIVATE include)
target_compile_definitions(seuYacc PRIVATE SEU_YACC_TEST_CXX="${CMAKE_CXX_COMPILER}")

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
  target_compile_options(seuYacc PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_test(NAME seuYacc_self_test COMMAND seuYacc --self-test)
set_tests_properties(seuYacc_self_test PROPERTIES WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/..)
