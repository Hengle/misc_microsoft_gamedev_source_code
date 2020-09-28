// This file MUST be kept in sync with work\xeffects\shared\intrinsics.inc!

// Shared
DECLARE_EFFECT_INTRINSIC(WorldToProj,             cIntrinsicTypeFloat4x4,       1, true)

DECLARE_EFFECT_INTRINSIC(DirLightEnabled,         cIntrinsicTypeBool,           1, true)

DECLARE_EFFECT_INTRINSIC(DirShadowWorldToTex,     cIntrinsicTypeFloat4x4,       1, true)
DECLARE_EFFECT_INTRINSIC(DirShadowParams0,        cIntrinsicTypeFloat4,         1, true)
DECLARE_EFFECT_INTRINSIC(DirShadowParams1,        cIntrinsicTypeFloat4,         1, true)
DECLARE_EFFECT_INTRINSIC(DirShadowZScales,        cIntrinsicTypeFloat4,         8, true)

DECLARE_EFFECT_INTRINSIC(WorldCameraPos,          cIntrinsicTypeFloat3,         1, true)

DECLARE_EFFECT_INTRINSIC(DirLightVecToLightWorld, cIntrinsicTypeFloat3,         1, true)

DECLARE_EFFECT_INTRINSIC(DirLightColor,           cIntrinsicTypeFloat4,         1, true)

DECLARE_EFFECT_INTRINSIC(PlanarFogEnabled,        cIntrinsicTypeBool,           1, true)
DECLARE_EFFECT_INTRINSIC(FogColor,                cIntrinsicTypeFloat3,         1, true)
DECLARE_EFFECT_INTRINSIC(PlanarFogColor,          cIntrinsicTypeFloat3,         1, true)
DECLARE_EFFECT_INTRINSIC(FogParams,               cIntrinsicTypeFloat4,         1, true)

DECLARE_EFFECT_INTRINSIC(SHFillAr,                cIntrinsicTypeFloat4,         1, true)
DECLARE_EFFECT_INTRINSIC(SHFillAg,                cIntrinsicTypeFloat4,         1, true)
DECLARE_EFFECT_INTRINSIC(SHFillAb,                cIntrinsicTypeFloat4,         1, true)
DECLARE_EFFECT_INTRINSIC(SHFillBr,                cIntrinsicTypeFloat4,         1, true)
DECLARE_EFFECT_INTRINSIC(SHFillBg,                cIntrinsicTypeFloat4,         1, true)
DECLARE_EFFECT_INTRINSIC(SHFillBb,                cIntrinsicTypeFloat4,         1, true)
DECLARE_EFFECT_INTRINSIC(SHFillC,                 cIntrinsicTypeFloat4,         1, true)

DECLARE_EFFECT_INTRINSIC(SceneWorldToProj,        cIntrinsicTypeFloat4x4,       1, true)

// Unshared
DECLARE_EFFECT_INTRINSIC(WorldToView,             cIntrinsicTypeFloat4x4,       1, false)
DECLARE_EFFECT_INTRINSIC(ViewToProj,              cIntrinsicTypeFloat4x4,       1, false)
DECLARE_EFFECT_INTRINSIC(DirShadowWorldToProj,    cIntrinsicTypeFloat4x4,       1, false)

DECLARE_EFFECT_INTRINSIC(ProjToScreen,            cIntrinsicTypeFloat4x4,       1, false)
DECLARE_EFFECT_INTRINSIC(ViewToWorld,             cIntrinsicTypeFloat4x4,       1, false)
DECLARE_EFFECT_INTRINSIC(ScreenToProj,            cIntrinsicTypeFloat4x4,       1, false)
DECLARE_EFFECT_INTRINSIC(ScreenToView,            cIntrinsicTypeFloat4x4,       1, false)

DECLARE_EFFECT_INTRINSIC(TimeLoop,                cIntrinsicTypeFloat,          1, false)

DECLARE_EFFECT_INTRINSIC(DirShadowMapTexture,     cIntrinsicTypeTexturePtr,     1, false)
DECLARE_EFFECT_INTRINSIC(DirShadowMapTextureSqr,  cIntrinsicTypeTexturePtr,     1, false)
DECLARE_EFFECT_INTRINSIC(LightTexture,            cIntrinsicTypeTexturePtr,     1, false)
DECLARE_EFFECT_INTRINSIC(LocalShadowMapTexture,   cIntrinsicTypeTexturePtr,     1, false)

DECLARE_EFFECT_INTRINSIC(ReflectionTexture,       cIntrinsicTypeTexturePtr,     1, false)
