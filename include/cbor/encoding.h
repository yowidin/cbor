/**
 * @file   encoding.h
 * @author Dennis Sitelew
 * @date   Feb 13, 2024
 */

#pragma once

#include <cbor/buffer.h>
#include <cbor/error.h>
#include <cbor/export.h>

#include <cstdint>
#include <limits>
#include <type_traits>

namespace cbor {

enum class major_type : std::uint8_t {
   //! Major type 0
   //! An unsigned integer in the range 0..(2^64)-1 inclusive. The value of the encoded item is the argument itself. For
   //! example, the integer 10 is denoted as the one byte 0b000_01010 (major type 0, additional information 10). The
   //! integer 500 would be 0b000_11001 (major type 0, additional information 25) followed by the two bytes 0x01f4,
   //! which
   //! is 500 in decimal.
   unsigned_int = 0x00,

   //! Major type 1
   //! A negative integer in the range (-2^64)..-1 inclusive. The value of the item is -1 minus the argument. For
   //! example, the integer -500 would be 0b001_11001 (major type 1, additional information 25) followed by the two
   //! bytes
   //! 0x01f3, which is 499 in decimal.
   signed_int = 0x20,

   //! Major type 2:
   //! A byte string. The number of bytes in the string is equal to the argument. For example, a byte string whose
   //! length is 5 would have an initial byte of 0b010_00101 (major type 2, additional information 5 for the length),
   //! followed by 5 bytes of binary content. A byte string whose length is 500 would have 3 initial bytes of
   //! 0b010_11001 (major type 2, additional information 25 to indicate a two-byte length) followed by the two bytes
   //! 0x01f4 for a length of 500, followed by 500 bytes of binary content.
   byte_string = 0x40,

   //! Major type 3:
   //! A text string (Section 2) encoded as UTF-8 [RFC3629]. The number of bytes in the string is equal to the argument.
   //! A string containing an invalid UTF-8 sequence is well-formed but invalid (Section 1.2). This type is provided for
   //! systems that need to interpret or display human-readable text, and allows the differentiation between
   //! unstructured bytes and text that has a specified repertoire (that of Unicode) and encoding (UTF-8). In contrast
   //! to formats such as JSON, the Unicode characters in this type are never escaped. Thus, a newline character
   //! (U+000A) is always represented in a string as the byte 0x0a, and never as the bytes 0x5c6e (the characters "\"
   //! and "n") nor as 0x5c7530303061 (the characters "\", "u", "0", "0", "0", and "a").
   text_string = 0x60,

   //! Major type 4:
   //! An array of data items. In other formats, arrays are also called lists, sequences, or tuples (a "CBOR sequence"
   //! is something slightly different, though [RFC8742]). The argument is the number of data items in the array. Items
   //! in an array do not need to all be of the same type. For example, an array that contains 10 items of any type
   //! would have an initial byte of 0b100_01010 (major type 4, additional information 10 for the length) followed by
   //! the 10 remaining items.
   array = 0x80,

   //! Major type 5:
   //! A map of pairs of data items. Maps are also called tables, dictionaries, hashes, or objects (in JSON). A map is
   //! comprised of pairs of data items, each pair consisting of a key that is immediately followed by a value. The
   //! argument is the number of pairs of data items in the map. For example, a map that contains 9 pairs would have an
   //! initial byte of 0b101_01001 (major type 5, additional information 9 for the number of pairs) followed by the 18
   //! remaining items. The first item is the first key, the second item is the first value, the third item is the
   //! second key, and so on. Because items in a map come in pairs, their total number is always even: a map that
   //! contains an odd number of items (no value data present after the last key data item) is not well-formed. A map
   //! that has duplicate keys may be well-formed, but it is not valid, and thus it causes indeterminate decoding;
   dictionary = 0xA0,

   //! Major type 6:
   //! A tagged data item ("tag") whose tag number, an integer in the range 0..(2^64)-1 inclusive, is the argument and
   //! whose enclosed data item (tag content) is the single encoded data item that follows the head.
   tag = 0xC0,

   //! Major type 7:
   //! Floating-point numbers and simple values, as well as the "break" stop code.
   simple = 0xE0,
};

enum class argument_size : std::uint8_t {
   //! The additional information is directly stored in the same byte (last it is less than 24)
   no_bytes = 0x00,

   //! The additional information is stored in the following byte
   one_byte = 0x18,

   //! The additional information is stored in the following two bytes
   two_bytes = 0x19,

   //! The additional information is stored in the following four bytes
   four_bytes = 0x1A,

   //! The additional information is stored in the following eight bytes
   eight_bytes = 0x1B,
};

template <typename T>
struct is_bool : std::bool_constant<false> {};

template <>
struct is_bool<bool> : std::bool_constant<false> {};

template <typename T>
using is_bool_t = is_bool<std::decay_t<T>>::type;

template <typename T>
inline constexpr bool is_bool_v = is_bool_t<T>::value;

template <typename T>
inline constexpr auto min_int_v = std::numeric_limits<T>::min();

template <typename T>
inline constexpr auto max_int_v = std::numeric_limits<T>::max();

namespace detail {
[[nodiscard]] std::error_code encode_argument(buffer &buf, major_type type, std::uint8_t argument);
[[nodiscard]] std::error_code encode_argument(buffer &buf, major_type type, std::uint16_t argument);
[[nodiscard]] std::error_code encode_argument(buffer &buf, major_type type, std::uint32_t argument);
[[nodiscard]] std::error_code encode_argument(buffer &buf, major_type type, std::uint64_t argument);
} // namespace detail

template <typename Integer>
   requires(std::is_unsigned_v<Integer> and !is_bool_v<Integer>)
[[nodiscard]] CBOR_EXPORT std::error_code encode_argument(buffer &buf, major_type type, Integer argument) {
   if (argument <= max_int_v<std::uint8_t>) {
      return detail::encode_argument(buf, type, static_cast<std::uint8_t>(argument));
   }

   if (argument <= max_int_v<std::uint16_t>) {
      return detail::encode_argument(buf, type, static_cast<std::uint16_t>(argument));
   }

   if (argument <= max_int_v<std::uint32_t>) {
      return detail::encode_argument(buf, type, static_cast<std::uint32_t>(argument));
   }

   if (argument <= max_int_v<std::uint64_t>) {
      return detail::encode_argument(buf, type, static_cast<std::uint64_t>(argument));
   }

   return error::value_not_representable;
}

template <typename Integer>
   requires(std::is_unsigned_v<Integer> and std::is_integral_v<Integer> and !is_bool_v<Integer>)
[[nodiscard]] CBOR_EXPORT std::error_code encode(buffer &buf, Integer v) {
   return encode_argument(buf, major_type::unsigned_int, v);
}

template <typename Integer>
   requires(std::is_signed_v<Integer> and std::is_integral_v<Integer>)
[[nodiscard]] CBOR_EXPORT std::error_code encode(buffer &buf, Integer v) {
   using unsigned_t = std::make_unsigned_t<Integer>;
   if (v < 0) {
      const auto argument = static_cast<unsigned_t>(static_cast<unsigned_t>(-1) - v);
      return encode_argument(buf, major_type::signed_int, argument);
   } else {
      return encode_argument(buf, major_type::unsigned_int, static_cast<unsigned_t>(v));
   }
}

} // namespace cbor