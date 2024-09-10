#include "String.h"
namespace MyString 
{
	// ����̍s�̒l�����o��.
	std::string ExtractAmount(const std::string& str)
	{
		std::istringstream iss(str);
		std::string valueStr;
		std::string typeStr;

		// ','�܂ł̒l�����Ƃ��̌�̌^�����𕪊�.
		if (std::getline(iss, valueStr, ',') && std::getline(iss, typeStr, ';')) {
			// ���s����菜���K�v������ꍇ�͂����ōs��.
			if (typeStr.back() == '\n') {
				typeStr.pop_back();
			}

			// �^��float�̏ꍇ�A�����_���ʂ܂ł𕶎���Ƃ��ĕԂ�.
			if (typeStr == "float") {
				char* end;
				float value = std::strtof(valueStr.c_str(), &end);

				std::ostringstream oss;
				oss << std::fixed << std::setprecision(1) << value;
				return oss.str();
			}

			// �^��bool�̏ꍇ�^�U�l�𕶎���Ƃ��ĕԂ�.
			if (typeStr == "bool") {
				// "true"�܂���"1"�̏ꍇ��true�Ɣ��f
				if (valueStr == "true" || valueStr == "1") {
					return "true";
				}
				else {
					return "false";
				}
			}
			// �l������Ԃ�.
			return valueStr;
		}

		return "";
	}

	// ����̍s�����o��.
	std::string ExtractLine(const std::string& str, int Line)
	{
		// UI���𕶎���Ƃ���istringstream�ɓǂݍ���.
		std::istringstream iss(str);

		// �w�肳�ꂽ�s�܂œǂݔ�΂�.
		std::string line;
		for (int i = 0; i <= Line; ++i)
		{
			if (!std::getline(iss, line))
			{
				return ""; // �s��������Ȃ��ꍇ�͋󕶎����Ԃ�.
			}
		}

		return line;
	}

	// �������float�֕ϊ�.
	float Stof(std::string str)
	{
		try {
			// std::stof ���g���ĕ����񂩂畂�������_���ɕϊ�.
			return std::stof(str); 
		}
	
		// TODO : ��O���������.
		catch (const std::invalid_argument& e) {
			// �����Ȉ������n���ꂽ�ꍇ�̗�O����.
			std::cerr << "������������: " << e.what() << std::endl;
		}
		catch (const std::out_of_range& e) {
			std::cerr << "���l���͈͊O����: " << e.what() << std::endl;
		}
		// �G���[�����������ꍇ�̓f�t�H���g�� 0.0 ��Ԃ�.
		return 0.0f;
	}

	// �������float�֕ϊ�.
	bool Stob(std::string str)
	{
		try {
			// ������true��1�Ȃ�true��Ԃ�.
			if (str == "true" || str == "1") {
				return true;
			}
			else {
				return false;
			}
		}

		// TODO : ��O���������.
		catch (const std::invalid_argument& e) {
			// �����Ȉ������n���ꂽ�ꍇ�̗�O����.
			std::cerr << "������������: " << e.what() << std::endl;
		}
		catch (const std::out_of_range& e) {
			std::cerr << "���l���͈͊O����: " << e.what() << std::endl;
		}
		// �G���[�����������ꍇ�̓f�t�H���g�� 0.0 ��Ԃ�.
		return 0.0f;
	}

	// ���C�h�������}���`�o�C�g�ɕϊ�.
	std::string WStringToString(const std::wstring& wideStr)
	{
		// �ϊ���̃o�b�t�@�T�C�Y���擾.
		int Size = WideCharToMultiByte(CP_THREAD_ACP, 0, wideStr.c_str(), -1, (char*)  nullptr, 0, nullptr, nullptr);
		// �ϊ����ʂ��i�[����o�b�t�@��p��.
		std::string result(Size - 1, 0); // �I�[�� NULL �������܂߂Ȃ��悤�ɂ���.
		WideCharToMultiByte(CP_THREAD_ACP, 0, wideStr.c_str(), -1, &result[0], Size, nullptr, nullptr);
		return result;
	}

	std::wstring StringToWString(const std::string& Str)
	{
		// �ϊ���̃o�b�t�@�T�C�Y���擾.
		int Size = MultiByteToWideChar(CP_THREAD_ACP, 0, Str.c_str(), -1, nullptr, 0);
		// �ϊ����ʂ��i�[����o�b�t�@��p��.
		std::wstring result(Size - 1, 0); // �I�[�� NULL �������܂߂Ȃ��悤�ɂ���.
		MultiByteToWideChar(CP_THREAD_ACP, 0, Str.c_str(), -1, &result[0], Size);
		return result;
	}
}