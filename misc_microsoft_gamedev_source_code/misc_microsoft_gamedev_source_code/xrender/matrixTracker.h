//============================================================================
//
//  matrixTracker.h
//  
//  Copyright (c) 2006, Ensemble Studios
//
//============================================================================
#pragma once

// xcore
#include "math\generalMatrix.h"

// local
#include "math\frustum.h"

struct D3DVIEWPORT;

//============================================================================
// enum eMTMatrix
// Important: Don't add any new matrices to eMTMatrix unless BMatrixTracker
// is responsible for updating them!
//============================================================================
enum eMTMatrix
{
   cMTInvalidMatrix = -1,

   // These are the basic/elemental matrices
   cMTWorldToView,     // world to view
   cMTViewToProj,      // view to projection
   cMTProjToScreen,    // projection to screen
   
   cMTViewToWorld,     // view to world
   cMTScreenToProj,    // screen to projection

   cMTNumBasicMatrices,

   // These are the derived/non-basic matrices, i.e. combinations of the basic matrices
   cMTWorldToProj = cMTNumBasicMatrices,     // world to projection
   cMTScreenToView,    // screen to view
   cMTWorldToScreen,

   cNumMTMatrices
};

//============================================================================
// class BMatrixTracker
// Must be bitwise-copyable!
//============================================================================
#if !defined(CODE_ANALYSIS_ENABLED)
__declspec(align(16))
#endif
class BMatrixTracker
{
public:
   BMatrixTracker(void);
   ~BMatrixTracker();

   BMatrixTracker(const BMatrixTracker& rhs);
   
   // Get a matrix.
   const XMMATRIX& getMatrix(eMTMatrix matrixIndex, bool transposed = false) const;
   const XMMATRIX getMatrixByValue(eMTMatrix matrixIndex, bool transposed = false) const;
   const BMatrix44& getMatrix44(eMTMatrix matrixIndex, bool transposed = false) const;
   
   // Set a basic matrix.
   // This method does the minimum work necessary to update the dependent matrices.
   void setMatrix(eMTMatrix basicMatrixIndex, const XMMATRIX m);
   
   // Sets the viewport, which changes the projection to screen matrix.
   void setViewport(int x, int y, int width, int height, float minZ = 0.0f, float maxZ = 1.0f);
   void setViewport(const D3DVIEWPORT9& viewport);
   const D3DVIEWPORT9& getViewport(void) const { return mViewport; }
     
   void setViewToProjPersp(float aspect, float zNear, float zFar, float fullFov);
   void setViewToProjPersp(float l, float r, float b, float t, float zNear, float zFar);
   void setViewToProjOrtho(float l, float r, float b, float t, float zn, float zf);

   void setWorldToView(const XMVECTOR eye, const XMVECTOR focus, const XMVECTOR up);
                              
   // Get a matrix's effect name.
   static const char* getMatrixEffectName(eMTMatrix matrixIndex);

   // Get the view vector that goes through the indicated physical screen pixel (i.e. relative to the physical screen, NOT the viewport).
   XMVECTOR getViewVector(const BVec2& screen) const;
   
   XMVECTOR getViewToProjMul(void) const;
   XMVECTOR getViewToProjAdd(void) const;
   
   const XMVECTOR getWorldCamPos(void) const { return mWorldCamPos; }
   const BVec4& getWorldCamPosVec4(void) const { return *reinterpret_cast<const BVec4*>(&mWorldCamPos); }
   
   void setWorldCamPos(const XMVECTOR camPos) { mWorldCamPos = camPos; }
   
   const BFrustum& getWorldFrustum(void) const { return mWorldFrustum; }
   const BFrustum& getViewFrustum(void) const { return mViewFrustum; }
   
   XMVECTOR getWorldAt(void) const;
   const BVec4& getWorldAtVec4(void) const;
   XMVECTOR getWorldUp(void) const;
   XMVECTOR getWorldRight(void) const;
   
   // Returns the 8 vertices of the view frustum in world/view space.
   // The first 4 will be on the near plane.
   enum { cNumFrustumVertices = 8 };
   void getFrustumVertices(XMVECTOR* RESTRICT pVerts, bool viewspace = false) const;
   
   // Returns the 6 planes of the view frustum in world/view space.
   // Order of planes: left, right, bottom, top, near, far (pos, neg, pos, neg, pos, neg)
   enum { cNumFrustumPlanes = 6 };
   void getFrustumPlanes(XMVECTOR* RESTRICT pPlanes, bool viewspace = false) const;
            
   void clear(void);

   static uint getSerializeSize(void);
   void serialize(uchar* pDst) const;
   void deserialize(const uchar* pSrc);

protected:
   // array of transposed and non-transposed matrices
   XMMATRIX mMatrices[cNumMTMatrices][2];           
   XMVECTOR mWorldCamPos;
   BFrustum mWorldFrustum;
   BFrustum mViewFrustum;
   D3DVIEWPORT9 mViewport;
      
   void clearMatrices(void);
   void setMatrixPair(eMTMatrix matrixIndex, const XMMATRIX m);
   
   XMMATRIX concatMatrices(eMTMatrix a, eMTMatrix b, bool aTransposed = false, bool bTransposed = false);
   void updateDirtyBasicMatrix(eMTMatrix matrix);
   void updateProjToScreenMatrix(void);
};

