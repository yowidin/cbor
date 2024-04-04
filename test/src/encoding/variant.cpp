/**
 * @file   variant.cpp
 * @author Dennis Sitelew 
 * @date   Apr 02, 2024
 *
 * Ensure that variants can be encoded when all the types in the type set are structs and have an encode function.
 */

#include <catch2/catch_test_macros.hpp>

#include <cbor/encoding.h>

#include <test/encoding.h>

#include <iostream>

struct variant_a {
   std::int8_t a;
   double b;
   std::string c;
};

struct variant_b {
   std::optional<int> a;
   bool b;
};

namespace cbor {

template <>
struct type_id<variant_a> : std::integral_constant<std::uint64_t, 0xBEEF> {};

[[nodiscard]] std::error_code encode(buffer &buf, const variant_a &v) {
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
struct type_id<variant_b> : std::integral_constant<std::uint64_t, 0xDEAF> {};

[[nodiscard]] std::error_code encode(buffer &buf, const variant_b &v) {
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

TEST_CASE("Variant", "[encoding]") {
   using value_t = std::variant<variant_a, variant_b>;

   value_t first = variant_a{.a = 1, .b = 0.0, .c = "a"};
   const value_t second = variant_b{.a = std::nullopt, .b = true};

   check_encoding(first,
                  {
                     0x19, 0xBE, 0xEF, // Type ID
                     0x01,             // a = 1
                     0xF9, 0x00, 0x00, // b = 0.0
                     0x61, 0x61        // c = "a"
                  });

   check_encoding(second,
                  {
                     0x19, 0xDE, 0xAF, // Type ID
                     0xF6,             // a = nullopt
                     0xF5,             // b = true
                  });
}
