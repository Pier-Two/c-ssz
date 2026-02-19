cmake_minimum_required(VERSION 3.16)

project(c_ssz LANGUAGES C)

if(NOT DEFINED CMAKE_EXPORT_COMPILE_COMMANDS)
  set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE BOOL "Export compile commands for tooling" FORCE)
endif()

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# Keep default warning/optimization/debug flags aligned with the Makefile.
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
  add_compile_options(-Wall -Wextra -O3 -g)
endif()

set(SSZ_SANITIZERS "" CACHE STRING "Sanitizers to enable (example: address or address,undefined)")
if(SSZ_SANITIZERS)
  if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
    add_compile_options(-fsanitize=${SSZ_SANITIZERS})
    add_link_options(-fsanitize=${SSZ_SANITIZERS})
  else()
    message(WARNING "SSZ_SANITIZERS requested, but this compiler does not use GCC/Clang sanitizer flags.")
  endif()
endif()

set(SSZ_INCLUDE_DIRS
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_CURRENT_SOURCE_DIR}/bench
  ${CMAKE_CURRENT_SOURCE_DIR}/tests
  ${CMAKE_CURRENT_SOURCE_DIR}/lib
)

set(SSZ_LIB_SOURCES
  src/ssz_deserialize.c
  src/ssz_serialize.c
  src/ssz_utils.c
  src/ssz_merkle.c
  src/ssz_constants.c
  lib/mincrypt/sha256.c
)

set(SSZ_PLATFORM_LIBS)
if(NOT WIN32)
  list(APPEND SSZ_PLATFORM_LIBS m)
endif()

add_library(ssz STATIC ${SSZ_LIB_SOURCES})
target_include_directories(ssz
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
  PRIVATE
    ${SSZ_INCLUDE_DIRS}
)
set_target_properties(ssz PROPERTIES
  OUTPUT_NAME ssz
  ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)

add_library(ssz_snappy_decode_obj OBJECT tests/snappy_decode.c)
target_include_directories(ssz_snappy_decode_obj PRIVATE ${SSZ_INCLUDE_DIRS})

add_library(ssz_yaml_parser_obj OBJECT bench/yaml_parser.c)
target_include_directories(ssz_yaml_parser_obj PRIVATE ${SSZ_INCLUDE_DIRS})

add_library(ssz_bench_common_obj OBJECT bench/bench.c)
target_include_directories(ssz_bench_common_obj PRIVATE ${SSZ_INCLUDE_DIRS})

enable_testing()

file(GLOB SSZ_TEST_SOURCES CONFIGURE_DEPENDS
  ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_ssz_*.c
)

set(SSZ_TEST_TARGETS)
foreach(TEST_SOURCE IN LISTS SSZ_TEST_SOURCES)
  get_filename_component(TEST_NAME ${TEST_SOURCE} NAME_WE)

  add_executable(${TEST_NAME}
    ${TEST_SOURCE}
    $<TARGET_OBJECTS:ssz_snappy_decode_obj>
    $<TARGET_OBJECTS:ssz_yaml_parser_obj>
  )
  target_include_directories(${TEST_NAME} PRIVATE ${SSZ_INCLUDE_DIRS})
  target_link_libraries(${TEST_NAME} PRIVATE ssz ${SSZ_PLATFORM_LIBS})
  set_target_properties(${TEST_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
  )

  add_test(NAME ${TEST_NAME} COMMAND $<TARGET_FILE:${TEST_NAME}>)
  set_tests_properties(${TEST_NAME} PROPERTIES
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  )

  list(APPEND SSZ_TEST_TARGETS ${TEST_NAME})

  string(REGEX REPLACE "^test_ssz_" "" TEST_SUFFIX ${TEST_NAME})
  add_custom_target(run-test-${TEST_SUFFIX}
    COMMAND $<TARGET_FILE:${TEST_NAME}>
    DEPENDS ${TEST_NAME}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    USES_TERMINAL
  )
endforeach()

add_custom_target(build-tests DEPENDS ${SSZ_TEST_TARGETS})
add_custom_target(run-tests
  COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
  DEPENDS build-tests
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
  USES_TERMINAL
)

file(GLOB SSZ_BENCH_SOURCES CONFIGURE_DEPENDS
  ${CMAKE_CURRENT_SOURCE_DIR}/bench/bench_ssz_*.c
)

set(SSZ_BENCH_TARGETS)
foreach(BENCH_SOURCE IN LISTS SSZ_BENCH_SOURCES)
  get_filename_component(BENCH_NAME ${BENCH_SOURCE} NAME_WE)

  add_executable(${BENCH_NAME}
    ${BENCH_SOURCE}
    $<TARGET_OBJECTS:ssz_bench_common_obj>
    $<TARGET_OBJECTS:ssz_snappy_decode_obj>
    $<TARGET_OBJECTS:ssz_yaml_parser_obj>
  )
  target_include_directories(${BENCH_NAME} PRIVATE ${SSZ_INCLUDE_DIRS})
  target_link_libraries(${BENCH_NAME} PRIVATE ssz ${SSZ_PLATFORM_LIBS})
  set_target_properties(${BENCH_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bench
  )

  list(APPEND SSZ_BENCH_TARGETS ${BENCH_NAME})

  string(REGEX REPLACE "^bench_ssz_" "" BENCH_SUFFIX ${BENCH_NAME})
  add_custom_target(bench-${BENCH_SUFFIX}
    COMMAND $<TARGET_FILE:${BENCH_NAME}>
    DEPENDS ${BENCH_NAME}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    USES_TERMINAL
  )
endforeach()

add_custom_target(build-benches DEPENDS ${SSZ_BENCH_TARGETS})
add_custom_target(bench)
add_dependencies(bench build-benches)
foreach(BENCH_TARGET IN LISTS SSZ_BENCH_TARGETS)
  add_custom_command(TARGET bench
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo "Running ${BENCH_TARGET}..."
    COMMAND $<TARGET_FILE:${BENCH_TARGET}>
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    USES_TERMINAL
  )
endforeach()
