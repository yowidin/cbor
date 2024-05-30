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

   std::array<int, 2> int_array;
   std::vector<int> int_vec;
};

[[maybe_unused]] consteval void enable_cbor_encoding(pfr_via_consteval);
#endif // CBOR_WITH(BOOST_PFR)

struct custom_reflection {
   int a, b;

   std::array<int, 2> int_array;
   std::vector<int> int_vec;
};

[[maybe_unused]] consteval void enable_cbor_encoding(custom_reflection);
} // namespace

#if CBOR_WITH(BOOST_PFR)
template <>
struct cbor::type_id<pfr_via_type_id> : std::integral_constant<std::uint64_t, 0x0A> {};
#endif // CBOR_WITH(BOOST_PFR)

template <>
consteval std::size_t cbor::get_member_count<custom_reflection>() {
   return 4;
}

template <>
const auto &cbor::get_member<0>(const custom_reflection &v) {
   return v.a;
}

template <>
const auto &cbor::get_member<1>(const custom_reflection &v) {
   return v.b;
}

template <>
const auto &cbor::get_member<2>(const custom_reflection &v) {
   return v.int_array;
}

template <>
const auto &cbor::get_member<3>(const custom_reflection &v) {
   return v.int_vec;
}

#if CBOR_WITH(BOOST_PFR)
TEST_CASE("Struct - encoding with PFR reflection", "[encoding, struct]") {
   // Defining a type_id overload should be enough to whitelist a struct with PFR
   check_encoding(pfr_via_type_id{10, 20}, {0x0A, 0x14});

   // Defining a consteval function should be enough to whitelist a struct with PFR
   check_encoding(pfr_via_consteval{5, 7, {1, 2}, {3, 4}}, {0x05, 0x07, 0x82, 0x01, 0x02, 0x82, 0x03, 0x04});
}
#endif // CBOR_WITH(BOOST_PFR)

TEST_CASE("Struct - encoding with custom reflection", "[encoding, struct]") {
   check_encoding(custom_reflection{10, 20, {1, 2}, {3, 4}}, {0x0A, 0x14, 0x82, 0x01, 0x02, 0x82, 0x03, 0x04});
}

TEST_CASE("Struct - encoding with custom reflection and wrapped in optional", "[encoding, struct, optional]") {
   check_encoding(std::optional<custom_reflection>{{10, 20, {1, 2}, {3, 4}}},
                  {0x0A, 0x14, 0x82, 0x01, 0x02, 0x82, 0x03, 0x04});

   check_encoding(std::optional<custom_reflection>{}, {0xF6});
}

TEST_CASE("Struct - encoding rollback on failure", "[encoding, struct, rollback]") {
   // {0x0A, 0x14, 0x82, 0x01, 0x02, 0x82, 0x03, 0x04}
   const custom_reflection v{10, 20, {1, 2}, {3, 4}};

   SECTION("No space for the first member") {
      std::vector<std::byte> target{};
      cbor::dynamic_buffer buf{target, 0};

      std::error_code ec;
      ec = cbor::encode(buf, v);
      REQUIRE(ec == cbor::error::buffer_overflow);
      REQUIRE(target.empty());
   }

   SECTION("No space for the last member") {
      std::vector<std::byte> target{};
      cbor::dynamic_buffer buf{target, 7};

      std::error_code ec;
      ec = cbor::encode(buf, v);
      REQUIRE(ec == cbor::error::buffer_overflow);
      REQUIRE(target.empty());
   }
}
