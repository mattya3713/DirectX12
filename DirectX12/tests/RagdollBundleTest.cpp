// RagdollDefinition/RagdollComponentの単体テスト(スタンドアロン. ゲーム本体には含まれない).
// ビルド例: cl /nologo /EHsc /std:c++20 /W4 /I..\Data\Library /I..\SourceCode RagdollBundleTest.cpp ^
//   ..\SourceCode\99_Utility\Ragdoll\RagdollDefinition.cpp ..\SourceCode\99_Utility\FileManager\FileManager.cpp ^
//   ..\SourceCode\99_Utility\String\String.cpp /Fe:RagdollBundleTest.exe
//
// 確認内容:
// 1. DefinitionのJSON往復で値が保持される
// 2. Activate→Updateでボディが落下する(重力積分)
// 3. 二重Activateは拒否される
// 4. Deactivate/Resetを繰り返しても状態が壊れない
// 5. Definition未設定のActivateは拒否される

#include <cassert>
#include <cmath>
#include <filesystem>
#include <iostream>

#include "../SourceCode/99_Utility/Ragdoll/RagdollDefinition.h"
#include "../SourceCode/99_Utility/Ragdoll/RagdollComponent.h"

namespace {

	const std::filesystem::path kTestFile = "ragdoll_test_output.json";

	RagdollDefinition MakeSample()
	{
		RagdollDefinition def = RagdollDefinition::CreateBossDefault();
		def.Bones[0].ColliderHalf = { 0.2f, 0.15f, 0.1f };
		return def;
	}

} // namespace

int main()
{
	std::error_code ec;
	std::filesystem::remove(kTestFile, ec);

	// ---- 1. JSON往復 ----
	{
		RagdollDefinition original = MakeSample();
		assert(original.SaveJson(kTestFile));

		RagdollDefinition loaded;
		assert(loaded.LoadJson(kTestFile));
		assert(loaded.Name == "boss_default");
		assert(loaded.Bones.size() == original.Bones.size());
		assert(loaded.Bones[0].BoneName == "hips");
		assert(std::abs(loaded.Bones[0].Mass - 8.0f) < 0.001f);
		assert(loaded.Bones[2].BoneName == "head");
		std::cout << "[PASS] 1. json round trip" << std::endl;
	}

	// ---- 2〜4. ライフサイクル ----
	{
		RagdollDefinition def = MakeSample();
		RagdollComponent ragdoll;
		ragdoll.SetDefinition(&def);

		std::vector<DirectX::XMFLOAT3> pose(def.Bones.size(), { 0.0f, 2.0f, 0.0f });

		// 未Active時にDeactivateはfalse.
		assert(!ragdoll.Deactivate());

		// Activate成功と重力落下.
		assert(ragdoll.Activate(pose));
		assert(ragdoll.IsActive());
		assert(ragdoll.GetBodyStates().size() == def.Bones.size());
		ragdoll.Update(0.5f);
		assert(ragdoll.GetBodyStates()[0].Position.y < 2.0f); // 落下した.

		// 二重Activate拒否.
		assert(!ragdoll.Activate(pose));

		// Deactivate→再Activate(壊れない).
		assert(ragdoll.Deactivate());
		assert(!ragdoll.IsActive());
		assert(ragdoll.GetBodyStates().empty());
		assert(ragdoll.Activate(pose));
		assert(ragdoll.IsActive());

		// Reset(冪等).
		ragdoll.Reset();
		ragdoll.Reset();
		assert(!ragdoll.IsActive());
		assert(ragdoll.Activate(pose));
		std::cout << "[PASS] 2-4. lifecycle" << std::endl;
	}

	// ---- 5. Definition未設定のActivate拒否 ----
	{
		RagdollComponent empty;
		assert(!empty.Activate({}));
		std::cout << "[PASS] 5. no definition rejected" << std::endl;
	}

	std::filesystem::remove(kTestFile, ec);
	std::cout << "All tests passed." << std::endl;
	return 0;
}
