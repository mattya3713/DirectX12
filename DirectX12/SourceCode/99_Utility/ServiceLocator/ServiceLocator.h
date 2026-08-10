#pragma once

/**********************************************************************************
* @author    : mattya3713.
* @date      : 2026/08/07.
* @brief     : 型ごとにサービス(非所有ポインタ)を登録・取得するサービスロケーター.
*            : 実体の生成・破棄は呼び出し側(所有者)が担当し、ここでは参照のみを保持する.
* @pattern   : ServiceLocator.
**********************************************************************************/

class ServiceLocator final
{
public:
	ServiceLocator() = delete;

	// サービスを登録する(所有権は移らない。nullptrを渡せば登録解除).
	template<typename T>
	static void Provide(T* Service) noexcept
	{
		GetSlot<T>() = Service;
	}

	// 登録済みのサービスを取得する(未登録ならnullptr).
	template<typename T>
	static T* Get() noexcept
	{
		return GetSlot<T>();
	}

private:
	// 型ごとに独立したstatic領域を返す(テンプレートが型ごとにインスタンス化される性質を利用).
	template<typename T>
	static T*& GetSlot() noexcept
	{
		static T* instance = nullptr;
		return instance;
	}
};
