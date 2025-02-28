#include "CPMXRenderer.h"
#include "..\\Data\\Library\\DirectXTex\\DirectXTex\\d3dx12.h"
#include<cassert>
#include<d3dcompiler.h>
#include"..\\DirectX\\CDirectX12.h"
#include<string>
#include<algorithm>

constexpr size_t PMDTexWide = 4;

CPMXRenderer::CPMXRenderer(CDirectX12& dx12)
	: m_pDx12	(dx12)
{
	CreateRootSignature();
	CreateGraphicsPipelineForPMX();

	// PMD�p�ėp�e�N�X�`���̐���.
	m_pWhiteTex = MyComPtr<ID3D12Resource>(CreateWhiteTexture());
	m_pBlackTex = MyComPtr<ID3D12Resource>(CreateBlackTexture());
	m_pGradTex  = MyComPtr<ID3D12Resource>(CreateGrayGradationTexture());
}


CPMXRenderer::~CPMXRenderer()
{
}


void CPMXRenderer::Update() {

}

void CPMXRenderer::Draw() {

}

// �e�N�X�`���̔ėp�f�ނ��쐬.
ID3D12Resource* CPMXRenderer::CreateDefaultTexture(size_t Width, size_t Height) {

	// ���\�[�X�̐ݒ�.
	auto ResourceDesc = 
		CD3DX12_RESOURCE_DESC::Tex2D(
			DXGI_FORMAT_R8G8B8A8_UNORM,		// RGB8bit�t�H�[�}�b�g.
			static_cast<UINT>(Width),		// ��.
			static_cast<UINT>(Height));		// ����.

	// �q�[�v�̐ݒ�.
	auto TexHeapProp = 
		CD3DX12_HEAP_PROPERTIES(
		D3D12_CPU_PAGE_PROPERTY_WRITE_BACK, // CPU����ǂݏ����\�ȃ������̈�ɔz�u.
			D3D12_MEMORY_POOL_L0);          // �p�t�H�[�}���X�D��̃������v�[��.

	ID3D12Resource* Buffer= nullptr;

	MyAssert::IsFailed(
		_T("��{�e�N�X�`���̍쐬"),
		&ID3D12Device::CreateCommittedResource, m_pDx12.GetDevice(),
		&TexHeapProp,
		D3D12_HEAP_FLAG_NONE,//���Ɏw��Ȃ�
		&ResourceDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		nullptr,
		IID_PPV_ARGS(&Buffer)
	);

	return Buffer;
}

// ���e�N�X�`���쐬.
ID3D12Resource* CPMXRenderer::CreateWhiteTexture()
{
	// �e�N�X�`�����\�[�X�̍쐬.
	ID3D12Resource* WhiteBuff = CreateDefaultTexture(PMDTexWide, PMDTexWide);
	
	// �e�N�X�`���͈̔͂̔��f�[�^�쐬.
	std::vector<unsigned char> data(PMDTexWide * PMDTexWide * 4);
	std::fill(data.begin(), data.end(), 0xff);

	MyAssert::IsFailed(
		_T("�e�N�X�`�����\�[�X�𔒂œh��Ԃ�"),
		&ID3D12Resource::WriteToSubresource, WhiteBuff,
		0, nullptr,
		data.data(), 4 * 4, data.size());

	return WhiteBuff;
}

ID3D12Resource* CPMXRenderer::CreateBlackTexture()
{
	// �e�N�X�`�����\�[�X�̍쐬.
	ID3D12Resource* BlackBuff = CreateDefaultTexture(PMDTexWide, PMDTexWide);

	// �e�N�X�`���͈̔͂̍��f�[�^���쐬.
	std::vector<unsigned char> data(PMDTexWide * PMDTexWide * 4);
	std::fill(data.begin(), data.end(), 0x00);

	MyAssert::IsFailed(
		_T("�e�N�X�`�����\�[�X�����œh��Ԃ�"),
		&ID3D12Resource::WriteToSubresource, BlackBuff,
		0, nullptr,
		data.data(), 4 * 4, data.size());

	return BlackBuff;
}

ID3D12Resource* CPMXRenderer::CreateGrayGradationTexture()
{
	ID3D12Resource* GradBuff = CreateDefaultTexture(4, 256);

	// �e�N�X�`���͈̔͂̔�<->���O���f�[�V�����f�[�^�̍쐬.
	std::vector<unsigned int> data(4 * 256);
	auto it = data.begin();
	unsigned int c = 0xff;
	for (; it != data.end(); it += 4) {
		auto col = (0xff << 24) | RGB(c, c, c);//RGBA���t���т��Ă��邽��RGB�}�N����0xff<<24��p���ĕ\���B
		std::fill(it, it + 4, col);
		--c;
	}

	MyAssert::IsFailed(
		_T("�e�N�X�`�����\�[�X�ɔ�<->���O���f�[�V��������������"),
		&ID3D12Resource::WriteToSubresource, GradBuff,
		0, nullptr,
		data.data(), 4 * 4, data.size());

	return GradBuff;
}

//�p�C�v���C��������
void CPMXRenderer::CreateGraphicsPipelineForPMX() {

	MyComPtr<ID3DBlob> VSBlob(nullptr);		// ���_�V�F�[�_�[�̃u���u.
	MyComPtr<ID3DBlob> PSBlob(nullptr);		// �s�N�Z���V�F�[�_�[�̃u���u.
	MyComPtr<ID3DBlob> ErrerBlob(nullptr);	// �G���[�̃u���u.
	HRESULT   result = S_OK;

	// ���_�V�F�[�_�[�̓ǂݍ���.
	CompileShaderFromFile(
		L"Data\\Shader\\PMX\\PMXBasicVertexShader.hlsl",
		"VS", "vs_5_0",
		VSBlob.ReleaseAndGetAddressOf());

	CompileShaderFromFile(
		L"Data\\Shader\\PMX\\PMXBasicPixelShader.hlsl",
		"PS", "ps_5_0",
		PSBlob.ReleaseAndGetAddressOf());

	// TODO : �Z���ł�����.
	D3D12_INPUT_ELEMENT_DESC PMXInputLayout[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL"	, 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT	, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpipeline = {};
	gpipeline.pRootSignature = m_pRootSignature.Get();
	gpipeline.VS = CD3DX12_SHADER_BYTECODE(VSBlob.Get());
	gpipeline.PS = CD3DX12_SHADER_BYTECODE(PSBlob.Get());

	gpipeline.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;//���g��0xffffffff

	gpipeline.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);

	gpipeline.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	gpipeline.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;//�J�����O���Ȃ�

	gpipeline.DepthStencilState.DepthEnable = true;//�[�x�o�b�t�@���g����
	gpipeline.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;//�S�ď�������
	gpipeline.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;//�����������̗p
	gpipeline.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	gpipeline.DepthStencilState.StencilEnable = false;

	gpipeline.InputLayout.pInputElementDescs = PMXInputLayout;//���C�A�E�g�擪�A�h���X
	gpipeline.InputLayout.NumElements = _countof(PMXInputLayout);//���C�A�E�g�z��

	gpipeline.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;//�X�g���b�v���̃J�b�g�Ȃ�
	gpipeline.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;//�O�p�`�ō\��

	gpipeline.NumRenderTargets = 1;//���͂P�̂�
	gpipeline.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;//0�`1�ɐ��K�����ꂽRGBA

	gpipeline.SampleDesc.Count = 1;//�T���v�����O��1�s�N�Z���ɂ��P
	gpipeline.SampleDesc.Quality = 0;//�N�I���e�B�͍Œ�

	MyAssert::IsFailed(
		_T("�O���t�B�b�N�p�C�v���C���̍쐬"),
		&ID3D12Device::CreateGraphicsPipelineState, m_pDx12.GetDevice(),
		&gpipeline,
		IID_PPV_ARGS(m_pPipelineState.ReleaseAndGetAddressOf())
	);
}

//���[�g�V�O�l�`��������
void CPMXRenderer::CreateRootSignature() 
{
	// �f�B�X�N���v�^�����W�̍쐬.
	D3D12_DESCRIPTOR_RANGE DescRanges[4] = {};

	//�萔[b0](�r���[�v���W�F�N�V�����p).
	DescRanges[0].NumDescriptors = 1;
	DescRanges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	DescRanges[0].BaseShaderRegister = 0;
	DescRanges[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	//�萔[b1](���[���h�A�{�[���p).
	DescRanges[1].NumDescriptors = 1;
	DescRanges[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	DescRanges[1].BaseShaderRegister = 1;
	DescRanges[1].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	//�萔[b2](�}�e���A���p).
	DescRanges[2].NumDescriptors = 1;
	DescRanges[2].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	DescRanges[2].BaseShaderRegister = 2;
	DescRanges[2].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	//�e�N�X�`��3��(��{��sph�ƃg�D�[��).
	DescRanges[3].NumDescriptors = 3;
	DescRanges[3].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	DescRanges[3].BaseShaderRegister = 0;
	DescRanges[3].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND; 
	
	// ���[�g�p�����[�^�̍쐬.
	CD3DX12_ROOT_PARAMETER RootParameters[3] = {};
	RootParameters[0].InitAsDescriptorTable(1, &DescRanges[0]);	// �r���[�v���W�F�N�V�����ϊ�.
	RootParameters[1].InitAsDescriptorTable(1, &DescRanges[1]);	// ���[���h�E�{�[���ϊ�.
	RootParameters[2].InitAsDescriptorTable(2, &DescRanges[2]);	// �}�e���A������.
	
	// ���[�g�V�O�l�N�`���̍쐬.
	D3D12_ROOT_SIGNATURE_DESC RootSignatureDesc = {};

	RootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	RootSignatureDesc.pParameters = RootParameters;
	RootSignatureDesc.NumParameters = 3; 
	
	// �T���v���[�̍쐬.
	CD3DX12_STATIC_SAMPLER_DESC SamplerDesc[2] = {};
	SamplerDesc[0].Init(0);
	SamplerDesc[1].Init(1, D3D12_FILTER_ANISOTROPIC, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP);

	RootSignatureDesc.pStaticSamplers = SamplerDesc;
	RootSignatureDesc.NumStaticSamplers = 2;

	MyComPtr<ID3DBlob> RootSigBlob(nullptr);
	MyComPtr<ID3DBlob> ErrorBlob(nullptr);

	MyAssert::IsFailed(
		_T("���[�g�V�O�l�N�`�����V���A���C�Y����"),
		&D3D12SerializeRootSignature,
		&RootSignatureDesc,
		D3D_ROOT_SIGNATURE_VERSION_1,
		RootSigBlob.GetAddressOf(),
		ErrorBlob.GetAddressOf()
	);
	
	MyAssert::IsFailed(
		_T("���[�g�V�O�l�N�`�����V���A���C�Y����"),
		&ID3D12Device::CreateRootSignature, m_pDx12.GetDevice(),
		0,
		RootSigBlob->GetBufferPointer(),
		RootSigBlob->GetBufferSize(),
		IID_PPV_ARGS(m_pRootSignature.ReleaseAndGetAddressOf())
	);

}

// �V�F�[�_�[�̃R���p�C��.
HRESULT CPMXRenderer::CompileShaderFromFile(
	const std::wstring& FilePath,
	LPCSTR EntryPoint,
	LPCSTR Target,
	ID3DBlob** ShaderBlob)
{
	ID3DBlob* ErrorBlob = nullptr;
	HRESULT Result = D3DCompileFromFile(
		FilePath.c_str(),
		nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
		EntryPoint, Target,
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, // �f�o�b�O�I�v�V����.
		0, ShaderBlob, &ErrorBlob
	);

	// �R���p�C���G���[���ɃG���[�n���h�����O���s��.
	MyAssert::ErrorBlob(Result, ErrorBlob);

	return Result;
}

// PMD�p�̃p�C�v���C���X�e�[�g���擾.
ID3D12PipelineState* CPMXRenderer::GetPipelineState()
{
	return m_pPipelineState.Get();
}

// PMD�p�̃��[�g�������擾.
ID3D12RootSignature* CPMXRenderer::GetRootSignature()
{
	return m_pRootSignature.Get();
}

MyComPtr<ID3D12Resource>& CPMXRenderer::GetWhiteTex()
{
	return m_pWhiteTex;
}

MyComPtr<ID3D12Resource>& CPMXRenderer::GetBlackTex()
{
	return m_pBlackTex;
}

MyComPtr<ID3D12Resource>& CPMXRenderer::GetGradTex()
{
	return m_pGradTex;
}
