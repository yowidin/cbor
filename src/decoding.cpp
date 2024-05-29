/**
 * @file   decoding.cpp
 * @author Dennis Sitelew 
 * @date   May 02, 2024
 */

#include <cbor/decoding.h>

#include <fhf/fhf.hh>

#include <array>
#include <ranges>

namespace cbor::detail {

std::error_code head::read(read_buffer &buf) {
   std::byte b0;
   auto res = buf.read(b0);
   if (res) {
      return res;
   }

   raw = static_cast<std::uint8_t>(b0);

   // The three MSb encode the major type
   type = static_cast<major_type>(raw & 0xE0);

   // The five LSb might encode the argument size (or they might contain a simple type)
   auto size = static_cast<argument_size>(raw & 0x1F);

   // The five LSb might encode the simple type (or they might contain a simple type)
   simple = static_cast<simple_type>(raw & 0x1F);

   switch (size) {
      case argument_size::one_byte:
         extra_bytes = 1;
         break;
      case argument_size::two_bytes:
         extra_bytes = 2;
         break;
      case argument_size::four_bytes:
         extra_bytes = 4;
         break;
      case argument_size::eight_bytes:
         extra_bytes = 8;
         break;
      case argument_size::reserved_0:
      case argument_size::reserved_1:
      case argument_size::reserved_2:
         return error::ill_formed;
      default:
         // If the encoded size doesn't match anything above, then it is most likely a simple type,
         // or something else, not containing any additional payload
         extra_bytes = 0;
         break;
   }

   std::array<std::byte, 8> argument_bytes{};
   res = buf.read(buffer::span_t{argument_bytes.data(), extra_bytes});
   if (res) {
      return res;
   }

   std::transform(std::begin(argument_bytes), std::begin(argument_bytes) + extra_bytes, std::begin(argument),
                  [](const auto b) { return static_cast<std::uint8_t>(b); });

   return error::success;
}

std::uint64_t head::decode_argument() const {
   // Arguments are encoded in big endian
   if (extra_bytes == 0) {
      return raw & 0x1F;
   }

   std::uint64_t result = 0U;
   for (unsigned i = 0; i < extra_bytes; ++i) {
      const std::uint64_t tmp = static_cast<std::uint8_t>(argument[i]);
      const unsigned offset = (extra_bytes - (i + 1)) * 8U;
      result |= (tmp << offset);
   }

   return result;
}

} // namespace cbor::detail

namespace cbor {

////////////////////////////////////////////////////////////////////////////////
/// Simple Types
////////////////////////////////////////////////////////////////////////////////
std::error_code decode(read_buffer &buf, bool &v) {
   detail::head head{};
   auto res = head.read(buf);
   if (res) {
      return res;
   }

   if (head.type != major_type::simple) {
      return error::unexpected_type;
   }

   switch (head.simple) {
      case simple_type::true_type:
         v = true;
         return error::success;

      case simple_type::false_type:
         v = false;
         return error::success;

      default:
         return error::unexpected_type;
   }
}

////////////////////////////////////////////////////////////////////////////////
/// Simple Types: floats
////////////////////////////////////////////////////////////////////////////////
namespace detail {

inline constexpr std::uint8_t operator""_u8(unsigned long long v) {
   if (v > std::numeric_limits<std::uint8_t>::max()) [[unlikely]] {
      // Dude, why?!
      std::terminate();
   }
   return static_cast<std::uint8_t>(v);
}

template <std::size_t Extent>
bool compare(const head &h, const std::array<std::uint8_t, Extent> &v) {
   if (v.size() != h.extra_bytes) [[unlikely]] {
      // Should never occur: we pick the decoding routine based on the number of extra bytes.
      return false;
   }

   auto hb = std::begin(h.argument);
   auto he = hb + h.extra_bytes;

   auto vb = std::begin(v);
   auto ve = std::end(v);

   auto m = std::mismatch(hb, he, vb, ve);
   return m.first == he && m.second == ve;
}

template <typename T>
std::error_code decode_hp(const detail::head &head, T &v) {
   static const std::array pos_inf = {0x7C_u8, 0x00_u8};
   static const std::array nan = {0x7E_u8, 0x00_u8};
   static const std::array neg_inf = {0xFC_u8, 0x00_u8};

   if (compare(head, pos_inf)) {
      v = std::numeric_limits<T>::infinity();
      return error::success;
   }

   if (compare(head, neg_inf)) {
      v = -std::numeric_limits<T>::infinity();
      return error::success;
   }

   if (compare(head, nan)) {
      v = std::numeric_limits<T>::quiet_NaN();
      return error::success;
   }

   const auto binary_argument = head.decode_argument();
   const auto binary_half = static_cast<std::uint16_t>(binary_argument);
   const auto half = fhf::unpack(binary_half);
   v = static_cast<T>(half);

   return error::success;
}

template <typename T>
std::error_code decode_sp(const detail::head &head, T &v) {
   static const std::array pos_inf = {0x7F_u8, 0x80_u8, 0x00_u8, 0x00_u8};
   static const std::array nan = {0x7F_u8, 0xC0_u8, 0x00_u8, 0x00_u8};
   static const std::array neg_inf = {0xFF_u8, 0x80_u8, 0x00_u8, 0x00_u8};

   if (compare(head, pos_inf)) {
      v = std::numeric_limits<T>::infinity();
      return error::success;
   }

   if (compare(head, neg_inf)) {
      v = -std::numeric_limits<T>::infinity();
      return error::success;
   }

   if (compare(head, nan)) {
      v = -std::numeric_limits<T>::quiet_NaN();
      return error::success;
   }

   const auto binary_argument = head.decode_argument();
   const auto binary_single = static_cast<std::uint32_t>(binary_argument);
   const auto single = std::bit_cast<float>(binary_single);
   v = static_cast<T>(single);

   return error::success;
}

template <typename T>
std::error_code decode_dp(const detail::head &head, T &v) {
   static const std::array pos_inf = {0x7F_u8, 0xF0_u8, 0x00_u8, 0x00_u8, 0x00_u8, 0x00_u8, 0x00_u8, 0x00_u8};
   static const std::array nan = {0x7F_u8, 0xF8_u8, 0x00_u8, 0x00_u8, 0x00_u8, 0x00_u8, 0x00_u8, 0x00_u8};
   static const std::array neg_inf = {0xFF_u8, 0xF0_u8, 0x00_u8, 0x00_u8, 0x00_u8, 0x00_u8, 0x00_u8, 0x00_u8};

   if (compare(head, pos_inf)) {
      v = std::numeric_limits<T>::infinity();
      return error::success;
   }

   if (compare(head, neg_inf)) {
      v = -std::numeric_limits<T>::infinity();
      return error::success;
   }

   if (compare(head, nan)) {
      v = -std::numeric_limits<T>::quiet_NaN();
      return error::success;
   }

   const auto binary_argument = head.decode_argument();
   const auto result = std::bit_cast<double>(binary_argument);

   const auto casted = static_cast<T>(result);
   if (casted != result) {
      // Down-casting looses precision
      return error::value_not_representable;
   }

   v = casted;
   return error::success;
}

template <std::floating_point T>
std::error_code decode_float(read_buffer &buf, T &v) {
   detail::head head{};
   auto res = head.read(buf);
   if (res) {
      return res;
   }

   if (head.type != major_type::simple) {
      return error::unexpected_type;
   }

   switch (head.simple) {
      case simple_type::hp_float:
         return decode_hp(head, v);
      case simple_type::sp_float:
         return decode_sp(head, v);
      case simple_type::dp_float:
         return decode_dp(head, v);
      default:
         return error::unexpected_type;
   }
}

} // namespace detail

std::error_code decode(read_buffer &buf, float &v) {
   return detail::decode_float(buf, v);
}

std::error_code decode(read_buffer &buf, double &v) {
   return detail::decode_float(buf, v);
}

} // namespace cbor
