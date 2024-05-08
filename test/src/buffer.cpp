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

   SECTION("Rollbacks and commits") {
      REQUIRE(buf.size() == 0);
      {
         // Rolling back, also test the move constructor
         auto helper = std::move(buf.get_rollback_helper());

         REQUIRE(!buf.write({bytes[0]}));
         REQUIRE(buf.size() == 1);
      }
      REQUIRE(buf.size() == 0);

      REQUIRE(buf.size() == 0);
      {
         // Commiting, also test the move assignment
         auto tmp = buf.get_rollback_helper();
         auto helper = buf.get_rollback_helper();
         helper = std::move(tmp);

         REQUIRE(!buf.write({bytes[0]}));
         REQUIRE(buf.size() == 1);
         helper.commit();
      }
      REQUIRE(buf.size() == 1);
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
   REQUIRE(buf.size() == 1);
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

   SECTION("Rollbacks and commits") {
      // NOTE: We start with one byte already in the buffer
      REQUIRE(buf.size() == 1);
      {
         // Rolling back
         auto helper = buf.get_rollback_helper();
         REQUIRE(!buf.write({bytes[0]}));
         REQUIRE(buf.size() == 2);
      }
      REQUIRE(buf.size() == 1);

      REQUIRE(buf.size() == 1);
      {
         // Commiting
         auto helper = buf.get_rollback_helper();
         REQUIRE(!buf.write({bytes[0]}));
         REQUIRE(buf.size() == 2);
         helper.commit();
      }
      REQUIRE(buf.size() == 2);
   }
}

TEST_CASE("Read buffer - NULL-source is allowed", "[buffer, read_buffer]") {
   const buffer::const_span_t src{};

   read_buffer buf{src};
   REQUIRE(buf.read_position() == 0);

   SECTION("Read byte") {
      std::byte b;
      std::error_code ec;

      REQUIRE_NOTHROW(ec = buf.read(b));
      REQUIRE(ec);
      REQUIRE(ec == error::invalid_usage);
      REQUIRE(buf.read_position() == 0);
   }

   SECTION("Read span") {
      std::array<std::byte, 4> target_buffer{};
      buffer::span_t target{target_buffer};
      std::error_code ec;

      REQUIRE_NOTHROW(ec = buf.read(target));
      REQUIRE(ec);
      REQUIRE(ec == error::invalid_usage);
      REQUIRE(buf.read_position() == 0);
   }
}

TEST_CASE("Read buffer - reading", "[buffer, read_buffer]") {
   const std::array<std::byte, 4> source_buffer{0x01_b, 0x02_b, 0x03_b, 0x04_b};
   const buffer::const_span_t src{source_buffer};

   read_buffer buf{src};
   REQUIRE(buf.read_position() == 0);

   SECTION("Read byte") {
      std::byte b;
      std::error_code ec;

      // Normal read
      for (auto i = 0; i < source_buffer.size(); ++i) {
         REQUIRE(buf.read_position() == i);

         REQUIRE_NOTHROW(ec = buf.read(b));
         REQUIRE(!ec);
         REQUIRE(ec == error::success);
         REQUIRE(b == source_buffer[i]);

         REQUIRE(buf.read_position() == i + 1);
      }

      // Buffer underflow
      REQUIRE_NOTHROW(ec = buf.read(b));
      REQUIRE(ec);
      REQUIRE(ec == error::buffer_underflow);
      REQUIRE(buf.read_position() == source_buffer.size());
   }

   SECTION("Empty target span") {
      buffer::span_t target{};
      std::error_code ec;

      // Read the middle chunk
      REQUIRE_NOTHROW(ec = buf.read(target));
      REQUIRE(ec);
      REQUIRE(ec == error::invalid_usage);
   }

   SECTION("Read span") {
      std::array<std::byte, 2> target_buffer{};
      buffer::span_t target{target_buffer};
      std::error_code ec;
      std::byte b;

      // Offset the read position by one byte - we want to read from the middle of the buffer
      REQUIRE_NOTHROW(ec = buf.read(b));
      REQUIRE(!ec);
      REQUIRE(ec == error::success);
      REQUIRE(b == source_buffer[0]);
      REQUIRE(buf.read_position() == 1);

      // Read the middle chunk
      REQUIRE_NOTHROW(ec = buf.read(target));
      REQUIRE(!ec);
      REQUIRE(ec == error::success);
      REQUIRE(buf.read_position() == 1 + target_buffer.size());

      auto ensure_equality = [&](auto begin, auto end) {
         auto tmp = mismatch(std::begin(target_buffer), std::end(target_buffer), begin);
         REQUIRE(tmp.first == std::end(target_buffer));
         REQUIRE(tmp.second == end);
      };

      // Make sure the middle is readout
      ensure_equality(std::begin(source_buffer) + 1, std::begin(source_buffer) + target_buffer.size() + 1);

      // Try reading more, than source buffer can provide
      REQUIRE_NOTHROW(ec = buf.read(target));
      REQUIRE(ec);
      REQUIRE(ec == error::buffer_underflow);
      REQUIRE(buf.read_position() == 1 + target_buffer.size());
   }
}
