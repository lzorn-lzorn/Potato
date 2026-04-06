#pragma once

#include "Core/Math/Vector.h"
#include "Core/CoreImpl.h"

#include <utility>
#include <memory>
#include <string>
#include <vector>
#include <array>
#include <type_traits>
#include <concepts>
#include <cmath>
#include <numeric>
#include <algorithm>
#include <unordered_map>

namespace Core
{
template <typename FunctionType, size_t Index>
concept InvocableWithIndex = requires(FunctionType &&func) 
{
    { func(std::integral_constant<size_t, Index>{}) } -> std::same_as<void>;
};

/**
 * @berif 无 break 的计数循环 (0 .. N-1)
 * StaticForBreak<5>([](auto i) -> bool {
 *      constexpr size_t idx = decltype(i)::value;
 *      SDL_Log("  处理索引 %zu\n", idx);
 *      // 遇到偶数就停止（返回 false）
 *      return (idx % 2 != 0);
 * });
 */
template <size_t N, typename FunctionType>
    requires (N == 0 || InvocableWithIndex<FunctionType, 0>)
constexpr void StaticFor(FunctionType &&func) 
{
    [&]<size_t... Is>(std::index_sequence<Is...>) 
    {
        (func(std::integral_constant<size_t, Is>{}), ...);
    }(std::make_index_sequence<N>{});
}

// 
/**
 * @berif 带 break 的计数循环：lambda 返回 bool，false 时停止
 * @usage:
 * StaticForBreak<5>([](auto i) -> bool {
 *      constexpr size_t idx = decltype(i)::value;
 *      SDL_Log("  处理索引 %zu\n", idx);
 *      // 遇到偶数就停止（返回 false）
 *      return (idx % 2 != 0);
 * });
 */
template <size_t N, typename FunctionType>
    requires requires(FunctionType&& func)
    {
        { func(std::integral_constant<size_t, 0>{}) } -> std::convertible_to<bool>;
    }
constexpr void StaticForBreak(FunctionType &&func) 
{
    [&]<size_t... Is>(std::index_sequence<Is...>)
    {
        bool cont = true;
        ((cont = cont && func(std::integral_constant<size_t, Is>{})), ...);
    }(std::make_index_sequence<N>{});
}

/**
 * @berif 范围版本 [Beg, End) 无 break
 * @usage:
 * StaticForRange<2, 6>([](auto i) {
 *      constexpr size_t idx = decltype(i)::value;
 *      SDL_Log("  索引 %zu\n", idx);
 * });
 */
template <size_t Beg, size_t End, typename FunctionType>
constexpr void StaticForRange(FunctionType&& func) 
{
    static_assert(Beg <= End);
    [&]<size_t... Is>(std::index_sequence<Is...>) 
    {
        (func(std::integral_constant<size_t, Beg + Is>{}), ...);
    }(std::make_index_sequence<End - Beg>{});
}

/**
 * @berif 范围版本 [Beg, End) 无 break
 * @usage: 范围版本带 break
 * StaticForRangeBreak<10, 15>([](auto i) -> bool {
 *     constexpr size_t idx = decltype(i)::value;
 *     SDL_Log("  处理索引 %zu\n", idx);
 *     return idx <= 12;  // 索引 >12 时返回 false 停止
 * });
 */
template <size_t Beg, size_t End, typename FunctionType>
constexpr void StaticForRangeBreak(FunctionType&& func) 
{
    static_assert(Beg <= End);
    [&]<size_t... Is>(std::index_sequence<Is...>) 
    {
        bool cont = true;
        ((cont = cont && func(std::integral_constant<size_t, Beg + Is>{})), ...);
    }(std::make_index_sequence<End - Beg>{});
}


#if defined(CORE_STATIC)
    #define CORE_API
#elif defined(_WIN32)
    #if defined(CORE_EXPORTS)
        #define CORE_API __declspec(dllexport)
    #else
        #define CORE_API __declspec(dllimport)
    #endif
#elif defined(__GNUC__) && __GNUC__ >= 4
    #define CORE_API __attribute__((visibility("default")))
#else
    #define CORE_API
#endif

} // namespace Core