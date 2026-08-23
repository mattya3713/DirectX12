// Standalone verification for ResourceCatalog (task_resource_catalog acceptance criteria).
// カテゴリ付きリソースIDの正規化/検索・表記揺れ収束・パス脱出拒否を検証する.
#include <cassert>
#include <cstdio>
#include <string>

#include "20_Resource/ResourceCatalog.h"

namespace {

	int s_PassCount = 0;
	int s_FailCount = 0;

	void Check(bool Condition, const char* Label)
	{
		if (Condition)
		{
			++s_PassCount;
			std::printf("[PASS] %s\n", Label);
		}
		else
		{
			++s_FailCount;
			std::printf("[FAIL] %s\n", Label);
		}
	}

}

int main()
{
	// 1. 正常系: キー生成.
	{
		std::string key;
		Check(ResourceCatalog::TryMakeKey(eResourceCategory::Mesh, "models/enemy.mskn", key), "normal key");
		Check(key == "mesh/models/enemy.mskn", "key format: <root>/<id>");
	}

	// 2. 表記揺れの収束(同一リソースは同一キー).
	{
		std::string a, b, c;
		const bool ra = ResourceCatalog::TryMakeKey(eResourceCategory::Mesh, "models/enemy.mskn", a);
		const bool rb = ResourceCatalog::TryMakeKey(eResourceCategory::Mesh, "./models//enemy.mskn", b);
		const bool rc = ResourceCatalog::TryMakeKey(eResourceCategory::Mesh, "models/tmp/../enemy.mskn", c);
		Check(ra && rb && rc, "variant inputs accepted");
		Check(a == b && b == c, "variants converge to same key");
	}

	// 3. バックスラッシュ表記の統一.
	{
		std::string slash_key, backslash_key;
		ResourceCatalog::TryMakeKey(eResourceCategory::Sound, "se/hit.wav", slash_key);
		ResourceCatalog::TryMakeKey(eResourceCategory::Sound, "se\\hit.wav", backslash_key);
		Check(slash_key == backslash_key, "backslash normalized to slash");
	}

	// 4. 拒否系.
	{
		std::string key = "untouched";

		Check(!ResourceCatalog::TryMakeKey(eResourceCategory::Mesh, "", key), "reject empty id");
		Check(key == "untouched", "out key untouched on reject");

		Check(!ResourceCatalog::TryMakeKey(eResourceCategory::Mesh, "..", key), "reject bare ..");
		Check(!ResourceCatalog::TryMakeKey(eResourceCategory::Mesh, "../outside.wav", key), "reject parent escape");
		Check(!ResourceCatalog::TryMakeKey(eResourceCategory::Mesh, "a/b/../../../c.wav", key), "reject deep escape");
		Check(!ResourceCatalog::TryMakeKey(eResourceCategory::Mesh, "/absolute/path.wav", key), "reject absolute path");
		Check(!ResourceCatalog::TryMakeKey(eResourceCategory::Mesh, "C:/Data/x.wav", key), "reject drive path");

		const eResourceCategory invalid = static_cast<eResourceCategory>(99);
		Check(!ResourceCatalog::TryMakeKey(invalid, "x", key), "reject invalid category");
	}

	// 5. カタログ登録・検索・解除.
	{
		ResourceCatalog catalog;
		std::string key;

		Check(ResourceCatalog::TryMakeKey(eResourceCategory::Effect, "fx/hit_burst.json", key), "catalog: make key");

		Check(catalog.Register(key), "catalog: register");
		Check(!catalog.Register(key), "catalog: duplicate register rejected");
		Check(catalog.Contains(key), "catalog: contains");
		Check(!catalog.Contains("mesh/not_registered.bin"), "catalog: unregistered not found");

		Check(catalog.Unregister(key), "catalog: unregister");
		Check(!catalog.Contains(key), "catalog: gone after unregister");
		Check(!catalog.Unregister(key), "catalog: double unregister rejected");
	}

	// 6. カテゴリごとに別キー(同名IDでも衝突しない).
	{
		std::string world_key, ui_key;
		ResourceCatalog::TryMakeKey(eResourceCategory::ImageWorldSprite, "gauge.png", world_key);
		ResourceCatalog::TryMakeKey(eResourceCategory::ImageUISprite, "gauge.png", ui_key);
		Check(world_key != ui_key, "same id in different categories -> different keys");
		Check(world_key.find("image/world/") == 0 && ui_key.find("image/ui/") == 0, "category root prefix");
	}

	std::printf("\nResult: PASS=%d FAIL=%d\n", s_PassCount, s_FailCount);
	return (s_FailCount == 0) ? 0 : 1;
}
