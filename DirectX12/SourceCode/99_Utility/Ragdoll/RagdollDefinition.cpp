#include "RagdollDefinition.h"

#include "99_Utility/FileManager/FileManager.h"

// JSONへ保存する.
bool RagdollDefinition::SaveJson(const std::filesystem::path& FilePath) const
{
	nlohmann::json bones = nlohmann::json::array();
	for (const RagdollBoneDesc& bone : Bones)
	{
		bones.push_back({
			{"bone_name",    bone.BoneName},
			{"mass",         bone.Mass},
			{"collider_half", { bone.ColliderHalf.x, bone.ColliderHalf.y, bone.ColliderHalf.z }},
			{"local_offset",  { bone.LocalOffset.x, bone.LocalOffset.y, bone.LocalOffset.z }},
		});
	}

	nlohmann::json out;
	out["name"]  = Name;
	out["bones"] = bones;

	return FileManager::JsonSave(FilePath, out);
}

// JSONから読み込む.
bool RagdollDefinition::LoadJson(const std::filesystem::path& FilePath)
{
	const nlohmann::json data = FileManager::JsonLoad(FilePath);
	if (data.empty()) { return false; }

	Name = data.value("name", "");

	Bones.clear();
	if (!data.contains("bones") || !data["bones"].is_array()) { return false; }

	for (const nlohmann::json& entry : data["bones"])
	{
		RagdollBoneDesc bone{};
		bone.BoneName = entry.value("bone_name", "");
		bone.Mass     = entry.value("mass", 1.0f);

		if (entry.contains("collider_half") && entry["collider_half"].is_array() && entry["collider_half"].size() >= 3)
		{
			bone.ColliderHalf = { entry["collider_half"][0], entry["collider_half"][1], entry["collider_half"][2] };
		}
		if (entry.contains("local_offset") && entry["local_offset"].is_array() && entry["local_offset"].size() >= 3)
		{
			bone.LocalOffset = { entry["local_offset"][0], entry["local_offset"][1], entry["local_offset"][2] };
		}

		Bones.push_back(std::move(bone));
	}

	return !Bones.empty();
}

// Boss撃破用のサンプル定義(ボーン名は実機骨格確認後に調整).
RagdollDefinition RagdollDefinition::CreateBossDefault()
{
	RagdollDefinition def;
	def.Name = "boss_default";

	const auto add_bone = [&def](const char* pName, float Mass) {
		RagdollBoneDesc bone{};
		bone.BoneName = pName;
		bone.Mass     = Mass;
		def.Bones.push_back(bone);
	};

	add_bone("hips",  8.0f);
	add_bone("spine", 5.0f);
	add_bone("head",  3.0f);
	add_bone("arm_l", 1.5f);
	add_bone("arm_r", 1.5f);
	add_bone("leg_l", 2.5f);
	add_bone("leg_r", 2.5f);

	return def;
}
