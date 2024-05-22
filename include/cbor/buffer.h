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

////////////////////////////////////////////////////////////////////////////////
/// Class: buffer
////////////////////////////////////////////////////////////////////////////////
/**
 * Base buffer class, provides functions for low-level buffer manipulations.
 */
class CBOR_EXPORT buffer {
public:
   using rollback_token_t = std::ptrdiff_t;
   using span_t = std::span<std::byte>;
   using const_span_t = std::span<const std::byte>;

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
   [[nodiscard]] virtual std::error_code write(std::byte v) { return write({v}); }

   [[nodiscard]] virtual std::error_code write(std::initializer_list<std::byte> v) {
      return write(const_span_t{v.begin(), v.end()});
   }

   [[nodiscard]] virtual std::error_code write(const_span_t v) = 0;
   [[nodiscard]] virtual std::size_t size() = 0;

   [[nodiscard]] rollback_helper get_rollback_helper() { return rollback_helper(*this); }

protected:
   [[nodiscard]] virtual rollback_token_t begin_nested_write() = 0;
   virtual void rollback_nested_write(rollback_token_t token) = 0;
};

////////////////////////////////////////////////////////////////////////////////
/// Class: dynamic_buffer
////////////////////////////////////////////////////////////////////////////////
/**
 * Dynamic buffer - buffer type that can grow up to an optional limit.
 */
class CBOR_EXPORT dynamic_buffer final : public buffer {
public:
   using vector_t = std::vector<std::byte>;

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

////////////////////////////////////////////////////////////////////////////////
/// Class: static_buffer
////////////////////////////////////////////////////////////////////////////////
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

////////////////////////////////////////////////////////////////////////////////
/// Class: read_buffer
////////////////////////////////////////////////////////////////////////////////
/**
 * Read buffer - container holding a decode buffer.
 */
class CBOR_EXPORT read_buffer final {
public:
   class rollback_helper {
   public:
      explicit rollback_helper(read_buffer &b)
         : buf_{&b}
         , start_position_{b.read_position()} {
         // Nothing to do here
      }

      ~rollback_helper() {
         if (buf_ && rollback_) {
            buf_->reset(start_position_);
         }
      }

      rollback_helper(rollback_helper &) = delete;
      rollback_helper(rollback_helper &&o) noexcept
         : buf_{o.buf_}
         , start_position_{o.start_position_}
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
         start_position_ = o.start_position_;
         rollback_ = o.rollback_;

         o.buf_ = nullptr;
         o.rollback_ = false;

         return *this;
      }

   private:
      read_buffer *buf_;
      std::ptrdiff_t start_position_; //! Read position to rollback to (in absence of a commit)
      bool rollback_{true};
   };

public:
   read_buffer(buffer::const_span_t span);

   read_buffer(const read_buffer &) = delete;
   read_buffer(read_buffer &&) = default;

public:
   read_buffer &operator=(const read_buffer &) = delete;
   read_buffer &operator=(read_buffer &&) = default;

public:
   [[nodiscard]] std::error_code read(std::byte &v);
   [[nodiscard]] std::error_code read(buffer::span_t v);

   [[nodiscard]] std::ptrdiff_t read_position() const { return read_position_; }
   void reset(std::ptrdiff_t position = 0) { read_position_ = position; }

   [[nodiscard]] rollback_helper get_rollback_helper() { return rollback_helper(*this); }

private:
   buffer::const_span_t span_;
   std::ptrdiff_t read_position_{0};
};

} // namespace cbor