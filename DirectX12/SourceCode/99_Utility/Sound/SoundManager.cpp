#include "SoundManager.h"

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <fstream>

SoundManager::SoundManager()
{
	if (FAILED(XAudio2Create(m_cpXAudio2.ReleaseAndGetAddressOf(), 0, XAUDIO2_DEFAULT_PROCESSOR)))
	{
		return;
	}

	m_cpXAudio2->CreateMasteringVoice(&m_pMasteringVoice);
}

SoundManager::~SoundManager()
{
	StopAll();

	if (m_pMasteringVoice)
	{
		m_pMasteringVoice->DestroyVoice();
		m_pMasteringVoice = nullptr;
	}
}

bool SoundManager::LoadSounds(const std::string& DirectoryPath)
{
	namespace fs = std::filesystem;

	if (!m_cpXAudio2 || !fs::exists(DirectoryPath)) { return false; }

	for (const auto& entry : fs::recursive_directory_iterator(DirectoryPath))
	{
		if (!entry.is_regular_file()) { continue; }

		std::string extension = entry.path().extension().string();
		std::transform(extension.begin(), extension.end(), extension.begin(),
			[](unsigned char C) { return static_cast<char>(std::tolower(C)); });

		if (extension != ".wav") { continue; }

		SoundClip clip;
		if (LoadWavFile(entry.path().wstring(), clip))
		{
			m_Clips[entry.path().stem().string()] = std::move(clip);
		}
	}

	return true;
}

bool SoundManager::LoadWavFile(const std::wstring& FilePath, SoundClip& OutClip) const
{
	std::ifstream file(FilePath, std::ios::binary);
	if (!file.is_open()) { return false; }

	char riff_id[4] = {};
	uint32_t riff_size = 0;
	char wave_id[4] = {};
	file.read(riff_id, 4);
	file.read(reinterpret_cast<char*>(&riff_size), sizeof(riff_size));
	file.read(wave_id, 4);

	if (!file || std::memcmp(riff_id, "RIFF", 4) != 0 || std::memcmp(wave_id, "WAVE", 4) != 0)
	{
		return false;
	}

	bool has_format = false;
	bool has_data   = false;

	while (file && !(has_format && has_data))
	{
		char chunk_id[4] = {};
		uint32_t chunk_size = 0;
		file.read(chunk_id, 4);
		file.read(reinterpret_cast<char*>(&chunk_size), sizeof(chunk_size));
		if (!file) { break; }

		if (std::memcmp(chunk_id, "fmt ", 4) == 0)
		{
			// WAVEFORMATEXは事前に{}でゼロ初期化済みなので、16バイトのfmtチャンク(cbSize無し)でも
			// 未読み込み部分(cbSize)は0のままになり問題ない.
			const uint32_t read_size = std::min<uint32_t>(chunk_size, static_cast<uint32_t>(sizeof(WAVEFORMATEX)));
			file.read(reinterpret_cast<char*>(&OutClip.Format), read_size);
			if (chunk_size > read_size) { file.seekg(chunk_size - read_size, std::ios::cur); }
			has_format = true;
		}
		else if (std::memcmp(chunk_id, "data", 4) == 0)
		{
			OutClip.AudioData.resize(chunk_size);
			file.read(reinterpret_cast<char*>(OutClip.AudioData.data()), chunk_size);
			has_data = true;
		}
		else
		{
			file.seekg(chunk_size, std::ios::cur);
		}

		// チャンクは2バイト境界にパディングされる.
		if (chunk_size % 2 != 0) { file.seekg(1, std::ios::cur); }
	}

	return has_format && has_data && OutClip.Format.wFormatTag == WAVE_FORMAT_PCM;
}

void SoundManager::Play(const std::string& Name, bool IsLoop, float Volume)
{
	if (!m_cpXAudio2) { return; }

	const auto it = m_Clips.find(Name);
	if (it == m_Clips.end()) { return; }

	const SoundClip& clip = it->second;

	IXAudio2SourceVoice* p_voice = nullptr;
	if (FAILED(m_cpXAudio2->CreateSourceVoice(&p_voice, &clip.Format))) { return; }

	XAUDIO2_BUFFER buffer{};
	buffer.AudioBytes = static_cast<UINT32>(clip.AudioData.size());
	buffer.pAudioData = clip.AudioData.data();
	buffer.Flags      = XAUDIO2_END_OF_STREAM;
	buffer.LoopCount  = IsLoop ? XAUDIO2_LOOP_INFINITE : 0;

	p_voice->SetVolume(Volume);

	if (FAILED(p_voice->SubmitSourceBuffer(&buffer)) || FAILED(p_voice->Start()))
	{
		p_voice->DestroyVoice();
		return;
	}

	m_ActiveVoices.push_back({ p_voice, Name, IsLoop });
}

void SoundManager::StopAll()
{
	for (const ActiveVoice& active_voice : m_ActiveVoices)
	{
		active_voice.Voice->Stop();
		active_voice.Voice->DestroyVoice();
	}
	m_ActiveVoices.clear();
}

void SoundManager::StopLooping(const std::string& Name)
{
	for (auto it = m_ActiveVoices.begin(); it != m_ActiveVoices.end(); )
	{
		if (it->IsLoop && it->Name == Name)
		{
			it->Voice->Stop();
			it->Voice->DestroyVoice();
			it = m_ActiveVoices.erase(it);
		}
		else
		{
			++it;
		}
	}
}

void SoundManager::Update()
{
	for (auto it = m_ActiveVoices.begin(); it != m_ActiveVoices.end(); )
	{
		XAUDIO2_VOICE_STATE state{};
		it->Voice->GetState(&state);

		if (state.BuffersQueued == 0)
		{
			it->Voice->DestroyVoice();
			it = m_ActiveVoices.erase(it);
		}
		else
		{
			++it;
		}
	}
}
