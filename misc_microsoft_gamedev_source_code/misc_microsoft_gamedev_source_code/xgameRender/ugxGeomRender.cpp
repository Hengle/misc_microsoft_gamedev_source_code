//============================================================================
//
//  ugxGeomRender.cpp
//  
//  Copyright (c) 2006, Ensemble Studios
//
//============================================================================
#include "xgameRender.h"
#include "ugxGeomRender.h"

#include "ugxGeomRenderPackedBone.h"
#include "stream\byteStream.h"

// shaders
#include "defConstRegs.inc"
#include "..\shaders\ugx\vShaderRegs.inc"
#include "..\shaders\shared\localLightingRegs.inc"
#if VSHADER_REGS_VER != 118
   #error Please update vShaderRegs.inc
#endif

// extlib
#include "granny.h"

// xgeom
#include "grannyToUnigeom.h"
#include "ugxInstancer.h"

// xrender/xgameRender
#include "tiledAA.h"
#include "renderDraw.h"
#include "visibleLightManager.h"
#include "ugxGeomManager.h"
#include "renderEventClasses.h"
#include "ugxGeomUberSectionRenderer.h"
#include "debugprimitives.h"

// xcore
#include "consoleOutput.h"

//============================================================================
// Constants
//============================================================================

static const uint cUGXGeomRenderMaxInstances       = 4;
static const uint cUGXGeomRenderMaxInstancesMask   = cUGXGeomRenderMaxInstances - 1;

// 20*8=160 regs
static const uint cMaxInstanceLights               = 20;

//============================================================================
// Globals
//============================================================================
eUGXGeomVisMode                           BUGXGeomRender::gVisMode;
BBlackmapParams                           BUGXGeomRender::gBlackmapParams;

BUGXGeomRenderCommonInstanceData*         BUGXGeomRender::gpRenderCommonData;

IDirect3DVertexBuffer9                    BUGXGeomRender::gBoneVB;
XMVECTOR                                  BUGXGeomRender::gInstanceControlRegs[4];
uint                                      BUGXGeomRender::gMaxInstancesPerDraw;
uint                                      BUGXGeomRender::gMaxInstancesPerDrawShift;
uint                                      BUGXGeomRender::gLightRegsPerInstance;
eUGXGeomPass                              BUGXGeomRender::gCurGlobalPass;
IUGXGeomSectionRendererManager*           BUGXGeomRender::gpCurSectionRendererManager;
double                                    BUGXGeomRender::gGameTime;
BManagedTextureHandle                     BUGXGeomRender::gGlobalEnvMap;
eUGXGeomTextureMode                       BUGXGeomRender::gTextureMode;
uint                                      BUGXGeomRender::gTotalDraws;
void*                                     BUGXGeomRender::gpGPUFrameStorageBones; // This is a write combined pointer!
uint                                      BUGXGeomRender::gGPUFrameStorageBonesSize;

#ifndef BUILD_FINAL
BDynamicRenderArray<AABB>                 BUGXGeomRender::gDebugBoundingBoxes;
bool                                      BUGXGeomRender::gDebugBoundingBoxesEnabled;
BUGXGeomRender::BStats                    BUGXGeomRender::gStats;
#endif

BUGXGeomUberSectionRendererManager        gUGXGeomUberSectionRendererManager;


//============================================================================
// BUGXGeomRender::BUGXGeomRender
//============================================================================
BUGXGeomRender::BUGXGeomRender(BEventReceiverHandle ownerHandle, BEventReceiverHandle managerHandle) :
   mStatus(cUGXGeomStatusInvalid),
   mOwnerEventHandle(ownerHandle),
   mManagerEventHandle(managerHandle),
   mNumBones(0),
   mRenderBegun(false),
   mLayerFlags(0),
   mpSectionRendererManager(&gUGXGeomUberSectionRendererManager),
   mpSectionRendererArray(NULL)
{
   ASSERT_MAIN_THREAD
      
   mEventHandle = gEventDispatcher.addClient(this, cThreadIndexRender);
}

//============================================================================
// BUGXGeomRender::getEventHandle
//============================================================================
BEventReceiverHandle BUGXGeomRender::getEventHandle(void) const 
{ 
   return mEventHandle; 
}

//============================================================================
// BUGXGeomRender::setEventHandle
//============================================================================
void BUGXGeomRender::setEventHandle(BEventReceiverHandle handle) 
{ 
   mEventHandle = handle;
}
 
//============================================================================
// BUGXGeomRender::getOwnerEventHandle
//============================================================================
BEventReceiverHandle BUGXGeomRender::getOwnerEventHandle(void) const 
{ 
   ASSERT_RENDER_THREAD
   return mOwnerEventHandle; 
}

//============================================================================
// BUGXGeomRender::setOwnerEventHandle
//============================================================================
void BUGXGeomRender::setOwnerEventHandle(BEventReceiverHandle handle) 
{ 
   ASSERT_RENDER_THREAD
   mOwnerEventHandle = handle;
}

//============================================================================
// BUGXGeomRender::getManagerEventHandle
//============================================================================
BEventReceiverHandle BUGXGeomRender::getManagerEventHandle(void) const 
{ 
   ASSERT_RENDER_THREAD
   return mOwnerEventHandle; 
}

//============================================================================
// BUGXGeomRender::setManagerEventHandle
//============================================================================
void BUGXGeomRender::setManagerEventHandle(BEventReceiverHandle handle) 
{ 
   ASSERT_RENDER_THREAD
   mManagerEventHandle = handle;
}

//============================================================================
// BUGXGeomRender::getStatus
//============================================================================
eUGXGeomStatus BUGXGeomRender::getStatus(void)
{
   //ASSERT_RENDER_THREAD;
   return mStatus;
}
      
//============================================================================
// BUGXGeomRender::~BUGXGeomRender
//============================================================================
BUGXGeomRender::~BUGXGeomRender()
{
   ASSERT_RENDER_THREAD
         
   if (mpSectionRendererManager)
   {
      mpSectionRendererManager->deinitSectionArray(mpSectionRendererArray);
      mpSectionRendererArray = NULL;
      
      mpSectionRendererManager = NULL;
   }
         
   changeStatus(cUGXGeomStatusInvalid);
         
   gEventDispatcher.removeClientImmediate(mEventHandle);
}

//============================================================================
// BUGXGeomRender::createRenderInfo
//============================================================================
BUGXGeomRenderInfo* BUGXGeomRender::createRenderInfo(void)
{
   const BUGXGeom::BNativeCachedDataType* pUGXGeom = mGeomData.getCachedData();
      
   BUGXGeomRenderInfo* pRenderInfo = BUGXGeomRenderInfo::newInstance();
      
   pRenderInfo->mNumBones = (WORD)pUGXGeom->numBones();
   pRenderInfo->mNumMeshes = (WORD)pUGXGeom->numAccessories();
   pRenderInfo->mNumSections = (WORD)pUGXGeom->numSections();
   pRenderInfo->mLayerFlags = mLayerFlags;
   pRenderInfo->mRenderFlags = mRenderFlags;
         
   return pRenderInfo;
}

//============================================================================
// BUGXGeomRender::changeStatus
//============================================================================
void BUGXGeomRender::changeStatus(eUGXGeomStatus newStatus)
{
   if (mStatus == newStatus)
      return;

   //trace("BUGXGeomRender::changeStatus: %i", newStatus);
            
   mStatus = newStatus;
   
   if (cInvalidEventReceiverHandle != mOwnerEventHandle)
      gEventDispatcher.send(mEventHandle, mOwnerEventHandle, cRenderEventClassUGXGeomStatusChanged, newStatus, 0, NULL, BEventDispatcher::cSendSynchronousDispatch);
   
   if (cInvalidEventReceiverHandle != mManagerEventHandle)
      gEventDispatcher.send(mEventHandle, mManagerEventHandle, cRenderEventClassUGXGeomStatusChanged, newStatus);
}

//============================================================================
// BUGXGeomRender::init
//============================================================================
bool BUGXGeomRender::init(BECFFileData* pECFFileData, eUGXGeomRenderFlags renderFlags)
{
   SCOPEDSAMPLE(BUGXGeomRender_init);
   ASSERT_RENDER_THREAD
   BDEBUG_ASSERT(pECFFileData);
      
   BASSERT((mStatus == cUGXGeomStatusInvalid) && (!mRenderBegun));
      
   BUnpackedUnivertPackerType bonePacker;

   //bonePacker.setUV(VertexElement::eDHEN3N, 0);
   //bonePacker.setUV(VertexElement::eDHEN3N, 1);
   //bonePacker.setUV(VertexElement::eDHEN3N, 2);
   bonePacker.setUV(VertexElement::eHALFFLOAT4, 0);
   bonePacker.setUV(VertexElement::eHALFFLOAT4, 1);
   bonePacker.setUV(VertexElement::eHALFFLOAT2, 2);
   bonePacker.setUV(VertexElement::eFLOAT3, 3);

   bonePacker.setPackOrder(BFixedString256(cVarArg, "%c0%c1%c2%c3", eTEXCOORDS_SPEC, eTEXCOORDS_SPEC, eTEXCOORDS_SPEC, eTEXCOORDS_SPEC));
   bonePacker.setDeclOrder(BFixedString256(cVarArg, "%c04%c15%c26%c37", eTEXCOORDS_SPEC, eTEXCOORDS_SPEC, eTEXCOORDS_SPEC, eTEXCOORDS_SPEC));

   BDEBUG_ASSERT(bonePacker.size() == cUGXGeomRenderBytesPerPackedBone);
   
   if (!mGeomData.setFileData(pECFFileData, &bonePacker))
   {
      gConsoleOutput.output(cMsgError, "BUGXGeomRender::init failed");
      
      changeStatus(cUGXGeomStatusFailed);
      return false;
   }
      
   mpSectionRendererArray = mpSectionRendererManager->initSectionArray(&mGeomData);
   if (!mpSectionRendererArray)
   {
      gConsoleOutput.output(cMsgError, "BUGXGeomRender::init failed");
      
      changeStatus(cUGXGeomStatusFailed);
      return false;
   }
   
   mLayerFlags = 0;
   bool shadowsDisabled = true;
   for (uint sectionIndex = 0; sectionIndex < mpSectionRendererArray->getSize(); sectionIndex++)
   {
      mLayerFlags = static_cast<uchar>(mLayerFlags | mpSectionRendererArray->getSection(sectionIndex).getLayerFlags());
      
      if (!mpSectionRendererArray->getSection(sectionIndex).getDisableShadows())
         shadowsDisabled = false;

      // Set local reflection flag if any section has it
      if (mpSectionRendererArray->getSection(sectionIndex).getLocalReflectionEnabled())
         renderFlags = (eUGXGeomRenderFlags) (renderFlags | cRFLocalReflection);
   }
      
   mNumBones = mGeomData.getCachedData()->numBones();      

   // If all sections are additive the model can't cast any shadows.
   if ((shadowsDisabled) || ((mLayerFlags & (cUGXGeomLayerOpaque | cUGXGeomLayerOver)) == 0))
   {
      renderFlags = (eUGXGeomRenderFlags)(renderFlags & ~cRFShadowCaster);
   }
   
   // If all sections are distortion materials the model can't cast any shadows.
   if (mLayerFlags == cUGXGeomLayerDistortion)
      renderFlags = (eUGXGeomRenderFlags)(renderFlags & ~cRFShadowCaster);
   
   if (mGeomData.getAABBTree(mAABBTree))
      renderFlags = (eUGXGeomRenderFlags)(renderFlags | cRFLargeModel);
         
   mRenderFlags = renderFlags;
            
   if (cInvalidEventReceiverHandle != mOwnerEventHandle)
      gEventDispatcher.send(mEventHandle, mOwnerEventHandle, cRenderEventClassUGXGeomRenderInfo, 0, 0, createRenderInfo(), BEventDispatcher::cSendSynchronousDispatch);
   
   changeStatus(cUGXGeomStatusReady);
         
   return true;
}

//============================================================================
// BUGXGeomRender::renderBegin
//============================================================================
void BUGXGeomRender::renderBegin(BUGXGeomRenderCommonInstanceData* pCommonData)
{
   if (mStatus != cUGXGeomStatusReady)
      return;
   
   BDEBUG_ASSERT(pCommonData->mNumPixelLights <= cMaxInstanceLights);
      
   BDEBUG_ASSERT(!mRenderBegun);
   mRenderBegun = true;
   
   if (gpCurSectionRendererManager != mpSectionRendererManager)
   {
      if (gpCurSectionRendererManager)
         gpCurSectionRendererManager->globalRenderEnd(gCurGlobalPass);
      
      gCurGlobalPass = pCommonData->mPass;
      gpCurSectionRendererManager = mpSectionRendererManager;
            
      mpSectionRendererManager->globalRenderBegin(gGameTime, gCurGlobalPass, gVisMode, gGlobalEnvMap, gTextureMode, gBlackmapParams);
   }
   
   gpRenderCommonData = pCommonData;
         
   mpSectionRendererManager->renderBegin(pCommonData);
   
   BDEBUG_ASSERT(mGeomData.getIB());
   
   mGeomData.setIndexBuffer();
      
   const BUGXGeom::BNativeHeaderType& header = mGeomData.getCachedData()->header();
   
   const int rigidBoneIndex = header.rigidBoneIndex() + 1;
   
   gInstanceControlRegs[0].x = (float)rigidBoneIndex;
   gInstanceControlRegs[0].y = mGeomData.getInstanceIndexMultiplier();
   gInstanceControlRegs[0].z = mGeomData.getInstanceIndexDivider();
   gInstanceControlRegs[0].w = 0;
   
   //gLightRegsPerInstance = 0;
         
   gMaxInstancesPerDraw = 1;
   gInstanceControlRegs[0].w = cMaxInstanceLights * 8;
   gLightRegsPerInstance = cMaxInstanceLights * 8;
   
   BCOMPILETIMEASSERT((cMaxInstanceLights >= BUGXGeomRenderPerInstanceData::cMaxPixelLights) && (cMaxInstanceLights >= BUGXGeomRenderPerInstanceData::cMaxPixelLights));
      
   if (0 == (mRenderFlags & cRFLargeModel))
   {
      gMaxInstancesPerDraw = mGeomData.getCachedData()->header().getMaxInstances();
      
      const uint numLights = pCommonData->mNumPixelLights;
            
      if (numLights <= 5)
      {
         gMaxInstancesPerDraw = Math::Min<uint>(4, gMaxInstancesPerDraw);
         
         // 5*8*4=160 regs
         gInstanceControlRegs[0].w = 5 * 8;
         gLightRegsPerInstance = 5 * 8;
      }
      else if (numLights <= 10)
      {
         gMaxInstancesPerDraw = Math::Min<uint>(2, gMaxInstancesPerDraw);
         
         // 10*8*2=160 regs
         gInstanceControlRegs[0].w = 10 * 8;
         gLightRegsPerInstance = 10 * 8;
      }
      else
      {
         gMaxInstancesPerDraw = 1;
      }
   }      
   
   BDEBUG_ASSERT(gLightRegsPerInstance * gMaxInstancesPerDraw <= NUM_LOCAL_LIGHT_PSHADER_CONSTANTS);
   
   gMaxInstancesPerDrawShift = Math::iLog2(gMaxInstancesPerDraw);
   
   D3DVECTOR4* pInstanceControlRegs;
   HRESULT hres = gpUGXD3DDev->GpuBeginVertexShaderConstantF4(INSTANCE_CONTROL_REG, &pInstanceControlRegs, 4);
   if (SUCCEEDED(hres))
   {
      memcpy(pInstanceControlRegs, gInstanceControlRegs, sizeof(XMVECTOR) * 4);
      gpUGXD3DDev->GpuEndVertexShaderConstantF4();
   }
}

//============================================================================
// BUGXGeomRender::renderEnd
//============================================================================
void BUGXGeomRender::renderEnd(void)
{
   if (mStatus != cUGXGeomStatusReady)
      return;
   
   BDEBUG_ASSERT(mRenderBegun);
   
   mpSectionRendererManager->renderEnd(gpRenderCommonData);
   
   mRenderBegun = false;
}

//============================================================================
// BUGXGeomRender::render 
//============================================================================
bool BUGXGeomRender::isLargeObjectSectionVisible(const XMMATRIX& worldMatrix, const BVolumeCuller& pCuller)
{
   //this function will return true if any leaf section of a large model is visible
   // this is important for functions like reflection determination, where the highest BB of the object
   //is visible, but the subsections aren't.

   if (!(mRenderFlags & cRFLargeModel))
      return false;

   if (!mAABBTree.numNodes())
      return false;

   BDEBUG_ASSERT(mAABBTree.numNodes() <= UINT16_MAX);

   BStaticArray<const BAABBTree::BNode*, 128> nodeStack;
   nodeStack.pushBack(mAABBTree.root());

   while (!nodeStack.isEmpty())
   {
      if (nodeStack.back() == NULL)
      {
         nodeStack.popBack();
         continue;
      }

      const BAABBTree::BNode& node = *nodeStack.back();
      nodeStack.popBack();

      const AABB& boneAABB = node.bounds();

      //CLM [11.10.08] The bone-to-world matricies look to not be updating at the proper frequency. So just root bone for now.
      AABB worldBounds(boneAABB.transform3(*(const BMatrix44*)&worldMatrix));

      if (!pCuller.isAABBVisible(
            *reinterpret_cast<const D3DXVECTOR3*>(&worldBounds[0]), 
            *reinterpret_cast<const D3DXVECTOR3*>(&worldBounds[1])))
      {
        continue;
      }

      if (node.leaf())
      {
         return true;
      }
      else
      {
         //-- FIXING PREFIX BUG ID 6668
         const BAABBTree::BNode* pLeftChild = node.child(0);
         //--
         if (pLeftChild)
            nodeStack.pushBack(pLeftChild);

         //-- FIXING PREFIX BUG ID 6669
         const BAABBTree::BNode* pRightChild = node.child(1);
         //--
         if (pRightChild)
            nodeStack.pushBack(pRightChild);
      }
   }

  return false;
}

//============================================================================
// BUGXGeomRender::render
//============================================================================
void BUGXGeomRender::render(const BUGXGeomRenderMeshMask& meshMask, const BUGXGeomRenderPerInstanceData* pInstances, uint numInstances)
{
   SCOPEDSAMPLE(BUGXGeomRender);
   
   BDEBUG_ASSERT(mRenderBegun);
   
   if (mStatus != cUGXGeomStatusReady)
      return;
      
   if ((mLayerFlags & gpRenderCommonData->mLayerFlags) == 0)
      return;

#ifndef BUILD_FINAL      
   gStats.mTotalInstances += numInstances;
#endif

   if (mRenderFlags & cRFLargeModel)
   {
#ifndef BUILD_FINAL      
      gStats.mTotalLargeRenders += numInstances;
#endif   
      for (uint i = 0; i < numInstances; i++)
         renderLargeModel(pInstances[i]);
   }
   else
   {
#ifndef BUILD_FINAL      
      gStats.mTotalInstanceRenders++;
#endif   

      renderWithInstancing(meshMask, pInstances, numInstances);
   }
}

//============================================================================
// BUGXGeomRender::getBoneMatrix
//============================================================================
XMMATRIX BUGXGeomRender::getBoneMatrix(uint instanceBoneVertexIndex, int boneIndex)
{
   const uchar* pPackedBone = 
      static_cast<const uchar*>(GPU_CONVERT_CPU_TO_CPU_CACHED_READONLY_ADDRESS(gpGPUFrameStorageBones)) + 
      ((instanceBoneVertexIndex + boneIndex + 2) << cUGXGeomRenderBytesPerPackedBoneLog2);
      
   return BUGXGeomRenderPackedBone::unpackMatrix(pPackedBone);
}

//============================================================================
// BUGXGeomRender::cullSections
//============================================================================
void BUGXGeomRender::cullSections(const BUGXGeomRenderPerInstanceData& instance, BSectionToRenderArray& sectionsToRender)
{
   const BUGXGeom::BNativeCachedDataType& ugxData = *mGeomData.getCachedData();
   const BUGXGeom::BNativeCachedDataType::GeomHeaderType& header = ugxData.header();
   
   const int rigidBoneIndex = header.hasLargeGeomBoneIndex() ? header.largeGeomBoneIndex() : -1;
   
   const uint instanceBoneVertexIndex = (uint)Math::FloatToUInt64TruncIntALU(&instance.mColorBoneMatrices.w);
   XMMATRIX boneToWorld = getBoneMatrix(instanceBoneVertexIndex, rigidBoneIndex);
      
   BDEBUG_ASSERT(mAABBTree.numNodes() <= UINT16_MAX);
   
   BStaticArray<const BAABBTree::BNode*, 128> nodeStack;
   nodeStack.pushBack(mAABBTree.root());
   
   while (!nodeStack.isEmpty())
   {
      if (nodeStack.back() == NULL)
      {
         nodeStack.popBack();
         continue;
      }
      
      const BAABBTree::BNode& node = *nodeStack.back();
      nodeStack.popBack();

      const AABB& boneAABB = node.bounds();

      AABB worldBounds(boneAABB.transform3(*(const BMatrix44*)&boneToWorld));

#ifndef BUILD_FINAL      
      gStats.mTotalLargeNodeCullTests++;
#endif      
              
      if (gpRenderCommonData && gpRenderCommonData->mpVolumeCuller)
      {
         if (!gpRenderCommonData->mpVolumeCuller->isAABBVisible(
            *reinterpret_cast<const D3DXVECTOR3*>(&worldBounds[0]), 
            *reinterpret_cast<const D3DXVECTOR3*>(&worldBounds[1])))
         {
            continue;
         }
      }

#ifndef BUILD_FINAL      
      gStats.mTotalLargeNodeCullPasses++;
#endif            

      if (node.leaf())
      {
#ifndef BUILD_FINAL
         if (gDebugBoundingBoxesEnabled)
            gDebugBoundingBoxes.pushBack(worldBounds);
#endif
      
         const uint nodeIndex = mAABBTree.index(&node);
         BDEBUG_ASSERT(nodeIndex < (uint)ugxData.numAccessories());

         const Unigeom::BNativeAccessoryType& accessory = ugxData.accessory(nodeIndex);
         BDEBUG_ASSERT(accessory.numObjectIndices());
         
         uint tileFlags = 0;

         if (gpRenderCommonData && gpRenderCommonData->mSetCommandBufferRunPredication)      
         {
            uint tileBitMask = 1;
            for (uint tileIndex = 0; tileIndex < gTiledAAManager.getNumTiles(); tileIndex++, tileBitMask <<= 1)
            {
               if (0 == (gpRenderCommonData->mTileFlags & tileBitMask))
                  continue;

               const BVolumeCuller& volumeCuller = gTiledAAManager.getTileVolumeCuller(tileIndex);

               if (volumeCuller.isAABBVisible(*reinterpret_cast<const D3DXVECTOR3*>(&worldBounds[0]), *reinterpret_cast<const D3DXVECTOR3*>(&worldBounds[1])))
                  tileFlags |= tileBitMask;
            }
         }

         for (int i = 0; i < accessory.numObjectIndices(); i++)
            sectionsToRender.pushBack(BSectionToRender(accessory.objectIndex(i), worldBounds, tileFlags));
      }
      else
      {
//-- FIXING PREFIX BUG ID 6668
         const BAABBTree::BNode* pLeftChild = node.child(0);
//--
         if (pLeftChild)
            nodeStack.pushBack(pLeftChild);

//-- FIXING PREFIX BUG ID 6669
         const BAABBTree::BNode* pRightChild = node.child(1);
//--
         if (pRightChild)
            nodeStack.pushBack(pRightChild);
      }
   }
   
   if (!sectionsToRender.isEmpty())
      std::sort(sectionsToRender.begin(), sectionsToRender.end());
   
#ifndef BUILD_FINAL      
   gStats.mTotalLargeSectionsRendered += sectionsToRender.getSize();
#endif   
}

//============================================================================
// BUGXGeomRender::shouldRenderSection
//============================================================================
bool BUGXGeomRender::shouldRenderSection(const IUGXGeomSectionRenderer& section)
{
   eUGXGeomPass pass = gpRenderCommonData->mPass;
   
   if (pass >= cUGXGeomPassShadowGen)        
   {
      if (section.getDisableShadows())
         return false;
   }
   
   if ((section.getLayerFlags() & gpRenderCommonData->mLayerFlags) == 0)
      return false;

   // If we're in the reflection pass and this geom and section are local reflectors
   // don't render the section.  We don't want to reflect the reflectors themselves
   if ((pass == cUGXGeomPassMainReflect) || (gpRenderCommonData->mPass == cUGXGeomPassOverallAlphaReflect))
   {
      if (gpRenderCommonData->mLocalReflection && section.getLocalReflectionEnabled())
         return false;
   }
   
   return true;
}

//============================================================================
// BUGXGeomRender::getLight
//============================================================================
void BUGXGeomRender::getLight(XMVECTOR* pDst, const XMVECTOR* pSrcTexels, int lightIndex)
{
   if (lightIndex < 0)
   {
      XMVECTOR zero = XMVectorZero();
      pDst[0] = zero;
      pDst[1] = zero;
      pDst[2] = zero;
      pDst[3] = zero;
   }
   else
   {
      lightIndex *= BVisibleLightManager::cTexelsPerLight;
      pDst[0] = pSrcTexels[lightIndex + 0];
      pDst[1] = pSrcTexels[lightIndex + 1];
      pDst[2] = pSrcTexels[lightIndex + 2];
      pDst[3] = pSrcTexels[lightIndex + 3];
   }
}
//============================================================================
// BUGXGeomRender::setLightGroup
//============================================================================
void BUGXGeomRender::setLightGroup(uint dstReg, const short* pVisibleLightIndices)
{
   XMVECTOR lightParams[4 * 4];
   
   const XMVECTOR* pLightTexels = reinterpret_cast<const XMVECTOR*>(gVisibleLightManager.getVisibleLightTexels(true));
   
   getLight(&lightParams[0], pLightTexels, pVisibleLightIndices[0]);
   getLight(&lightParams[4], pLightTexels, pVisibleLightIndices[1]);
   getLight(&lightParams[8], pLightTexels, pVisibleLightIndices[2]);
   getLight(&lightParams[12], pLightTexels, pVisibleLightIndices[3]);
         
#if 0   
// 0 OmniPos, OmniMul
// 1 Color, OmniAdd
// 2 DecayDist, SpotMul, SpotAdd, ShadowIndex
// 3 spotAt 

#define lightPos0     gLightData[lightIndex    ]
#define lightPos1     gLightData[lightIndex + 1]
#define lightPos2     gLightData[lightIndex + 2]
#define lightPos3     gLightData[lightIndex + 3]
#define lightColor0   gLightData[lightIndex + 4]
#define lightColor1   gLightData[lightIndex + 5]
#define lightColor2   gLightData[lightIndex + 6]
#define lightColor3   gLightData[lightIndex + 7]
#define omniMul0123   gLightData[lightIndex + 8]
#define omniAdd0123   gLightData[lightIndex + 9]
#define decayDist0123 gLightData[lightIndex + 10]

#endif

   XMVECTOR* pDst;
   HRESULT hres = gpUGXD3DDev->GpuBeginVertexShaderConstantF4(dstReg, (D3DVECTOR4**)&pDst, 12);
   if (SUCCEEDED(hres))
   {
      pDst[0] = lightParams[0]; _WriteBarrier();
      pDst[1] = lightParams[4]; _WriteBarrier();
      pDst[2] = lightParams[8]; _WriteBarrier();
      pDst[3] = lightParams[12]; _WriteBarrier();
      
      pDst[4] = lightParams[1]; _WriteBarrier();
      pDst[5] = lightParams[5]; _WriteBarrier();
      pDst[6] = lightParams[9]; _WriteBarrier();
      pDst[7] = lightParams[13]; _WriteBarrier();
      
      pDst[8] = XMVectorSet(lightParams[0].w, lightParams[4].w, lightParams[8].w, lightParams[12].w); _WriteBarrier();
      pDst[9] = XMVectorSet(lightParams[1].w, lightParams[5].w, lightParams[9].w, lightParams[13].w); _WriteBarrier();
      pDst[10] = XMVectorSet(lightParams[2].x, lightParams[6].x, lightParams[10].x, lightParams[14].x); _WriteBarrier();
      pDst[11] = XMVectorZero(); _WriteBarrier();
         
      gpUGXD3DDev->GpuEndVertexShaderConstantF4();
   }      
} 

//============================================================================
// BUGXGeomRender::renderLargeModel
//============================================================================
void BUGXGeomRender::renderLargeModel(const BUGXGeomRenderPerInstanceData& instance)
{
   if (!mAABBTree.numNodes())
      return;
      
   BSectionToRenderArray sectionsToRender;
   cullSections(instance, sectionsToRender);

   const BUGXGeom::BNativeCachedDataType& ugxData = *mGeomData.getCachedData();
   const BUGXGeom::BNativeCachedDataType::GeomHeaderType& header = ugxData.header();

   const eUGXGeomPass pass = gpRenderCommonData->mPass;      
   
   const XMFLOAT4* pTexels = gVisibleLightManager.getVisibleLightTexels();

   const uint cMaxPassStateBufSize = 256;
   __declspec(align(16)) uchar passStateBuf[cMaxPassStateBufSize];
   
   BSceneLightManager::BActiveLightIndexArray lights;
   
   for (uint sectionIter = 0; sectionIter < sectionsToRender.getSize(); sectionIter++)
   {
      const AABB& worldBounds = sectionsToRender[sectionIter].mWorldBounds;
      const int sectionIndex = sectionsToRender[sectionIter].mSectionIndex;
      const uint tileFlags = sectionsToRender[sectionIter].mTileFlags;
      
      const BUGXGeom::BNativeSectionType& UGXSection = ugxData.section(sectionIndex);
      IUGXGeomSectionRenderer& section = mpSectionRendererArray->getSection(sectionIndex);
            
      if (!shouldRenderSection(section))
         continue;

      if (gpRenderCommonData->mSetCommandBufferRunPredication)      
         gpUGXGeomRSFilter->setCommandBufferPredication(0, tileFlags);
         
      uint numPixelLights = 0;
                        
      if ((pass < cUGXGeomPassShadowGen) && (gpRenderCommonData->mLocalLighting))
      {
         const XMVECTOR boundsMin = XMLoadFloat3((const XMFLOAT3*)&worldBounds[0]);
         const XMVECTOR boundsMax = XMLoadFloat3((const XMFLOAT3*)&worldBounds[1]);
         const BSceneLightManager::BGridRect gridRect = gRenderSceneLightManager.getGridRect(boundsMin, boundsMax);
            
         lights.resize(0);
         gVisibleLightManager.findLights(lights, gridRect, boundsMin, boundsMax, true, true);

         numPixelLights = Math::Min<uint>(lights.getSize(), cMaxInstanceLights);

         bool localShadowing = false;         
         for (uint lightIter = 0; lightIter < lights.size(); lightIter++)
         {
            const uint visibleLightIndex = lights[lightIter];
            if (gVisibleLightManager.getVisibleLightShadows(visibleLightIndex))
            {
               localShadowing = true;
               break;
            }
         }   
                                                
         gpRenderCommonData->mNumPixelLights = static_cast<uchar>(numPixelLights);
         gpRenderCommonData->mLocalLightShadows = localShadowing;      
         mpSectionRendererManager->renderSetLocalLightState(gpRenderCommonData);
      }
      
      if (!section.beginPass(gpRenderCommonData, passStateBuf, cMaxPassStateBufSize, gVisMode))
         return;

      if (!header.rigidOnly())
      {
         if (UGXSection.rigidOnly())
         {
            // Rigid only - set this section's matrix.
            const int rigidBoneIndex = UGXSection.rigidBoneIndex() + 1;

            gInstanceControlRegs[0].x = static_cast<float>(rigidBoneIndex);

            D3DVECTOR4* pInstanceControlRegs;
            HRESULT hres = gpUGXD3DDev->GpuBeginVertexShaderConstantF4(INSTANCE_CONTROL_REG, &pInstanceControlRegs, 4);
            if (SUCCEEDED(hres))
            {
               memcpy(pInstanceControlRegs, gInstanceControlRegs, sizeof(XMVECTOR) * 4);
               gpUGXD3DDev->GpuEndVertexShaderConstantF4();
            }
         }

         {
            BOOL boneFlags[2];
            boneFlags[0] = (UGXSection.maxBones() == 1);
            boneFlags[1] = (UGXSection.maxBones() > 2);
            gpUGXGeomRSFilter->setVertexShaderConstantB(ONE_BONE_REG, boneFlags, 2);     
         }
      }

      //mGeomData.drawSectionInit(sectionIndex);
      gpUGXD3DDev->SetStreamSource(0, mGeomData.getVB(), UGXSection.VBOfs(), 0);
                           
      for (uint i = 0; i < numPixelLights; i++)
         gpUGXD3DDev->GpuLoadPixelShaderConstantF4Pointer(i * 8, pTexels + 8 * lights[i], 8);
                                    
      D3DVECTOR4* pWriteCombinedConstantData;
      HRESULT hres = gpUGXD3DDev->GpuBeginVertexShaderConstantF4(INSTANCE_DATA_REG, &pWriteCombinedConstantData, 4);
      if (SUCCEEDED(hres))
      {
         XMStoreFloat4NC((XMFLOAT4*)pWriteCombinedConstantData, instance.mColorBoneMatrices);
         XMVECTOR zero = XMVectorZero();
         XMStoreFloat4NC((XMFLOAT4*)pWriteCombinedConstantData + 1, zero);
         XMStoreFloat4NC((XMFLOAT4*)pWriteCombinedConstantData + 2, zero);
         XMStoreFloat4NC((XMFLOAT4*)pWriteCombinedConstantData + 3, zero);

         gpUGXD3DDev->GpuEndVertexShaderConstantF4();
      }         

      gpUGXGeomRSFilter->flushDeferredState();
      
      mGeomData.drawSection(sectionIndex, 1);
                  
      section.endPass(gpRenderCommonData, passStateBuf, cMaxPassStateBufSize);
      
      gTotalDraws++;
      
#ifndef BUILD_FINAL      
      gStats.mTotalDraws++;
#endif      
   }   
}

//============================================================================
// BUGXGeomRender::createSectionIndices
//============================================================================
void BUGXGeomRender::createSectionIndices(BStaticArray<uint16, 128>& sectionIndices, const BUGXGeomRenderMeshMask& meshMask)
{
   const BUGXGeom::BNativeCachedDataType& ugxData = *mGeomData.getCachedData();
   
   const BUGXGeom::BNativeCachedDataType::AccessoryArrayType& accessories = ugxData.accessories();
      
   for (uint accessoryIndex = 0; accessoryIndex < accessories.getSize(); accessoryIndex++)
   {
      if (!meshMask.getBit(accessoryIndex))
         continue;
      
      const Unigeom::BNativeAccessoryType& accessory = accessories[accessoryIndex];
        
      for (int i = 0; i < accessory.numObjectIndices(); i++)
         sectionIndices.pushBack(static_cast<uint16>(accessory.objectIndex(i)));
   }
   
   sectionIndices.sort();   
}

//============================================================================
// BUGXGeomRender::renderWithInstancing
//============================================================================
void BUGXGeomRender::renderWithInstancing(BUGXGeomRenderMeshMask meshMask, const BUGXGeomRenderPerInstanceData* pInstances, uint numInstances)
{
   const BUGXGeom::BNativeCachedDataType& ugxData = *mGeomData.getCachedData();
   const BUGXGeom::BNativeHeaderType& header = mGeomData.getCachedData()->header();   
   
   BStaticArray<uint16, 128> sectionIndices;
   uint numSectionsToRender = ugxData.numSections();
   if (meshMask.areAnyNotSet())
   {
      if (!ugxData.numAccessories())
         meshMask.setAll();
      else
      {
         createSectionIndices(sectionIndices, meshMask);
         numSectionsToRender = sectionIndices.getSize();
      }
   }
            
   const XMFLOAT4* pTexels = gVisibleLightManager.getVisibleLightTexels();
   
   const uint cMaxPassStateBufSize = 256;
   __declspec(align(16)) uchar passStateBuf[cMaxPassStateBufSize];
   
   const uint totalInstanceGroups = numInstances >> gMaxInstancesPerDrawShift;
   
   const bool useMeshMask = meshMask.areAnyNotSet();
   for (uint sectionIter = 0; sectionIter < numSectionsToRender; sectionIter++)
   {
      const int sectionIndex = useMeshMask ? sectionIndices[sectionIter] : sectionIter;
            
      const BUGXGeom::BNativeSectionType& UGXSection = mGeomData.getCachedData()->section(sectionIndex);
      IUGXGeomSectionRenderer& section = mpSectionRendererArray->getSection(sectionIndex);
            
      if (!shouldRenderSection(section))
         continue;
         
      if (!section.beginPass(gpRenderCommonData, passStateBuf, cMaxPassStateBufSize, gVisMode))
         return;
         
#ifndef BUILD_FINAL      
      gStats.mTotalInstanceSectionsRendered++;
#endif                     

      if (!header.rigidOnly())
      {
         if (UGXSection.rigidOnly())
         {
            // Rigid only - set this section's matrix.
            const int rigidBoneIndex = UGXSection.rigidBoneIndex() + 1;

            gInstanceControlRegs[0].x = static_cast<float>(rigidBoneIndex);
                                    
            D3DVECTOR4* pInstanceControlRegs;
            HRESULT hres = gpUGXD3DDev->GpuBeginVertexShaderConstantF4(INSTANCE_CONTROL_REG, &pInstanceControlRegs, 4);
            if (SUCCEEDED(hres))
            {
               memcpy(pInstanceControlRegs, gInstanceControlRegs, sizeof(XMVECTOR) * 4);
               gpUGXD3DDev->GpuEndVertexShaderConstantF4();
            }
         }
         
         {
            BOOL boneFlags[2];
            boneFlags[0] = (UGXSection.maxBones() == 1);
            boneFlags[1] = (UGXSection.maxBones() > 2);
            gpUGXGeomRSFilter->setVertexShaderConstantB(ONE_BONE_REG, boneFlags, 2);     
         }
      }
      
      //mGeomData.drawSectionInit(sectionIndex);
      gpUGXD3DDev->SetStreamSource(0, mGeomData.getVB(), UGXSection.VBOfs(), 0);
            
      gpUGXGeomRSFilter->flushDeferredState();
      
      //const uint numIndices = mGeomData.getCachedData()->section(sectionIndex).numTris() * 3 * gMaxInstancesPerDraw;
      //void* pGPUIndexBufferData = (void*)GPU_CONVERT_CPU_TO_GPU_ADDRESS((WORD*)mGeomData.getIndexBufferData() + UGXSection.IBOfs());
                       
      const BUGXGeomRenderPerInstanceData* pCurInstances = pInstances;

      // 1 or more local lights
      for (uint instanceIndex = 0; instanceIndex < totalInstanceGroups; instanceIndex++)
      {
         if (gpRenderCommonData->mNumPixelLights)
         {
            uint constOfs = 0;
            for (uint mesh = 0; mesh < gMaxInstancesPerDraw; mesh++)
            {
               for (uint i = 0; i < gpRenderCommonData->mNumPixelLights; i++)
                  gpUGXD3DDev->GpuLoadPixelShaderConstantF4Pointer(constOfs + i * 8, pTexels + 8 * pCurInstances[mesh].mVisiblePixelLightIndices[i], 8);
               
               constOfs += gLightRegsPerInstance;
            }
         }     
                  
         D3DVECTOR4* pWriteCombinedConstantData;
         HRESULT hres = gpUGXD3DDev->GpuBeginVertexShaderConstantF4(INSTANCE_DATA_REG, &pWriteCombinedConstantData, 4);
         if (SUCCEEDED(hres))
         {
            if (gMaxInstancesPerDraw < 4)
            {
               for (uint i = 0; i < gMaxInstancesPerDraw; i++)
                  XMStoreFloat4NC((XMFLOAT4*)pWriteCombinedConstantData+i, pCurInstances[i].mColorBoneMatrices);

               for (uint i = gMaxInstancesPerDraw; i < 4; i++)
                  XMStoreFloat4NC((XMFLOAT4*)pWriteCombinedConstantData+i, XMVectorZero());
            }
            else
            {
               XMStoreFloat4NC((XMFLOAT4*)pWriteCombinedConstantData, pCurInstances[0].mColorBoneMatrices);
               XMStoreFloat4NC((XMFLOAT4*)pWriteCombinedConstantData+1, pCurInstances[1].mColorBoneMatrices);
               XMStoreFloat4NC((XMFLOAT4*)pWriteCombinedConstantData+2, pCurInstances[2].mColorBoneMatrices);
               XMStoreFloat4NC((XMFLOAT4*)pWriteCombinedConstantData+3, pCurInstances[3].mColorBoneMatrices);
            }                  

            gpUGXD3DDev->GpuEndVertexShaderConstantF4();
         }            

#ifndef BUILD_FINAL      
         gStats.mTotalDraws++;
#endif
         mGeomData.drawSection(sectionIndex, gMaxInstancesPerDraw);

         gTotalDraws++;

         pCurInstances += gMaxInstancesPerDraw;
      }

      const uint numLeftoverInstances = numInstances & (gMaxInstancesPerDraw - 1);
      if (numLeftoverInstances)
      {
         XMVECTOR constants[4];

         uint constOfs = 0;
         uint mesh;
         for (mesh = 0; mesh < numLeftoverInstances; mesh++)
         {
            constants[mesh] = pCurInstances[mesh].mColorBoneMatrices;

            for (uint i = 0; i < gpRenderCommonData->mNumPixelLights; i++)
               gpUGXD3DDev->GpuLoadPixelShaderConstantF4Pointer(constOfs + i * 8, pTexels + 8 * pCurInstances[mesh].mVisiblePixelLightIndices[i], 8);

            constOfs += gLightRegsPerInstance;
         }

         while (mesh < 4)
            constants[mesh++] = XMVectorZero();

         D3DVECTOR4* pWriteCombinedConstantData;
         HRESULT hres = gpUGXD3DDev->GpuBeginVertexShaderConstantF4(INSTANCE_DATA_REG, &pWriteCombinedConstantData, 4);
         if (SUCCEEDED(hres))
         {
            XMStoreFloat4NC((XMFLOAT4*)pWriteCombinedConstantData, constants[0]);
            XMStoreFloat4NC((XMFLOAT4*)pWriteCombinedConstantData + 1, constants[1]);
            XMStoreFloat4NC((XMFLOAT4*)pWriteCombinedConstantData + 2, constants[2]);
            XMStoreFloat4NC((XMFLOAT4*)pWriteCombinedConstantData + 3, constants[3]);
            gpUGXD3DDev->GpuEndVertexShaderConstantF4();
         }            

#ifndef BUILD_FINAL      
         gStats.mTotalDraws++;
#endif
         mGeomData.drawSection(sectionIndex, numLeftoverInstances);

         gTotalDraws++;
      }
        
      section.endPass(gpRenderCommonData, passStateBuf, cMaxPassStateBufSize);
   }      
}

//============================================================================
// BUGXGeomRender::globalRenderBegin
//============================================================================
void BUGXGeomRender::globalRenderBegin(double gameTime, void* pGPUFrameStorageBones, uint GPUFrameStorageBonesSize, eUGXGeomPass pass, BManagedTextureHandle globalEnvMap)
{
   //ASSERT_RENDER_THREAD

#ifndef BUILD_FINAL      
   gStats.mTotalGlobalBegins++;
#endif
         
   //mpSectionRendererManager->globalRenderBegin(pass, visMode);
   
   gGameTime = gameTime;
   gGlobalEnvMap = globalEnvMap;
   
   gpGPUFrameStorageBones = pGPUFrameStorageBones;
   gGPUFrameStorageBonesSize = GPUFrameStorageBonesSize;
               
   XGSetVertexBufferHeader(GPUFrameStorageBonesSize, 0, 0, 0, &gBoneVB);
   XGOffsetResourceAddress(&gBoneVB, pGPUFrameStorageBones);

   gpUGXD3DDev->SetStreamSource(1, &gBoneVB, 0, cUGXGeomRenderBytesPerPackedBone);
         
   if (BD3D::mpDev == gpUGXD3DDev)
   {
      if (pass >= cUGXGeomPassShadowGen)
         gpUGXD3DDev->SetShaderGPRAllocation(0, 96, 32);
      else
         gpUGXD3DDev->SetShaderGPRAllocation(0, 26, 102);
   }         
   
   gpUGXD3DDev->SetVertexDeclaration(NULL);
   
   gpUGXGeomRSFilter->setDevice(gpUGXD3DDev);
   gpUGXGeomRSFilter->gpuOwn(240, 16, 240, 16);
      
   gpUGXD3DDev->GpuOwnVertexShaderConstantF(FIRST_VSHADER_INSTANCE_REG, NUM_VSHADER_INSTANCE_REGS);
}

//============================================================================
// BUGXGeomRender::globalRenderEnd
//============================================================================
void BUGXGeomRender::globalRenderEnd(eUGXGeomPass pass)
{
   //ASSERT_RENDER_THREAD
   
   gpUGXD3DDev->GpuDisownVertexShaderConstantF(FIRST_VSHADER_INSTANCE_REG, NUM_VSHADER_INSTANCE_REGS);
   
   if (gpCurSectionRendererManager)
   {
      gpCurSectionRendererManager->globalRenderEnd(gCurGlobalPass);
      gpCurSectionRendererManager = NULL;
   }
         
   gpUGXGeomRSFilter->gpuDisown();
   gpUGXGeomRSFilter->setDevice(BD3D::mpDev);
   
   if (BD3D::mpDev == gpUGXD3DDev)
      gpUGXD3DDev->SetShaderGPRAllocation(0, 64, 64);
      
   gpUGXD3DDev->SetStreamSource(0, NULL, 0, 0);
   gpUGXD3DDev->SetStreamSource(1, NULL, 0, 0);
         
   gpUGXD3DDev->SetIndices(NULL);
   
   for (uint i = 0; i < cMaxD3DTextureSamplers; i++)
      gpUGXD3DDev->SetTexture(i, NULL);
      
   gpUGXD3DDev->SetPixelShader(NULL);      
   gpUGXD3DDev->SetVertexShader(NULL);      
   
   gpUGXD3DDev->SetVertexDeclaration(NULL);
}

//============================================================================
// BUGXGeomRender::receiveEvent
//============================================================================   
bool BUGXGeomRender::receiveEvent(const BEvent& event, BThreadIndex threadIndex)
{
   ASSERT_RENDER_THREAD
      
   switch (event.mEventClass)
   {
      case cRenderEventClassUGXGeomInit:
      {
         init(reinterpret_cast<BECFFileData*>(event.mPrivateData), static_cast<eUGXGeomRenderFlags>(event.mPrivateData2));
         break;
      }
   }

   return false;
}

//============================================================================
// BUGXGeomRender::tickDebugBoundingBoxes
//============================================================================   
void BUGXGeomRender::tickDebugBoundingBoxes(bool boundingBoxesEnabled)
{
#ifndef BUILD_FINAL
   gDebugBoundingBoxesEnabled = boundingBoxesEnabled;
   
   if (gpDebugPrimitives)
   {
      for (uint i = 0; i < gDebugBoundingBoxes.getSize(); i++)
      {
         const AABB& box = gDebugBoundingBoxes[i];
         
         gpDebugPrimitives->addDebugBox(BVector(box[0][0], box[0][1], box[0][2]), BVector(box[1][0], box[1][1], box[1][2]), 0xFF2FE0FF);
      }
   }
   
   gDebugBoundingBoxes.resize(0);
#endif   
}

