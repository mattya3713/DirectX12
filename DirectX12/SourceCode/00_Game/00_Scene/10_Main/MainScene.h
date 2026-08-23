#pragma once

#include <filesystem>
#include <memory>
#include <unordered_set>
#include <vector>

#include "00_Game/00_Scene/00_Base/SceneBase.h"

class MmdlRenderer;
class MstcRenderer;
class MstcActor;
class DirectionLight;
class SpriteRenderer;
class TextRenderer;
class Player;
class Boss;
class Enemy;
class EnemyDefinitionCatalog;
class PooledEnemyFactory;
class ModelPreviewPanel;
class ThirdPersonCamera;
class LockOnCamera;
class CutScenePlayer;
class ParticleSystem;
class EventBus;
class UILayoutRuntime;
// 非同期モデルロードはDebug/Release共通の機能(Player/Bossのモデル読込に常時使う).
class AsyncModelLoader;
class AsyncModelRequest;
#if _DEBUG
class CutSceneEditor;
class LevelEditor;
class CombatTuningEditor;
class ParticleSystemEditor;
class SoundEventEditor;
#endif

/**********************************************************************************
* @author    : mattya3713.
* @date      : 2026/08/10.
* @brief     : メインシーン.
**********************************************************************************/

class MainScene final : public SceneBase
{
public:
	MainScene();
	~MainScene() override;

	void Initialize() override;
	void Create() override;
	void Update() override;
	void LateUpdate() override;
	void Draw() override;

#if _DEBUG
	// 撃破シーケンスのデバッグ用強制発動(演出確認用. 正式な発動経路はゲージMAX+ヒット).
	void DebugStartFinisherSequence();
#endif

public:
	// 【演出完了通知API】撃破カットシーン側から呼ぶ(演出完了→ゲーム状態を次へ進める).
	// カットシーン内容自体はユーザー実装。本メソッドは状態区切りのみ担当する.
	void NotifyFinisherCutsceneFinished();

private:
	std::shared_ptr<MmdlRenderer> m_pMmdlRenderer;
	std::unique_ptr<DirectionLight> m_upDirectionLight; // 平行光源(デバッグパネルで方向・色を調整可能).
	std::unique_ptr<SpriteRenderer> m_upSpriteRenderer; // Sprite2D/Sprite3D描画基盤.
	std::unique_ptr<TextRenderer>    m_upTextRenderer;    // テキスト描画.

	std::unique_ptr<Player>      m_upPlayer;
	std::unique_ptr<Boss>        m_upBoss;

	// レベルJSONのEnemySpawnsから生成した雑魚敵(実体の所有はPooledEnemyFactoryのプール).
	std::unique_ptr<EnemyDefinitionCatalog> m_upEnemyCatalog;       // 遅延ロード(初回スポーン時にJSONから読む).
	std::unique_ptr<PooledEnemyFactory>     m_upPooledEnemyFactory; // 敵実体プール(再ロード時は破棄せず返却して再利用する).
	std::vector<Enemy*>                     m_pEnemies;             // 使用中の敵(非所有. 所有はプール).
	std::unordered_set<Enemy*>              m_MeshAssignedEnemies;  // メッシュ割当済みインスタンス(再利用時に再アタッチしてGPU再allocを避ける).
#if _DEBUG
	// 敵スポーン結果とプール統計のDEBUG表示用.
	size_t m_EnemySpawnPlanned  = 0; // 直近ロードの計画数(除外前).
	size_t m_EnemySpawnedCount  = 0; // 直近ロードの生成成功数.
	size_t m_EnemyNewAllocCount = 0; // 直近ロードでの新規生成数(再ロード後に0なら全個体が再利用).
	std::vector<std::pair<std::string, std::string>> m_EnemySpawnIssues; // (DefinitionId, 理由).
#endif

	std::unique_ptr<CutScenePlayer> m_upCutScenePlayer; // カットシーンランタイム再生.
	std::unique_ptr<ParticleSystem> m_upParticleSystem; // パーティクルシステム(VFX基盤).
	std::unique_ptr<UILayoutRuntime> m_upUILayoutRuntime; // layout.jsonランタイムUI(HPバー等のHUD).
#if _DEBUG
	std::unique_ptr<CutSceneEditor> m_upCutSceneEditor; // カットシーン編集ツール(デバッグのみ).
	std::unique_ptr<SoundEventEditor> m_upSoundEventEditor; // Sound Event編集ツール(デバッグのみ).
	std::unique_ptr<ParticleSystemEditor> m_upParticleEditor; // パーティクル編集ツール(デバッグのみ).
	std::unique_ptr<LevelEditor>    m_upLevelEditor;    // レベルシーン編集ツール(デバッグのみ).
	std::unique_ptr<CombatTuningEditor> m_upCombatTuningEditor; // Combat調整ツール(デバッグのみ).
#endif

	// 非同期モデルロード(Debug/Release共通. ロード完了前もゲーム更新を継続するために使う).
	std::unique_ptr<AsyncModelLoader>  m_upAsyncModels;      // 非同期モデルローダー.
	std::shared_ptr<AsyncModelRequest> m_PlayerModelRequest; // Playerモデルのロード要求.
	std::shared_ptr<AsyncModelRequest> m_BossModelRequest;   // Bossモデルのロード要求.

	// レベルデータ(Data\Json\Level配下)から復元した静的オブジェクト.
	std::shared_ptr<MstcRenderer>               m_pMstcRenderer;
	std::vector<std::unique_ptr<MstcActor>>     m_LevelActors;
	std::filesystem::path                       m_LevelPath; // 現在読み込んでいるレベルJSON.

	void LoadLevelFromJson(const std::filesystem::path& Path); // レベルJSONから静的オブジェクト・スポーンを再構築する.
	void SpawnEnemiesFromLevel(const struct LevelDesc& Level); // 敵スポーン計画をPool経由でEnemy実体へ変換する(容量枯渇等の失敗個体はログしてスキップ).

	ThirdPersonCamera* m_pThirdPersonCamera = nullptr; // 追従対象を渡すための非所有ポインタ(所有はCameraManager).
	LockOnCamera*      m_pLockOnCamera      = nullptr; // Player/Boss位置を渡すための非所有ポインタ(所有はCameraManager).

	bool m_IsGameOver = false;     // 勝敗確定フラグ(確定後はPlayer/Bossの更新を止める).
	bool m_WinnerIsPlayer = false; // true=Player勝利(WIN). false=Boss勝利(LOSE).

	// ===== 撃破シーケンス基盤(演出内容はユーザー実装. 玄武は状態遷移とフックのみ) =====
	enum class FinisherPhase
	{
		None,      // 通常戦闘中.
		Playing,   // 撃破成立〜演出再生中(Combat/AI停止. CutScenePlayerへ制御移譲).
		Completed, // 演出完了通知済み(ゲーム状態を次へ進めた状態).
	};
	FinisherPhase m_FinisherPhase = FinisherPhase::None;
	float m_PrevBossHpForFinisher = -1.0f; // 前フレームのBoss HP(必殺ヒット検出用).

	void TickFinisherSequence();      // ゲージ加速・成立判定・演出開始フックを毎フレーム処理する.
	void BeginFinisherSequence();     // 撃破成立: 戦闘停止+イベント発行+演出開始フックを呼ぶ.
};
