#include "CPMXActor.h"
#include "CPMXRenderer.h"
#include "../DirectX/CDirectX12.h"	
#include "Utility/String/FilePath/FilePath.h"
#include <d3dx12.h>
#include <chrono>

// PMXモデルデフォルトの共通トゥーン素材のパス.
static constexpr char c_PMXCommonToonPath[] = "Data\\Model\\PMX\\toon\\toon%02d.bmp";

void* CPMXActor::Transform::operator new(size_t size) {
	return _aligned_malloc(size, 16);
}

// 回転情報を末端まで伝播させる再帰関数.
void CPMXActor::RecursiveMatrixMultipy(
	BoneNode* node, 
	const DirectX::XMMATRIX& mat)
{
	m_BoneMatrix[node->BoneIndex] = mat;
	for (auto& cnode : node->Children) {
		// 子も同じ動作をする.
		RecursiveMatrixMultipy(cnode, m_BoneMatrix[cnode->BoneIndex] * mat);
	}
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
uint32_t CPMXActor::ReadIndicesNum(FILE* fp, uint8_t indexSize)
{
    uint32_t IndexNum = 0;

    // 常に4バイト分のデータを読み込む.
    if (fread(&IndexNum, sizeof(IndexNum), 1, fp) != 1) {
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
        throw std::invalid_argument("Unsupported index size.");
    }

    return result;
}

CPMXActor::CPMXActor(const char* filepath, CPMXRenderer& renderer):
	m_pRenderer(renderer),
	m_pDx12(renderer.m_pDx12),
	_angle(0.0f)
{
	try {
		// ワールド行列を初期化
		m_Transform.world = DirectX::XMMatrixIdentity();

		// Z 軸方向に少し奥に移動（例: Z = -5.0f）
		DirectX::XMMATRIX translation = DirectX::XMMatrixTranslation(0.0f, 0.0f, -5.0f);

		// 平行移動を適用
		m_Transform.world = translation;

		LoadPMDFile(filepath);
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


void CPMXActor::LoadPMDFile(const char* path)
{
	FILE* fp = nullptr;
	auto err = fopen_s(&fp, path, "rb");
	if (err != 0 || !fp) {
		throw std::runtime_error("ファイルを開くことができませんでした。");
	}

	// PMXのシグネチャとバージョンを読み取る
	char Signature[4];
	fread(Signature, sizeof(Signature), 1, fp);

	// ヘッダー情報を読み込む.
	PMXHeader Header;
	fread(&Header.Version, sizeof(Header.Version), 1, fp);

	// 後続データ列のサイズ(PMX 2.0の場合は8).
	uint8_t nextDataSize;
	fread(&nextDataSize, sizeof(nextDataSize), 1, fp);

	fread(&Header.Encoding, sizeof(Header.Encoding), 1, fp);
	fread(&Header.AdditionalUV, sizeof(Header.AdditionalUV), 1, fp);
	fread(&Header.VertexIndexSize, sizeof(Header.VertexIndexSize), 1, fp);
	fread(&Header.TextureIndexSize, sizeof(Header.TextureIndexSize), 1, fp);
	fread(&Header.MaterialIndexSize, sizeof(Header.MaterialIndexSize), 1, fp);
	fread(&Header.BoneIndexSize, sizeof(Header.BoneIndexSize), 1, fp);
	fread(&Header.MorphIndexSize, sizeof(Header.MorphIndexSize), 1, fp);
	fread(&Header.RigidBodyIndexSize, sizeof(Header.RigidBodyIndexSize), 1, fp);

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
	std::vector<char> NameBuffer(NameLength);
	fread(NameBuffer.data(), NameLength, 1, fp);

	// モデル名のエンコーディング処理.
	if (Header.Encoding == 0) {
		// UTF-16.
		std::u16string UTF16ModelName(reinterpret_cast<char16_t*>(NameBuffer.data()), NameLength / 2);
		// UTF-16からUTF-8へ変換.
		ModelInfo.ModelName = MyString::UTF16ToUTF8(UTF16ModelName);
	}
	else {
		// UTF-8.
		ModelInfo.ModelName = std::string(NameBuffer.data(), NameLength);
	}

	// モデル名英の読み込み.
	uint32_t NameEnglishLength = 0;
	fread(&NameEnglishLength, sizeof(NameEnglishLength), 1, fp);
	std::vector<char> NameEnglishBuffer(NameEnglishLength);
	fread(NameEnglishBuffer.data(), NameEnglishLength, 1, fp);

	// モデル名英のエンコーディング処理.
	if (Header.Encoding == 0) {
		// UTF-16.
		std::u16string UTF16NameEnglish(reinterpret_cast<char16_t*>(NameEnglishBuffer.data()), NameEnglishLength / 2);
		// UTF-16からUTF-8へ変換.
		ModelInfo.ModelNameEnglish = MyString::UTF16ToUTF8(UTF16NameEnglish);
	}
	else {
		// UTF-8.
		ModelInfo.ModelNameEnglish = std::string(NameEnglishBuffer.data(), NameEnglishLength);
	}

	// コメントの読み込み.
	uint32_t CommentLength = 0;
	fread(&CommentLength, sizeof(CommentLength), 1, fp);
	std::vector<char> CommentBuffer(CommentLength);
	fread(CommentBuffer.data(), CommentLength, 1, fp);

	// コメントのエンコーディング処理.
	if (Header.Encoding == 0) {
		// UTF-16.
		std::u16string UTF16ModelComment(reinterpret_cast<char16_t*>(CommentBuffer.data()), CommentLength / 2);
		// UTF-16からUTF-8へ変換.
		ModelInfo.ModelComment = MyString::UTF16ToUTF8(UTF16ModelComment);
	}
	else {
		// UTF-8.
		ModelInfo.ModelComment = std::string(CommentBuffer.data(), CommentLength);
	}

	// コメント英の読み込み.
	uint32_t CommentEnglishLength = 0;
	fread(&CommentEnglishLength, sizeof(CommentEnglishLength), 1, fp);
	std::vector<char> CommentEnglishBuffer(CommentEnglishLength);
	fread(CommentEnglishBuffer.data(), CommentEnglishLength, 1, fp);

	// コメント英のエンコーディング処理.
	if (Header.Encoding == 0) {
		// UTF-16.
		std::u16string UTF16ModelCommentEnglish(reinterpret_cast<char16_t*>(CommentEnglishBuffer.data()), CommentEnglishLength / 2);
		// UTF-16からUTF-8へ変換.
		ModelInfo.ModelCommentEnglish = MyString::UTF16ToUTF8(UTF16ModelCommentEnglish);
	}
	else {
		// UTF-8
		ModelInfo.ModelCommentEnglish = std::string(CommentEnglishBuffer.data(), CommentEnglishLength);
	}
#endif
	// 処理.

	// 頂点データの読み込み
	std::vector<PMXVertex> Vertices;
	uint32_t VerticesLength;
	fread(&VerticesLength, sizeof(VerticesLength), 1, fp);
	Vertices.reserve(VerticesLength);

	// サイズ計算用頂点構造体.
	struct VertexSize {
		DirectX::XMFLOAT3	Position;	// 頂点位置.
		DirectX::XMFLOAT3	Normal;		// 頂点法線.
		DirectX::XMFLOAT2	UV;			// 頂点UV座標.
	};

	// 頂点情報の読み込み.
	for (uint32_t i = 0; i < VerticesLength; ++i) {
		// 空の頂点を直接ベクター内で構築.
		Vertices.emplace_back();

		// 頂点位置、頂点法線、頂点UV座標の読み込み.
		fread(&Vertices.back(), sizeof(VertexSize), 1, fp);

		// 追加UV座標 (最大4つまで).
		uint8_t AdditionalUVCount;
		fread(&AdditionalUVCount, sizeof(AdditionalUVCount * Header.AdditionalUV), static_cast<size_t>(Header.AdditionalUV), fp);
		Vertices.back().AdditionalUV.resize(Header.AdditionalUV);

		if (Header.AdditionalUV > 0) {
			// 追加UVを一度に読み込む.
			fread(Vertices.back().AdditionalUV.data(), sizeof(DirectX::XMFLOAT4), static_cast<size_t>(Header.AdditionalUV), fp);
		}

		// ボーンウェイトやその他の情報を読み込む.
		uint8_t WeightType;
		fread(&WeightType, sizeof(WeightType), 1, fp);

		switch (WeightType) {
		case 0: // BDEF1.
		{
			uint32_t BoneIndex1;
			fread(&BoneIndex1, static_cast<size_t>(Header.BoneIndexSize), 1, fp);
			Vertices.back().BoneWeight = PMXBoneWeight(BoneIndex1);
		}
		break;
		case 1: // BDEF2.
			uint16_t BoneIndex2_1, BoneIndex2_2;
			float Weight2_1;
			fread(&BoneIndex2_1, static_cast<size_t>(Header.BoneIndexSize), 1, fp);
			fread(&BoneIndex2_2, static_cast<size_t>(Header.BoneIndexSize), 1, fp);
			fread(&Weight2_1, sizeof(Weight2_1), 1, fp);
			Vertices.back().BoneWeight = PMXBoneWeight(BoneIndex2_1, BoneIndex2_2, Weight2_1);
			break;
		case 2: // BDEF4.
			uint16_t BoneIndex4_1, BoneIndex4_2, BoneIndex4_3, BoneIndex4_4;
			float Weight4_1, Weight4_2, Weight4_3, Weight4_4;
			fread(&BoneIndex4_1, static_cast<size_t>(Header.BoneIndexSize), 1, fp);
			fread(&BoneIndex4_2, static_cast<size_t>(Header.BoneIndexSize), 1, fp);
			fread(&BoneIndex4_3, static_cast<size_t>(Header.BoneIndexSize), 1, fp);
			fread(&BoneIndex4_4, static_cast<size_t>(Header.BoneIndexSize), 1, fp);
			fread(&Weight4_1, sizeof(Weight4_1), 1, fp);
			fread(&Weight4_2, sizeof(Weight4_2), 1, fp);
			fread(&Weight4_3, sizeof(Weight4_3), 1, fp);
			fread(&Weight4_4, sizeof(Weight4_4), 1, fp);
			Vertices.back().BoneWeight = PMXBoneWeight(BoneIndex4_1, BoneIndex4_2, BoneIndex4_3, BoneIndex4_4, Weight4_1, Weight4_2, Weight4_3, Weight4_4);
			break;
		case 3: // SDEF.
			uint16_t BoneIndexSDEF_1, BoneIndexSDEF_2;
			float WeightSDEF_1;
			DirectX::XMFLOAT3 C, R0, R1;
			fread(&BoneIndexSDEF_1, static_cast<size_t>(Header.BoneIndexSize), 1, fp);
			fread(&BoneIndexSDEF_2, static_cast<size_t>(Header.BoneIndexSize), 1, fp);
			fread(&WeightSDEF_1, sizeof(WeightSDEF_1), 1, fp);
			fread(&C, sizeof(DirectX::XMFLOAT3), 1, fp);
			fread(&R0, sizeof(DirectX::XMFLOAT3), 1, fp);
			fread(&R1, sizeof(DirectX::XMFLOAT3), 1, fp);
			Vertices.back().BoneWeight = PMXBoneWeight(BoneIndexSDEF_1, BoneIndexSDEF_2, WeightSDEF_1, C, R0, R1);
			break;
		}

		// エッジ倍率の読み込み.
		fread(&Vertices.back().Edge, sizeof(float), 1, fp);
	}

	// DirectXバッファを作成.
	auto heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto resDesc = CD3DX12_RESOURCE_DESC::Buffer(Vertices.size() * sizeof(PMXVertex));

	MyAssert::IsFailed(
		_T("頂点バッファの作成"),
		&ID3D12Device::CreateCommittedResource,
		m_pDx12.GetDevice().Get(),
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(m_pVertexBuffer.ReleaseAndGetAddressOf())
	);

	// データをGPUバッファにコピー.
	unsigned char* vertMap = nullptr;
	m_pVertexBuffer->Map(0, nullptr, reinterpret_cast<void**>(&vertMap));

	// PMXVertexのデータをバッファにコピー.
	std::memcpy(vertMap, Vertices.data(), Vertices.size() * sizeof(PMXVertex));

	m_pVertexBuffer->Unmap(0, nullptr);

	// 頂点バッファビューの設定.
	m_pVertexBufferView.BufferLocation = m_pVertexBuffer->GetGPUVirtualAddress();
	m_pVertexBufferView.SizeInBytes = static_cast<UINT>(Vertices.size() * sizeof(PMXVertex));
	m_pVertexBufferView.StrideInBytes = sizeof(PMXVertex);

	// インデックス数を読み込む.
	uint32_t IndicesNum;
	fread(&IndicesNum, sizeof(uint32_t), 1, fp);

	// インデックスバッファを読み込む.
	std::vector<PMXFace> FlatIndices(IndicesNum / 3);
	fread(FlatIndices.data(), sizeof(PMXFace), IndicesNum / 3, fp);

	// インデックスバッファ用のDirectX12リソースを作成.
	auto ResDescBuf = CD3DX12_RESOURCE_DESC::Buffer(FlatIndices.size() * sizeof(FlatIndices[0]));
	MyAssert::IsFailed(
		_T("インデックスバッファの作成"),
		&ID3D12Device::CreateCommittedResource, m_pDx12.GetDevice().Get(),
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&ResDescBuf,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(m_pIndexBuffer.ReleaseAndGetAddressOf())
	);

	// インデックスデータのマッピングとGPU転送.
	unsigned short* MappedIdx1 = new unsigned short[IndicesNum];
	for (size_t i = 0; i < IndicesNum / 3; ++i) {
		MappedIdx1[i * 3 + 0] = static_cast<unsigned short>(FlatIndices[i].Index[0]);
		MappedIdx1[i * 3 + 1] = static_cast<unsigned short>(FlatIndices[i].Index[1]);
		MappedIdx1[i * 3 + 2] = static_cast<unsigned short>(FlatIndices[i].Index[2]);
	}

	unsigned short* mappedIdx = nullptr;
	m_pIndexBuffer->Map(0, nullptr, (void**)&mappedIdx);

	std::memcpy(mappedIdx, MappedIdx1, IndicesNum * sizeof(unsigned short));

	delete[] MappedIdx1;

	m_pIndexBuffer->Unmap(0, nullptr);

	// インデックスバッファビューの設定.
	m_pIndexBufferView.BufferLocation = m_pIndexBuffer->GetGPUVirtualAddress();
	m_pIndexBufferView.Format = DXGI_FORMAT_R16_UINT;
	m_pIndexBufferView.SizeInBytes = static_cast<UINT>(FlatIndices.size() * sizeof(FlatIndices[0]));

	// テクスチャの読み込み.
	TexturePath TextureInfo;

	// テクスチャ数の読み込み.
	fread(&TextureInfo.TextureCount, sizeof(uint32_t), 1, fp);

	TextureInfo.TexturePaths.resize(TextureInfo.TextureCount);

	for (uint32_t i = 0; i < TextureInfo.TextureCount; ++i)
	{
		// バッファのサイズを取得.
		uint32_t bufferLength = 0;
		fread(&bufferLength, sizeof(uint32_t), 1, fp);

		// バッファのデータ(テクスチャパス).
		std::vector<uint8_t> buffer(bufferLength);
		fread(buffer.data(), sizeof(uint8_t), bufferLength, fp);

		// パスエンコーディング処理.
		if (Header.Encoding == 0) {  
			// UTF-16.
			std::u16string UTF16Name(reinterpret_cast<char16_t*>(buffer.data()), bufferLength / 2);
			// モデルからの相対パスをアプリからの相対パス変換し保存.
			TextureInfo.TexturePaths[i] = MyFilePath::GetTexPath(path, MyString::UTF16ToUTF8(UTF16Name).c_str());
		}
		else {  
			// UTF-8.
			// モデルからの相対パスをアプリからの相対パス変換し保存.
			TextureInfo.TexturePaths[i] = MyFilePath::GetTexPath(path, std::string(buffer.begin(), buffer.end()).c_str());
		}

		// ファイルパスの/を\\に統一.
		MyFilePath::ReplaceSlashWithBackslash(&TextureInfo.TexturePaths[i]);
	}

	// マテリアル読み込み.
	uint32_t MaterialNum;
	fread(&MaterialNum, sizeof(MaterialNum), 1, fp);
	
	// マテリアル用バッファをリサイズ.
	m_pMaterials.resize(MaterialNum);
	m_pTextureResource.resize(MaterialNum);
	m_pSphResource.resize(MaterialNum);
	m_pSpaResource.resize(MaterialNum);
	m_pToonResource.resize(MaterialNum);

	// マテリアルデータの読み込み.
	std::vector<PMXMaterial> Materials; 
	Materials.reserve(MaterialNum);


	for (uint32_t i = 0; i < MaterialNum; ++i) {
		Materials.emplace_back();
		m_pMaterials[i] = std::make_shared<Material>();

		uint32_t NameLength = 0;
		fread(&NameLength, sizeof(NameLength), 1, fp);
		std::vector<char> NameBuffer(NameLength);
		fread(NameBuffer.data(), NameLength, 1, fp);

		uint32_t NameEnglishLength = 0;
		fread(&NameEnglishLength, sizeof(NameEnglishLength), 1, fp);
		std::vector<char> NameEnglishBuffer(NameEnglishLength);
		fread(NameEnglishBuffer.data(), NameEnglishLength, 1, fp);

		// パスエンコーディング処理.
		if (Header.Encoding == 0) {
			// UTF-16.
			std::u16string UTF16Name(reinterpret_cast<char16_t*>(NameBuffer.data()), NameLength / 2);
			Materials.back().Name = MyString::UTF16ToUTF8(UTF16Name);
		}
		else {
			// UTF-8. 
			Materials.back().Name = std::string(NameBuffer.begin(), NameBuffer.end());
		}

		// パスエンコーディング処理.
		if (Header.Encoding == 0) {
			// UTF-16.
			std::u16string UTF16Name(reinterpret_cast<char16_t*>(NameEnglishBuffer.data()), NameEnglishLength / 2);
			Materials.back().EnglishName = MyString::UTF16ToUTF8(UTF16Name);
		}
		else {
			// UTF-8.
			Materials.back().EnglishName = std::string(NameEnglishBuffer.begin(), NameEnglishBuffer.end());
		}

		// 固定部分読み取り
		fread(&Materials.back().Diffuse, sizeof(DirectX::XMFLOAT4), 1, fp);
		fread(&Materials.back().Specular, sizeof(DirectX::XMFLOAT3), 1, fp);
		fread(&Materials.back().Specularity, sizeof(float), 1, fp);
		fread(&Materials.back().Ambient, sizeof(DirectX::XMFLOAT3), 1, fp);

		m_pMaterials[i]->Materialhlsl.Diffuse = Materials.back().Diffuse;
		m_pMaterials[i]->Materialhlsl.Specular = Materials.back().Specular;
		m_pMaterials[i]->Materialhlsl.Specularity = Materials.back().Specularity;
		m_pMaterials[i]->Materialhlsl.Ambient = Materials.back().Ambient;


		// 描画フラグ (bitFlag).
		uint8_t bitFlag;
		fread(&bitFlag, sizeof(uint8_t), 1, fp);

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

		fread(&Materials.back().ToonTextureIndex, sizeof(Materials.back().ToonTextureIndex), 1, fp);


		// メモ (TextBuf)
		uint32_t MemoLength = 0;
		fread(&MemoLength, sizeof(MemoLength), 1, fp);
		std::vector<char> MameBuffer(MemoLength);
		fread(MameBuffer.data(), MameBuffer.size(), 1, fp);

		// 面数
		fread(&Materials.back().FaceCount, sizeof(uint32_t), 1, fp);
		Materials.back().FaceCount /= 3;

		m_pMaterials[i]->IndicesNum = Materials.back().FaceCount;
	}

	std::vector<uint8_t> emp(Materials.size(), -1);

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
			if (Materials[i].ToonTextureIndex + 1 >= TextureInfo.TexturePaths.size()) { continue; }

			m_pToonResource[i] = m_pDx12.GetTextureByPath(TextureInfo.TexturePaths[Materials[i].ToonTextureIndex + 1].c_str());
		}

		// テクスチャパスの分解とリソースのロード.
		std::string TexFileName = TextureInfo.TexturePaths[Materials[i].ToonTextureIndex + 1];
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
	}

	// ファイルを閉じる.
	fclose(fp);
}

void CPMXActor::LoadVMDFile(const char* FilePath, const char* Name)
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

void CPMXActor::CreateTransformView() {
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

	//マップとコピー
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

void CPMXActor::CreateMaterialData() {
	//マテリアルバッファを作成
	auto MaterialBuffSize = sizeof(MaterialForHlsl);
	MaterialBuffSize = (MaterialBuffSize + 0xff)&~0xff;

	auto heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto resDesc = CD3DX12_RESOURCE_DESC::Buffer(MaterialBuffSize * m_pMaterials.size());

	MyAssert::IsFailed(
		_T("マテリアル作成"),
		&ID3D12Device::CreateCommittedResource, m_pDx12.GetDevice(),
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,//勿体ないけど仕方ないですね
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(m_pMaterialBuff.ReleaseAndGetAddressOf()));

	//マップマテリアルにコピー
	char* mapMaterial = nullptr;

	MyAssert::IsFailed(
		_T("マテリアルマップにコピー"),
		&ID3D12Resource::Map, m_pMaterialBuff.Get(),
		0, nullptr, 
		(void**)&mapMaterial);

	for (auto& m : m_pMaterials) {
		*((MaterialForHlsl*)mapMaterial) = m->Materialhlsl;//データコピー
		mapMaterial += MaterialBuffSize;//次のアライメント位置まで進める
	}

	m_pMaterialBuff->Unmap(0, nullptr);

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
	D3D12_DESCRIPTOR_HEAP_DESC MaterialDescHeapDesc = {};
	MaterialDescHeapDesc.NumDescriptors = static_cast<UINT>(m_pMaterials.size() * 5);//マテリアル数ぶん(定数1つ、テクスチャ3つ)
	MaterialDescHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	MaterialDescHeapDesc.NodeMask = 0;

	MaterialDescHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;//デスクリプタヒープ種別

	MyAssert::IsFailed(
		_T("マテリアルヒープの作成"),
		&ID3D12Device::CreateDescriptorHeap, m_pDx12.GetDevice(),
		&MaterialDescHeapDesc,
		IID_PPV_ARGS(m_pMaterialHeap.ReleaseAndGetAddressOf()));

	auto MaterialBuffSize = sizeof(MaterialForHlsl);
	MaterialBuffSize = (MaterialBuffSize + 0xff)&~0xff;
	D3D12_CONSTANT_BUFFER_VIEW_DESC matCBVDesc = {};
	matCBVDesc.BufferLocation = m_pMaterialBuff->GetGPUVirtualAddress();
	matCBVDesc.SizeInBytes = static_cast<UINT>(MaterialBuffSize);
	
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;//後述
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;//2Dテクスチャ
	srvDesc.Texture2D.MipLevels = 1;//ミップマップは使用しないので1
	CD3DX12_CPU_DESCRIPTOR_HANDLE matDescHeapH(m_pMaterialHeap->GetCPUDescriptorHandleForHeapStart());
	auto incSize = m_pDx12.GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	for (int i = 0; i < m_pMaterials.size(); ++i) {
		//マテリアル固定バッファビュー
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


void CPMXActor::Update() {
	_angle += 0.03f;
	//MotionUpdate();
}

void CPMXActor::Draw() {
	m_pDx12.GetCommandList()->IASetVertexBuffers(0, 1, &m_pVertexBufferView);
	m_pDx12.GetCommandList()->IASetIndexBuffer(&m_pIndexBufferView);

	// 必要なディスクリプタヒープをすべて設定
	ID3D12DescriptorHeap* heaps[] = { m_pTransformHeap.Get(), m_pMaterialHeap.Get() };
	m_pDx12.GetCommandList()->SetDescriptorHeaps(_countof(heaps), heaps);

	// トランスフォームヒープを設定
	m_pDx12.GetCommandList()->SetGraphicsRootDescriptorTable(1, m_pTransformHeap->GetGPUDescriptorHandleForHeapStart());

	// マテリアルヒープを設定
	auto MaterialHeapHandle = m_pMaterialHeap->GetGPUDescriptorHandleForHeapStart();
	m_pDx12.GetCommandList()->SetGraphicsRootDescriptorTable(2, MaterialHeapHandle);

	unsigned int IdxOffset = 0;
	auto cbvsrvIncSize = m_pDx12.GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) * 5;
	for (auto& m : m_pMaterials) {
		m_pDx12.GetCommandList()->DrawIndexedInstanced(m->IndicesNum, 1, IdxOffset, 0, 0);
		IdxOffset += m->IndicesNum;
		MaterialHeapHandle.ptr += cbvsrvIncSize;
	}
}

// アニメーション開始.
void CPMXActor::PlayAnimation()
{
	m_StartTime = timeGetTime();
}

void CPMXActor::MotionUpdate()
{
	auto elapsedTime = timeGetTime() - m_StartTime;
	unsigned int frameNo = 30 * (elapsedTime / 1000.0f);

	//行列情報クリア(してないと前フレームのポーズが重ね掛けされてモデルが壊れる)
	std::fill(m_BoneMatrix.begin(), m_BoneMatrix.end(), DirectX::XMMatrixIdentity());

	//モーションデータ更新
	for (auto& bonemotion : m_MotionData) {
		auto node = m_BoneNodeTable[bonemotion.first];
		//合致するものを探す
		auto keyframes = bonemotion.second;

		auto rit = find_if(keyframes.rbegin(), keyframes.rend(), [frameNo](const KeyFrame& keyframe) {
			return keyframe.FrameNo <= frameNo;
			});
		if (rit == keyframes.rend())continue;//合致するものがなければ飛ばす
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
		auto mat = DirectX::XMMatrixTranslation(-pos.x, -pos.y, -pos.z) * //原点に戻し
			Rotation * //回転
			DirectX::XMMatrixTranslation(pos.x, pos.y, pos.z);//元の座標に戻す
		m_BoneMatrix[node.BoneIndex] = mat;
	}
	RecursiveMatrixMultipy(&m_BoneNodeTable["センター"], DirectX::XMMatrixIdentity());
	copy(m_BoneMatrix.begin(), m_BoneMatrix.end(), m_MappedMatrices + 1);
}
