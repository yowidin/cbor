/**
 * @file   main.cpp
 * @author Dennis Sitelew 
 * @date   Feb 05, 2024
 */

#include <catch2/catch_test_macros.hpp>

#include <cbor/library.h>

#include <iostream>

TEST_CASE("Library is callable", "[api]") {
   auto ec = cbor::hello();
   std::cout << ec << " " << ec.message() << std::endl;
   REQUIRE(!cbor::hello());
}