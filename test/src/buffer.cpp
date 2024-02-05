/**
 * @file   buffer.cpp
 * @author Dennis Sitelew 
 * @date   Feb 05, 2024
 */

#include <catch2/catch_test_macros.hpp>

#include <cbor/buffer.h>

#include <array>

using namespace cbor;

TEST_CASE("Dynamic buffer can grow", "[buffer]") {
   std::vector<std::uint8_t> target{};
   dynamic_buffer buf{target};

   // Don't change the buffer if nothing is written
   REQUIRE(target.empty());
   REQUIRE(buf.size() == 0);

   std::uint8_t bytes[] = {0xBE, 0xEF, 0xDE, 0xAD};

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
      std::span<const std::uint8_t> span{bytes};
      REQUIRE(!buf.write(span));
      ensure_equality();
   }

   SECTION("temporary span") {
      REQUIRE(!buf.write(bytes));
      ensure_equality();
   }
}

TEST_CASE("Dynamic buffer size can be limited", "[buffer]") {
   std::vector<std::uint8_t> target{};
   dynamic_buffer buf{target, 2};

   // Don't change the buffer if nothing is written
   REQUIRE(target.empty());
   REQUIRE(buf.size() == 0);

   REQUIRE(!buf.write(42));
   REQUIRE(target.size() == 1);
   REQUIRE(target[0] == 42);

   std::uint8_t bytes[] = {0xBE, 0xEF, 0xDE, 0xAD};

   SECTION("one byte") {
      std::error_code ec = buf.write(bytes[0]);
      REQUIRE(!ec);

      ec = buf.write(bytes[1]);
      REQUIRE(ec);
      REQUIRE(ec == error::buffer_overflow);

      REQUIRE(target.size() == 2);
      REQUIRE(target[0] == 42);
      REQUIRE(target[1] == 0xBE);
   }

   SECTION("initializer list") {
      std::error_code ec = buf.write({bytes[0], bytes[1], bytes[2], bytes[3]});
      REQUIRE(ec);
      REQUIRE(ec == error::buffer_overflow);

      REQUIRE(target.size() == 1);
      REQUIRE(target[0] == 42);
   }

   SECTION("local span") {
      std::span<const std::uint8_t> span{bytes};
      std::error_code ec = buf.write(span);

      REQUIRE(ec);
      REQUIRE(ec == error::buffer_overflow);

      REQUIRE(target.size() == 1);
      REQUIRE(target[0] == 42);
   }

   SECTION("temporary span") {
      std::error_code ec = buf.write(bytes);

      REQUIRE(ec);
      REQUIRE(ec == error::buffer_overflow);

      REQUIRE(target.size() == 1);
      REQUIRE(target[0] == 42);
   }
}

TEST_CASE("Static buffer size is limited", "[buffer]") {
   std::array<std::uint8_t, 2> target{};
   static_buffer buf{target};

   // Don't change the buffer if nothing is written
   REQUIRE(buf.size() == 0);

   REQUIRE(!buf.write(42));
   REQUIRE(buf.size() == 1);
   REQUIRE(target[0] == 42);

   std::uint8_t bytes[] = {0xBE, 0xEF, 0xDE, 0xAD};

   SECTION("one byte") {
      std::error_code ec = buf.write(bytes[0]);
      REQUIRE(!ec);

      ec = buf.write(bytes[1]);
      REQUIRE(ec);
      REQUIRE(ec == error::buffer_overflow);

      REQUIRE(buf.size() == 2);
      REQUIRE(target[0] == 42);
      REQUIRE(target[1] == 0xBE);
   }

   SECTION("initializer list") {
      std::error_code ec = buf.write({bytes[0], bytes[1], bytes[2], bytes[3]});
      REQUIRE(ec);
      REQUIRE(ec == error::buffer_overflow);

      REQUIRE(buf.size() == 1);
      REQUIRE(target[0] == 42);
   }

   SECTION("local span") {
      std::span<const std::uint8_t> span{bytes};
      std::error_code ec = buf.write(span);

      REQUIRE(ec);
      REQUIRE(ec == error::buffer_overflow);

      REQUIRE(buf.size() == 1);
      REQUIRE(target[0] == 42);
   }

   SECTION("temporary span") {
      std::error_code ec = buf.write(bytes);

      REQUIRE(ec);
      REQUIRE(ec == error::buffer_overflow);

      REQUIRE(buf.size() == 1);
      REQUIRE(target[0] == 42);
   }
}
