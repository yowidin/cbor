/**
 * @file   array.cpp
 * @author Dennis Sitelew 
 * @date   Apr 08, 2024
 */

#include <catch2/catch_test_macros.hpp>

#include <test/encoding.h>

using namespace test;

TEST_CASE("Array", "[encoding, array]") {
   // []
   const std::vector<std::uint32_t> a1{};
   check_encoding(a1, {0x80});
   check_encoding(std::span{a1}, {0x80});

   // [1, 2, 3]
   const std::array<std::uint8_t, 3> a2{1, 2, 3};
   check_encoding(a2, {0x83, 0x01, 0x02, 0x03});
   check_encoding(std::span{a2}, {0x83, 0x01, 0x02, 0x03});

   // [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25]
   const std::vector a3{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25};
   const std::initializer_list<std::uint8_t> expected{0x98, 0x19, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                                                      0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12,
                                                      0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x18, 0x18, 0x19};
   check_encoding(a3, expected);
   check_encoding(std::span{a3}, expected);
}

TEST_CASE("Array - rollback on failure", "[encoding, array, rollback]") {
   // [1, 2, 3] -> {0x83, 0x01, 0x02, 0x03}
   const std::array<std::uint8_t, 3> a2{1, 2, 3};

   SECTION("No space for size") {
      std::vector<std::byte> target{};
      cbor::dynamic_buffer buf{target, 0};

      std::error_code ec;
      ec = cbor::encode(buf, a2);
      REQUIRE(ec == cbor::error::buffer_overflow);
      REQUIRE(target.empty());
   }

   SECTION("No space for one entry") {
      std::vector<std::byte> target{};
      cbor::dynamic_buffer buf{target, 3};

      std::error_code ec;
      ec = cbor::encode(buf, a2);
      REQUIRE(ec == cbor::error::buffer_overflow);
      REQUIRE(target.empty());
   }
}
