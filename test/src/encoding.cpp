/**
 * @file   encoding.cpp
 * @author Dennis Sitelew 
 * @date   Feb 13, 2024
 */

#include <catch2/catch_test_macros.hpp>

#include <cbor/encoding.h>

using namespace cbor;

TEST_CASE("Check argument encoding", "[encoding]") {
   // Ensure that an unsigned int is encoded as the expected byte array
   auto check_encoding = [](std::uint64_t value, std::initializer_list<std::uint8_t> expected) {
      std::vector<std::uint8_t> target{};
      dynamic_buffer buf{target};
      auto res = encode_argument(buf, major_type::unsigned_int, value);
      REQUIRE(!res);

      REQUIRE(target.size() == expected.size());

      auto m = std::mismatch(std::begin(target), std::end(target), std::begin(expected));
      REQUIRE(m.first == std::end(target));
      REQUIRE(m.second == std::end(expected));
   };

   check_encoding(0U, {0x00});
   check_encoding(1U, {0x01});
   check_encoding(10U, {0x0A});
   check_encoding(23U, {0x17});
   check_encoding(24U, {0x18, 0x18});
   check_encoding(25U, {0x18, 0x19});
   check_encoding(100U, {0x18, 0x64});
   check_encoding(1000U, {0x19, 0x03, 0xE8});
   check_encoding(1000000000000U, {0x1B, 0x00, 0x00, 0x00, 0xE8, 0xD4, 0xA5, 0x10, 0x00});
   check_encoding(18446744073709551615U, {0x1B, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF});
}