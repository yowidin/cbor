/**
 * @file   maps.cpp
 * @author Dennis Sitelew 
 * @date   Jun 05, 2024
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <test/decoding.h>

#include <cbor/decoding.h>

#include <map>
#include <unordered_map>

using namespace test;

TEST_CASE("Dictionary - basic decoding", "[decoding, dictionary]") {
   using namespace std;

   // {1: "1", 2: "22"}
   expect({0xA2, 0x01, 0x61, 0x31, 0x02, 0x62, 0x32, 0x32}, map<int, string>{{1, "1"}, {2, "22"}});

   // {}
   expect({0xA0}, map<int, int>{});

   // {1: 2, 3: 4}
   expect({0xA2, 0x01, 0x02, 0x03, 0x04}, map<int, int>{{1, 2}, {3, 4}});

   // {"a": "A", "b": "B", "c": "C", "d": "D", "e": "E"}
   expect({0xA5, 0x61, 0x61, 0x61, 0x41, 0x61, 0x62, 0x61, 0x42, 0x61, 0x63,
           0x61, 0x43, 0x61, 0x64, 0x61, 0x44, 0x61, 0x65, 0x61, 0x45},
          map<string, string>{{"a", "A"}, {"b", "B"}, {"c", "C"}, {"d", "D"}, {"e", "E"}});

   SECTION("Unordered map sanity check") {
      const std::array source{0xA2_b, 0x01_b, 0x61_b, 0x31_b, 0x02_b, 0x62_b, 0x32_b, 0x32_b};
      cbor::read_buffer buf{source};

      unordered_map<int, string> v;
      REQUIRE(cbor::decode(buf, v) == cbor::error::success);
      REQUIRE(v.size() == 2);
      REQUIRE(v[1] == "1");
      REQUIRE(v[2] == "22");
   }
}

TEST_CASE("Dictionary - decoding error cases", "[decoding, dictionary, errors]") {
   std::map<int, int> v{};

   SECTION("Not enough data to read head") {
      std::array<std::byte, 2> source{};
      cbor::read_buffer buf{span_t{source.data(), 0}};
      REQUIRE(cbor::decode(buf, v) == cbor::error::buffer_underflow);
   }

   SECTION("Invalid major type") {
      std::array source{0x20_b};
      cbor::read_buffer buf{span_t{source}};
      REQUIRE(cbor::decode(buf, v) == cbor::error::unexpected_type);
   }

   SECTION("Size above max size") {
      std::array source{0xA2_b, 0x01_b, 0x02_b, 0x03_b, 0x04_b};
      cbor::read_buffer buf{span_t{source}};
      REQUIRE(cbor::decode(buf, v, 1) == cbor::error::buffer_overflow);
   }

   SECTION("Error reading a key") {
      std::array source{0xA2_b};
      cbor::read_buffer buf{span_t{source}};
      REQUIRE(cbor::decode(buf, v) == cbor::error::buffer_underflow);
   }

   SECTION("Error reading a value") {
      std::array source{0xA2_b, 0x01_b, 0x02_b, 0x03_b};
      cbor::read_buffer buf{span_t{source}};
      REQUIRE(cbor::decode(buf, v) == cbor::error::buffer_underflow);
   }
}
