//
// Created by admin on 26-2-11.
//

#pragma once
#include "CoreDefination/StaticTools.hpp"

#include <array>
#include <concepts>
#include <cstddef>
#include <initializer_list>
#include <type_traits>
#include <utility>
#include <cmath>


namespace Core::Math {
/**
 * @brief 使用 CRTP 实现的通用向量类模板基类
 * @note 如果需要特化 例如 整型 的 Ty, 则在 对应函数中, 增加  if constexpr(std::is_integral_v<Ty>) 之类的分支处理
 * @note 如果是新增函数, 可以专门使用
 *  template <typename U = Ty>
 *       requires std::is_integral_v<U>
 *  [[nodiscard]] std::size_t NewFunctionForIntegral() const noexcept { ... }
 */
template <class Derived, Arithmetic Ty, std::size_t N>
struct BasicVectorCRTP 
{
    using value_type      = Ty;
    using reference       = Ty&;
    using pointer         = Ty*;
    using const_pointer   = const Ty*;
    using const_reference = const Ty&;

    static constexpr std::size_t dimensions = N;

    std::array<Ty, N> coordinates{};

    constexpr Derived&       derived()       noexcept { return static_cast<Derived&>(*this); }
    constexpr const Derived& derived() const noexcept { return static_cast<const Derived&>(*this); }

    constexpr BasicVectorCRTP() = default;
    explicit constexpr BasicVectorCRTP(Ty value) 
    {
        coordinates.fill(value);
    }

    template <typename... Args>
        requires (sizeof...(Args) == N) &&
                 (std::conjunction_v<std::is_convertible<Args, Ty>...>)
    explicit constexpr BasicVectorCRTP(Args&&... args) 
    {
        std::size_t i = 0;
        ((coordinates[i++] = static_cast<Ty>(std::forward<Args>(args))), ...);
    }

    constexpr const_reference operator[](std::size_t index) const noexcept { return coordinates[index]; }
    constexpr reference operator[](std::size_t index) noexcept { return coordinates[index]; }

    /*
     * @note: 
     * 以下函数是有二义性问题的: 反向重写候选(reversed rewritten candidate)
     * C++20 允许编译器在找不到精确匹配的 operator== 时, 尝试将 a == b 重写为 b == a(即参数顺序反转). 
     * 定义了一个成员 operator==(const Derived& other) 后, 编译器会同时生成一个对应的非成员候选 operator==(const Derived&, const Derived&), 这个候选被视为"反向调用". 
     
     constexpr bool operator==(const Derived& other) const noexcept 
     { 
        return coordinates == other.coordinates; 
     }
    
     * 此时当 operator== 的两个参数类型完全相同 (都是 Derived const&) 时, 
     *  1. 常规调用候选: 成员函数 operator==(const Derived& other), 第一个参数是隐式的 this(类型 const Derived&), 第二个参数是 other
     *  2. 反向调用候选: 编译器生成的非成员函数 operator==(const Derived&, const Derived&), 两个参数都是 const Derived&
     * 因此, 当执行 v1 == v2 时, 编译器发现两个完全等价的候选函数, 无法选择, 从而报歧义错误
     */
    friend constexpr bool operator==(const Derived& lhs, const Derived& rhs) noexcept 
    { 
        return lhs.coordinates == rhs.coordinates; 
    }

    constexpr Derived& operator+=(const Derived& other) noexcept 
    {
        for (std::size_t i = 0; i < N; ++i) coordinates[i] += other.coordinates[i];
        return derived();
    }

    constexpr Derived& operator-=(const Derived& other) noexcept 
    {
        for (std::size_t i = 0; i < N; ++i) coordinates[i] -= other.coordinates[i];
        return derived();
    }

    constexpr Derived& operator*=(const Derived& other) noexcept 
    {
        for (std::size_t i = 0; i < N; ++i) coordinates[i] *= other.coordinates[i];
        return derived();
    }

    constexpr Derived& operator/=(const Derived& other) noexcept 
    {
        for (std::size_t i = 0; i < N; ++i) coordinates[i] /= other.coordinates[i];
        return derived();
    }

    constexpr Derived& operator+=(value_type other) noexcept 
    {
        for (auto& v : coordinates) v += other;
        return derived();
    }

    constexpr Derived& operator-=(value_type other) noexcept 
    {
        for (auto& v : coordinates) v -= other;
        return derived();
    }

    constexpr Derived& operator*=(value_type other) noexcept 
    {
        for (auto& v : coordinates) v *= other;
        return derived();
    }

    constexpr Derived& operator/=(value_type other) noexcept 
    {
        for (auto& v : coordinates) v /= other;
        return derived();
    }

    constexpr Derived operator-() const noexcept 
    {
        Derived result;
        for (std::size_t i = 0; i < N; ++i) result.coordinates[i] = -coordinates[i];
        return result;
    }

    friend constexpr Derived operator+(Derived lhs, const Derived& rhs) noexcept { return lhs += rhs, lhs; }
    friend constexpr Derived operator-(Derived lhs, const Derived& rhs) noexcept { return lhs -= rhs, lhs; }
    friend constexpr Derived operator*(Derived lhs, const Derived& rhs) noexcept { return lhs *= rhs, lhs; }
    friend constexpr Derived operator/(Derived lhs, const Derived& rhs) noexcept { return lhs /= rhs, lhs; }
    friend constexpr Derived operator+(Derived lhs, value_type rhs) noexcept { return lhs += rhs, lhs; }
    friend constexpr Derived operator+(value_type lhs, Derived rhs) noexcept { return rhs += lhs, rhs; }
    friend constexpr Derived operator*(Derived lhs, value_type rhs) noexcept { return lhs *= rhs, lhs; }
    friend constexpr Derived operator*(value_type lhs, Derived rhs) noexcept { return rhs *= lhs, rhs; }
    friend constexpr Derived operator-(Derived lhs, value_type rhs) noexcept { return lhs -= rhs, lhs; }
    friend constexpr Derived operator/(Derived lhs, value_type rhs) noexcept { return lhs /= rhs, lhs; }
    friend constexpr Derived operator-(value_type lhs, Derived rhs) noexcept 
    {
        for (std::size_t i = 0; i < N; ++i) rhs.coordinates[i] = lhs - rhs.coordinates[i];
        return rhs;
    }

    [[nodiscard]] constexpr float Square() const noexcept 
    {
        float sum{};
        for (const auto& v : coordinates) sum += v * v;
        return sum;
    }

    [[nodiscard]] constexpr float Dot(const Derived& other) const noexcept 
    {
        float sum{};
        for (std::size_t i = 0; i < N; ++i) sum += coordinates[i] * other.coordinates[i];
        return sum;
    }

    [[nodiscard]] float Length() const noexcept {
        return static_cast<float>(std::sqrt(static_cast<long double>(Square())));
    }
    
    [[nodiscard]] Derived Normalized() const
        requires std::is_floating_point_v<Ty>
    {
        value_type len = Length();
        if (len == value_type(0)) {
            return Derived{};
        }
        return static_cast<const Derived&>(*this) / len;
    }

    constexpr Derived& Normalize() requires std::is_floating_point_v<Ty> {
        value_type len = Length();
        if (len != value_type(0)) {
            *this /= len;
        }
        return derived();
    }
};

template <Arithmetic Ty, std::size_t Dimensions>
struct Vector : BasicVectorCRTP<Vector<Ty, Dimensions>, Ty, Dimensions> {
    using base = BasicVectorCRTP<Vector<Ty, Dimensions>, Ty, Dimensions>;
    using base::base;
    // 通用版本的静态工厂（若需要可提供，但原始版本没有，此处保持兼容）
};

template <Arithmetic Ty>
struct Vector<Ty, 2> : BasicVectorCRTP<Vector<Ty, 2>, Ty, 2> {
    using base = BasicVectorCRTP<Vector<Ty, 2>, Ty, 2>;
    using value_type      = typename base::value_type;
    using reference       = typename base::reference;
    using const_reference = typename base::const_reference;

    using base::base;

    constexpr reference X() noexcept { return this->coordinates[0]; }
    constexpr reference Y() noexcept { return this->coordinates[1]; }
    [[nodiscard]] constexpr const_reference X() const noexcept { return this->coordinates[0]; }
    [[nodiscard]] constexpr const_reference Y() const noexcept { return this->coordinates[1]; }

    static constexpr Vector ZeroVector() noexcept { return Vector{}; }
    static constexpr Vector OneVector()  noexcept { return Vector{Ty{1}, Ty{1}}; }
    static constexpr Vector XAxisVector() noexcept { return Vector{Ty{1}, Ty{0}}; }
    static constexpr Vector YAxisVector() noexcept { return Vector{Ty{0}, Ty{1}}; }
};

template <Arithmetic Ty>
struct Vector<Ty, 3> : BasicVectorCRTP<Vector<Ty, 3>, Ty, 3> {
    using base = BasicVectorCRTP<Vector<Ty, 3>, Ty, 3>;
    using value_type      = typename base::value_type;
    using reference       = typename base::reference;
    using const_reference = typename base::const_reference;

    using base::base;

    constexpr reference X() noexcept { return this->coordinates[0]; }
    constexpr reference Y() noexcept { return this->coordinates[1]; }
    constexpr reference Z() noexcept { return this->coordinates[2]; }
    [[nodiscard]] constexpr const_reference X() const noexcept { return this->coordinates[0]; }
    [[nodiscard]] constexpr const_reference Y() const noexcept { return this->coordinates[1]; }
    [[nodiscard]] constexpr const_reference Z() const noexcept { return this->coordinates[2]; }

    static constexpr Vector ZeroVector() noexcept { return Vector{Ty{0}, Ty{0}, Ty{0}}; }
    static constexpr Vector OneVector() noexcept { return Vector{Ty{1}, Ty{1}, Ty{1}}; }
    static constexpr Vector XAxisVector() noexcept { return Vector{Ty{1}, Ty{0}, Ty{0}}; }
    static constexpr Vector YAxisVector() noexcept { return Vector{Ty{0}, Ty{1}, Ty{0}}; }
    static constexpr Vector ZAxisVector() noexcept { return Vector{Ty{0}, Ty{0}, Ty{1}}; }
    static constexpr Vector UpVector() noexcept { return Vector{Ty{0}, Ty{0}, Ty{1}}; }
    static constexpr Vector DownVector() noexcept { return Vector{Ty{0}, Ty{0}, Ty{-1}}; }
    static constexpr Vector ForwardVector() noexcept { return Vector{Ty{1}, Ty{0}, Ty{0}}; }
    static constexpr Vector BackwardVector() noexcept { return Vector{Ty{-1}, Ty{0}, Ty{0}}; }
    static constexpr Vector LeftVector() noexcept { return Vector{Ty{0}, Ty{1}, Ty{0}}; }
    static constexpr Vector RightVector() noexcept { return Vector{Ty{0}, Ty{-1}, Ty{0}}; }
};

template <Arithmetic Ty>
struct Vector<Ty, 4> : BasicVectorCRTP<Vector<Ty, 4>, Ty, 4> {
    using base = BasicVectorCRTP<Vector<Ty, 4>, Ty, 4>;
    using value_type      = typename base::value_type;
    using reference       = typename base::reference;
    using const_reference = typename base::const_reference;

    using base::base;

    constexpr reference X() noexcept { return this->coordinates[0]; }
    constexpr reference Y() noexcept { return this->coordinates[1]; }
    constexpr reference Z() noexcept { return this->coordinates[2]; }
    constexpr reference W() noexcept { return this->coordinates[3]; }
    [[nodiscard]] constexpr const_reference X() const noexcept { return this->coordinates[0]; }
    [[nodiscard]] constexpr const_reference Y() const noexcept { return this->coordinates[1]; }
    [[nodiscard]] constexpr const_reference Z() const noexcept { return this->coordinates[2]; }
    [[nodiscard]] constexpr const_reference W() const noexcept { return this->coordinates[3]; }

    static constexpr Vector ZeroVector() noexcept { return Vector{Ty{0}, Ty{0}, Ty{0}, Ty{0}}; }
    static constexpr Vector OneVector() noexcept { return Vector{Ty{1}, Ty{1}, Ty{1}, Ty{1}}; }
    static constexpr Vector XAxisVector() noexcept { return Vector{Ty{1}, Ty{0}, Ty{0}, Ty{0}}; }
    static constexpr Vector YAxisVector() noexcept { return Vector{Ty{0}, Ty{1}, Ty{0}, Ty{0}}; }
    static constexpr Vector ZAxisVector() noexcept { return Vector{Ty{0}, Ty{0}, Ty{1}, Ty{0}}; }
    static constexpr Vector WAxisVector() noexcept { return Vector{Ty{0}, Ty{0}, Ty{0}, Ty{1}}; }
};


template <Arithmetic Ty> using Vector2D = Vector<Ty, 2>;
template <Arithmetic Ty> using Vector3D = Vector<Ty, 3>;
template <Arithmetic Ty> using Vector4D = Vector<Ty, 4>;

template <class Derived, Arithmetic Ty, std::size_t N>
inline Derived operator*(const Ty s, const BasicVectorCRTP<Derived, Ty, N>& v) noexcept 
{
    Derived result(static_cast<const Derived&>(v));
    result *= s;
    return result;
}

template <class Derived, Arithmetic Ty, std::size_t N>
inline Derived operator/(const BasicVectorCRTP<Derived, Ty, N>& v, const Ty s) noexcept 
{
    Derived result(static_cast<const Derived&>(v));
    result /= s;
    return result;
}

} // namespace Core::Math



