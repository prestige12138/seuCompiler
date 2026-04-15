cmake_minimum_required(VERSION 3.16)

project(seuIntermediate LANGUAGES CXX)

if(NOT UNIX)
  message(FATAL_ERROR "seuIntermediate currently requires a UNIX/POSIX environment.")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

enable_testing()

add_executable(seuIntermediate
  src/ast_builder.cpp
  src/symbol_table.cpp
  src/tri_addr_generator.cpp
  src/intermediate_code.cpp
  src/target_ir_emitter.cpp
  src/main.cpp
)

target_include_directories(seuIntermediate PRIVATE include)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
  target_compile_options(seuIntermediate PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_test(NAME seuIntermediate_self_test COMMAND seuIntermediate --self-test)
set_tests_properties(seuIntermediate_self_test PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
