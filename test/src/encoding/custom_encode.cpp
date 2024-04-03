/**
 * @file   custom_encode.cpp
 * @author Dennis Sitelew 
 * @date   Apr 02, 2024
 *
 * Ensure that it is possible to provide a custom "encode" function for a struct.
 */

#include <catch2/catch_test_macros.hpp>

#include <test/encoding.h>

#include <cbor/encoding.h>

#include <iostream>

struct custom_encode {
   std::int8_t a;
   double b;
   std::string c;
};

namespace cbor {

template <>
struct type_id<custom_encode> : std::integral_constant<std::uint64_t, 0xBEEF> {};

[[nodiscard]] std::error_code encode(buffer &buf, const custom_encode &v) {
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

} // namespace cbor

static_assert(cbor::type_id_v<custom_encode> == 0xBEEF);

using namespace test;

TEST_CASE("User-provided encode", "[encoding]") {
   check_encoding(custom_encode{.a = 1, .b = 0.0, .c = "a"},
                  {
                     0x19, 0xBE, 0xEF, // Type ID
                     0x01,             // a = 1
                     0xF9, 0x00, 0x00, // b = 0.0
                     0x61, 0x61        // c = "a"
                  });
}
