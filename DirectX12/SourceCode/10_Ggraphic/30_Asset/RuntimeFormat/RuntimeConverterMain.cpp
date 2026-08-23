#include "RuntimeConverter.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

namespace {

	void PrintUsage()
	{
		std::cerr << "Usage:\n"
			"  RuntimeConverter.exe x <input.x> <output-directory>\n"
			"  RuntimeConverter.exe pmx <input.pmx> <input.vmd> <output-directory>\n"
			"  RuntimeConverter.exe xstatic <input.x> <output-directory> <material-directory>\n";
	}

	int PrintFailure(const std::string& Message)
	{
		std::cerr << "Error: " << Message << '\n';
		return 1;
	}

	// 実行ファイルの配置からプロジェクトのモデルディレクトリを探す.
	std::filesystem::path FindModelDirectory(const char* p_Executable)
	{
		std::vector<std::filesystem::path> starts;
		std::filesystem::path fallback_model_directory;
		std::error_code error;
		const std::filesystem::path executable_path = std::filesystem::absolute(p_Executable, error);
		if (!error)
		{
			starts.push_back(executable_path.parent_path());
		}
		starts.push_back(std::filesystem::current_path(error));
		for (const std::filesystem::path& start : starts)
		{
			std::filesystem::path directory = start;
			while (!directory.empty())
			{
				if (std::filesystem::is_regular_file(directory / "RuntimeConverter.vcxproj", error) &&
					std::filesystem::is_directory(directory / "Data" / "Model", error))
				{
					return directory / "Data" / "Model";
				}
				const std::filesystem::path model_directory = directory / "Data" / "Model";
				if (std::filesystem::is_directory(model_directory, error))
				{
					if (fallback_model_directory.empty())
					{
						fallback_model_directory = model_directory;
					}
				}
				const std::filesystem::path parent = directory.parent_path();
				if (parent == directory)
				{
					break;
				}
				directory = parent;
			}
		}
		return fallback_model_directory;
	}

	// 指定した拡張子の入力ファイルを再帰的に列挙する.
	std::vector<std::filesystem::path> FindFiles(const std::filesystem::path& Directory, const std::string& Extension)
	{
		std::vector<std::filesystem::path> files;
		std::error_code error;
		if (!std::filesystem::is_directory(Directory, error))
		{
			return files;
		}
		for (const std::filesystem::directory_entry& entry : std::filesystem::recursive_directory_iterator(
			Directory, std::filesystem::directory_options::skip_permission_denied, error))
		{
			std::string actual_extension = entry.path().extension().string();
			std::string expected_extension = Extension;
			std::transform(actual_extension.begin(), actual_extension.end(), actual_extension.begin(), [](unsigned char Character) {
				return static_cast<char>(std::tolower(Character));
			});
			std::transform(expected_extension.begin(), expected_extension.end(), expected_extension.begin(), [](unsigned char Character) {
				return static_cast<char>(std::tolower(Character));
			});
			if (entry.is_regular_file(error) && actual_extension == expected_extension)
			{
				files.push_back(entry.path());
			}
		}
		return files;
	}

	// 変換結果をランタイム形式ごとの出力先へ移動する.
	bool MoveFilesWithExtension(const std::filesystem::path& SourceDirectory,
		const std::filesystem::path& DestinationDirectory, const std::string& Extension)
	{
		std::error_code error;
		std::filesystem::create_directories(DestinationDirectory, error);
		if (error)
		{
			return false;
		}
		for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(SourceDirectory, error))
		{
			if (error)
			{
				return false;
			}
			if (!entry.is_regular_file(error) || entry.path().extension().string() != Extension)
			{
				continue;
			}
		const std::filesystem::path destination = DestinationDirectory / entry.path().filename();
		if (std::filesystem::exists(destination, error))
		{
			if (Extension == ".mmat")
				{
					std::filesystem::remove(entry.path(), error);
				if (error) return false;
				continue;
			}
			std::filesystem::remove(destination, error);
			if (error) return false;
		}
			std::filesystem::rename(entry.path(), destination, error);
			if (error)
			{
				return false;
			}
		}
		return true;
	}

	// 既存マテリアルを次の変換の共有候補として作業先へ反映する.
	bool CopyMaterialsToConversionDirectory(const std::filesystem::path& MaterialDirectory,
		const std::filesystem::path& ConversionDirectory)
	{
		std::error_code error;
		for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(MaterialDirectory, error))
		{
			if (error)
			{
				return false;
			}
			if (entry.is_regular_file(error) && entry.path().extension() == ".mmat")
			{
				const std::filesystem::path destination = ConversionDirectory / entry.path().filename();
				if (!std::filesystem::exists(destination, error))
				{
					std::filesystem::copy_file(entry.path(), destination, std::filesystem::copy_options::skip_existing, error);
					if (error) return false;
				}
			}
		}
		return true;
	}

	// 変換先に残った旧命名のクリップを生成前に除去する.
	bool RemoveGeneratedClips(const std::filesystem::path& Directory)
	{
		std::error_code error;
		if (!std::filesystem::is_directory(Directory, error)) { return !error; }
		for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(Directory, error))
		{
			if (error) { return false; }
			if (entry.is_regular_file(error) && entry.path().extension() == ".mclp")
			{
				std::filesystem::remove(entry.path(), error);
				if (error) { return false; }
			}
		}
		return true;
	}

	// 指定した出力階層に存在する生成ファイルを標準出力へ列挙する.
	void PrintGeneratedFiles(const std::filesystem::path& OutputDirectory)
	{
		std::error_code error;
		for (const std::filesystem::directory_entry& entry : std::filesystem::recursive_directory_iterator(OutputDirectory, error))
		{
			if (entry.is_regular_file(error))
			{
				const std::string extension = entry.path().extension().string();
				if (extension == ".mskn" || extension == ".mclp" || extension == ".mmat")
				{
					std::cout << "  " << entry.path().lexically_relative(OutputDirectory).string() << '\n';
				}
			}
		}
	}

	// プロジェクト内のX/PMX/VMDを走査してランタイム形式へ変換する.
	int ConvertProject(const char* p_Executable)
	{
		const std::filesystem::path model_directory = FindModelDirectory(p_Executable);
		if (model_directory.empty())
		{
			return PrintFailure("Data\\Model directory was not found relative to the executable.");
		}
		const std::filesystem::path x_directory = model_directory / "X";
		const std::filesystem::path pmx_directory = model_directory / "PMX";
		const std::filesystem::path mmdl_directory = model_directory / "mmdl";
		const std::filesystem::path mskin_directory = mmdl_directory / "mskin";
		const std::filesystem::path mstc_directory = mmdl_directory / "mstc";
		const std::filesystem::path mmat_directory = mmdl_directory / "mmat";
		std::error_code error;
		std::filesystem::create_directories(mskin_directory, error);
		std::filesystem::create_directories(mstc_directory, error);
		std::filesystem::create_directories(mmat_directory, error);
		if (error)
		{
			return PrintFailure("runtime output directories could not be created.");
		}
		if (!RemoveGeneratedClips(mskin_directory) || !RemoveGeneratedClips(mstc_directory))
		{
			return PrintFailure("old generated animation clips could not be removed.");
		}
		bool failed = false;
		std::size_t converted_count = 0;
		for (const std::filesystem::path& input : FindFiles(x_directory, ".x"))
		{
			if (!CopyMaterialsToConversionDirectory(mmat_directory, mskin_directory))
			{
				return PrintFailure("existing material candidates could not be prepared.");
			}
			const RuntimeConverter::ConversionResult result = RuntimeConverter::ConvertX(input, mskin_directory);
			for (const std::string& warning : result.Warnings) std::cerr << "Warning: " << warning << '\n';
			if (!result.Success)
			{
				std::cerr << "Error: " << input.string() << ": " << result.Error << '\n';
				failed = true;
				continue;
			}
			if (!MoveFilesWithExtension(mskin_directory, mstc_directory, ".mclp") ||
				!MoveFilesWithExtension(mskin_directory, mmat_directory, ".mmat"))
			{
				std::cerr << "Error: generated X assets could not be placed in mmdl.\n";
				failed = true;
				continue;
			}
			std::cout << "Converted: " << input.string() << '\n';
			++converted_count;
		}
		for (const std::filesystem::path& input : FindFiles(pmx_directory, ".pmx"))
		{
			std::vector<std::filesystem::path> vmd_files;
			for (const std::filesystem::path& vmd : FindFiles(input.parent_path(), ".vmd")) vmd_files.push_back(vmd);
			if (vmd_files.empty()) vmd_files.push_back({});
			for (const std::filesystem::path& vmd : vmd_files)
			{
				if (!CopyMaterialsToConversionDirectory(mmat_directory, mskin_directory))
				{
					return PrintFailure("existing material candidates could not be prepared.");
				}
				const RuntimeConverter::ConversionResult result = RuntimeConverter::ConvertPmx(input, vmd, mskin_directory);
				for (const std::string& warning : result.Warnings) std::cerr << "Warning: " << warning << '\n';
				if (!result.Success)
				{
					std::cerr << "Error: " << input.string() << " + " << vmd.string() << ": " << result.Error << '\n';
					failed = true;
					continue;
				}
				if (!MoveFilesWithExtension(mskin_directory, mstc_directory, ".mclp") ||
					!MoveFilesWithExtension(mskin_directory, mmat_directory, ".mmat"))
				{
					std::cerr << "Error: generated PMX assets could not be placed in mmdl.\n";
					failed = true;
					continue;
				}
				std::cout << "Converted: " << input.string() << " + " << vmd.string() << '\n';
				++converted_count;
			}
		}
		if (!failed && converted_count > 0)
		{
			std::cout << "Generated files:\n";
			PrintGeneratedFiles(mmdl_directory);
		}
		if (converted_count == 0) return PrintFailure("No usable X or PMX assets were converted.");
		return failed ? 1 : 0;
	}

}

int main(int ArgumentCount, char** p_Arguments)
{
	if (ArgumentCount == 1)
	{
		return ConvertProject(p_Arguments[0]);
	}
	if (std::string(p_Arguments[1]) == "--help" || std::string(p_Arguments[1]) == "-h")
	{
		PrintUsage();
		return 0;
	}

	std::filesystem::path input_path;
	std::filesystem::path output_directory;
	RuntimeConverter::ConversionResult result;
	const std::string mode = p_Arguments[1];
	if (mode == "x" && ArgumentCount == 4)
	{
		input_path = p_Arguments[2];
		output_directory = p_Arguments[3];
		if (!std::filesystem::is_regular_file(input_path)) return PrintFailure("入力Xファイルが存在しません。");
		result = RuntimeConverter::ConvertX(input_path, output_directory);
	}
	else if (mode == "pmx" && ArgumentCount == 5)
	{
		input_path = p_Arguments[2];
		output_directory = p_Arguments[4];
		if (!std::filesystem::is_regular_file(input_path) || !std::filesystem::is_regular_file(p_Arguments[3])) return PrintFailure("入力PMX/VMDファイルが存在しません。");
		result = RuntimeConverter::ConvertPmx(input_path, p_Arguments[3], output_directory);
	}
	else if (mode == "xstatic" && (ArgumentCount == 4 || ArgumentCount == 5))
	{
		input_path = p_Arguments[2];
		output_directory = p_Arguments[3];
		const std::filesystem::path material_directory = (ArgumentCount == 5) ? std::filesystem::path(p_Arguments[4]) : std::filesystem::path{};
		if (!std::filesystem::is_regular_file(input_path)) return PrintFailure("入力Xファイルが存在しません。");
		result = RuntimeConverter::ConvertXStatic(input_path, output_directory, material_directory);
		std::cout << "Converted(static): " << input_path.string() << '\n';
		for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(output_directory))
		{
			if (entry.is_regular_file() && entry.path().extension() == ".mstc")
			{
				std::cout << "  " << entry.path().filename().string() << '\n';
			}
		}
		return result.Success ? 0 : PrintFailure(result.Error.empty() ? "変換に失敗しました。" : result.Error);
	}
	else
	{
		PrintUsage();
		return 1;
	}

	for (const std::string& warning : result.Warnings)
	{
		std::cerr << "Warning: " << warning << '\n';
	}
	if (!result.Success)
	{
		return PrintFailure(result.Error.empty() ? "変換に失敗しました。" : result.Error);
	}

	std::cout << "Converted files:\n";
	for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(output_directory))
	{
		if (entry.is_regular_file() && (entry.path().extension() == ".mskn" || entry.path().extension() == ".mmat" || entry.path().extension() == ".mclp"))
		{
			std::cout << "  " << entry.path().filename().string() << '\n';
		}
	}
	return 0;
}
