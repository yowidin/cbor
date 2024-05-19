/**
 * @file   decoding.cpp
 * @author Dennis Sitelew 
 * @date   May 02, 2024
 */

#include <cbor/decoding.h>

#include <array>

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
