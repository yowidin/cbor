/**
 * @file   misc.cpp
 * @author Dennis Sitelew 
 * @date   Apr 03, 2024
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <test/encoding.h>

#include <functional>
#include <iostream>

using namespace test;

namespace arguments {
template <typename T>
auto private_codec(cbor::buffer &b, cbor::major_type t, T v) {
   return cbor::detail::encode_argument(b, t, v, true);
}

template <typename T>
auto public_codec(cbor::buffer &b, cbor::major_type t, T v) {
   return cbor::encode_argument(b, t, v);
}

template <typename IntType>
auto test_smallest_argument() {
   auto check = [](const auto &codec, auto value, std::initializer_list<std::uint8_t> expected) {
      std::vector<std::byte> target{};
      cbor::dynamic_buffer buf{target};

      std::error_code res;
      res = codec(buf, cbor::major_type::unsigned_int, value);

      REQUIRE(!res);

      compare_arrays(value, target, expected);
   };

   using int_t = IntType;
   auto codec = GENERATE(private_codec<int_t>, public_codec<int_t>);
   check(codec, static_cast<int_t>(1U), {0x01});
   check(codec, static_cast<int_t>(0xFFU), {0x18, 0xFF});

   if constexpr (sizeof(int_t) > 2) {
      check(codec, static_cast<int_t>(0xFFFFU), {0x19, 0xFF, 0xFF});
   }

   if constexpr (sizeof(int_t) > 4) {
      check(codec, static_cast<int_t>(0xFFFFFFFFU), {0x1A, 0xFF, 0xFF, 0xFF, 0xFF});
   }
}

} // namespace arguments

TEST_CASE("Arguments - smallest int is used", "[encoding]") {
   using namespace arguments;

   SECTION("16 Bits") {
      test_smallest_argument<std::uint16_t>();
   }

   SECTION("32 Bits") {
      test_smallest_argument<std::uint32_t>();
   }

   SECTION("64 Bits") {
      test_smallest_argument<std::uint64_t>();
   }
}

TEST_CASE("Arguments - basic encoding", "[encoding]") {
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

TEST_CASE("Byte Array - happy path", "[encoding, byte_array]") {
   // Ensure that a signed int is encoded as the expected byte array
   auto check_encoding = [](std::initializer_list<std::uint8_t> value, std::initializer_list<std::uint8_t> expected) {
      ::check_encoding(std::as_bytes(std::span{value}), expected);
   };

   check_encoding({}, {0x40});
   check_encoding({0x01, 0x02, 0x03, 0x04}, {0x44, 0x01, 0x02, 0x03, 0x04});
}

TEST_CASE("Byte Array - error conditions", "[encoding, byte_array]") {
   SECTION("Not enough space for the argument type") {
      std::vector<std::byte> target;
      cbor::dynamic_buffer buf{target, 0};

      auto payload = {0x01, 0x02};
      std::error_code ec = cbor::encode(buf, std::as_bytes(std::span{payload}));
      REQUIRE(ec == cbor::error::buffer_overflow);
   }

   SECTION("Not enough space for payload") {
      std::array<std::byte, 2> target{};
      cbor::static_buffer buf{target};

      auto payload = {0x01, 0x02};
      std::error_code ec = cbor::encode(buf, std::as_bytes(std::span{payload}));
      REQUIRE(ec == cbor::error::buffer_overflow);
   }
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

TEST_CASE("String - error conditions", "[encoding, string]") {
   SECTION("Not enough space for payload") {
      std::vector<std::byte> target;
      cbor::dynamic_buffer buf{target, 0};

      // Small target - nothing can be encoded
      std::error_code ec = cbor::encode(buf, "a");
      REQUIRE(ec == cbor::error::buffer_overflow);
      REQUIRE(buf.size() == 0);
   }

   SECTION("Not enough space for payload") {
      std::array<std::byte, 2> target{};
      cbor::static_buffer buf{target};

      // Major type can be encoded, but the "b" character not
      std::error_code ec = cbor::encode(buf, "ab");
      REQUIRE(ec == cbor::error::buffer_overflow);
      REQUIRE(buf.size() == 0);
   }
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

TEST_CASE("Dictionary", "[encoding, dictionary]") {
   using namespace std;

   // {1: "1", 2: "22"}
   check_encoding(
      map<int, string>{
         {1, "1"},
         {2, "22"},
      },
      {0xA2, 0x01, 0x61, 0x31, 0x02, 0x62, 0x32, 0x32});

   // {}
   check_encoding(map<int, int>{}, {0xA0});

   // {1: 2, 3: 4}
   check_encoding(map<int, int>{{1, 2}, {3, 4}}, {0xA2, 0x01, 0x02, 0x03, 0x04});

   // {"a": "A", "b": "B", "c": "C", "d": "D", "e": "E"}
   check_encoding(map<string, string>{{"a", "A"}, {"b", "B"}, {"c", "C"}, {"d", "D"}, {"e", "E"}},
                  {0xA5, 0x61, 0x61, 0x61, 0x41, 0x61, 0x62, 0x61, 0x42, 0x61, 0x63,
                   0x61, 0x43, 0x61, 0x64, 0x61, 0x44, 0x61, 0x65, 0x61, 0x45});
}

TEST_CASE("Dictionary - rollback on failure", "[encoding, dictionary, rollback]") {
   using namespace std;

   // {0xA2, 0x01, 0x02, 0x03, 0x04}
   const map<int, int> dict{{1, 2}, {3, 4}};

   SECTION("No space for major type") {
      std::vector<std::byte> target{};
      cbor::dynamic_buffer buf{target, 0};

      std::error_code ec;
      ec = cbor::encode(buf, dict);
      REQUIRE(ec == cbor::error::buffer_overflow);
      REQUIRE(target.empty());
   }

   SECTION("No space for key") {
      std::vector<std::byte> target{};
      cbor::dynamic_buffer buf{target, 3};

      std::error_code ec;
      ec = cbor::encode(buf, dict);
      REQUIRE(ec == cbor::error::buffer_overflow);
      REQUIRE(target.empty());
   }

   SECTION("No space for value") {
      std::vector<std::byte> target{};
      cbor::dynamic_buffer buf{target, 4};

      std::error_code ec;
      ec = cbor::encode(buf, dict);
      REQUIRE(ec == cbor::error::buffer_overflow);
      REQUIRE(target.empty());
   }
}
