#include "CPMDActor.h"
#include "CPMDRenderer.h"
#include "..\\DirectX\\CDirectX12.h"	
#include "Utility\\String\\FilePath\\FilePath.h"
#include "..\\Data\\Library\\DirectXTex\\DirectXTex\\d3dx12.h"

void* CPMDActor::Transform::operator new(size_t size) {
	return _aligned_malloc(size, 16);
}

// ��]���𖖒[�܂œ`�d������ċA�֐�.
void CPMDActor::RecursiveMatrixMultipy(
	BoneNode* node, 
	const DirectX::XMMATRIX& mat)
{
	m_BoneMatrix[node->BoneIndex] = mat;
	for (auto& cnode : node->Children) {
		// �q���������������.
		RecursiveMatrixMultipy(cnode, m_BoneMatrix[cnode->BoneIndex] * mat);
	}
}

float CPMDActor::GetYFromXOnBezier(
	float x,
	const DirectX::XMFLOAT2& a,
	const DirectX::XMFLOAT2& b, uint8_t n)
{
	if (a.x == a.y && b.x == b.y)return x;//�v�Z�s�v
	float t = x;
	const float k0 = 1 + 3 * a.x - 3 * b.x;//t^3�̌W��
	const float k1 = 3 * b.x - 6 * a.x;//t^2�̌W��
	const float k2 = 3 * a.x;//t�̌W��

	//�덷�͈͓̔����ǂ����Ɏg�p����萔
	constexpr float epsilon = 0.0005f;

	for (int i = 0; i < n; ++i) {
		//f(t)���߂܁[��
		auto ft = k0 * t * t * t + k1 * t * t + k2 * t - x;
		//�������ʂ�0�ɋ߂�(�덷�͈͓̔�)�Ȃ�ł��؂�
		if (ft <= epsilon && ft >= -epsilon)break;

		t -= ft / 2;
	}
	//���ɋ��߂���t�͋��߂Ă���̂�y���v�Z����
	auto r = 1 - t;
	return t * t * t + 3 * t * t * r * b.y + 3 * t * r * r * a.y;
}

CPMDActor::CPMDActor(const char* filepath,CPMDRenderer& renderer):
	m_pRenderer(renderer),
	m_pDx12(renderer.m_pDx12),
	_angle(0.0f)
{
	try {
		m_Transform.world = DirectX::XMMatrixIdentity();
		LoadPMDFile(filepath);
		CreateTransformView();
		CreateMaterialData();
		CreateMaterialAndTextureView();
	}
	catch (const std::runtime_error& Msg) {

		// �G���[���b�Z�[�W��\��.
		std::wstring WStr = MyString::StringToWString(Msg.what());
		_ASSERT_EXPR(false, WStr.c_str());
	}
}


CPMDActor::~CPMDActor()
{
}


void CPMDActor::LoadPMDFile(const char* path)
{
	// �w�b�_�[�ǂݍ��ݗp�̃V�O�l�`��.
	char Signature[3];
	PMDHeader Pmdheader = {};

	FILE* fp = nullptr;

	// �t�@�C�����o�C�i�����[�h�ŊJ��.
	auto err = fopen_s(&fp, path, "rb");
	if (err != 0 || !fp) {
		throw std::runtime_error("�t�@�C�����J�����Ƃ��ł��܂���ł����B");
	}

	// �w�b�_�[����ǂݍ���.
	fread(Signature, sizeof(Signature), 1, fp);
	fread(&Pmdheader, sizeof(Pmdheader), 1, fp);

	// ���_����ǂݍ���.
	unsigned int vertNum;
	fread(&vertNum, sizeof(vertNum), 1, fp);

	// ���_�f�[�^�̃o�b�t�@���m�ۂ��A�ꊇ�ǂݍ���.
	std::vector<unsigned char> vertices(vertNum * PmdVertexSize);
	fread(vertices.data(), vertices.size(), 1, fp);

	// ���_�o�b�t�@�p��DirectX 12���\�[�X���쐬.
	auto heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto resDesc = CD3DX12_RESOURCE_DESC::Buffer(vertices.size() * sizeof(vertices[0]));

	auto result = m_pDx12.GetDevice()->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(m_pVertexBuffer.ReleaseAndGetAddressOf())
	);

	// ���_�f�[�^��GPU�o�b�t�@�ɃR�s�[.
	unsigned char* vertMap = nullptr;
	result = m_pVertexBuffer->Map(0, nullptr, (void**)&vertMap);
	std::copy(vertices.begin(), vertices.end(), vertMap);
	m_pVertexBuffer->Unmap(0, nullptr);

	// ���_�o�b�t�@�r���[�̐ݒ�.
	m_pVertexBufferView.BufferLocation = m_pVertexBuffer->GetGPUVirtualAddress();
	m_pVertexBufferView.SizeInBytes = static_cast<UINT>(vertices.size());
	m_pVertexBufferView.StrideInBytes = PmdVertexSize;

	// �C���f�b�N�X����ǂݍ���.
	unsigned int IndicesNum;
	fread(&IndicesNum, sizeof(IndicesNum), 1, fp);

	// �C���f�b�N�X�f�[�^�̃o�b�t�@���m�ۂ��A�ꊇ�ǂݍ���.
	std::vector<unsigned short> indices(IndicesNum);
	fread(indices.data(), indices.size() * sizeof(indices[0]), 1, fp);

	// �C���f�b�N�X�o�b�t�@�p��DirectX 12���\�[�X���쐬.
	auto resDescBuf = CD3DX12_RESOURCE_DESC::Buffer(indices.size() * sizeof(indices[0]));
	result = m_pDx12.GetDevice()->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&resDescBuf,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(m_pIndexBuffer.ReleaseAndGetAddressOf())
	);

	// �C���f�b�N�X�f�[�^��GPU�o�b�t�@�ɃR�s�[.
	unsigned short* mappedIdx = nullptr;
	m_pIndexBuffer->Map(0, nullptr, (void**)&mappedIdx);
	std::copy(indices.begin(), indices.end(), mappedIdx);
	m_pIndexBuffer->Unmap(0, nullptr);

	// �C���f�b�N�X�o�b�t�@�r���[�̐ݒ�.
	m_pIndexBufferView.BufferLocation = m_pIndexBuffer->GetGPUVirtualAddress();
	m_pIndexBufferView.Format = DXGI_FORMAT_R16_UINT;
	m_pIndexBufferView.SizeInBytes = static_cast<UINT>(indices.size() * sizeof(indices[0]));

	// �}�e���A������ǂݍ���.
	int MaterialNum;
	fread(&MaterialNum, sizeof(MaterialNum), 1, fp);

	// �}�e���A���⃊�\�[�X�̃o�b�t�@�����T�C�Y.
	m_pMaterial.resize(MaterialNum);
	m_pTextureResource.resize(MaterialNum);
	m_pSphResource.resize(MaterialNum);
	m_pSpaResource.resize(MaterialNum);
	m_pToonResource.resize(MaterialNum);

	// �}�e���A���f�[�^��ǂݍ���.
	std::vector<PMDMaterial> pmdMaterials(MaterialNum);
	fread(pmdMaterials.data(), pmdMaterials.size() * sizeof(PMDMaterial), 1, fp);

	// �e�}�e���A����ݒ�.
	for (int i = 0; i < MaterialNum; ++i) {
		m_pMaterial[i] = std::make_shared<Material>();
	}

	// �}�e���A�������R�s�[.
	for (int i = 0; i < pmdMaterials.size(); ++i) {
		m_pMaterial[i]->IndicesNum = pmdMaterials[i].IndicesNum;
		m_pMaterial[i]->Materialhlsl.Diffuse = pmdMaterials[i].Diffuse;
		m_pMaterial[i]->Materialhlsl.Alpha = pmdMaterials[i].Alpha;
		m_pMaterial[i]->Materialhlsl.Specular = pmdMaterials[i].Specular;
		m_pMaterial[i]->Materialhlsl.Specularity = pmdMaterials[i].Specularity;
		m_pMaterial[i]->Materialhlsl.Ambient = pmdMaterials[i].Ambient;
		m_pMaterial[i]->Additional.ToonIdx = pmdMaterials[i].ToonIdx;
	}

	// �g�D�[�����\�[�X�ƃe�N�X�`����ݒ�.
	for (int i = 0; i < pmdMaterials.size(); ++i) {
		// �g�D�[���e�N�X�`���̃t�@�C���p�X���\�z.
		char toonFilePath[32];

		// Idx��255�Ȃ�Ȃ��Ȃ̂�continue.
		if (pmdMaterials[i].ToonIdx == 255) { continue; }

		sprintf_s(toonFilePath, "Data/Image/toon/toon%02d.bmp", pmdMaterials[i].ToonIdx + 1);
		m_pToonResource[i] = m_pDx12.GetTextureByPath(toonFilePath);

		// �e�N�X�`���p�X����̏ꍇ�A���\�[�X�����Z�b�g.
		if (strlen(pmdMaterials[i].TexFilePath) == 0) {
			m_pTextureResource[i].Reset();
			continue;
		}

		// �e�N�X�`���p�X�̕����ƃ��\�[�X�̃��[�h.
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

		// ���\�[�X�����[�h.
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
	// �{�[�����̎擾.
	unsigned short BoneNum = 0;
	fread(&BoneNum, sizeof(BoneNum), 1, fp);

	std::vector<PMDBone> PMDBones(BoneNum);
	fread(PMDBones.data(), sizeof(PMDBone), BoneNum, fp);

	std::vector<std::string> BoneNames(PMDBones.size());

	// �{�[���m�[�h�����.
	for (size_t i = 0; i < PMDBones.size(); ++i)
	{
		PMDBone& PmdBone = PMDBones[i];
		BoneNames[i] = std::string(reinterpret_cast<char*>(PmdBone.BoneName));
		auto& Node = m_BoneNodeTable[std::string(reinterpret_cast<char*>(PmdBone.BoneName))];
		Node.BoneIndex = static_cast<int>(i);
		Node.StartPos = PmdBone.Pos;
	}

	// �e�H�֌W�̍\�z.
	for (PMDBone& pb : PMDBones)
	{
		// ���肦�Ȃ��ԍ��Ȃ�Ƃ΂�.
		if (pb.ParentNo >= PMDBones.size()) { continue; }

		auto ParentName = BoneNames[pb.ParentNo];
		m_BoneNodeTable[ParentName].Children.emplace_back(&m_BoneNodeTable[std::string(reinterpret_cast<char*>(pb.BoneName))]);
	}

	//�{�[���\�z
	m_BoneMatrix.resize(PMDBones.size());

	// �{�[��������������.
	std::fill(
		m_BoneMatrix.begin(),
		m_BoneMatrix.end(),
		DirectX::XMMatrixIdentity()
	);

	// �t�@�C�������.
	fclose(fp);
}

void CPMDActor::LoadVMDFile(const char* FilePath, const char* Name)
{
	FILE* fp = nullptr;
	// fopen_s���g���ăt�@�C�����o�C�i�����[�h�ŊJ��.
	auto err = fopen_s(&fp, FilePath, "rb");
	if (err != 0 || !fp) {
		throw std::runtime_error("�t�@�C�����J�����Ƃ��ł��܂���ł����B");
	}
	fseek(fp, 50, SEEK_SET);//�ŏ���50�o�C�g�͔�΂���OK
	unsigned int keyframeNum = 0;
	fread(&keyframeNum, sizeof(keyframeNum), 1, fp);

	std::vector<VMDKeyFrame> Keyframes(keyframeNum);
	for (auto& keyframe : Keyframes) {
		fread(keyframe.BoneName, sizeof(keyframe.BoneName), 1, fp);	// �{�[����.
		fread(&keyframe.FrameNo, sizeof(keyframe.FrameNo) +			// �t���[���ԍ�.
			sizeof(keyframe.Location) +								// �ʒu(IK�̂Ƃ��Ɏg�p�\��).
			sizeof(keyframe.Quaternion) +							// �N�I�[�^�j�I��.
			sizeof(keyframe.Bezier), 1, fp);						// ��ԃx�W�F�f�[�^.
	}

	for (auto& motion : m_MotionData) {
		std::sort(motion.second.begin(), motion.second.end(),
			[](const KeyFrame& lval, const KeyFrame& rval) {
				return lval.FrameNo <= rval.FrameNo;
			});
	}

	//VMD�̃L�[�t���[���f�[�^����A���ۂɎg�p����L�[�t���[���e�[�u���֕ϊ�.
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

	RecursiveMatrixMultipy(&m_BoneNodeTable["�Z���^�["], DirectX::XMMatrixIdentity());
	std::copy(m_BoneMatrix.begin(), m_BoneMatrix.end(), m_MappedMatrices + 1);

}

void CPMDActor::CreateTransformView() {
	//GPU�o�b�t�@�쐬
	auto buffSize = sizeof(Transform) * (1 + m_BoneMatrix.size());
	buffSize = (buffSize + 0xff)&~0xff;
	auto heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto resDesc = CD3DX12_RESOURCE_DESC::Buffer(buffSize);

	MyAssert::IsFailed(
		_T("���W�o�b�t�@�쐬"),
		&ID3D12Device::CreateCommittedResource, m_pDx12.GetDevice(),
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(m_pTransformBuff.ReleaseAndGetAddressOf())
	);

	//�}�b�v�ƃR�s�[
	MyAssert::IsFailed(
		_T("���W�̃}�b�v"),
		&ID3D12Resource::Map, m_pTransformBuff.Get(),
		0, nullptr, 
		(void**)&m_MappedMatrices);

	m_MappedMatrices[0] = m_Transform.world;
	copy(m_BoneMatrix.begin(), m_BoneMatrix.end(), m_MappedMatrices + 1);

	// �r���[�̍쐬.
	D3D12_DESCRIPTOR_HEAP_DESC transformDescHeapDesc = {};
	transformDescHeapDesc.NumDescriptors = 1; // �Ƃ肠�������[���h�ЂƂ�.
	transformDescHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	transformDescHeapDesc.NodeMask = 0;

	transformDescHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV; // �f�X�N���v�^�q�[�v���.

	MyAssert::IsFailed(
		_T("���W�q�[�v�̍쐬"),
		&ID3D12Device::CreateDescriptorHeap, m_pDx12.GetDevice(),
		&transformDescHeapDesc, 
		IID_PPV_ARGS(m_pTransformHeap.ReleaseAndGetAddressOf()));//����

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.BufferLocation = m_pTransformBuff->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = static_cast<UINT>(buffSize);
	m_pDx12.GetDevice()->CreateConstantBufferView(
		&cbvDesc,
		m_pTransformHeap->GetCPUDescriptorHandleForHeapStart());
}

void CPMDActor::CreateMaterialData() {
	//�}�e���A���o�b�t�@���쐬
	auto MaterialBuffSize = sizeof(MaterialForHlsl);
	MaterialBuffSize = (MaterialBuffSize + 0xff)&~0xff;

	auto heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto resDesc = CD3DX12_RESOURCE_DESC::Buffer(MaterialBuffSize * m_pMaterial.size());

	MyAssert::IsFailed(
		_T("�}�e���A���쐬"),
		&ID3D12Device::CreateCommittedResource, m_pDx12.GetDevice(),
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,//�ܑ̂Ȃ����ǎd���Ȃ��ł���
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(m_pMaterialBuff.ReleaseAndGetAddressOf()));

	//�}�b�v�}�e���A���ɃR�s�[
	char* mapMaterial = nullptr;

	MyAssert::IsFailed(
		_T("�}�e���A���}�b�v�ɃR�s�["),
		&ID3D12Resource::Map, m_pMaterialBuff.Get(),
		0, nullptr, 
		(void**)&mapMaterial);

	for (auto& m : m_pMaterial) {
		*((MaterialForHlsl*)mapMaterial) = m->Materialhlsl;//�f�[�^�R�s�[
		mapMaterial += MaterialBuffSize;//���̃A���C�����g�ʒu�܂Ői�߂�
	}

	m_pMaterialBuff->Unmap(0, nullptr);

	// -- ��.

	m_MappedMatrices[0] = DirectX::XMMatrixRotationY(_angle);

	auto armnode = m_BoneNodeTable["���r"];
	auto& armpos = armnode.StartPos;
	auto armMat =
		DirectX::XMMatrixTranslation(-armpos.x, -armpos.y, -armpos.x)
		* DirectX::XMMatrixRotationZ(DirectX::XM_PIDIV2)
		* DirectX::XMMatrixTranslation(armpos.x, armpos.y, armpos.x);
	
	auto elbowNode = m_BoneNodeTable["���Ђ�"];
	auto& elbowpos = elbowNode.StartPos;
	auto elbowMat = DirectX::XMMatrixTranslation(-elbowpos.x, -elbowpos.y, -elbowpos.x)
		* DirectX::XMMatrixRotationZ(-DirectX::XM_PIDIV2)
		* DirectX::XMMatrixTranslation(elbowpos.x, elbowpos.y, elbowpos.x);

	m_BoneMatrix[armnode.BoneIndex] = armMat;
	m_BoneMatrix[elbowNode.BoneIndex] = elbowMat;

	RecursiveMatrixMultipy(&m_BoneNodeTable ["�Z���^�["], DirectX::XMMatrixIdentity());


	copy(m_BoneMatrix.begin(), m_BoneMatrix.end(), m_MappedMatrices + 1);
	// -- ��.
}


void CPMDActor::CreateMaterialAndTextureView() {
	D3D12_DESCRIPTOR_HEAP_DESC MaterialDescHeapDesc = {};
	MaterialDescHeapDesc.NumDescriptors = static_cast<UINT>(m_pMaterial.size() * 5);//�}�e���A�����Ԃ�(�萔1�A�e�N�X�`��3��)
	MaterialDescHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	MaterialDescHeapDesc.NodeMask = 0;

	MaterialDescHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;//�f�X�N���v�^�q�[�v���

	MyAssert::IsFailed(
		_T("�}�e���A���q�[�v�̍쐬"),
		&ID3D12Device::CreateDescriptorHeap, m_pDx12.GetDevice(),
		&MaterialDescHeapDesc,
		IID_PPV_ARGS(m_pMaterialHeap.ReleaseAndGetAddressOf()));

	auto MaterialBuffSize = sizeof(MaterialForHlsl);
	MaterialBuffSize = (MaterialBuffSize + 0xff)&~0xff;
	D3D12_CONSTANT_BUFFER_VIEW_DESC matCBVDesc = {};
	matCBVDesc.BufferLocation = m_pMaterialBuff->GetGPUVirtualAddress();
	matCBVDesc.SizeInBytes = static_cast<UINT>(MaterialBuffSize);
	
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;//��q
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;//2D�e�N�X�`��
	srvDesc.Texture2D.MipLevels = 1;//�~�b�v�}�b�v�͎g�p���Ȃ��̂�1
	CD3DX12_CPU_DESCRIPTOR_HANDLE matDescHeapH(m_pMaterialHeap->GetCPUDescriptorHandleForHeapStart());
	auto incSize = m_pDx12.GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	for (int i = 0; i < m_pMaterial.size(); ++i) {
		//�}�e���A���Œ�o�b�t�@�r���[
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
	//�}�e���A��.
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

// �A�j���[�V�����J�n.
void CPMDActor::PlayAnimation()
{
	m_StartTime = timeGetTime();
}

void CPMDActor::MotionUpdate()
{
	auto elapsedTime = timeGetTime() - m_StartTime;
	unsigned int frameNo = 30 * (elapsedTime / 1000.0f);


	//�s����N���A(���ĂȂ��ƑO�t���[���̃|�[�Y���d�ˊ|������ă��f��������)
	std::fill(m_BoneMatrix.begin(), m_BoneMatrix.end(), DirectX::XMMatrixIdentity());

	//���[�V�����f�[�^�X�V
	for (auto& bonemotion : m_MotionData) {
		auto node = m_BoneNodeTable[bonemotion.first];
		//���v������̂�T��
		auto keyframes = bonemotion.second;

		auto rit = find_if(keyframes.rbegin(), keyframes.rend(), [frameNo](const KeyFrame& keyframe) {
			return keyframe.FrameNo <= frameNo;
			});
		if (rit == keyframes.rend())continue;//���v������̂��Ȃ���Δ�΂�
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
		auto mat = DirectX::XMMatrixTranslation(-pos.x, -pos.y, -pos.z) * //���_�ɖ߂�
			Rotation * //��]
			DirectX::XMMatrixTranslation(pos.x, pos.y, pos.z);//���̍��W�ɖ߂�
		m_BoneMatrix[node.BoneIndex] = mat;
	}
	RecursiveMatrixMultipy(&m_BoneNodeTable["�Z���^�["], DirectX::XMMatrixIdentity());
	copy(m_BoneMatrix.begin(), m_BoneMatrix.end(), m_MappedMatrices + 1);
}
