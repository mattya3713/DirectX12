#include "stdafx.h"
#include "ParticleSystem.h"

#include <algorithm>
#include <cmath>
#include <d3dcompiler.h>

#include <d3d12.h>
#include "..\..\..\..\Data\Library\DirectXTex\Common\d3dx12.h"

#include "10_Device/DirectX/DirectX12.h"
#include "00_Game/30_Camera/00_Base/CameraBase.h"
#include "00_Game/30_Camera/99_Manager/CameraManager.h"
#include "99_Utility/ComPtr/ComPtr.h"
#include "99_Utility/ServiceLocator/ServiceLocator.h"
#include "..\..\..\..\Data\Library\DirectXTex\Common\d3dx12.h"

namespace {

	// パーティクル描画の頂点(CPU側でビルボード展開したワールド座標+色).
	struct ParticleVertex
	{
		DirectX::XMFLOAT3 Position;
		DirectX::XMFLOAT4 Color;
	};

	constexpr size_t kVertsPerParticle = 6;                          // 2三角形(インデックスバッファ無し).
	constexpr size_t kMaxParticles     = 1024;                       // 同時存在上限(ヘッダ側と一致).
	constexpr size_t kMaxVertexCount   = kMaxParticles * kVertsPerParticle;
	constexpr float  kFadeStartRatio   = 0.5f; // 寿命の後半でフェード開始.

	// 描画パイプライン(プロセス単位で1つ. ParticleSystemが複数生成されても共有).
	MyComPtr<ID3D12RootSignature>& GetRootSignature()
	{
		static MyComPtr<ID3D12RootSignature> s_pRootSignature;
		return s_pRootSignature;
	}

	MyComPtr<ID3D12PipelineState>& GetPipelineState()
	{
		static MyComPtr<ID3D12PipelineState> s_pPipelineState;
		return s_pPipelineState;
	}

	// 動的頂点バッファ(FrameBufferCount分スロット. SceneBufferと同じ二重バッファ規約).
	MyComPtr<ID3D12Resource>& GetDynamicVertexBuffer(UINT FrameIndex)
	{
		static MyComPtr<ID3D12Resource> s_pBuffers[2];
		return s_pBuffers[FrameIndex];
	}

	ParticleVertex** GetMappedVertices(UINT FrameIndex)
	{
		static ParticleVertex* s_pMapped[2] = {};
		return &s_pMapped[FrameIndex];
	}

	HRESULT CompileShaderFromFile(const std::wstring& FilePath, LPCSTR EntryPoint, LPCSTR Target, ID3DBlob** ShaderBlob)
	{
		ID3DBlob* error_blob = nullptr;
		const HRESULT result = D3DCompileFromFile(
			FilePath.c_str(),
			nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
			EntryPoint, Target,
			D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
			0, ShaderBlob, &error_blob
		);

		if (error_blob) { error_blob->Release(); }

		return result;
	}
}

// パイプライン(ルートシグネチャ/PSO/動的頂点バッファ)を構築する.
bool ParticleSystem::Initialize(ID3D12Device* pDevice)
{
	if (!pDevice) { return false; }

	if (GetRootSignature()) { return true; } // 既に構築済み(2度目以降は何もしない).

	// ルートシグネチャ: SceneBuffer(b0)のルートCBVのみ(DirectX12が毎フレーム更新する共有バッファを指す).
	CD3DX12_ROOT_PARAMETER root_param{};
	root_param.InitAsConstantBufferView(0);

	D3D12_ROOT_SIGNATURE_DESC root_desc{};
	root_desc.Flags         = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	root_desc.pParameters   = &root_param;
	root_desc.NumParameters = 1;

	MyComPtr<ID3DBlob> sig_blob(nullptr);
	MyComPtr<ID3DBlob> sig_error(nullptr);
	if (FAILED(D3D12SerializeRootSignature(&root_desc, D3D_ROOT_SIGNATURE_VERSION_1, sig_blob.GetAddressOf(), sig_error.GetAddressOf())))
	{
		return false;
	}
	if (FAILED(pDevice->CreateRootSignature(0, sig_blob->GetBufferPointer(), sig_blob->GetBufferSize(), IID_PPV_ARGS(GetRootSignature().GetAddressOf()))))
	{
		return false;
	}

	// シェーダーコンパイル.
	MyComPtr<ID3DBlob> vs_blob(nullptr);
	MyComPtr<ID3DBlob> ps_blob(nullptr);
	if (FAILED(CompileShaderFromFile(L"Data/Shader/Particle/ParticleVS.hlsl", "main", "vs_5_0", vs_blob.GetAddressOf()))) { return false; }
	if (FAILED(CompileShaderFromFile(L"Data/Shader/Particle/ParticlePS.hlsl", "main", "ps_5_0", ps_blob.GetAddressOf()))) { return false; }

	D3D12_INPUT_ELEMENT_DESC input_layout[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

	D3D12_BLEND_DESC blend = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	blend.RenderTarget[0].BlendEnable = TRUE;
	blend.RenderTarget[0].SrcBlend    = D3D12_BLEND_SRC_ALPHA;
	blend.RenderTarget[0].DestBlend   = D3D12_BLEND_INV_SRC_ALPHA;
	blend.RenderTarget[0].BlendOp     = D3D12_BLEND_OP_ADD;

	D3D12_DEPTH_STENCIL_DESC depth = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	depth.DepthEnable    = TRUE;
	depth.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO; // 半透明のため深度書き込みはしない.

	D3D12_GRAPHICS_PIPELINE_STATE_DESC pso_desc{};
	pso_desc.InputLayout           = { input_layout, _countof(input_layout) };
	pso_desc.pRootSignature        = GetRootSignature().Get();
	pso_desc.VS                    = CD3DX12_SHADER_BYTECODE(vs_blob.Get());
	pso_desc.PS                    = CD3DX12_SHADER_BYTECODE(ps_blob.Get());
	pso_desc.BlendState            = blend;
	pso_desc.DepthStencilState     = depth;
	pso_desc.RasterizerState       = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	pso_desc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	pso_desc.SampleMask            = UINT_MAX;
	pso_desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	pso_desc.NumRenderTargets      = 1;
	pso_desc.RTVFormats[0]         = DXGI_FORMAT_R8G8B8A8_UNORM;
	pso_desc.DSVFormat             = DXGI_FORMAT_D32_FLOAT;
	pso_desc.SampleDesc.Count      = 1;

	if (FAILED(pDevice->CreateGraphicsPipelineState(&pso_desc, IID_PPV_ARGS(GetPipelineState().GetAddressOf()))))
	{
		return false;
	}

	// 動的頂点バッファ(FrameBufferCount分. Mapしっぱなしで毎フレームCPU書き込み).
	for (UINT i = 0; i < 2; ++i) // DirectX12::FrameBufferCount(2)と同じスロット数.
	{
		const auto heap_props = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		const auto res_desc   = CD3DX12_RESOURCE_DESC::Buffer(kMaxVertexCount * sizeof(ParticleVertex));

		if (FAILED(pDevice->CreateCommittedResource(&heap_props, D3D12_HEAP_FLAG_NONE,
			&res_desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
			IID_PPV_ARGS(GetDynamicVertexBuffer(i).GetAddressOf()))))
		{
			return false;
		}

		if (FAILED(GetDynamicVertexBuffer(i)->Map(0, nullptr, reinterpret_cast<void**>(GetMappedVertices(i)))))
		{
			return false;
		}
	}

	return true;
}

void ParticleSystem::Update(float DeltaTime)
{
	std::vector<Particle*> dead_particles;

	// 走査中のAcquire/Releaseは禁止(ObjectPoolの規約)のため、死んだものは後から解放する.
	m_Pool.ForEachActive([&](Particle& particle) {
		if (particle.LifeTime <= 0.0f) { dead_particles.push_back(&particle); return; }

		particle.LifeTime -= DeltaTime;
		particle.Velocity.y -= 9.8f * particle.GravityScale * DeltaTime;
		particle.Position.x += particle.Velocity.x * DeltaTime;
		particle.Position.y += particle.Velocity.y * DeltaTime;
		particle.Position.z += particle.Velocity.z * DeltaTime;

		if (particle.LifeTime <= 0.0f)
		{
			particle.LifeTime = 0.0f;
			dead_particles.push_back(&particle);
		}
	});

	for (Particle* p_dead : dead_particles) { m_Pool.Release(p_dead); }
}

void ParticleSystem::Draw()
{
	if (!GetPipelineState()) { return; }

	DirectX12* p_dx12 = ServiceLocator::Get<DirectX12>();
	CameraManager* p_camera_manager = ServiceLocator::Get<CameraManager>();
	if (!p_dx12 || !p_camera_manager) { return; }

	CameraBase* p_camera = p_camera_manager->GetActive();
	if (!p_camera) { return; }

	// アクティブカメラの右方向でビルボード展開する(縦軸はワールド上方向を使用).
	const DirectX::XMFLOAT3 camera_right = p_camera->GetRight();
	const DirectX::XMFLOAT3 world_up     = { 0.0f, 1.0f, 0.0f };

	const UINT frame_index = p_dx12->GetFrameIndex();
	ParticleVertex* p_vertices = *GetMappedVertices(frame_index);
	if (!p_vertices) { return; }

	size_t vertex_count = 0;

	m_Pool.ForEachActive([&](const Particle& particle) {
		if (particle.LifeTime <= 0.0f || vertex_count + kVertsPerParticle > kMaxVertexCount) { return; }

		// 寿命の後半でフェードさせる.
		const float life_ratio = (particle.MaxLifeTime > 0.0f) ? (particle.LifeTime / particle.MaxLifeTime) : 0.0f;
		const float alpha = std::min(1.0f, life_ratio / kFadeStartRatio);
		const float half_size = particle.Size * 0.5f;

		const DirectX::XMFLOAT3 right = { camera_right.x * half_size, camera_right.y * half_size, camera_right.z * half_size };
		const DirectX::XMFLOAT3 up    = { world_up.x * half_size,    world_up.y * half_size,    world_up.z * half_size };

		// 左下/右下/右上/左上の4隅.
		const DirectX::XMFLOAT3 corners[4] = {
			{ particle.Position.x - right.x - up.x, particle.Position.y - right.y - up.y, particle.Position.z - right.z - up.z },
			{ particle.Position.x + right.x - up.x, particle.Position.y + right.y - up.y, particle.Position.z + right.z - up.z },
			{ particle.Position.x + right.x + up.x, particle.Position.y + right.y + up.y, particle.Position.z + right.z + up.z },
			{ particle.Position.x - right.x + up.x, particle.Position.y - right.y + up.y, particle.Position.z - right.z + up.z },
		};

		DirectX::XMFLOAT4 color = particle.Color;
		color.w *= alpha;

		const size_t base = vertex_count;
		p_vertices[base + 0] = { corners[0], color };
		p_vertices[base + 1] = { corners[1], color };
		p_vertices[base + 2] = { corners[2], color };
		p_vertices[base + 3] = { corners[0], color };
		p_vertices[base + 4] = { corners[2], color };
		p_vertices[base + 5] = { corners[3], color };

		vertex_count += kVertsPerParticle;
	});

	if (vertex_count == 0) { return; }

	ID3D12GraphicsCommandList* p_cmd_list = p_dx12->GetCommandList().Get();
	if (!p_cmd_list) { return; }

	ID3D12Resource* p_scene_cb = p_dx12->GetSceneConstantBuffer();

	p_cmd_list->SetGraphicsRootSignature(GetRootSignature().Get());
	p_cmd_list->SetGraphicsRootConstantBufferView(0, p_scene_cb ? p_scene_cb->GetGPUVirtualAddress() : 0);
	p_cmd_list->SetPipelineState(GetPipelineState().Get());

	const D3D12_VERTEX_BUFFER_VIEW vb_view{
		GetDynamicVertexBuffer(frame_index)->GetGPUVirtualAddress(),
		kMaxVertexCount * sizeof(ParticleVertex),
		sizeof(ParticleVertex),
	};
	p_cmd_list->IASetVertexBuffers(0, 1, &vb_view);
	p_cmd_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	p_cmd_list->DrawInstanced(static_cast<UINT>(vertex_count), 1, 0, 0);
}

// ヒットエフェクト用の既定emit(白い粒子が放射状に散る).
void ParticleSystem::SpawnHitEffect(const DirectX::XMFLOAT3& Position)
{
	EmitterParams params{};
	Emit(Position, params);
}

// 指定位置へ指定パラメータでパーティクルを発生させる(ObjectPool経由. new/deleteなし).
void ParticleSystem::Emit(const DirectX::XMFLOAT3& Position, const EmitterParams& Params)
{
	for (int i = 0; i < Params.Count; ++i)
	{
		Particle* p_particle = m_Pool.Acquire();

		// ランダムな球面方向へ放射する(簡易乱数).
		const float angle     = (static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX)) * DirectX::XM_2PI;
		const float elevation = (static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX)) - 0.5f;
		const float speed     = Params.SpeedMin + (static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX)) * (Params.SpeedMax - Params.SpeedMin);
		const float life      = Params.LifeTimeMin + (static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX)) * (Params.LifeTimeMax - Params.LifeTimeMin);

		p_particle->Position     = Position;
		p_particle->Velocity     = { std::cos(angle) * speed, elevation * speed * 2.0f, std::sin(angle) * speed };
		p_particle->Color        = Params.Color;
		p_particle->LifeTime     = life;
		p_particle->MaxLifeTime  = life;
		p_particle->Size         = Params.Size;
		p_particle->GravityScale = Params.GravityScale;
	}
}
