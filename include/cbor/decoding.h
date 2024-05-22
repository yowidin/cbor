/**
 * @file   decoding.h
 * @author Dennis Sitelew 
 * @date   May 02, 2024
 */

#pragma once

#include <cbor/buffer.h>
#include <cbor/encoding.h>

#include <array>

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

} // namespace cbor