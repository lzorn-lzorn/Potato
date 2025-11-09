#pragma once

#include <iostream>
#include <sstream>
#include <string>
#include <iomanip>

/* 
    ToDo: 接入GMP大整数库 
*/
namespace Core {
/*
 * @function: 数字的封装类型
 */
struct Number{
    Number() : value(0.0) {}
    Number(int v) : value(static_cast<double>(v)) {}
    Number(float v) : value(static_cast<double>(v)) {}
    Number(double v) : value(v) {}
    Number(long double v) : value(static_cast<double>(v)) {}
    Number(Number&& other) noexcept : value(other.value) {
        other.value = 0.0;
    }
    Number& operator=(Number&& other) noexcept {
        if (this != &other) {
            value = other.value;
            other.value = 0.0;
        }
        return *this;
    }
    Number(const Number& other) : value(other.value) {}
    Number& operator=(const Number& other){
        if (this != &other){
            value = other.value;
        }
        return *this;
    }
    Number& operator=(int v){
        value = static_cast<double>(v);
        return *this;
    }
    Number& operator=(float v){
        value = static_cast<double>(v);
        return *this;
    }
    Number& operator=(double v){
        value = v;
        return *this;
    }

    // 类型转换
    operator int() const noexcept {
        return static_cast<int>(value);
    }   
    operator float() const noexcept {
        return static_cast<float>(value);
    }
    operator double() const noexcept {
        return value;
    }
    operator long double() const noexcept {
        return static_cast<long double>(value);
    }
    // 比较运算符
    bool operator==(const Number& other) const noexcept {
        return value == other.value;
    }
    bool operator!=(const Number& other) const noexcept {
        return value != other.value;
    }
    bool operator<(const Number& other) const noexcept {
        return value < other.value;
    }
    bool operator>(const Number& other) const noexcept {
        return value > other.value;
    }
    bool operator<=(const Number& other) const noexcept {
        return value <= other.value;
    }
    bool operator>=(const Number& other) const noexcept {
        return value >= other.value;
    }

    // 算术运算符
    Number operator+(const Number& other) const { return Number(value + other.value); }
    Number operator-(const Number& other) const { return Number(value - other.value); }
    Number operator*(const Number& other) const { return Number(value * other.value); }
    Number operator/(const Number& other) const { return Number(value / other.value); }
    Number operator-() const { return Number(-value); } // 一元负号
    Number operator^(const double exp) const { return Number(std::powl(value, exp)); }
    Number operator^(const int exp) const { return Number(std::pow(value, exp)); }


    Number& operator+=(const Number& other) { value += other.value; return *this; }
    Number& operator-=(const Number& other) { value -= other.value; return *this; }
    Number& operator*=(const Number& other) { value *= other.value; return *this; }
    Number& operator/=(const Number& other) { value /= other.value; return *this; }
    Number operator^=(const int exp) { value = std::pow(value, exp); return *this; }
    Number operator^=(const double exp) { value = std::powl(value, exp); return *this; }

    // 与原生类型运算
    Number operator+(int v) const { return Number(value + v); }
    Number operator-(int v) const { return Number(value - v); }
    Number operator*(int v) const { return Number(value * v); }
    Number operator/(int v) const { return Number(value / v); }

    Number operator+(double v) const { return Number(value + v); }
    Number operator-(double v) const { return Number(value - v); }
    Number operator*(double v) const { return Number(value * v); }
    Number operator/(double v) const { return Number(value / v); }

    Number& operator+=(int v) { value += v; return *this; }
    Number& operator-=(int v) { value -= v; return *this; }
    Number& operator*=(int v) { value *= v; return *this; }
    Number& operator/=(int v) { value /= v; return *this; }

    Number& operator+=(double v) { value += v; return *this; }
    Number& operator-=(double v) { value -= v; return *this; }
    Number& operator*=(double v) { value *= v; return *this; }
    Number& operator/=(double v) { value /= v; return *this; }

    // 自增/自减
    Number& operator++() { ++value; return *this; }       // 前置++
    Number operator++(int) { Number tmp(*this); ++value; return tmp; } // 后置++
    Number& operator--() { --value; return *this; }       // 前置--
    Number operator--(int) { Number tmp(*this); --value; return tmp; } // 后置--

    // 输出流重载
    friend std::ostream& operator<<(std::ostream& os, const Number& num) {
        os << num.value;
        return os;
    }

    Number Sqrt() const {
        return Number(std::sqrt(value));
    }
    std::string ToString(int precision = 2) const {
        std::ostringstream oss;
        if (precision == 0) {
            oss << int(value); // 输出整数部分
        } else {
            oss << std::fixed << std::setprecision(precision) << value;
        }
        return oss.str();
    }

    std::string ToHexString() const {
        std::ostringstream oss;
        oss << std::hex << int(value);
        return oss.str();
    }

    std::string GetRtType() const noexcept {
        return "Number";
    }
    double value;
};

// 支持 int + Number 形式
inline Number operator+(int lhs, const Number& rhs) { return Number(lhs + rhs.value); }
inline Number operator-(int lhs, const Number& rhs) { return Number(lhs - rhs.value); }
inline Number operator*(int lhs, const Number& rhs) { return Number(lhs * rhs.value); }
inline Number operator/(int lhs, const Number& rhs) { return Number(lhs / rhs.value); }

inline Number operator+(double lhs, const Number& rhs) { return Number(lhs + rhs.value); }
inline Number operator-(double lhs, const Number& rhs) { return Number(lhs - rhs.value); }
inline Number operator*(double lhs, const Number& rhs) { return Number(lhs * rhs.value); }
inline Number operator/(double lhs, const Number& rhs) { return Number(lhs / rhs.value); }
}
