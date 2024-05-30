/**
 * @file   floats.cpp
 * @author Dennis Sitelew 
 * @date   May 26, 2024
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/catch_template_test_macros.hpp>

#include <test/decoding.h>

#include <cbor/decoding.h>

#include <cmath>

using namespace test;

using span_t = cbor::buffer::const_span_t;

TEST_CASE("Floats - basic decoding", "[decoding, floats]") {
   expect({0xF9, 0x00, 0x00}, 0.0f);
   expect({0xF9, 0x00, 0x00}, 0.0);

   expect({0xF9, 0x3C, 0x00}, 1.0f);
   expect({0xF9, 0x3C, 0x00}, 1.0);

   expect({0xFA, 0x3F, 0x8C, 0xCC, 0xCD}, 1.1f);
   expect({0xFB, 0x3F, 0xF1, 0x99, 0x99, 0x99, 0x99, 0x99, 0x9A}, 1.1);

   expect({0xF9, 0x3E, 0x00}, 1.5f);
   expect({0xF9, 0x3E, 0x00}, 1.5);

   expect({0xF9, 0x7B, 0xFF}, 65504.0f);
   expect({0xF9, 0x7B, 0xFF}, 65504.0);

   expect({0xFA, 0x47, 0xC3, 0x50, 0x00}, 100000.0f);
   expect({0xFA, 0x47, 0xC3, 0x50, 0x00}, 100000.0);

   expect({0xFA, 0x7F, 0x7F, 0xFF, 0xFF}, 3.4028234663852886e+38f);
   expect({0xFA, 0x7F, 0x7F, 0xFF, 0xFF}, 3.4028234663852886e+38);

   expect({0xFB, 0x7E, 0x37, 0xE4, 0x3C, 0x88, 0x00, 0x75, 0x9C}, 1.0e+300);

   expect({0xF9, 0x00, 0x01}, 5.960464477539063e-8f);
   expect({0xF9, 0x00, 0x01}, 5.960464477539063e-8);

   expect({0xF9, 0x04, 0x00}, 0.00006103515625f);
   expect({0xF9, 0x04, 0x00}, 0.00006103515625);

   expect({0xF9, 0xC4, 0x00}, -4.0f);
   expect({0xF9, 0xC4, 0x00}, -4.0);

   expect({0xFA, 0xC0, 0x83, 0x33, 0x33}, -4.1f);
   expect({0xFB, 0xC0, 0x10, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66}, -4.1);
}

enum class float_class { nan, neg_inf, pos_inf, regular };

template <std::floating_point T>
float_class to_class(T v) {
   auto cls = std::fpclassify(v);
   switch (cls) {
      case FP_NAN:
         return float_class::nan;
      case FP_INFINITE:
         if (v > 0) {
            return float_class::pos_inf;
         } else {
            return float_class::neg_inf;
         }
      default:
         return float_class::regular;
   }
}

template <std::floating_point T>
void expect_class(std::initializer_list<std::uint8_t> cbor, float_class fc) {
   T decoded;
   decode(cbor, decoded);

   auto value_type = to_class(decoded);
   REQUIRE(value_type == fc);
}

TEST_CASE("Floats - NaN decoding", "[decoding, floats, nan]") {
   // Sanity check
   expect_class<float>({0xF9, 0xC4, 0x00}, float_class::regular);
   expect_class<double>({0xF9, 0xC4, 0x00}, float_class::regular);

   // Float
   expect_class<float>({0xF9, 0x7C, 0x00}, float_class::pos_inf);
   expect_class<float>({0xFA, 0x7F, 0x80, 0x00, 0x00}, float_class::pos_inf);
   expect_class<float>({0xFB, 0x7F, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, float_class::pos_inf);

   expect_class<float>({0xF9, 0x7E, 0x00}, float_class::nan);
   expect_class<float>({0xFA, 0x7F, 0xC0, 0x00, 0x00}, float_class::nan);
   expect_class<float>({0xFB, 0x7F, 0xF8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, float_class::nan);

   expect_class<float>({0xF9, 0xFC, 0x00}, float_class::neg_inf);
   expect_class<float>({0xFA, 0xFF, 0x80, 0x00, 0x00}, float_class::neg_inf);
   expect_class<float>({0xFB, 0xFF, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, float_class::neg_inf);

   // Double
   expect_class<double>({0xF9, 0x7C, 0x00}, float_class::pos_inf);
   expect_class<double>({0xFA, 0x7F, 0x80, 0x00, 0x00}, float_class::pos_inf);
   expect_class<double>({0xFB, 0x7F, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, float_class::pos_inf);

   expect_class<double>({0xF9, 0x7E, 0x00}, float_class::nan);
   expect_class<double>({0xFA, 0x7F, 0xC0, 0x00, 0x00}, float_class::nan);
   expect_class<double>({0xFB, 0x7F, 0xF8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, float_class::nan);

   expect_class<double>({0xF9, 0xFC, 0x00}, float_class::neg_inf);
   expect_class<double>({0xFA, 0xFF, 0x80, 0x00, 0x00}, float_class::neg_inf);
   expect_class<double>({0xFB, 0xFF, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, float_class::neg_inf);
}

TEMPLATE_TEST_CASE("Floats - decoding errors", "[decoding, floats, errors]", float, double) {
   SECTION("Not enough data to read head") {
      std::array<std::byte, 2> source{};
      cbor::read_buffer buf{span_t{source.data(), 0}};

      TestType v;
      REQUIRE(cbor::decode(buf, v) == cbor::error::buffer_underflow);
   }

   SECTION("Unexpected major type") {
      std::array source{0x79_b, 0x3E_b, 0xE8_b};
      cbor::read_buffer buf{span_t{source}};

      TestType v;
      REQUIRE(cbor::decode(buf, v) == cbor::error::unexpected_type);
   }

   SECTION("Unexpected simple type") {
      std::array source{0xF4_b};
      cbor::read_buffer buf{span_t{source}};

      TestType v;
      REQUIRE(cbor::decode(buf, v) == cbor::error::unexpected_type);
   }
}

TEST_CASE("Floats - precision loss", "[decoding, floats, errors]") {
   // 1.0e+300 is not representable as a float
   std::array source{0xFB_b, 0x7E_b, 0x37_b, 0xE4_b, 0x3C_b, 0x88_b, 0x00_b, 0x75_b, 0x9C_b};
   cbor::read_buffer buf{span_t{source}};

   float v;
   REQUIRE(cbor::decode(buf, v) == cbor::error::value_not_representable);
}
