#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <xaudio2.h>

#include "99_Utility/ComPtr/ComPtr.h"

/**********************************************************************************
* @author    : mattya3713.
* @date      : 2026/08/11.
* @brief     : SE(効果音)再生マネージャー. XAudio2を使用する. ServiceLocator経由で利用する.
*            : Data\Sound以下のWAV(PCM)ファイルを起動時に全て読み込み、ファイル名(拡張子無し)
*            : をキーとして再生する. 1つの名前を同時に複数回再生することもできる
*            : (再生の都度SourceVoiceを新規作成するため).
* @pattern   : ServiceLocator.
**********************************************************************************/

class SoundManager final
{
public:
	SoundManager();
	~SoundManager();

	SoundManager(const SoundManager&)            = delete;
	SoundManager& operator=(const SoundManager&) = delete;
	SoundManager(SoundManager&&)                 = delete;
	SoundManager& operator=(SoundManager&&)      = delete;

	// 指定ディレクトリ以下のWAVファイルを再帰的に全て読み込む.
	bool LoadSounds(const std::string& DirectoryPath = "Data\\Sound");

	// 名前を指定して再生する(IsLoop=trueでループ再生. Volumeは0.0〜1.0).
	void Play(const std::string& Name, bool IsLoop = false, float Volume = 1.0f);

	// ピッチ・音量を指定して再生する(Sound Event等の拡張用. Pitchは0.5〜2.0程度を推奨).
	void PlayEx(const std::string& Name, float Volume = 1.0f, float Pitch = 1.0f, bool IsLoop = false);

	// 名前を指定して再生中の音だけ停止する(Sound Eventの個別停止用).
	void Stop(const std::string& Name);

	// 指定した名前で現在实际に再生中のボイス数を取得する(同時再生数上限判定用).
	int GetActiveVoiceCount(const std::string& Name) const;

	// 再生中の音を全て停止する.
	void StopAll();

	// 毎フレーム呼び出し、再生終了したボイスを片付ける.
	void Update();

private:
	// 読み込み済みのサウンドデータ(PCM生データ + フォーマット).
	struct SoundClip
	{
		std::vector<BYTE> AudioData;
		WAVEFORMATEX      Format{};
	};

	// WAV(PCM)ファイルを読み込む. fmt/dataチャンクのみ対応(WAVE_FORMAT_PCM前提).
	bool LoadWavFile(const std::wstring& FilePath, SoundClip& OutClip) const;

private:
	MyComPtr<IXAudio2>      m_cpXAudio2;
	IXAudio2MasteringVoice* m_pMasteringVoice = nullptr; // DestroyVoice()で解放する(Release()ではない).

	std::unordered_map<std::string, SoundClip> m_Clips; // 読み込み済みサウンド(名前がキー).

	// 再生中のボイス(Update()で終了したものを破棄する. Nameはイベント単位の停止/カウント用).
	struct ActiveVoice
	{
		IXAudio2SourceVoice* pVoice = nullptr;
		std::string          Name;
	};

	std::vector<ActiveVoice> m_ActiveVoices;
};
