#include "PMXActor.h"
#include "10_Ggraphic/30_Asset/RuntimeModel/MMdl/MmdlRenderer.h"
#include "VMD/VMDLoader.h"
#include "10_Device/DirectX/DirectX12.h"
#include "30_Asset/Parser/PMXParser.h"
#include "..\\..\\..\\Data\\Library\\DirectXTex\\Common\\d3dx12.h"
#include <chrono>
#include <algorithm>
#include <cfloat>

// PMXActor コンストラクタ
PMXActor::PMXActor(const char* filepath, MmdlRenderer& renderer)
	: m_pRenderer(renderer)
	, m_pDx12(renderer.m_pDx12)
	, m_ModelData{}
	, m_pTextureResource{}
	, m_pSphResource{}
	, m_pToonResource{}
	, m_RuntimeBones{}
	, m_MotionData{}
	, m_PMXBoneNameToIndexMap{}
	, m_VMDBoneNameToPmxBoneIndexMap{}
	, m_IsPlayingAnimation(true)
	, m_CurrentAnimationTime(0.0f)
	, m_AnimationSpeed(30.0f)
	, m_MaxFrame(0)
	, m_StartFrame(0.0f)
	, m_EndFrame(0.0f)
	, m_AnimationStartTime(std::chrono::high_resolution_clock::now())
	, m_pMappedVertex(nullptr)
	, m_MappedIndex(nullptr)
	, m_pMappedTransformCB(nullptr)
	, m_pMappedBoneTransforms(nullptr)
	, m_CbvSrvUavDescriptorSize(0)
	, m_gpuDescriptorHandles(RootParamIndex::RP_COUNT) // Enumのサイズで初期化
	, m_materialRootTableGpuHandles{}
{
	try {
		// 1. PMXファイルからCPU側データ（ヘッダー、頂点、インデックス、マテリアル、ボーンなど）を読み込む
		PMXParser parser;
		parser.Load(filepath, m_ModelData);

#if _DEBUG
		{
			float min_y = FLT_MAX;
			float max_y = -FLT_MAX;
			for (const Model::Vertex& vertex : m_ModelData.Vertices)
			{
				min_y = std::min(min_y, vertex.Position.y);
				max_y = std::max(max_y, vertex.Position.y);
			}
			m_LocalHeight = (max_y > min_y) ? (max_y - min_y) : 0.0f;
		}
#endif

		// 2. 読み込んだPMXボーンデータに基づいてRuntimeBonesを初期化し、親子関係やマップを構築
		InitializeRuntimeBones();

		// 3. VMDファイルをロード（例として固定パス、必要に応じて引数で渡す）
		LoadVMDFile("Data\\Model\\PMX\\Hatune\\Anim\\LateralMove.vmd"); // 実際のVMDファイルパスに置き換える
		// 例: LoadVMDFile("C:\\Users\\<YourUser>\\Documents\\MMD\\VMD\\motion.vmd");
		
		// 5. 全てのCPU側データが準備できた後、GPUリソースを作成し、データを転送
		CreateResources();

	}
	catch (const std::runtime_error& Msg) {
		std::wstring WStr = MyString::StringToWString(Msg.what());
		_ASSERT_EXPR(false, WStr.c_str());
		//throw; // エラーを上位に伝える（ここではテスト用なのでアサートのみ）
	}
}

PMXActor::~PMXActor()
{
	// マップされたリソースのアンマップ
	if (m_pMappedVertex && m_pVertexBuffer) {
		m_pVertexBuffer->Unmap(0, nullptr);
		m_pMappedVertex = nullptr;
	}
	if (m_MappedIndex && m_pIndexBuffer) {
		m_pIndexBuffer->Unmap(0, nullptr);
		m_MappedIndex = nullptr;
	}
	if (m_pMappedTransformCB && m_pTransformConstantBuffer) {
		// Transform CBは常にマップされている可能性があるので、アンマップが必要な場合のみ
		// 通常はレンダーフレームごとに更新し、Unmap/Mapを繰り返すか、Persistent mapを使う
		// 今回はコンストラクタでMapしっぱなしなので、デストラクタでUnmap
		m_pTransformConstantBuffer->Unmap(0, nullptr);
		m_pMappedTransformCB = nullptr;
	}
	if (m_pMappedBoneTransforms && m_pBoneTransformStructuredBuffer) {
		// Bone StructuredBufferも同様
		m_pBoneTransformStructuredBuffer->Unmap(0, nullptr);
		m_pMappedBoneTransforms = nullptr;
	}
}

void PMXActor::Update() {

	if (m_IsPlayingAnimation) {
		// 現在時刻から経過時間を計算
		auto currentTime = std::chrono::high_resolution_clock::now();
		std::chrono::duration<float> deltaTimeChrono = currentTime - m_AnimationStartTime;
		float deltaTime = deltaTimeChrono.count(); // 秒単位の経過時間

		// 再生範囲(m_StartFrame ～ m_EndFrame)内でループする. 範囲未設定(EndFrame <= StartFrame)なら全体をループ.
		float range = m_EndFrame - m_StartFrame;
		if (range <= 0.0f) { range = static_cast<float>(m_MaxFrame + 1); }

		// アニメーションフレーム数に変換 (MMDは30FPSが標準)
		m_CurrentAnimationTime = m_StartFrame + fmod(deltaTime * m_AnimationSpeed, range); // ループ再生

	}

	UpdateAnimation();
}

void PMXActor::StepFrame() {
	// 再生範囲内で1フレームだけ進める(壁時計に依存しない. Editorでの単一ステップ用).
	float range = m_EndFrame - m_StartFrame;
	if (range <= 0.0f) { range = static_cast<float>(m_MaxFrame + 1); }

	float localTime = m_CurrentAnimationTime - m_StartFrame + 1.0f;
	m_CurrentAnimationTime = m_StartFrame + fmod(localTime, range);

	UpdateAnimation();
}

void PMXActor::Draw() {
	auto commandList = m_pDx12.GetCommandList();
	auto descriptorSize = m_CbvSrvUavDescriptorSize; // メンバー変数に持っているディスクリプタサイズ

	// 1. 頂点バッファとインデックスバッファを設定
	commandList->IASetVertexBuffers(0, 1, &m_pVertexBufferView);
	commandList->IASetIndexBuffer(&m_pIndexBufferView);
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// 2. ディスクリプタヒープを設定
	// m_pCbvSrvUavHeap が唯一のCBV/SRV/UAVヒープとして使われる
	ID3D12DescriptorHeap* ppHeaps[] = { m_pCbvSrvUavHeap.Get() };
	commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

	// 3. ルートパラメータのバインド (Root Signature の定義と一致させる)


	// --- ルートパラメータ0: Scene Constant Buffer (b0) ---
	// Scene CBV はヒープのインデックス0に作成されているので、オフセットは0のまま
	D3D12_GPU_DESCRIPTOR_HANDLE sceneCbvGpuHandle = m_pCbvSrvUavHeap->GetGPUDescriptorHandleForHeapStart();
	commandList->SetGraphicsRootDescriptorTable(0, sceneCbvGpuHandle); // RootParam[0] にScene CBVをバインド

	// --- ルートパラメータ1: Transform Constant Buffer (b1) ---
	// m_pCbvSrvUavHeap の先頭からTransform CBVまでのオフセットを計算
	// Scene CBV (b0) がヒープのインデックス0にあるので、その分オフセットする
	D3D12_GPU_DESCRIPTOR_HANDLE transformCbvGpuHandle = m_pCbvSrvUavHeap->GetGPUDescriptorHandleForHeapStart();
	transformCbvGpuHandle.ptr += descriptorSize; // Scene CBVの分だけオフセットしてインデックス1を指す
	commandList->SetGraphicsRootDescriptorTable(1, transformCbvGpuHandle); // RootParam[1] にTransform CBVをバインド

	// --- ルートパラメータ3: Bone StructuredBuffer (t3) ---
	D3D12_GPU_DESCRIPTOR_HANDLE boneSrvGpuHandle = m_pCbvSrvUavHeap->GetGPUDescriptorHandleForHeapStart();
	boneSrvGpuHandle.ptr += descriptorSize; // Scene CBVの分だけオフセット
	boneSrvGpuHandle.ptr += descriptorSize; // Transform CBVの分だけオフセット
	boneSrvGpuHandle.ptr += static_cast<UINT64>(m_ModelData.Materials.size()) * 4 * descriptorSize; // 全マテリアルセットのオフセットを加算

	commandList->SetGraphicsRootDescriptorTable(3, boneSrvGpuHandle); // RootParam[3] にBone StructuredBuffer SRVをバインド

	// 不透明材質を先に描画し、半透明材質が深度を書き込まないようにする.
	for (int pass = 0; pass < 2; ++pass)
	{
		commandList->SetPipelineState(pass == 0 ? m_pRenderer.GetPipelineState() : m_pRenderer.GetTransparentPipelineState());
		unsigned int idxOffset = 0;
		for (int i = 0; i < static_cast<int>(m_ModelData.Materials.size()); ++i)
		{
			const bool is_transparent = m_ModelData.Materials[i].Diffuse.w < 0.9999f;
			if (is_transparent != (pass == 1))
			{
				idxOffset += m_ModelData.Materials[i].NumFaceCount;
				continue;
			}
			const unsigned int numFaceVertices = m_ModelData.Materials[i].NumFaceCount;

		// --- ルートパラメータ2: マテリアルごとのディスクリプタテーブル ---
		D3D12_GPU_DESCRIPTOR_HANDLE materialRootTableGpuHandle = m_pCbvSrvUavHeap->GetGPUDescriptorHandleForHeapStart();
		materialRootTableGpuHandle.ptr += descriptorSize; // Scene CBVの分だけオフセット
		materialRootTableGpuHandle.ptr += descriptorSize; // Transform CBVの分だけオフセット
		materialRootTableGpuHandle.ptr += static_cast<UINT64>(i) * 4 * descriptorSize; // 現在のマテリアルセットの先頭へ

		commandList->SetGraphicsRootDescriptorTable(2, materialRootTableGpuHandle); // RootParam[2] にマテリアルディスクリプタテーブルをバインド

		// 描画コマンド
			commandList->DrawIndexedInstanced(numFaceVertices, 1, idxOffset, 0, 0);

			idxOffset += numFaceVertices;
		}
	}
}

void PMXActor::LoadVMDFile(const std::string& filepath)
{
	try {
		m_MotionData = VMDLoader::Load(filepath);
		// VMDデータとPMXボーンのマッピング
		MapVmdBonesToPmxBones();
	}
	catch (const std::runtime_error& e) {
		std::cerr << "Error loading VMD file: " << e.what() << std::endl;
		// エラー処理
	}
}

// アニメーション開始.
void PMXActor::PlayAnimation()
{

}

void PMXActor::StopAnimation()
{

}

void PMXActor::InitializeRuntimeBones()
{
	const auto& bones = m_ModelData.Bones;
	m_RuntimeBones.resize(bones.size());
	m_PMXBoneNameToIndexMap.clear();

	// 1. 各ボーンの基本的な情報を設定し、OffsetMatrix を計算
	for (size_t i = 0; i < bones.size(); ++i) {
		const Model::Bone& modelBone = bones[i];
		RuntimeBone& runtimeBone = m_RuntimeBones[i];

		runtimeBone.ModelBoneData = &modelBone;
		m_PMXBoneNameToIndexMap[modelBone.Name] = static_cast<int>(i);

		// PMXのボーン位置はワールド空間の絶対座標なので、
		// 親ボーンからの相対位置に変換する必要がある
		DirectX::XMFLOAT3 localPosition = modelBone.Position;

		// 親ボーンが存在する場合、親の位置を引いて相対位置を求める
		if (modelBone.ParentBoneIndex != Model::Bone::NoParentIndex && modelBone.ParentBoneIndex < bones.size()) {
			const Model::Bone& parentBone = bones[modelBone.ParentBoneIndex];
			localPosition.x -= parentBone.Position.x;
			localPosition.y -= parentBone.Position.y;
			localPosition.z -= parentBone.Position.z;

			// 隣接リスト(親→子)を事前構築しておく. これが無いと毎フレーム
			// UpdateBoneGlobalTransforms()が子ボーンを全ボーンから線形探索することになり、
			// ボーン数nに対してO(n^2)かかってしまう(実機でFPS低下の原因と判明).
			m_RuntimeBones[modelBone.ParentBoneIndex].ChildIndices.push_back(static_cast<int32_t>(i));
		}

		runtimeBone.OffsetMatrix = DirectX::XMMatrixTranslationFromVector(DirectX::XMLoadFloat3(&localPosition));

		runtimeBone.CurrentAnimationLocalTransform = DirectX::XMMatrixIdentity();
		runtimeBone.InitialGlobalMatrix = DirectX::XMMatrixIdentity();
// 再帰関数で計算されるので初期化のみ
		runtimeBone.InverseInitialGlobalMatrix = DirectX::XMMatrixIdentity(); // 後で計算
		runtimeBone.FinalWorldMatrix = DirectX::XMMatrixIdentity(); // アニメーション時まで使用しない
	}

	// 2. ボーン階層をたどって InitialGlobalMatrix を再帰的に計算
	for (size_t i = 0; i < m_RuntimeBones.size(); ++i) {
		if (m_RuntimeBones[i].ModelBoneData->ParentBoneIndex == Model::Bone::NoParentIndex) { // ルートボーンの判定
			CalculateInitialGlobalMatricesRecursive(static_cast<int>(i), DirectX::XMMatrixIdentity());
		}
	}

	// 3. 各ボーンの InitialGlobalMatrix の逆行列を計算し、InverseInitialGlobalMatrix に格納
	for (size_t i = 0; i < m_RuntimeBones.size(); ++i) {
		RuntimeBone& runtimeBone = m_RuntimeBones[i];
		runtimeBone.InverseInitialGlobalMatrix = DirectX::XMMatrixInverse(nullptr, runtimeBone.InitialGlobalMatrix);
	}
}

// CalculateInitialGlobalMatricesRecursive 関数は、InitialGlobalMatrix の計算では不要になる
// ただし、UpdateAnimation 関数でのアニメーション適用時のグローバル行列計算（階層をたどる）には
// 同様の再帰ロジックが必要になる可能性がある
// (PMXActor::UpdateAnimation() で呼ばれる CalculateFinalBoneMatricesRecursive() のようなもの)

void PMXActor::CalculateInitialGlobalMatricesRecursive(int boneIndex, const DirectX::XMMATRIX& parentInitialGlobalMatrix)
{
	RuntimeBone& currentBone = m_RuntimeBones[boneIndex];

	// ここで InitialGlobalMatrix を計算し、専用のメンバーに格納
	// 自身のInitialGlobalMatrix = 親のInitialGlobalMatrix * 自身のOffsetMatrix
	// これはMMDの一般的な計算順序に近く、親の空間に自身を配置する意味合い
	currentBone.InitialGlobalMatrix = parentInitialGlobalMatrix * currentBone.OffsetMatrix;
	// もしモデルが飛ぶなら、 currentBone.OffsetMatrix * parentInitialGlobalMatrix; も試す価値あり

	// このボーンの子ボーンを検索し、再帰的に処理
	for (size_t i = 0; i < m_RuntimeBones.size(); ++i) {
		if (m_RuntimeBones[i].ModelBoneData->ParentBoneIndex == boneIndex) {
			// 子の再帰呼び出しには、現在のボーンの InitialGlobalMatrix を渡す
			CalculateInitialGlobalMatricesRecursive(static_cast<int>(i), currentBone.InitialGlobalMatrix);
		}
	}
}

void PMXActor::MapVmdBonesToPmxBones()
{
	m_VMDBoneNameToPmxBoneIndexMap.clear();
	for (const auto& pair : m_MotionData.BoneKeyFrames) {
		const std::string& vmdBoneName = pair.first;
		auto it = m_PMXBoneNameToIndexMap.find(vmdBoneName);
		if (it != m_PMXBoneNameToIndexMap.end()) {
			// PMXモデルにVMDのボーン名と同じボーンが存在する場合.
			m_VMDBoneNameToPmxBoneIndexMap[vmdBoneName] = it->second;
		}
		else {
			// PMXモデルにVMDボーン名と同じボーンがない場合、警告などを出すと良い
			std::cerr << "Warning: VMD bone '" << vmdBoneName << "' not found in PMX model." << std::endl;
		}
	}

	// アニメーションの最大フレーム数を計算 (全てのボーンキーフレームから).
	m_MaxFrame = 0;
	for (const auto& pair : m_MotionData.BoneKeyFrames) {
		for (const auto& frame : pair.second) {
			if (frame.FrameNo > m_MaxFrame) {
				m_MaxFrame = frame.FrameNo;
			}
		}
	}

	// 再生範囲をロードしたVMDの全体(0～MaxFrame)にリセットする.
	m_StartFrame = 0.0f;
	m_EndFrame = static_cast<float>(m_MaxFrame);

	// VMDボーンキーフレームをFrameNoでソート.
	for (auto& pair : m_MotionData.BoneKeyFrames) {
		std::vector<VMD::BoneFrame>& keyFrames = pair.second;
		std::sort(keyFrames.begin(), keyFrames.end(),
			[](const VMD::BoneFrame& a, const VMD::BoneFrame& b) {
				return a.FrameNo < b.FrameNo;
			});
	}
}

void PMXActor::UpdateAnimation()
{ 
	// 全てのPMXボーンについて処理
	for (int i = 0; i < m_RuntimeBones.size(); ++i) {
		RuntimeBone& runtimeBone = m_RuntimeBones[i];
		const std::string& pmxBoneName = runtimeBone.ModelBoneData->Name; // PMXボーン名を取得

		// このPMXボーンに対応するVMDキーフレームデータがあるか確認
		auto vmdBoneMapIt = m_VMDBoneNameToPmxBoneIndexMap.find(pmxBoneName);
		if (vmdBoneMapIt != m_VMDBoneNameToPmxBoneIndexMap.end()) {
			// VMDボーン名からキーフレームリストを取得
			const std::vector<VMD::BoneFrame>& keyFrames = m_MotionData.BoneKeyFrames.at(pmxBoneName);

			// キーフレームがない場合はスキップ (通常はありえないが念のため)
			if (keyFrames.empty()) continue;

			// 現在のフレーム番号
			uint32_t currentFrameNo = static_cast<uint32_t>(m_CurrentAnimationTime);

			// 直前と直後のキーフレームを検索
			auto itNext = std::lower_bound(keyFrames.begin(), keyFrames.end(), currentFrameNo,
				[](const VMD::BoneFrame& frame, uint32_t val) { return frame.FrameNo < val; });

			VMD::BoneFrame currentFrame = {};
			VMD::BoneFrame nextFrame = {};

			if (itNext == keyFrames.end()) {
				// 現在のフレームが最後のキーフレーム以降の場合
				currentFrame = keyFrames.back();
				nextFrame = keyFrames.back(); // 最後のフレームで固定
			}
			else if (itNext == keyFrames.begin()) {
				// 現在のフレームが最初のキーフレーム以前の場合 (通常は0フレームが最初のキーフレーム)
				currentFrame = keyFrames.front();
				nextFrame = keyFrames.front(); // 最初のフレームで固定
			}
			else {
				nextFrame = *itNext;
				currentFrame = *(--itNext); // 直前のキーフレーム
			}

			// 補間係数の計算 (tは0.0～1.0)
			float t = 0.0f;
			if (currentFrame.FrameNo != nextFrame.FrameNo) {
				t = (m_CurrentAnimationTime - static_cast<float>(currentFrame.FrameNo)) / static_cast<float>(nextFrame.FrameNo - currentFrame.FrameNo);
			}

			// (VMDの補間曲線データInterpolation[64]を考慮する場合はここでtを変換する)
			// 位置と回転の補間
			DirectX::XMVECTOR interpolatedPosition = DirectX::XMVectorLerp(
				DirectX::XMLoadFloat3(&currentFrame.Position),
				DirectX::XMLoadFloat3(&nextFrame.Position),
				t
			);
			DirectX::XMVECTOR interpolatedRotation = DirectX::XMQuaternionSlerp(
				DirectX::XMLoadFloat4(&currentFrame.Rotation),
				DirectX::XMLoadFloat4(&nextFrame.Rotation),
				t
			);

			if (pmxBoneName == "FHair_M2_2")
			{
				DirectX::XMFLOAT3 pos;
				DirectX::XMStoreFloat3(&pos, interpolatedPosition);
				DirectX::XMFLOAT4 rot;
				DirectX::XMStoreFloat4(&rot, interpolatedRotation);

				// デバッグ用: 全ての親ボーンの位置と回転をログ出力
				char debugMessage[512]; // 十分なサイズのバッファを用意
				sprintf_s(debugMessage, sizeof(debugMessage),
					"DEBUG: 全ての親ボーン - Position: (%.6f, %.6f, %.6f), Rotation: (%.6f, %.6f, %.6f, %.6f)\n",
					pos.x, pos.y, pos.z, rot.x, rot.y, rot.z, rot.w);

				OutputDebugStringA(debugMessage);
			}

			// 補間された位置と回転からローカル変換行列を構築し、CurrentAnimationLocalTransform に設定
			// VMDのPositionはボーンの「ローカル移動」なので、回転してから移動を適用
			DirectX::XMMATRIX animTranslation = DirectX::XMMatrixTranslationFromVector(interpolatedPosition);
			DirectX::XMMATRIX animRotation = DirectX::XMMatrixRotationQuaternion(interpolatedRotation);

			// VMDのアニメーション行列は、PMXの初期ローカル姿勢からの「追加変位」と考える
			// これは「アニメーションによるローカル変位」であり、PMXの初期ローカル移動(OffsetMatrix)とは別々に管理
			runtimeBone.CurrentAnimationLocalTransform = animRotation * animTranslation;
		}
		else {
			// VMDにこのPMXボーンのキーフレームデータがない場合
			// アニメーションしないボーンは、VMDアニメーションによる変位がないので単位行列
			runtimeBone.CurrentAnimationLocalTransform = DirectX::XMMatrixIdentity();
		}
	}

	// ボーン階層をたどってグローバル変換行列を計算し、StructuredBufferに転送
	// ルートボーンから順に計算していく
	// (PMXモデルのParentBoneIndexが0xFFFFUのボーンから始める)
	for (size_t i = 0; i < m_RuntimeBones.size(); ++i) {
		if (m_RuntimeBones[i].ModelBoneData->ParentBoneIndex == Model::Bone::NoParentIndex) {
			// ルートボーンのFinalWorldMatrix計算の修正
			// ルートボーンのグローバル行列 = (PMXの初期ローカル移動) * (VMDアニメーションによるローカル変位)
			// parentWorldMatrix はワールド原点なので単位行列を渡す
			UpdateBoneGlobalTransforms(static_cast<int>(i), DirectX::XMMatrixIdentity());
		}
	}

	// 計算された最終ボーン行列 (m_RuntimeBones[i].FinalWorldMatrix) をGPUのStructuredBuffer (m_pMappedBoneTransforms) に転送
	for (size_t i = 0; i < m_ModelData.Bones.size(); ++i) {
		// スキニング用の最終行列を計算
		// DirectXMathは行ベクトル規約: pos * Matrix
		// 頂点 * InverseBindPose * AnimatedPose の順で適用される
		// つまり boneMatrix = InverseBindPose * AnimatedPose (右から左へ適用)
		m_pMappedBoneTransforms[i] = m_RuntimeBones[i].InverseInitialGlobalMatrix * m_RuntimeBones[i].FinalWorldMatrix;
	}
}

// ボーンのグローバル変換行列を更新する再帰関数 (変更)
// parentGlobalTransform: 親ボーンのすでに計算されたグローバル変換行列
void PMXActor::UpdateBoneGlobalTransforms(int boneIndex, const DirectX::XMMATRIX& parentGlobalTransform)
{

	if (boneIndex == -1 || boneIndex >= m_RuntimeBones.size()) {
		return;
	}

	RuntimeBone& runtimeBone = m_RuntimeBones[boneIndex];

	// このボーンのアニメーション適用後の「ローカル行列」を計算
	// これは「初期ローカル移動 (OffsetMatrix)」に「VMDアニメーションによる変位 (CurrentAnimationLocalTransform)」を乗算したもの
	DirectX::XMMATRIX currentLocalAnimatedMatrix =
		runtimeBone.OffsetMatrix * runtimeBone.CurrentAnimationLocalTransform;

	// このボーンの「グローバル行列」を計算
	// 自分のアニメーション適用後のローカル行列を、親のグローバル行列に乗算
	runtimeBone.FinalWorldMatrix = currentLocalAnimatedMatrix * parentGlobalTransform;

	// 子ボーンを再帰的に更新(InitializeRuntimeBonesで事前構築した隣接リストを辿るだけ.
	// 以前は毎回全ボーンを線形探索していたためO(ボーン数^2)だった).
	for (int32_t childIndex : runtimeBone.ChildIndices) {
		UpdateBoneGlobalTransforms(childIndex, runtimeBone.FinalWorldMatrix);
	}
}

void PMXActor::CreateResources()
{
	// UPLOADヒーププロパティを一度定義
	D3D12_HEAP_PROPERTIES uploadHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

	// ===== 頂点バッファの作成とマップ =====
	D3D12_RESOURCE_DESC vertexBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(m_ModelData.Vertices.size() * Model::GPU_VERTEX_SIZE);
	MyAssert::IsFailed(_T("頂点バッファの作成"), &ID3D12Device::CreateCommittedResource, m_pDx12.GetDevice(),
		&uploadHeapProperties, D3D12_HEAP_FLAG_NONE, &vertexBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
		IID_PPV_ARGS(m_pVertexBuffer.ReleaseAndGetAddressOf()));
	MyAssert::IsFailed(_T("頂点バッファをマップ"), &ID3D12Resource::Map, m_pVertexBuffer.Get(),
		0, nullptr, (void**)&m_pMappedVertex);
	std::copy(m_ModelData.Vertices.begin(), m_ModelData.Vertices.end(), m_pMappedVertex);
	m_pVertexBuffer->Unmap(0, nullptr);
	m_pMappedVertex = nullptr; // マップ解除後はポインタをnullptrにする

	m_pVertexBufferView.BufferLocation = m_pVertexBuffer->GetGPUVirtualAddress();
	m_pVertexBufferView.SizeInBytes = static_cast<UINT>(m_ModelData.Vertices.size()) * Model::GPU_VERTEX_SIZE;
	m_pVertexBufferView.StrideInBytes = Model::GPU_VERTEX_SIZE;

	// ===== インデックスバッファの作成とマップ =====
	D3D12_RESOURCE_DESC indexBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(static_cast<UINT64>(m_ModelData.Indices.size()) * Model::GPU_INDEX_SIZE);
	MyAssert::IsFailed(_T("インデックスバッファの作成"), &ID3D12Device::CreateCommittedResource, m_pDx12.GetDevice(),
		&uploadHeapProperties, D3D12_HEAP_FLAG_NONE, &indexBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
		IID_PPV_ARGS(m_pIndexBuffer.ReleaseAndGetAddressOf()));
	MyAssert::IsFailed(_T("インデックスバッファをマップ"), &ID3D12Resource::Map, m_pIndexBuffer.Get(),
		0, nullptr, (void**)&m_MappedIndex);
	std::copy(m_ModelData.Indices.begin(), m_ModelData.Indices.end(), m_MappedIndex);
	m_pIndexBuffer->Unmap(0, nullptr);
	m_MappedIndex = nullptr; // マップ解除後はポインタをnullptrにする

	m_pIndexBufferView.BufferLocation = m_pIndexBuffer->GetGPUVirtualAddress();
	m_pIndexBufferView.Format = DXGI_FORMAT_R32_UINT;
	m_pIndexBufferView.SizeInBytes = static_cast<UINT>(m_ModelData.Indices.size()) * Model::GPU_INDEX_SIZE;

	// ===== Transform Constant Buffer (b1) の作成とマップ =====
	const UINT transformCbvSizeAligned = (sizeof(PMX::TransformConstantBuffer) + 255) & ~255;
	D3D12_RESOURCE_DESC transformBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(transformCbvSizeAligned);
	MyAssert::IsFailed(_T("Transform Constant Bufferの作成"), &ID3D12Device::CreateCommittedResource, m_pDx12.GetDevice(),
		&uploadHeapProperties, D3D12_HEAP_FLAG_NONE, &transformBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
		IID_PPV_ARGS(m_pTransformConstantBuffer.ReleaseAndGetAddressOf()));
	MyAssert::IsFailed(_T("Transform Constant Bufferをマップ"), &ID3D12Resource::Map, m_pTransformConstantBuffer.Get(),
		0, nullptr, (void**)&m_pMappedTransformCB);

	if (m_pMappedTransformCB != nullptr) {
		m_pMappedTransformCB->World = DirectX::XMMatrixScaling(1.f, 1.f, 1.f);
		m_pMappedTransformCB->BoneCount = static_cast<UINT>(m_ModelData.Bones.size());
	}
	// Transform CBは頻繁に更新されるため、Mapしっぱなしが一般的。Unmapはデストラクタで行う。

	// ===== Bone StructuredBuffer (t3) の作成とマップ =====
	D3D12_RESOURCE_DESC boneBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(static_cast<UINT64>(m_ModelData.Bones.size()) * sizeof(DirectX::XMMATRIX));
	MyAssert::IsFailed(_T("ボーンStructuredBufferの作成"), &ID3D12Device::CreateCommittedResource, m_pDx12.GetDevice(),
		&uploadHeapProperties, D3D12_HEAP_FLAG_NONE, &boneBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
		IID_PPV_ARGS(m_pBoneTransformStructuredBuffer.ReleaseAndGetAddressOf()));
	MyAssert::IsFailed(_T("ボーンStructuredBufferをマップ"), &ID3D12Resource::Map, m_pBoneTransformStructuredBuffer.Get(),
		0, nullptr, (void**)&m_pMappedBoneTransforms);
	// Bone StructuredBufferも毎フレーム更新されるため、Mapしっぱなしが一般的。

	// ===== CBV/SRV/UAV ディスクリプタヒープの作成 =====
	UINT totalDescriptors = 0;
	totalDescriptors += 1; // RP_SCENE_CBV (b0)
	totalDescriptors += 1; // RP_TRANSFORM_CBV (b1)
	totalDescriptors += (1 + 3) * static_cast<UINT>(m_ModelData.Materials.size()); // マテリアルごとの (Material CBV + BaseTex SRV + ToonTex SRV + SphTex SRV)
	totalDescriptors += 1; // RP_BONE_SRV (t3)

	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.NumDescriptors = totalDescriptors;
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	heapDesc.NodeMask = 0;

	MyAssert::IsFailed(_T("CBV/SRV/UAV ディスクリプタヒープを作成"), &ID3D12Device::CreateDescriptorHeap, m_pDx12.GetDevice(),
		&heapDesc, IID_PPV_ARGS(m_pCbvSrvUavHeap.ReleaseAndGetAddressOf()));

	m_CbvSrvUavDescriptorSize = m_pDx12.GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	D3D12_CPU_DESCRIPTOR_HANDLE currentCpuHandle = m_pCbvSrvUavHeap->GetCPUDescriptorHandleForHeapStart();
	D3D12_GPU_DESCRIPTOR_HANDLE currentGpuHandle = m_pCbvSrvUavHeap->GetGPUDescriptorHandleForHeapStart();

	m_materialRootTableGpuHandles.resize(m_ModelData.Materials.size());

	// ===== ディスクリプタの作成 (ルートパラメータの順序と一致させる) =====

	// --- RP_SCENE_CBV (b0) ---
	ID3D12Resource* pSceneCBResource = m_pDx12.GetSceneConstantBuffer(); // Directx12クラスから取得すると仮定
	if (pSceneCBResource) {
		D3D12_CONSTANT_BUFFER_VIEW_DESC sceneCbvDesc = {};
		sceneCbvDesc.BufferLocation = pSceneCBResource->GetGPUVirtualAddress();
		sceneCbvDesc.SizeInBytes = static_cast<UINT>(pSceneCBResource->GetDesc().Width);
		m_pDx12.GetDevice()->CreateConstantBufferView(&sceneCbvDesc, currentCpuHandle);
		m_gpuDescriptorHandles[RP_SCENE_CBV] = currentGpuHandle;
	}
	else { /* エラーログまたはデフォルト設定 */ }
	currentCpuHandle.ptr += m_CbvSrvUavDescriptorSize;
	currentGpuHandle.ptr += m_CbvSrvUavDescriptorSize;

	// --- RP_TRANSFORM_CBV (b1) ---
	D3D12_CONSTANT_BUFFER_VIEW_DESC transformCbvDesc = {};
	transformCbvDesc.BufferLocation = m_pTransformConstantBuffer->GetGPUVirtualAddress();
	transformCbvDesc.SizeInBytes = transformCbvSizeAligned;
	m_pDx12.GetDevice()->CreateConstantBufferView(&transformCbvDesc, currentCpuHandle);
	m_gpuDescriptorHandles[RP_TRANSFORM_CBV] = currentGpuHandle;
	currentCpuHandle.ptr += m_CbvSrvUavDescriptorSize;
	currentGpuHandle.ptr += m_CbvSrvUavDescriptorSize;

	// --- マテリアルデータとテクスチャリソースのロード・ディスクリプタ作成 ---
	std::vector<Model::MaterialForHLSL> materialsForHLSL(m_ModelData.Materials.size()); // HLSL用マテリアルデータ.
	m_pTextureResource.resize(m_ModelData.Materials.size()); // 各マテリアルに対応するテクスチャリソースのサイズ確保
	m_pSphResource.resize(m_ModelData.Materials.size());
	m_pToonResource.resize(m_ModelData.Materials.size());

	// マテリアルデータをGPU転送するためのアップロードバッファ
	// サイズを256アライメント.
	int MaterialBufferSizeAligned = Model::GPU_MATERIAL_SIZE; // Model::GPU_MATERIAL_SIZE が256アラインメント済みか確認
	MaterialBufferSizeAligned = (MaterialBufferSizeAligned + 255) & ~255; // 念のため256アラインメントを保証

	D3D12_RESOURCE_DESC MaterialBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(static_cast<UINT64>(MaterialBufferSizeAligned) * m_ModelData.Materials.size());

	MyComPtr<ID3D12Resource> pMaterialUploadBuffer;
	const UINT materialUploadBufferSize = static_cast<UINT>(static_cast<UINT64>(m_ModelData.Materials.size()) * MaterialBufferSizeAligned);
	D3D12_RESOURCE_DESC materialUploadBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(materialUploadBufferSize);
	MyAssert::IsFailed(_T("マテリアルアップロードバッファの作成"), &ID3D12Device::CreateCommittedResource, m_pDx12.GetDevice(),
		&uploadHeapProperties, D3D12_HEAP_FLAG_NONE, &materialUploadBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
		IID_PPV_ARGS(pMaterialUploadBuffer.ReleaseAndGetAddressOf()));
	char* pMappedMaterialUpload = nullptr;
	MyAssert::IsFailed(_T("マテリアルアップロードバッファをマップ"), &ID3D12Resource::Map, pMaterialUploadBuffer.Get(),
		0, nullptr, (void**)&pMappedMaterialUpload);

	for (int i = 0; i < static_cast<int>(m_ModelData.Materials.size()); ++i) {
		m_materialRootTableGpuHandles[i] = currentGpuHandle; // このマテリアルのルートテーブルの開始GPUハンドルを保存

		// CPU側マテリアルデータをHLSL構造体に変換して、アップロードバッファにコピー
		const Model::Material& material = m_ModelData.Materials[i];
		Model::MaterialForHLSL& gpuMaterial = materialsForHLSL[i];
		gpuMaterial.Diffuse = material.Diffuse;
		gpuMaterial.Specular = material.Specular;
		gpuMaterial.SpecularPower = material.SpecularPower;
		gpuMaterial.Ambient = material.Ambient;
		gpuMaterial.UseSphereMap = material.Textures.UseSphereMap ? 1.f : 0.f; // スフィアマップの使用フラグ
		gpuMaterial.UseToonMap = material.Textures.UseToonMap ? 1.f : 0.f; // トゥーン計算の使用フラグ
		std::memcpy(pMappedMaterialUpload + i * MaterialBufferSizeAligned, &gpuMaterial, Model::GPU_MATERIAL_SIZE);

		// テクスチャロード(パス自体はパーサーが解決済み).
		// トゥーンテクスチャ: パスが無い場合はレンダラーのデフォルト(黒)を使う.
		m_pToonResource[i] = material.Textures.ToonTexture.empty()
			? m_pRenderer.GetBlackTex()
			: LoadTexture(material.Textures.ToonTexture.string());

		m_pTextureResource[i] = LoadTexture(material.Textures.BaseTexture.string());
		m_pSphResource[i] = LoadTexture(material.Textures.SphereTexture.string());

		// --- マテリアルCBV (b2) の作成 ---
		D3D12_CONSTANT_BUFFER_VIEW_DESC materialCbvDesc = {};
		materialCbvDesc.BufferLocation = pMaterialUploadBuffer->GetGPUVirtualAddress() + static_cast<UINT64>(i) * MaterialBufferSizeAligned;
		materialCbvDesc.SizeInBytes = MaterialBufferSizeAligned;
		m_pDx12.GetDevice()->CreateConstantBufferView(&materialCbvDesc, currentCpuHandle);
		currentCpuHandle.ptr += m_CbvSrvUavDescriptorSize;
		currentGpuHandle.ptr += m_CbvSrvUavDescriptorSize;

		// --- ベーステクスチャSRV (t0) の作成 ---
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = 1; // 後で実際のMipLevelsに更新される

		ID3D12Resource* pBaseTex = m_pTextureResource[i].Get();
		if (pBaseTex) {
			srvDesc.Format = pBaseTex->GetDesc().Format;
			srvDesc.Texture2D.MipLevels = pBaseTex->GetDesc().MipLevels;
			m_pDx12.GetDevice()->CreateShaderResourceView(pBaseTex, &srvDesc, currentCpuHandle);
		}
		else { /* エラー処理またはデフォルト */ }
		currentCpuHandle.ptr += m_CbvSrvUavDescriptorSize;
		currentGpuHandle.ptr += m_CbvSrvUavDescriptorSize;

		// --- トゥーンテクスチャSRV (t1) の作成 ---
		ID3D12Resource* pToonTex = m_pToonResource[i].Get();
		if (pToonTex) {
			srvDesc.Format = pToonTex->GetDesc().Format;
			srvDesc.Texture2D.MipLevels = pToonTex->GetDesc().MipLevels;
			m_pDx12.GetDevice()->CreateShaderResourceView(pToonTex, &srvDesc, currentCpuHandle);
		}
		else { /* エラー処理またはデフォルト */ }
		currentCpuHandle.ptr += m_CbvSrvUavDescriptorSize;
		currentGpuHandle.ptr += m_CbvSrvUavDescriptorSize;

		// --- スフィアテクスチャSRV (t2) の作成 ---
		ID3D12Resource* pSphTex = m_pSphResource[i].Get();
		if (pSphTex) {
			srvDesc.Format = pSphTex->GetDesc().Format;
			srvDesc.Texture2D.MipLevels = pSphTex->GetDesc().MipLevels;
			m_pDx12.GetDevice()->CreateShaderResourceView(pSphTex, &srvDesc, currentCpuHandle);
		}
		else { /* エラー処理またはデフォルト */ }
		currentCpuHandle.ptr += m_CbvSrvUavDescriptorSize;
		currentGpuHandle.ptr += m_CbvSrvUavDescriptorSize;
	}
	pMaterialUploadBuffer->Unmap(0, nullptr); // マテリアルアップロードバッファのアンマップ

	// --- RP_BONE_SRV (t3) ---
	D3D12_SHADER_RESOURCE_VIEW_DESC boneSrvDesc = {};
    boneSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    boneSrvDesc.Format = DXGI_FORMAT_UNKNOWN; // StructuredBufferの場合
    boneSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    boneSrvDesc.Buffer.FirstElement = 0;
    boneSrvDesc.Buffer.NumElements = static_cast<UINT>(m_ModelData.Bones.size());
    boneSrvDesc.Buffer.StructureByteStride = sizeof(DirectX::XMMATRIX); // XMMATRIXのサイズ
    boneSrvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

    m_pDx12.GetDevice()->CreateShaderResourceView(m_pBoneTransformStructuredBuffer.Get(), &boneSrvDesc, currentCpuHandle);
    m_gpuDescriptorHandles[RootParamIndex::RP_BONE_SRV] = currentGpuHandle; // ここでハンドルを格納
    currentCpuHandle.ptr += m_CbvSrvUavDescriptorSize; // 次のディスクリプタのために進める
    currentGpuHandle.ptr += m_CbvSrvUavDescriptorSize;
}

float PMXActor::GetYFromXOnBezier(
	float x,
	const DirectX::XMFLOAT2& a,
	const DirectX::XMFLOAT2& b, uint8_t n)
{
	if (a.x == a.y && b.x == b.y)return x;//計算不要
	float t = x;
	const float k0 = 1 + 3 * a.x - 3 * b.x;//t^3の係数
	const float k1 = 3 * b.x - 6 * a.x;//t^2の係数
	const float k2 = 3 * a.x;//tの係数

	//誤差の範囲内かどうかに使用する定数
	constexpr float epsilon = 0.0005f;

	for (int i = 0; i < n; ++i) {
		//f(t)求めまーす
		auto ft = k0 * t * t * t + k1 * t * t + k2 * t - x;
		//もし結果が0に近い(誤差の範囲内)なら打ち切り
		if (ft <= epsilon && ft >= -epsilon)break;

		t -= ft / 2;
	}
	//既に求めたいtは求めているのでyを計算する
	auto r = 1 - t;
	return t * t * t + 3 * t * t * r * b.y + 3 * t * r * r * a.y;
}

// テクスチャがなかった場合defaulttextureを渡す.
MyComPtr<ID3D12Resource> PMXActor::LoadTexture(const std::string& path)
{
	if (path.empty()) {
		// 例: パスが空の場合は白テクスチャを返す (用途に合わせて変更)
		return m_pRenderer.GetWhiteTex();
	}

	MyComPtr<ID3D12Resource> texture = m_pDx12.GetTextureByPath(path.c_str());
	// 読み込み失敗(ファイル欠損等)時もnullptrのまま返さず、白テクスチャへフォールバックする
	// (nullptrのままだと該当ディスクリプタスロットが未初期化のままになってしまうため).
	return texture ? texture : m_pRenderer.GetWhiteTex();
}
