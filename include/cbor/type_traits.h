/**
 * @file   type_traits.h
 * @author Dennis Sitelew 
 * @date   Feb 18, 2024
 */

#pragma once

#include <cstdint>
#include <limits>
#include <ranges>
#include <type_traits>

namespace cbor {

////////////////////////////////////////////////////////////////////////////////
/// Traits
////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline constexpr auto min_int_v = std::numeric_limits<T>::min();

template <typename T>
inline constexpr auto max_int_v = std::numeric_limits<T>::max();

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

template <typename Target, typename Source>
concept CopyableAs = std::is_same_v<Target, std::remove_cvref_t<Source>>;

template <typename Source>
concept CopyableAsU8 = CopyableAs<std::uint8_t, Source>;

template <typename Source>
concept CopyableAsChar = CopyableAs<char, Source>;

template <class T>
concept ConstByteArray = std::ranges::contiguous_range<T> && requires(const T &t) {
   { *std::cbegin(t) } -> CopyableAsU8;
   { std::size(t) } -> std::convertible_to<std::size_t>;
};

template <class T>
concept ByteArray = std::ranges::contiguous_range<T> && requires(T &t) {
   { *std::begin(t) } -> std::same_as<std::uint8_t &>;
   { std::size(t) } -> std::convertible_to<std::size_t>;
};

template <class T>
concept ConstTextArray = std::ranges::contiguous_range<T> && requires(T &t) {
   { *std::begin(t) } -> CopyableAsChar;
   { std::size(t) } -> std::convertible_to<std::size_t>;
};

template <class T>
concept TextArray = std::ranges::contiguous_range<T> && requires(T &t) {
   { *std::begin(t) } -> std::same_as<char &>;
   { std::size(t) } -> std::convertible_to<std::size_t>;
};

template <typename T>
concept WithTypeID = requires(T) {
   { type_id<std::remove_cvref_t<T>>::value } -> Int;
};

template <typename... T>
concept AllWithTypeID = (WithTypeID<T> && ...);

} // namespace cbor