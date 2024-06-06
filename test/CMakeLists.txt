include(CTest)

set(CMAKE_CXX_STANDARD 20)

Include(FetchContent)
set(FETCHCONTENT_UPDATES_DISCONNECTED ON)
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.5.3
)
FetchContent_MakeAvailable(Catch2)
message(STATUS "Catch2: ${Catch2_SOURCE_DIR}")
list(APPEND CMAKE_MODULE_PATH ${Catch2_SOURCE_DIR}/extras)

FetchContent_Declare(
    shp
    GIT_REPOSITORY https://github.com/yowidin/simple-hex-printer
    GIT_TAG v0.1.2
)
FetchContent_MakeAvailable(shp)

add_executable(cbor_tests
    src/buffer.cpp
    src/error.cpp

    src/decoding/arrays.cpp
    src/decoding/byte_arrays.cpp
    src/decoding/dictionaries.cpp
    src/decoding/enums.cpp
    src/decoding/floats.cpp
    src/decoding/head.cpp
    src/decoding/integers.cpp
    src/decoding/reflection.cpp
    src/decoding/simple_types.cpp
    src/decoding/strings.cpp
    src/decoding/variant.cpp

    src/encoding/array.cpp
    src/encoding/custom_encode.cpp
    src/encoding/float.cpp
    src/encoding/misc.cpp
    src/encoding/reflection.cpp
    src/encoding/variant.cpp
)

target_link_libraries(cbor_tests
    PRIVATE CBOR::library
    PRIVATE Catch2::Catch2WithMain
    PRIVATE SimpleHexPrinter::library
)

target_include_directories(cbor_tests PRIVATE include)

add_test(NAME Catch2Tests COMMAND "tests")