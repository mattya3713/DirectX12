#pragma once

#include<d3d12.h>
#include<DirectXMath.h>
#include<filesystem>
#include<string>

// 前方宣言.
class DirectX12;
class MstcRenderer;

// Transform定数バッファ(b1. HLSL側のTransformとレイアウトを合わせる).
struct MstcTransformConstantBuffer
{
	DirectX::XMMATRIX World; // ワールド変換行列.
};

/**********************************************************************************
* @author    : Coder(閃斬 Production Loop).
* @date      : 2026/08/23.
* @brief     : .mstc(静的メッシュ)を描画するアクター. 変形しない剛体オブジェクト
*            : (地面・壁・障害物等)専用. スキニング・アニメーションは持たず、
*            : ワールド行列の設定と描画のみを行う.
*            : 法線マップはオブジェクト空間絶対法線(DESIGN.mdの.mstc設計準拠).
**********************************************************************************/

class MstcActor
{
public:
	MstcActor(const char* FilePath, MstcRenderer& Renderer);
	MstcActor(const std::filesystem::path& FilePath, MstcRenderer& Renderer);
	~MstcActor();

	MstcActor(const MstcActor&)            = delete;
	MstcActor& operator=(const MstcActor&) = delete;

	void Draw();

	// ワールド行列を設定する(Transformの反映用. 即GPUから参照される).
	void SetWorldMatrix(const DirectX::XMMATRIX& World) noexcept { if (m_pMappedTransformCB) { m_pMappedTransformCB->World = World; } }

private:
	// .mstcと参照先.mmatの読み込みとGPUリソースの生成.
	void CreateResources();

	// ファイルパスが空でないテクスチャを読み込む(失敗時は白テクスチャへフォールバック).
	MyComPtr<ID3D12Resource> LoadTexture(const std::string& Path);

private:
	DirectX12&   m_Dx12;
	MstcRenderer& m_Renderer;

	std::filesystem::path m_FilePath;

	MyComPtr<ID3D12Resource> m_pVertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW m_VertexBufferView{};
	MyComPtr<ID3D12Resource> m_pIndexBuffer;
	D3D12_INDEX_BUFFER_VIEW  m_IndexBufferView{};

	MyComPtr<ID3D12Resource>      m_pTransformConstantBuffer;
	MstcTransformConstantBuffer*  m_pMappedTransformCB = nullptr; // 永続マップ(SetWorldMatrixから即座に書き込まれる).
	MyComPtr<ID3D12Resource>      m_pMaterialConstantBuffer;
	D3D12_CONSTANT_BUFFER_VIEW_DESC m_MaterialCBVDesc{};

	MyComPtr<ID3D12DescriptorHeap> m_pCbvSrvUavHeap;
	UINT                           m_CbvSrvUavDescriptorSize = 0;

	MyComPtr<ID3D12Resource> m_pBaseTexture;       // ベースカラー.
	MyComPtr<ID3D12Resource> m_pNormalMapTexture;  // オブジェクト空間法線マップ(無ければ白).

	std::uint32_t m_IndexCount = 0; // 描画インデックス数.
};
