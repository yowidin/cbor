/**
 * @file   arrays.cpp
 * @author Dennis Sitelew 
 * @date   Jun 01, 2024
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <test/decoding.h>

#include <cbor/decoding.h>

using namespace test;

namespace {

template <std::size_t Extent>
void expect_array(std::initializer_list<std::uint8_t> cbor, std::initializer_list<std::uint32_t> expected) {
   INFO("Decoding '" << hex(cbor) << "' into '" << hex(expected) << "'");

   auto cbor_bytes = as_bytes(cbor);
   cbor::read_buffer buf{span_t{cbor_bytes}};

   std::array<std::uint32_t, Extent> v{};

   auto res = cbor::decode(buf, v);
   INFO("Decoding result: " << res.message());
   REQUIRE(res == cbor::error::success);

   compare_containers(cbor, v, expected);
}
} // namespace

TEST_CASE("Array - basic decoding - array and span", "[decoding, array, span]") {
   auto expect_span = [](std::initializer_list<std::uint8_t> cbor, std::initializer_list<std::uint32_t> expected) {
      INFO("Decoding '" << hex(cbor) << "' into '" << hex(expected) << "'");

      auto cbor_bytes = as_bytes(cbor);
      cbor::read_buffer buf{span_t{cbor_bytes}};

      std::vector<std::uint32_t> v;
      v.resize(expected.size());

      auto res = cbor::decode(buf, std::span{v});
      INFO("Decoding result: " << res.message());
      REQUIRE(res == cbor::error::success);

      compare_containers(cbor, v, expected);
   };

   expect_span({0x80}, {});
   expect_array<0>({0x80}, {});

   expect_span({0x83, 0x01, 0x02, 0x03}, {1, 2, 3});
   expect_array<3>({0x83, 0x01, 0x02, 0x03}, {1, 2, 3});

   const std::initializer_list<std::uint8_t> encoded{0x98, 0x1A, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09,
                                                     0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14,
                                                     0x15, 0x16, 0x17, 0x18, 0x18, 0x18, 0x19, 0x19, 0x03, 0xE8};
   const std::initializer_list<std::uint32_t> expected{1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13,
                                                       14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 1000};
   expect_span(encoded, expected);
   expect_array<26>(encoded, expected);
}

TEST_CASE("Array - basic decoding - vector", "[decoding, array, vector]") {
   auto expect_vector = [](std::initializer_list<std::uint8_t> cbor, std::initializer_list<std::uint32_t> expected) {
      INFO("Decoding '" << hex(cbor) << "' into '" << hex(expected) << "'");

      auto cbor_bytes = as_bytes(cbor);
      cbor::read_buffer buf{span_t{cbor_bytes}};

      std::vector<std::uint32_t> v;

      auto res = cbor::decode(buf, v);
      INFO("Decoding result: " << res.message());
      REQUIRE(res == cbor::error::success);

      compare_containers(cbor, v, expected);
   };

   expect_vector({0x80}, {});
   expect_vector({0x83, 0x01, 0x02, 0x03}, {1, 2, 3});

   const std::initializer_list<std::uint8_t> encoded{0x98, 0x1A, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09,
                                                     0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14,
                                                     0x15, 0x16, 0x17, 0x18, 0x18, 0x18, 0x19, 0x19, 0x03, 0xE8};
   const std::initializer_list<std::uint32_t> expected{1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13,
                                                       14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 1000};
   expect_vector(encoded, expected);
}

TEST_CASE("Array - decoding error cases - array and span", "[decoding, array, span, errors]") {
   std::array<std::uint32_t, 2> array{};
   auto v = std::span{array};

   SECTION("Not enough data to read head") {
      std::array<std::byte, 2> source{};
      cbor::read_buffer buf{span_t{source.data(), 0}};
      REQUIRE(cbor::decode(buf, v) == cbor::error::buffer_underflow);
   }

   SECTION("Invalid major type") {
      std::array source{0x20_b};
      cbor::read_buffer buf{span_t{source}};
      REQUIRE(cbor::decode(buf, v) == cbor::error::unexpected_type);
   }

   SECTION("Size above max size") {
      std::array source{0x83_b, 0x01_b, 0x02_b, 0x03_b};
      cbor::read_buffer buf{span_t{source}};
      REQUIRE(cbor::decode(buf, v) == cbor::error::buffer_overflow);
   }

   SECTION("Size less than max size") {
      std::array source{0x81_b, 0x01_b};
      cbor::read_buffer buf{span_t{source}};
      REQUIRE(cbor::decode(buf, v) == cbor::error::buffer_underflow);
   }

   SECTION("Error reading an element") {
      std::array source{0x82_b};
      cbor::read_buffer buf{span_t{source}};
      REQUIRE(cbor::decode(buf, v) == cbor::error::buffer_underflow);
   }
}

TEST_CASE("Array - decoding error cases - vector", "[decoding, array, vector, errors]") {
   std::vector<std::uint32_t> v{};

   SECTION("Not enough data to read head") {
      std::array<std::byte, 2> source{};
      cbor::read_buffer buf{span_t{source.data(), 0}};
      REQUIRE(cbor::decode(buf, v) == cbor::error::buffer_underflow);
   }

   SECTION("Invalid major type") {
      std::array source{0x20_b};
      cbor::read_buffer buf{span_t{source}};
      REQUIRE(cbor::decode(buf, v) == cbor::error::unexpected_type);
   }

   SECTION("Size above max size") {
      std::array source{0x83_b, 0x01_b, 0x02_b, 0x03_b};
      cbor::read_buffer buf{span_t{source}};
      REQUIRE(cbor::decode(buf, v, 2) == cbor::error::buffer_overflow);
   }

   SECTION("Error reading an element") {
      std::array source{0x82_b};
      cbor::read_buffer buf{span_t{source}};
      REQUIRE(cbor::decode(buf, v) == cbor::error::buffer_underflow);
   }
}
