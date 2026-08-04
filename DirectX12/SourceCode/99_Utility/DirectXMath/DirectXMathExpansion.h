#pragma once

#include <DirectXMath.h>
#include <concepts>
#include <cmath>
#include <span>
#include <numbers>

namespace MathExpansion
{
    // -----------------------------------------------------------
    // 浮動小数点数の近似比較 (C++20 std::floating_point / constexpr std::abs)
    // -----------------------------------------------------------
    template <std::floating_point T>
    [[nodiscard]] constexpr bool NearlyEqual(const T a, const T b, const T epsilon = std::numeric_limits<T>::epsilon()) noexcept
    {
        return std::abs(a - b) <= epsilon;
    }

    // -----------------------------------------------------------
    // XMFLOAT3 の近似比較
    // -----------------------------------------------------------
    [[nodiscard]] constexpr bool NearlyEqual(
        const DirectX::XMFLOAT3& a,
        const DirectX::XMFLOAT3& b,
        const float epsilon = std::numeric_limits<float>::epsilon()) noexcept
    {
        return NearlyEqual(a.x, b.x, epsilon) &&
            NearlyEqual(a.y, b.y, epsilon) &&
            NearlyEqual(a.z, b.z, epsilon);
    }
}

// せんざんからこれに対して足りないものをAIに出させる

// -----------------------------------------------------------
// DirectX::XMFLOAT3 の演算子オーバーロード拡張
// -----------------------------------------------------------
[[nodiscard]] constexpr DirectX::XMFLOAT3 operator+(const DirectX::XMFLOAT3& lhs, const DirectX::XMFLOAT3& rhs) noexcept
{
    return { lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z };
}

[[nodiscard]] constexpr DirectX::XMFLOAT3 operator-(const DirectX::XMFLOAT3& lhs, const DirectX::XMFLOAT3& rhs) noexcept
{
    return { lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z };
}

[[nodiscard]] constexpr DirectX::XMFLOAT3 operator*(const DirectX::XMFLOAT3& lhs, const DirectX::XMFLOAT3& rhs) noexcept
{
    return { lhs.x * rhs.x, lhs.y * rhs.y, lhs.z * rhs.z };
}

[[nodiscard]] constexpr DirectX::XMFLOAT3 operator*(const DirectX::XMFLOAT3& v, const float scalar) noexcept
{
    return { v.x * scalar, v.y * scalar, v.z * scalar };
}

[[nodiscard]] constexpr DirectX::XMFLOAT3 operator*(const float scalar, const DirectX::XMFLOAT3& v) noexcept
{
    return v * scalar;
}

[[nodiscard]] constexpr DirectX::XMFLOAT3 operator/(const DirectX::XMFLOAT3& v, const float scalar) noexcept
{
    const float inv = 1.0f / scalar;
    return { v.x * inv, v.y * inv, v.z * inv };
}

constexpr DirectX::XMFLOAT3& operator+=(DirectX::XMFLOAT3& lhs, const DirectX::XMFLOAT3& rhs) noexcept
{
    lhs.x += rhs.x; lhs.y += rhs.y; lhs.z += rhs.z;
    return lhs;
}

constexpr DirectX::XMFLOAT3& operator-=(DirectX::XMFLOAT3& lhs, const DirectX::XMFLOAT3& rhs) noexcept
{
    lhs.x -= rhs.x; lhs.y -= rhs.y; lhs.z -= rhs.z;
    return lhs;
}

constexpr DirectX::XMFLOAT3& operator*=(DirectX::XMFLOAT3& lhs, const float scalar) noexcept
{
    lhs.x *= scalar; lhs.y *= scalar; lhs.z *= scalar;
    return lhs;
}

constexpr DirectX::XMFLOAT3& operator/=(DirectX::XMFLOAT3& lhs, const float scalar) noexcept
{
    const float inv = 1.0f / scalar;
    lhs.x *= inv; lhs.y *= inv; lhs.z *= inv;
    return lhs;
}

[[nodiscard]] constexpr bool operator==(const DirectX::XMFLOAT3& lhs, const DirectX::XMFLOAT3& rhs) noexcept
{
    return MathExpansion::NearlyEqual(lhs, rhs);
}

[[nodiscard]] constexpr bool operator!=(const DirectX::XMFLOAT3& lhs, const DirectX::XMFLOAT3& rhs) noexcept
{
    return !(lhs == rhs);
}

// -----------------------------------------------------------
// std::span を用いた要素設定 (要素数3の固定長span)
// -----------------------------------------------------------
constexpr void SetFromSpan(DirectX::XMFLOAT3& out, std::span<const float, 3> values) noexcept
{
    out.x = values[0];
    out.y = values[1];
    out.z = values[2];
}