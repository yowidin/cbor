include(CTest)

Include(FetchContent)
set(FETCHCONTENT_UPDATES_DISCONNECTED ON)
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.5.2
)

FetchContent_MakeAvailable(Catch2)
message(STATUS "Catch2: ${Catch2_SOURCE_DIR}")
list(APPEND CMAKE_MODULE_PATH ${Catch2_SOURCE_DIR}/extras)

add_executable(tests
    src/buffer.cpp
    src/encoding.cpp
    src/main.cpp
)

set_target_properties(tests PROPERTIES CXX_STANDARD 20)

target_link_libraries(tests
    PRIVATE CBOR::library
    PRIVATE Catch2::Catch2WithMain
)

add_test(NAME Catch2Tests COMMAND "tests")