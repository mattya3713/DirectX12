#pragma once

/***********************************************************************************
* Singletonテンプレートクラス.
* 参考元：https://shinh.skr.jp/template/singleton.html
*
* 使用方法：
*	特定のクラスをシングルトンにする場合、このクラスを継承させる.
*
* 注意：friend classの宣言をする.
*		コンストラクタをprivateにする.
*
* 使用例（MyClassの場合）：
*	class MyClass : public Singleton<MyClass>
*	{
*		friend class Singleton<MyClass>; // Singletonからのアクセスを許可.
*	private:
*		MyClass(){}// コンストラクタをprivateにする.
*	public:
*		void DoSomething(){
*			// 何らかの処理.
*		}
*	}
*
* 使用時：
*	MyClass* instance = MyClass::GetInstance();
*
***********************************************************************************/

template<typename T>
class Singleton
{
public:
	/************************************************
	* @brief シングルトンのインスタンスを取得.
	* @return インスタンスのポインタ.
	************************************************/
	static T* GetInstance()
	{
		static T instance;
		return &instance;
	}
protected:
	// 継承先以外でのコンストラクタ、デストラクタの使用禁止.
	Singleton() {};
	virtual ~Singleton() {};
private:
	//コピー,代入禁止.
	Singleton(const Singleton& obj) = delete;
	Singleton& operator = (const Singleton& obj) = delete;
};