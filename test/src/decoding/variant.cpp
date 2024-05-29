/**
 * @file   variant.cpp
 * @author Dennis Sitelew 
 * @date   May 29, 2024
 */

#include <catch2/catch_test_macros.hpp>

#include <cbor/encoding.h>

#include <test/decoding.h>

#include <iostream>

using span_t = cbor::buffer::const_span_t;

struct variant_a {
   std::int8_t a{};
   double b{};
   std::string c{};
};

bool operator==(const variant_a &lhs, const variant_a &rhs) {
   return std::make_tuple(lhs.a, lhs.b, lhs.c) == std::make_tuple(rhs.a, rhs.b, rhs.c);
}

struct variant_b {
   std::optional<int> a{};
   bool b{};
};

bool operator==(const variant_b &lhs, const variant_b &rhs) {
   return std::make_tuple(lhs.a, lhs.b) == std::make_tuple(rhs.a, rhs.b);
}

namespace cbor {

template <>
struct type_id<variant_a> : std::integral_constant<std::uint64_t, 0xBEEF> {};

[[nodiscard]] std::error_code decode(read_buffer &buf, variant_a &v) {
   auto rollback_helper = buf.get_rollback_helper();

   auto res = decode(buf, v.a);
   if (res) {
      return res;
   }

   res = decode(buf, v.b);
   if (res) {
      return res;
   }

   res = decode(buf, v.c);
   if (res) {
      return res;
   }

   rollback_helper.commit();

   return res;
}

template <>
struct type_id<variant_b> : std::integral_constant<std::uint64_t, 0xDEAF> {};

[[nodiscard]] std::error_code decode(read_buffer &buf, variant_b &v) {
   auto rollback_helper = buf.get_rollback_helper();

   auto res = decode(buf, v.a);
   if (res) {
      return res;
   }

   res = decode(buf, v.b);
   if (res) {
      return res;
   }

   rollback_helper.commit();

   return res;
}

} // namespace cbor

static_assert(cbor::type_id_v<variant_a> == 0xBEEF);
static_assert(cbor::type_id_v<variant_b> == 0xDEAF);

template <typename... T>
   requires cbor::AllWithTypeID<T...>
constexpr inline bool all_with_id() {
   return true;
}

template <typename... T>
constexpr inline bool all_with_id() {
   return false;
}

static_assert(all_with_id<variant_a, variant_b>());
static_assert(!all_with_id<variant_a, int>());

using namespace test;

using value_t = std::variant<variant_a, variant_b>;

TEST_CASE("Variant - basic decoding", "[decoding, variant]") {
   value_t first = variant_a{.a = 1, .b = 0.0, .c = "a"};
   const value_t second = variant_b{.a = std::nullopt, .b = true};

   expect(
      {
         0x19, 0xBE, 0xEF, // Type ID
         0x01,             // a = 1
         0xF9, 0x00, 0x00, // b = 0.0
         0x61, 0x61        // c = "a"
      },
      first);

   expect(
      {
         0x19, 0xDE, 0xAF, // Type ID
         0xF6,             // a = nullopt
         0xF5,             // b = true
      },
      second);
}

TEST_CASE("Variant decoding - error cases", "[decoding, variant, errors]") {
   SECTION("Not enough data to read type_id head") {
      std::array<std::byte, 2> source{};
      cbor::read_buffer buf{span_t{source.data(), 0}};

      value_t v;
      REQUIRE(cbor::decode(buf, v) == cbor::error::buffer_underflow);
   }

   SECTION("Invalid type_id type") {
      std::array source{0x40_b};
      cbor::read_buffer buf{span_t{source}};

      value_t v;
      REQUIRE(cbor::decode(buf, v) == cbor::error::unexpected_type);
   }

   SECTION("Unexpected alternative type_id") {
      std::array source{0x19_b, 0xBE_b, 0xED_b, 0xF9_b, 0x00_b, 0x00_b};
      cbor::read_buffer buf{span_t{source}};

      value_t v;
      REQUIRE(cbor::decode(buf, v) == cbor::error::unexpected_type);
   }

   SECTION("Alternative decoding error should be propagated") {
      std::array source{0x19_b, 0xBE_b, 0xEF_b, 0x01_b, 0xF9_b, 0x00_b, 0x00_b};
      cbor::read_buffer buf{span_t{source}};

      value_t v;
      REQUIRE(cbor::decode(buf, v) == cbor::error::buffer_underflow);
   }
}

