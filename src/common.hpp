#pragma once

// Common headers

#include "config.h"
#include "logging.hpp"

#include <cassert>
#include <cinttypes>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>

#include <array>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

/***************************** Helper types **********************************/

template <class ContainerType>
concept Container = requires(ContainerType type, const ContainerType ctype) {
    requires std::regular<ContainerType>;
    requires std::swappable<ContainerType>;
    requires std::destructible<typename ContainerType::value_type>;
    requires std::
        same_as<typename ContainerType::reference, typename ContainerType::value_type&>;
    requires std::same_as<
        typename ContainerType::const_reference,
        const typename ContainerType::value_type&>;
    requires std::forward_iterator<typename ContainerType::iterator>;
    requires std::forward_iterator<typename ContainerType::const_iterator>;
    requires std::signed_integral<typename ContainerType::difference_type>;
    requires std::same_as<
        typename ContainerType::difference_type,
        typename std::iterator_traits<
            typename ContainerType::iterator>::difference_type>;
    requires std::same_as<
        typename ContainerType::difference_type,
        typename std::iterator_traits<
            typename ContainerType::const_iterator>::difference_type>;
    {
        type.begin()
    } -> std::same_as<typename ContainerType::iterator>;

    {
        type.end()
    } -> std::same_as<typename ContainerType::iterator>;

    {
        ctype.begin()
    } -> std::same_as<typename ContainerType::const_iterator>;

    {
        ctype.end()
    } -> std::same_as<typename ContainerType::const_iterator>;

    {
        type.cbegin()
    } -> std::same_as<typename ContainerType::const_iterator>;

    {
        type.cend()
    } -> std::same_as<typename ContainerType::const_iterator>;

    {
        type.size()
    } -> std::same_as<typename ContainerType::size_type>;

    {
        type.max_size()
    } -> std::same_as<typename ContainerType::size_type>;

    {
        type.empty()
    } -> std::same_as<bool>;
};

/***************************** Helper macros *********************************/
// NOLINTBEGIN(*-macro-usage)

/** Mark a variable as unused. */
#define UNUSED(_var) (void)(_var)

/** Get the size of a stack-allocated array. */
#define ARR_SIZE(_arr) ((size_t)(sizeof(_arr) / sizeof((_arr)[0])))

/**
 * Get the container of a pointer to a member.
 */
template <class T, class M>
static inline constexpr ptrdiff_t
offset_of_impl(const M T::*member)
{
    return reinterpret_cast<ptrdiff_t>(&(reinterpret_cast<T*>(0)->*member));
}

template <class T, class M>
static inline constexpr T*
container_of_impl(M* ptr, const M T::*member)
{
    return reinterpret_cast<T*>(
        reinterpret_cast<intptr_t>(ptr) - offset_of_impl(member)
    );
}

#define OFFSET_OF(_type, _member)          offset_of_impl(&_type::_member)
#define CONTAINER_OF(_ptr, _type, _member) container_of_impl(_ptr, &_type::_member)

/* Cache alignment. */
#ifdef _MSC_VER
#  define ALIGN(n) __declspec(align(n))
#elif defined(__GNUC__) || defined(__clang__)
#  define ALIGN(n) __attribute__((aligned(n)))
#else
#  warning "Unknown compiler, aligned attributes will not work"
#  define ALIGN(n)
#endif

#define CACHE_ALIGN ALIGN(64)

// NOLINTEND(*-macro-usage)
