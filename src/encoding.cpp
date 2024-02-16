/**
 * @file   encoding.cpp
 * @author Dennis Sitelew
 * @date   Feb 13, 2024
 */

#include <cbor/encoding.h>

namespace cbor::detail {

inline constexpr uint8_t ZERO_EXTRA_BYTES_VALUE_LIMIT = 23U;
inline constexpr uint16_t ONE_EXTRA_BYTE_VALUE_LIMIT = 0xFFU;
inline constexpr uint32_t TWO_EXTRA_BYTES_VALUE_LIMIT = 0xFFFFU;
inline constexpr uint64_t FOUR_EXTRA_BYTES_VALUE_LIMIT = 0xFFFFFFFFU;

inline std::uint8_t operator|(major_type m, argument_size s) {
   const auto lhs = static_cast<std::underlying_type_t<major_type>>(m);
   const auto rhs = static_cast<std::underlying_type_t<argument_size>>(s);
   return lhs | rhs;
}

std::error_code encode_argument(buffer &buf, major_type type, std::uint8_t v) {
   if (v <= ZERO_EXTRA_BYTES_VALUE_LIMIT) {
      // The argument's value is the value of the additional information
      const auto value = (type | argument_size::no_bytes) | v;
      return buf.write(value);
   }

   return buf.write({type | argument_size::one_byte, v});
}

std::error_code encode_argument(buffer &buf, major_type type, std::uint16_t v) {
   if (v <= ONE_EXTRA_BYTE_VALUE_LIMIT) {
      return encode_argument(buf, type, static_cast<std::uint8_t>(v));
   }

   const auto b0 = static_cast<uint8_t>((v >> 8U) & 0xFFU);
   const auto b1 = static_cast<uint8_t>((v) & 0xFFU);

   return buf.write({type | argument_size::two_bytes, b0, b1});
}

std::error_code encode_argument(buffer &buf, major_type type, std::uint32_t v) {
   if (v <= ONE_EXTRA_BYTE_VALUE_LIMIT) {
      return encode_argument(buf, type, static_cast<std::uint8_t>(v));
   }

   if (v <= TWO_EXTRA_BYTES_VALUE_LIMIT) {
      return encode_argument(buf, type, static_cast<std::uint16_t>(v));
   }

   const auto b0 = static_cast<uint8_t>((v >> 24U) & 0xFFU);
   const auto b1 = static_cast<uint8_t>((v >> 16U) & 0xFFU);
   const auto b2 = static_cast<uint8_t>((v >> 8U) & 0xFFU);
   const auto b3 = static_cast<uint8_t>((v) & 0xFFU);

   return buf.write({type | argument_size::four_bytes, b0, b1, b2, b3});
}

std::error_code encode_argument(buffer &buf, major_type type, std::uint64_t v) {
   if (v <= ONE_EXTRA_BYTE_VALUE_LIMIT) {
      return encode_argument(buf, type, static_cast<std::uint8_t>(v));
   }

   if (v <= TWO_EXTRA_BYTES_VALUE_LIMIT) {
      return encode_argument(buf, type, static_cast<std::uint16_t>(v));
   }

   if (v <= FOUR_EXTRA_BYTES_VALUE_LIMIT) {
      return encode_argument(buf, type, static_cast<std::uint32_t>(v));
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

} // namespace cbor
