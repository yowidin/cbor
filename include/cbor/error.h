/**
 * @file   error.h
 * @author Dennis Sitelew 
 * @date   Feb 05, 2024
 */

#pragma once

#include <cbor/export.h>

#include <system_error>

namespace cbor {

enum class error {
   //! Not an error
   success = 0,

   //! Encoding error
   encoding_error,

   //! Decoding error
   decoding_error,

   //! Not enough buffer space left to read an entry
   buffer_underflow,

   //! Not enough buffer space left to write an entry
   buffer_overflow,

   //! The provided value cannot be represented in CBOR (by this library)
   value_not_representable,

   //! Invalid library usage
   invalid_usage,

   //! Encountered an unexpected type while decoding
   unexpected_type,
};

const std::error_category &cbor_category() noexcept CBOR_EXPORT;

inline std::error_code make_error_code(cbor::error ec) {
   return {static_cast<int>(ec), cbor_category()};
}

} // namespace cbor

namespace std {

template <>
struct CBOR_EXPORT is_error_code_enum<cbor::error> : true_type {};

} // namespace std
