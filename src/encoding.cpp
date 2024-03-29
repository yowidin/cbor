/**
 * @file   encoding.cpp
 * @author Dennis Sitelew
 * @date   Feb 13, 2024
 */

#include <cbor/encoding.h>

#include <fhf/fhf.hh>

#include <cmath>

#include <cstring>

namespace cbor {

namespace detail {

inline constexpr uint8_t ZERO_EXTRA_BYTES_VALUE_LIMIT = 23U;
inline constexpr uint16_t ONE_EXTRA_BYTE_VALUE_LIMIT = 0xFFU;
inline constexpr uint32_t TWO_EXTRA_BYTES_VALUE_LIMIT = 0xFFFFU;
inline constexpr uint64_t FOUR_EXTRA_BYTES_VALUE_LIMIT = 0xFFFFFFFFU;

inline std::uint8_t operator|(major_type m, argument_size s) {
   const auto lhs = static_cast<std::underlying_type_t<major_type>>(m);
   const auto rhs = static_cast<std::underlying_type_t<argument_size>>(s);
   return lhs | rhs;
}

inline std::uint8_t operator|(major_type m, simple_type s) {
   const auto lhs = static_cast<std::underlying_type_t<major_type>>(m);
   const auto rhs = static_cast<std::underlying_type_t<simple_type>>(s);
   return lhs | rhs;
}

std::error_code encode_argument(buffer &buf, major_type type, std::uint8_t v, bool compress) {
   if (compress && v <= ZERO_EXTRA_BYTES_VALUE_LIMIT) {
      // The argument's value is the value of the additional information
      const auto value = (type | argument_size::no_bytes) | v;
      return buf.write(value);
   }

   return buf.write({type | argument_size::one_byte, v});
}

std::error_code encode_argument(buffer &buf, major_type type, std::uint16_t v, bool compress) {
   if (compress && v <= ONE_EXTRA_BYTE_VALUE_LIMIT) {
      return encode_argument(buf, type, static_cast<std::uint8_t>(v), compress);
   }

   const auto b0 = static_cast<uint8_t>((v >> 8U) & 0xFFU);
   const auto b1 = static_cast<uint8_t>((v) & 0xFFU);

   return buf.write({type | argument_size::two_bytes, b0, b1});
}

std::error_code encode_argument(buffer &buf, major_type type, std::uint32_t v, bool compress) {
   if (compress && v <= ONE_EXTRA_BYTE_VALUE_LIMIT) {
      return encode_argument(buf, type, static_cast<std::uint8_t>(v), compress);
   }

   if (compress && v <= TWO_EXTRA_BYTES_VALUE_LIMIT) {
      return encode_argument(buf, type, static_cast<std::uint16_t>(v), compress);
   }

   const auto b0 = static_cast<uint8_t>((v >> 24U) & 0xFFU);
   const auto b1 = static_cast<uint8_t>((v >> 16U) & 0xFFU);
   const auto b2 = static_cast<uint8_t>((v >> 8U) & 0xFFU);
   const auto b3 = static_cast<uint8_t>((v) & 0xFFU);

   return buf.write({type | argument_size::four_bytes, b0, b1, b2, b3});
}

std::error_code encode_argument(buffer &buf, major_type type, std::uint64_t v, bool compress) {
   if (compress && v <= ONE_EXTRA_BYTE_VALUE_LIMIT) {
      return encode_argument(buf, type, static_cast<std::uint8_t>(v), compress);
   }

   if (compress && v <= TWO_EXTRA_BYTES_VALUE_LIMIT) {
      return encode_argument(buf, type, static_cast<std::uint16_t>(v), compress);
   }

   if (compress && v <= FOUR_EXTRA_BYTES_VALUE_LIMIT) {
      return encode_argument(buf, type, static_cast<std::uint32_t>(v), compress);
   }

   const auto b0 = static_cast<uint8_t>((v >> 56U) & 0xFFU);
   const auto b1 = static_cast<uint8_t>((v >> 48U) & 0xFFU);
   const auto b2 = static_cast<uint8_t>((v >> 40U) & 0xFFU);
   const auto b3 = static_cast<uint8_t>((v >> 32U) & 0xFFU);
   const auto b4 = static_cast<uint8_t>((v >> 24U) & 0xFFU);
   const auto b5 = static_cast<uint8_t>((v >> 16U) & 0xFFU);
   const auto b6 = static_cast<uint8_t>((v >> 8U) & 0xFFU);
   const auto b7 = static_cast<uint8_t>((v) & 0xFFU);

   return buf.write({type | argument_size::eight_bytes, b0, b1, b2, b3, b4, b5, b6, b7});
}

} // namespace detail

////////////////////////////////////////////////////////////////////////////////
/// Strings
////////////////////////////////////////////////////////////////////////////////
std::error_code encode(buffer &buf, const char *v) {
   const auto size = std::strlen(v);
   return encode(buf, std::string_view(v, size));
}

////////////////////////////////////////////////////////////////////////////////
/// Simple Types
////////////////////////////////////////////////////////////////////////////////
std::error_code encode(buffer &buf, bool v) {
   using namespace cbor::detail;
   return buf.write({major_type::simple | (v ? simple_type::true_type : simple_type::false_type)});
}

std::error_code encode(buffer &buf, std::nullptr_t) {
   using namespace cbor::detail;
   return buf.write({major_type::simple | simple_type::null_type});
}

////////////////////////////////////////////////////////////////////////////////
/// Simple Types: floats
////////////////////////////////////////////////////////////////////////////////
std::error_code encode(buffer &buf, float v) {
   using namespace cbor::detail;

   // Ensure matching encoding
   static_assert((int)argument_size::eight_bytes == (int)simple_type::dp_float);
   static_assert((int)argument_size::four_bytes == (int)simple_type::sp_float);
   static_assert((int)argument_size::two_bytes == (int)simple_type::hp_float);

   int value_type = std::fpclassify(v);
   switch (value_type) {
      case FP_NAN:
         // Always use deterministic encoding - the smallest possible float for NAN
         return buf.write({major_type::simple | simple_type::hp_float, 0x7E, 0x00});
      case FP_INFINITE:
         // Always use deterministic encoding - the smallest possible float for INF and -INF
         if (v > 0) {
            return buf.write({major_type::simple | simple_type::hp_float, 0x7C, 0x00});
         } else {
            return buf.write({major_type::simple | simple_type::hp_float, 0xFC, 0x00});
         }
      default: {
         // Floats require all bytes to be present (no compression is allowed), thus the last argument to
         // encode_argument is always false
         const auto binary_half = fhf::pack(v);
         const auto half = fhf::unpack(binary_half);
         if (half == v) {
            return detail::encode_argument(buf, major_type::simple, binary_half, false);
         }

         return detail::encode_argument(buf, major_type::simple, std::bit_cast<std::uint32_t>(v), false);
      }
   }
}

std::error_code encode(buffer &buf, double v) {
   using namespace cbor::detail;

   // Ensure matching encoding
   static_assert((int)argument_size::eight_bytes == (int)simple_type::dp_float);
   static_assert((int)argument_size::four_bytes == (int)simple_type::sp_float);
   static_assert((int)argument_size::two_bytes == (int)simple_type::hp_float);

   int value_type = std::fpclassify(v);
   switch (value_type) {
      case FP_NAN:
         // Always use deterministic encoding - the smallest possible float for NAN
         return buf.write({major_type::simple | simple_type::hp_float, 0x7E, 0x00});
      case FP_INFINITE:
         // Always use deterministic encoding - the smallest possible float for INF and -INF
         if (v > 0) {
            return buf.write({major_type::simple | simple_type::hp_float, 0x7C, 0x00});
         } else {
            return buf.write({major_type::simple | simple_type::hp_float, 0xFC, 0x00});
         }
      default: {
         // Floats require all bytes to be present (no compression is allowed), thus the last argument to
         // encode_argument is always false
         const auto single = static_cast<float>(v);
         if (single == v) {
            // Double can be encoded as float, let's also check for half-float
            const auto binary_half = fhf::pack(single);
            const auto half = fhf::unpack(binary_half);
            if (half == single) {
               return detail::encode_argument(buf, major_type::simple, binary_half, false);
            }

            return detail::encode_argument(buf, major_type::simple, std::bit_cast<std::uint32_t>(single), false);
         }

         return detail::encode_argument(buf, major_type::simple, std::bit_cast<std::uint64_t>(v), false);
      }
   }
}

} // namespace cbor
