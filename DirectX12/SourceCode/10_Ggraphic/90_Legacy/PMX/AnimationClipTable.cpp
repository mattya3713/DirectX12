#include "AnimationClipTable.h"

#include <filesystem>
#include <fstream>

void AnimationClipTable::Set(const std::string& ClipName, const AnimationClipData& Data)
{
	m_Clips[ClipName] = Data;
}

const AnimationClipData* AnimationClipTable::Find(const std::string& ClipName) const
{
	const auto it = m_Clips.find(ClipName);
	return (it != m_Clips.end()) ? &it->second : nullptr;
}

bool AnimationClipTable::Load(const std::string& FilePath)
{
	std::ifstream file(FilePath);
	if (!file.is_open()) { return false; }

	m_Clips.clear();

	std::string clip_name;
	AnimationClipData data;
	while (file >> clip_name >> data.StartFrame >> data.EndFrame >> data.Speed)
	{
		m_Clips[clip_name] = data;
	}

	return true;
}

bool AnimationClipTable::Save(const std::string& FilePath) const
{
	const std::filesystem::path path(FilePath);
	if (path.has_parent_path())
	{
		std::filesystem::create_directories(path.parent_path());
	}

	std::ofstream file(FilePath);
	if (!file.is_open()) { return false; }

	for (const auto& [clip_name, data] : m_Clips)
	{
		file << clip_name << ' ' << data.StartFrame << ' ' << data.EndFrame << ' ' << data.Speed << '\n';
	}

	return true;
}
