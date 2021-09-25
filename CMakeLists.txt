cmake_minimum_required(VERSION 3.16)
project(abu_mem VERSION 0.1.0)

set(ABU_REPO_PREFIX https://github.com/abu-lib/ CACHE STRING "")

# Boilerplate to access common build utilities
set(abu_base_ver 0.1.2)
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
# configure_file(${abu_base_SOURCE_DIR}/.clang-format ${CMAKE_CURRENT_SOURCE_DIR}/.clang-format COPYONLY)
# configure_file(${abu_base_SOURCE_DIR}/.clang-tidy ${CMAKE_CURRENT_SOURCE_DIR}/.clang-tidy COPYONLY)
# configure_file(${abu_base_SOURCE_DIR}/sonar-project.properties ${CMAKE_CURRENT_SOURCE_DIR}/sonar-project.properties COPYONLY)
# configure_file(${abu_base_SOURCE_DIR}/.github/workflows/main_ci.yml ${CMAKE_CURRENT_SOURCE_DIR}/.github/workflows/main_ci.yml COPYONLY)
# configure_file(${abu_base_SOURCE_DIR}/.github/workflows/pull_request.yml ${CMAKE_CURRENT_SOURCE_DIR}/.github/workflows/pull_request.yml COPYONLY)

# add_custom_target(abu_mem_sync_with_base 
#   DEPENDS
#     ${CMAKE_CURRENT_SOURCE_DIR}/.clang_format
#     ${CMAKE_CURRENT_SOURCE_DIR}/.clang_tidy
#     ${CMAKE_CURRENT_SOURCE_DIR}/.clang_format
#     ${CMAKE_CURRENT_SOURCE_DIR}/.github/workflows/main_ci.yml
#     ${CMAKE_CURRENT_SOURCE_DIR}/.github/workflows/pull_request.yml
# )

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
