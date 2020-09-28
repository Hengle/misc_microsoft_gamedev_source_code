//============================================================================
// debugprimitives.h
//
// Ensemble Studios, Copyright (c) 2002-2006
//============================================================================
#pragma once

#include "math\vector.h"
#include "math\matrix.h"
#include "color.h"

//==============================================================================
// class BPrimitiveBox
//==============================================================================
class BPrimitiveBox
{
public: 
   BMatrix mMatrix;
   BVector mScale;
   DWORD   mColor;
   float   mTimeout;
   BYTE    mCategory;
};

//==============================================================================
// class BPrimitiveLine
//==============================================================================
class BPrimitiveLine
{
public: 
   BVector mPoint1;
   BVector mPoint2;
   DWORD   mColor1;
   DWORD   mColor2;
   float   mTimeout;
   float   mThickness;
   BYTE    mCategory;
};

//==============================================================================
// class BPrimitiveSphere
//==============================================================================
class BPrimitiveSphere
{
public:
   BMatrix mMatrix;
   float   mRadius;
   DWORD   mColor;
   float   mTimeout;
   float   mThickness; //-- used for thick circles
   BYTE    mCategory;
};

//==============================================================================
// class BPrimitiveText
//==============================================================================
class BPrimitiveText
{
public:
   float mPos[3];
   float mSize;
   float mTimeout;
   DWORD mColor;
   BYTE mCategory;
   BYTE mZTest;
   BFixedString<102> mText;
};

//==============================================================================
// class BDebugPrimitives
//==============================================================================
class BDebugPrimitives 
{
public:

   enum
   {
      cCategoryNone = 0,
      cCategoryObject,
      cCategoryTest,
      cCategoryPhysics,
      cCategoryFormations,
      cCategoryPathing,
      cCategoryMovement,
      cCategoryParticles,
      cCategoryCinematics,
      cCategoryAI,
      cCategoryControls,

      // Pather categories
      cCategoryPatherLowLevelPather,
      cCategoryPatherHulls,
      cCategoryPatherQuadPather,
      cCategoryPatherUnobstructedPoints,
      cCategoryPatherStartEndPoints,
      cCategoryPatherPostHullPather,
      cCategoryPatherHulledArea,
      cCategoryPatherHulledItems,
      cCategoryPatherPostHulls,
      cCategoryPatherNonConvex,
      cCategoryPatherNonConvex2,
      cCategoryPatherNewPolyStart,
      cCategoryPatherPathNodes,
      cCategoryPatherPostNchs,

      cNumberCategories
   };
   
            BDebugPrimitives();
   virtual ~BDebugPrimitives();

   void init();
   void deInit();
   
   // Timeout:
   //--  -1 = single frame only
   //--  >= 0 = for x secs
   // Returns false if too many primitives of that type are active.
   bool  addDebugLine(BVector point, BVector point2, DWORD color1 = 0xFFFFFFFF, DWORD color2 = 0xFFFFFFFF, int category = cCategoryNone, float timeout=-1.0f);   
   bool  addDebugThickLine(BVector point, BVector point2, float thickness, DWORD color1 = 0xFFFFFFFF, DWORD color2 = 0xFFFFFFFF, int category = cCategoryNone, float timeout=-1.0f);   
   
   bool  addDebugText(const char* pText, BVector point, float size = 1.0f, DWORD color = 0xFFFFFFFF, int category = cCategoryNone, bool zTest = false, float timeout=-1.0f);   
   
   bool  addDebugCircle(BMatrix matrix, float radius, DWORD color1 = 0xFFFFFFFF, int category = cCategoryNone, float timeout=-1.0f);
   bool  addDebugThickCircle(BMatrix matrix, float radius, float thickness, DWORD color1 = 0xFFFFFFFF, int category = cCategoryNone, float timeout=-1.0f);
   
   bool  addDebugSphere(BMatrix matrix, float radius, DWORD color1 = 0xFFFFFFFF, int category = cCategoryNone, float timeout=-1.0f);
   bool  addDebugSphere(BVector origin, float radius, DWORD color1 = 0xFFFFFFFF, int category = cCategoryNone, float timeout=-1.0f);
   
   bool  addDebugBox(BMatrix matrix, BVector scale, DWORD color1 = 0xFFFFFFFF, int category = cCategoryNone, float timeout=-1.0f);
   bool  addDebugBox(BVector min, BVector max, DWORD color1 = 0xFFFFFFFF, int category = cCategoryNone, float timeout=-1.0f);
   
   bool  addDebugArrow(BMatrix matrix, BVector scale, DWORD color = 0xFFFFFFFF, int category= cCategoryNone, float timeout=-1.0f);
   
   bool  addDebugAxis(BMatrix matrix, float scale, int category=cCategoryNone, float timeout=-1.0f);
   bool  addDebugAxis(BVector pos, BVector right, BVector up, BVector at, float scale, int category=cCategoryNone, float timeout=-1.0f);

   bool  addDebugCylinder(BMatrix matrix, float radius, float height, DWORD color = 0xFFFFFFFF, int category=cCategoryNone, float timeout=-1.0f);

   void  addDebugScreenLines(const BVector* screenPoints, long pointCount, DWORD color = 0xFFFFFFFF, int category=cCategoryNone, float timeout=-1.0f);
   void  addDebugScreenLine(BVector p1, BVector p2, DWORD color = 0xFFFFFFFF, int category=cCategoryNone, float timeout=-1.0f);
   void  addDebugScreenRect(float left, float top, float right, float bottom, DWORD color = 0xFFFFFFFF, int category=cCategoryNone, float timeout=-1.0f);
   void  addDebugScreenRect(const BVector* screenPoints, DWORD color = 0xFFFFFFFF, int category=cCategoryNone, float timeout=-1.0f);
   void  addDebugScreenCircle(BVector up, float uxr, float uyr, DWORD color = 0xFFFFFFFF, int category=cCategoryNone, float timeout=-1.0f);
   void  addDebugScreenTriangle(BVector p1, BVector p2, BVector p3, DWORD color = 0xFFFFFFFF, int category=cCategoryNone, float timeout=-1.0f);
   void  addDebugScreenPolygon(BVector cp, BVector* tps, long pointCount, DWORD color = 0xFFFFFFFF, int category=cCategoryNone, float timeout=-1.0f);

   void  clear(int category = -1);
   void  beginFrame(double curTime);
   void  endFrame(void);

   void  setThreadSafeFlag(bool bValue) { mThreadSafeFlag = bValue; };
   bool  getThreadSafeFlag() const      { return mThreadSafeFlag; }
      
   void  workerRender(void);

protected:
   
   template<class T> void updatePrimitives(T& list);
   template<class T> void clearPrimitives(T& list, int category);

   float createTimeout(float durationInSeconds);

   double mCurTime;  
   BDynamicArray<BPrimitiveBox, 16>    mBoxPrimitives;
   BDynamicArray<BPrimitiveBox, 16>    mArrowPrimitives;
   BDynamicArray<BPrimitiveSphere, 16> mCirclePrimitives; //-- reuse circle / spheres use same data
   BDynamicArray<BPrimitiveSphere, 16> mThickCirclePrimitives; //-- reuse circle / spheres use same data
   BDynamicArray<BPrimitiveSphere, 16> mSpherePrimitives;
   BDynamicArray<BPrimitiveLine, 16>   mLinePrimitives;
   BDynamicArray<BPrimitiveLine, 16>   mThickLinePrimitives;
   BDynamicArray<BPrimitiveText, 16>   mTextPrimitives; 

   BLightWeightMutex mMutex;
   bool mThreadSafeFlag : 1;
};

//============================================================================
// Externs
//============================================================================
extern __declspec(thread) BDebugPrimitives* gpDebugPrimitives;
extern BDebugPrimitives* gpRenderDebugPrimitives;
