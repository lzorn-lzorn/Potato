//
// Created by admin on 26-2-11.
//

#ifndef VECTOR_H
#define VECTOR_H

#include <array>
#include <concepts>
#include <cstddef>
#include <initializer_list>
#include <type_traits>
#include <utility>
#include <cmath>

/**
 * @brief 使用 CRTP 实现的通用向量类模板基类
 * @note 如果需要特化 例如 整型 的 Ty, 则在 对应函数中, 增加  if constexpr(std::is_integral_v<Ty>) 之类的分支处理
 * @note 如果是新增函数, 可以专门使用
 *  template <typename U = Ty>
 *       requires std::is_integral_v<U>
 *  [[nodiscard]] std::size_t NewFunctionForIntegral() const noexcept { ... }
 */
template <class Derived, typename Ty, std::size_t N>
    requires std::is_arithmetic_v<Ty>
struct BasicVectorCRTP {
    using value_type      = Ty;
    using reference       = Ty&;
    using pointer         = Ty*;
    using const_pointer   = const Ty*;
    using const_reference = const Ty&;

    static constexpr std::size_t dimensions = N;

    std::array<Ty, N> coordinates{};

    // 获取派生类引用的辅助函数
    constexpr Derived&       derived()       noexcept { return static_cast<Derived&>(*this); }
    constexpr const Derived& derived() const noexcept { return static_cast<const Derived&>(*this); }

    constexpr BasicVectorCRTP() = default;
    explicit constexpr BasicVectorCRTP(Ty value) {
        coordinates.fill(value);
    }

    template <typename... Args>
        requires (sizeof...(Args) == N) &&
                 (std::conjunction_v<std::is_convertible<Args, Ty>...>)
    explicit constexpr BasicVectorCRTP(Args&&... args) {
        std::size_t i = 0;
        ((coordinates[i++] = static_cast<Ty>(std::forward<Args>(args))), ...);
    }

    constexpr const_reference operator[](std::size_t index) const noexcept { return coordinates[index]; }
    constexpr reference operator[](std::size_t index) noexcept { return coordinates[index]; }
    constexpr bool operator==(const Derived& other) const noexcept { return coordinates == other.coordinates; }

    constexpr Derived& operator+=(const Derived& other) noexcept {
        for (std::size_t i = 0; i < N; ++i)
            coordinates[i] += other.coordinates[i];
        return derived();
    }

    constexpr Derived& operator-=(const Derived& other) noexcept {
        for (std::size_t i = 0; i < N; ++i)
            coordinates[i] -= other.coordinates[i];
        return derived();
    }

    constexpr Derived& operator*=(const Derived& other) noexcept {
        for (std::size_t i = 0; i < N; ++i)
            coordinates[i] *= other.coordinates[i];
        return derived();
    }

    constexpr Derived& operator/=(const Derived& other) noexcept {
        for (std::size_t i = 0; i < N; ++i)
            coordinates[i] /= other.coordinates[i];
        return derived();
    }

    constexpr Derived& operator+=(value_type other) noexcept {
        for (auto& v : coordinates) v += other;
        return derived();
    }

    constexpr Derived& operator-=(value_type other) noexcept {
        for (auto& v : coordinates) v -= other;
        return derived();
    }

    constexpr Derived& operator*=(value_type other) noexcept {
        for (auto& v : coordinates) v *= other;
        return derived();
    }

    constexpr Derived& operator/=(value_type other) noexcept {
        for (auto& v : coordinates) v /= other;
        return derived();
    }


    constexpr Derived operator-() const noexcept {
        Derived result;
        for (std::size_t i = 0; i < N; ++i)
            result.coordinates[i] = -coordinates[i];
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
    friend constexpr Derived operator-(value_type lhs, Derived rhs) noexcept {
        for (std::size_t i = 0; i < N; ++i)
            rhs.coordinates[i] = lhs - rhs.coordinates[i];
        return rhs;
    }

    [[nodiscard]] constexpr value_type Square() const noexcept {
        value_type sum{};
        for (const auto& v : coordinates)
            sum += v * v;
        return sum;
    }

    [[nodiscard]] constexpr value_type Dot(const Derived& other) const noexcept {
        value_type sum{};
        for (std::size_t i = 0; i < N; ++i)
            sum += coordinates[i] * other.coordinates[i];
        return sum;
    }

    [[nodiscard]] value_type Length() const noexcept {
        return static_cast<value_type>(std::sqrt(static_cast<long double>(Square())));
    }

};


template <typename Ty, std::size_t Dimensions>
    requires std::is_arithmetic_v<Ty>
struct Vector : BasicVectorCRTP<Vector<Ty, Dimensions>, Ty, Dimensions>{
    using Base            =  BasicVectorCRTP<Vector<Ty, Dimensions>, Ty, Dimensions>;
    using value_type      = typename Base::value_type;
    using reference       = typename Base::reference;
    using const_reference = typename Base::const_reference;
    using Base::Base;
};

template <typename Ty>
    requires std::is_arithmetic_v<Ty>
struct Vector2D : BasicVectorCRTP<Vector2D<Ty>, Ty, 2> {
    using Base            = BasicVectorCRTP<Vector2D<Ty>, Ty, 2>;
    using value_type      = typename Base::value_type;
    using reference       = typename Base::reference;
    using const_reference = typename Base::const_reference;

    using Base::Base;

    constexpr reference X() noexcept { return this->coordinates[0]; }
    constexpr reference Y() noexcept { return this->coordinates[1]; }

    [[nodiscard]] constexpr const_reference X() const noexcept { return this->coordinates[0]; }
    [[nodiscard]] constexpr const_reference Y() const noexcept { return this->coordinates[1]; }

    static constexpr Vector2D ZeroVector() noexcept { return Vector2D{}; }
    static constexpr Vector2D OneVector()  noexcept { return Vector2D{Ty{1}, Ty{1}}; }
    static constexpr Vector2D XAxisVector() noexcept { return Vector2D{Ty{1}, Ty{0}}; }
    static constexpr Vector2D YAxisVector() noexcept { return Vector2D{Ty{0}, Ty{1}}; }
};

template <typename Ty>
    requires std::is_arithmetic_v<Ty>
struct Vector3D : BasicVectorCRTP<Vector3D<Ty>, Ty, 3> {
    using Base            = BasicVectorCRTP<Vector3D<Ty>, Ty, 3>;
    using value_type      = typename Base::value_type;
    using reference       = typename Base::reference;
    using const_reference = typename Base::const_reference;

    using Base::Base;

    constexpr reference X() noexcept { return this->coordinates[0]; }
    constexpr reference Y() noexcept { return this->coordinates[1]; }
    constexpr reference Z() noexcept { return this->coordinates[2]; }
    [[nodiscard]] constexpr const_reference X() const noexcept { return this->coordinates[0]; }
    [[nodiscard]] constexpr const_reference Y() const noexcept { return this->coordinates[1]; }
    [[nodiscard]] constexpr const_reference Z() const noexcept { return this->coordinates[2]; }

    constexpr static Vector3D ZeroVector() noexcept { return Vector3D{Ty{0.0f}, Ty{0.0f}, Ty{0.0f}}; }
    constexpr static Vector3D OneVector() noexcept { return Vector3D{Ty{1.0f}, Ty{1.0f}, Ty{1.0f}}; }
    constexpr static Vector3D XAxisVector() noexcept { return Vector3D{Ty{1.0f}, Ty{0.0f}, Ty{0.0f}}; }
    constexpr static Vector3D YAxisVector() noexcept { return Vector3D{Ty{0.0f}, Ty{1.0f}, Ty{0.0f}}; }
    constexpr static Vector3D ZAxisVector() noexcept { return Vector3D{Ty{0.0f}, Ty{0.0f}, Ty{1.0f}}; }
    constexpr static Vector3D UpVector() noexcept { return Vector3D{Ty{0.0f}, Ty{0.0f}, Ty{1.0f}}; }
    constexpr static Vector3D DownVector() noexcept { return Vector3D{Ty{0.0f}, Ty{0.0f}, Ty{-1.0f}}; }
    constexpr static Vector3D ForwardVector() noexcept { return Vector3D{Ty{1.0f}, Ty{0.0f}, Ty{0.0f}}; }
    constexpr static Vector3D BackwardVector() noexcept { return Vector3D{Ty{-1.0f}, Ty{0.0f}, Ty{0.0f}}; }
    constexpr static Vector3D LeftVector() noexcept { return Vector3D{Ty{0.0f}, Ty{1.0f}, Ty{0.0f}}; }
    constexpr static Vector3D RightVector() noexcept { return Vector3D{Ty{0.0f}, Ty{-1.0f}, Ty{0.0f}}; }
};

template <typename Ty>
    requires std::is_arithmetic_v<Ty>
struct Vector4D : BasicVectorCRTP<Vector4D<Ty>, Ty, 4> {
    using Base = BasicVectorCRTP<Vector4D<Ty>, Ty, 4>;
    using Base::Base;

    using value_type      = typename Base::value_type;
    using reference       = typename Base::reference;
    using const_reference = typename Base::const_reference;

    // 语义访问器
    constexpr reference X() noexcept { return this->coordinates[0]; }
    constexpr reference Y() noexcept { return this->coordinates[1]; }
    constexpr reference Z() noexcept { return this->coordinates[2]; }
    constexpr reference W() noexcept { return this->coordinates[3]; }
    [[nodiscard]] constexpr const_reference X() const noexcept { return this->coordinates[0]; }
    [[nodiscard]] constexpr const_reference Y() const noexcept { return this->coordinates[1]; }
    [[nodiscard]] constexpr const_reference Z() const noexcept { return this->coordinates[2]; }
    [[nodiscard]] constexpr const_reference W() const noexcept { return this->coordinates[3]; }

    constexpr static Vector4D ZeroVector() noexcept { return Vector4D{Ty{0.0f}, Ty{0.0f}, Ty{0.0f}, Ty{0.0f}}; }
    constexpr static Vector4D OneVector() noexcept { return Vector4D{Ty{1.0f}, Ty{1.0f}, Ty{1.0f}, Ty{1.0f}}; }
    constexpr static Vector4D XAxisVector() noexcept { return Vector4D{Ty{1.0f}, Ty{0.0f}, Ty{0.0f}, Ty{0.0f}}; }
    constexpr static Vector4D YAxisVector() noexcept { return Vector4D{Ty{0.0f}, Ty{1.0f}, Ty{0.0f}, Ty{0.0f}}; }
    constexpr static Vector4D ZAxisVector() noexcept { return Vector4D{Ty{0.0f}, Ty{0.0f}, Ty{1.0f}, Ty{0.0f}}; }
    constexpr static Vector4D WAxisVector() noexcept { return Vector4D{Ty{0.0f}, Ty{0.0f}, Ty{0.0f}, Ty{1.0f}}; }
};

template <class Derived, typename Ty, std::size_t N>
    requires std::is_arithmetic_v<Ty>
inline Derived operator*(const Ty s, const BasicVectorCRTP<Derived, Ty, N>& v) noexcept {
    Derived result(static_cast<const Derived&>(v)); // 拷贝一份
    result *= s;
    return result;
}

template <class Derived, typename Ty, std::size_t N>
    requires std::is_arithmetic_v<Ty>
inline Derived operator/(const BasicVectorCRTP<Derived, Ty, N>& v, const Ty s) noexcept {
    Derived result(static_cast<const Derived&>(v));
    result /= s;
    return result;
}

// 平行判断：同向或反向均视为平行
// a 和 b 至少有一个非零才有意义
template <class Derived, typename Ty, std::size_t N>
    requires std::is_arithmetic_v<Ty>
inline bool IsParallel(const BasicVectorCRTP<Derived, Ty, N>& a,
                       const BasicVectorCRTP<Derived, Ty, N>& b,
                       Ty epsilon = Ty(1e-6)) noexcept
{
    using Vec = BasicVectorCRTP<Derived, Ty, N>;
    const auto& da = static_cast<const Derived&>(a);
    const auto& db = static_cast<const Derived&>(b);

    // 处理零向量：这里约定任意向量与零向量不算平行
    if (da.Square() == Ty{} || db.Square() == Ty{})
        return false;

    if constexpr (std::is_floating_point_v<Ty>) {
        Ty cos2 = da.Dot(db);
        cos2 *= cos2;
        Ty sq = da.Square() * db.Square();
        if (sq == Ty{}) return false;
        // |cos^2 - 1| <= eps 判定为平行
        return std::fabs(static_cast<double>(cos2 - sq)) <= static_cast<double>(epsilon);
    } else {
        // 整型：只在精确满足 cos^2 == 1 的情况下认为平行
        Ty cos2 = da.Dot(db);
        cos2 *= cos2;
        Ty sq = da.Square() * db.Square();
        return cos2 == sq;
    }
}

template <class Derived, typename Ty, std::size_t N>
    requires std::is_arithmetic_v<Ty>
inline bool IsVertical(const BasicVectorCRTP<Derived, Ty, N>& a,
                       const BasicVectorCRTP<Derived, Ty, N>& b,
                       Ty epsilon = Ty(1e-6)) noexcept
{
    const auto& da = static_cast<const Derived&>(a);
    const auto& db = static_cast<const Derived&>(b);

    if constexpr (std::is_floating_point_v<Ty>) {
        return std::fabs(static_cast<double>(da.Dot(db))) <= static_cast<double>(epsilon);
    } else {
        return da.Dot(db) == Ty{};
    }
}

template <class Derived, typename Ty, std::size_t N, typename FloatTy = float>
    requires std::is_arithmetic_v<Ty> && std::is_floating_point_v<FloatTy>
inline auto Normalize(const BasicVectorCRTP<Derived, Ty, N>& v)
{
    using FloatVector = Vector<FloatTy, N>;

    FloatVector result{};
    const auto& dv = static_cast<const Derived&>(v);

    for (std::size_t i = 0; i < N; ++i)
        result.coordinates[i] = static_cast<FloatTy>(dv.coordinates[i]);

    FloatTy len_sq{};
    for (std::size_t i = 0; i < N; ++i)
        len_sq += result.coordinates[i] * result.coordinates[i];

    if (len_sq == FloatTy(0)) {
        return result;
    }

    const FloatTy len = std::sqrt(len_sq);
    for (std::size_t i = 0; i < N; ++i)
        result.coordinates[i] /= len;

    return result;
}



#endif //VECTOR_H
