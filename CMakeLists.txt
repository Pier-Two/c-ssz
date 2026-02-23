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

enable_testing()

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
    target_link_libraries(${bench_name} PRIVATE ${SSZ_PLATFORM_LIBS})
  endforeach()
endif()
