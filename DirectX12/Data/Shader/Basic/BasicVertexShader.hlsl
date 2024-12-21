#include"..\BasicShaderHeader.hlsli"
Texture2D<float4> tex : register(t0); //0�ԃX���b�g�ɐݒ肳�ꂽ�e�N�X�`��(�x�[�X)
Texture2D<float4> sph : register(t1); //1�ԃX���b�g�ɐݒ肳�ꂽ�e�N�X�`��(��Z)
Texture2D<float4> spa : register(t2); //2�ԃX���b�g�ɐݒ肳�ꂽ�e�N�X�`��(���Z)
Texture2D<float4> toon : register(t3); //3�ԃX���b�g�ɐݒ肳�ꂽ�e�N�X�`��(�g�D�[��)

SamplerState smp : register(s0); //0�ԃX���b�g�ɐݒ肳�ꂽ�T���v��
SamplerState smpToon : register(s1); //1�ԃX���b�g�ɐݒ肳�ꂽ�T���v��

// �萔�o�b�t�@0.
cbuffer SceneData : register(b0)
{
    matrix View;
    matrix Proj; // �r���[�v���W�F�N�V�����s��.
    float3 Eye;
};
cbuffer Transform : register(b1)
{
    matrix World;       // ���[���h�ϊ��s��
	matrix Bones[256];  // �{�[���s��.
}

// �萔�o�b�t�@1.
// �}�e���A���p.
cbuffer Material : register(b2)
{
    float4 Diffuse;     // �f�B�t���[�Y�F.
    float4 Specular;    // �X�y�L����.
    float3 Ambient;     // �A���r�G���g.
};

//���_�V�F�[�_���s�N�Z���V�F�[�_�ւ̂����Ɏg�p����
//�\����
struct Output
{
	float4 svpos : SV_POSITION; //�V�X�e���p���_���W
	float4 pos : POSITION; //�V�X�e���p���_���W
	float4 normal : NORMAL0; //�@���x�N�g��
	float4 vnormal : NORMAL1; //�@���x�N�g��
	float2 uv : TEXCOORD; //UV�l
	float3 ray : VECTOR; //�x�N�g��
};

BasicType BasicVS(
    float4 Pos          : POSITION,
    float4 Normal       : NORMAL,
    float2 UV           : TEXCOORD,
    min16uint Weight    : WEIGHT,
    min16uint2 BoneNo   : BONENO)
{ 
    // �s�N�Z���V�F�[�_�֓n���l.
	Output output;
    
    // 0~100��0~1f�Ɋۂ߂�.
	float w = Weight * 0.01f;
    // �{�[������`���.
	matrix Bone = Bones[BoneNo[0]] * w + Bones[BoneNo[1]] * (1 - w);
    // �{�[���s�����Z.
	Pos = mul(Bones[BoneNo[0]], Pos);           
	Pos = mul(World, Pos);
	output.svpos = mul(mul(Proj, View), Pos);   // �V�F�[�_�ł͗�D��Ȃ̂Œ���
	output.pos = mul(View, Pos);
	Normal.w = 0;                               // �����d�v(���s�ړ������𖳌��ɂ���)
	output.normal = mul(World, Normal);         // �@���ɂ����[���h�ϊ����s��
    output.vnormal = mul(View, output.normal);
	output.uv = UV;
	output.ray = normalize(Pos.xyz - mul((float4x3) View, Eye).xyz); //�����x�N�g��

    return output;
}

float4 BasicPS(Output input) : SV_TARGET
{
	float3 light = normalize(float3(1, -1, 1)); //���̌������x�N�g��(���s����)
	float3 lightColor = float3(1, 1, 1); //���C�g�̃J���[(1,1,1�Ő^����)

	//�f�B�t���[�Y�v�Z
	float diffuseB = saturate(dot(-light, input.normal.xyz));
	float4 toonDif = toon.Sample(smpToon, float2(0, 1.0 - diffuseB));

	//���̔��˃x�N�g��
	float3 refLight = normalize(reflect(light, input.normal.xyz));
	float specularB = pow(saturate(dot(refLight, -input.ray)), Specular.a);

	//�X�t�B�A�}�b�v�pUV
	float2 sphereMapUV = input.vnormal.xy;
	sphereMapUV = (sphereMapUV + float2(1, -1)) * float2(0.5, -0.5);

	float4 ambCol = float4(Ambient * 0.6, 1);
	float4 texColor = tex.Sample(smp, input.uv); //�e�N�X�`���J���[
	return saturate((toonDif //�P�x(�g�D�[��)
		* Diffuse + ambCol * 0.5) //�f�B�t���[�Y�F
		* texColor //�e�N�X�`���J���[
		* sph.Sample(smp, sphereMapUV) //�X�t�B�A�}�b�v(��Z)
		+ spa.Sample(smp, sphereMapUV) //�X�t�B�A�}�b�v(���Z)
		+ float4(specularB * Specular.rgb, 1) //�X�y�L�����[
		);
}