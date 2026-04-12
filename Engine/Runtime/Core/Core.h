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