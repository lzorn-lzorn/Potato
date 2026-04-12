
#ifndef CORE_FUNCTIONS_DETAIL_H
#define CORE_FUNCTIONS_DETAIL_H

#include "Vector.h"

#include <cmath>
#include <limits>
#include <cmath>


namespace Core::Math::Impl
{

template <typename Ty>
inline Ty Clamp(Ty value, Ty min, Ty max)
{
	return std::clamp(value, min, max);
}

template<typename Vec, std::size_t N>
	requires (Vec::dimensions >= 1) && std::is_arithmetic_v<typename Vec::value_type>
Vector<typename Vec::value_type, N> Clamp(const Vector<typename Vec::value_type, N>& value,
                    const Vector<typename Vec::value_type, N>& min,
                    const Vector<typename Vec::value_type, N>& max) {
    Vector<typename Vec::value_type, N> result;
    for (std::size_t i = 0; i < N; ++i) 
	{
        result[i] = Clamp(value[i], min[i], max[i]);
    }
    return result;
}

template <typename Ty>
inline Ty Lerp(Ty a, Ty b, Ty t)
{
	return std::lerp(a, b, t);
}

template<typename Vec, std::size_t N>
	requires (Vec::dimensions >= 1) && std::is_arithmetic_v<typename Vec::value_type>
inline Vector<typename Vec::value_type, N> Lerp(const Vector<typename Vec::value_type, N>& a, const Vector<typename Vec::value_type, N>& b, typename Vec::value_type t)
{
    Vector<typename Vec::value_type, N> result;
    for (std::size_t i = 0; i < N; ++i) 
	{
        result[i] = Lerp(a[i], b[i], t);
    }
    return result;
}
}


#endif