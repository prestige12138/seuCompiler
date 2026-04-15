cmake_minimum_required(VERSION 3.16)

project(SEUCompiler2026 LANGUAGES CXX)

if(NOT UNIX)
  message(FATAL_ERROR "SEUCompiler2026 currently requires a UNIX/POSIX environment.")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

enable_testing()

add_subdirectory(seuLex)
add_subdirectory(seuYacc)
add_subdirectory(intermediate)

add_test(
  NAME seuCompiler_pipeline_expr_integration
  COMMAND ${CMAKE_COMMAND} -E env
          SEU_LEX_BIN=$<TARGET_FILE:seuLex>
          SEU_YACC_BIN=$<TARGET_FILE:seuYacc>
          SEU_TEST_CXX=${CMAKE_CXX_COMPILER}
          ${CMAKE_CURRENT_SOURCE_DIR}/integration/tests/pipeline/run_pipeline_test.sh
)

set_tests_properties(
  seuCompiler_pipeline_expr_integration
  PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

add_test(
  NAME seuCompiler_ir_pipeline_integration
  COMMAND ${CMAKE_COMMAND} -E env
          SEU_LEX_BIN=$<TARGET_FILE:seuLex>
          SEU_YACC_BIN=$<TARGET_FILE:seuYacc>
          SEU_TEST_CXX=${CMAKE_CXX_COMPILER}
          bash
          ${CMAKE_CURRENT_SOURCE_DIR}/integration/tests/ir_pipeline/run_ir_pipeline_test.sh
)

set_tests_properties(
  seuCompiler_ir_pipeline_integration
  PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
