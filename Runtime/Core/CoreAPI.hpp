#pragma once

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
