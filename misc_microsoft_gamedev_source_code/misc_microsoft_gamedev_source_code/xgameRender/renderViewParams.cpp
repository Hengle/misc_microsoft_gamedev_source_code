// File: renderViewParams.cpp
#include "xgameRender.h"
#include "renderViewParams.h"

#include "renderDraw.h"
#include "mathutil.h"

#include "configsgamerender.h"

//============================================================================
// BRenderViewParams::BRenderViewParams
//============================================================================
BRenderViewParams::BRenderViewParams()
{
   clear();
}

//============================================================================
// BRenderViewParams::BRenderViewParams
//============================================================================
BRenderViewParams::BRenderViewParams(const BRenderViewParams& other)
{
   Utils::FastMemCpy(this, &other, sizeof(other));
}

//============================================================================
// BRenderViewParams::operator=
//============================================================================
BRenderViewParams& BRenderViewParams::operator= (const BRenderViewParams& rhs)
{
   if (this != &rhs)
      Utils::FastMemCpy(this, &rhs, sizeof(rhs));
   return *this;      
}

//============================================================================
// BRenderViewParams::clear
//============================================================================
void BRenderViewParams::clear(void)
{
   mNearZ = 0.0f;
   mFarZ = 0.0f;
   mScaleX = 1.0f;
   mScaleY = 1.0f;

   mFOV = 45.0f*cRadiansPerDegree;
   mExtraClipLeft = 0;
   mExtraClipRight = 0;
   mExtraClipTop = 0;
   mExtraClipBottom = 0;
   mViewportMinX = 0;
   mViewportMinY = 0;
   mViewportWidth = 640;
   mViewportHeight = 480;
   mViewportHalfWidth = 320.0f;
   mViewportHalfHeight = 240.0f;
   mOffCenterXFactor1 = 0.0f;
   mOffCenterYFactor1 = 0.0f;
   mOffCenterXFactor2 = 1.0f;
   mOffCenterYFactor2 = 1.0f;

   mScaleBackBufferToDisplay = 1.0f;
   mScaleDisplayToBackBuffer = 1.0f;
   mSaveScaleBackBufferToDisplay = 1.0f;
   mSaveScaleDisplayToBackBuffer = 1.0f;

   D3DXMatrixIdentity(&mViewMatrix);
   D3DXMatrixIdentity(&mProjectionMatrix);
   
   D3DXMatrixIdentity(&mViewportMatrix);
   mViewportMatrix._11 = 0.5f;
   mViewportMatrix._22 = -0.5f;
   mViewportMatrix._33 = 0.0f;
   mViewportMatrix._41 = 0.5f;
   mViewportMatrix._42 = 0.5f;
   mViewportMatrix._43 = 1.0f;
   mViewportMatrix._44 = 1.0f;
   
   mViewBMatrix.makeIdentity();
   
   mDeviceViewport.X = 0;
   mDeviceViewport.Y = 0;
   mDeviceViewport.Width = 640;
   mDeviceViewport.Height = 480;
}

//============================================================================
// BRenderViewParams::disableAspectRatioScaling
//============================================================================
void BRenderViewParams::disableAspectRatioScaling(void)
{
   mSaveScaleBackBufferToDisplay=mScaleBackBufferToDisplay;
   mSaveScaleDisplayToBackBuffer=mScaleDisplayToBackBuffer;

   mScaleBackBufferToDisplay=1.0f;
   mScaleDisplayToBackBuffer=1.0f;
}

//============================================================================
// BRenderViewParams::restoreAspectRatioScaling
//============================================================================
void BRenderViewParams::restoreAspectRatioScaling(void)
{
   mScaleBackBufferToDisplay=mSaveScaleBackBufferToDisplay;
   mScaleDisplayToBackBuffer=mSaveScaleDisplayToBackBuffer;
}

//============================================================================
// BRenderViewParams::setupBackBufferToDisplayScale
//============================================================================
void BRenderViewParams::setupBackBufferToDisplayScale(void)
{
   // ajl 3/8/05 - Support for stretching a 4:3 back buffer (such as 640x480)
   // to fullscreen on a 16:9 display (such as 1280x720). Normally the output 
   // will be stretched. To get around that, scale the x and width values that
   // are used to calculate the projection matrix. The "displayAspectRatio" 
   // config should be set to the native resolution of the display (such as
   // "1280:720".

   mScaleBackBufferToDisplay=1.0f;
   mScaleDisplayToBackBuffer=1.0f;
   
   if(!gConfig.isDefined(cConfigDisplayAspectRatio))
      return;

   BSimString aspectRatioText;
   gConfig.get(cConfigDisplayAspectRatio, aspectRatioText);

   BSimString token;
   int loc=token.copyTok(aspectRatioText, -1, -1, B(":xX,/;- "));
   if(loc==-1)
      return;
   long displayWidth=token.asLong();

   token.copyTok(aspectRatioText, -1, loc+1);
   long displayHeight=token.asLong();

   if(displayWidth==0 || displayHeight==0)
      return;

   calcBackBufferToDisplayScale((float)displayWidth, (float)displayHeight);
}

//============================================================================
// BRenderDevice::calcBackBufferToDisplayScale
//============================================================================
void BRenderViewParams::calcBackBufferToDisplayScale(float displayWidth, float displayHeight)
{
   float displayAspectRatio=displayHeight/displayWidth;

   float backBufferWidth=(float)gRenderDraw.getDisplayMode().Width;
   float backBufferHeight=(float)gRenderDraw.getDisplayMode().Height;
   float backBufferAspectRatio=backBufferHeight/backBufferWidth;

   if(floatEqual(displayAspectRatio, backBufferAspectRatio, 0.001f))
   {
      mScaleBackBufferToDisplay=1.0f;
      mScaleDisplayToBackBuffer=1.0f;
      return;
   }

   float adjustedBackBufferWidth=backBufferHeight/displayAspectRatio;

   mScaleBackBufferToDisplay=adjustedBackBufferWidth/backBufferWidth;
   mScaleDisplayToBackBuffer=backBufferWidth/adjustedBackBufferWidth;
}

//============================================================================
// BRenderViewParams::setViewportAndProjection
//============================================================================
bool BRenderViewParams::setViewportAndProjection(int minX, int minY, int width, int height, float minZ, float maxZ, float fov,
   int extraClipLeft, int extraClipRight, int extraClipTop, int extraClipBottom)
{
   BDEBUG_ASSERT((minZ > 0.0f) && (minZ < maxZ));
   
   int saveMinX=minX;
   int saveWidth=width;

   // ajl 3/9/05 - See comment in setupBackBufferToDisplayScale()
   if(mScaleBackBufferToDisplay!=1.0f)
   {
      width=(int)((width*mScaleBackBufferToDisplay)+0.5f);
      minX=(int)((minX*mScaleBackBufferToDisplay)+0.5f);
   }

   // Get a properly projection matrix that takes the viewport into account.
   int vx, vy, vw, vh;
   RECT scirect, vwprect, xrect;
   
   scirect.left   = minX + extraClipLeft;
   scirect.top    = minY + extraClipTop;
   scirect.right  = minX + width - extraClipRight;
   scirect.bottom = minY + height - extraClipBottom;
   vwprect.left   = minX; 
   vwprect.top    = minY; 
   vwprect.right  = minX + width; 
   vwprect.bottom = minY + height; 

   if(!IntersectRect(&xrect, &scirect, &vwprect))
   {
      BFAIL("Viewport must be on screen.");
      return(false);
   }

   if(EqualRect(&xrect, &vwprect)) // Check whether viewport is completely within scissor rect
   {
      vx = minX; 
      vy = minY;
      vw = width; 
      vh = height; 
   }
   else
   {
      // We need to use xrect rather than scirect (ie clip scissor rect to viewport)
      vx = xrect.left; 
      vy = xrect.top; 
      vw = xrect.right - xrect.left; 
      vh = xrect.bottom - xrect.top; 

   }

   float dvClipX, dvClipY, dvClipWidth, dvClipHeight;
   if(EqualRect(&xrect, &vwprect)) // Check whether viewport is completely within scissor rect
   {
      dvClipX = -1.f;
      dvClipY = -1.f;
      dvClipWidth = 2.f;
      dvClipHeight = 2.f;
   }
   else
   {
      // We need to use xrect rather than scirect (ie clip scissor rect to viewport)
      // and transform the clipped scissor rect into viewport relative coordinates
      // to "correctly" compute the clip stuff
      dvClipX = (2.f * (xrect.left - minX)) / width - 1.0f;
      dvClipY = (2.f * (xrect.top - minY)) / height - 1.0f;
      dvClipWidth = (2.f * (xrect.right - xrect.left)) / width;
      dvClipHeight = (2.f * (xrect.bottom - xrect.top)) / height;
   }

   D3DXMATRIX c;
   c._11 = 2.f / dvClipWidth;
   c._21 = 0.f;
   c._31 = 0.f;
   c._41 = -1.f - 2.f * (dvClipX / dvClipWidth);
   c._12 = 0.f;
   c._22 = 2.f / dvClipHeight;
   c._32 = 0.f;
   c._42 = 1.f + 2.f * (dvClipY / dvClipHeight);
   c._13 = 0.f;
   c._23 = 0.f;
   c._33 = 1.f;
   c._43 = 0.f;
   c._14 = 0.f;
   c._24 = 0.f;
   c._34 = 0.f;
   c._44 = 1.f;

   // Create basic projection matrix.
   float aspectRatio=float(width)/float(height);
   D3DXMATRIXA16 matProj;

   float fovToUse = fov;

   float halfFov = 0.5f*fovToUse;
   float top = minZ/(cos(halfFov) / sin(halfFov));
   //float bottom = -top;
   float right = top*aspectRatio;
   //float left = -right;

   D3DXMatrixPerspectiveOffCenterLH(&matProj, right*(2.0f*mOffCenterXFactor1-1.0f), right*(2.0f*mOffCenterXFactor2-1.0f), 
      top*(2.0f*mOffCenterYFactor1-1.0f), top*(2.0f*mOffCenterYFactor2-1.0f), minZ, maxZ);

   //D3DXMatrixPerspectiveFovLH(&matProj, fovToUse, aspectRatio, minZ, maxZ);

   // Multiply in wacky clip matrix.
   D3DXMATRIXA16 clipProject;
   D3DXMatrixMultiply(&clipProject, &matProj, &c);

   // Set this as the projection matrix.
   setProjectionMatrix((float*)&clipProject);

   // Set the D3D viewport.
   if(mScaleDisplayToBackBuffer!=1.0f)
   {
      // ajl 3/9/05 - See comment in setupBackBufferToDisplayScale()
      int tvx=(int)((vx*mScaleDisplayToBackBuffer)+0.5f);
      int tvw=(int)((vw*mScaleDisplayToBackBuffer)+0.5f);
      setDeviceViewport(tvx, vy, tvw, vh, 0.0f, 1.0f);
   }
   else
      setDeviceViewport(vx, vy, vw, vh, 0.0f, 1.0f);

   float fovFactor=cosf(0.5f*fov)/sinf(0.5f*fov);
   mScaleX = 0.5f*height*fovFactor;
   mScaleY = 0.5f*height*fovFactor;

   // calculate values that define side and top/bottom clipping planes
   //float rightCoeff = -(width-2.0f*extraClipRight)/(2*mScaleX);
   //float leftCoeff = -(width-2.0f*extraClipLeft)/(2*mScaleX);
   //float topCoeff = -(height-2.0f*extraClipTop)/(2*mScaleY);
   //float bottomCoeff = -(height-2.0f*extraClipBottom)/(2*mScaleY);

   // Save off z planes.
   mNearZ=minZ;
   mFarZ=maxZ;

   // Save fov.
   mFOV=fov;

   // Save extra clip stuff.
   mExtraClipLeft = extraClipLeft;
   mExtraClipRight = extraClipRight;
   mExtraClipTop = extraClipTop;
   mExtraClipBottom = extraClipBottom;

   // Save off viewport params.
   mViewportMinX = saveMinX;
   mViewportMinY = minY;
   mViewportWidth = saveWidth;
   mViewportHeight = height;
   mViewportHalfWidth = (float)saveWidth*0.5f;
   mViewportHalfHeight = (float)height*0.5f;

   // Multiple scale X by the display to backbuffer scale so that screen to world/world to screen calculations will come out right.
   mScaleX*=mScaleDisplayToBackBuffer;

   // Success.
   return(true);
}   

//============================================================================
// BRenderViewParams::setExtraClip
//============================================================================
void BRenderViewParams::setExtraClip(int left, int right, int top, int bottom)
{
   setViewportAndProjection(getViewportMinX(), getViewportMinY(), getViewportWidth(), getViewportHeight(), getNearZ(), getFarZ(), getFOV(), left, right, top, bottom);
}

//============================================================================
// BRenderViewParams::setNearZ
//============================================================================
void BRenderViewParams::setNearZ(float nearZ)
{
   setViewportAndProjection(getViewportMinX(), getViewportMinY(), 
      getViewportWidth(), getViewportHeight(), nearZ, getFarZ(), getFOV(), 
      getExtraClipLeft(), getExtraClipRight(), getExtraClipTop(), getExtraClipBottom());
}

//============================================================================
// BRenderViewParams::setFarZ
//============================================================================
void BRenderViewParams::setFarZ(float farZ)
{
   setViewportAndProjection(getViewportMinX(), getViewportMinY(), 
      getViewportWidth(), getViewportHeight(), getNearZ(), farZ, getFOV(), 
      getExtraClipLeft(), getExtraClipRight(), getExtraClipTop(), getExtraClipBottom());
}

//============================================================================
// BRenderViewParams::setFOV
//============================================================================
void BRenderViewParams::setFOV(float fov)
{
   setViewportAndProjection(getViewportMinX(), getViewportMinY(), 
      getViewportWidth(), getViewportHeight(), getNearZ(), getFarZ(), fov, 
      getExtraClipLeft(), getExtraClipRight(), getExtraClipTop(), getExtraClipBottom());
}

//============================================================================
// BRenderViewParams::setOffCenterFactor
//============================================================================
void BRenderViewParams::setOffCenterFactor(float x1, float y1, float x2, float y2)
{
   mOffCenterXFactor1=x1; 
   mOffCenterYFactor1=y1; 
   mOffCenterXFactor2=x2; 
   mOffCenterYFactor2=y2;
}

//============================================================================
// BRenderViewParams::getOffCenterFactor
//============================================================================
void BRenderViewParams::getOffCenterFactor(float& x1, float& y1, float& x2, float& y2) const
{  
   x1 = mOffCenterXFactor1; 
   y1 = mOffCenterYFactor1; 
   x2 = mOffCenterXFactor2; 
   y2 = mOffCenterYFactor2; 
}

//============================================================================
// BRenderViewParams::isUsingOffCenterFactors
//============================================================================
bool BRenderViewParams::isUsingOffCenterFactors(void) const
{
   return (mOffCenterXFactor1 != 0.0f) || (mOffCenterYFactor1 != 0.0f) || (mOffCenterXFactor2 != 1.0f) || (mOffCenterYFactor2 != 1.0f); 
}

//============================================================================
// BRenderViewParams::calculateWorldRay
//============================================================================
void BRenderViewParams::calculateWorldRay(float screenX, float screenY, BVector &vector) const
{
   calculateWorldRay(screenX, screenY, mViewMatrix, mFOV, vector);
}

//============================================================================
// BRenderViewParams::calculateWorldRay
//============================================================================
void BRenderViewParams::calculateWorldRay(float screenX, float screenY, const D3DXMATRIX& viewMtx, float fov, BVector &vector) const
{
   const uint width = getViewportWidth();
   const uint height = getViewportHeight();

   float fovFactor=cosf(0.5f*fov)/sinf(0.5f*fov);
   float scaleX = float(height/2) * fovFactor;
   float scaleY = scaleX;
   float dX=screenX-float(width/2);
   float dY=screenY-float(height/2);
   BVector temp;
   temp.x=(mNearZ/scaleX)*(float)dX;
   temp.y=-(mNearZ/scaleY)*(float)dY;
   temp.z=mNearZ;
   temp.normalize();

   vector.x=temp.x*viewMtx._11;
   vector.y=temp.x*viewMtx._21;
   vector.z=temp.x*viewMtx._31;

   vector.x+=temp.y*viewMtx._12;
   vector.y+=temp.y*viewMtx._22;
   vector.z+=temp.y*viewMtx._32;

   vector.x+=temp.z*viewMtx._13;
   vector.y+=temp.z*viewMtx._23;
   vector.z+=temp.z*viewMtx._33;
}

//============================================================================
// BRenderViewParams::calculateWorldToScreen
//============================================================================
void BRenderViewParams::calculateWorldToScreen(const BVector worldPt, BPoint& screenPt) const
{
   calculateWorldToScreen(worldPt, screenPt.x, screenPt.y);
}

//============================================================================
// BRenderViewParams::calculateWorldToScreen
//============================================================================
void BRenderViewParams::calculateWorldToScreen(const BVector worldPt, long& x, long& y) const
{
   BVector camPoint, temp;
   getViewBMatrix().transformVectorAsPoint(worldPt, camPoint);

   temp.z = 1.0f/camPoint.z;
   temp.x = camPoint.x*getScaleX()*temp.z + 0.5f*getViewportWidth();
   temp.y = -camPoint.y*getScaleY()*temp.z + 0.5f*getViewportHeight();
   x = (int)temp.x;
   y = (int)temp.y;
}

//============================================================================
// BRenderViewParams::calculateWorldToScreen
//============================================================================
void BRenderViewParams::calculateWorldToScreen(const BVector worldPt, float& x, float& y) const
{
   BVector camPoint;
   mViewBMatrix.transformVectorAsPoint(worldPt, camPoint);

   x=mViewportHalfWidth;
   y=mViewportHalfHeight;

   float scaleX=camPoint.x*mScaleX;

   float z = 1.0f/camPoint.z;

   float scaleY=camPoint.y*mScaleY;

   x += scaleX*z;
   y += -scaleY*z;
}

//============================================================================
// BRenderViewParams::isPointOnScreen
//============================================================================
bool BRenderViewParams::isPointOnScreen(const BVector point) const
{
   // Transform center point into view space.
   BVector xformPos;
   getViewBMatrix().transformVectorAsPoint(point, xformPos);

   // Check if behind or past the nearz plane.
   if(xformPos.z<getNearZ() || xformPos.z>getFarZ())
      return(false);

   // Project it.
   float ooz=1.0f/xformPos.z;
   float x=getScaleX()*xformPos.x*ooz+0.5f*getViewportWidth();
   float y=-getScaleY()*xformPos.y*ooz+0.5f*getViewportHeight();

   // Check if offscreen.
   if(x<0.0f)
      return(false);
   if(x>getViewportWidth())
      return(false);
   if(y<0.0f)
      return(false);
   if(y>getViewportHeight())
      return(false);

   // Onscreen.
   return(true);
}

//============================================================================
// BRenderViewParams::setProjectionMatrix
//============================================================================
void BRenderViewParams::setProjectionMatrix(const float* mat)
{
   Utils::FastMemCpy(&mProjectionMatrix, mat, sizeof(mProjectionMatrix));
}

//============================================================================
// BRenderViewParams::setViewMatrix
//============================================================================
void BRenderViewParams::setViewMatrix(const float* mat)
{
   Utils::FastMemCpy(&mViewMatrix, mat, sizeof(mViewMatrix));
   
   mViewBMatrix.setD3DXMatrix(*(const D3DMATRIX*)mat);
}

//============================================================================
// BRenderDevice::setViewMatrix
//============================================================================
void BRenderViewParams::setViewMatrix(const BMatrix &mat)
{
   mViewBMatrix = mat;

   D3DMATRIX d3dmat;
   mat.getD3DXMatrix(d3dmat);
   setViewMatrix((float*)&d3dmat);
}


//============================================================================
// BRenderDevice::setViewMatrixIdentity
//============================================================================
void BRenderViewParams::setViewMatrixIdentity(void)
{
   BMatrix identity;
   identity.makeIdentity();
   setViewMatrix(identity);
}

//============================================================================
// BRenderViewParams::setDeviceViewport
//============================================================================
void BRenderViewParams::setDeviceViewport(uint x, uint y, uint width, uint height, float minZ, float maxZ)
{
   mDeviceViewport.X = x;
   mDeviceViewport.Y = y;
   mDeviceViewport.Width = width;
   mDeviceViewport.Height = height;
   mDeviceViewport.MinZ = minZ;
   mDeviceViewport.MaxZ = maxZ;
}

