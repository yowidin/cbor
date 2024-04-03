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
void compare_arrays(T v, const std::vector<std::uint8_t> &res, const std::vector<std::uint8_t> &expected) {
   REQUIRE(res.size() == expected.size());

   auto m = std::mismatch(std::begin(res), std::end(res), std::begin(expected));
   if (m.first != std::end(res) || m.second != std::end(expected)) {
      std::cerr << "Array missmatch for " << shp::hex(v) << ":\n"
                << "Expected:\n"
                << shp::hex(expected) << "\n\nFound:\n"
                << shp::hex(res) << std::endl;
   }
   REQUIRE(m.first == std::end(res));
   REQUIRE(m.second == std::end(expected));
}

template <typename T>
void check_encoding(const T &value, std::initializer_list<std::uint8_t> expected) {
   std::vector<std::uint8_t> target{};
   cbor::dynamic_buffer buf{target};

   std::error_code res;
   res = cbor::encode(buf, value);

   REQUIRE(!res);

   compare_arrays(value, target, expected);
}

} // namespace test