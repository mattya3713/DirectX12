#pragma once

/**********************************************************************************
* @author    : Coder 青龍(せいりゅう).
* @date      : 2026/08/23.
* @brief     : モデルCPUパースをワーカースレッドで実行する非同期ローダー.
*            : スレッド境界: ワーカー=ファイル読込+CPUパース(LoadFromFiles) /
*            : メインスレッド=GPUリソース生成(CreateResources)と描画への接続.
*            : 同一パスの要求は同一リクエストへ収束(重複ロードなし).
**********************************************************************************/

#include <atomic>
#include <condition_variable>
#include <deque>
#include <filesystem>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>

#include "10_Ggraphic/30_Asset/RuntimeModel/MMdl/MmdlResource.h"

enum class eModelLoadState
{
	Loading   = 0, // CPUパース中(または順番待ち).
	CpuParsed = 1, // CPUパース完了(GPUリソース生成とメッシュ接続をメインスレッドで行う段階).
	Failed    = 2, // ロード失敗(Errorに原因を保持).
	Ready     = 3, // 全て完了(描画可能).
};

// ロード要求1件分(スレッドセーフ. メイン/ワーカー双方から触れる).
class AsyncModelRequest final
{
public:
	const std::filesystem::path& GetPath() const noexcept { return m_FilePath; }
	eModelLoadState GetState() const
	{
		std::lock_guard<std::mutex> lock(m_Mutex);
		return m_State;
	}
	std::shared_ptr<MmdlResource> GetResource() const
	{
		std::lock_guard<std::mutex> lock(m_Mutex);
		return m_Resource;
	}
	std::string GetError() const
	{
		std::lock_guard<std::mutex> lock(m_Mutex);
		return m_Error;
	}

	// メインスレッドでGPUリソース生成とメッシュ接続を完了した後に呼ぶ(Ready遷移).
	void MarkGpuAttached()
	{
		std::lock_guard<std::mutex> lock(m_Mutex);
		if (m_State == eModelLoadState::CpuParsed) { m_State = eModelLoadState::Ready; }
	}

	// ワーカーキューへの積み込み済みフラグ(重複積み防止用).
	bool IsQueued() const noexcept { return m_Queued.load(); }
	void SetQueued() noexcept { m_Queued.store(true); }

	// 失敗ログの二重出力防止フラグ(MainScene側で使用).
	bool IsFailureLogged() const noexcept { return m_FailureLogged.load(); }
	void SetFailureLogged() noexcept { m_FailureLogged.store(true); }

private:
	friend class AsyncModelLoader;

	void Complete(std::shared_ptr<MmdlResource> Resource)
	{
		std::lock_guard<std::mutex> lock(m_Mutex);
		m_Resource = std::move(Resource);
		m_State    = eModelLoadState::CpuParsed;
	}
	void Fail(const std::string& Message)
	{
		std::lock_guard<std::mutex> lock(m_Mutex);
		m_Error = Message;
		m_State = eModelLoadState::Failed;
	}


	std::filesystem::path         m_FilePath;
	std::shared_ptr<MmdlResource> m_Resource;
	std::string                   m_Error;
	eModelLoadState               m_State = eModelLoadState::Loading;
	std::atomic<bool>             m_Queued{ false };
	std::atomic<bool>             m_FailureLogged{ false };
	mutable std::mutex            m_Mutex;
};

class AsyncModelLoader final
{
public:
	AsyncModelLoader() = default;
	~AsyncModelLoader() { Shutdown(); }

	AsyncModelLoader(const AsyncModelLoader&)            = delete;
	AsyncModelLoader& operator=(const AsyncModelLoader&) = delete;

	// ワーカースレッドを起動する(MainScene::Createで呼ぶ).
	bool Initialize();

	// ワーカーを停止し、残りのジョブを処理してから終了する.
	void Shutdown();

	// モデルロードを要求する(同一正規化パスは同一リクエストを返す).
	std::shared_ptr<AsyncModelRequest> Request(const std::filesystem::path& Path);

private:
	void WorkerLoop();

	std::thread                                                    m_Worker;
	std::mutex                                                     m_QueueMutex;
	std::condition_variable                                        m_QueueCv;
	std::deque<std::shared_ptr<AsyncModelRequest>>                 m_JobQueue;
	std::unordered_map<std::string, std::weak_ptr<AsyncModelRequest>> m_RequestMap; // 重複要求排除用.
	std::atomic<bool>                                              m_Shutdown{ false };
};
