#include "PMDActor.h"
#include "PMDRenderer.h"
#include "../DirectX/CDirectX12.h"	
#include <d3dx12.h>

namespace {
	///�e�N�X�`���̃p�X���Z�p���[�^�����ŕ�������
	///@param path �Ώۂ̃p�X������
	///@param splitter ��؂蕶��
	///@return �����O��̕�����y�A
	std::pair<std::string, std::string>
		SplitFileName(const std::string& path, const char splitter = '*') {
		int idx = path.find(splitter);
		std::pair<std::string, std::string> ret;
		ret.first = path.substr(0, idx);
		ret.second = path.substr(idx + 1, path.length() - idx - 1);
		return ret;
	}
	///�t�@�C��������g���q���擾����
	///@param path �Ώۂ̃p�X������
	///@return �g���q
	std::string
		GetExtension(const std::string& path) {
		int idx = path.rfind('.');
		return path.substr(idx + 1, path.length() - idx - 1);
	}
	///���f���̃p�X�ƃe�N�X�`���̃p�X���獇���p�X�𓾂�
	///@param modelPath �A�v���P�[�V�������猩��pmd���f���̃p�X
	///@param texPath PMD���f�����猩���e�N�X�`���̃p�X
	///@return �A�v���P�[�V�������猩���e�N�X�`���̃p�X
	std::string GetTexturePathFromModelAndTexPath(const std::string& modelPath, const char* texPath) {
		//�t�@�C���̃t�H���_��؂��\��/�̓��ނ��g�p�����\��������
		//�Ƃ�����������\��/�𓾂���΂����̂ŁA�o����rfind���Ƃ��r����
		//int�^�ɑ�����Ă���̂͌�����Ȃ������ꍇ��rfind��epos(-1��0xffffffff)��Ԃ�����
		int pathIndex1 = modelPath.rfind('/');
		int pathIndex2 = modelPath.rfind('\\');
		auto pathIndex = std::max(pathIndex1, pathIndex2);
		auto folderPath = modelPath.substr(0, pathIndex + 1);
		return folderPath + texPath;
	}
}

void* 
PMDActor::Transform::operator new(size_t size) {
	return _aligned_malloc(size, 16);
}

PMDActor::PMDActor(const char* filepath,PMDRenderer& renderer):
	m_pRenderer(renderer),
	m_pDx12(renderer.m_pDx12),
	_angle(0.0f)
{
	m_Transform.world = DirectX::XMMatrixIdentity();
	LoadPMDFile(filepath);
	CreateTransformView();
	CreateMaterialData();
	CreateMaterialAndTextureView();
}


PMDActor::~PMDActor()
{
}


HRESULT
PMDActor::LoadPMDFile(const char* path) {
	//PMD�w�b�_�\����
	struct PMDHeader {
		float version; //��F00 00 80 3F == 1.00
		char model_name[20];//���f����
		char comment[256];//���f���R�����g
	};
	char signature[3];
	PMDHeader pmdheader = {};

	std::string strModelPath = path;

	auto fp = fopen(strModelPath.c_str(), "rb");
	if (fp == nullptr) {
		//�G���[����
		assert(0);
		return ERROR_FILE_NOT_FOUND;
	}
	fread(signature, sizeof(signature), 1, fp);
	fread(&pmdheader, sizeof(pmdheader), 1, fp);

	unsigned int vertNum;//���_��
	fread(&vertNum, sizeof(vertNum), 1, fp);


#pragma pack(1)//��������1�o�C�g�p�b�L���O�c�A���C�����g�͔������Ȃ�
	//PMD�}�e���A���\����
	struct PMDMaterial {
		DirectX::XMFLOAT3 Diffuse; //�f�B�t���[�Y�F
		float Alpha; // �f�B�t���[�Y��
		float Specularity;//�X�y�L�����̋���(��Z�l)
		DirectX::XMFLOAT3 Specular; //�X�y�L�����F
		DirectX::XMFLOAT3 Ambient; //�A���r�G���g�F
		unsigned char ToonIdx; //�g�D�[���ԍ�(��q)
		unsigned char edgeFlg;//�}�e���A�����̗֊s���t���O
		//2�o�C�g�̃p�f�B���O�������I�I
		unsigned int IndicesNum; //���̃}�e���A�������蓖����C���f�b�N�X��
		char texFilePath[20]; //�e�N�X�`���t�@�C����(�v���X�A���t�@�c��q)
	};//70�o�C�g�̂͂��c�ł��p�f�B���O���������邽��72�o�C�g
#pragma pack()//1�o�C�g�p�b�L���O����

	constexpr unsigned int pmdvertex_size = 38;//���_1������̃T�C�Y
	std::vector<unsigned char> vertices(vertNum*pmdvertex_size);//�o�b�t�@�m��
	fread(vertices.data(), vertices.size(), 1, fp);//��C�ɓǂݍ���

	unsigned int IndicesNum;//�C���f�b�N�X��
	fread(&IndicesNum, sizeof(IndicesNum), 1, fp);//

	auto heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto resDesc = CD3DX12_RESOURCE_DESC::Buffer(vertices.size()*sizeof(vertices[0]));

	//UPLOAD(�m�ۂ͉\)
	auto result = m_pDx12.GetDevice()->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(m_pVertexBuffer.ReleaseAndGetAddressOf()));

	unsigned char* vertMap = nullptr;
	result = m_pVertexBuffer->Map(0, nullptr, (void**)&vertMap);
	std::copy(vertices.begin(), vertices.end(), vertMap);
	m_pVertexBuffer->Unmap(0, nullptr);


	m_pVertexBufferView.BufferLocation = m_pVertexBuffer->GetGPUVirtualAddress();//�o�b�t�@�̉��z�A�h���X
	m_pVertexBufferView.SizeInBytes = vertices.size();//�S�o�C�g��
	m_pVertexBufferView.StrideInBytes = pmdvertex_size;//1���_������̃o�C�g��

	std::vector<unsigned short> indices(IndicesNum);
	fread(indices.data(), indices.size() * sizeof(indices[0]), 1, fp);//��C�ɓǂݍ���


	auto resDescBuf=CD3DX12_RESOURCE_DESC::Buffer(indices.size() * sizeof(indices[0]));
	
	
	//�ݒ�́A�o�b�t�@�̃T�C�Y�ȊO���_�o�b�t�@�̐ݒ���g���܂킵��
	//OK���Ǝv���܂��B
	result = m_pDx12.GetDevice()->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&resDescBuf,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(m_pIndexBuffer.ReleaseAndGetAddressOf()));

	//������o�b�t�@�ɃC���f�b�N�X�f�[�^���R�s�[
	unsigned short* mappedIdx = nullptr;
	m_pIndexBuffer->Map(0, nullptr, (void**)&mappedIdx);
	std::copy(indices.begin(), indices.end(), mappedIdx);
	m_pIndexBuffer->Unmap(0, nullptr);


	//�C���f�b�N�X�o�b�t�@�r���[���쐬
	m_pIndexBufferView.BufferLocation = m_pIndexBuffer->GetGPUVirtualAddress();
	m_pIndexBufferView.Format = DXGI_FORMAT_R16_UINT;
	m_pIndexBufferView.SizeInBytes = indices.size() * sizeof(indices[0]);

	unsigned int MaterialNum;
	fread(&MaterialNum, sizeof(MaterialNum), 1, fp);
	m_pMaterial.resize(MaterialNum);
	m_pTextureResource.resize(MaterialNum);
	m_pSphResource.resize(MaterialNum);
	m_pSpaResource.resize(MaterialNum);

	m_pToonResource.resize(MaterialNum);
	std::vector<PMDMaterial> pmdMaterials(MaterialNum);
	fread(pmdMaterials.data(), pmdMaterials.size() * sizeof(PMDMaterial), 1, fp);
	//�R�s�[
	for (int i = 0; i < pmdMaterials.size(); ++i) {
		m_pMaterial[i]->IndicesNum = pmdMaterials[i].IndicesNum;
		m_pMaterial[i]->Material.Diffuse = pmdMaterials[i].Diffuse;
		m_pMaterial[i]->Material.Alpha = pmdMaterials[i].Alpha;
		m_pMaterial[i]->Material.Specular = pmdMaterials[i].Specular;
		m_pMaterial[i]->Material.Specularity = pmdMaterials[i].Specularity;
		m_pMaterial[i]->Material.Ambient = pmdMaterials[i].Ambient;
		m_pMaterial[i]->Additional.ToonIdx = pmdMaterials[i].ToonIdx;
	}

	for (int i = 0; i < pmdMaterials.size(); ++i) {
		//�g�D�[�����\�[�X�̓ǂݍ���
		char toonFilePath[32];
		sprintf(toonFilePath, "toon/toon%02d.bmp", pmdMaterials[i].ToonIdx + 1);
		m_pToonResource[i] = m_pDx12.GetTextureByPath(toonFilePath);

		if (strlen(pmdMaterials[i].texFilePath) == 0) {
			m_pTextureResource[i].Reset();
			continue;
		}

		std::string TexFileName = pmdMaterials[i].texFilePath;
		std::string SphFileName = "";
		std::string SpaFileName = "";
		if (count(TexFileName.begin(), TexFileName.end(), '*') > 0) {//�X�v���b�^������
			auto namepair = SplitFileName(TexFileName);
			if (GetExtension(namepair.first) == "sph") {
				TexFileName = namepair.second;
				SphFileName = namepair.first;
			}
			else if (GetExtension(namepair.first) == "spa") {
				TexFileName = namepair.second;
				SpaFileName = namepair.first;
			}
			else {
				TexFileName = namepair.first;
				if (GetExtension(namepair.second) == "sph") {
					SphFileName = namepair.second;
				}
				else if (GetExtension(namepair.second) == "spa") {
					SpaFileName = namepair.second;
				}
			}
		}
		else {
			if (GetExtension(pmdMaterials[i].texFilePath) == "sph") {
				SphFileName = pmdMaterials[i].texFilePath;
				TexFileName = "";
			}
			else if (GetExtension(pmdMaterials[i].texFilePath) == "spa") {
				SpaFileName = pmdMaterials[i].texFilePath;
				TexFileName = "";
			}
			else {
				TexFileName = pmdMaterials[i].texFilePath;
			}
		}
		//���f���ƃe�N�X�`���p�X����A�v���P�[�V��������̃e�N�X�`���p�X�𓾂�
		if (TexFileName != "") {
			auto texFilePath = GetTexturePathFromModelAndTexPath(strModelPath, TexFileName.c_str());
			m_pTextureResource[i] = m_pDx12.GetTextureByPath(texFilePath.c_str());
		}
		if (SphFileName != "") {
			auto sphFilePath = GetTexturePathFromModelAndTexPath(strModelPath, SphFileName.c_str());
			m_pSphResource[i] = m_pDx12.GetTextureByPath(sphFilePath.c_str());
		}
		if (SpaFileName != "") {
			auto spaFilePath = GetTexturePathFromModelAndTexPath(strModelPath, SpaFileName.c_str());
			m_pSpaResource[i] = m_pDx12.GetTextureByPath(spaFilePath.c_str());
		}
	}
	fclose(fp);

}

HRESULT 
PMDActor::CreateTransformView() {
	//GPU�o�b�t�@�쐬
	auto buffSize = sizeof(Transform);
	buffSize = (buffSize + 0xff)&~0xff;
	auto heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto resDesc = CD3DX12_RESOURCE_DESC::Buffer(buffSize);

	auto result = m_pDx12.GetDevice()->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(m_pTransformBuff.ReleaseAndGetAddressOf())
	);
	if (FAILED(result)) {
		assert(SUCCEEDED(result));
		return result;
	}

	//�}�b�v�ƃR�s�[
	result = m_pTransformBuff->Map(0, nullptr, (void**)&m_MappedTransform);
	if (FAILED(result)) {
		assert(SUCCEEDED(result));
		return result;
	}
	*m_MappedTransform = m_Transform;

	//�r���[�̍쐬
	D3D12_DESCRIPTOR_HEAP_DESC transformDescHeapDesc = {};
	transformDescHeapDesc.NumDescriptors = 1;//�Ƃ肠�������[���h�ЂƂ�
	transformDescHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	transformDescHeapDesc.NodeMask = 0;

	transformDescHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;//�f�X�N���v�^�q�[�v���
	result = m_pDx12.GetDevice()->CreateDescriptorHeap(&transformDescHeapDesc, IID_PPV_ARGS(m_pTransformHeap.ReleaseAndGetAddressOf()));//����
	if (FAILED(result)) {
		assert(SUCCEEDED(result));
		return result;
	}

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.BufferLocation = m_pTransformBuff->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = buffSize;
	m_pDx12.GetDevice()->CreateConstantBufferView(&cbvDesc, m_pTransformHeap->GetCPUDescriptorHandleForHeapStart());

	return S_OK;
}

HRESULT
PMDActor::CreateMaterialData() {
	//�}�e���A���o�b�t�@���쐬
	auto MaterialBuffSize = sizeof(MaterialForHlsl);
	MaterialBuffSize = (MaterialBuffSize + 0xff)&~0xff;

	auto heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto resDesc = CD3DX12_RESOURCE_DESC::Buffer(MaterialBuffSize * m_pMaterial.size());

	auto result = m_pDx12.GetDevice()->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,//�ܑ̂Ȃ����ǎd���Ȃ��ł���
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(m_pMaterialHeap.ReleaseAndGetAddressOf())
	);
	if (FAILED(result)) {
		assert(SUCCEEDED(result));
		return result;
	}

	//�}�b�v�}�e���A���ɃR�s�[
	char* mapMaterial = nullptr;
	result = m_pMaterialBuff->Map(0, nullptr, (void**)&mapMaterial);
	if (FAILED(result)) {
		assert(SUCCEEDED(result));
		return result;
	}
	for (auto& m : m_pMaterial) {
		*((MaterialForHlsl*)mapMaterial) = m->Material;//�f�[�^�R�s�[
		mapMaterial += MaterialBuffSize;//���̃A���C�����g�ʒu�܂Ői�߂�
	}
	m_pMaterialBuff->Unmap(0, nullptr);

	return S_OK;

}


HRESULT 
PMDActor::CreateMaterialAndTextureView() {
	D3D12_DESCRIPTOR_HEAP_DESC MaterialDescHeapDesc = {};
	MaterialDescHeapDesc.NumDescriptors = m_pMaterial.size() * 5;//�}�e���A�����Ԃ�(�萔1�A�e�N�X�`��3��)
	MaterialDescHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	MaterialDescHeapDesc.NodeMask = 0;

	MaterialDescHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;//�f�X�N���v�^�q�[�v���
	auto result = m_pDx12.GetDevice()->CreateDescriptorHeap(&MaterialDescHeapDesc, IID_PPV_ARGS(m_pMaterialHeap.ReleaseAndGetAddressOf()));//����
	if (FAILED(result)) {
		assert(SUCCEEDED(result));
		return result;
	}
	auto MaterialBuffSize = sizeof(MaterialForHlsl);
	MaterialBuffSize = (MaterialBuffSize + 0xff)&~0xff;
	D3D12_CONSTANT_BUFFER_VIEW_DESC matCBVDesc = {};
	matCBVDesc.BufferLocation = m_pMaterialBuff->GetGPUVirtualAddress();
	matCBVDesc.SizeInBytes = MaterialBuffSize;
	
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
			srvDesc.Format = m_pRenderer._whiteTex->GetDesc().Format;
			m_pDx12.GetDevice()->CreateShaderResourceView(m_pRenderer._whiteTex.Get(), &srvDesc, matDescHeapH);
		}
		else {
			srvDesc.Format = m_pTextureResource[i]->GetDesc().Format;
			m_pDx12.GetDevice()->CreateShaderResourceView(m_pTextureResource[i].Get(), &srvDesc, matDescHeapH);
		}
		matDescHeapH.Offset(incSize);

		if (m_pSphResource[i].Get() == nullptr) {
			srvDesc.Format = m_pRenderer._whiteTex->GetDesc().Format;
			m_pDx12.GetDevice()->CreateShaderResourceView(m_pRenderer._whiteTex.Get(), &srvDesc, matDescHeapH);
		}
		else {
			srvDesc.Format = m_pSphResource[i]->GetDesc().Format;
			m_pDx12.GetDevice()->CreateShaderResourceView(m_pSphResource[i].Get(), &srvDesc, matDescHeapH);
		}
		matDescHeapH.ptr += incSize;

		if (m_pSpaResource[i].Get() == nullptr) {
			srvDesc.Format = m_pRenderer._blackTex->GetDesc().Format;
			m_pDx12.GetDevice()->CreateShaderResourceView(m_pRenderer._blackTex.Get(), &srvDesc, matDescHeapH);
		}
		else {
			srvDesc.Format = m_pSpaResource[i]->GetDesc().Format;
			m_pDx12.GetDevice()->CreateShaderResourceView(m_pSpaResource[i].Get(), &srvDesc, matDescHeapH);
		}
		matDescHeapH.ptr += incSize;


		if (m_pToonResource[i].Get() == nullptr) {
			srvDesc.Format = m_pRenderer._gradTex->GetDesc().Format;
			m_pDx12.GetDevice()->CreateShaderResourceView(m_pRenderer._gradTex.Get(), &srvDesc, matDescHeapH);
		}
		else {
			srvDesc.Format = m_pToonResource[i]->GetDesc().Format;
			m_pDx12.GetDevice()->CreateShaderResourceView(m_pToonResource[i].Get(), &srvDesc, matDescHeapH);
		}
		matDescHeapH.ptr += incSize;
	}
}


void 
PMDActor::Update() {
	_angle += 0.03f;
	m_MappedTransform->world =  DirectX::XMMatrixRotationY(_angle);
}
void 
PMDActor::Draw() {
	m_pDx12.GetCommandList()->IASetVertexBuffers(0, 1, &m_pVertexBufferView);
	m_pDx12.GetCommandList()->IASetIndexBuffer(&m_pIndexBufferView);

	ID3D12DescriptorHeap* transheaps[] = { m_pTransformHeap.Get()};
	m_pDx12.GetCommandList()->SetDescriptorHeaps(1, transheaps);
	m_pDx12.GetCommandList()->SetGraphicsRootDescriptorTable(1, m_pTransformHeap->GetGPUDescriptorHandleForHeapStart());



	ID3D12DescriptorHeap* mdh[] = { m_pMaterialHeap.Get() };
	//�}�e���A��.
	m_pDx12.GetCommandList()->SetDescriptorHeaps(1, mdh);

	auto MaterialH = m_pMaterialHeap->GetGPUDescriptorHandleForHeapStart();
	unsigned int idxOffset = 0;

	auto cbvsrvIncSize = m_pDx12.GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) * 5;
	for (auto& m : m_pMaterial) {
		m_pDx12.GetCommandList()->SetGraphicsRootDescriptorTable(2, MaterialH);
		m_pDx12.GetCommandList()->DrawIndexedInstanced(m->IndicesNum, 1, idxOffset, 0, 0);
		MaterialH.ptr += cbvsrvIncSize;
		idxOffset += m->IndicesNum;
	}

}