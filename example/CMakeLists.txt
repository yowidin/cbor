cmake_minimum_required(VERSION 3.16)
project(cbor_example)

set(CMAKE_CXX_STANDARD 20)

# Fetch the CBOR library
include(FetchContent)
FetchContent_Declare(
    cbor
    SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/../
)
FetchContent_MakeAvailable(cbor)

# Also fetch the Simple Hex Printer Library for easier printing (not required).
FetchContent_Declare(
    shp
    GIT_REPOSITORY https://github.com/yowidin/simple-hex-printer
    GIT_TAG v0.1.2
)
FetchContent_MakeAvailable(shp)

# Gather all dependencies
add_library(dependencies INTERFACE)
target_link_libraries(dependencies INTERFACE CBOR::library SimpleHexPrinter::library)

# Add client-server example
add_executable(client-server client-server.cpp)
target_link_libraries(client-server PRIVATE dependencies)

# Add simple example
add_executable(simple simple.cpp)
target_link_libraries(simple PRIVATE dependencies)
