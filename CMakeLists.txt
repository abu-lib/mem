cmake_minimum_required(VERSION 3.16)
project(abu_mem VERSION 0.1.0)

set(ABU_REPO_PREFIX https://github.com/abu-lib/ CACHE STRING "")

# Boilerplate to access common build utilities
set(abu_base_ver 0.1.4)
include(FetchContent)
if(NOT TARGET abu_base)
  message(STATUS "abu_base not found, fetching it from github")
  FetchContent_Declare(abu_base
    GIT_REPOSITORY ${ABU_REPO_PREFIX}base.git 
    GIT_TAG ${abu_base_ver}
  )
  FetchContent_MakeAvailable(abu_base)
endif()
# Boilerplate end

abu_install_library_boilerplate()

enable_testing()

abu_add_library(
  DEPENDS
    base>=${abu_base_ver}
  PUBLIC_HEADERS
    abu/mem/check.h
    abu/mem/ref_count_ptr.h
    abu/mem.h
  TESTS
    tests/test_ref_count_ptr.cpp
  BENCHMARKS
    benchmarks/benchmark_ref_counted_ptr.cpp
)
