#include "CPMDActor.h"
#include "CPMDRenderer.h"
#include "..\\DirectX\\CDirectX12.h"	
#include "Utility\\String\\FilePath\\FilePath.h"
#include "..\\Data\\Library\\DirectXTex\\DirectXTex\\d3dx12.h"
#include <iomanip>  // std::setprecision を使うために必要
static constexpr char AFTERCONVERSIONFILEPATH[] = "Out\\";
// テンプレート文字列を格納
static constexpr char TEMPLATESTRING[] = R"(template Header {
 <3D82AB43-62DA-11cf-AB39-0020AF71E433>
 WORD major;
 WORD minor;
 DWORD flags;
}

template Vector {
 <3D82AB5E-62DA-11cf-AB39-0020AF71E433>
 FLOAT x;
 FLOAT y;
 FLOAT z;
}

template Coords2d {
 <F6F23F44-7686-11cf-8F52-0040333594A3>
 FLOAT u;
 FLOAT v;
}

template Matrix4x4 {
 <F6F23F45-7686-11cf-8F52-0040333594A3>
 array FLOAT matrix[16];
}

template ColorRGBA {
 <35FF44E0-6C7C-11cf-8F52-0040333594A3>
 FLOAT red;
 FLOAT green;
 FLOAT blue;
 FLOAT alpha;
}

template ColorRGB {
 <D3E16E81-7835-11cf-8F52-0040333594A3>
 FLOAT red;
 FLOAT green;
 FLOAT blue;
}

template IndexedColor {
 <1630B820-7842-11cf-8F52-0040333594A3>
 DWORD index;
 ColorRGBA indexColor;
}

template Boolean {
 <4885AE61-78E8-11cf-8F52-0040333594A3>
 WORD truefalse;
}

template Boolean2d {
 <4885AE63-78E8-11cf-8F52-0040333594A3>
 Boolean u;
 Boolean v;
}

template MaterialWrap {
 <4885AE60-78E8-11cf-8F52-0040333594A3>
 Boolean u;
 Boolean v;
}

template TextureFilename {
 <A42790E1-7810-11cf-8F52-0040333594A3>
 STRING filename;
}

template Material {
 <3D82AB4D-62DA-11cf-AB39-0020AF71E433>
 ColorRGBA faceColor;
 FLOAT power;
 ColorRGB specularColor;
 ColorRGB emissiveColor;
 [...]
}

template MeshFace {
 <3D82AB5F-62DA-11cf-AB39-0020AF71E433>
 DWORD nFaceVertexIndices;
 array DWORD faceVertexIndices[nFaceVertexIndices];
}

template MeshFaceWraps {
 <4885AE62-78E8-11cf-8F52-0040333594A3>
 DWORD nFaceWrapValues;
 Boolean2d faceWrapValues;
}

template MeshTextureCoords {
 <F6F23F40-7686-11cf-8F52-0040333594A3>
 DWORD nTextureCoords;
 array Coords2d textureCoords[nTextureCoords];
}

template MeshMaterialList {
 <F6F23F42-7686-11cf-8F52-0040333594A3>
 DWORD nMaterials;
 DWORD nFaceIndexes;
 array DWORD faceIndexes[nFaceIndexes];
 [Material]
}

template MeshNormals {
 <F6F23F43-7686-11cf-8F52-0040333594A3>
 DWORD nNormals;
 array Vector normals[nNormals];
 DWORD nFaceNormals;
 array MeshFace faceNormals[nFaceNormals];
}

template MeshVertexColors {
 <1630B821-7842-11cf-8F52-0040333594A3>
 DWORD nVertexColors;
 array IndexedColor vertexColors[nVertexColors];
}

template Mesh {
 <3D82AB44-62DA-11cf-AB39-0020AF71E433>
 DWORD nVertices;
 array Vector vertices[nVertices];
 DWORD nFaces;
 array MeshFace faces[nFaces];
 [...]
}

Header{
1;
0;
1;
})";

void* CPMDActor::Transform::operator new(size_t size) {
	return _aligned_malloc(size, 16);
}

// 回転情報を末端まで伝播させる再帰関数.
void CPMDActor::RecursiveMatrixMultipy(
	BoneNode* node, 
	const DirectX::XMMATRIX& mat)
{
	m_BoneMatrix[node->BoneIndex] = mat;
	for (auto& cnode : node->Children) {
		// 子も同じ動作をする.
		RecursiveMatrixMultipy(cnode, m_BoneMatrix[cnode->BoneIndex] * mat);
	}
}

float CPMDActor::GetYFromXOnBezier(
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

CPMDActor::CPMDActor(const char* filepath,CPMDRenderer& renderer):
	m_pRenderer(renderer),
	m_pDx12(renderer.m_pDx12),
	_angle(0.0f)
{
	try {

		m_Name = filepath;

		MyFilePath::ReplaceSlashWithBackslash(&m_Name);
		std::string Names = MyFilePath::GetFileNameFromPath(m_Name);
		std::pair Name = MyFilePath::SplitFileName(Names, '.');
		m_Name = Name.first;
		
		m_Transform.world = DirectX::XMMatrixIdentity();
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


CPMDActor::~CPMDActor()
{
}


void CPMDActor::LoadPMDFile(const char* path)
{
	// ヘッダー読み込み用のシグネチャ.
	char Signature[3];
	PMDHeader Pmdheader = {};

	FILE* fp = nullptr;

	// ファイルをバイナリモードで開く.
	auto err = fopen_s(&fp, path, "rb");
	if (err != 0 || !fp) {
		throw std::runtime_error("ファイルを開くことができませんでした。");
	}

	// ヘッダー情報を読み込む.
	fread(Signature, sizeof(Signature), 1, fp);
	fread(&Pmdheader, sizeof(Pmdheader), 1, fp);

	// 頂点数を読み込む.
	unsigned int vertNum;
	fread(&vertNum, sizeof(vertNum), 1, fp);

	// 頂点データのバッファを確保し、一括読み込み.
	m_vertices.resize(vertNum * PmdVertexSize);
	fread(m_vertices.data(), m_vertices.size(), 1, fp);

	// 頂点バッファ用のDirectX 12リソースを作成.
	auto heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto resDesc = CD3DX12_RESOURCE_DESC::Buffer(m_vertices.size() * sizeof(m_vertices[0]));

	auto result = m_pDx12.GetDevice()->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(m_pVertexBuffer.ReleaseAndGetAddressOf())
	);

	// 頂点データをGPUバッファにコピー.
	unsigned char* vertMap = nullptr;
	result = m_pVertexBuffer->Map(0, nullptr, (void**)&vertMap);
	std::copy(m_vertices.begin(), m_vertices.end(), vertMap);
	m_pVertexBuffer->Unmap(0, nullptr);

	// 頂点バッファビューの設定.
	m_pVertexBufferView.BufferLocation = m_pVertexBuffer->GetGPUVirtualAddress();
	m_pVertexBufferView.SizeInBytes = static_cast<UINT>(m_vertices.size());
	m_pVertexBufferView.StrideInBytes = PmdVertexSize;

	// インデックス数を読み込む.
	fread(&m_IndicesNum, sizeof(m_IndicesNum), 1, fp);

	// インデックスデータのバッファを確保し、一括読み込み.
	std::vector<unsigned short> indices(m_IndicesNum);
	fread(indices.data(), indices.size() * sizeof(indices[0]), 1, fp);

	// インデックスバッファ用のDirectX 12リソースを作成.
	auto resDescBuf = CD3DX12_RESOURCE_DESC::Buffer(indices.size() * sizeof(indices[0]));
	result = m_pDx12.GetDevice()->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&resDescBuf,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(m_pIndexBuffer.ReleaseAndGetAddressOf())
	);

	// インデックスデータをGPUバッファにコピー.
	unsigned short* mappedIdx = nullptr;
	m_pIndexBuffer->Map(0, nullptr, (void**)&mappedIdx);
	std::copy(indices.begin(), indices.end(), mappedIdx);
	m_pIndexBuffer->Unmap(0, nullptr);

	// インデックスバッファビューの設定.
	m_pIndexBufferView.BufferLocation = m_pIndexBuffer->GetGPUVirtualAddress();
	m_pIndexBufferView.Format = DXGI_FORMAT_R16_UINT;
	m_pIndexBufferView.SizeInBytes = static_cast<UINT>(indices.size() * sizeof(indices[0]));

	// マテリアル数を読み込む.
	int MaterialNum;
	fread(&MaterialNum, sizeof(MaterialNum), 1, fp);

	// マテリアルやリソースのバッファをリサイズ.
	m_pMaterial.resize(MaterialNum);
	m_pTextureResource.resize(MaterialNum);
	m_pSphResource.resize(MaterialNum);
	m_pSpaResource.resize(MaterialNum);
	m_pToonResource.resize(MaterialNum);

	// マテリアルデータを読み込む.
	std::vector<PMDMaterial> pmdMaterials(MaterialNum);
	fread(pmdMaterials.data(), pmdMaterials.size() * sizeof(PMDMaterial), 1, fp);

	// 各マテリアルを設定.
	for (int i = 0; i < MaterialNum; ++i) {
		m_pMaterial[i] = std::make_shared<Material>();
	}

	// マテリアル情報をコピー.
	for (int i = 0; i < pmdMaterials.size(); ++i) {
		m_pMaterial[i]->IndicesNum = pmdMaterials[i].IndicesNum;
		m_pMaterial[i]->Materialhlsl.Diffuse = pmdMaterials[i].Diffuse;
		m_pMaterial[i]->Materialhlsl.Alpha = pmdMaterials[i].Alpha;
		m_pMaterial[i]->Materialhlsl.Specular = pmdMaterials[i].Specular;
		m_pMaterial[i]->Materialhlsl.Specularity = pmdMaterials[i].Specularity;
		m_pMaterial[i]->Materialhlsl.Ambient = pmdMaterials[i].Ambient;
		m_pMaterial[i]->Additional.ToonIdx = pmdMaterials[i].ToonIdx;
	}

	// トゥーンリソースとテクスチャを設定.
	for (int i = 0; i < pmdMaterials.size(); ++i) {
		// トゥーンテクスチャのファイルパスを構築.
		char toonFilePath[32];

		// Idxが255ならなしなのでcontinue.
		if (pmdMaterials[i].ToonIdx == 255) { continue; }

		sprintf_s(toonFilePath, "Data/Image/toon/toon%02d.bmp", pmdMaterials[i].ToonIdx + 1);
		m_pToonResource[i] = m_pDx12.GetTextureByPath(toonFilePath);

		// テクスチャパスが空の場合、リソースをリセット.
		if (strlen(pmdMaterials[i].TexFilePath) == 0) {
			m_pTextureResource[i].Reset();
			continue;
		}

		// テクスチャパスの分解とリソースのロード.
		std::string TexFileName = pmdMaterials[i].TexFilePath;
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
			if (MyFilePath::GetExtension(pmdMaterials[i].TexFilePath) == "sph") {
				SphFileName = pmdMaterials[i].TexFilePath;
				TexFileName = "";
			}
			else if (MyFilePath::GetExtension(pmdMaterials[i].TexFilePath) == "spa") {
				SpaFileName = pmdMaterials[i].TexFilePath;
				TexFileName = "";
			}
			else {
				TexFileName = pmdMaterials[i].TexFilePath;
			}
		}

		// リソースをロード.
		if (!TexFileName.empty()) {
			auto TexFilePath = MyFilePath::GetTexPath(path, TexFileName.c_str());
			m_pTextureResource[i] = m_pDx12.GetTextureByPath(TexFilePath.c_str());
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

	// ----------------
	// ボーン数の取得.
	unsigned short BoneNum = 0;
	fread(&BoneNum, sizeof(BoneNum), 1, fp);

	std::vector<PMDBone> PMDBones(BoneNum);
	fread(PMDBones.data(), sizeof(PMDBone), BoneNum, fp);

	std::vector<std::string> BoneNames(PMDBones.size());

	// ボーンノードを作る.
	for (size_t i = 0; i < PMDBones.size(); ++i)
	{
		PMDBone& PmdBone = PMDBones[i];
		BoneNames[i] = std::string(reinterpret_cast<char*>(PmdBone.BoneName));
		auto& Node = m_BoneNodeTable[std::string(reinterpret_cast<char*>(PmdBone.BoneName))];
		Node.BoneIndex = static_cast<int>(i);
		Node.StartPos = PmdBone.Pos;
	}

	// 親工関係の構築.
	for (PMDBone& pb : PMDBones)
	{
		// ありえない番号ならとばす.
		if (pb.ParentNo >= PMDBones.size()) { continue; }

		auto ParentName = BoneNames[pb.ParentNo];
		m_BoneNodeTable[ParentName].Children.emplace_back(&m_BoneNodeTable[std::string(reinterpret_cast<char*>(pb.BoneName))]);
	}

	//ボーン構築
	m_BoneMatrix.resize(PMDBones.size());

	// ボーンを初期化する.
	std::fill(
		m_BoneMatrix.begin(),
		m_BoneMatrix.end(),
		DirectX::XMMatrixIdentity()
	);

	// ファイルを閉じる.
	fclose(fp);
}

void CPMDActor::LoadVMDFile(const char* FilePath, const char* Name)
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

void CPMDActor::CreateTransformView() {
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

void CPMDActor::CreateMaterialData() {
	//マテリアルバッファを作成
	auto MaterialBuffSize = sizeof(MaterialForHlsl);
	MaterialBuffSize = (MaterialBuffSize + 0xff)&~0xff;

	auto heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto resDesc = CD3DX12_RESOURCE_DESC::Buffer(MaterialBuffSize * m_pMaterial.size());

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

	for (auto& m : m_pMaterial) {
		*((MaterialForHlsl*)mapMaterial) = m->Materialhlsl;//データコピー
		mapMaterial += MaterialBuffSize;//次のアライメント位置まで進める
	}

	m_pMaterialBuff->Unmap(0, nullptr);

	// -- 仮.

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
	// -- 仮.
}


void CPMDActor::CreateMaterialAndTextureView() {
	D3D12_DESCRIPTOR_HEAP_DESC MaterialDescHeapDesc = {};
	MaterialDescHeapDesc.NumDescriptors = static_cast<UINT>(m_pMaterial.size() * 5);//マテリアル数ぶん(定数1つ、テクスチャ3つ)
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
	for (int i = 0; i < m_pMaterial.size(); ++i) {
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


void CPMDActor::Update() {
	_angle += 0.03f;
	MotionUpdate();
}

void CPMDActor::Draw() {
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
	for (auto& m : m_pMaterial) {
		m_pDx12.GetCommandList()->SetGraphicsRootDescriptorTable(2, MaterialHeapHandle);
		m_pDx12.GetCommandList()->DrawIndexedInstanced(m->IndicesNum, 1, IdxOffset, 0, 0);
		MaterialHeapHandle.ptr += cbvsrvIncSize;
		IdxOffset += m->IndicesNum;
	}

}

// Xファイルの変換出力.
bool CPMDActor::SaveAsX() {
	// 頂点とインデックスデータを格納する変数
	std::vector<unsigned short> indices; // インデックスデータ

	// 頂点データを取得
	{
		void* pData = nullptr;
		HRESULT hr = m_pVertexBuffer->Map(0, nullptr, &pData);
		if (FAILED(hr)) {
			std::cerr << "Failed to map vertex buffer." << std::endl;
			return false;
		}

	}

	// インデックスデータを取得
	{
		void* pData = nullptr;
		HRESULT hr = m_pIndexBuffer->Map(0, nullptr, &pData);
		if (FAILED(hr)) {
			std::cerr << "Failed to map index buffer." << std::endl;
			return false;
		}

		indices.assign(static_cast<unsigned short*>(pData),
			static_cast<unsigned short*>(pData) +
			(m_pIndexBuffer->GetDesc().Width / sizeof(unsigned short)));

		m_pIndexBuffer->Unmap(0, nullptr);
	}

	// Xファイルの書き込み
	std::ofstream outFile(AFTERCONVERSIONFILEPATH + m_Name + "3.x");
	if (!outFile.is_open()) {
		std::cerr << "Failed to open file: " << AFTERCONVERSIONFILEPATH << std::endl;
		return false;
	}

	// Xファイルのヘッダー
	outFile << "xof 0302txt 0064\n";
	outFile << TEMPLATESTRING;

	// Mesh セクションの開始
	outFile << "\n\n";
	outFile << "Mesh {\n";
	outFile << " " << m_vertices.size() / 38 << ";\n";

	// 頂点データの書き込み
	for (size_t i = 0; i < m_vertices.size() / 38; ++i) {
		// char型のデータをPMDVertex構造体として扱う
		PMDVertex& vertex = *reinterpret_cast<PMDVertex*>(&m_vertices[i * 38]);

		outFile << std::fixed << std::setprecision(6)  // 小数点以下6桁に設定
			<< vertex.Pos.x << ";" << vertex.Pos.y << ";" << vertex.Pos.z << ";"; // 座標を書き込む

		if (i < m_vertices.size() / 38 - 1) {
			outFile << ",\n"; // 次の頂点がある場合、カンマと改行を追加
		}
		else {
			outFile << ";\n"; // 最後の頂点の場合はセミコロンと改行を追加
		}
	}

	outFile << "\n";

	// インデックスデータの書き込み
	outFile << indices.size() / 3 << ";\n";
	for (size_t i = 0; i < indices.size(); i += 3) {
		outFile << "3;" << indices[i] << "," << indices[i + 1] << "," << indices[i + 2] << ";";
		if (i < indices.size() - 3) {
			outFile << ",\n";
		}
		else {
			outFile << ";\n";
		}
	}

	outFile << "\n";

	// Material情報の記述
	outFile << "MeshMaterialList {\n";
	outFile << " 1;\n"; // マテリアル数
	outFile << indices.size() / 3 << ";\n"; // 面数
	for (size_t i = 0; i < indices.size() / 3; ++i) {
		outFile << "0";
		if (i < indices.size() / 3 - 1) {
			outFile << ",\n";
		}
		else {
			outFile << ";;\n";
		}
	}


	// デフォルトのMaterialを記述
	outFile << " Material {\n";
	outFile << "  1.000000;1.000000;1.000000;1.000000;;\n";
	outFile << "  0.000000;\n";
	outFile << "  0.000000;0.000000;0.000000;;\n";
	outFile << "  0.000000;0.000000;0.000000;;\n";
	outFile << " }\n";
	outFile << "}\n";

	outFile << "MeshNormals {\n";
	outFile << indices.size() / 3 << ";\n";

	for (size_t i = 0; i < indices.size() / 3; ++i)
	{
		outFile << "1.000000;1.000000;1.000000;"; 
		
		if (i < indices.size() / 3 - 1) {
			outFile << ",\n";
		}
		else {
			outFile << ";\n";
		}
	}

	outFile << indices.size() / 3 << ";\n";

	for (size_t i = 0; i < indices.size() / 3; ++i)
	{
		outFile << 3 << ";" << i << ","<< i << ","<< i << ";";
		if (i < indices.size() / 3 - 1) {
			outFile << ",\n";
		}
		else {
			outFile << ";\n";
		}
	}
	outFile << "}\n";

	int i = m_vertices.size() / 38;

	outFile << "MeshTextureCoords {\n" << m_vertices.size() / 38 << ";\n";
	
	// 頂点データの書き込み
	for (size_t i = 0; i < m_vertices.size() / 38; ++i) {
		// char型のデータをPMDVertex構造体として扱う
		PMDVertex& vertex = *reinterpret_cast<PMDVertex*>(&m_vertices[i * 38]);

		outFile << std::fixed << std::setprecision(6)  // 小数点以下6桁に設定
		<< vertex.UV.x << ";" << vertex.UV.y << ";";  // 座標を書き込む

		if (i < m_vertices.size() /38 - 1) {
			outFile << ",\n"; // 次の頂点がある場合、カンマと改行を追加
		}
		else {
			outFile << ";\n"; // 最後の頂点の場合はセミコロンと改行を追加
		}
	}
	outFile << "}\n";
	outFile << "}\n";



	outFile.close();
	return true;
}

// アニメーション開始.
void CPMDActor::PlayAnimation()
{
	m_StartTime = timeGetTime();
}

void CPMDActor::MotionUpdate()
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
