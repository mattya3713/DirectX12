// Buffer.h シーンをまたいでinstanceを呼び出したいときに使用.

#pragma once

#include <vector>
#include <memory>
#include "Any.h"

class Buffer
{
public:
    Buffer() = default;
    ~Buffer() = default;

    // バッファを追加.
    void AddBuffer(Any buffer);

    // バッファ消去.
    void ClearBuffer();

    // バッファ取得.
    const Any& GetBuffer(size_t index) const;

private:
    std::vector<Any> m_pBuffer;
};