#pragma once

#include <DirectXMath.h>
#include <filesystem>
#include <functional>
#include <string>
#include <vector>

/**********************************************************************************
* @author    : 朱雀 (Suzaku / 閃斬 Production Loop Coder).
* @date      : 2026/08/23.
* @brief     : レベルシーンエディター(v1). .mstc静的オブジェクトの配置
*            : (位置・回転・スケール)とPlayer/Bossスポーン地点を編集し、
*            : Data\Json\Level配下のJSONへ保存/読込するImGuiツール.
*            : 保存・読込時にOnLevelChangedコールバックを飛ばすため、
*            : MainSceneはそれを受けてレベルを再構築できる.
*            : 当たり判定・地形高低差は対象外(見た目の配置のみ).
**********************************************************************************/

struct LevelObjectDesc
{
	std::string          MstcFile;    // Data\Model\mmdl\mstc配下のファイル名.
	DirectX::XMFLOAT3    Position     = { 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT3    RotationDeg  = { 0.0f, 0.0f, 0.0f }; // オイラー角(度. X=Pitch, Y=Yaw, Z=Roll).
	DirectX::XMFLOAT3    Scale        = { 1.0f, 1.0f, 1.0f };
};

struct LevelSpawnDesc
{
	bool                 HasValue = false; // JSONにキーが存在したか(無い場合は既存のハードコード値を使う).
	DirectX::XMFLOAT3    Position = { 0.0f, 0.0f, 0.0f };
	float                YawDeg   = 0.0f;
};

struct LevelDesc
{
	std::vector<LevelObjectDesc> Objects;
	LevelSpawnDesc PlayerSpawn;
	LevelSpawnDesc BossSpawn;
};

class LevelEditor final
{
public:
	LevelEditor();
	~LevelEditor() = default;

	// 毎フレーム呼ぶ. ファイル選択・オブジェクト編集・保存/読込を行う.
	void Draw();

	// 保存・読込でレベルJSONが確定するたびに呼ばれるコールバックを設定する.
	void SetOnLevelChanged(std::function<void(const std::filesystem::path&)> Callback);

	// 現在編集中のレベルJSONパス.
	const std::filesystem::path& GetCurrentPath() const noexcept { return m_SelectedPath; }

	// レベルJSONを読み込む(ファイルが無い・空の場合は空のLevelDescを返す).
	static LevelDesc LoadLevelJson(const std::filesystem::path& Path);

private:
	// Data\Json\Level配下の*.jsonを走査して選択肢を作る.
	void ScanFiles();

	// 選択中のJSONを読み込み、編集データへ反映する.
	void LoadSelected();

	// 編集内容を選択中(または新規名)のJSONへ書き戻す.
	bool SaveSelected();

	// Data\Model\mmdl\mstc配下の*.mstcを走査して選択肢を作る.
	void ScanMstcFiles();

private:
	static constexpr const char* kJsonDir  = "Data\\Json\\Level";
	static constexpr const char* kMstcDir  = "Data\\Model\\mmdl\\mstc";

	std::vector<std::string>    m_FileNames;         // 選択肢(レベルJSONファイル名).
	std::string                 m_SelectedFile;      // 現在選択中のレベルJSONファイル名.
	std::filesystem::path       m_SelectedPath;      // 現在選択中のレベルJSONフルパス.

	std::vector<std::string>    m_MstcFileNames;     // 配置候補の.mstcファイル名.

	std::vector<LevelObjectDesc> m_Objects;          // 編集中の配置オブジェクト.
	LevelSpawnDesc              m_PlayerSpawn;       // Player初期位置.
	LevelSpawnDesc              m_BossSpawn;         // Boss初期位置.

	char m_NewFileName[128] = {};                    // 新規保存用のファイル名入力バッファ.

	std::function<void(const std::filesystem::path&)> m_OnLevelChanged; // レベル確定時のコールバック.
};
