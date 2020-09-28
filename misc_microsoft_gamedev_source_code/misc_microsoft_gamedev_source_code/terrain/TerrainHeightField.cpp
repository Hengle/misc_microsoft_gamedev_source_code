//==============================================================================
// TerrainHeightField.cpp
//
// Copyright (c) 2006 Ensemble Studios
//==============================================================================

// terrain
#include "TerrainPCH.h"
#include "TerrainHeightField.h"
#include "terrain.h"

// xcore
#include "consoleOutput.h"
#include "resource\ecfUtils.h"
#include "math\generalMatrix.h"
#include "containers\dynamicArray2D.h"
#include "math\random.h"
#include "bfileStream.h"

// xrender
#include "gpuHeap.h"
#include "render.h"
#include "debugprimitives.h"
#include "vertexTypes.h"
#include "visibleLightManager.h"

// ximage
#include "rgbaImage.h"
#include "imageUtils.h"



#define SHADER_FILENAME "terrain\\terrainHeightField.bin"

//#define DUMP_HEIGHTFIELD_IMAGE

//==============================================================================
// Globals
//==============================================================================
BTerrainHeightField gTerrainHeightField;

//==============================================================================
// BTerrainHeightField::BTerrainHeightField
//==============================================================================
BTerrainHeightField::BTerrainHeightField() :
   mpHeightField(NULL),
   mpHeightFieldAlpha(NULL),
   mpEffectLoader(NULL),
   mpPatchInstanceDecl(NULL)
{
   ASSERT_THREAD(cThreadIndexSim);
   
   Utils::ClearObj(mHeightFieldAttributes);
}

//==============================================================================
// BTerrainHeightField::~BTerrainHeightField
//==============================================================================
BTerrainHeightField::~BTerrainHeightField()
{
   ASSERT_THREAD(cThreadIndexSim);
}

//==============================================================================
// BTerrainHeightField::init
//==============================================================================
void BTerrainHeightField::init(void)
{
   ASSERT_THREAD(cThreadIndexSim);
   
   if (mCommandHandle != cInvalidCommandListenerHandle)
      return;
   
   commandListenerInit();
}

//==============================================================================
// BTerrainHeightField::deinit
//==============================================================================
void BTerrainHeightField::deinit(void)
{
   ASSERT_THREAD(cThreadIndexSim);
   
   if (mCommandHandle == cInvalidCommandListenerHandle)
      return;
         
   commandListenerDeinit();
}

//==============================================================================
// BTerrainHeightField::computeTransforms
//==============================================================================
void BTerrainHeightField::computeTransforms(XMMATRIX& worldToView, XMMATRIX& viewToProj, AABB& worldBounds, double& worldMinY, double& worldMaxY)
{
   D3DXVECTOR3 worldMin(gTerrain.getMin());
   D3DXVECTOR3 worldMax(gTerrain.getMax());
   
   worldMin.y -= 1.0f;
   worldMax.y += 1.0f;
               
   XMVECTOR eyePos = XMVectorSet((worldMin.x + worldMax.x) * .5f, worldMax.y + 5.0f, (worldMin.z + worldMax.z) * .5f, 1.0f);
         
   XMVECTOR focusPos = eyePos + XMVectorSet(0.0f, -1.0f, 0.0f, 0.0f);
   XMVECTOR upDir = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
         
   worldToView = XMMatrixLookAtLH(eyePos, focusPos, upDir);
   
   worldBounds.set(BVec3(worldMin.x, worldMin.y, worldMin.z), BVec3(worldMax.x, worldMax.y, worldMax.z));
   
   AABB viewBounds(AABB::eInitExpand);
   for (int i = 0; i < worldBounds.numCorners(); i++)
   {
      BVec3 corner(worldBounds.corner(i));
      
      XMVECTOR viewCorner = XMVector4Transform(XMVectorSet(corner[0], corner[1], corner[2], 1.0f), worldToView);
      
      viewBounds.expand((const BVec3&)viewCorner);
   }
   
   float minZ = viewBounds[0][2];
   float maxZ = viewBounds[1][2];
         
   viewToProj = XMMatrixOrthographicOffCenterLH(
      viewBounds[0][0], viewBounds[1][0], 
      viewBounds[0][1], viewBounds[1][1], 
      minZ, maxZ);
      
   worldMinY = eyePos.y + -minZ;
   worldMaxY = eyePos.y + -maxZ;
}

//==============================================================================
// BTerrainHeightField::renderInit
//==============================================================================
void BTerrainHeightField::renderInit(uint width, uint height, IDirect3DSurface9* pDepthStencilSurf, bool furthestDepth, XMMATRIX worldToView, XMMATRIX viewToProj)
{
   gRenderDraw.beginScene();
   gRenderDraw.setDefaultRenderStates();
   gRenderDraw.setDefaultSamplerStates();
   
   BMatrixTracker&  matrixTracker   = gRenderDraw.getWorkerActiveMatrixTracker();
   BRenderViewport& renderViewport  = gRenderDraw.getWorkerActiveRenderViewport();
   //BVolumeCuller&   volumeCuller    = gRenderDraw.getWorkerActiveVolumeCuller();
           
   matrixTracker.setMatrix(cMTWorldToView, worldToView);
   matrixTracker.setMatrix(cMTViewToProj, viewToProj);

   D3DVIEWPORT9 viewport;
   viewport.X = 0;
   viewport.Y = 0;
   viewport.Width = width;
   viewport.Height = height;
   viewport.MinZ = 0.0f;
   viewport.MaxZ = 1.0f;

   matrixTracker.setViewport(viewport);

   renderViewport.setSurf(0, NULL);
   renderViewport.setDepthStencilSurf(pDepthStencilSurf);
   renderViewport.setViewport(viewport);
   
   gRenderDraw.setWorkerActiveMatrixTracker(matrixTracker);
   gRenderDraw.setWorkerActiveRenderViewport(renderViewport);

   gRenderDraw.clear(D3DCLEAR_STENCIL|D3DCLEAR_ZBUFFER, 0, furthestDepth ? 0.0f : 1.0f, 0);

   BD3D::mpDev->SetRenderState(D3DRS_COLORWRITEENABLE, 0);
   BD3D::mpDev->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
   
   BD3D::mpDev->SetRenderState(D3DRS_ZFUNC, furthestDepth ?  D3DCMP_GREATEREQUAL : D3DCMP_LESSEQUAL);
   
   BD3D::mpDev->SetRenderState(D3DRS_HALFPIXELOFFSET, TRUE);
}

//==============================================================================
// BTerrainHeightField::renderDeinit
//==============================================================================
void BTerrainHeightField::renderDeinit(IDirect3DTexture9* pDepthStencilTex)
{
   BD3D::mpDev->SetRenderState(D3DRS_HALFPIXELOFFSET, FALSE);
   
   BD3D::mpDev->Resolve(D3DRESOLVE_DEPTHSTENCIL|D3DRESOLVE_ALLFRAGMENTS, NULL, pDepthStencilTex, NULL, 0, 0, NULL, 1.0f, 0, NULL);
   
   gRenderDraw.resetWorkerActiveMatricesAndViewport();

   BD3D::mpDev->SetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_ALL);
   BD3D::mpDev->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
   
   gRenderDraw.endScene();   
}   

//==============================================================================
// BTerrainHeightField::renderTerrain
//==============================================================================
void BTerrainHeightField::renderTerrain(void)
{
   const bool origLODEnabled = gTerrainVisual.isLODEnabled();
   gTerrainVisual.setLODEnabled(false);
   
   gTerrain.evalSceneNodesLODImmediate(
                              BTerrain::cRPLocalShadowBuffer, 
                              gRenderDraw.getWorkerActiveMatrixTracker().getWorldCamPosVec4(), 
                              gRenderDraw.getWorkerActiveVolumeCuller(),
                              true);

   gTerrain.renderBegin(cTRP_ShadowGen);   

   gTerrain.renderCustomNoTile(
                            cTRP_ShadowGen, 
                            BTerrain::cRPLocalShadowBuffer, 
                            &gRenderDraw.getWorkerActiveMatrixTracker().getWorldCamPosVec4(), 
                            &gRenderDraw.getWorkerActiveVolumeCuller(), 
                            true,
                            false);

   gTerrain.renderEnd(cTRP_ShadowGen);

   gTerrainVisual.setLODEnabled(origLODEnabled);
}

//==============================================================================
// BTerrainHeightField::generateDepthBuffer
//==============================================================================
IDirect3DTexture9* BTerrainHeightField::generateDepthBuffer(uint width, uint height, bool furthestDepth, XMMATRIX worldToView, XMMATRIX viewToProj)
{
   HRESULT hres;
   
   D3DSURFACE_PARAMETERS surfParams;
   Utils::ClearObj(surfParams);
   
   IDirect3DTexture9* pDepthStencilTex;
   hres = gGPUFrameHeap.createTexture(width, height, 1, 0, D3DFMT_D24S8, 0, &pDepthStencilTex, NULL);
   BVERIFY(SUCCEEDED(hres));
   
   IDirect3DSurface9* pDepthStencilSurf;
   
   hres = gGPUFrameHeap.createRenderTarget(width, height, D3DFMT_D24S8, D3DMULTISAMPLE_NONE, 0, 0, &pDepthStencilSurf, &surfParams);
   BVERIFY(SUCCEEDED(hres));
   
   renderInit(width, height, pDepthStencilSurf, furthestDepth, worldToView, viewToProj);
      
   renderTerrain();
   
   renderDeinit(pDepthStencilTex);
      
   gGPUFrameHeap.releaseD3DResource(pDepthStencilSurf);
         
   return pDepthStencilTex;
}

//==============================================================================
// BTerrainHeightField::fillHeightFieldTexture
//==============================================================================
void BTerrainHeightField::fillHeightFieldTexture(
   uint width, uint height, 
   IDirect3DTexture9* pTex, 
   IDirect3DTexture9* pLowDepths, 
   IDirect3DTexture9* pHighDepths, 
   uint& minDepth, uint& maxDepth,
   double& minDepthF, double& maxDepthF)
{
   gRenderThread.blockUntilGPUIdle();
   
   const DWORD* pSrcLow = static_cast<const DWORD*>(gRenderDraw.getResourceAddress(pLowDepths, true));
   const DWORD* pSrcHigh = static_cast<const DWORD*>(gRenderDraw.getResourceAddress(pHighDepths, true));

   const bool firstPass = (minDepth == UINT_MAX);               
   
   if (minDepth == UINT_MAX)
   {
      minDepth = UINT_MAX;
      maxDepth = 0;
      
      for (uint y = 0; y < height; y++)
      {
         for (uint x = 0; x < width; x++)
         {
            uint ofs = XGAddress2DTiledOffset(x, y, width, sizeof(DWORD));
            uint l = *(pSrcLow + ofs); l >>= 8;
            uint h = *(pSrcHigh + ofs); h >>= 8;
            
            if ((l != 0xFFFFFF) && (l != 0))
            {
               if (l < minDepth) minDepth = l;
               if (l > maxDepth) maxDepth = l;
            }
            
            if ((h != 0xFFFFFF) && (h != 0))
            {
               if (h < minDepth) minDepth = h;
               if (h > maxDepth) maxDepth = h;
            }
         }
      }
      
      if (minDepth == UINT_MAX)
      {
         minDepth = 0;
         maxDepth = 1;
      }
      
      if (minDepth == maxDepth)
      {
         if (minDepth == 0)
            maxDepth++;
         else
            minDepth--;
      }
      
      minDepthF = minDepth / double(0xFFFFFF);
      maxDepthF = maxDepth / double(0xFFFFFF);
   }      
                  
   const uint64 depthDelta = Math::Max(maxDepth - minDepth, 1U);
         
   D3DLOCKED_RECT dstRect;
   pTex->LockRect(0, &dstRect, NULL, 0);
   
//-- FIXING PREFIX BUG ID 6937
   const DWORD* pDstReadable = static_cast<const DWORD*>(gRenderDraw.getResourceAddress(pTex, true));
//--
   
#ifdef DUMP_HEIGHTFIELD_IMAGE      
   BRGBAImage grayImage;
   if (firstPass)
      grayImage.setSize(width, height);
#endif   
   
   DWORD* pDst = static_cast<DWORD*>(dstRect.pBits);
   
   BDynamicArray2D<ushort> lowHeights(width, height);
   BDynamicArray2D<ushort> highHeights(width, height);
   
   for (uint y = 0; y < height; y++)
   {
      for (uint x = 0; x < width; x++)
      {
         uint ofs = XGAddress2DTiledOffset(x, y, width, sizeof(DWORD));
         
         uint d = 0;
         
         uint64 l = pSrcLow[ofs]; l >>= 8;
         if ((l != 0xFFFFFF) && (l != 0))
         {
            if (l < minDepth) l = minDepth; else if (l > maxDepth) l = maxDepth;
            d = (uint)(((l - minDepth) * (uint64)65535 + (depthDelta >> 1)) / depthDelta);
         }
         
         uint64 h = pSrcHigh[ofs]; h >>= 8;
         if ((h != 0xFFFFFF) && (h != 0))
         {
            if (h < minDepth) h = minDepth; else if (h > maxDepth) h = maxDepth;
            d |= ((((h - minDepth) * (uint64)65535 + (depthDelta >> 1)) / depthDelta) << 16);
         }
         
         if (firstPass)
         {
            pDst[ofs] = d;
            
            lowHeights(x, y) = static_cast<ushort>(d);
            highHeights(x, y) = static_cast<ushort>(d >> 16);
         }
         else
         {
            DWORD c = pDstReadable[ofs];
            
            DWORD nL = d & 0xFFFF;
            DWORD cL = c & 0xFFFF;
                                    
            DWORD nH = d >> 16;
            DWORD cH = c >> 16;
            
            cL = Math::Min(cL, nL);
            cH = Math::Min(cH, nH);
            
            lowHeights(x, y) = static_cast<ushort>(cL);
            highHeights(x, y) = static_cast<ushort>(cH);
            
            pDst[ofs] = cL | (cH << 16);                        
         }
         
#ifdef DUMP_HEIGHTFIELD_IMAGE            
         if (firstPass)
            grayImage.setPixel(x, y, BRGBAColor(255 - ((d >> 8) & 0xFF), 255 - ((d >> 24) & 0xFF), 0, 255));
#endif         
      }
   }
   
   for (uint y = 0; y < height; y++)
   {
      for (uint x = 0; x < width; x++)
      {
         uint ofs = XGAddress2DTiledOffset(x, y, width, sizeof(DWORD));
         
         DWORD c = pDstReadable[ofs];
         
         uint l = c & 0xFFFF;
         uint h = c >> 16;
         
         bool modified = false;
         
         if (l == 0)
         {
            float ave = 0.0f;
            uint num = 0;
            
            for (int xd = -2; xd < 2; xd++)
            {
               int yd;
               for (yd = -2; yd < 2; yd++)
               {
                  int xx = Math::Clamp<int>(xd + x, 0, width - 1);
                  int yy = Math::Clamp<int>(yd + y, 0, height - 1);
                  
                  uint p = lowHeights(xx, yy);
                  if (p)
                  {
                     ave += p;
                     num++;
                  }
               }
            }
            
            if (num)
            {
               l = Math::Clamp<uint>(Math::FloatToIntRound(ave / num), 0, 65535U);
               modified = true;
            }
         }
         
         if (h == 0)
         {
            float ave = 0.0f;
            uint num = 0;
            
            for (int xd = -2; xd < 2; xd++)
            {
               int yd;
               for (yd = -2; yd < 2; yd++)
               {
                  int xx = Math::Clamp<int>(xd + x, 0, width - 1);
                  int yy = Math::Clamp<int>(yd + y, 0, height - 1);

                  uint p = highHeights(xx, yy);
                  if (p)
                  {
                     ave += p;
                     num++;
                  }
               }
            }
            
            if (num)
            {
               h = Math::Clamp<uint>(Math::FloatToIntRound(ave / num), 0, 65535U);
               modified = true;
            }
         }
         
         if (modified)
         {
            pDst[ofs] = l | (h << 16);                          
         }
      }
   }      
    
#ifdef DUMP_HEIGHTFIELD_IMAGE   
   if (firstPass)
   {
      BFileSystemStream imageStream;
      imageStream.open(0, "height.tga", cSFWritable | cSFSeekable | cSFEnableBuffering);
      BImageUtils::writeTGA(imageStream, grayImage, cTGAImageTypeBGR);
      imageStream.close();
   }      
#endif   
      
   pTex->UnlockRect(0);
}

//==============================================================================
// BTerrainHeightField::computeHeightField
//==============================================================================
void BTerrainHeightField::computeHeightField(uint width, uint height)
{
   ASSERT_THREAD(cThreadIndexRender);
      
   SCOPEDSAMPLE(BTerrainHeightField_computeHeightField)

   releaseHeightField();   //CLM temp, incase we've created data from an XTH load
            
   if (!mpHeightField)
   {
      HRESULT hres = gRenderDraw.createTexture(width, height, 1, 0, D3DFMT_G16R16, 0, &mpHeightField, NULL);
      BVERIFY(SUCCEEDED(hres));
   }
         
   XMMATRIX worldToView;
   XMMATRIX viewToProj;
   AABB worldBounds;
   double worldMinY, worldMaxY;
   computeTransforms(worldToView, viewToProj, worldBounds, worldMinY, worldMaxY);
   
   Random rand;
   
   double minDepth = Math::fNearlyInfinite, maxDepth = Math::fNearlyInfinite;             
   uint minDepthI = UINT_MAX, maxDepthI = 0;
   
   const uint cNumSamples = 32;
   for (uint sampleIndex = 0; sampleIndex < cNumSamples; sampleIndex++)
   {
      //float xOfs = sampleIndex ? rand.fRand(-.7f, .7f) : 0.0f;
      //float yOfs = sampleIndex ? rand.fRand(-.7f, .7f) : 0.0f;
      
      float xOfs = sampleIndex ? rand.fRand(-1.0f, 1.0f) : 0.0f;
      float yOfs = sampleIndex ? rand.fRand(-1.0f, 1.0f) : 0.0f;
      
      XMMATRIX projOfs = XMMatrixTranslation(xOfs / width, yOfs / height, 0.0f);
   
      IDirect3DTexture9* pHighDepths = generateDepthBuffer(width, height, false, worldToView, viewToProj * projOfs);
      IDirect3DTexture9* pLowDepths = generateDepthBuffer(width, height, true, worldToView, viewToProj * projOfs);
      
      fillHeightFieldTexture(width, height, mpHeightField, pLowDepths, pHighDepths, minDepthI, maxDepthI, minDepth, maxDepth);
   
      gGPUFrameHeap.releaseD3DResource(pHighDepths);
      gGPUFrameHeap.releaseD3DResource(pLowDepths);
   }
   
   viewToProj = viewToProj * XMMatrixTranslation(0.0f, 0.0f, float(-minDepth)) * XMMatrixScaling(1.0f, 1.0f, float(1.0f / (maxDepth - minDepth)));
   double newWorldMinY = worldMinY + (worldMaxY - worldMinY) * minDepth;
   worldMaxY = worldMinY + (worldMaxY - worldMinY) * maxDepth;
   worldMinY = newWorldMinY;
   minDepth = 0.0f;
   maxDepth = 1.0f;
   
   BMatrixTracker matrixTracker;
   matrixTracker.setMatrix(cMTWorldToView, worldToView);
   matrixTracker.setMatrix(cMTViewToProj, viewToProj);
   matrixTracker.setViewport(0, 0, width, height);

   mHeightFieldAttributes.mWidth = width;
   mHeightFieldAttributes.mHeight = height;
   mHeightFieldAttributes.mBounds = worldBounds;
   
   mHeightFieldAttributes.mWorldMinY = worldMinY;
   mHeightFieldAttributes.mWorldMaxY = worldMaxY;
   mHeightFieldAttributes.mWorldRangeY = worldMaxY - worldMinY;

   mHeightFieldAttributes.mNormZToWorld = matrixTracker.getMatrix(cMTScreenToView) * matrixTracker.getMatrix(cMTViewToWorld);
         
   mHeightFieldAttributes.mWorldToNormZ = worldToView * viewToProj * matrixTracker.getMatrix(cMTProjToScreen);
      
   //XMVECTOR det;
   //XMMATRIX normZToWorld = XMMatrixInverse(&det, mHeightFieldAttributes.mWorldToNormZ);
      
   mHeightFieldAttributes.mpTexels = static_cast<const DWORD*>(gRenderDraw.getResourceAddress(mpHeightField, true));
   BVERIFY(mHeightFieldAttributes.mpTexels);
}

//==============================================================================
// BTerrainHeightField::releaseHeightField
//==============================================================================
void BTerrainHeightField::releaseHeightField(void)
{
   ASSERT_THREAD(cThreadIndexRender);
   
   mHeightFieldAttributes.mpTexels = NULL;

   if(mHeightFieldAttributes.mpPhysicalMemoryPointer)
   {
      if(mpHeightField)
      {
         delete mpHeightField;
         mpHeightField=NULL;
      }
      XPhysicalFree(mHeightFieldAttributes.mpPhysicalMemoryPointer);
      mHeightFieldAttributes.mpPhysicalMemoryPointer=NULL;
   }
   else  //CLM TEMP UNTIL EVERY MAP HAS EXPORTED HEIGHT DATA
   {
      if (mpHeightField)
      {
         mpHeightField->Release();
         mpHeightField = NULL;
      }
   }

   if(mHeightFieldAttributes.mpAlphaPhysicalMemoryPointer)
   {
      if(mpHeightFieldAlpha)
      {
         delete mpHeightFieldAlpha;
         mpHeightFieldAlpha=NULL;
      }
      XPhysicalFree(mHeightFieldAttributes.mpAlphaPhysicalMemoryPointer);
      mHeightFieldAttributes.mpAlphaPhysicalMemoryPointer=NULL;
   }
   else  //CLM TEMP UNTIL EVERY MAP HAS EXPORTED HEIGHT DATA
   {
      if(mpHeightFieldAlpha)
      {
         mpHeightFieldAlpha->Release();
         mpHeightFieldAlpha=NULL;
      }
   }
}

//==============================================================================
// BTerrainHeightField::render
//==============================================================================
void BTerrainHeightField::render(XMMATRIX worldMatrix, uint tessLevel)
{
   ASSERT_THREAD(cThreadIndexRender);
}

//==============================================================================
// BTerrainHeightField::initDeviceData
//==============================================================================
void BTerrainHeightField::initDeviceData(void)
{
   if (!mpEffectLoader)
   {
      mpEffectLoader = ALIGNED_NEW(BFXLEffectFileLoader, gRenderHeap);
      const bool status = mpEffectLoader->init(gRender.getEffectCompilerDefaultDirID(), SHADER_FILENAME, true, false, true);
      BVERIFY(status);
   }
   
   if (!mpPatchInstanceDecl)
   {
      BVertexTypes::BVertexDeclHelper vertexDeclHelper;
                        
      vertexDeclHelper.addSimple(D3DDECLTYPE_FLOAT3,     D3DDECLUSAGE_POSITION, 0);
      vertexDeclHelper.addSimple(D3DDECLTYPE_FLOAT16_4,  D3DDECLUSAGE_TEXCOORD, 0);
      vertexDeclHelper.addSimple(D3DDECLTYPE_FLOAT16_4,  D3DDECLUSAGE_TEXCOORD, 1);
      vertexDeclHelper.addSimple(D3DDECLTYPE_FLOAT16_4,  D3DDECLUSAGE_TEXCOORD, 2);
      vertexDeclHelper.addSimple(D3DDECLTYPE_D3DCOLOR,   D3DDECLUSAGE_COLOR,    0);
      
      vertexDeclHelper.createVertexDeclaration(&mpPatchInstanceDecl);
      BVERIFY(mpPatchInstanceDecl);
   }
}

//==============================================================================
// BTerrainHeightField::frameBegin
//==============================================================================
void BTerrainHeightField::frameBegin(void)
{

}

//==============================================================================
// BTerrainHeightField::processCommand
//==============================================================================
void BTerrainHeightField::processCommand(const BRenderCommandHeader& header, const uchar *pData)
{

}

//==============================================================================
// BTerrainHeightField::frameEnd
//==============================================================================
void BTerrainHeightField::frameEnd(void)
{

}

//==============================================================================
// BTerrainHeightField::deinitDeviceData
//==============================================================================
void BTerrainHeightField::deinitDeviceData(void)
{
   releaseHeightField();
   
   if (mpEffectLoader)
   {
      ALIGNED_DELETE(mpEffectLoader, gRenderHeap);
      mpEffectLoader = NULL;
   }
   
   if (mpPatchInstanceDecl)
   {
      mpPatchInstanceDecl->Release();
      mpPatchInstanceDecl = NULL;
   }
}
//==============================================================================
// BTerrainHeightField::unload
//==============================================================================
void BTerrainHeightField::unload(void)
{
    ASSERT_THREAD(cThreadIndexRender);
   releaseHeightField();
}
//==============================================================================
// BTerrainHeightField::computeWorldY
//==============================================================================
float BTerrainHeightField::computeWorldY(float normZ) const
{
   //return mHeightFieldAttributes.mNormZToWorld._32 * normZ + mHeightFieldAttributes.mNormZToWorld._42;
   return float(normZ * mHeightFieldAttributes.mWorldRangeY + mHeightFieldAttributes.mWorldMinY);
}

//==============================================================================
// BTerrainHeightField::computeHeightfieldNormZ
//==============================================================================
float BTerrainHeightField::computeHeightfieldNormZ(float worldY) const
{
   return float(((worldY -  mHeightFieldAttributes.mWorldMinY) /  mHeightFieldAttributes.mWorldRangeY));
}

//==============================================================================
// BTerrainHeightField::sampleHeights
//==============================================================================
bool BTerrainHeightField::sampleHeights(int x, int y, float& lowNormZ, float& highNormZ) const
{
   if (!mHeightFieldAttributes.mpTexels)
   {
      lowNormZ = 0.0f;
      highNormZ = 0.0f;
      return false;
   }

   x = Math::Clamp<uint>(x,0,mHeightFieldAttributes.mWidth - 1);
   y = Math::Clamp<uint>(y,0,mHeightFieldAttributes.mHeight - 1);
   
   const float rcpShort = 1.0f/65535.0f;

   const uint ofs = XGAddress2DTiledOffset(x, y, mHeightFieldAttributes.mWidth, sizeof(DWORD));
   const uint d = mHeightFieldAttributes.mpTexels[ofs];

   const float l = (float)(d & 0xFFFF);
   const float h = (float)(d >> 16);

   lowNormZ =  l * rcpShort;
   highNormZ = h * rcpShort;
   
   return true;
}

//==============================================================================
// BTerrainHeightField::sampleHeights
//==============================================================================
bool BTerrainHeightField::sampleHeights(float x, float y, float& lowNormZ, float& highNormZ) const
{
   if (!mHeightFieldAttributes.mpTexels)
   {
      lowNormZ = 0.0f;
      highNormZ = 0.0f;
      return false;
   }
   
   // This places the texel centers at (.5, .5)
   x -= .5f;
   y -= .5f;
   
   int x0 = (int)floor(x);
   int y0 = (int)floor(y);
   int x1 = x0 + 1;
   int y1 = y0 + 1;

   float a = x - x0;
   float b = y - y0;
         
   float l0, h0;
   sampleHeights(x0, y0, l0, h0);
   float m0 = (1.0f - a) * (1.0f - b);
   l0 *= m0;
   h0 *= m0;
   
   float l1, h1;
   sampleHeights(x1, y0, l1, h1);
   float m1 = a * (1.0f - b);
   l1 *= m1;
   h1 *= m1;
   
   float l2, h2;
   sampleHeights(x1, y1, l2, h2);
   float m2 = a * b;
   l2 *= m2;
   h2 *= m2;
   
   float l3, h3;
   sampleHeights(x0, y1, l3, h3);
   float m3 = (1.0f - a) * b;
   l3 *= m3;
   h3 *= m3;
   
   lowNormZ = l0 + l1 + l2 + l3;
   highNormZ = h0 + h1 + h2 + h3;

   return true;
}

//==============================================================================
// BTerrainHeightField::computeMapCoords
//==============================================================================
XMVECTOR BTerrainHeightField::computeMapCoords(XMVECTOR worldPos) const
{
   return XMVector4Transform(worldPos, mHeightFieldAttributes.mWorldToNormZ);
}

//==============================================================================
// BTerrainHeightField::computeMapCoords
//==============================================================================
void BTerrainHeightField::computeMapCoords(XMVECTOR worldPos, int& x, int& y, float& normZ, bool clamp) const
{
   XMVECTOR p = XMVector4Transform(worldPos, mHeightFieldAttributes.mWorldToNormZ);
   
   // Continuous to discrete
   x = (int)floor(p.x);
   y = (int)floor(p.y);
   normZ = p.z;
   
   if (clamp)
   {
      if (x < 0) 
         x = 0;
      else if (x >= (int)mHeightFieldAttributes.mWidth)
         x = mHeightFieldAttributes.mWidth - 1;
         
      if (y < 0) 
         y = 0;
      else if (y >= (int)mHeightFieldAttributes.mHeight)
         y = mHeightFieldAttributes.mHeight - 1;
   }   
}

//==============================================================================
// BTerrainHeightField::computeWorldCoords
//==============================================================================
XMVECTOR BTerrainHeightField::computeWorldCoords(int x, int y, float normZ) const
{
   XMVECTOR w = XMVectorSet(x + .5f, y + .5f, normZ, 1.0f);
   
   return XMVector4Transform(w, mHeightFieldAttributes.mNormZToWorld);
}

//==============================================================================
// BTerrainHeightField::castRay
//==============================================================================
bool BTerrainHeightField::castRay(XMVECTOR worldPos, float& lowNormZ, float& highNormZ) const
{
   XMVECTOR mapPos = computeMapCoords(worldPos);
   
   return sampleHeights(mapPos.x, mapPos.y, lowNormZ, highNormZ);
}

//==============================================================================
// BTerrainHeightField::renderDebugGrid
//==============================================================================
void BTerrainHeightField::renderDebugGrid(void)
{
   XMVECTOR camPos = gRenderDraw.getWorkerSceneMatrixTracker().getWorldCamPos();

   int cx, cy;
   float cz;
   gTerrainHeightField.computeMapCoords(camPos, cx, cy, cz, true);

   for (uint iy = 0; iy < 32; iy++)
   {
      for (uint ix = 0; ix < 32; ix++)
      {
         XMVECTOR w = gTerrainHeightField.computeWorldCoords(ix - 16 + cx, iy - 16 + cy, cz);

         float lowHeight, highHeight;
         gTerrainHeightField.castRay(w, lowHeight, highHeight);

         XMVECTOR l = XMVectorSet(w.x, gTerrainHeightField.computeWorldY(lowHeight), w.z, 1.0f);
         XMVECTOR h = XMVectorSet(w.x, gTerrainHeightField.computeWorldY(highHeight), w.z, 1.0f);

         gpDebugPrimitives->addDebugAxis(l, cXAxisVector, cYAxisVector, cZAxisVector, 2.0f);
         gpDebugPrimitives->addDebugAxis(h, cXAxisVector, cYAxisVector, cZAxisVector, 2.0f);
      }
   }
}

//============================================================================
// BTerrainHeightField::setBlackmapShaderParms
//============================================================================
void BTerrainHeightField::setBlackmapShaderParms(const BBlackmapParams& params)
{
   if (!tickEffect())
      return;
   
   if (!params.mpTexture)
      mBlackmapEnabled = false;
   else
   {
      mBlackmapEnabled = true;
      mBlackmapSampler = params.mpTexture;
      mBlackmapUnexploredSampler = params.mpUnexploredTexture;
      BCOMPILETIMEASSERT(sizeof(params.mParams)/sizeof(gTerrainRender.getBlackmapParams().mParams[0]) == 3);
      mBlackmapParams0 = params.mParams[0];
      mBlackmapParams1 = params.mParams[1];
      mBlackmapParams2 = params.mParams[2];
   }
}

//============================================================================
// BTerrainHeightField::setLightBufferingParams
//============================================================================
void BTerrainHeightField::setLightBufferingParams(void)
{
   if (!tickEffect())
      return;
      
   const bool lightBufferingEnabled = (gVisibleLightManager.getLightBuffer(BVisibleLightManager::cLightBufferColorTexture) != NULL);
   mLightBufferingEnabled = lightBufferingEnabled;
   if (lightBufferingEnabled)
   {
      mLightBufferColorSampler = gVisibleLightManager.getLightBuffer(BVisibleLightManager::cLightBufferColorTexture);
      mLightBufferVectorSampler = gVisibleLightManager.getLightBuffer(BVisibleLightManager::cLightBufferVectorTexture);
      
      const XMMATRIX& worldToLightBuffer = gVisibleLightManager.getWorldToLightBuffer();

      BVec4 cols[3];
      cols[0].set(worldToLightBuffer.m[0][0], worldToLightBuffer.m[1][0], worldToLightBuffer.m[2][0], worldToLightBuffer.m[3][0]);
      cols[1].set(worldToLightBuffer.m[0][1], worldToLightBuffer.m[1][1], worldToLightBuffer.m[2][1], worldToLightBuffer.m[3][1]);
      cols[2].set(worldToLightBuffer.m[0][2], worldToLightBuffer.m[1][2], worldToLightBuffer.m[2][2], worldToLightBuffer.m[3][2]);

      mWorldToLightBufCols.setArray(cols, 3);
   }
   else
   {
      mLightBufferColorSampler = NULL;
      mLightBufferVectorSampler = NULL;
   }
}

//============================================================================
// BTerrainHeightField::initEffectParams
//============================================================================
void BTerrainHeightField::initEffectParams(void)
{
   if (!mpEffectLoader)
      return;
      
   BFXLEffect& effect = mpEffectLoader->getFXLEffect();

   mRenderPatchesTechnique = effect.getTechnique("RenderPatches");
   
   mHeightfieldSamplerParam = effect("gHeightfieldSampler");
   mHeightfieldSizesParam = effect("gHeightfieldSampleParams");
   mWorldToHeightfieldParam = effect("gWorldToHeightfield");
   mHeightfieldYScaleOfsParam = effect("gHeightfieldYScaleOfs");
   mConformToTerrainFlagParam = effect("gConformToTerrainFlag");
   mTerrainAlphaSamplerParam = effect("gTerrainAlphaSampler");
   

   //BLACKMAP
   mBlackmapEnabled                        = effect("gBlackmapEnabled");
   mBlackmapSampler                        = effect("gBlackmapSampler");
   mBlackmapUnexploredSampler              = effect("gBlackmapUnexploredSampler");
   mBlackmapParams0                        = effect("gBlackmapParams0");
   mBlackmapParams1                        = effect("gBlackmapParams1");
   mBlackmapParams2                        = effect("gBlackmapParams2");

   mLightBufferColorSampler                = effect("gLightBufferColorSampler");
   mLightBufferVectorSampler               = effect("gLightBufferVectorSampler");
   mLightBufferingEnabled                  = effect("gLightBufferingEnabled");
   mWorldToLightBufCols                    = effect("gWorldToLightBufCols");
      
   effect("gVisControl0").setRegisterUpdateMode(true);
   effect("gVisControl1").setRegisterUpdateMode(true);
   effect("gVisControl2").setRegisterUpdateMode(true);
   effect("gVisControl3").setRegisterUpdateMode(true);
   
   effect("gLightData").setRegisterUpdateMode(true);
   effect("gNumLights").setRegisterUpdateMode(true);
   effect("gExtendedLocalLightingEnabled").setRegisterUpdateMode(true);
   effect("gNumExtendedLights").setRegisterUpdateMode(true);
   effect("gExtendedLocalLightingParams").setRegisterUpdateMode(true);
   effect("gLocalLightingEnabled").setRegisterUpdateMode(true);
   effect("gLocalShadowingEnabled").setRegisterUpdateMode(true);
}

//============================================================================
// BTerrainHeightField::tickEffect
//============================================================================
bool BTerrainHeightField::tickEffect(void)
{
   if (!mpEffectLoader)
      return false;
     
   if (mpEffectLoader->tick(true))
      initEffectParams();
   
   if (!mpEffectLoader->isEffectValid())
      return false;
   
   mpEffectLoader->getFXLEffect().updateIntrinsicParams();
         
   return true;
}


//==============================================================================
// BTerrainHeightField::renderHeightfieldForOcclusion
//==============================================================================
void BTerrainHeightField::renderHeightfieldForOcclusion(void)
{
   if (!mpHeightField)
      return;

   if (!tickEffect())
      return;

   BFXLEffect& effect = mpEffectLoader->getFXLEffect();

   BFXLEffectTechnique technique(effect.getTechnique("RenderHeightFieldForOcclusion"));
   if (!technique.getValid())
      return;

   // x - 1.0/numTrisPerRow
   // y - numTrisPerRow
   // z - 1.0/width
   // w - 1.0/height

   BVec4 sampleParams(1.0f / (mHeightFieldAttributes.mWidth * 2.0f), mHeightFieldAttributes.mWidth * 2.0f, 1.0f / mHeightFieldAttributes.mWidth,1.0f / mHeightFieldAttributes.mHeight);

   static float yBias = 0.25f;

   XMMATRIX heightfieldToProj = 
      XMMatrixTranslation(.5f, .5f, 0.0f) * 
      mHeightFieldAttributes.mNormZToWorld * 
      XMMatrixTranslation(0.0f, yBias, 0.0f) * 
      gRenderDraw.getWorkerActiveMatrixTracker().getMatrix(cMTWorldToProj);

   effect("gHeightfieldSampleParams")  = sampleParams;
   effect("gHeightfieldToProj")        = heightfieldToProj;
   effect("gHeightfieldSampler")       = mpHeightField;

   static float depthBias = -.00001f;
   BD3D::mpDev->SetRenderState(D3DRS_DEPTHBIAS, CAST(DWORD, depthBias));

   technique.beginRestoreDefaultState();
      technique.beginPass(0);
   technique.commit();

   gRenderDraw.clearStreamSource(0);
   gRenderDraw.clearStreamSource(1);

   BD3D::mpDev->SetIndices(NULL);

   // dummy decl
   BD3D::mpDev->SetVertexDeclaration(BTLVertex::msVertexDecl);

   BD3D::mpDev->DrawVertices(D3DPT_TRIANGLELIST, 0, (mHeightFieldAttributes.mWidth * 2 * mHeightFieldAttributes.mHeight) * 3);

   technique.endPass();
   technique.end();

   BD3D::mpDev->SetRenderState(D3DRS_DEPTHBIAS, 0);
   BD3D::mpDev->SetVertexShader(NULL);
   BD3D::mpDev->SetPixelShader(NULL);
    
}

//==============================================================================
// BTerrainHeightField::renderHeightField
//==============================================================================
bool BTerrainHeightField::renderDebugHeightField(void)
{
   if (!mpHeightField)
      return false;
      
   if (!tickEffect())
      return false;

   BFXLEffect& effect = mpEffectLoader->getFXLEffect();
   
   BFXLEffectTechnique technique(effect.getTechnique("RenderDebugHeightField"));
   if (!technique.getValid())
      return false;

   // x - 1.0/numTrisPerRow
   // y - numTrisPerRow
   // z - 1.0/width
   // w - 1.0/height

   BVec4 sampleParams(1.0f / (mHeightFieldAttributes.mWidth * 2.0f), mHeightFieldAttributes.mWidth * 2.0f, 1.0f / mHeightFieldAttributes.mWidth,1.0f / mHeightFieldAttributes.mHeight);
   
   static float yBias = 0.25f;
   
   XMMATRIX heightfieldToProj = 
      XMMatrixTranslation(.5f, .5f, 0.0f) * 
      mHeightFieldAttributes.mNormZToWorld * 
      XMMatrixTranslation(0.0f, yBias, 0.0f) * 
      gRenderDraw.getWorkerActiveMatrixTracker().getMatrix(cMTWorldToProj);
   
   effect("gHeightfieldSampleParams")  = sampleParams;
   effect("gHeightfieldToProj")        = heightfieldToProj;
   effect("gHeightfieldSampler")       = mpHeightField;
   
   static float depthBias = -.00001f;
   BD3D::mpDev->SetRenderState(D3DRS_DEPTHBIAS, CAST(DWORD, depthBias));
   
   technique.beginRestoreDefaultState();
   technique.beginPass(0);
   technique.commit();
   
   gRenderDraw.clearStreamSource(0);
   gRenderDraw.clearStreamSource(1);
   
   BD3D::mpDev->SetIndices(NULL);
   
   // dummy decl
   BD3D::mpDev->SetVertexDeclaration(BTLVertex::msVertexDecl);
      
   BD3D::mpDev->DrawVertices(D3DPT_TRIANGLELIST, 0, (mHeightFieldAttributes.mWidth * 2 * mHeightFieldAttributes.mHeight) * 3);
   
   technique.endPass();
   technique.end();
   
   BD3D::mpDev->SetRenderState(D3DRS_DEPTHBIAS, 0);
   BD3D::mpDev->SetVertexShader(NULL);
   BD3D::mpDev->SetPixelShader(NULL);
   
   return true;      
}

//==============================================================================
// BTerrainHeightField::renderPatches
//==============================================================================
bool BTerrainHeightField::renderPatches(
   const BPatchInstance* pInstances, 
   uint numInstances, 
   float tessLevel, 
   float yLowRange, float yHighRange,
   bool conformToTerrain, 
   eRenderPassIndex techniquePassIndex)
{
   BDEBUG_ASSERT(pInstances);
   
   if (!numInstances)
      return true;
      
   if (!tickEffect())
      return false;
   
   if (!mRenderPatchesTechnique.getValid())
      return false;
      
   const uint instanceArraySize = sizeof(BPatchInstance) * numInstances;
   
   void* pDst = gRenderDraw.lockDynamicVB(numInstances, sizeof(BPatchInstance));
   
   IDirect3DVertexBuffer9* pInstanceVB = gRenderDraw.getDynamicVB();
   
   memcpy(pDst, pInstances, instanceArraySize);
   
   gRenderDraw.unlockDynamicVB();
   
   BD3D::mpDev->SetStreamSource(0, pInstanceVB, 0, sizeof(BPatchInstance));
      
   BD3D::mpDev->SetIndices(NULL);
   
   BD3D::mpDev->SetVertexDeclaration(mpPatchInstanceDecl);
      
   BD3D::mpDev->SetRenderState(D3DRS_MAXTESSELLATIONLEVEL, CAST(DWORD, tessLevel));
   
   float minTessLevel = 1.0f;
   BD3D::mpDev->SetRenderState(D3DRS_MINTESSELLATIONLEVEL, CAST(DWORD, minTessLevel));
   
   BD3D::mpDev->SetRenderState(D3DRS_TESSELLATIONMODE, D3DTM_CONTINUOUS);
         
   mHeightfieldSamplerParam = mpHeightField;
   mWorldToHeightfieldParam = mHeightFieldAttributes.mWorldToNormZ;
   mHeightfieldYScaleOfsParam = BVec4((float)mHeightFieldAttributes.mWorldRangeY, (float)mHeightFieldAttributes.mWorldMinY, yLowRange, yHighRange);
   mConformToTerrainFlagParam = conformToTerrain;
   mTerrainAlphaSamplerParam = mpHeightFieldAlpha;
   mHeightfieldSizesParam = BVec4((float)gTerrainVisual.getNumXVerts(), 0 , 1.0f / mHeightFieldAttributes.mWidth,1.0f / mHeightFieldAttributes.mHeight);


            
   mRenderPatchesTechnique.beginRestoreDefaultState();
   mRenderPatchesTechnique.beginPass(techniquePassIndex);
   mRenderPatchesTechnique.commit();
   
   BD3D::mpDev->DrawTessellatedPrimitive(D3DTPT_QUADPATCH, 0, numInstances);
   
   mRenderPatchesTechnique.endPass();
   mRenderPatchesTechnique.end();
   
   gRenderDraw.clearStreamSource(0);
   BD3D::mpDev->SetVertexShader(NULL);
   BD3D::mpDev->SetPixelShader(NULL);
      
   mBlackmapSampler=NULL;
   return true;
}


//==============================================================================
// BTerrainHeightField::renderPatches
//==============================================================================
#include "TerrainRibbon.h"
bool BTerrainHeightField::renderRibbon(const D3DVertexBuffer* pVB,
                                       int vertMemSize,
                                       int numVerts,
                                       eRenderPassIndex techniquePassIndex)
{
   if (!tickEffect())
      return false;

   if (!mRenderPatchesTechnique.getValid())
      return false;


   BD3D::mpDev->SetStreamSource(0, const_cast<D3DVertexBuffer*>(pVB), 0, vertMemSize);
   BD3D::mpDev->SetIndices(NULL);
   //BD3D::mpDev->SetVertexDeclaration(mpPatchInstanceDecl);   //done by calling method




   mHeightfieldSamplerParam = mpHeightField;
   mWorldToHeightfieldParam = mHeightFieldAttributes.mWorldToNormZ;
   mHeightfieldYScaleOfsParam = BVec4((float)mHeightFieldAttributes.mWorldRangeY, (float)mHeightFieldAttributes.mWorldMinY, 10.0f, 10.0f);
   mHeightfieldSizesParam = BVec4((float)gTerrainVisual.getNumXVerts(), 0, 1.0f / mHeightFieldAttributes.mWidth,1.0f / mHeightFieldAttributes.mHeight);
   mConformToTerrainFlagParam = true;
   mTerrainAlphaSamplerParam = mpHeightFieldAlpha;

   mRenderPatchesTechnique.beginRestoreDefaultState();
   mRenderPatchesTechnique.beginPass(techniquePassIndex);
   mRenderPatchesTechnique.commit();

   BD3D::mpDev->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
   BD3D::mpDev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
   BD3D::mpDev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);

   BD3D::mpDev->DrawPrimitive(D3DPT_TRIANGLESTRIP , 0, numVerts-2);

   mRenderPatchesTechnique.endPass();
   mRenderPatchesTechnique.end();

   gRenderDraw.clearStreamSource(0);
   BD3D::mpDev->SetVertexShader(NULL);
   BD3D::mpDev->SetPixelShader(NULL);

   mBlackmapSampler=NULL;
   return true;
}

//==============================================================================
// BTerrainHeightField::loadECFXTH Enums
//==============================================================================
enum eXTHChunkID
{
   cXTH_Version  = 0x0001,

   cXTH_XTHHeader =  0x1111,
   cXTH_TerrainHeightfield = 0x2222,
   cXTH_TerrainHeightfieldAlpha = 0x3333,
};


//==============================================================================
// BTerrainHeightField::loadECFXTH
//==============================================================================
bool BTerrainHeightField::loadECFXTHTInternal(int dirID, const char* pFilenameNoExtension)
{
   SCOPEDSAMPLE(BTerrainHeightField_loadECFXTHTInternal)
   releaseHeightField();

   BString filenameXTH(pFilenameNoExtension);
   filenameXTH += ".xth";

   gConsoleOutput.resource("BTerrainHeightField::loadECFXTH: %s", filenameXTH.getPtr());

   BFileSystemStream tfile;
   if (!tfile.open(dirID, filenameXTH, cSFReadable | cSFSeekable | cSFDiscardOnClose))
   {
      gConsoleOutput.error("BTerrainHeightField::loadECFXTH : Unable to open file %s\n", filenameXTH.getPtr());
      return false;
   }

   BECFFileStream ecfReader;
   if (!ecfReader.open(&tfile)) //file checking and version ID done during constructor
   {
      gConsoleOutput.error("BTerrainHeightField::loadECFXTH : ECFHeader or Checksum invalid  %s\n", filenameXTH.getPtr());
      tfile.close();
      return false;
   }


   Utils::ClearObj(mHeightFieldAttributes);

   BECFHeader ecfHeader = ecfReader.getHeader();
   int numECFChunks = ecfHeader.getNumChunks();

   //find our PTHChunk header
   int headerIndex = ecfReader.findChunkByID(cXTH_XTHHeader);
   if (headerIndex==-1)
   { 
      gConsoleOutput.error("BTerrainHeightField:loadECFXTH : Could not find XTH Chunk Header\n");
      tfile.close();
      return false;
   }
   ecfReader.seekToChunk(headerIndex);

   //check our header data
   int version = 0;
   ecfReader.getStream()->readObj(version);
   if (version != cXTH_Version)
   {
      gConsoleOutput.error("BTerrainHeightField:loadECFXTH : Could not find XTH Chunk Header\n");
      tfile.close();
      return false;
   }

   //we're good. start walking through chunks and doing stuff...
   for(int i=0;i<numECFChunks;i++)
   {
      const BECFChunkHeader& cHeader = ecfReader.getChunkHeader(i);
      const uint64 id = cHeader.getID();
      switch(id)
      {
         case cXTH_TerrainHeightfield:
         {
            ecfReader.seekToChunk(i);

            ecfReader.getStream()->readObj(mHeightFieldAttributes.mWidth);
            ecfReader.getStream()->readObj(mHeightFieldAttributes.mHeight);
            
            float temp = 0;
            ecfReader.getStream()->readObj(temp); mHeightFieldAttributes.mWorldMinY=temp;
            ecfReader.getStream()->readObj(temp); mHeightFieldAttributes.mWorldMaxY=temp;
            ecfReader.getStream()->readObj(temp); mHeightFieldAttributes.mWorldRangeY=temp;

            ecfReader.getStream()->readObj(mHeightFieldAttributes.mBounds[0][0]);
            ecfReader.getStream()->readObj(mHeightFieldAttributes.mBounds[0][1]);
            ecfReader.getStream()->readObj(mHeightFieldAttributes.mBounds[0][2]);
            ecfReader.getStream()->readObj(mHeightFieldAttributes.mBounds[1][0]);
            ecfReader.getStream()->readObj(mHeightFieldAttributes.mBounds[1][1]);
            ecfReader.getStream()->readObj(mHeightFieldAttributes.mBounds[1][2]);
            
            ecfReader.getStream()->readBytes(&mHeightFieldAttributes.mNormZToWorld,sizeof(float)*16);
            ecfReader.getStream()->readBytes(&mHeightFieldAttributes.mWorldToNormZ,sizeof(float)*16);
    
            int numTexels = 0;
            ecfReader.getStream()->readObj(numTexels);
            int texelsMemSize = 0;
            ecfReader.getStream()->readObj(texelsMemSize);
            
            mHeightFieldAttributes.mpPhysicalMemoryPointer = XPhysicalAlloc( texelsMemSize, MAXULONG_PTR, 0, PAGE_READWRITE);
            BVERIFY(mHeightFieldAttributes.mpPhysicalMemoryPointer);
            if (!ecfReader.getStream()->readBytes(mHeightFieldAttributes.mpPhysicalMemoryPointer,texelsMemSize))
            {
               XPhysicalFree(mHeightFieldAttributes.mpPhysicalMemoryPointer);
               mHeightFieldAttributes.mpPhysicalMemoryPointer = NULL;

               gConsoleOutput.error("BTerrainHeightField:loadECFXTH : Could not read precomputed height data. Please re-export\n");

               tfile.close();
               return false;
            }

            mHeightFieldAttributes.mpTexels = static_cast<const DWORD*>(mHeightFieldAttributes.mpPhysicalMemoryPointer);
            BVERIFY(mHeightFieldAttributes.mpTexels);

            //create the texture
            mpHeightField = new IDirect3DTexture9();
            int imgSize = XGSetTextureHeader(mHeightFieldAttributes.mWidth,mHeightFieldAttributes.mHeight,1,0,D3DFMT_G16R16,0,0,0,0,mpHeightField,NULL,NULL);
            imgSize;
            XGOffsetResourceAddress( mpHeightField, mHeightFieldAttributes.mpPhysicalMemoryPointer ); 
            
            break;
         }

         case cXTH_TerrainHeightfieldAlpha:
         {
            ecfReader.seekToChunk(i);


            int alphaWidth = 256;
            int alphaHeight = 256;
            ecfReader.getStream()->readObj(alphaWidth);
            ecfReader.getStream()->readObj(alphaHeight);

            int texelsMemSize = 0;
            ecfReader.getStream()->readObj(texelsMemSize);

            mHeightFieldAttributes.mpAlphaPhysicalMemoryPointer = XPhysicalAlloc( texelsMemSize, MAXULONG_PTR, 0, PAGE_READWRITE);
            BVERIFY(mHeightFieldAttributes.mpAlphaPhysicalMemoryPointer);
            if (!ecfReader.getStream()->readBytes(mHeightFieldAttributes.mpAlphaPhysicalMemoryPointer,texelsMemSize))
            {
               XPhysicalFree(mHeightFieldAttributes.mpAlphaPhysicalMemoryPointer);
               mHeightFieldAttributes.mpAlphaPhysicalMemoryPointer = NULL;

               gConsoleOutput.error("BTerrainHeightField:loadECFXTH : Could not read precomputed height data. Please re-export\n");

               tfile.close();
               return false;
            }

            //create the texture
            mpHeightFieldAlpha = new IDirect3DTexture9();
            int imgSize = XGSetTextureHeader(alphaWidth,alphaHeight,1,0,D3DFMT_LIN_DXT3A_1111,0,0,0,0,mpHeightFieldAlpha,NULL,NULL);
            imgSize;
            XGOffsetResourceAddress( mpHeightFieldAlpha, mHeightFieldAttributes.mpAlphaPhysicalMemoryPointer ); 


            break;
         }
      }
   } 
 
   //failsafe
   if(!mHeightFieldAttributes.mpAlphaPhysicalMemoryPointer)
   {
      int alphaWidth = 512;//gTerrainVisual.getNumXVerts();
      int alphaHeight = 512;//gTerrainVisual.getNumXVerts();
      const uint PackedWidth = alphaWidth>>2;

      BD3D::mpDev->CreateTexture(PackedWidth,alphaHeight,1,0, D3DFMT_LIN_DXT3A_1111, 0,&mpHeightFieldAlpha,0);

      //numXBlocks-1 + numXBlocks * numYBlocks
      const uint sizeToSet = (((alphaWidth>>4)-1) + (alphaWidth>>4) * ((alphaHeight>>2)-1)) * sizeof(int64);

      D3DLOCKED_RECT rect;
      mpHeightFieldAlpha->LockRect(0,&rect,0,0);
      byte* pDat = (byte*)rect.pBits; 
      memset(pDat,0xFF,sizeToSet);
      mpHeightFieldAlpha->UnlockRect(0);
   }

   tfile.close();
   return true;
}

//==============================================================================
// BTerrainHeightField::flattenAreaInstant
//==============================================================================
void BTerrainHeightField::flattenAreaInstant(float mMinXPerc, float mMaxXPerc, float mMinZPerc, float mMaxZPerc, float desiredHeight)
{
   float heightfieldNormHeight = computeHeightfieldNormZ(desiredHeight);
   ushort shortHeight = (ushort)(heightfieldNormHeight * 65535);

   int minZ = (int)floor(mHeightFieldAttributes.mHeight * (1.0f-mMinZPerc));
   int maxZ = (int)ceil(mHeightFieldAttributes.mHeight * (1.0f-mMaxZPerc));
   int minX = (int)floor(mHeightFieldAttributes.mWidth * (mMinXPerc));
   int maxX = (int)ceil(mHeightFieldAttributes.mWidth * (mMaxXPerc));

   if(maxZ < minZ) std::swap(maxZ,minZ);
   if(maxX < minX) std::swap(maxX,minX);

   DWORD *texPtr = reinterpret_cast<DWORD*>(mHeightFieldAttributes.mpPhysicalMemoryPointer);

   for(int y=minZ;y<maxZ;y++)
   {
      for(int x=minX;x<maxX;x++)
      {
         uint ofs = XGAddress2DTiledOffset(x, y, mHeightFieldAttributes.mWidth, sizeof(DWORD));

         uint d = texPtr[ofs];

      //   float l = float(d & 0xFFFF);
      //   float h = float(d >> 16);
         d = (d & 0x0000FFFF) | (shortHeight<<16);

         texPtr[ofs] = d;
      }
   }

   int totalSize = mHeightFieldAttributes.mWidth * mHeightFieldAttributes.mHeight * sizeof(int);
   BD3D::mpDev->InvalidateGpuCache(mHeightFieldAttributes.mpPhysicalMemoryPointer,totalSize,0);
}