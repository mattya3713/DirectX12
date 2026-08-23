#pragma once

/**********************************************************************************
* @author    : Coder 青龍(せいりゅう).
* @date      : 2026/08/23.
* @brief     : Async Compute動作確認デモ(コンピュートキューで決定的計算を実行し、
*            : フェンス同期後にCPUで読み戻して検証する).
*            : 定期的(フレームカウンタ)に実行し、検証結果をDebugLogへ出力する.
*            : スレッド境界: 記録はコンピュートキュー、読み戻し検証はメインスレッド
*            : (フェンスイベント待ちの後). グラフィックスキューにも完了待ちを挿入し
*            : キュー間同期(コンピュート→グラフィックス)を検証する.
**********************************************************************************/

struct ID3D12Device;
class DirectX12;

class AsyncComputeDemo final
{
public:
	AsyncComputeDemo() = default;
	~AsyncComputeDemo();

	AsyncComputeDemo(const AsyncComputeDemo&)            = delete;
	AsyncComputeDemo& operator=(const AsyncComputeDemo&) = delete;

	// コンピュート用PSO/バッファを作成する(DirectX12::Updateから初回に呼ぶ).
	bool Initialize(struct ID3D12Device* pDevice, class DirectX12& Dx12);

	// 毎フレーム呼ぶ(指定間隔ごとにコンピュートを実行し、完了後に検証ログを出す).
	void Tick(class DirectX12& Dx12);

private:
	class Impl;
	std::shared_ptr<Impl> m_Impl; // shared_ptrはデリータを型消去するため不完全なImplでも安全.
};
