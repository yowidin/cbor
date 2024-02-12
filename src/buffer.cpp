/**
 * @file   buffer.cpp
 * @author Dennis Sitelew 
 * @date   Feb 05, 2024
 */

#include <cbor/buffer.h>
#include <cbor/config.h>

#include <algorithm>

using namespace cbor;
using namespace std;

////////////////////////////////////////////////////////////////////////////////
/// Class: dynamic_buffer
////////////////////////////////////////////////////////////////////////////////
dynamic_buffer::dynamic_buffer(vector_t &vec, std::size_t max_capacity)
   : vec_{&vec}
   , max_capacity_{max_capacity} {
   if constexpr (dynamic_buffer_initial_size != 0) {
      if (max_capacity_ != buffer::unlimited_capacity) {
         vec_->reserve(std::min(dynamic_buffer_initial_size, max_capacity));
      }
   }
}

std::error_code dynamic_buffer::write(const_span_t v) {
   const auto res = ensure_capacity(v.size());
   if (res) {
      return res;
   }

   vec_->insert(end(*vec_), begin(v), end(v));
   return error::success;
}

dynamic_buffer::rollback_token_t dynamic_buffer::begin_nested_write() {
   // Just keep track of the vector's size before writing
   return static_cast<std::ptrdiff_t>(vec_->size());
}

std::error_code dynamic_buffer::rollback_nested_write(rollback_token_t token) {
   // Rollback by resizing to the original size
   vec_->resize(token);
   return error::success;
}

std::error_code dynamic_buffer::ensure_capacity(std::size_t num_bytes) {
   const auto remainder = vec_->capacity() - vec_->size();
   if (remainder >= num_bytes) {
      return error::success;
   }

   const auto target_capacity = vec_->capacity() + num_bytes;
   if (max_capacity_ != buffer::unlimited_capacity && target_capacity > max_capacity_) {
      return error::buffer_overflow;
   }

   return error::success;
}

////////////////////////////////////////////////////////////////////////////////
/// Class: static_buffer
////////////////////////////////////////////////////////////////////////////////
static_buffer::static_buffer(span_t span)
   : span_{span} {
   // Nothing to do here
}

std::error_code static_buffer::write(const_span_t v) {
   const auto res = ensure_capacity(v.size());
   if (res) {
      return res;
   }

   copy(begin(v), end(v), begin(span_) + data_size_);
   data_size_ += static_cast<std::ptrdiff_t>(v.size());

   return error::success;
}

static_buffer::rollback_token_t static_buffer::begin_nested_write() {
   // Just keep track of the data size before writing
   return data_size_;
}

std::error_code static_buffer::rollback_nested_write(rollback_token_t token) {
   // Rollback by changing to the original data size
   data_size_ = token;
   return error::success;
}

std::error_code static_buffer::ensure_capacity(std::size_t num_bytes) {
   const auto remainder = span_.size() - data_size_;
   if (remainder >= num_bytes) {
      return error::success;
   }

   return error::buffer_overflow;
}
