#pragma once

#include<d3d12.h>
#include<DirectXMath.h>
#include<vector>
#include<string>
#include"PMXStructHeader.h"		// PMX用構造体まとめ.
#include"VMD/VMDStructHeader.h"	// VMD用構造体まとめ.
#include"Model/ModelData.h"		// フォーマットを問わないモデルデータ.

// 前方宣言.
class DirectX12;
class PMXRenderer;
class VMDLoader;

// ランタイム時に使用するボーンデータ.
struct RuntimeBone
{
	const Model::Bone* ModelBoneData;					// 共通ボーンデータへのポインタ.
	DirectX::XMMATRIX OffsetMatrix;					// Model::Bone::Positionから作成されるローカル移動行列.
	DirectX::XMMATRIX CurrentAnimationLocalTransform;	// VMDキーフレームから計算されるアニメーションによるローカル変換.
	DirectX::XMMATRIX InitialGlobalMatrix;				// 初期バインドポーズにおけるボーンのグローバル変換行列.
	DirectX::XMMATRIX InverseInitialGlobalMatrix;		// InitialGlobalMatrix の逆行列.
	DirectX::XMMATRIX FinalWorldMatrix;				// アニメーション適用後の最終的なボーンのグローバル変換行列.

	RuntimeBone() : ModelBoneData(nullptr),
		OffsetMatrix(DirectX::XMMatrixIdentity()),
		CurrentAnimationLocalTransform(DirectX::XMMatrixIdentity()),
		InitialGlobalMatrix(DirectX::XMMatrixIdentity()),
		InverseInitialGlobalMatrix(DirectX::XMMatrixIdentity()),
		FinalWorldMatrix(DirectX::XMMatrixIdentity())
	{
	}
};

/**************************************************
*	PMXモデルクラス.
*	担当：淵脇 未来
**/

class PMXActor
{
	friend PMXRenderer;

public:
	PMXActor(const char* filepath, PMXRenderer& renderer);
	~PMXActor();

	///クローンは頂点およびマテリアルは共通のバッファを見るようにする
	PMXActor* Clone(); // 後ほど実装

	void Update();
	void Draw();

	/*******************************************
	* @brief	VMDのロード.
	* @param	ファイルパス.
	*******************************************/
	void LoadVMDFile(const std::string& filepath);

	// アニメーション開始.
	void PlayAnimation();
	void StopAnimation();

private:
	// ルートパラメータのインデックス (シェーダーと合わせる)
	enum RootParamIndex
	{
		RP_SCENE_CBV = 0,
		RP_TRANSFORM_CBV,
		RP_MATERIAL_TABLE_CBV_SRV, // マテリアルごとのテーブルの開始点 (ここから複数ディスクリプタ)
		RP_BONE_SRV,
		// ... その他のルートパラメータがあれば追加 ...
		RP_COUNT
	};

private:
	// アニメーション関連.
	// PMXボーンの初期ローカル変換、親子関係設定、およびPMXボーン名とインデックスのマッピングを構築
	void InitializeRuntimeBones(); // BuildPmxBoneMap の役割も含む
	void CalculateInitialGlobalMatricesRecursive(int boneIndex, const DirectX::XMMATRIX& parentInitialGlobalMatrix);
	void MapVmdBonesToPmxBones();					// VMDボーン名とPMXボーンインデックスをマッピング
	void UpdateAnimation();			// アニメーション更新のメインロジック (引数にdeltaTimeを追加)

	/*******************************************
	* @brief	VMDキーフレームの補間を適用し、ボーンのローカル変換を計算.
	* @param	currentFrame: 現在のキーフレーム.
	* @param	nextFrame: 次のキーフレーム.
	* @param	t: 補間係数 (0.0～1.0).
	* @param	pmxBoneIndex: 対象のPMXボーンインデックス.
	*******************************************/
	void ApplyVMDKeyFrame(const VMD::BoneFrame& currentFrame, const VMD::BoneFrame& nextFrame, float t, int pmxBoneIndex);

	// 再帰的にボーンのグローバル行列を更新
	void UpdateBoneGlobalTransforms(int boneIndex, const DirectX::XMMATRIX& parentGlobalTransform);

	// GPUリソースの作成と初期データ転送を担当
	void CreateResources();

	// LoadTexture ヘルパー関数.
	MyComPtr<ID3D12Resource> LoadTexture(const std::string& path);

	// ベジェ曲線補間
	float GetYFromXOnBezier(float x, const DirectX::XMFLOAT2& a, const DirectX::XMFLOAT2& b, uint8_t n = 12);

	// メンバー変数
private:
	PMXRenderer& m_pRenderer;
	DirectX12& m_pDx12;

	MyComPtr<ID3D12Resource> m_pVertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW m_pVertexBufferView;
	Model::Vertex* m_pMappedVertex; // マップされた頂点バッファへのポインタ (Unmap後はnullptrにする)

	MyComPtr<ID3D12Resource> m_pIndexBuffer;
	D3D12_INDEX_BUFFER_VIEW m_pIndexBufferView;
	uint32_t* m_MappedIndex; // マップされたインデックスバッファへのポインタ (Unmap後はnullptrにする)

	MyComPtr<ID3D12Resource> m_pTransformConstantBuffer;
	PMX::TransformConstantBuffer* m_pMappedTransformCB; // マップされたTransform CBへのポインタ

	MyComPtr<ID3D12Resource> m_pBoneTransformStructuredBuffer;
	DirectX::XMMATRIX* m_pMappedBoneTransforms; // マップされたボーン行列バッファへのポインタ

	MyComPtr<ID3D12DescriptorHeap> m_pCbvSrvUavHeap;
	UINT m_CbvSrvUavDescriptorSize; // ディスクリプタサイズ

	std::vector<D3D12_GPU_DESCRIPTOR_HANDLE> m_gpuDescriptorHandles; // ルートパラメータごとのGPUハンドル
	std::vector<D3D12_GPU_DESCRIPTOR_HANDLE> m_materialRootTableGpuHandles; // マテリアルごとのルートテーブル開始GPUハンドル (各マテリアルが持つビューの開始位置)

	// モデルデータ (CPU側、フォーマットを問わない共通データ).
	Model::ModelData m_ModelData;

	// テクスチャリソース (GPU側).
	std::vector<MyComPtr<ID3D12Resource>> m_pTextureResource; // ベーステクスチャ (MaterialIndexに対応)
	std::vector<MyComPtr<ID3D12Resource>> m_pSphResource;     // スフィアマップ (MaterialIndexに対応)
	std::vector<MyComPtr<ID3D12Resource>> m_pToonResource;    // トゥーンマップ (MaterialIndexに対応)

	std::vector<RuntimeBone> m_RuntimeBones; // アニメーション適用・階層計算用のボーンデータ (CPU側、m_ModelData.Bonesを参照)

	// VMDデータとPMXボーンのマッピング
	VMD::MotionData m_MotionData; // ロードされたVMDデータ
	std::unordered_map<std::string, int> m_PMXBoneNameToIndexMap; // PMXボーン名 -> インデックス
	std::unordered_map<std::string, int> m_VMDBoneNameToPmxBoneIndexMap; // VMDボーン名 -> PMXボーンインデックス

	// アニメーション再生状態
	bool m_IsPlayingAnimation;
	float m_CurrentAnimationTime; // 現在のアニメーション時刻 (フレーム数または秒数)
	float m_AnimationSpeed;       // アニメーション再生速度 (例: 30.0f で30FPS)
	uint32_t m_MaxFrame;          // VMDアニメーションの最大フレーム数
	std::chrono::time_point<std::chrono::high_resolution_clock> m_AnimationStartTime; // アニメーション開始時刻
};