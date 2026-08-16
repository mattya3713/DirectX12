#pragma once
#include "VMDStructHeader.h"

// VMD ファイルを読み込むクラス (または静的関数群)
class VMDLoader
{
public:
    static VMD::MotionData Load(const std::string& filepath);
private:
    static void ReadString(std::ifstream& ifs, char* buffer, size_t bufferSize);
};
