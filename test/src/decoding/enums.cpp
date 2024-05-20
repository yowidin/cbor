/**
 * @file   enums.cpp
 * @author Dennis Sitelew 
 * @date   May 20, 2024
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <test/decoding.h>

#include <cbor/decoding.h>

using namespace test;

using span_t = cbor::buffer::const_span_t;

TEST_CASE("Enums - basic decoding", "[decoding, enums]") {
   enum class long_class : long {
      a = -10,
      b = 23,
   };

   enum long_simple : long {
      ls_a = -10,
      ls_b = 23,
   };

   enum class untyped {
      a = -10,
      b = 23,
   };

   expect({0x29}, long_class::a);
   expect({0x17}, long_class::b);

   expect({0x29}, untyped::a);
   expect({0x17}, untyped::b);

   expect({0x29}, ls_a);
   expect({0x17}, ls_b);

   SECTION("Not enough data to read head") {
      std::array<std::byte, 2> source{};
      cbor::read_buffer buf{span_t{source.data(), 0}};

      long_class v;
      REQUIRE(cbor::decode(buf, v) == cbor::error::buffer_underflow);
   }
}
