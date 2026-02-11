#pragma once

// CORE_API handles symbol visibility for the Core DLL/shared library.
// Define CORE_STATIC when building/using the static variant to disable decorations.
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
