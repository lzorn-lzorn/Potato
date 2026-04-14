
#ifndef CORE_COMPILERHINT_H
#define CORE_COMPILERHINT_H

#pragma once

#if defined(__GNUC__) || defined(__clang__)
  #define WARN_IMPL(msg) __builtin_warning(msg)
#elif defined(_MSC_VER)
  #define WARN_IMPL(msg) __pragma(message(__FILE__ "(" STRINGIZE(__LINE__) "): warning: " msg))
#else
  #define WARN_IMPL(msg) \
      [[deprecated(msg)]] static void deprecated_helper() {} \
      deprecated_helper()
#endif

#define STRINGIZE_IMPL(x) #x
#define STRINGIZE(x)  STRINGIZE_IMPL(x)

// 用户接口：函数风格的宏（最便携）
#define WARNING(msg) do { WARN_IMPL(msg); } while (0)

// 可选：C++20 函数风格包装
consteval void warning(std::string_view) {}
struct warning_fn {
    template<size_t N>
    consteval void operator()(const char (&msg)[N]) const {
        WARN_IMPL(msg);
    }
};
inline constexpr warning_fn warn;

#endif