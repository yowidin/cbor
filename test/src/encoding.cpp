/**
 * @file   encoding.cpp
 * @author Dennis Sitelew 
 * @date   Feb 13, 2024
 */

#include <catch2/catch_test_macros.hpp>

#include <cbor/encoding.h>

#include <shp/shp.h>

#include <iostream>

struct foo {
   std::int8_t a;
   double b;
   std::string c;
};

struct bar {
   std::optional<int> a;
   std::array<std::uint8_t, 4> b;
};

namespace cbor {

template <>
struct type_id<foo> : std::integral_constant<std::uint64_t, 0xBEEF> {};

[[nodiscard]] std::error_code encode(buffer &buf, const foo &v) {
   auto rollback_helper = buf.get_rollback_helper();

   auto res = encode(buf, v.a);
   if (res) {
      return res;
   }

   res = encode(buf, v.b);
   if (res) {
      return res;
   }

   res = encode(buf, v.c);
   if (res) {
      return res;
   }

   rollback_helper.commit();

   return res;
}

template <>
struct type_id<bar> : std::integral_constant<std::uint64_t, 0xDEAF> {};

[[nodiscard]] std::error_code encode(buffer &buf, const bar &v) {
   auto rollback_helper = buf.get_rollback_helper();

   auto res = encode(buf, v.a);
   if (res) {
      return res;
   }

   res = encode(buf, v.b);
   if (res) {
      return res;
   }

   rollback_helper.commit();

   return res;
}

} // namespace cbor

static_assert(cbor::type_id_v<foo> == 0xBEEF);
static_assert(cbor::type_id_v<bar> == 0xDEAF);

template <typename... T>
   requires cbor::AllWithTypeID<T...>
constexpr inline bool all_with_id() {
   return true;
}

template <typename... T>
constexpr inline bool all_with_id() {
   return false;
}

static_assert(all_with_id<foo, bar>());
static_assert(!all_with_id<foo, int>());

using namespace cbor;

template <typename T>
void compare_arrays(T v, const std::vector<std::uint8_t> &res, const std::vector<std::uint8_t> &expected) {
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

template <typename T>
void check_encoding(const T &value, std::initializer_list<std::uint8_t> expected) {
   std::vector<std::uint8_t> target{};
   dynamic_buffer buf{target};

   std::error_code res;
   res = encode(buf, value);

   REQUIRE(!res);

   compare_arrays(value, target, expected);
}

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

TEST_CASE("Floating Point", "[encoding]") {
   check_encoding(0.0f, {0xF9, 0x00, 0x00});
   check_encoding(0.0, {0xF9, 0x00, 0x00});

   check_encoding(-0.0f, {0xF9, 0x80, 0x00});
   check_encoding(-0.0, {0xF9, 0x80, 0x00});

   check_encoding(1.0f, {0xF9, 0x3C, 0x00});
   check_encoding(1.0, {0xF9, 0x3C, 0x00});

   check_encoding(1.1f, {0xFA, 0x3F, 0x8C, 0xCC, 0xCD});
   check_encoding(1.1, {0xFB, 0x3F, 0xF1, 0x99, 0x99, 0x99, 0x99, 0x99, 0x9A});

   check_encoding(1.5f, {0xF9, 0x3E, 0x00});
   check_encoding(1.5, {0xF9, 0x3E, 0x00});

   check_encoding(65504.0f, {0xF9, 0x7B, 0xFF});
   check_encoding(65504.0, {0xF9, 0x7B, 0xFF});

   check_encoding(100000.0f, {0xFA, 0x47, 0xC3, 0x50, 0x00});
   check_encoding(100000.0, {0xFA, 0x47, 0xC3, 0x50, 0x00});

   check_encoding(3.4028234663852886e+38f, {0xFA, 0x7F, 0x7F, 0xFF, 0xFF});
   check_encoding(3.4028234663852886e+38, {0xFA, 0x7F, 0x7F, 0xFF, 0xFF});

   // check_encoding(1.0e+300f, {0xFB, 0x7E, 0x37, 0xE4, 0x3C, 0x88, 0x00, 0x75, 0x9C}); // not representable with float
   check_encoding(1.0e+300, {0xFB, 0x7E, 0x37, 0xE4, 0x3C, 0x88, 0x00, 0x75, 0x9C});

   check_encoding(5.960464477539063e-8f, {0xF9, 0x00, 0x01});
   check_encoding(5.960464477539063e-8, {0xF9, 0x00, 0x01});

   check_encoding(0.00006103515625f, {0xF9, 0x04, 0x00});
   check_encoding(0.00006103515625, {0xF9, 0x04, 0x00});

   check_encoding(-4.0f, {0xF9, 0xC4, 0x00});
   check_encoding(-4.0, {0xF9, 0xC4, 0x00});

   check_encoding(-4.1f, {0xFA, 0xC0, 0x83, 0x33, 0x33});
   check_encoding(-4.1, {0xFB, 0xC0, 0x10, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66});

   check_encoding(std::numeric_limits<float>::infinity(), {0xF9, 0x7C, 0x00});
   check_encoding(std::numeric_limits<float>::quiet_NaN(), {0xF9, 0x7E, 0x00});
   check_encoding(-std::numeric_limits<float>::infinity(), {0xF9, 0xFC, 0x00});

   check_encoding(std::numeric_limits<double>::infinity(), {0xF9, 0x7C, 0x00});
   check_encoding(std::numeric_limits<double>::quiet_NaN(), {0xF9, 0x7E, 0x00});
   check_encoding(-std::numeric_limits<double>::infinity(), {0xF9, 0xFC, 0x00});
}

TEST_CASE("Variant", "[encoding]") {
   using value_t = std::variant<foo, bar>;

   value_t first = foo{.a = 1, .b = 0.0, .c = "a"};
   const value_t second = bar{.a = std::nullopt, .b = {1, 2, 3, 4}};

   check_encoding(first,
                  {
                     0x19, 0xBE, 0xEF, // Type ID
                     0x01,             // a = 1
                     0xF9, 0x00, 0x00, // b = 0.0
                     0x61, 0x61        // c = "a"
                  });

   check_encoding(second,
                  {
                     0x19, 0xDE, 0xAF,             // Type ID
                     0xF6,                         // a = nullopt
                     0x44, 0x01, 0x02, 0x03, 0x04, // b = {1, 2, 3, 4}
                  });
}
