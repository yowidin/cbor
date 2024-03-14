/**
 * @file   buffer.h
 * @author Dennis Sitelew 
 * @date   Feb 05, 2024
 */

#pragma once

#include <cbor/error.h>
#include <cbor/export.h>

#include <cstdint>
#include <span>
#include <vector>

namespace cbor {

/**
 * Base buffer class, provides functions for low-level buffer manipulations.
 */
class CBOR_EXPORT buffer {
public:
   using rollback_token_t = std::ptrdiff_t;
   using span_t = std::span<std::uint8_t>;
   using const_span_t = std::span<const std::uint8_t>;

   class rollback_helper {
   public:
      explicit rollback_helper(buffer &b)
         : buf_{&b}
         , token_{b.begin_nested_write()} {
         // Nothing to do here
      }

      ~rollback_helper() {
         if (buf_ && rollback_) {
            buf_->rollback_nested_write(token_);
         }
      }

      rollback_helper(rollback_helper &) = delete;
      rollback_helper(rollback_helper &&o) noexcept
         : buf_{o.buf_}
         , token_{o.token_}
         , rollback_{o.rollback_} {
         o.buf_ = nullptr;
         o.rollback_ = false;
      }

   public:
      void commit() { rollback_ = false; }

   public:
      rollback_helper &operator=(rollback_helper &) = delete;
      rollback_helper &operator=(rollback_helper &&o) noexcept {
         buf_ = o.buf_;
         token_ = o.token_;
         rollback_ = o.rollback_;

         o.buf_ = nullptr;
         o.rollback_ = false;

         return *this;
      }

   private:
      buffer *buf_;
      rollback_token_t token_;
      bool rollback_{true};
   };

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
   [[nodiscard]] virtual std::size_t size() = 0;

   rollback_helper get_rollback_helper() { return rollback_helper(*this); }

protected:
   [[nodiscard]] virtual rollback_token_t begin_nested_write() = 0;
   virtual void rollback_nested_write(rollback_token_t token) = 0;
};

/**
 * Dynamic buffer - buffer type that can grow up to an optional limit.
 */
class CBOR_EXPORT dynamic_buffer final : public buffer {
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
   [[nodiscard]] std::size_t size() override { return vec_->size(); };

protected:
   [[nodiscard]] rollback_token_t begin_nested_write() override;
   void rollback_nested_write(rollback_token_t token) override;

private:
   [[nodiscard]] std::error_code ensure_capacity(std::size_t num_bytes);

private:
   vector_t *vec_;
   std::size_t max_capacity_;
};

/**
 * Static buffer - buffer with a fixed size.
 */
class CBOR_EXPORT static_buffer final : public buffer {
public:
   static_buffer(span_t span);

   static_buffer(const static_buffer &) = delete;
   static_buffer(static_buffer &&) = default;

public:
   static_buffer &operator=(const static_buffer &) = delete;
   static_buffer &operator=(static_buffer &&) = default;

public:
   using buffer::write;

   [[nodiscard]] std::error_code write(const_span_t v) override;
   [[nodiscard]] std::size_t size() override { return data_size_; };

protected:
   [[nodiscard]] rollback_token_t begin_nested_write() override;
   void rollback_nested_write(rollback_token_t token) override;

private:
   [[nodiscard]] std::error_code ensure_capacity(std::size_t num_bytes);

private:
   span_t span_;
   std::ptrdiff_t data_size_{0};
};

} // namespace cbor