/**
 * @file   decoding.h
 * @author Dennis Sitelew 
 * @date   May 20, 2024
 */

#pragma once

#include <cbor/decoding.h>

#include <shp/shp.h>
#include <catch2/catch_test_macros.hpp>

#include <test/encoding.h>

#include <iostream>

namespace test {

using span_t = cbor::buffer::const_span_t;

template <typename T>
void decode(std::initializer_list<std::uint8_t> cbor, T &decoded) {
   const auto cbor_bytes = as_bytes(cbor);
   const cbor::buffer::const_span_t span{cbor_bytes};
   cbor::read_buffer buf{span};

   auto res = cbor::decode(buf, decoded);

   INFO("Result: " << res.message());
   REQUIRE(!res);

   INFO("Consumed " << buf.read_position() << " out of " << cbor_bytes.size() << " bytes");
   REQUIRE(buf.read_position() == cbor_bytes.size());
}

template <typename T>
void expect(std::initializer_list<std::uint8_t> cbor, T expected) {
   INFO("Decoding " << print(expected) << " from '" << hex(cbor) << "'");

   T decoded;
   test::decode(cbor, decoded);

   REQUIRE(expected == decoded);
}

template <typename cborT, typename DecodedT, typename ExpectedT>
void compare_containers(const cborT &cbor, const DecodedT &decoded, const ExpectedT &expected) {
   INFO("Comparing '" << hex(decoded) << "' with '" << hex(expected) << "' from '" << hex(cbor) << "'");

   REQUIRE(std::size(decoded) == std::size(expected));

   auto expected_bytes = as_bytes(expected);
   auto decoded_bytes = as_bytes(decoded);

   const auto d_begin = std::begin(decoded_bytes);
   const auto d_end = std::end(decoded_bytes);

   const auto e_begin = std::begin(expected_bytes);
   const auto e_end = std::end(expected_bytes);

   auto m = std::mismatch(d_begin, d_end, e_begin);
   REQUIRE(m.first == d_end);
   REQUIRE(m.second == e_end);
}

} // namespace test
