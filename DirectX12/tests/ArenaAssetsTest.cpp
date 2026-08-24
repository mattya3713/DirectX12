// 最小アリーナ資産(ArenaFloor/ArenaRing)の単体テスト(スタンドアロン. ゲーム本体には含まない).
// ビルド方法: cl /nologo /EHsc /std:c++20 /utf-8 /W4 /I SourceCode /I Data\Library tests\ArenaAssetsTest.cpp SourceCode\10_Ggraphic\30_Asset\RuntimeFormat\RuntimeFormatIO.cpp /Fe:tests\ArenaAssetsTest.exe
// 実行場所: DirectX12\ (Data配下の実物資産を読む)
//
// 確認内容:
// 1. 生成資産がRuntimeFormatIOで読める(ヘッダー/ペイロードサイズ/マテリアルパスが正当)
// 2. 床円盤のジオメトリが仕様通り(y=0平面・半径20m・法線+y・インデックス範囲内)
// 3. 境界リングのジオメトリが仕様通り(r=20m帯・高さ1.2m以下・水平法線・両面とも範囲内)
// 4. マテリアルが単色(テクスチャ無し)で、リングはAmbient底上げ済み
// 5. main.jsonがアリーナ資産を参照し、Player/Bossスポーンがアリーナ内に収まっている

#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

#include "../SourceCode/10_Ggraphic/30_Asset/RuntimeFormat/RuntimeFormatIO.h"
#include "../SourceCode/00_Game/00_Scene/Level/LevelData.h"

namespace {

	constexpr float kArenaRadius = 20.0f;
	constexpr float kRingHeight = 1.2f;
	constexpr float kEps = 0.05f;

	int g_CheckCount = 0;

	void Check(bool Condition, const char* Label)
	{
		++g_CheckCount;
		if (!Condition)
		{
			std::cerr << "FAILED: " << Label << '\n';
			std::exit(1);
		}
		std::cout << "PASS: " << Label << '\n';
	}

	float Length(float X, float Y, float Z)
	{
		return std::sqrt(X * X + Y * Y + Z * Z);
	}

	bool IsNear(float A, float B, float Eps = kEps)
	{
		return std::fabs(A - B) <= Eps;
	}

} // namespace

int main()
{
	const RuntimeFormat::MstcData* p_floor = nullptr;
	RuntimeFormat::MstcData floor{};
	RuntimeFormat::MstcData ring{};

	// ===== 床円盤 =====
	Check(RuntimeFormatIO::ReadMstc("Data/Model/mmdl/mstc/ArenaFloor.mstc", floor), "Floor: MSTC読み込み成功");
	p_floor = &floor;

	Check(p_floor->MaterialPath == "ArenaFloor.mmat", "Floor: マテリアルパスが正しい");
	Check(p_floor->Indices.size() % 3 == 0, "Floor: インデックス数が3の倍数");
	Check(!p_floor->Vertices.empty(), "Floor: 頂点が空でない");

	bool index_valid = true;
	for (const std::uint16_t index : p_floor->Indices) {
		if (index >= p_floor->Vertices.size()) { index_valid = false; }
	}
	Check(index_valid, "Floor: インデックスが頂点数未収録");

	const RuntimeFormat::StaticVertex& center = p_floor->Vertices[0];
	Check(IsNear(center.Position.x, 0.0f) && IsNear(center.Position.y, 0.0f) && IsNear(center.Position.z, 0.0f),
		"Floor: 中心頂点が原点");

	bool on_ground = true;
	bool within_radius = true;
	bool has_edge = false;
	bool normal_up = true;
	for (const RuntimeFormat::StaticVertex& v : p_floor->Vertices) {
		if (!IsNear(v.Position.y, 0.0f)) { on_ground = false; }
		const float r = Length(v.Position.x, 0.0f, v.Position.z);
		if (r > kArenaRadius + kEps) { within_radius = false; }
		if (r > kArenaRadius - kEps) { has_edge = true; }
		if (!IsNear(v.Normal.y, 1.0f) || !IsNear(v.Normal.x, 0.0f) || !IsNear(v.Normal.z, 0.0f)) { normal_up = false; }
	}
	Check(on_ground,     "Floor: 全頂点がy=0平面上");
	Check(within_radius, "Floor: 全頂点が半径20m以内");
	Check(has_edge,      "Floor: 外周(半径ほぼ20m)の頂点を持つ");
	Check(normal_up,     "Floor: 法線が全て+y方向");

	RuntimeFormat::MmatData floor_material{};
	Check(RuntimeFormatIO::ReadMmat("Data/Model/mmdl/mmat/ArenaFloor.mmat", floor_material), "Floor: MMAT読み込み成功");
	Check(IsNear(floor_material.Diffuse.w, 1.0f), "Floor: Diffuse.aが不透明");
	Check(floor_material.BaseColorTexturePath.empty() && floor_material.ToonTexturePath.empty(),
		"Floor: 単色マテリアル(テクスチャ無し)");

	// ===== 境界リング =====
	Check(RuntimeFormatIO::ReadMstc("Data/Model/mmdl/mstc/ArenaRing.mstc", ring), "Ring: MSTC読み込み成功");
	Check(ring.MaterialPath == "ArenaRing.mmat", "Ring: マテリアルパスが正しい");

	index_valid = true;
	for (const std::uint16_t index : ring.Indices) {
		if (index >= ring.Vertices.size()) { index_valid = false; }
	}
	Check(index_valid, "Ring: インデックスが頂点数未収録");

	bool height_ok = true;
	bool band_ok = true;
	bool horizontal_normal = true;
	for (const RuntimeFormat::StaticVertex& v : ring.Vertices) {
		if (v.Position.y < -kEps || v.Position.y > kRingHeight + kEps) { height_ok = false; }
		const float r = Length(v.Position.x, 0.0f, v.Position.z);
		if (r < kArenaRadius - 0.5f || r > kArenaRadius + 0.5f) { band_ok = false; }
		if (!IsNear(v.Normal.y, 0.0f)) { horizontal_normal = false; }
	}
	Check(height_ok,          "Ring: 全頂点の高さが1.2m以下");
	Check(band_ok,            "Ring: 全頂点がr=20m付近のバンド内");
	Check(horizontal_normal,  "Ring: 法線が全て水平方向");
	Check(ring.Vertices.size() >= 64, "Ring: 円周を十分な分割数で持つ");

	RuntimeFormat::MmatData ring_material{};
	Check(RuntimeFormatIO::ReadMmat("Data/Model/mmdl/mmat/ArenaRing.mmat", ring_material), "Ring: MMAT読み込み成功");
	const bool glow_boosted = ring_material.Ambient.x > 0.0f && ring_material.Ambient.y > 0.0f && ring_material.Ambient.z > 0.0f;
	Check(glow_boosted, "Ring: Ambientが設定済み(発光風の底上げ)");
	Check(IsNear(ring_material.Diffuse.w, 1.0f), "Ring: Diffuse.aが不透明");

	// ===== レベルJSONとの整合 =====
	const LevelDesc level = LevelData::LoadFromFile("Data/Json/Level/main.json");
	Check(level.PlayerSpawn.HasValue, "Level: PlayerSpawnが定義済み");
	Check(level.BossSpawn.HasValue,   "Level: BossSpawnが定義済み");

	bool has_floor_object = false;
	bool has_ring_object = false;
	int cube_count = 0;
	for (const LevelObjectDesc& object : level.Objects) {
		if (object.MstcFile == "ArenaFloor.mstc") { has_floor_object = true; }
		if (object.MstcFile == "ArenaRing.mstc") { has_ring_object = true; }
		if (object.MstcFile == "Cube.mstc") { ++cube_count; }
	}
	Check(has_floor_object, "Level: ArenaFloor.mstcを配置済み");
	Check(has_ring_object,  "Level: ArenaRing.mstcを配置済み");
	Check(cube_count >= 2,  "Level: 既存のCube配置が残っている");

	const float player_r = Length(level.PlayerSpawn.Position.x, 0.0f, level.PlayerSpawn.Position.z);
	const float boss_r = Length(level.BossSpawn.Position.x, 0.0f, level.BossSpawn.Position.z);
	Check(player_r < kArenaRadius, "Level: Playerスポーンがアリーナ内");
	Check(boss_r < kArenaRadius,   "Level: Bossスポーンがアリーナ内");

	std::cout << "\nAll " << g_CheckCount << " checks passed.\n";
	return 0;
}
