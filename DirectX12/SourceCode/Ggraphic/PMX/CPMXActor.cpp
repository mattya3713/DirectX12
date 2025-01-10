#include "CPMXActor.h"
#include "CPMXRenderer.h"
#include "../DirectX/CDirectX12.h"	
#include "Utility/String/FilePath/FilePath.h"
#include <d3dx12.h>
#include <chrono>

// PMXモデルデフォルトの共通トゥーン素材のパス.
static constexpr char c_PMXCommonToonPath[] = "Data\\Model\\PMX\\toon\\toon%02d.bmp";

CPMXActor::CPMXActor(const char* filepath, CPMXRenderer& renderer) 
	: m_pRenderer(renderer)
	, m_pDx12(renderer.m_pDx12)
	, _angle(0.0f)
	, m_VerticesForHLSL	{}
{
	try {
		m_Transform.world = DirectX::XMMatrixIdentity();
		LoadPMXFile(filepath);
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

CPMXActor::~CPMXActor()
{
}

void CPMXActor::Update() {
	_angle += 0.03f;
	//MotionUpdate();
}

void CPMXActor::Draw() {
	m_pDx12.GetCommandList()->IASetVertexBuffers(0, 1, &m_pVertexBufferView);
	m_pDx12.GetCommandList()->IASetIndexBuffer(&m_pIndexBufferView);

	ID3D12DescriptorHeap* TransHeap[] = { m_pTransformHeap.Get() };
	m_pDx12.GetCommandList()->SetDescriptorHeaps(1, TransHeap);
	m_pDx12.GetCommandList()->SetGraphicsRootDescriptorTable(1, m_pTransformHeap->GetGPUDescriptorHandleForHeapStart());

	ID3D12DescriptorHeap* MaterialHeap[] = { m_pMaterialHeap.Get() };
	//マテリアル.
	m_pDx12.GetCommandList()->SetDescriptorHeaps(1, MaterialHeap);

	auto MaterialHeapHandle = m_pMaterialHeap->GetGPUDescriptorHandleForHeapStart();
	unsigned int IdxOffset = 0;

	auto cbvsrvIncSize = m_pDx12.GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) * 5;
	//for (auto& m : m_pMaterials) {
	//	m_pDx12.GetCommandList()->SetGraphicsRootDescriptorTable(2, MaterialHeapHandle);
	//	m_pDx12.GetCommandList()->DrawIndexedInstanced(m->IndicesNum, 1, IdxOffset, 0, 0);
	//	MaterialHeapHandle.ptr += cbvsrvIncSize;
	//	IdxOffset += m->IndicesNum;
	//}
}

// アニメーション開始.
void CPMXActor::PlayAnimation()
{
	//m_StartTime = timeGetTime();
}

void CPMXActor::MotionUpdate()
{
	//auto elapsedTime = timeGetTime() - m_StartTime;
	//unsigned int frameNo = 30 * (elapsedTime / 1000.0f);

	////行列情報クリア(してないと前フレームのポーズが重ね掛けされてモデルが壊れる)
	//std::fill(m_BoneMatrix.begin(), m_BoneMatrix.end(), DirectX::XMMatrixIdentity());

	////モーションデータ更新
	//for (auto& bonemotion : m_MotionData) {
	//	auto node = m_BoneNodeTable[bonemotion.first];
	//	//合致するものを探す
	//	auto keyframes = bonemotion.second;

	//	auto rit = find_if(keyframes.rbegin(), keyframes.rend(), [frameNo](const KeyFrame& keyframe) {
	//		return keyframe.FrameNo <= frameNo;
	//		});
	//	if (rit == keyframes.rend())continue;//合致するものがなければ飛ばす
	//	DirectX::XMMATRIX Rotation;
	//	auto it = rit.base();
	//	if (it != keyframes.end()) {
	//		auto t = static_cast<float>(frameNo - rit->FrameNo) /
	//			static_cast<float>(it->FrameNo - rit->FrameNo);
	//		t = GetYFromXOnBezier(t, it->p1, it->p2, 12);

	//		Rotation = DirectX::XMMatrixRotationQuaternion(
	//			DirectX::XMQuaternionSlerp(rit->Quaternion, it->Quaternion, t)
	//		);
	//	}
	//	else {
	//		Rotation = DirectX::XMMatrixRotationQuaternion(rit->Quaternion);
	//	}

	//	auto& pos = node.StartPos;
	//	auto mat = DirectX::XMMatrixTranslation(-pos.x, -pos.y, -pos.z) * //原点に戻し
	//		Rotation * //回転
	//		DirectX::XMMatrixTranslation(pos.x, pos.y, pos.z);//元の座標に戻す
	//	m_BoneMatrix[node.BoneIndex] = mat;
	//}
	//RecursiveMatrixMultipy(&m_BoneNodeTable["センター"], DirectX::XMMatrixIdentity());
	//copy(m_BoneMatrix.begin(), m_BoneMatrix.end(), m_MappedMatrices + 1);
}

void* CPMXActor::Transform::operator new(size_t size) {
	return _aligned_malloc(size, 16);
}
//
//// 回転情報を末端まで伝播させる再帰関数.
//void CPMXActor::RecursiveMatrixMultipy(
//	BoneNode* node, 
//	const DirectX::XMMATRIX& mat)
//{
//	m_BoneMatrix[node->BoneIndex] = mat;
//	for (auto& cnode : node->Children) {
//		// 子も同じ動作をする.
//		RecursiveMatrixMultipy(cnode, m_BoneMatrix[cnode->BoneIndex] * mat);
//	}
//}

// モデル読み込み.
void CPMXActor::LoadPMXFile(const char* path)
{
	FILE* fp = nullptr;
	auto err = fopen_s(&fp, path, "rb");
	if (err != 0 || !fp) {
		throw std::runtime_error( path + std::string("ファイルを開くことができませんでした。") );
	}
	// ヘッダー情報を読み込む.
	PMX::Header Header;
	ReadPMXHeader(fp, &Header);

	//--------------
	// MEMO : モデル情報の読み込みだが必要なのか.
	//		: 0 == 読み込む.
	//		: 0 != 読み飛ばす.
#if 1

	// モデル情報を読み飛ばす.

	// モデル名(日本).
	uint32_t NameLength = 0;
	fread(&NameLength, sizeof(NameLength), 1, fp);
	fseek(fp, NameLength, SEEK_CUR);

	// モデル名(英語).
	uint32_t NameEnglishLength = 0;
	fread(&NameEnglishLength, sizeof(NameEnglishLength), 1, fp);
	fseek(fp, NameEnglishLength, SEEK_CUR);

	// モデルコメント(日本).
	uint32_t CommentLength = 0;
	fread(&CommentLength, sizeof(CommentLength), 1, fp);
	fseek(fp, CommentLength, SEEK_CUR);

	// モデルコメント(英語).
	uint32_t CommentEnglishLength = 0;
	fread(&CommentEnglishLength, sizeof(CommentEnglishLength), 1, fp);
	fseek(fp, CommentEnglishLength, SEEK_CUR);

#else
	// モデル情報の読み込み.
	PMXModelInfo ModelInfo;

	// モデル名の読み込み.
	uint32_t NameLength = 0;
	fread(&NameLength, sizeof(NameLength), 1, fp);
	std::vector<uint8_t> NameBuffer(NameLength);
	fread(NameBuffer.data(), NameLength, 1, fp);

	// モデル名のエンコーディング処理.
	(this->*PathConverter)(NameBuffer, ModelInfo.ModelName);

	// モデル名英の読み込み.
	uint32_t NameEnglishLength = 0;
	fread(&NameEnglishLength, sizeof(NameEnglishLength), 1, fp);
	std::vector<uint8_t> NameEnglishBuffer(NameEnglishLength);
	fread(NameEnglishBuffer.data(), NameEnglishLength, 1, fp);

	// モデル名英のエンコーディング処理.
	(this->*PathConverter)(NameEnglishBuffer, ModelInfo.ModelNameEnglish);

	// コメントの読み込み.
	uint32_t CommentLength = 0;
	fread(&CommentLength, sizeof(CommentLength), 1, fp);
	std::vector<uint8_t> CommentBuffer(CommentLength);
	fread(CommentBuffer.data(), CommentLength, 1, fp);

	// コメントのエンコーディング処理.
	(this->*PathConverter)(CommentBuffer, ModelInfo.ModelComment);

	// コメント英の読み込み.
	uint32_t CommentEnglishLength = 0;
	fread(&CommentEnglishLength, sizeof(CommentEnglishLength), 1, fp);
	std::vector<uint8_t> CommentEnglishBuffer(CommentEnglishLength);
	fread(CommentEnglishBuffer.data(), CommentEnglishLength, 1, fp);

	// コメント英のエンコーディング処理.
	(this->*PathConverter)(CommentEnglishBuffer, ModelInfo.ModelCommentEnglish);
#endif
	// 処理.

	// 頂点データの読み込み
	std::vector<PMX::Vertex> Vertices;
	uint32_t VerticesNum;
	fread(&VerticesNum, sizeof(VerticesNum), 1, fp);
	Vertices.reserve(VerticesNum);
	m_VerticesForHLSL.resize(VerticesNum);

	// サイズ計算用頂点構造体.
	struct VertexSize {
		DirectX::XMFLOAT3	Position;	// 位置.
		DirectX::XMFLOAT3	Normal;		// 法線.
		DirectX::XMFLOAT2	UV;			// UV座標.
	};

	// 頂点情報の読み込み.
	for (uint32_t i = 0; i < VerticesNum; ++i) {

		// 空の頂点を直接ベクター内で構築.
		Vertices.emplace_back();
		m_VerticesForHLSL.emplace_back();

		// 位置、法線、UV座標の読み込み.
		fread(&Vertices.back(), sizeof(VertexSize), 1, fp);

		// GPU用にワタスデータをコピー.
		m_VerticesForHLSL.back().Position = Vertices.back().Position;
		m_VerticesForHLSL.back().Normal = Vertices.back().Normal;
		m_VerticesForHLSL.back().UV = Vertices.back().UV;

		// 追加UV座標 (最大4つまで).
		fread(Vertices.back().AdditionalUV.data(), sizeof(DirectX::XMFLOAT4) * static_cast<size_t>(Header.AdditionalUV), 1, fp);

		// ボーンウェイト系情報を読み込む.
		uint8_t WeightType;
		fread(&WeightType, sizeof(uint8_t), 1, fp);

		if (20201 == i)
		{
			PMX::Vertex copy = Vertices.back();
		}

		switch (WeightType)
		{
		case 0: // BDEF1.
			fread(&Vertices.back().BoneIndices[0], Header.BoneIndexSize, 1, fp);
			break;
		case 1: // BDEF2.
			fread(&Vertices.back().BoneIndices[0], Header.BoneIndexSize, 1, fp);
			fread(&Vertices.back().BoneIndices[1], Header.BoneIndexSize, 1, fp);
			fread(&Vertices.back().BoneWeights[0], sizeof(float), 1, fp);
			break;
		case 2: // BDEF4.
			fread(&Vertices.back().BoneIndices[0], Header.BoneIndexSize, 1, fp);
			fread(&Vertices.back().BoneIndices[1], Header.BoneIndexSize, 1, fp);
			fread(&Vertices.back().BoneIndices[2], Header.BoneIndexSize, 1, fp);
			fread(&Vertices.back().BoneIndices[3], Header.BoneIndexSize, 1, fp);
			fread(&Vertices.back().BoneWeights[0], sizeof(float), 1, fp);
			fread(&Vertices.back().BoneWeights[1], sizeof(float), 1, fp);
			fread(&Vertices.back().BoneWeights[2], sizeof(float), 1, fp);
			fread(&Vertices.back().BoneWeights[3], sizeof(float), 1, fp);
			break;
		case 3: // SDEF.
			fread(&Vertices.back().BoneIndices[0], Header.BoneIndexSize, 1, fp);
			fread(&Vertices.back().BoneIndices[1], Header.BoneIndexSize, 1, fp);
			fread(&Vertices.back().BoneWeights[0], sizeof(float), 1, fp);
			fread(&Vertices.back().SDEF_C , sizeof(DirectX::XMFLOAT3), 1, fp);
			fread(&Vertices.back().SDEF_R0, sizeof(DirectX::XMFLOAT3), 1, fp);
			fread(&Vertices.back().SDEF_R1, sizeof(DirectX::XMFLOAT3), 1, fp);
			PMX::Vertex copy = Vertices.back();
			break;
		default:
			throw std::invalid_argument("Ido't know this WeightType.");
		}

		// エッジ倍率の読み込み.
		fread(&Vertices.back().Edge, sizeof(float), 1, fp);
	}

	D3D12_HEAP_PROPERTIES HeapProp = {};
	HeapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	HeapProp.Type = D3D12_HEAP_TYPE_UPLOAD;
	HeapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;

	D3D12_RESOURCE_DESC ResDesc = {};
	ResDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	ResDesc.Width = VerticesNum * PMX::GPU_VERTEX_SIZE;
	ResDesc.Height = 1;
	ResDesc.DepthOrArraySize = 1;
	ResDesc.MipLevels = 1;
	ResDesc.Format = DXGI_FORMAT_UNKNOWN;
	ResDesc.SampleDesc.Count = 1;
	ResDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
	ResDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	MyAssert::IsFailed(
		_T("頂点バッファの作成"), 
		&ID3D12Device::CreateCommittedResource, m_pDx12.GetDevice(),
		&HeapProp,
		D3D12_HEAP_FLAG_NONE,
		&ResDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(m_pVertexBuffer.ReleaseAndGetAddressOf()));
			
	MyAssert::IsFailed(
		_T("頂点マップ"), 
		&ID3D12Resource::Map, m_pVertexBuffer.Get(),
		0, nullptr, (void**)&m_pMappedVertex);
		
	std::copy(std::begin(m_VerticesForHLSL), std::end(m_VerticesForHLSL), m_pMappedVertex);
	m_pVertexBuffer->Unmap(0, nullptr);

	m_pVertexBufferView.BufferLocation = m_pVertexBuffer->GetGPUVirtualAddress();
	m_pVertexBufferView.SizeInBytes = PMX::GPU_VERTEX_SIZE * VerticesNum;
	m_pVertexBufferView.StrideInBytes = PMX::GPU_VERTEX_SIZE;

	// インデックスバッファを読み込む.
	std::vector<PMX::Face> Faces;
	uint32_t IndicesNum;
	ReadPMXIndices(fp, &Faces, &IndicesNum);

	// インデックスバッファ用にサイズを変更.
	ResDesc.Width = IndicesNum * sizeof(uint32_t);

	MyAssert::IsFailed(
		_T("インデックスバッファの作成"),
		&ID3D12Device::CreateCommittedResource, m_pDx12.GetDevice(),
		&HeapProp,
		D3D12_HEAP_FLAG_NONE,
		&ResDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(m_pIndexBuffer.ReleaseAndGetAddressOf()));

	MyAssert::IsFailed(
		_T("インデックスをマップする"),
		&ID3D12Resource::Map, m_pIndexBuffer.Get(),
		0, nullptr, (void**)&m_MappedIndex);

	std::copy(std::begin(Faces), std::end(Faces), m_MappedIndex);
	m_pIndexBuffer->Unmap(0, nullptr);

	m_pIndexBufferView.BufferLocation = m_pIndexBuffer->GetGPUVirtualAddress();
	m_pIndexBufferView.Format = DXGI_FORMAT_R32_UINT;
	m_pIndexBufferView.SizeInBytes = sizeof(uint32_t) * IndicesNum;

	auto BuffSize = sizeof(DirectX::XMMATRIX);
	BuffSize = (BuffSize + 0xff) & ~0xff;

	ResDesc.Width = BuffSize;

	MyAssert::IsFailed(
		_T("座標バッファの作成"),
		&ID3D12Device::CreateCommittedResource, m_pDx12.GetDevice(),
		&HeapProp,
		D3D12_HEAP_FLAG_NONE,
		&ResDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(m_pTransformBuffer.ReleaseAndGetAddressOf()));

	MyAssert::IsFailed(
		_T("インデックスをマップする"),
		&ID3D12Resource::Map, m_pTransformBuffer.Get(),
		0, nullptr, (void**)&m_MappedMatrices);

	m_MappedMatrices[0] = DirectX::XMMatrixIdentity();
	m_pTransformBuffer->Unmap(0, nullptr);

	D3D12_DESCRIPTOR_HEAP_DESC TransformDescHeapDesc = {};

	TransformDescHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	TransformDescHeapDesc.NodeMask = 0;
	TransformDescHeapDesc.NumDescriptors = 1;
	TransformDescHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	MyAssert::IsFailed(
		_T("座標のディスクリプタヒープを作成"),
		&ID3D12Device::CreateDescriptorHeap, m_pDx12.GetDevice(),
		&TransformDescHeapDesc, 
		IID_PPV_ARGS(m_pTransformHeap.ReleaseAndGetAddressOf()));

	D3D12_CONSTANT_BUFFER_VIEW_DESC CBVDesc = {};
	CBVDesc.BufferLocation = m_pTransformBuffer->GetGPUVirtualAddress();
	CBVDesc.SizeInBytes = static_cast<UINT>(BuffSize);

	auto Handle = m_pTransformHeap->GetCPUDescriptorHandleForHeapStart();
	auto IncSize = m_pDx12.GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	m_pDx12.GetDevice()->CreateConstantBufferView(&CBVDesc, Handle);

	// テクスチャの読み込み.
	uint32_t TexturesNum;
	fread(&TexturesNum, sizeof(uint32_t), 1, fp);
	std::vector<PMX::TexturePath> TextureInfo(TexturesNum);

	for (uint32_t i = 0; i < TexturesNum; ++i) {
		// 名前読み込み.
		ReadString(fp, TextureInfo[i].Path);

		// exeから見た相対テクスチャパスに変換.
		TextureInfo[i].Path = MyFilePath::GetTexPath(path, TextureInfo[i].Path.c_str());

		// ファイルパスのスラッシュを統一.
		MyFilePath::ReplaceSlashWithBackslash(&TextureInfo[i].Path);
	}

	// マテリアル読み込み.
	uint32_t MaterialNum;
	fread(&MaterialNum, sizeof(MaterialNum), 1, fp);
	std::vector<PMX::Material> Materials; 
	Materials.reserve(MaterialNum);

	// マテリアル用バッファをリサイズ.
	/*m_pMaterials.resize(MaterialNum);
	m_pTextureResource.resize(MaterialNum);
	m_pSphResource.resize(MaterialNum);
	m_pSpaResource.resize(MaterialNum);
	m_pToonResource.resize(MaterialNum);*/

	for (uint32_t i = 0; i < MaterialNum; ++i) {
		Materials.emplace_back();

		// 名前読み込み.
		ReadString(fp, Materials.back().Name);
		ReadString(fp, Materials.back().EnglishName);

		// 固定部分読み取り.
		fread(&Materials.back().Diffuse		, sizeof(DirectX::XMFLOAT4), 1, fp);
		fread(&Materials.back().Specular	, sizeof(DirectX::XMFLOAT3), 1, fp);
		fread(&Materials.back().Specularity	, sizeof(float), 1, fp);
		fread(&Materials.back().Ambient		, sizeof(DirectX::XMFLOAT3), 1, fp);

		// 描画フラグ.
		fread(&Materials.back().DrawMode, sizeof(uint8_t), 1, fp);

		// エッジ色・サイズ.
		fread(&Materials.back().EdgeColor, sizeof(DirectX::XMFLOAT4), 1, fp);
		fread(&Materials.back().EdgeSize, sizeof(float), 1, fp);

		// テクスチャIndex.
		fread(&Materials.back().TextureIndex, Header.TextureIndexSize, 1, fp);

		// スフィアテクスチャIndex.
		fread(&Materials.back().SphereTextureIndex, Header.TextureIndexSize, 1, fp);

		// スフィアモード.
		fread(&Materials.back().SphereMode, sizeof(uint8_t), 1, fp);

		// 共有Toonフラグ.
		fread(&Materials.back().ToonFlag, sizeof(uint8_t), 1, fp);

		if (Materials.back().ToonFlag == 0) {
			fread(&Materials.back().ToonTextureIndex, Header.TextureIndexSize, 1, fp);
		}
		else {
			fread(&Materials.back().ToonTextureIndex, sizeof(uint8_t), 1, fp);
		}

		// メモ.
		ReadString(fp, Materials.back().Memo);

		// 面数.
		fread(&Materials.back().NumFaceCount, sizeof(uint32_t), 1, fp);
		Materials.back().NumFaceCount /= 3;
	}

	// サイズを256アライアンス.
	int MaterialBufferSize = PMX::GPU_MATERIAL_SIZE;
	MaterialBufferSize = (MaterialBufferSize + 0xff) & ~0xff;

	ResDesc.Width = static_cast<UINT>(MaterialBufferSize * MaterialNum);

	MyAssert::IsFailed(
		_T("マテリアルバッファの作成"),
		&ID3D12Device::CreateCommittedResource, m_pDx12.GetDevice(),
		&HeapProp,
		D3D12_HEAP_FLAG_NONE,
		&ResDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(m_pMaterialBuff.ReleaseAndGetAddressOf())
	);

	m_pMaterialBuff->Map(0, nullptr, (void**)&m_pMappedMaterial);

	m_MaterialForHLSL.resize(MaterialNum);

	char* MappedMaterialPtr = m_pMappedMaterial;
	int MaterialIndex = 0;
	for (const auto& Material : Materials)
	{
		//m_MaterialForHLSL[MaterialIndex].visible		= true;
		m_MaterialForHLSL[MaterialIndex].Name			= Material.Name;
		m_MaterialForHLSL[MaterialIndex].Diffuse		= Material.Diffuse;
		m_MaterialForHLSL[MaterialIndex].Specular		= Material.Specular;
		m_MaterialForHLSL[MaterialIndex].Specularity	= Material.Specularity;
		m_MaterialForHLSL[MaterialIndex].Ambient		= Material.Ambient;
		//m_MaterialForHLSL[MaterialIndex].isTransparent	= false;
		MaterialIndex++;

		PMX::MaterialForHLSL* UpLoadMat = reinterpret_cast<PMX::MaterialForHLSL*>(MappedMaterialPtr);
		UpLoadMat->Diffuse = Material.Diffuse;
		UpLoadMat->Specular = Material.Specular;
		UpLoadMat->SpecularPower = Material.Specularity;
		UpLoadMat->Ambient = Material.Ambient;

		MappedMaterialPtr += MaterialBufferSize;
	}

	m_pMaterialBuff->Unmap(0, nullptr);

	/*
	// トゥーンリソースとテクスチャを設定.
	for (int i = 0; i < Materials.size(); ++i) {

		// トゥーンテクスチャのファイルパスを構築.
		char toonFilePath[32];

		// 共通のテクスチャをロード.
		if (Materials[i].ToonFlag) {
			sprintf_s(toonFilePath, c_PMXCommonToonPath, Materials[i].ToonTextureIndex + 1);
			m_pToonResource[i] = m_pDx12.GetTextureByPath(toonFilePath);
		}
		// モデル特有のテクスチャをロード.
		else {

			// リソース数よりテクスチャインデックスが大きかったらcontinue.
			// MEMO : トゥーンを使用してないと255が入ってる.
			if (Materials[i].ToonTextureIndex + 1 >= TextureInfo.size()) { continue; }

			m_pToonResource[i] = m_pDx12.GetTextureByPath(TextureInfo[Materials[i].ToonTextureIndex + 1].Path.c_str());
		}

		// テクスチャパスの分解とリソースのロード.
		std::string TexFileName = TextureInfo[Materials[i].ToonTextureIndex + 1].Path;
		std::string SphFileName = "";
		std::string SpaFileName = "";

		if (count(TexFileName.begin(), TexFileName.end(), '*') > 0) {
			auto namepair = MyFilePath::SplitFileName(TexFileName);
			if (MyFilePath::GetExtension(namepair.first) == "sph") {
				TexFileName = namepair.second;
				SphFileName = namepair.first;
			}
			else if (MyFilePath::GetExtension(namepair.first) == "spa") {
				TexFileName = namepair.second;
				SpaFileName = namepair.first;
			}
			else {
				TexFileName = namepair.first;
				if (MyFilePath::GetExtension(namepair.second) == "sph") {
					SphFileName = namepair.second;
				}
				else if (MyFilePath::GetExtension(namepair.second) == "spa") {
					SpaFileName = namepair.second;
				}
			}
		}
		else {
			if (MyFilePath::GetExtension(TexFileName) == "sph") {
				SphFileName = TexFileName;
				TexFileName = "";
			}
			else if (MyFilePath::GetExtension(TexFileName) == "spa") {
				SpaFileName = TexFileName;
				TexFileName = "";
			}
			else {
				TexFileName = TexFileName;
			}
		}

		// リソースをロード.
		if (!TexFileName.empty()) {
			m_pTextureResource[i] = m_pDx12.GetTextureByPath(TexFileName.c_str());
		}
		if (!SphFileName.empty()) {
			auto sphFilePath = MyFilePath::GetTexPath(path, SphFileName.c_str());
			m_pSphResource[i] = m_pDx12.GetTextureByPath(sphFilePath.c_str());
		}
		if (!SpaFileName.empty()) {
			auto spaFilePath = MyFilePath::GetTexPath(path, SpaFileName.c_str());
			m_pSpaResource[i] = m_pDx12.GetTextureByPath(spaFilePath.c_str());
		}
	}*/

	// ボーンの読み込み.
	uint32_t BoneNum;
	fread(&BoneNum, sizeof(BoneNum), 1, fp);
	std::vector<PMX::Bone> Bones(BoneNum);

	for (uint32_t i = 0; i < BoneNum; ++i)
	{
		Bones.emplace_back();

		// 名前読み込み.
		ReadString(fp, Bones.back().Name);
		ReadString(fp, Bones.back().EnglishName);

		// ボーンの位置情報を読み込む (スフィアモード)
		fread(&Bones.back().Position		, sizeof(DirectX::XMFLOAT3), 1, fp);
		fread(&Bones.back().ParentBoneIndex	, Header.BoneIndexSize, 1, fp);
		fread(&Bones.back().DeformDepth		, sizeof(uint32_t), 1, fp);
		fread(&Bones.back().BoneFlag		, sizeof(uint16_t), 1, fp);

		// TargetShowModeが無効の場合、位置オフセットを読み込む.
		if ((Bones.back().BoneFlag & PMX::BoneFlags::TargetShowMode) == 0) {
			// 位置オフセット.
			fread(&Bones.back().PositionOffset, sizeof(DirectX::XMFLOAT3), 1, fp);
		}
		else {
			// リンクボーンインデックス.
			fread(&Bones.back().LinkBoneIndex, Header.BoneIndexSize, 1, fp);
		}

		// ボーンが回転または移動を補間する場合.
		if ((Bones.back().BoneFlag & PMX::BoneFlags::AppendRotate) ||
			(Bones.back().BoneFlag & PMX::BoneFlags::AppendTranslate)) {
			fread(&Bones.back().AppendBoneIndex	, Header.BoneIndexSize, 1, fp);
			fread(&Bones.back().AppendWeight	, sizeof(float), 1, fp);
		}

		// 固定軸が有効な場合.
		if (Bones.back().BoneFlag & PMX::BoneFlags::FixedAxis) {
			fread(&Bones.back().FixedAxis, sizeof(DirectX::XMFLOAT3), 1, fp);
		}

		// ローカル軸が有効な場合.
		if (Bones.back().BoneFlag & PMX::BoneFlags::LocalAxis) {
			fread(&Bones.back().LocalXAxis, sizeof(DirectX::XMFLOAT3), 1, fp);
			fread(&Bones.back().LocalZAxis, sizeof(DirectX::XMFLOAT3), 1, fp);
		}

		// 親ボーンの変形が外部によって制限される場合.
		if (Bones.back().BoneFlag & PMX::BoneFlags::DeformOuterParent) {
			fread(&Bones.back().KeyValue, sizeof(uint32_t), 1, fp);
		}

		// IKが有効な場合.
		if (Bones.back().BoneFlag & PMX::BoneFlags::IK) {
			fread(&Bones.back().IKTargetBoneIndex, Header.BoneIndexSize, 1, fp);
			fread(&Bones.back().IKIterationCount , sizeof(uint32_t), 1, fp);
			fread(&Bones.back().IKLimit			 , sizeof(float), 1, fp);

			// IKリンクの数を読み込み.
			uint32_t LinkCount = 0;
			fread(&LinkCount, sizeof(uint32_t), 1, fp);

			// IKリンクを読み込む.
			Bones.back().IKLinks.resize(LinkCount);
			for (auto& IkLink : Bones.back().IKLinks) {
				fread(&IkLink.IKBoneIndex, Header.BoneIndexSize, 1, fp);
				fread(&IkLink.EnableLimit, sizeof(uint8_t), 1, fp);

				// 制限が有効な場合、制限範囲を読み込む.
				if (IkLink.EnableLimit != 0) {
					fread(&IkLink.LimitMin, sizeof(DirectX::XMFLOAT3), 1, fp);
					fread(&IkLink.LimitMax, sizeof(DirectX::XMFLOAT3), 1, fp);
				}
			}
		}
	}


	// もーふの読み込み.
	uint32_t MorphNum;
	fread(&MorphNum, sizeof(MorphNum), 1, fp);
	std::vector<PMX::Morph> Morphs(MorphNum);

	for (uint32_t i = 0; i < MorphNum; ++i)
	{
		Morphs.emplace_back();

		// 名前読み込み.
		ReadString(fp, Morphs.back().Name);
		ReadString(fp, Morphs.back().EnglishName);

		fread(&Morphs.back().ControlPanel, sizeof(Morphs.back().ControlPanel), 1, fp);
		fread(&Morphs.back().MorphType, sizeof(Morphs.back().MorphType), 1, fp);

		unsigned int DataCount;
		fread(&DataCount, sizeof(DataCount), 1, fp);

		// モーフタイプによって読み込む情報を変更.
		switch (Morphs.back().MorphType)
		{
		case PMX::MorphType::Position:	// 座標モーフの場合.
			Morphs.back().PositionMorphs.resize(DataCount);
			for (auto& MorphData : Morphs.back().PositionMorphs)
			{
				fread(&MorphData.VertexIndex, Header.VertexIndexSize, 1, fp);
				fread(&MorphData.Position, sizeof(MorphData.Position), 1, fp);
			}
			break;
		case PMX::MorphType::UV:		// UVモーフの場合.
		case PMX::MorphType::AddUV1:	
		case PMX::MorphType::AddUV2:
		case PMX::MorphType::AddUV3:
		case PMX::MorphType::AddUV4:	
			Morphs.back().UVMorphs.resize(DataCount);
			for (auto& MorphData : Morphs.back().UVMorphs)
			{
				fread(&MorphData.VertexIndex, Header.VertexIndexSize, 1, fp);
				fread(&MorphData.UV			, sizeof(MorphData.UV), 1, fp);
			}
			break;
		case PMX::MorphType::Bone: // ボーンモーフの場合.
			Morphs.back().BoneMorphs.resize(DataCount);
			for (auto& MorphData : Morphs.back().BoneMorphs)
			{
				fread(&MorphData.BoneIndex	, Header.BoneIndexSize, 1, fp);
				fread(&MorphData.Position	, sizeof(MorphData.Position), 1, fp);
				fread(&MorphData.Quaternion	, sizeof(MorphData.Quaternion), 1, fp);
			}
			break;
		case PMX::MorphType::Material: // マテリアルモーフの場合.
			Morphs.back().MaterialMorphs.resize(DataCount);
			for (auto& MorphData : Morphs.back().MaterialMorphs)
			{
				fread(&MorphData.MaterialIndex		, Header.MaterialIndexSize, 1, fp);
				fread(&MorphData.OpType				, sizeof(MorphData.OpType), 1, fp);
				fread(&MorphData.Diffuse			, sizeof(MorphData.Diffuse), 1, fp);
				fread(&MorphData.Specular			, sizeof(MorphData.Specular), 1, fp);
				fread(&MorphData.SpecularPower		, sizeof(MorphData.SpecularPower), 1, fp);
				fread(&MorphData.Ambient			, sizeof(MorphData.Ambient), 1, fp);
				fread(&MorphData.EdgeColor			, sizeof(MorphData.EdgeColor), 1, fp);
				fread(&MorphData.EdgeSize			, sizeof(MorphData.EdgeSize), 1, fp);
				fread(&MorphData.TextureFactor		, sizeof(MorphData.TextureFactor), 1, fp);
				fread(&MorphData.SphereTextureFactor, sizeof(MorphData.SphereTextureFactor), 1, fp);
				fread(&MorphData.ToonTextureFactor	, sizeof(MorphData.ToonTextureFactor), 1, fp);
			}
			break;
		case PMX::MorphType::Group: // グループモーフの場合.			
			Morphs.back().GroupMorphs.resize(DataCount);
			for (auto& MorphData : Morphs.back().GroupMorphs)
			{
				fread(&MorphData.MorphIndex	, Header.MorphIndexSize, 1, fp);
				fread(&MorphData.Weight		, sizeof(MorphData.Weight), 1, fp);
			}
			break;

		case PMX::MorphType::Flip: // フリップモーフの場合.			
			Morphs.back().FlipMorphs.resize(DataCount);
			for (auto& MorphData : Morphs.back().FlipMorphs)
			{
				fread(&MorphData.MorphIndex	, Header.MorphIndexSize, 1, fp);
				fread(&MorphData.Weight		, sizeof(MorphData.Weight), 1, fp);
			}
			break;

		case PMX::MorphType::Impluse: // インパルスモーフの場合.			
			Morphs.back().ImpulseMorphs.resize(DataCount);
			for (auto& MorphData : Morphs.back().ImpulseMorphs)
			{
				fread(&MorphData.RigidBodyIndex		, Header.RigidBodyIndexSize, 1, fp);
				fread(&MorphData.LocalFlag			, sizeof(MorphData.LocalFlag), 1, fp);
				fread(&MorphData.TranslateVelocity	, sizeof(MorphData.TranslateVelocity), 1, fp);
				fread(&MorphData.RotateTorque		, sizeof(MorphData.RotateTorque), 1, fp);
			}
			break;
		default:
			throw std::runtime_error("Ido't know this MorphType.");
			break;
		}
	}

	// ファイルを閉じる.
	fclose(fp);
}

// インデックスを読み込む(1Byte).
void CPMXActor::ReadPMXIndices1Byte(FILE* fp, const uint32_t& IndicesNum, std::vector<PMX::Face>* Faces)
{
	std::vector<uint8_t> TempIndices(IndicesNum);
	fread(TempIndices.data(), sizeof(uint8_t), IndicesNum, fp);

	Faces->resize(IndicesNum / 3);
	for (size_t i = 0; i < Faces->size(); ++i)
	{
		(*Faces)[i].Index[0] = static_cast<uint32_t>(TempIndices[i * 3 + 0]);
		(*Faces)[i].Index[1] = static_cast<uint32_t>(TempIndices[i * 3 + 1]);
		(*Faces)[i].Index[2] = static_cast<uint32_t>(TempIndices[i * 3 + 2]);
	}
}

// インデックスを読み込む(2Byte).
void CPMXActor::ReadPMXIndices2Byte(FILE* fp, const uint32_t& IndicesNum, std::vector<PMX::Face>* Faces)
{
	std::vector<uint16_t> TempIndices(IndicesNum);
	fread(TempIndices.data(), sizeof(uint16_t), IndicesNum, fp);

	Faces->resize(IndicesNum / 3);
	for (size_t i = 0; i < Faces->size(); ++i)
	{
		(*Faces)[i].Index[0] = static_cast<uint32_t>(TempIndices[i * 3 + 0]);
		(*Faces)[i].Index[1] = static_cast<uint32_t>(TempIndices[i * 3 + 1]);
		(*Faces)[i].Index[2] = static_cast<uint32_t>(TempIndices[i * 3 + 2]);
	}
}

// インデックスを読み込む(4Byte).
void CPMXActor::ReadPMXIndices4Byte(FILE* fp, const uint32_t& IndicesNum, std::vector<PMX::Face>* Faces)
{
	std::vector<uint32_t> TempIndices(IndicesNum);
	fread(TempIndices.data(), sizeof(uint32_t), IndicesNum, fp);

	Faces->resize(IndicesNum / 3);
	for (size_t i = 0; i < Faces->size(); ++i)
	{
		(*Faces)[i].Index[0] = TempIndices[i * 3 + 0];
		(*Faces)[i].Index[1] = TempIndices[i * 3 + 1];
		(*Faces)[i].Index[2] = TempIndices[i * 3 + 2];
	}
}

// PMXヘッター読み込み.
void CPMXActor::ReadPMXHeader(FILE* fp, PMX::Header* Header)
{
	// ヘッダー情報を全て読み込む.
	fread(Header, PMX::HEADER_SIZE, 1, fp);

	// PMXファイルか同課の判断.
	if (Header->Signature != PMX::SIGNATURE) { throw std::runtime_error("This File is not PMx."); }

	// エンコードに基づいて関数ポインタを選択.
	PathConverter = nullptr;
	if (Header->Encoding) {
		// エンコードがUTF-8なのでそのまま.
		PathConverter = &CPMXActor::ConvertUTF8;
	}
	else {
		// エンコードがUTF-16なのでUTF-8へ.
		PathConverter = &CPMXActor::ConvertUTF16ToUTF8;
	}

	// インデックスサイズに基づいて関数ポインタを選択.
	ReadIndices = nullptr;
	switch (Header->VertexIndexSize)
	{
	case 1: // インデックスが1バイト.
		ReadIndices = &CPMXActor::ReadPMXIndices1Byte;
		break;
	case 2: // インデックスが2バイト.
		ReadIndices = &CPMXActor::ReadPMXIndices2Byte;
		break;
	case 4: // インデックスが4バイト.
		ReadIndices = &CPMXActor::ReadPMXIndices4Byte;
		break;
	default:
		throw std::runtime_error("Unsupported VertexIndexSize in PMX header.");
	}
}

// 文字列を読み込む.
void CPMXActor::ReadString(FILE* fp, std::string& Name)
{
	// 名前読み込み.
	uint32_t NameLength = 0;
	fread(&NameLength, sizeof(NameLength), 1, fp);
	std::vector<uint8_t> NameBuffer(NameLength);
	fread(NameBuffer.data(), NameLength, 1, fp);

	// パスエンコーディング処理.
	(this->*PathConverter)(NameBuffer, Name);
}

// UTF-16 から UTF-8 への変換.
void CPMXActor::ConvertUTF16ToUTF8(const std::vector<uint8_t>& buffer, std::string& OutString)
{
	std::u16string utf16String(reinterpret_cast<const char16_t*>(buffer.data()), buffer.size() / 2);
	OutString = MyString::UTF16ToUTF8(utf16String);
}

// UTF-8 の処理.
void CPMXActor::ConvertUTF8(const std::vector<uint8_t>& buffer, std::string& OutString)
{
	// UTF-8バイト列からstd::stringを直接生成.
	OutString.assign(buffer.begin(), buffer.end());
}

// PMXバイナリからインデックス数とインデックスを読み込む.
void CPMXActor::ReadPMXIndices(FILE* fp, std::vector<PMX::Face>* Faces, uint32_t* IndicesNum)
{
	// インデックス数を読み込む.
	fread(IndicesNum, sizeof(uint32_t), 1, fp);

#if _DEBUG
	// インデックスサイズに基づき読み込む.
	if (ReadIndices) {
		(this->*ReadIndices)(fp, *IndicesNum, Faces);
	}
	else {
		throw std::runtime_error("ReadIndicesFunctionPointer is not initialized.");
	}
#else
	// インデックスサイズに基づき読み込む.
	(this->*ReadIndices)(fp, *IndicesNum, Faces);
#endif
}

void CPMXActor::LoadVMDFile(const char* FilePath, const char* Name)
{
	//FILE* fp = nullptr;
	//// fopen_sを使ってファイルをバイナリモードで開く.
	//auto err = fopen_s(&fp, FilePath, "rb");
	//if (err != 0 || !fp) {
	//	throw std::runtime_error("ファイルを開くことができませんでした。");
	//}
	//fseek(fp, 50, SEEK_SET);//最初の50バイトは飛ばしてOK
	//unsigned int keyframeNum = 0;
	//fread(&keyframeNum, sizeof(keyframeNum), 1, fp);

	//std::vector<VMDKeyFrame> Keyframes(keyframeNum);
	//for (auto& keyframe : Keyframes) {
	//	fread(keyframe.BoneName, sizeof(keyframe.BoneName), 1, fp);	// ボーン名.
	//	fread(&keyframe.FrameNo, sizeof(keyframe.FrameNo) +			// フレーム番号.
	//		sizeof(keyframe.Location) +								// 位置(IKのときに使用予定).
	//		sizeof(keyframe.Quaternion) +							// クオータニオン.
	//		sizeof(keyframe.Bezier), 1, fp);						// 補間ベジェデータ.
	//}

	//for (auto& motion : m_MotionData) {
	//	std::sort(motion.second.begin(), motion.second.end(),
	//		[](const KeyFrame& lval, const KeyFrame& rval) {
	//			return lval.FrameNo <= rval.FrameNo;
	//		});
	//}

	////VMDのキーフレームデータから、実際に使用するキーフレームテーブルへ変換.
	//for (auto& f : Keyframes) {
	//	m_MotionData[f.BoneName].emplace_back(
	//		KeyFrame(
	//			f.FrameNo,
	//			DirectX::XMLoadFloat4(&f.Quaternion),
	//			DirectX::XMFLOAT2((float)f.Bezier[3] / 127.0f, (float)f.Bezier[7] / 127.0f),
	//			DirectX::XMFLOAT2((float)f.Bezier[11] / 127.0f, (float)f.Bezier[15] / 127.0f)
	//		));
	//}

	//for (auto& bonemotion : m_MotionData) {
	//	auto node = m_BoneNodeTable[bonemotion.first];
	//	auto& pos = node.StartPos;
	//	auto mat = DirectX::XMMatrixTranslation(-pos.x, -pos.y, -pos.z) *
	//		DirectX::XMMatrixRotationQuaternion(bonemotion.second[0].Quaternion) *
	//		DirectX::XMMatrixTranslation(pos.x, pos.y, pos.z);
	//	m_BoneMatrix[node.BoneIndex] = mat;
	//}

	//RecursiveMatrixMultipy(&m_BoneNodeTable["センター"], DirectX::XMMatrixIdentity());
	//std::copy(m_BoneMatrix.begin(), m_BoneMatrix.end(), m_MappedMatrices + 1);

}

void CPMXActor::CreateTransformView() {
	////GPUバッファ作成
	//auto buffSize = sizeof(Transform) * (1 + m_BoneMatrix.size());
	//buffSize = (buffSize + 0xff)&~0xff;
	//auto heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	//auto resDesc = CD3DX12_RESOURCE_DESC::Buffer(buffSize);

	//MyAssert::IsFailed(
	//	_T("座標バッファ作成"),
	//	&ID3D12Device::CreateCommittedResource, m_pDx12.GetDevice(),
	//	&heapProp,
	//	D3D12_HEAP_FLAG_NONE,
	//	&resDesc,
	//	D3D12_RESOURCE_STATE_GENERIC_READ,
	//	nullptr,
	//	IID_PPV_ARGS(m_pTransformBuff.ReleaseAndGetAddressOf())
	//);

	////マップとコピー
	//MyAssert::IsFailed(
	//	_T("座標のマップ"),
	//	&ID3D12Resource::Map, m_pTransformBuff.Get(),
	//	0, nullptr, 
	//	(void**)&m_MappedMatrices);

	//m_MappedMatrices[0] = m_Transform.world;
	//copy(m_BoneMatrix.begin(), m_BoneMatrix.end(), m_MappedMatrices + 1);

	//// ビューの作成.
	//D3D12_DESCRIPTOR_HEAP_DESC transformDescHeapDesc = {};
	//transformDescHeapDesc.NumDescriptors = 1; // とりあえずワールドひとつ.
	//transformDescHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	//transformDescHeapDesc.NodeMask = 0;

	//transformDescHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV; // デスクリプタヒープ種別.

	//MyAssert::IsFailed(
	//	_T("座標ヒープの作成"),
	//	&ID3D12Device::CreateDescriptorHeap, m_pDx12.GetDevice(),
	//	&transformDescHeapDesc, 
	//	IID_PPV_ARGS(m_pTransformHeap.ReleaseAndGetAddressOf()));//生成

	//D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	//cbvDesc.BufferLocation = m_pTransformBuff->GetGPUVirtualAddress();
	//cbvDesc.SizeInBytes = static_cast<UINT>(buffSize);
	//m_pDx12.GetDevice()->CreateConstantBufferView(
	//	&cbvDesc,
	//	m_pTransformHeap->GetCPUDescriptorHandleForHeapStart());

}

void CPMXActor::CreateMaterialData() {
	////マテリアルバッファを作成
	//auto MaterialBuffSize = sizeof(MaterialForHlsl);
	//MaterialBuffSize = (MaterialBuffSize + 0xff)&~0xff;

	//auto heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	//auto resDesc = CD3DX12_RESOURCE_DESC::Buffer(MaterialBuffSize * m_pMaterials.size());

	//MyAssert::IsFailed(
	//	_T("マテリアル作成"),
	//	&ID3D12Device::CreateCommittedResource, m_pDx12.GetDevice(),
	//	&heapProp,
	//	D3D12_HEAP_FLAG_NONE,
	//	&resDesc,//勿体ないけど仕方ないですね
	//	D3D12_RESOURCE_STATE_GENERIC_READ,
	//	nullptr,
	//	IID_PPV_ARGS(m_pMaterialBuff.ReleaseAndGetAddressOf()));

	////マップマテリアルにコピー
	//char* mapMaterial = nullptr;

	//MyAssert::IsFailed(
	//	_T("マテリアルマップにコピー"),
	//	&ID3D12Resource::Map, m_pMaterialBuff.Get(),
	//	0, nullptr, 
	//	(void**)&mapMaterial);

	//for (auto& m : m_pMaterials) {
	//	*((MaterialForHlsl*)mapMaterial) = m->Materialhlsl;//データコピー
	//	mapMaterial += MaterialBuffSize;//次のアライメント位置まで進める
	//}

	//m_pMaterialBuff->Unmap(0, nullptr);

	//// -- 仮.

	//m_MappedMatrices[0] = DirectX::XMMatrixRotationY(_angle);

	//auto armnode = m_BoneNodeTable["左腕"];
	//auto& armpos = armnode.StartPos;
	//auto armMat =
	//	DirectX::XMMatrixTranslation(-armpos.x, -armpos.y, -armpos.x)
	//	* DirectX::XMMatrixRotationZ(DirectX::XM_PIDIV2)
	//	* DirectX::XMMatrixTranslation(armpos.x, armpos.y, armpos.x);
	//
	//auto elbowNode = m_BoneNodeTable["左ひじ"];
	//auto& elbowpos = elbowNode.StartPos;
	//auto elbowMat = DirectX::XMMatrixTranslation(-elbowpos.x, -elbowpos.y, -elbowpos.x)
	//	* DirectX::XMMatrixRotationZ(-DirectX::XM_PIDIV2)
	//	* DirectX::XMMatrixTranslation(elbowpos.x, elbowpos.y, elbowpos.x);

	//m_BoneMatrix[armnode.BoneIndex] = armMat;
	//m_BoneMatrix[elbowNode.BoneIndex] = elbowMat;

	//RecursiveMatrixMultipy(&m_BoneNodeTable ["センター"], DirectX::XMMatrixIdentity());


	//copy(m_BoneMatrix.begin(), m_BoneMatrix.end(), m_MappedMatrices + 1);
	//// -- 仮.
}


void CPMXActor::CreateMaterialAndTextureView() {
	//D3D12_DESCRIPTOR_HEAP_DESC MaterialDescHeapDesc = {};
	//MaterialDescHeapDesc.NumDescriptors = static_cast<UINT>(m_pMaterials.size() * 5);//マテリアル数ぶん(定数1つ、テクスチャ3つ)
	//MaterialDescHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	//MaterialDescHeapDesc.NodeMask = 0;

	//MaterialDescHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;//デスクリプタヒープ種別

	//MyAssert::IsFailed(
	//	_T("マテリアルヒープの作成"),
	//	&ID3D12Device::CreateDescriptorHeap, m_pDx12.GetDevice(),
	//	&MaterialDescHeapDesc,
	//	IID_PPV_ARGS(m_pMaterialHeap.ReleaseAndGetAddressOf()));

	//auto MaterialBuffSize = sizeof(MaterialForHlsl);
	//MaterialBuffSize = (MaterialBuffSize + 0xff)&~0xff;
	//D3D12_CONSTANT_BUFFER_VIEW_DESC matCBVDesc = {};
	//matCBVDesc.BufferLocation = m_pMaterialBuff->GetGPUVirtualAddress();
	//matCBVDesc.SizeInBytes = static_cast<UINT>(MaterialBuffSize);
	//
	//D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	//srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;//後述
	//srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;//2Dテクスチャ
	//srvDesc.Texture2D.MipLevels = 1;//ミップマップは使用しないので1
	//CD3DX12_CPU_DESCRIPTOR_HANDLE matDescHeapH(m_pMaterialHeap->GetCPUDescriptorHandleForHeapStart());
	//auto incSize = m_pDx12.GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	//for (int i = 0; i < m_pMaterials.size(); ++i) {
	//	//マテリアル固定バッファビュー
	//	m_pDx12.GetDevice()->CreateConstantBufferView(&matCBVDesc, matDescHeapH);
	//	matDescHeapH.ptr += incSize;
	//	matCBVDesc.BufferLocation += MaterialBuffSize;
	//	if (m_pTextureResource[i].Get() == nullptr) {
	//		srvDesc.Format = m_pRenderer.m_pWhiteTex->GetDesc().Format;
	//		m_pDx12.GetDevice()->CreateShaderResourceView(m_pRenderer.m_pWhiteTex.Get(), &srvDesc, matDescHeapH);
	//	}
	//	else {
	//		srvDesc.Format = m_pTextureResource[i]->GetDesc().Format;
	//		m_pDx12.GetDevice()->CreateShaderResourceView(m_pTextureResource[i].Get(), &srvDesc, matDescHeapH);
	//	}
	//	matDescHeapH.Offset(incSize);

	//	if (m_pSphResource[i].Get() == nullptr) {
	//		srvDesc.Format = m_pRenderer.m_pWhiteTex->GetDesc().Format;
	//		m_pDx12.GetDevice()->CreateShaderResourceView(m_pRenderer.m_pWhiteTex.Get(), &srvDesc, matDescHeapH);
	//	}
	//	else {
	//		srvDesc.Format = m_pSphResource[i]->GetDesc().Format;
	//		m_pDx12.GetDevice()->CreateShaderResourceView(m_pSphResource[i].Get(), &srvDesc, matDescHeapH);
	//	}
	//	matDescHeapH.ptr += incSize;

	//	if (m_pSpaResource[i].Get() == nullptr) {
	//		srvDesc.Format = m_pRenderer.m_pBlackTex->GetDesc().Format;
	//		m_pDx12.GetDevice()->CreateShaderResourceView(m_pRenderer.m_pBlackTex.Get(), &srvDesc, matDescHeapH);
	//	}
	//	else {
	//		srvDesc.Format = m_pSpaResource[i]->GetDesc().Format;
	//		m_pDx12.GetDevice()->CreateShaderResourceView(m_pSpaResource[i].Get(), &srvDesc, matDescHeapH);
	//	}
	//	matDescHeapH.ptr += incSize;


	//	if (m_pToonResource[i].Get() == nullptr) {
	//		srvDesc.Format = m_pRenderer.m_pGradTex->GetDesc().Format;
	//		m_pDx12.GetDevice()->CreateShaderResourceView(m_pRenderer.m_pGradTex.Get(), &srvDesc, matDescHeapH);
	//	}
	//	else {
	//		srvDesc.Format = m_pToonResource[i]->GetDesc().Format;
	//		m_pDx12.GetDevice()->CreateShaderResourceView(m_pToonResource[i].Get(), &srvDesc, matDescHeapH);
	//	}
	//	matDescHeapH.ptr += incSize;
	//}
}

float CPMXActor::GetYFromXOnBezier(
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

// 頂点の総数を読み込む.
uint32_t CPMXActor::ReadAndCastIndices(FILE* fp, uint8_t indexSize)
{
	uint32_t IndexNum = 0;

	// indexSize分のデータを読み込む.
	if (fread(&IndexNum, static_cast<size_t>(indexSize), 1, fp) != 1) {
		throw std::runtime_error("Failed to read 4 bytes from file.");
	}

	// 取り出すデータの結果を格納する変数.
	uint32_t result = 0;

	switch (indexSize) {
	case 1: {
		// 最初の1バイトだけを取り出す.
		uint8_t firstByte = static_cast<uint8_t>(IndexNum & 0xFF);
		result = static_cast<uint32_t>(firstByte);
		break;
	}
	case 2: {
		// 最初の2バイトだけを取り出す.
		uint16_t firstTwoBytes = static_cast<uint16_t>(IndexNum & 0xFFFF);
		result = static_cast<uint32_t>(firstTwoBytes);
		break;
	}
	case 4: {
		// 全4バイトをそのまま使用.
		result = IndexNum;
		break;
	}
	default:
		throw std::invalid_argument("Ido't know this index size.");
	}

	return result;
}

