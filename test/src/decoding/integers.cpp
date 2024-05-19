/**
 * @file   misc.cpp
 * @author Dennis Sitelew 
 * @date   May 10, 2024
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <test/encoding.h>

#include <cbor/decoding.h>

#include <iostream>

using namespace test;

TEST_CASE("Unsigned - decoding errors", "[decoding, unsigned, errors]") {
   using span_t = cbor::buffer::const_span_t;

   SECTION("Not enough data to read head") {
      std::array<std::byte, 2> source{};
      cbor::read_buffer buf{span_t{source.data(), 0}};

      std::uint8_t v;
      REQUIRE(cbor::decode(buf, v) == cbor::error::buffer_underflow);
   }

   SECTION("Unexpected type") {
      std::array source{0x39_b, 0x3E_b, 0xE8_b};
      cbor::read_buffer buf{span_t{source}};

      std::uint8_t v;
      REQUIRE(cbor::decode(buf, v) == cbor::error::unexpected_type);
   }

   SECTION("Value is too big") {
      std::array source{0x19_b, 0x03_b, 0xE8_b};
      cbor::read_buffer buf{span_t{source}};

      std::uint8_t v;
      REQUIRE(cbor::decode(buf, v) == cbor::error::value_not_representable);
   }
}

template <typename T>
void expect(std::initializer_list<std::uint8_t> cbor, T expected) {
   std::vector<std::byte> as_bytes{};
   for (auto v : cbor) {
      as_bytes.push_back(static_cast<std::byte>(v));
   }

   cbor::buffer::const_span_t span{as_bytes};
   cbor::read_buffer buf{span};

   T decoded;
   auto res = cbor::decode(buf, decoded);

   REQUIRE(!res);
   REQUIRE(expected == decoded);
}

TEST_CASE("Unsigned - basic decoding", "[decoding, unsigned]") {
   expect({0x00}, 0U);
   expect({0x01}, 1U);
   expect({0x0A}, 10U);
   expect({0x17}, 23U);
   expect({0x18, 0x18}, 24U);
   expect({0x18, 0x19}, 25U);
   expect({0x18, 0x64}, 100U);
   expect({0x19, 0x03, 0xE8}, 1000U);
   expect({0x1A, 0xDE, 0xAD, 0xBE, 0xEF}, 0xDEADBEEF);
   expect({0x1B, 0x00, 0x00, 0x00, 0xE8, 0xD4, 0xA5, 0x10, 0x00}, static_cast<std::uint64_t>(1000000000000));
   expect({0x1B, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, static_cast<std::uint64_t>(18446744073709551615U));

   // Big integer types with small values should still fit
   expect<std::uint8_t>({0x19, 0x00, 0xE8}, 0xE8);

   // Reading small int into a bigger type should work
   expect<std::uint32_t>({0x19, 0xBE, 0xEF}, 0xBEEF);
}
