/**
 * @file   reflection.cpp
 * @author Dennis Sitelew 
 * @date   Apr 03, 2024
 *
 * Ensure that
 * - it is possible to provide custom reflection functions when Boost PFR is disabled.
 * - Boost PFR works for bare structs.
 */

#include <catch2/catch_test_macros.hpp>

#include <cbor/config.h>

#include <test/encoding.h>

using namespace test;

namespace {
#if CBOR_WITH(BOOST_PFR)
struct pfr_via_type_id {
   int a, b;
};

struct pfr_via_consteval {
   int a, b;
};

[[maybe_unused]] consteval void enable_cbor_encoding(pfr_via_consteval);
#endif // CBOR_WITH(BOOST_PFR)

struct custom_reflection {
   int a, b;
};

[[maybe_unused]] consteval void enable_cbor_encoding(custom_reflection);
} // namespace

#if CBOR_WITH(BOOST_PFR)
template <>
struct cbor::type_id<pfr_via_type_id> : std::integral_constant<std::uint64_t, 0x0A> {};
#endif // CBOR_WITH(BOOST_PFR)

template <>
consteval std::size_t cbor::get_member_count<custom_reflection>() {
   return 2;
}

template <>
const auto &cbor::get_member<0>(const custom_reflection &v) {
   return v.a;
}

template <>
const auto &cbor::get_member<1>(const custom_reflection &v) {
   return v.b;
}

#if CBOR_WITH(BOOST_PFR)
TEST_CASE("Reflection PFR", "[encoding]") {
   // Defining a type_id overload should be enough to whitelist a struct with PFR
   check_encoding(pfr_via_type_id{10, 20}, {0x0A, 0x14});

   // Defining a consteval function should be enough to whitelist a struct with PFR
   check_encoding(pfr_via_consteval{5, 7}, {0x05, 0x07});
}
#endif // CBOR_WITH(BOOST_PFR)

TEST_CASE("Reflection custom", "[encoding]") {
   check_encoding(custom_reflection{10, 20}, {0x0A, 0x14});
}
