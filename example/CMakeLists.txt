cmake_minimum_required(VERSION 3.16)
project(cbor_example)

set(CMAKE_CXX_STANDARD 20)

include(FetchContent)
FetchContent_Declare(
    cbor
    SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/../
)
FetchContent_MakeAvailable(cbor)

FetchContent_Declare(
    shp
    GIT_REPOSITORY https://github.com/yowidin/simple-hex-printer
    GIT_TAG v0.1.2
)
FetchContent_MakeAvailable(shp)

add_executable(example main.cpp)
target_link_libraries(example PRIVATE CBOR::library SimpleHexPrinter::library)
