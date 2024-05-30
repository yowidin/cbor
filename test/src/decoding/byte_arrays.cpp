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

using vec_t = std::vector<std::byte>;
using array_t = std::array<std::byte, 4>;

TEST_CASE("Byte arrays - vector error cases", "[decoding, byte_array, vector, errors]") {
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

TEST_CASE("Byte arrays - array error cases", "[decoding, byte_array, vector, errors]") {
   SECTION("Not enough data to read head") {
      std::array<std::byte, 2> source{};
      cbor::read_buffer buf{span_t{source.data(), 0}};

      array_t v;
      REQUIRE(cbor::decode(buf, v) == cbor::error::buffer_underflow);
   }

   SECTION("Invalid type") {
      std::array source{0x20_b};
      cbor::read_buffer buf{span_t{source}};

      array_t v;
      REQUIRE(cbor::decode(buf, v) == cbor::error::unexpected_type);
   }

   SECTION("Size above max size") {
      std::array source{0x42_b, 0x01_b, 0x02_b};
      cbor::read_buffer buf{span_t{source}};

      std::array<std::byte, 1> v{};
      REQUIRE(cbor::decode(buf, v) == cbor::error::buffer_overflow);
   }

   SECTION("Size less than max size") {
      std::array source{0x42_b, 0x01_b, 0x02_b};
      cbor::read_buffer buf{span_t{source}};

      std::array<std::byte, 3> v{};
      REQUIRE(cbor::decode(buf, v) == cbor::error::buffer_underflow);
   }
}

TEST_CASE("Byte arrays - vector basic decoding", "[decoding, byte_array, vector]") {
   auto expect_vector = [](std::initializer_list<std::uint8_t> cbor, std::initializer_list<std::uint8_t> expected) {
      INFO("Decoding '" << hex(cbor) << "' into '" << hex(expected) << "'");

      auto cbor_bytes = as_bytes(cbor);
      cbor::read_buffer buf{span_t{cbor_bytes}};

      std::vector<std::byte> v;
      auto res = cbor::decode(buf, v);
      REQUIRE(res == cbor::error::success);

      compare_containers(cbor, v, expected);
   };

   expect_vector({0x40}, {});
   expect_vector({0x44, 0x01, 0x02, 0x03, 0x04}, {0x01, 0x02, 0x03, 0x04});
}

template <std::size_t Extent>
void expect_array(std::initializer_list<std::uint8_t> cbor, std::initializer_list<std::uint8_t> expected) {
   INFO("Decoding '" << hex(cbor) << "' into '" << hex(expected) << "'");

   auto cbor_bytes = as_bytes(cbor);
   auto expected_bytes = as_bytes(expected);

   cbor::read_buffer buf{span_t{cbor_bytes}};

   std::array<std::byte, Extent> v{};
   auto res = cbor::decode(buf, v);
   REQUIRE(res == cbor::error::success);

   compare_containers(cbor, v, expected);
}

TEST_CASE("Byte arrays - array basic decoding", "[decoding, byte_array, array]") {
   expect_array<0>({0x40}, {});
   expect_array<4>({0x44, 0x01, 0x02, 0x03, 0x04}, {0x01, 0x02, 0x03, 0x04});
}
