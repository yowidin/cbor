/**
 * @file   decoding.h
 * @author Dennis Sitelew 
 * @date   May 02, 2024
 */

#pragma once

#include <cbor/buffer.h>
#include <cbor/encoding.h>

#include <array>
#include <utility>
#include <variant>

namespace cbor {

namespace detail {

struct head {
   //!< Raw header byte
   std::uint8_t raw;

   //!< Masked-out major type
   major_type type;

   //!< Masked-out simple-type-bits (might be invalid, if the major type is not simple)
   simple_type simple;

   //!< Number of extra bytes in argument encoding
   std::uint8_t extra_bytes;

   //!< Up to 8 bytes of argument
   std::array<std::uint8_t, 8> argument;

   [[nodiscard]] std::error_code read(read_buffer &buf);

   [[nodiscard]] std::uint64_t decode_argument() const;
};

} // namespace detail

////////////////////////////////////////////////////////////////////////////////
/// Integers
////////////////////////////////////////////////////////////////////////////////
template <UnsignedInt T>
[[nodiscard]] CBOR_EXPORT std::error_code decode(read_buffer &buf, T &v) {
   detail::head head{};
   auto res = head.read(buf);
   if (res) {
      return res;
   }

   if (head.type != major_type::unsigned_int) {
      return error::unexpected_type;
   }

   auto u64 = head.decode_argument();
   if (u64 > max_int_v<T>) {
      return error::value_not_representable;
   }

   v = static_cast<T>(u64);

   return error::success;
}

template <SignedInt T>
[[nodiscard]] CBOR_EXPORT std::error_code decode(read_buffer &buf, T &v) {
   detail::head head{};
   auto res = head.read(buf);
   if (res) {
      return res;
   }

   if (head.type == major_type::unsigned_int) {
      // The requested type is signed, but the stored value is unsigned.
      // This just means that the stored value is positive.
      const auto u64 = head.decode_argument();
      if (u64 > max_int_v<T>) {
         return error::value_not_representable;
      }

      v = static_cast<T>(u64);
      return error::success;
   }

   if (head.type != major_type::signed_int) {
      return error::unexpected_type;
   }

   const auto u64 = head.decode_argument();
   if (u64 > max_int_v<std::int64_t>) {
      return error::value_not_representable;
   }

   // The value is encoded as u64 = -1 - n, meaning that to get back to n we have to do the following:
   // n = -1 - u64
   std::int64_t n = static_cast<std::int64_t>(-1) - static_cast<std::int64_t>(u64);
   if (n < min_int_v<T>) {
      return error::value_not_representable;
   }

   v = static_cast<T>(n);

   return error::success;
}

////////////////////////////////////////////////////////////////////////////////
/// Enums
////////////////////////////////////////////////////////////////////////////////
template <Enum T>
[[nodiscard]] CBOR_EXPORT std::error_code decode(read_buffer &buf, T &v) {
   using int_t = std::underlying_type_t<T>;

   int_t as_int;
   auto res = decode(buf, as_int);
   if (res) {
      return res;
   }

   v = static_cast<T>(as_int);
   return error::success;
}

////////////////////////////////////////////////////////////////////////////////
/// Byte Arrays
////////////////////////////////////////////////////////////////////////////////
template <typename Allocator>
[[nodiscard]] CBOR_EXPORT std::error_code decode(read_buffer &buf,
                                                 std::vector<std::byte, Allocator> &v,
                                                 typename std::vector<std::byte, Allocator>::size_type max_size =
                                                    max_int_v<typename std::vector<std::byte, Allocator>::size_type>) {
   using vector_t = std::vector<std::byte, Allocator>;
   static_assert(max_int_v<std::uint64_t> <= max_int_v<typename vector_t::size_type>);

   detail::head head{};
   auto res = head.read(buf);
   if (res) {
      return res;
   }

   if (head.type != major_type::byte_string) {
      return error::unexpected_type;
   }

   auto u64 = head.decode_argument();
   if (u64 > max_size) {
      return error::buffer_overflow;
   }

   v.resize(u64);

   if (u64 != 0) {
      return buf.read(buffer::span_t{v});
   }

   return error::success;
}

template <std::size_t Extent>
[[nodiscard]] CBOR_EXPORT std::error_code decode(read_buffer &buf, std::array<std::byte, Extent> &v) {
   using array_t = std::array<std::byte, Extent>;
   static_assert(max_int_v<std::uint64_t> <= max_int_v<typename array_t::size_type>);

   detail::head head{};
   auto res = head.read(buf);
   if (res) {
      return res;
   }

   if (head.type != major_type::byte_string) {
      return error::unexpected_type;
   }

   auto u64 = head.decode_argument();
   if (u64 > Extent) {
      return error::buffer_overflow;
   }

   if (u64 < Extent) {
      return error::buffer_underflow;
   }

   if (u64 != 0) {
      return buf.read(buffer::span_t{v});
   }

   return error::success;
}

////////////////////////////////////////////////////////////////////////////////
/// Strings
////////////////////////////////////////////////////////////////////////////////
template <typename CharT, typename Traits, typename Allocator>
[[nodiscard]] CBOR_EXPORT std::error_code decode(
   read_buffer &buf,
   std::basic_string<CharT, Traits, Allocator> &v,
   typename std::basic_string<CharT, Traits, Allocator>::size_type max_size =
      max_int_v<typename std::basic_string<CharT, Traits, Allocator>::size_type>) {
   using string_t = std::basic_string<CharT, Traits, Allocator>;
   static_assert(max_int_v<std::uint64_t> <= max_int_v<typename string_t::size_type>);
   static_assert(sizeof(typename string_t::value_type) == sizeof(std::byte));

   detail::head head{};
   auto res = head.read(buf);
   if (res) {
      return res;
   }

   if (head.type != major_type::text_string) {
      return error::unexpected_type;
   }

   auto u64 = head.decode_argument();
   if (u64 > max_size) {
      return error::buffer_overflow;
   }

   v.resize(u64);

   if (u64 != 0) {
      return buf.read(buffer::span_t{reinterpret_cast<std::byte *>(v.data()), v.size()});
   }

   return error::success;
}

////////////////////////////////////////////////////////////////////////////////
/// Simple Types
////////////////////////////////////////////////////////////////////////////////
[[nodiscard]] CBOR_EXPORT std::error_code decode(read_buffer &buf, bool &v);

template <typename T>
[[nodiscard]] CBOR_EXPORT std::error_code decode(read_buffer &buf, std::optional<T> &v) {
   {
      using namespace cbor::detail;

      // Handle the NULL-case
      auto rollback_helper = buf.get_rollback_helper();

      std::byte head;
      auto res = buf.read(head);
      if (res) {
         return res;
      }

      const auto nullptr_byte = major_type::simple | simple_type::null_type;
      if (head == nullptr_byte) {
         v = std::nullopt;
         rollback_helper.commit();
         return error::success;
      }
   }

   // At this point we rolled back the buffer read position, and can try reading out
   // the actual value
   T value;
   auto res = decode(buf, value);
   if (res) {
      return res;
   }

   v = value;
   return error::success;
}

////////////////////////////////////////////////////////////////////////////////
/// Simple Types: floats
////////////////////////////////////////////////////////////////////////////////
[[nodiscard]] CBOR_EXPORT std::error_code decode(read_buffer &buf, float &v);
[[nodiscard]] CBOR_EXPORT std::error_code decode(read_buffer &buf, double &v);

////////////////////////////////////////////////////////////////////////////////
/// Variants
////////////////////////////////////////////////////////////////////////////////
template <typename T>
concept Decodable = requires(T &t) { decode(std::declval<read_buffer &>(), t); };

template <typename... T>
concept AllDecodable = (Decodable<T> && ...);

namespace detail {

template <typename Current, typename... All>
   requires AllWithTypeID<All...> && AllDecodable<All...>
bool try_decode(std::uint64_t type_id, read_buffer &buf, std::variant<All...> &v, std::error_code &ec) {
   if (type_id_v<Current> != type_id) {
      // Not the encoded type - return true to avoid short-circuiting and continue search
      return true;
   }

   Current res;
   ec = decode(buf, res);
   if (!ec) {
      v = res;
   }

   // Always return false to signal that we finished decoding: we found a match for the Type ID
   // this will also short-circuit the try_decode_all function and finish decoding.
   return false;
}

template <typename VariantT, std::size_t... Ns>
std::error_code try_decode_all(std::uint64_t type_id, read_buffer &buf, VariantT &v, std::index_sequence<Ns...>) {
   std::error_code ec;
   bool missing_type = ((try_decode<std::variant_alternative_t<Ns, VariantT>>(type_id, buf, v, ec)) && ...);
   if (missing_type) {
      // The encoded value is not in the variant's alternatives set
      return error::unexpected_type;
   }
   return ec;
}
} // namespace detail

/**
 * Decode a variant.
 *
 * Variants are encoded as an array of two elements, where the first one is a type identifier and the second one is
 * the encoding of an alternative.
 *
 * This function intentionally doesn't support primitive variant types. It is intended to be used with structs and
 * classes, because the primitive types:
 * - Have explicit Type ID as part of the argument encoding.
 * - Should not be used as a variant selector (a well-defined struct is almost always better in communicating intent).
 *
 * @tparam T variant types.
 * @param[in] buf Buffer to decode the value from.
 * @param[out] v Value to be decoded.
 * @return Operation result.
 */
template <typename... T>
   requires AllWithTypeID<T...> && AllDecodable<T...>
[[nodiscard]] CBOR_EXPORT std::error_code decode(read_buffer &buf, std::variant<T...> &v) {
   static_assert(detail::all_alternatives_are_unique<T...>(),
                 "TypeID duplicates are not allowed for variant alternatives");
   // Decode the array header
   std::byte head;
   auto res = buf.read(head);
   if (res) {
      return res;
   }

   const auto array_byte = static_cast<std::byte>(major_type::array) | static_cast<std::byte>(2);
   if (head != array_byte) {
      return error::decoding_error;
   }

   // Decode the type ID
   std::int64_t type_id;
   res = decode(buf, type_id);
   if (res) {
      return res;
   }

   // Use the type id to decode a currently active alternative
   using type_idx_t = std::make_index_sequence<std::variant_size_v<std::remove_cvref_t<decltype(v)>>>;
   res = detail::try_decode_all(type_id, buf, v, type_idx_t{});
   return res;
}

////////////////////////////////////////////////////////////////////////////////
/// Structs
////////////////////////////////////////////////////////////////////////////////
namespace detail {

template <typename T>
bool decode_member(read_buffer &buf, T &v, std::error_code &ec) {
   // Simplify the fold expression handling by capturing the error code via a reference
   // and returning false if decoding fails.
   ec = decode(buf, v);
   if (ec) {
      return false;
   }
   return true;
}

template <typename T, std::size_t... Ns>
std::error_code decode_all(read_buffer &buf, T &v, std::index_sequence<Ns...>) {
   std::error_code ec;
   ((decode_member(buf, get_member_non_const<Ns>(v), ec)) && ...);
   return ec;
}
} // namespace detail

template <DecodableStruct T>
[[nodiscard]] CBOR_EXPORT std::error_code decode(read_buffer &buf, T &v) {
   using member_idx_t = std::make_index_sequence<get_member_count<T>()>;
   auto res = detail::decode_all(buf, v, member_idx_t{});
   if (res) {
      return res;
   }

   return res;
}

} // namespace cbor