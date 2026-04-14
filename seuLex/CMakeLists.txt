cmake_minimum_required(VERSION 3.16)

project(seuLex LANGUAGES CXX)

if(NOT UNIX)
  message(FATAL_ERROR "seuLex currently requires a UNIX/POSIX environment.")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

enable_testing()

add_executable(seuLex
  src/node.cpp
  src/lex_state.cpp
  src/lex_parser.cpp
  src/regex_expander.cpp
  src/nfa_constructor.cpp
  src/dfa_builder.cpp
  src/dfa_minimizer.cpp
  src/code_generator.cpp
  src/main.cpp
)

target_include_directories(seuLex PRIVATE include)
target_compile_definitions(seuLex PRIVATE SEU_LEX_TEST_CXX="${CMAKE_CXX_COMPILER}")

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
  target_compile_options(seuLex PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_test(NAME seuLex_self_test COMMAND seuLex --self-test)
set_tests_properties(seuLex_self_test PROPERTIES WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/..)
