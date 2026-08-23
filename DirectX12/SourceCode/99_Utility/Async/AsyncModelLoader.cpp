#include "stdafx.h"
#include "AsyncModelLoader.h"

#include <deque>

// ワーカーループ(キュージョブを順番に処理する).
void AsyncModelLoader::WorkerLoop()
{
	while (true)
	{
		std::shared_ptr<AsyncModelRequest> job;

		{
			std::unique_lock<std::mutex> lock(m_QueueMutex);
			m_QueueCv.wait(lock, [this]() { return m_Shutdown.load() || !m_JobQueue.empty(); });

			if (m_Shutdown.load() && m_JobQueue.empty()) { return; }

			job = m_JobQueue.front();
			m_JobQueue.pop_front();
		}

		try
		{
			auto resource = std::make_shared<MmdlResource>(job->GetPath());
			resource->LoadFromFiles(); // CPUパースのみ(GPUリソース生成はメインスレッドで行う).
			job->Complete(resource);
		}
		catch (const std::exception& e)
		{
			job->Fail(e.what());
		}
		catch (...)
		{
			job->Fail("unknown error");
		}
	}
}

bool AsyncModelLoader::Initialize()
{
	if (m_Worker.joinable()) { return true; }

	m_Shutdown.store(false);
	m_Worker = std::thread([this]() { WorkerLoop(); });
	return true;
}

void AsyncModelLoader::Shutdown()
{
	if (!m_Worker.joinable()) { return; }

	m_Shutdown.store(true);
	m_QueueCv.notify_all();
	m_Worker.join();
}

std::shared_ptr<AsyncModelRequest> AsyncModelLoader::Request(const std::filesystem::path& Path)
{
	const std::string key = Path.lexically_normal().generic_string();

	std::shared_ptr<AsyncModelRequest> request;

	{
		std::lock_guard<std::mutex> lock(m_QueueMutex);

		// 期限切れエントリを掃除しつつ同一パスの要求を検索.
		for (auto it = m_RequestMap.begin(); it != m_RequestMap.end(); )
		{
			if (it->second.expired()) { it = m_RequestMap.erase(it); continue; }
			++it;
		}

		const auto map_it = m_RequestMap.find(key);
		if (map_it != m_RequestMap.end())
		{
			request = map_it->second.lock();
		}
		else
		{
			request = std::make_shared<AsyncModelRequest>();
			request->m_FilePath = Path; // ワーカーがこのパスを読むため、キューへ積む前に必ず設定する.
			m_RequestMap[key] = request;
		}
	}

	if (request->GetState() == eModelLoadState::Loading && !request->IsQueued())
	{
		request->SetQueued();
		{
			std::lock_guard<std::mutex> lock(m_QueueMutex);
			m_JobQueue.push_back(request);
		}
		m_QueueCv.notify_one();
	}

	return request;
}
