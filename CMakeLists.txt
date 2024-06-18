cmake_minimum_required(VERSION 3.12 FATAL_ERROR)
project(CBOR VERSION 0.0.1)

# Fix the issue of missing DLLs on Windows by putting everything into a bin directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

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

set(CBOR_GENERATED_CONFIG_HEADER "${CBOR_GENERATED_INCLUDE_DIR}/cbor/config.h")
set(CBOR_GENERATED_EXPORT_HEADER "${CBOR_GENERATED_INCLUDE_DIR}/cbor/export.h")

# --- Library options --- #
set(CBOR_DYNAMIC_BUFFER_INITIAL_SIZE 8 CACHE STRING "Initial amount of memory to reserve for a dynamic buffer.")
option(CBOR_WITH_BOOST_PFR "Use the Boost PFR for reflection" ON)

file(MAKE_DIRECTORY ${CBOR_GENERATED_INCLUDE_DIR})
configure_file(cmake/config.h.in ${CBOR_GENERATED_CONFIG_HEADER} @ONLY)

# --- Dependencies --- #
include(FetchContent)
FetchContent_Declare(
    fhf
    GIT_REPOSITORY https://github.com/yowidin/fast-half-float
    GIT_TAG v0.0.1
)
FetchContent_MakeAvailable(fhf)

if(CBOR_WITH_BOOST_PFR)
    message(STATUS "Boost PFR is enabled. See limitations: https://www.boost.org/doc/libs/develop/doc/html/boost_pfr/limitations_and_configuration.html")
    FetchContent_Declare(
        boost_pfr

        GIT_REPOSITORY https://github.com/boostorg/pfr
        GIT_TAG 2.2.0
    )
    FetchContent_MakeAvailable(boost_pfr)
endif()

# --- Actual library --- #
add_library(cbor
    src/buffer.cpp
    src/decoding.cpp
    src/encoding.cpp
    src/error.cpp
)

target_include_directories(cbor
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/>
    PUBLIC $<BUILD_INTERFACE:${CBOR_GENERATED_INCLUDE_DIR}>

    PUBLIC $<INSTALL_INTERFACE:include>
)

target_link_libraries(cbor PUBLIC FastHalfFloat::library $<$<BOOL:${CBOR_WITH_BOOST_PFR}>:Boost::pfr>)

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
generate_export_header(cbor
   BASE_NAME CBOR
   EXPORT_FILE_NAME ${CBOR_GENERATED_EXPORT_HEADER}
)

include(cmake/install_helper.cmake)
include(cmake/package_helper.cmake)