// スタンドアロン単体テスト用の強制インクルード(/FIで全TUの先頭に読ませる).
// ゲーム本体ではプロジェクト設定(PCH)経由でWindows.hが先行するため、
// ColliderBase.hのDEFINE_ENUM_FLAG_OPERATORSが解決できるようここで先に読む.
#include <Windows.h>
