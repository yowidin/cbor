/**
 * @file   encoding.h
 * @author Dennis Sitelew
 * @date   Feb 13, 2024
 */

#pragma once

#include <cbor/buffer.h>
#include <cbor/error.h>
#include <cbor/export.h>
#include <cbor/type_traits.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <map>
#include <optional>
#include <type_traits>
#include <variant>

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

   //! Reserved, not well-formed in the present RFC document
   reserved_0 = 0x1C,
   reserved_1 = 0x1D,
   reserved_2 = 0x1E,
};

enum class simple_type : std::uint8_t {
   //! Unassigned values
   unassigned_start = 0x00,
   unassigned_end = 0x13,

   //! False
   false_type = 0x14,

   //! True
   true_type = 0x15,

   //! NULL
   null_type = 0x16,

   //! Undefined
   undefined_type = 0x17,

   //! Simple value (value 32..255 in following byte)
   simple_value = 0x18,

   //! IEEE 754 Half-Precision Float (16 bits follow)
   hp_float = 0x19,

   //! IEEE 754 Single-Precision Float (32 bits follow)
   sp_float = 0x1A,

   //! IEEE 754 Double-Precision Float (64 bits follow)
   dp_float = 0x1B,

   //! Reserved, not well-formed in the present RFC document
   reserved_start = 0x1C,
   reserved_end = 0x1E,

   //! "break" stop code for indefinite-length items
   break_type = 0x1F,
};

namespace detail {

inline constexpr uint8_t ZERO_EXTRA_BYTES_VALUE_LIMIT = 23U;
inline constexpr uint16_t ONE_EXTRA_BYTE_VALUE_LIMIT = 0xFFU;
inline constexpr uint32_t TWO_EXTRA_BYTES_VALUE_LIMIT = 0xFFFFU;
inline constexpr uint64_t FOUR_EXTRA_BYTES_VALUE_LIMIT = 0xFFFFFFFFU;

[[nodiscard]] std::error_code encode_argument(buffer &buf,
                                              major_type type,
                                              std::uint8_t argument,
                                              bool compress = true);
[[nodiscard]] std::error_code encode_argument(buffer &buf,
                                              major_type type,
                                              std::uint16_t argument,
                                              bool compress = true);
[[nodiscard]] std::error_code encode_argument(buffer &buf,
                                              major_type type,
                                              std::uint32_t argument,
                                              bool compress = true);
[[nodiscard]] std::error_code encode_argument(buffer &buf,
                                              major_type type,
                                              std::uint64_t argument,
                                              bool compress = true);

inline std::byte operator|(major_type m, argument_size s) {
   const auto lhs = static_cast<std::byte>(m);
   const auto rhs = static_cast<std::byte>(s);
   return lhs | rhs;
}

inline std::byte operator|(major_type m, simple_type s) {
   const auto lhs = static_cast<std::byte>(m);
   const auto rhs = static_cast<std::byte>(s);
   return lhs | rhs;
}

inline std::byte operator|(std::byte b, std::uint8_t v) {
   return b | std::byte{v};
}

} // namespace detail

////////////////////////////////////////////////////////////////////////////////
/// Integers
////////////////////////////////////////////////////////////////////////////////
template <UnsignedInt T>
[[nodiscard]] CBOR_EXPORT std::error_code encode_argument(buffer &buf, major_type type, T argument) {
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

template <UnsignedInt T>
[[nodiscard]] CBOR_EXPORT std::error_code encode(buffer &buf, T v) {
   return encode_argument(buf, major_type::unsigned_int, v);
}

template <SignedInt T>
[[nodiscard]] CBOR_EXPORT std::error_code encode(buffer &buf, T v) {
   using unsigned_t = std::make_unsigned_t<T>;
   if (v < 0) {
      const auto argument = static_cast<unsigned_t>(static_cast<T>(-1) - v);
      return encode_argument(buf, major_type::signed_int, argument);
   } else {
      return encode_argument(buf, major_type::unsigned_int, static_cast<unsigned_t>(v));
   }
}

////////////////////////////////////////////////////////////////////////////////
/// Enums
////////////////////////////////////////////////////////////////////////////////
template <Enum T>
[[nodiscard]] CBOR_EXPORT std::error_code encode(buffer &buf, T v) {
   const auto as_int = static_cast<std::underlying_type_t<T>>(v);
   return encode(buf, as_int);
}

////////////////////////////////////////////////////////////////////////////////
/// Byte Arrays
////////////////////////////////////////////////////////////////////////////////
[[nodiscard]] CBOR_EXPORT std::error_code encode(buffer &buf, buffer::const_span_t v);

////////////////////////////////////////////////////////////////////////////////
/// Strings
////////////////////////////////////////////////////////////////////////////////
[[nodiscard]] CBOR_EXPORT std::error_code encode(buffer &buf, std::string_view v);

[[nodiscard]] CBOR_EXPORT std::error_code encode(buffer &buf, const char *v);

template <typename CharT, typename Traits, typename Allocator>
[[nodiscard]] CBOR_EXPORT std::error_code encode(buffer &buf, const std::basic_string<CharT, Traits, Allocator> &v) {
   return encode(buf, std::string_view{v});
}

////////////////////////////////////////////////////////////////////////////////
/// Simple Types
////////////////////////////////////////////////////////////////////////////////
[[nodiscard]] CBOR_EXPORT std::error_code encode(buffer &buf, bool v);

[[nodiscard]] CBOR_EXPORT std::error_code encode(buffer &buf, std::nullptr_t);

template <typename T>
[[nodiscard]] CBOR_EXPORT std::error_code encode(buffer &buf, const std::optional<T> &v) {
   if (!v.has_value()) {
      return encode(buf, nullptr);
   } else {
      return encode(buf, *v);
   }
}

////////////////////////////////////////////////////////////////////////////////
/// Simple Types: floats
////////////////////////////////////////////////////////////////////////////////
[[nodiscard]] CBOR_EXPORT std::error_code encode(buffer &buf, float v);
[[nodiscard]] CBOR_EXPORT std::error_code encode(buffer &buf, double v);

////////////////////////////////////////////////////////////////////////////////
/// Variants
////////////////////////////////////////////////////////////////////////////////
template <typename T>
concept Encodable = requires(const T &t) { encode(std::declval<cbor::buffer &>(), t); };

template <typename... T>
concept AllEncodable = (Encodable<T> && ...);

namespace detail {

template <typename Current, std::size_t Extent>
consteval bool store_type_id(std::size_t idx, std::array<std::uint64_t, Extent> &ids) {
   ids[idx] = static_cast<std::uint64_t>(type_id_v<Current>);
   return true;
}

template <typename VariantT, std::size_t... Ns>
consteval bool collect_type_ids(std::array<std::uint64_t, std::variant_size_v<VariantT>> &ids,
                                std::index_sequence<Ns...>) {
   return ((store_type_id<std::variant_alternative_t<Ns, VariantT>>(Ns, ids)) && ...);
}

template <typename... T>
   requires AllWithTypeID<T...>
consteval bool all_alternatives_are_unique() {
   using variant_t = std::variant<T...>;
   const std::size_t num_alternatives = std::variant_size_v<variant_t>;
   using type_idx_t = std::make_index_sequence<num_alternatives>;

   std::array<std::uint64_t, num_alternatives> ids{};
   collect_type_ids<variant_t>(ids, type_idx_t{});

   std::sort(std::begin(ids), std::end(ids));

   auto it = std::adjacent_find(std::begin(ids), std::end(ids));
   return it == std::end(ids);
}

} // namespace detail

/**
 * Encode a boxed variant: the encoding is prefixed with a type identifier, specified via a type_id template type
 * specialization.
 *
 * This function intentionally doesn't support primitive variant types. It is intended to be used with structs and
 * classes, because the primitive types:
 * - Have explicit Type ID as part of the argument encoding.
 * - Should not be used as a variant selector (a well-defined struct is almost always better in communicating intent).
 *
 * @tparam T variant types.
 * @param buf Buffer to encode the value into.
 * @param v Value to be encoded.
 * @return Operation result.
 */
template <typename... T>
   requires AllWithTypeID<T...> && AllEncodable<T...>
[[nodiscard]] CBOR_EXPORT std::error_code encode(buffer &buf, const std::variant<T...> &v) {
   static_assert(detail::all_alternatives_are_unique<T...>(),
                 "TypeID duplicates are not allowed for variant alternatives");

   auto rollback_helper = buf.get_rollback_helper();

   const auto id = std::visit([](const auto &unwrapped) { return type_id_v<decltype(unwrapped)>; }, v);
   auto res = encode(buf, id);
   if (res) {
      return res;
   }

   res = std::visit([&buf](const auto &unwrapped) { return encode(buf, unwrapped); }, v);
   if (res) {
      return res;
   }

   rollback_helper.commit();

   return res;
}

////////////////////////////////////////////////////////////////////////////////
/// Structs
////////////////////////////////////////////////////////////////////////////////
namespace detail {

template <typename T>
bool encode_member(buffer &buf, const T &v, std::error_code &ec) {
   // Simplify the fold expression handling by capturing the error code via a reference
   // and returning false if encoding fails.
   ec = encode(buf, v);
   if (ec) {
      return false;
   }
   return true;
}

template <typename T, std::size_t... Ns>
std::error_code encode_all(buffer &buf, const T &v, std::index_sequence<Ns...>) {
   std::error_code ec;
   ((encode_member(buf, get_member<Ns>(v), ec)) && ...);
   return ec;
}
} // namespace detail

template <EncodableStruct T>
[[nodiscard]] CBOR_EXPORT std::error_code encode(buffer &buf, const T &v) {
   auto rollback_helper = buf.get_rollback_helper();

   using member_idx_t = std::make_index_sequence<get_member_count<T>()>;
   auto res = detail::encode_all(buf, v, member_idx_t{});
   if (res) {
      return res;
   }

   rollback_helper.commit();

   return res;
}

////////////////////////////////////////////////////////////////////////////////
/// Arrays
////////////////////////////////////////////////////////////////////////////////
template <typename T, std::size_t Extent>
   requires Encodable<T>
[[nodiscard]] CBOR_EXPORT std::error_code encode(buffer &buf, std::span<T, Extent> v) {
   auto rollback_helper = buf.get_rollback_helper();

   const auto size = v.size();
   auto res = encode_argument(buf, major_type::array, size);
   if (res) {
      return res;
   }

   for (const auto &e : v) {
      res = encode(buf, e);
      if (res) {
         return res;
      }
   }

   rollback_helper.commit();

   return res;
}

template <typename T, std::size_t Extent>
   requires Encodable<T>
[[nodiscard]] CBOR_EXPORT std::error_code encode(buffer &buf, const std::array<T, Extent> &v) {
   return encode(buf, std::span{v});
}

template <typename T, typename Allocator>
   requires Encodable<T>
[[nodiscard]] CBOR_EXPORT std::error_code encode(buffer &buf, const std::vector<T, Allocator> &v) {
   return encode(buf, std::span{v});
}

////////////////////////////////////////////////////////////////////////////////
/// Boxed Types
////////////////////////////////////////////////////////////////////////////////
template <WithTypeID T>
struct boxed {
   using value_t = T;
   static constexpr auto type_id = type_id_v<T>;

   explicit operator T &() { return v; };
   explicit operator const T &() const { return v; };

   value_t v;
};

template <typename T>
struct is_boxed : std::false_type {};

template <typename T>
struct is_boxed<boxed<T>> : std::true_type {};

template <typename T>
inline constexpr auto is_boxed_v = is_boxed<std::remove_cvref_t<T>>::value;

template <typename T>
concept Boxed = is_boxed_v<std::remove_cvref_t<T>>;

/**
 * Encode a boxed value.
 *
 * Boxed vlue is encoded as an array of two elements, where the first one is the type ID, and the second one is
 * the unboxed value.
 *
 * @tparam T value type
 * @param buf buffer to encode the value into.
 * @param v value to be encoded
 * @return operation result
 */
template <typename T>
[[nodiscard]] CBOR_EXPORT std::error_code encode(buffer &buf, const boxed<T> &v) {
   auto rollback_helper = buf.get_rollback_helper();

   auto res = encode_argument(buf, major_type::array, 2U);
   if (res) {
      return res;
   }

   res = encode(buf, v.type_id);
   if (res) {
      return res;
   }

   res = encode(buf, v.v);
   if (res) {
      return res;
   }

   rollback_helper.commit();

   return res;
}

////////////////////////////////////////////////////////////////////////////////
/// Dictionaries
////////////////////////////////////////////////////////////////////////////////
template <Encodable Key, Encodable T, typename Compare, typename Allocator>
[[nodiscard]] CBOR_EXPORT std::error_code encode(buffer &buf, const std::map<Key, T, Compare, Allocator> &v) {
   auto rollback_helper = buf.get_rollback_helper();

   // Encode size
   const auto size = std::size(v);
   auto res = encode_argument(buf, major_type::dictionary, size);
   if (res) {
      return res;
   }

   // Encode values
   for (const auto &[key, value] : v) {
      res = encode(buf, key);
      if (res) {
         return res;
      }

      res = encode(buf, value);
      if (res) {
         return res;
      }
   }

   rollback_helper.commit();

   return res;
}

} // namespace cbor