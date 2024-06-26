/**
 * @file   simple_types.cpp
 * @author Dennis Sitelew 
 * @date   May 22, 2024
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <test/decoding.h>

#include <cbor/decoding.h>

#include <iostream>

using namespace test;

TEST_CASE("Boolean - decoding errors", "[decoding, boolean, errors]") {
   SECTION("Not enough data to read head") {
      std::array<std::byte, 2> source{};
      cbor::read_buffer buf{span_t{source.data(), 0}};

      bool v;
      REQUIRE(cbor::decode(buf, v) == cbor::error::buffer_underflow);
   }

   SECTION("Unexpected major type") {
      std::array source{0x39_b, 0x3E_b, 0xE8_b};
      cbor::read_buffer buf{span_t{source}};

      bool v;
      REQUIRE(cbor::decode(buf, v) == cbor::error::unexpected_type);
   }

   SECTION("Unexpected simple type") {
      std::array source{0xF6_b};
      cbor::read_buffer buf{span_t{source}};

      bool v;
      REQUIRE(cbor::decode(buf, v) == cbor::error::unexpected_type);
   }
}

TEST_CASE("Boolean - normal decoding", "[decoding, boolean]") {
   expect({0xF4}, false);
   expect({0xF5}, true);
}

TEST_CASE("Optional - decoding errors", "[decoding, optional, errors]") {
   SECTION("Not enough data to read head") {
      std::array<std::byte, 2> source{};
      cbor::read_buffer buf{span_t{source.data(), 0}};

      std::optional<bool> v;
      REQUIRE(cbor::decode(buf, v) == cbor::error::buffer_underflow);
   }

   SECTION("Bool decoding error - unexpected type") {
      std::array source{0x39_b, 0x3E_b, 0xE8_b};
      cbor::read_buffer buf{span_t{source}};

      std::optional<bool> v;
      REQUIRE(cbor::decode(buf, v) == cbor::error::unexpected_type);
   }
}

TEST_CASE("Optional - decoding", "[decoding, optional]") {
   expect({0xF4}, std::optional<bool>{false});
   expect({0xF5}, std::optional<bool>{true});

   expect({0xF6}, std::optional<bool>{});

   expect({0xF6}, std::optional<int>{});
}
