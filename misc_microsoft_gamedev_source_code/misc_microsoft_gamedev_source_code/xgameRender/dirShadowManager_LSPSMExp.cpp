//============================================================================
//
// File: dirShadowManager.cpp
//  
// Copyright (c) 2006, Ensemble Studios
//
//============================================================================
#include "xgameRender.h"
#include "dirShadowManager.h"
#include "debugprimitives.h"

#include "math\generateCombinations.h"
#include "math\plane.h"

#include "primDraw2D.h"
#include "renderDraw.h"
#include "sceneLightManager.h"
#include "effectIntrinsicManager.h"
#include "cullingManager.h"
#include "render.h"

const uint cShadowBufferWidth = 1024;
const uint cShadowBufferHeight = 1024;

BDirShadowManager gDirShadowManager;

BDirShadowManager::BDirShadowManager()
{
   clear();
}

BDirShadowManager::~BDirShadowManager()
{
}

void BDirShadowManager::clear(void)
{
   for (uint i = 0; i < cMaxPasses; i++)
   {
      mWorldToView[i] = XMMatrixIdentity();
      mViewToProj[i] = XMMatrixIdentity();
      mWorldToProj[i] = XMMatrixIdentity();
      mPrevWorldToView[i] = XMMatrixIdentity();
      mPrevViewToProj[i] = XMMatrixIdentity();
      mWorldToTex[i] = XMMatrixIdentity();    
      
      mViewMin[i].clear();
      mViewMax[i].clear();
   }      

   Utils::ClearObj(mpRenderTarget);
   
   mpRenderTarget = NULL;
   mpShadowBuffer = NULL;

   mInShadowPass = false;
}

void BDirShadowManager::init(void)
{
   clear();
   
   updateTextureScaleMatrix();
   
   const uint cNumArraySlices = cMaxPasses;
   HRESULT hres = gRenderDraw.createArrayTexture(cShadowBufferWidth, cShadowBufferHeight, cNumArraySlices, 1, 0, D3DFMT_D24S8, 0, &mpShadowBuffer, NULL);
   if (FAILED(hres))
   {
      BFATAL_FAIL("BDirShadowManager::init: createTexture() failed!");
   }
   
   D3DSURFACE_PARAMETERS surfaceParams;
   Utils::ClearObj(surfaceParams);

   surfaceParams.Base = 0;

   hres = gRenderDraw.createDepthStencilSurface(cShadowBufferWidth, cShadowBufferHeight, D3DFMT_D24S8, D3DMULTISAMPLE_NONE, 0, FALSE, &mpRenderTarget, &surfaceParams);
   if (FAILED(hres))
   {
      BFATAL_FAIL("BDirShadowManager::init: createTexture() failed!");
   }
   
   gEffectIntrinsicManager.set(cIntrinsicDirShadowMapTexture, &mpShadowBuffer, cIntrinsicTypeTexturePtr);
}

void BDirShadowManager::deinit(void)
{
   if (mpShadowBuffer)
   {
      gRenderDraw.releaseD3DResource(mpShadowBuffer);
      mpShadowBuffer = NULL;
      
      gEffectIntrinsicManager.set(cIntrinsicDirShadowMapTexture, &mpShadowBuffer, cIntrinsicTypeTexturePtr);
   }
   
   if (mpRenderTarget)
   {
      gRenderDraw.releaseD3DResource(mpRenderTarget);
      mpRenderTarget = NULL;
   }
}

void BDirShadowManager::updateTextureScaleMatrix(void)
{   
   D3DXMATRIX& matrix = CAST(D3DXMATRIX, mTextureScale);
   
   const float fOffsetX = 0.5f + (0.5f / cShadowBufferWidth);
   const float fOffsetY = 0.5f + (0.5f / cShadowBufferHeight);

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

void BDirShadowManager::calcClippedPolyhedron(BPointArray& points, float worldMinY, float worldMaxY, float extraNearClipDist)
{
   const BMatrixTracker& matrixTracker = gRenderDraw.getMainSceneMatrixTracker();
   const BFrustum& sceneFrustum = matrixTracker.getWorldFrustum();

   BStaticArray<Plane, 10, false> planes;
   planes.pushBack(Plane(0.0f, 1.0f, 0.0f, worldMinY));
   planes.pushBack(Plane(0.0f, -1.0f, 0.0f, -worldMaxY));
   
   if (extraNearClipDist > 0.0f)
   {
      BVec3 n(-matrixTracker.getWorldAt());
      n[1] = 0.0f;
      if (!n.normalize(NULL))
         n = -matrixTracker.getWorldAt();
      Plane p(n[0], n[1], n[2], 0.0f);

      BVec3 f(BVec3(matrixTracker.getWorldCamPos()) + -n * extraNearClipDist);
      p.d = p.distanceToPoint(f);
      
      planes.pushBack(p);
   }         
   
   planes.pushBack(sceneFrustum.getPlanes(), 6);

   BStaticArray<int, 256, false> combinations;
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
   
   if (points.size() < 4)
   {
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
   
   const uint numOrigPoints = points.size();
   const BVec3 lightDir(&gSceneLightManager.getDirLight(cLCTerrain).mDir.x);
   for (uint i = 0; i < numOrigPoints; i++)
   {
      BVec3 p(points[i]);
      p += -lightDir * 600.0f;
      points.pushBack(p);
   }
}   

void BDirShadowManager::calcWorldPolyhedron(BPointArray& points, float dist)
{
   const BMatrixTracker& matrixTracker = gRenderDraw.getMainSceneMatrixTracker();
      
   points.resize(0);
   
   BVec3 center(matrixTracker.getWorldCamPos());
   center[1] = 0.0f;
   
   points.pushBack(center + BVec3(-dist, 0, -dist));
   points.pushBack(center + BVec3( dist, 0, -dist));
   points.pushBack(center + BVec3( dist, 0,  dist));
   points.pushBack(center + BVec3(-dist, 0,  dist));
}   

struct M4
{
   float array[16];
   
   float operator() (uint col, uint row) const { BDEBUG_ASSERT(col>=1 && col<=4 && row>=1 && row<=4);  return array[col - 1 + (row - 1) * 4]; }
   float& operator() (uint col, uint row) { BDEBUG_ASSERT(col>=1 && col<=4 && row>=1 && row<=4); return array[col - 1 + (row - 1) * 4]; }
   float operator[](uint i) const { BDEBUG_ASSERT(i <= 15); return array[i]; }
   float& operator[](uint i) { BDEBUG_ASSERT(i <= 15); return array[i]; }
   
   BVec3 mulHomogenPoint(const BVec3& v) const 
   {
      double x = array[0]*v[0] + array[4]*v[1] + array[ 8]*v[2] + array[12];
      double y = array[1]*v[0] + array[5]*v[1] + array[ 9]*v[2] + array[13];
      double z = array[2]*v[0] + array[6]*v[1] + array[10]*v[2] + array[14];
      double w = array[3]*v[0] + array[7]*v[1] + array[11]*v[2] + array[15];

      if (w != 0.0f)
         return BVec3(x/w, y/w, z/w);
      else
         return BVec3(x, y, z);
   }
   
   BVec4 transform(const BVec4& v) const 
   {
      double x = array[0]*v[0] + array[4]*v[1] + array[ 8]*v[2] + array[12]*v[3];
      double y = array[1]*v[0] + array[5]*v[1] + array[ 9]*v[2] + array[13]*v[3];
      double z = array[2]*v[0] + array[6]*v[1] + array[10]*v[2] + array[14]*v[3];
      double w = array[3]*v[0] + array[7]*v[1] + array[11]*v[2] + array[15]*v[3];
      return BVec4(x, y, z, w);
   }
   
   void setZero(void)
   {
      memset(this, 0, sizeof(float)*16);
   }
   
   void setIdentity(void)
   {
      setZero();
      (*this)(1,1) = 1.0f;
      (*this)(2,2) = 1.0f;
      (*this)(3,3) = 1.0f;
      (*this)(4,4) = 1.0f;
   }
   
   static M4 makeZero(void)
   {
      M4 ret;
      ret.setZero();
      return ret;
   }
   
   static M4 makeScale(float x, float y, float z)
   {
      M4 ret;
      ret.setIdentity();
      ret(1,1) = x;
      ret(2,2) = y;
      ret(3,3) = z;
      return ret;
   }
   
   static M4 makeTranslate(float x, float y, float z)
   {
      M4 ret;
      ret.setIdentity();
      ret[12] = x;
      ret[13] = y;
      ret[14] = z;
      return ret;
   }
   
   static M4 makeTranslate(const BVec3& p)
   {
      return makeTranslate(p[0], p[1], p[2]);
   }
   
   static M4& makeScaleTranslate(M4& output, const BVec3& vMin, const BVec3& vMax) 
   {
      const double diffX = vMax[0]-vMin[0];
      output[ 0] = 2/diffX;
      output[ 4] = 0;
      output[ 8] = 0;
      output[12] = -(vMax[0]+vMin[0])/diffX;

      const double diffY = vMax[1]-vMin[1];
      output[ 1] = 0;
      output[ 5] = 2/diffY;
      output[ 9] = 0;
      output[13] = -(vMax[1]+vMin[1])/diffY;

      const double diffZ = vMax[2]-vMin[2];
      output[ 2] = 0;
      output[ 6] = 0;
      output[10] = 2/diffZ;
      output[14] = -(vMax[2]+vMin[2])/diffZ;

      output[ 3] = 0;
      output[ 7] = 0;
      output[11] = 0;
      output[15] = 1;
      return output;
   }
   
   friend M4 operator*(const M4& a, const M4& b) 
   {
      M4 ret;
      
      for(unsigned iCol = 1; iCol <= 4; iCol++) 
      {
         for(unsigned iRow = 1; iRow <= 4; iRow++) 
         {
            double o = 0.0f;
            
            for(unsigned k = 1; k <= 4; k++) 
               o += a(iCol,k) * b(k,iRow);
            
            ret(iCol, iRow) = o;
         }
      }
      
      return ret;
   }
   
   static float det2x2(float a1, float a2, float b1, float b2)
   {
      return a1*b2 - b1*a2;
   }
   
   static det3x3(float a1, float a2, float a3,
                 float b1, float b2, float b3,
                 float c1, float c2, float c3) 
   {
      return a1*det2x2(b2,b3,c2,c3) - b1*det2x2(a2,a3,c2,c3) + c1*det2x2(a2,a3,b2,b3);
   }
         
   //output = i^(-1)
   void invert(const M4& i) const
   {
      D3DXMatrixInverse((D3DXMATRIX*)this, NULL, (const D3DXMATRIX*)&i);
   }
   
   M4 getInverse(void) const
   {
      M4 ret;
      ret.invert(*this);
      return ret;
   }
   
   M4 transpose(void)  
   {
      std::swap(array[ 1],array[ 4]);
      std::swap(array[ 2],array[ 8]);
      std::swap(array[ 3],array[12]);
      std::swap(array[ 6],array[ 9]);
      std::swap(array[ 7],array[13]);
      std::swap(array[11],array[14]);
      return *this;
   }
};

const M4& look(M4& output, const BVec3& pos, const BVec3& dir, const BVec3& up) 
{
   BVec3 dirN = dir;
   dirN.normalize();

   BVec3 upN = up;
   upN.normalize();

   BVec3 xN;
   xN = dirN % upN;
   upN = xN % dirN;

   output(1,1) = xN[0];
   output(2,1) = upN[0];
   output(3,1) = -dirN[0];
   output(4,1) = 0.0f;

   output(1,2) = xN[1];
   output(2,2) = upN[1];
   output(3,2) = -dirN[1];
   output(4,2) = 0.0f;

   output(1,3) = xN[2];
   output(2,3) = upN[2];
   output(3,3) = -dirN[2];
   output(4,3) = 0.0f;

   output(1,4) = -xN.dot(pos);
   output(2,4) = -upN.dot(pos);
   output(3,4) = dirN.dot(pos);
   output(4,4) = 1.0f;
   
   return output;
}

void frustumGL(M4& output, float left, float right,
               float bottom, float top,
               float zNearDis, float zFarDis) 
{
   const float diffX = float(1.0)/(right-left);
   const float diffY = float(1.0)/(top-bottom);
   const float diffZ = float(1.0)/(zNearDis-zFarDis);

   output(1,1) = 2*zNearDis*diffX;
   output(1,2) = 0;
   output(1,3) = (right+left)*diffX;
   output(1,4) = 0;

   output(2,1) = 0;
   output(2,2) = 2*zNearDis*diffY;
   output(2,3) = (top+bottom)*diffY;
   output(2,4) = 0;

   output(3,1) = 0;
   output(3,2) = 0;
   output(3,3) = (zFarDis+zNearDis)*diffZ;
   output(3,4) = 2*zNearDis*zFarDis*diffZ;

   output(4,1) = 0;
   output(4,2) = 0;
   output(4,3) = -1;
   output(4,4) = 0;
}


struct LSPSMState
{
   BPointArray* mpPoints;
   BVec3 mEyePos;
   BVec3 mEyeDir;
   BVec3 mLightDir;
   M4 mEyeView;
};

void calcStandardLightSpace(M4& lightView, M4& lightProj, const LSPSMState& state) 
{
   const BVec3 eyeDir = state.mEyeDir;
   const BVec3 eyePos = state.mEyePos;
   const BVec3 lightDir = state.mLightDir;
   const BVec3 up       = eyeDir;

   look(lightView, eyePos, lightDir,up);	
   lightProj.setIdentity();
}

BVec3 getNearCameraPointE(const LSPSMState& state)
{
   BVec3 nearestPoint(0.0f);
   
   float minZ = 1e+10f;
   for (uint i = 0; i < state.mpPoints->size() / 2; i++)
   {
      BVec4 p(BVec4( (*state.mpPoints)[i], 1.0f) * gRenderDraw.getMainSceneMatrixTracker().getMatrix(cMTWorldToView));
      if (p[2] < minZ)
      {  
         minZ = p[2];
         nearestPoint = (*state.mpPoints)[i];
      }
   }
   return nearestPoint;
}

const BVec3 getProjViewDir_ls(const M4& lightSpace, const LSPSMState& state) 
{
   const BVec3 eyeDir(state.mEyeDir);
   
   //get the point in the LVS volume that is nearest to the camera
   const BVec3 e = getNearCameraPointE(state);
   //construct edge to transform into light-space
   const BVec3 b = e + eyeDir;
   //transform to light-space
   const BVec3 e_lp = lightSpace.mulHomogenPoint(e);
   const BVec3 b_lp = lightSpace.mulHomogenPoint(b);
   
   BVec3 projDir(b_lp-e_lp);
   
   //project the view direction into the shadow map plane
   projDir[1] = 0.0;
   
   return projDir;
}

struct AABox
{
   BVec3 min;
   BVec3 max;
   
   AABox(const M4& transform, const LSPSMState& state)
   {
      initExpand();
      
      for (uint i = 0; i < state.mpPoints->size(); i++)
         expand( transform.mulHomogenPoint((*state.mpPoints)[i]) );
   }
   
   void initExpand(void)
   {
      min.set(1e+20f);
      max.set(-1e+20f); 
   }
   
   void expand(const BVec3& p)
   {
      for (uint i = 0; i < 3; i++)
      {
         if (p[i] < min[i]) min[i] = p[i];
         if (p[i] > max[i]) max[i] = p[i];
      }
   }
   
   const BVec3& getMin(void) const { return min; }
   const BVec3& getMax(void) const { return max; }
};


struct LPlane
{
   BVec3 n;
   float d;

   LPlane(const BVec3& origin, const BVec3& norm)
   {
      n = norm;
      n.tryNormalize();
      d = n * origin;
   }

   BVec4 getHomogenEquation(void) const
   {
      return BVec4(n[0], n[1], n[2], -d);
   }

   void transform(const M4& invTranSpace) 
   { 
      const BVec4 p = invTranSpace.transform(getHomogenEquation());
      n.set(p[0], p[1], p[2]);
      d = -p[3];
      hessNorm();
   }

   void hessNorm(void)
   {
      double t = n.squaredLen();
      if (t != 0.0f) 
      {
         t = sqrt(t);
         n /= t;
         d /= t;
      }
   }

   float getD(void) const
   {
      return d;
   }

   const BVec3& getN(void) const
   {
      return n;
   }
};


//z0 is the point that lies on the parallel plane to the near plane through e (A)
//and on the near plane of the C frustum (the plane z = bZmax) and on the line x = e.x
const BVec3 getZ0_ls(const M4& lightSpace, const BVec3& e, const float& b_lsZmax, const LSPSMState& state) 
{
   //to calculate the parallel plane to the near plane through e we 
   //calculate the plane A with the three points
   LPlane A = LPlane(e, state.mEyeDir);
   //to transform plane A into lightSpace calculate transposed inverted lightSpace	
   const M4 invTlightSpace = lightSpace.getInverse().transpose();
   //and transform the plane equation with it
   A.transform(invTlightSpace);
   //get the parameters of A from the plane equation n dot d = 0
   const float d = A.getD();
   const BVec3 n = A.getN();
   //transform to light space
   const BVec3 e_ls = lightSpace.mulHomogenPoint(e);
   //z_0 has the x coordinate of e, the y coord of B.max() 
   //and the z coord of the plane intersection
   return BVec3(e_ls[0],(d-n[2]*b_lsZmax-n[0]*e_ls[0])/n[1],b_lsZmax);
}

const float calcNoptGeneral(const M4& lightSpace, const AABox& B_ls, const LSPSMState& state) 
{
   BVec3 nearCameraPoint(getNearCameraPointE(state));
   
#if 0
   lightSpace[0]	= -0.89752930402755737	;
   lightSpace[1]	= 0.39324793219566345	;
   lightSpace[2]	= -0.19949236512184143	;
   lightSpace[3]	= 0.00000000000000000	;
   lightSpace[4]	= -0.28747794032096863	;
   lightSpace[5]	= -0.86488050222396851	;
   lightSpace[6]	= -0.41150712966918945	;
   lightSpace[7]	= 0.00000000000000000	;
   lightSpace[8]	= -0.33436137437820435	;
   lightSpace[9]	= -0.31199005246162415	;
   lightSpace[10]	= 0.88930571079254150	;
   lightSpace[11]	= 0.00000000000000000	;
   lightSpace[12]	= 12.936507225036621	;
   lightSpace[13]	= 38.919624328613281	;
   lightSpace[14]	= 18.517820358276367	;
   lightSpace[15]	= 1.0000000000000000	;
   
   B_ls.min[0]	= -161.41285705566406;
   B_ls.min[1]	= -0.84298324584960938;
   B_ls.min[2] = -237.47804260253906;
   
   B_ls.max[0]	= 118.68687438964844;
   B_ls.max[1]	= 194.07199096679687;
   B_ls.max[2]	= 2.8049869537353516;
   
   nearCameraPoint[0]	= -1.9786794185638428;
   nearCameraPoint[1]	= 40.000007629394531	;
   nearCameraPoint[2]	= -4.8041267395019531;
   
   state.mEyeDir[0]	= 0.43536257743835449;
   state.mEyeDir[1]	= -0.45733854174613953;
   state.mEyeDir[2]	= -0.77543610334396362;

   state.mEyePos[0] = 0.0f;
   state.mEyePos[1] = 45.0f;
   state.mEyePos[2] = 0.0f;
   
   state.mEyeView[0]	=0.87196940183639526	;
   state.mEyeView[1]	=0.22389483451843262	;
   state.mEyeView[2]	=-0.43536251783370972;
   state.mEyeView[3]	=0.00000000000000000	;
   state.mEyeView[4]	=0.00000000000000000	;
   state.mEyeView[5]	=0.88929271697998047	;
   state.mEyeView[6]	=0.45733848214149475	;
   state.mEyeView[7]	=0.00000000000000000	;
   state.mEyeView[8]	=0.48956045508384705	;
   state.mEyeView[9]	=-0.39878517389297485	;
   state.mEyeView[10]	=0.77543598413467407	;
   state.mEyeView[11]	=0.00000000000000000	;
   state.mEyeView[12]	=0.00000000000000000	;
   state.mEyeView[13]	=-40.018173217773438	;
   state.mEyeView[14]	=-20.580232620239258	;
   state.mEyeView[15]	=1.0000000000000000	;
#endif

   const M4 eyeView = state.mEyeView;
   const M4 invLightSpace(lightSpace.getInverse());
      
   const BVec3 z0_ls = getZ0_ls(lightSpace,nearCameraPoint,B_ls.getMax()[2], state);
   const BVec3 z1_ls = BVec3(z0_ls[0],z0_ls[1],B_ls.getMin()[2]);

   //to world
   const BVec3 z0_ws = invLightSpace.mulHomogenPoint(z0_ls);
   const BVec3 z1_ws = invLightSpace.mulHomogenPoint(z1_ls);

   //to eye
   const BVec3 z0_cs = eyeView.mulHomogenPoint(z0_ws);
   const BVec3 z1_cs = eyeView.mulHomogenPoint(z1_ws);

   const float z0 = z0_cs[2];
   const float z1 = z1_cs[2];
   const float d = fabs(B_ls.getMax()[2]-B_ls.getMin()[2]);

   return d/( sqrt(z1/z0)-float(1.0) );
}

//this is the algorithm discussed in the article
const M4 getLispSmMtx(const M4& lightSpace, const LSPSMState& state) 
{
   const AABox B_ls(lightSpace, state);

   //get the coordinates of the near camera point in light space
   const BVec3 e_ls = lightSpace.mulHomogenPoint(getNearCameraPointE(state));
   //c start has the x and y coordinate of e, the z coord of B.min() 
   const BVec3 Cstart_lp(e_ls[0],e_ls[1],B_ls.getMax()[2]);

   static float n = 10.0f;//calcNoptGeneral(lightSpace, B_ls, state); 
         
   if (n > 512.0f)
   {
      M4 ret;
      ret.setIdentity();
      return ret;
   }
   
   static int q;
   q++;
   if (q == 50)
   {
      q = 0;
      trace("%f", n);  
   }
      
   //calc C the projection center
   //new projection center C, n behind the near plane of P
   //we work along a negative axis so we transform +n*<the positive axis> == -n*<neg axis>
   const BVec3 C(Cstart_lp+n*BVec3(0,0,1));
   
   //construct a translation that moves to the projection center
   const M4 projectionCenter = M4::makeTranslate(-C);

   //calc d the perspective transform depth or light space y extents
   const float d = fabs(B_ls.getMax()[2]-B_ls.getMin()[2]);

   //the lispsm perspective transformation
   M4 P;
   //here done with a standard frustum call that maps P onto the unit cube with
   //corner points [-1,-1,-1] and [1,1,1].
   //in directX you can use the same mapping and do a mapping to the directX post-perspective cube
   //with corner points [-1,-1,0] and [1,1,1] as the final step after all the shadow mapping.
   frustumGL(P,-1.0,1.0,-1.0,1.0,n,n+d);
   //invert the transform from right handed into left handed coordinate system for the ndc
   //done by the openGL style frustumGL call
   //so we stay in a right handed system
   P = M4::makeScale(1.0,1.0,-1.0)*P;
   //return the lispsm frustum with the projection center
   return P*projectionCenter;
}

M4 scaleTranslateShadowMtx(const M4& lightSpace, const LSPSMState& state) 
{
   //transform the light volume points from world into light space
   //calculate the cubic hull (an AABB) 
   //of the light space extents of the intersection body B
   const AABox b_ls(lightSpace, state);

   //refit to unit cube
   //this operation calculates a scale translate matrix that
   //maps the two extreme points min and max into (-1,-1,-1) and (1,1,1)
   
   M4 ret;
   return M4::makeScaleTranslate(ret, b_ls.getMin(), b_ls.getMax());
}

void updateLightMtx(M4& lightView, M4& lightProj, const LSPSMState& state) 
{
	calcStandardLightSpace(lightView, lightProj, state);

   M4 switchToArticle(M4::makeZero());
   switchToArticle(1,1) = 1.0f;
   switchToArticle(2,3) = -1.0f; // y -> -z
   switchToArticle(3,2) = 1.0f;  // z -> y
   switchToArticle(4,4) = 1.0f;
   
   //switch to the lightspace used in the article
   lightProj = switchToArticle * lightProj;
      
   const M4 L = lightProj * lightView; 

   BVec3 projViewDir = getProjViewDir_ls(L, state); 

   //do Light Space Perspective shadow mapping
   //rotate the lightspace so that the proj light view always points upwards
   //calculate a frame matrix that uses the projViewDir[light-space] as up vector
   //look(from position, into the direction of the projected direction, with unchanged up-vector)
   
   lightProj = look(M4(), BVec3(0.0f), projViewDir, BVec3(0,1,0)) * lightProj;
   M4 lispsm = getLispSmMtx(lightProj * lightView, state);
   lightProj = lispsm * lightProj;
   
   const M4 PL = lightProj * lightView; 
   
   //map to unit cube
   lightProj = scaleTranslateShadowMtx(PL, state) * lightProj;

   //coordinate system change for calculations in the article
   M4 switchToGL(M4::makeZero());
   switchToGL(1,1) = 1.0;
   switchToGL(2,3) = 1.0; // y -> z
   switchToGL(3,2) = -1.0;  // z -> -y
   switchToGL(4,4) = 1.0;

   //back to open gl coordinate system y <-> z
   lightProj = switchToGL * lightProj;
   
   //transform from right handed system into left handed ndc
   lightProj = M4::makeScale(1.0,1.0,-1.0) * lightProj;
}

void BDirShadowManager::shadowGenPrep(float worldMinY, float worldMaxY, int pass)
{
   const BMatrixTracker& matrixTracker = gRenderDraw.getMainSceneMatrixTracker();
   const BVec3 cameraPos(gRenderDraw.getMainSceneMatrixTracker().getWorldCamPos());
   const BVec3 cameraDir(gRenderDraw.getMainSceneMatrixTracker().getWorldAt());
   const BVec3 cameraRight(gRenderDraw.getMainSceneMatrixTracker().getWorldRight());
   const BVec3 lightDir(&gSceneLightManager.getDirLight(cLCTerrain).mDir.x);
   
   worldMaxY = Math::fSelectMin(cameraPos[1], worldMaxY);
   if (worldMaxY < (worldMinY + .5f))
      worldMinY = worldMaxY - 1.0f;
      
#if 0   
   float nearDist = 19.0f;
   if (matrixTracker.getWorldCamPos()[1] < 6.0f)
      nearDist = 8.0f;
   else if (matrixTracker.getWorldCamPos()[1] > 15.0f)
      nearDist = 30.0f;
      
   const float clipDistances[cMaxPasses] = { nearDist, 65.0f, 250.0f };
   if (pass < 1)
      calcWorldPolyhedron(mWorldPoints[pass], clipDistances[pass]);
   else
      calcClippedPolyhedron(mWorldPoints[pass], worldMinY, worldMaxY, clipDistances[pass]);
#endif      

   static float z = 250.0f;
   calcClippedPolyhedron(mWorldPoints[pass], worldMinY, worldMaxY, z);
   
      
   LSPSMState state;
   state.mEyePos = cameraPos;
   state.mEyeDir = cameraDir;
   state.mLightDir = lightDir;
   state.mpPoints = &mWorldPoints[pass];
   state.mEyeView = CONST_CAST(M4, gRenderDraw.getMainSceneMatrixTracker().getMatrix(cMTWorldToView, false));
   
   M4 lightView;
   M4 lightProj;
   updateLightMtx(lightView, lightProj, state);
   
   //lightView.transpose();
   //lightProj.transpose();
   
   mWorldToView[pass] = XMLoadFloat4x4((XMFLOAT4X4*)&lightView);
   mViewToProj[pass] = XMLoadFloat4x4((XMFLOAT4X4*)&lightProj);


#if 0
   BVec3 lightAt(lightDir);
   
   BVec3 lightPos(gRenderDraw.getMainSceneMatrixTracker().getWorldCamPos());
   lightPos -= lightAt * 300.0f;
      
   BVec3 lightRight(lightAt % cameraDir);
   if (!lightRight.normalize(NULL))
   {
      lightRight = lightAt % cameraRight;
      lightRight.normalize(NULL);
   }         
   
   BVec3 lightUp(lightAt % lightRight);
   lightUp.normalize(NULL);
                        
   XMMATRIX translate = XMMatrixTranslation(-lightPos[0], -lightPos[1], -lightPos[2]);
      
   mWorldToView[pass].r[0] = XMVectorSet(lightRight[0], lightUp[0], lightAt[0], 0.0f);
   mWorldToView[pass].r[1] = XMVectorSet(lightRight[1], lightUp[1], lightAt[1], 0.0f);
   mWorldToView[pass].r[2] = XMVectorSet(lightRight[2], lightUp[2], lightAt[2], 0.0f);
   mWorldToView[pass].r[3] = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
   
   mWorldToView[pass] = XMMatrixMultiply(translate, mWorldToView[pass]);
#endif   

   mViewMin[pass].set(1e+10f);
   mViewMax[pass].set(-1e+10f);

   mViewPoints[pass].resize(mWorldPoints[pass].size());   
   for (uint i = 0; i < mWorldPoints[pass].size(); i++)
   {
      XMVECTOR v = XMVector4Transform(XMVectorSet(mWorldPoints[pass][i][0], mWorldPoints[pass][i][1], mWorldPoints[pass][i][2], 1.0f), mWorldToView[pass]);

      mViewPoints[pass][i] = BVec3(v.x, v.y, v.z);

      mViewMin[pass] = BVec3::elementMin(mViewMin[pass], mViewPoints[pass][i]);
      mViewMax[pass] = BVec3::elementMax(mViewMax[pass], mViewPoints[pass][i]);
   }
}

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

void snapMatrixTranslation(D3DXMATRIX& worldToProj, D3DXMATRIX& viewToProj)
{
   static float xs = .25f;
   static float ys = .25f;
   float snapX = floor(worldToProj._41 * xs*cShadowBufferWidth + .5f) / float(cShadowBufferWidth*ys);
   float snapY = floor(worldToProj._42 * ys*cShadowBufferHeight + .5f) / float(cShadowBufferHeight*xs);
   float snapZ = floor(worldToProj._43 * 256.0f) / float(256.0f);
   viewToProj._41 += snapX - worldToProj._41;
   viewToProj._42 += snapY - worldToProj._42;
   viewToProj._43 += snapZ - worldToProj._43;
   worldToProj._41 = snapX;
   worldToProj._42 = snapY;
   worldToProj._43 = snapZ;
}   

void BDirShadowManager::shadowGenBegin(float worldMinY, float worldMaxY, int pass)
{
   BDEBUG_ASSERT(pass || !mInShadowPass);
   
   mInShadowPass = true;
   
   shadowGenPrep(worldMinY, worldMaxY, pass);
               
   //mViewToProj[pass] = XMMatrixOrthographicOffCenterLH(mViewMin[pass][0], mViewMax[pass][0], mViewMax[pass][1], mViewMin[pass][1], mViewMin[pass][2], mViewMax[pass][2]);
  
   
#if 1   
   static float viewTol = .0005f;
   static float projTol = .00005f;
   snapMatrix(CAST(D3DXMATRIX, mWorldToView[pass]), CAST(D3DXMATRIX, mPrevWorldToView[pass]), viewTol);
   snapMatrix(CAST(D3DXMATRIX, mViewToProj[pass]), CAST(D3DXMATRIX, mPrevViewToProj[pass]), projTol);
#endif
   
   mWorldToProj[pass] = XMMatrixMultiply(mWorldToView[pass], mViewToProj[pass]);

#if 1      
   snapMatrixTranslation(CAST(D3DXMATRIX, mWorldToProj[pass]), CAST(D3DXMATRIX, mViewToProj[pass]));
#endif
   
   mWorldToTex[pass] = XMMatrixMultiply(mWorldToProj[pass], mTextureScale);   
      
   gEffectIntrinsicManager.setMatrix(cIntrinsicDirShadowWorldToProj, &mWorldToProj[pass]);
   
   BMatrixTracker& matrixTracker = gRenderDraw.getMainActiveMatrixTracker();
   BRenderViewport& renderViewport = gRenderDraw.getMainActiveRenderViewport();
          
   matrixTracker.setMatrix(cMTWorldToView, CAST(BMatrix44, mWorldToView[pass]));
   matrixTracker.setMatrix(cMTViewToProj, CAST(BMatrix44, mViewToProj[pass]));

   D3DVIEWPORT9 viewport;
   viewport.X = 0;
   viewport.Y = 0;
   viewport.Width = cShadowBufferWidth;
   viewport.Height = cShadowBufferHeight;
   viewport.MinZ = 0.0f;
   viewport.MaxZ = 1.0f;
   
   matrixTracker.setViewport(viewport);
   
   RECT scissorRect;
   const uint scissorPixelBorder = 1;
   scissorRect.top = scissorPixelBorder;
   scissorRect.left = scissorPixelBorder;
   scissorRect.bottom = cShadowBufferHeight - scissorPixelBorder * 2;
   scissorRect.right = cShadowBufferWidth - scissorPixelBorder * 2;
         
   renderViewport.setSurf(0, NULL);
   renderViewport.setDepthStencilSurf(mpRenderTarget);
   renderViewport.setViewport(viewport);
   renderViewport.setScissorEnabled(true);
   
   renderViewport.setScissorRect(scissorRect);
         
   gRenderDraw.setActiveMatrixTracker(matrixTracker);
   gRenderDraw.setActiveRenderViewport(renderViewport);

   //if (pass == 0)
   {
      gRenderDraw.setRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
   
      gRenderDraw.clear(D3DCLEAR_STENCIL|D3DCLEAR_ZBUFFER);
   
      gRenderDraw.setRenderState(D3DRS_SCISSORTESTENABLE, TRUE);
   }      
   
   gRenderDraw.setRenderState(D3DRS_COLORWRITEENABLE, 0);
   gRenderDraw.setRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
   
   static float depthBias = .00002f;
   gRenderDraw.setRenderState(D3DRS_DEPTHBIAS, CAST(DWORD, depthBias));

   gCullingManager.update(gRenderDraw.getMainActiveMatrixTracker().getWorldFrustum());
}

void BDirShadowManager::shadowGenEnd(int pass)
{
   BDEBUG_ASSERT(mInShadowPass);
   
   gRenderDraw.setRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
   
   gRenderDraw.resolve(
      D3DRESOLVE_DEPTHSTENCIL|D3DRESOLVE_ALLFRAGMENTS,// | ((pass < (cMaxPasses - 1)) ? D3DRESOLVE_CLEARDEPTHSTENCIL : 0),
      NULL,
      mpShadowBuffer,
      NULL,
      0,
      pass,
      NULL,
      1.0f,
      0,
      NULL);
         
   gRenderDraw.setRenderState(D3DRS_SCISSORTESTENABLE, TRUE);
         
   if (pass == cMaxPasses - 1)
   {
      mInShadowPass = false;
         
      gRender.updateActiveMatricesAndViewport();
      
      gRenderDraw.setRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_ALL);
      gRenderDraw.setRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
      gRenderDraw.setRenderState(D3DRS_DEPTHBIAS, 0);
      
      gEffectIntrinsicManager.setMatrix(cIntrinsicDirShadowWorldToTex, mWorldToTex, cMaxPasses);
   }         
}

#ifndef BUILD_FINAL   
void BDirShadowManager::drawDebugInfo(void)
{
   static bool renderFlag = true;
   if (!renderFlag)
      return;
    
   gRenderDraw.setTexture(0, mpShadowBuffer);
   gRenderDraw.setSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
   gRenderDraw.setSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
   gRenderDraw.setSamplerState(0, D3DSAMP_ADDRESSW, D3DTADDRESS_CLAMP);
      
   gRenderDraw.setSamplerState(0, D3DSAMP_SEPARATEZFILTERENABLE, TRUE);
   gRenderDraw.setSamplerState(0, D3DSAMP_MINFILTERZ, D3DTEXF_POINT);
   gRenderDraw.setSamplerState(0, D3DSAMP_MAGFILTERZ, D3DTEXF_POINT);
   
   gRenderDraw.setSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
   gRenderDraw.setSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
   gRenderDraw.setSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
   gRenderDraw.setSamplerState(0, D3DSAMP_MAXANISOTROPY, 1);
   
   //for (uint pass = 0; pass < cMaxPasses; pass++)
   for (uint pass = 0; pass < 1; pass++)
   {
      BVec4 mul(-1.0f);
      BVec4 add(1.0f);
      gRenderDraw.setPixelShaderConstantF(0, mul.getPtr(), 1);
      gRenderDraw.setPixelShaderConstantF(1, add.getPtr(), 1);

      // 0.16666666666666666666666666666667   
      // 0.5
      // 0.83333333333333333333333333333333
      
      BVec4 slice(1.0f/(2.0f*cMaxPasses) + (float)pass / cMaxPasses);
      gRenderDraw.setPixelShaderConstantF(2, slice.getPtr(), 1);
      
      uint xOfs = 50 + pass * 300;
      uint yOfs = 50;
      const uint cWidth = 512;//256;
      const uint cHeight = 512;//256;
      BPrimDraw2D::drawSolidRect2D(xOfs, yOfs, xOfs+cWidth, yOfs+cHeight, 0.0f, 0.0f, 1.0f, 1.0f, 0xFFFFFFFF, 0xFFFFFFFF, cPosTex1VS, cDepthVisPS);
      
      for (uint i = 0; i < mViewPoints[pass].size(); i++)
      {
         const BVec3& p = mViewPoints[pass][i];
         
         XMVECTOR v = XMVector4Transform(XMVectorSet(p[0], p[1], p[2], 1.0f), mViewToProj[pass]);
         
         if (v.w != 0.0f)
         {
            v.x /= v.w;
            v.y /= v.w;
            v.z /= v.w;
         }
                  
         int x = (int)(cWidth * (v.x * .5f + .5f));
         int y = (int)(cHeight * (v.y * -.5f + .5f));
         
         BPrimDraw2D::drawLine2D(xOfs+x-8, yOfs+y, xOfs+x+8, yOfs+y, 0xFF00FF00);
         BPrimDraw2D::drawLine2D(xOfs+x, yOfs+y-8, xOfs+x, yOfs+y+8, 0xFF00FF00);
      }
      
      {
         const BVec3 s(gRenderDraw.getMainSceneMatrixTracker().getWorldCamPos());
         const BVec3 e(gRenderDraw.getMainSceneMatrixTracker().getWorldCamPos() + gRenderDraw.getMainSceneMatrixTracker().getWorldAt() * 20.0f);
         
         XMVECTOR vs = XMVector4Transform(XMVectorSet(s[0], s[1], s[2], 1.0f), mWorldToProj[pass]);
         if (vs.w != 0.0f)
         {
            vs.x /= vs.w;
            vs.y /= vs.w;
            vs.z /= vs.w;
         }
         
         XMVECTOR ve = XMVector4Transform(XMVectorSet(e[0], e[1], e[2], 1.0f), mWorldToProj[pass]);
         if (ve.w != 0.0f)
         {
            ve.x /= ve.w;
            ve.y /= ve.w;
            ve.z /= ve.w;
         }
         
         int xs = (int)(cWidth * (vs.x * .5f + .5f));
         int ys = (int)(cHeight * (vs.y * -.5f + .5f));
         int xe = (int)(cWidth * (ve.x * .5f + .5f));
         int ye = (int)(cHeight * (ve.y * -.5f + .5f));
         BPrimDraw2D::drawLine2D(xOfs+xs-8, yOfs+ys, xOfs+xs+8, yOfs+ys, 0xFFFF00FF);
         BPrimDraw2D::drawLine2D(xOfs+xs, yOfs+ys-8, xOfs+xs, yOfs+ys+8, 0xFFFF00FF);
         
         BPrimDraw2D::drawLine2D(xOfs+xs, yOfs+ys, xOfs+xe, yOfs+ye, 0xFFFF00FF);
      }         
   }      
   
   gRenderDraw.setSamplerState(0, D3DSAMP_SEPARATEZFILTERENABLE, FALSE);
}
#endif   















