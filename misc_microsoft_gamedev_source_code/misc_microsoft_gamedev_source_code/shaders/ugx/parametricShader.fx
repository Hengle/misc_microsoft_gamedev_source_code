// File: parametricShader.fx
// All parameters in this effect should be set to manual register update.

// Remark this out for faster compilation during development
#define DECLARE_OPTIMIZED_PASSES

#include "..\shared\intrinsics.inc"
#include "..\shared\helpers.inc"
#include "..\shared\fogHelpers.inc"

#include "..\shared\pcf.inc"
#include "..\shared\dirLighting.inc"
#include "..\shared\localLighting.inc"
#include "..\shared\shFillLighting.inc"
#include "..\shared\blackmap.inc"

#include "pShaderRegs.inc"
#include "vShaderRegs.inc"

sampler DiffuseSampler   : register(DIFFUSE_SAMPLER_REG);
sampler BumpSampler      : register(BUMP_SAMPLER_REG);
sampler SpecularSampler  : register(SPECULAR_SAMPLER_REG);
sampler OpacitySampler   : register(OPACITY_SAMPLER_REG);
sampler XFormSampler     : register(XFORM_SAMPLER_REG);
sampler SelfSampler      : register(SELF_SAMPLER_REG);
sampler AOSampler        : register(AO_SAMPLER_REG);
sampler EnvSampler       : register(ENV_SAMPLER_REG);
sampler EnvMaskSampler   : register(ENV_MASK_SAMPLER_REG);
sampler EmXFormSampler   : register(EMXFORM_SAMPLER_REG);
sampler HighlightSampler : register(HIGHLIGHT_SAMPLER_REG);
sampler BlackmapSampler  : register(BLACKMAP_SAMPLER_REG);
sampler ModulateSampler  : register(MODULATE_SAMPLER_REG);
sampler LightBufferColorSampler : register(LIGHT_BUFFER_COLOR_SAMPLER_REG);
sampler LightBufferVectorSampler : register(LIGHT_BUFFER_VECTOR_SAMPLER_REG);
sampler AddTexSampler   : register(ADD_TEX_SAMPLER_REG);
sampler LerpTexSampler  : register(LERP_TEX_SAMPLER_REG);

bool gSpecMapEnabled    : register(ENABLE_SPEC_MAP_REG);
bool gOpacityMapEnabled : register(ENABLE_OPACITY_MAP_REG);
bool gXFormMapEnabled   : register(ENABLE_XFORM_MAP_REG);
bool gSelfMapEnabled    : register(ENABLE_SELF_MAP_REG);
bool gAOMapEnabled      : register(ENABLE_AO_MAP_REG);
bool gEnvMapEnabled     : register(ENABLE_ENV_MAP_REG);
bool gEnvMaskMapEnabled : register(ENABLE_ENVMASK_MAP_REG);
bool gEmXFormMapEnabled : register(ENABLE_EMXFORM_MAP_REG);
bool gHighlightMapEnabled : register(ENABLE_HIGHLIGHT_MAP_REG);
bool gModulateMapEnabled : register(ENABLE_MODULATE_MAP_REG);

int gNumPixelLights         : register(NUM_LOCAL_PIXEL_LIGHTS_REG);
bool gLocalPixelLightingEnabled  : register(ENABLE_LOCAL_PIXEL_LIGHTS_REG);
bool gLocalShadowingEnabled : register(ENABLE_LOCAL_SHADOWING_REG);

bool gTwoSided          : register(ENABLE_TWO_SIDED_REG);

bool gLocalReflectionEnabled : register(ENABLE_LOCAL_REFL_REG);

bool gBlackmapEnabled : register(ENABLE_BLACKMAP_REG);

bool gLightBufferingEnabled : register(ENABLE_LIGHT_BUFFERING_REG);

bool gAddTexEnabled : register(ENABLE_ADD_TEX_REG);
bool gLerpTexEnabled : register(ENABLE_LERP_TEX_REG);

// tint color w is overall alpha
float4 gTintColor       : register(TINT_COLOR_REG);
float4 gSpecColorPower  : register(SPEC_COLOR_POWER_REG);

// x = emissive
// y = env
// z = highlight
float4 gHDRTexScale     : register(HDR_TEX_SCALE_REG);

// x = vis index
// w = env intensity;
float4 gEnvVisControl   : register(ENV_VIS_CONTROL_REG);

float4 gUVOfs0          : register(UV_OFFSET_0_REG);
float4 gUVOfs1          : register(UV_OFFSET_1_REG);
float4 gUVOfs2          : register(UV_OFFSET_2_REG);
float4 gUVOfs3          : register(UV_OFFSET_3_REG);
float4 gUVOfs4          : register(UV_OFFSET_4_REG);
float4 gUVOfs5          : register(UV_OFFSET_5_REG);

float4 gUVChannel0      : register(UV_CHANNEL_0_REG);
float4 gUVChannel1      : register(UV_CHANNEL_1_REG);
float4 gUVChannel2      : register(UV_CHANNEL_2_REG);

float4 gBlackmapParams0      : register(BLACKMAP_PARAMS0_REG);
float4 gBlackmapParams1      : register(BLACKMAP_PARAMS1_REG);
float4 gBlackmapParams2      : register(BLACKMAP_PARAMS2_REG);

float4 gWorldToLightBufCol0 : register(WORLD_TO_LIGHTBUF0_REG);
float4 gWorldToLightBufCol1 : register(WORLD_TO_LIGHTBUF1_REG);
float4 gWorldToLightBufCol2 : register(WORLD_TO_LIGHTBUF2_REG);

// xy = UV Ofs, z = Scale, w = inten/opacity
float4 gAddTexParams    : register(ADD_TEX_PARAMS_REG);
float4 gAddTexUVXForm0   : register(ADD_TEX_UVXFORM0_REG);
float4 gAddTexUVXForm1   : register(ADD_TEX_UVXFORM1_REG);

float4 gLerpTexParams   : register(LERP_TEX_PARAMS_REG);
float4 gLerpTexUVXForm0   : register(LERP_TEX_UVXFORM0_REG);
float4 gLerpTexUVXForm1   : register(LERP_TEX_UVXFORM1_REG);

// UV offset registers 
// Distortion and diffuse are mutually exclusive
#define DIFFUSE_UV_OFS        gUVOfs0.xy
#define BUMP_UV_OFS           gUVOfs0.zw
#define DISTORTION_UV_OFS     gUVOfs0.xy

#define SPEC_UV_OFS           gUVOfs1.xy
#define OPACITY_UV_OFS        gUVOfs1.zw

#define SELF_UV_OFS           gUVOfs2.xy
#define ENVMASK_UV_OFS        gUVOfs2.zw

#define AO_UV_OFS             gUVOfs3.xy
#define XFORM_UV_OFS          gUVOfs3.zw

#define EMXFORM_UV_OFS        gUVOfs4.xy
#define DIFFUSE_W_OFS         gUVOfs4.z
#define BUMP_W_OFS            gUVOfs4.w

#define HIGHLIGHT_UV_OFS      gUVOfs5.xy
#define MODULATE_UV_OFS       gUVOfs5.zw

#define DIFFUSE_UV_SELECTOR   gUVChannel0.x
#define BUMP_UV_SELECTOR      gUVChannel0.y
#define SPEC_UV_SELECTOR      gUVChannel0.z
#define OPACITY_UV_SELECTOR   gUVChannel0.w
#define XFORM_UV_SELECTOR     gUVChannel1.x
#define SELF_UV_SELECTOR      gUVChannel1.y
#define AO_UV_SELECTOR        gUVChannel1.z
#define ENVMASK_UV_SELECTOR   gUVChannel1.w
#define EMXFORM_UV_SELECTOR   gUVChannel2.x
#define DISTORTON_UV_SELECTOR gUVChannel2.y
#define HIGHLIGHT_UV_SELECTOR gUVChannel2.z 
#define MODULATE_UV_SELECTOR  gUVChannel2.w 

#include "vertexShader.inc"
#include "pixelShader.inc"

#define AUTO   true, false
#define DISA   false, false
#define ENAB   false, true

// Only pixel shader material parameters can be specialized!
#define DEFINE_PASS(spec, dirl, dirs, locl, specmap, opmap, xformmap, emxformmap, selfmap, aomap, envmap, envmaskmap, twosided, uvsel, uvofs, localReflection, highlightMap, modulateMap) \
   pass < \
      bool specFlag            = spec;                 \
      bool dirlFlag            = dirl;                 \
      bool dirsFlag            = dirs;                 \
      bool loclFlag            = locl;                 \
      bool specmapFlag         = specmap;              \
      bool opmapFlag           = opmap;                \
      bool xformmapFlag        = xformmap;             \
      bool emxformmapFlag      = emxformmap;           \
      bool selfmapFlag         = selfmap;              \
      bool aomapFlag           = aomap;                \
      bool envmapFlag          = envmap;               \
      bool envmaskmapFlag      = envmaskmap;           \
      bool twosidedFlag        = twosided;             \
      bool uvselFlag           = uvsel;                \
      bool uvofsFlag           = uvofs;                \
      bool localReflectionFlag = localReflection;      \
      bool highlightMapFlag    = highlightMap;         \
      bool modulateMapFlag     = modulateMap;          \
   > { \
   PixelShader = compile ps_3_0 psMainInternal(spec, dirl, dirs, locl, specmap, opmap, xformmap, emxformmap, selfmap, aomap, envmap, envmaskmap, twosided, uvsel, uvofs, localReflection, highlightMap, modulateMap); \
   }
      
// Defines both local lit and not local lit passes   
#define DEFINE_PASSES(spec, dirl, dirs, specmap, opmap, xformmap, emxformmap, selfmap, aomap, envmap, envmaskmap, twosided, uvsel, uvofs, localReflection, highlightMap, modulateMap) \
   DEFINE_PASS(spec, dirl, dirs, false, specmap, opmap, xformmap, emxformmap, selfmap, aomap, envmap, envmaskmap, twosided, uvsel, uvofs, localReflection, highlightMap, modulateMap) \
   DEFINE_PASS(spec, dirl, dirs, true, specmap, opmap, xformmap, emxformmap, selfmap, aomap, envmap, envmaskmap, twosided, uvsel, uvofs, localReflection, highlightMap, modulateMap) \
  
technique Visual
{
    pass Skinned
    {
      VertexShader = compile vs_3_0 vsMain(false, true, true, false);
    }
    
    pass Rigid
    {
      VertexShader = compile vs_3_0 vsMain(true, true, true, false);
    }
    
    pass SkinnedTerrainConform
    {
      VertexShader = compile vs_3_0 vsMain(false, true, true, true);
    }
    
    pass RigidTerrainConform
    {
      VertexShader = compile vs_3_0 vsMain(true, true, true, true);
    }
       
    pass GenericPixelShader
    {
      PixelShader = compile ps_3_0 psMain(true, AUTO, true,  AUTO,     AUTO,        AUTO,          AUTO,            AUTO,          AUTO,          AUTO,          AUTO,   AUTO,    AUTO, true, true, AUTO, AUTO, AUTO );    
    }
    
    pass GenericNoDirShadowPixelShader
    {
      PixelShader = compile ps_3_0 psMain(true, AUTO, false,  AUTO,     AUTO,        AUTO,          AUTO,            AUTO,          AUTO,          AUTO,          AUTO,   AUTO,    AUTO, true, true, AUTO, AUTO, AUTO );    
    }
    
//               SPEC   DIRL   DIRS   SPECMAP      OPMAP          XFORMMAP         EMXFORMMAP     SELFMAP        AOMAP          ENVMAP    ENVMASK  TWOSIDED  UVSEL    UVOFS  LOCALREFLECTION HIGHLIGHTMAP MODULATEMAP
#if 0    
   DEFINE_PASSES(false, true,  true,  false,       false,         false,           false,         false,         false,         false,    false,   false,    false,   false, false,          false,       false)
#endif    

   // Testing
   //DEFINE_PASSES(false, true,  true,  false,       false,         false,           false,         false,         false,         false,    false,   false,    false,   false, false,          false,       false)

#ifdef DECLARE_OPTIMIZED_PASSES 

   #ifdef BUMP
      DEFINE_PASSES(true, true, true, false, false, false, false, false, false, false, false, true, false, false, false, false, false  ) // 4
      DEFINE_PASSES(true, true, true, true, false, true, false, false, true, false, true, false, false, false, false, false, false  ) // 2
      DEFINE_PASSES(true, true, false, true, false, false, false, true, false, false, false, false, false, false, false, false, false  ) // 74
      DEFINE_PASSES(false, true, true, false, true, true, false, false, false, false, false, false, false, false, false, false, false  ) // 54
      DEFINE_PASSES(true, true, true, true, true, false, false, false, false, false, false, false, false, true, false, false, false  ) // 8
      DEFINE_PASSES(true, true, true, false, false, false, false, true, false, true, false, false, true, true, false, false, false  ) // 16
      DEFINE_PASSES(true, true, true, true, true, false, false, false, false, false, false, false, false, false, false, false, false  ) // 228
      DEFINE_PASSES(true, true, true, true, false, true, false, true, false, true, true, true, false, false, false, false, false  ) // 2
      DEFINE_PASSES(true, true, true, true, false, true, false, true, false, true, false, false, false, false, false, false, false  ) // 72
      DEFINE_PASSES(true, true, true, true, true, true, false, true, true, true, true, false, false, false, false, false, false  ) // 6
      DEFINE_PASSES(true, true, true, true, true, false, false, false, false, true, true, false, false, false, false, false, false  ) // 12
      DEFINE_PASSES(true, true, false, false, false, false, false, true, false, false, false, false, false, false, false, false, false  ) // 2
      DEFINE_PASSES(true, true, true, true, true, true, false, true, false, false, false, false, false, false, false, false, false  ) // 12
      DEFINE_PASSES(false, true, false, false, false, false, false, true, false, false, false, true, false, true, false, false, false  ) // 2
      DEFINE_PASSES(true, true, true, true, false, true, false, false, false, false, false, true, false, false, false, false, false  ) // 240
      DEFINE_PASSES(true, true, true, true, false, false, false, false, false, true, false, false, false, false, false, false, false  ) // 614
      DEFINE_PASSES(true, true, true, false, false, true, false, false, false, false, false, true, false, true, false, false, false  ) // 6
      DEFINE_PASSES(true, true, true, true, false, false, false, false, false, false, false, false, false, false, false, false, false  ) // 1574
      DEFINE_PASSES(false, true, true, false, false, false, false, false, false, false, false, true, false, false, false, false, false  ) // 192
      DEFINE_PASSES(true, true, true, false, false, true, false, false, false, false, false, false, false, true, false, false, false  ) // 16
      DEFINE_PASSES(true, true, true, true, true, false, false, false, false, false, false, true, false, false, false, false, false  ) // 266
      DEFINE_PASSES(true, true, true, true, false, false, false, false, true, true, true, false, false, false, false, false, false  ) // 2
      DEFINE_PASSES(true, true, true, true, true, false, false, true, false, true, true, false, false, false, false, false, false  ) // 14
      DEFINE_PASSES(true, true, true, false, true, false, false, false, false, false, false, false, false, false, false, false, false  ) // 72
      DEFINE_PASSES(true, true, true, false, false, true, false, false, false, false, false, true, false, false, false, false, false  ) // 78
      DEFINE_PASSES(false, true, true, false, false, false, false, true, false, true, false, false, false, false, false, false, false  ) // 2
      DEFINE_PASSES(false, true, true, false, false, false, false, false, false, false, false, false, false, false, false, false, false  ) // 4214
      DEFINE_PASSES(true, true, true, false, false, false, false, false, false, false, false, false, false, false, false, false, false  ) // 720
      DEFINE_PASSES(true, true, true, true, false, false, false, false, false, true, false, true, false, false, false, false, false  ) // 124
      DEFINE_PASSES(true, true, true, true, false, false, false, true, false, true, false, false, false, false, false, false, false  ) // 334
      DEFINE_PASSES(true, true, true, true, true, false, false, true, false, false, false, false, false, false, false, false, false  ) // 2
      DEFINE_PASSES(true, true, true, true, false, false, false, false, false, true, true, true, false, false, true, false, false  ) // 8
      DEFINE_PASSES(true, true, true, false, true, false, false, false, false, true, false, false, false, false, false, false, false  ) // 4
      DEFINE_PASSES(true, true, true, true, false, false, false, true, false, false, false, true, false, false, false, false, false  ) // 4
      DEFINE_PASSES(true, true, true, true, false, true, false, true, false, false, false, false, false, true, false, true, false  ) // 12
      DEFINE_PASSES(true, true, true, false, false, false, false, false, false, true, false, true, false, false, false, false, false  ) // 8
      DEFINE_PASSES(true, true, true, false, false, true, false, false, false, false, false, false, false, false, false, false, false  ) // 78
      DEFINE_PASSES(false, true, true, false, true, false, false, true, false, false, false, true, false, true, false, false, false  ) // 8
      DEFINE_PASSES(true, true, true, true, false, false, false, true, false, true, false, false, false, true, false, false, false  ) // 2
      DEFINE_PASSES(true, true, true, true, true, true, false, false, false, true, false, true, false, false, false, false, false  ) // 2
      DEFINE_PASSES(true, true, true, true, true, false, false, false, false, false, false, true, true, false, false, false, false  ) // 20
      DEFINE_PASSES(false, true, true, false, false, false, false, true, false, false, false, false, false, true, false, false, false  ) // 76
      DEFINE_PASSES(true, true, true, false, false, true, false, false, false, true, true, false, false, false, false, false, false  ) // 6
      DEFINE_PASSES(true, true, true, true, false, true, false, false, true, true, true, false, false, false, false, false, false  ) // 4
      DEFINE_PASSES(true, true, true, true, true, true, false, false, false, false, false, false, false, false, false, false, false  ) // 8
      DEFINE_PASSES(true, true, true, true, false, false, false, true, false, false, false, false, false, false, false, false, false  ) // 716
      DEFINE_PASSES(true, true, true, true, false, false, false, true, false, false, false, false, false, true, false, true, false  ) // 2
      DEFINE_PASSES(true, true, true, true, true, false, false, false, false, true, false, false, false, false, false, false, false  ) // 2
      DEFINE_PASSES(true, true, true, false, true, false, false, true, false, false, false, false, false, true, false, false, false  ) // 6
      DEFINE_PASSES(false, true, true, false, true, false, false, false, false, false, false, false, false, false, false, false, false  ) // 60
      DEFINE_PASSES(true, true, true, false, false, false, false, true, false, true, false, false, false, false, false, false, false  ) // 22
      DEFINE_PASSES(true, true, true, true, false, true, false, true, false, false, false, false, false, false, false, false, false  ) // 28
      DEFINE_PASSES(true, true, false, true, false, false, false, false, false, true, false, false, true, true, false, false, false  ) // 12
      DEFINE_PASSES(true, true, true, true, true, true, false, false, false, true, true, true, false, false, false, false, false  ) // 12
      DEFINE_PASSES(true, true, true, true, false, true, false, false, false, true, false, true, false, false, false, false, false  ) // 2
      DEFINE_PASSES(true, true, true, true, false, false, false, false, false, true, true, false, false, false, false, false, false  ) // 216
      DEFINE_PASSES(true, true, true, true, false, false, false, false, false, true, false, true, false, false, true, false, false  ) // 12
      DEFINE_PASSES(true, true, true, true, true, false, false, false, false, true, true, true, false, false, false, false, false  ) // 6
      DEFINE_PASSES(true, true, true, true, true, false, false, true, false, false, false, false, false, true, false, false, false  ) // 2
      DEFINE_PASSES(true, true, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false  ) // 74
      DEFINE_PASSES(true, true, true, true, false, false, false, true, true, false, false, false, false, false, false, false, false  ) // 2
      DEFINE_PASSES(true, true, true, true, true, false, false, true, false, true, false, false, false, false, false, false, false  ) // 2
      DEFINE_PASSES(false, true, true, false, false, false, false, true, false, false, false, false, false, false, false, false, false  ) // 114
      DEFINE_PASSES(true, true, true, true, false, true, false, false, false, true, true, false, false, false, false, false, false  ) // 2592
      DEFINE_PASSES(true, true, true, true, true, false, false, false, false, false, false, false, true, false, false, false, false  ) // 4
      DEFINE_PASSES(true, true, true, true, false, true, false, true, true, true, false, false, false, false, false, false, false  ) // 108
      DEFINE_PASSES(true, true, true, false, false, false, false, false, false, true, false, false, false, false, false, false, false  ) // 6
      DEFINE_PASSES(true, true, true, true, false, false, false, true, false, true, true, false, false, false, false, false, false  ) // 428
      DEFINE_PASSES(false, true, true, false, false, false, false, true, false, false, false, true, false, true, false, false, false  ) // 2
      DEFINE_PASSES(true, true, true, true, false, false, false, true, false, true, true, true, false, false, false, false, false  ) // 4
      DEFINE_PASSES(true, true, true, true, false, true, false, true, false, true, false, false, true, true, false, true, false  ) // 2
      DEFINE_PASSES(true, true, true, true, false, true, false, true, true, false, false, false, false, false, false, false, false  ) // 2
      DEFINE_PASSES(true, true, true, true, false, true, true, true, false, true, false, false, false, false, false, false, false  ) // 16
      DEFINE_PASSES(true, true, true, false, true, false, false, false, false, false, false, true, false, false, false, false, false  ) // 6
      DEFINE_PASSES(true, true, true, true, true, false, false, true, false, false, false, true, false, false, false, false, false  ) // 100
      DEFINE_PASSES(true, true, true, true, true, false, false, true, false, true, true, true, false, false, false, false, false  ) // 202
      DEFINE_PASSES(true, true, true, true, false, false, false, false, false, false, false, true, false, false, false, false, false  ) // 188
      DEFINE_PASSES(true, true, true, true, false, false, false, false, false, false, false, false, true, false, false, false, false  ) // 20
      DEFINE_PASSES(true, true, true, true, false, true, false, false, false, true, false, false, false, false, false, false, false  ) // 6
      DEFINE_PASSES(true, true, true, true, false, true, false, false, false, true, true, true, false, false, false, false, false  ) // 780
      DEFINE_PASSES(true, true, false, true, true, false, false, false, false, false, false, true, false, false, false, false, false  ) // 4
      DEFINE_PASSES(true, true, true, true, false, false, false, true, false, true, false, false, true, true, false, true, false  ) // 2
      DEFINE_PASSES(true, true, true, true, false, false, false, true, false, false, false, false, false, true, false, false, false  ) // 2
      DEFINE_PASSES(true, true, true, true, false, true, false, false, false, false, true, false, false, false, false, false, false  ) // 64
      DEFINE_PASSES(true, true, true, true, false, false, false, true, true, true, true, false, false, false, false, false, false  ) // 4
      DEFINE_PASSES(true, true, true, true, true, false, false, false, false, false, true, false, false, false, false, false, false  ) // 6
      DEFINE_PASSES(true, true, true, false, false, false, false, true, false, false, false, false, false, true, false, false, false  ) // 18
      DEFINE_PASSES(true, true, true, false, true, false, false, true, false, false, false, true, true, true, false, false, false  ) // 2
      DEFINE_PASSES(true, true, true, true, false, true, false, false, false, false, false, false, false, false, false, false, false  ) // 2952
      DEFINE_PASSES(false, true, true, false, true, false, false, true, false, false, false, false, false, false, false, false, false  ) // 1162
      DEFINE_PASSES(true, true, true, false, false, false, false, true, false, false, false, false, false, false, false, false, false  ) // 722
      DEFINE_PASSES(true, true, true, false, true, false, false, true, false, false, false, false, false, false, false, false, false  ) // 100
      DEFINE_PASSES(true, true, true, true, false, true, false, false, true, false, false, false, false, false, false, false, false  ) // 30
      DEFINE_PASSES(true, true, true, false, false, false, false, true, false, true, false, false, false, true, false, false, false  ) // 8
      DEFINE_PASSES(true, true, false, true, false, false, false, false, false, false, false, false, false, false, false, false, false  ) // 166
      DEFINE_PASSES(true, true, true, false, false, false, false, true, false, false, false, true, true, true, false, false, false  ) // 84
      DEFINE_PASSES(true, true, true, true, true, false, false, false, false, true, false, true, false, false, false, false, false  ) // 136
      DEFINE_PASSES(false, true, true, false, true, false, false, true, false, false, false, true, false, false, false, false, false  ) // 208
      DEFINE_PASSES(true, true, false, true, false, false, false, true, false, true, false, false, true, true, false, false, false  ) // 8

      // start of scenario shaders
      DEFINE_PASSES(true, true, true, false, false, false, false, false, true, false, false, false, false, false, false, false, false  ) // 24
      DEFINE_PASSES(true, true, true, false, true, false, false, false, false, true, true, false, true, true, false, false, false  ) // 8
      DEFINE_PASSES(true, true, true, false, false, false, false, false, true, false, false, false, true, false, false, false, false  ) // 712
      DEFINE_PASSES(true, true, true, true, false, false, false, false, true, false, false, false, false, false, false, false, false  ) // 4
      DEFINE_PASSES(true, true, true, true, true, true, false, false, true, false, false, false, false, false, false, false, false  ) // 6
      DEFINE_PASSES(true, true, true, false, true, false, false, false, false, false, false, false, true, false, false, false, false  ) // 8
      DEFINE_PASSES(true, true, true, true, true, false, false, true, false, false, false, true, false, true, false, false, false  ) // 2
      DEFINE_PASSES(false, true, true, false, false, false, false, false, false, false, false, false, false, true, false, false, false  ) // 8
      DEFINE_PASSES(false, true, false, false, true, false, false, true, false, false, false, false, false, false, false, false, false  ) // 1116
      DEFINE_PASSES(false, true, false, false, false, false, false, true, false, false, false, false, false, false, false, false, false  ) // 220
      DEFINE_PASSES(true, true, true, true, false, false, false, true, false, false, true, false, false, false, false, false, false  ) // 2
      DEFINE_PASSES(true, true, true, false, true, false, false, false, false, true, false, false, false, false, true, false, false  ) // 6
      DEFINE_PASSES(false, true, true, false, true, false, false, false, false, false, false, true, false, false, false, false, false  ) // 36
      DEFINE_PASSES(true, true, true, false, true, false, false, false, false, false, false, false, false, true, false, false, false  ) // 8
      DEFINE_PASSES(true, true, true, false, true, false, false, false, true, true, false, true, true, false, false, false, false  ) // 20
      DEFINE_PASSES(false, true, true, false, false, false, false, false, true, false, false, false, false, false, false, false, false  ) // 4
      DEFINE_PASSES(true, true, true, true, true, false, false, true, false, true, false, true, false, false, false, false, false  ) // 2
      DEFINE_PASSES(true, true, true, true, false, false, false, false, false, true, true, false, true, true, false, false, false  ) // 136
      DEFINE_PASSES(true, true, false, true, true, false, false, true, false, false, false, false, false, false, false, false, false  ) // 2
      DEFINE_PASSES(true, true, true, true, false, false, false, true, false, true, true, false, false, true, false, false, false  ) // 2
      DEFINE_PASSES(false, true, true, false, true, false, false, false, false, true, true, false, true, true, false, false, false  ) // 2
      DEFINE_PASSES(true, true, false, false, true, false, false, false, false, false, false, false, false, false, false, false, false  ) // 270
      DEFINE_PASSES(true, true, false, false, false, false, false, true, false, false, false, false, false, true, false, false, false  ) // 140
      DEFINE_PASSES(true, true, true, true, false, false, false, true, false, false, false, false, true, false, false, false, false  ) // 344
      DEFINE_PASSES(true, true, true, false, false, false, false, false, true, false, false, false, true, false, false, false, true  ) // 52
      DEFINE_PASSES(true, true, true, false, false, false, false, false, false, true, true, true, false, false, false, false, false  ) // 18
      DEFINE_PASSES(true, true, true, true, false, false, false, false, false, false, true, false, false, false, false, false, false  ) // 4
      DEFINE_PASSES(true, true, true, false, false, false, false, false, false, true, true, false, true, true, false, false, false  ) // 4
      DEFINE_PASSES(false, true, true, false, false, false, false, false, true, false, false, false, true, false, false, false, false  ) // 16
      DEFINE_PASSES(true, true, true, true, false, false, false, false, false, true, true, false, true, true, true, false, false  ) // 56
      DEFINE_PASSES(true, true, true, true, true, false, false, false, false, true, true, false, true, true, false, false, false  ) // 68
      DEFINE_PASSES(true, true, true, false, false, false, false, false, false, true, true, false, false, true, false, false, false  ) // 4
      DEFINE_PASSES(true, true, true, true, false, true, true, true, false, false, false, false, false, false, false, false, false  ) // 28
      DEFINE_PASSES(true, true, true, true, true, false, false, true, false, false, false, false, true, true, false, false, false  ) // 4
      DEFINE_PASSES(true, true, true, true, false, false, false, false, false, true, true, true, false, false, false, false, false  ) // 10
      DEFINE_PASSES(true, true, true, false, false, false, false, false, false, false, false, false, false, true, false, false, false  ) // 16
      DEFINE_PASSES(true, true, true, false, false, false, false, false, true, true, false, false, true, false, false, false, false  ) // 56
      DEFINE_PASSES(true, true, true, false, false, false, false, false, true, true, false, true, true, false, false, false, false  ) // 36
      DEFINE_PASSES(true, true, false, true, true, false, false, false, false, true, false, false, false, false, false, false, false  ) // 2

      // start of skirmish shaders
      DEFINE_PASSES(true, true, true, true, true, false, false, false, true, true, false, false, false, false, false, false, false  ) // 6
      DEFINE_PASSES(true, true, true, true, false, false, false, false, false, true, false, false, false, false, true, false, false  ) // 2
      DEFINE_PASSES(true, true, true, false, true, false, false, false, false, false, false, true, true, false, false, false, false  ) // 4
      DEFINE_PASSES(false, true, true, false, true, false, false, false, false, true, true, false, true, true, true, false, false  ) // 2
      DEFINE_PASSES(true, true, true, true, true, false, false, false, false, false, true, true, false, false, false, false, false  ) // 2
      DEFINE_PASSES(true, true, true, false, false, false, false, false, true, true, false, false, false, false, false, false, false  ) // 46
      DEFINE_PASSES(true, true, true, true, false, false, false, false, false, true, false, false, true, false, false, false, false  ) // 18
      DEFINE_PASSES(false, true, true, false, true, false, false, false, false, false, false, false, true, true, false, false, false  ) // 4
      DEFINE_PASSES(false, true, true, false, true, false, false, false, false, false, false, false, false, true, false, false, false  ) // 4
      DEFINE_PASSES(true, true, true, false, true, false, false, false, false, false, false, false, true, true, false, false, false  ) // 4
      DEFINE_PASSES(true, true, true, true, false, false, false, true, false, true, false, false, true, false, false, false, false  ) // 14

   #else // ndef BUMP - Non-tangent
   
      DEFINE_PASSES(false, true, true, false, true, false, false, true, false, false, false, false, false, true, false, false, false  ) // 2
      DEFINE_PASSES(true, true, false, false, true, false, false, true, false, false, false, true, true, true, false, false, false  ) // 4
      DEFINE_PASSES(false, true, true, false, true, false, false, true, false, false, false, false, false, false, false, false, false  ) // 20
      DEFINE_PASSES(true, true, true, false, false, false, false, false, false, false, false, true, false, true, false, false, false  ) // 6
      DEFINE_PASSES(true, true, true, false, false, true, false, false, false, true, false, false, false, false, false, false, false  ) // 6
      DEFINE_PASSES(true, true, true, false, false, false, false, false, false, false, false, true, false, false, false, false, false  ) // 8
      DEFINE_PASSES(true, true, true, false, false, true, false, false, false, false, false, true, false, false, false, false, false  ) // 16
      DEFINE_PASSES(true, true, true, true, false, false, false, true, false, false, false, true, false, true, false, false, false  ) // 20
      DEFINE_PASSES(true, true, true, false, true, false, false, false, false, false, false, false, false, false, false, false, false  ) // 6
      DEFINE_PASSES(true, true, true, true, false, false, false, false, false, false, false, false, false, false, false, false, false  ) // 2
      DEFINE_PASSES(true, true, true, false, false, true, false, false, false, false, false, false, false, false, false, false, false  ) // 74
      DEFINE_PASSES(false, true, true, false, false, false, false, false, false, false, false, true, false, false, false, false, false  ) // 2
      DEFINE_PASSES(true, true, true, false, false, false, false, true, false, false, false, false, false, false, false, false, false  ) // 20
      DEFINE_PASSES(true, true, true, true, false, true, false, false, false, false, false, false, false, false, false, false, false  ) // 6
      DEFINE_PASSES(true, true, true, false, false, false, false, false, false, false, false, false, false, false, false, false, false  ) // 154
      DEFINE_PASSES(true, true, true, true, false, false, false, true, false, false, false, false, false, false, false, false, false  ) // 10
      DEFINE_PASSES(false, true, true, false, false, false, false, true, true, false, false, false, false, true, false, false, false  ) // 2
      DEFINE_PASSES(true, true, true, false, false, false, false, true, false, false, false, false, false, true, false, false, false  ) // 6
      DEFINE_PASSES(true, true, true, true, false, false, false, false, false, false, false, false, false, true, false, false, false  ) // 14
      DEFINE_PASSES(true, true, true, false, false, false, false, true, false, false, false, true, false, true, false, false, false  ) // 4
      DEFINE_PASSES(false, true, true, false, true, false, false, true, false, false, false, false, true, true, false, false, false  ) // 4
      DEFINE_PASSES(true, true, true, false, false, true, false, true, false, false, false, false, false, false, false, false, false  ) // 2
      DEFINE_PASSES(false, true, true, false, true, false, false, true, false, false, false, true, false, true, false, false, false  ) // 16
      DEFINE_PASSES(true, true, true, true, false, false, false, true, false, false, false, false, false, true, false, false, false  ) // 10
      DEFINE_PASSES(true, true, true, true, true, false, false, true, false, true, false, true, true, true, false, false, false  ) // 4
      DEFINE_PASSES(false, true, true, false, true, false, false, false, false, false, false, false, false, false, false, false, false  ) // 10
      DEFINE_PASSES(false, true, true, false, false, false, false, false, false, false, false, false, false, false, false, false, false  ) // 10
      DEFINE_PASSES(true, true, true, false, false, false, false, true, false, true, true, false, false, true, false, false, false  ) // 12

      // start of scenario shaders
      DEFINE_PASSES(false, true, false, false, false, false, false, false, false, false, false, false, false, true, false, false, false  ) // 2
      DEFINE_PASSES(true, true, true, true, false, false, false, false, false, true, false, false, false, false, false, false, false  ) // 2
      DEFINE_PASSES(false, true, true, false, false, false, false, true, false, false, false, false, false, false, false, false, false  ) // 2
      DEFINE_PASSES(true, true, false, true, false, false, false, false, false, true, true, false, true, false, false, false, false  ) // 2
      DEFINE_PASSES(true, true, true, true, false, false, false, false, false, true, true, false, false, false, false, false, false  ) // 2
      DEFINE_PASSES(false, true, true, false, false, false, false, true, true, false, false, false, false, false, false, false, false  ) // 14
      DEFINE_PASSES(true, true, true, false, true, false, false, false, false, false, false, false, false, true, false, false, false  ) // 10
      DEFINE_PASSES(true, true, true, false, true, false, false, false, false, false, false, true, false, false, false, false, false  ) // 4
      DEFINE_PASSES(false, true, true, false, false, false, false, true, false, false, false, true, true, true, false, false, false  ) // 44
      DEFINE_PASSES(false, true, true, false, true, false, false, false, false, false, false, false, true, true, false, false, false  ) // 2

      // start of skirmish shaders
      DEFINE_PASSES(false, true, true, false, false, false, false, true, false, true, false, false, false, false, false, false, false  ) // 2
      DEFINE_PASSES(false, true, true, false, true, false, false, true, false, true, false, false, false, false, false, false, false  ) // 2
      DEFINE_PASSES(true, true, true, true, true, false, false, false, false, true, false, false, false, false, true, false, false  ) // 4
      DEFINE_PASSES(false, true, true, false, true, false, false, false, false, false, false, false, false, true, false, false, false  ) // 6
      DEFINE_PASSES(true, true, true, true, true, false, false, false, false, false, false, false, true, false, false, false, false  ) // 162
      DEFINE_PASSES(true, true, true, true, true, false, false, false, false, false, false, false, false, false, false, false, false  ) // 38

   #endif

#endif //DECLARE_OPTIMIZED_PASSES   

}

technique Sky
{
    pass Skinned
    {
      VertexShader = compile vs_3_0 vsMain(false, true, true, false);
      PixelShader = compile ps_3_0 psSky(true, true);
    }
    pass Rigid
    {
      VertexShader = compile vs_3_0 vsMain(true, true, true, false);
      PixelShader = compile ps_3_0 psSky(true, true);
    }
}

technique Vis
{
   pass Skinned
   {
      VertexShader = compile vs_3_0 vsMain(false, true, true, false);
      PixelShader = compile ps_3_0 psMainVis(true, true, true, true, true);
   }
   pass Rigid
   {
      VertexShader = compile vs_3_0 vsMain(true, true, true, false);
      PixelShader = compile ps_3_0 psMainVis(true, true, true, true, true);
   }
   pass SkinnedTerrainConform
   {
      VertexShader = compile vs_3_0 vsMain(false, true, true, true);
      PixelShader = compile ps_3_0 psMainVis(true, true, true, true, true);
   }
   pass RigidTerrainConform
   {
      VertexShader = compile vs_3_0 vsMain(true, true, true, true);
      PixelShader = compile ps_3_0 psMainVis(true, true, true, true, true);
   }
}    

technique Shadow
{
    pass Skinned
    {
        VertexShader = compile vs_3_0 vsShadowMain(false, false);
        PixelShader = compile ps_3_0 psShadowMain(false);
    }
    pass Rigid
    {
        VertexShader = compile vs_3_0 vsShadowMain(true, false);
        PixelShader = compile ps_3_0 psShadowMain(false);
    }
}

technique ShadowOverallAlpha
{
    pass Skinned
    {
        VertexShader = compile vs_3_0 vsShadowMain(false, false);
        PixelShader = compile ps_3_0 psShadowMain(true);
    }
    pass Rigid
    {
        VertexShader = compile vs_3_0 vsShadowMain(true, false);
        PixelShader = compile ps_3_0 psShadowMain(true);
    }
}

technique ShadowAlphaTest
{
    pass Skinned
    {
        VertexShader = compile vs_3_0 vsShadowMainAlphaTest(false, true, true, false);
        PixelShader = compile ps_3_0 psShadowMainAlphaTest(true, true);
    }
    pass Rigid
    {
        VertexShader = compile vs_3_0 vsShadowMainAlphaTest(true, true, true, false);
        PixelShader = compile ps_3_0 psShadowMainAlphaTest(true, true);
    }
}

technique DPShadow
{
    pass Skinned
    {
        VertexShader = compile vs_3_0 vsShadowMain(false, true);
        PixelShader = compile ps_3_0 psDPShadowMain(false);
    }
    pass Rigid
    {
        VertexShader = compile vs_3_0 vsShadowMain(true, true);
        PixelShader = compile ps_3_0 psDPShadowMain(false);
    }
}

technique DPShadowOverallAlpha
{
    pass Skinned
    {
        VertexShader = compile vs_3_0 vsShadowMain(false, true);
        PixelShader = compile ps_3_0 psDPShadowMain(true);
    }
    pass Rigid
    {
        VertexShader = compile vs_3_0 vsShadowMain(true, true);
        PixelShader = compile ps_3_0 psDPShadowMain(true);
    }
}

technique DPShadowAlphaTest
{
    pass Skinned
    {
        VertexShader = compile vs_3_0 vsShadowMainAlphaTest(false, true, true, true);
        PixelShader = compile ps_3_0 psDPShadowMainAlphaTest(true, true);
    }
    pass Rigid
    {
        VertexShader = compile vs_3_0 vsShadowMainAlphaTest(true, true, true, true);
        PixelShader = compile ps_3_0 psDPShadowMainAlphaTest(true, true);
    }
}

technique Distortion
{
    pass Skinned
    {
        VertexShader = compile vs_3_0 vsMain(false, true, true, false);
        PixelShader = compile ps_3_0 psDistortionMain();
    }
    pass Rigid
    {
        VertexShader = compile vs_3_0 vsMain(true, true, true, false);
        PixelShader = compile ps_3_0 psDistortionMain();
    }
    pass SkinnedTerrainConform
    {
       VertexShader = compile vs_3_0 vsMain(false, true, true, true);
       PixelShader = compile ps_3_0 psDistortionMain();
    }
    pass RigidTerrainConform
    {
       VertexShader = compile vs_3_0 vsMain(true, true, true, true);
       PixelShader = compile ps_3_0 psDistortionMain();
    }
}
