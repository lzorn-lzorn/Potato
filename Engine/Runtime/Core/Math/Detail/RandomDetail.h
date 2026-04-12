
#ifndef CORE_RANDOM_DETAIL_H
#define CORE_RANDOM_DETAIL_H

#include "FunctionsDetail.h"
#include "MathConstant.h"

#include <random>
#include <cmath>
#include <limits>
#include <algorithm>
#include <utility>
#include <cstdint>
#include <vector>
#include <map>

namespace Impl
{
	

inline void NormalizeIntBounds(int& lo, int& hi) noexcept {
  if (lo > hi) std::swap(lo, hi);
}

inline std::mt19937& GetRandomEngine() {
    thread_local static std::mt19937 gen(std::random_device{}());
    return gen;
}


/**
 * @about 截断正态分布: (https://en.wikipedia.org/wiki/Truncated_normal_distribution?#Generating_values_from_the_truncated_normal_distribution)
*/

/**
 * 
 * @about std::erf 是C++11 引入的数学误差函数,
 *     erf(x) = (2/sqrt(pi)) * integral from 0 to x of exp(-t^2) dt
 * C++ 标准要求 std::erf 的精度与 C 标准库一致, 通常实现采用切比雪夫逼近或有理函数逼近
 * @about 互补误差函数 erfc(x) = 1 - erf(x), 也可用于计算 Phi(x) = 0.5 * erfc(-x/sqrt(2))
 *     erfc(x) = (2/sqrt(pi)) * integral from x to infinity of exp(-t^2) dt
 * @note 当 x 比较大时, erf(x) 接近 1, 直接用 1 - erf(x) 会因灾难性抵消损失精度
*/
inline float ErfInv(float y) noexcept {
	/**
	 * @note erfinv(y) 在 y 接近 ±1 时, 斜率趋于无穷大, 直接计算会有数值不稳定问题.
	 *     如果 y = ±1, erfinv(y) 的值是 ±infinity/Nan
	 *     因此将 y 限制在 (-1+eps, 1-eps) 范围内
	 */
  	constexpr float eps = 1e-7f;
  	y = Clamp(y, -1.0f + eps, 1.0f - eps);

	/**
	 * @note: 魔法数字 0.147f 由 Sergei Winitzki 给出
	 *  其给定 erfinv 一个初始值 x0, 并用牛顿迭代修正两次(Newton-Raphson steps), 这里的 a 是该近似里为了让误差更小的常数
	 *
	 * 	erfinv(y) ~=
	 * 	  sign(y)
	 * 	  	 \times
	 * 	  sqrt(
	 * 	  	  sqrt(
	 * 	  	     (2/(pi*a) + ln(1-y^2)/2)^2 - ln(1-y^2)/a) - (2/(pi*a)+ln(1-y^2)/2
	 *                                              ^
	 *                                            ln1my2
	 * 	  	  )
	 * 	  )
	 *
	 * @about std::copysign  是 C++11 起引入的用于复制一个数值的符号到另一个数值上的函数
	 *   即返回一个大小等于第一个参数, 但符号与第二个参数相同的值
	 *
	 *   float       copysign( float mag, float sgn );
	 *   double      copysign( double mag, double sgn );
	 *   long double copysign( long double mag, long double sgn );
	 *   Promoted    copysign( Arithmetic1 mag, Arithmetic2 sgn );
	 *
	 *   当 mag 和 sgn 等于 Nan 时, 行为未定义; (可以区分 IEEE754 的 ±0).
	 *   Eg: 符号函数的实现: 直接 return x > 0 ? 1 : -1; 的问题
	 *     - 无法正确处理零和 Nan;
	 *     - 会触发 Wsign-compare 警告;
	 *     - if-else 不利于分支预测, std::copysign 内部通常实现为位操作, 性能更好且无分支.
	 *
	 *      template<typename T>
	 *      T sign(T x) { return std::copysign(T(1), x); }
	 */

	constexpr float a = 0.147f;
	float ln1my2 = std::log(1.0f - y * y);
	float t1 = 2.0f / (static_cast<float>(M_PI) * a) + 0.5f * ln1my2;
	float t2 = 1.0f / a * ln1my2;
	float x = std::copysign(std::sqrt(std::max(0.0f, std::sqrt(std::max(0.0f, t1 * t1 - t2)) - t1)), y);

	/**
	 * @note:  牛顿迭代修正两次(Newton-Raphson steps):
	 *     Solve erf(x) - y = 0
	 *     x_{n+1} = x_n - (erf(x_n)-y) / (2/sqrt(pi) * exp(-x_n^2))
	 */
	constexpr float two_over_sqrt_pi = 2/sqrt_pi;
	for (int i = 0; i < 2; ++i) {
		float err = static_cast<float>(std::erf(static_cast<double>(x))) - y;
		float der = two_over_sqrt_pi * std::exp(-x * x);
		x -= err / der;
	}
	return x;
}

/**
 * @brief 标准正态分布的逆累计分布函数Inverse CDF, Phi^{-1}(p) = sqrt(2) * erfinv(2p-1)
 * @reference: https://en.wikipedia.org/wiki/Normal_distribution
*/
inline double NormalCDF01(double x) noexcept {
	constexpr double inv_sqrt2 = inv_sqrt2; // 1/sqrt(2)
	double xd = static_cast<double>(x);
	// double v = 0.5 * (1.0 + std::erf(xd * inv_sqrt2));
	if (xd >= 0) 
	{
		return 0.5 * (1.0 + std::erf(xd * inv_sqrt2));
	} else {
		// 对于负 x，利用 \Phi(-x) = 1 - \Phi(x), 使用 erfc 稳定计算
		return 0.5 * std::erfc(-xd * inv_sqrt2);
	}
}

inline float NormalInvCDF01(float p) noexcept {
    constexpr float eps = epsilon;
    p = std::clamp(p, 0.0f, 1.0f);
    if (p == 0.0f) return -infinity;
    if (p == 1.0f) return  infinity;
    // 避免硬截断，改用更小的 eps 让 ErfInv 自然处理尾部
    float safe_p = std::clamp(p, eps, 1.0f - eps);
    float y = 2.0f * safe_p - 1.0f;
    return sqrt2 * ErfInv(y);
}

/**
 * @about DiscretizedCDF: 计算一个整数 k 在以 mean 和 stddev 为参数的正态分布下被四舍五入到 k 的概率质量
 *     这个函数的性能较低, 但能更精确地计算出当 k 在分布的尾部时, 由于连续正态分布的概率密度非常小, 直接用 
 *     RandomFloat 生成一个浮点数再取整可能会因为数值精度问题导致 k 的概率被低估为 0.
 * 
 *         P(K=k) = Phi((k+0.5-mean)/stddev) - Phi((k-0.5-mean)/stddev)
 * 
 *     先从连续正态采样 (X), 再对 (X) 做 round 到最近整数 得到 (K), 再将 support set 设置为 [min, max] 进行截断.
 *     本质上是是在采样条件分布:
 *       
 *         P(K = k | min <= K <= max) = P(K=k) / sum_{i=min}^{max} P(K=i)
 * 
*/
inline float DiscretizedNormalPMF(int k, float mean, float stddev) noexcept 
{
	const float hi = ( (static_cast<float>(k) + 0.5) - mean ) / stddev;
	const float lo = ( (static_cast<float>(k) - 0.5) - mean ) / stddev;
	
	auto Phi = [inv_sqrt2](double x) 
	{
        return 0.5 * std::erfc(-x * inv_sqrt2);
    };
    auto Survival = [inv_sqrt2](double x) 
	{
        return 0.5 * std::erfc(x * inv_sqrt2);
    };

    double p;
    if (hi <= 0.0) 
	{
        // 均在左尾
        p = Phi(hi) - Phi(lo);
    } else if (lo >= 0.0) 
	{
        // 均在右尾：用生存函数避免相近数相减
        p = Survival(lo) - Survival(hi);
    } else 
	{
        // 跨越零点，差值较大，直接相减精度足够
        p = Phi(hi) - Phi(lo);
    }
    return std::max(0.0, p);
}

/**
 * @brief 标准正态分布的逆累计分布函数Inverse CDF; Phi^{-1}(p) = sqrt(2) * erfinv(2p-1)
 * @reference Acklam 算法: https://stackedboxes.org/2017/05/01/acklams-normal-quantile-function/
 * @param 默认 p 满足 0 < p < 1 
 */
inline double NormalInvCDF01Precisely(double p) noexcept {
	constexpr double eps = epsilon;
	p = Clamp(p, eps, 1.0 - eps);
	// 对于 p < 0.02425 或 p > 0.97575 使用尾部逼近，否则用中心逼近
    double q = p - 0.5;
    double r, val;
    if (std::abs(q) <= 0.425) {
        // 中心区域
        r = 0.180625 - q * q;
        val = q * (((((((r * 2509.0809287301226727 +
                          33430.575583588128105) * r +
                          67265.770927008700853) * r +
                          45921.953931549871457) * r +
                          13731.693765509461125) * r +
                          1971.5909503065514427) * r +
                          133.14166789178437745) * r +
                          3.387132872796366608)
                  / (((((((r * 5226.495278852854561 +
                          28729.085735721942674) * r +
                          39307.89580009271061) * r +
                          21213.794301586595867) * r +
                          5394.1960214247511077) * r +
                          687.1870074920579083) * r +
                          42.313330701600911252) * r + 1.0);
    } else {
        // 尾部区域
        if (q > 0) {
            r = 1.0 - p;
        } else {
            r = p;
        }
        r = std::sqrt(-std::log(r));
        if (r <= 5.0) {
            r -= 1.6;
            val = (((((((r * 0.00077454501427834139 +
                          0.02272383098914782) * r +
                          0.2417807251774506) * r +
                          1.2704582524523684) * r +
                          3.6478483247632045) * r +
                          5.769497221460691) * r +
                          4.630337846156545) * r +
                          1.4234371107496835)
                  / (((((((r * 0.0000000010507500716 +
                          0.000005475938084995) * r +
                          0.000015198650539458) * r +
                          0.00014810397642748) * r +
                          0.0006897673349851) * r +
                          0.001456638290981) * r +
                          0.001028135261854) * r + 1.0);
        } else {
            r -= 5.0;
            val = (((((((r * 2.0103343992922881e-7 +
                          2.7115555687434876e-5) * r +
                          0.0012426609473880784) * r +
                          0.026532189526576123) * r +
                          0.29656057182850487) * r +
                          1.7848265399172913) * r +
                          5.463784911164114) * r +
                          6.657904643501103)
                  / (((((((r * 2.0442631033899397e-15 +
                          1.4215117583164459e-7) * r +
                          1.8463183175100547e-5) * r +
                          0.000786869310145) * r +
                          0.01487536129085) * r +
                          0.13692988092273) * r +
                          0.599832206555) * r + 1.0);
        }
        if (q < 0.0) val = -val;
    }
    return val;
}


struct Distribution_t
{

};

struct Uniform_t : public Distribution_t
{
	Uniform_t() = default;
	constexpr explicit Uniform_t(float min, float max) : min(min), max(max) {}

	float FloatValue() const noexcept
	{
		return ValueImpl<float>();
	}

	float FloatValue() noexcept
	{
		return ValueImpl<float>();
	}
	
	int32_t IntValue() const noexcept
	{
		return ValueImpl<int32_t>();
	}

	int32_t IntValue() noexcept
	{
    	return ValueImpl<int32_t>();
	}

	float min = 0.0f;
	float max = 1.0f;

private:
	template <typename Ty>
	Ty ValueImpl() noexcept
	{
		std::uniform_int_distribution<Ty> dist(static_cast<Ty>(min), static_cast<Ty>(max));
    	return dist(GetRandomEngine());
	}
};

struct Normal_t : public Distribution_t
{
private:
	using PreciseCacheKey = std::tuple<double, double, int, int>;
	template <std::integral IntegalType>
	struct PreciseCache 
	{
		/**
		 * @about prefix, prefix[i] = \Sum_{j=0}^{i} p_{min+j}
		 * 	  其中 p_k = P(K=k) 是离散化正态分布在整数 k 处的概率质量函数值
		 *    prefix.back(), 就是窗口内总质量 
		 * 
		 * 使用 double 为了降低误差
		*/
		std::vector<double> prefix; // 存储累计概率
		double sum = 0.0;
		bool valid = false;

		void Rebuild(double mean, double stddev, IntegalType min, IntegalType max)
		{
			// 防止中间计算溢出, 使用 uint64_t
			const uint64_t n = static_cast<uint64_t>(max - min + 1);
			prefix.resize(n);
			sum = 0.0;
			for (size_t i = 0; i < n; ++i) {
				int k = min + static_cast<int>(i);
				double pk = StableDiscretizedNormalPMF(k, mean, stddev);
				sum += pk;
				prefix[i] = sum;
			}
			// 处理一种病态情况: sum 非常小甚至为 0, 可能是因为 mean 和 stddev 导致分布的质量
			// 几乎完全落在整数范围之外了. 这时只能退化为一个点质量了, 返回离 mean 最近的那个整数
			valid = (sum > 0.0) && std::isfinite(sum);
		}
	};

public:	
	Normal_t() = default;
	constexpr explicit Normal_t(float mean, float stddev) : mean(mean), stddev(stddev) {}

	float FloatValue(float min, float max) const noexcept
	{
		return FloatValueImpl(min, max);
	}

	float FloatValue(float min, float max) noexcept
	{
		return FloatValueImpl(min, max);
	}

	int32_t IntegerValue(float min, float max) const noexcept
	{
		return IntegerValueImpl(min, max);
	}

	int32_t IntegerValue(float min, float max) noexcept
	{
		return IntegerValueImpl(min, max);
	}

	float mean = 0.0f;
	float stddev = 1.0f;

private:
	int32_t IntegerValueImpl(float min, float max) noexcept
	{
		std::normal_distribution<float> nd(mean, stddev);
		float x = nd(rng);
		int32_t i = static_cast<int32_t>(std::round(x));
		return std::clamp(i, static_cast<int32_t>(min), static_cast<int32_t>(max));
	}

	float FloatValueImpl(float min, float max) noexcept
	{
		if (!std::isfinite(mean) || !std::isfinite(stddev) ||
			!std::isfinite(min)  || !std::isfinite(max)) [[unlikely]]
		{
			return std::numeric_limits<float>::quiet_NaN();
		}

		if (!(stddev > 0.0f)) [[unlikely]]
		{
			// Degenerate: treat as point mass at mean, then clamp to [min,max]
			if (min > max) std::swap(min, max);
			return Clamp(mean, min, max);
		}

		if (min > max) [[unlikely]]
		{
			std::swap(min, max);
		}

		// If interval is empty or nearly empty, return the clamped mean as best effort.
		if (!(min < max)) [[unlikely]]
		{
			return Clamp(mean, min, max);
		}

		auto rng = GetRandomEngine();
		const float a = (min - mean) / stddev;
		const float b = (max - mean) / stddev;

		// If [a,b] is extremely wide, rejection will accept almost always.
		// If [a,b] is narrow or far in tail, rejection can be slow.
		// We'll try rejection for a limited number of iterations then fallback to inverse-CDF method.

		std::normal_distribution<float> nd { 0.0f, 1.0f };

		// Heuristic: cap attempts. 64 is usually plenty; if not, inverse-CDF will finish.
		constexpr int max_attempts = 64;

		for (int i = 0; i < max_attempts; ++i) 
		{
			float z = nd(rng);
			if (z >= a && z <= b) 
			{
				return mean + stddev * z;
			}
		}

		// Fallback: inverse transform on truncated CDF:
		// p = Phi(a) + u*(Phi(b)-Phi(a)),  z = Phi^{-1}(p)
		float phi_a = NormalCDF01(a);
		float phi_b = NormalCDF01(b);

		// Handle pathological cases where phi_a==phi_b due to underflow/precision (extreme tail + narrow interval).
		// In that case, best effort: return nearest bound in original scale depending on which side has mass.
		if (!(phi_b > phi_a))
		{
			// pick the closer bound to mean in standardized space
			float z = (std::abs(a) < std::abs(b)) ? a : b;
			return Clamp(mean + stddev * z, min, max);
		}

		std::uniform_real_distribution<float> ud(0.0f, 1.0f);
		float u = ud(rng);
		float p = phi_a + u * (phi_b - phi_a);
		float z = NormalInvCDF01(p);

		float x = mean + stddev * z;
		return Clamp(x, min, max);
	}

	IntegalType IntegerValuePreciselyImpl(float min, float max) noexcept
	{
		auto rng = GetRandomEngine();
		// 对浮点数实现hash非常困难, 不能使用 std::unoredered_map
		static std::map<PreciseCacheKey, PreciseCache<IntegalType>> cache;
		PreciseCacheKey key = std::make_tuple(mean, stddev, min, max);

		auto it = cache.find(key);
		if (it == cache.end() || !it->second.valid) 
		{
			// 没有缓存或者缓存无效, 重新计算
			PreciseCache<IntegalType> new_cache;
			new_cache.Rebuild(mean, stddev, min, max);
			it = cache.emplace(key, std::move(new_cache)).first;
		}
		const PreciseCache<IntegalType>& cache = it->second;

		if (!cache.valid) [[unlikely]] 
		{
			int km = static_cast<int>(std::llround(mean));
			return static_cast<IntegalType>(std::clamp(km, min, max));
		}

		std::uniform_real_distribution<double> ud(0.0, sum);
		double u = ud(rng);

		/**
		 * @note lower_bound 找到第一个满足 prefix[idx] >= u 的位置
		 * 
		 * [0, prefix{min}, prefix{min+1}, ..., prefix{min+1}]
		 * 
		 * 而 prefix[i] 正是第 i 段的右端点. u 落在哪一段, 就返回哪个整数.
		 * 即逆变换采样(discrete inverse transform sampling): 先从均匀分布采样一个 u, 
		 * 再在 prefix 中找到第一个 >= u 的位置 idx, 那么对应的整数就是 min + idx
		 * 
		 *     K = min(k): C(k) >= u
		 * 
		 * 其中 C(k) = \Sum_{j=min}^{k} p_j 是离散化正态分布的累计分布函数
		*/
		auto it = std::lower_bound(prefix.begin(), prefix.end(), u);
		int32_t idx = static_cast<int32_t>(it - prefix.begin());
		return min + idx;
	}

};

struct Bernoulli_t : public Distribution_t 
{

	float p;
	uint32_t n;
	float Mean() const
	{
		return static_cast<float>(n) * p;
	}
	float Variance() const
	{
		return static_cast<float>(n) * p * (1.0f - p);
	}

};

struct Exponential_t : public Distribution_t
{

	float lambda;
	float Mean() const
	{
		return 1.0f / lambda;
	}
	float Variance() const
	{
		return 1.0f / (lambda * lambda);
	}
};

} // namespace Impl

inline constexpr Impl::Uniform_t Uniform;
inline constexpr Impl::Normal_t Normal;
// 暂时不支持二项分布
// inline constexpr Impl::Bernoulli_t Bernoulli;

// ---------- 辅助调度函数 ----------
template<typename DistributionTag, Arithmetic Ty>
Ty RandomNumber(DistributionTag tag, Ty a, Ty b) 
{
    if constexpr (std::is_floating_point_v<Ty>) 
	{
        return static_cast<Ty>(RandomFloat(tag, static_cast<float>(a), static_cast<float>(b)));
    } 
	else if constexpr (std::is_integral_v<Ty>) 
	{
        return static_cast<Ty>(RandomInteger(tag, static_cast<int>(a), static_cast<int>(b)));
    }
}

/**
 * @brief 随机生成一个 N 维向量
 * @param tag: 分布类型标签, 目前支持 Uniform_t 和 Normal_t
 * @param is_normalized: 是否对生成的向量进行归一化
 * @param a, b: 分布参数, 对于 Uniform 是区间 [a,b], 对于 Normal 是 mean 和 stddev
 */
template <Arithmetic Ty, size_t N, typename DistributionTag>
inline Vector<Ty, N> RandomVector(DistributionTag tag, bool is_normalized = false, Ty a = Ty(0), Ty b = Ty(1)) 
{
	static_assert(std::is_same_v<DistributionTag, Uniform_t> || std::is_same_v<DistributionTag, Normal_t>, "Unsupported distribution tag, 目前只支持高斯分布和均匀分布");
	Vector<Ty, N> v;
	StaticFor<N>([&](auto i) 
	{
		constexpr size_t idx = decltype(i)::value;
		v[idx] = Impl::RandomNumber(tag, a, b);
	});
	if (is_normalized) 
	{
		v = v.Normalized();
	}
	return v;
}



#endif