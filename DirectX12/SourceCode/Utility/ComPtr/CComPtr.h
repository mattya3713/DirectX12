#pragma once
#include <wrl/client.h>
#include <utility> // std::swap を使用するためにインクルード

template <typename T>
class MyComPtr {
public:
    // デフォルトコンストラクタ.
    MyComPtr() : m_ptr(nullptr) {}

    // ポインタを受け取るコンストラクタ.
    explicit MyComPtr(T* ptr) : m_ptr(ptr) {
        if (m_ptr) {
            m_ptr->AddRef();
        }
    }

    // コピーコンストラクタ.
    MyComPtr(const MyComPtr& other) : m_ptr(other.m_ptr) {
        if (m_ptr) {
            m_ptr->AddRef();
        }
    }

    // ムーブコンストラクタ.
    MyComPtr(MyComPtr&& other) noexcept : m_ptr(other.m_ptr) {
        // 移動元を無効にする.
        other.m_ptr = nullptr;
    }

    // コピー代入演算子.
    MyComPtr& operator=(const MyComPtr& other) {
        if (this != &other) {
            Reset(); // 現在のポインタを解放.
            m_ptr = other.m_ptr;
            if (m_ptr) {
                m_ptr->AddRef();
            }
        }
        return *this;
    }

    // ムーブ代入演算子.
    MyComPtr& operator=(MyComPtr&& other) noexcept {
        if (this != &other) {
            Reset(); // 現在のポインタを解放.
            m_ptr = other.m_ptr;
            other.m_ptr = nullptr; // 移動元を無効にする.
        }
        return *this;
    }

    // デストラクタ.
    ~MyComPtr() {
        Reset();
    }

    // 内部ポインタの取得.
    T* Get() const { return m_ptr; }

    // ポインタのデリファレンス.
    T& operator*() const { return *m_ptr; }

    // ポインタへのアクセス.
    T* operator->() const { return m_ptr; }

    // 新しいポインタを設定する.
    void Reset(T* ptr = nullptr) {
        if (m_ptr) {
            m_ptr->Release();
        }

        m_ptr = ptr;

        if (m_ptr) {
            m_ptr->AddRef();
        }
    }

    // ポインタを交換する.
    void Swap(MyComPtr& other) noexcept {
        std::swap(m_ptr, other.m_ptr);
    }

    // 内部ポインタのアドレスを取得する.
    T** GetAddressOf() {
        Reset();
        return &m_ptr;
    }

    // 現在のポインタを解放し、そのアドレスを返す.
    T** ReleaseAndGetAddressOf() {
        T** address = &m_ptr;
        Reset();
        return address;
    }

private:
    T* m_ptr; // COM オブジェクトのポインタ.
};