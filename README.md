# CBOR C++ Library ![status](https://github.com/yowidin/cbor/actions/workflows/main.yml/badge.svg)

`cbor` is a C++20 library designed to encode and decode CBOR (Concise Binary Object Representation) sequences.
The library optionally supports reflection via [Boost PFR](https://github.com/boostorg/pfr).

## Usage

The recommended way to include this library in your project is to use the CMake's `FetchContent` functionality, for
example:

```cmake
cmake_minimum_required(VERSION 3.16)
project(cbor_example)

set(CMAKE_CXX_STANDARD 20)

include(FetchContent)

# Fetch the CBOR library
FetchContent_Declare(
    cbor
    GIT_REPOSITORY https://github.com/yowidin/cbor
    GIT_TAG master
)
FetchContent_MakeAvailable(cbor)

# Also fetch the Simple Hex Printer Library for easier printing (not required).
FetchContent_Declare(
    shp
    GIT_REPOSITORY https://github.com/yowidin/simple-hex-printer
    GIT_TAG master
)
FetchContent_MakeAvailable(shp)

add_executable(example main.cpp)
target_link_libraries(example PRIVATE CBOR::library SimpleHexPrinter::library)
```

And then in a C++ file:

```cpp
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
```

Output:

```
Simple CBOR example.
   Use https://cbor.me/ to check the serialization.

Encoded:
0x00: 84 82 66 42 61 69 6C 65 79 01 82 68 57 68 69 73  ..fBailey..hWhis
0x10: 6B 65 72 73 00 82 65 53 75 73 68 69 03 82 69 42  kers..eSushi..iB
0x20: 75 64 77 65 69 73 65 72 02                       udweiser.

Decoded:
- Pet dog named Bailey
- Pet cat named Whiskers
- Pet fish named Sushi
- Pet hamster named Budweiser
```

For more examples, please refer to the [example](./example) directory in the repository.

## License

This project is licensed under the MIT License. See the [LICENSE.txt](./LICENSE.txt) file for details.

## Feedback

Feel free to provide feedback or ask questions by opening an issue on the repository.