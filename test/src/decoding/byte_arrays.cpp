/**
 * @file   byte_arrays.cpp
 * @author Dennis Sitelew 
 * @date   May 20, 2024
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <test/decoding.h>

#include <cbor/decoding.h>

using namespace test;

using span_t = cbor::buffer::const_span_t;
using vec_t = std::vector<std::byte>;

TEST_CASE("Byte arrays - error cases", "[decoding, byte_array, errors]") {
   SECTION("Not enough data to read head") {
      std::array<std::byte, 2> source{};
      cbor::read_buffer buf{span_t{source.data(), 0}};

      vec_t v;
      REQUIRE(cbor::decode(buf, v) == cbor::error::buffer_underflow);
   }

   SECTION("Invalid type") {
      std::array source{0x20_b};
      cbor::read_buffer buf{span_t{source}};

      vec_t v;
      REQUIRE(cbor::decode(buf, v) == cbor::error::unexpected_type);
   }

   SECTION("Size above max size") {
      std::array source{0x42_b, 0x01_b, 0x02_b};
      cbor::read_buffer buf{span_t{source}};

      vec_t v;
      REQUIRE(cbor::decode(buf, v, 1) == cbor::error::buffer_overflow);
   }
}

TEST_CASE("Byte arrays - basic decoding", "[decoding, byte_array]") {
   auto expect_array = [](std::initializer_list<std::uint8_t> cbor, std::initializer_list<std::uint8_t> expected) {
      auto cbor_bytes = as_bytes(cbor);
      auto expected_bytes = as_bytes(expected);

      cbor::read_buffer buf{span_t{cbor_bytes}};

      std::vector<std::byte> v;
      auto res = cbor::decode(buf, v);
      if (res) {
         std::cerr << "Decoding failed for:\n";
         std::cerr << shp::hex(cbor_bytes) << "\n\n";
         std::cerr << "Expected:\n";
         std::cerr << shp::hex(expected) << "\n\n";
         std::cerr << "With error: " << res.message() << std::endl;
      }
      REQUIRE(res == cbor::error::success);

      REQUIRE(v.size() == expected.size());

      const auto v_begin = v.begin();
      const auto v_end = v.end();

      const auto e_begin = expected_bytes.begin();
      const auto e_end = expected_bytes.end();

      auto m = std::mismatch(v_begin, v_end, e_begin);
      if (m.first != v_end || m.second != e_end) {
         std::cerr << "Array missmatch for:\n";
         std::cerr << shp::hex(cbor_bytes) << "\n\n";
         std::cerr << "Expected:\n";
         std::cerr << shp::hex(expected) << "\n\nFound:\n";
         std::cerr << shp::hex(v) << std::endl;
      }
      REQUIRE(m.first == v_end);
      REQUIRE(m.second == e_end);
   };

   expect_array({0x40}, {});
   expect_array({0x44, 0x01, 0x02, 0x03, 0x04}, {0x01, 0x02, 0x03, 0x04});
}
