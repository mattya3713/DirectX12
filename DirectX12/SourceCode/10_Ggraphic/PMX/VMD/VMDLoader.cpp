#include "stdafx.h"
#include "VMDLoader.h"

VMD::MotionData VMDLoader::Load(const std::string& filepath)
{
    VMD::MotionData motionData;
    std::ifstream ifs(filepath, std::ios::binary); // バイナリモードでファイルを開く

    if (!ifs.is_open()) {
        throw std::runtime_error("Failed to open VMD file: " + filepath);
    }

    try {
        // ヘッダーを読み込み.
        ifs.read(motionData.header.VMDHeader, 30);
        if (ifs.gcount() != 30) { // gcount() で実際に読み込まれたバイト数を確認.
            throw std::runtime_error("Failed to read VMD header VMDHeader.");
        }
        motionData.header.VMDHeader[29] = '\0'; // 強制null終端.

        ifs.read(motionData.header.ModelName, 20);
        if (ifs.gcount() != 20) {
            throw std::runtime_error("Failed to read VMD header ModelName.");
        }
        motionData.header.ModelName[19] = '\0'; // 強制null終端.

        // ボーンフレームの数を読み込み.
        uint32_t numBoneFrames;
        ifs.read(reinterpret_cast<char*>(&numBoneFrames), sizeof(uint32_t));
        if (ifs.gcount() != sizeof(uint32_t)) {
            throw std::runtime_error("Failed to read number of bone frames.");
        }

        // 各ボーンフレームを読み込み.
        for (uint32_t i = 0; i < numBoneFrames; ++i) {
            VMD::BoneFrame boneFrame;

            ReadString(ifs, boneFrame.BoneName, 15); // ボーン名.

            // reinterpret_cast<char*> を使用してバイト列として読み込む.
            ifs.read(reinterpret_cast<char*>(&boneFrame.FrameNo), sizeof(uint32_t));
            if (ifs.gcount() != sizeof(uint32_t)) {
                throw std::runtime_error("Failed to read bone frame number.");
            }
            ifs.read(reinterpret_cast<char*>(&boneFrame.Position), sizeof(DirectX::XMFLOAT3));
            if (ifs.gcount() != sizeof(DirectX::XMFLOAT3)) {
                throw std::runtime_error("Failed to read bone position.");
            }
            ifs.read(reinterpret_cast<char*>(&boneFrame.Rotation), sizeof(DirectX::XMFLOAT4));
            if (ifs.gcount() != sizeof(DirectX::XMFLOAT4)) {
                throw std::runtime_error("Failed to read bone rotation.");
            }
            ifs.read(reinterpret_cast<char*>(&boneFrame.Interpolation), sizeof(uint8_t) * 64);
            if (ifs.gcount() != sizeof(uint8_t) * 64) {
                throw std::runtime_error("Failed to read bone interpolation data.");
            }

            // ボーン名を使ってマップに格納.
            motionData.BoneKeyFrames[std::string(boneFrame.BoneName)].push_back(boneFrame);
        }

        // モーフフレームの数を読み込み.
        uint32_t numMorphFrames;
        ifs.read(reinterpret_cast<char*>(&numMorphFrames), sizeof(uint32_t));
        if (ifs.gcount() != sizeof(uint32_t)) {
            throw std::runtime_error("Failed to read number of morph frames.");
        }

        // 各モーフフレームを読み込み.
        for (uint32_t i = 0; i < numMorphFrames; ++i) {
            VMD::MorphFrame morphFrame;
            ReadString(ifs, morphFrame.MorphName, 15);

            ifs.read(reinterpret_cast<char*>(&morphFrame.FrameNo), sizeof(uint32_t));
            if (ifs.gcount() != sizeof(uint32_t)) {
                throw std::runtime_error("Failed to read morph frame number.");
            }
            ifs.read(reinterpret_cast<char*>(&morphFrame.Value), sizeof(float));
            if (ifs.gcount() != sizeof(float)) {
                throw std::runtime_error("Failed to read morph value.");
            }
            motionData.MorphKeyFrames[std::string(morphFrame.MorphName)].push_back(morphFrame);
        }

        // MEMO : 以降、IKフレーム、カメラフレーム、照明フレームは必要ないので読み込まない.

        ifs.close();
        return motionData;

    }
    catch (const std::runtime_error& e) { 
        throw std::runtime_error( e.what() + std::string("Failed to VMDData."));
    }
}

void VMDLoader::ReadString(std::ifstream& ifs, char* buffer, size_t bufferSize)
{
    // ifs.read() は読み込みが成功したバイト数を返さないため、.
    // gcount() を使って実際に読み込まれたバイト数を確認する.
    ifs.read(buffer, bufferSize);
    if (ifs.gcount() != bufferSize) {
        throw std::runtime_error("Failed to read string data from VMD file.");
    }
    // 終端nullがない場合に備えて、強制的にnull終端.
    buffer[bufferSize - 1] = '\0';
}
