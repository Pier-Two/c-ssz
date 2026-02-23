cmake_minimum_required(VERSION 3.16)

project(c_ssz LANGUAGES C)

if(NOT DEFINED CMAKE_EXPORT_COMPILE_COMMANDS)
  set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE BOOL "Export compile commands for tooling" FORCE)
endif()

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
  add_compile_options(-Wall -Wextra -O3 -g)
  if(CMAKE_C_COMPILER_ID MATCHES "Clang|AppleClang")
    add_compile_options(-Wdocumentation)
  endif()
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

set(SSZ_PLATFORM_LIBS)
if(NOT WIN32)
  list(APPEND SSZ_PLATFORM_LIBS m)
endif()

set(SSZ_HACL_INCLUDE_DIRS
  ${CMAKE_CURRENT_SOURCE_DIR}/external/hacl/include
  ${CMAKE_CURRENT_SOURCE_DIR}/external/hacl/karamel/include
  ${CMAKE_CURRENT_SOURCE_DIR}/external/hacl/karamel/krmllib/dist/minimal
)

add_library(ssz
  src/ssz_types.c
  src/ssz_serialize.c
  src/ssz_deserialize.c
  src/ssz_merkle.c
  src/ssz_proof.c
  src/ssz_hash.c
  src/ssz_endian.c
  external/hacl/src/Hacl_Hash_SHA2.c
)

target_include_directories(ssz
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${SSZ_HACL_INCLUDE_DIRS}
)

# HACL*/karamel's lowstar_endianness.h uses be32toh/htobe32 etc. from <endian.h>.
# In strict C99 mode (-std=c99), glibc does not expose these macros unless
# _DEFAULT_SOURCE is defined.
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
  set_source_files_properties(
    external/hacl/src/Hacl_Hash_SHA2.c
    PROPERTIES COMPILE_DEFINITIONS _DEFAULT_SOURCE
  )
endif()

target_link_libraries(ssz PUBLIC ${SSZ_PLATFORM_LIBS})

enable_testing()

add_executable(test_serialize tests/unit/test_serialize.c)
target_link_libraries(test_serialize PRIVATE ssz)
add_test(NAME test_serialize COMMAND test_serialize)

add_executable(test_deserialize tests/unit/test_deserialize.c)
target_link_libraries(test_deserialize PRIVATE ssz)
add_test(NAME test_deserialize COMMAND test_deserialize)

add_executable(test_merkle tests/unit/test_merkle.c)
target_link_libraries(test_merkle PRIVATE ssz)
add_test(NAME test_merkle COMMAND test_merkle)

add_executable(test_proof tests/unit/test_proof.c)
target_link_libraries(test_proof PRIVATE ssz)
add_test(NAME test_proof COMMAND test_proof)

add_executable(test_hash tests/unit/test_hash.c)
target_link_libraries(test_hash PRIVATE ssz)
add_test(NAME test_hash COMMAND test_hash)

add_executable(test_types tests/unit/test_types.c)
target_link_libraries(test_types PRIVATE ssz)
add_test(NAME test_types COMMAND test_types)

# --- Internal tests (#include .c files for static function access) ---
# These do NOT link against libssz — the tested source file is #included
# directly, and all other sources are compiled as explicit dependencies.

set(SSZ_COMMON_SOURCES
  src/ssz_types.c
  src/ssz_serialize.c
  src/ssz_deserialize.c
  src/ssz_merkle.c
  src/ssz_proof.c
  src/ssz_hash.c
  src/ssz_endian.c
  external/hacl/src/Hacl_Hash_SHA2.c
)

# test_proof_internal: #includes ssz_proof.c — compile everything else
set(SSZ_SOURCES_NO_PROOF ${SSZ_COMMON_SOURCES})
list(REMOVE_ITEM SSZ_SOURCES_NO_PROOF src/ssz_proof.c)

add_executable(test_proof_internal tests/unit/test_proof_internal.c ${SSZ_SOURCES_NO_PROOF})
target_include_directories(test_proof_internal PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${SSZ_HACL_INCLUDE_DIRS}
)
target_link_libraries(test_proof_internal PRIVATE ${SSZ_PLATFORM_LIBS})
add_test(NAME test_proof_internal COMMAND test_proof_internal)

# test_deserialize_internal: #includes ssz_deserialize.c — compile everything else
set(SSZ_SOURCES_NO_DESER ${SSZ_COMMON_SOURCES})
list(REMOVE_ITEM SSZ_SOURCES_NO_DESER src/ssz_deserialize.c)

add_executable(test_deserialize_internal tests/unit/test_deserialize_internal.c ${SSZ_SOURCES_NO_DESER})
target_include_directories(test_deserialize_internal PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${SSZ_HACL_INCLUDE_DIRS}
)
target_link_libraries(test_deserialize_internal PRIVATE ${SSZ_PLATFORM_LIBS})
add_test(NAME test_deserialize_internal COMMAND test_deserialize_internal)

# --- Consensus-spec SSZ generic fixture tests ---
set(SSZ_SPEC_FIXTURES_DIR
  "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/general/phase0/ssz_generic"
  CACHE PATH
  "Path to consensus-spec SSZ generic fixtures root"
)

if(EXISTS "${SSZ_SPEC_FIXTURES_DIR}")
  message(STATUS "Spec fixtures found at ${SSZ_SPEC_FIXTURES_DIR} — registering spec tests")

  set(SSZ_SPEC_COMMON_SOURCES
    tests/spec/spec_common.c
    tests/spec/snappy_decode.c
    tests/spec/yaml_parser.c
  )

  function(ssz_add_spec_test target_name fixture_handler source_file)
    file(TO_CMAKE_PATH "${SSZ_SPEC_FIXTURES_DIR}/${fixture_handler}" SPEC_TESTS_DIR)

    add_executable(${target_name}
      ${source_file}
      ${SSZ_SPEC_COMMON_SOURCES}
    )

    target_include_directories(${target_name} PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}/tests
    )

    target_compile_definitions(${target_name} PRIVATE TESTS_DIR="${SPEC_TESTS_DIR}")
    target_link_libraries(${target_name} PRIVATE ssz)
    add_test(NAME ${target_name} COMMAND ${target_name})
  endfunction()

  ssz_add_spec_test(
    test_ssz_generic_uints
    uints
    tests/spec/test_ssz_generic_uints.c
  )

  ssz_add_spec_test(
    test_ssz_generic_boolean
    boolean
    tests/spec/test_ssz_generic_boolean.c
  )

  ssz_add_spec_test(
    test_ssz_generic_bitvector
    bitvector
    tests/spec/test_ssz_generic_bitvector.c
  )

  ssz_add_spec_test(
    test_ssz_generic_bitlist
    bitlist
    tests/spec/test_ssz_generic_bitlist.c
  )

  ssz_add_spec_test(
    test_ssz_generic_basic_vector
    basic_vector
    tests/spec/test_ssz_generic_basic_vector.c
  )

  ssz_add_spec_test(
    test_ssz_generic_containers
    containers
    tests/spec/test_ssz_generic_containers.c
  )

  ssz_add_spec_test(
    test_ssz_generic_compatible_unions
    compatible_unions
    tests/spec/test_ssz_generic_compatible_unions.c
  )

  ssz_add_spec_test(
    test_ssz_generic_progressive_bitlist
    progressive_bitlist
    tests/spec/test_ssz_generic_progressive_bitlist.c
  )

  ssz_add_spec_test(
    test_ssz_generic_basic_progressive_list
    basic_progressive_list
    tests/spec/test_ssz_generic_basic_progressive_list.c
  )

  ssz_add_spec_test(
    test_ssz_generic_progressive_containers
    progressive_containers
    tests/spec/test_ssz_generic_progressive_containers.c
  )
else()
  message(STATUS "Spec fixtures not found at ${SSZ_SPEC_FIXTURES_DIR} — skipping spec tests")
endif()

find_package(Doxygen QUIET)
if(DOXYGEN_FOUND)
  add_custom_target(doxygen
    COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Running Doxygen — warnings indicate undocumented public symbols"
    USES_TERMINAL
  )
endif()

# --- Benchmarks (ubench.h) ---
option(SSZ_BUILD_BENCH "Build ubench.h micro-benchmarks" OFF)
if(SSZ_BUILD_BENCH)
  file(GLOB SSZ_BENCH_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/bench/bench_*.c)
  foreach(bench_src ${SSZ_BENCH_SOURCES})
    get_filename_component(bench_name ${bench_src} NAME_WE)
    add_executable(${bench_name} ${bench_src})
    # ubench.h uses GNU asm volatile — needs C extensions enabled
    set_target_properties(${bench_name} PROPERTIES C_EXTENSIONS ON)
    target_include_directories(${bench_name} PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}/bench
      ${CMAKE_CURRENT_SOURCE_DIR}/include
      ${CMAKE_CURRENT_SOURCE_DIR}/lib
    )
    target_link_libraries(${bench_name} PRIVATE ssz ${SSZ_PLATFORM_LIBS})
  endforeach()
endif()
