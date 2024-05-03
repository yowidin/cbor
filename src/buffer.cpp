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

void dynamic_buffer::rollback_nested_write(rollback_token_t token) {
   // Rollback by resizing to the original size
   vec_->resize(token);
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

void static_buffer::rollback_nested_write(rollback_token_t token) {
   // Rollback by changing to the original data size
   data_size_ = token;
}

std::error_code static_buffer::ensure_capacity(std::size_t num_bytes) {
   const auto remainder = span_.size() - data_size_;
   if (remainder >= num_bytes) {
      return error::success;
   }

   return error::buffer_overflow;
}

////////////////////////////////////////////////////////////////////////////////
/// Class: read_buffer
////////////////////////////////////////////////////////////////////////////////
read_buffer::read_buffer(buffer::const_span_t span)
   : span_{span} {
   // Nothing to do here
}

std::error_code read_buffer::read(std::byte &v) {
   if (!span_.data()) {
      return error::invalid_usage;
   }

   if (span_.size() - read_position_ <= 0) {
      return error::buffer_underflow;
   }

   v = span_[read_position_++];

   return error::success;
}

std::error_code read_buffer::read(buffer::span_t v) {
   if (!span_.data()) {
      return error::invalid_usage;
   }

   if (!v.data()) {
      return error::invalid_usage;
   }

   if (span_.size() - read_position_ < v.size()) {
      return error::buffer_underflow;
   }

   using diff_t = std::iterator_traits<buffer::const_span_t::iterator>::difference_type;
   auto begin = span_.begin() + read_position_;
   auto end = begin + static_cast<diff_t>(v.size());
   std::copy(begin, end, v.begin());

   read_position_ += static_cast<decltype(read_position_)>(v.size());

   return error::success;
}
