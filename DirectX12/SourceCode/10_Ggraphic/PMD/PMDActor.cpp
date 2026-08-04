#include "PMDActor.h"
#include "PMDRenderer.h"
#include "DirectX\\DirectX12.h"
#include "99_Utility\\String\\FilePath\\FilePath.h"
#include "Model\\PMDParser.h"
#include "..\\..\\..\\Data\\Library\\DirectXTex\\Common\\d3dx12.h"

void* PMDActor::Transform::operator new(size_t size) {
	return _aligned_malloc(size, 16);
}

// 回転行列を末端まで伝搬させる再帰関数.
void PMDActor::RecursiveMatrixMultipy(
	BoneNode* node,
	const DirectX::XMMATRIX& mat)
{
	m_BoneMatrix[node->BoneIndex] = mat;
	for (auto& cnode : node->Children) {
		// 子にも同様の処理を行う.
		RecursiveMatrixMultipy(cnode, m_BoneMatrix[cnode->BoneIndex] * mat);
	}
}

float PMDActor::GetYFromXOnBezier(
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
		//f(t)を求める
		auto ft = k0 * t * t * t + k1 * t * t + k2 * t - x;
		//求めた結果が0に近ければ(誤差の範囲内)なら打ち切る
		if (ft <= epsilon && ft >= -epsilon)break;

		t -= ft / 2;
	}
	//既に求めたtは求めているのでyを計算する
	auto r = 1 - t;
	return t * t * t + 3 * t * t * r * b.y + 3 * t * r * r * a.y;
}

PMDActor::PMDActor(const char* filepath,PMDRenderer& renderer):
	m_pRenderer(renderer),
	m_pDx12(renderer.m_pDx12),
	_angle(0.0f)
{
	try {
		m_Transform.world = DirectX::XMMatrixIdentity();

		// 1. PMDファイルからCPU側データを読み込む.
		PMDParser parser;
		parser.Load(filepath, m_ModelData);

		// 2. 読み込んだボーンデータをもとにボーンノードの階層を構築.
		InitializeBoneNodeTable();

		// 3. GPUリソースを作成.
		CreateVertexIndexBuffers();
		CreateTransformView();
		CreateMaterialData();
		CreateMaterialAndTextureView();
	}
	catch (const std::runtime_error& Msg) {

		// エラーメッセージを表示.
		std::wstring WStr = MyString::StringToWString(Msg.what());
		_ASSERT_EXPR(false, WStr.c_str());
	}
}


PMDActor::~PMDActor()
{
}

// GPU用の頂点/インデックスバッファを作成.
void PMDActor::CreateVertexIndexBuffers()
{
	auto heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

	// 頂点バッファ用のDirectX 12リソースを作成.
	auto vertexBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(m_ModelData.Vertices.size() * Model::GPU_VERTEX_SIZE);
	MyAssert::IsFailed(_T("頂点バッファの作成"), &ID3D12Device::CreateCommittedResource, m_pDx12.GetDevice(),
		&heapProp, D3D12_HEAP_FLAG_NONE, &vertexBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
		IID_PPV_ARGS(m_pVertexBuffer.ReleaseAndGetAddressOf()));

	// 頂点データをGPUバッファにコピー.
	Model::Vertex* vertMap = nullptr;
	MyAssert::IsFailed(_T("頂点バッファをマップ"), &ID3D12Resource::Map, m_pVertexBuffer.Get(),
		0, nullptr, (void**)&vertMap);
	std::copy(m_ModelData.Vertices.begin(), m_ModelData.Vertices.end(), vertMap);
	m_pVertexBuffer->Unmap(0, nullptr);

	// 頂点バッファビューの設定.
	m_pVertexBufferView.BufferLocation = m_pVertexBuffer->GetGPUVirtualAddress();
	m_pVertexBufferView.SizeInBytes = static_cast<UINT>(m_ModelData.Vertices.size()) * Model::GPU_VERTEX_SIZE;
	m_pVertexBufferView.StrideInBytes = Model::GPU_VERTEX_SIZE;

	// インデックスバッファ用のDirectX 12リソースを作成.
	auto indexBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(static_cast<UINT64>(m_ModelData.Indices.size()) * Model::GPU_INDEX_SIZE);
	MyAssert::IsFailed(_T("インデックスバッファの作成"), &ID3D12Device::CreateCommittedResource, m_pDx12.GetDevice(),
		&heapProp, D3D12_HEAP_FLAG_NONE, &indexBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
		IID_PPV_ARGS(m_pIndexBuffer.ReleaseAndGetAddressOf()));

	// インデックスデータをGPUバッファにコピー.
	uint32_t* mappedIdx = nullptr;
	MyAssert::IsFailed(_T("インデックスバッファをマップ"), &ID3D12Resource::Map, m_pIndexBuffer.Get(),
		0, nullptr, (void**)&mappedIdx);
	std::copy(m_ModelData.Indices.begin(), m_ModelData.Indices.end(), mappedIdx);
	m_pIndexBuffer->Unmap(0, nullptr);

	// インデックスバッファビューの設定.
	m_pIndexBufferView.BufferLocation = m_pIndexBuffer->GetGPUVirtualAddress();
	m_pIndexBufferView.Format = DXGI_FORMAT_R32_UINT;
	m_pIndexBufferView.SizeInBytes = static_cast<UINT>(m_ModelData.Indices.size()) * Model::GPU_INDEX_SIZE;
}

// 読み込んだボーンデータをもとにボーンノードの階層を構築.
void PMDActor::InitializeBoneNodeTable()
{
	const auto& bones = m_ModelData.Bones;
	std::vector<std::string> boneNames(bones.size());

	// ボーンノードを作成.
	for (size_t i = 0; i < bones.size(); ++i)
	{
		const Model::Bone& bone = bones[i];
		boneNames[i] = bone.Name;
		auto& node = m_BoneNodeTable[bone.Name];
		node.BoneIndex = static_cast<int>(i);
		node.StartPos = bone.Position;
	}

	// 親子関係の構築.
	for (const Model::Bone& bone : bones)
	{
		// ありえない番号ならとばす.
		if (bone.ParentBoneIndex >= bones.size()) { continue; }

		auto parentName = boneNames[bone.ParentBoneIndex];
		m_BoneNodeTable[parentName].Children.emplace_back(&m_BoneNodeTable[bone.Name]);
	}

	// ボーン数分確保.
	m_BoneMatrix.resize(bones.size());

	// ボーン行列を初期化.
	std::fill(
		m_BoneMatrix.begin(),
		m_BoneMatrix.end(),
		DirectX::XMMatrixIdentity()
	);
}

void PMDActor::LoadVMDFile(const char* FilePath, const char* Name)
{
	FILE* fp = nullptr;
	// fopen_sを使ってファイルをバイナリモードで開く.
	auto err = fopen_s(&fp, FilePath, "rb");
	if (err != 0 || !fp) {
		throw std::runtime_error("ファイルを開くことができませんでした。");
	}
	fseek(fp, 50, SEEK_SET);//最初の50バイトは飛ばしてOK
	unsigned int keyframeNum = 0;
	fread(&keyframeNum, sizeof(keyframeNum), 1, fp);

	std::vector<VMDKeyFrame> Keyframes(keyframeNum);
	for (auto& keyframe : Keyframes) {
		fread(keyframe.BoneName, sizeof(keyframe.BoneName), 1, fp);	// ボーン名.
		fread(&keyframe.FrameNo, sizeof(keyframe.FrameNo) +			// フレーム番号.
			sizeof(keyframe.Location) +								// 位置(IKのときに使用予定).
			sizeof(keyframe.Quaternion) +							// クオータニオン.
			sizeof(keyframe.Bezier), 1, fp);						// 補間ベジェデータ.
	}

	for (auto& motion : m_MotionData) {
		std::sort(motion.second.begin(), motion.second.end(),
			[](const KeyFrame& lval, const KeyFrame& rval) {
				return lval.FrameNo <= rval.FrameNo;
			});
	}

	//VMDのキーフレームデータから、実際に使用するキーフレームテーブルへ変換.
	for (auto& f : Keyframes) {
		m_MotionData[f.BoneName].emplace_back(
			KeyFrame(
				f.FrameNo,
				DirectX::XMLoadFloat4(&f.Quaternion),
				DirectX::XMFLOAT2((float)f.Bezier[3] / 127.0f, (float)f.Bezier[7] / 127.0f),
				DirectX::XMFLOAT2((float)f.Bezier[11] / 127.0f, (float)f.Bezier[15] / 127.0f)
			));
	}

	for (auto& bonemotion : m_MotionData) {
		auto node = m_BoneNodeTable[bonemotion.first];
		auto& pos = node.StartPos;
		auto mat = DirectX::XMMatrixTranslation(-pos.x, -pos.y, -pos.z) *
			DirectX::XMMatrixRotationQuaternion(bonemotion.second[0].Quaternion) *
			DirectX::XMMatrixTranslation(pos.x, pos.y, pos.z);
		m_BoneMatrix[node.BoneIndex] = mat;
	}

	RecursiveMatrixMultipy(&m_BoneNodeTable["センター"], DirectX::XMMatrixIdentity());
	std::copy(m_BoneMatrix.begin(), m_BoneMatrix.end(), m_MappedMatrices + 1);

}

void PMDActor::CreateTransformView() {
	//GPUバッファ作成
	auto buffSize = sizeof(Transform) * (1 + m_BoneMatrix.size());
	buffSize = (buffSize + 0xff)&~0xff;
	auto heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto resDesc = CD3DX12_RESOURCE_DESC::Buffer(buffSize);

	MyAssert::IsFailed(
		_T("座標バッファ作成"),
		&ID3D12Device::CreateCommittedResource, m_pDx12.GetDevice(),
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(m_pTransformBuff.ReleaseAndGetAddressOf())
	);

	//マップしてコピー
	MyAssert::IsFailed(
		_T("座標のマップ"),
		&ID3D12Resource::Map, m_pTransformBuff.Get(),
		0, nullptr,
		(void**)&m_MappedMatrices);

	m_MappedMatrices[0] = m_Transform.world;
	copy(m_BoneMatrix.begin(), m_BoneMatrix.end(), m_MappedMatrices + 1);

	// ビューの作成.
	D3D12_DESCRIPTOR_HEAP_DESC transformDescHeapDesc = {};
	transformDescHeapDesc.NumDescriptors = 1; // とりあえずワールドひとつ.
	transformDescHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	transformDescHeapDesc.NodeMask = 0;

	transformDescHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV; // デスクリプタヒープ種別.

	MyAssert::IsFailed(
		_T("座標ヒープの作成"),
		&ID3D12Device::CreateDescriptorHeap, m_pDx12.GetDevice(),
		&transformDescHeapDesc,
		IID_PPV_ARGS(m_pTransformHeap.ReleaseAndGetAddressOf()));//生成

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.BufferLocation = m_pTransformBuff->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = static_cast<UINT>(buffSize);
	m_pDx12.GetDevice()->CreateConstantBufferView(
		&cbvDesc,
		m_pTransformHeap->GetCPUDescriptorHandleForHeapStart());
}

void PMDActor::CreateMaterialData() {
	//マテリアルバッファを作成
	auto MaterialBuffSize = sizeof(Model::MaterialForHLSL);
	MaterialBuffSize = (MaterialBuffSize + 0xff)&~0xff;

	auto heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto resDesc = CD3DX12_RESOURCE_DESC::Buffer(MaterialBuffSize * m_ModelData.Materials.size());

	MyAssert::IsFailed(
		_T("マテリアル作成"),
		&ID3D12Device::CreateCommittedResource, m_pDx12.GetDevice(),
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,//クリアバリューは不要
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(m_pMaterialBuff.ReleaseAndGetAddressOf()));

	//マップしてマテリアルにコピー
	char* mapMaterial = nullptr;

	MyAssert::IsFailed(
		_T("マテリアルをマップにコピー"),
		&ID3D12Resource::Map, m_pMaterialBuff.Get(),
		0, nullptr,
		(void**)&mapMaterial);

	for (const auto& material : m_ModelData.Materials) {
		Model::MaterialForHLSL gpuMaterial;
		gpuMaterial.Diffuse = material.Diffuse;
		gpuMaterial.Specular = material.Specular;
		gpuMaterial.SpecularPower = material.SpecularPower;
		gpuMaterial.Ambient = material.Ambient;
		gpuMaterial.UseSphereMap = material.Textures.UseSphereMap ? 1.f : 0.f;

		*((Model::MaterialForHLSL*)mapMaterial) = gpuMaterial;//データコピー
		mapMaterial += MaterialBuffSize;//次のアライメント位置まで進める
	}

	m_pMaterialBuff->Unmap(0, nullptr);

	// -- 仮実装.

	m_MappedMatrices[0] = DirectX::XMMatrixRotationY(_angle);

	auto armnode = m_BoneNodeTable["左腕"];
	auto& armpos = armnode.StartPos;
	auto armMat =
		DirectX::XMMatrixTranslation(-armpos.x, -armpos.y, -armpos.x)
		* DirectX::XMMatrixRotationZ(DirectX::XM_PIDIV2)
		* DirectX::XMMatrixTranslation(armpos.x, armpos.y, armpos.x);

	auto elbowNode = m_BoneNodeTable["左ひじ"];
	auto& elbowpos = elbowNode.StartPos;
	auto elbowMat = DirectX::XMMatrixTranslation(-elbowpos.x, -elbowpos.y, -elbowpos.x)
		* DirectX::XMMatrixRotationZ(-DirectX::XM_PIDIV2)
		* DirectX::XMMatrixTranslation(elbowpos.x, elbowpos.y, elbowpos.x);

	m_BoneMatrix[armnode.BoneIndex] = armMat;
	m_BoneMatrix[elbowNode.BoneIndex] = elbowMat;

	RecursiveMatrixMultipy(&m_BoneNodeTable ["センター"], DirectX::XMMatrixIdentity());


	copy(m_BoneMatrix.begin(), m_BoneMatrix.end(), m_MappedMatrices + 1);
	// -- 仮実装.
}


void PMDActor::CreateMaterialAndTextureView() {
	// マテリアルごとのテクスチャリソースをロード(パス解決はパーサー側で済).
	m_pTextureResource.resize(m_ModelData.Materials.size());
	m_pSphResource.resize(m_ModelData.Materials.size());
	m_pSpaResource.resize(m_ModelData.Materials.size());
	m_pToonResource.resize(m_ModelData.Materials.size());

	for (size_t i = 0; i < m_ModelData.Materials.size(); ++i) {
		const auto& textures = m_ModelData.Materials[i].Textures;

		if (!textures.BaseTexture.empty()) {
			m_pTextureResource[i] = m_pDx12.GetTextureByPath(textures.BaseTexture.string().c_str());
		}
		if (!textures.SphereTexture.empty()) {
			m_pSphResource[i] = m_pDx12.GetTextureByPath(textures.SphereTexture.string().c_str());
		}
		if (!textures.SphereAddTexture.empty()) {
			m_pSpaResource[i] = m_pDx12.GetTextureByPath(textures.SphereAddTexture.string().c_str());
		}
		if (!textures.ToonTexture.empty()) {
			m_pToonResource[i] = m_pDx12.GetTextureByPath(textures.ToonTexture.string().c_str());
		}
	}

	D3D12_DESCRIPTOR_HEAP_DESC MaterialDescHeapDesc = {};
	MaterialDescHeapDesc.NumDescriptors = static_cast<UINT>(m_ModelData.Materials.size() * 5);//マテリアル数分繰り返す(定数1、テクスチャ4つ)
	MaterialDescHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	MaterialDescHeapDesc.NodeMask = 0;

	MaterialDescHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;//デスクリプタヒープ種別

	MyAssert::IsFailed(
		_T("マテリアルヒープの作成"),
		&ID3D12Device::CreateDescriptorHeap, m_pDx12.GetDevice(),
		&MaterialDescHeapDesc,
		IID_PPV_ARGS(m_pMaterialHeap.ReleaseAndGetAddressOf()));

	auto MaterialBuffSize = sizeof(Model::MaterialForHLSL);
	MaterialBuffSize = (MaterialBuffSize + 0xff)&~0xff;
	D3D12_CONSTANT_BUFFER_VIEW_DESC matCBVDesc = {};
	matCBVDesc.BufferLocation = m_pMaterialBuff->GetGPUVirtualAddress();
	matCBVDesc.SizeInBytes = static_cast<UINT>(MaterialBuffSize);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;//前述のとおり
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;//2Dテクスチャ
	srvDesc.Texture2D.MipLevels = 1;//ミップマップは使用しないので1
	CD3DX12_CPU_DESCRIPTOR_HANDLE matDescHeapH(m_pMaterialHeap->GetCPUDescriptorHandleForHeapStart());
	auto incSize = m_pDx12.GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	for (int i = 0; i < m_ModelData.Materials.size(); ++i) {
		//マテリアル毎の定数バッファビュー
		m_pDx12.GetDevice()->CreateConstantBufferView(&matCBVDesc, matDescHeapH);
		matDescHeapH.ptr += incSize;
		matCBVDesc.BufferLocation += MaterialBuffSize;
		if (m_pTextureResource[i].Get() == nullptr) {
			srvDesc.Format = m_pRenderer.m_pWhiteTex->GetDesc().Format;
			m_pDx12.GetDevice()->CreateShaderResourceView(m_pRenderer.m_pWhiteTex.Get(), &srvDesc, matDescHeapH);
		}
		else {
			srvDesc.Format = m_pTextureResource[i]->GetDesc().Format;
			m_pDx12.GetDevice()->CreateShaderResourceView(m_pTextureResource[i].Get(), &srvDesc, matDescHeapH);
		}
		matDescHeapH.Offset(incSize);

		if (m_pSphResource[i].Get() == nullptr) {
			srvDesc.Format = m_pRenderer.m_pWhiteTex->GetDesc().Format;
			m_pDx12.GetDevice()->CreateShaderResourceView(m_pRenderer.m_pWhiteTex.Get(), &srvDesc, matDescHeapH);
		}
		else {
			srvDesc.Format = m_pSphResource[i]->GetDesc().Format;
			m_pDx12.GetDevice()->CreateShaderResourceView(m_pSphResource[i].Get(), &srvDesc, matDescHeapH);
		}
		matDescHeapH.ptr += incSize;

		if (m_pSpaResource[i].Get() == nullptr) {
			srvDesc.Format = m_pRenderer.m_pBlackTex->GetDesc().Format;
			m_pDx12.GetDevice()->CreateShaderResourceView(m_pRenderer.m_pBlackTex.Get(), &srvDesc, matDescHeapH);
		}
		else {
			srvDesc.Format = m_pSpaResource[i]->GetDesc().Format;
			m_pDx12.GetDevice()->CreateShaderResourceView(m_pSpaResource[i].Get(), &srvDesc, matDescHeapH);
		}
		matDescHeapH.ptr += incSize;

		if (m_pToonResource[i].Get() == nullptr) {
			srvDesc.Format = m_pRenderer.m_pGradTex->GetDesc().Format;
			m_pDx12.GetDevice()->CreateShaderResourceView(m_pRenderer.m_pGradTex.Get(), &srvDesc, matDescHeapH);
		}
		else {
			srvDesc.Format = m_pToonResource[i]->GetDesc().Format;
			m_pDx12.GetDevice()->CreateShaderResourceView(m_pToonResource[i].Get(), &srvDesc, matDescHeapH);
		}
		matDescHeapH.ptr += incSize;
	}
}


void PMDActor::Update() {
	_angle += 0.03f;
	MotionUpdate();
}

void PMDActor::Draw() {
	return;

	m_pDx12.GetCommandList()->IASetVertexBuffers(0, 1, &m_pVertexBufferView);
	m_pDx12.GetCommandList()->IASetIndexBuffer(&m_pIndexBufferView);

	ID3D12DescriptorHeap* TransHeap[] = { m_pTransformHeap.Get()};
	m_pDx12.GetCommandList()->SetDescriptorHeaps(1, TransHeap);
	m_pDx12.GetCommandList()->SetGraphicsRootDescriptorTable(1, m_pTransformHeap->GetGPUDescriptorHandleForHeapStart());

	ID3D12DescriptorHeap* MaterialHeap[] = { m_pMaterialHeap.Get() };
	//マテリアル.
	m_pDx12.GetCommandList()->SetDescriptorHeaps(1, MaterialHeap);

	auto MaterialHeapHandle = m_pMaterialHeap->GetGPUDescriptorHandleForHeapStart();
	unsigned int IdxOffset = 0;

	auto cbvsrvIncSize = m_pDx12.GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) * 5;
	for (const auto& material : m_ModelData.Materials) {
		m_pDx12.GetCommandList()->SetGraphicsRootDescriptorTable(2, MaterialHeapHandle);
		m_pDx12.GetCommandList()->DrawIndexedInstanced(material.NumFaceCount, 1, IdxOffset, 0, 0);
		MaterialHeapHandle.ptr += cbvsrvIncSize;
		IdxOffset += material.NumFaceCount;
	}

}

// アニメーション開始.
void PMDActor::PlayAnimation()
{
	m_StartTime = timeGetTime();
}

void PMDActor::MotionUpdate()
{
	auto elapsedTime = timeGetTime() - m_StartTime;
	unsigned int frameNo = 30 * (elapsedTime / 1000.0f);


	//行列をクリア(しないと前フレームのポーズが重ね掛けされてモデルが崩れる)
	std::fill(m_BoneMatrix.begin(), m_BoneMatrix.end(), DirectX::XMMatrixIdentity());

	//モーションデータ更新
	for (auto& bonemotion : m_MotionData) {
		auto node = m_BoneNodeTable[bonemotion.first];
		//一致するものを探す
		auto keyframes = bonemotion.second;

		auto rit = find_if(keyframes.rbegin(), keyframes.rend(), [frameNo](const KeyFrame& keyframe) {
			return keyframe.FrameNo <= frameNo;
			});
		if (rit == keyframes.rend())continue;//一致するものがなければ飛ばす
		DirectX::XMMATRIX Rotation;
		auto it = rit.base();
		if (it != keyframes.end()) {
			auto t = static_cast<float>(frameNo - rit->FrameNo) /
				static_cast<float>(it->FrameNo - rit->FrameNo);
			t = GetYFromXOnBezier(t, it->p1, it->p2, 12);

			Rotation = DirectX::XMMatrixRotationQuaternion(
				DirectX::XMQuaternionSlerp(rit->Quaternion, it->Quaternion, t)
			);
		}
		else {
			Rotation = DirectX::XMMatrixRotationQuaternion(rit->Quaternion);
		}

		auto& pos = node.StartPos;
		auto mat = DirectX::XMMatrixTranslation(-pos.x, -pos.y, -pos.z) * //原点に戻す
			Rotation * //回転
			DirectX::XMMatrixTranslation(pos.x, pos.y, pos.z);//元の座標に戻す
		m_BoneMatrix[node.BoneIndex] = mat;
	}
	RecursiveMatrixMultipy(&m_BoneNodeTable["センター"], DirectX::XMMatrixIdentity());
	copy(m_BoneMatrix.begin(), m_BoneMatrix.end(), m_MappedMatrices + 1);
}
