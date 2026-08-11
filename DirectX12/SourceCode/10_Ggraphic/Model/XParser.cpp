#include "XParser.h"

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <unordered_map>

#include "99_Utility/String/FilePath/FilePath.h"

namespace {

	// .xファイル(テキスト形式)用の簡易トークナイザ.
	// ';' ',' 空白は区切り文字として無視し、'{' '}' は単独トークン、
	// '"..."' はクォートを除いた中身を1トークンとして返す(コメント// #にも対応).
	class Tokenizer
	{
	public:
		explicit Tokenizer(std::string Source) noexcept
			: m_Source { std::move(Source) }
		{
		}

		// 次のトークンを取得する(ファイル終端ならfalseを返す).
		bool Next(std::string& OutToken)
		{
			SkipSeparatorsAndComments();
			if (m_Pos >= m_Source.size()) { return false; }

			const char c = m_Source[m_Pos];

			if (c == '{' || c == '}')
			{
				OutToken = std::string(1, c);
				++m_Pos;
				return true;
			}

			if (c == '"')
			{
				size_t close = m_Source.find('"', m_Pos + 1);
				if (close == std::string::npos) { close = m_Source.size() - 1; }
				OutToken = m_Source.substr(m_Pos + 1, close - (m_Pos + 1));
				m_Pos = close + 1;
				return true;
			}

			const size_t start = m_Pos;
			while (m_Pos < m_Source.size() && !IsSeparator(m_Source[m_Pos])
				&& m_Source[m_Pos] != '{' && m_Source[m_Pos] != '}' && m_Source[m_Pos] != '"')
			{
				++m_Pos;
			}
			OutToken = m_Source.substr(start, m_Pos - start);
			return true;
		}

		float NextFloat()
		{
			std::string token;
			Next(token);
			return std::strtof(token.c_str(), nullptr);
		}

		uint32_t NextUInt()
		{
			std::string token;
			Next(token);
			return static_cast<uint32_t>(std::strtoul(token.c_str(), nullptr, 10));
		}

		// 直前に読み取った"{"に対応する"}"まで、中身を無視して読み飛ばす.
		void SkipBlock()
		{
			int depth = 1;
			std::string token;
			while (depth > 0 && Next(token))
			{
				if (token == "{") { ++depth; }
				else if (token == "}") { --depth; }
			}
		}

	private:
		static bool IsSeparator(char c) noexcept
		{
			return c == ';' || c == ',' || std::isspace(static_cast<unsigned char>(c)) != 0;
		}

		void SkipSeparatorsAndComments()
		{
			for (;;)
			{
				while (m_Pos < m_Source.size() && IsSeparator(m_Source[m_Pos])) { ++m_Pos; }

				const bool is_line_comment =
					(m_Pos < m_Source.size() && m_Source[m_Pos] == '#') ||
					(m_Pos + 1 < m_Source.size() && m_Source[m_Pos] == '/' && m_Source[m_Pos + 1] == '/');

				if (!is_line_comment) { return; }

				const size_t newline = m_Source.find('\n', m_Pos);
				m_Pos = (newline == std::string::npos) ? m_Source.size() : newline + 1;
			}
		}

		std::string m_Source;
		size_t      m_Pos = 0;
	};

	using Face = std::vector<uint32_t>; // 面を構成する頂点(または法線)インデックス. 3つ以上(N角形).
	using NamedMaterialTable = std::unordered_map<std::string, Model::Material>; // トップレベルの名前付きMaterial定義.

	// 1つのMeshブロックから読み取った、変換前の生データ.
	struct RawMesh
	{
		std::vector<DirectX::XMFLOAT3> Positions;
		std::vector<Face>              Faces;

		std::vector<DirectX::XMFLOAT3> Normals;
		std::vector<Face>              FaceNormals; // Facesと同じ数、各面の法線インデックス.

		std::vector<DirectX::XMFLOAT2> UVs; // Positionsと同じインデックス空間(面ごとの別インデックスは持たない).

		uint32_t              MaterialCount = 0;
		std::vector<uint32_t> FaceMaterialIndices; // Facesと同じ数、各面が使うマテリアルIndex.
		std::vector<Model::Material> Materials;

		// Frame階層(FrameTransformMatrix)から蓄積された、このMeshをワールド空間へ運ぶ変換行列.
		DirectX::XMFLOAT4X4 Transform = { 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 };
	};

	Face ParseFaceIndices(Tokenizer& Tok)
	{
		Face face;
		const uint32_t index_count = Tok.NextUInt();
		face.resize(index_count);
		for (uint32_t i = 0; i < index_count; ++i) { face[i] = Tok.NextUInt(); }
		return face;
	}

	void ParseMeshNormals(Tokenizer& Tok, RawMesh& OutMesh)
	{
		const uint32_t normal_count = Tok.NextUInt();
		OutMesh.Normals.resize(normal_count);
		for (uint32_t i = 0; i < normal_count; ++i)
		{
			OutMesh.Normals[i].x = Tok.NextFloat();
			OutMesh.Normals[i].y = Tok.NextFloat();
			OutMesh.Normals[i].z = Tok.NextFloat();
		}

		const uint32_t face_normal_count = Tok.NextUInt();
		OutMesh.FaceNormals.resize(face_normal_count);
		for (uint32_t i = 0; i < face_normal_count; ++i)
		{
			OutMesh.FaceNormals[i] = ParseFaceIndices(Tok);
		}

		std::string token;
		Tok.Next(token); // "}"(MeshNormalsの. このブロックに子オブジェクトは無い想定).
	}

	void ParseMeshTextureCoords(Tokenizer& Tok, RawMesh& OutMesh)
	{
		const uint32_t uv_count = Tok.NextUInt();
		OutMesh.UVs.resize(uv_count);
		for (uint32_t i = 0; i < uv_count; ++i)
		{
			OutMesh.UVs[i].x = Tok.NextFloat();
			OutMesh.UVs[i].y = Tok.NextFloat();
		}

		std::string token;
		Tok.Next(token); // "}".
	}

	Model::Material ParseMaterial(Tokenizer& Tok, const std::string& ModelDirPath)
	{
		Model::Material material{};

		material.Diffuse.x = Tok.NextFloat();
		material.Diffuse.y = Tok.NextFloat();
		material.Diffuse.z = Tok.NextFloat();
		material.Diffuse.w = Tok.NextFloat();

		material.SpecularPower = Tok.NextFloat();

		material.Specular.x = Tok.NextFloat();
		material.Specular.y = Tok.NextFloat();
		material.Specular.z = Tok.NextFloat();

		// emissiveColor: Model::MaterialにEmissive相当のフィールドが無いため、
		// 素の黒(0,0,0)より見た目が近くなるようAmbientへ流用する(厳密な等価ではない).
		material.Ambient.x = Tok.NextFloat();
		material.Ambient.y = Tok.NextFloat();
		material.Ambient.z = Tok.NextFloat();

		std::string token;
		while (Tok.Next(token))
		{
			if (token == "}") { break; }

			if (token == "TextureFilename")
			{
				std::string brace;
				Tok.Next(brace); // "{".

				std::string filename;
				Tok.Next(filename); // クォート除去済みのファイル名.

				std::string tex_path = MyFilePath::GetTexPath(ModelDirPath, filename.c_str());
				MyFilePath::ReplaceSlashWithBackslash(&tex_path);
				material.Textures.BaseTexture = tex_path;

				std::string closing;
				Tok.Next(closing); // "}"(TextureFilenameの).
			}
			else
			{
				Tok.SkipBlock();
			}
		}

		return material;
	}

	void ParseMeshMaterialList(Tokenizer& Tok, const std::string& ModelDirPath, const NamedMaterialTable& NamedMaterials, RawMesh& OutMesh)
	{
		OutMesh.MaterialCount = Tok.NextUInt();

		const uint32_t face_index_count = Tok.NextUInt();
		OutMesh.FaceMaterialIndices.resize(face_index_count);
		for (uint32_t i = 0; i < face_index_count; ++i)
		{
			OutMesh.FaceMaterialIndices[i] = Tok.NextUInt();
		}

		std::string token;
		while (Tok.Next(token))
		{
			if (token == "}") { return; }

			if (token == "Material")
			{
				// "Material { ... }"(無名) または "Material name { ... }"(名前付き)のインライン定義.
				std::string next;
				Tok.Next(next);
				if (next != "{")
				{
					std::string brace;
					Tok.Next(brace);
				}
				OutMesh.Materials.push_back(ParseMaterial(Tok, ModelDirPath));
				continue;
			}

			if (token == "{")
			{
				// "{ materialName }" — トップレベルで定義済みの名前付きMaterialへの参照.
				std::string name;
				Tok.Next(name);
				std::string closing;
				Tok.Next(closing); // "}".

				auto it = NamedMaterials.find(name);
				OutMesh.Materials.push_back(it != NamedMaterials.end() ? it->second : Model::Material{});
				continue;
			}

			// 未知の子オブジェクト. 名前付きインスタンスの可能性を考慮してから読み飛ばす.
			std::string next;
			Tok.Next(next);
			if (next != "{") { Tok.Next(next); }
			Tok.SkipBlock();
		}
	}

	void ParseMeshBody(Tokenizer& Tok, const std::string& ModelDirPath, const NamedMaterialTable& NamedMaterials, RawMesh& OutMesh)
	{
		const uint32_t vertex_count = Tok.NextUInt();
		OutMesh.Positions.resize(vertex_count);
		for (uint32_t i = 0; i < vertex_count; ++i)
		{
			OutMesh.Positions[i].x = Tok.NextFloat();
			OutMesh.Positions[i].y = Tok.NextFloat();
			OutMesh.Positions[i].z = Tok.NextFloat();
		}

		const uint32_t face_count = Tok.NextUInt();
		OutMesh.Faces.resize(face_count);
		for (uint32_t i = 0; i < face_count; ++i)
		{
			OutMesh.Faces[i] = ParseFaceIndices(Tok);
		}

		// 子オブジェクト(MeshNormals/MeshTextureCoords/MeshMaterialList等. 出現順は不定).
		std::string token;
		while (Tok.Next(token))
		{
			if (token == "}") { return; }

			std::string next;
			Tok.Next(next);
			if (next != "{")
			{
				// nextは名前付きインスタンスの識別子. さらに次で"{"を読む.
				std::string brace;
				Tok.Next(brace);
				if (brace != "{") { continue; } // 想定外の形式は安全側に倒して無視.
			}

			if (token == "MeshNormals")            { ParseMeshNormals(Tok, OutMesh); }
			else if (token == "MeshTextureCoords")  { ParseMeshTextureCoords(Tok, OutMesh); }
			else if (token == "MeshMaterialList")   { ParseMeshMaterialList(Tok, ModelDirPath, NamedMaterials, OutMesh); }
			else                                    { Tok.SkipBlock(); } // MeshVertexColors等、興味の無いブロック.
		}
	}

	// トップレベル(またはFrame内)を再帰的に走査し、見つかったMeshを全てOutMeshesへ追加する.
	// ParentTransformはこのブロックの親から蓄積されたワールド変換(Frame階層が無ければ単位行列).
	// NamedMaterialsは出現順に蓄積されるため、Meshからの名前参照は定義より後ろで行われる前提
	// (通常のXファイルの並び=トップレベルでMaterialを定義してからFrame/Meshで参照する、に対応).
	void ParseChildren(
		Tokenizer& Tok,
		const std::string& ModelDirPath,
		const DirectX::XMMATRIX& ParentTransform,
		NamedMaterialTable& NamedMaterials,
		std::vector<RawMesh>& OutMeshes)
	{
		DirectX::XMMATRIX local_transform = DirectX::XMMatrixIdentity();

		std::string token;
		while (Tok.Next(token))
		{
			if (token == "}") { return; }

			if (token == "template")
			{
				std::string name, brace;
				Tok.Next(name);
				Tok.Next(brace);
				Tok.SkipBlock();
				continue;
			}

			const std::string block_name = token;

			std::string next;
			if (!Tok.Next(next)) { return; }

			bool has_name = (next != "{");
			std::string instance_name = has_name ? next : std::string{};
			if (has_name)
			{
				// nextは名前付きインスタンスの識別子. さらに次で"{"を読む.
				std::string brace;
				Tok.Next(brace);
				if (brace != "{") { continue; } // 想定外の形式は安全側に倒して無視.
			}

			if (block_name == "Mesh")
			{
				RawMesh mesh{};
				DirectX::XMStoreFloat4x4(&mesh.Transform, DirectX::XMMatrixMultiply(local_transform, ParentTransform));
				ParseMeshBody(Tok, ModelDirPath, NamedMaterials, mesh);
				OutMeshes.push_back(std::move(mesh));
			}
			else if (block_name == "Frame")
			{
				// ボーン階層自体は使わないが、配下にMeshがネストされている場合があるため中身は探索する.
				// このFrame自身のFrameTransformMatrixは子オブジェクトとして下の再帰呼び出しの中で処理される.
				ParseChildren(Tok, ModelDirPath, DirectX::XMMatrixMultiply(local_transform, ParentTransform), NamedMaterials, OutMeshes);
			}
			else if (block_name == "FrameTransformMatrix")
			{
				// array FLOAT matrix[16] (X File仕様: 行優先・行ベクトル前提でDirectXMathとそのまま整合する).
				float m[16];
				for (float& value : m) { value = Tok.NextFloat(); }
				const DirectX::XMFLOAT4X4 loaded_matrix(m);
				local_transform = DirectX::XMLoadFloat4x4(&loaded_matrix);

				std::string closing;
				Tok.Next(closing); // "}".
			}
			else if (block_name == "Material")
			{
				// トップレベル(またはFrame直下)の名前付きMaterial定義. 後続のMeshMaterialListから
				// "{name}"参照で使われる. 名前が無い場合はどこからも参照できないため保存不要.
				Model::Material material = ParseMaterial(Tok, ModelDirPath);
				if (has_name) { NamedMaterials[instance_name] = material; }
			}
			else
			{
				Tok.SkipBlock();
			}
		}
	}

	// 解析済みのRawMesh群を、マテリアルごとに面をグルーピングしながらModel::ModelDataへ変換する.
	// (PMX/PMD同様、IndicesはMaterialのNumFaceCountぶんずつ連続した区間になっている必要があるため).
	void BuildModelData(const std::vector<RawMesh>& RawMeshes, Model::ModelData& OutData)
	{
		for (const RawMesh& raw_mesh : RawMeshes)
		{
			const DirectX::XMMATRIX world = DirectX::XMLoadFloat4x4(&raw_mesh.Transform);

			std::vector<std::vector<Model::Vertex>> vertices_per_material(std::max<size_t>(raw_mesh.MaterialCount, 1));

			for (size_t face_index = 0; face_index < raw_mesh.Faces.size(); ++face_index)
			{
				const Face& face = raw_mesh.Faces[face_index];
				const Face* face_normals = (face_index < raw_mesh.FaceNormals.size()) ? &raw_mesh.FaceNormals[face_index] : nullptr;

				uint32_t material_index = (face_index < raw_mesh.FaceMaterialIndices.size()) ? raw_mesh.FaceMaterialIndices[face_index] : 0;
				if (material_index >= vertices_per_material.size()) { material_index = 0; }

				// N角形をファン三角形分割する(0, i, i+1).
				for (size_t i = 1; i + 1 < face.size(); ++i)
				{
					const size_t corner_local[3] = { 0, i, i + 1 };
					for (size_t corner : corner_local)
					{
						const uint32_t position_index = face[corner];

						Model::Vertex vertex{};
						// ボーン無し(スキニング無し)を明示する. Model::Vertexの既定値(BoneWeights[0]=1.0)の
						// ままだと「ボーン0に全ウェイト」という意味になってしまうため、明示的に0クリアする.
						vertex.BoneWeights[0] = 0.0f;

						if (position_index < raw_mesh.Positions.size())
						{
							DirectX::XMVECTOR pos = DirectX::XMLoadFloat3(&raw_mesh.Positions[position_index]);
							pos = DirectX::XMVector3Transform(pos, world);
							DirectX::XMStoreFloat3(&vertex.Position, pos);
						}
						if (position_index < raw_mesh.UVs.size()) { vertex.UV = raw_mesh.UVs[position_index]; }

						if (face_normals && corner < face_normals->size())
						{
							const uint32_t normal_index = (*face_normals)[corner];
							if (normal_index < raw_mesh.Normals.size())
							{
								DirectX::XMVECTOR normal = DirectX::XMLoadFloat3(&raw_mesh.Normals[normal_index]);
								normal = DirectX::XMVector3TransformNormal(normal, world);
								normal = DirectX::XMVector3Normalize(normal);
								DirectX::XMStoreFloat3(&vertex.Normal, normal);
							}
						}

						vertices_per_material[material_index].push_back(vertex);
					}
				}
			}

			for (size_t material_index = 0; material_index < vertices_per_material.size(); ++material_index)
			{
				std::vector<Model::Vertex>& verts = vertices_per_material[material_index];
				if (verts.empty()) { continue; }

				Model::Material material = (material_index < raw_mesh.Materials.size()) ? raw_mesh.Materials[material_index] : Model::Material{};
				material.NumFaceCount = static_cast<uint32_t>(verts.size());

				const uint32_t vertex_base = static_cast<uint32_t>(OutData.Vertices.size());
				OutData.Vertices.insert(OutData.Vertices.end(), verts.begin(), verts.end());
				for (uint32_t i = 0; i < verts.size(); ++i)
				{
					OutData.Indices.push_back(vertex_base + i);
				}

				OutData.Materials.push_back(material);
			}
		}
	}

} // namespace

// .xファイル(テキスト形式)を読み込み、Model::ModelDataへ変換する.
bool XParser::Load(const std::string& FilePath, Model::ModelData& OutData)
{
	std::ifstream file(FilePath, std::ios::binary);
	if (!file.is_open())
	{
		throw std::runtime_error(FilePath + "ファイルを開くことができませんでした。");
	}

	std::string header_line;
	std::getline(file, header_line);
	if (header_line.compare(0, 3, "xof") != 0)
	{
		throw std::runtime_error(FilePath + "は.xファイル(xofヘッダー)ではありません。");
	}

	std::ostringstream body_stream;
	body_stream << file.rdbuf();
	Tokenizer tokenizer(body_stream.str());

	std::string model_dir_path;
	const size_t last_slash = FilePath.find_last_of("/\\");
	if (last_slash != std::string::npos)
	{
		model_dir_path = FilePath.substr(0, last_slash + 1);
	}

	std::vector<RawMesh> raw_meshes;
	NamedMaterialTable named_materials;
	ParseChildren(tokenizer, model_dir_path, DirectX::XMMatrixIdentity(), named_materials, raw_meshes);

	if (raw_meshes.empty())
	{
		throw std::runtime_error(FilePath + "にMeshが見つかりませんでした。");
	}

	BuildModelData(raw_meshes, OutData);
	return true;
}
