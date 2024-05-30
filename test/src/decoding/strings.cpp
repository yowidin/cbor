/**
 * @file   strings.cpp
 * @author Dennis Sitelew 
 * @date   May 20, 2024
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <test/decoding.h>

#include <cbor/decoding.h>

using namespace test;

using span_t = cbor::buffer::const_span_t;
using string_t = std::string;

TEST_CASE("Strings - error cases", "[decoding, string, errors]") {
   SECTION("Not enough data to read head") {
      std::array<std::byte, 2> source{};
      cbor::read_buffer buf{span_t{source.data(), 0}};

      string_t v;
      REQUIRE(cbor::decode(buf, v) == cbor::error::buffer_underflow);
   }

   SECTION("Invalid type") {
      std::array source{0x20_b};
      cbor::read_buffer buf{span_t{source}};

      string_t v;
      REQUIRE(cbor::decode(buf, v) == cbor::error::unexpected_type);
   }

   SECTION("Size above max size") {
      std::array source{0x62_b, 0x01_b, 0x02_b};
      cbor::read_buffer buf{span_t{source}};

      string_t v;
      REQUIRE(cbor::decode(buf, v, 1) == cbor::error::buffer_overflow);
   }
}

TEST_CASE("Strings - basic decoding", "[decoding, string]") {
   using namespace std::literals::string_literals;
   expect({0x60}, ""s);
   expect({0x64, 0x31, 0x32, 0x33, 0x34}, "1234"s);

   expect({0x61, 0x61}, "a"s);
   expect({0x64, 0x49, 0x45, 0x54, 0x46}, "IETF"s);
   expect({0x62, 0x22, 0x5C}, R"("\)"s);
   expect({0x62, 0xC3, 0xBC}, "\u00fc"s);
   expect({0x63, 0xE6, 0xB0, 0xB4}, "\u6c34"s);
}
