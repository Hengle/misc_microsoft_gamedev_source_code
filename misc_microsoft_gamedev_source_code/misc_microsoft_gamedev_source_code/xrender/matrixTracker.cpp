//============================================================================
//
//  matrixTracker.cpp
//  
//  Copyright (c) 2006, Ensemble Studios
//
//============================================================================
#include "xrender.h"
#include "matrixTracker.h"

namespace 
{
   // Effect Matrix Names
   struct 
   {
      const char* pName;
      eMTMatrix eMatrix;
   } gMatrixNames[] = 
   {
      { "WorldToView",    cMTWorldToView  },
      { "ViewToProj",     cMTViewToProj   },
      { "ProjToScreen",   cMTProjToScreen },
      { "ViewToWorld",    cMTViewToWorld  },
      { "ScreenToProj",   cMTScreenToProj },
      { "WorldToProj",    cMTWorldToProj  },
      { "ScreenToView",   cMTScreenToView },
      { "WorldToScreen",  cMTWorldToScreen }
   };
   const int cNumMatrixNames = sizeof(gMatrixNames) / sizeof(gMatrixNames[0]);
} // anonymous namespace

//============================================================================
// BMatrixTracker::BMatrixTracker
//============================================================================
BMatrixTracker::BMatrixTracker()
{
   clear();
}

//============================================================================
// BMatrixTracker::~BMatrixTracker
//============================================================================
BMatrixTracker::~BMatrixTracker()
{
}

//============================================================================
// BMatrixTracker::BMatrixTracker
//============================================================================
BMatrixTracker::BMatrixTracker(const BMatrixTracker& rhs)
{
   *this = rhs;
}

//============================================================================
// BMatrixTracker::clear
//============================================================================
void BMatrixTracker::clear(void)
{
   for (int i = 0; i < cNumMTMatrices; i++)
   {
      mMatrices[i][0] = XMMatrixIdentity();
      mMatrices[i][1] = XMMatrixIdentity();
   }
      
   mWorldCamPos = XMVectorZero();
   
   mWorldFrustum.clear();
   mViewFrustum.clear();
   
   mViewport.X       = 0;
   mViewport.Y       = 0;
   mViewport.Width   = 640;
   mViewport.Height  = 480;
   mViewport.MinZ    = 0.0f;
   mViewport.MaxZ    = 1.0f;
}

//============================================================================
// BMatrixTracker::setMatrixPair
//============================================================================
void BMatrixTracker::setMatrixPair(eMTMatrix matrixIndex, const XMMATRIX m)
{
   BDEBUG_ASSERT(this);
   mMatrices[debugRangeCheck<int>(matrixIndex, cNumMTMatrices)][0] = m;
   mMatrices[matrixIndex][1] = XMMatrixTranspose(m);
}

//============================================================================
// BMatrixTracker::getMatrix
//============================================================================
const XMMATRIX& BMatrixTracker::getMatrix(eMTMatrix matrixIndex, bool transposed) const
{
   BDEBUG_ASSERT(this);
   return mMatrices[debugRangeCheck<int>(matrixIndex, cNumMTMatrices)][transposed];
}

//============================================================================
// BMatrixTracker::getMatrixByValue
//============================================================================
const XMMATRIX BMatrixTracker::getMatrixByValue(eMTMatrix matrixIndex, bool transposed) const
{
   BDEBUG_ASSERT(this);
   return mMatrices[debugRangeCheck<int>(matrixIndex, cNumMTMatrices)][transposed];
}

//============================================================================
// BMatrixTracker::getMatrix44
//============================================================================
const BMatrix44& BMatrixTracker::getMatrix44(eMTMatrix matrixIndex, bool transposed) const
{
   BDEBUG_ASSERT(this);
   return reinterpret_cast<const BMatrix44&>(mMatrices[debugRangeCheck<int>(matrixIndex, cNumMTMatrices)][transposed]);
}

//============================================================================
// BMatrixTracker::getMatrixEffectName
//============================================================================
const char* BMatrixTracker::getMatrixEffectName(eMTMatrix matrixIndex) 
{
   return gMatrixNames[debugRangeCheck<int>(matrixIndex, cNumMTMatrices)].pName;
}

//============================================================================
// BMatrixTracker::concatMatrices
//============================================================================
XMMATRIX BMatrixTracker::concatMatrices(eMTMatrix a, eMTMatrix b, bool aTransposed, bool bTransposed)
{
   return XMMatrixMultiply(getMatrix(a, aTransposed), getMatrix(b, bTransposed));
}

//============================================================================
// Helper methods
//============================================================================
namespace
{
   XMMATRIX calcInverse(XMMATRIX m)
   {  
      XMVECTOR det;
      return XMMatrixInverse(&det, m);
      
      //BMatrix44* pSrc = (BMatrix44*)&m;
      //pSrc->invert();
      //return (XMMATRIX&)*pSrc;
   }
   
   XMVECTOR getTranslate(XMMATRIX m)
   {
      return m.r[3];
   }
   
   XMVECTOR getColumn(XMMATRIX m, int c)
   {
      return XMVectorSet(m(0, c), m(1, c), m(2, c), m(3, c));
   }
}   

//============================================================================
// BMatrixTracker::updateDirtyBasicMatrix
//============================================================================
void BMatrixTracker::updateDirtyBasicMatrix(eMTMatrix matrix)
{
   switch (matrix)
   {
      case cMTWorldToView:
      {
         setMatrixPair(cMTViewToWorld, calcInverse(getMatrix(cMTWorldToView)));
         setMatrixPair(cMTWorldToProj, concatMatrices(cMTWorldToView, cMTViewToProj));
         setMatrixPair(cMTWorldToScreen, concatMatrices(cMTWorldToProj, cMTProjToScreen));

         mWorldFrustum.set(getMatrix(cMTWorldToProj));
         mViewFrustum.set(getMatrix(cMTViewToProj));
         
         mWorldCamPos = getTranslate(getMatrix(cMTViewToWorld));
         
         break;
      }
      case cMTViewToProj:
      {
         setMatrixPair(cMTWorldToProj, concatMatrices(cMTWorldToView, cMTViewToProj));
         setMatrixPair(cMTWorldToScreen, concatMatrices(cMTWorldToProj, cMTProjToScreen));
         
         // screenToProj * projToView
         const XMMATRIX screenToProj(calcInverse(getMatrix(cMTProjToScreen)));
         const XMMATRIX projToView(calcInverse(getMatrix(cMTViewToProj)));
         setMatrixPair(cMTScreenToView, XMMatrixMultiply(screenToProj, projToView));
                           
         mWorldFrustum.set(getMatrix(cMTWorldToProj));
         mViewFrustum.set(getMatrix(cMTViewToProj));
                     
         break;
      }
      case cMTProjToScreen:
      {
         const XMMATRIX screenToProj(calcInverse(getMatrix(cMTProjToScreen)));
         const XMMATRIX projToView(calcInverse(getMatrix(cMTViewToProj)));
         
         setMatrixPair(cMTScreenToProj, screenToProj);
                           
         // screenToProj * projToView
         setMatrixPair(cMTScreenToView, XMMatrixMultiply(screenToProj, projToView));
         
         setMatrixPair(cMTWorldToScreen, concatMatrices(cMTWorldToProj, cMTProjToScreen));
         
         break;
      }
      default:
      {
         BDEBUG_ASSERT(0);
      }
   }
}

//============================================================================
// BMatrixTracker::setViewport
//============================================================================
void BMatrixTracker::setViewport(int x, int y, int width, int height, float minZ, float maxZ)
{
   mViewport.X       = x;
   mViewport.Y       = y;
   mViewport.Width   = width;
   mViewport.Height  = height;
   mViewport.MinZ    = minZ;
   mViewport.MaxZ    = maxZ;
   
   XMMATRIX projToScreen = XMMatrixSet(
      width * .5f,      0.0f,             0.0f,          0.0f,
      0.0f,             height * -.5f,    0.0f,          0.0f,
      0.0f,             0.0f,             maxZ - minZ,   0.0f,
      x + width * .5f,  y + height * .5f, minZ,          1.0f);
      
   setMatrixPair(cMTProjToScreen, projToScreen);
   
   updateDirtyBasicMatrix(cMTProjToScreen);
}

//============================================================================
// BMatrixTracker::setViewport
//============================================================================
void BMatrixTracker::setViewport(const D3DVIEWPORT9& viewport)
{
   setViewport(viewport.X, viewport.Y, viewport.Width, viewport.Height, viewport.MinZ, viewport.MaxZ);
}

//============================================================================
// BMatrixTracker::setViewToProjPersp
//============================================================================
void BMatrixTracker::setViewToProjPersp(float aspect, float zNear, float zFar, float fullFov)
{
   setMatrix(cMTViewToProj, XMMatrixPerspectiveFovLH(fullFov, aspect, zNear, zFar));
}

//============================================================================
// BMatrixTracker::setWorldToView
//============================================================================
void BMatrixTracker::setWorldToView(const XMVECTOR eye, const XMVECTOR focus, const XMVECTOR up)
{
   mWorldCamPos = eye;
   setMatrix(cMTWorldToView, XMMatrixLookAtLH(eye, focus, up));
}

//============================================================================
// BMatrixTracker::setViewToProjPersp
//============================================================================
void BMatrixTracker::setViewToProjPersp(
   float l, float r,
   float b, float t,
   float zNear, 
   float zFar)
{
   setMatrix(cMTViewToProj, XMMatrixPerspectiveOffCenterLH(l, r, b, t, zNear, zFar));
}

//============================================================================
// BMatrixTracker::setViewToProjOrtho
//============================================================================
void BMatrixTracker::setViewToProjOrtho(float l, float r, float b, float t, float zn, float zf)
{
   setMatrix(cMTViewToProj, XMMatrixOrthographicOffCenterLH(l, r, b, t, zn, zf));
}
   
//============================================================================
// BMatrixTracker::setMatrix
//============================================================================
void BMatrixTracker::setMatrix(eMTMatrix basicMatrixIndex, const XMMATRIX m)
{
   setMatrixPair(debugRangeCheck<eMTMatrix>(basicMatrixIndex, cMTNumBasicMatrices), m);
   updateDirtyBasicMatrix(basicMatrixIndex);
}

//============================================================================
// BMatrixTracker::getViewVector
//============================================================================   
XMVECTOR BMatrixTracker::getViewVector(const BVec2& screen) const
{
   return XMVector3Normalize(XMVector3TransformCoord(XMVectorSet(screen[0], screen[1], 0.0f, 1.0f), getMatrix(cMTScreenToView)));
}
      
//============================================================================
// BMatrixTracker::getViewToProjMul
//============================================================================
XMVECTOR BMatrixTracker::getViewToProjMul(void) const
{
   const XMMATRIX viewToProj = getMatrix(cMTViewToProj);
   return XMVectorSet(viewToProj(0,0), viewToProj(1,1), viewToProj(2,2), viewToProj(2,3));
}

//============================================================================
// BMatrixTracker::getViewToProjAdd
//============================================================================
XMVECTOR BMatrixTracker::getViewToProjAdd(void) const
{
   return getMatrix(cMTViewToProj).r[3];
}

//============================================================================
// Frustum vertices
//============================================================================
namespace
{
   static const XMVECTOR frustVerts[] = 
   {
      {-1,-1,0,1},
      {1,-1,0,1},
      {1,1,0,1},
      {-1,1,0,1},
      
      {-1,-1,1,1},
      {1,-1,1,1},
      {1,1,1,1},
      {-1,1,1,1}
   };
}

//============================================================================
// BMatrixTracker::getFrustumVertices
//============================================================================
void BMatrixTracker::getFrustumVertices(XMVECTOR* RESTRICT pVerts, bool viewspace) const
{
   BDEBUG_ASSERT(pVerts);
   
   XMMATRIX transform = calcInverse(getMatrix(viewspace ? cMTViewToProj : cMTWorldToProj));
   
   for (uint i = 0; i < 8; i++)
      pVerts[i] = XMVector3TransformCoord(frustVerts[i], transform);
}

//============================================================================
// BMatrixTracker::getFrustumPlanes
//============================================================================
void BMatrixTracker::getFrustumPlanes(XMVECTOR* RESTRICT pPlanes, bool viewspace) const
{
   const XMMATRIX proj = getMatrix(viewspace ? cMTViewToProj : cMTWorldToProj, true);
   
   XMVECTOR sign = XMVectorSplatOne();
   
   for (int i = 0; i < cNumFrustumPlanes; i++)
   {
      XMVECTOR p = proj.r[i >> 1] * sign;
      
      if (i != 4) // Z near
         p += proj.r[3];
      
      p = p * XMVector3ReciprocalLength(p);
      
      pPlanes[i] = p;
      
      sign = XMVectorNegate(sign);
   }
}   

//============================================================================
// Helpers
//============================================================================
namespace
{
   template<class T>
   void writeObj(uchar*& pDst, const T& obj)
   {
      memcpy(pDst, &obj, sizeof(obj));
      pDst += sizeof(obj);
   }

   template<class T>
   void readObj(const uchar*& pSrc, T& obj)
   {
      memcpy(&obj, pSrc, sizeof(obj));
      pSrc += sizeof(obj);
   }
}

//============================================================================
// BMatrixTracker::getSerializeSize
//============================================================================
uint BMatrixTracker::getSerializeSize(void)
{
   return sizeof(XMMATRIX) * 2 + sizeof(BVec4) + sizeof(D3DVIEWPORT9);
}

//============================================================================
// BMatrixTracker::serialize
//============================================================================
void BMatrixTracker::serialize(uchar* pDst) const
{
   writeObj(pDst, mMatrices[cMTWorldToView][0]);
   writeObj(pDst, mMatrices[cMTViewToProj][0]);
   writeObj(pDst, mWorldCamPos);
   writeObj(pDst, mViewport);
}

//============================================================================
// BMatrixTracker::deserialize
//============================================================================
void BMatrixTracker::deserialize(const uchar* pSrc)
{
   readObj(pSrc, mMatrices[cMTWorldToView][0]);
   readObj(pSrc, mMatrices[cMTViewToProj][0]);
   mMatrices[cMTWorldToView][1] = XMMatrixTranspose(mMatrices[cMTWorldToView][0]);
   mMatrices[cMTViewToProj][1] = XMMatrixTranspose(mMatrices[cMTViewToProj][0]);
   
   readObj(pSrc, mWorldCamPos);
   
   setMatrixPair(cMTViewToWorld, calcInverse(getMatrix(cMTWorldToView)));
   setMatrixPair(cMTWorldToProj, concatMatrices(cMTWorldToView, cMTViewToProj));
      
   mWorldFrustum.set(getMatrix(cMTWorldToProj));
   mViewFrustum.set(getMatrix(cMTViewToProj));
      
   D3DVIEWPORT9 viewport;
   readObj(pSrc, viewport);
   setViewport(viewport);
}

//============================================================================
// BMatrixTracker::getWorldAt
//============================================================================
XMVECTOR BMatrixTracker::getWorldAt(void) const 
{ 
   return getMatrix(cMTWorldToView, true).r[2];
}

//============================================================================
// BMatrixTracker::getWorldAtVec4
//============================================================================
const BVec4& BMatrixTracker::getWorldAtVec4(void) const 
{ 
   return *reinterpret_cast<const BVec4*>(&getMatrix(cMTWorldToView, true).r[2]);
}

//============================================================================
// BMatrixTracker::getWorldUp
//============================================================================
XMVECTOR BMatrixTracker::getWorldUp(void) const 
{ 
   return getMatrix(cMTWorldToView, true).r[1];
}

//============================================================================
// BMatrixTracker::getWorldRight
//============================================================================
XMVECTOR BMatrixTracker::getWorldRight(void) const 
{ 
   return getMatrix(cMTWorldToView, true).r[0];
}   
