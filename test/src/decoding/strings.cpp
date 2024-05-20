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
   auto expect_string = [](std::initializer_list<std::uint8_t> cbor, std::string_view expected) {
      auto cbor_bytes = as_bytes(cbor);
      auto expected_bytes = as_bytes(expected);

      cbor::read_buffer buf{span_t{cbor_bytes}};

      string_t v;
      auto res = cbor::decode(buf, v);
      if (res) {
         std::cerr << "Decoding failed for:\n";
         std::cerr << shp::hex(cbor_bytes) << "\n\n";
         std::cerr << "Expected:\n";
         std::cerr << '"' << expected << '"' << "\n\n";
         std::cerr << "With error: " << res.message() << std::endl;
      }
      REQUIRE(res == cbor::error::success);

      REQUIRE(v.size() == expected.size());

      if (v != expected) {
         std::cerr << "String missmatch for:\n";
         std::cerr << shp::hex(cbor_bytes) << "\n\n";
         std::cerr << "Expected:\n";
         std::cerr << '"' << expected << '"' << "\n\nFound:\n";
         std::cerr << v << std::endl;
      }
      REQUIRE(v == expected);
   };

   expect_string({0x60}, "");
   expect_string({0x64, 0x31, 0x32, 0x33, 0x34}, "1234");

   expect_string({0x61, 0x61}, "a");
   expect_string({0x64, 0x49, 0x45, 0x54, 0x46}, "IETF");
   expect_string({0x62, 0x22, 0x5C}, "\"\\");
   expect_string({0x62, 0xC3, 0xBC}, "\u00fc");
   expect_string({0x63, 0xE6, 0xB0, 0xB4}, "\u6c34");
}
