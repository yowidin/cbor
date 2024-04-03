/**
 * @file   misc.cpp
 * @author Dennis Sitelew 
 * @date   Apr 03, 2024
 */

#include <catch2/catch_test_macros.hpp>

#include <test/encoding.h>

#include <iostream>

using namespace test;

TEST_CASE("Argument encoding", "[encoding]") {
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

TEST_CASE("Unsigned", "[encoding]") {
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

TEST_CASE("Signed", "[encoding]") {
   check_encoding(-1, {0x20});
   check_encoding(-10, {0x29});
   check_encoding(-100, {0x38, 0x63});
   check_encoding(static_cast<long>(-100), {0x38, 0x63});
   check_encoding(-1000, {0x39, 0x03, 0xE7});
   check_encoding(std::numeric_limits<std::int64_t>::min(), {0x3B, 0x7F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF});
}

TEST_CASE("Enum", "[encoding]") {
   enum class long_class : long {
      a = -10,
      b = 23,
   };

   enum long_simple : long {
      ls_a = -10,
      ls_b = 23,
   };

   enum class untyped {
      a = -10,
      b = 23,
   };

   check_encoding(long_class::a, {0x29});
   check_encoding(long_class::b, {0x17});

   check_encoding(untyped::a, {0x29});
   check_encoding(untyped::b, {0x17});

   check_encoding(ls_a, {0x29});
   check_encoding(ls_b, {0x17});
}

TEST_CASE("Array", "[encoding]") {
   // Ensure that a signed int is encoded as the expected byte array
   auto check_encoding = [](std::initializer_list<std::uint8_t> value, std::initializer_list<std::uint8_t> expected) {
     ::check_encoding(value, expected);
   };

   check_encoding({}, {0x40});
   check_encoding({0x01, 0x02, 0x03, 0x04}, {0x44, 0x01, 0x02, 0x03, 0x04});
}

TEST_CASE("String", "[encoding]") {
   check_encoding("", {0x60});
   check_encoding("a", {0x61, 0x61});
   check_encoding("IETF", {0x64, 0x49, 0x45, 0x54, 0x46});
   check_encoding("\"\\", {0x62, 0x22, 0x5C});
   check_encoding("\u00fc", {0x62, 0xC3, 0xBC});
   check_encoding("\u6c34", {0x63, 0xE6, 0xB0, 0xB4});

   // Standard strings should also drop the \0
   check_encoding(std::string(""), {0x60});
   check_encoding(std::string("a"), {0x61, 0x61});

   const char *decayed_char_array_1 = "";
   check_encoding(decayed_char_array_1, {0x60});

   const char *decayed_char_array_2 = "a";
   check_encoding(decayed_char_array_2, {0x61, 0x61});
}

TEST_CASE("Simple type", "[encoding]") {
   check_encoding(false, {0xF4});
   check_encoding(true, {0xF5});
   check_encoding(nullptr, {0xF6});
}

TEST_CASE("Optional", "[encoding]") {
   check_encoding(std::optional<int>{}, {0xF6});
   check_encoding(std::optional<int>{25}, {0x18, 0x19});

   check_encoding(std::optional<std::string>{}, {0xF6});
   check_encoding(std::optional<std::string>{"IETF"}, {0x64, 0x49, 0x45, 0x54, 0x46});
}
