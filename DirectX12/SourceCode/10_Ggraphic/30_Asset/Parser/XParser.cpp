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

	// 1つのSkinWeightsブロック(まだボーン名を解決していない生データ).
	struct RawSkinWeightBlock
	{
		std::string           BoneName;
		std::vector<uint32_t> VertexIndices; // Positionsと同じインデックス空間.
		std::vector<float>    Weights;
		DirectX::XMFLOAT4X4   OffsetMatrix { 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 };
	};

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
		// SkinWeightBlocksが空(=スキニングされない剛体パーツ)の場合のみ頂点位置へ直接焼き込む.
		DirectX::XMFLOAT4X4 Transform { 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 };

		std::vector<RawSkinWeightBlock> SkinWeightBlocks; // ボーン名はまだ未解決.

		// ResolveSkinning()で解決された、頂点ごとの(SkinSlot Index, ウェイト)候補一覧
		// (Positionsと同じインデックス空間. SkinWeightBlocksが空なら空のまま).
		std::vector<std::vector<std::pair<int32_t, float>>> ResolvedInfluences;
	};

	// 1つのAnimationブロック(対象ボーン名はまだ未解決).
	struct RawBoneAnimation
	{
		std::string BoneName;
		XSkeleton::BoneAnimation Anim;
	};

	// 1つのAnimationSetブロック.
	struct RawAnimationClip
	{
		std::string Name;
		std::vector<RawBoneAnimation> BoneAnimations;
	};

	// 解析全体を通して蓄積していく状態.
	struct ParseContext
	{
		std::string ModelDirPath;
		NamedMaterialTable NamedMaterials;
		std::vector<XSkeleton::Bone> Bones; // Frame階層をそのまま反映したボーン木.
		std::vector<XSkeleton::SkinSlot> SkinSlots; // ResolveSkinning()で(メッシュ,ボーン)の組ごとに追加する.
		std::vector<RawAnimationClip> RawClips;
		std::vector<RawMesh> Meshes;
		uint32_t TicksPerSecond = 4800;
	};

	int32_t FindBoneIndexByName(const ParseContext& Ctx, const std::string& Name)
	{
		for (size_t i = 0; i < Ctx.Bones.size(); ++i)
		{
			if (Ctx.Bones[i].Name == Name) { return static_cast<int32_t>(i); }
		}
		return -1;
	}

	Face ParseFaceIndices(Tokenizer& Tok)
	{
		Face face;
		const uint32_t index_count = Tok.NextUInt();
		face.resize(index_count);
		for (uint32_t i = 0; i < index_count; ++i) { face[i] = Tok.NextUInt(); }
		return face;
	}

	DirectX::XMFLOAT4X4 ParseMatrix16(Tokenizer& Tok)
	{
		float m[16];
		for (float& value : m) { value = Tok.NextFloat(); }
		return DirectX::XMFLOAT4X4(m);
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

	void ParseSkinWeights(Tokenizer& Tok, RawMesh& OutMesh)
	{
		RawSkinWeightBlock block{};

		std::string bone_name;
		Tok.Next(bone_name); // クォート除去済み.
		block.BoneName = bone_name;

		const uint32_t weight_count = Tok.NextUInt();
		block.VertexIndices.resize(weight_count);
		for (uint32_t i = 0; i < weight_count; ++i) { block.VertexIndices[i] = Tok.NextUInt(); }
		block.Weights.resize(weight_count);
		for (uint32_t i = 0; i < weight_count; ++i) { block.Weights[i] = Tok.NextFloat(); }

		block.OffsetMatrix = ParseMatrix16(Tok);

		std::string closing;
		Tok.Next(closing); // "}".

		OutMesh.SkinWeightBlocks.push_back(std::move(block));
	}

	void ParseMeshBody(Tokenizer& Tok, const ParseContext& Ctx, RawMesh& OutMesh)
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

		// 子オブジェクト(MeshNormals/MeshTextureCoords/MeshMaterialList/XSkinMeshHeader/SkinWeights等.
		// 出現順は不定).
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
			else if (token == "MeshMaterialList")   { ParseMeshMaterialList(Tok, Ctx.ModelDirPath, Ctx.NamedMaterials, OutMesh); }
			else if (token == "SkinWeights")        { ParseSkinWeights(Tok, OutMesh); }
			else                                    { Tok.SkipBlock(); } // XSkinMeshHeader/MeshVertexColors等、興味の無いブロック.
		}
	}

	void ParseAnimationKey(Tokenizer& Tok, XSkeleton::BoneAnimation& OutAnim)
	{
		const uint32_t key_type = Tok.NextUInt();
		const uint32_t key_count = Tok.NextUInt();

		for (uint32_t i = 0; i < key_count; ++i)
		{
			const uint32_t time = Tok.NextUInt();
			const uint32_t value_count = Tok.NextUInt();

			float values[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
			for (uint32_t v = 0; v < value_count && v < 4; ++v) { values[v] = Tok.NextFloat(); }
			for (uint32_t v = 4; v < value_count; ++v) { Tok.NextFloat(); } // 想定外に多い場合は読み捨てる.

			switch (key_type)
			{
			case 0: // Rotation(クォータニオン). (w,x,y,z)→(x,y,z,w)への並べ替え+共役(x,y,z反転).
				OutAnim.RotationKeys.push_back({ time, DirectX::XMFLOAT4(-values[1], -values[2], -values[3], values[0]) });
				break;
			case 1: // Scale.
				OutAnim.ScaleKeys.push_back({ time, DirectX::XMFLOAT3(values[0], values[1], values[2]) });
				break;
			case 2: // Position.
				OutAnim.PositionKeys.push_back({ time, DirectX::XMFLOAT3(values[0], values[1], values[2]) });
				break;
			default:
				break; // keyType==4(Matrix)等、このファイルでは未使用のため非対応.
			}
		}

		std::string closing;
		Tok.Next(closing); // "}".
	}

	void ParseAnimation(Tokenizer& Tok, std::vector<RawBoneAnimation>& OutAnimations)
	{
		RawBoneAnimation raw{};

		std::string token;
		while (Tok.Next(token))
		{
			if (token == "}") { break; }

			if (token == "{")
			{
				// "{ FrameName }" — このAnimationが対象とするボーン名.
				std::string name;
				Tok.Next(name);
				std::string closing;
				Tok.Next(closing);
				raw.BoneName = name;
				continue;
			}

			if (token == "AnimationKey")
			{
				std::string next;
				Tok.Next(next);
				if (next != "{")
				{
					std::string brace;
					Tok.Next(brace); // 名前付き("AnimationKey S {"等)の場合の"{".
				}
				ParseAnimationKey(Tok, raw.Anim);
				continue;
			}

			std::string next;
			Tok.Next(next);
			if (next != "{") { Tok.Next(next); }
			Tok.SkipBlock();
		}

		OutAnimations.push_back(std::move(raw));
	}

	void ParseAnimationSet(Tokenizer& Tok, const std::string& Name, std::vector<RawAnimationClip>& OutClips)
	{
		RawAnimationClip clip{};
		clip.Name = Name;

		std::string token;
		while (Tok.Next(token))
		{
			if (token == "}") { break; }

			if (token == "Animation")
			{
				std::string next;
				Tok.Next(next);
				if (next != "{")
				{
					std::string brace;
					Tok.Next(brace);
				}
				ParseAnimation(Tok, clip.BoneAnimations);
				continue;
			}

			std::string next;
			Tok.Next(next);
			if (next != "{") { Tok.Next(next); }
			Tok.SkipBlock();
		}

		OutClips.push_back(std::move(clip));
	}

	// トップレベル(またはFrame内)を再帰的に走査する.
	// ParentTransformはこのブロックの親から蓄積されたワールド変換(スキニングされない剛体パーツ用).
	// ParentBoneIndexはこのブロック自身に対応するCtx.Bones内のIndex(名前無しFrame/トップレベルなら
	// 呼び出し元からそのまま引き継いだ値、無ければ-1).
	// NamedMaterials/Bonesは出現順に蓄積されるため、参照(Mesh→Material、SkinWeights/Animation→Bone名)は
	// 全体を1回走査し終えてから解決する(トップレベルの並び順に依存しないようにするため).
	void ParseChildren(
		Tokenizer& Tok,
		ParseContext& Ctx,
		const DirectX::XMMATRIX& ParentTransform,
		int32_t SelfBoneIndex)
	{
		DirectX::XMMATRIX local_transform = DirectX::XMMatrixIdentity();

		std::string token;
		while (Tok.Next(token))
		{
			if (token == "}") { break; }

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
			if (!Tok.Next(next)) { break; }

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
				ParseMeshBody(Tok, Ctx, mesh);
				Ctx.Meshes.push_back(std::move(mesh));
			}
			else if (block_name == "Frame")
			{
				int32_t child_bone_index = -1;
				if (has_name)
				{
					XSkeleton::Bone bone{};
					bone.Name = instance_name;
					bone.ParentIndex = SelfBoneIndex;
					Ctx.Bones.push_back(std::move(bone));
					child_bone_index = static_cast<int32_t>(Ctx.Bones.size()) - 1;
				}
				ParseChildren(Tok, Ctx, DirectX::XMMatrixMultiply(local_transform, ParentTransform), child_bone_index);
			}
			else if (block_name == "FrameTransformMatrix")
			{
				// array FLOAT matrix[16] (Xファイル仕様: 行優先・行ベクトル前提でDirectXMathとそのまま整合する).
				const DirectX::XMFLOAT4X4 loaded_matrix = ParseMatrix16(Tok);
				local_transform = DirectX::XMLoadFloat4x4(&loaded_matrix);

				std::string closing;
				Tok.Next(closing); // "}".
			}
			else if (block_name == "Material")
			{
				// トップレベル(またはFrame直下)の名前付きMaterial定義. 後続のMeshMaterialListから
				// "{name}"参照で使われる. 名前が無い場合はどこからも参照できないため保存不要.
				Model::Material material = ParseMaterial(Tok, Ctx.ModelDirPath);
				if (has_name) { Ctx.NamedMaterials[instance_name] = material; }
			}
			else if (block_name == "AnimationSet")
			{
				ParseAnimationSet(Tok, instance_name, Ctx.RawClips);
			}
			else if (block_name == "AnimTicksPerSecond")
			{
				Ctx.TicksPerSecond = Tok.NextUInt();
				std::string closing;
				Tok.Next(closing); // "}".
			}
			else
			{
				Tok.SkipBlock();
			}
		}

		// このFrame(=SelfBoneIndex)のFrameTransformMatrixが見つかっていれば、対応するボーンへ反映する
		// (子要素ループを抜けた後にまとめて反映することで、出現順に関わらず正しく設定される).
		if (SelfBoneIndex >= 0 && static_cast<size_t>(SelfBoneIndex) < Ctx.Bones.size())
		{
			DirectX::XMStoreFloat4x4(&Ctx.Bones[SelfBoneIndex].LocalBindMatrix, local_transform);
		}
	}

	// SkinWeightsのボーン名を解決し、頂点ごとの(SkinSlot Index,ウェイト)候補を確定する.
	// matrixOffsetは実際には(メッシュ,ボーン)の組ごとに値が異なりうるため、ボーンではなく
	// このSkinWeightsブロック1件ごとに新しいSkinSlotを割り当てる(詳細はMMdlSkeletonData.h参照).
	void ResolveSkinning(ParseContext& Ctx)
	{
		for (RawMesh& mesh : Ctx.Meshes)
		{
			if (mesh.SkinWeightBlocks.empty()) { continue; }

			std::vector<std::vector<std::pair<int32_t, float>>> per_vertex(mesh.Positions.size());

			for (const RawSkinWeightBlock& block : mesh.SkinWeightBlocks)
			{
				const int32_t bone_index = FindBoneIndexByName(Ctx, block.BoneName);
				if (bone_index < 0) { continue; }

				XSkeleton::SkinSlot slot{};
				slot.BoneIndex = bone_index;
				slot.OffsetMatrix = block.OffsetMatrix;
				slot.MeshTransform = mesh.Transform;
				Ctx.SkinSlots.push_back(slot);
				const int32_t slot_index = static_cast<int32_t>(Ctx.SkinSlots.size()) - 1;

				const size_t count = std::min(block.VertexIndices.size(), block.Weights.size());
				for (size_t i = 0; i < count; ++i)
				{
					const uint32_t vertex_index = block.VertexIndices[i];
					if (vertex_index < per_vertex.size())
					{
						per_vertex[vertex_index].push_back({ slot_index, block.Weights[i] });
					}
				}
			}

			mesh.ResolvedInfluences = std::move(per_vertex);
		}
	}

	// RawClipsのボーン名を解決し、XSkeleton::AnimationClipへ変換する.
	void ResolveAnimationClips(ParseContext& Ctx, std::vector<XSkeleton::AnimationClip>& OutClips)
	{
		for (const RawAnimationClip& raw_clip : Ctx.RawClips)
		{
			XSkeleton::AnimationClip clip{};
			clip.Name = raw_clip.Name;

			for (const RawBoneAnimation& raw_anim : raw_clip.BoneAnimations)
			{
				const int32_t bone_index = FindBoneIndexByName(Ctx, raw_anim.BoneName);
				if (bone_index < 0) { continue; }

				XSkeleton::BoneAnimation anim = raw_anim.Anim;
				anim.BoneIndex = bone_index;

				for (const auto& key : anim.RotationKeys) { clip.MaxTime = std::max(clip.MaxTime, key.Time); }
				for (const auto& key : anim.ScaleKeys)    { clip.MaxTime = std::max(clip.MaxTime, key.Time); }
				for (const auto& key : anim.PositionKeys) { clip.MaxTime = std::max(clip.MaxTime, key.Time); }

				clip.BoneAnimations.push_back(std::move(anim));
			}

			OutClips.push_back(std::move(clip));
		}
	}

	// 解析済みのRawMesh群を、マテリアルごとに面をグルーピングしながらModel::ModelDataへ変換する.
	// (PMX/PMD同様、IndicesはMaterialのNumFaceCountぶんずつ連続した区間になっている必要があるため).
	void BuildModelData(const std::vector<RawMesh>& RawMeshes, Model::ModelData& OutData,
		std::vector<DirectX::XMFLOAT4X4>& OutVertexTransforms)
	{
		for (const RawMesh& raw_mesh : RawMeshes)
		{
			const bool is_skinned = !raw_mesh.ResolvedInfluences.empty();
			const DirectX::XMMATRIX rigid_world = DirectX::XMLoadFloat4x4(&raw_mesh.Transform);

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
						// 既定でボーン無し(スキニング無し)を明示する. Model::Vertexの既定値
						// (BoneWeights[0]=1.0)のままだと「ボーン0に全ウェイト」という意味に
						// なってしまうため、明示的に0クリアする(スキニングされる場合は下で上書きする).
						vertex.BoneWeights[0] = 0.0f;

						if (position_index < raw_mesh.Positions.size())
						{
							DirectX::XMVECTOR pos = DirectX::XMLoadFloat3(&raw_mesh.Positions[position_index]);
							if (!is_skinned) { pos = DirectX::XMVector3Transform(pos, rigid_world); }
							DirectX::XMStoreFloat3(&vertex.Position, pos);
						}
						if (position_index < raw_mesh.UVs.size()) { vertex.UV = raw_mesh.UVs[position_index]; }

						if (face_normals && corner < face_normals->size())
						{
							const uint32_t normal_index = (*face_normals)[corner];
							if (normal_index < raw_mesh.Normals.size())
							{
								DirectX::XMVECTOR normal = DirectX::XMLoadFloat3(&raw_mesh.Normals[normal_index]);
								if (!is_skinned) { normal = DirectX::XMVector3TransformNormal(normal, rigid_world); }
								normal = DirectX::XMVector3Normalize(normal);
								DirectX::XMStoreFloat3(&vertex.Normal, normal);
							}
						}

						if (is_skinned && position_index < raw_mesh.ResolvedInfluences.size())
						{
							// 重み降順に最大4つまで採用し、合計が1になるよう正規化する
							// (XSkinMeshHeaderの実測ではnMaxSkinWeightsPerVertex<=3だったため通常は超過しない).
							std::vector<std::pair<int32_t, float>> influences = raw_mesh.ResolvedInfluences[position_index];
							std::sort(influences.begin(), influences.end(),
								[](const auto& a, const auto& b) { return a.second > b.second; });
							if (influences.size() > 4) { influences.resize(4); }

							float total_weight = 0.0f;
							for (const auto& influence : influences) { total_weight += influence.second; }

							if (total_weight > 0.0001f)
							{
								for (size_t w = 0; w < influences.size(); ++w)
								{
									vertex.BoneIndices[w] = static_cast<uint32_t>(influences[w].first);
									vertex.BoneWeights[w] = influences[w].second / total_weight;
								}
							}
						}

						vertices_per_material[material_index].push_back(vertex);
						OutVertexTransforms.push_back(is_skinned ? raw_mesh.Transform : DirectX::XMFLOAT4X4{ 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 });
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

// .xファイル(テキスト形式)を読み込み、Model::ModelData(+骨組みが不要なら捨てる)へ変換する.
bool XParser::Load(const std::string& FilePath, Model::ModelData& OutData)
{
	XSkeleton::SkeletalData skeleton;
	return LoadSkeletal(FilePath, OutData, skeleton);
}

bool XParser::LoadSkeletal(const std::string& FilePath, Model::ModelData& OutData, XSkeleton::SkeletalData& OutSkeleton)
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

	ParseContext ctx;
	ctx.ModelDirPath = model_dir_path;
	ParseChildren(tokenizer, ctx, DirectX::XMMatrixIdentity(), -1);

	if (ctx.Meshes.empty())
	{
		throw std::runtime_error(FilePath + "にMeshが見つかりませんでした。");
	}

	ResolveSkinning(ctx);
	BuildModelData(ctx.Meshes, OutData, OutSkeleton.VertexTransforms);

	// ボーン名の解決にctx.Bonesを使うため、moveで空にする前に呼ぶこと.
	ResolveAnimationClips(ctx, OutSkeleton.Clips);
	OutSkeleton.Bones = std::move(ctx.Bones);
	OutSkeleton.SkinSlots = std::move(ctx.SkinSlots);
	if (ctx.TicksPerSecond > 0) { OutSkeleton.TicksPerSecond = ctx.TicksPerSecond; }

	return true;
}
