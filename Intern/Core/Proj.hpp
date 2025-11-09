#pragma once

/*
 * 使用CMake显示指定使用:
 * Proj_EXPORTS: 导入/出win下的API
 * 导出 C API, Useage:
    #ifdef __cplusplus
    extern "C" {
    #endif

    Proj_API void MYLIB_CALL hello_c();

    #ifdef __cplusplus
    }
    #endif
 * 不兼容 x86 的话, Proj_CALL 是无效的, 所以可以不用管
 */ 
#if defined(_WIN32) || defined(__CYGWIN__)
    #if defined (Proj_STATIC)
        #define Proj_API
    #elif defined (Proj_EXPORTS)
        #define Proj_API __declspec(dllexport)
    #else
        #define Proj_API __declspec(dllimport)
    #endif

    #if defined(_M_X64) || defined(__x86_64__)
        #define Proj_CALL
    #else
        #if defined(Proj_USE_STDCALL)
            #define Proj_CALL __stdcall
        #else
            #define Proj_CALL __cdecl
        #endif
    #endif
#else
    #if defined (Proj_STATIC)
        #define Proj_API
    #else
        #if __GNUC__ >= 4 && defined (__GNUC__)
            #define Proj_API __attribute__ ((visibility ("default")))
        #else
            #define Proj_API
        #endif
    #endif
#endif

#if defined(__cpp_inline_variables)
    #define INLINE inline

#else 
    #if defined(_MSC_VER)
        #define INLINE __inline
    #elif defined(__GNUC__) || defined(__clang__)
        #define INLINE __inline__
    #else
        #define INLINE 
    #endif
    #define Proj_CALL
#endif

/* 
 * @Usage: if (LIKELY(x > 0)) or if (UNLIKELY(x > 0))
 */
#if defined(__cplusplus) && __cplusplus >= 202002L
    #define LIKELY(x)   (x) [[likely]]
    #define UNLIKELY(x) (x) [[unlikely]]
#elif defined(__GNUC__) || defined(__clang__)
    #define LIKELY(x)   __builtin_expect(!!(x), 1)
    #define UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
    #define LIKELY(x)   (x)
    #define UNLIKELY(x) (x)
#endif

#if defined(__cplusplus) && __cplusplus >= 201103L
    #define CONSTEXPR constexpr
#else
    #define CONSTEXPR
#endif

#if __cplusplus >= 202002L
#define CONSTEXPR20 constexpr
#else
#define CONSTEXPR20
#endif



