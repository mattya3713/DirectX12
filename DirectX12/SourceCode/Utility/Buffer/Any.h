#pragma once

#include <memory>
#include <typeinfo>
#include <stdexcept>
#include <vector>

// 型消去クラス.

class Any
{
private:
    struct Base
    {
        virtual ~Base() = default;
        virtual std::unique_ptr<Base> Clone() const = 0; // クローンを作成する純粋仮想関数.
        virtual const std::type_info& Type() const = 0;  // 型情報を取得する純粋仮想関数.
    };

    // 値型を保持するホルダー.
    template<typename T>
    struct Holder : Base
    {
        Holder(T value) : value(std::move(value)) {}
        std::unique_ptr<Base> Clone() const override
        {
            return std::make_unique<Holder<T>>(*this);
        }
        const std::type_info& Type() const override
        {
            return typeid(T);
        }

        // 実際の値.
        T value; 
    };

    // 生ポを保持するホルダー.
    template<typename T>
    struct PtrHolder : Base
    {
        PtrHolder(T* value) : value(value) {}
        std::unique_ptr<Base> Clone() const override
        {
            return std::make_unique<PtrHolder<T>>(value);
        }
        const std::type_info& Type() const override
        {
            return typeid(T*);
        }

        // 生ポで保持された値.
        T* value;
    };

    // std::unique_ptr<T> を保持するホルダー.
    template<typename T>
    struct UniquePtrHolder : Base
    {
        UniquePtrHolder(std::unique_ptr<T> value) : value(std::move(value)) {}
        std::unique_ptr<Base> Clone() const override
        {
            return std::make_unique<UniquePtrHolder<T>>(std::make_unique<T>(*value));
        }
        const std::type_info& Type() const override
        {
            return typeid(std::unique_ptr<T>);
        }

        // std::unique_ptr で保持された値.
        std::unique_ptr<T> value; 
    };

    // std::vector<std::unique_ptr<T>> を保持するホルダー.
    template<typename T>
    struct VectorUniquePtrHolder : Base
    {
        VectorUniquePtrHolder(std::vector<std::unique_ptr<T>> value) : value(std::move(value)) {}

        std::unique_ptr<Base> Clone() const override
        {
            std::vector<std::unique_ptr<T>> clonedValue;
            clonedValue.reserve(value.size());
            for (const auto& ptr : value)
            {
                clonedValue.push_back(std::make_unique<T>(*ptr));
            }
            return std::make_unique<VectorUniquePtrHolder<T>>(std::move(clonedValue));
        }

        const std::type_info& Type() const override
        {
            return typeid(std::vector<std::unique_ptr<T>>);
        }

        // std::vector<std::unique_ptr<T>> で保持された値.
        std::vector<std::unique_ptr<T>> value;
    };

public:
    Any() = default;

    // 値型を受け取るコンストラクタ.
    template<typename T>
    Any(T value) : ptr(std::make_unique<Holder<T>>(std::move(value))) {}

    // 生ポを受け取るコンストラクタ.
    template<typename T>
    Any(T* value) : ptr(std::make_unique<PtrHolder<T>>(value)) {}

    // std::unique_ptr<T> を受け取るコンストラクタ.
    template<typename T>
    Any(std::unique_ptr<T> value) : ptr(std::make_unique<UniquePtrHolder<T>>(std::move(value))) {}

    // std::vector<std::unique_ptr<T>> を受け取るコンストラクタ.
    template<typename T>
    Any(std::vector<std::unique_ptr<T>> value) : ptr(std::make_unique<VectorUniquePtrHolder<T>>(std::move(value))) {}

    // コピーコンストラクタ.
    Any(const Any& other) : ptr(other.ptr ? other.ptr->Clone() : nullptr) {}

    // コピー代入演算子.
    Any& operator=(const Any& other)
    {
        if (this != &other)
        {
            ptr = other.ptr ? other.ptr->Clone() : nullptr;
        }
        return *this;
    }

    // ムーブコンストラクタ.
    Any(Any&&) noexcept = default;
    // ムーブ代入演算子.
    Any& operator=(Any&&) noexcept = default;

    // 値型を取得するためのメソッド.
    template<typename T>
    T& Get() const
    {
        if (typeid(T) != ptr->Type())
        {
            throw std::bad_cast();
        }
        return static_cast<Holder<T>*>(ptr.get())->value;
    }

    // 生ポを取得するためのメソッド.
    template<typename T>
    T* GetPtr() const
    {
        if (typeid(T*) != ptr->Type())
        {
            throw std::bad_cast();
        }
        return static_cast<PtrHolder<T>*>(ptr.get())->value;
    }

    // std::unique_ptr<T> を取得するためのメソッド.
    template<typename T>
    std::unique_ptr<T>& GetUniquePtr() const
    {
        if (typeid(std::unique_ptr<T>) != ptr->Type())
        {
            throw std::bad_cast();
        }
        return static_cast<UniquePtrHolder<T>*>(ptr.get())->value;
    }

    // std::vector<std::unique_ptr<T>> を取得するためのメソッド.
    template<typename T>
    std::vector<std::unique_ptr<T>>& GetVector() const
    {
        if (typeid(std::vector<std::unique_ptr<T>>) != ptr->Type())
        {
            throw std::bad_cast();
        }
        return static_cast<VectorUniquePtrHolder<T>*>(ptr.get())->value;
    }

    // ヌルポインタかどうかを判定するメソッド.
    bool IsNull() const { return !ptr; } 

private:
    std::unique_ptr<Base> ptr; // 基底クラスのポインタを保持.
};