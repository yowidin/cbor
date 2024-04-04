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

namespace test {

template <typename T>
void compare_arrays(T v, const std::vector<std::byte> &res, const std::vector<std::uint8_t> &expected) {
   REQUIRE(res.size() == expected.size());

   const auto v_begin = reinterpret_cast<const std::uint8_t *>(res.data());
   const auto v_end = v_begin + res.size();

   const auto e_begin = expected.data();
   const auto e_end = expected.data() + expected.size();

   auto m = std::mismatch(v_begin, v_end, e_begin);
   if (m.first != v_end || m.second != e_end) {
      std::cerr << "Array missmatch for " << shp::hex(v) << ":\n"
                << "Expected:\n"
                << shp::hex(expected) << "\n\nFound:\n"
                << shp::hex(res) << std::endl;
   }
   REQUIRE(m.first == v_end);
   REQUIRE(m.second == e_end);
}

template <typename T>
void check_encoding(const T &value, std::initializer_list<std::uint8_t> expected) {
   std::vector<std::byte> target{};
   cbor::dynamic_buffer buf{target};

   std::error_code res;
   res = cbor::encode(buf, value);

   REQUIRE(!res);

   compare_arrays(value, target, expected);
}

} // namespace test