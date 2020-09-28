//==============================================================================
// debugprimitives.cpp
//
// Copyright (c) 2003-2006, Ensemble Studios
//
// FIXME: Disable this in final builds!
//
//==============================================================================

// Includes
#include "xgamerender.h"
#include "debugprimitives.h"
#include "renderprimitiveutility.h"
#include "xboxmath.h"

//-- rendering
#include "renderThread.h"
#include "renderDraw.h"
#include "vertexTypes.h"
#include "fixedFuncShaders.h"
#include "render.h"
#include "renderViewParams.h"

//==============================================================================
// Globals
//==============================================================================
__declspec(thread) BDebugPrimitives* gpDebugPrimitives;
BDebugPrimitives* gpRenderDebugPrimitives = NULL;

#ifdef BUILD_FINAL
const uint cMaxLinePrimitives        = 1;
const uint cMaxThickLinePrimitives   = 1;
const uint cMaxTextPrimitives        = 1;
const uint cMaxCirclePrimitives      = 1;
const uint cMaxThickCirclePrimitives = 1;
const uint cMaxArrowPrimitives       = 1;
const uint cMaxSpherePrimitives      = 1;
const uint cMaxBoxPrimitives         = 1;
#else
//const uint cMaxLinePrimitives        = 8192;
// jce [10/2/2008] -- bumping this way up for terrain obstruction rendering.  If it causes an issue
// we can try to do something else with it.  It's only a cap to a dynamic array so all that space
// shouldn't get used in a normal game with turning on debugging displays.
const uint cMaxLinePrimitives        = 32768;

const uint cMaxThickLinePrimitives   = 4096;
const uint cMaxTextPrimitives        = 2048;
const uint cMaxCirclePrimitives      = 2048;
const uint cMaxThickCirclePrimitives = 2048;
const uint cMaxArrowPrimitives       = 2048;
const uint cMaxSpherePrimitives      = 2048;
const uint cMaxBoxPrimitives         = 2048;
#endif

//==============================================================================
// BDebugPrimitives::BDebugPrimitives
//==============================================================================
BDebugPrimitives::BDebugPrimitives() :
   mCurTime(0.0f),
   mThreadSafeFlag(false)
{
   BCOMPILETIMEASSERT(sizeof(BPrimitiveText) == 32*4);
}

//==============================================================================
// BDebugPrimitives::~BDebugPrimitives
//==============================================================================
BDebugPrimitives::~BDebugPrimitives()
{
}

//==============================================================================
// BDebugPrimitives::init
//==============================================================================
void BDebugPrimitives::init()
{
   ASSERT_MAIN_OR_WORKER_THREAD
      
   gRenderPrimitiveUtility.init();
}

//==============================================================================
// BDebugPrimitives::deInit
//==============================================================================
void BDebugPrimitives::deInit()
{   
   ASSERT_MAIN_OR_WORKER_THREAD 
   
   gRenderPrimitiveUtility.deInit();
 }

//==============================================================================
// BDebugPrimitives::addDebugLine
//==============================================================================
bool  BDebugPrimitives::addDebugLine(BVector point, BVector point2, DWORD color1, DWORD color2, int category, float timeout)
{
   if (mThreadSafeFlag)
      mMutex.lock();

   if (mLinePrimitives.getSize() >= cMaxLinePrimitives)
   {
      if (mThreadSafeFlag)
         mMutex.unlock();

      return false;
   }

   BPrimitiveLine* pPrim = mLinePrimitives.pushBackNoConstruction(1);
   BDEBUG_ASSERT(pPrim);
      
   pPrim->mPoint1 = point;
   pPrim->mPoint2 = point2;
   pPrim->mColor1 = color1;
   pPrim->mColor2 = color2;
   pPrim->mThickness = 0.0f;
   pPrim->mCategory = (BYTE)category;   
   pPrim->mTimeout = createTimeout(timeout);

   if (mThreadSafeFlag)
      mMutex.unlock();
   
   return true;
}

//==============================================================================
// BDebugPrimitives::addDebugThickLine
//==============================================================================
bool  BDebugPrimitives::addDebugThickLine(BVector point, BVector point2, float thickness, DWORD color1, DWORD color2, int category, float timeout)
{
   if (mThreadSafeFlag)
      mMutex.lock();

   if (mThickLinePrimitives.getSize() >= cMaxThickLinePrimitives)
   {
      if (mThreadSafeFlag)
         mMutex.unlock();
      return false;
   }

   BPrimitiveLine* pPrim = mThickLinePrimitives.pushBackNoConstruction(1);
   BDEBUG_ASSERT(pPrim);

   pPrim->mPoint1 = point;
   pPrim->mPoint2 = point2;
   pPrim->mColor1 = color1;
   pPrim->mColor2 = color2;
   pPrim->mThickness = thickness;
   pPrim->mCategory = (BYTE)category;
   pPrim->mTimeout = createTimeout(timeout);

   if (mThreadSafeFlag)
      mMutex.unlock();
   
   return true;
}

//==============================================================================
// BDebugPrimitives::addDebugText
//==============================================================================
bool  BDebugPrimitives::addDebugText(const char* pText, BVector point, float size, DWORD color, int category, bool zTest, float timeout)
{
   if (mThreadSafeFlag)
      mMutex.lock();

   BDEBUG_ASSERT(pText);
   
   if (mTextPrimitives.getSize() >= cMaxTextPrimitives)
   {
      if (mThreadSafeFlag)
         mMutex.unlock();
      return false;
   }
         
   //const XMVECTOR scaleVec = XMVectorSet(size, size, size, 1.0f);
   
   BPrimitiveText* pPrim = mTextPrimitives.pushBackNoConstruction(1);

   pPrim->mPos[0] = point.x;
   pPrim->mPos[1] = point.y;
   pPrim->mPos[2] = point.z;
   pPrim->mSize = size;         
      
   pPrim->mColor = color;
   pPrim->mText.set(pText);
   pPrim->mTimeout = createTimeout(timeout);
   pPrim->mCategory = static_cast<BYTE>(category);
   pPrim->mZTest = zTest;


   if (mThreadSafeFlag)
      mMutex.unlock();

   return true;   
}

//==============================================================================
// BDebugPrimitives::addDebugCircle
//==============================================================================
bool BDebugPrimitives::addDebugCircle(BMatrix matrix, float radius, DWORD color, int category, float timeout)
{
   if (mThreadSafeFlag)
      mMutex.lock();
         
   if (mCirclePrimitives.getSize() >= cMaxCirclePrimitives)
   {
      if (mThreadSafeFlag)
         mMutex.unlock();

      return false;
   }

   BPrimitiveSphere* pPrim = mCirclePrimitives.pushBackNoConstruction(1);

   BDEBUG_ASSERT(pPrim);
   pPrim->mMatrix = matrix;
   pPrim->mRadius = radius;
   pPrim->mColor  = color;
   pPrim->mTimeout= createTimeout(timeout);
   pPrim->mCategory=(BYTE)category;

   if (mThreadSafeFlag)
      mMutex.unlock();
   
   return true;
}

//==============================================================================
// BDebugPrimitives::addDebugCircle
//==============================================================================
bool BDebugPrimitives::addDebugThickCircle(BMatrix matrix, float radius, float thickness, DWORD color, int category, float timeout)
{
   if (mThreadSafeFlag)
      mMutex.lock();

   static bool useLines = false;
   if (useLines)
   {
      const long cNumObjectSelectPoints=48;
      BVector points[cNumObjectSelectPoints];
      BVector position;
      matrix.getTranslation(position);

      float radiansPerSegment = Math::fTwoPi / cNumObjectSelectPoints;
      float angularOffset = -XM_PI;

      float sinAngle, cosAngle;
      for (long segment = 0; segment < cNumObjectSelectPoints; ++segment)
      {
         BVector& pt=points[segment];

         XMScalarSinCosEst(&sinAngle, &cosAngle, angularOffset);

         pt.set(sinAngle * radius + position.x, position.y, cosAngle * radius + position.z);
         angularOffset += radiansPerSegment;
      }

      for(long i=0; i<cNumObjectSelectPoints; i++)
         addDebugThickLine(points[i], (i==cNumObjectSelectPoints-1?points[0]:points[i+1]), thickness, color, color, category, timeout);
   }
   else
   {
      if (mThickCirclePrimitives.getSize() >= cMaxThickCirclePrimitives)
      {
         if (mThreadSafeFlag)
            mMutex.unlock();
         return false;
      }

      BPrimitiveSphere* pPrim = mThickCirclePrimitives.pushBackNoConstruction(1);

      BDEBUG_ASSERT(pPrim);
      pPrim->mMatrix = matrix;
      pPrim->mRadius = radius;
      pPrim->mColor  = color;
      pPrim->mThickness = thickness;
      pPrim->mTimeout= createTimeout(timeout);
      pPrim->mCategory=(BYTE)category;
   }

   if (mThreadSafeFlag)
      mMutex.unlock();
   return true;
}

//==============================================================================
// BDebugPrimitives::addDebugSphere
//==============================================================================
bool  BDebugPrimitives::addDebugSphere(BMatrix matrix, float radius, DWORD color1, int category, float timeout)
{
   if (mThreadSafeFlag)
      mMutex.lock();
   
   if (mSpherePrimitives.getSize() >= cMaxSpherePrimitives)
   {
      if (mThreadSafeFlag)
         mMutex.unlock();

      return false;
   }

   BPrimitiveSphere* pPrim = mSpherePrimitives.pushBackNoConstruction(1);

   BDEBUG_ASSERT(pPrim);
   pPrim->mMatrix = matrix;
   pPrim->mRadius = radius;
   pPrim->mColor  = color1;
   pPrim->mTimeout= createTimeout(timeout);
   pPrim->mCategory=(BYTE)category;

   if (mThreadSafeFlag)
      mMutex.unlock();
   
   return true;
}

//==============================================================================
// BDebugPrimitives::addDebugSphere
//==============================================================================
bool  BDebugPrimitives::addDebugSphere(BVector origin, float radius, DWORD color1, int category, float timeout)
{
   if (mThreadSafeFlag)
      mMutex.lock();
   
   BMatrix matrix;
   matrix.r[0] = cXAxisVector;
   matrix.r[1] = cYAxisVector;
   matrix.r[2] = cZAxisVector;
   matrix.setTranslation(origin);
   
   bool result = addDebugSphere(matrix, radius, color1, category, timeout);

   if (mThreadSafeFlag)
      mMutex.unlock();

   return result;
}

//==============================================================================
// BDebugPrimitives::addDebugBox
//==============================================================================
bool  BDebugPrimitives::addDebugBox(BMatrix matrix, BVector scale, DWORD color, int category, float timeout)
{
   if (mThreadSafeFlag)
      mMutex.lock();
   
   if (mBoxPrimitives.getSize() >= cMaxBoxPrimitives)
   {
      if (mThreadSafeFlag)
         mMutex.unlock();

      return false;
   }
      
   BPrimitiveBox* pPrim = mBoxPrimitives.pushBackNoConstruction(1);
   BDEBUG_ASSERT(pPrim);
   pPrim->mMatrix = matrix;
   pPrim->mScale  = scale;
   pPrim->mColor  = color;
   pPrim->mTimeout = createTimeout(timeout);
   pPrim->mCategory = (BYTE)category;

   if (mThreadSafeFlag)
      mMutex.unlock();
   
   return true;
}

//==============================================================================
// BDebugPrimitives::addDebugBox
//==============================================================================
bool  BDebugPrimitives::addDebugBox(BVector min, BVector max, DWORD color1, int category, float timeout)
{
   if (mThreadSafeFlag)
      mMutex.lock();
   
   BMatrix matrix;

   matrix.r[0] = cXAxisVector;
   matrix.r[1] = cYAxisVector;
   matrix.r[2] = cZAxisVector;
   
   BVector center( (min + max) * .5f );
   matrix.setTranslation(center);
   
   bool result = gpDebugPrimitives->addDebugBox(matrix, max - center, color1, category, timeout);

   if (mThreadSafeFlag)
      mMutex.unlock();

   return result;
}

//==============================================================================
// BDebugPrimitives::addDebugArrow
//==============================================================================
bool  BDebugPrimitives::addDebugArrow(BMatrix matrix, BVector scale, DWORD color, int category, float timeout)
{
   if (mThreadSafeFlag)
      mMutex.lock();
   
   if (mArrowPrimitives.getSize() >= cMaxArrowPrimitives)
   {
      if (mThreadSafeFlag)
         mMutex.unlock();
      return false;
   }

   BPrimitiveBox* pPrim = mArrowPrimitives.pushBackNoConstruction(1);
   BDEBUG_ASSERT(pPrim);
   pPrim->mMatrix = matrix;
   pPrim->mScale  = scale;
   pPrim->mColor  = color;
   pPrim->mTimeout = createTimeout(timeout);
   pPrim->mCategory = (BYTE)category;

   if (mThreadSafeFlag)
      mMutex.unlock();
   
   return true;
}

//==============================================================================
// BDebugPrimitives::addDebugAxis
//==============================================================================
bool  BDebugPrimitives::addDebugAxis(BMatrix matrix, float scale, int category, float timeout)
{
   if (mThreadSafeFlag)
      mMutex.lock();

   BVector point, forward, up, right;
   matrix.getTranslation(point);
   matrix.getForward(forward);
   matrix.getUp(up);
   matrix.getRight(right);

   if (!addDebugLine(point, point+(forward*scale), cDWORDBlue, cDWORDBlue, category, timeout))
   {
      if (mThreadSafeFlag)
         mMutex.unlock();
      return false;
   }

   if (!addDebugLine(point, point+(up*scale), cDWORDGreen, cDWORDGreen, category, timeout))
   {
      if (mThreadSafeFlag)
         mMutex.unlock();
      return false;
   }

   if (!addDebugLine(point, point+(right*scale), cDWORDRed, cDWORDRed, category, timeout))
   {
      if (mThreadSafeFlag)
         mMutex.unlock();
      return false;
   }

   if (mThreadSafeFlag)
      mMutex.unlock();
   return true;
}

//==============================================================================
// BDebugPrimitives::addDebugAxis
//==============================================================================
bool BDebugPrimitives::addDebugAxis(BVector pos, BVector right, BVector up, BVector at, float scale, int category, float timeout)
{
   if (mThreadSafeFlag)
      mMutex.lock();

   BMatrix matrix;
   XMVECTOR zero = XMVectorZero();
   matrix.r[0] = __vrlimi(right, zero, 1, 0);
   matrix.r[1] = __vrlimi(up, zero, 1, 0);
   matrix.r[2] = __vrlimi(at, zero, 1, 0);
   matrix.r[3] = __vrlimi(pos, XMVectorSplatOne(), 1, 0);

   bool result =  addDebugAxis(matrix, scale, category, timeout);

   if (mThreadSafeFlag)
      mMutex.unlock();

   return result;
}

//==============================================================================
// BDebugPrimitives::addDebugCylinder
//==============================================================================
bool BDebugPrimitives::addDebugCylinder(BMatrix matrix, float radius, float height, DWORD color, int category, float timeout)
{
   if (mThreadSafeFlag)
      mMutex.lock();

   BMatrix rotation = XMMatrixIdentity();
   BMatrix tempM    = matrix;
   rotation.r[0] = matrix.r[0];
   rotation.r[1] = matrix.r[1];
   rotation.r[2] = matrix.r[2];

   XMVECTOR heightV = XMVectorSplatX(XMLoadScalar(&height));
   XMVECTOR radiusV = XMVectorSplatX(XMLoadScalar(&radius));
   
   XMVECTOR axisV;
   axisV = XMVectorMultiply(cYAxisVector, heightV);
   axisV = XMVectorSelect(XMVector3Transform(axisV, rotation), XMVectorZero(), XMVectorSelectControl(0,0,0,1));

   BVector pointA = matrix.r[3];
   BVector pointB = XMVectorAdd(pointA, axisV);
                     
   if (!addDebugLine(pointA, pointB, color, color, category, timeout))
   {
      if (mThreadSafeFlag)
         mMutex.unlock();

      return false;
   }
   
   tempM.r[3] = XMVectorSelect(pointA, XMVectorSet(1,1,1,1), XMVectorSelectControl(0,0,0,1));
   if (!addDebugCircle(tempM, radius, color, category, timeout))
   {
      if (mThreadSafeFlag)
         mMutex.unlock();

      return false;
   }

   XMVECTOR offset = XMVector3Transform(XMVectorMultiply(cXAxisVector, radiusV), rotation);
   BVector temp1 = XMVectorSubtract(pointA, offset);
   BVector temp2 = XMVectorSubtract(pointB, offset);
   if (!addDebugLine(temp1, temp2, color, color, category, timeout))
   {
      if (mThreadSafeFlag)
         mMutex.unlock();

      return false;
   }

   temp1 = XMVectorAdd(pointA, offset);
   temp2 = XMVectorAdd(pointB, offset);
   if (!addDebugLine(temp1, temp2, color, color, category, timeout))
   {
      if (mThreadSafeFlag)
         mMutex.unlock();

      return false;
   }

   tempM.r[3] = XMVectorSelect(XMVectorLerp(pointA, pointB, 0.5f), XMVectorSet(1,1,1,1), XMVectorSelectControl(0,0,0,1));
   if (!addDebugCircle(tempM, radius, color, category, timeout))
   {
      if (mThreadSafeFlag)
         mMutex.unlock();

      return false;
   }

   tempM.r[3] = XMVectorSelect(pointB, XMVectorSet(1,1,1,1), XMVectorSelectControl(0,0,0,1));
   if (!addDebugCircle(tempM, radius, color, category, timeout))
   {
      if (mThreadSafeFlag)
         mMutex.unlock();

      return false;
   }

   offset = XMVector3Transform(XMVectorMultiply(cZAxisVector, radiusV), rotation);
   temp1 = XMVectorSubtract(pointA, offset);
   temp2 = XMVectorSubtract(pointB, offset);
   if (!addDebugLine(temp1, temp2, color, color, category, timeout))
   {
      if (mThreadSafeFlag)
         mMutex.unlock();

      return false;
   }

   temp1 = XMVectorAdd(pointA, offset);
   temp2 = XMVectorAdd(pointB, offset);
   if (!addDebugLine(temp1, temp2, color, color, category, timeout))
   {
      if (mThreadSafeFlag)
         mMutex.unlock();

      return false;
   }

   if (mThreadSafeFlag)
      mMutex.unlock();

   return true;
}

//==============================================================================
// BDebugPrimitives::addDebugScreenLines
//==============================================================================
void BDebugPrimitives::addDebugScreenLines(const BVector* screenPoints, long pointCount, DWORD color, int category, float timeout)
{
   if (mThreadSafeFlag)
      mMutex.lock();

   // rg - [8/16/06] - hack, only support this on the sim thread for now because of gRender usage
   ASSERT_MAIN_THREAD
   
   float vphw=gRender.getViewParams().getViewportHalfWidth();
   float vphh=gRender.getViewParams().getViewportHalfHeight();
   float vpsx=gRender.getViewParams().getScaleX();
   float vpsy=gRender.getViewParams().getScaleY();

   BMatrix matrix=gRender.getViewParams().getViewBMatrix();
   matrix.invert();

   BVector worldPoint[2];
   for(long i=0; i<pointCount-1; i+=2)
   {
      for(long j=0; j<2; j++)
      {
         const BVector& sp=screenPoints[i+j];
         BVector cp;
         cp.z=1.0f/sp.z;
         cp.x=((sp.x-vphw)/sp.z)/vpsx;
         cp.y=-(((sp.y-vphh)/sp.z)/vpsy);
         matrix.transformVectorAsPoint(cp, worldPoint[j]);
      }      
      addDebugLine(worldPoint[0], worldPoint[1], color, color, category, timeout);
   }

   if (mThreadSafeFlag)
      mMutex.unlock();
}

//==============================================================================
// BDebugPrimitives::addDebugScreenLine
//==============================================================================
void BDebugPrimitives::addDebugScreenLine(BVector p1, BVector p2, DWORD color, int category, float timeout)
{
   if (mThreadSafeFlag)
      mMutex.lock();

   BVector screenPoints[2];
   screenPoints[0]=p1;
   screenPoints[1]=p2;
   addDebugScreenLines(screenPoints, 2, color, category, timeout);

   if (mThreadSafeFlag)
      mMutex.unlock();
}

//==============================================================================
// BDebugPrimitives::addDebugScreenRect
//==============================================================================
void BDebugPrimitives::addDebugScreenRect(float left, float top, float right, float bottom, DWORD color, int category, float timeout)
{
   if (mThreadSafeFlag)
      mMutex.lock();

   BVector screenPoints[8];
   screenPoints[0].set(left, top, 1.0f);;
   screenPoints[1].set(right, top, 1.0f);
   screenPoints[2].set(right, top, 1.0f);
   screenPoints[3].set(right, bottom, 1.0f);
   screenPoints[4].set(right, bottom, 1.0f);
   screenPoints[5].set(left, bottom, 1.0f);
   screenPoints[6].set(left, bottom, 1.0f);
   screenPoints[7].set(left, top, 1.0f);
   addDebugScreenLines(screenPoints, 8, color, category, timeout);

   if (mThreadSafeFlag)
      mMutex.unlock();
}

//==============================================================================
// BDebugPrimitives::addDebugScreenRect
//==============================================================================
void BDebugPrimitives::addDebugScreenRect(const BVector* screenPoints, DWORD color, int category, float timeout)
{
   if (mThreadSafeFlag)
      mMutex.lock();

   // rg - [8/16/06] - hack, only support this on the sim thread for now because of gRender usage
   
   float vphw=gRender.getViewParams().getViewportHalfWidth();
   float vphh=gRender.getViewParams().getViewportHalfHeight();
   float vpsx=gRender.getViewParams().getScaleX();
   float vpsy=gRender.getViewParams().getScaleY();

   BMatrix matrix=gRender.getViewParams().getViewBMatrix();
   matrix.invert();

   BVector worldPoint[2];
   for(long i=0; i<4; i++)
   {
      for(long j=0; j<2; j++)
      {
         long pi=i+j;
         if(pi>=4)
            pi=0;
         const BVector& sp=screenPoints[pi];
         BVector cp;
         cp.z=1.0f/sp.z;
         cp.x=((sp.x-vphw)/sp.z)/vpsx;
         cp.y=-(((sp.y-vphh)/sp.z)/vpsy);
         matrix.transformVectorAsPoint(cp, worldPoint[j]);
      }      
      gpDebugPrimitives->addDebugLine(worldPoint[0], worldPoint[1], color, color, category, timeout);
   }

   if (mThreadSafeFlag)
      mMutex.unlock();
}

//==============================================================================
// BDebugPrimitives::addDebugScreenCircle
//==============================================================================
void BDebugPrimitives::addDebugScreenCircle(BVector up, float uxr, float uyr, DWORD color, int category, float timeout)
{
   if (mThreadSafeFlag)
      mMutex.lock();

   const cNumSegments=16;
   BVector circlePoints[cNumSegments];
   float radiansPerSegment = cTwoPi / cNumSegments;
   float angularOffset = 0.0f;
   for(long j=0; j<cNumSegments; j++)
   {
      BVector& pt=circlePoints[j];
      float sinAngle = (float)sin(angularOffset);
      float cosAngle = (float)cos(angularOffset);
      pt.set(sinAngle * uxr + up.x, cosAngle * uyr + up.y, 1.0f);
      angularOffset += radiansPerSegment;
   }
   BVector screenPoints2[cNumSegments*2];
   for(long j=0; j<cNumSegments; j++)
   {
      screenPoints2[j*2]=circlePoints[j];
      screenPoints2[j*2+1]=(j==cNumSegments-1?circlePoints[0]:circlePoints[j+1]);
   }
   addDebugScreenLines(screenPoints2, cNumSegments*2, color, category, timeout);

   if (mThreadSafeFlag)
      mMutex.unlock();
}

//==============================================================================
// BDebugPrimitives::addDebugScreenTriangle
//==============================================================================
void BDebugPrimitives::addDebugScreenTriangle(BVector p1, BVector p2, BVector p3, DWORD color, int category, float timeout)
{
   if (mThreadSafeFlag)
      mMutex.lock();

   BVector screenPoints[6];
   screenPoints[0]=p1;
   screenPoints[1]=p2;
   screenPoints[2]=p2;
   screenPoints[3]=p3;
   screenPoints[4]=p3;
   screenPoints[5]=p1;
   addDebugScreenLines(screenPoints, 6, color, category, timeout);

   if (mThreadSafeFlag)
      mMutex.unlock();
}

//==============================================================================
// BDebugPrimitives::addDebugScreenPolygon
//==============================================================================
void BDebugPrimitives::addDebugScreenPolygon(BVector cp, BVector* tps, long pointCount, DWORD color, int category, float timeout)
{
   if (mThreadSafeFlag)
      mMutex.lock();

   if(pointCount>64)
      return;
   BVector screenPoints[64*6];
   for(long i=0; i<pointCount; i++)
   {
      screenPoints[i*6]=cp;
      screenPoints[i*6+1]=tps[i];
      screenPoints[i*6+2]=cp;
      screenPoints[i*6+3]=(i==pointCount-1?tps[0]:tps[i+1]);
      screenPoints[i*6+4]=tps[i];
      screenPoints[i*6+5]=(i==pointCount-1?tps[0]:tps[i+1]);
   }
   addDebugScreenLines(screenPoints, pointCount*6, color, category, timeout);

   if (mThreadSafeFlag)
      mMutex.unlock();
}

//==============================================================================
// BDebugPrimitives::clear
//==============================================================================
void BDebugPrimitives::clear(int category /*= -1*/)
{
   if (mThreadSafeFlag)
      mMutex.lock();

   BDEBUG_ASSERT(category < cNumberCategories);

   if (category == -1)
   {
      mBoxPrimitives.resize(0);
      mCirclePrimitives.resize(0);
      mSpherePrimitives.resize(0);
      mLinePrimitives.resize(0);
      mArrowPrimitives.resize(0);
      mTextPrimitives.resize(0);
      mThickLinePrimitives.resize(0);
      mThickCirclePrimitives.resize(0);

      if (mThreadSafeFlag)
         mMutex.unlock();

      return;
   }

   clearPrimitives(mBoxPrimitives, category);
   clearPrimitives(mArrowPrimitives, category);
   clearPrimitives(mCirclePrimitives, category);
   clearPrimitives(mSpherePrimitives, category);
   clearPrimitives(mLinePrimitives, category);
   clearPrimitives(mTextPrimitives, category);
   clearPrimitives(mThickLinePrimitives, category);
   clearPrimitives(mThickCirclePrimitives, category);

   if (mThreadSafeFlag)
      mMutex.unlock();
}

//==============================================================================
// BDebugPrimitives::beginFrame(float curTime)
//==============================================================================
void BDebugPrimitives::beginFrame(double curTime)
{
   if (mThreadSafeFlag)
      mMutex.lock();

   mCurTime = curTime;   
   updatePrimitives(mBoxPrimitives);
   updatePrimitives(mArrowPrimitives);
   updatePrimitives(mCirclePrimitives);
   updatePrimitives(mSpherePrimitives);
   updatePrimitives(mLinePrimitives);   
   updatePrimitives(mTextPrimitives);
   updatePrimitives(mThickLinePrimitives);
   updatePrimitives(mThickCirclePrimitives);

   if (mThreadSafeFlag)
      mMutex.unlock();
}

//==============================================================================
// BDebugPrimitives::endFrame(void)
//==============================================================================
void BDebugPrimitives::endFrame(void)
{
   if (mThreadSafeFlag)
      mMutex.lock();
   
   gRenderPrimitiveUtility.update(BRenderPrimitiveUtility::eBRUUpdateLine, mLinePrimitives.getSize(), mLinePrimitives.getPtr());
   gRenderPrimitiveUtility.update(BRenderPrimitiveUtility::eBRUUpdateBox, mBoxPrimitives.getSize(), mBoxPrimitives.getPtr());
   gRenderPrimitiveUtility.update(BRenderPrimitiveUtility::eBRUUpdateCircle, mCirclePrimitives.getSize(), mCirclePrimitives.getPtr());   
   gRenderPrimitiveUtility.update(BRenderPrimitiveUtility::eBRUUpdateArrow, mArrowPrimitives.getSize(), mArrowPrimitives.getPtr());   
   gRenderPrimitiveUtility.update(BRenderPrimitiveUtility::eBRUUpdateSphere, mSpherePrimitives.getSize(), mSpherePrimitives.getPtr());
   gRenderPrimitiveUtility.update(BRenderPrimitiveUtility::eBRUUpdateText, mTextPrimitives.getSize(), mTextPrimitives.getPtr());
   gRenderPrimitiveUtility.update(BRenderPrimitiveUtility::eBRUUpdateThickLine, mThickLinePrimitives.getSize(), mThickLinePrimitives.getPtr());
   gRenderPrimitiveUtility.update(BRenderPrimitiveUtility::eBRUUpdateThickCircle, mThickCirclePrimitives.getSize(), mThickCirclePrimitives.getPtr());

   if (mThreadSafeFlag)
      mMutex.lock();
}

//==============================================================================
// template<class T> void BDebugPrimitives::updatePrimitives(T& list)
//==============================================================================
template<class T> void BDebugPrimitives::updatePrimitives(T& list)
{
   if (list.empty())
      return;
      
   int n = list.getSize();
   
   for (int i = 0; i < n; i++)
   {
      //-- is this primitive dead?
      if (mCurTime >= list[i].mTimeout)
      {
         list[i] = list.back();
         list.popBack();
         n--;
         i--;
      }
   }
}

//==============================================================================
// template<class T> void BDebugPrimitives::clearPrimitives(T& list, int category);
//==============================================================================
template<class T> void BDebugPrimitives::clearPrimitives(T& list, int category)
{
   for (uint i = 0; i < list.getSize();i++)
   {      
      //-- is this primitive dead?
      if ((list[i].mCategory == category))
      {
         list[i] = list.back();
         list.popBack();
         i--;
      }
   }
}

//==============================================================================
// BDebugPrimitives::workerRender
//==============================================================================
void BDebugPrimitives::workerRender(void)
{
   gRenderPrimitiveUtility.workerRender();
}

//==============================================================================
// BDebugPrimitives::createTimeout
//==============================================================================
float BDebugPrimitives::createTimeout(float durationInSeconds)
{
   if (durationInSeconds <= 0.0f)
      return -1.0f;

   // rg [11/21/06] - This is converting absolute time from double to float!
   float timeOut = static_cast<float>(mCurTime + durationInSeconds);
   return timeOut;
}
