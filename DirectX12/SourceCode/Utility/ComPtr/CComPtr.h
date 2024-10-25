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

    // デストラクタ.
    ~MyComPtr() {
        Reset();
    }

    // コピーコンストラクタは削除.
    MyComPtr(const MyComPtr&) = delete;

    // ムーブコンストラクタ.
    MyComPtr(MyComPtr&& other) noexcept : m_ptr(other.m_ptr) {
        // 移動元を無効にする.
        other.m_ptr = nullptr; 
    }

    // コピー代入演算子は削除.
    MyComPtr& operator=(const MyComPtr&) = delete;

    // ムーブ代入演算子.
    MyComPtr& operator=(MyComPtr&& other) noexcept {
        if (this != &other) {
            // 現在のポインタを解放.
            Reset(); 
            // 新しいポインタを移動.
            m_ptr = other.m_ptr;
            // 移動元を無効にする.
            other.m_ptr = nullptr; 
        }
        return *this;
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
            // 現在のポインタを解放.
            m_ptr->Release(); 
        }

        m_ptr = ptr;

        if (m_ptr) {
            // 新しいポインタを参照カウント.
            m_ptr->AddRef();
        }
    }

    // ポインタを交換する.
    void Swap(MyComPtr& other) noexcept {
        std::swap(m_ptr, other.m_ptr);
    }

    // 内部ポインタのアドレスを取得する.
    T** GetAddressOf() {

        // 以前のポインタを解放.
        Reset();

        // 内部ポインタのアドレスを返す.
        return &m_ptr;     
    }

    // 現在のポインタを解放し、そのアドレスを返す.
    T** ReleaseAndGetAddressOf() {

        // 現在のポインタのアドレスを保持.
        T** address = &m_ptr;

        Reset(); // 現在のポインタを解放.

        // 解放したポインタのアドレスを返す.
        return address;
    }

private:
    T* m_ptr; // COM オブジェクトのポインタ.
};