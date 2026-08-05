#pragma once

#include <DirectXMath.h>
#include <span>
#include <DirectXMathConvert.inl>
#include <DirectXMathMatrix.inl>
#include "99_Utility\DirectXMath\DirectXMathExpansion.h"

struct Transform
{
    DirectX::XMFLOAT3 Position{ 0.0f, 0.0f, 0.0f };
    DirectX::XMFLOAT3 Rotation{ 0.0f, 0.0f, 0.0f }; // オイラー角 (Pitch, Yaw, Roll)
    DirectX::XMFLOAT3 Scale{ 1.0f, 1.0f, 1.0f };

    // -----------------------------------------------------------
    // コンストラクタ
    // -----------------------------------------------------------
    constexpr Transform() noexcept = default;

    constexpr Transform(
        const DirectX::XMFLOAT3& position,
        const DirectX::XMFLOAT3& rotation = { 0.0f, 0.0f, 0.0f },
        const DirectX::XMFLOAT3& scale = { 1.0f, 1.0f, 1.0f }) noexcept
        : Position{ position }
        , Rotation{ rotation }
        , Scale{ scale }
    {
    }

    // std::span を使用したコンストラクタ
    // std::array, std::vector, Cスタイル配列 (float[3]) などから直接構築可能
    constexpr Transform(
        std::span<const float, 3> position,
        std::span<const float, 3> rotation,
        std::span<const float, 3> scale) noexcept
    {
        SetFromSpan(Position, position);
        SetFromSpan(Rotation, rotation);
        SetFromSpan(Scale, scale);
    }

    // -----------------------------------------------------------
    // 行列変換（SIMD最適化）
    // -----------------------------------------------------------
    // SRT (Scale * Rotation * Translation) 行列の合成
    [[nodiscard]] DirectX::XMMATRIX GetMatrix() const noexcept
    {
        const DirectX::XMVECTOR p = DirectX::XMLoadFloat3(&Position);
        const DirectX::XMVECTOR r = DirectX::XMLoadFloat3(&Rotation);
        const DirectX::XMVECTOR s = DirectX::XMLoadFloat3(&Scale);

        return DirectX::XMMatrixScalingFromVector(s) *
            DirectX::XMMatrixRotationRollPitchYawFromVector(r) *
            DirectX::XMMatrixTranslationFromVector(p);
    }

    // -----------------------------------------------------------
    // 演算子オーバーロード (二項演算子)
    // -----------------------------------------------------------
    [[nodiscard]] constexpr Transform operator+(const Transform& other) const noexcept
    {
        return Transform{
            Position + other.Position,
            Rotation + other.Rotation,
            Scale + other.Scale
        };
    }

    [[nodiscard]] constexpr Transform operator-(const Transform& other) const noexcept
    {
        return Transform{
            Position - other.Position,
            Rotation - other.Rotation,
            Scale - other.Scale
        };
    }

    // Transform 同士の要素ごとの乗算
    [[nodiscard]] constexpr Transform operator*(const Transform& other) const noexcept
    {
        return Transform{
            Position * other.Position,
            Rotation * other.Rotation,
            Scale * other.Scale
        };
    }

    // スカラー乗算
    [[nodiscard]] constexpr Transform operator*(const float scalar) const noexcept
    {
        return Transform{
            Position * scalar,
            Rotation * scalar,
            Scale * scalar
        };
    }

    // スカラー除算
    [[nodiscard]] constexpr Transform operator/(const float scalar) const noexcept
    {
        return Transform{
            Position / scalar,
            Rotation / scalar,
            Scale / scalar
        };
    }

    // -----------------------------------------------------------
    // 演算子オーバーロード (複合代入演算子)
    // -----------------------------------------------------------
    constexpr Transform& operator+=(const Transform& other) noexcept
    {
        Position += other.Position;
        Rotation += other.Rotation;
        Scale += other.Scale;
        return *this;
    }

    constexpr Transform& operator-=(const Transform& other) noexcept
    {
        Position -= other.Position;
        Rotation -= other.Rotation;
        Scale -= other.Scale;
        return *this;
    }

    constexpr Transform& operator*=(const Transform& other) noexcept
    {
        Position *= other.Position;
        Rotation *= other.Rotation;
        Scale *= other.Scale;
        return *this;
    }

    constexpr Transform& operator*=(const float scalar) noexcept
    {
        Position *= scalar;
        Rotation *= scalar;
        Scale *= scalar;
        return *this;
    }

    constexpr Transform& operator/=(const float scalar) noexcept
    {
        Position /= scalar;
        Rotation /= scalar;
        Scale /= scalar;
        return *this;
    }

    // -----------------------------------------------------------
    // 比較演算子 (MathExpansion::NearlyEqual を使用)
    // -----------------------------------------------------------
    [[nodiscard]] constexpr bool operator==(const Transform& other) const noexcept
    {
        return Position == other.Position &&
            Rotation == other.Rotation &&
            Scale == other.Scale;
    }

    [[nodiscard]] constexpr bool operator!=(const Transform& other) const noexcept
    {
        return !(*this == other);
    }
};