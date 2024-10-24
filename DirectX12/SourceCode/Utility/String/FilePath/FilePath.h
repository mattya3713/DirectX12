/****************************
*	�t�@�C���p�X�̂��ꂻ��.
*   �S��:���e ����
****/
#pragma once
#include <string>   // string, wstring
#include <utility>  // pair

namespace MyFilePath {
	// �萔�Ƃ��ăZ�p���[�^�[���`
	constexpr char DEFAULT_SPLITTER = '*';

	/*******************************************
	* �e�N�X�`���̃p�X���Z�p���[�^�����ŕ�������.
	*�@@param	Path	: �Ώۂ̃p�X������.
	*�@@param	Splitter: ��؂蕶��.
	*�@@return			: �����O��̕�����y�A.
	*******************************************/
	static inline std::pair<std::string, std::string> SplitFileName(const std::string& Path, const char Splitter = DEFAULT_SPLITTER);

	/*******************************************
	* �t�@�C��������g���q���擾����.
	* @param	Path	: �Ώۂ̃p�X������.
	* @return			: �g���q.
	*******************************************/
	/*static inline std::string GetExtension(const std::string& Path);
	static inline std::wstring GetExtension(const std::wstring& Path);*/

	/*******************************************
	* ���f���̃p�X�ƃe�N�X�`���̃p�X���獇���p�X�𓾂�
	* @param ModelPath	: �A�v���P�[�V�������猩��pmd���f���̃p�X
	* @param TexPath	: PMD���f�����猩���e�N�X�`���̃p�X
	* @return			: �A�v���P�[�V�������猩���e�N�X�`���̃p�X
	*******************************************/
	//static inline std::string GetTexPath(const std::string& ModelPath, const char* TexPath);
}

#include "FilePath.inl" // �C�����C���������܂ރt�@�C�����C���N���[�h.