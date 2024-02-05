/**
 * @file   buffer.h
 * @author Dennis Sitelew 
 * @date   Feb 05, 2024
 */

#pragma once

#include <cbor/error.h>

#include <array>
#include <cinttypes>
#include <span>
#include <vector>

namespace cbor {

////////////////////////////////////////////////////////////////////////////////
/// Class: buffer
////////////////////////////////////////////////////////////////////////////////
class buffer {
public:
   using rollback_token_t = std::size_t;
   using span_t = std::span<std::uint8_t>;
   using const_span_t = std::span<const std::uint8_t>;

public:
   inline static constexpr std::size_t unlimited_capacity = -1;

public:
   virtual ~buffer() = default;

public:
   [[nodiscard]] virtual std::error_code write(std::uint8_t v) { return write({v}); }

   [[nodiscard]] virtual std::error_code write(std::initializer_list<std::uint8_t> v) {
      return write(const_span_t{v.begin(), v.end()});
   }

   [[nodiscard]] virtual std::error_code write(const_span_t v) = 0;

   [[nodiscard]] virtual rollback_token_t begin_nested_write() = 0;
   [[nodiscard]] virtual std::error_code rollback_nested_write(rollback_token_t token) = 0;
};

////////////////////////////////////////////////////////////////////////////////
/// Class: dynamic_buffer
////////////////////////////////////////////////////////////////////////////////
class dynamic_buffer final : public buffer {
public:
   using vector_t = std::vector<std::uint8_t>;

public:
   explicit dynamic_buffer(vector_t &vec, std::size_t max_capacity = buffer::unlimited_capacity);

   dynamic_buffer(const dynamic_buffer &) = delete;
   dynamic_buffer(dynamic_buffer &&) = default;

public:
   dynamic_buffer &operator=(const dynamic_buffer &) = delete;
   dynamic_buffer &operator=(dynamic_buffer &&) = default;

public:
   using buffer::write;

   [[nodiscard]] std::error_code write(const_span_t v) override;

   [[nodiscard]] rollback_token_t begin_nested_write() override;
   [[nodiscard]] std::error_code rollback_nested_write(rollback_token_t token) override;

private:
   [[nodiscard]] std::error_code ensure_capacity(std::size_t num_bytes);

private:
   vector_t *vec_;
   std::size_t max_capacity_;
};

} // namespace cbor