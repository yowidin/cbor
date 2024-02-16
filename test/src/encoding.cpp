/**
 * @file   encoding.cpp
 * @author Dennis Sitelew 
 * @date   Feb 13, 2024
 */

#include <catch2/catch_test_macros.hpp>

#include <cbor/encoding.h>

#include <shp/shp.h>

#include <iostream>

using namespace cbor;

template <typename Integer>
void compare_arrays(Integer v, const std::vector<std::uint8_t> &res, const std::vector<std::uint8_t> &expected) {
   REQUIRE(res.size() == expected.size());

   auto m = std::mismatch(std::begin(res), std::end(res), std::begin(expected));
   if (m.first != std::end(res) || m.second != std::end(expected)) {
      std::cerr << "Array missmatch for " << shp::hex(v) << ":\n"
                << "Expected:\n"
                << shp::hex(expected) << "\n\nFound:\n"
                << shp::hex(res) << std::endl;
   }
   REQUIRE(m.first == std::end(res));
   REQUIRE(m.second == std::end(expected));
}

TEST_CASE("Check argument encoding", "[encoding]") {
   // Ensure that an unsigned int is encoded as the expected byte array
   auto check_encoding = [](auto value, std::initializer_list<std::uint8_t> expected) {
      std::vector<std::uint8_t> target{};
      dynamic_buffer buf{target};
      auto res = encode_argument(buf, major_type::unsigned_int, value);
      REQUIRE(!res);
      compare_arrays(value, target, expected);
   };

   check_encoding(0U, {0x00});
   check_encoding(1U, {0x01});
   check_encoding(10U, {0x0A});
   check_encoding(23U, {0x17});
   check_encoding(24U, {0x18, 0x18});
   check_encoding(25U, {0x18, 0x19});
   check_encoding(100U, {0x18, 0x64});
   check_encoding(1000U, {0x19, 0x03, 0xE8});
   check_encoding(static_cast<std::uint64_t>(1000000000000), {0x1B, 0x00, 0x00, 0x00, 0xE8, 0xD4, 0xA5, 0x10, 0x00});
   check_encoding(static_cast<std::uint64_t>(18446744073709551615U),
                  {0x1B, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF});
}

TEST_CASE("Check unsigned encoding", "[encoding]") {
   // Ensure that an unsigned int is encoded as the expected byte array
   auto check_encoding = [](auto value, std::initializer_list<std::uint8_t> expected) {
      std::vector<std::uint8_t> target{};
      dynamic_buffer buf{target};
      auto res = encode(buf, value);
      REQUIRE(!res);
      compare_arrays(value, target, expected);
   };

   check_encoding(0U, {0x00});
   check_encoding(1U, {0x01});
   check_encoding(10U, {0x0A});
   check_encoding(23U, {0x17});
   check_encoding(24U, {0x18, 0x18});
   check_encoding(25U, {0x18, 0x19});
   check_encoding(100U, {0x18, 0x64});
   check_encoding(static_cast<unsigned long>(100), {0x18, 0x64});
   check_encoding(1000U, {0x19, 0x03, 0xE8});
   check_encoding(static_cast<std::uint64_t>(1000000000000), {0x1B, 0x00, 0x00, 0x00, 0xE8, 0xD4, 0xA5, 0x10, 0x00});
   check_encoding(std::numeric_limits<std::uint64_t>::max(), {0x1B, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF});
}

TEST_CASE("Check signed encoding", "[encoding]") {
   // Ensure that a signed int is encoded as the expected byte array
   auto check_encoding = [](auto value, std::initializer_list<std::uint8_t> expected) {
      std::vector<std::uint8_t> target{};
      dynamic_buffer buf{target};
      auto res = encode(buf, value);
      REQUIRE(!res);
      compare_arrays(value, target, expected);
   };

   check_encoding(-1, {0x20});
   check_encoding(-10, {0x29});
   check_encoding(-100, {0x38, 0x63});
   check_encoding(static_cast<long>(-100), {0x38, 0x63});
   check_encoding(-1000, {0x39, 0x03, 0xE7});
   check_encoding(std::numeric_limits<std::int64_t>::min(), {0x3B, 0x7F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF});
}