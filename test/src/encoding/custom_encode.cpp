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

using namespace test;

TEST_CASE("User-provided encode", "[encoding]") {
   check_encoding(custom_encode{.a = 1, .b = 0.0, .c = "a"},
                  {
                     0x01,             // a = 1
                     0xF9, 0x00, 0x00, // b = 0.0
                     0x61, 0x61        // c = "a"
                  });
}
