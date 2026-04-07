
#pragma once
#include "CoreDefination/MarcoArgsNum.h"	
#include "CoreDefination/TypeTraits.hpp"

#define CORE_CONCAT_IMPL(a, b) a##b
#define CORE_CONCAT(a, b) CORE_CONCAT_IMPL(a, b)

#define CORE_EXPAND_IMPL(...) __VA_ARGS__
#define CORE_EXPAND(...) CORE_EXPAND_IMPL(__VA_ARGS__)

// 用于去掉括号, Eg: CORE_REMOVE_PARENS((a,b,c)) -> a,b,c
#define CORE_REMOVE_PARENS(...) __VA_ARGS__

// CORE_CONCAT 和 CORE_EXPAND 都是为了确保宏被正确的求值
// Eg: 如果只有 CORE_CONCAT(CORE_FOR_EACH_MEMBER_, CORE_NUM_ARGS(__VA_ARGS__))(Macro, __VA_ARGS__)
//     此时 CORE_CONCAT 求值结束后 CORE_FOR_EACH_MEMBER_2(Macro, int, double) 就不会再求值
//     所以为了让其继续求值, 在最外层包裹一层 CORE_CONCAT, 让求值继续下去
#define REFLECT_FOR_EACH_MEMBER(Fn, ...) \
	CORE_EXPAND(CORE_CONCAT(MACRO_ARGS_NUM_FOREACH_, MACRO_ARGS_NUM(__VA_ARGS__))(Fn, __VA_ARGS__))

#define REFLECT_EXPAND_LIST(Macro, List) \
	REFLECT_FOR_EACH_MEMBER(Macro, CORE_REMOVE_PARENS List)

#define CORE_FIRST_IMPL(x, ...) x
#define CORE_FIRST(...) CORE_FIRST_IMPL(__VA_ARGS__, )

