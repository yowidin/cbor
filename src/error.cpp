/**
 * @file   error.cpp
 * @author Dennis Sitelew 
 * @date   Feb 05, 2024
 */

#include <cbor/error.h>

#include <string>

namespace detail {

struct cbor_error_category : std::error_category {
   [[nodiscard]] const char *name() const noexcept override;
   [[nodiscard]] std::string message(int ev) const override;
};

const char *cbor_error_category::name() const noexcept {
   return "cbor-error";
}

std::string cbor_error_category::message(int ev) const {
   using namespace cbor;
   switch (static_cast<error>(ev)) {
      case error::success:
         return "not an error";

      case error::encoding_error:
         return "internal CBOR encoding error";

      case error::decoding_error:
         return "internal CBOR decoding error";

      case error::buffer_underflow:
         return "not enough buffer space to read";

      case error::buffer_overflow:
         return "not enough buffer space to write";

      case error::value_not_representable:
         return "value cannot be represented in CBOR";

      case error::invalid_usage:
         return "invalid library usage";

      case error::unexpected_type:
         return "unexpected type while decoding";

      case error::ill_formed:
         return "encoded byte-sequence is ill-formed";

      default:
         return "(unrecognized error)";
   }
}

} // namespace detail

namespace cbor {

const std::error_category &cbor_category() noexcept {
   static const detail::cbor_error_category category_const;
   return category_const;
}

} // namespace cbor