// File: intrinsics.inc
// All effect files must include this file.
// rg [2/23/06] - Don't manually change any of these parameters at run-time! The effect intrinsic
// manager will update shared params automatically. It will update the non-shared intrinsic params when
// you call BEffect's updateIntrinsicParams(). Treat these params as const global variables.
// This file must be kept in sync with effectIntrinsicDefs.h!

// Warning: Don't place any shared regs higher than address 239, because 240-255 are reserved for HLSL compiler literal constants.

// Shared
shared float4x4   gWorldToProj               : register(c200) < string intrinsic="WorldToProj"; >;

shared bool       gDirLightEnabled           : register(b31);

shared float4x4   gDirShadowWorldToTex       : register(c204) < string intrinsic="DirShadowWorldToTex"; >;

shared float4     gDirShadowParams0          : register(c208) < string intrinsic="gDirShadowParams0"; >;
shared float4     gDirShadowParams1          : register(c209) < string intrinsic="gDirShadowParams1"; >;

#define MAX_CSM_PASSES 8
shared float4     gDirShadowZScales[MAX_CSM_PASSES] : register(c210)  < string intrinsic="gDirShadowZScales"; >;

shared float3     gWorldCameraPos            : register(c220) < string intrinsic="WorldCameraPos"; >;

shared float3     gDirLightVecToLightWorld   : register(c221) < string intrinsic="DirLightVecToLightWorld"; >;
shared float4     gDirLightColor             : register(c222) < string intrinsic="DirLightColor"; >;
#define gDirShadowDarkness    gDirLightColor.w

shared bool       gPlanarFogEnabled          : register(b30)  < string intrinsic="PlanarFogEnabled"; >;

shared float3     gFogColor                  : register(c223) < string intrinsic="FogColor"; >;
shared float3     gPlanarFogColor            : register(c224) < string intrinsic="PlanarFogColor"; >;
shared float4     gFogParams                 : register(c225) < string intrinsic="FogParams"; >;
#define gFogDensity2          gFogParams.x
#define gFogStart2            gFogParams.y
#define gPlanarFogDensity2    gFogParams.z
#define gPlanarFogStart       gFogParams.w

shared float4     gSHFillAr                  : register(c226) < string intrinsic="SHFillAr"; >;
shared float4     gSHFillAg                  : register(c227) < string intrinsic="SHFillAg"; >;

shared float4     gSHFillAb                  : register(c228) < string intrinsic="SHFillAb"; >;
shared float4     gSHFillBr                  : register(c229) < string intrinsic="SHFillBr"; >;
shared float4     gSHFillBg                  : register(c230) < string intrinsic="SHFillBg"; >;
shared float4     gSHFillBb                  : register(c231) < string intrinsic="SHFillBb"; >;

shared float4     gSHFillC                   : register(c232) < string intrinsic="SHFillCc"; >;
shared float4x4   gSceneWorldToProj          : register(c233) < string intrinsic="SceneWorldToProj"; >;

shared float4     gPaddingDummy0             : register(c237);
shared float4     gPaddingDummy1             : register(c238);
shared float4     gPaddingDummy2             : register(c239);

// Unshared

// These correspond to BRenderDraw's "active" matrices.
// rg [1/2/08] - Note: Unshared intrinsics are not compatible with DCB recording/playback!

float4x4          gWorldToView            < string intrinsic="WorldToView"; >;
float4x4          gViewToProj             < string intrinsic="ViewToProj"; >;
float4x4          gDirShadowWorldToProj   < string intrinsic="DirShadowWorldToProj"; >;

float4x4          gProjToScreen           < string intrinsic="ProjToScreen"; >;
float4x4          gViewToWorld            < string intrinsic="ViewToWorld"; >;
float4x4          gScreenToProj           < string intrinsic="ScreenToProj"; >;
float4x4          gScreenToView           < string intrinsic="ScreenToView"; >;

float             gTimeLoop               < string intrinsic="TimeLoop"; >;

sampler gDirShadowMapTexture
< string intrinsic="DirShadowMapTexture"; > = sampler_state
{
    MipFilter = NONE;
    MinFilter = POINT;
    MagFilter = POINT;
    
    AddressU = CLAMP;
    AddressV = CLAMP;
    AddressW = CLAMP;
    
    MaxAnisotropy = 1;
    
    SeparateZFilterEnable = TRUE;
    MinFilterZ = POINT;
    MagFilterZ = POINT;
};

sampler gLocalShadowMapTexture
< string intrinsic="LocalShadowMapTexture"; > = sampler_state
{
    MipFilter = NONE;
    MinFilter = POINT;
    MagFilter = POINT;
    
    AddressU = CLAMP;
    AddressV = CLAMP;
    AddressW = CLAMP;
    
    SeparateZFilterEnable = TRUE;
    MinFilterZ = POINT;
    MagFilterZ = POINT;
};

sampler gLightTexture
< string intrinsic="LightTexture"; > = sampler_state
{
    MipFilter = NONE;
    MinFilter = POINT;
    MagFilter = POINT;
    
    AddressU = CLAMP;
    AddressV = CLAMP;
    AddressW = CLAMP;
};

sampler gReflectionTexture
< string intrinsic="ReflectionTexture"; > = sampler_state
{
    MipFilter = NONE;
    MinFilter = LINEAR;
    MagFilter = LINEAR;
    
    AddressU = CLAMP;
    AddressV = CLAMP;
    AddressW = CLAMP;
};

