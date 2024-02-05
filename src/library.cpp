#include <cbor/library.h>

#include <iostream>

std::error_code cbor::hello() {
    std::cout << "Hello, World!" << std::endl;
    return cbor::error::success;
}
