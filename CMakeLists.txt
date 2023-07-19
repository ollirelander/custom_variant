cmake_minimum_required(VERSION 3.2)

project(
    custom_variant
    VERSION "0.1"
    HOMEPAGE_URL "https://github.com/ollirelander/custom_variant"
    DESCRIPTION "single-header C++14 library for creating an custom variant type that provides a fast and simple variant implementation"
    LANGUAGES CXX
)

option(CUSTOM_VARIANT_OPT_BUILD_EXAMPLES "Build custom_variant examples" ON)

if(CUSTOM_VARIANT_OPT_BUILD_EXAMPLES)
    add_subdirectory(example)
endif()

add_library(${PROJECT_NAME} INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include/custom_variant.hpp")
target_include_directories(${PROJECT_NAME} INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")
