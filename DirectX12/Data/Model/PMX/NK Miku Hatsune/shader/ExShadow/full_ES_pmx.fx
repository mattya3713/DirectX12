////////////////////////////////////////////////////////////////////////////////////////////////
//
//  full_ES.fx ver1.4 for ExcellentShadow
//  �쐬: ���͉��P
//  ����: ���ڂ�
//
////////////////////////////////////////////////////////////////////////////////////////////////
// �p�����[�^�錾


// �G�t�F�N�g�̐擪�ɒǉ�
/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
// �� ExcellentShadow�V�X�e���@�������火

float X_SHADOWPOWER = 1.0;   //�A�N�Z�T���e�Z��
float PMD_SHADOWPOWER = 0.2; //���f���e�Z��


//�X�N���[���V���h�E�}�b�v�擾
shared texture2D ScreenShadowMapProcessed : RENDERCOLORTARGET <
    float2 ViewPortRatio = {1.0,1.0};
    int MipLevels = 1;
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
    int MipLevels = 1;
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
/////////////////////////////////////////////////////////////////////////////////////////



float4 EgColor; //MMD�ގ����[�t��J���[�擾�p
float4 SpcColor; //MMD�ގ����[�t��J���[�擾�p


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
float4   EdgeColor         : EDGECOLOR;
// ���C�g�F
float3   LightDiffuse      : DIFFUSE   < string Object = "Light"; >;
float3   LightAmbient      : AMBIENT   < string Object = "Light"; >;
float3   LightSpecular     : SPECULAR  < string Object = "Light"; >;
static float4 DiffuseColor  = MaterialDiffuse  * float4(LightDiffuse, 1.0f);
static float3 AmbientColor  = min(1, MaterialAmbient * LightAmbient + MaterialEmmisive);
static float3 SpecularColor = MaterialSpecular * LightSpecular;

bool     parthf;   // �p�[�X�y�N�e�B�u�t���O
bool     transp;   // �������t���O
bool     spadd;    // �X�t�B�A�}�b�v���Z�����t���O
#define SKII1    1500
#define SKII2    8000
#define Toon     3

// �I�u�W�F�N�g�̃e�N�X�`��
texture ObjectTexture: MATERIALTEXTURE;
sampler ObjTexSampler = sampler_state {
    texture = <ObjectTexture>;
    MINFILTER = ANISOTROPIC;
    MAGFILTER = ANISOTROPIC;
    MIPFILTER = LINEAR;
    MAXANISOTROPY = 16;
};

// �X�t�B�A�}�b�v�̃e�N�X�`��
texture ObjectSphereMap: MATERIALSPHEREMAP;
sampler ObjSphareSampler = sampler_state {
    texture = <ObjectSphereMap>;
    MINFILTER = ANISOTROPIC;
    MAGFILTER = ANISOTROPIC;
    MIPFILTER = LINEAR;
    MAXANISOTROPY = 16;
};

// MMD�{����sampler���㏑�����Ȃ����߂̋L�q�ł��B�폜�s�B
sampler MMDSamp0 : register(s0);
sampler MMDSamp1 : register(s1);
sampler MMDSamp2 : register(s2);

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
    // �֊s�F�œh��Ԃ�
    return EdgeColor;
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
    return float4(EgColor.rgb, 0.65f);
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
    float2 SpTex      : TEXCOORD4;     // �X�t�B�A�}�b�v�e�N�X�`�����W
    float4 Color      : COLOR0;      // �f�B�t���[�Y�F
};

// ���_�V�F�[�_
VS_OUTPUT Basic_VS(float4 Pos : POSITION, float3 Normal : NORMAL, float2 Tex : TEXCOORD0, uniform bool useTexture, uniform bool useSphereMap, uniform bool useToon)
{
    VS_OUTPUT Out = (VS_OUTPUT)0;
    
    // �J�������_�̃��[���h�r���[�ˉe�ϊ�
    Out.Pos = mul( Pos, WorldViewProjMatrix );
    
    // �J�����Ƃ̑��Έʒu
    Out.Eye = CameraPosition - mul( Pos, WorldMatrix ).rgb;
    // ���_�@��
    Out.Normal = normalize( mul( Normal, (float3x3)WorldMatrix ) );
    
    // �f�B�t���[�Y�F�{�A���r�G���g�F �v�Z
    Out.Color.rgb = EgColor.rgb;
    if ( !useToon ) {
        Out.Color.rgb += max(0,dot( Out.Normal, -LightDirection )) * DiffuseColor.rgb;
    }
    Out.Color.a = EgColor.a;
    Out.Color = saturate( Out.Color );
    
    // �e�N�X�`�����W
    Out.Tex = Tex;
    
    if ( useSphereMap ) {
        // �X�t�B�A�}�b�v�e�N�X�`�����W
        float2 NormalWV = mul( Out.Normal, (float3x3)ViewMatrix ).xy;
        Out.SpTex.x = NormalWV.x * 0.5f + 0.5f;
        Out.SpTex.y = NormalWV.y * -0.5f + 0.5f;
    }
    
    return Out;
}

// �s�N�Z���V�F�[�_
float4 Basic_PS(VS_OUTPUT IN, uniform bool useTexture, uniform bool useSphereMap, uniform bool useToon) : COLOR0
{
    // �X�y�L�����F�v�Z
    float3 HalfVector = normalize( normalize(IN.Eye) + -LightDirection );
    float3 Specular = pow( max(0,dot( HalfVector, normalize(IN.Normal) )), SpcColor.a ) * SpcColor.rgb;
    
    float4 Color = IN.Color;
    if ( useTexture ) {
        // �e�N�X�`���K�p
        Color *= tex2D( ObjTexSampler, IN.Tex );
    }
    if ( useSphereMap ) {
        // �X�t�B�A�}�b�v�K�p
        if(spadd) Color.rgb += tex2D(ObjSphareSampler,IN.SpTex).rgb;
        else      Color.rgb *= tex2D(ObjSphareSampler,IN.SpTex).rgb;
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

// �I�u�W�F�N�g�`��p�e�N�j�b�N�i�A�N�Z�T���p�j
// �s�v�Ȃ��͍̂폜��
technique MainTec0 < string MMDPass = "object"; bool UseTexture = false; bool UseSphereMap = false; bool UseToon = false; > {
    pass DrawObject {
        VertexShader = compile vs_2_0 Basic_VS(false, false, false);
        PixelShader  = compile ps_2_0 Basic_PS(false, false, false);
    }
}

technique MainTec1 < string MMDPass = "object"; bool UseTexture = true; bool UseSphereMap = false; bool UseToon = false; > {
    pass DrawObject {
        VertexShader = compile vs_2_0 Basic_VS(true, false, false);
        PixelShader  = compile ps_2_0 Basic_PS(true, false, false);
    }
}

technique MainTec2 < string MMDPass = "object"; bool UseTexture = false; bool UseSphereMap = true; bool UseToon = false; > {
    pass DrawObject {
        VertexShader = compile vs_2_0 Basic_VS(false, true, false);
        PixelShader  = compile ps_2_0 Basic_PS(false, true, false);
    }
}

technique MainTec3 < string MMDPass = "object"; bool UseTexture = true; bool UseSphereMap = true; bool UseToon = false; > {
    pass DrawObject {
        VertexShader = compile vs_2_0 Basic_VS(true, true, false);
        PixelShader  = compile ps_2_0 Basic_PS(true, true, false);
    }
}

// �I�u�W�F�N�g�`��p�e�N�j�b�N�iPMD���f���p�j
technique MainTec4 < string MMDPass = "object"; bool UseTexture = false; bool UseSphereMap = false; bool UseToon = true; > {
    pass DrawObject {
        VertexShader = compile vs_2_0 Basic_VS(false, false, true);
        PixelShader  = compile ps_2_0 Basic_PS(false, false, true);
    }
}

technique MainTec5 < string MMDPass = "object"; bool UseTexture = true; bool UseSphereMap = false; bool UseToon = true; > {
    pass DrawObject {
        VertexShader = compile vs_2_0 Basic_VS(true, false, true);
        PixelShader  = compile ps_2_0 Basic_PS(true, false, true);
    }
}

technique MainTec6 < string MMDPass = "object"; bool UseTexture = false; bool UseSphereMap = true; bool UseToon = true; > {
    pass DrawObject {
        VertexShader = compile vs_2_0 Basic_VS(false, true, true);
        PixelShader  = compile ps_2_0 Basic_PS(false, true, true);
    }
}

technique MainTec7 < string MMDPass = "object"; bool UseTexture = true; bool UseSphereMap = true; bool UseToon = true; > {
    pass DrawObject {
        VertexShader = compile vs_2_0 Basic_VS(true, true, true);
        PixelShader  = compile ps_2_0 Basic_PS(true, true, true);
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
    float2 SpTex    : TEXCOORD4;    // �X�t�B�A�}�b�v�e�N�X�`�����W
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
BufferShadow_OUTPUT BufferShadow_VS(float4 Pos : POSITION, float3 Normal : NORMAL, float2 Tex : TEXCOORD0, uniform bool useTexture, uniform bool useSphereMap, uniform bool useToon)
{
    BufferShadow_OUTPUT Out = (BufferShadow_OUTPUT)0;

    // �J�������_�̃��[���h�r���[�ˉe�ϊ�
    Out.Pos = mul( Pos, WorldViewProjMatrix );
    
    // �J�����Ƃ̑��Έʒu
    Out.Eye = CameraPosition - mul( Pos, WorldMatrix ).rgb;
    // ���_�@��
    Out.Normal = normalize( mul( Normal, (float3x3)WorldMatrix ) );
    // ���C�g���_�ɂ�郏�[���h�r���[�ˉe�ϊ�
    Out.ZCalcTex = mul( Pos, LightWorldViewProjMatrix );
    
    // �f�B�t���[�Y�F�{�A���r�G���g�F �v�Z
    Out.Color.rgb = EgColor.rgb;
    if ( !useToon ) {
        Out.Color.rgb += max(0,dot( Out.Normal, -LightDirection )) * DiffuseColor.rgb;
    }
    Out.Color.a = EgColor.a;
    Out.Color = saturate( Out.Color );
    
    // �e�N�X�`�����W
    Out.Tex = Tex;
    
    if ( useSphereMap ) {
        // �X�t�B�A�}�b�v�e�N�X�`�����W
        float2 NormalWV = mul( Out.Normal, (float3x3)ViewMatrix ).xy;
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
float4 BufferShadow_PS(BufferShadow_OUTPUT IN, uniform bool useTexture, uniform bool useSphereMap, uniform bool useToon) : COLOR
{
    // �X�y�L�����F�v�Z
    float3 HalfVector = normalize( normalize(IN.Eye) + -LightDirection );
    float3 Specular = pow( max(0,dot( HalfVector, normalize(IN.Normal) )), SpcColor.a ) * SpcColor.rgb;
    
    float4 Color = IN.Color;
    float4 ShadowColor = float4(EgColor.rgb, Color.a);  // �e�̐F
    if ( useTexture ) {
        // �e�N�X�`���K�p
        float4 TexColor = tex2D( ObjTexSampler, IN.Tex );
        Color *= TexColor;
        ShadowColor *= TexColor;
    }
    if ( useSphereMap ) {
        // �X�t�B�A�}�b�v�K�p
        float4 TexColor = tex2D(ObjSphareSampler,IN.SpTex);
        if(spadd) {
            Color.rgb += TexColor.rgb;
            ShadowColor.rgb += TexColor.rgb;
        } else {
            Color.rgb *= TexColor.rgb;
            ShadowColor.rgb *= TexColor.rgb;
        }
    }
    // �X�y�L�����K�p
    Color.rgb += Specular;
    
    
    
    // �e�N�X�`�����W�ɕϊ�
    IN.ZCalcTex /= IN.ZCalcTex.w;
    float2 TransTexCoord;
    TransTexCoord.x = (1.0f + IN.ZCalcTex.x) * 0.5f;
    TransTexCoord.y = (1.0f - IN.ZCalcTex.y) * 0.5f;
    
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
            SadowMapVal = min(saturate(dot(IN.Normal, -lightdir) * 3), SadowMapVal);
            ShadowColor.rgb *= MaterialToon;
            
            ShadowColor.rgb *= (1 - (1 - ShadowRate) * PMD_SHADOWPOWER);
        }else{
            ShadowColor.rgb *= (1 - (1 - ShadowRate) * X_SHADOWPOWER);
        }
        
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
    // ���́uif�v�̒��O�ɒǉ�
    
    if( any( saturate(TransTexCoord) != TransTexCoord ) ) {
        // �V���h�E�o�b�t�@�O
        return Color;
        
    } else {
        float comp;
        float dist = IN.ZCalcTex.z - tex2D(DefSampler,TransTexCoord).r;
        
        if(parthf) {
            // �Z���t�V���h�E mode2
            comp = 1 - saturate(max(dist, 0.0f)* SKII2 * TransTexCoord.y - 0.3f);
        } else {
            // �Z���t�V���h�E mode1
            comp = 1 - saturate(max(dist, 0.0f) * SKII1 - 0.3f);
        }
        if ( useToon ) {
            // �g�D�[���K�p
            comp = min(saturate(dot(IN.Normal, -LightDirection) * Toon), comp);
            ShadowColor.rgb *= MaterialToon;
        }
        
        float4 ans = lerp(ShadowColor, Color, comp);
        if( transp ) ans.a = 0.5f;
        
        return ans;
    }
    
    
}

// �I�u�W�F�N�g�`��p�e�N�j�b�N�i�A�N�Z�T���p�j
technique MainTecBS0  < string MMDPass = "object_ss"; bool UseTexture = false; bool UseSphereMap = false; bool UseToon = false; > {
    pass DrawObject {
        VertexShader = compile vs_3_0 BufferShadow_VS(false, false, false);
        PixelShader  = compile ps_3_0 BufferShadow_PS(false, false, false);
    }
}

technique MainTecBS1  < string MMDPass = "object_ss"; bool UseTexture = true; bool UseSphereMap = false; bool UseToon = false; > {
    pass DrawObject {
        VertexShader = compile vs_3_0 BufferShadow_VS(true, false, false);
        PixelShader  = compile ps_3_0 BufferShadow_PS(true, false, false);
    }
}

technique MainTecBS2  < string MMDPass = "object_ss"; bool UseTexture = false; bool UseSphereMap = true; bool UseToon = false; > {
    pass DrawObject {
        VertexShader = compile vs_3_0 BufferShadow_VS(false, true, false);
        PixelShader  = compile ps_3_0 BufferShadow_PS(false, true, false);
    }
}

technique MainTecBS3  < string MMDPass = "object_ss"; bool UseTexture = true; bool UseSphereMap = true; bool UseToon = false; > {
    pass DrawObject {
        VertexShader = compile vs_3_0 BufferShadow_VS(true, true, false);
        PixelShader  = compile ps_3_0 BufferShadow_PS(true, true, false);
    }
}

// �I�u�W�F�N�g�`��p�e�N�j�b�N�iPMD���f���p�j
technique MainTecBS4  < string MMDPass = "object_ss"; bool UseTexture = false; bool UseSphereMap = false; bool UseToon = true; > {
    pass DrawObject {
        VertexShader = compile vs_3_0 BufferShadow_VS(false, false, true);
        PixelShader  = compile ps_3_0 BufferShadow_PS(false, false, true);
    }
}

technique MainTecBS5  < string MMDPass = "object_ss"; bool UseTexture = true; bool UseSphereMap = false; bool UseToon = true; > {
    pass DrawObject {
        VertexShader = compile vs_3_0 BufferShadow_VS(true, false, true);
        PixelShader  = compile ps_3_0 BufferShadow_PS(true, false, true);
    }
}

technique MainTecBS6  < string MMDPass = "object_ss"; bool UseTexture = false; bool UseSphereMap = true; bool UseToon = true; > {
    pass DrawObject {
        VertexShader = compile vs_3_0 BufferShadow_VS(false, true, true);
        PixelShader  = compile ps_3_0 BufferShadow_PS(false, true, true);
    }
}

technique MainTecBS7  < string MMDPass = "object_ss"; bool UseTexture = true; bool UseSphereMap = true; bool UseToon = true; > {
    pass DrawObject {
        VertexShader = compile vs_3_0 BufferShadow_VS(true, true, true);
        PixelShader  = compile ps_3_0 BufferShadow_PS(true, true, true);
    }
}


///////////////////////////////////////////////////////////////////////////////////////////////
