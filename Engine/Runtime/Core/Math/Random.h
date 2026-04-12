
#ifndef CORE_RANDOM_H
#define CORE_RANDOM_H

#include "StaticTools.hpp"
#include "Vector.h"
#include "MathConstant.h"
#include "Detail/RandomDetail.h"

#include <concepts>

namespace Core::Math
{

inline float RandomFloat(Impl::Normal_t tag, float mean, float stddev, float min, float max) 
{
  auto& rng = Impl::GetRandomEngine();
  return Impl::RandomFloat(tag, rng, mean, stddev, min, max);
}

inline float RandomFloat(Impl::Bernoulli_t, float p) 
{
    std::bernoulli_distribution dist(p);
    return dist(Impl::GetRandomEngine());
}

inline int32_t RandomInteger(Impl::Uniform_t, int32_t min, int32_t max) 
{
    std::uniform_int_distribution<int32_t> dist(min, max);
    return dist(Impl::GetRandomEngine());
}

/**
 * @note: 这个函数不是严格的服从正态分布, 只是从正态分布中采样出的浮点数取整之后的结果. 但是性能高
*/
template <std::integral IntegalType>
inline IntegalType RandomInteger(Impl::Normal_t, float mean, float stddev, float min, float max) 
{
	if (!std::isfinite(mean) || !std::isfinite(stddev) ||
		!std::isfinite(min)  || !std::isfinite(max)) [[unlikely]]
	{
		// or throw?
		return std::numeric_limits<IntegalType>::min(); 
	}
	if ((!(stddev > 0.0f) or std::abs(stddev) < 1e-6f)) [[unlikely]] 
	{
		// 当标准差非常接近 0 时(Degenerate), 认为分布退化为一个点质量, 直接返回均值并 clamp
		IntegalType m = static_cast<IntegalType>(std::round(mean));
		return static_cast<IntegalType>(Impl::Clamp(m, static_cast<IntegalType>(min), static_cast<IntegalType>(max)));
	}

	auto& rng = Impl::GetRandomEngine();
	if (min > max) [[unlikely]] 
	{
		std::swap(min, max);
	}
	return Impl::RandomIntegerFaster(Normal, rng, mean, stddev, min, max);
}

template <std::integral IntegalType>
inline IntegalType RandomIntegerPrecisely(Impl::Normal_t, float mean, float stddev, float min, float max) 
{
	if (!std::isfinite(mean) || !std::isfinite(stddev) ||
		!std::isfinite(min)  || !std::isfinite(max)) [[unlikely]]
	{
		// or throw?
		return std::numeric_limits<IntegalType>::min(); 
	}
	if ((!(stddev > 0.0f) or std::abs(stddev) < 1e-6f)) [[unlikely]] 
	{
		// 当标准差非常接近 0 时(Degenerate), 认为分布退化为一个点质量, 直接返回均值并 clamp
		IntegalType m = static_cast<IntegalType>(std::round(mean));
		return static_cast<IntegalType>(Impl::Clamp(m, static_cast<IntegalType>(min), static_cast<IntegalType>(max)));
	}

	auto& rng = Impl::GetRandomEngine();
	if (min > max) [[unlikely]] 
	{
		std::swap(min, max);
	}
	return Impl::RandomIntegerPrecisely(Normal, rng, mean, stddev, min, max);
}

/**
 * @brief 在一个以 center 为中心, radius 为半径的圆内随机生成一个二维向量
 */
inline Vector2D RandomVector2DInDisk(Vector2D center, float radius) 
{
	return Vector2D::OneVector();
}

/**
 * @brief 在一个以 center 为中心, radius 为半径的圆内随机生成一个二维向量
 */
inline Vector2D RandomVector2DInUnitDisk(Vector2D center, float radius) 
{
	return Vector2D::OneVector();
}


inline Vector3D RandomVector3DInSphere(Vector3D center, float radius) 
{
	return Vector3D::OneVector();
}

inline Vector3D RandomVector3DInHemisphere(Vector3D center, float radius) 
{
	return Vector3D::OneVector();
}

inline Vector3D RandomVector3DInUnitSphere(Vector3D center, float radius) 
{
	return Vector3D::OneVector();
}

inline Vector3D RandomVector3DInUnitHemisphere(Vector3D center, float radius) 
{
	return Vector3D::OneVector();
}

/**
 * @brief 生成概率分布为cos(theta)/pi的随机方向
 */
inline Vector3D RandomCosineDirection() 
{
	return Vector3D::OneVector();
}

/**
 * @brief 在球体外对球体随机采样
 */
inline Vector3D RandomToSphere(float radius, float distance_squared)
{

}
#endif // CORE_RANDOM_H