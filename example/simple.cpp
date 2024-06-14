/**
 * @file   main.cpp
 * @author Dennis Sitelew 
 * @date   Jun 08, 2024
 */

#include <cbor/cbor.h>
#include <shp/shp.h>

#include <format>
#include <iostream>
#include <vector>

enum class kind {
   cat = 0,
   dog,
   hamster,
   fish,
};

struct pet {
   std::string name{};
   kind kind{};
};

// Plain (w/o the type ID) structures have to be whitelisted
[[maybe_unused]] consteval void enable_cbor_encoding(pet);

std::string_view to_string(kind k) {
   switch (k) {
      case kind::cat: return "cat";
      case kind::dog: return "dog";
      case kind::hamster: return "hamster";
      case kind::fish: return "fish";
      default: return "alien";
   }
}

std::string to_string(const pet &p) {
   return std::format("Pet {} named {}", to_string(p.kind), p.name);
}

int main(int, char **) {
   std::cout << "Simple CBOR example.\n";
   std::cout << "   Use https://cbor.me/ to check the serialization.\n\n";

   // Encoding
   std::vector<std::byte> target{};
   cbor::dynamic_buffer out{target};

   const std::vector<pet> input{
      {.name = "Bailey", .kind = kind::dog},
      {.name = "Whiskers", .kind = kind::cat},
      {.name = "Sushi", .kind = kind::fish},
      {.name = "Budweiser", .kind = kind::hamster},
   };

   if (auto res = cbor::encode(out, input)) {
      std::cerr << std::format("Encoding error: {}\n", res.message());
   }

   std::cout << "Encoded:\n" << shp::hex(target) << "\n\n";

   // Decoding
   cbor::read_buffer buf{{target}};
   std::vector<pet> decoded{};

   if (auto res = cbor::decode(buf, decoded)) {
      std::cerr << std::format("Decoding error: {}\n", res.message());
   }

   std::cout << "Decoded:\n";
   for (const auto &p : decoded) {
      std::cout << std::format("- {}\n", to_string(p));
   }

   return 0;
}