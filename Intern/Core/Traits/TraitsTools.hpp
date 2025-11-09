#pragma once
#include <type_traits>
namespace Core::Traits {
/*
 * @function: 非平台拷贝构造的代理
 * @note: 基类需要自己实现 __ConstructFrom(const BaseClass&) 这个方法
 */
template <typename BaseClass>
struct NonTrivialCopy : BaseClass {
	using BaseClass::BaseClass;
	NonTrivialCopy() = default;
	/* C++20 */
	constexpr NonTrivialCopy(const NonTrivialCopy& that) 
		noexcept(
			noexcept(
				BaseClass::__ConstructFrom(static_cast<const BaseClass&>(that))
			)
		) {
		BaseClass::__ConstructFrom(static_cast<const BaseClass&>(that));
	}
	NonTrivialCopy(NonTrivialCopy&&) = default;
	NonTrivialCopy& operator=(const NonTrivialCopy&) = default;
	NonTrivialCopy& operator=(NonTrivialCopy&&) = default;
};
/*
 * @function: 非平台拷贝构造的代理
 * @note: 基类需要自己实现 __ConstructFrom(const BaseClass&) 这个方法
 */
template <typename BaseClass>
struct DeletedCopy : BaseClass {
	using BaseClass::BaseClass;
	DeletedCopy() = default;
	DeletedCopy(const DeletedCopy&) = delete;
	DeletedCopy(DeletedCopy&&) = default;
	DeletedCopy& operator=(const DeletedCopy&) = default;
	DeletedCopy& operator=(DeletedCopy&&) = default;
};

template <typename BaseClass, typename... Types>
using SMFControlCopy = std::conditional_t<
	/* 如果所有类型都是平凡拷贝构造的 (int, double...) */
	std::conjunction_v<std::is_trivially_copy_constructible<Types>...>,
	/* 则使用基类, 让编译器自动生成SMF(拷贝构造是自定义的)即可 */
	BaseClass,
	/* 否则 */
	std::conditional_t<
		/* 如果所有类型都是可拷贝的 */
		/* 因为这个模板是支持 tuple 这种多个参数的, 所以要支持多类型判断 */
		std::conjunction_v<std::is_copy_constructible<Types>...>,
		/* 让编译器自动生成SMF但是拷贝构造是自定义的 */
		NonTrivialCopy<BaseClass>,
		/* 否则, 禁止拷贝 */
		DeletedCopy<BaseClass>
	>
>;
/*
 * @function: 非平台移动构造的代理
 * @note: SMF Generation Rules: 自动生成 move 构造/赋值，前提是 copy 构造/赋值没有被 delete
 * @note: 如果你的类型不可拷贝(copy 被 delete), 则编译器不会为你自动生成移动构造/赋值
 * @note: 所以, 对于非平凡的移动要先有非平凡的拷贝, 不然编译器也不会自动生成
 */
template <typename BaseClass, typename... Types>
struct NonTrivialMove : SMFControlCopy<BaseClass, Types...> {
	using MyBaseClassTp = SMFControlCopy<BaseClass, Types...>;
	using MyBaseClassTp::MyBaseClassTp;

	NonTrivialMove() = default;
	NonTrivialMove(const NonTrivialMove&) = default;
	/* C++20 */
	constexpr NonTrivialMove(NonTrivialMove&& that) 
		noexcept(
			noexcept(
				MyBaseClassTp::__ConstructFrom(static_cast<BaseClass&&>(that))
			)
		) 
	{
		MyBaseClassTp::__ConstructFrom(static_cast<BaseClass&&>(that));
	}
	NonTrivialMove& operator=(const NonTrivialMove&) = default;
	NonTrivialMove& operator=(NonTrivialMove&&) = default;
};

template <typename BaseClass, typename... Types>
struct DeletedMove : SMFControlCopy<BaseClass, Types...> {
	using MyBaseClassTp = SMFControlCopy<BaseClass, Types...>;
	using MyBaseClassTp::MyBaseClassTp;

	DeletedMove() = default;
	DeletedMove(const DeletedMove&) = default;
	DeletedMove(DeletedMove&&) = delete;
	DeletedMove& operator=(const DeletedMove&) = default;
	DeletedMove& operator=(DeletedMove&&) = default;
};

template <typename BaseClass, typename... Types>
using SMFControlMove = std::conditional_t<
	/* 判断是不是可以平凡移动的 */
	std::conjunction_v<std::is_trivially_move_constructible<Types>...>,
	/* 可以平凡移动, 就使用 SMFControlCopy */
	SMFControlCopy<BaseClass, Types...>,
	/* 不可以平凡移动, 就找 Type... 中的类型有没有可以移动构造的 */	
	std::conditional_t<
		std::conjunction_v<std::is_move_constructible<Types>...>,
		NonTrivialMove<BaseClass, Types...>,
		/* 只要有一个没有, 就禁止移动 */
		DeletedMove<BaseClass, Types...>
	>
>;

template <typename BaseClass, typename... Types>
struct NonTrivialCopyAssign : SMFControlMove<BaseClass, Types...> {
	using MyBaseClassTp = SMFControlMove<BaseClass, Types...>;
	using MyBaseClassTp::MyBaseClassTp;

	NonTrivialCopyAssign() = default;
	NonTrivialCopyAssign(const NonTrivialCopyAssign&) = default;
	NonTrivialCopyAssign(NonTrivialCopyAssign&&) = default;
	/* C++20 constexpr */
	constexpr NonTrivialCopyAssign& operator=(const NonTrivialCopyAssign& that) 
		noexcept(
			noexcept(
				MyBaseClassTp::__AssignFrom(static_cast<const BaseClass&>(that))
			)
		) 
	{
		MyBaseClassTp::__AssignFrom(static_cast<const BaseClass&>(that));
		return *this;
	}
	NonTrivialCopyAssign& operator=(NonTrivialCopyAssign&&) = default;
};

template <typename BaseClass, typename... Types>
struct DeletedCopyAssign : SMFControlMove<BaseClass, Types...> {
	using MyBaseClassTp = SMFControlMove<BaseClass, Types...>;
	using MyBaseClassTp::MyBaseClassTp;

	DeletedCopyAssign() = default;
	DeletedCopyAssign(const DeletedCopyAssign&) = default;
	DeletedCopyAssign(DeletedCopyAssign&&) = default;
	DeletedCopyAssign& operator=(const DeletedCopyAssign&) = delete;
	DeletedCopyAssign& operator=(DeletedCopyAssign&&) = default;
};

template <typename BaseClass, typename... Types>
using SMFControlCopyAssign = std::conditional_t<
	std::conjunction_v<
		std::is_trivially_destructible<Types>...,
		std::is_trivially_copy_constructible<Types>...,
		std::is_trivially_copy_assignable<Types>...
	>,
	/* 自动生成 trivial 拷贝赋值 */
	SMFControlMove<BaseClass, Types...>, 
	std::conditional_t<
		std::conjunction_v<
			std::is_copy_constructible<Types>...,
			std::is_copy_assignable<Types>...
		>,
		/* 自动生成非 trivial 的拷贝赋值 */
		NonTrivialCopyAssign<BaseClass, Types...>,
		DeletedCopyAssign<BaseClass, Types...>
	>
>;

template <typename BaseClass, typename... Types>
struct NonTrivialMoveAssign : SMFControlCopyAssign <BaseClass, Types...>{
	using MyBaseClassTp = SMFControlCopyAssign<BaseClass, Types...>;
	using MyBaseClassTp::MyBaseClassTp;

	NonTrivialMoveAssign() = default;
	NonTrivialMoveAssign(const NonTrivialMoveAssign&) = default;
	NonTrivialMoveAssign(NonTrivialMoveAssign&&) = default;
	NonTrivialMoveAssign& operator=(const NonTrivialMoveAssign&) = default;
	/* C++20 constexpr */
	constexpr NonTrivialMoveAssign& operator=(NonTrivialMoveAssign&& that)
		noexcept(
			noexcept(
				MyBaseClassTp::__AssignFrom(static_cast<BaseClass&&>(that))
			)
		) {
		MyBaseClassTp::__AssignFrom(static_cast<BaseClass&&>(that));
		return *this;
	}
};

template <typename BaseClass, typename... Types>
struct DeletedMoveAssign : SMFControlCopyAssign<BaseClass, Types...> {
	using MyBaseClassTp = SMFControlCopyAssign<BaseClass, Types...>;
	using MyBaseClassTp::MyBaseClassTp;

	DeletedMoveAssign()                                    = default;
	DeletedMoveAssign(const DeletedMoveAssign&)            = default;
	DeletedMoveAssign(DeletedMoveAssign&&)                 = default;
	DeletedMoveAssign& operator=(const DeletedMoveAssign&) = default;
	DeletedMoveAssign& operator=(DeletedMoveAssign&&)      = delete;
};

template <typename BaseClass, typename... Types>
using SMFControlMoveAssign = std::conditional_t<
	std::conjunction_v<
		std::is_trivially_destructible<Types>...,
		std::is_trivially_move_constructible<Types>...,
		std::is_trivially_move_assignable<Types>...
	>,
	SMFControlCopyAssign<BaseClass, Types...>,
	std::conditional_t<
		std::conjunction_v<
			std::is_move_constructible<Types>...,
			std::is_move_assignable<Types>...
		>,
		NonTrivialMoveAssign<BaseClass, Types...>,
		DeletedMoveAssign<BaseClass, Types...>
	>
>;

template <typename BaseClass, typename... Types>
using AutoControlSMF = SMFControlMoveAssign<BaseClass, Types...>;

}