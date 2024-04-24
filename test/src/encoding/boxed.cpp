/**
 * @file   boxed.cpp
 * @author Dennis Sitelew 
 * @date   Apr 11, 2024
 */

#include <catch2/catch_test_macros.hpp>

#include <cbor/encoding.h>

#include <test/encoding.h>

#include <iostream>

namespace {
struct foo {
   char m;
};

} // namespace

namespace cbor {
template <>
struct type_id<foo> : std::integral_constant<std::uint64_t, 0xA0AA> {};

[[nodiscard]] std::error_code encode(buffer &buf, const foo &v) {
   return encode(buf, v.m);
}

} // namespace cbor

static_assert(cbor::type_id_v<foo> == 0xA0AA);
static_assert(!cbor::is_boxed_v<foo>);
static_assert(cbor::is_boxed_v<cbor::boxed<foo>>);
static_assert(cbor::boxed<foo>::type_id == cbor::type_id_v<foo>);

using boxed_foo = cbor::boxed<foo>;

using namespace test;

TEST_CASE("Boxed", "[encoding]") {
   const auto raw = foo{.m = 'r'};
   const auto boxed = boxed_foo{.v = {.m = 'b'}};

   check_encoding(raw,
                  {
                     0x18, 0x72 // m = "r"
                  });

   check_encoding(boxed,
                  {
                     0x82,             // Array of two elements: [type_id, value]
                     0x19, 0xA0, 0xAA, // Type ID
                     0x18, 0x62        // m = "b"
                  });
}