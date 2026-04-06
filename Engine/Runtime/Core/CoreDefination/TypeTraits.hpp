
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

}