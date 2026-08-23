#pragma once

#include <DirectXMath.h>
#include <filesystem>
#include <string>
#include <vector>

#include "json/json.hpp"

// キャラクタ種別に依存しないラグドール定義(骨格マッピング+物理パラメータ).
// JSONへ保存/読込可能で、Boss/雑魚敵などDefinition差し替えで再利用する.
struct RagdollBoneDesc
{
	std::string            BoneName;                 // 対象ボーン名(Mmdlのボーン名と一致させる).
	float                  Mass         = 1.0f;
	DirectX::XMFLOAT3      ColliderHalf = { 0.1f, 0.1f, 0.1f }; // Box近似の半サイズ.
	DirectX::XMFLOAT3      LocalOffset  = { 0.0f, 0.0f, 0.0f }; // ボーン原点からのオフセット.
};

class RagdollDefinition
{
public:
	std::string             Name;   // 定義名(デバッグ表示用).
	std::vector<RagdollBoneDesc> Bones;

	// JSON保存/読込(FileManager経由).
	bool SaveJson(const std::filesystem::path& FilePath) const;
	bool LoadJson(const std::filesystem::path& FilePath);

	// Boss撃破用のサンプル定義(ボーン名は実機骨格確認後に調整).
	static RagdollDefinition CreateBossDefault();

	// 空かどうか.
	bool IsEmpty() const noexcept { return Bones.empty(); }
};
