
#include "CoreImpl.h"
#include <nlohmann/json.hpp> 
#include <cstdio>
#include <chrono>
#include <functional>
#include <string_view>
#include <print>
#include <cassert>
#include <iostream>
#include <cmath>
#include <string>
#include <cmath>
#include <numeric>
#include <filesystem>
#include <functional>
#include <memory>
#include <optional>

using json = nlohmann::json;
using ordered_json = nlohmann::ordered_json;
// template <>
// struct TypeInfo<Person> 
// {
//     static constexpr auto functions = std::make_tuple(
//         Core::FieldTraits{ &Person::Introduce },
//         Core::FieldTraits{ &Person::GetAge }
//     );

//     static constexpr auto variables = std::make_tuple(
//         Core::FieldTraits{ &Person::name },
//         Core::FieldTraits{ &Person::age }
//     );
// };



template <typename Ty>
struct TypeInfo;

// 放在命名空间 Core 或全局
struct NullBase {};

template <>
struct TypeInfo<NullBase> {
    using base_type = NullBase;
	using has_base = std::false_type;
	using self_type = NullBase;
    static constexpr auto functions = std::tuple{};
    static constexpr auto variables = std::tuple{};
    static constexpr void ForEachMembers(...) {}  // 空操作
    static constexpr const char* GetClassName() { return "NullBase"; }
};

#define BEGIN_CLASS(Type, ...)  \
    template <>         \
    struct TypeInfo<Type>  \
    {\
		using self_type = Type;\
		using base_type = CORE_FIRST(__VA_ARGS__, NullBase);\
		using has_base  = std::bool_constant<!std::is_same_v<base_type, NullBase>>;\
		static_assert(std::is_same_v<base_type, NullBase> || std::is_class_v<base_type>, "Base must be a class or NullBase");\
		constexpr static std::string_view name = #Type; \
		constexpr static std::string_view StaticClassName() { return name; }

#define FUNCTIONS(...) \
    static constexpr auto functions = [] \
	{\
		if constexpr(!std::is_same_v<base_type, NullBase>)\
		{\
			return std::tuple_cat(TypeInfo<base_type>::functions, std::make_tuple(__VA_ARGS__));\
		}\
		else\
		{\
			return std::make_tuple(__VA_ARGS__);\
		}\
	}();
#define FUNCTION_FIELD(Fn) \
    Core::FieldTraits{ Fn, #Fn }

#define VARIABLES(...) \
    static constexpr auto variables = [] \
	{\
		if constexpr(!std::is_same_v<base_type, NullBase>)\
		{\
			return std::tuple_cat(TypeInfo<base_type>::variables, std::make_tuple(__VA_ARGS__));\
		}\
		else\
		{\
			return std::make_tuple(__VA_ARGS__);\
		}\
	}();
#define VARIABLE_FIELD(Var) \
    Core::FieldTraits{ Var, #Var }


#define FOREACH_MEMBERS_DECL_BEGIN(Type) \
	template <typename FunctionType>\
	static constexpr void ForEachMembers(Type& obj, FunctionType&& fn)\
	{\
		if constexpr(!std::is_same_v<typename TypeInfo<Type>::base_type, NullBase>)\
		{\
			using base = typename TypeInfo<Type>::base_type;\
			TypeInfo<base>::ForEachMembers(obj, std::forward<FunctionType>(fn));\
		}
		
#define REFLECT_PER_MEMBER(x)\
		std::apply([&](auto&... field)\
		{\
			(fn(field.name.data(), obj.*(field.pointer)), ...);\
		}, variables);
		// fn(#x, obj.x);

#define FOREACH_MEMBERS_DECL_END()\
	}

#define GET_CLASS_NAME_DECL(Type) \
	static constexpr const char* GetClassName() { return #Type; }

#define END_CLASS(SelfType) \
    };\
	inline auto SelfType::StaticClass() { return TypeInfo<SelfType>{}; }              
    

#define REFLECT_STATIC_CLASS() \
    static auto StaticClass();

#define REFLECT_CLASS_NAME(Type)\
	GET_CLASS_NAME_DECL(Type) \

#define REFLECT_FOREACH_MEMBERS(Type, ...)\
	FOREACH_MEMBERS_DECL_BEGIN(Type) \
	REFLECT_FOR_EACH_MEMBER(REFLECT_PER_MEMBER, __VA_ARGS__) \
	FOREACH_MEMBERS_DECL_END()




struct Person {
	REFLECT_STATIC_CLASS();

    std::string name;
    int age;
	bool gender;

    void Introduce() const 
	{
        printf("Hi, I'm %s and I'm %d years old.", name.c_str(), age);
    }

    int GetAge() const 
	{
        return age;
    }
};

BEGIN_CLASS(Person, NullBase)
    FUNCTIONS(
        FUNCTION_FIELD(&Person::Introduce),
        FUNCTION_FIELD(&Person::GetAge)
    )
    VARIABLES(
        VARIABLE_FIELD(&Person::name),
        VARIABLE_FIELD(&Person::age),
		VARIABLE_FIELD(&Person::gender)
    )
	REFLECT_FOREACH_MEMBERS(Person, name, age, gender)
	REFLECT_CLASS_NAME(Person)
END_CLASS(Person)

// template <typename Ty>
// auto StaticClass() 
// {
//     return TypeInfo<Ty>{};
// };

// @Eg:
// int main()
// {
// 	Person p {"Alice", 30, true};
// 	ordered_json j;
// 	Person::StaticClass().ForEachMembers(p, [&j](const char* member_name, auto& member_value) {
// 		j[member_name] = member_value;
// 	});
// 	printf("%s\n", Person::StaticClass().GetClassName());
// 	std::cout << j.dump(4) << std::endl;

	
	
//     // using type1 = Core::FunctionPointerType<&Person::Introduce>;

//     // static_assert(std::is_same_v<type1, void (Person::*)() const>);

//     // auto field = Core::FieldTraits{&Person::Introduce};
//     // printf("Is const: %s\n", field.IsConst() ? "true" : "false");
//     // printf("Is function: %s\n", field.IsFunction() ? "true" : "false");
//     // printf("Is variable: %s\n", field.IsVariable() ? "true" : "false");
//     return 0;
// }