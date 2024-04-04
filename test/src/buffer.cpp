/**
 * @file   buffer.cpp
 * @author Dennis Sitelew 
 * @date   Feb 05, 2024
 */

#include <catch2/catch_test_macros.hpp>

#include <cbor/buffer.h>

#include <array>

using namespace cbor;

namespace {
inline constexpr std::byte operator""_b(unsigned long long v) {
   if (v > std::numeric_limits<std::uint8_t>::max()) {
      // Dude, why?!
      std::terminate();
   }

   return std::byte{static_cast<std::uint8_t>(v)};
}
} // namespace

TEST_CASE("Dynamic buffer can grow", "[buffer]") {
   std::vector<std::byte> target{};
   dynamic_buffer buf{target};

   // Don't change the buffer if nothing is written
   REQUIRE(target.empty());
   REQUIRE(buf.size() == 0);

   std::byte bytes[] = {0xBE_b, 0xEF_b, 0xDE_b, 0xAD_b};

   auto ensure_equality = [&]() {
      using namespace std;
      auto tmp = mismatch(begin(bytes), end(bytes), begin(target));
      REQUIRE(tmp.first == end(bytes));
      REQUIRE(tmp.second == end(target));
   };

   SECTION("one byte") {
      for (auto b : bytes) {
         REQUIRE(!buf.write(b));
      }

      ensure_equality();
   }

   SECTION("initializer list") {
      REQUIRE(!buf.write({bytes[0], bytes[1], bytes[2], bytes[3]}));
      ensure_equality();
   }

   SECTION("local span") {
      std::span<const std::byte> span{bytes};
      REQUIRE(!buf.write(span));
      ensure_equality();
   }

   SECTION("temporary span") {
      REQUIRE(!buf.write(bytes));
      ensure_equality();
   }
}

TEST_CASE("Dynamic buffer size can be limited", "[buffer]") {
   std::vector<std::byte> target{};
   dynamic_buffer buf{target, 2};

   // Don't change the buffer if nothing is written
   REQUIRE(target.empty());
   REQUIRE(buf.size() == 0);

   REQUIRE(!buf.write(42_b));
   REQUIRE(target.size() == 1);
   REQUIRE(target[0] == 42_b);

   std::byte bytes[] = {0xBE_b, 0xEF_b, 0xDE_b, 0xAD_b};

   SECTION("one byte") {
      std::error_code ec = buf.write(bytes[0]);
      REQUIRE(!ec);

      ec = buf.write(bytes[1]);
      REQUIRE(ec);
      REQUIRE(ec == error::buffer_overflow);

      REQUIRE(target.size() == 2);
      REQUIRE(target[0] == 42_b);
      REQUIRE(target[1] == 0xBE_b);
   }

   SECTION("initializer list") {
      std::error_code ec = buf.write({bytes[0], bytes[1], bytes[2], bytes[3]});
      REQUIRE(ec);
      REQUIRE(ec == error::buffer_overflow);

      REQUIRE(target.size() == 1);
      REQUIRE(target[0] == 42_b);
   }

   SECTION("local span") {
      std::span<const std::byte> span{bytes};
      std::error_code ec = buf.write(span);

      REQUIRE(ec);
      REQUIRE(ec == error::buffer_overflow);

      REQUIRE(target.size() == 1);
      REQUIRE(target[0] == 42_b);
   }

   SECTION("temporary span") {
      std::error_code ec = buf.write(bytes);

      REQUIRE(ec);
      REQUIRE(ec == error::buffer_overflow);

      REQUIRE(target.size() == 1);
      REQUIRE(target[0] == 42_b);
   }
}

TEST_CASE("Static buffer size is limited", "[buffer]") {
   std::array<std::byte, 2> target{};
   static_buffer buf{target};

   // Don't change the buffer if nothing is written
   REQUIRE(buf.size() == 0);

   REQUIRE(!buf.write(42_b));
   REQUIRE(buf.size() == 1);
   REQUIRE(target[0] == 42_b);

   std::byte bytes[] = {0xBE_b, 0xEF_b, 0xDE_b, 0xAD_b};

   SECTION("one byte") {
      std::error_code ec = buf.write(bytes[0]);
      REQUIRE(!ec);

      ec = buf.write(bytes[1]);
      REQUIRE(ec);
      REQUIRE(ec == error::buffer_overflow);

      REQUIRE(buf.size() == 2);
      REQUIRE(target[0] == 42_b);
      REQUIRE(target[1] == 0xBE_b);
   }

   SECTION("initializer list") {
      std::error_code ec = buf.write({bytes[0], bytes[1], bytes[2], bytes[3]});
      REQUIRE(ec);
      REQUIRE(ec == error::buffer_overflow);

      REQUIRE(buf.size() == 1);
      REQUIRE(target[0] == 42_b);
   }

   SECTION("local span") {
      std::span<const std::byte> span{bytes};
      std::error_code ec = buf.write(span);

      REQUIRE(ec);
      REQUIRE(ec == error::buffer_overflow);

      REQUIRE(buf.size() == 1);
      REQUIRE(target[0] == 42_b);
   }

   SECTION("temporary span") {
      std::error_code ec = buf.write(bytes);

      REQUIRE(ec);
      REQUIRE(ec == error::buffer_overflow);

      REQUIRE(buf.size() == 1);
      REQUIRE(target[0] == 42_b);
   }
}