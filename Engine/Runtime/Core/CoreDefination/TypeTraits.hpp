
#pragma once
#include <type_traits>
#include <tuple>
#include <concepts>
#include <string_view>

namespace Core
{

// ========================= VariableType =========================
template <typename Ty>
struct VariableType
{
    using original_type = Ty;
    using pure_type     = std::remove_cvref_t<Ty>;
};

template <typename Class, typename Ty>
struct VariableType<Ty Class::*>
{
    using original_type = Ty;
    using pure_type     = std::remove_cvref_t<Ty>;
};

// ========================= 萃取成员指针的类类型 =========================
template <typename T>
struct MemberPointerClass;

template <typename Class, typename Ty>
struct MemberPointerClass<Ty Class::*>
{
    using type = Class;
};

template <typename T>
using MemberPointerClass_t = typename MemberPointerClass<T>::type;

// ========================= VariableTraits =========================
template <typename Ty>
struct VariableTraits
{
    using original_type = typename VariableType<Ty>::original_type;
    using pure_type     = typename VariableType<Ty>::pure_type;
    using class_type    = void; 

	static constexpr bool is_member_pointer = false;
	static constexpr bool is_pointer        = std::is_pointer_v<Ty>;
	static constexpr bool is_reference      = std::is_reference_v<Ty>;
	static constexpr bool is_const          = std::is_const_v<std::remove_reference_t<Ty>>;
	static constexpr size_t arity           = 0;
};

template <typename Ty>
    requires std::is_member_pointer_v<Ty>
struct VariableTraits<Ty>
{
    using original_type = typename VariableType<Ty>::original_type;
    using pure_type     = typename VariableType<Ty>::pure_type;
    using class_type    = MemberPointerClass_t<Ty>;

	static constexpr bool is_member_pointer = true;
	static constexpr bool is_pointer        = false;
	static constexpr bool is_reference      = false;
	static constexpr bool is_const          = std::is_const_v<std::remove_reference_t<original_type>>;
	static constexpr size_t arity           = 0;
};

// ========================= FunctionTraits =========================
template <typename Ty>
struct FunctionTraits;

template <typename ReturnType, typename... Args>
struct FunctionTraits<ReturnType(Args...)>
{
    using return_type    = ReturnType;
    using argument_types = std::tuple<Args...>;
    using class_type     = void;

    static constexpr size_t arity = sizeof...(Args);
    static constexpr bool is_member_function = false;
    static constexpr bool is_const = false;
};

template <typename ReturnType, typename Class, typename... Args>
struct FunctionTraits<ReturnType(Class::*)(Args...)>
{
    using return_type = ReturnType;
    using argument_types = std::tuple<Class*, Args...>;
    using class_type = Class;

    static constexpr size_t arity = sizeof...(Args);
    static constexpr bool is_member_function = true;
    static constexpr bool is_const = false;
};

template <typename ReturnType, typename Class, typename... Args>
struct FunctionTraits<ReturnType(Class::*)(Args...) const>
{
    using return_type = ReturnType;
    using argument_types = std::tuple<const Class*, Args...>;
    using class_type = Class;

    static constexpr size_t arity = sizeof...(Args);
    static constexpr bool is_member_function = true;
    static constexpr bool is_const = true;
};

// ========================= 惰性求值辅助（避免实例化 FunctionTraits 对非函数类型）=========================
template <typename Ty, bool IsMemFn>
struct ReturnTypeHelper;

template <typename Ty>
struct ReturnTypeHelper<Ty, true> { using type = typename FunctionTraits<Ty>::return_type; };

template <typename Ty>
struct ReturnTypeHelper<Ty, false> { using type = void; };

template <typename Ty, bool IsMemFn>
struct ArgumentTypesHelper;

template <typename Ty>
struct ArgumentTypesHelper<Ty, true> { using type = typename FunctionTraits<Ty>::argument_types; };

template <typename Ty>
struct ArgumentTypesHelper<Ty, false> { using type = std::tuple<>; };

// ========================= BasicFieldTraits =========================
template <typename Ty>
struct BasicFieldTraits
{
private:
    static constexpr bool is_mem_fn = std::is_member_function_pointer_v<Ty>;
    using traits = std::conditional_t<is_mem_fn, FunctionTraits<Ty>, VariableTraits<Ty>>;

public:
    using return_type = typename ReturnTypeHelper<Ty, is_mem_fn>::type;
    using argument_types = typename ArgumentTypesHelper<Ty, is_mem_fn>::type;
    using class_type = typename traits::class_type;

    static constexpr size_t arity = is_mem_fn ? FunctionTraits<Ty>::arity : 0;
    static constexpr bool is_member_function = is_mem_fn;
    static constexpr bool is_const = is_mem_fn ? FunctionTraits<Ty>::is_const : VariableTraits<Ty>::is_const;
    static constexpr bool is_function = is_mem_fn;
    static constexpr bool is_variable = !is_mem_fn;

    constexpr bool IsMemberFunction() const noexcept { return is_member_function; }
    constexpr bool IsConst()        const noexcept { return is_const; }
    constexpr size_t GetArity()     const noexcept { return arity; }
    constexpr bool IsFunction()     const noexcept { return is_function; }
    constexpr bool IsVariable()     const noexcept { return is_variable; }
};

// ========================= FieldTraits =========================
template <typename Ty>
struct FieldTraits : BasicFieldTraits<Ty>
{
    using traits = BasicFieldTraits<Ty>;
    Ty pointer;
    std::string_view name;
    constexpr std::string_view GetRealName(std::string_view name) 
    {
        return name.substr(name.find_last_of(':') + 1);
    }

    constexpr FieldTraits(Ty ptr, std::string_view name) noexcept : pointer(ptr), name(GetRealName(name)) {}
};

// 推导指引
template <typename Ty>
FieldTraits(Ty, std::string_view) -> FieldTraits<Ty>;

// ========================= 辅助别名 =========================
template <auto FuncPtr>
using FunctionPointerType = decltype(FuncPtr);

template <typename... Args>
struct TypeList
{
	static constexpr size_t size = sizeof...(Args);
};
namespace Detail{

template <typename>
struct HeadImpl;

template <typename Ty, typename... Remains>
struct HeadImpl<TypeList<Ty, Remains...>>
{
	using type = Ty;
};

template <typename>
struct TailImpl;

template <typename Ty, typename... Remains>
struct TailImpl<TypeList<Ty, Remains...>>
{
	using type = TypeList<Remains...>;
};

template <typename, size_t>
struct NthImpl;

template <typename Ty, typename... Remains, size_t N>
struct NthImpl<TypeList<Ty, Remains...>, N>
{
    using type = typename NthImpl<TypeList<Remains...>, N-1>::type;
};

template <typename Ty, typename... Remains>
struct NthImpl<TypeList<Ty, Remains...>, 0>
{
    using type = Ty;
};

template <typename, template <typename> typename, size_t N>
struct CountIfImpl;

template <typename Ty, typename... Remains, template <typename> typename Pred, size_t N>
struct CountIfImpl<TypeList<Ty, Remains...>, Pred, N>
{
    static constexpr int value = (Pred<Ty>::value ? 1 : 0) + CountIfImpl<TypeList<Remains...>, Pred, N-1>::value;
};

template <typename Ty, typename... Remains, template <typename> typename Pred>
struct CountIfImpl<TypeList<Ty, Remains...>, Pred, 0>
{
    static constexpr int value = Pred<Ty>::value ? 1 : 0;
};



} // namespace Detail

/**
 * @usage Map
 * template <typename Ty>
 * struct ChangeToFloat
 * {
 *     using type = float;
 * };
 * 
 * using List = Map<type, ChangeToFloat>;
 */
template <typename, template <typename> typename>
struct Map;


template <typename... Args, template <typename> typename Transform>
struct Map<TypeList<Args...>, Transform>
{
    using type = TypeList<typename Transform<Args>::type...>;
}; 

/**
 * @usage CountIf
 *  template <typename Ty>
 *  struct IsIntegral {
 *      static constexpr bool value = std::is_integral_v<Ty>;
 *  }; 
 *  using tyoe = TypeList<int, char, double, int>;
 *  constexpr auto value = CountIf<type, IsIntegral>;
 */
template <typename TypeList, template <typename> typename Pred>
constexpr int CountIf = Detail::CountIfImpl<TypeList, Pred, TypeList::size - 1>::value;


template <typename, typename>
struct Concat;

template <typename... Args1, typename... Args2>
struct Concat<TypeList<Args1...>, TypeList<Args2...>>
{
    using type = TypeList<Args1..., Args2...>;
};




template <typename TypeList, size_t N>
using Nth = typename Detail::NthImpl<TypeList, N>::type;

template <typename TypeList>
using Head = typename Detail::HeadImpl<TypeList>::type;

template <typename TypeList>
using Tail = typename Detail::TailImpl<TypeList>::type;

} // namespace Core