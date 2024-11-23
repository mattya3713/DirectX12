////////////////////////////////////////////////////////////////////////////////////////////////
//
//  full.fx ver1.2+
//  �쐬: ���͉��P
//�@Modified: Furia
//	�G�b�W�J���[�Ή�
//	�m�[�}���}�b�v�ǉ� (�ύX�����̓m�[�}���}�b�v�Ō����j
//
////////////////////////////////////////////////////////////////////////////////////////////////

//******************************************
//Furia�l�́u����"full.fx"�W�v��full+NormMap���x�[�X�ɁAfull+SpecMap
//less.�l��AlternativeFull
//���ڂ�l��full_ES_pmx(ExcellentShadow2)
//��t�������č쐬�����Ă��������܂����B�Z���t�V���h�E�K�{�ł��B
//******************************************
//�X�y�L�����l�����Z

/////////////////////////////////////////////////////////////////////////////////////////
// �� ExcellentShadow�V�X�e���@�������火

float X_SHADOWPOWER = 1.0;   //�A�N�Z�T���e�Z��
float PMD_SHADOWPOWER = 0.2; //���f���e�Z��


//�X�N���[���V���h�E�}�b�v�擾
shared texture2D ScreenShadowMapProcessed : RENDERCOLORTARGET <
    float2 ViewPortRatio = {1.0,1.0};
    int MipLevels = 8;
    string Format = "D3DFMT_R16F";
>;
sampler2D ScreenShadowMapProcessedSamp = sampler_state {
    texture = <ScreenShadowMapProcessed>;
    MinFilter = LINEAR; MagFilter = LINEAR; MipFilter = NONE;
    AddressU  = CLAMP; AddressV = CLAMP;
};

//SSAO�}�b�v�擾
shared texture2D ExShadowSSAOMapOut : RENDERCOLORTARGET <
    float2 ViewPortRatio = {1.0,1.0};
    int MipLevels = 8;
    string Format = "R16F";
>;

sampler2D ExShadowSSAOMapSamp = sampler_state {
    texture = <ExShadowSSAOMapOut>;
    MinFilter = LINEAR; MagFilter = LINEAR; MipFilter = NONE;
    AddressU  = CLAMP; AddressV = CLAMP;
};

// �X�N���[���T�C�Y
float2 ES_ViewportSize : VIEWPORTPIXELSIZE;
static float2 ES_ViewportOffset = (float2(0.5,0.5)/ES_ViewportSize);

bool Exist_ExcellentShadow : CONTROLOBJECT < string name = "ExcellentShadow.x"; >;
bool Exist_ExShadowSSAO : CONTROLOBJECT < string name = "ExShadowSSAO.x"; >;
float ShadowRate : CONTROLOBJECT < string name = "ExcellentShadow.x"; string item = "Tr"; >;
float3   ES_CameraPos1      : POSITION  < string Object = "Camera"; >;
float es_size0 : CONTROLOBJECT < string name = "ExcellentShadow.x"; string item = "Si"; >;
float4x4 es_mat1 : CONTROLOBJECT < string name = "ExcellentShadow.x"; >;

static float3 es_move1 = float3(es_mat1._41, es_mat1._42, es_mat1._43 );
static float CameraDistance1 = length(ES_CameraPos1 - es_move1); //�J�����ƃV���h�E���S�̋���

// �� ExcellentShadow�V�X�e���@�����܂Ł�
/////////////////////////////////////////////////////////////////////////////////////////



//�e�N�X�`���t�B���^�ꊇ�ݒ�////////////////////////////////////////////////////////////////////
//�ٕ������t�B���^�i�Ȃ߂炩�\���������Ă��Y��@��TEXFILTER_MAXANISOTROPY���ݒ肵�܂��傤�j
//#define TEXFILTER_SETTING ANISOTROPIC
//�o�C���j�A�i�Ȃ߂炩�\���F�W���j
#define TEXFILTER_SETTING LINEAR
//�j�A���X�g�l�C�o�[�i��������\���F�e�N�X�`���̃h�b�g���Y��ɏo��j
//#define TEXFILTER_SETTING POINT

//�ٕ������t�B���^���g���Ƃ��̔{��
//1:Off or 2�̔{���Ŏw��(�傫���قǂȂ߂炩���ʁE���ב�)
//�Ή����Ă��邩�ǂ����́A�n�[�h�E�F�A����iRadeonHD�@��Ȃ�16�܂ł͑Ή����Ă���͂�
#define TEXFILTER_MAXANISOTROPY 32

//�ٕ������t�B���^�ɂ���
//�e�N�X�`����Miplevels��������x����Ƃ���Y��ɂȂ�܂��B
//���e�N�X�`���ǂݍ��݂�MME�ŏ����K�v������܂�
//2�̗ݏ�T�C�Y�̃e�N�X�`���ł���Ɣ�r�I�Â��n�[�h�E�F�A�ɂ��D�����ł��B


//�e�N�X�`���A�h���b�V���O�ꊇ�ݒ�//////////////////////////////////////////////////////////////
//�e�N�X�`���ʂɐݒ肵�����Ƃ��́A�T���v���[�X�e�[�g�L�q���ōs���܂��傤�B
//U�����[�v
#define TEXADDRESS_U WRAP 
//U���~���[�����[�v
//#define TEXADDRESS_U MIRROR
//U��0to1�����FMMD�W��
//#define TEXADDRESS_U CLAMP
//U��-1to1�����F-�����̓~���[�����O
//#define TEXADDRESS_U MIRRORONCE

//V�����[�v
#define TEXADDRESS_V WRAP 
//V���~���[�����[�v
//#define TEXADDRESS_V MIRROR
//V��0to1�����FMMD�W��
//#define TEXADDRESS_V CLAMP
//V��-1to1�����F-�����̓~���[�����O
//#define TEXADDRESS_V MIRRORONCE

// �p�����[�^�錾

// ���@�ϊ��s��
float4x4 WorldViewProjMatrix      : WORLDVIEWPROJECTION;
float4x4 WorldMatrix              : WORLD;
float4x4 ViewMatrix               : VIEW;
float4x4 LightWorldViewProjMatrix : WORLDVIEWPROJECTION < string Object = "Light"; >;

float3   LightDirection    : DIRECTION < string Object = "Light"; >;
float3   CameraPosition    : POSITION  < string Object = "Camera"; >;

// �}�e���A���F
float4   MaterialDiffuse   : DIFFUSE  < string Object = "Geometry"; >;
float3   MaterialAmbient   : AMBIENT  < string Object = "Geometry"; >;
float3   MaterialEmmisive  : EMISSIVE < string Object = "Geometry"; >;
float3   MaterialSpecular  : SPECULAR < string Object = "Geometry"; >;
float    SpecularPower     : SPECULARPOWER < string Object = "Geometry"; >;
float3   MaterialToon      : TOONCOLOR;
// ���C�g�F
float3   LightDiffuse      : DIFFUSE   < string Object = "Light"; >;
float3   LightAmbient      : AMBIENT   < string Object = "Light"; >;
float3   LightSpecular     : SPECULAR  < string Object = "Light"; >;
static float4 DiffuseColor  = MaterialDiffuse  * float4(LightDiffuse, 1.0f);
static float3 AmbientColor  = saturate(MaterialAmbient  * LightAmbient + MaterialEmmisive);
static float3 SpecularColor = MaterialSpecular * LightSpecular;

//�֊s�F
float4	 EgColor;
bool     parthf;   // �p�[�X�y�N�e�B�u�t���O
bool     transp;   // �������t���O
bool	 spadd;    // �X�t�B�A�}�b�v���Z�����t���O
#define SKII1    1500
#define SKII2    8000
#define Toon     2.5

// �I�u�W�F�N�g�̃e�N�X�`��
texture ObjectTexture: MATERIALTEXTURE;
sampler ObjTexSampler = sampler_state {
    texture = <ObjectTexture>;
    MINFILTER = TEXFILTER_SETTING;
    MAGFILTER = TEXFILTER_SETTING;
    MIPFILTER = TEXFILTER_SETTING;
	AddressU = TEXADDRESS_U;
	AddressV = TEXADDRESS_V;
	MAXANISOTROPY = TEXFILTER_MAXANISOTROPY;
};

// �X�t�B�A�}�b�v�̃e�N�X�`��
texture ObjectSphereMap: MATERIALSPHEREMAP;
sampler ObjSphareSampler = sampler_state {
    texture = <ObjectSphereMap>;
    MINFILTER = LINEAR;
    MAGFILTER = LINEAR;
};

//�X�y�L�����}�b�v�p�e�N�X�`��/////////////////////////////////////////////////////////
texture2D SpMap <
    string ResourceName = "sleeves_s.png";
>;
sampler SpMapSamp = sampler_state {
    texture = <SpMap>;
    MINFILTER = LINEAR;
    MAGFILTER = LINEAR;
};
//---------------------------


// MMD�{����sampler���㏑�����Ȃ����߂̋L�q�ł��B�폜�s�B
sampler MMDSamp0 : register(s0);
sampler MMDSamp1 : register(s1);
sampler MMDSamp2 : register(s2);


//�m�[�}���}�b�v�p�e�N�X�`��/////////////////////////////////////////////////////////
//�m�[�}���}�b�v�̍ő�l�ł��B�傫���قǔ��˂��܂���B
#define SpecularPowerFactor1 50
//�m�[�}���}�b�v���g�p����ގ��ԍ����w�肵�܂��傤�iMME���t�@�����X Subset���Q�Ɓj
//#define USE_NORMAL_MAP_MATERIALS1 "8"
texture NormalMapTex1 <
	string ResourceName = "sleeves_n.png";
	int Miplevels = 8;
>;
sampler NormalMapTexSampler1 = sampler_state {
	texture = <NormalMapTex1>;
    MINFILTER = TEXFILTER_SETTING;
    MAGFILTER = TEXFILTER_SETTING;
	MIPFILTER = TEXFILTER_SETTING;
	AddressU = TEXADDRESS_U;
	AddressV = TEXADDRESS_V;
	MAXANISOTROPY = TEXFILTER_MAXANISOTROPY;
};

////////////////////////////////////////////////////////////////////////////////////////////////
//�ڋ�Ԏ擾
float3x3 compute_tangent_frame(float3 Normal, float3 View, float2 UV)
{
  float3 dp1 = ddx(View);
  float3 dp2 = ddy(View);
  float2 duv1 = ddx(UV);
  float2 duv2 = ddy(UV);

  float3x3 M = float3x3(dp1, dp2, cross(dp1, dp2));
  float2x3 inverseM = float2x3(cross(M[1], M[2]), cross(M[2], M[0]));
  float3 Tangent = mul(float2(duv1.x, duv2.x), inverseM);
  float3 Binormal = mul(float2(duv1.y, duv2.y), inverseM);

  return float3x3(normalize(Tangent), normalize(Binormal), Normal);
}
/*
	float3x3 tangentFrame = compute_tangent_frame(In.Normal, In.Eye, In.Tex);
	float3 normal = normalize(mul(2.0f * tex2D(normalSamp, In.Tex) - 1.0f, tangentFrame));
*/

////////////////////////////////////////////////////////////////////////////////////////////////
// �֊s�`��

// ���_�V�F�[�_
float4 ColorRender_VS(float4 Pos : POSITION) : POSITION 
{
    // �J�������_�̃��[���h�r���[�ˉe�ϊ�
    return mul( Pos, WorldViewProjMatrix );
}

// �s�N�Z���V�F�[�_
float4 ColorRender_PS() : COLOR
{
    // ���œh��Ԃ�
    return EgColor;//float4(0,0,0,1);
}

// �֊s�`��p�e�N�j�b�N
technique EdgeTec < string MMDPass = "edge"; > {
    pass DrawEdge {
        AlphaBlendEnable = FALSE;
        AlphaTestEnable  = FALSE;

        VertexShader = compile vs_2_0 ColorRender_VS();
        PixelShader  = compile ps_2_0 ColorRender_PS();
    }
}


///////////////////////////////////////////////////////////////////////////////////////////////
// �e�i��Z���t�V���h�E�j�`��

// ���_�V�F�[�_
float4 Shadow_VS(float4 Pos : POSITION) : POSITION
{
    // �J�������_�̃��[���h�r���[�ˉe�ϊ�
    return mul( Pos, WorldViewProjMatrix );
}

// �s�N�Z���V�F�[�_
float4 Shadow_PS() : COLOR
{
    // �A���r�G���g�F�œh��Ԃ�
    return float4(AmbientColor.rgb, 0.65f);
}

// �e�`��p�e�N�j�b�N
technique ShadowTec < string MMDPass = "shadow"; > {
    pass DrawShadow {
        VertexShader = compile vs_2_0 Shadow_VS();
        PixelShader  = compile ps_2_0 Shadow_PS();
    }
}


///////////////////////////////////////////////////////////////////////////////////////////////
// �I�u�W�F�N�g�`��i�Z���t�V���h�EOFF�j

struct VS_OUTPUT {
    float4 Pos        : POSITION;    // �ˉe�ϊ����W
    float2 Tex        : TEXCOORD1;   // �e�N�X�`��
    float3 Normal     : TEXCOORD2;   // �@��
    float3 Eye        : TEXCOORD3;   // �J�����Ƃ̑��Έʒu
    float2 SpTex      : TEXCOORD4;	 // �X�t�B�A�}�b�v�e�N�X�`�����W
    float4 Color      : COLOR0;      // �f�B�t���[�Y�F

};



// ���_�V�F�[�_
VS_OUTPUT Basic_VS(float4 Pos : POSITION, float3 Normal : NORMAL, float2 Tex : TEXCOORD0, uniform bool useTexture, uniform bool useSphereMap, uniform bool useToon)
{
    VS_OUTPUT Out = (VS_OUTPUT)0;
    
    // �J�������_�̃��[���h�r���[�ˉe�ϊ�
    Out.Pos = mul( Pos, WorldViewProjMatrix );
    
    // �J�����Ƃ̑��Έʒu
    Out.Eye = CameraPosition - mul( Pos, WorldMatrix );
    // ���_�@��
    Out.Normal = normalize( mul( Normal, (float3x3)WorldMatrix ) );
    
    // �f�B�t���[�Y�F�{�A���r�G���g�F �v�Z
    Out.Color.rgb = AmbientColor;
    if ( !useToon ) {
        Out.Color.rgb += max(0,dot( Out.Normal, -LightDirection )) * DiffuseColor.rgb;
    }
    Out.Color.a = DiffuseColor.a;
    Out.Color = saturate( Out.Color );
    
    // �e�N�X�`�����W
    Out.Tex = Tex;
    
    if ( useSphereMap ) {
        // �X�t�B�A�}�b�v�e�N�X�`�����W
        float2 NormalWV = mul( Out.Normal, (float3x3)ViewMatrix );
        Out.SpTex.x = NormalWV.x * 0.5f + 0.5f;
        Out.SpTex.y = NormalWV.y * -0.5f + 0.5f;
    }
    
    return Out;
}

// �s�N�Z���V�F�[�_
float4 Basic_PS(VS_OUTPUT IN, uniform bool useTexture, uniform bool useSphereMap, uniform bool useToon
, uniform sampler DiffuseSamp
) : COLOR0
{
    // �f�B�t���[�Y�F�{�A���r�G���g�F �v�Z
    IN.Color.rgb = AmbientColor;
    if ( !useToon ) {
        IN.Color.rgb += max(0,dot( IN.Normal, -LightDirection )) * DiffuseColor.rgb;
    }
    IN.Color.a = DiffuseColor.a;
    IN.Color = saturate( IN.Color );
    
    // �X�y�L�����F�v�Z
    float3 HalfVector = normalize( normalize(IN.Eye) + -LightDirection );
    float3 Specular = pow( max(0,dot( HalfVector, normalize(IN.Normal) )), SpecularPower ) * SpecularColor;
    
    float4 Color = IN.Color;
    if ( useTexture ) {
        // �e�N�X�`���K�p
        Color *= tex2D( DiffuseSamp, IN.Tex );
    }
    if ( useSphereMap ) {
        // �X�t�B�A�}�b�v�K�p
        if(spadd) Color += tex2D(ObjSphareSampler,IN.SpTex);
        else      Color *= tex2D(ObjSphareSampler,IN.SpTex);
    }
    
    if ( useToon ) {
        // �g�D�[���K�p
        float LightNormal = dot( IN.Normal, -LightDirection );
        Color.rgb *= lerp(MaterialToon, float3(1,1,1), saturate(LightNormal * 16 + 0.5));
    }
    
    // �X�y�L�����K�p
    Color.rgb += Specular;
    
    return Color;
}
// �s�N�Z���V�F�[�_�@�m�[�}���}�b�v�ǉ�
float4 Basic_PS_NormalMap(VS_OUTPUT IN, uniform bool useTexture, uniform bool useSphereMap, uniform bool useToon
, uniform sampler DiffuseSamp
, uniform sampler normalSamp) : COLOR0
{
	float3x3 tangentFrame = compute_tangent_frame(IN.Normal, IN.Eye, IN.Tex);
	float3 normal = normalize(mul(2.0f * tex2D(normalSamp, IN.Tex) - 1.0f, tangentFrame));
	
    IN.Color.rgb = AmbientColor;
    if ( !useToon ) {
        IN.Color.rgb += max(0,dot( normal, -LightDirection )) * DiffuseColor.rgb;
    }
    IN.Color.a = DiffuseColor.a;
    IN.Color = saturate( IN.Color );
	
	float2 NormalWV = mul( normal, (float3x3)ViewMatrix );
	IN.SpTex.x = NormalWV.x * 0.5f + 0.5f;
	IN.SpTex.y = NormalWV.y * -0.5f + 0.5f;
	
    // �X�y�L�����F�v�Z
    float3 HalfVector = normalize( normalize(IN.Eye) + -LightDirection );
    float3 Specular = pow( max(0,dot( HalfVector, normalize(normal) )), SpecularPower ) * SpecularColor;
    
    float4 Color = IN.Color;
    if ( useTexture ) {
        // �e�N�X�`���K�p
        Color *= tex2D( DiffuseSamp, IN.Tex );
    }
    if ( useSphereMap ) {
        // �X�t�B�A�}�b�v�K�p
        if(spadd) Color.rgb += tex2D(ObjSphareSampler,IN.SpTex);
        else      Color.rgb *= tex2D(ObjSphareSampler,IN.SpTex);
    }
    
    if ( useToon ) {
        // �g�D�[���K�p
        float LightNormal = dot( normal, -LightDirection );
        Color.rgb *= lerp(MaterialToon, float3(1,1,1), saturate(LightNormal * 16 + 0.5));
    }
    
    // �X�y�L�����K�p
    Color.rgb += Specular;
    
    return Color;
}

//-----------------------------------------------------------------------------------------------------
// �m�[�}���}�b�v�Ή��� �I�u�W�F�N�g�`��p�e�N�j�b�N�i�A�N�Z�T���p�j
// �s�v�Ȃ��͍̂폜��
technique NormalMap_MainTec0 <
// string Subset = USE_NORMAL_MAP_MATERIALS1;
 string MMDPass = "object"; bool UseTexture = false; bool UseSphereMap = false; bool UseToon = false; > {
    pass DrawObject {
        VertexShader = compile vs_3_0 Basic_VS(false, false, false);
        PixelShader  = compile ps_3_0 Basic_PS_NormalMap(false, false, false,
		ObjTexSampler, NormalMapTexSampler1);
    }
}

technique NormalMap_MainTec1 < 
// string Subset = USE_NORMAL_MAP_MATERIALS1;
 string MMDPass = "object"; bool UseTexture = true; bool UseSphereMap = false; bool UseToon = false; > {
    pass DrawObject {
        VertexShader = compile vs_3_0 Basic_VS(true, false, false);
        PixelShader  = compile ps_3_0 Basic_PS_NormalMap(true, false, false,
		ObjTexSampler, NormalMapTexSampler1);
    }
}

technique NormalMap_MainTec2 < 
// string Subset = USE_NORMAL_MAP_MATERIALS1;
 string MMDPass = "object"; bool UseTexture = false; bool UseSphereMap = true; bool UseToon = false; > {
    pass DrawObject {
        VertexShader = compile vs_3_0 Basic_VS(false, true, false);
        PixelShader  = compile ps_3_0 Basic_PS_NormalMap(false, true, false,
		ObjTexSampler, NormalMapTexSampler1);
    }
}

technique NormalMap_MainTec3 < 
// string Subset = USE_NORMAL_MAP_MATERIALS1;
 string MMDPass = "object"; bool UseTexture = true; bool UseSphereMap = true; bool UseToon = false; > {
    pass DrawObject {
        VertexShader = compile vs_3_0 Basic_VS(true, true, false);
        PixelShader  = compile ps_3_0 Basic_PS_NormalMap(true, true, false,
		ObjTexSampler, NormalMapTexSampler1);
    }
}

// �m�[�}���}�b�v�Ή��� �I�u�W�F�N�g�`��p�e�N�j�b�N�iPMD���f���p�j
technique NormalMap_MainTec4 < 
// string Subset = USE_NORMAL_MAP_MATERIALS1;
 string MMDPass = "object"; bool UseTexture = false; bool UseSphereMap = false; bool UseToon = true; > {
    pass DrawObject {
        VertexShader = compile vs_3_0 Basic_VS(false, false, true);
        PixelShader  = compile ps_3_0 Basic_PS_NormalMap(false, false, true,
		ObjTexSampler, NormalMapTexSampler1);
    }
}

technique NormalMap_MainTec5 < 
// string Subset = USE_NORMAL_MAP_MATERIALS1;
 string MMDPass = "object"; bool UseTexture = true; bool UseSphereMap = false; bool UseToon = true; > {
    pass DrawObject {
        VertexShader = compile vs_3_0 Basic_VS(true, false, true);
        PixelShader  = compile ps_3_0 Basic_PS_NormalMap(true, false, true,
		ObjTexSampler, NormalMapTexSampler1);
    }
}

technique NormalMap_MainTec6 < 
// string Subset = USE_NORMAL_MAP_MATERIALS1;
 string MMDPass = "object"; bool UseTexture = false; bool UseSphereMap = true; bool UseToon = true; > {
    pass DrawObject {
        VertexShader = compile vs_3_0 Basic_VS(false, true, true);
        PixelShader  = compile ps_3_0 Basic_PS_NormalMap(false, true, true,
		ObjTexSampler, NormalMapTexSampler1);
    }
}

technique NormalMap_MainTec7 < 
// string Subset = USE_NORMAL_MAP_MATERIALS1;
 string MMDPass = "object"; bool UseTexture = true; bool UseSphereMap = true; bool UseToon = true; > {
    pass DrawObject {
        VertexShader = compile vs_3_0 Basic_VS(true, true, true);
        PixelShader  = compile ps_3_0 Basic_PS_NormalMap(true, true, true,
		ObjTexSampler, NormalMapTexSampler1);
    }
}

//-----------------------------------------------------------------------------------------------------
// �W���G�~�����[�g
// �I�u�W�F�N�g�`��p�e�N�j�b�N�i�A�N�Z�T���p�j
// �s�v�Ȃ��͍̂폜��
technique MainTec0 < string MMDPass = "object"; bool UseTexture = false; bool UseSphereMap = false; bool UseToon = false; > {
    pass DrawObject {
        VertexShader = compile vs_2_0 Basic_VS(false, false, false);
        PixelShader  = compile ps_2_0 Basic_PS(false, false, false,
		ObjTexSampler);
    }
}

technique MainTec1 < string MMDPass = "object"; bool UseTexture = true; bool UseSphereMap = false; bool UseToon = false; > {
    pass DrawObject {
        VertexShader = compile vs_2_0 Basic_VS(true, false, false);
        PixelShader  = compile ps_2_0 Basic_PS(true, false, false,
		ObjTexSampler);
    }
}

technique MainTec2 < string MMDPass = "object"; bool UseTexture = false; bool UseSphereMap = true; bool UseToon = false; > {
    pass DrawObject {
        VertexShader = compile vs_2_0 Basic_VS(false, true, false);
        PixelShader  = compile ps_2_0 Basic_PS(false, true, false,
		ObjTexSampler);
    }
}

technique MainTec3 < string MMDPass = "object"; bool UseTexture = true; bool UseSphereMap = true; bool UseToon = false; > {
    pass DrawObject {
        VertexShader = compile vs_2_0 Basic_VS(true, true, false);
        PixelShader  = compile ps_2_0 Basic_PS(true, true, false,
		ObjTexSampler);
    }
}

// �I�u�W�F�N�g�`��p�e�N�j�b�N�iPMD���f���p�j
technique MainTec4 < string MMDPass = "object"; bool UseTexture = false; bool UseSphereMap = false; bool UseToon = true; > {
    pass DrawObject {
        VertexShader = compile vs_2_0 Basic_VS(false, false, true);
        PixelShader  = compile ps_2_0 Basic_PS(false, false, true,
		ObjTexSampler);
    }
}

technique MainTec5 < string MMDPass = "object"; bool UseTexture = true; bool UseSphereMap = false; bool UseToon = true; > {
    pass DrawObject {
        VertexShader = compile vs_2_0 Basic_VS(true, false, true);
        PixelShader  = compile ps_2_0 Basic_PS(true, false, true,
		ObjTexSampler);
    }
}

technique MainTec6 < string MMDPass = "object"; bool UseTexture = false; bool UseSphereMap = true; bool UseToon = true; > {
    pass DrawObject {
        VertexShader = compile vs_2_0 Basic_VS(false, true, true);
        PixelShader  = compile ps_2_0 Basic_PS(false, true, true,
		ObjTexSampler);
    }
}

technique MainTec7 < string MMDPass = "object"; bool UseTexture = true; bool UseSphereMap = true; bool UseToon = true; > {
    pass DrawObject {
        VertexShader = compile vs_2_0 Basic_VS(true, true, true);
        PixelShader  = compile ps_2_0 Basic_PS(true, true, true,
		ObjTexSampler);
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////
// �Z���t�V���h�E�pZ�l�v���b�g

struct VS_ZValuePlot_OUTPUT {
    float4 Pos : POSITION;              // �ˉe�ϊ����W
    float4 ShadowMapTex : TEXCOORD0;    // Z�o�b�t�@�e�N�X�`��
};

// ���_�V�F�[�_
VS_ZValuePlot_OUTPUT ZValuePlot_VS( float4 Pos : POSITION )
{
    VS_ZValuePlot_OUTPUT Out = (VS_ZValuePlot_OUTPUT)0;

    // ���C�g�̖ڐ��ɂ�郏�[���h�r���[�ˉe�ϊ�������
    Out.Pos = mul( Pos, LightWorldViewProjMatrix );

    // �e�N�X�`�����W�𒸓_�ɍ��킹��
    Out.ShadowMapTex = Out.Pos;

    return Out;
}

// �s�N�Z���V�F�[�_
float4 ZValuePlot_PS( float4 ShadowMapTex : TEXCOORD0 ) : COLOR
{
    // R�F������Z�l���L�^����
    return float4(ShadowMapTex.z/ShadowMapTex.w,0,0,1);
}

// Z�l�v���b�g�p�e�N�j�b�N
technique ZplotTec < string MMDPass = "zplot"; > {
    pass ZValuePlot {
        AlphaBlendEnable = FALSE;
        VertexShader = compile vs_2_0 ZValuePlot_VS();
        PixelShader  = compile ps_2_0 ZValuePlot_PS();
    }
}


///////////////////////////////////////////////////////////////////////////////////////////////
// �I�u�W�F�N�g�`��i�Z���t�V���h�EON�j

// �V���h�E�o�b�t�@�̃T���v���B"register(s0)"�Ȃ̂�MMD��s0���g���Ă��邩��
sampler DefSampler : register(s0);

struct BufferShadow_OUTPUT {
    float4 Pos      : POSITION;     // �ˉe�ϊ����W
    float4 ZCalcTex : TEXCOORD0;    // Z�l
    float2 Tex      : TEXCOORD1;    // �e�N�X�`��
    float3 Normal   : TEXCOORD2;    // �@��
    float3 Eye      : TEXCOORD3;    // �J�����Ƃ̑��Έʒu
    float2 SpTex    : TEXCOORD4;	 // �X�t�B�A�}�b�v�e�N�X�`�����W
    float4 Color    : COLOR0;       // �f�B�t���[�Y�F

    /////////////////////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////////////////////
    // �� ExcellentShadow�V�X�e���@�������火
    
    float4 ScreenTex : TEXCOORD5;   // �X�N���[�����W
    
    // �� ExcellentShadow�V�X�e���@�����܂Ł�
    /////////////////////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////////////////////
    // BufferShadow_OUTPUT�\���̂̃����o�ɒǉ�
    // �G���[���o��Ƃ���TEXCOORD5��TEXCOORD6�Ȃǂɂ��Ă݂�

};

// ���_�V�F�[�_
BufferShadow_OUTPUT BufferShadow_VS(float4 Pos : POSITION, float4 Normal : NORMAL, float2 Tex : TEXCOORD0, uniform bool useTexture, uniform bool useSphereMap, uniform bool useToon)
{
    BufferShadow_OUTPUT Out = (BufferShadow_OUTPUT)0;

    // �J�������_�̃��[���h�r���[�ˉe�ϊ�
    Out.Pos = mul( Pos, WorldViewProjMatrix );
    
    // �J�����Ƃ̑��Έʒu
    Out.Eye = CameraPosition - mul( Pos, WorldMatrix );
    // ���_�@��
    Out.Normal = normalize( mul( Normal, (float3x3)WorldMatrix ) );
	// ���C�g���_�ɂ�郏�[���h�r���[�ˉe�ϊ�
    Out.ZCalcTex = mul( Pos, LightWorldViewProjMatrix );
    
    // �f�B�t���[�Y�F�{�A���r�G���g�F �v�Z
    Out.Color.rgb = AmbientColor;
    if ( !useToon ) {
        Out.Color.rgb += max(0,dot( Out.Normal, -LightDirection )) * DiffuseColor.rgb;
    }
    Out.Color.a = DiffuseColor.a;
    Out.Color = saturate( Out.Color );
    




    // �e�N�X�`�����W
    Out.Tex = Tex;
    
    if ( useSphereMap ) {
        // �X�t�B�A�}�b�v�e�N�X�`�����W
        float2 NormalWV = mul( Out.Normal, (float3x3)ViewMatrix );
        Out.SpTex.x = NormalWV.x * 0.5f + 0.5f;
        Out.SpTex.y = NormalWV.y * -0.5f + 0.5f;

    }
    /////////////////////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////////////////////
    // �� ExcellentShadow�V�X�e���@�������火
    
    //�X�N���[�����W�擾
    Out.ScreenTex = Out.Pos;
    
    //�����i�ɂ����邿����h�~
    Out.Pos.z -= max(0, (int)((CameraDistance1 - 6000) * 0.04));
    
    // �� ExcellentShadow�V�X�e���@�����܂Ł�
    /////////////////////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////////////////////
    // ���́ureturn Out;�v�̒��O�ɒǉ�
    return Out;
}

// �s�N�Z���V�F�[�_
float4 BufferShadow_PS(BufferShadow_OUTPUT IN, uniform bool useTexture, uniform bool useSphereMap, uniform bool useToon
, uniform sampler DiffuseSamp
) : COLOR
{
    // �f�B�t���[�Y�F�{�A���r�G���g�F �v�Z
    IN.Color.rgb = AmbientColor;
    if ( !useToon ) {
        IN.Color.rgb += max(0,dot( IN.Normal, -LightDirection )) * DiffuseColor.rgb;
    }
    IN.Color.a = DiffuseColor.a;
    IN.Color = saturate( IN.Color );
    
    // �X�y�L�����F�v�Z
    float3 HalfVector = normalize( normalize(IN.Eye) + -LightDirection );
    float3 Specular = pow( max(0,dot( HalfVector, normalize(IN.Normal) )), SpecularPower ) * SpecularColor;

//-------

    float4 Color = IN.Color;
    float4 ShadowColor = float4(AmbientColor, Color.a);  // �e�̐F
    if ( useTexture ) {
        // �e�N�X�`���K�p
        float4 TexColor = tex2D( DiffuseSamp, IN.Tex );
        Color *= TexColor;
        ShadowColor *= TexColor;
    }
    if ( useSphereMap ) {
        // �X�t�B�A�}�b�v�K�p
        float4 TexColor = tex2D(ObjSphareSampler,IN.SpTex);
        if(spadd) {
//            Color += TexColor * spa2m;
//            ShadowColor += TexColor * spa2m;
            Color += TexColor;
            ShadowColor += TexColor;
        } else {
            Color *= TexColor;
            ShadowColor *= TexColor;
        }
    }
    // �X�y�L�����K�p
    Color.rgb += Specular;
    
    // �e�N�X�`�����W�ɕϊ�
    IN.ZCalcTex /= IN.ZCalcTex.w;
    float2 TransTexCoord;
    TransTexCoord.x = (1.0f + IN.ZCalcTex.x)*0.5f;
    TransTexCoord.y = (1.0f - IN.ZCalcTex.y)*0.5f;


    if( any( saturate(TransTexCoord) != TransTexCoord ) ) {
        // �V���h�E�o�b�t�@�O
        return Color;
    } else {
        float comp;
        if(parthf) {
            // �Z���t�V���h�E mode2
            comp=1-saturate(max(IN.ZCalcTex.z-tex2D(DefSampler,TransTexCoord).r , 0.0f)*SKII2*TransTexCoord.y-0.3f);
        } else {
            // �Z���t�V���h�E mode1
            comp=1-saturate(max(IN.ZCalcTex.z-tex2D(DefSampler,TransTexCoord).r , 0.0f)*SKII1-0.3f);
        }
        if ( useToon ) {
            // �g�D�[���K�p
            comp = min(saturate(dot(IN.Normal,-LightDirection)*Toon),comp);
            ShadowColor.rgb *= MaterialToon;
        }
        
        float4 ans = lerp(ShadowColor, Color, comp);
		if( transp ) ans.a = 0.5f;
        return ans;
    }
}

// �s�N�Z���V�F�[�_ �m�[�}���}�b�v�ǉ�
float4 BufferShadow_PS_NormalMap(BufferShadow_OUTPUT IN, uniform bool useTexture, uniform bool useSphereMap, uniform bool useToon
, uniform sampler DiffuseSamp
, uniform sampler normalSamp) : COLOR0
{
	float3x3 tangentFrame = compute_tangent_frame(IN.Normal, IN.Eye, IN.Tex);
	float3 normal = normalize(mul(2.0f * tex2D(normalSamp, IN.Tex) - 1.0f, tangentFrame));
	
    IN.Color.rgb = AmbientColor;
    if ( !useToon ) {
        IN.Color.rgb += max(0,dot( normal, -LightDirection )) * DiffuseColor.rgb;
    }
    IN.Color.a = DiffuseColor.a;
    IN.Color = saturate( IN.Color );
    
	float2 NormalWV = mul( normal, (float3x3)ViewMatrix );
	IN.SpTex.x = NormalWV.x * 0.5f + 0.5f;
	IN.SpTex.y = NormalWV.y * -0.5f + 0.5f;
	
    // �X�y�L�����F�v�Z
//	SpecularColor += 0.15;
    float3 HalfVector = normalize( normalize(IN.Eye) + -LightDirection );
    float3 Specular = pow( max(0,dot( HalfVector, normalize(normal) )), SpecularPower ) * SpecularColor;


//=========================================================================
//full+SpecMap�ڐA�A�X�y�L�����}�b�v�K�p
	float spmaps = tex2D( SpMapSamp,IN.Tex).rgb;
	Specular *= spmaps * 2.0;
//=========================================================================

    float4 Color = IN.Color;
    float4 ShadowColor = float4(AmbientColor, Color.a);  // �e�̐F
    if ( useTexture ) {
        // �e�N�X�`���K�p
        float4 TexColor = tex2D( DiffuseSamp, IN.Tex );
        Color *= TexColor;
        ShadowColor *= TexColor;
    }





    if ( useSphereMap ) {
        // �X�t�B�A�}�b�v�K�p
       float4 TexColor = tex2D(ObjSphareSampler,IN.SpTex);
        if(spadd) {
            Color.rgb += TexColor;//.rgb�ɂ��Ȃ���AL�ňُ킠��A(D9exo)�X�t�B�A�}�b�v�̃}�X�N�폜
            ShadowColor.rgb += TexColor;
        } else {
            Color.rgb *= TexColor;
            ShadowColor *= TexColor;
        }
    }
    // �X�y�L�����K�p
    Color.rgb += Specular;
    
    // �e�N�X�`�����W�ɕϊ�
    IN.ZCalcTex /= IN.ZCalcTex.w;
    float2 TransTexCoord;
    TransTexCoord.x = (1.0f + IN.ZCalcTex.x)*0.5f;
    TransTexCoord.y = (1.0f - IN.ZCalcTex.y)*0.5f;



    /////////////////////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////////////////////
    // �� ExcellentShadow�V�X�e���@�������火
    
    #ifdef MIKUMIKUMOVING
        float4 ShadowColor = float4(saturate(AmbientColor[0].rgb), Color.a);  // �e�̐F
        ShadowColor.rgb *= texColor.rgb;
        if ( useSphereMap ) {
            float3 spcolor = (tex2D(ObjSphareSampler,IN.SubTex.zw).rgb * MultiplySphere.rgb + AddingSphere.rgb);
            if(spadd) ShadowColor.rgb += spcolor;
            else      ShadowColor.rgb *= spcolor;
        }
    #endif
    
    if(Exist_ExcellentShadow){
        
        IN.ScreenTex.xyz /= IN.ScreenTex.w;
        float2 TransScreenTex;
        TransScreenTex.x = (1.0f + IN.ScreenTex.x) * 0.5f;
        TransScreenTex.y = (1.0f - IN.ScreenTex.y) * 0.5f;
        TransScreenTex += ES_ViewportOffset;
        float SadowMapVal = tex2D(ScreenShadowMapProcessedSamp, TransScreenTex).r;
        
        float SSAOMapVal = 0;
        
        if(Exist_ExShadowSSAO){
            SSAOMapVal = tex2D(ExShadowSSAOMapSamp , TransScreenTex).r; //�A�x�擾
        }
        
        #ifdef MIKUMIKUMOVING
            float3 lightdir = LightDirection[0];
            bool toonflag = useToon && usetoontexturemap;
        #else
            float3 lightdir = LightDirection;
            bool toonflag = useToon;
        #endif
        
        if (toonflag) {
            // �g�D�[���K�p
//----------------------------------
//            SadowMapVal = min(saturate(dot(IN.Normal, -lightdir) * 3), SadowMapVal);
            SadowMapVal = min(saturate(dot(normal, -lightdir) * 3), SadowMapVal);
            ShadowColor.rgb *= MaterialToon;

            
            ShadowColor.rgb *= (1 - (1 - ShadowRate) * PMD_SHADOWPOWER);
        }else{
            ShadowColor.rgb *= (1 - (1 - ShadowRate) * X_SHADOWPOWER);
        }


//        float comp;
//            comp=1-saturate(max(IN.ZCalcTex.z-tex2D(DefSampler,TransTexCoord).r , 0.0f)*SKII2*TransTexCoord.y-0.3f);
//            comp = min(saturate(dot(normal,-LightDirection)*Toon),comp);


        
        //�e������SSAO����
        float4 ShadowColor2 = ShadowColor;
        ShadowColor2.rgb -= ((Color.rgb - ShadowColor2.rgb) + 0.3) * SSAOMapVal * 0.2;
        ShadowColor2.rgb = max(ShadowColor2.rgb, 0);//ShadowColor.rgb * 0.5);
        
        //����������SSAO����
        Color = lerp(Color, ShadowColor, saturate(SSAOMapVal * 0.4));
        
        //�ŏI����
        Color = lerp(ShadowColor2, Color, SadowMapVal);
        
        #ifndef MIKUMIKUMOVING
        return Color;
        #endif
        
    }else
    
    // �� ExcellentShadow�V�X�e���@�����܂Ł�
    /////////////////////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////////////////////



    if( any( saturate(TransTexCoord) != TransTexCoord ) ) {
        // �V���h�E�o�b�t�@�O
        return Color;
    } else {
        float comp;
        if(parthf) {
            // �Z���t�V���h�E mode2


            comp=1-saturate(max(IN.ZCalcTex.z-tex2D(DefSampler,TransTexCoord).r , 0.0f)*SKII2*TransTexCoord.y-0.3f);
        } else {
            // �Z���t�V���h�E mode1
            comp=1-saturate(max(IN.ZCalcTex.z-tex2D(DefSampler,TransTexCoord).r , 0.0f)*SKII1-0.3f);
        }
        if ( useToon ) {
            // �g�D�[���K�p
            comp = min(saturate(dot(normal,-LightDirection)*Toon),comp);
            ShadowColor.rgb *= MaterialToon;
        }
        


        float4 ans = lerp(ShadowColor, Color, comp);
		if( transp ) ans.a = 0.5f;

        return ans;
    }
}

//-----------------------------------------------------------------------------------------------------
// �m�[�}���}�b�v�Ή��� �I�u�W�F�N�g�`��p�e�N�j�b�N�i�A�N�Z�T���p�j
// �s�v�Ȃ��͍̂폜��
technique NormalMap_MainTecBS0  <
// string Subset = USE_NORMAL_MAP_MATERIALS1;
 string MMDPass = "object_ss"; bool UseTexture = false; bool UseSphereMap = false; bool UseToon = false; > {
    pass DrawObject {
        VertexShader = compile vs_3_0 BufferShadow_VS(false, false, false);
        PixelShader  = compile ps_3_0 BufferShadow_PS_NormalMap(false, false, false,
		ObjTexSampler, NormalMapTexSampler1);
    }
}

technique NormalMap_MainTecBS1  <
// string Subset = USE_NORMAL_MAP_MATERIALS1;
 string MMDPass = "object_ss"; bool UseTexture = true; bool UseSphereMap = false; bool UseToon = false; > {
    pass DrawObject {
        VertexShader = compile vs_3_0 BufferShadow_VS(true, false, false);
        PixelShader  = compile ps_3_0 BufferShadow_PS_NormalMap(true, false, false,
		ObjTexSampler, NormalMapTexSampler1);
    }
}

technique NormalMap_MainTecBS2  <
// string Subset = USE_NORMAL_MAP_MATERIALS1;
 string MMDPass = "object_ss"; bool UseTexture = false; bool UseSphereMap = true; bool UseToon = false; > {
    pass DrawObject {
        VertexShader = compile vs_3_0 BufferShadow_VS(false, true, false);
        PixelShader  = compile ps_3_0 BufferShadow_PS_NormalMap(false, true, false,
		ObjTexSampler, NormalMapTexSampler1);
    }
}

technique NormalMap_MainTecBS3  <
// string Subset = USE_NORMAL_MAP_MATERIALS1;
 string MMDPass = "object_ss"; bool UseTexture = true; bool UseSphereMap = true; bool UseToon = false; > {
    pass DrawObject {
        VertexShader = compile vs_3_0 BufferShadow_VS(true, true, false);
        PixelShader  = compile ps_3_0 BufferShadow_PS_NormalMap(true, true, false,
		ObjTexSampler, NormalMapTexSampler1);
    }
}

// �I�u�W�F�N�g�`��p�e�N�j�b�N�iPMD���f���p�j
technique NormalMap_MainTecBS4  <
// string Subset = USE_NORMAL_MAP_MATERIALS1;
 string MMDPass = "object_ss"; bool UseTexture = false; bool UseSphereMap = false; bool UseToon = true; > {
    pass DrawObject {
        VertexShader = compile vs_3_0 BufferShadow_VS(false, false, true);
        PixelShader  = compile ps_3_0 BufferShadow_PS_NormalMap(false, false, true,
		ObjTexSampler, NormalMapTexSampler1);
    }
}

technique NormalMap_MainTecBS5  <
// string Subset = USE_NORMAL_MAP_MATERIALS1;
 string MMDPass = "object_ss"; bool UseTexture = true; bool UseSphereMap = false; bool UseToon = true; > {
    pass DrawObject {
        VertexShader = compile vs_3_0 BufferShadow_VS(true, false, true);
        PixelShader  = compile ps_3_0 BufferShadow_PS_NormalMap(true, false, true,
		ObjTexSampler, NormalMapTexSampler1);
    }
}

technique NormalMap_MainTecBS6  <
// string Subset = USE_NORMAL_MAP_MATERIALS1;
 string MMDPass = "object_ss"; bool UseTexture = false; bool UseSphereMap = true; bool UseToon = true; > {
    pass DrawObject {
        VertexShader = compile vs_3_0 BufferShadow_VS(false, true, true);
        PixelShader  = compile ps_3_0 BufferShadow_PS_NormalMap(false, true, true,
		ObjTexSampler, NormalMapTexSampler1);
    }
}

technique NormalMap_MainTecBS7  <
// string Subset = USE_NORMAL_MAP_MATERIALS1;
 string MMDPass = "object_ss"; bool UseTexture = true; bool UseSphereMap = true; bool UseToon = true; > {
    pass DrawObject {
        VertexShader = compile vs_3_0 BufferShadow_VS(true, true, true);
        PixelShader  = compile ps_3_0 BufferShadow_PS_NormalMap(true, true, true,
		ObjTexSampler, NormalMapTexSampler1);
    }
}

//-----------------------------------------------------------------------------------------------------
// �W���G�~�����[�g
// �I�u�W�F�N�g�`��p�e�N�j�b�N�i�A�N�Z�T���p�j
technique MainTecBS0  < string MMDPass = "object_ss"; bool UseTexture = false; bool UseSphereMap = false; bool UseToon = false; > {
    pass DrawObject {
        VertexShader = compile vs_2_0 BufferShadow_VS(false, false, false);
        PixelShader  = compile ps_2_0 BufferShadow_PS(false, false, false,
		ObjTexSampler);
    }
}

technique MainTecBS1  < string MMDPass = "object_ss"; bool UseTexture = true; bool UseSphereMap = false; bool UseToon = false; > {
    pass DrawObject {
        VertexShader = compile vs_2_0 BufferShadow_VS(true, false, false);
        PixelShader  = compile ps_2_0 BufferShadow_PS(true, false, false,
		ObjTexSampler);
    }
}

technique MainTecBS2  < string MMDPass = "object_ss"; bool UseTexture = false; bool UseSphereMap = true; bool UseToon = false; > {
    pass DrawObject {
        VertexShader = compile vs_2_0 BufferShadow_VS(false, true, false);
        PixelShader  = compile ps_2_0 BufferShadow_PS(false, true, false,
		ObjTexSampler);
    }
}

technique MainTecBS3  < string MMDPass = "object_ss"; bool UseTexture = true; bool UseSphereMap = true; bool UseToon = false; > {
    pass DrawObject {
        VertexShader = compile vs_2_0 BufferShadow_VS(true, true, false);
        PixelShader  = compile ps_2_0 BufferShadow_PS(true, true, false,
		ObjTexSampler);
    }
}

// �I�u�W�F�N�g�`��p�e�N�j�b�N�iPMD���f���p�j
technique MainTecBS4  < string MMDPass = "object_ss"; bool UseTexture = false; bool UseSphereMap = false; bool UseToon = true; > {
    pass DrawObject {
        VertexShader = compile vs_2_0 BufferShadow_VS(false, false, true);
        PixelShader  = compile ps_2_0 BufferShadow_PS(false, false, true,
		ObjTexSampler);
    }
}

technique MainTecBS5  < string MMDPass = "object_ss"; bool UseTexture = true; bool UseSphereMap = false; bool UseToon = true; > {
    pass DrawObject {
        VertexShader = compile vs_2_0 BufferShadow_VS(true, false, true);
        PixelShader  = compile ps_2_0 BufferShadow_PS(true, false, true,
		ObjTexSampler);
    }
}

technique MainTecBS6  < string MMDPass = "object_ss"; bool UseTexture = false; bool UseSphereMap = true; bool UseToon = true; > {
    pass DrawObject {
        VertexShader = compile vs_2_0 BufferShadow_VS(false, true, true);
        PixelShader  = compile ps_2_0 BufferShadow_PS(false, true, true,
		ObjTexSampler);
    }
}

technique MainTecBS7  < string MMDPass = "object_ss"; bool UseTexture = true; bool UseSphereMap = true; bool UseToon = true; > {
    pass DrawObject {
        VertexShader = compile vs_2_0 BufferShadow_VS(true, true, true);
        PixelShader  = compile ps_2_0 BufferShadow_PS(true, true, true,
		ObjTexSampler);
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////
