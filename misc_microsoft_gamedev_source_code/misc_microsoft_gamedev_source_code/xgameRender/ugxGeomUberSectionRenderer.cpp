//============================================================================
//
//  ugxGeomUberSectionRenderer.cpp
//  
//  Copyright (c) 2006-2007, Ensemble Studios
//
//============================================================================
#include "xgameRender.h"
#include "ugxGeomUberSectionRenderer.h"
#include "ugxGeomUberEffectManager.h"
#include "TerrainHeightField.h"

// xcore
#include "consoleOutput.h"

// shaders
#include "defConstRegs.inc"
#include "..\shaders\ugx\vShaderRegs.inc"
#include "..\shaders\ugx\pShaderRegs.inc"
#include "..\shaders\shared\localLightingRegs.inc"
#if VSHADER_REGS_VER != 118 
   #error Please update vShaderRegs.inc
#endif
#if PSHADER_REGS_VER != 120
   #error Please update pShaderRegs.inc
#endif

// xrender/xgameRender
#include "BD3D.h"
#include "ugxGeomData.h"
#include "ugxGeomManager.h"
#include "renderEventClasses.h"
#include "xcolorUtils.h"
#ifndef BUILD_FINAL
   #include "waterManager.h"
#endif
#include "D3DTextureManager.h"
#include "vertexDeclUtils.h"
#include "effectBinder.h"
#include "visibleLightManager.h"
#include "deviceStateDumper.h"

#define MIPVIS_TEXTURE_PATH "test\\sw_texel_01\\red1x1_02_df"

const uint cMaxUVChannels = 3;

// Important: Update this macro if you change the number of registers for vertex/pixel texture samplers (the /Xsampreg option).
#define FIRST_VERTEX_SHADER_SAMPLER (21)

// Samplers used in vertex shaders
#define UGX_D3DVERTEXTEXTURESAMPLER0 ((DWORD) (FIRST_VERTEX_SHADER_SAMPLER+0))
#define UGX_D3DVERTEXTEXTURESAMPLER1 ((DWORD) (FIRST_VERTEX_SHADER_SAMPLER+1))
#define UGX_D3DVERTEXTEXTURESAMPLER2 ((DWORD) (FIRST_VERTEX_SHADER_SAMPLER+2))
#define UGX_D3DVERTEXTEXTURESAMPLER3 ((DWORD) (FIRST_VERTEX_SHADER_SAMPLER+3))


//============================================================================
// enum eTechniquePasses
//============================================================================
enum eTechniquePasses
{
   cTPSkinned,
   cTPRigid,
   cTPSkinnedTerrainConform,
   cTPRigidTerrainConform,
         
   cTPGenericPixelShader,
   cTPGenericNoShadowPixelShader,
   
   cTPFirstOptimized
};

//============================================================================
// Gobals
//============================================================================
static double gCurTime;
static DWORD gZWriteEnable;
static DWORD gCullMode;
static DWORD gDepthBias;
static DWORD gStencilRef;
static DWORD gSeperateAlphaBlendEnabled;
static DWORD gHiStencilWriteEnable;
static BManagedTextureHandle gGlobalEnvMap;
static eUGXGeomTextureMode gTextureMode;
static BOOL gEnableLocalPixelLights;
static BOOL gEnableLocalLightShadows;
static BOOL gBlackmapEnabled;

static uint gTotalShadersSet;
static uint gTotalOptimizedShadersUsed;

#ifndef BUILD_FINAL
static BD3DTextureManager::BManagedTexture* gpMipVisTexture = NULL;
#endif   

#ifndef BUILD_FINAL
typedef BHashMap<uint64, uint, BHasher<uint64>, BEqualTo<uint64>, true, BRenderFixedHeapAllocator> BUInt64HashMap;
static BUInt64HashMap gOptimizedShaderHistogram;
static BUInt64HashMap gUnoptimizedShaderHistogram;
static BDynamicRenderArray<BString> gOptimizedShaderParamNames;
static BCriticalSection gOptimizedShaderParamStatLock;
#endif

//============================================================================
// BUGXGeomUberSectionRenderer::BUGXGeomUberSectionRenderer
//============================================================================
BUGXGeomUberSectionRenderer::BUGXGeomUberSectionRenderer() :
   IUGXGeomSectionRenderer(),
   mLoadedTextures(false),
   mRigid(false),
   mDeclHasTangent(false),
   mPixelXForm(false),
   mHasOpacityMap(false),
   mBaseEffectIndex(0),
   mValidMapFlags(0),
   mSkyMaterial(false),
   mRequiresBlending(false),
   mUVScroll(false),
   mUVScrollFlags(0),
   mOpacityValid(false),
   mOpacity(1.0f),
   mUVWrapFlags(0),
   mTwoSided(false),
   mZWrites(false),
   mGlobalEnvMapping(false),
   mNeedsUVChannels(false),
   mNeedsSpecular(true),
   mTerrainConform(false),
   mpDecl(NULL),
   mStream0Size(0), 
   mStream1Size(0)
{
   Utils::ClearObj(mpTextures);
   Utils::ClearObj(mNumTextureFrames);
   Utils::ClearObj(mUVChannel);
   Utils::ClearObj(mUVWVelocity);
   
   for (uint i = 0; i < cMaxOptimizedPassIndices; i++)
      mOptimizedPassIndices[i] = -1;

   //mEventHandle = gEventDispatcher.addClient(this);
}

//============================================================================
// BUGXGeomUberSectionRenderer::~BUGXGeomUberSectionRenderer
//============================================================================
BUGXGeomUberSectionRenderer::~BUGXGeomUberSectionRenderer()
{
   deinit();
}

//============================================================================
// BUGXGeomUberSectionRenderer::init
//============================================================================
bool BUGXGeomUberSectionRenderer::init(IDirect3DVertexDeclaration9* pDecl, const Unigeom::BMaterial& material, const UnivertPackerType& vertexPacker)
{
   deinit();
   
   mpDecl = pDecl;
   
   const uint cMaxElements = 64;
   uint numElements = cMaxElements;
   D3DVERTEXELEMENT9 elements[cMaxElements];
   pDecl->GetDeclaration(elements, &numElements);
   
   mStream0Size = static_cast<uchar>(BVertexDeclUtils::getVertexDeclarationStreamVertexSize(elements, 0));
   mStream1Size = static_cast<uchar>(BVertexDeclUtils::getVertexDeclarationStreamVertexSize(elements, 1));
     
   if (!initTextures(material))
      return false;

   if (!initEffect(material, vertexPacker))
      return false;

   return true;
}

//============================================================================
// BUGXGeomUberSectionRenderer::deinit
//============================================================================
void BUGXGeomUberSectionRenderer::deinit(void)
{
   for (uint i = 0; i < Unigeom::BMaterial::cNumMapTypes; i++)
      for (uint j = 0; j < Unigeom::BMaterial::cMaxMultiframeTextures; j++)
         if ((mpTextures[i][j]) && (!gD3DTextureManager.isDefaultTexture(mpTextures[i][j])))
            mpTextures[i][j]->release();
         
   Utils::ClearObj(mpTextures);
   Utils::ClearObj(mNumTextureFrames);
   Utils::ClearObj(mUVChannel);
   Utils::ClearObj(mUVWVelocity);

   mpDecl = NULL;
   mStream0Size = 0;
   mStream1Size = 0;
   
   mBlendType = cBlendAlphaToCoverage;
   mLayerFlags = 0;
   mDisableShadows = false;
   
   mBaseEffectIndex = 0;
   
   mValidMapFlags = 0;
   mRigid = false;
   mDeclHasTangent = true;
   mPixelXForm = false;
   mHasOpacityMap = false;
   mSkyMaterial = false;
   mRequiresBlending = false;
   mUVScroll = false;
   
   mOpacityValid = false;
   mOpacity = 1.0f;
   
   mTwoSided = false;
   mZWrites = false;
   mGlobalEnvMapping = false;
   mTerrainConform = false;
   mLocalReflection = false;
   
   mUVScrollFlags = 0;
   
   mUVWrapFlags = 0;
   
   mNeedsUVChannels = false;
   
   mEffectManagerLoadCount = UINT_MAX;
   for (uint i = 0; i < cMaxOptimizedPassIndices; i++)
      mOptimizedPassIndices[i] = -1;
   
   mNeedsSpecular = true;
}

//============================================================================
// BUGXGeomUberSectionRenderer::getDefaultTexture
//============================================================================
eDefaultTexture BUGXGeomUberSectionRenderer::getDefaultTexture(Unigeom::BMaterial::eMapType typeIndex)
{
   eDefaultTexture defaultTexture = cDefaultTextureBlack;

   switch (typeIndex)
   {
      case Unigeom::BMaterial::cNormal:
      {
         defaultTexture = cDefaultTextureNormal;
         break;
      }
      case Unigeom::BMaterial::cOpacity:
      case Unigeom::BMaterial::cDiffuse: 
      case Unigeom::BMaterial::cXForm:
      case Unigeom::BMaterial::cEmXForm:
      case Unigeom::BMaterial::cAmbOcc:
      case Unigeom::BMaterial::cGloss:
      {
         defaultTexture = cDefaultTextureWhite;
         break;
      }
      case Unigeom::BMaterial::cEnv:
      case Unigeom::BMaterial::cEmissive:
      {
         // White, not black, because these are packed DXT5H textures and RGB=255,255,255 is black once unpacked.
         defaultTexture = cDefaultTextureWhite;
         break;
      }
   }
   
   return defaultTexture;
}

//============================================================================
// BUGXGeomUberSectionRenderer::getIsSRGBTexture
//============================================================================
bool BUGXGeomUberSectionRenderer::getIsSRGBTexture(Unigeom::BMaterial::eMapType typeIndex)
{
   bool srgbTexture = false;
   switch (typeIndex)
   {
      case Unigeom::BMaterial::cDiffuse:
      case Unigeom::BMaterial::cGloss:
      case Unigeom::BMaterial::cEnvMask:
         // Opacity and xform are grayscale, but they are most likely authored using a 2.2 gamma.
      case Unigeom::BMaterial::cOpacity:
      case Unigeom::BMaterial::cXForm:
      case Unigeom::BMaterial::cEmXForm:
      {
         srgbTexture = true;
         break;
      }
   }
   
   return srgbTexture;
}

//============================================================================
// BUGXGeomUberSectionRenderer::initTextures
//============================================================================
bool BUGXGeomUberSectionRenderer::initTextures(const Unigeom::BMaterial& material)
{
#ifndef BUILD_FINAL
   if (!gpMipVisTexture)
      gpMipVisTexture = gD3DTextureManager.getOrCreate(MIPVIS_TEXTURE_PATH, BFILE_OPEN_DISCARD_ON_CLOSE, BD3DTextureManager::cUGXCommon, true, cDefaultTextureWhite, true, false, "UGX");
#endif   

   const BString* pSPCTexturePrefix = NULL;
   uint spcPrefixIndex;
   if (gUGXGeomManager.getOptions().find("SPCTexturePrefix", spcPrefixIndex))
      pSPCTexturePrefix = &gUGXGeomManager.getOptions().getString(spcPrefixIndex);

   bool usingSPCPrefix = false;
   
   Utils::ClearObj(mpTextures);
   Utils::ClearObj(mNumTextureFrames);
   
   for (uint typeIndex = 0; typeIndex < Unigeom::BMaterial::cNumMapTypes; typeIndex++)
   {
      const eDefaultTexture defaultTexture = getDefaultTexture(static_cast<Unigeom::BMaterial::eMapType>(typeIndex));
      const bool srgbTexture = getIsSRGBTexture(static_cast<Unigeom::BMaterial::eMapType>(typeIndex));
                  
      if (!material.getNumMaps(static_cast<Unigeom::BMaterial::eMapType>(typeIndex)))  
         continue;
               
      if (!material.getMap(static_cast<Unigeom::BMaterial::eMapType>(typeIndex), 0).getName().length())
         continue;

      BRenderString name(material.getMap(static_cast<Unigeom::BMaterial::eMapType>(typeIndex), 0).getName());
      name.standardizePath();

      const long multiframeStringIndex = name.findLeft(UGX_MATERIAL_MULTIFRAME_SUBSTRING);
      bool isMultiFrame = (multiframeStringIndex >= 0);
      
      BDynamicRenderArray<BRenderString> filenames;
      filenames.pushBack(name);
      
      uint numMaps = 1;
      
      if (isMultiFrame)
      {
         uint i;
         for (i = 2; i <= Unigeom::BMaterial::cMaxMultiframeTextures; i++)
         {
            BRenderString filename;
            filename.set(name.getPtr());
            
            const uint frameIndex = i;
            filename.setChar(multiframeStringIndex + UGX_MATERIAL_MULTIFRAME_INDEX_OFS, (frameIndex / 10) + '0');            
            filename.setChar(multiframeStringIndex + UGX_MATERIAL_MULTIFRAME_INDEX_OFS + 1, (frameIndex % 10) + '0');            
            
            BRenderString actualFilename;
            if (!gD3DTextureManager.getActualFilename(filename, actualFilename))
               break;
               
            filenames.pushBack(actualFilename);
         }
         
         numMaps = i - 1;         
         if (numMaps == 1)
            isMultiFrame = false;
      }
      
      BDEBUG_ASSERT(filenames.getSize() == numMaps);
      
      BDEBUG_ASSERT(numMaps <= UCHAR_MAX);
      mNumTextureFrames[typeIndex] = static_cast<uchar>(numMaps);
      
      for (uint mapIndex = 0; mapIndex < numMaps; mapIndex++)
      {
         mpTextures[typeIndex][mapIndex] = NULL;
         
         BRenderString filename(filenames[mapIndex]);

         if (isMultiFrame)
         {
            const uint frameIndex = mapIndex + 1;
            filename.setChar(multiframeStringIndex + UGX_MATERIAL_MULTIFRAME_INDEX_OFS, (frameIndex / 10) + '0');            
            filename.setChar(multiframeStringIndex + UGX_MATERIAL_MULTIFRAME_INDEX_OFS + 1, (frameIndex % 10) + '0');            
         }
         
         if ( (pSPCTexturePrefix) && 
              ((typeIndex == Unigeom::BMaterial::cDiffuse) || (typeIndex == Unigeom::BMaterial::cNormal) || (typeIndex == Unigeom::BMaterial::cGloss)) )
         {
            BRenderString path;
            BRenderString name;
            strPathGetDirectory(filename, path, true);
            strPathGetFilename(filename, name);
            
            BRenderString spcTextureFilename;
            spcTextureFilename.format("%s%s%s", path.getPtr(), pSPCTexturePrefix->getPtr(), name.getPtr());
            
            BRenderString actualSpcTextureFilename;
            if (gD3DTextureManager.getActualFilename(spcTextureFilename, actualSpcTextureFilename))
            {
               mpTextures[typeIndex][mapIndex] = gD3DTextureManager.getOrCreate(
                  spcTextureFilename, BFILE_OPEN_DISCARD_ON_CLOSE, BD3DTextureManager::cUGXMaterial, srgbTexture, defaultTexture, false, true, "UGX", true);
                  
               usingSPCPrefix = true;
            }
         }
         
         if (!mpTextures[typeIndex][mapIndex])
         {
            if ((usingSPCPrefix) && ((typeIndex == Unigeom::BMaterial::cXForm) || (typeIndex == Unigeom::BMaterial::cEmXForm)))
            {
               // disable pixel xform if we're using the SPC prefix
            }
            else
            {
              mpTextures[typeIndex][mapIndex] = gD3DTextureManager.getOrCreate(
                  filename, BFILE_OPEN_DISCARD_ON_CLOSE, BD3DTextureManager::cUGXMaterial, srgbTexture, defaultTexture, false, true, "UGX", true);
            }                  
         }               
      }
      
      if (!mpTextures[typeIndex][0])
      {
         mpTextures[typeIndex][0] = gD3DTextureManager.getDefaultTexture(defaultTexture);
         mNumTextureFrames[typeIndex] = 1;
      }
   }

   // Load textures immediately if archives aren't enabled. Otherwise, they will be loaded all at once before the scenario's first frame.
   if ( ((gFileManager.getConfigFlags() & BFileManager::cWillBeUsingArchives) == 0) && (!gD3DTextureManager.getBackgroundLoadingEnabled()) )
   {
      loadTextures();
   }
   
   return true;
}

static const float gUVSelectorValues[3] = { 0.0f, 1.0f, -1.0f };

//============================================================================
// BUGXGeomUberSectionRenderer::getUVSelector
//============================================================================
float BUGXGeomUberSectionRenderer::getUVSelector(const Unigeom::BMaterial& mat, Unigeom::BMaterial::eMapType mapType)
{
   BCOMPILETIMEASSERT((sizeof(gUVSelectorValues) / sizeof(gUVSelectorValues[0])) == cMaxUVChannels);
      
   const uint numMaps = mat.getNumMaps(mapType);
   
   uint channelIndex = 0;
   
   if (numMaps) 
      channelIndex = Math::Min<uint>(mat.getMap(mapType, 0).getChannel(), cMaxUVChannels - 1U);
   
   return gUVSelectorValues[channelIndex];
}

//============================================================================
// BUGXGeomUberSectionRenderer::findPass
//============================================================================
void BUGXGeomUberSectionRenderer::findPass(void)
{
   for (uint i = 0; i < cMaxOptimizedPassIndices; i++)
      mOptimizedPassIndices[i] = -1;
      
   BFXLEffect& effect = gUGXGeomUberEffectManager.getEffect(mDeclHasTangent, mDisableShadowReception);
   
   if (!effect.getEffect())
      return;
       
   struct BAnnotation
   {
      const char* mpName;
      bool mValue;
   };
   
   const uint cMaxAnnotations = 32;
   BAnnotation annotations[cMaxAnnotations];
   uint mNumAnnotations = 0;
   uint64 paramValueFlags = 0;
   
#define DEFINE_ANNOTATION(name, val) { annotations[mNumAnnotations].mpName = #name; annotations[mNumAnnotations].mValue = val; if (val) paramValueFlags |= ((uint64)1U << ((uint64)mNumAnnotations)); mNumAnnotations++; }
   // Only static MATERIAL properties should be checked here.  
   // Must match the order and annotation names of the DEFINE_PASS macro!
   DEFINE_ANNOTATION(specFlag,            mNeedsSpecular);
   DEFINE_ANNOTATION(dirlFlag,            true);        
   DEFINE_ANNOTATION(dirsFlag,            !mDisableShadowReception);         
   DEFINE_ANNOTATION(specmapFlag,         isValidMap(Unigeom::BMaterial::cGloss));
   DEFINE_ANNOTATION(opmapFlag,           isValidMap(Unigeom::BMaterial::cOpacity));    
   DEFINE_ANNOTATION(xformmapFlag,        isValidMap(Unigeom::BMaterial::cXForm));   
   DEFINE_ANNOTATION(emxformmapFlag,      isValidMap(Unigeom::BMaterial::cEmXForm));    
   DEFINE_ANNOTATION(selfmapFlag,         isValidMap(Unigeom::BMaterial::cEmissive)); 
   DEFINE_ANNOTATION(aomapFlag,           isValidMap(Unigeom::BMaterial::cAmbOcc));     
   DEFINE_ANNOTATION(envmapFlag,          isValidMap(Unigeom::BMaterial::cEnv) || mGlobalEnvMapping);
   DEFINE_ANNOTATION(envmaskmapFlag,      isValidMap(Unigeom::BMaterial::cEnvMask));
   DEFINE_ANNOTATION(twosidedFlag,        mTwoSided);  
   DEFINE_ANNOTATION(uvselFlag,           mNeedsUVChannels);    
   DEFINE_ANNOTATION(uvofsFlag,           mUVScroll);
   DEFINE_ANNOTATION(localReflectionFlag, mLocalReflection);
   DEFINE_ANNOTATION(highlightMapFlag,    isValidMap(Unigeom::BMaterial::cHighlight));
   DEFINE_ANNOTATION(modulateMapFlag,     isValidMap(Unigeom::BMaterial::cModulate));
#undef DEFINE_ANNOTATION
   
   BFXLEffectTechnique technique(effect.getTechniqueFromIndex(cUberEffectTechniqueVisible));
            
   for (uint passIndex = 0; passIndex < technique.getNumPasses(); passIndex++)
   {
      BFXLEffectPass pass(technique.getPassFromIndex(passIndex));
      
      if (!pass.getNumAnnotations())
         continue;
         
      uint i;
      for (i = 0; i < mNumAnnotations; i++)
      {
         BFXLEffectAnnotation annotation(pass.getAnnotation(annotations[i].mpName)); 
         if ((!annotation.isValid()) || (!annotation.isBool()))  
         { 
            BASSERT(0); 
            break;
         } 
         if ((annotation.getBool()) != (annotations[i].mValue)) 
            break; 
      }
      if (i < mNumAnnotations)
         continue;
                  
      BFXLEffectAnnotation localLightingAnnotation(pass.getAnnotation("loclFlag"));
      BASSERT(localLightingAnnotation.isValid() && localLightingAnnotation.isBool());
      
      if (localLightingAnnotation.getBool())      
         mOptimizedPassIndices[cLocalLightingOptimizedPassIndex] = static_cast<short>(passIndex);
      else
         mOptimizedPassIndices[cNormalOptimizedPassIndex] = static_cast<short>(passIndex);
                  
      for (i = 0; i < cMaxOptimizedPassIndices; i++)
         if (mOptimizedPassIndices[i] < 0)
            break;
      
      if (i == cMaxOptimizedPassIndices)
         break;
   }

   for (uint i = 0; i < cMaxOptimizedPassIndices; i++)
   {
      if (mOptimizedPassIndices[i] < 0)
         mOptimizedPassIndices[i] = -2;
   }
   
#ifndef BUILD_FINAL
   {
      BScopedCriticalSection lock(gOptimizedShaderParamStatLock);

      if (gOptimizedShaderParamNames.empty())
      {
         for (uint i = 0; i < mNumAnnotations; i++)
            gOptimizedShaderParamNames.pushBack(BString(annotations[i].mpName));
      }
   
      // Or in the bump vs. non-bump effect for the histogram
      if (mDeclHasTangent)
         paramValueFlags |= ((uint64)1U << ((uint64)mNumAnnotations));

      for (uint i = 0; i < cMaxOptimizedPassIndices; i++)
      {
         if (mOptimizedPassIndices[i] < 0)
         {
            BUInt64HashMap::InsertResult result = gUnoptimizedShaderHistogram.insert(paramValueFlags, 0);
            result.first->second = result.first->second++;
         }
         else
         {
            BUInt64HashMap::InsertResult result = gOptimizedShaderHistogram.insert(paramValueFlags, 0);
            result.first->second = result.first->second++;
         }
      }
   }      
#endif   
            
   mEffectManagerLoadCount = gUGXGeomUberEffectManager.getLoadCount();   
}

//============================================================================
// BUGXGeomUberSectionRenderer::initEffect
//============================================================================
bool BUGXGeomUberSectionRenderer::initEffect(const Unigeom::BMaterial& material, const UnivertPackerType& vertexPacker)
{
   const BNativeUnivertPackerType::ElementStats baseStats(vertexPacker.getStats());

   const bool normals      = baseStats.numNorm != 0;
   const bool skin         = baseStats.numSkin != 0; 
   const bool hasTangent   = baseStats.numTangent != 0;
   const int numUVCoords   = baseStats.numTexCoords;
   
   BDEBUG_ASSERT(normals && (numUVCoords <= cMaxUVChannels));
   normals;

   mRigid = !skin;
   
   mDeclHasTangent = hasTangent;
   
   mPixelXForm = material.getNumMaps(Unigeom::BMaterial::cXForm) > 0;
   mHasOpacityMap = material.getNumMaps(Unigeom::BMaterial::cOpacity) > 0;

   mValidMapFlags = 0;
   for (uint mapTypeIter = 0; mapTypeIter < Unigeom::BMaterial::cNumMapTypes; mapTypeIter++)
   {
      if (mpTextures[mapTypeIter][0])
      {
         if (!gD3DTextureManager.isDefaultTexture(mpTextures[mapTypeIter][0]))
            mValidMapFlags |= (1 << mapTypeIter);
      }
   }

   mSpecColorPower = BVec4(material.getSpecColor(), material.getSpecPower());
   mEnvControl = BVec4(material.getEnvFresnelPower(), Math::Clamp(material.getEnvSharpness(), 0.0f, 13.0f), material.getEnvFresnel(), material.getEnvReflectivity());
   
   mNeedsSpecular = true;
   if ((!isValidMap(Unigeom::BMaterial::cGloss)) &&
       (mSpecColorPower[0] == 0.0f) &&
       (mSpecColorPower[1] == 0.0f) &&
       (mSpecColorPower[2] == 0.0f))
   {
      mNeedsSpecular = false;
   }

   // Convert to approximate sRGB.
   for (uint i = 0; i < 3; i++)
      mSpecColorPower[i] = powf(mSpecColorPower[i], 2.2f);
         
   mBlendType = cBlendAlphaToCoverage;
   mLayerFlags = 0;
   
   if (isValidMap(Unigeom::BMaterial::cDistortion))
      mLayerFlags |= cUGXGeomLayerDistortion;
   else
   {
      switch (material.getBlendType())
      {
         case Unigeom::BMaterial::cBlendAlphaToCoverage:   mBlendType = cBlendAlphaToCoverage; mLayerFlags |= cUGXGeomLayerOpaque; break;
         case Unigeom::BMaterial::cBlendAlphaTest:         mBlendType = cBlendAlphaTest;       mLayerFlags |= cUGXGeomLayerOpaque; break;
         case Unigeom::BMaterial::cBlendAdditive:          mBlendType = cBlendAdditive;        mLayerFlags |= cUGXGeomLayerAdditive; break;
         case Unigeom::BMaterial::cBlendOver:              mBlendType = cBlendOver;            mLayerFlags |= cUGXGeomLayerOver; break;
         default: 
            BDEBUG_ASSERT(0);
            mLayerFlags |= cUGXGeomLayerOpaque; 
            break;
      }
   }      
            
   mRequiresBlending = ((mBlendType == cBlendAdditive) || (mBlendType == cBlendOver));
   
   mNumUV = static_cast<uchar>(numUVCoords);
   
   mOpacityValid = material.getFlag(Unigeom::BMaterial::cFlagOpacityValid);
   mOpacity = mOpacityValid ? (material.getOpacity() * 1.0f/255.0f) : 1.0f;
      
   mTwoSided = material.getFlag(Unigeom::BMaterial::cFlagTwoSided);
   mTerrainConform = material.getFlag(Unigeom::BMaterial::cFlagTerrainConform);
   mDisableShadowReception = material.getFlag(Unigeom::BMaterial::cFlagDisableShadowReception);
   
   // Diffuse Bump Spec Opacity XForm Self AO EnvMask EMXForm
   // x       y    z    w       x     y    z  w        x
   BCOMPILETIMEASSERT(NUM_UV_CHANNEL_REGS == sizeof(mUVChannel)/sizeof(mUVChannel[0]));
   
   mUVChannel[0][0] = getUVSelector(material, Unigeom::BMaterial::cDiffuse);
   mUVChannel[0][1] = getUVSelector(material, Unigeom::BMaterial::cNormal);    
   mUVChannel[0][2] = getUVSelector(material, Unigeom::BMaterial::cGloss);
   mUVChannel[0][3] = getUVSelector(material, Unigeom::BMaterial::cOpacity);
   mUVChannel[1][0] = getUVSelector(material, Unigeom::BMaterial::cXForm);
   mUVChannel[1][1] = getUVSelector(material, Unigeom::BMaterial::cEmissive);
   mUVChannel[1][2] = getUVSelector(material, Unigeom::BMaterial::cAmbOcc);
   mUVChannel[1][3] = getUVSelector(material, Unigeom::BMaterial::cEnvMask);
   mUVChannel[2][0] = getUVSelector(material, Unigeom::BMaterial::cEmXForm);
   mUVChannel[2][1] = getUVSelector(material, Unigeom::BMaterial::cDistortion);
   mUVChannel[2][2] = getUVSelector(material, Unigeom::BMaterial::cHighlight);
   mUVChannel[2][3] = getUVSelector(material, Unigeom::BMaterial::cModulate);
   
   mNeedsUVChannels = (mUVChannel[0] != BVec4(0.0f)) || (mUVChannel[1] != BVec4(0.0f)) || (mUVChannel[2] != BVec4(0.0f)) || (mUVChannel[3] != BVec4(0.0f));
   
   mUVScroll = false;
   mUVScrollFlags = 0;
   Utils::ClearObj(mUVWVelocity);
   mUVWrapFlags = 0;
   for (uint i = 0; i < Unigeom::BMaterial::cNumMapTypes; i++)
   {
      Unigeom::BMaterial::eMapType mapType = static_cast<Unigeom::BMaterial::eMapType>(i);
      
      if (i == Unigeom::BMaterial::cEnv)
      {
         mUVWVelocity[i].clear();
      }
      else
      {
         mUVWVelocity[i] = material.getUVWVelocity(mapType);
         if (mUVWVelocity[i].len() > 0.0000125f)
         {
            mUVScroll = true;
            mUVScrollFlags |= (1 << i);
            mUVWrapFlags |= (1 << i);
         }
      }         
      
      if (material.getNumMaps(mapType) > 0)
      {
         if ( (!material.getMap(mapType, 0).getClampMode(0)) || (!material.getMap(mapType, 0).getClampMode(1)) )
         {
            mUVWrapFlags |= (1 << i);
         }
      }   
   }
   
   mUVWrapFlags &= ~(1 << Unigeom::BMaterial::cEnv);
   
   mSkyMaterial = false;
   if (stricmp(material.getName(), "sky") == 0)
   {
      mSkyMaterial = true;

      gConsoleOutput.output(cMsgResource, "BUGXGeomUberSectionRenderer::initEffect: Found sky material!");
   }

   mLocalReflection = material.getFlag(Unigeom::BMaterial::cFlagLocalReflection);

   mDisableShadows = material.getFlag(Unigeom::BMaterial::cFlagDisableShadows);
   
   mZWrites = true;
   
   if ( (mBlendType == cBlendAdditive) ||
        ((mOpacityValid) && (mBlendType == cBlendOver)) ||
        mTerrainConform )
   {
      mZWrites = false;
   }        
   
   mGlobalEnvMapping = material.getFlag(Unigeom::BMaterial::cFlagGlobalEnv);
   
   mEffectManagerLoadCount = UINT_MAX;
      
   findPass();
   
   return true;
}

//============================================================================
// BUGXGeomUberSectionRenderer::loadTextures
//============================================================================
void BUGXGeomUberSectionRenderer::loadTextures(void)
{
   for (uint i = 0; i < Unigeom::BMaterial::cNumMapTypes; i++)
   {
      for (uint j = 0; j < Unigeom::BMaterial::cMaxMultiframeTextures; j++)
      {
         if (mpTextures[i][j])
            mpTextures[i][j]->load();
      }
   }
   
   mLoadedTextures = true;
}

//============================================================================
// BUGXGeomUberSectionRenderer::unloadTextures
//============================================================================
void BUGXGeomUberSectionRenderer::unloadTextures(void)
{
   for (uint i = 0; i < Unigeom::BMaterial::cNumMapTypes; i++)
      for (uint j = 0; j < Unigeom::BMaterial::cMaxMultiframeTextures; j++)
         if (mpTextures[i][j])
            mpTextures[i][j]->unload();
}

//============================================================================
// BUGXGeomUberSectionRenderer::setTexture
//============================================================================
void BUGXGeomUberSectionRenderer::setTexture(Unigeom::BMaterial::eMapType type, int samplerReg, uint multiframeTextureIndex, const BD3DTexture* pOverrideTex)
{
   uint numTextureFrames = mNumTextureFrames[type];
   if (!numTextureFrames)
      multiframeTextureIndex = 0;
   else if (multiframeTextureIndex >= mNumTextureFrames[type])
      multiframeTextureIndex = mNumTextureFrames[type] - 1;
      
//-- FIXING PREFIX BUG ID 6619
   const BD3DTextureManager::BManagedTexture* pTexture = mpTextures[type][multiframeTextureIndex];
//--
   
   IDirect3DBaseTexture9* pD3DTexture;
   
   if (pOverrideTex)
      pD3DTexture = pOverrideTex->getBaseTexture();
   else if (pTexture)
      pD3DTexture = pTexture->getD3DTexture().getBaseTexture();
   else
      pD3DTexture = gD3DTextureManager.getDefaultTexture(getDefaultTexture(type))->getD3DTexture().getBaseTexture();
   
   gpUGXGeomRSFilter->setTexture(samplerReg, pD3DTexture);
   
   const DWORD samplerStateValue = ((mUVWrapFlags & (1 << type)) != 0) ? D3DTADDRESS_WRAP : D3DTADDRESS_CLAMP;
   gpUGXGeomRSFilter->setSamplerState(samplerReg, D3DSAMP_ADDRESSU, samplerStateValue);
   gpUGXGeomRSFilter->setSamplerState(samplerReg, D3DSAMP_ADDRESSV, samplerStateValue);
   gpUGXGeomRSFilter->setSamplerState(samplerReg, D3DSAMP_ADDRESSW, samplerStateValue);
}

//============================================================================
// BUGXGeomUberSectionRenderer::calcUVOffset
//============================================================================
void BUGXGeomUberSectionRenderer::calcUVOffset(Unigeom::BMaterial::eMapType mapType, float& u, float& v)
{
   if (mUVScrollFlags & (1 << mapType))
   {
      u = Math::fPosMod(static_cast<float>(gCurTime * mUVWVelocity[mapType][0]), 128.0f);
      v = Math::fPosMod(static_cast<float>(gCurTime * mUVWVelocity[mapType][1]), 128.0f);
   }
   else
   {
      u = 0.0f;
      v = 0.0f;
   }
}

//============================================================================
// BUGXGeomUberSectionRenderer::calcUVWOffset
//============================================================================
void BUGXGeomUberSectionRenderer::calcUVWOffset(Unigeom::BMaterial::eMapType mapType, float& u, float& v, float& w)
{
   if (mUVScrollFlags & (1 << mapType))
   {
      u = Math::fPosMod(static_cast<float>(gCurTime * mUVWVelocity[mapType][0]), 128.0f);
      v = Math::fPosMod(static_cast<float>(gCurTime * mUVWVelocity[mapType][1]), 128.0f);
      w = Math::fPosMod(static_cast<float>(gCurTime * mUVWVelocity[mapType][2]), 128.0f);
   }
   else
   {
      u = 0.0f;
      v = 0.0f;
      w = 0.0f;
   }
}

//============================================================================
// BUGXGeomUberSectionRenderer::setUVConstants
//============================================================================
void BUGXGeomUberSectionRenderer::setUVConstants(void)
{
   BCOMPILETIMEASSERT(NUM_UV_CHANNEL_REGS == sizeof(mUVChannel)/sizeof(mUVChannel[0]));
   gpUGXGeomRSFilter->setPixelShaderConstantF(UV_CHANNEL_0_REG, (const float*)mUVChannel, NUM_UV_CHANNEL_REGS);
   
   // UV Ofs:
   // Diffuse, Normal, Gloss, Opacity, Emissive, EnvMask
   // 0        1       2      3        4         5
   // XY       ZW      XY     ZW       XY        ZW

   if (!mUVScroll)
   {
      gpUGXGeomRSFilter->setZeroPixelShaderConstantF(UV_OFFSET_0_REG, NUM_UV_OFFSET_REGS);
   }
   else
   {
      BVec4 uvOffsets[NUM_UV_OFFSET_REGS];
      
      BCOMPILETIMEASSERT(NUM_UV_OFFSET_REGS == 6);
      
      if (isValidMap(Unigeom::BMaterial::cDistortion))
         calcUVWOffset(Unigeom::BMaterial::cDistortion, uvOffsets[0][0], uvOffsets[0][1], uvOffsets[4][2]);
      else
         calcUVWOffset(Unigeom::BMaterial::cDiffuse, uvOffsets[0][0], uvOffsets[0][1], uvOffsets[4][2]);
         
      calcUVWOffset(Unigeom::BMaterial::cNormal,  uvOffsets[0][2], uvOffsets[0][3], uvOffsets[4][3]);
      calcUVOffset(Unigeom::BMaterial::cGloss,    uvOffsets[1][0], uvOffsets[1][1]);
      calcUVOffset(Unigeom::BMaterial::cOpacity,  uvOffsets[1][2], uvOffsets[1][3]);
      calcUVOffset(Unigeom::BMaterial::cEmissive, uvOffsets[2][0], uvOffsets[2][1]);
      calcUVOffset(Unigeom::BMaterial::cEnvMask,  uvOffsets[2][2], uvOffsets[2][3]);
      
      calcUVOffset(Unigeom::BMaterial::cAmbOcc,   uvOffsets[3][0], uvOffsets[3][1]);
      calcUVOffset(Unigeom::BMaterial::cXForm,    uvOffsets[3][2], uvOffsets[3][3]);
      calcUVOffset(Unigeom::BMaterial::cEmXForm,  uvOffsets[4][0], uvOffsets[4][1]);
      
      calcUVOffset(Unigeom::BMaterial::cHighlight, uvOffsets[5][0], uvOffsets[5][1]);
      calcUVOffset(Unigeom::BMaterial::cModulate, uvOffsets[5][2], uvOffsets[5][3]);
      
      gpUGXGeomRSFilter->setPixelShaderConstantF(UV_OFFSET_0_REG, (const float*)uvOffsets, NUM_UV_OFFSET_REGS);
   }
}

//============================================================================
// BUGXGeomUberSectionRenderer::beginPass
//============================================================================
bool BUGXGeomUberSectionRenderer::beginPass(const BUGXGeomRenderCommonInstanceData* pCommonData, void* pState, uint stateMaxSize, eUGXGeomVisMode visMode)
{
   //BDEBUG_ASSERT(stateMaxSize >= sizeof(BPassState));
   //BPassState& state = *reinterpret_cast<BPassState*>(pState);

   const eUGXGeomPass passIndex = pCommonData->mPass;

   BDEBUG_ASSERT((passIndex >= 0) && (passIndex < cUGXGeomPassMax));

   const bool dirShadowing = pCommonData->mDirLightShadows;

   BFXLEffect& effect = gUGXGeomUberEffectManager.getEffect(mDeclHasTangent, mDisableShadowReception);
   if (!effect.getEffect())
      return false;

   //effect.updateIntrinsicParams();

   // Currently unused
   BUberVisualRenderAttributes* pUberVisualRenderAttributes = NULL;
   if (pCommonData->mpExtendedAttributes)
   {
      if (pCommonData->mpExtendedAttributes->getType() == cEVRAUber)
         pUberVisualRenderAttributes = (BUberVisualRenderAttributes*)pCommonData->mpExtendedAttributes;
   }
   
   BOOL tex12Flags[2] = { mNumUV >= 2, mNumUV >= 3 };
   gpUGXGeomRSFilter->setVertexShaderConstantB(TEX1_REG, tex12Flags, 2);
   
   uint techniqueIndex = 0;
      
   int techniquePassIndex = mRigid ? cTPRigid : cTPSkinned;
   
   if (mTerrainConform &&
      ((passIndex == cUGXGeomPassMain) || (passIndex == cUGXGeomPassOverallAlpha) || (passIndex == cUGXGeomPassMainReflect) || (passIndex == cUGXGeomPassOverallAlphaReflect) || (passIndex == cUGXGeomPassDistortion)) &&
      !mSkyMaterial)
   {      
      techniquePassIndex = mRigid ? cTPRigidTerrainConform : cTPSkinnedTerrainConform;
   }
   
   int pixelShaderTechniquePassIndex = -1;
         
   const uint multiframeTextureIndex = pCommonData->mMultiframeTextureIndex;      

   switch (passIndex)
   {
      case cUGXGeomPassMain:
      case cUGXGeomPassOverallAlpha:
      case cUGXGeomPassMainReflect:
      case cUGXGeomPassOverallAlphaReflect:
      {
         const BOOL blackmapEnabled = pCommonData->mSampleBlackmap && gBlackmapEnabled;
                  
         XMVECTOR tintColor = BXColorUtils::D3DColorToLinearApprox(pCommonData->mTintColor);
         // rg [1/2/08] - HACK HACK - The reflection pass only has 2 bits for alpha, and the artists are using really small values for the material opacity on some objects
         // like trees which is causing them to not appear at all in the reflection.
         if (passIndex != cUGXGeomPassMainReflect)
            tintColor *= mOpacity;
         
         gpUGXGeomRSFilter->setPixelShaderConstantF(TINT_COLOR_REG, reinterpret_cast<const float*>(&tintColor), 1);      
      
         setUVConstants();
         
         gpUGXGeomRSFilter->setPixelShaderConstantB(ENABLE_BLACKMAP_REG, blackmapEnabled);
         
         BD3DTextureManager::BManagedTexture* pDiffuseTexOverride = NULL;
         BD3DTextureManager::BManagedTexture* pEmissiveTexOverride = NULL;
         BD3DTextureManager::BManagedTexture* pNormalMapOverride = NULL;

#ifndef BUILD_FINAL         
         switch (gTextureMode)
         {
            case cTMAllWhite:
            {
               pDiffuseTexOverride = gD3DTextureManager.getDefaultTexture(cDefaultTextureWhite);
               break;
            }
            case cTMMipVis:
            {
               pDiffuseTexOverride = gpMipVisTexture;
               break;
            }
            case cTMAllFlat:
            {
               pNormalMapOverride = gD3DTextureManager.getDefaultTexture(cDefaultTextureNormal);
               break;
            }
         }
#endif         
                  
         setTexture(Unigeom::BMaterial::cDiffuse,   DIFFUSE_SAMPLER_REG, multiframeTextureIndex, pDiffuseTexOverride ? &pDiffuseTexOverride->getD3DTexture() : NULL);
         setTexture(Unigeom::BMaterial::cNormal,    BUMP_SAMPLER_REG, multiframeTextureIndex, pNormalMapOverride ? &pNormalMapOverride->getD3DTexture() : NULL);
         setTexture(Unigeom::BMaterial::cGloss,     SPECULAR_SAMPLER_REG, multiframeTextureIndex);
         setTexture(Unigeom::BMaterial::cOpacity,   OPACITY_SAMPLER_REG, multiframeTextureIndex);
         setTexture(Unigeom::BMaterial::cXForm,     XFORM_SAMPLER_REG, multiframeTextureIndex);
         
         if (mGlobalEnvMapping)
         {
            setTexture(
               Unigeom::BMaterial::cEnv,       ENV_SAMPLER_REG, multiframeTextureIndex, 
               (gGlobalEnvMap == cInvalidManagedTextureHandle) ? &gD3DTextureManager.getDefaultTexture(cDefaultTextureWhite)->getD3DTexture() : &gD3DTextureManager.getManagedTextureByHandle(gGlobalEnvMap)->getD3DTexture());
         }               
         else
            setTexture(Unigeom::BMaterial::cEnv,       ENV_SAMPLER_REG, multiframeTextureIndex);
                           
         setTexture(Unigeom::BMaterial::cEnvMask,   ENV_MASK_SAMPLER_REG, multiframeTextureIndex);
         setTexture(Unigeom::BMaterial::cAmbOcc,    AO_SAMPLER_REG, multiframeTextureIndex);
         setTexture(Unigeom::BMaterial::cEmissive,  SELF_SAMPLER_REG, multiframeTextureIndex, pEmissiveTexOverride ? &pEmissiveTexOverride->getD3DTexture() : NULL);
         setTexture(Unigeom::BMaterial::cEmXForm,   EMXFORM_SAMPLER_REG, multiframeTextureIndex);
         setTexture(Unigeom::BMaterial::cHighlight, HIGHLIGHT_SAMPLER_REG, multiframeTextureIndex);
         setTexture(Unigeom::BMaterial::cModulate,  MODULATE_SAMPLER_REG, multiframeTextureIndex);
                  
         // Set up terrain heightfield texture / constant data for conforming
         if (mTerrainConform)
         {
            const BTerrainHeightField::BHeightFieldAttributes& hfAttribs = gTerrainHeightField.getHeightFieldAttributes();
            const float yRange = 10.0f;
            const float yOffset = 0.2f; // was .05
            XMMATRIX heightfieldWorldToNormZT = XMMatrixTranspose(hfAttribs.mWorldToNormZ);
            BVec4 yScaleOfs;
            yScaleOfs.set((float) hfAttribs.mWorldRangeY, (float) hfAttribs.mWorldMinY, yRange, yOffset);

            // Vertex texture and constant data
            gpUGXGeomRSFilter->setTexture(UGX_D3DVERTEXTEXTURESAMPLER0, gTerrainHeightField.getHeightFieldTex());
            gpUGXGeomRSFilter->setVertexShaderConstantF(WORLD_TO_HEIGHTFIELD_REG, reinterpret_cast<float*>(&heightfieldWorldToNormZT), 4);
            gpUGXGeomRSFilter->setVertexShaderConstantF(HEIGHTFIELD_Y_SCALE_OFS_REG, yScaleOfs.getPtr(), 1);

            gpUGXGeomRSFilter->setSamplerState(UGX_D3DVERTEXTEXTURESAMPLER0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
            gpUGXGeomRSFilter->setSamplerState(UGX_D3DVERTEXTEXTURESAMPLER0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
            gpUGXGeomRSFilter->setSamplerState(UGX_D3DVERTEXTEXTURESAMPLER0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
            gpUGXGeomRSFilter->setSamplerState(UGX_D3DVERTEXTEXTURESAMPLER0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
            gpUGXGeomRSFilter->setSamplerState(UGX_D3DVERTEXTEXTURESAMPLER0, D3DSAMP_MIPFILTER, D3DTEXF_NONE);

         }
         else
         {
            gpUGXGeomRSFilter->setTexture(UGX_D3DVERTEXTEXTURESAMPLER0, NULL);
         }

         BOOL mapEnables[LAST_MAP_ENABLE_REG - FIRST_MAP_ENABLE_REG + 1];
         BCOMPILETIMEASSERT(ENABLE_SPEC_MAP_REG == FIRST_MAP_ENABLE_REG);
         BCOMPILETIMEASSERT(ENABLE_MODULATE_MAP_REG == LAST_MAP_ENABLE_REG);

         mapEnables[ENABLE_SPEC_MAP_REG      - FIRST_MAP_ENABLE_REG] = isValidMap(Unigeom::BMaterial::cGloss);
         mapEnables[ENABLE_OPACITY_MAP_REG   - FIRST_MAP_ENABLE_REG] = isValidMap(Unigeom::BMaterial::cOpacity);
         mapEnables[ENABLE_XFORM_MAP_REG     - FIRST_MAP_ENABLE_REG] = isValidMap(Unigeom::BMaterial::cXForm);
         mapEnables[ENABLE_SELF_MAP_REG      - FIRST_MAP_ENABLE_REG] = isValidMap(Unigeom::BMaterial::cEmissive);
         mapEnables[ENABLE_AO_MAP_REG        - FIRST_MAP_ENABLE_REG] = isValidMap(Unigeom::BMaterial::cAmbOcc);
         mapEnables[ENABLE_ENV_MAP_REG       - FIRST_MAP_ENABLE_REG] = isValidMap(Unigeom::BMaterial::cEnv) || (mGlobalEnvMapping && (gGlobalEnvMap != cInvalidManagedTextureHandle));
         mapEnables[ENABLE_ENVMASK_MAP_REG   - FIRST_MAP_ENABLE_REG] = isValidMap(Unigeom::BMaterial::cEnvMask);
         mapEnables[ENABLE_EMXFORM_MAP_REG   - FIRST_MAP_ENABLE_REG] = isValidMap(Unigeom::BMaterial::cEmXForm);
         mapEnables[ENABLE_HIGHLIGHT_MAP_REG - FIRST_MAP_ENABLE_REG] = isValidMap(Unigeom::BMaterial::cHighlight);
         mapEnables[ENABLE_MODULATE_MAP_REG  - FIRST_MAP_ENABLE_REG] = isValidMap(Unigeom::BMaterial::cModulate);
         
         gpUGXGeomRSFilter->setPixelShaderConstantB(FIRST_MAP_ENABLE_REG, mapEnables, LAST_MAP_ENABLE_REG - FIRST_MAP_ENABLE_REG + 1);

         gpUGXGeomRSFilter->setPixelShaderConstantF(SPEC_COLOR_POWER_REG, reinterpret_cast<const float*>(&mSpecColorPower), 1);

         if (visMode != cVMDisabled)
         {
            BVec4 envVisControl(mEnvControl);
            envVisControl[0] = static_cast<float>(visMode - cVMAlbedo);
            gpUGXGeomRSFilter->setPixelShaderConstantF(ENV_VIS_CONTROL_REG, envVisControl.getPtr(), 1);
         }
         else
         {
            gpUGXGeomRSFilter->setPixelShaderConstantF(ENV_VIS_CONTROL_REG, mEnvControl.getPtr(), 1);
         }

         BVec4 HDRTexScale;
         HDRTexScale.set(1.0f);

//-- FIXING PREFIX BUG ID 6622
         const BD3DTextureManager::BManagedTexture* pEmissiveTexture = mpTextures[Unigeom::BMaterial::cEmissive][0];
//--
         if (pEmissiveTexture)
            HDRTexScale[0] = pEmissiveTexture->getHDRScale();
         
         if ((mGlobalEnvMapping) && (gGlobalEnvMap != cInvalidManagedTextureHandle))
         {
            BD3DTextureManager::BManagedTexture* pTexture = gD3DTextureManager.getManagedTextureByHandle(gGlobalEnvMap);

            if (pTexture)
               HDRTexScale[1] = pTexture->getHDRScale();
         }
         else 
         {  
//-- FIXING PREFIX BUG ID 6621
            const BD3DTextureManager::BManagedTexture* pEnvTexture = mpTextures[Unigeom::BMaterial::cEnv][0];
//--
            if (pEnvTexture)
               HDRTexScale[1] = pEnvTexture->getHDRScale();
         }
         
//-- FIXING PREFIX BUG ID 6623
         const BD3DTextureManager::BManagedTexture* pHighlightTexture = mpTextures[Unigeom::BMaterial::cHighlight][0];
//--
         if (pHighlightTexture)
            HDRTexScale[2] = pHighlightTexture->getHDRScale();
         
         HDRTexScale[0] *= pCommonData->mEmissiveIntensity;
         HDRTexScale[2] *= pCommonData->mHighlightIntensity;
                                    
         gpUGXGeomRSFilter->setPixelShaderConstantF(HDR_TEX_SCALE_REG, HDRTexScale.getPtr(), 1);
         
         gpUGXGeomRSFilter->setRenderState(D3DRS_CULLMODE, mTwoSided ? D3DCULL_NONE : gCullMode);
         BOOL twoSided = mTwoSided;
         gpUGXGeomRSFilter->setPixelShaderConstantB(ENABLE_TWO_SIDED_REG, twoSided);

         // Terrain conform render state - depth bias and stencil ref to 0 (so that ui decals will draw over them)
         float depthBias = -.000025f;
         gpUGXGeomRSFilter->setRenderState(D3DRS_DEPTHBIAS, mTerrainConform ? CAST(DWORD, depthBias) : gDepthBias);
         //gpUGXD3DDev->SetRenderState(D3DRS_STENCILREF, mTerrainConform ? 0 : gStencilRef);

         // Never render reflection buffer during reflection pass
         if ((passIndex == cUGXGeomPassMainReflect) || (passIndex == cUGXGeomPassOverallAlphaReflect))
         {
            BOOL localRefl = FALSE;
            gpUGXGeomRSFilter->setPixelShaderConstantB(ENABLE_LOCAL_REFL_REG, localRefl);
         }
         else
         {
            // Only turn on reflection buffer if this instance and section have reflection enabled
            BOOL localRefl = FALSE;
          
            if (pCommonData->mLocalReflection && mLocalReflection)
               localRefl = TRUE;
            #ifndef BUILD_FINAL
               if (gWaterManager.getRenderMode() == BWaterManager::cRenderNone)
                  localRefl = FALSE;
            #endif
           
            gpUGXGeomRSFilter->setPixelShaderConstantB(ENABLE_LOCAL_REFL_REG, localRefl);
         }
         
         //gpUGXD3DDev->SetRenderState(D3DRS_ZWRITEENABLE, ((mBlendType == cBlendAdditive) || mOpacityValid) ? FALSE : gZWriteEnable);
         gpUGXGeomRSFilter->setRenderState(D3DRS_ZWRITEENABLE, mZWrites ? gZWriteEnable : FALSE);

         if ((passIndex == cUGXGeomPassMain) || (passIndex == cUGXGeomPassMainReflect))
         {
            gpUGXGeomRSFilter->setRenderState(D3DRS_ALPHATESTENABLE, mHasOpacityMap); 
            gpUGXGeomRSFilter->setRenderState(D3DRS_HISTENCILWRITEENABLE, mHasOpacityMap ? FALSE : gHiStencilWriteEnable);
                        
            if (mRequiresBlending)
            {
               gpUGXGeomRSFilter->setRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
               gpUGXGeomRSFilter->setRenderState(D3DRS_HIGHPRECISIONBLENDENABLE, TRUE);
               gpUGXGeomRSFilter->setRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
               gpUGXGeomRSFilter->setRenderState(D3DRS_DESTBLEND, (mBlendType == cBlendAdditive) ? D3DBLEND_ONE : D3DBLEND_INVSRCALPHA);
               gpUGXGeomRSFilter->setRenderState(D3DRS_ALPHATOMASKENABLE, FALSE);
            }
            else
            {
               if (gSeperateAlphaBlendEnabled)
               {
                  gpUGXGeomRSFilter->setRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
                  gpUGXGeomRSFilter->setRenderState(D3DRS_HIGHPRECISIONBLENDENABLE, TRUE);
               }
               else
               {
                  gpUGXGeomRSFilter->setRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
                  gpUGXGeomRSFilter->setRenderState(D3DRS_HIGHPRECISIONBLENDENABLE, FALSE);
               }  
               gpUGXGeomRSFilter->setRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
               gpUGXGeomRSFilter->setRenderState(D3DRS_DESTBLEND, D3DBLEND_ZERO);          
                              
               if (mBlendType == cBlendAlphaToCoverage)
                  gpUGXGeomRSFilter->setRenderState(D3DRS_ALPHATOMASKENABLE, mHasOpacityMap || mOpacityValid || (pCommonData->mTintColor < 255));
               else if (mBlendType == cBlendAlphaTest)
                  gpUGXGeomRSFilter->setRenderState(D3DRS_ALPHATOMASKENABLE, FALSE);
            }
         }
         else
         {
            // Overall alpha
            // renderBegin() has already enabled alpha blending, testing, etc.
            
            gpUGXGeomRSFilter->setRenderState(D3DRS_DESTBLEND, (mBlendType == cBlendAdditive) ? D3DBLEND_ONE : D3DBLEND_INVSRCALPHA);
         }
         
         if (!pUberVisualRenderAttributes)
         {
            gpUGXGeomRSFilter->setPixelShaderConstantB(ENABLE_ADD_TEX_REG, FALSE);         
            gpUGXGeomRSFilter->setPixelShaderConstantB(ENABLE_LERP_TEX_REG, FALSE);
            gpUGXGeomRSFilter->setTexture(ADD_TEX_SAMPLER_REG, NULL);
            gpUGXGeomRSFilter->setTexture(LERP_TEX_SAMPLER_REG, NULL);
         }
         else
         {
            if (pUberVisualRenderAttributes->mAddTexture != cInvalidManagedTextureHandle)
            {
               gpUGXGeomRSFilter->setPixelShaderConstantB(ENABLE_ADD_TEX_REG, TRUE);
               BD3DTextureManager::BManagedTexture* pTex = gD3DTextureManager.getManagedTextureByHandle(pUberVisualRenderAttributes->mAddTexture);
               if (pTex)
                  gpUGXGeomRSFilter->setTexture(ADD_TEX_SAMPLER_REG, pTex->getD3DTexture().getTexture());
               gpUGXGeomRSFilter->setPixelShaderConstantF(ADD_TEX_PARAMS_REG, (float*)&pUberVisualRenderAttributes->mAddTexR, 3);
               
               DWORD wrapType;
               if (pUberVisualRenderAttributes->mAddTexClamp)
                  wrapType = D3DTADDRESS_CLAMP;
               else
                  wrapType = D3DTADDRESS_WRAP;

               gpUGXGeomRSFilter->setSamplerState(ADD_TEX_SAMPLER_REG, D3DSAMP_ADDRESSU, wrapType);
               gpUGXGeomRSFilter->setSamplerState(ADD_TEX_SAMPLER_REG, D3DSAMP_ADDRESSV, wrapType);
            }
            else
            {
               gpUGXGeomRSFilter->setPixelShaderConstantB(ENABLE_ADD_TEX_REG, FALSE);         
               gpUGXGeomRSFilter->setTexture(ADD_TEX_SAMPLER_REG, NULL);
            }
            
            if (pUberVisualRenderAttributes->mLerpTexture != cInvalidManagedTextureHandle)
            {
               gpUGXGeomRSFilter->setPixelShaderConstantB(ENABLE_LERP_TEX_REG, TRUE);
               BD3DTextureManager::BManagedTexture* pTex = gD3DTextureManager.getManagedTextureByHandle(pUberVisualRenderAttributes->mLerpTexture);
               if (pTex)
                  gpUGXGeomRSFilter->setTexture(LERP_TEX_SAMPLER_REG, pTex->getD3DTexture().getTexture());
                  
               gpUGXGeomRSFilter->setPixelShaderConstantF(LERP_TEX_PARAMS_REG, (float*)&pUberVisualRenderAttributes->mLerpTexR, 3);
               
               DWORD wrapType;
               if (pUberVisualRenderAttributes->mLerpTexClamp)
                  wrapType = D3DTADDRESS_CLAMP;
               else
                  wrapType = D3DTADDRESS_WRAP;

               gpUGXGeomRSFilter->setSamplerState(LERP_TEX_SAMPLER_REG, D3DSAMP_ADDRESSU, wrapType);
               gpUGXGeomRSFilter->setSamplerState(LERP_TEX_SAMPLER_REG, D3DSAMP_ADDRESSV, wrapType);
            }
            else
            {
               gpUGXGeomRSFilter->setPixelShaderConstantB(ENABLE_LERP_TEX_REG, FALSE);
               gpUGXGeomRSFilter->setTexture(LERP_TEX_SAMPLER_REG, NULL);
            }
         }
         
         //const uint prevEffectManagerLoadCount = mEffectManagerLoadCount;
         if ((mOptimizedPassIndices[0] == -1) || (mEffectManagerLoadCount != gUGXGeomUberEffectManager.getLoadCount()))
            findPass();
         
         int optimizedPixelShaderPassIndex = -1;
         if ((passIndex == cUGXGeomPassMain) || (passIndex == cUGXGeomPassMainReflect))
            optimizedPixelShaderPassIndex = mOptimizedPassIndices[gEnableLocalPixelLights ? cLocalLightingOptimizedPassIndex : cNormalOptimizedPassIndex];

#ifndef BUILD_FINAL         
         if (visMode != cVMDisabled)
            techniqueIndex = cUberEffectTechniqueVis;
         else
#endif                        
         if (mSkyMaterial)
            techniqueIndex = cUberEffectTechniqueSky;
         else
         {
            techniqueIndex = cUberEffectTechniqueVisible;
            if (dirShadowing)
               pixelShaderTechniquePassIndex = (optimizedPixelShaderPassIndex >= 0) ? optimizedPixelShaderPassIndex : cTPGenericPixelShader;
            else
               pixelShaderTechniquePassIndex = cTPGenericNoShadowPixelShader;
         }
                  
         break;
      }
      case cUGXGeomPassDistortion:
      {
         XMVECTOR tintColor = XMLoadColor(reinterpret_cast<const XMCOLOR*>(&pCommonData->mTintColor)) * mOpacity;
         gpUGXGeomRSFilter->setPixelShaderConstantF(TINT_COLOR_REG, reinterpret_cast<const float*>(&tintColor), 1);      
         
         setUVConstants();
         
         setTexture(Unigeom::BMaterial::cDistortion, DIFFUSE_SAMPLER_REG, multiframeTextureIndex);
         
         gpUGXGeomRSFilter->setRenderState(D3DRS_CULLMODE, mTwoSided ? D3DCULL_NONE : gCullMode);
         BOOL twoSided = mTwoSided;
         gpUGXGeomRSFilter->setPixelShaderConstantB(ENABLE_TWO_SIDED_REG, twoSided);

         // Terrain conform render state - depth bias and stencil ref to 0 (so that ui decals will draw over them)
         float depthBias = -.000025f;
         gpUGXGeomRSFilter->setRenderState(D3DRS_DEPTHBIAS, mTerrainConform ? CAST(DWORD, depthBias) : gDepthBias);
         
         techniqueIndex = cUberEffectTechniqueDistortion;
         break;
      }
      case cUGXGeomPassShadowGen:
      case cUGXGeomPassDPShadowGen: 
      {
         const BOOL overallAlpha = (pCommonData->mTintColor < 0xFF000000) ? TRUE : FALSE;
                              
         if (mHasOpacityMap)
         {
            setUVConstants();
            
            setTexture(Unigeom::BMaterial::cOpacity, OPACITY_SAMPLER_REG, multiframeTextureIndex);
         }

         gpUGXGeomRSFilter->setRenderState(D3DRS_ALPHATOMASKENABLE, overallAlpha);
         
         if ((overallAlpha) || (mHasOpacityMap) || (mOpacityValid))
         {
            XMVECTOR tintColor = XMLoadColor(reinterpret_cast<const XMCOLOR*>(&pCommonData->mTintColor)) * mOpacity;
            gpUGXGeomRSFilter->setPixelShaderConstantF(TINT_COLOR_REG, reinterpret_cast<const float*>(&tintColor), 1);      
                                    
            if (passIndex == cUGXGeomPassDPShadowGen)
            {
               techniqueIndex = mHasOpacityMap ? cUberEffectTechniqueDPShadowGenAlphaTest : cUberEffectTechniqueDPShadowGenOverallAlpha;
               
               // alpha testing is always on with DP shadow gen
            }
            else
            {
               techniqueIndex = mHasOpacityMap ? cUberEffectTechniqueShadowGenAlphaTest : cUberEffectTechniqueShadowGenOverallAlpha;
               
               gpUGXGeomRSFilter->setRenderState(D3DRS_ALPHATESTENABLE, (overallAlpha || mOpacityValid) ? FALSE : mHasOpacityMap);
            }
         }
         else
         {
            if (passIndex != cUGXGeomPassDPShadowGen)
               gpUGXGeomRSFilter->setRenderState(D3DRS_ALPHATESTENABLE, FALSE);
            
            techniqueIndex = (passIndex == cUGXGeomPassDPShadowGen) ? cUberEffectTechniqueDPShadowGen : cUberEffectTechniqueShadowGen;
         }
                          
         break;
      }
   }
   
   const int numTechniquePasses = (int)effect.getTechniqueFromIndex(techniqueIndex).getNumPasses();
   numTechniquePasses;
   BDEBUG_ASSERT(techniquePassIndex < numTechniquePasses);
   BDEBUG_ASSERT(pixelShaderTechniquePassIndex < numTechniquePasses);

   BFXLEffectTechnique technique(effect.getTechniqueFromIndex(techniqueIndex));
   BFXLEffectPass pass(technique.getPassFromIndex(techniquePassIndex));
   
   // If we share vertex shaders, we can't bind them to individual pixel shaders.
   bool bindVertexToPixelShaders = true;
   if (pixelShaderTechniquePassIndex >= 0)
      bindVertexToPixelShaders = false;
   BBoundEffectPassData* pBoundEffectPassData = gBoundEffectManager.get(effect, pass, mpDecl, mStream0Size, mStream1Size, bindVertexToPixelShaders);
   pBoundEffectPassData->setToDevice(*gpUGXGeomRSFilter, gUGXGeomUberEffectManager.getIntrinsicPool()->getTable());
   
   if (pixelShaderTechniquePassIndex >= 0)
   {
      BFXLEffectPass psPass(technique.getPassFromIndex(pixelShaderTechniquePassIndex));

      BBoundEffectPassData* pPSBoundEffectPassData = gBoundEffectManager.get(effect, psPass, NULL, 0, 0, false);
      pPSBoundEffectPassData->setToDevice(*gpUGXGeomRSFilter, gUGXGeomUberEffectManager.getIntrinsicPool()->getTable());
      
#ifndef BUILD_FINAL               
      if (pixelShaderTechniquePassIndex >= cTPFirstOptimized)
         gTotalOptimizedShadersUsed++;
#endif               
   }

#ifndef BUILD_FINAL               
   gTotalShadersSet++;
#endif               

   return true;
}

//============================================================================
// BUGXGeomUberSectionRenderer::endPass
//============================================================================
void BUGXGeomUberSectionRenderer::endPass(const BUGXGeomRenderCommonInstanceData* pCommonData, void* pState, uint stateMaxSize)
{
#if 0
   BDEBUG_ASSERT(stateMaxSize >= sizeof(BPassState));
   BPassState& state = *reinterpret_cast<BPassState*>(pState);

   state.mTechnique.endPass();

   state.mTechnique.end();      
#endif   
}

//============================================================================
// BUGXGeomUberSectionRenderer::getTextures
//============================================================================
void BUGXGeomUberSectionRenderer::getTextures(BD3DTextureManager::BManagedTexture* const*& pTextureList, uint& numTextures, uint multiframeTextureIndex) const
{
   // Not thread safe!!
   static BD3DTextureManager::BManagedTexture* textures[Unigeom::BMaterial::cNumMapTypes];
   
   if (!mLoadedTextures)
   {
      const_cast<BUGXGeomUberSectionRenderer*>(this)->loadTextures();
   }
         
   for (uint i = 0; i < Unigeom::BMaterial::cNumMapTypes; i++)
   {
      if (multiframeTextureIndex >= mNumTextureFrames[i])
         textures[i] = NULL;
      else
         textures[i] = mpTextures[i][multiframeTextureIndex];
   }
   
   pTextureList = textures;
   numTextures = Unigeom::BMaterial::cNumMapTypes;
}

//============================================================================
// BUGXGeomUberSectionRenderer::processTextureStatusChanged
//============================================================================
bool BUGXGeomUberSectionRenderer::processTextureStatusChanged(const BEvent& event)
{
   return true;
}

//============================================================================
// BUGXGeomUberSectionRenderer::receiveEvent
//============================================================================
bool BUGXGeomUberSectionRenderer::receiveEvent(const BEvent& event, BThreadIndex threadIndex)
{
   switch (event.mEventClass)
   {
      case cRenderEventClassTextureStatusChanged:
      {
         processTextureStatusChanged(event);
         break;
      }
   }

   return false;
}

//============================================================================
// BUGXGeomUberSectionRendererArray::BUGXGeomUberSectionRendererArray
//============================================================================
BUGXGeomUberSectionRendererArray::BUGXGeomUberSectionRendererArray() :
   mNumSections(0),
   mpSections(NULL)
{

}

//============================================================================
// BUGXGeomUberSectionRendererArray::~BUGXGeomUberSectionRendererArray
//============================================================================
BUGXGeomUberSectionRendererArray::~BUGXGeomUberSectionRendererArray()
{
   deinit();
}

//============================================================================
// BUGXGeomUberSectionRendererArray::init
//============================================================================
bool BUGXGeomUberSectionRendererArray::init(BUGXGeomData* pGeomData)
{
   deinit();
      
   mNumSections = pGeomData->getCachedData()->numSections();
   mpSections = ALIGNED_NEW_ARRAY(BUGXGeomUberSectionRenderer, mNumSections, gRenderHeap);

   BUGXGeomData::BUnigeomMaterialArray materials;
   if (!pGeomData->getMaterials(materials))
      return false;
      
   for (uint sectionIndex = 0; sectionIndex < mNumSections; sectionIndex++)
   {
      const BUGXGeom::BNativeCachedDataType::SectionType& section = pGeomData->getCachedData()->section(sectionIndex);
      
      const uint materialIndex = section.materialIndex();
      if (materialIndex >= materials.getSize())
      {
         gConsoleOutput.error("BUGXGeomUberSectionRendererArray::init: Invalid section material index!\n");
         
         mNumSections = 0;
         ALIGNED_DELETE_ARRAY(mpSections, gRenderHeap);
         mpSections = NULL;
         return false;
      }
            
      if (!mpSections[sectionIndex].init(pGeomData->getVertexDecl(sectionIndex), materials[materialIndex], section.baseVertPacker()))
      {
         mNumSections = 0;
         ALIGNED_DELETE_ARRAY(mpSections, gRenderHeap);
         mpSections = NULL;
         return false;
      }
   }
   
   return true;
}

//============================================================================
// BUGXGeomUberSectionRendererArray::deinit
//============================================================================
void BUGXGeomUberSectionRendererArray::deinit(void)
{
   mNumSections = 0;
   ALIGNED_DELETE_ARRAY(mpSections, gRenderHeap);
   mpSections = NULL;
}

//============================================================================
// BUGXGeomUberSectionRendererManager::BUGXGeomUberSectionRendererManager
//============================================================================
BUGXGeomUberSectionRendererManager::BUGXGeomUberSectionRendererManager()
{
}

//============================================================================
// BUGXGeomUberSectionRendererManager::~BUGXGeomUberSectionRendererManager
//============================================================================
BUGXGeomUberSectionRendererManager::~BUGXGeomUberSectionRendererManager()
{
}

//============================================================================
// BUGXGeomUberSectionRendererManager::init
//============================================================================
IUGXGeomSectionRendererArray* BUGXGeomUberSectionRendererManager::initSectionArray(BUGXGeomData* pGeomData)
{
   BUGXGeomUberSectionRendererArray* pSectionArray = ALIGNED_NEW(BUGXGeomUberSectionRendererArray, gRenderHeap);

   if (!pSectionArray->init(pGeomData))
   {
      ALIGNED_DELETE(pSectionArray, gRenderHeap);
      return NULL;
   }

   return pSectionArray;
}

//============================================================================
// BUGXGeomUberSectionRendererManager::deinit
//============================================================================
void BUGXGeomUberSectionRendererManager::deinitSectionArray(IUGXGeomSectionRendererArray* pSectionArray)
{
   ALIGNED_DELETE(pSectionArray, gRenderHeap);
}

//============================================================================
// BUGXGeomUberSectionRendererManager::globalRenderBegin
//============================================================================
void BUGXGeomUberSectionRendererManager::globalRenderBegin(
   double gameTime, 
   eUGXGeomPass pass, 
   eUGXGeomVisMode visMode, 
   BManagedTextureHandle globalEnvMap, 
   eUGXGeomTextureMode textureMode, 
   const BBlackmapParams& blackmapParams)
{
   for (uint i = 0; i < cMaxD3DTextureSamplers; i++)
   {
      gpUGXGeomRSFilter->setTexture(i, NULL);
      gpUGXGeomRSFilter->setSamplerState(i, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
      gpUGXGeomRSFilter->setSamplerState(i, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
      gpUGXGeomRSFilter->setSamplerState(i, D3DSAMP_ADDRESSW, D3DTADDRESS_CLAMP);
      gpUGXGeomRSFilter->setSamplerState(i, D3DSAMP_SEPARATEZFILTERENABLE, FALSE);

      gpUGXGeomRSFilter->setSamplerState(i, D3DSAMP_MINFILTER, D3DTEXF_ANISOTROPIC);
      gpUGXGeomRSFilter->setSamplerState(i, D3DSAMP_MAGFILTER, D3DTEXF_ANISOTROPIC);
      gpUGXGeomRSFilter->setSamplerState(i, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);
      gpUGXGeomRSFilter->setSamplerState(i, D3DSAMP_MAXANISOTROPY, 3);
   }

   gpUGXGeomRSFilter->setRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
   gpUGXGeomRSFilter->setRenderState(D3DRS_HIGHPRECISIONBLENDENABLE, FALSE);
   gpUGXGeomRSFilter->setRenderState(D3DRS_ALPHATESTENABLE, FALSE);
   gpUGXGeomRSFilter->setRenderState(D3DRS_ALPHATOMASKENABLE, FALSE);
   gpUGXGeomRSFilter->setRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATEREQUAL);
   
   gpUGXGeomRSFilter->setRenderState(D3DRS_ALPHATOMASKOFFSETS, D3DALPHATOMASK_DITHERED);

   gpUGXGeomRSFilter->setRenderState(D3DRS_ALPHAREF, (pass == cUGXGeomPassShadowGen) ? 128 : 1);
      
   gpUGXGeomRSFilter->getRenderState(D3DRS_ZWRITEENABLE, &gZWriteEnable);
   gpUGXGeomRSFilter->getRenderState(D3DRS_CULLMODE, &gCullMode);
   gpUGXGeomRSFilter->getRenderState(D3DRS_DEPTHBIAS, &gDepthBias);
   gpUGXGeomRSFilter->getRenderState(D3DRS_STENCILREF, &gStencilRef);
   
   gpUGXGeomRSFilter->getRenderState(D3DRS_SEPARATEALPHABLENDENABLE, &gSeperateAlphaBlendEnabled);
   
   gpUGXGeomRSFilter->getRenderState(D3DRS_HISTENCILWRITEENABLE, &gHiStencilWriteEnable);
   
   gCurTime = gameTime;
   
   gGlobalEnvMap = globalEnvMap;
   
   gTextureMode = textureMode;
   
   if (cUGXGeomPassDistortion == pass)
   {
      gpUGXGeomRSFilter->setRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
      gpUGXGeomRSFilter->setRenderState(D3DRS_HIGHPRECISIONBLENDENABLE, TRUE);
      gpUGXGeomRSFilter->setRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
      gpUGXGeomRSFilter->setRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
      gpUGXGeomRSFilter->setRenderState(D3DRS_ALPHATESTENABLE, TRUE);

      gpUGXGeomRSFilter->setRenderState(D3DRS_ZWRITEENABLE, FALSE);
   }
      
   if ((!blackmapParams.mpTexture) || (pass >= cUGXGeomPassShadowGen))
   {
      gBlackmapEnabled = FALSE;
      //gpUGXGeomRSFilter->setPixelShaderConstantB(ENABLE_BLACKMAP_REG, &gBlackmapEnabled, 1);
      gpUGXGeomRSFilter->setTexture(BLACKMAP_SAMPLER_REG, NULL);
   }
   else
   {
      gBlackmapEnabled = TRUE;
            
      gpUGXGeomRSFilter->setTexture(BLACKMAP_SAMPLER_REG, blackmapParams.mpTexture);
      gpUGXGeomRSFilter->setSamplerState(BLACKMAP_SAMPLER_REG, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
      gpUGXGeomRSFilter->setSamplerState(BLACKMAP_SAMPLER_REG, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
      gpUGXGeomRSFilter->setSamplerState(BLACKMAP_SAMPLER_REG, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
      gpUGXGeomRSFilter->setSamplerState(BLACKMAP_SAMPLER_REG, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
      gpUGXGeomRSFilter->setSamplerState(BLACKMAP_SAMPLER_REG, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
            
      gpUGXGeomRSFilter->setPixelShaderConstantF(BLACKMAP_PARAMS0_REG, blackmapParams.mParams[0].getPtr(), 1);
      gpUGXGeomRSFilter->setPixelShaderConstantF(BLACKMAP_PARAMS1_REG, blackmapParams.mParams[1].getPtr(), 1);
      gpUGXGeomRSFilter->setPixelShaderConstantF(BLACKMAP_PARAMS2_REG, blackmapParams.mParams[2].getPtr(), 1);
   }
   
   BOOL lightBuffering = (gVisibleLightManager.getLightBuffer(BVisibleLightManager::cLightBufferColorTexture) != NULL);
   gpUGXGeomRSFilter->setPixelShaderConstantB(ENABLE_LIGHT_BUFFERING_REG, lightBuffering);
   if (lightBuffering)
   {
      //BDEBUG_ASSERT(gVisibleLightManager.getLightBufferIntensityScale() == 12.0f);
      
      gpUGXGeomRSFilter->setTexture(LIGHT_BUFFER_COLOR_SAMPLER_REG, gVisibleLightManager.getLightBuffer(BVisibleLightManager::cLightBufferColorTexture));
      gpUGXGeomRSFilter->setSamplerState(LIGHT_BUFFER_COLOR_SAMPLER_REG, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
      gpUGXGeomRSFilter->setSamplerState(LIGHT_BUFFER_COLOR_SAMPLER_REG, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
      gpUGXGeomRSFilter->setSamplerState(LIGHT_BUFFER_COLOR_SAMPLER_REG, D3DSAMP_ADDRESSW, D3DTADDRESS_CLAMP);
      gpUGXGeomRSFilter->setSamplerState(LIGHT_BUFFER_COLOR_SAMPLER_REG, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
      gpUGXGeomRSFilter->setSamplerState(LIGHT_BUFFER_COLOR_SAMPLER_REG, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
      gpUGXGeomRSFilter->setSamplerState(LIGHT_BUFFER_COLOR_SAMPLER_REG, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
      gpUGXGeomRSFilter->setSamplerState(LIGHT_BUFFER_COLOR_SAMPLER_REG, D3DSAMP_SEPARATEZFILTERENABLE, FALSE);
      
      gpUGXGeomRSFilter->setTexture(LIGHT_BUFFER_VECTOR_SAMPLER_REG, gVisibleLightManager.getLightBuffer(BVisibleLightManager::cLightBufferVectorTexture));
      gpUGXGeomRSFilter->setSamplerState(LIGHT_BUFFER_VECTOR_SAMPLER_REG, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
      gpUGXGeomRSFilter->setSamplerState(LIGHT_BUFFER_VECTOR_SAMPLER_REG, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
      gpUGXGeomRSFilter->setSamplerState(LIGHT_BUFFER_VECTOR_SAMPLER_REG, D3DSAMP_ADDRESSW, D3DTADDRESS_CLAMP);
      gpUGXGeomRSFilter->setSamplerState(LIGHT_BUFFER_VECTOR_SAMPLER_REG, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
      gpUGXGeomRSFilter->setSamplerState(LIGHT_BUFFER_VECTOR_SAMPLER_REG, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
      gpUGXGeomRSFilter->setSamplerState(LIGHT_BUFFER_VECTOR_SAMPLER_REG, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
      gpUGXGeomRSFilter->setSamplerState(LIGHT_BUFFER_VECTOR_SAMPLER_REG, D3DSAMP_SEPARATEZFILTERENABLE, FALSE);
      
      const XMMATRIX& worldToLightBuffer = gVisibleLightManager.getWorldToLightBuffer();
      
      BVec4 cols[3];
      cols[0].set(worldToLightBuffer.m[0][0], worldToLightBuffer.m[1][0], worldToLightBuffer.m[2][0], worldToLightBuffer.m[3][0]);
      cols[1].set(worldToLightBuffer.m[0][1], worldToLightBuffer.m[1][1], worldToLightBuffer.m[2][1], worldToLightBuffer.m[3][1]);
      cols[2].set(worldToLightBuffer.m[0][2], worldToLightBuffer.m[1][2], worldToLightBuffer.m[2][2], worldToLightBuffer.m[3][2]);
      
      gpUGXGeomRSFilter->setPixelShaderConstantF(WORLD_TO_LIGHTBUF0_REG, cols[0].getPtr(), 3);
   }
}

//============================================================================
// BUGXGeomUberSectionRendererManager::renderBegin
//============================================================================
void BUGXGeomUberSectionRendererManager::renderBegin(const BUGXGeomRenderCommonInstanceData* pCommonData)
{
   renderSetLocalLightState(pCommonData);
   
   switch (pCommonData->mPass)
   {
      case cUGXGeomPassMain:
      case cUGXGeomPassMainReflect:
      {
         if (gSeperateAlphaBlendEnabled)
         {
            gpUGXGeomRSFilter->setRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
            gpUGXGeomRSFilter->setRenderState(D3DRS_HIGHPRECISIONBLENDENABLE, TRUE);
            gpUGXGeomRSFilter->setRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
            gpUGXGeomRSFilter->setRenderState(D3DRS_DESTBLEND, D3DBLEND_ZERO);
         }
         else
         {
            gpUGXGeomRSFilter->setRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
            gpUGXGeomRSFilter->setRenderState(D3DRS_HIGHPRECISIONBLENDENABLE, FALSE);
         }            
         gpUGXGeomRSFilter->setRenderState(D3DRS_ALPHATESTENABLE, FALSE);
         gpUGXGeomRSFilter->setRenderState(D3DRS_HISTENCILWRITEENABLE, gHiStencilWriteEnable);
                  
         break;
      }
      case cUGXGeomPassOverallAlpha:
      case cUGXGeomPassOverallAlphaReflect:
      {
         gpUGXGeomRSFilter->setRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
         gpUGXGeomRSFilter->setRenderState(D3DRS_HIGHPRECISIONBLENDENABLE, TRUE);
         gpUGXGeomRSFilter->setRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
         gpUGXGeomRSFilter->setRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
         gpUGXGeomRSFilter->setRenderState(D3DRS_ALPHATESTENABLE, TRUE);
         gpUGXGeomRSFilter->setRenderState(D3DRS_HISTENCILWRITEENABLE, FALSE);
                           
         break;
      }
   }
   
   gpUGXGeomRSFilter->setRenderState(D3DRS_ALPHATOMASKENABLE, FALSE);
            
   switch (pCommonData->mPass)
   {
      case cUGXGeomPassMain:
      case cUGXGeomPassMainReflect:
      case cUGXGeomPassOverallAlpha:
      case cUGXGeomPassOverallAlphaReflect:
      {
         gpUGXD3DDev->GpuOwnPixelShaderConstantF(0, NUM_LOCAL_LIGHT_PSHADER_CONSTANTS);
         gpUGXD3DDev->GpuOwnVertexShaderConstantF(0, NUM_LOCAL_LIGHT_PSHADER_CONSTANTS);

         break;
      }
      case cUGXGeomPassShadowGen:
      {
         break;
      }
      case cUGXGeomPassDPShadowGen:
      {
         gpUGXGeomRSFilter->setRenderState(D3DRS_ALPHATESTENABLE, TRUE);
         break;
      }
   }
}

//============================================================================
// BUGXGeomUberSectionRendererManager::renderSetLocalLightState
//============================================================================
void BUGXGeomUberSectionRendererManager::renderSetLocalLightState(const BUGXGeomRenderCommonInstanceData* pCommonData)
{
   switch (pCommonData->mPass)
   {
      case cUGXGeomPassMain:
      case cUGXGeomPassMainReflect:
      case cUGXGeomPassOverallAlpha:
      case cUGXGeomPassOverallAlphaReflect:
      {
         gEnableLocalPixelLights = (pCommonData->mNumPixelLights > 0);
         
         gpUGXGeomRSFilter->setPixelShaderConstantB(ENABLE_LOCAL_PIXEL_LIGHTS_REG, gEnableLocalPixelLights);

         if (gEnableLocalPixelLights)
         {
            const int numPixelLights[4] = { pCommonData->mNumPixelLights, 0, 1, 0 };
            gpUGXGeomRSFilter->setPixelShaderConstantI(NUM_LOCAL_PIXEL_LIGHTS_REG, numPixelLights, 1);

            gEnableLocalLightShadows = pCommonData->mLocalLightShadows;
            gpUGXGeomRSFilter->setPixelShaderConstantB(ENABLE_LOCAL_SHADOWING_REG, gEnableLocalLightShadows);   
         }
                  
         break;   
      }
   }   
}

//============================================================================
// BUGXGeomUberSectionRendererManager::renderEnd
//============================================================================
void BUGXGeomUberSectionRendererManager::renderEnd(const BUGXGeomRenderCommonInstanceData* pCommonData)
{
   switch (pCommonData->mPass)
   {
      case cUGXGeomPassMain:
      case cUGXGeomPassMainReflect:
      case cUGXGeomPassOverallAlpha:
      case cUGXGeomPassOverallAlphaReflect:
      {
         gpUGXD3DDev->GpuDisownPixelShaderConstantF(0, NUM_LOCAL_LIGHT_PSHADER_CONSTANTS);
         gpUGXD3DDev->GpuDisownVertexShaderConstantF(0, NUM_LOCAL_LIGHT_PSHADER_CONSTANTS);         
         break;
      }
   }
}

//============================================================================
// BUGXGeomUberSectionRendererManager::globalRenderEnd
//============================================================================
void BUGXGeomUberSectionRendererManager::globalRenderEnd(eUGXGeomPass pass)
{
   for (uint i = 0; i < cMaxD3DTextureSamplers; i++)
   {
      gpUGXGeomRSFilter->setTexture(i, NULL);
      gpUGXGeomRSFilter->setSamplerState(i, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
      gpUGXGeomRSFilter->setSamplerState(i, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);
      gpUGXGeomRSFilter->setSamplerState(i, D3DSAMP_ADDRESSW, D3DTADDRESS_WRAP);

      gpUGXGeomRSFilter->setSamplerState(i, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
      gpUGXGeomRSFilter->setSamplerState(i, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
      gpUGXGeomRSFilter->setSamplerState(i, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);
      gpUGXGeomRSFilter->setSamplerState(i, D3DSAMP_MAXANISOTROPY, 1);
   }

   gpUGXGeomRSFilter->setRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
   gpUGXGeomRSFilter->setRenderState(D3DRS_HIGHPRECISIONBLENDENABLE, FALSE);
   gpUGXGeomRSFilter->setRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
   gpUGXGeomRSFilter->setRenderState(D3DRS_DESTBLEND, D3DBLEND_ZERO);
   gpUGXGeomRSFilter->setRenderState(D3DRS_ALPHATESTENABLE, FALSE);
   gpUGXGeomRSFilter->setRenderState(D3DRS_ALPHATOMASKENABLE, FALSE);
   gpUGXGeomRSFilter->setRenderState(D3DRS_ALPHAREF, 0);
   gpUGXGeomRSFilter->setRenderState(D3DRS_ALPHAFUNC, D3DCMP_ALWAYS);
   
   gpUGXGeomRSFilter->setRenderState(D3DRS_ZWRITEENABLE, gZWriteEnable);
   gpUGXGeomRSFilter->setRenderState(D3DRS_CULLMODE, gCullMode);
   gpUGXGeomRSFilter->setRenderState(D3DRS_DEPTHBIAS, gDepthBias);
   gpUGXGeomRSFilter->setRenderState(D3DRS_STENCILREF, gStencilRef);
   gpUGXGeomRSFilter->setRenderState(D3DRS_HISTENCILWRITEENABLE, gHiStencilWriteEnable);
}

//============================================================================
// BUGXGeomUberSectionRendererManager::displayStats
//============================================================================
void BUGXGeomUberSectionRendererManager::displayStats(BDebugTextDisplay& textDisplay, uint page)
{
#ifndef BUILD_FINAL
   BDEBUG_ASSERT(page < cMaxDisplayPages);
   
   BScopedCriticalSection lock(gOptimizedShaderParamStatLock);
   
   textDisplay.printf("UGX Renderer Total Shaders Set: %u, Total Optimized Shaders: %u", gTotalShadersSet, gTotalOptimizedShadersUsed);
      
   BUInt64HashMap& hashMap = page ? gOptimizedShaderHistogram : gUnoptimizedShaderHistogram;
   
   textDisplay.printf("Total %s shaders: %u", page ? "optimized" : "unoptimized", hashMap.getSize());
   for (BUInt64HashMap::const_iterator it = hashMap.begin(); it != hashMap.end(); ++it)
   {
      uint64 paramValueFlags = it->first;
      uint count = it->second;
      
      BFixedString<512> buf;
      buf.format(" %u: ", count);
      
      for (uint i = 0; i < gOptimizedShaderParamNames.getSize(); i++)
      {
         BFixedString256 name(gOptimizedShaderParamNames[i].getPtr());
         int j = name.find("Flag");
         if (j >= 0)
            name.truncate(j);
         buf.formatAppend("%s:%u ", name.getPtr(), (paramValueFlags & ((uint64)1U) << (uint64)i) != 0 );
         if (buf.getLen() >= 120)
         {
            textDisplay.print(cColorWhite, buf);      
            buf.set("  ");
         }
      }
      buf.formatAppend("%s:%u ", "tngt", (paramValueFlags & ((uint64)1U) << (uint64)gOptimizedShaderParamNames.getSize()) != 0 );
      
      if (buf.getLen() > 2)
         textDisplay.print(cColorWhite, buf);      
   }
                  
   gTotalShadersSet = 0;
   gTotalOptimizedShadersUsed = 0;
#endif   
}

//============================================================================
// BUGXGeomUberSectionRendererManager::dumpShaderMacros
//============================================================================
void BUGXGeomUberSectionRendererManager::dumpShaderMacros(bool optimizedShaders)
{
#ifndef BUILD_FINAL
   BScopedCriticalSection lock(gOptimizedShaderParamStatLock);

   BUInt64HashMap& hashMap = optimizedShaders ? gOptimizedShaderHistogram : gUnoptimizedShaderHistogram;
   BFixedString<512> buf;

   buf.format("#ifdef BUMP");
   gConsoleOutput.output(cMsgConsole, "%s\n", buf.getPtr());   

   // Do a pass for tangent case
   for (BUInt64HashMap::const_iterator it = hashMap.begin(); it != hashMap.end(); ++it)
   {
      uint64 paramValueFlags = it->first;
      uint count = it->second;
      bool hasTangent = (paramValueFlags & ((uint64)1U) << (uint64)gOptimizedShaderParamNames.getSize()) ? true : false;
      if (!hasTangent)
         continue;
      
      buf.format(" DEFINE_PASSES(");

      for (uint i = 0; i < gOptimizedShaderParamNames.getSize(); i++)
      {
         BFixedString256 name(gOptimizedShaderParamNames[i].getPtr());
         buf.formatAppend("%s%c ", (paramValueFlags & ((uint64)1U) << (uint64)i) ? "true" : "false", i < (gOptimizedShaderParamNames.getSize() - 1) ? ',' : ' ');
      }

      buf.formatAppend(") // %u", count);

      gConsoleOutput.output(cMsgConsole, "%s\n", buf.getPtr());   
   }

   buf.format("#else // ndef BUMP - Non-tangent");
   gConsoleOutput.output(cMsgConsole, "%s\n", buf.getPtr());   

   // Do a pass for tangent case
   for (BUInt64HashMap::const_iterator it = hashMap.begin(); it != hashMap.end(); ++it)
   {
      uint64 paramValueFlags = it->first;
      uint count = it->second;
      bool hasTangent = (paramValueFlags & ((uint64)1U) << (uint64)gOptimizedShaderParamNames.getSize()) ? true : false;
      if (hasTangent)
         continue;
      
      buf.format(" DEFINE_PASSES(");

      for (uint i = 0; i < gOptimizedShaderParamNames.getSize(); i++)
      {
         BFixedString256 name(gOptimizedShaderParamNames[i].getPtr());
         buf.formatAppend("%s%c ", (paramValueFlags & ((uint64)1U) << (uint64)i) ? "true" : "false", i < (gOptimizedShaderParamNames.getSize() - 1) ? ',' : ' ');
      }

      buf.formatAppend(") // %u", count);

      gConsoleOutput.output(cMsgConsole, "%s\n", buf.getPtr());   
   }

   buf.format("#endif");
   gConsoleOutput.output(cMsgConsole, "%s\n", buf.getPtr());   
#endif   
}
