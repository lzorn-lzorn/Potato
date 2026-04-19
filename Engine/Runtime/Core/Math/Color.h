#pragma once
#include "Vector.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>
#include <string_view>
#include <sstream>
#include <iomanip>
#include <charconv>
#include <random>

/**
 * @about Color: 
 * 		在物理世界中, 颜色本质上一种光能的体现, 也就是光的辐射度. 在实际的 BPR 中, 我们往往使用
 *    　一个三维向量来表示颜色的 RGB 三个分量
 * 
 * @about sRGB
 * 		由于人眼的生物特性, 导致我们对不同波长的光有不同的敏感度, 所以对于实际的线性 RBG 颜色值,
 * 		我们通常会进行一个非线性变换(伽马校正), 将其映射到一个更符合人眼感知的空间(如 sRGB), 以
 * 		获得更好的视觉效果和更高的色彩精度.
 * 
 *  	sRBG 是一种非线性的颜色编码, 其传递函数由 IEC 61966-2-1:1999 标准定义.
 * 
 *  其变换为:
 *  $$
 *  C_{\text{sRGB}} = \begin{cases}
 * 		12.92 \cdot C_{\text{linear}} & \text{if } C_{\text{linear}} \le 0.0031308 \\
 * 		1.055 \cdot C_{\text{linear}}^{1/2.4} - 0.055 & \text{otherwise}
 *  \end{cases}
 *  $$
 *  通过 Linear Color 计算得到的 sRBG 的方式叫做伽马矫正 Gamma Correction, 其目的就是为了
 *  更好地适应人眼的感知特性, 使得在较暗的颜色区域能够有更好的细节表现, 同时在较亮的颜色区域能
 *  够避免过曝.
 * 
 *  逆变换为
 *  $$
 *  C_{\text{linear}} = \begin{cases}
 *  	\dfrac{C_{\text{sRGB}}}{12.92} & \text{if } C_{\text{sRGB}} \le 0.04045 \\
 *  	\left(
 *  		\dfrac{C_{\text{sRGB}} + 0.055}{1.055}
 * 		\right)^{2.4} & \text{otherwise}
 *  \end{cases}
 *  $$
 * 
 *  	所以 sRGB 是为了满足人眼而进行的校准, 并不是物理意义上上的光辐射度. 所以在进行光照计算
 *      时, 先使用 Linear Color 计算, 再将其转换到 sRBG 空间(现代渲染管线可以自动完成)
 * 
 *  	一个完整流程是: 对于一张纹理其可能源于各种 DDC 工具, 其本身就是 sRBG (Albedo Map), 
 *      Albedo Map -> Linear Color (进行光照计算) -> 色调映射 (Tonemapping) 映射到 LDR 颜
 * 		色 [0,1] -> 伽马矫正得到 sRBG (输出到屏幕)
 * 
 *  	需要注意的是对于 HDR 显示器, 不需要幂律的伽马矫正, 而是使用的是 PQ 或者 HLG 传递函数, 
 * 		所以得到的颜色空间也不再是简单 sRGB 颜色空间. 
 * 
 * 		伽马矫正源于最早的 CRT 显示器, 也就是基于电子发射的显示技术, 其亮度与输入电压之间的关系
 * 		是非线性的, 大约是幂律关系. 所以为了在 CRT 上正确显示颜色, 需要进行伽马矫正. 但是 HDR
 *   	使用的是 OLED 或者 LCD, 其发光机制是线性的, 不能使用 CRT 的伽马矫正, 需要使用 PQ 
 *   	(Perceptual Quantizer，感知量化)或者 HLG (Hybrid Log-Gamma, 混合对数伽马) 这类更适
 *   	合 HDR 显示的传递函数.
 * 
 * @about 色调映射 (Tonemapping):
 * 		色调映射是离线渲染和实时图形中连接 HDR 光照计算与 LDR 显示设备的核心操作. 其解决的问题
 * 		如何将范围可能是 [0, \inf) 的物理亮度，压缩进显示器只能显示的 [0, 1] 范围内，同时保留视觉
 * 		上的对比度和色彩
 * 		
 *  Reinhard 色调映射
 *  $$
 *  C_{\text{mapped}} = \frac{C_{\text{linear}}}{C_{\text{linear}} + 1}
 *  $$
 *  
 *  带曝光的 Reinhard 色调映射 (高光部分容易偏向纯白)
 *  $$
 *  C_{\text{mapped}} = 
 *  	\frac{
 *  		C_{\text{linear}} \cdot E
 *  	}{
 *  		C_{\text{linear}} \cdot E + 1
 *  	}
 *  $$ 
 *  
 * 	ACES 色调映射, Narkowicz 拟合 (ACES函数)
 * 
 *   AgX 色调映射, Narkowicz 拟合 (AgX函数)
 * 
 */
namespace Core::Math {

struct Color3D;
struct Color4D;

class LinearColor4D;
class LinearColor3D 
{
public:
    using vector_type = Vector3D<float>;
    using value_type = float;

    // ---------- 构造与转换 ----------
    constexpr LinearColor3D() noexcept : data{0.0f, 0.0f, 0.0f} {}
    constexpr LinearColor3D(float r, float g, float b) noexcept : data{r, g, b} {}
    explicit constexpr LinearColor3D(float gray) noexcept : data{gray, gray, gray} {}
    explicit constexpr LinearColor3D(const vector_type& v) noexcept : data(v) {}
    explicit constexpr LinearColor3D(const LinearColor4D& c) noexcept;

    [[nodiscard]] constexpr LinearColor4D WithAlpha(float alpha = 1.0f) const noexcept;
    // 允许从其他算术类型的 Vector 转换
    template <typename U>
        requires std::is_arithmetic_v<U>
    explicit constexpr LinearColor3D(const Vector3D<U>& v) noexcept 
        : data{static_cast<float>(v.X()), 
                static_cast<float>(v.Y()), 
                static_cast<float>(v.Z())} {}

    // ---------- 分量访问 ----------
    [[nodiscard]] constexpr float R() const noexcept { return data.X(); }
    [[nodiscard]] constexpr float G() const noexcept { return data.Y(); }
    [[nodiscard]] constexpr float B() const noexcept { return data.Z(); }
    constexpr void SetR(float r) noexcept { data.X() = r; }
    constexpr void SetG(float g) noexcept { data.Y() = g; }
    constexpr void SetB(float b) noexcept { data.Z() = b; }

    [[nodiscard]] constexpr float  operator[](std::size_t i) const noexcept { return data[i]; }
    constexpr float& operator[](std::size_t i) noexcept { return data[i]; }

    // 获取底层向量（用于需要向量操作的特殊场景）
    [[nodiscard]] constexpr const vector_type& AsVector() const noexcept { return data; }

    // ---------- 算术运算符（转发给 Vector） ----------
    constexpr LinearColor3D& operator+=(const LinearColor3D& rhs) noexcept 
	{
        data += rhs.data;
        return *this;
    }
    constexpr LinearColor3D& operator-=(const LinearColor3D& rhs) noexcept 
	{
        data -= rhs.data;
        return *this;
    }
    constexpr LinearColor3D& operator*=(const LinearColor3D& rhs) noexcept 
	{
        data *= rhs.data;
        return *this;
    }
    constexpr LinearColor3D& operator/=(const LinearColor3D& rhs) noexcept 
	{
        data /= rhs.data;
        return *this;
    }
    constexpr LinearColor3D& operator*=(float s) noexcept 
	{
        data *= s;
        return *this;
    }
    constexpr LinearColor3D& operator/=(float s) noexcept 
	{
        data /= s;
        return *this;
    }

    constexpr LinearColor3D operator-() const noexcept 
	{
        return LinearColor3D(-data);
    }

    // 二元运算符（使用 friend 以便隐式转换标量）
    friend constexpr LinearColor3D operator+(LinearColor3D lhs, const LinearColor3D& rhs) noexcept 
	{
        return lhs += rhs;
    }
    friend constexpr LinearColor3D operator-(LinearColor3D lhs, const LinearColor3D& rhs) noexcept 
	{
        return lhs -= rhs;
    }
    friend constexpr LinearColor3D operator*(LinearColor3D lhs, const LinearColor3D& rhs) noexcept 
	{
        return lhs *= rhs;
    }
    friend constexpr LinearColor3D operator/(LinearColor3D lhs, const LinearColor3D& rhs) noexcept 
	{
        return lhs /= rhs;
    }
    friend constexpr LinearColor3D operator*(LinearColor3D lhs, float rhs) noexcept 
	{
        return lhs *= rhs;
    }
    friend constexpr LinearColor3D operator*(float lhs, LinearColor3D rhs) noexcept 
	{
        return rhs *= lhs;
    }
    friend constexpr LinearColor3D operator/(LinearColor3D lhs, float rhs) noexcept 
	{
        return lhs /= rhs;
    }

    friend constexpr bool operator==(const LinearColor3D& lhs, const LinearColor3D& rhs) noexcept 
	{
        return lhs.data == rhs.data;
    }

    // ---------- 颜色专属功能 ----------

	/**
     * @brief 从 RGBE 格式解码（Radiance HDR 格式）
     * @param rgbe 4 字节 RGBE 数据（R, G, B, Exponent）
     * @return 线性 HDR 颜色
     */
    static constexpr LinearColor3D FromRGBE(const uint8_t rgbe[4]) noexcept 
	{
        if (rgbe[3] == 0) {
            return LinearColor3D::Zero();
        }
        const float scale = std::ldexp(1.0f, static_cast<int>(rgbe[3]) - 128); // 2^(E-128)
        return LinearColor3D(static_cast<float>(rgbe[0]) * scale,
                       static_cast<float>(rgbe[1]) * scale,
                       static_cast<float>(rgbe[2]) * scale) / 255.0f;
    }

	/**
     * @brief 将当前 HDR 颜色编码为 RGBE 格式
     * @param out_rgbe 输出 4 字节数组
     */
    void ToRGBE(uint8_t out_rgbe[4]) const noexcept 
	{
        float v = std::max({R(), G(), B()});
        if (v < 1e-32f) {
            out_rgbe[0] = out_rgbe[1] = out_rgbe[2] = out_rgbe[3] = 0;
            return;
        }
        int e;
        float scale = std::frexp(v, &e) * 256.0f / v;
        out_rgbe[0] = static_cast<uint8_t>(std::clamp(R() * scale, 0.0f, 255.0f));
        out_rgbe[1] = static_cast<uint8_t>(std::clamp(G() * scale, 0.0f, 255.0f));
        out_rgbe[2] = static_cast<uint8_t>(std::clamp(B() * scale, 0.0f, 255.0f));
        out_rgbe[3] = static_cast<uint8_t>(e + 128);
    }

	/**
     * @brief 从十六进制字符串构造（支持 #RGB, #RRGGBB, RRGGBB, RGB）
     * @param hex 字符串，大小写不敏感
     * @return 线性颜色（注意：输入被解释为 sRGB，返回线性值）
     */
    static LinearColor3D FromHex(std::string_view hex) 
	{
        std::string_view trimmed = hex;
        if (trimmed.starts_with('#'))
            trimmed.remove_prefix(1);
        
        uint32_t int_value = 0;
        if (trimmed.size() == 3) 
		{
            // RGB -> 每个字符重复一次
            auto fromHexChar = [](char c) -> uint8_t {
                if (c >= '0' && c <= '9') return c - '0';
                if (c >= 'a' && c <= 'f') return c - 'a' + 10;
                if (c >= 'A' && c <= 'F') return c - 'A' + 10;
                return 0;
            };
            uint8_t r = fromHexChar(trimmed[0]);
            uint8_t g = fromHexChar(trimmed[1]);
            uint8_t b = fromHexChar(trimmed[2]);
            int_value = (r << 20) | (r << 16) | (g << 12) | (g << 8) | (b << 4) | b;
        } else if (trimmed.size() == 6) 
		{
            auto result = std::from_chars(trimmed.data(), trimmed.data() + trimmed.size(), int_value, 16);
            if (result.ec != std::errc{}) 
			{
                int_value = 0;
            }
        } else 
		{
            // 非法长度，返回黑色
            return LinearColor3D::Black();
        }

        uint8_t r = (int_value >> 16) & 0xFF;
        uint8_t g = (int_value >> 8) & 0xFF;
        uint8_t b = int_value & 0xFF;
        
        // 输入为 sRGB 编码，需转为线性
        return FromByteSRGB(r, g, b);
    }

	/**
     * @brief 从 8-bit sRGB 分量构造线性颜色
     */
    static LinearColor3D FromByteSRGB(uint8_t r, uint8_t g, uint8_t b) 
	{
        LinearColor3D srgb(r / 255.0f, g / 255.0f, b / 255.0f);
        return srgb.ToLinear();
    }
    /**
     * @brief 计算相对亮度（Rec.709 系数）
     */
    [[nodiscard]] constexpr float Luminance() const noexcept 
	{
        return 0.212671f * R() + 0.715160f * G() + 0.072169f * B();
    }

    /**
     * @brief 判断是否为黑色（考虑浮点误差）
     */
    [[nodiscard]] bool IsBlack(float eps = 1e-6f) const noexcept 
	{
        return data.Square() <= eps * eps;
    }

    /**
     * @brief 逐分量钳位
     */
    [[nodiscard]] LinearColor3D Clamp(float minVal = 0.0f, float maxVal = INFINITY) const noexcept 
	{
        return LinearColor3D(std::clamp(R(), minVal, maxVal),
                       std::clamp(G(), minVal, maxVal),
                       std::clamp(B(), minVal, maxVal));
    }

    /**
     * @brief 简单伽马校正（幂函数）
     */
    [[nodiscard]] LinearColor3D GammaCorrect(float gamma = 2.2f) const noexcept 
	{
        float inv = 1.0f / gamma;
        return LinearColor3D(std::pow(R(), inv),
                       std::pow(G(), inv),
                       std::pow(B(), inv));
    }

    /**
     * @brief 线性 RGB → sRGB 转换（带线性段的标准公式）
     */
    [[nodiscard]] LinearColor3D ToSRGB() const noexcept 
	{
        auto LinearToSRGB = [](float x) -> float 
		{
            if (x <= 0.0031308f)
                return 12.92f * x;
            else
                return 1.055f * std::pow(x, 1.0f / 2.4f) - 0.055f;
        };
        return LinearColor3D(LinearToSRGB(R()), LinearToSRGB(G()), LinearToSRGB(B()));
    }

    /**
     * @brief sRGB → 线性 RGB 转换
     */
    [[nodiscard]] LinearColor3D ToLinear() const noexcept 
	{
        auto sRGBToLinear = [](float x) -> float 
		{
            if (x <= 0.04045f)
                return x / 12.92f;
            else
                return std::pow((x + 0.055f) / 1.055f, 2.4f);
        };
        return LinearColor3D(sRGBToLinear(R()), sRGBToLinear(G()), sRGBToLinear(B()));
    }

    /**
     * @brief ACES 电影色调映射（Narkowicz 拟合）
     */
    [[nodiscard]] LinearColor3D ACES() const noexcept 
	{
        auto aces = [](float x) 
		{
            const float a = 2.51f, b = 0.03f, c = 2.43f, d = 0.59f, e = 0.14f;
            return (x * (a * x + b)) / (x * (c * x + d) + e);
        };
        return LinearColor3D(aces(R()), aces(G()), aces(B())).Clamp();
    }

	[[nodiscard]] LinearColor3D AgX() const noexcept
	{
		auto agx = [](float x) 
		{
			const float a = 0.22f, b = 0.30f, c = 0.10f, d = 0.20f, e = 0.01f;
			return (x * (a * x + b)) / (x * (c * x + d) + e);
		};
		return LinearColor3D(agx(R()), agx(G()), agx(B())).Clamp();
	}

	[[nodiscard]] LinearColor3D Filmic() const noexcept {
		auto filmic = [](float x) 
		{
			const float A = 0.22f; // Shoulder strength
			const float B = 0.30f; // Linear strength
			const float C = 0.10f; // Linear angle
			const float D = 0.20f; // Toe strength
			const float E = 0.01f; // Toe numerator
			const float F = 0.30f; // Toe denominator
			return ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - E / F;
		};
		LinearColor3D mapped(filmic(R()), filmic(G()), filmic(B()));
		// 归一化除以白点（通常为 filmic(11.2) 的值）
		float whiteScale = 1.0f / filmic(11.2f);
		return mapped * whiteScale;
	}

    /**
     * @brief Reinhard 色调映射
     */
    [[nodiscard]] LinearColor3D Reinhard() const noexcept 
	{
        return LinearColor3D(R() / (1.0f + R()),
                       G() / (1.0f + G()),
                       B() / (1.0f + B()));
    }

    /**
     * @brief 调整饱和度
     * @param factor 0.0 = 灰度，1.0 = 原图，>1.0 = 增强
     */
    [[nodiscard]] LinearColor3D Saturation(float factor) const noexcept 
	{
        float lum = Luminance();
        return LinearColor3D(std::lerp(lum, R(), factor),
                       std::lerp(lum, G(), factor),
                       std::lerp(lum, B(), factor));
    }

	/**
     * @brief 生成一个随机但视觉舒适的线性颜色
	 * @todo: 接入随机库 Random.h
     */
    static LinearColor3D MakeRandomColor() 
	{
        static thread_local std::mt19937 gen(std::random_device{}());
        static std::uniform_real_distribution<float> dist(0.0f, 1.0f);
        // 在 HSV 空间生成，保证饱和度与亮度适中
        float h = dist(gen);
        float s = 0.5f + 0.5f * dist(gen); // 0.5 ~ 1.0
        float v = 0.6f + 0.4f * dist(gen); // 0.6 ~ 1.0
        return FromHSV(h * 360.0f, s, v);
    }

	/**
     * @brief 根据标量值在红-绿之间插值（0=红, 1=绿）
     */
    static LinearColor3D MakeRedToGreenColorFromScalar(float scalar) {
        scalar = std::clamp(scalar, 0.0f, 1.0f);
        return LinearColor3D(1.0f - scalar, scalar, 0.0f);
    }

	/**
     * @brief 从黑体辐射色温（开尔文）转换为 RGB 色度
     * @param temp 色温（K），典型范围 1000 ~ 40000
     * @return 线性 RGB 颜色（未经亮度归一化）
     * @note 使用 Tanner Helland 的经典算法
     */
    static LinearColor3D MakeFromColorTemperature(float temp) 
	{
        temp = std::clamp(temp, 1000.0f, 40000.0f) / 100.0f;
        
        float r, g, b;
        
        // 红色分量
        if (temp <= 66.0f) 
		{
            r = 1.0f;
        } else 
		{
            r = 1.29293618606f * std::pow(temp - 60.0f, -0.1332047592f);
            r = std::clamp(r, 0.0f, 1.0f);
        }
        
        // 绿色分量
        if (temp <= 66.0f) 
		{
            g = 0.39008157876f * std::log(temp) - 0.63184144378f;
            g = std::clamp(g, 0.0f, 1.0f);
        } else 
		{
            g = 1.1298908609f * std::pow(temp - 60.0f, -0.0755148492f);
            g = std::clamp(g, 0.0f, 1.0f);
        }
        
        // 蓝色分量
        if (temp >= 66.0f) 
		{
            b = 1.0f;
        } else if (temp <= 19.0f) 
		{
            b = 0.0f;
        } else 
		{
            b = 0.54320678911f * std::log(temp - 10.0f) - 1.19625408914f;
            b = std::clamp(b, 0.0f, 1.0f);
        }
        
        return LinearColor3D(r, g, b);
    }

	
    /**
     * @brief 预乘 Alpha（用于合成）
     */
    [[nodiscard]] LinearColor3D Premultiply(float alpha) const noexcept 
	{
        return *this * alpha;
    }

    // ---------- 常用静态工厂 ----------
    static constexpr LinearColor3D Zero() noexcept { return LinearColor3D(0.0f); }
    static constexpr LinearColor3D One() noexcept { return LinearColor3D(1.0f); }
    static constexpr LinearColor3D Black() noexcept { return Zero(); }
    static constexpr LinearColor3D White() noexcept { return One(); }
    static constexpr LinearColor3D Red() noexcept { return LinearColor3D(1.0f, 0.0f, 0.0f); }
    static constexpr LinearColor3D Green() noexcept { return LinearColor3D(0.0f, 1.0f, 0.0f); }
    static constexpr LinearColor3D Blue() noexcept { return LinearColor3D(0.0f, 0.0f, 1.0f); }
    static constexpr LinearColor3D Yellow() noexcept { return LinearColor3D(1.0f, 1.0f, 0.0f); }
    static constexpr LinearColor3D Cyan() noexcept { return LinearColor3D(0.0f, 1.0f, 1.0f); }
    static constexpr LinearColor3D Magenta() noexcept { return LinearColor3D(1.0f, 0.0f, 1.0f); }


	 /**
     * @brief 从 HSV 构造线性 RGB
     * @param h 色相 [0, 360)
     * @param s 饱和度 [0, 1]
     * @param v 明度 [0, 1]
     */
    static LinearColor3D FromHSV(float h, float s, float v) {
        h = std::fmod(h, 360.0f);
        if (h < 0) h += 360.0f;
        h /= 60.0f;
        int i = static_cast<int>(h);
        float f = h - i;
        float p = v * (1.0f - s);
        float q = v * (1.0f - s * f);
        float t = v * (1.0f - s * (1.0f - f));
        
        switch (i) {
            case 0:  return LinearColor3D(v, t, p);
            case 1:  return LinearColor3D(q, v, p);
            case 2:  return LinearColor3D(p, v, t);
            case 3:  return LinearColor3D(p, q, v);
            case 4:  return LinearColor3D(t, p, v);
            default: return LinearColor3D(v, p, q);
        }
    }

    /**
     * @brief 转换为 HSV 表示
     * @param h 输出色相 [0, 360)
     * @param s 输出饱和度 [0, 1]
     * @param v 输出明度 [0, 1]
     */
    void ToHSV(float& h, float& s, float& v) const 
	{
        float minVal = std::min({R(), G(), B()});
        float maxVal = std::max({R(), G(), B()});
        float delta = maxVal - minVal;
        
        v = maxVal;
        if (maxVal == 0.0f) 
		{
            s = 0.0f;
            h = 0.0f;
            return;
        }
        s = delta / maxVal;
        
        if (delta == 0.0f) 
		{
            h = 0.0f;
        } else if (maxVal == R()) 
		{
            h = 60.0f * std::fmod((G() - B()) / delta, 6.0f);
        } else if (maxVal == G()) 
		{
            h = 60.0f * ((B() - R()) / delta + 2.0f);
        } else 
		{
            h = 60.0f * ((R() - G()) / delta + 4.0f);
        }
        if (h < 0.0f) h += 360.0f;
    }
	/**
     * @brief 格式化为 "(R=..., G=..., B=...)" 字符串
     * @param precision 小数位数（默认 3）
     */
    [[nodiscard]] std::string ToString(int precision = 3) const 
	{
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(precision);
        oss << "(R=" << R() << ", G=" << G() << ", B=" << B() << ")";
        return oss.str();
    }

    // ---------- 辅助：从线性值生成 8-bit sRGB ----------
    [[nodiscard]] LinearColor3D Quantize() const 
	{
        LinearColor3D srgb = ToSRGB();
        auto toByte = [](float x) -> uint8_t {
            return static_cast<uint8_t>(std::clamp(std::round(x * 255.0f), 0.0f, 255.0f));
        };
        return FromByteSRGB(toByte(srgb.R()), toByte(srgb.G()), toByte(srgb.B()));
    }
private:
    vector_type data;
};

// ---------- 与 Vector 的互操作（可选） ----------
inline Vector3D<float> ToVector(const LinearColor3D& c) noexcept {
    return c.AsVector();
}

inline LinearColor3D FromVector(const Vector3D<float>& v) noexcept {
    return LinearColor3D(v);
}

/**
 * @brief 四维颜色（线性空间，HDR 支持），用于合成、纹理、材质属性等
 * @note 包含 Alpha 通道，语义上为 (R, G, B, A)
 * @note 底层基于 Vector4D<float>，复用向量运算，屏蔽几何接口
 */
class LinearColor4D {
public:
    using vector_type = Vector4D<float>;
    using value_type = float;

    // ---------- 构造 ----------
    constexpr LinearColor4D() noexcept : data{0.0f, 0.0f, 0.0f, 1.0f} {}
    constexpr LinearColor4D(float r, float g, float b, float a = 1.0f) noexcept : data{r, g, b, a} {}
    explicit constexpr LinearColor4D(float gray, float a = 1.0f) noexcept : data{gray, gray, gray, a} {}
    explicit constexpr LinearColor4D(const vector_type& v) noexcept : data(v) {}

    // 从三维颜色构造（Alpha 默认为 1）
    explicit constexpr LinearColor4D(const LinearColor3D& c, float a = 1.0f) noexcept 
        : data{c.R(), c.G(), c.B(), a} {}

	/**
     * @brief 从十六进制字符串构造（支持 #RGBA, #RRGGBBAA, RRGGBBAA, RGBA）
     * @return 线性颜色（输入为 sRGB）
     */
    static LinearColor4D FromHex(std::string_view hex) 
	{
        std::string_view trimmed = hex;
        if (trimmed.starts_with('#'))
            trimmed.remove_prefix(1);
        
        uint32_t int_value = 0;
        uint8_t a = 255;
        
        if (trimmed.size() == 4) 
		{
            // RGBA -> 每个字符重复
            auto fromHexChar = [](char c) -> uint8_t 
			{
                if (c >= '0' && c <= '9') return c - '0';
                if (c >= 'a' && c <= 'f') return c - 'a' + 10;
                if (c >= 'A' && c <= 'F') return c - 'A' + 10;
                return 0;
            };
            uint8_t r = fromHexChar(trimmed[0]);
            uint8_t g = fromHexChar(trimmed[1]);
            uint8_t b = fromHexChar(trimmed[2]);
            a = fromHexChar(trimmed[3]);
            int_value = (r << 20) | (r << 16) | (g << 12) | (g << 8) | (b << 4) | b;
        } 
		else if (trimmed.size() == 8) 
		{
            auto result = std::from_chars(trimmed.data(), trimmed.data() + trimmed.size(), int_value, 16);
            if (result.ec == std::errc{}) 
			{
                a = int_value & 0xFF;
                int_value >>= 8;
            } 
			else 
			{
                int_value = 0;
            }
        } else if (trimmed.size() == 6) 
		{
            // 只有 RGB，Alpha 默认 255
            auto result = std::from_chars(trimmed.data(), trimmed.data() + trimmed.size(), int_value, 16);
            if (result.ec != std::errc{}) 
			{
				int_value = 0;
			}
        }

        uint8_t r = (int_value >> 16) & 0xFF;
        uint8_t g = (int_value >> 8) & 0xFF;
        uint8_t b = int_value & 0xFF;
        
        LinearColor3D rgb = LinearColor3D::FromByteSRGB(r, g, b);
        return LinearColor4D(rgb, a / 255.0f);
    }
    // 允许从其他算术类型的 Vector4D 转换
    template <typename U> requires std::is_arithmetic_v<U>
    explicit constexpr LinearColor4D(const Vector4D<U>& v) noexcept
        : data{static_cast<float>(v.X()), static_cast<float>(v.Y()),
                static_cast<float>(v.Z()), static_cast<float>(v.W())} {}

    // ---------- 分量访问 ----------
    [[nodiscard]] constexpr float R() const noexcept { return data.X(); }
    [[nodiscard]] constexpr float G() const noexcept { return data.Y(); }
    [[nodiscard]] constexpr float B() const noexcept { return data.Z(); }
    [[nodiscard]] constexpr float A() const noexcept { return data.W(); }

    constexpr void SetR(float r) noexcept { data.X() = r; }
    constexpr void SetG(float g) noexcept { data.Y() = g; }
    constexpr void SetB(float b) noexcept { data.Z() = b; }
    constexpr void SetA(float a) noexcept { data.W() = a; }

    [[nodiscard]] constexpr float  operator[](std::size_t i) const noexcept { return data[i]; }
    constexpr float& operator[](std::size_t i) noexcept { return data[i]; }

    // 提取 RGB 部分（显式丢弃 Alpha）
    [[nodiscard]] constexpr LinearColor3D ToColor3D() const noexcept 
	{
        return LinearColor3D{R(), G(), B()};
    }

    // 获取底层向量（用于需要向量操作的特殊场景）
    [[nodiscard]] constexpr const vector_type& AsVector() const noexcept { return data; }

    // ---------- 算术运算符（转发给 Vector）----------
    constexpr LinearColor4D& operator+=(const LinearColor4D& rhs) noexcept 
	{
        data += rhs.data;
        return *this;
    }
    constexpr LinearColor4D& operator-=(const LinearColor4D& rhs) noexcept 
	{
        data -= rhs.data;
        return *this;
    }
    constexpr LinearColor4D& operator*=(const LinearColor4D& rhs) noexcept 
	{
        data *= rhs.data;
        return *this;
    }
    constexpr LinearColor4D& operator/=(const LinearColor4D& rhs) noexcept 
	{
        data /= rhs.data;
        return *this;
    }
    constexpr LinearColor4D& operator*=(float s) noexcept 
	{
        data *= s;
        return *this;
    }
    constexpr LinearColor4D& operator/=(float s) noexcept 
	{
        data /= s;
        return *this;
    }

    constexpr LinearColor4D operator-() const noexcept 
	{
        return LinearColor4D(-data);
    }

    friend constexpr LinearColor4D operator+(LinearColor4D lhs, const LinearColor4D& rhs) noexcept 
	{
        return lhs += rhs;
    }
    friend constexpr LinearColor4D operator-(LinearColor4D lhs, const LinearColor4D& rhs) noexcept 
	{
        return lhs -= rhs;
    }
    friend constexpr LinearColor4D operator*(LinearColor4D lhs, const LinearColor4D& rhs) noexcept 
	{
        return lhs *= rhs;
    }
    friend constexpr LinearColor4D operator/(LinearColor4D lhs, const LinearColor4D& rhs) noexcept 
	{
        return lhs /= rhs;
    }
    friend constexpr LinearColor4D operator*(LinearColor4D lhs, float rhs) noexcept 
	{
        return lhs *= rhs;
    }
    friend constexpr LinearColor4D operator*(float lhs, LinearColor4D rhs) noexcept 
	{
        return rhs *= lhs;
    }
    friend constexpr LinearColor4D operator/(LinearColor4D lhs, float rhs) noexcept 
	{
        return lhs /= rhs;
    }

    friend constexpr bool operator==(const LinearColor4D& lhs, const LinearColor4D& rhs) noexcept 
	{
        return lhs.data == rhs.data;
    }

    // ---------- 颜色专属功能（扩展至四通道）----------

    /**
     * @brief 计算 RGB 部分相对亮度（忽略 Alpha）
     */
    [[nodiscard]] constexpr float Luminance() const noexcept 
	{
        return ToColor3D().Luminance();
    }

    /**
     * @brief 判断 RGB 是否为黑色（考虑浮点误差）
     */
    [[nodiscard]] constexpr bool IsBlack(float eps = 1e-6f) const noexcept 
	{
        return ToColor3D().IsBlack(eps);
    }

    /**
     * @brief 逐分量钳位（包含 Alpha）
     */
    [[nodiscard]] constexpr LinearColor4D Clamp(float minVal = 0.0f, float maxVal = INFINITY) const noexcept 
	{
        return LinearColor4D(std::clamp(R(), minVal, maxVal),
                       std::clamp(G(), minVal, maxVal),
                       std::clamp(B(), minVal, maxVal),
                       std::clamp(A(), minVal, maxVal));
    }

    /**
     * @brief 简单伽马校正（仅应用于 RGB，Alpha 保持不变）
     */
    [[nodiscard]] constexpr LinearColor4D GammaCorrect(float gamma = 2.2f) const noexcept 
	{
        LinearColor3D rgb = ToColor3D().GammaCorrect(gamma);
        return LinearColor4D(rgb, A());
    }

    /**
     * @brief RGB 部分线性 → sRGB 转换，Alpha 不变
     */
    [[nodiscard]] constexpr LinearColor4D ToSRGB() const noexcept 
	{
        LinearColor3D rgb = ToColor3D().ToSRGB();
        return LinearColor4D(rgb, A());
    }

    /**
     * @brief RGB 部分 sRGB → 线性转换，Alpha 不变
     */
    [[nodiscard]] LinearColor4D ToLinear() const noexcept 
	{
        LinearColor3D rgb = ToColor3D().ToLinear();
        return LinearColor4D(rgb, A());
    }

    /**
     * @brief 色调映射（应用于 RGB，Alpha 不变）
     */
    [[nodiscard]] LinearColor4D ACES() const noexcept 
	{
        LinearColor3D rgb = ToColor3D().ACES();
        return LinearColor4D(rgb, A());
    }

    [[nodiscard]] LinearColor4D Reinhard() const noexcept 
	{
        LinearColor3D rgb = ToColor3D().Reinhard();
        return LinearColor4D(rgb, A());
    }

    /**
     * @brief 调整 RGB 饱和度，Alpha 不变
     */
    [[nodiscard]] LinearColor4D Saturation(float factor) const noexcept 
	{
        LinearColor3D rgb = ToColor3D().Saturation(factor);
        return LinearColor4D(rgb, A());
    }

    /**
     * @brief 预乘 Alpha（将 RGB 乘以 A，Alpha 不变）
     * @note 常用于合成管线
     */
    [[nodiscard]] LinearColor4D PremultipliedAlpha() const noexcept 
	{
        return LinearColor4D(R() * A(), G() * A(), B() * A(), A());
    }

    /**
     * @brief 反预乘 Alpha（将 RGB 除以 A，Alpha 不变）
     * @note 要求 A > 0
     */
    [[nodiscard]] LinearColor4D UnpremultipliedAlpha() const noexcept 
	{
        float invA = (A() != 0.0f) ? 1.0f / A() : 0.0f;
        return LinearColor4D(R() * invA, G() * invA, B() * invA, A());
    }

    /**
     * @brief Alpha 混合（Over 操作）
     * @param bg 背景颜色（假定已预乘或未预乘？这里采用非预乘标准公式）
     * @note 公式：result = fg * fg.A + bg * (1 - fg.A)
     */
    [[nodiscard]] LinearColor4D Over(const LinearColor4D& bg) const noexcept 
	{
        float a = A();
        return LinearColor4D(R() + bg.R() * (1.0f - a),
                       G() + bg.G() * (1.0f - a),
                       B() + bg.B() * (1.0f - a),
                       a + bg.A() * (1.0f - a));
    }

	/**
     * @brief 生成随机颜色（带透明度）
     * @param random_alpha 是否随机透明度（默认固定为 1.0）
     */
    static LinearColor4D MakeRandomColor(bool random_alpha = false) 
	{
        LinearColor3D rgb = LinearColor3D::MakeRandomColor();
        float a = 1.0f;
        if (random_alpha) {
            static thread_local std::mt19937 gen(std::random_device{}());
            static std::uniform_real_distribution<float> dist(0.0f, 1.0f);
            a = dist(gen);
        }
        return LinearColor4D(rgb, a);
    }

    /**
     * @brief 根据标量值在红-绿之间插值（0=红, 1=绿），带透明度
     */
    static LinearColor4D MakeRedToGreenColorFromScalar(float scalar, float alpha = 1.0f) 
	{
        LinearColor3D rgb = LinearColor3D::MakeRedToGreenColorFromScalar(scalar);
        return LinearColor4D(rgb, alpha);
    }

    /**
     * @brief 从色温构造颜色（带透明度）
     */
    static LinearColor4D MakeFromColorTemperature(float temp, float alpha = 1.0f) 
	{
        LinearColor3D rgb = LinearColor3D::MakeFromColorTemperature(temp);
        return LinearColor4D(rgb, alpha);
    }

    /**
     * @brief 格式化为 "(R=..., G=..., B=..., A=...)" 字符串
     */
    [[nodiscard]] std::string ToString(int precision = 3) const 
	{
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(precision);
        oss << "(R=" << R() << ", G=" << G() << ", B=" << B() << ", A=" << A() << ")";
        return oss.str();
    }

    /**
     * @brief 从 RGBE 构造（Alpha 固定为 1.0）
     */
    static LinearColor4D FromRGBE(const uint8_t rgbe[4]) 
	{
        return LinearColor4D(LinearColor3D::FromRGBE(rgbe), 1.0f);
    }

    /**
     * @brief 编码为 RGBE（忽略 Alpha）
     */
    void ToRGBE(uint8_t outRgbe[4]) const 
	{
        ToColor3D().ToRGBE(outRgbe);
    }
    // ---------- 静态工厂 ----------
    static constexpr LinearColor4D Zero() noexcept { return LinearColor4D(0.0f, 0.0f, 0.0f, 0.0f); }
    static constexpr LinearColor4D One() noexcept { return LinearColor4D(1.0f, 1.0f, 1.0f, 1.0f); }
    static constexpr LinearColor4D Black() noexcept { return LinearColor4D(0.0f, 0.0f, 0.0f, 1.0f); }
    static constexpr LinearColor4D White() noexcept { return LinearColor4D(1.0f, 1.0f, 1.0f, 1.0f); }
    static constexpr LinearColor4D Transparent() noexcept { return LinearColor4D(0.0f, 0.0f, 0.0f, 0.0f); }
    static constexpr LinearColor4D Red(float a = 1.0f) noexcept { return LinearColor4D(1.0f, 0.0f, 0.0f, a); }
    static constexpr LinearColor4D Green(float a = 1.0f) noexcept { return LinearColor4D(0.0f, 1.0f, 0.0f, a); }
    static constexpr LinearColor4D Blue(float a = 1.0f) noexcept { return LinearColor4D(0.0f, 0.0f, 1.0f, a); }
    static constexpr LinearColor4D Yellow(float a = 1.0f) noexcept { return LinearColor4D(1.0f, 1.0f, 0.0f, a); }
    static constexpr LinearColor4D Cyan(float a = 1.0f) noexcept { return LinearColor4D(0.0f, 1.0f, 1.0f, a); }
    static constexpr LinearColor4D Magenta(float a = 1.0f) noexcept { return LinearColor4D(1.0f, 0.0f, 1.0f, a); }

private:
    vector_type data;
};

// ---------- 与 Vector4D 的互操作 ----------
inline Vector4D<float> ToVector(const LinearColor4D& c) noexcept {

    return c.AsVector();
}

inline LinearColor4D FromVector(const Vector4D<float>& v) noexcept 
{
    return LinearColor4D(v);
}

/**
 * @brief 8-bit 整型 RGB 颜色（隐式 sRGB 编码）
 * @note 内存布局与 uint8_t[3] 兼容，可直接写入图像缓冲区
 * @note 所有运算应通过 ToLinear() 转换为 LinearColor3D 进行
 */
struct Color3D 
{
    uint8_t R, G, B;

    // ---------- 构造 ----------
    constexpr Color3D() noexcept : R(0), G(0), B(0) {}
    constexpr Color3D(uint8_t r, uint8_t g, uint8_t b) noexcept : R(r), G(g), B(b) {}

    // 从线性浮点颜色构造（自动执行 sRGB 编码与量化）
    explicit Color3D(const LinearColor3D& linear) noexcept 
	{
        LinearColor3D srgb = linear.ToSRGB();
        auto saturate = [](float x) -> uint8_t {
            // 注意：std::clamp 与 std::round 在 constexpr 中可用 (C++23 前部分编译器支持有限)
            // 为兼容性，使用简单条件判断，但此处不需要 constexpr，故直接使用 std::clamp
            return static_cast<uint8_t>(std::clamp(std::round(x * 255.0f), 0.0f, 255.0f));
        };
        R = saturate(srgb.R());
        G = saturate(srgb.G());
        B = saturate(srgb.B());
    }

    // 从十六进制字符串构造（支持 #RGB, #RRGGBB, RRGGBB, RGB）
    static Color3D FromHex(std::string_view hex) 
	{
        // 复用 LinearColor3D 的解析逻辑，再转回整型
        LinearColor3D linear = LinearColor3D::FromHex(hex);
        return Color3D(linear);
    }

    // ---------- 转换 ----------
    [[nodiscard]] LinearColor3D ToLinear() const 
	{
        return LinearColor3D::FromByteSRGB(R, G, B);
    }

    // ---------- 访问器 ----------
    const uint8_t* Data() const noexcept { return &R; }
    uint8_t* Data() noexcept { return &R; }

    // ---------- 字符串输出 ----------
    [[nodiscard]] std::string ToString() const 
	{
        std::ostringstream oss;
        oss << "(R=" << static_cast<int>(R)
            << ", G=" << static_cast<int>(G)
            << ", B=" << static_cast<int>(B) << ")";
        return oss.str();
    }

    // ---------- 静态工厂 ----------
    static constexpr Color3D Black() noexcept { return {0, 0, 0}; }
    static constexpr Color3D White() noexcept { return {255, 255, 255}; }
    static constexpr Color3D Red() noexcept   { return {255, 0, 0}; }
    static constexpr Color3D Green() noexcept { return {0, 255, 0}; }
    static constexpr Color3D Blue() noexcept  { return {0, 0, 255}; }
    static constexpr Color3D Yellow() noexcept { return {255, 255, 0}; }
    static constexpr Color3D Cyan() noexcept   { return {0, 255, 255}; }
    static constexpr Color3D Magenta() noexcept{ return {255, 0, 255}; }

    // 随机颜色
    static Color3D MakeRandomColor() {
        LinearColor3D lin = LinearColor3D::MakeRandomColor();
        return Color3D(lin);
    }

    // 色温
    static Color3D MakeFromColorTemperature(float temp) {
        LinearColor3D lin = LinearColor3D::MakeFromColorTemperature(temp);
        return Color3D(lin);
    }

    // 红-绿插值
    static Color3D MakeRedToGreenColorFromScalar(float scalar) {
        LinearColor3D lin = LinearColor3D::MakeRedToGreenColorFromScalar(scalar);
        return Color3D(lin);
    }
};

/**
 * @brief 8-bit 整型 RGBA 颜色（隐式 sRGB 编码）
 * @note 内存布局与 uint8_t[4] 兼容，适用于纹理像素与帧缓存
 */
struct Color4D 
{
    uint8_t R, G, B, A;

    // ---------- 构造 ----------
    constexpr Color4D() noexcept : R(0), G(0), B(0), A(255) {}
    constexpr Color4D(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) noexcept : R(r), G(g), B(b), A(a) {}

    // 从线性浮点颜色构造（自动 sRGB 编码与量化）
    explicit Color4D(const LinearColor4D& linear) noexcept 
	{
        LinearColor4D srgb = linear.ToSRGB();
        auto saturate = [](float x) -> uint8_t {
            return static_cast<uint8_t>(std::clamp(std::round(x * 255.0f), 0.0f, 255.0f));
        };
        R = saturate(srgb.R());
        G = saturate(srgb.G());
        B = saturate(srgb.B());
        A = saturate(srgb.A());
    }

    // 从 LinearColor3D + Alpha 构造
    explicit Color4D(const LinearColor3D& rgb, uint8_t a = 255) noexcept
        : Color4D(LinearColor4D(rgb, a / 255.0f)) {}

    // 从十六进制字符串构造（支持 #RGBA, #RRGGBBAA, RRGGBBAA, RGBA）
    static Color4D FromHex(std::string_view hex) 
	{
        LinearColor4D linear = LinearColor4D::FromHex(hex);
        return Color4D(linear);
    }

    // ---------- 转换 ----------
    [[nodiscard]] LinearColor4D ToLinear() const 
	{
        LinearColor3D rgb = LinearColor3D::FromByteSRGB(R, G, B);
        return LinearColor4D(rgb, A / 255.0f);
    }

    // 提取 RGB 部分
    [[nodiscard]] Color3D ToColor3D() const noexcept 
	{
        return {R, G, B};
    }

    // ---------- 访问器 ----------
    const uint8_t* Data() const noexcept { return &R; }
    uint8_t* Data() noexcept { return &R; }

    // 打包为 uint32_t (RGBA 顺序)
    [[nodiscard]] uint32_t ToPackedRGBA() const noexcept 
	{
        return (static_cast<uint32_t>(R) << 24) |
               (static_cast<uint32_t>(G) << 16) |
               (static_cast<uint32_t>(B) << 8)  |
               static_cast<uint32_t>(A);
    }

    // ---------- 字符串输出 ----------
    [[nodiscard]] std::string ToString() const 
	{
        std::ostringstream oss;
        oss << "(R=" << static_cast<int>(R)
            << ", G=" << static_cast<int>(G)
            << ", B=" << static_cast<int>(B)
            << ", A=" << static_cast<int>(A) << ")";
        return oss.str();
    }

    // ---------- 静态工厂 ----------
    static constexpr Color4D Black() noexcept       { return {0, 0, 0, 255}; }
    static constexpr Color4D White() noexcept       { return {255, 255, 255, 255}; }
    static constexpr Color4D Transparent() noexcept { return {0, 0, 0, 0}; }
    static constexpr Color4D Red(uint8_t a = 255) noexcept   { return {255, 0, 0, a}; }
    static constexpr Color4D Green(uint8_t a = 255) noexcept { return {0, 255, 0, a}; }
    static constexpr Color4D Blue(uint8_t a = 255) noexcept  { return {0, 0, 255, a}; }
    static constexpr Color4D Yellow(uint8_t a = 255) noexcept{ return {255, 255, 0, a}; }
    static constexpr Color4D Cyan(uint8_t a = 255) noexcept  { return {0, 255, 255, a}; }
    static constexpr Color4D Magenta(uint8_t a = 255) noexcept{ return {255, 0, 255, a}; }

    // 随机颜色
    static Color4D MakeRandomColor(bool randomAlpha = false) 
	{
        LinearColor4D lin = LinearColor4D::MakeRandomColor(randomAlpha);
        return Color4D(lin);
    }

    // 色温
    static Color4D MakeFromColorTemperature(float temp, uint8_t alpha = 255) 
	{
        LinearColor4D lin = LinearColor4D::MakeFromColorTemperature(temp, alpha / 255.0f);
        return Color4D(lin);
    }

    // 红-绿插值
    static Color4D MakeRedToGreenColorFromScalar(float scalar, uint8_t alpha = 255) 
	{
        LinearColor4D lin = LinearColor4D::MakeRedToGreenColorFromScalar(scalar, alpha / 255.0f);
        return Color4D(lin);
    }
};


inline constexpr LinearColor3D::LinearColor3D(const LinearColor4D& c) noexcept
    : data{c.R(), c.G(), c.B()} {}

inline constexpr LinearColor4D LinearColor3D::WithAlpha(float alpha) const noexcept {
    return LinearColor4D{*this, alpha};
}

} // namespace Core::Math