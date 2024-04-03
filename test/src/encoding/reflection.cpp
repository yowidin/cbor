/**
 * @file   reflection.cpp
 * @author Dennis Sitelew 
 * @date   Apr 03, 2024
 *
 * Ensure that
 * - it is possible to provide custom reflection functions when Boost PFR is disabled.
 * - it is enough to provide a type ID with Boost PFR for encoding to work.
 */

#include <catch2/catch_test_macros.hpp>

#include <cbor/config.h>

#include <test/encoding.h>

using namespace test;

struct reflection_pfr {
   int a, b;
};

struct reflection_custom {
   int a, b;
};

namespace cbor {

template <>
struct type_id<reflection_pfr> : std::integral_constant<std::uint64_t, 0x0A> {};

template <>
struct type_id<reflection_custom> : std::integral_constant<std::uint64_t, 0x0B> {};

template <>
consteval std::size_t get_member_count<reflection_custom>() {
   return 2;
}

template <>
const auto &get_member<0>(const reflection_custom &v) {
   return v.a;
}

template <>
const auto &get_member<1>(const reflection_custom &v) {
   return v.b;
}

} // namespace cbor

#if CBOR_WITH(BOOST_PFR)
TEST_CASE("Reflection PFR", "[encoding]") {
   check_encoding(reflection_pfr{10, 20}, {0x0A, 0x0A, 0x14});
}
#endif // CBOR_WITH(BOOST_PFR)

TEST_CASE("Reflection custom", "[encoding]") {
   check_encoding(reflection_custom{10, 20}, {0x0B, 0x0A, 0x14});
}
