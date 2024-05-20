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

template <typename T>
void expect(std::initializer_list<std::uint8_t> cbor, T expected) {
   std::vector<std::byte> as_bytes{};
   for (auto v : cbor) {
      as_bytes.push_back(static_cast<std::byte>(v));
   }

   cbor::buffer::const_span_t span{as_bytes};
   cbor::read_buffer buf{span};

   T decoded;
   auto res = cbor::decode(buf, decoded);

   auto print_error = [&](auto msg, auto res) {
      std::cerr << msg << " for: " << shp::hex(expected) << ":\n";
      std::cerr << "Expected:\n";
      std::cerr << shp::hex(expected) << "\n\nFound:\n";
      std::cerr << shp::hex(decoded) << "\n\n";
      std::cerr << "Error code: " << res.message() << std::endl;
   };

   if (res) {
      print_error("Decoding failed", res);
   }
   REQUIRE(!res);

   const auto full_match = (expected == decoded);
   if (!full_match) {
      print_error("Decoded value mismatch", res);
   }
   REQUIRE(full_match);

   const auto all_consumed = (buf.read_position() == as_bytes.size());
   if (!all_consumed) {
      print_error("Not all bytes consumed", res);
      std::cerr << "Bytes consumed: " << buf.read_position() << " != " << as_bytes.size() << std::endl;
   }
   REQUIRE(all_consumed);
}

} // namespace test
