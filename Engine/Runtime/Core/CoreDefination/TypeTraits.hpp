
#include <type_traits>
#include <utility>
namespace Core 
{

template <typename Ty>
struct VariableType 
{
	using original_type = Ty;
	using pure_type     = std::remove_cv_t<std::remove_reference_t<Ty>>;
};

template <typename Class, typename Ty>
struct VariableType<Ty Class::*>
{
	using original_type = Ty;
	using pure_type = std::remove_cv_t<std::remove_reference_t<Ty>>;
};

template <typename Ty>
struct VariableTraits
{
	using original_type = typename VariableType<Ty>::original_type;
	using pure_type     = typename VariableType<Ty>::pure_type;
	using class_type    = void;

	static constexpr bool is_member_pointer = std::is_member_pointer_v<Ty>;
	static constexpr bool is_pointer        = std::is_pointer_v<Ty>;
	static constexpr bool is_reference      = std::is_reference_v<Ty>;
	static constexpr bool is_const          = std::is_const_v<std::remove_reference_t<Ty>>;
};

template <typename Class, typename Ty>
struct VariableTraits<Ty Class::*>
{
	using original_type = typename VariableType<Ty Class::*>::original_type;
	using pure_type     = typename VariableType<Ty Class::*>::pure_type;
	using class_type	= Class;

	static constexpr bool is_member_pointer = std::is_member_pointer_v<Ty>;
	static constexpr bool is_pointer        = std::is_pointer_v<Ty>;
	static constexpr bool is_reference      = std::is_reference_v<Ty>;
	static constexpr bool is_const          = std::is_const_v<std::remove_reference_t<Ty>>;
};

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

template <typename ReturnType, typename... Args>
auto FunctionPointerTypeTraits(ReturnType (*)(Args...)) -> ReturnType (*)(Args...);


template <typename ReturnType, typename Class, typename... Args>
auto FunctionPointerTypeTraits(ReturnType (Class::*)(Args...)) -> ReturnType (Class::*)(Args...);

template <typename ReturnType, typename Class, typename... Args>
auto FunctionPointerTypeTraits(ReturnType (Class::*)(Args...) const) -> ReturnType (Class::*)(Args...) const;
template <auto FunctionPtr>
using FunctionPointerType = decltype(FunctionPointerTypeTraits(FunctionPtr));

template <typename Ty, bool IsFunction = std::is_member_function_pointer_v<Ty> || std::is_function_v<Ty> || std::is_function_v<std::remove_pointer_t<Ty>>>
struct BasicFieldTraits;
template <typename Ty>
struct BasicFieldTraits<Ty, true> : FunctionTraits<Ty>
{
	using traits = FunctionTraits<Ty>;

	constexpr bool IsMemberFunction() const noexcept 
	{ 
		return traits::is_member_function; 
	}

	constexpr bool IsConst() const noexcept 
	{ 
		return traits::is_const; 
	}

	constexpr size_t GetArity() const noexcept 
	{ 
		return traits::arity; 
	}

	constexpr bool IsFunction() const noexcept 
	{
		return true; 
	}

	constexpr bool IsVariable() const noexcept 
	{
		return false; 
	}
};

template <typename Ty>
struct BasicFieldTraits<Ty, false> : VariableTraits<Ty>
{
	using traits = VariableTraits<Ty>;

	constexpr bool IsMemberFunction() const noexcept 
	{ 
		return false; 
	}

	constexpr bool IsConst() const noexcept 
	{ 
		return traits::is_const; 
	}

	constexpr size_t GetArity() const noexcept 
	{ 
		return traits::arity; 
	}

	constexpr bool IsFunction() const noexcept 
	{
		return false; 
	}

	constexpr bool IsVariable() const noexcept 
	{
		return true; 
	}
};

template <typename Ty>
struct FieldTraits : BasicFieldTraits<Ty>
{
	constexpr FieldTraits(Ty&& pointer) : pointer(pointer) {}
	using traits = BasicFieldTraits<Ty>;

	Ty pointer;
};

}