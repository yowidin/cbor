/**
 * @file   encoding.h
 * @author Dennis Sitelew 
 * @date   Apr 03, 2024
 */

#pragma once

#include <cbor/encoding.h>

#include <shp/shp.h>
#include <catch2/catch_test_macros.hpp>

#include <iostream>
#include <map>

namespace test {

////////////////////////////////////////////////////////////////////////////////
/// Hex printers
////////////////////////////////////////////////////////////////////////////////
template <typename T>
auto hex(const T &v) {
   return shp::hex(v, shp::NoOffsets{}, shp::NoNibbleSeparation{}, shp::SingleRow{}, shp::NoASCII{}, shp::UpperCase{});
}

template <std::integral T>
auto hex(const T &v) {
   return shp::hex(v);
}

////////////////////////////////////////////////////////////////////////////////
/// Value printers: either as is (if printable), or as hex
////////////////////////////////////////////////////////////////////////////////
template <typename K, typename V>
std::ostream &operator<<(std::ostream &os, const std::pair<K, V> &kv) {
   return os << "[" << kv.first << ", " << kv.second << "]";
}

template <typename Key,
          typename T,
          class Compare = std::less<Key>,
          class Allocator = std::allocator<std::pair<const Key, T>>>
std::ostream &operator<<(std::ostream &os, const std::map<Key, T, Compare, Allocator> &m) {
   for (const auto &kv : m) {
      os << kv;
   }
   return os;
}

template <typename T>
concept Printable = requires(const T &t) { std::declval<std::ostream &>() << t; };

template <typename T>
struct ValuePrinter {
   explicit ValuePrinter(const T &v)
      : value{&v} {}

   const T *value;
};

template <typename T>
std::ostream &operator<<(std::ostream &os, const ValuePrinter<T> &v) {
   return os << "'" << *v.value << "'";
}

template <typename T>
struct HexPrinter {
   explicit HexPrinter(const T &v)
      : value{&v} {}

   const T *value;
};

template <typename T>
std::ostream &operator<<(std::ostream &os, const HexPrinter<T> &v) {
   return os << "'" << hex(*v.value) << "'";
}

template <typename T>
auto print(const T &v) {
   return HexPrinter(v);
}

template <Printable T>
auto print(const T &v) {
   return ValuePrinter(v);
}

////////////////////////////////////////////////////////////////////////////////
/// Helper functions
////////////////////////////////////////////////////////////////////////////////
template <typename T>
void compare_arrays(T v, const std::vector<std::byte> &res, const std::vector<std::uint8_t> &expected) {
   INFO("Comparing '" << hex(res) << "' with '" << hex(expected) << "' for " << print(v));

   REQUIRE(res.size() == expected.size());

   const auto v_begin = reinterpret_cast<const std::uint8_t *>(res.data());
   const auto v_end = v_begin + res.size();

   const auto e_begin = expected.data();
   const auto e_end = expected.data() + expected.size();

   auto m = std::mismatch(v_begin, v_end, e_begin);
   REQUIRE(m.first == v_end);
   REQUIRE(m.second == e_end);
}

template <typename T>
void check_encoding(const T &value, std::initializer_list<std::uint8_t> expected) {
   INFO("Encoding " << print(value) << " into '" << hex(expected) << "'");

   using namespace cbor;

   std::vector<std::byte> target{};
   cbor::dynamic_buffer buf{target};

   std::error_code res;
   res = encode(buf, value);

   INFO("Encoding result: " << res.message());
   REQUIRE(!res);

   compare_arrays(value, target, expected);
}

template <typename T>
inline constexpr std::vector<std::byte> as_bytes(const T &v) {
   std::vector<std::byte> result{};
   result.resize(std::size(v));
   std::transform(std::begin(v), std::end(v), std::begin(result), [](auto e) { return static_cast<std::byte>(e); });
   return result;
}

inline constexpr std::byte as_byte(std::uint8_t v) {
   return std::byte{static_cast<std::uint8_t>(v)};
}

inline constexpr std::byte operator""_b(unsigned long long v) {
   if (v > std::numeric_limits<std::uint8_t>::max()) {
      // Dude, why?!
      std::terminate();
   }

   return std::byte{static_cast<std::uint8_t>(v)};
}

} // namespace test