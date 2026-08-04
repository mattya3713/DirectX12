#pragma once

namespace VMD {
    // ヘッダー構造体
    struct Header
    {
        char VMDHeader[30];     // "Vocaloid Motion Data".
        char ModelName[20];     // モデル名 (通常は無視してもOK).
        uint8_t Version;        // VMD バージョン (未使用の場合が多い).
    };
    
    // ボーンキーフレームデータ.
    struct BoneFrame
    {
        char BoneName[15];          // ボーン名 (UTF-8, 終端null含む).
        uint32_t FrameNo;           // フレーム番号.
        DirectX::XMFLOAT3 Position; // 移動 (x, y, z).
        DirectX::XMFLOAT4 Rotation; // 回転 (x, y, z, w) Quaternion.
        uint8_t Interpolation[64];  // 補間曲線データ (各軸16バイト * 4軸 = 64バイト).
        // X, Y, Z, Rotation の順で補間データが入る.
    };

    // モーフキーフレームデータ.
    struct MorphFrame
    {
        char MorphName[15];     // モーフ名 (UTF-8, 終端null含む).
        uint32_t FrameNo;       // フレーム番号.
        float Value;            // モーフの値.
    };

    // VMD ファイル全体を保持するデータ構造.
    struct MotionData
    {
        Header header;
        std::map<std::string, std::vector<BoneFrame>> BoneKeyFrames;    // ボーン名 -> そのボーンの全キーフレーム.
        std::map<std::string, std::vector<MorphFrame>> MorphKeyFrames;  // モーフ名 -> そのモーフの全キーフレーム.
        // 必要に応じてIK, Camera, Light等のフレームも追加.
    };
}