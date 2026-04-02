cmake_minimum_required(VERSION 3.16)

project(c_ssz LANGUAGES C)

if(NOT DEFINED CMAKE_EXPORT_COMPILE_COMMANDS)
  set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE BOOL "Export compile commands for tooling" FORCE)
endif()

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

option(SSZ_USE_SYSTEM_CRYPTO "Use system libcrypto (OpenSSL/LibreSSL) instead of AWS-LC" OFF)

if(SSZ_USE_SYSTEM_CRYPTO)
  find_package(OpenSSL REQUIRED)
  set(SSZ_CRYPTO_LIB OpenSSL::Crypto)
else()
  set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
  set(BUILD_LIBSSL OFF CACHE BOOL "" FORCE)
  set(BUILD_TOOL OFF CACHE BOOL "" FORCE)
  set(DISABLE_GO ON CACHE BOOL "" FORCE)
  set(DISABLE_PERL ON CACHE BOOL "" FORCE)
  add_subdirectory(external/aws-lc)
  set(SSZ_CRYPTO_LIB crypto)
endif()

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

add_library(ssz
  src/ssz_types.c
  src/ssz_serialize.c
  src/ssz_deserialize.c
  src/ssz_merkle.c
  src/ssz_merkle_cache.c
  src/ssz_proof.c
  src/ssz_hash.c
  src/ssz_endian.c
)

target_include_directories(ssz
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(ssz PUBLIC ${SSZ_PLATFORM_LIBS} ${SSZ_CRYPTO_LIB})

option(SSZ_BUILD_TESTS "Build c-ssz unit and spec tests" ON)

if(SSZ_BUILD_TESTS)
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

add_executable(test_merkle_cache tests/unit/test_merkle_cache.c)
target_link_libraries(test_merkle_cache PRIVATE ssz)
add_test(NAME test_merkle_cache COMMAND test_merkle_cache)

add_executable(test_proof tests/unit/test_proof.c)
target_link_libraries(test_proof PRIVATE ssz)
add_test(NAME test_proof COMMAND test_proof)

add_executable(test_hash tests/unit/test_hash.c)
target_include_directories(test_hash PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_link_libraries(test_hash PRIVATE ssz)
add_test(NAME test_hash COMMAND test_hash)

add_executable(test_types tests/unit/test_types.c)
target_link_libraries(test_types PRIVATE ssz)
add_test(NAME test_types COMMAND test_types)

add_executable(test_default tests/unit/test_default.c)
target_link_libraries(test_default PRIVATE ssz)
add_test(NAME test_default COMMAND test_default)

# --- Internal tests (#include .c files for static function access) ---
# These do NOT link against libssz — the tested source file is #included
# directly, and all other sources are compiled as explicit dependencies.

set(SSZ_COMMON_SOURCES
  src/ssz_types.c
  src/ssz_serialize.c
  src/ssz_deserialize.c
  src/ssz_merkle.c
  src/ssz_merkle_cache.c
  src/ssz_proof.c
  src/ssz_hash.c
  src/ssz_endian.c
)

# test_proof_internal: #includes ssz_proof.c — compile everything else
set(SSZ_SOURCES_NO_PROOF ${SSZ_COMMON_SOURCES})
list(REMOVE_ITEM SSZ_SOURCES_NO_PROOF src/ssz_proof.c)

add_executable(test_proof_internal tests/unit/test_proof_internal.c ${SSZ_SOURCES_NO_PROOF})
target_include_directories(test_proof_internal PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_link_libraries(test_proof_internal PRIVATE ${SSZ_PLATFORM_LIBS} ${SSZ_CRYPTO_LIB})
add_test(NAME test_proof_internal COMMAND test_proof_internal)

# test_merkle_internal: #includes ssz_merkle.c — compile everything else
set(SSZ_SOURCES_NO_MERKLE ${SSZ_COMMON_SOURCES})
list(REMOVE_ITEM SSZ_SOURCES_NO_MERKLE src/ssz_merkle.c)

add_executable(
  test_merkle_internal
  tests/unit/test_merkle_internal.c
  ${SSZ_SOURCES_NO_MERKLE}
)
target_include_directories(test_merkle_internal PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_link_libraries(test_merkle_internal PRIVATE ${SSZ_PLATFORM_LIBS} ${SSZ_CRYPTO_LIB})
add_test(NAME test_merkle_internal COMMAND test_merkle_internal)

# test_types_internal: #includes ssz_types.c — compile everything else
set(SSZ_SOURCES_NO_TYPES ${SSZ_COMMON_SOURCES})
list(REMOVE_ITEM SSZ_SOURCES_NO_TYPES src/ssz_types.c)

add_executable(
  test_types_internal
  tests/unit/test_types_internal.c
  ${SSZ_SOURCES_NO_TYPES}
)
target_include_directories(test_types_internal PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_link_libraries(test_types_internal PRIVATE ${SSZ_PLATFORM_LIBS} ${SSZ_CRYPTO_LIB})
add_test(NAME test_types_internal COMMAND test_types_internal)

# test_merkle_cache_internal: #includes ssz_merkle_cache.c — compile everything else
set(SSZ_SOURCES_NO_MERKLE_CACHE ${SSZ_COMMON_SOURCES})
list(REMOVE_ITEM SSZ_SOURCES_NO_MERKLE_CACHE src/ssz_merkle_cache.c)

add_executable(
  test_merkle_cache_internal
  tests/unit/test_merkle_cache_internal.c
  ${SSZ_SOURCES_NO_MERKLE_CACHE}
)
target_include_directories(test_merkle_cache_internal PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_link_libraries(test_merkle_cache_internal PRIVATE ${SSZ_PLATFORM_LIBS} ${SSZ_CRYPTO_LIB})
add_test(NAME test_merkle_cache_internal COMMAND test_merkle_cache_internal)


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

endif() # SSZ_BUILD_TESTS

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
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench/bench_arena_compare.c")
    list(APPEND SSZ_BENCH_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/bench/bench_arena_compare.c)
  endif()
  list(REMOVE_DUPLICATES SSZ_BENCH_SOURCES)
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

# --- libFuzzer harnesses ---
option(SSZ_BUILD_FUZZ "Build libFuzzer harnesses" OFF)
if(SSZ_BUILD_FUZZ)
  if(NOT CMAKE_C_COMPILER_ID MATCHES "Clang|AppleClang")
    message(FATAL_ERROR "SSZ_BUILD_FUZZ requires Clang or AppleClang.")
  endif()

  file(GLOB SSZ_FUZZ_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/fuzz/fuzz_*.c)
  foreach(fuzz_src ${SSZ_FUZZ_SOURCES})
    get_filename_component(fuzz_name ${fuzz_src} NAME_WE)
    add_executable(${fuzz_name} ${fuzz_src} ${SSZ_COMMON_SOURCES})
    target_include_directories(${fuzz_name} PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}/include
      ${CMAKE_CURRENT_SOURCE_DIR}/src
    )
    target_compile_options(${fuzz_name} PRIVATE -fsanitize=fuzzer,address)
    target_link_options(${fuzz_name} PRIVATE -fsanitize=fuzzer,address)
    target_link_libraries(${fuzz_name} PRIVATE ${SSZ_PLATFORM_LIBS} ${SSZ_CRYPTO_LIB})
  endforeach()
endif()
