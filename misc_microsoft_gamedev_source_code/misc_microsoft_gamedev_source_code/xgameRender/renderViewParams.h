// File: renderViewParams.h
#pragma once

// xsystem
#include "point.h"

#pragma warning(push)
#pragma warning(disable:4324) // pad warning
class BRenderViewParams
{
public:
   BRenderViewParams();
   BRenderViewParams(const BRenderViewParams& other);
         
   BRenderViewParams& operator= (const BRenderViewParams& rhs);
     
   void                    clear(void);
         
   // Viewport and frustum
   void                    disableAspectRatioScaling(void);
   void                    restoreAspectRatioScaling(void);
   
   void                    calcBackBufferToDisplayScale(float displayWidth, float displayHeight);
   
   void					      setupBackBufferToDisplayScale(void);
   
   bool                    setViewportAndProjection(
                              int minX, int minY, 
                              int width, int height, 
                              float minZ, float maxZ, 
                              float fov,
                              int extraClipLeft = 0, 
                              int extraClipRight = 0, 
                              int extraClipTop = 0, 
                              int extraClipBottom = 0);

   // These methods only change the indicated values. All other viewport parameters remain unmodified..
   // They call setViewportAndProjection() internally.
   void                    setExtraClip(int left, int right, int top, int bottom);
   void                    setNearZ(float nearZ);
   void                    setFarZ(float farZ);
   void                    setFOV(float fov);

   // Accessors for frustum info.  These will not work if you manually set projection matrix or viewport.  
   // You must use setViewportAndProjection for these to be accurate.
   
   float                   getNearZ(void) const { return mNearZ; }
   float                   getFarZ(void) const { return mFarZ; }
   float                   getScaleX(void) const { return mScaleX; }
   float                   getScaleY(void) const { return mScaleY; }
   float                   getFOV(void) const { return mFOV; }
   
   int                     getExtraClipLeft(void) const { return mExtraClipLeft; }
   int                     getExtraClipRight(void) const { return mExtraClipRight; }
   int                     getExtraClipTop(void) const { return mExtraClipTop; }
   int                     getExtraClipBottom(void) const { return mExtraClipBottom; }
   
   int                     getViewportMinX(void) const { return mViewportMinX; }
   int                     getViewportMinY(void) const { return mViewportMinY; }
   int                     getViewportWidth(void) const { return mViewportWidth; }
   int                     getViewportHeight(void) const { return mViewportHeight; }
   float                   getViewportHalfWidth() const { return mViewportHalfWidth; }
   float                   getViewportHalfHeight() const { return mViewportHalfHeight; }

   void                    setOffCenterFactor(float x1, float y1, float x2, float y2);
   void                    getOffCenterFactor(float& x1, float& y1, float& x2, float& y2) const;
   
   bool                    isUsingOffCenterFactors(void) const;

   // Ray origin is camera's location.
   // rg [2/8/06] - I have not tested any of these methods. No idea if they actually work as advertised.
   void                    calculateWorldRay(float screenX, float screenY, BVector &vector) const;
   void                    calculateWorldRay(float screenX, float screenY, const D3DXMATRIX& viewMtx, float fov, BVector &vector) const;
   void                    calculateWorldToScreen(const BVector worldPt, BPoint& screenPt) const;
   void                    calculateWorldToScreen(const BVector worldPt, long& x, long& y) const;
   void                    calculateWorldToScreen(const BVector worldPt, float& x, float& y) const;

   bool                    isPointOnScreen(const BVector point) const;

   // World->View matrix methods.
   void                    setViewMatrixIdentity(void);
   void                    setViewMatrix(const float* mat);
   void                    setViewMatrix(const BMatrix &mat);
   
   const D3DXMATRIXA16&    getViewMatrix(void) const { return mViewMatrix; }   
   const BMatrix&          getViewBMatrix(void) const { return mViewBMatrix; }
      
   // If you directly set the projection matrix or viewport, the above accessors we not return valid information.
   void                    setProjectionMatrix(const float* mat);
   const D3DXMATRIXA16&    getProjectionMatrix(void) const { return mProjectionMatrix; }
      
   // The viewport matrix is the standard, constant D3D projection->viewport matrix.
   const D3DXMATRIXA16&    getViewportMatrix(void) const { return mViewportMatrix; }   
   
   // Note that setDeviceViewport doesn't actually set the viewport to the device. Yes this is badly named, but this whole class is kinda fucked up anyway. 
   void                    setDeviceViewport(uint x, uint y, uint width, uint height, float minZ, float maxZ);
   
   // The device viewport structure is updated by setViewportAndProjection() or setDeviceViewport().
   const D3DVIEWPORT9&     getDeviceViewport(void) const { return mDeviceViewport; }
         
private:   

   // Don't put anything here that can't be bitwise copied.      
   D3DVIEWPORT9            mDeviceViewport;
      
   // Front/back of view frustum.
   float                   mNearZ;
   float                   mFarZ;
   float                   mScaleX;
   float                   mScaleY;

   // More viewport/projection stuff.
   float                   mFOV;
   int                     mExtraClipLeft;
   int                     mExtraClipRight;
   int                     mExtraClipTop;
   int                     mExtraClipBottom;
   int                     mViewportMinX;
   int                     mViewportMinY;
   int                     mViewportWidth;
   int                     mViewportHeight;
   float                   mViewportHalfWidth;
   float                   mViewportHalfHeight;
   float                   mOffCenterXFactor1;
   float                   mOffCenterYFactor1;
   float                   mOffCenterXFactor2;
   float                   mOffCenterYFactor2;
   
   float                   mScaleBackBufferToDisplay;
   float                   mScaleDisplayToBackBuffer;
   float                   mSaveScaleBackBufferToDisplay;
   float                   mSaveScaleDisplayToBackBuffer;
   
   D3DXMATRIXA16           mViewMatrix;
   D3DXMATRIXA16           mProjectionMatrix;
   D3DXMATRIXA16           mViewportMatrix;        
   BMatrix                 mViewBMatrix;           
};   
#pragma warning(pop)
