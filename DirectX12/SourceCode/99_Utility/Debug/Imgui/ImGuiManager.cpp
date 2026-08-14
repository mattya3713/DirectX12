#include "ImGuiManager.h"

#include "10_Ggraphic/DirectX/DirectX12.h"
#include "99_Utility/ServiceLocator/ServiceLocator.h"

#include "ImGui/imgui_impl_win32.h"
#include "ImGui/imgui_impl_dx12.h"

#include <type_traits>
#include <cstdio>

// imgui_impl_win32.hはWindows.hへの依存を避けるため、WndProcHandlerの宣言を#if 0で無効化している.
// 実体はimgui_impl_win32.cppにあるため、ここで手動宣言してから呼び出す(公式ヘッダーのコメントに従った書き方).
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);

namespace
{
	// 使用する日本語フォント.
	constexpr char FONT_FILE_PATH[] = "Data\\Font\\NotoSansJP-SemiBold.ttf";
	constexpr float FONT_SIZE = 18.0f;

	// 実行時文字コード(ANSI. このプロジェクトは/utf-8未設定のためCP932相当)のconst char*をUTF-8へ変換する.
	// ImGuiはconst char*をUTF-8前提で解釈するため、ラッパー関数のLabel引数はここを通してから渡す.
	// 呼び出し側は素の日本語リテラルをそのまま書けばよく、IMGUI_JPマクロで包む必要はなくなる.
	std::string ToUtf8(const char* AnsiText)
	{
		int wide_len = MultiByteToWideChar(CP_ACP, 0, AnsiText, -1, nullptr, 0);
		std::wstring wide_text(static_cast<size_t>(wide_len), L'\0');
		MultiByteToWideChar(CP_ACP, 0, AnsiText, -1, wide_text.data(), wide_len);

		int utf8_len = WideCharToMultiByte(CP_UTF8, 0, wide_text.c_str(), -1, nullptr, 0, nullptr, nullptr);
		std::string utf8_text(static_cast<size_t>(utf8_len), '\0');
		WideCharToMultiByte(CP_UTF8, 0, wide_text.c_str(), -1, utf8_text.data(), utf8_len, nullptr, nullptr);

		utf8_text.resize(static_cast<size_t>(utf8_len) - 1); // 末尾のヌル文字分を除く.
		return utf8_text;
	}
}

ImGuiManager::ImGuiManager()
	: m_pDx12         { nullptr }
	, m_cpSrvHeap     {}
	, m_IsInitialized { false }
{
}

ImGuiManager::~ImGuiManager()
{
}

HRESULT ImGuiManager::Init(HWND hWnd, DirectX12& Dx12)
{
	m_pDx12 = &Dx12;

	if (!CreateSrvHeap(Dx12)) { return E_FAIL; }

	ImGui::CreateContext();

	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

	// 日本語フォントを読み込む(見つからない場合はデフォルトフォントのまま続行する).
	io.Fonts->AddFontFromFileTTF(FONT_FILE_PATH, FONT_SIZE, nullptr, io.Fonts->GetGlyphRangesJapanese());

	ImGui::StyleColorsDark();

	if (ImGui_ImplWin32_Init(hWnd) == false) { return E_FAIL; }

	auto device = Dx12.GetDevice();
	bool is_dx12_init = ImGui_ImplDX12_Init(
		device.Get(),
		FRAME_COUNT,
		DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
		m_cpSrvHeap.Get(),
		m_cpSrvHeap->GetCPUDescriptorHandleForHeapStart(),
		m_cpSrvHeap->GetGPUDescriptorHandleForHeapStart());

	if (!is_dx12_init) { return E_FAIL; }

	m_IsInitialized = true;
	return S_OK;
}

void ImGuiManager::Shutdown()
{
	if (!m_IsInitialized) { return; }

	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	m_cpSrvHeap.Reset();
	m_IsInitialized = false;
}

bool ImGuiManager::CreateSrvHeap(DirectX12& Dx12)
{
	D3D12_DESCRIPTOR_HEAP_DESC heap_desc = {};
	heap_desc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	heap_desc.NumDescriptors = 2; // スロット0=フォント, スロット1=オフスクリーンシーンテクスチャ.
	heap_desc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

	auto device = Dx12.GetDevice();
	HRESULT hr = device->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(m_cpSrvHeap.GetAddressOf()));

	return SUCCEEDED(hr);
}

D3D12_CPU_DESCRIPTOR_HANDLE ImGuiManager::GetSceneTextureCpuHandle() const noexcept
{
	D3D12_CPU_DESCRIPTOR_HANDLE handle = m_cpSrvHeap->GetCPUDescriptorHandleForHeapStart();
	handle.ptr += m_pDx12->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	return handle;
}

D3D12_GPU_DESCRIPTOR_HANDLE ImGuiManager::GetSceneTextureGpuHandle() const noexcept
{
	D3D12_GPU_DESCRIPTOR_HANDLE handle = m_cpSrvHeap->GetGPUDescriptorHandleForHeapStart();
	handle.ptr += m_pDx12->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	return handle;
}

void ImGuiManager::NewFrame()
{
	ImGuiManager* p_instance = ServiceLocator::Get<ImGuiManager>();
	if (!p_instance || !p_instance->m_IsInitialized) { return; }

	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
}

void ImGuiManager::Render()
{
	ImGuiManager* p_instance = ServiceLocator::Get<ImGuiManager>();
	if (!p_instance || !p_instance->m_IsInitialized) { return; }

	ImGui::Render();

	auto cmd_list = p_instance->m_pDx12->GetCommandList();

	ID3D12DescriptorHeap* heaps[] = { p_instance->m_cpSrvHeap.Get() };
	cmd_list->SetDescriptorHeaps(1, heaps);

	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), cmd_list.Get());
}

LRESULT ImGuiManager::WndProcHandler(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	ImGuiManager* p_instance = ServiceLocator::Get<ImGuiManager>();
	if (!p_instance || !p_instance->m_IsInitialized) { return 0; }

	return ImGui_ImplWin32_WndProcHandler(hWnd, Msg, wParam, lParam);
}

// テキスト表示.
void ImGuiManager::Text(const char* InText)
{
	const std::string utf8_text = ToUtf8(InText);
	ImGui::Text("%s", utf8_text.c_str());
}

// スライダー表示.
template<typename T>
void ImGuiManager::Slider(const char* Label, T& Value, T ValueMin, T ValueMax, bool IsLabel)
{
	const std::string utf8_label = ToUtf8(Label);

	if (IsLabel)
	{
		ImGui::Text("%s", utf8_label.c_str());
		ImGui::SameLine(SAME_LINE_OFFSET);
	}

	const std::string new_label = "##" + utf8_label;

	if constexpr (std::is_same_v<T, int>)
	{
		ImGui::SliderInt(new_label.c_str(), &Value, ValueMin, ValueMax);
	}
	else if constexpr (std::is_same_v<T, float>)
	{
		ImGui::SliderFloat(new_label.c_str(), &Value, ValueMin, ValueMax);
	}
}
template void ImGuiManager::Slider<int>(const char*, int&, int, int, bool);
template void ImGuiManager::Slider<float>(const char*, float&, float, float, bool);

// スライダー + クリップボードコピーボタン.
template<typename T>
void ImGuiManager::Tweak(const char* Label, T& Value, T ValueMin, T ValueMax, bool IsLabel)
{
	Slider(Label, Value, ValueMin, ValueMax, IsLabel);

	ImGui::SameLine();

	const std::string button_id = "Copy##" + std::string(Label);
	if (ImGui::Button(button_id.c_str()))
	{
		char buffer[64];

		// C++のリテラルとしてそのまま貼り付けられる形式にする(float型は末尾に"f"を付与).
		if constexpr (std::is_same_v<T, int>)
		{
			snprintf(buffer, sizeof(buffer), "%d", Value);
		}
		else if constexpr (std::is_same_v<T, float>)
		{
			snprintf(buffer, sizeof(buffer), "%.6ff", Value);
		}

		ImGui::SetClipboardText(buffer);
	}

	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("%s", ToUtf8("現在の値をC++リテラルとしてコピー").c_str());
	}
}
template void ImGuiManager::Tweak<int>(const char*, int&, int, int, bool);
template void ImGuiManager::Tweak<float>(const char*, float&, float, float, bool);

// 数値入力ボックス表示.
template<typename T>
bool ImGuiManager::Input(const char* Label, T& Value, bool IsLabel, float Step, float StepFast, const char* Format)
{
	const std::string utf8_label = ToUtf8(Label);

	if (IsLabel)
	{
		ImGui::Text("%s", utf8_label.c_str());
		ImGui::SameLine(SAME_LINE_OFFSET);
	}

	const std::string new_label = "##" + utf8_label;

	if constexpr (std::is_same_v<T, int>)
	{
		return ImGui::InputInt(new_label.c_str(), &Value, static_cast<int>(Step), static_cast<int>(StepFast));
	}
	else if constexpr (std::is_same_v<T, float>)
	{
		return ImGui::InputFloat(new_label.c_str(), &Value, Step, StepFast, Format);
	}
	else if constexpr (std::is_same_v<T, std::string>)
	{
		char buffer[256];
		strncpy_s(buffer, Value.c_str(), sizeof(buffer));

		if (ImGui::InputText(new_label.c_str(), buffer, sizeof(buffer)))
		{
			Value = buffer;
			return true;
		}
		return false;
	}
	else
	{
		return false;
	}
}
template bool ImGuiManager::Input<int>(const char*, int&, bool, float, float, const char*);
template bool ImGuiManager::Input<float>(const char*, float&, bool, float, float, const char*);
template bool ImGuiManager::Input<std::string>(const char*, std::string&, bool, float, float, const char*);

// チェックボックス表示.
bool ImGuiManager::CheckBox(const char* Label, bool& Flag, bool IsLabel)
{
	const std::string utf8_label = ToUtf8(Label);

	if (IsLabel)
	{
		ImGui::Text("%s", utf8_label.c_str());
		ImGui::SameLine(SAME_LINE_OFFSET);
	}

	const std::string new_label = "##" + utf8_label;
	return ImGui::Checkbox(new_label.c_str(), &Flag);
}

// コンボボックス表示.
std::string ImGuiManager::Combo(const char* Label, std::string& NowItem, const std::vector<std::string>& List, bool IsLabel, float Space)
{
	const int list_size = static_cast<int>(List.size());
	int select_index = 0;

	for (int i = 0; i < list_size; ++i)
	{
		if (List[i] == NowItem)
		{
			select_index = i;
			break;
		}
	}

	if (IsLabel)
	{
		ImGui::Text("%s", ToUtf8(Label).c_str());
		ImGui::SameLine(Space);
	}

	const std::string new_label = "##" + ToUtf8(Label);
	if (ImGui::BeginCombo(new_label.c_str(), NowItem.c_str()))
	{
		for (int i = 0; i < list_size; ++i)
		{
			bool is_selected = (NowItem == List[i]);

			if (ImGui::Selectable(List[i].c_str(), is_selected))
			{
				select_index = i;
			}

			if (is_selected)
			{
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}

	if (list_size > 0)
	{
		NowItem = List[select_index];
	}
	return NowItem;
}
