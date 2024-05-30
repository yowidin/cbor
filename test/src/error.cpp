/**
 * @file   error.cpp
 * @author Dennis Sitelew 
 * @date   May 03, 2024
 */

#include <catch2/catch_test_macros.hpp>

#include <cbor/error.h>

#include <algorithm>
#include <array>
#include <list>
#include <string>

using namespace cbor;

TEST_CASE("Error - category is named", "[error]") {
   std::error_code ec = error::success;
   REQUIRE(!std::string{ec.category().name()}.empty());
}

TEST_CASE("Error - success should be zero", "[error]") {
   std::error_code ec = error::success;
   REQUIRE(ec.value() == 0);
   REQUIRE(!ec);
}

TEST_CASE("Error - codes are named", "[error]") {
   const std::error_code bad_code{-1, cbor_category()};
   REQUIRE(!bad_code.message().empty());

   std::list<std::string> messages{};

   // Ensure error messages for valid error codes have distinct name, which do not match an invalid error code
   auto check_code = [&](error err) {
      // Error message should not match an invalid message
      const std::error_code ec{err};
      REQUIRE(ec != bad_code);
      REQUIRE(ec.message() != bad_code.message());

      // Error message should be unique
      const auto it = std::find(std::begin(messages), std::end(messages), ec.message());
      REQUIRE(it == std::end(messages));
      messages.push_back(ec.message());
   };

   std::array codes = {
      error::success,         error::encoding_error,          error::decoding_error, error::buffer_underflow,
      error::buffer_overflow, error::value_not_representable, error::invalid_usage,  error::unexpected_type, error::ill_formed,
   };

   for (auto code : codes) {
      check_code(code);
   }
}
