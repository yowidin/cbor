/**
 * @file   type_traits.h
 * @author Dennis Sitelew 
 * @date   Feb 18, 2024
 */

#pragma once

#include <cbor/config.h>

#include <cstdint>
#include <limits>
#include <ranges>
#include <type_traits>

#if CBOR_WITH(BOOST_PFR)
#include <boost/pfr.hpp>
#endif

namespace cbor {

////////////////////////////////////////////////////////////////////////////////
/// Traits
////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline constexpr auto min_int_v = std::numeric_limits<T>::min();

template <typename T>
inline constexpr auto max_int_v = std::numeric_limits<T>::max();

template <typename T>
using max_size_t = typename std::remove_cvref_t<T>::size_type;

template <typename T>
inline constexpr auto max_size_v = max_int_v<max_size_t<T>>;

template <typename T>
using value_type_t = typename std::remove_cvref_t<T>::value_type;

template <typename T>
using key_type_t = typename std::remove_cvref_t<T>::key_type;

template <typename T>
using mapped_type_t = typename std::remove_cvref_t<T>::mapped_type;

template <typename T>
using iterator_t = typename std::remove_cvref_t<T>::iterator;

/**
 * Type ID trait.
 *
 * When specialized for a type provides mapping from that type to an integral constant, identifying that type.
 * @example
 * @code{.cpp}
 * struct foo {
 *    int bar;
 * };
 *
 * namespace cbor {
 * template &lt;&gt;
 * struct type_id&lt;foo&gt;
 *    using value = std::integral_constant&lt;int, 0xBEEF&gt;;
 * };
 * @endcode
 */
template <typename T>
struct type_id;

template <typename T>
inline constexpr auto type_id_v = type_id<std::remove_cvref_t<T>>::value;

////////////////////////////////////////////////////////////////////////////////
/// Concepts
////////////////////////////////////////////////////////////////////////////////

template <typename T>
struct is_bool : std::bool_constant<false> {};

template <>
struct is_bool<bool> : std::bool_constant<true> {};

template <typename T>
using is_bool_t = typename is_bool<std::remove_cvref_t<T>>::type;

template <typename T>
inline constexpr bool is_bool_v = is_bool_t<std::remove_cvref_t<T>>::value;

template <typename T>
concept IsBool = is_bool_v<T>;

template <typename T>
concept UnsignedInt =
   std::is_unsigned_v<std::remove_cvref_t<T>> && std::is_integral_v<std::remove_cvref_t<T>> && !is_bool_v<T>;

template <typename T>
concept SignedInt = std::is_signed_v<std::remove_cvref_t<T>> && std::is_integral_v<std::remove_cvref_t<T>>;

template <typename T>
concept Int = UnsignedInt<T> || SignedInt<T>;

template <typename T>
concept Enum = std::is_enum_v<T>;

template <typename T>
struct is_byte : std::bool_constant<false> {};

template <>
struct is_byte<std::byte> : std::bool_constant<true> {};

template <typename T>
using is_byte_t = typename is_byte<std::remove_cvref_t<T>>::type;

template <typename T>
inline constexpr bool is_byte_v = is_byte_t<std::remove_cvref_t<T>>::value;

template <typename T>
concept IsByte = is_byte_v<T>;

template <typename T>
concept WithTypeID = requires(T) {
   { type_id<std::remove_cvref_t<T>>::value } -> Int;
};

template <typename... T>
concept AllWithTypeID = (WithTypeID<T> && ...);

template <typename T>
concept AssociativeContainer = std::is_same_v<value_type_t<T>, std::pair<const key_type_t<T>, mapped_type_t<T>>>;

template <typename T>
concept Dictionary = AssociativeContainer<T> && requires(T &v) {
   { v.insert(std::declval<value_type_t<T> &&>()) };
};

////////////////////////////////////////////////////////////////////////////////
/// Structs
////////////////////////////////////////////////////////////////////////////////
//! Treat structs with either a type_id specialization, or explicitly enabled CBOR encoding as whitelisted.
template <typename T>
concept WhitelistedStruct = std::is_class_v<T> && (WithTypeID<T> || requires(T e) { enable_cbor_encoding(e); });

#if CBOR_WITH(BOOST_PFR)
template <WhitelistedStruct T>
[[nodiscard]] consteval std::size_t get_member_count() {
   return boost::pfr::tuple_size_v<T>;
}

template <std::size_t Idx, WhitelistedStruct T>
[[nodiscard]] const auto &get_member(const T &v) {
   return boost::pfr::get<Idx>(v);
}

template <std::size_t Idx, WhitelistedStruct T>
[[nodiscard]] auto &get_member_non_const(T &v) {
   return boost::pfr::get<Idx>(v);
}
#else
template <WhitelistedStruct T>
[[nodiscard]] consteval std::size_t get_member_count();

template <std::size_t Idx, WhitelistedStruct T>
[[nodiscard]] const auto &get_member(const T &v);

template <std::size_t Idx, WhitelistedStruct T>
[[nodiscard]] auto &get_member_non_const(T &v);
#endif // CBOR_WITH(BOOST_PFR)

template <typename T>
concept EncodableStruct = WhitelistedStruct<T> && requires(const T &t) {
   { get_member_count<T>() } -> std::same_as<std::size_t>;
   { get_member<0>(t) };
};

template <typename T>
concept DecodableStruct = WhitelistedStruct<T> && requires(T &t) {
   { get_member_count<T>() } -> std::same_as<std::size_t>;
   { get_member_non_const<0>(t) };
};

} // namespace cbor