#pragma once
#include "FilePath.h"

namespace MyFilePath {

    inline std::pair<std::string, std::string> SplitFileName(const std::string& Path, const char Splitter) 
    {
        // セパレーター文字の位置を探査.
        auto Index = Path.find(Splitter);
        std::pair<std::string, std::string> SpliteFilePath;

        // セパレータが見つからない場合は、元のパスと空の文字列を返す.
        if (Index == std::string::npos) {

            // セパレータ以前は元のパス.
            SpliteFilePath.first = Path; 
            // セパレータ以降は空.
            SpliteFilePath.second = "";   

            return SpliteFilePath;
        }

        // セパレータ以前を取得.
        SpliteFilePath.first = Path.substr(0, Index);

        // セパレータ以降を取得.
        SpliteFilePath.second = Path.substr(Index + 1, Path.length() - Index - 1);

        // 分離した文字列を返す.
        return SpliteFilePath;
    }

    inline std::string GetExtension(const std::string& path) {
        // 後ろから「.」を探す
        auto Index = path.rfind('.');

        // 拡張子が見つからない場合.
        if (Index == std::string::npos) {
            // 空文字列を返す.
            return "";
        }

        // 拡張子を返す
        return path.substr(Index + 1);
    }

    inline std::wstring GetExtension(const std::wstring& path) {
        // 後ろから「.」を探す
        auto Index = path.rfind(L'.');

        // 拡張子が見つからない場合.
        if (Index == std::string::npos) {
            // 空文字列を返す.
            return L"";
        }

        // 拡張子を返す
        return path.substr(Index + 1);
    }

    // モデルからの相対テクスチャパスをアプリから見たパスをに変換,取得する.
    inline std::string GetTexPath(const std::string& ModelPath, const char* TexPath) {
       
        // モデルパスの最後のスラッシュまたはバックスラッシュを探す.
        int pathIndex1 = static_cast<int>(ModelPath.rfind('/'));
        int pathIndex2 = static_cast<int>(ModelPath.rfind('\\'));
        int pathIndex = std::max(pathIndex1, pathIndex2);

        // モデルパスの最後がスラッシュでない場合.
        if (pathIndex == std::string::npos) {

            // テクスチャパスを追加.
            return ModelPath + TexPath;
        }

        // モデルパスからフォルダパスを取得し、テクスチャパスを追加.
        auto folderPath = ModelPath.substr(0, pathIndex + 1);
        return folderPath + TexPath;
    }

    // ファイルパスなどの/を\\に変換.
    void ReplaceSlashWithBackslash(std::string* Path)
    {
        // nullptrチェック.
        if (Path == nullptr) { return; }

        for (char& c : *Path) {
            if (c == '/') {
                c = '\\';
            }
        }
    }
}
