
#pragma once
#include "CoreDefination/MarcoArgsNum.h"	
#include "CoreDefination/TypeTraits.hpp"
namespace Core
{

#define CORE_CONCAT_IMPL(a, b) a##b
#define CORE_CONCAT(a, b) CORE_CONCAT_IMPL(a, b)

#define CORE_EXPAND_IMPL(...) __VA_ARGS__
#define CORE_EXPAND(...) CORE_EXPAND_IMPL(__VA_ARGS__)

} // namespace Core
