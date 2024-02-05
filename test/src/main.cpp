/**
 * @file   main.cpp
 * @author Dennis Sitelew 
 * @date   Feb 05, 2024
 */

#include <catch2/catch_test_macros.hpp>

#include <cbor/library.h>


TEST_CASE("Library is callable", "[api]") {
    REQUIRE(cbor::hello() == 42);
}