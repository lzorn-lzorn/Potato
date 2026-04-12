
// #pragma once
#ifndef CORE_MATHFUNCTIONS_H
#define CORE_MATHFUNCTIONS_H

#include "Detail/FunctionsDetail.h"

namespace Core 
{
statie constexpr float epsilon  = Impl::epsilon;
statie constexpr float infinity = Impl::infinity;
statie constexpr float pi       = Impl::pi;

template <typename Ty>
inline Ty Clamp(Ty value, Ty min, Ty max)
{
	return Impl::Clamp(value, min, max);
}

template <typename Ty>
inline Ty Lerp(Ty a, Ty b, Ty t)
{
	return Impl::Lerp(a, b, t);
}


// 平行判断：同向或反向均视为平行
// a 和 b 至少有一个非零才有意义
template <class Derived, typename Ty, std::size_t N>
    requires std::is_arithmetic_v<Ty>
inline bool IsParallel(const BasicVectorCRTP<Derived, Ty, N>& a,
                       const BasicVectorCRTP<Derived, Ty, N>& b,
                       Ty epsilon = Ty(1e-6)) noexcept
{
    const auto& da = static_cast<const Derived&>(a);
    const auto& db = static_cast<const Derived&>(b);

    const Ty a_sq = da.Square();
    const Ty b_sq = db.Square();

    // 处理零向量：这里约定任意向量与零向量不算平行
    if (a_sq == Ty{} || b_sq == Ty{})
    {
        return false;
    }

    const Ty dot = da.Dot(db);
    const Ty cos2 = dot * dot;
    const Ty sq = a_sq * b_sq;

    if constexpr (std::is_floating_point_v<Ty>) 
    {
        if (sq == Ty{}) 
        {
            return false;
        }
        // |cos^2 - 1| <= eps 判定为平行
        return std::fabs(static_cast<double>(cos2 - sq)) <= static_cast<double>(epsilon);
    } 
    else 
    {
        // 整型：只在精确满足 cos^2 == 1 的情况下认为平行
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

    if constexpr (std::is_floating_point_v<Ty>) 
    {
        return std::fabs(static_cast<double>(da.Dot(db))) <= static_cast<double>(epsilon);
    } 
    else 
    {
        return da.Dot(db) == Ty{};
    }
}

template <class Derived, typename Ty, std::size_t N, typename FloatTy = float>
    requires std::is_arithmetic_v<Ty> && std::is_floating_point_v<FloatTy>
inline auto Normalize(const BasicVectorCRTP<Derived, Ty, N>& v)
{
    using FloatVector = Vector<FloatTy, N>;
    const auto& dv = static_cast<const Derived&>(v);

    FloatTy len_sq{};
    for (std::size_t i = 0; i < N; ++i)
    {
        const FloatTy c = static_cast<FloatTy>(dv.coordinates[i]);
        len_sq += c * c;
    }

    FloatVector result{};

    if (len_sq == FloatTy(0)) {
        return result;
    }

    const FloatTy inv_len = FloatTy(1) / std::sqrt(len_sq);
    for (std::size_t i = 0; i < N; ++i)
        result.coordinates[i] = static_cast<FloatTy>(dv.coordinates[i]) * inv_len;

    return result;
}



}

#endif // CORE_MATHFUNCTIONS_H