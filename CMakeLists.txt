cmake_minimum_required(VERSION 3.12 FATAL_ERROR)
project(CBOR VERSION 0.0.1)

# --- Some CMake boilerplate --- #
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)
set(CMAKE_DISABLE_SOURCE_CHANGES ON)
if("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
    message(SEND_ERROR "In-source builds are not allowed.")
endif()

set(CBOR_GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(CBOR_GENERATED_INCLUDE_DIR "${CBOR_GENERATED_DIR}/include")
set(CBOR_GENERATED_CMAKE_DIR "${CBOR_GENERATED_DIR}/cmake")

set(CBOR_GENERATED_EXPORT_HEADER "${CBOR_GENERATED_INCLUDE_DIR}/cbor/export.h")
file(MAKE_DIRECTORY ${CBOR_GENERATED_INCLUDE_DIR})

# --- Actual library --- #
add_library(cbor
    src/library.cpp

    src/error.cpp
)

target_include_directories(cbor
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/>
    PUBLIC $<BUILD_INTERFACE:${CBOR_GENERATED_INCLUDE_DIR}>

    PUBLIC $<INSTALL_INTERFACE:include>
)

set_target_properties(cbor PROPERTIES
    OUTPUT_NAME cbor

    CXX_STANDARD 20
)

# --- Testing --- #
if(BUILD_TESTS)
    include(CTest)
    enable_testing()
    add_subdirectory(test)
endif()


# --- Configure installation settings --- #
include(GenerateExportHeader)

include(cmake/install_helper.cmake)
include(cmake/package_helper.cmake)