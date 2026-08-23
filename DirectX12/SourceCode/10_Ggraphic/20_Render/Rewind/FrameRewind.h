#pragma once

#include <d3d12.h>
#include <vector>

#include "..\\..\\..\\Data\\Library\\DirectXTex\\Common\\d3dx12.h"

// 前方宣言.
class DirectX12;

/**********************************************************************************
* @brief     : 敗北時巻き戻り演出(フレームリングバッファ逆再生).
*            : 毎フレームのバックバッファ内容を縮小リングへ保存し、敗北確定時に
*            : 最新→最古の順でフルスクリーン逆再生する。DirectX12がhas-aで保持し、
*            : 描画はDirectX12が管理する現在のレンダーターゲット(バックバッファ)へ行う。
**********************************************************************************/

class FrameRewind final
{
public:
	explicit FrameRewind(DirectX12& Dx12);
	~FrameRewind() = default;

	FrameRewind(const FrameRewind&)            = delete;
	FrameRewind& operator=(const FrameRewind&) = delete;

	// リング・PSO・ヒープ類を生成する(構築直後に1度だけ呼ぶ).
	void Create();

	// バックバッファSRVを作り直す(スワップチェーン再生成後に呼ぶ).
	void RefreshBackBufferSRVs();

	// 毎フレーム、現在のバックバッファ内容を縮小リングへ1枚保存する(逆再生中は無視).
	void Capture();

	// 巻き戻り逆再生を開始する(保存フレームが無い場合はfalseで何もしない).
	bool StartPlayback();

	// 逆再生を1フレーム分描画し、再生位置を古い方向へ進める(完了でIsPlaying()==false).
	void DrawPlaybackFrame();

	// 巻き戻り逆再生中か(呼び出し側は通常のシーン描画を止める).
	bool IsActive() const noexcept { return m_State == RewindState::Playing && !m_Finished; }

private:
	// 巻き戻り演出の内部状態.
	enum class RewindState
	{
		Capture, // 毎フレーム保存中(通常プレイ).
		Playing  // 逆再生中.
	};

	// フルスクリーン.blitパイプラインとルートシグネチャを生成する(Create()内部用).
	void CreatePipeline();
	// テクスチャ1枚分のリング要素を作る.
	void CreateRingTexture(UINT Index);

private:
	DirectX12& m_Dx12;

	// リングの諸元(縮小解像度でVRAM圧迫を避ける. 総容量は約180MB).
	static constexpr UINT REWIND_FRAME_COUNT    = 90;  // 保持フレーム数(60FPSで約1.5秒分).
	static constexpr UINT REWIND_WIDTH          = 960;
	static constexpr UINT REWIND_HEIGHT         = 540;
	static constexpr UINT REWIND_PLAYBACK_SPEED = 2;   // 1描画フレームで進める保存フレーム数(2倍速).

	std::vector<MyComPtr<ID3D12Resource>>	m_Ring;				// リングバッファ(縮小コピー先テクスチャ配列).
	MyComPtr<ID3D12DescriptorHeap>			m_pRtvHeap;			// リング用RTVヒープ(REWIND_FRAME_COUNT個).
	MyComPtr<ID3D12DescriptorHeap>			m_pSrvHeap;			// SRVヒープ(先頭2個=バックバッファ, 続くN個=リング).
	UINT	m_SrvDescriptorSize = 0;
	MyComPtr<ID3D12PipelineState>			m_pPipelineState;	// フルスクリーン.blitパイプライン.
	MyComPtr<ID3D12RootSignature>			m_pRootSignature;

	UINT	m_WriteIndex   = 0;	// 次に書き込むリング位置.
	UINT	m_ValidCount   = 0;	// 実際に保存済みの枚数(起動直後は未充足).
	UINT	m_PlayIndex    = 0;	// 逆再生中の再生位置(リングインデックス).
	UINT	m_FramesShown  = 0;	// 逆再生で表示済みの保存フレーム数.
	RewindState m_State    = RewindState::Capture;
	bool	m_Finished     = false; // 最古フレームまで再生し終えたか.
};
