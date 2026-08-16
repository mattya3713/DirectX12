using System.Buffers.Binary;
using System.IO;
using System.Numerics;
using System.Text;

namespace ModelViewerExtension;

public sealed class RuntimeFormatException : Exception
{
    public RuntimeFormatException(string message) : base(message) { }
}

public sealed record RuntimeMaterial(
    Vector4 Diffuse,
    Vector3 Specular,
    float SpecularPower,
    Vector3 Ambient,
    bool UseSphereMap,
    bool UseToonMap,
    string BaseColorTexturePath,
    string NormalMapTexturePath,
    string ToonTexturePath,
    string SphereTexturePath);

public sealed record RuntimeBone(string Name, int ParentIndex, Vector3 BindPosition, Quaternion BindRotation, Vector3 BindScale);
public sealed record RuntimeSkinSlot(int BoneIndex, Matrix4x4 OffsetMatrix);
public sealed record RuntimeSkinSubmesh(string MaterialPath, uint IndexCount, uint Role);
public sealed record RuntimeSkinVertex(Vector3 Position, Vector3 Normal, Vector2 UV, ushort[] BoneIndices, float[] BoneWeights);

public sealed class RuntimeSkinModel
{
    public required IReadOnlyList<RuntimeSkinVertex> Vertices { get; init; }
    public required IReadOnlyList<uint> Indices { get; init; }
    public required IReadOnlyList<RuntimeBone> Bones { get; init; }
    public required IReadOnlyList<RuntimeSkinSlot> SkinSlots { get; init; }
    public required IReadOnlyList<RuntimeSkinSubmesh> Submeshes { get; init; }
    public required IReadOnlyList<RuntimeMaterial?> Materials { get; init; }
    public required float FrontCompositeOpacity { get; init; }
    public required float FrontCompositeMaxDistance { get; init; }
}

public static class RuntimeFormatReader
{
    private const uint MsknVersion = 4;
    private const uint MmatVersion = 3;
    private const int MaximumVertices = 16_777_216;

    public static RuntimeSkinModel ReadMskn(string filePath)
    {
        byte[] bytes = File.ReadAllBytes(filePath);
        Reader reader = new(bytes, filePath);
        string magic = reader.ReadAscii(4);
        uint version = reader.ReadUInt32();
        if (magic != "MSKN" || version != MsknVersion)
        {
            throw new RuntimeFormatException($"MSKN version {version} is not supported (expected {MsknVersion}).");
        }

        uint vertexCount = reader.ReadUInt32();
        uint indexCount = reader.ReadUInt32();
        uint boneCount = reader.ReadUInt32();
        uint submeshCount = reader.ReadUInt32();
        uint skinSlotCount = reader.ReadUInt32();
        uint sourceCount = reader.ReadUInt32();
        uint relaxedCount = reader.ReadUInt32();
        float opacity = reader.ReadSingle();
        float maxDistance = reader.ReadSingle();
        ValidateCount(vertexCount, MaximumVertices, "vertex");
        ValidateCount(indexCount, MaximumVertices * 3, "index");
        ValidateCount(boneCount, MaximumVertices, "bone");
        ValidateCount(submeshCount, MaximumVertices, "submesh");
        ValidateCount(skinSlotCount, MaximumVertices, "skin slot");
        if (sourceCount == 0 != (relaxedCount == 0) || !IsFiniteScalar(opacity) || opacity is < 0 or > 1 || !IsFiniteScalar(maxDistance) || maxDistance < 0)
        {
            throw new RuntimeFormatException("MSKN composite metadata is invalid.");
        }
        reader.ValidatePayloadCapacity(vertexCount, 56, "vertex");

        List<RuntimeSkinVertex> vertices = new((int)vertexCount);
        for (uint i = 0; i < vertexCount; ++i)
        {
            Vector3 position = reader.ReadVector3();
            Vector3 normal = reader.ReadVector3();
            Vector2 uv = reader.ReadVector2();
            ushort[] boneIndices = [reader.ReadUInt16(), reader.ReadUInt16(), reader.ReadUInt16(), reader.ReadUInt16()];
            float[] boneWeights = [reader.ReadSingle(), reader.ReadSingle(), reader.ReadSingle(), reader.ReadSingle()];
            if (!IsFinite(position) || !IsFinite(normal) || !IsFinite(uv) || boneWeights.Any(static value => !IsFiniteScalar(value) || value < 0))
            {
                throw new RuntimeFormatException("MSKN vertex contains a non-finite or negative value.");
            }
            vertices.Add(new RuntimeSkinVertex(position, normal, uv, boneIndices, boneWeights));
        }

        reader.ValidatePayloadCapacity(indexCount, 4, "index");
        List<uint> indices = new((int)indexCount);
        for (uint i = 0; i < indexCount; ++i) { indices.Add(reader.ReadUInt32()); }
        reader.ValidatePayloadCapacity(boneCount, 108, "bone");
        List<RuntimeBone> bones = new((int)boneCount);
        for (uint i = 0; i < boneCount; ++i)
        {
            string name = reader.ReadFixedString(64);
            int parent = reader.ReadInt32();
            bones.Add(new RuntimeBone(name, parent, reader.ReadVector3(), reader.ReadQuaternion(), reader.ReadVector3()));
            if (parent != -1 && (parent < 0 || parent >= i)) { throw new RuntimeFormatException("MSKN bone parent order is invalid."); }
        }
        reader.ValidatePayloadCapacity(skinSlotCount, 68, "skin slot");
        List<RuntimeSkinSlot> slots = new((int)skinSlotCount);
        for (uint i = 0; i < skinSlotCount; ++i)
        {
            int boneIndex = reader.ReadInt32();
            Matrix4x4 matrix = reader.ReadMatrix4x4();
            if (boneIndex < 0 || boneIndex >= boneCount) { throw new RuntimeFormatException("MSKN skin slot bone index is out of range."); }
            slots.Add(new RuntimeSkinSlot(boneIndex, matrix));
        }
        reader.ValidatePayloadCapacity(submeshCount, 136, "submesh");
        List<RuntimeSkinSubmesh> submeshes = new((int)submeshCount);
        ulong totalSubmeshIndices = 0;
        for (uint i = 0; i < submeshCount; ++i)
        {
            string path = reader.ReadFixedString(128);
            uint count = reader.ReadUInt32();
            uint role = reader.ReadUInt32();
            if (role > 2 || !TryAdd(totalSubmeshIndices, count, out totalSubmeshIndices)) { throw new RuntimeFormatException("MSKN submesh range is invalid."); }
            submeshes.Add(new RuntimeSkinSubmesh(path, count, role));
        }
        if (totalSubmeshIndices != indexCount) { throw new RuntimeFormatException("MSKN submesh index counts do not cover the index buffer."); }
        foreach (RuntimeSkinVertex vertex in vertices)
        {
            for (int i = 0; i < vertex.BoneIndices.Length; ++i)
            {
                if (vertex.BoneWeights[i] > 0.00001f && vertex.BoneIndices[i] >= skinSlotCount) { throw new RuntimeFormatException("MSKN vertex skin slot is out of range."); }
            }
        }
        foreach (uint index in indices) { if (index >= vertexCount) { throw new RuntimeFormatException("MSKN vertex index is out of range."); } }
        if (reader.Remaining != 0) { throw new RuntimeFormatException("MSKN contains an unexpected trailing payload."); }

        List<RuntimeMaterial?> materials = new(submeshes.Count);
        string modelDirectory = Path.GetDirectoryName(filePath) ?? string.Empty;
        string materialDirectory = Path.Combine(modelDirectory, "..", "mmat");
        foreach (RuntimeSkinSubmesh submesh in submeshes)
        {
            string materialPath = Path.GetFullPath(Path.Combine(materialDirectory, submesh.MaterialPath));
            if (!File.Exists(materialPath)) { materialPath = Path.GetFullPath(Path.Combine(modelDirectory, submesh.MaterialPath)); }
            materials.Add(File.Exists(materialPath) ? ReadMmat(materialPath) : null);
        }
        return new RuntimeSkinModel { Vertices = vertices, Indices = indices, Bones = bones, SkinSlots = slots, Submeshes = submeshes, Materials = materials, FrontCompositeOpacity = opacity, FrontCompositeMaxDistance = maxDistance };
    }

    public static RuntimeMaterial ReadMmat(string filePath)
    {
        byte[] bytes = File.ReadAllBytes(filePath);
        Reader reader = new(bytes, filePath);
        if (reader.ReadAscii(4) != "MMAT") { throw new RuntimeFormatException("MMAT magic is invalid."); }
        uint version = reader.ReadUInt32();
        if (version is not (2 or MmatVersion)) { throw new RuntimeFormatException($"MMAT version {version} is not supported (expected 2 or {MmatVersion})."); }
        uint[] lengths = [reader.ReadUInt32(), reader.ReadUInt32(), reader.ReadUInt32(), reader.ReadUInt32()];
        Vector4 diffuse = reader.ReadVector4();
        Vector3 specular = reader.ReadVector3();
        float power = reader.ReadSingle();
        Vector3 ambient = reader.ReadVector3();
        uint useSphere = reader.ReadUInt32();
        uint useToon = version == 2 ? 0 : reader.ReadUInt32();
        if (useSphere > 1 || useToon > 1 || !IsFinite(diffuse) || !IsFinite(specular) || !IsFiniteScalar(power) || !IsFinite(ambient)) { throw new RuntimeFormatException("MMAT value is invalid."); }
        string[] paths = [reader.ReadFixedString(128), reader.ReadFixedString(128), reader.ReadFixedString(128), reader.ReadFixedString(128)];
        for (int i = 0; i < paths.Length; ++i) { if (Encoding.UTF8.GetByteCount(paths[i]) != lengths[i]) { throw new RuntimeFormatException("MMAT texture path length is invalid."); } }
        if (reader.Remaining != 0) { throw new RuntimeFormatException("MMAT contains an unexpected trailing payload."); }
        return new RuntimeMaterial(diffuse, specular, power, ambient, useSphere != 0, version == 2 ? paths[2].Length != 0 : useToon != 0, paths[0], paths[1], paths[2], paths[3]);
    }

    private static void ValidateCount(uint value, int maximum, string label) { if (value > maximum || value > int.MaxValue) { throw new RuntimeFormatException($"MSKN {label} count is unreasonable."); } }
    private static bool TryAdd(ulong left, ulong right, out ulong result) { result = left + right; return result >= left; }
    private static bool IsFiniteScalar(float value) => !float.IsNaN(value) && !float.IsInfinity(value);
    private static bool IsFinite(Vector2 value) => IsFiniteScalar(value.X) && IsFiniteScalar(value.Y);
    private static bool IsFinite(Vector3 value) => IsFiniteScalar(value.X) && IsFiniteScalar(value.Y) && IsFiniteScalar(value.Z);
    private static bool IsFinite(Vector4 value) => IsFiniteScalar(value.X) && IsFiniteScalar(value.Y) && IsFiniteScalar(value.Z) && IsFiniteScalar(value.W);

    private sealed class Reader
    {
        private readonly byte[] m_Bytes;
        private readonly string m_FilePath;
        private int m_Offset;
        public int Remaining => m_Bytes.Length - m_Offset;
        public Reader(byte[] bytes, string filePath) { m_Bytes = bytes; m_FilePath = filePath; }
        private ReadOnlySpan<byte> Take(int count) { if (count < 0 || count > Remaining) { throw new RuntimeFormatException($"{m_FilePath}: truncated or out-of-range field."); } ReadOnlySpan<byte> value = m_Bytes.AsSpan(m_Offset, count); m_Offset += count; return value; }
        public void ValidatePayloadCapacity(uint count, int itemSize, string label) { if ((ulong)count * (ulong)itemSize > (ulong)Remaining) { throw new RuntimeFormatException($"{m_FilePath}: {label} payload is truncated."); } }
        public string ReadAscii(int count) => Encoding.ASCII.GetString(Take(count).ToArray());
        public uint ReadUInt32() => BinaryPrimitives.ReadUInt32LittleEndian(Take(4));
        public int ReadInt32() => BinaryPrimitives.ReadInt32LittleEndian(Take(4));
        public ushort ReadUInt16() => BinaryPrimitives.ReadUInt16LittleEndian(Take(2));
        public float ReadSingle() => BitConverter.ToSingle(Take(4).ToArray(), 0);
        public string ReadFixedString(int capacity) { ReadOnlySpan<byte> data = Take(capacity); int length = data.IndexOf((byte)0); if (length < 0) { throw new RuntimeFormatException($"{m_FilePath}: fixed string is not terminated."); } return Encoding.UTF8.GetString(data.Slice(0, length).ToArray()); }
        public Vector2 ReadVector2() => new(ReadSingle(), ReadSingle());
        public Vector3 ReadVector3() => new(ReadSingle(), ReadSingle(), ReadSingle());
        public Vector4 ReadVector4() => new(ReadSingle(), ReadSingle(), ReadSingle(), ReadSingle());
        public Quaternion ReadQuaternion() => new(ReadSingle(), ReadSingle(), ReadSingle(), ReadSingle());
        public Matrix4x4 ReadMatrix4x4() => new(ReadSingle(), ReadSingle(), ReadSingle(), ReadSingle(), ReadSingle(), ReadSingle(), ReadSingle(), ReadSingle(), ReadSingle(), ReadSingle(), ReadSingle(), ReadSingle(), ReadSingle(), ReadSingle(), ReadSingle(), ReadSingle());
    }
}
