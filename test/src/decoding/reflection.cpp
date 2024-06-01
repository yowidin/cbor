/**
 * @file   reflection.cpp
 * @author Dennis Sitelew 
 * @date   May 30, 2024
 *
 * Ensure that
 * - it is possible to provide custom reflection functions when Boost PFR is disabled.
 * - Boost PFR works for bare structs.
 */

#include <catch2/catch_test_macros.hpp>

#include <cbor/config.h>

#include <test/decoding.h>

using namespace test;

namespace {
#if CBOR_WITH(BOOST_PFR)
struct pfr_via_type_id {
   int a, b;
};

bool operator==(const pfr_via_type_id &lhs, const pfr_via_type_id &rhs) {
   return std::make_tuple(lhs.a, lhs.b) == std::make_tuple(rhs.a, rhs.b);
}

struct pfr_via_consteval {
   int a, b;

   std::array<std::byte, 2> byte_array;
   std::vector<std::byte> byte_vec;
};

bool operator==(const pfr_via_consteval &lhs, const pfr_via_consteval &rhs) {
   return std::make_tuple(lhs.a, lhs.b, lhs.byte_array, lhs.byte_vec)
      == std::make_tuple(rhs.a, rhs.b, rhs.byte_array, rhs.byte_vec);
}

[[maybe_unused]] consteval void enable_cbor_encoding(pfr_via_consteval);
#endif // CBOR_WITH(BOOST_PFR)

struct custom_reflection {
   int a, b;

   std::array<std::byte, 2> byte_array;
   std::vector<std::byte> byte_vec;
};

bool operator==(const custom_reflection &lhs, const custom_reflection &rhs) {
   return std::make_tuple(lhs.a, lhs.b, lhs.byte_array, lhs.byte_vec)
      == std::make_tuple(rhs.a, rhs.b, rhs.byte_array, rhs.byte_vec);
}

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
auto &cbor::get_member_non_const<0>(custom_reflection &v) {
   return v.a;
}

template <>
auto &cbor::get_member_non_const<1>(custom_reflection &v) {
   return v.b;
}

template <>
auto &cbor::get_member_non_const<2>(custom_reflection &v) {
   return v.byte_array;
}

template <>
auto &cbor::get_member_non_const<3>(custom_reflection &v) {
   return v.byte_vec;
}

#if CBOR_WITH(BOOST_PFR)
TEST_CASE("Struct - decoding with PFR reflection", "[decoding, struct]") {
   // Defining a type_id overload should be enough to whitelist a struct with PFR
   expect({0x82, 0x0A, 0x14}, pfr_via_type_id{10, 20});

   // Defining a consteval function should be enough to whitelist a struct with PFR
   expect({0x84, 0x05, 0x07, 0x42, 0x01, 0x02, 0x42, 0x03, 0x04}, pfr_via_consteval{5, 7, {1_b, 2_b}, {3_b, 4_b}});
}
#endif // CBOR_WITH(BOOST_PFR)

TEST_CASE("Struct - decoding with custom reflection", "[decoding, struct]") {
   expect({0x84, 0x0A, 0x14, 0x42, 0x01, 0x02, 0x42, 0x03, 0x04}, custom_reflection{10, 20, {1_b, 2_b}, {3_b, 4_b}});
}

TEST_CASE("Struct - decoding with custom reflection and rapped in optional", "[decoding, struct]") {
   using optional_t = std::optional<custom_reflection>;
   expect({0x84, 0x0A, 0x14, 0x42, 0x01, 0x02, 0x42, 0x03, 0x04}, optional_t{{10, 20, {1_b, 2_b}, {3_b, 4_b}}});
   expect({0xF6}, optional_t{});
}

TEST_CASE("Struct - decoding error cases", "[decoding, struct, errors]") {
   SECTION("Not enough data to read the array") {
      std::array<std::byte, 2> source{};
      cbor::read_buffer buf{span_t{source.data(), 0}};

      custom_reflection v{};
      REQUIRE(cbor::decode(buf, v) == cbor::error::buffer_underflow);
   }

   SECTION("Invalid array type") {
      std::array<std::byte, 2> source{0x02_b};
      cbor::read_buffer buf{span_t{source.data(), 1}};

      custom_reflection v{};
      REQUIRE(cbor::decode(buf, v) == cbor::error::unexpected_type);
   }

   SECTION("Invalid number of fields") {
      std::array<std::byte, 2> source{0x82_b};
      cbor::read_buffer buf{span_t{source.data(), 1}};

      custom_reflection v{};
      REQUIRE(cbor::decode(buf, v) == cbor::error::decoding_error);
   }

   SECTION("Custom - not enough data to read the first member's head") {
      std::array source{0x84_b, 0x0A_b, 0x14_b, 0x42_b, 0x01_b, 0x02_b, 0x42_b, 0x03_b, 0x04_b};
      cbor::read_buffer buf{span_t{source.data(), 1}};

      custom_reflection v{};
      REQUIRE(cbor::decode(buf, v) == cbor::error::buffer_underflow);
   }
}
