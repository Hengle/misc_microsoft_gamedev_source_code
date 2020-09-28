//============================================================================
//
// File: dirShadowManager.cpp
//  
// Copyright (c) 2006, Ensemble Studios
//
// rg [2/24/06] - Initial implementation
// 
//============================================================================
#include "xgamerender.h"
#include "dirShadowManager.h"

#include "math\generateCombinations.h"
#include "math\plane.h"
#include "math\VMXUtils.h"

#include "primDraw2D.h"
#include "renderDraw.h"
#include "sceneLightManager.h"
#include "effectIntrinsicManager.h"
#include "render.h"
#include "inputsystem.h"
#include "gpuHeap.h"
#include "effectFileLoader.h"

#include "terrainHeightField.h"

#include "debugPrimitives.h"

#define SHADER_FILENAME "dirShadowing\\dirShadowing.bin"

#define D3DFMT_R16 MAKED3DFMT(GPUTEXTUREFORMAT_16, GPUENDIAN_8IN16, TRUE, GPUSIGN_ALL_UNSIGNED, GPUNUMFORMAT_FRACTION, GPUSWIZZLE_OOOR)

#define GPUSWIZZLE_OOOG (GPUSWIZZLE_Y | GPUSWIZZLE_1<<3 | GPUSWIZZLE_1<<6 | GPUSWIZZLE_1<<9)
#define D3DFMT_G16 MAKED3DFMT(GPUTEXTUREFORMAT_16, GPUENDIAN_8IN16, TRUE, GPUSIGN_ALL_UNSIGNED, GPUNUMFORMAT_FRACTION, GPUSWIZZLE_OOOG)

// MIKEY - define custom texture/rendertarget/resolve formats for shadowmap rendering
#define GPUSWIZZLE_ZZZR (GPUSWIZZLE_X | GPUSWIZZLE_0<<3 | GPUSWIZZLE_0<<6 | GPUSWIZZLE_0<<9)
#define D3DFMT_SHADOW16        (D3DFORMAT)MAKED3DFMT(GPUTEXTUREFORMAT_16, GPUENDIAN_8IN16, TRUE, GPUSIGN_ALL_UNSIGNED, GPUNUMFORMAT_INTEGER, GPUSWIZZLE_ZZZR)
#define D3DFMT_SHADOW16_EDRAM  (D3DFORMAT)D3DFMT_G16R16_EDRAM
const INT32 SHADOW16_RENDERTARGET_BIAS =   5; // Convert render target writes from to [0,1] to [0,32] range
const INT32 SHADOW16_RESOLVE_BIAS      =  11; // Convert render target resolves from to [0,32] to [0,65535] range
const INT32 SHADOW16_SAMPLE_BIAS       = -16; // Convert texture format from [0,65535] to [0,1] range

//============================================================================
// Globals
//============================================================================
BDirShadowManager gDirShadowManager;

//============================================================================
// BDirShadowManager::BDirShadowManager
//============================================================================
BDirShadowManager::BDirShadowManager()
{
   clear();
}

//============================================================================
// BDirShadowManager::~BDirShadowManager
//============================================================================
BDirShadowManager::~BDirShadowManager()
{
}

//============================================================================
// BDirShadowManager::clear
//============================================================================
void BDirShadowManager::clear(void)
{
   for (uint i = 0; i < cMaxPasses; i++)
   {
      mWorldToView[i]      = XMMatrixIdentity();
      mViewToProj[i]       = XMMatrixIdentity();
      mWorldToProj[i]      = XMMatrixIdentity();
      mPrevWorldToView[i]  = XMMatrixIdentity();
      mPrevViewToProj[i]   = XMMatrixIdentity();
   }      
   
   mpRenderTarget = NULL;   
   mpDepthStencil = NULL;
   mpShadowBuffer = NULL;
   
   mpEffectLoader = NULL;

   mInShadowPass = false;
   mNumPasses = cMaxPasses;
   mFinalCSMPassIndex = cMaxPasses - 1;
   mWidth = 0;
   mHeight = 0;
   
   Utils::ClearObj(mViewMin);
   Utils::ClearObj(mViewMax);
   Utils::ClearObj(mMinZ);
   Utils::ClearObj(mMaxZ);
   
   Utils::ClearObj(mInclusionPlanes);
   mNumInclusionPlanes = 0;

#ifndef BUILD_FINAL
   mDebugCurPage = 0;   
   mDebugDisplay = false; 
#endif

   mPrevShadowMode = cSMFPS;
}

//============================================================================
// BDirShadowManager::init
//============================================================================
void BDirShadowManager::init(void)
{
   ASSERT_THREAD(cThreadIndexSim);
   
   if (mCommandHandle != cInvalidCommandListenerHandle)
      return;
   
   clear();
   
   commandListenerInit();
}

//============================================================================
// BDirShadowManager::deinit
//============================================================================
void BDirShadowManager::deinit(void)
{
   ASSERT_THREAD(cThreadIndexSim);
   
   if (mCommandHandle == cInvalidCommandListenerHandle)
      return;
   
   gRenderThread.blockUntilGPUIdle();
         
   mpShadowBuffer = NULL;
   
   gEffectIntrinsicManager.set(cIntrinsicDirShadowMapTexture, &mpShadowBuffer, cIntrinsicTypeTexturePtr);
                  
   commandListenerDeinit();
}

//============================================================================
// BDirShadowManager::updateTextureScaleMatrix
// (width, height) must be the resolution of the shadow buffer when rendering
// the 3D scene.
//============================================================================
void BDirShadowManager::updateTextureScaleMatrix(uint width, uint height)
{   
   D3DXMATRIX& matrix = CAST(D3DXMATRIX, mTextureScale);
   
   const float fOffsetX = 0.5f + (0.5f / width);
   const float fOffsetY = 0.5f + (0.5f / height);

   matrix._11 =  0.5f;
   matrix._12 =  0.0f;
   matrix._13 =  0.0f;
   matrix._14 =  0.0f;
   matrix._21 =  0.0f;
   matrix._22 = -0.5f;
   matrix._23 =  0.0f;
   matrix._24 =  0.0f;
   matrix._31 =  0.0f;
   matrix._32 =  0.0f;

   matrix._33 =  1.0f;
   
   matrix._34 =  0.0f;
   matrix._41 =  fOffsetX;
   matrix._42 =  fOffsetY;

   matrix._43 =  0.0f;
   
   matrix._44 =  1.0f;
}

//============================================================================
// BDirShadowManager::calcClippedPolyhedronVerts
//============================================================================
bool BDirShadowManager::calcClippedPolyhedronVerts(
   BVec3StaticArray& points, 
   float worldMinY, float worldMaxY, 
   float extraNearClipDist, 
   const Plane* pExtraPlanes, uint numExtraPlanes)
{
   const BMatrixTracker& matrixTracker = gRenderDraw.getWorkerSceneMatrixTracker();
   const BFrustum& sceneFrustum = matrixTracker.getWorldFrustum();

   BStaticArray<Plane, 16, false> planes;
   
   if (worldMinY != worldMaxY)
   {
      planes.pushBack(Plane(0.0f, 1.0f, 0.0f, worldMinY));
      planes.pushBack(Plane(0.0f, -1.0f, 0.0f, -worldMaxY));
   }
   
   if (extraNearClipDist > 0.0f)
   {
      BVec3 n(-matrixTracker.getWorldAtVec4());
      n[1] = 0.0f;
      if (!n.normalize(NULL))
         n = -matrixTracker.getWorldAtVec4();
      Plane p(n[0], n[1], n[2], 0.0f);

      BVec3 f(BVec3(matrixTracker.getWorldCamPosVec4()) + -n * extraNearClipDist);
      p.d = p.distanceToPoint(f);
      
      planes.pushBack(p);
   }         
   
   if (numExtraPlanes)
      planes.pushBack(pExtraPlanes, numExtraPlanes);
   
   planes.pushBack(sceneFrustum.getPlanes(), 6);

   BStaticArray<WORD, 512> combinations;
   uint numCombinations = GenerateCombinations(combinations, planes.size(), 3);
   
   points.resize(0);

   for (uint i = 0; i < numCombinations; i++)
   {
      const Plane& p1 = planes[combinations[i*3+0]];
      const Plane& p2 = planes[combinations[i*3+1]];
      const Plane& p3 = planes[combinations[i*3+2]];

      double det;
      det  =  p1.n[0]*(p2.n[1]*p3.n[2]-p2.n[2]*p3.n[1]);
      det -=  p2.n[0]*(p1.n[1]*p3.n[2]-p1.n[2]*p3.n[1]);
      det +=  p3.n[0]*(p1.n[1]*p2.n[2]-p1.n[2]*p2.n[1]);

      if (Math::EqualTol<double>(det, 0.0f, Math::fMinuteEpsilon))
         continue;

      const BVec3 point ( 
         ( (p1.d * (p2.n % p3.n)) +
         (p2.d * (p3.n % p1.n)) +
         (p3.d * (p1.n % p2.n)) ) / (float)det );

      uint j;
      for (j = 0; j < planes.size(); j++)
      {
         float dist = planes[j].distanceToPoint(point);
         if (dist < -Math::fTinyEpsilon)
            break;
      }
      if (j < planes.size())
         continue;

      points.pushBack(point);
   }
   
   bool empty = false;
   if (points.size() < 4)
   {
      empty = true;
      
      points.resize(0);
      points.pushBack(BVec3(-1.0f, 0.0f, -1.0f));
      points.pushBack(BVec3(1.0f, 0.0f, -1.0f));
      points.pushBack(BVec3(1.0f, 0.0f, 1.0f));
      points.pushBack(BVec3(-1.0f, 0.0f, 1.0f));

      points.pushBack(BVec3(-1.0f, 1.0f, -1.0f));
      points.pushBack(BVec3(1.0f, 1.0f, -1.0f));
      points.pushBack(BVec3(1.0f, 1.0f, 1.0f));
      points.pushBack(BVec3(-1.0f, 1.0f, 1.0f));
   }
     
   return empty;
}

//============================================================================
// BDirShadowManager::calcWorldPolyhedronVerts
//============================================================================
void BDirShadowManager::calcWorldPolyhedronVerts(BVec3StaticArray& points, float dist, float worldMinY, float worldMaxY, float lowY, float highY, float midY)
{
   const BMatrixTracker& matrixTracker = gRenderDraw.getWorkerSceneMatrixTracker();
   BVec3 camPos(matrixTracker.getWorldCamPosVec4());
         
   points.resize(0);
   
   BVec3 focus(camPos);
   focus[1] = lowY;
   points.pushBack(focus + BVec3(-dist, 0, -dist));
   points.pushBack(focus + BVec3( dist, 0, -dist));
   points.pushBack(focus + BVec3( dist, 0,  dist));
   points.pushBack(focus + BVec3(-dist, 0,  dist));
   
   static float g = 25.0f;
   focus[1] = highY + g;
   points.pushBack(focus + BVec3(-dist, 0, -dist));
   points.pushBack(focus + BVec3( dist, 0, -dist));
   points.pushBack(focus + BVec3( dist, 0,  dist));
   points.pushBack(focus + BVec3(-dist, 0,  dist));
}   

namespace
{
   //============================================================================
   // snapMatrix
   //============================================================================
   void snapMatrix(D3DXMATRIX& mat, D3DXMATRIX& oldMat, float tol)
   {
      int row;
      for (row = 0; row < 3; row++)
      {
         int col;
         for (col = 0; col < 3; col++)
         {
            if (fabs(mat(row, col) - oldMat(row, col)) > tol)
               break;
         }
         if (col < 3)
            break;
      }

      if (row == 3)
      {
         for (int row = 0; row < 3; row++)
            for (int col = 0; col < 3; col++)
               mat(row, col) = oldMat(row, col);
      }
      else
      {
         oldMat = mat;
      }
   }

   //============================================================================
   // snapMatrixTranslation
   //============================================================================
   void snapMatrixTranslation(D3DXMATRIX& worldToProj, D3DXMATRIX& viewToProj, uint width, uint height, float s)
   {
      float snapX = floor(worldToProj._41 *  s*width + .5f) / float(width*s);
      float snapY = floor(worldToProj._42 * s*height + .5f) / float(height*s);
      //float snapZ = floor(worldToProj._43 * 256.0f) / float(256.0f);
      viewToProj._41 += snapX - worldToProj._41;
      viewToProj._42 += snapY - worldToProj._42;
      //viewToProj._43 += snapZ - worldToProj._43;
      worldToProj._41 = snapX;
      worldToProj._42 = snapY;
      //worldToProj._43 = snapZ;
   }   
   
   //============================================================================
   // Tables
   //============================================================================
   const uchar gQuadVerts[6][4] =
   {
      { 4,0,3,7 }, // 0 left
      { 1,5,6,2 }, // 1 right

      { 0,4,5,1 }, // 2 bottom
      { 2,6,7,3 }, // 3 top

      { 0,1,2,3 }, // 4 near
      { 7,6,5,4 }  // 5 far
   };

   const uchar gEdgeVerts[12][2] =    
   {
      { 4, 0 },
      { 0, 3 },
      { 3, 7 },
      { 7, 4 },
      { 1, 5 },
      { 5, 6 },
      { 6, 2 },
      { 2, 1 },
      { 4, 5 },
      { 1, 0 },
      { 6, 7 },
      { 3, 2 }
   };      

   const uchar gEdgeQuads[12][2] =
   {
      { 0,	2 },
      { 0,	4 },
      { 0,	3 },
      { 0,	5 },
      { 1,	2 },
      { 1,	5 },
      { 1,	3 },
      { 1,	4 },
      { 2,	5 },
      { 2,	4 },
      { 3,	5 },
      { 3,	4 }
   };      
}

//============================================================================
// BDirShadowManager::computeFocus
//============================================================================
void BDirShadowManager::computeFocus(float& lowY, float& highY, float& midY)
{
   const XMVECTOR origin = gRenderDraw.getWorkerSceneMatrixTracker().getWorldCamPos();

   uint numIntersections = 0;
   
   lowY = 0.0f;
   highY = 0.0f;
   midY = 0.0f;
   
   const uint cN = 10;
   for (uint y = 0; y < cN; y++)
   {
      for (uint x = 0; x < cN; x++)
      {
         const float a = 34.0f;
         XMVECTOR ofs = XMVectorSet( Math::Lerp(-a, a, x/float(cN-1)), 0.0f, Math::Lerp(-a, a, y/float(cN-1)), 0.0f);
                  
         float lowNormZ, highNormZ;
         if (gTerrainHeightField.castRay(origin + ofs, lowNormZ, highNormZ))
         {
            float y = gTerrainHeightField.computeWorldY(lowNormZ);
            lowY += lowY;
            
            y = gTerrainHeightField.computeWorldY(highNormZ);
            highY += highY;
            
            midY += (lowY + highY) * .5f;
            
            numIntersections++;
         }
      }         
   }         

   if (!numIntersections)
   {
      lowY = 0.0f;
      highY = 10.0f;
      midY = 5.0f;
   }
   else
   {
      double s = 1.0f/numIntersections;
      lowY = (float)(lowY * s);
      highY = (float)(highY * s);
      midY = (float)(midY * s);
   }
   
   lowY = floor(lowY / 10.0f) * 10.0f;
   midY = floor(midY / 10.0f) * 10.0f;
   highY = ceil(highY / 10.0f) * 10.0f;
}

//============================================================================
// BDirShadowManager::updateInclusionPlanes
//============================================================================
void BDirShadowManager::updateInclusionPlanes(void)
{
   const BMatrixTracker& matrixTracker = gRenderDraw.getWorkerSceneMatrixTracker();
   const BFrustum& frustum = matrixTracker.getWorldFrustum();
   const BVec3 lightDir(&gRenderSceneLightManager.getDirLight(cLCTerrain).mDir.x);

   XMVECTOR frustumVerticesVecs[8];

   matrixTracker.getFrustumVertices(frustumVerticesVecs);

   const BVec4* frustumVertices = reinterpret_cast<const BVec4*>(frustumVerticesVecs);

   const BVec3 frustumCenter((frustumVertices[0] + frustumVertices[6]) * .5f);

   bool facingFlags[6];
   uint numInclusionPlanes = 0;

   for (uint quadIndex = 0; quadIndex < 6; quadIndex++)
   {
      const Plane& plane = frustum.plane(quadIndex);

      //BDEBUG_ASSERT(plane.distanceToPoint(frustumCenter) >= 0.0f);

      const bool facing = (plane.normal() * lightDir) < 0.0f;

      facingFlags[quadIndex] = facing;

      if (facing)
      {
         mInclusionPlanes[numInclusionPlanes++] = XMLoadFloat4((XMFLOAT4*)plane.equation().getPtr());

         Plane oppPlane(plane);

         // rg [3/4/06] - This is a hack, we should always push each plane "up" towards the light to a maximum height.
         // This will fail if the far clip plane is too high and the camera looks down.
         oppPlane.d += 3000.0f;
         oppPlane = oppPlane.flipped();

         //BDEBUG_ASSERT(oppPlane.distanceToPoint(frustumCenter) >= 0.0f);

         mInclusionPlanes[numInclusionPlanes++] = XMLoadFloat4((XMFLOAT4*)oppPlane.equation().getPtr());
      }         
   }

   for (uint edgeIndex = 0; edgeIndex < 12; edgeIndex++)
   {
      if (facingFlags[gEdgeQuads[edgeIndex][0]] == facingFlags[gEdgeQuads[edgeIndex][1]])
         continue;

      BVec3 edgeVec(frustumVertices[gEdgeVerts[edgeIndex][0]] - frustumVertices[gEdgeVerts[edgeIndex][1]]);
      edgeVec.tryNormalize();

      BVec3 norm(edgeVec % lightDir);
      Plane plane(norm, frustumVertices[gEdgeVerts[edgeIndex][0]]);

      if (plane.distanceToPoint(frustumCenter) < 0.0f)
         plane = plane.flipped();

      if (numInclusionPlanes == cMaxInclusionPlanes)
         break;

      mInclusionPlanes[numInclusionPlanes++] = XMLoadFloat4((XMFLOAT4*)plane.equation().getPtr());            
   }

   mNumInclusionPlanes = numInclusionPlanes;
}

//============================================================================
// BDirShadowManager::computeZScaleMatrix
//============================================================================
XMMATRIX BDirShadowManager::computeZScaleMatrix(float minZ, float maxZ)
{
   if (maxZ <= minZ)
      return XMMatrixIdentity();
      
   XMMATRIX matrix;
   
   matrix._11 =  1.0f; matrix._12 =  0.0f; matrix._13 =  0.0f; matrix._14 =  0.0f;

   matrix._21 =  0.0f; matrix._22 =  1.0f; matrix._23 =  0.0f; matrix._24 =  0.0f;

   matrix._31 =  0.0f;
   matrix._32 =  0.0f;
   matrix._33 =  1.0f / (maxZ - minZ);
   matrix._34 =  0.0f;

   matrix._41 =  0.0f;
   matrix._42 =  0.0f;
   matrix._43 =  -minZ / (maxZ - minZ);

   matrix._44 =  1.0f;

   // Same thing, but less precise:
   //matrix = XMMatrixTranslation(0.0f, 0.0f, -minZ) * XMMatrixScaling(1.0f, 1.0f, 1.0f / (maxZ - minZ));
   
   return matrix;
}

//============================================================================
// BDirShadowManager::sceneIteratorCallback
//============================================================================
void BDirShadowManager::sceneIteratorCallback(XMVECTOR worldMin, XMVECTOR worldMax, bool terrainChunk)
{
   AABB worldBounds;
   worldBounds[0].set(worldMin.x, worldMin.y, worldMin.z);
   worldBounds[1].set(worldMax.x, worldMax.y, worldMax.z);
   
   XMVECTOR c0 = worldMin;
   XMVECTOR c1 = __vrlimi(worldMin, worldMax, VRLIMI_CONST(1, 0, 0, 0), 0);   
   XMVECTOR c2 = __vrlimi(worldMin, worldMax, VRLIMI_CONST(0, 1, 0, 0), 0);   
   XMVECTOR c3 = __vrlimi(worldMin, worldMax, VRLIMI_CONST(1, 1, 0, 0), 0);   
   XMVECTOR c4 = __vrlimi(worldMin, worldMax, VRLIMI_CONST(0, 0, 1, 0), 0);   
   XMVECTOR c5 = __vrlimi(worldMin, worldMax, VRLIMI_CONST(1, 0, 1, 0), 0);   
   XMVECTOR c6 = __vrlimi(worldMin, worldMax, VRLIMI_CONST(0, 1, 1, 0), 0);   
   XMVECTOR c7 = worldMax;
   
   if (!terrainChunk)   
   {
      // Try to filter bogus bounding boxes (especially the ones during combat).
      if ((worldBounds.dimension(0) > 1000.0f) || (worldBounds.dimension(1) > 1000.0f) || (worldBounds.dimension(2) > 1000.0f))
         return;
         
      // Ignore bounds outside of what's supposed to be the valid world.
      if (!mWorldBounds.overlaps(worldBounds))
         return;
   }         
   
   XMMATRIX worldToProj = mWorldToProj[mFinalCSMPassIndex];
   c0 = XMVector3Transform(c0, worldToProj);
   c1 = XMVector3Transform(c1, worldToProj);
   c2 = XMVector3Transform(c2, worldToProj);
   c3 = XMVector3Transform(c3, worldToProj);
   c4 = XMVector3Transform(c4, worldToProj);
   c5 = XMVector3Transform(c5, worldToProj);
   c6 = XMVector3Transform(c6, worldToProj);
   c7 = XMVector3Transform(c7, worldToProj);
   
   XMVECTOR p0 = XMVectorPermuteControl(2, 6, 0, 0);
   XMVECTOR p1 = XMVectorPermuteControl(0, 1, 6, 0);
   XMVECTOR p2 = XMVectorPermuteControl(0, 1, 2, 6);
   XMVECTOR p3 = XMVectorPermuteControl(3, 7, 0, 0);
   XMVECTOR p4 = XMVectorPermuteControl(0, 1, 7, 0);
   XMVECTOR p5 = XMVectorPermuteControl(0, 1, 2, 7);
      
   XMVECTOR z0 = XMVectorPermute(c0, c1, p0);
   z0 = XMVectorPermute(z0, c2, p1);
   z0 = XMVectorPermute(z0, c3, p2);

   XMVECTOR z1 = XMVectorPermute(c4, c5, p0);
   z1 = XMVectorPermute(z1, c6, p1);
   z1 = XMVectorPermute(z1, c7, p2);
   
   XMVECTOR w0 = XMVectorPermute(c0, c1, p3);
   w0 = XMVectorPermute(w0, c2, p4);
   w0 = XMVectorPermute(w0, c3, p5);
   
   XMVECTOR w1 = XMVectorPermute(c4, c5, p3);
   w1 = XMVectorPermute(w1, c6, p4);
   w1 = XMVectorPermute(w1, c7, p5);
   
   XMVECTOR zero = XMVectorZero();
   XMVECTOR k0 = XMVectorGreater(z0, zero);
   XMVECTOR k1 = XMVectorGreater(z1, zero);
         
   z0 = XMVectorAndInt(k0, XMVectorMultiply(z0, XMVectorReciprocal(w0)));
   z1 = XMVectorAndInt(k1, XMVectorMultiply(z1, XMVectorReciprocal(w1)));
   
   XMVECTOR MinZ = XMVectorMin(z0, z1);
   MinZ = XMVectorMin(MinZ, __vrlimi(MinZ, MinZ, VRLIMI_CONST(1,1,1,1), 2));
   MinZ = XMVectorMin(MinZ, XMVectorSplatY(MinZ));
   
   XMVECTOR MaxZ = XMVectorMax(z0, z1);
   MaxZ = XMVectorMax(MaxZ, __vrlimi(MaxZ, MaxZ, VRLIMI_CONST(1,1,1,1), 2));
   MaxZ = XMVectorMax(MaxZ, XMVectorSplatY(MaxZ));

#if 0   
   float minZ = Math::fNearlyInfinite;
   float maxZ = -Math::fNearlyInfinite;
   
   for (uint i = 0; i < 8; i++)
   {
      BVec4 p(BVec4(worldBounds.corner(i)).toPoint());
      
      p = p * ((BMatrix44&)mWorldToProj[mFinalCSMPassIndex]);
      
      float z = 0.0f;

      if (p[3] > 0.0f)
         z = p[2] / p[3];

      minZ = Math::Min(minZ, z);
      maxZ = Math::Max(maxZ, z);
   }
   
   BDEBUG_ASSERT(fabs(minZ - MinZ.x) < .000125f);
   BDEBUG_ASSERT(fabs(maxZ - MaxZ.x) < .000125f);
#endif   

   BOOL u[cMaxPasses];   
   for (uint pass = 0; pass < mNumPasses; pass++)
      u[pass] = mVolumeCullers[pass].isAABBVisibleBounds(worldMin, worldMax);
                 
   for (uint pass = 0; pass < mNumPasses; pass++)
   {
      if (u[pass])
      {
         mMinZ[pass] = (float)Math::fSelectMin(mMinZ[pass], MinZ.x);
         mMaxZ[pass] = (float)Math::fSelectMax(mMaxZ[pass], MaxZ.x);
      }
   }      
}

//============================================================================
// BDirShadowManager::terrainIteratorCallback
//============================================================================
bool BDirShadowManager::terrainIteratorCallback(const BTerrain::BSceneIterateParams& params, uint chunkX, uint chunkY, XMVECTOR worldMin, XMVECTOR worldMax, void* pData)
{
   static_cast<BDirShadowManager*>(pData)->sceneIteratorCallback(worldMin, worldMax, true);
   return true;
}

//============================================================================
// BDirShadowManager::ugxIteratorCallback
//============================================================================
bool BDirShadowManager::ugxIteratorCallback(const BUGXGeomInstanceManager::BSceneIterateParams& params, uint instanceIndex, XMVECTOR worldMin, XMVECTOR worldMax, void* pData)
{
   static_cast<BDirShadowManager*>(pData)->sceneIteratorCallback(worldMin, worldMax, false);
   return true;
}

//============================================================================
// BDirShadowManager::updateMatricesAndVolumerCullers
//============================================================================
void BDirShadowManager::updateMatricesAndVolumerCullers(bool snapTranslation)
{
   for (int pass = mNumPasses - 1; pass >= 0; pass--)
   {   
      mWorldToProj[pass] = XMMatrixMultiply(mWorldToView[pass], mViewToProj[pass]);

      if (snapTranslation)
      {  
         static float s1 = .25f;
         snapMatrixTranslation(CAST(D3DXMATRIX, mWorldToProj[pass]), CAST(D3DXMATRIX, mViewToProj[pass]), mWidth, mHeight, s1);
      }

      mFrustums[pass].set(mWorldToProj[pass]);

      mVolumeCullers[pass].setBasePlanes(mFrustums[pass], mWorldToProj[pass]);
      mVolumeCullers[pass].enableInclusionPlanes(mInclusionPlanes, mNumInclusionPlanes);
   }
   
   for (int pass = mNumPasses - 1; pass >= 0; pass--)
   {   
      if (pass > 0)
         mVolumeCullers[pass].enableExclusionPlanes(mFrustums[pass - 1]);
      else
         mVolumeCullers[pass].disableExclusionPlanes();
   }
}

//============================================================================
// BDirShadowManager::optimizeZExtents
//============================================================================
void BDirShadowManager::optimizeZExtents(const BVec3& worldMin, const BVec3& worldMax, bool enableZExtentOptimization)
{
   SCOPEDSAMPLE(OptimizeZExtents);
   
   if (!enableZExtentOptimization)
   {  
      for (uint i = 0; i < mNumPasses; i++)
      {
         mMinZ[i] = 0.0f;
         mMaxZ[i] = 1.0f;
      }
      return;
   }
   
   updateMatricesAndVolumerCullers(true);
   
   BVolumeCuller volumeCuller(mVolumeCullers[mFinalCSMPassIndex]);
   volumeCuller.disableExclusionPlanes();
   
   for (uint i = 0; i < mNumPasses; i++)
   {
      mMinZ[i] = Math::fNearlyInfinite;
      mMaxZ[i] = -Math::fNearlyInfinite;
   }
   
   BTerrain::BSceneIterateParams terrainIterateParams;
   terrainIterateParams.mpVolumeCuller = &volumeCuller;
   terrainIterateParams.mIncludeSkirt = false;
   terrainIterateParams.mShadowCastersOnly = false;

   uint numNodes;
   
   {
      SCOPEDSAMPLE(TerrainIterate);
      numNodes = gTerrain.sceneIterate(terrainIterateParams, terrainIteratorCallback, this);
   }      
   
   BUGXGeomInstanceManager::BSceneIterateParams ugxIterateParams;
   ugxIterateParams.mpVolumeCuller = &volumeCuller;
   ugxIterateParams.mShadowCastersOnly = false;
   
   {
      SCOPEDSAMPLE(UnitIterate);
      numNodes += gUGXGeomInstanceManager.renderSceneIterate(ugxIterateParams, ugxIteratorCallback, this);
   }
         
   if (!numNodes) 
   {
      for (uint i = 0; i < mNumPasses; i++)
      {
         mMinZ[i] = 0.0f;
         mMaxZ[i] = 1.0f;
      }
   }    
   else
   {
      for (uint i = 0; i < mNumPasses; i++)
      {
         mMinZ[i] = Math::Clamp(mMinZ[i], 0.0f, 1.0f);
         mMaxZ[i] = Math::Clamp(mMaxZ[i], 0.0f, 1.0f);
         if (mMinZ[i] >= mMaxZ[i])
         {
            mMinZ[i] = 0.0f;
            mMaxZ[i] = 1.0f;
         }
      }
      
      //for (uint pass = 0; pass < mNumPasses; pass++)
      XMMATRIX finalZScale;
      for (int pass = mNumPasses - 1; pass >= 0; pass--)
      {
         XMMATRIX zScale = computeZScaleMatrix(mMinZ[pass], mMaxZ[pass]);
         
         mViewToProj[pass] = mViewToProj[pass] * zScale;
         
         if ((uint)pass == mFinalCSMPassIndex)
         {
            finalZScale = zScale;
            mMinZ[pass] = 0.0f;
            mMaxZ[pass] = 1.0f;
         }
         else
         {
            //mMinZ[pass] = (mMinZ[pass] - mMinZ[mFinalCSMPassIndex]) / (mMaxZ[mFinalCSMPassIndex] - mMinZ[mFinalCSMPassIndex]);
            //mMaxZ[pass] = (mMaxZ[pass] - mMinZ[mFinalCSMPassIndex]) / (mMaxZ[mFinalCSMPassIndex] - mMinZ[mFinalCSMPassIndex]);
            
            mMinZ[pass] = mMinZ[pass] * finalZScale._33 + finalZScale._43;
            mMaxZ[pass] = mMaxZ[pass] * finalZScale._33 + finalZScale._43;
         }
      }
   }        
}         

//============================================================================
// BDirShadowManager::shadowGenPrep
//============================================================================
void BDirShadowManager::shadowGenPrep(const BVec3& worldMin, const BVec3& worldMax, BShadowMode shadowMode, bool enableZExtentOptimization)
{
   SCOPEDSAMPLE(ShadowGenPrep);

   ASSERT_THREAD(cThreadIndexRender);
   
   mWorldBounds.set(worldMin, worldMax);

   uint p = 5;
   uint w = 768;
   float initialDist = 25.0f;
   
   if (shadowMode != mPrevShadowMode)
   {
      for (uint i = 0; i < cMaxPasses; i++)
      {
         mPrevWorldToView[i]  = XMMatrixIdentity();
         mPrevViewToProj[i]   = XMMatrixIdentity();
      }      
   }
   
   mPrevShadowMode = shadowMode;
   
   switch (shadowMode)
   {
      case cSMSingleMap:
      {
         static uint numPasses = 1;
         static uint width = 1024;
         static float dist = 300.0f;
         p = 1;
         w = 1024;
         initialDist = 300.0f;
         break;
      }
      case cSMRTS:
      {
         static uint numPasses = 3;
         static uint width = 768;
         static float dist = 80.0f;
         p = numPasses;
         w = width;
         initialDist = dist;
         break;
      }
      case cSMHighestQuality:
      {
         static uint numPassesHQ = 7;
         static uint widthHQ = 1024;
         static float distHQ = 23.0f;
         p = numPassesHQ;
         w = widthHQ;
         initialDist = distHQ;
         break;
      }
      case cSMMegaScreenshot:
      {
         static uint numPasses = 6;
         static uint width = 768;
         static float dist = 30.0f;
         p = numPasses;
         w = width;
         initialDist = dist;
         break;
      }
      default:
      {
         static uint numPasses = 5;
         static uint width = 768;
         static float dist = 25.0f;
         p = numPasses;
         w = width;
         initialDist = dist;
         break;
      }
   }
   
   const bool cascaded = p > 1;
   
   mNumPasses = p;   
      
   mWidth = w;
   mHeight = w;
   
   mFinalCSMPassIndex = mNumPasses - 1;
   
   float lowY, highY, midY;
   computeFocus(lowY, highY, midY);
         
   updateTextureScaleMatrix(mWidth, mHeight);
   
   updateInclusionPlanes();
      
   float dist = initialDist;
   for (uint pass = 0; pass < mNumPasses; pass++)
   {
      shadowGenPrepForPass(worldMin, worldMax, lowY, highY, midY, dist, pass, cascaded);
      dist *= 2.0f;
   }
   
   static float viewTol = .0005f;
   snapMatrix(CAST(D3DXMATRIX, mWorldToView[mFinalCSMPassIndex]), CAST(D3DXMATRIX, mPrevWorldToView[mFinalCSMPassIndex]), viewTol);
   
   static float projTol = .00005f;
   snapMatrix(CAST(D3DXMATRIX, mViewToProj[mFinalCSMPassIndex]), CAST(D3DXMATRIX, mPrevViewToProj[mFinalCSMPassIndex]), projTol);

   mWorldToProj[mFinalCSMPassIndex] = XMMatrixMultiply(mWorldToView[mFinalCSMPassIndex], mViewToProj[mFinalCSMPassIndex]);
   static float s0 = .25f;
   snapMatrixTranslation(CAST(D3DXMATRIX, mWorldToProj[mFinalCSMPassIndex]), CAST(D3DXMATRIX, mViewToProj[mFinalCSMPassIndex]), mWidth, mHeight, s0);
   
   for (uint pass = 0; pass < mFinalCSMPassIndex; pass++)
   {
      mWorldToView[pass] = mWorldToView[mFinalCSMPassIndex];

      mViewToProj[pass] = mViewToProj[mFinalCSMPassIndex];

      float scale = powf(2.0f, ((float)mFinalCSMPassIndex - pass));

      mViewToProj[pass] = mViewToProj[mFinalCSMPassIndex] * XMMatrixScaling(scale, scale, 1.0f);
   }
   
   optimizeZExtents(worldMin, worldMax, enableZExtentOptimization);
   
   updateMatricesAndVolumerCullers(true);
}

//============================================================================
// BDirShadowManager::shadowGenPrepForPass
//============================================================================
void BDirShadowManager::shadowGenPrepForPass(const BVec3& worldMin, const BVec3& worldMax, float lowY, float highY, float midY, float dist, uint pass, bool cascaded)
{
   if (cascaded)
      calcWorldPolyhedronVerts(mWorldPoints[pass], dist, worldMin[1], worldMax[1], lowY, highY, midY);
   else
   {
      if (calcClippedPolyhedronVerts(mWorldPoints[pass], worldMin[1], worldMax[1], dist))
         calcWorldPolyhedronVerts(mWorldPoints[pass], dist, worldMin[1], worldMax[1], lowY, highY, midY);
   }      
      
   const uint numOrigPoints = mWorldPoints[pass].size();
   const BVec3 lightDir(&gRenderSceneLightManager.getDirLight(cLCTerrain).mDir.x);
   
   for (uint i = 0; i < numOrigPoints; i++)
   {
      BVec3 p(mWorldPoints[pass][i]);
      
      // This is used for culling only, 1000 units is arbitrary.
      static float lightDirPushDist = 1000.0f;
      p += -lightDir * lightDirPushDist;
      mWorldPoints[pass].pushBack(p);
   }      
         
   BVec3 lightPos(gRenderDraw.getWorkerSceneMatrixTracker().getWorldCamPosVec4());
   BVec3 lightAt(CONST_CAST(BVec3, gRenderSceneLightManager.getDirLight(cLCTerrain).mDir));
   static float pullBackDist = 300.0f;
   lightPos -= lightAt * pullBackDist;
   BVec3 lightRight(BVec3(0.0f, 1.0f, 0.0f) % lightAt);
   if (!lightRight.normalize(NULL))
      lightRight.set(0.0f, 0.0f, 1.0f);
      
   BVec3 lightUp(lightAt % lightRight);
   if (!lightUp.normalize(NULL))
      lightUp.set(-1.0f, 0.0f, 0.0f);
      
   XMMATRIX translate = XMMatrixTranslation(-lightPos[0], -lightPos[1], -lightPos[2]);

   mViewPoints[pass].resize(mWorldPoints[pass].size());   

   float lightCenterX = 0.0f;
   float lightCenterY = 0.0f;
   for (uint p = 0; p < 2; p++)
   {
      mWorldToView[pass].r[0] = XMVectorSet(lightRight[0], lightUp[0], lightAt[0], 0.0f);
      mWorldToView[pass].r[1] = XMVectorSet(lightRight[1], lightUp[1], lightAt[1], 0.0f);
      mWorldToView[pass].r[2] = XMVectorSet(lightRight[2], lightUp[2], lightAt[2], 0.0f);
      mWorldToView[pass].r[3] = XMVectorSet(-lightCenterX, -lightCenterY, 0.0f, 1.0f);
      
      mWorldToView[pass] = XMMatrixMultiply(translate, mWorldToView[pass]);
                     
      mViewMin[pass].set(1e+10f);
      mViewMax[pass].set(-1e+10f);
           
      for (uint i = 0; i < mWorldPoints[pass].size(); i++)
      {
         XMVECTOR v = XMVector4Transform(XMVectorSet(mWorldPoints[pass][i][0], mWorldPoints[pass][i][1], mWorldPoints[pass][i][2], 1.0f), mWorldToView[pass]);
         
         mViewPoints[pass][i] = BVec3(v.x, v.y, v.z);
                           
         mViewMin[pass] = BVec3::elementMin(mViewMin[pass], mViewPoints[pass][i]);
         mViewMax[pass] = BVec3::elementMax(mViewMax[pass], mViewPoints[pass][i]);
      }
      
      if (p == 0)
      {
         lightCenterX = .5f * (mViewMin[pass][0] + mViewMax[pass][0]);
         lightCenterY = .5f * (mViewMin[pass][1] + mViewMax[pass][1]);
      }         
   }      
               
   mViewToProj[pass] = XMMatrixOrthographicOffCenterLH(
      mViewMin[pass][0], mViewMax[pass][0], 
      mViewMax[pass][1], mViewMin[pass][1], 
      mViewMin[pass][2] - 1024.0f, mViewMax[pass][2] + 1024.0f);
}

//============================================================================
// BDirShadowManager::getNumPasses
//============================================================================
uint BDirShadowManager::getNumPasses(void) const 
{ 
   if ( ((!gRenderSceneLightManager.getDirLight(cLCTerrain).mEnabled) || (!gRenderSceneLightManager.getDirLight(cLCTerrain).mShadows)) &&
        ((!gRenderSceneLightManager.getDirLight(cLCUnits).mEnabled) || (!gRenderSceneLightManager.getDirLight(cLCUnits).mShadows)) )
   {
      return 0;
   }

   return mNumPasses;
}

//============================================================================
// BDirShadowManager::allocateShadowBuffers
//============================================================================
void BDirShadowManager::allocateShadowBuffers(void)
{
   if (mpShadowBuffer)
      return;
         
   // MIKEY - use the custom SHADOW16 format
   HRESULT hres = gGPUFrameHeap.createArrayTexture(mWidth, mHeight, mNumPasses, 1, 0, D3DFMT_SHADOW16, 0, &mpShadowBuffer, NULL);
   BVERIFY(SUCCEEDED(hres));
   mpShadowBuffer->Format.ExpAdjust = SHADOW16_SAMPLE_BIAS;

   gEffectIntrinsicManager.set(cIntrinsicDirShadowMapTexture, &mpShadowBuffer, cIntrinsicTypeTexturePtr);
}

//============================================================================
// BDirShadowManager::releaseTempBuffers
//============================================================================
void BDirShadowManager::releaseTempBuffers(void)
{
   gGPUFrameHeap.releaseD3DResource(mpRenderTarget);
   mpRenderTarget = NULL;
   
   gGPUFrameHeap.releaseD3DResource(mpDepthStencil);
   mpDepthStencil = NULL;
}

//============================================================================
// BDirShadowManager::releaseShadowBuffers
//============================================================================
void BDirShadowManager::releaseShadowBuffers(void)
{
   if (!mpShadowBuffer)
      return;
      
   gRenderDraw.unsetTextures();      
   
   gGPUFrameHeap.releaseD3DResource(mpShadowBuffer);
   mpShadowBuffer = NULL;
               
   gEffectIntrinsicManager.set(cIntrinsicDirShadowMapTexture, &mpShadowBuffer, cIntrinsicTypeTexturePtr);
}

//============================================================================
// BDirShadowManager::shadowGenBegin
//============================================================================
void BDirShadowManager::shadowGenBeginPass(uint pass)
{
   ASSERT_THREAD(cThreadIndexRender);
   
   BDEBUG_ASSERT(mInShadowPass);
            
   gEffectIntrinsicManager.setMatrix(cIntrinsicDirShadowWorldToProj, &mWorldToProj[pass]);
   
   BMatrixTracker&  matrixTracker   = gRenderDraw.getWorkerActiveMatrixTracker();
   BRenderViewport& renderViewport  = gRenderDraw.getWorkerActiveRenderViewport();
             
   matrixTracker.setMatrix(cMTWorldToView, mWorldToView[pass]);
   matrixTracker.setMatrix(cMTViewToProj, mViewToProj[pass]);

   D3DVIEWPORT9 viewport;
   viewport.X = 0;
   viewport.Y = 0;
   viewport.Width = mWidth;
   viewport.Height = mHeight;
   viewport.MinZ = 0.0f;
   viewport.MaxZ = 1.0f;
   
   matrixTracker.setViewport(viewport);
        
   renderViewport.setSurf(0, mpRenderTarget);
   renderViewport.setDepthStencilSurf(mpDepthStencil);
   renderViewport.setViewport(viewport);
   
   BVolumeCuller& volumeCuller = gRenderDraw.getWorkerActiveVolumeCuller();
   volumeCuller = mVolumeCullers[pass];
               
   gRenderDraw.setWorkerActiveMatrixTracker(matrixTracker);
   gRenderDraw.setWorkerActiveRenderViewport(renderViewport);
   
   gRenderDraw.clear(D3DCLEAR_TARGET0|D3DCLEAR_STENCIL|D3DCLEAR_ZBUFFER);
         
   //BD3D::mpDev->SetRenderState(D3DRS_COLORWRITEENABLE, 0);
   //BD3D::mpDev->SetRenderState(D3DRS_CULLMODE, D3DCULL_CW);
   BD3D::mpDev->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);

   static float depthBias = 0.0f;//.00003;//.00003f;
   static float slopeScaleBias = 0.0f;//1.0f;//.75f;
   //approx 2.5 for 16-bit
      
   BD3D::mpDev->SetRenderState(D3DRS_DEPTHBIAS, CAST(DWORD, depthBias));
   BD3D::mpDev->SetRenderState(D3DRS_SLOPESCALEDEPTHBIAS, CAST(DWORD, slopeScaleBias));
      
   BD3D::mpDev->SetPixelShader(NULL);
}

//============================================================================
// BDirShadowManager::tickEffect
//============================================================================
bool BDirShadowManager::tickEffect(void)
{
   if (mpEffectLoader->tick())
   {
      if (mpEffectLoader->isEffectValid())     
      {
         mHFilterTechnique = mpEffectLoader->getFXLEffect().getTechnique("VSMHFilter");
         mVFilterTechnique = mpEffectLoader->getFXLEffect().getTechnique("VSMVFilter");
         m2DFilterTechnique = mpEffectLoader->getFXLEffect().getTechnique("VSM2DFilter");
      }
      else
      {
         mHFilterTechnique.clear();
         mVFilterTechnique.clear();
         m2DFilterTechnique.clear();
      }
   }

   if (mpEffectLoader->isEffectValid())
   {
      mpEffectLoader->getFXLEffect().updateIntrinsicParams();

      return mHFilterTechnique.getValid() && mVFilterTechnique.getValid() && m2DFilterTechnique.getValid();
   }
   
   return false;
}

//============================================================================
// BDirShadowManager::resolveShadowBufferSurface
//============================================================================
void BDirShadowManager::resolveShadowBufferSurface(uint pass)
{
   SCOPEDSAMPLE(ResolveShadowBuffer);
         
   // MIKEY - use the custom SHADOW16 format
   BD3D::mpDev->Resolve(
      D3DRESOLVE_RENDERTARGET0 | (DWORD)D3DRESOLVE_EXPONENTBIAS(SHADOW16_RESOLVE_BIAS),
      NULL,
      mpShadowBuffer,
      NULL,
      0,
      pass, // dest slice was pass,
      NULL,
      1.0f,
      0,
      NULL);
}

//============================================================================
// BDirShadowManager::shadowGenEnd
//============================================================================
void BDirShadowManager::shadowGenEndPass(uint pass)
{
   ASSERT_THREAD(cThreadIndexRender);
   
   BDEBUG_ASSERT(mInShadowPass);
         
   resolveShadowBufferSurface(pass);
}

//============================================================================
// BDirShadowManager::shadowGenInit
//============================================================================
void BDirShadowManager::shadowGenInit(void)
{
   mInShadowPass = true;
   
   // MIKEY - use the custom SHADOW16 format
   D3DSURFACE_PARAMETERS surfaceParams;
   Utils::ClearObj(surfaceParams);
   surfaceParams.Base = 0;
   surfaceParams.ColorExpBias = SHADOW16_RENDERTARGET_BIAS;

   HRESULT hres = gGPUFrameHeap.createRenderTarget(mWidth, mHeight, D3DFMT_SHADOW16_EDRAM, D3DMULTISAMPLE_NONE, 0, FALSE, &mpRenderTarget, &surfaceParams);
   BVERIFY(SUCCEEDED(hres));
   
   surfaceParams.Base = XGSurfaceSize(mWidth, mHeight, D3DFMT_SHADOW16_EDRAM, D3DMULTISAMPLE_NONE);
   hres = gGPUFrameHeap.createRenderTarget(mWidth, mHeight, D3DFMT_D24S8, D3DMULTISAMPLE_NONE, 0, FALSE, &mpDepthStencil, &surfaceParams);
   BVERIFY(SUCCEEDED(hres));
}

//============================================================================
// BDirShadowManager::shadowGenDeinit
//============================================================================
void BDirShadowManager::shadowGenDeinit(void)
{
   if (!mInShadowPass)
      return;

//   BD3D::mpDev->SetRenderTarget(0, BD3D::mpDevBackBuffer);      
//   BD3D::mpDev->SetDepthStencilSurface(BD3D::mpDevDepthStencil);     
            
   gRenderDraw.getWorkerActiveVolumeCuller().disableExclusionPlanes();
   gRenderDraw.getWorkerActiveVolumeCuller().disableInclusionPlanes();

   gRenderDraw.resetWorkerActiveMatricesAndViewport();
   
   releaseTempBuffers();

   //BD3D::mpDev->SetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_ALL);
   BD3D::mpDev->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
   BD3D::mpDev->SetRenderState(D3DRS_DEPTHBIAS, 0);
   BD3D::mpDev->SetRenderState(D3DRS_SLOPESCALEDEPTHBIAS, 0);

   gEffectIntrinsicManager.setMatrix(cIntrinsicDirShadowWorldToTex, &mWorldToProj[mFinalCSMPassIndex], 1);

   BVec4 params0((float)mWidth, .5f + (.5f / mWidth), 4.0f / mWidth, 1.0f / mWidth);
   BVec4 params1((float)mFinalCSMPassIndex, 1.0f / mNumPasses, 1.0f / (2.0f * mNumPasses), powf(2.0, (float)mFinalCSMPassIndex));
   
   gEffectIntrinsicManager.set(cIntrinsicDirShadowParams0, &params0, cIntrinsicTypeFloat4);
   gEffectIntrinsicManager.set(cIntrinsicDirShadowParams1, &params1, cIntrinsicTypeFloat4);
   
   BVec4 zScales[cMaxPasses];
   for (uint i = 0; i < cMaxPasses; i++)
   {
      if ((mMaxZ[i] - mMinZ[i]) > 0.0f)
         zScales[i].set(1.0f / (mMaxZ[i] - mMinZ[i]), -mMinZ[i] / (mMaxZ[i] - mMinZ[i]), 0.0f, 0.0f);
      else
         zScales[i].clear();
   }
   
   gEffectIntrinsicManager.set(cIntrinsicDirShadowZScales, zScales, cIntrinsicTypeFloat4, 8);
         
   mInShadowPass = false;
}

//============================================================================
// BDirShadowManager::initDeviceData
//============================================================================
void BDirShadowManager::initDeviceData(void)
{
   if (!mpEffectLoader)
   {
      mpEffectLoader = ALIGNED_NEW(BFXLEffectFileLoader, gRenderHeap);
      const bool status = mpEffectLoader->init(gRender.getEffectCompilerDefaultDirID(), SHADER_FILENAME, true, false, true);
      BVERIFY(status);
   }
}

//============================================================================
// BDirShadowManager::frameBegin
//============================================================================
void BDirShadowManager::frameBegin(void)
{
}

//============================================================================
// BDirShadowManager::processCommand
//============================================================================
void BDirShadowManager::processCommand(const BRenderCommandHeader &header, const uchar *pData)
{
}

//============================================================================
// BDirShadowManager::frameEnd
//============================================================================
void BDirShadowManager::frameEnd(void)
{
}

//============================================================================
// BDirShadowManager::deinitDeviceData
//============================================================================
void BDirShadowManager::deinitDeviceData(void)
{
   releaseTempBuffers();
   
   releaseShadowBuffers();
   
   if (mpEffectLoader)
   {
      ALIGNED_DELETE(mpEffectLoader, gRenderHeap);
      mpEffectLoader = NULL;
   }
}

#ifndef BUILD_FINAL   
//============================================================================
// BDirShadowManager::drawDebugFrustum
//============================================================================
void BDirShadowManager::drawDebugFrustum(const BMatrix44& frustProjToWorld, const BMatrix44& worldToProj, DWORD nearColor, DWORD farColor, DWORD connectColor, uint xOfs, uint yOfs, uint rectWidth, uint rectHeight)
{
   const BVec4 frustPoints[] = 
   {
      BVec4(-1,-1,0,1),
      BVec4(1,-1,0,1),
      BVec4(1,1,0,1),
      BVec4(-1,1,0,1),

      BVec4(-1,-1,1,1),
      BVec4(1,-1,1,1),
      BVec4(1,1,1,1),
      BVec4(-1,1,1,1)
   };

   int frustX[8], frustY[8];
   for (uint i = 0; i < 8; i++)
   {
      BVec4 p( (frustPoints[i] * frustProjToWorld).project() );

      p = p * *(BMatrix44*)&worldToProj; //mWorldToProj[pass];
      p = p.project();

      frustX[i] = (int)(rectWidth * (p[0] * .5f + .5f));
      frustY[i] = (int)(rectHeight * (p[1] * -.5f + .5f));
   }

   BPrimDraw2D::drawLine2D(xOfs+frustX[0], yOfs+frustY[0], xOfs+frustX[1], yOfs+frustY[1], nearColor);
   BPrimDraw2D::drawLine2D(xOfs+frustX[1], yOfs+frustY[1], xOfs+frustX[2], yOfs+frustY[2], nearColor);
   BPrimDraw2D::drawLine2D(xOfs+frustX[2], yOfs+frustY[2], xOfs+frustX[3], yOfs+frustY[3], nearColor);
   BPrimDraw2D::drawLine2D(xOfs+frustX[3], yOfs+frustY[3], xOfs+frustX[0], yOfs+frustY[0], nearColor);
   
   BPrimDraw2D::drawLine2D(xOfs+frustX[4], yOfs+frustY[4], xOfs+frustX[5], yOfs+frustY[5], farColor);
   BPrimDraw2D::drawLine2D(xOfs+frustX[5], yOfs+frustY[5], xOfs+frustX[6], yOfs+frustY[6], farColor);
   BPrimDraw2D::drawLine2D(xOfs+frustX[6], yOfs+frustY[6], xOfs+frustX[7], yOfs+frustY[7], farColor);
   BPrimDraw2D::drawLine2D(xOfs+frustX[7], yOfs+frustY[7], xOfs+frustX[4], yOfs+frustY[4], farColor);

   BPrimDraw2D::drawLine2D(xOfs+frustX[0], yOfs+frustY[0], xOfs+frustX[4], yOfs+frustY[4], connectColor);
   BPrimDraw2D::drawLine2D(xOfs+frustX[1], yOfs+frustY[1], xOfs+frustX[5], yOfs+frustY[5], connectColor);
   BPrimDraw2D::drawLine2D(xOfs+frustX[2], yOfs+frustY[2], xOfs+frustX[6], yOfs+frustY[6], connectColor);
   BPrimDraw2D::drawLine2D(xOfs+frustX[3], yOfs+frustY[3], xOfs+frustX[7], yOfs+frustY[7], connectColor);
}
#endif

#ifndef BUILD_FINAL   
//============================================================================
// BDirShadowManager::debugHandleInpit
//============================================================================
void BDirShadowManager::debugHandleInput(long event, long controlType, bool altPressed, bool shiftPressed, bool controlPressed)
{
   if (event == cInputEventControlStart)
   {
      switch (controlType)
      {
         case cKeyD:
         {
            if (altPressed)
               mDebugCurPage++;
            else if (shiftPressed)
               mDebugCurPage--;
            else if (controlPressed)
            {
#if DIR_SHADOW_MANAGER_DEBUG                           
               mDebugDisplay = !mDebugDisplay;
               gConsoleOutput.status("Dir shadow manager debug display %s", mDebugDisplay ? "Enabled" : "Disabled");
#else
               gConsoleOutput.status("Must define DIR_SHADOW_MANAGER_DEBUG");
#endif               
            }
               
            if (mDebugCurPage < 0) 
               mDebugCurPage = cMaxPasses;
            else if (mDebugCurPage > cMaxPasses)
               mDebugCurPage = 0;
                           
            break;
         }
      }
   }
}

//============================================================================
// BDirShadowManager::debugDraw
//============================================================================
void BDirShadowManager::debugDraw(ATG::Font& font)
{
   ASSERT_THREAD(cThreadIndexRender);
   
   if ((!mDebugDisplay) || (!mpShadowBuffer))
      return;
         
   BD3D::mpDev->SetRenderState(D3DRS_ZENABLE, FALSE);      
   
   WCHAR textBuf[256];
   StringCchPrintfW(textBuf, sizeof(textBuf)/sizeof(textBuf[0]), L"Page %i", mDebugCurPage);
   font.DrawText(50.0f, 20.0f, 0xFFFFFFFF, textBuf);   

   BD3D::mpDev->SetTexture(0, mpShadowBuffer);
   BD3D::mpDev->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
   BD3D::mpDev->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
   BD3D::mpDev->SetSamplerState(0, D3DSAMP_ADDRESSW, D3DTADDRESS_CLAMP);
      
   BD3D::mpDev->SetSamplerState(0, D3DSAMP_SEPARATEZFILTERENABLE, TRUE);
   BD3D::mpDev->SetSamplerState(0, D3DSAMP_MINFILTERZ, D3DTEXF_POINT);
   BD3D::mpDev->SetSamplerState(0, D3DSAMP_MAGFILTERZ, D3DTEXF_POINT);
   
   BD3D::mpDev->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
   BD3D::mpDev->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
   BD3D::mpDev->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
   BD3D::mpDev->SetSamplerState(0, D3DSAMP_MAXANISOTROPY, 1);
   
   uint firstPass = 0;
   uint lastPass = 0;
   uint rectWidth = 512;
   uint rectHeight = 512;
   
   if ((uint)mDebugCurPage == mNumPasses)
   {
      firstPass = 0;
      lastPass = mFinalCSMPassIndex;
      
      rectWidth = 256;
      rectHeight = 256;
   }
   else
   {
      firstPass = mDebugCurPage;
      lastPass = mDebugCurPage;
      rectWidth = 512;
      rectHeight = 512;
   }
   
   lastPass = Math::Min(lastPass, mFinalCSMPassIndex);   
      
   for (uint pass = firstPass; pass <= lastPass; pass++)
   {
      static BVec4 mul(1.0f);
      static BVec4 add(0.0f);
      BD3D::mpDev->SetPixelShaderConstantF(0, mul.getPtr(), 1);
      BD3D::mpDev->SetPixelShaderConstantF(1, add.getPtr(), 1);

      BVec4 slice(1.0f / (2.0f * mNumPasses) + (float)pass / mNumPasses);
      BD3D::mpDev->SetPixelShaderConstantF(2, slice.getPtr(), 1);
      
      uint xOfs = 50 + ((pass - firstPass) % 4) * (rectWidth + 25);
      uint yOfs = 50 + ((pass - firstPass) / 4) * (rectHeight + 25);
      BPrimDraw2D::drawSolidRect2D(xOfs, yOfs, xOfs+rectWidth, yOfs+rectHeight, 0.0f, 0.0f, 1.0f, 1.0f, 0xFFFFFFFF, 0xFFFFFFFF, cPosTex1VS, cDepthVisPS);
      
      for (uint i = 0; i < mViewPoints[pass].size(); i++)
      {
         const BVec3& p = mViewPoints[pass][i];
         
         XMVECTOR v = XMVector4Transform(XMVectorSet(p[0], p[1], p[2], 1.0f), mViewToProj[pass]);
         
         v.x /= v.w;
         v.y /= v.w;
         
         int x = (int)(rectWidth * (v.x * .5f + .5f));
         int y = (int)(rectHeight * (v.y * -.5f + .5f));
         
         BPrimDraw2D::drawLine2D(xOfs+x-8, yOfs+y, xOfs+x+8, yOfs+y, 0xFF00FF00);
         BPrimDraw2D::drawLine2D(xOfs+x, yOfs+y-8, xOfs+x, yOfs+y+8, 0xFF00FF00);
      }
      
      {
         const BVec3 s(gRenderDraw.getWorkerSceneMatrixTracker().getWorldCamPosVec4());
         const BVec3 e(gRenderDraw.getWorkerSceneMatrixTracker().getWorldCamPosVec4() + gRenderDraw.getWorkerSceneMatrixTracker().getWorldAtVec4() * 20.0f);

         XMVECTOR vs = XMVector4Transform(XMVectorSet(s[0], s[1], s[2], 1.0f), mWorldToProj[pass]);
         vs.x /= vs.w;
         vs.y /= vs.w;
         
         XMVECTOR ve = XMVector4Transform(XMVectorSet(e[0], e[1], e[2], 1.0f), mWorldToProj[pass]);
         ve.x /= ve.w;
         ve.y /= ve.w;
         
         int xs = (int)(rectWidth * (vs.x * .5f + .5f));
         int ys = (int)(rectHeight * (vs.y * -.5f + .5f));
         int xe = (int)(rectWidth * (ve.x * .5f + .5f));
         int ye = (int)(rectHeight * (ve.y * -.5f + .5f));
         BPrimDraw2D::drawLine2D(xOfs+xs-8, yOfs+ys, xOfs+xs+8, yOfs+ys, 0xFFFF00FF);
         BPrimDraw2D::drawLine2D(xOfs+xs, yOfs+ys-8, xOfs+xs, yOfs+ys+8, 0xFFFF00FF);

         BPrimDraw2D::drawLine2D(xOfs+xs, yOfs+ys, xOfs+xe, yOfs+ye, 0xFFFF00FF);
      }         
      
      BMatrix44 projToWorld(gRenderDraw.getWorkerSceneMatrixTracker().getMatrix44(cMTWorldToProj));
      projToWorld.invert();
      drawDebugFrustum(
         projToWorld,
         (const BMatrix44&)mWorldToProj[pass], 0xFFA0A0A0, 0xFFA0A0A0, 0xFFA0A0A0, xOfs, yOfs, rectWidth, rectHeight);
         
      for (uint k = 0; k < pass; k++)
      {
         const DWORD colors[4] = { 0xFF10A010, 0xFF10A0A0, 0xFFFFA0A0, 0xFFFFFFFF };
         
         BMatrix44 projToWorld((const BMatrix44&)mWorldToProj[k]);
         projToWorld.invert();
         drawDebugFrustum(
            projToWorld,
            (const BMatrix44&)mWorldToProj[pass], colors[k], colors[k], colors[k], xOfs, yOfs, rectWidth, rectHeight);         
      }
      
   }      
   
   BD3D::mpDev->SetSamplerState(0, D3DSAMP_SEPARATEZFILTERENABLE, FALSE);
}
#endif      

