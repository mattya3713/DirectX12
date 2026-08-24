// FontRegistry単体テスト用のDefaultLoaderスタブ(FontLoader非依存でリンクするため).
#include "20_Resource/Font/FontRegistry.h"

int FontRegistry::DefaultLoader(const std::string&, const std::wstring&, int PixelHeight)
{
	return PixelHeight; // 呼ばれたことが分かるダミー値.
}
