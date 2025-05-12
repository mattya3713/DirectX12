
#include "Buffer.h"

// バッファを追加.
void Buffer::AddBuffer(Any buffer)
{
    m_pBuffer.push_back(std::move(buffer));
}

// バッファ消去.
void Buffer::ClearBuffer()
{
    m_pBuffer.clear();
}

// バッファ取得.
const Any& Buffer::GetBuffer(size_t index) const
{
    return m_pBuffer.at(index);
}