#pragma once

#include <functional>
#include <typeindex>
#include <unordered_map>
#include <vector>

/**********************************************************************************
* @author    : Coder 玄武(閃斬 Production Loop).
* @date      : 2026/08/23.
* @brief     : 汎用Publish/Subscribe型イベントバス(即時配信・シングルスレッド).
*            : イベント型(単純な構造体)ごとに購読者リストを管理し、Publishで
*            : 全購読者へ同時通知する。ServiceLocator経由で取得して使う.
*            : NOTE: 購読解除はUnsubscribeAll()(イベント型単位)のみの最小構成.
*            :       個別解除・生存期間管理は本格導入タスクで設計する.
**********************************************************************************/

class EventBus final
{
public:
	EventBus() = default;
	~EventBus() = default;

	EventBus(const EventBus&)            = delete;
	EventBus& operator=(const EventBus&) = delete;
	EventBus(EventBus&&)                 = delete;
	EventBus& operator=(EventBus&&)      = delete;

	// イベント型TEventの購読を登録する(同一ハンドラの重複登録も可能. 呼ばれた回数分通知される).
	template<typename TEvent>
	void Subscribe(std::function<void(const TEvent&)> Handler)
	{
		m_Handlers[std::type_index(typeid(TEvent))].push_back(
			[Handler = std::move(Handler)](const void* p_event) {
				Handler(*static_cast<const TEvent*>(p_event));
			});
	}

	// イベントを全購読者へ通知する(購読者がいない場合は何もしない).
	// Publish中に購読者リストが変わっても安全なよう、呼び出し前にリストをコピーする.
	template<typename TEvent>
	void Publish(const TEvent& Event)
	{
		const auto it = m_Handlers.find(std::type_index(typeid(TEvent)));
		if (it == m_Handlers.end()) { return; }

		const auto handlers = it->second;
		for (const auto& handler : handlers)
		{
			handler(&Event);
		}
	}

	// 指定イベント型の購読を全て解除する(デモ/テスト用の最小限の解除手段).
	template<typename TEvent>
	void UnsubscribeAll()
	{
		m_Handlers.erase(std::type_index(typeid(TEvent)));
	}

private:
	std::unordered_map<std::type_index, std::vector<std::function<void(const void*)>>> m_Handlers;
};
