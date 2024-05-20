/**
 * @file   head.cpp
 * @author Dennis Sitelew 
 * @date   May 19, 2024
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <test/decoding.h>

#include <cbor/decoding.h>

using namespace test;

using span_t = cbor::buffer::const_span_t;
using head_t = cbor::detail::head;

TEST_CASE("Head decoding - buffer underflow", "[decoding, head, underflow]") {
   SECTION("Not enough data to read the head") {
      std::array<std::byte, 1U> source{};
      cbor::read_buffer buf{span_t{source.data(), 0U}};

      head_t h{};
      std::error_code ec = h.read(buf);
      REQUIRE(ec == cbor::error::buffer_underflow);
      REQUIRE(buf.read_position() == 0U);
   }

   SECTION("Not enough data to read the argument") {
      // Argument should be encoded in one byte, but we have none
      std::array source{0x18_b};
      cbor::read_buffer buf{span_t{source}};

      head_t h{};
      std::error_code ec = h.read(buf);
      REQUIRE(ec == cbor::error::buffer_underflow);
      REQUIRE(buf.read_position() == 1U);
   }
}

TEST_CASE("Head decoding - size extraction", "[decoding, head, size]") {
   SECTION("0 extra bytes") {
      std::array source{as_byte(cbor::detail::ZERO_EXTRA_BYTES_VALUE_LIMIT)};
      cbor::read_buffer buf{span_t{source}};

      head_t h{};
      std::error_code ec = h.read(buf);
      REQUIRE(ec == cbor::error::success);
      REQUIRE(buf.read_position() == 1U);
      REQUIRE(h.type == cbor::major_type::unsigned_int);
      REQUIRE(h.raw == cbor::detail::ZERO_EXTRA_BYTES_VALUE_LIMIT);
      REQUIRE(h.extra_bytes == 0U);
   }

   SECTION("1 extra byte") {
      std::array source{0x18_b, 0x1A_b};
      cbor::read_buffer buf{span_t{source}};

      head_t h{};
      std::error_code ec = h.read(buf);
      REQUIRE(ec == cbor::error::success);
      REQUIRE(buf.read_position() == 2U);
      REQUIRE(h.type == cbor::major_type::unsigned_int);
      REQUIRE(h.raw == 0x18U);
      REQUIRE(h.extra_bytes == 1U);
      REQUIRE(h.argument[0] == 0x1AU);
   }

   SECTION("2 extra bytes") {
      std::array source{0x39_b, 0x03_b, 0xE8_b};
      cbor::read_buffer buf{span_t{source}};

      head_t h{};
      std::error_code ec = h.read(buf);
      REQUIRE(ec == cbor::error::success);
      REQUIRE(buf.read_position() == 3U);
      REQUIRE(h.type == cbor::major_type::signed_int);
      REQUIRE(h.raw == 0x39U);
      REQUIRE(h.extra_bytes == 2U);
      REQUIRE(h.argument[0] == 0x03U);
      REQUIRE(h.argument[1] == 0xE8U);
   }

   SECTION("4 extra bytes") {
      std::array source{0x5A_b, 0x01_b, 0x02_b, 0x03_b, 0x04_b};
      cbor::read_buffer buf{span_t{source}};

      head_t h{};
      std::error_code ec = h.read(buf);
      REQUIRE(ec == cbor::error::success);
      REQUIRE(buf.read_position() == 5U);
      REQUIRE(h.type == cbor::major_type::byte_string);
      REQUIRE(h.raw == 0x5AU);
      REQUIRE(h.extra_bytes == 4U);
      REQUIRE(h.argument[0] == 0x01U);
      REQUIRE(h.argument[1] == 0x02U);
      REQUIRE(h.argument[2] == 0x03U);
      REQUIRE(h.argument[3] == 0x04U);
   }

   SECTION("8 extra bytes") {
      std::array source{0x9B_b, 0x01_b, 0x02_b, 0x03_b, 0x04_b, 0x05_b, 0x06_b, 0x07_b, 0x08_b};
      cbor::read_buffer buf{span_t{source}};

      head_t h{};
      std::error_code ec = h.read(buf);
      REQUIRE(ec == cbor::error::success);
      REQUIRE(buf.read_position() == 9U);
      REQUIRE(h.type == cbor::major_type::array);
      REQUIRE(h.raw == 0x9BU);
      REQUIRE(h.extra_bytes == 8U);
      for (uint8_t i = 0; i < 8U; ++i) {
         REQUIRE(h.argument[i] == i + 1);
      }
   }

   SECTION("Reserved bytes") {
      auto codec = GENERATE(0x7C, 0x7D, 0x7E);

      std::array source{as_byte(codec)};
      cbor::read_buffer buf{span_t{source}};

      head_t h{};
      std::error_code ec = h.read(buf);
      REQUIRE(ec == cbor::error::ill_formed);
   }
}

TEST_CASE("Head decoding - argument decoding", "[decoding, head, argument]") {
   auto expect = [](std::initializer_list<std::uint8_t> cbor, std::uint64_t expected) {
      auto cbor_bytes = as_bytes(cbor);

      cbor::read_buffer buf{span_t{cbor_bytes}};

      head_t h{};
      REQUIRE(h.read(buf) == cbor::error::success);

      REQUIRE(h.decode_argument() == expected);
   };

   SECTION("0 extra bytes") {
      expect({12}, 12);
   }

   SECTION("1 extra byte") {
      expect({0x18, 0x1A}, 0x1A);
   }

   SECTION("2 extra bytes") {
      expect({0x39, 0x03, 0xE8}, 0x03E8);
   }

   SECTION("4 extra bytes") {
      expect({0x5A, 0xDE, 0xAD, 0xBE, 0xEF}, 0xDEADBEEF);
   }

   SECTION("8 extra bytes") {
      expect({0x1B, 0x00, 0x00, 0x00, 0xE8, 0xD4, 0xA5, 0x10, 0x00}, 0xE8D4A51000);
   }
}
