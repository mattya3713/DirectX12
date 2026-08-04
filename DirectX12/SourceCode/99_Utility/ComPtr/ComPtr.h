#pragma once
#include <wrl\\client.h>
#include <utility> // std::swap を使用するためにインクルード

template <typename T>
class MyComPtr {
public:
    // デフォルトコンストラクタ.
    MyComPtr() : m_ptr(nullptr) {}

    // ポインタを受け取るコンストラクタ.
    explicit MyComPtr(T* ptr) : m_ptr(ptr) {
        if (m_ptr) { AddRef(); }
    }

    // コピーコンストラクタ.
    MyComPtr(const MyComPtr& other) : m_ptr(other.m_ptr) {
        if (m_ptr) { AddRef(); }
    }

    // ムーブコンストラクタ.
    MyComPtr(MyComPtr&& other) noexcept : m_ptr(other.m_ptr) {
        other.m_ptr = nullptr; // 移動元を無効にする.
    }

    // コピー代入演算子.
    MyComPtr& operator=(const MyComPtr& other) {
        if (this != &other) {
            if (m_ptr) { m_ptr->Release(); }
            m_ptr = other.m_ptr;
            if (m_ptr) { m_ptr->AddRef(); }
        }
        return *this;
    }

    // ムーブ代入演算子.
    MyComPtr& operator=(MyComPtr&& other) noexcept {
        if (this != &other) {
            if (m_ptr) { m_ptr->Release(); }
            m_ptr = other.m_ptr;
            other.m_ptr = nullptr; // 移動元を無効にする.
        }
        return *this;
    }

    // デストラクタ.
    ~MyComPtr() {
        if (m_ptr) { m_ptr->Release(); }
    }

    // 内部ポインタの取得.
    T* Get() const { return m_ptr; }

    // ポインタのデリファレンス.
    T& operator*() const { return *m_ptr; }

    // ポインタへのアクセス.
    T* operator->() const { return m_ptr; }

	// ポインタの比較演算子.
    bool operator!() const { return m_ptr == nullptr; }

    // nullptr でなければ true を返す.
    operator bool() const { return m_ptr != nullptr; }

    // 新しいポインタを設定する.
    void Reset(T* ptr = nullptr) {
        T* old_ptr = m_ptr;

        m_ptr = ptr;

        if (m_ptr) { m_ptr->AddRef(); }

        if (old_ptr) { old_ptr->Release(); }
    }

    // ポインタを交換する.
    void Swap(MyComPtr& other) noexcept {
        std::swap(m_ptr, other.m_ptr);
    }

    // 内部ポインタのアドレスを取得する.
    T** GetAddressOf() {
        if (m_ptr) { m_ptr->Release(); }
        m_ptr = nullptr; 
        return &m_ptr;
    }

    // 現在のポインタを解放し、そのアドレスを返す.
    T** ReleaseAndGetAddressOf() {
        if (m_ptr) { m_ptr->Release(); }
        m_ptr = nullptr; // nullptr に設定
        return &m_ptr;
    }

    // 現在のポインタを解放し、内部ポインタを nullptr に設定する(所有権を放棄).
    T* Detach() noexcept
    {
        T* ptr = m_ptr;
        m_ptr = nullptr;
        return ptr;
    }
private:
    // COM オブジェクトの参照カウントを増加させる.
    void AddRef() {
        if (m_ptr) {
            m_ptr->AddRef();
        }
    }

    T* m_ptr; // COM オブジェクトのポインタ.
};
