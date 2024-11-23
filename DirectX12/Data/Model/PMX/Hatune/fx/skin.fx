/* ------------------------------------------------------------
 * AlternativeFull
 * ------------------------------------------------------------ */
/* created by AlternativeFullFrontend. */
#define TEXTURE_THRESHOLD "shading_hint_skin.png"
#define USE_MATERIAL_TEXTURE
#define USE_NORMALMAP
#define TEXTURE_NORMALMAP "skin_n.png"
float NormalMapResolution = 1;

#define USE_SELFSHADOW_MODE
#define USE_NONE_SELFSHADOW_MODE

float SelfShadowPower = 2.2;
#define USE_SHADOWCOLOR_SELFPOWER_MODE
float SelfPowerShadowStrength = 0.3;

#define USE_MATERIAL_SPECULAR
#define USE_MATERIAL_SPHERE
#define HIGHLIGHT_ANTI_AUTOLUMINOUS
float3 DefaultModeShadowColor = {1,1,1};

#define MAX_ANISOTROPY 16
#define USE_ADAPTIVE_TEXTURE_FILTERING


#include "AlternativeFull.fxsub"
