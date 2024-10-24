#pragma once

//�x���ɂ��Ă̺��ޕ��͂𖳌��ɂ���.4005:�Ē�`.
#pragma warning(disable:4005)


// MEMO : Window���C�u�����ɓ����Ă�min,max���ז��Ȃ̂Ŗ����ɂ���.
//		: Windows.h����ɒ�`���邷�邱�Ƃ���������Ă��邽�߂��̈ʒu.

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <Windows.h>
#include <crtdbg.h>

#include <iostream>
#include <vector>       // �ϒ��z��.	
#include <array>        // �萔���z��.
#include <algorithm>    // �A���S���Y��.
#include <map>          // �}�b�v.
#include <unordered_map>// �}�b�v.
#include <cmath>        // ���w�֐�.
#include <memory>       // �������Ǘ�.
#include <string>       // ������.
#include <fstream>		// �t�@�C�����o��.	

#include "Utility/Assert/Assert.inl"	// HRESULT��trycatch������.
#include "Utility/Macro/Macro.h"		// �}�N��.
#include "Utility/Math/Math.h"			// �Z���n.	

// DirectX12.
#include <D3D12.h>
#include <dxgi1_6.h>
#include <DirectXMath.h>

// �w�b�^�[��ݒ肵�Ȃ��Ă͂Ȃ�Ȃ�.
// �v���p�e�B����w�b�^�[�܂ł̃p�X�̊��ϐ�������$(DXTEXDIR)��C++>�S��>�ǉ��̃C���N���[�h�f�B�e�N�g���ɒǉ�.
#include<DirectXTex.h>

// DirectSound.
#include <dsound.h>


//���C�u�����ǂݍ���.
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
//DirectSound.
#pragma comment( lib, "dsound.lib" )

// ���C�u������ݒ肵�Ȃ��Ă͂Ȃ�Ȃ�.
// $(DXTEX_DIR)\Bin\Desktop_2022_Win10\x64\Debug�������J�[>�S��>�ǉ��̃��C�u�����f�B�e�N�g���ɒǉ�.
#pragma comment( lib,"DirectXTex.lib" )


//=================================================
//	�萔.
//=================================================
static constexpr int	WND_W	= 1280;		// �E�B���h�E�̕�.
static constexpr float	WND_WF	= 1280.f;	// �E�B���h�E�̕�.
static constexpr int	WND_H	= 720;		// �E�B���h�E�̍���.
static constexpr float	WND_HF	= 720.f;	// �E�B���h�E�̍���.
static constexpr int	FPS		= 60;			// �t���[�����[�g.
static constexpr float SNOW_SIZE_MIN = 20.f;	// ��ʂ̍ŏ��T�C�Y.
static constexpr float SNOW_SIZE_MAX = 100.f;	// ��ʂ̍ő�T�C�Y.
