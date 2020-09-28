//==============================================================================
// camera.cpp
//
// Copyright (c) 1998, 1999 Ensemble Studios
//==============================================================================

//==============================================================================
// Includes
#include "xgameRender.h"
#include "camera.h"
#include "mathutil.h"
#include "chunker.h"
#include "tonemapManager.h"
#include "render.h"
#include "gamefilemacros.h"

// xsystem
#include "bfileStream.h"

#define SAVE_CHECK_OK if(!ok) {setBlogError(4186); blogerror("BCamera::save -- error writing to file."); return(false);}
#define SAVE_CHECK_RESULT if(!result) {setBlogError(4187); blogerror("BCamera::save -- error writing to file."); return(false);}
#define LOAD_CHECK_OK if(!ok) {setBlogError(4188); blogerror("BCamera::load -- error reading from file."); return(false);}
#define LOAD_CHECK_RESULT if(!result) {setBlogError(4189); blogerror("BCamera::load -- error reading from file."); return(false);}


bool getXZZoom(const BVector &dir, const BVector &pos, float &dist)
{
   if(fabs(dir.y)<cFloatCompareEpsilon)
      return(false);

   // Get distance to plane y=0.
   float ooDirY = 1.0f/dir.y;
   float dx=dir.x*pos.y*ooDirY;
   float dz=dir.z*pos.y*ooDirY;
   dist=(float)sqrt(dx*dx + pos.y*pos.y + dz*dz);

   // Seems we want distance "above" the plane, reverse the sign of the distance
   // if we are in fact below the plane.
   if(pos.y<0.0f)
      dist=-dist;

   return(true);
}


bool getXZPitch(const BVector &dir, const BVector &up, float &angle)
{
   // Get the angle between the direction vector and the xz plane by taking inverse 
   // cosine of the dot product of the vector with its projection on the xz plane.
   BVector proj = BVector(dir.x, 0.0f, dir.z);
   float length = proj.length();
   if(length<cFloatCompareEpsilon)
      return(false);

   // Normalize.
   proj /= length;

   float cosAngle = dir.x*proj.x + dir.z*proj.z;
   angle = (float)acos(cosAngle);

   //Adjust the angle accordingly.  We map the angles as follows:
   //
   //              PI/2
   //               |
   //               |
   //         PI/---+--- 0
   //        -PI    |
   //               | 
   //               -PI/2
   //
   //newCameraDir.y tells us whether the camera is pointing up or down:  >0.0f ==> up, <0.0f ==> down
   //newCameraUp.y tells us if its pointing forward or backward:          >0.0f ==> forward, <0.0f ==> backward
   //
   if(dir.y < 0.0f && up.y > 0.0f)      // down and forward
      angle = -angle;
   if(dir.y < 0.0f && up.y < 0.0f)      // down and backward
      angle -= cPi;
   if(dir.y > 0.0f && up.y < 0.0f)      // up and backward
      angle = cPi-angle;

   return(true);
}




//==============================================================================
// BCameraEffectData
//==============================================================================

//==============================================================================
//==============================================================================
void BCameraEffectData::reset()
{
   mColorTransformRTable.clear();
   mColorTransformGTable.clear();
   mColorTransformBTable.clear();
   mColorTransformFactorTable.clear();
   mBlurFactorTable.clear();
   mBlurXYTable.clear();
   mRadialBlurFactorTable.clear();
   mFOVTable.clear();
   mZoomTable.clear();
   mYawTable.clear();
   mPitchTable.clear();
   mRadialBlur = false;
   mUse3DPosition = false;
   mModeCameraEffect = false;
   mUserHoverPointAs3DPosition = false;
}

//==============================================================================
//==============================================================================
void BCameraEffectData::makeClearData(DWORD duration, uint viewportIndex)
{
   reset();
   float endTime = duration * 0.001f;

   BToneMapParams params = gToneMapManager.getParams(viewportIndex);

   mColorTransformRTable.addKeyValue(0.0f, params.mRTransform);
   mColorTransformRTable.addKeyValue(endTime, BVector(1.0f, 0.0f, 0.0f));
   mColorTransformGTable.addKeyValue(0.0f, params.mGTransform);
   mColorTransformGTable.addKeyValue(endTime, BVector(0.0f, 1.0f, 0.0f));
   mColorTransformBTable.addKeyValue(0.0f, params.mBTransform);
   mColorTransformBTable.addKeyValue(endTime, BVector(0.0f, 0.0f, 1.0f));
   mColorTransformFactorTable.addKeyValue(0.0f, params.mColorTransformFactor);
   mColorTransformFactorTable.addKeyValue(endTime, 0.0f);
   mBlurFactorTable.addKeyValue(0.0f, params.mBlurFactor);
   mBlurFactorTable.addKeyValue(endTime, 0.0f);
   mBlurXYTable.addKeyValue(0.0f, BVector2(params.mBlurX, params.mBlurY));
   mBlurXYTable.addKeyValue(endTime, BVector2(0.0f, 0.0f));
   mRadialBlurFactorTable.addKeyValue(0.0f, BVector2(params.mRadialBlurScale, params.mRadialBlurMax));
   mRadialBlurFactorTable.addKeyValue(endTime, BVector2(0.0f, 0.0f));
   mRadialBlur = false;
   mUse3DPosition = false;
   mUserHoverPointAs3DPosition = false;
   // TODO - fov support
}

//==============================================================================
//==============================================================================
void BCameraEffectData::setInitialValuesToCurrent(float currentZoom, float currentYaw, float currentPitch, uint viewportIndex)
{
   BToneMapParams params = gToneMapManager.getParams(viewportIndex);

   // Only update tables with more than 1 key/value pairs as this means we're transitioning
   // from 1 value to tne next.  Otherwise it is to be ignored (0 keys) or pop without
   // transitioning (1 key).

   // Color transform
   if (mColorTransformFactorTable.getNumKeys() > 1)
      mColorTransformFactorTable.setValue(0, params.mColorTransformFactor);
   if (mColorTransformRTable.getNumKeys() > 1)
      mColorTransformRTable.setValue(0, params.mRTransform);
   if (mColorTransformGTable.getNumKeys() > 1)
      mColorTransformGTable.setValue(0, params.mRTransform);
   if (mColorTransformBTable.getNumKeys() > 1)
      mColorTransformBTable.setValue(0, params.mRTransform);

   // Blur
   if (mBlurFactorTable.getNumKeys() > 1)
      mBlurFactorTable.setValue(0, params.mBlurFactor);
   if (mBlurXYTable.getNumKeys() > 1)
      mBlurXYTable.setValue(0, BVector2(params.mBlurX, params.mBlurY));
   if (mRadialBlurFactorTable.getNumKeys() > 1)
      mRadialBlurFactorTable.setValue(0, BVector2(params.mRadialBlurScale, params.mRadialBlurMax));

   // Camera
   if (mZoomTable.getNumKeys() > 1)
      mZoomTable.setValue(0, currentZoom);
   if (mYawTable.getNumKeys() > 1)
      mYawTable.setValue(0, currentYaw);
   if (mPitchTable.getNumKeys() > 1)
      mPitchTable.setValue(0, currentPitch);
   // TODO - add fov support
}

//==============================================================================
//==============================================================================
float BCameraEffectData::getMaxTime() const
{
   float maxTime = 0.0f;

   maxTime = Math::Max(maxTime, mColorTransformRTable.getLastKey());
   maxTime = Math::Max(maxTime, mColorTransformGTable.getLastKey());
   maxTime = Math::Max(maxTime, mColorTransformBTable.getLastKey());
   maxTime = Math::Max(maxTime, mColorTransformFactorTable.getLastKey());
   maxTime = Math::Max(maxTime, mBlurFactorTable.getLastKey());
   maxTime = Math::Max(maxTime, mBlurXYTable.getLastKey());
   maxTime = Math::Max(maxTime, mRadialBlurFactorTable.getLastKey());
   maxTime = Math::Max(maxTime, mFOVTable.getLastKey());
   maxTime = Math::Max(maxTime, mZoomTable.getLastKey());
   maxTime = Math::Max(maxTime, mYawTable.getLastKey());
   maxTime = Math::Max(maxTime, mPitchTable.getLastKey());

   return maxTime;
}

//==============================================================================
//==============================================================================
void BCameraEffectData::load(BXMLNode& node)
{
   for (int i = 0; i < node.getNumberChildren(); i++)
   {
      BXMLNode childNode(node.getChild(i));
      const BPackedString& name = childNode.getName();

      if (name == "Name")
         childNode.getText(mName);
      else if (name == "ColorTransformFactor")
         mColorTransformFactorTable.load(childNode);
      else if (name == "BlurFactor")
         mBlurFactorTable.load(childNode);
      else if (name == "ColorTransformR")
         mColorTransformRTable.load(childNode);
      else if (name == "ColorTransformG")
         mColorTransformGTable.load(childNode);
      else if (name == "ColorTransformB")
         mColorTransformBTable.load(childNode);
      else if (name == "BlurXY")
         mBlurXYTable.load(childNode);
      else if (name == "RadialBlurFactor")
      {
         mRadialBlurFactorTable.load(childNode);
         if (mRadialBlurFactorTable.getKeys().getNumber() > 0)
            mRadialBlur = true;
         else
            mRadialBlur = false;

         bool use3d = false;
         childNode.getAttribValueAsBool("use3DPos", use3d);
         mUse3DPosition = use3d;

         bool useHoverPoint = false;
         childNode.getAttribValueAsBool("useHoverPointAs3DPos", useHoverPoint);
         mUserHoverPointAs3DPosition = useHoverPoint;
      }
      else if (name == "FOV")
         mFOVTable.load(childNode);
      else if (name == "Zoom")
         mZoomTable.load(childNode);
      else if (name == "Yaw")
         mYawTable.load(childNode);
      else if (name == "Pitch")
         mPitchTable.load(childNode);
   }

   // DMG: 11/15/08 - Giving these tables "default" values to prevent cameraEffect weirdness later on.
   // Many of the other values should probably have a similar change but for now doing the minimal change until more
   // issues arise.

   BVector2 emptyVec(0,0);

   if (mBlurXYTable.getNumKeys() == 0)
   {
      mBlurXYTable.addKeyValue(0,emptyVec);
   }

   if (mRadialBlurFactorTable.getNumKeys() == 0)
   {
      mRadialBlurFactorTable.addKeyValue(0,emptyVec);
   }
}

//==============================================================================
//==============================================================================
bool BCameraEffectData::save(BStream* pStream, int saveType) const
{
   GFWRITESTRING(pStream, BSimString, mName, 100);
   GFWRITECLASS(pStream, saveType, mColorTransformRTable);
   GFWRITECLASS(pStream, saveType, mColorTransformGTable);
   GFWRITECLASS(pStream, saveType, mColorTransformBTable);
   GFWRITECLASS(pStream, saveType, mColorTransformFactorTable);
   GFWRITECLASS(pStream, saveType, mBlurFactorTable);
   GFWRITECLASS(pStream, saveType, mBlurFactorTable);
   GFWRITECLASS(pStream, saveType, mBlurFactorTable);
   GFWRITECLASS(pStream, saveType, mFOVTable);
   GFWRITECLASS(pStream, saveType, mZoomTable);
   GFWRITECLASS(pStream, saveType, mYawTable);
   GFWRITECLASS(pStream, saveType, mPitchTable);
   GFWRITEBITBOOL(pStream, mRadialBlur);
   GFWRITEBITBOOL(pStream, mUse3DPosition);
   GFWRITEBITBOOL(pStream, mModeCameraEffect);
   GFWRITEBITBOOL(pStream, mUserHoverPointAs3DPosition);
   return true;
}

//==============================================================================
//==============================================================================
bool BCameraEffectData::load(BStream* pStream, int saveType)
{
   GFREADSTRING(pStream, BSimString, mName, 100);
   GFREADCLASS(pStream, saveType, mColorTransformRTable);
   GFREADCLASS(pStream, saveType, mColorTransformGTable);
   GFREADCLASS(pStream, saveType, mColorTransformBTable);
   GFREADCLASS(pStream, saveType, mColorTransformFactorTable);
   GFREADCLASS(pStream, saveType, mBlurFactorTable);
   GFREADCLASS(pStream, saveType, mBlurFactorTable);
   GFREADCLASS(pStream, saveType, mBlurFactorTable);
   GFREADCLASS(pStream, saveType, mFOVTable);
   GFREADCLASS(pStream, saveType, mZoomTable);
   GFREADCLASS(pStream, saveType, mYawTable);
   GFREADCLASS(pStream, saveType, mPitchTable);
   GFREADBITBOOL(pStream, mRadialBlur);
   GFREADBITBOOL(pStream, mUse3DPosition);
   GFREADBITBOOL(pStream, mModeCameraEffect);
   GFREADBITBOOL(pStream, mUserHoverPointAs3DPosition);
   return true;
}


//==============================================================================
//==============================================================================
// BCamera
//==============================================================================
//==============================================================================

//==============================================================================
// BCamera::msSaveVersion
//==============================================================================
// Version history:
// 0: initial version
// 1: removed cameratype from scenario save (Xemu, 5/8/00)
const DWORD BCamera::msSaveVersion=6;


//==============================================================================
// BCamera::BCamera(void)
//
// Creates a camera at the origin looking down the positive z-axis with the
// the positive y-axis being up.
//==============================================================================
BCamera::BCamera(void) :
   mPositioningType(0),
   mMaximumPositionDifference(0.0f),
   mMaximumPositionChange(0.0f),
   mPositionChangeSpeed(1.0f),
   mPositionInertia(0.0f),
   mMoving(false),
   mLimitedLastTime(false),
   lastPositionDifferenceLength(0.0f),
   mSpeed(0.03f),
   mTurnspeed(0.001f),
   mMovementType(cStrategyMovement),
   mPitchMin(-cPiOver2),
   mPitchMax(cPiOver2),
   mBob(false),
   mBobOffsetVertical(0.0f),
   mBobOffsetHorizontal(0.0f),
   mMoved(true),
   mZoomLimitOn(false),
   mZoomDistMin(-cMaximumFloat),
   mZoomDistMax(cMaximumFloat),   
   mFOV(40.0f*cRadiansPerDegree),
   mHaveLastLookAtPos(false),
   mLastLookAtPos(0.0f, 0.0f, 0.0f),
   mLastEntityPos(0.0f, 0.0f, 0.0f),
   mLastEntityDir(0.0f, 0.0f, 0.0f),
   mLookAtCameraSpeed(2.0f),
   mShakeTime(0),
   mShakeStrength(0.0f),
   mShakeTrailOffDuration(DWORD(cDefaultShakeTrailOffDuration * 1000)),
   mShakeConservationFactor(cDefaultShakeConservationFactor),
   mEffectStartTime(0),
   mpCurrentCamEffectData(NULL),
   mCurrentCamEffectLocation(0.0f, 0.0f, 0.0f),
   mCamEffectViewportIndex(0),
   mOverride(0L)
{
   mTimer.start();
   
   setCameraLoc(BVector(0.0f, 0.0f, 0.0f));   // camera is at the origin
   setCameraDir(BVector(0.0f, 0.0f, 1.0f));   // camera is looking down positive z-axis
   setCameraUp(BVector(0.0f, 1.0f, 0.0f));      // up is the positive y-axis
   calcCameraRight();         // calculate the corresponding right vector
   setBoundingBoxOn(false);   // don't restrict movement to bounding box
   setBoundingBox(BVector(0.0f, 0.0f, 0.0f), BVector(0.0f, 0.0f, 0.0f));
   setPitchLimitOn(false);      // don't limit pitch
   setDesiredPosition(BVector(0.0f, 0.0f, 0.0f));
   setDesiredPerspective(BVector(0.0f, 0.0f, 0.0f));
   setDesiredRelativePosition(0.0f, 0.0f, 0.0f);
   setDesiredRelativePerspective(0.0f, 0.0f, 0.0f);
   mAccumulatedShake.zero();

   
   mLastMovedTime = 0;
} // BCamera::BCamera

//==============================================================================
// BCamera::BCamera
//
// Creates a camera with the given location, direction, and up vector
// Having the view direction parallel to the up direction will cause
// a degenerate camera to be created.
//==============================================================================
BCamera::BCamera(BVector loc, BVector dir, BVector up) :
   mPositioningType(0),
   mMaximumPositionDifference(0.0f),
   mMaximumPositionChange(0.0f),
   mPositionChangeSpeed(1.0f),
   mPositionInertia(0.0f),
   mMoving(false),
   mLimitedLastTime(false),
   lastPositionDifferenceLength(0.0f),
   mHaveLastLookAtPos(false),
   mLastLookAtPos(0.0f, 0.0f, 0.0f),
   mLastEntityPos(0.0f, 0.0f, 0.0f),
   mLastEntityDir(0.0f, 0.0f, 0.0f),
   mLookAtCameraSpeed(2.0f),
   mEffectStartTime(0),
   mpCurrentCamEffectData(NULL),
   mCurrentCamEffectLocation(0.0f, 0.0f, 0.0f),
   mCamEffectViewportIndex(0)
{
   mTimer.start();
   
   setCameraLoc(loc);
   dir.normalize();
   up.normalize();
   setCameraDir(dir);
   setCameraUp(up);
   calcCameraRight();         // calculate the corresponding right vector
   setBoundingBoxOn(false);   // don't restrict movement to bounding box
   setBoundingBox(BVector(0.0f, 0.0f, 0.0f), BVector(0.0f, 0.0f, 0.0f));
   setDesiredPosition(BVector(0.0f, 0.0f, 0.0f));
   setDesiredPerspective(BVector(0.0f, 0.0f, 0.0f));
   setDesiredRelativePosition(0.0f, 0.0f, 0.0f);
   setDesiredRelativePerspective(0.0f, 0.0f, 0.0f);
      
} // BCamera::BCamera

//==============================================================================
// BCamera::getViewMatrix()
//
// Returns a view transformation matrix for the camera location and orientation
//==============================================================================
void BCamera::getViewMatrix(BMatrix &result) const
{
   result.makeView(getCameraLoc(), getCameraDir(), getCameraUp(), getCameraRight());
} // BCamera::getViewMatrix

//==============================================================================
// BCamera::getInverseViewMatrix()
//
// Returns a view transformation matrix for the camera location and orientation
//==============================================================================
void BCamera::getInverseViewMatrix(BMatrix &result) const
{
   result.makeOrient(getCameraDir(), getCameraUp(), getCameraRight());
   result.multTranslate(getCameraLoc().x, getCameraLoc().y, getCameraLoc().z);
} // BCamera::getInverseViewMatrix


//==============================================================================
// BCamera::zoomTo(float d)
//
// Moves the camera to a SPECIFIC zoom distance
//==============================================================================
void BCamera::zoomTo(float d)
{
   BVector newLoc;
   float currDist;

   bool ok=::getXZZoom(mCameraDir, getCameraLoc(), currDist);
   if (!ok)
      return;

   // move the camera just the differential 
   float adjDist = currDist-abs(d);
   newLoc = getCameraLoc() + adjDist*getCameraDir();

   // If "zoom" limiting is on and we can compute our "zoom" from the y=0 plane, do the check
   if(mZoomLimitOn)
   {
      float dist;
      bool ok2 =::getXZZoom(mCameraDir, newLoc, dist);

      // If we are past one of the limits, compute new clamped position.
      if(ok2 && (dist<mZoomDistMin || dist>mZoomDistMax))
      {
         if(dist<mZoomDistMin)
            dist-=mZoomDistMin;
         else if(dist>mZoomDistMax)
            dist-=mZoomDistMax;


         newLoc+=dist*getCameraDir();
      }
   }
   // Set the location
   setCameraLoc(newLoc);
   restrictToBox();  // restrict to bounding box if necessary
}

//==============================================================================
// BCamera::moveForward(float d)
//
// Moves the camera forward a distance d
//==============================================================================
void BCamera::moveForward(float d)
{
   // Get what new location would be.
   BVector newLoc=getCameraLoc() + d*getCameraDir();

   // If "zoom" limiting is on and we can compute our "zoom" from the y=0 plane, do
   // the check.
   if(mZoomLimitOn)
   {
      float dist;
      bool ok=::getXZZoom(mCameraDir, newLoc, dist);

      // If we are past one of the limits, compute new clamped position.
      if(ok && (dist<mZoomDistMin || dist>mZoomDistMax))
      {
         if(dist<mZoomDistMin)
            dist-=mZoomDistMin;
         else if(dist>mZoomDistMax)
            dist-=mZoomDistMax;

         newLoc+=dist*getCameraDir();
      }
   }

   // Set the location
   setCameraLoc(newLoc);
   restrictToBox();  // restrict to bounding box if necessary
} // BCamera::moveForward


//==============================================================================
// BCamera::moveRight(float d)
//
// Moves the camera right a distance d
//==============================================================================
void BCamera::moveRight(float d)
{
   setCameraLoc(getCameraLoc() + d*getCameraRight());
   restrictToBox();  // restrict to bounding box if necessary
} // BCamera::moveRight

//==============================================================================
// BCamera::setCameraLoc
//==============================================================================
void BCamera::setCameraLoc(const BVector &loc) 
{
   mCameraLoc = loc; mMoved=true; mLastMovedTime = mTimer.getElapsedMilliseconds();
}


//==============================================================================
// BCamera::moveUp(float d)
//
// Moves the camera up a distance d
//==============================================================================
void BCamera::moveUp(float d)
{
   setCameraLoc(getCameraLoc() + d*getCameraUp());
   restrictToBox();  // restrict to bounding box if necessary
} // BCamera::moveUp


//==============================================================================
// BCamera::moveWorldForward(float d)
//
// Moves the camera forward a distance d (in xz plane)
//==============================================================================
void BCamera::moveWorldForward(float d)
{
   BVector dir;
   if(_fabs(getCameraDir().y)<_fabs(getCameraUp().y))
      dir=BVector(getCameraDir().x,0,getCameraDir().z);
   else
      dir=BVector(getCameraUp().x,0,getCameraUp().z);
   dir.normalize();
   setCameraLoc(getCameraLoc() + d*dir);
   restrictToBox();  // restrict to bounding box if necessary
} // BCamera::moveWorldForward


//==============================================================================
// BCamera::moveWorldRight(float d)
//
// Moves the camera right a distance d (in xz plane)
//==============================================================================
void BCamera::moveWorldRight(float d)
{
   BVector dir = BVector(getCameraRight().x,0,getCameraRight().z);
   dir.normalize();
   setCameraLoc(getCameraLoc() + d*dir);
   restrictToBox();  // restrict to bounding box if necessary
} // BCamera::moveWorldRight


//==============================================================================
// BCamera::moveWorldUp(float d)
//
// Moves the camera up a distance d (along world's y axis)
//==============================================================================
void BCamera::moveWorldUp(float d)
{
   setCameraLoc(getCameraLoc() + d*BVector(0,1,0));
   restrictToBox();  // restrict to bounding box if necessary
} // BCamera::moveWorldUp



//==============================================================================
// BCamera::yaw(float a)
//
// Yaws the camera by angle a (rotate around camera's up vector)
//==============================================================================
void BCamera::yaw(float a)
{
   BMatrix rot;
   rot.makeRotateArbitrary(a,getCameraUp());
   setCameraDir(rot*getCameraDir());
   setCameraRight(rot*getCameraRight());

   // ensure that vectors have not drifted away from being normalized by
   // transformations -- this may be overkill
   mCameraDir.normalize();
   mCameraRight.normalize();
} // BCamera::yaw


//==============================================================================
// BCamera::pitch(float a)
//
// Pitches the camera by angle a (rotate around camera's right vector)
//==============================================================================
void BCamera::pitch(float a)
{
   BMatrix rot;
   rot.makeRotateArbitrary(a,getCameraRight());
   BVector newCameraDir = rot*getCameraDir();
   BVector newCameraUp = rot*getCameraUp();
   bool canPitch = false;
   if(mPitchLimitOn)
   {
      float angle;
      bool ok = ::getXZPitch(newCameraDir, newCameraUp, angle);

      if(ok && angle > mPitchMin && angle < mPitchMax)
         canPitch = true;
   }
   else
      canPitch = true;

   if(canPitch)
   {
      setCameraUp(newCameraUp);
      setCameraDir(newCameraDir);
      // ensure that vectors have not drifted away from being normalized by
      // transformations -- this may be overkill
      mCameraDir.normalize();
      mCameraUp.normalize();
   }
} // BCamera::pitch


//==============================================================================
// BCamera::roll(float a)
//
// Rolls the camera by angle a (rotate around camera's direction vector)
//==============================================================================
void BCamera::roll(float a)
{
   BMatrix rot;
   rot.makeRotateArbitrary(a,getCameraDir());
   setCameraUp(rot*getCameraUp());
   setCameraRight(rot*getCameraRight());

   // ensure that vectors have not drifted away from being normalized by
   // transformations -- this may be overkill
   mCameraUp.normalize();
   mCameraRight.normalize();
} // BCamera::roll


//==============================================================================
// BCamera::yawWorld(float a)
//
// Yaws the camera by angle a (rotate around world's y axis)
//==============================================================================
void BCamera::yawWorld(float a)
{
   BMatrix rot;
   rot.makeRotateY(a);
   setCameraDir(rot*getCameraDir());
   setCameraRight(rot*getCameraRight());
   setCameraUp(rot*getCameraUp());

   // ensure that vectors have not drifted away from being normalized by
   // transformations -- this may be overkill
   mCameraDir.normalize();
   mCameraRight.normalize();
   mCameraUp.normalize();
} // BCamera::yawWorld


//==============================================================================
// BCamera::yawWorldAbout(float a, const BVector &point)
//==============================================================================
void BCamera::yawWorldAbout(float a, const BVector &point)
{
   BMatrix rot;
   rot.makeTranslate(-point.x, 0.0f, -point.z);
   rot.multRotateY(a);
   rot.multTranslate(point.x, 0.0f, point.z);

   setCameraDir(rot*getCameraDir());
   setCameraRight(rot*getCameraRight());
   setCameraUp(rot*getCameraUp());
   BVector newLoc;
   rot.transformVectorAsPoint(getCameraLoc(),newLoc);
   setCameraLoc(newLoc);

   // ensure that vectors have not drifted away from being normalized by
   // transformations -- this may be overkill
   mCameraDir.normalize();
   mCameraRight.normalize();
   mCameraUp.normalize();

   // Update the last look at pos
   if (mHaveLastLookAtPos)
   {
      rot.transformVectorAsPoint(mLastLookAtPos, newLoc);
      mLastLookAtPos = newLoc;
   }

} // BCamera::yawWorldAbout



//==============================================================================
// BCamera::pitchWorld(float a)
//
// Pitches the camera by angle a (rotate around camera's right vector projected on xz plane)
//==============================================================================
void BCamera::pitchWorld(float a)
{
   BMatrix rot;
   BVector dir = BVector(getCameraRight().x, 0, getCameraRight().z);
   dir.normalize();
   rot.makeRotateArbitrary(a,dir);
   setCameraDir(rot*getCameraDir());
   setCameraUp(rot*getCameraUp());
   setCameraRight(rot*getCameraRight());

   // ensure that vectors have not drifted away from being normalized by
   // transformations -- this may be overkill
   mCameraDir.normalize();
   mCameraUp.normalize();
   mCameraRight.normalize();
} // BCamera::pitchWorld


//==============================================================================
// BCamera::pitchWorldAbout(float a, const BVector &point)
//==============================================================================
void BCamera::pitchWorldAbout(float a, const BVector &point)
{
   BMatrix rot;
   rot.makeTranslate(-point.x, -point.y, -point.z);
   rot.multRotateArbitrary(a, mCameraRight);
   rot.multTranslate(point.x, point.y, point.z);

   BVector tempVector;
   rot.transformVector(mCameraDir, tempVector);
   setCameraDir(tempVector);

   rot.transformVector(mCameraUp, tempVector);
   setCameraUp(tempVector);

   rot.transformVector(mCameraRight, tempVector);
   setCameraRight(tempVector);

   mCameraDir.normalize();
   mCameraUp.normalize();
   mCameraRight.normalize();

   rot.transformVectorAsPoint(mCameraLoc, tempVector);
   setCameraLoc(tempVector);

} // BCamera::pitchWorldAbout


//==============================================================================
// BCamera::roll(float a)
//
// Rolls the camera by angle a (rotate around camera's direction vector projected on xz plane)
//==============================================================================
void BCamera::rollWorld(float a)
{
   BMatrix rot;
   BVector dir = BVector(getCameraDir().x, 0, getCameraDir().y);
   dir.normalize();
   rot.makeRotateArbitrary(a,dir);
   setCameraUp(rot*getCameraUp());
   setCameraRight(rot*getCameraRight());
   setCameraDir(rot*getCameraDir());

   // ensure that vectors have not drifted away from being normalized by
   // transformations -- this may be overkill
   mCameraUp.normalize();
   mCameraRight.normalize();
   mCameraDir.normalize();
} // BCamera::roll


//==============================================================================
// BCamera::calcCameraRight()
//
// Calculates the right vector from the camera's view and up vectors
//==============================================================================
void BCamera::calcCameraRight()
{
   BVector right = getCameraUp().cross(getCameraDir());
   right.normalize();
   setCameraRight(right);
} // BCamera::calcCameraRight


//==============================================================================
// BCamera::setBoundingBox(const BVector &boxMin, const BVector &boxMax)
//
// Sets the bounding box for restricting camera motion to the box defined by
// the points given in min and max.  (The bounding box is only used to restrict
// camera motion when a call to setBoundingBoxOn(true) has been made).
//==============================================================================
void BCamera::setBoundingBox(const BVector &boxMin, const BVector &boxMax)
{
   // Set the bounding box as requested, but don't trust that the caller
   // has set the mins in boxMin and the maxes in boxMax

   mBoundingBoxMin.x = min(boxMin.x, boxMax.x);
   mBoundingBoxMin.y = min(boxMin.y, boxMax.y);
   mBoundingBoxMin.z = min(boxMin.z, boxMax.z);

   mBoundingBoxMax.x = max(boxMin.x, boxMax.x);
   mBoundingBoxMax.y = max(boxMin.y, boxMax.y);
   mBoundingBoxMax.z = max(boxMin.z, boxMax.z);
} // BCamera::setBoundingBox


//==============================================================================
// BCamera::restrictToBox()
//
// Adjusts the camera so that it does not fall outside of the bounding box if
// the bounding box is turned on.
//==============================================================================
void BCamera::restrictToBox()
{
   if(getBoundingBoxOn())
   {
      if(getCameraLoc().x < getBoundingBoxMin().x)
         mCameraLoc.x = getBoundingBoxMin().x;
      if(getCameraLoc().x > getBoundingBoxMax().x)
         mCameraLoc.x = getBoundingBoxMax().x;

      if(getCameraLoc().y < getBoundingBoxMin().y)
         mCameraLoc.y = getBoundingBoxMin().y;
      if(getCameraLoc().y > getBoundingBoxMax().y)
         mCameraLoc.y = getBoundingBoxMax().y;

      if(getCameraLoc().z < getBoundingBoxMin().z)
         mCameraLoc.z = getBoundingBoxMin().z;
      if(getCameraLoc().z > getBoundingBoxMax().z)
         mCameraLoc.z = getBoundingBoxMax().z;
   }
} // BCamera::restrictToBox()

//==============================================================================
// BCamera::setDesiredRelativePosition
//==============================================================================
void BCamera::setDesiredRelativePosition(float x, float y, float z)
{
   mDesiredRelativePosition.x=x;
   mDesiredRelativePosition.y=y;
   mDesiredRelativePosition.z=z;
} // BCamera::setDesiredRelativePosition

//==============================================================================
// BCamera::setDesiredRelativePerspective
//==============================================================================
void BCamera::setDesiredRelativePerspective(float x, float y, float z)
{
   mDesiredRelativePerspective.x=x;
   mDesiredRelativePerspective.y=y;
   mDesiredRelativePerspective.z=z;
} // BCamera::setDesiredRelativePerspective

//==============================================================================
// BCamera::setPositioningType
//==============================================================================
void BCamera::setPositioningType(BYTE v)
{
   //Types:
   //0      Does nothing.
   mPositioningType=v;
} // BCamera::setPositioningType

//==============================================================================
// BCamera::positionForStrategy
//==============================================================================
bool BCamera::positionForStrategy(DWORD elapsed, bool heightAdjust, const BVector * pLookAtPos /*=NULL*/)
{
   //Returns true if the camera is moved (and is then in the 'Moving' state)
   //and false if the camera is not moved (and is then not in the 'Moving' state).

   if (!mMoved)
      return (false);

   if(heightAdjust)
   {
      //Calc the position difference between our current pos and our desired pos.
      float heightDiff = mDesiredPosition.y-mCameraLoc.y;


      if (heightDiff == 0.0f)
      {
         //blog("  NO POS CHANGE, returning false.");
         mMoving=false;
         lastPositionDifferenceLength=heightDiff;
         return(false);
      }

      //If the magnitude of the position difference is less than our positionInertia AND
      //we're not moving, don't move and return false.  We need to check the mMoving
      //flag so that once we start moving, we don't stop until we're where we want to be
      //(otherwise, we'll get an annoying jumpy effect as the camera is not moved, then
      //moved to the pos, then not moved again because it's close enough, etc.).
      if ((heightDiff < mPositionInertia) && (mMoving == false))
      {
         //blog("  Within posInertia and not moving, returning false.");
         mMoving=false;
         lastPositionDifferenceLength=heightDiff;
         return(false);
      }

      //Calc the amount of position change based on our pos change speed.
      heightDiff*=mPositionChangeSpeed;

      //Normalize the position change to our maximum position change if it's longer than that.
      if ((heightDiff > mMaximumPositionDifference) &&
         (mMaximumPositionDifference > 0.0f))
      {
         //blog("  Setting posDiffLength to %f because it's too large otherwise.", mMaximumPositionDifference);
         heightDiff=heightDiff-mMaximumPositionDifference;
      }
      //Normalize the position change to our maximum position change if it's longer than that.
      else if ((heightDiff > mMaximumPositionChange) &&
         (mMaximumPositionChange > 0.0f))
      {
         //blog("  Limiting posDiffLength to %f.", mMaximumPositionChange);
         heightDiff=mMaximumPositionChange;
         mLimitedLastTime=true;
      }
      else
         mLimitedLastTime=false;

      //Now, we finally add the position changes to our camera position.
      mCameraLoc.y+=heightDiff;

      //-- continue to look at the same point on the ground
      //-- set the dir
      if (pLookAtPos)
      {
         BVector dir = (*pLookAtPos) - getCameraLoc();
         dir.safeNormalize();

         BVector upVector(0.0f, 1.0f, 0.0f);

         BVector rightVector = upVector.cross(dir);
         rightVector.normalize();

         upVector = dir.cross(rightVector);
         upVector.normalize();

         setCameraDir(dir);
         setCameraRight(rightVector);
         setCameraUp(upVector);
      }


      mMoved=true;
      mMoving=true;
      mLastMovedTime = mTimer.getElapsedMilliseconds(); 
      lastPositionDifferenceLength=heightDiff;
   }

   // Bob if requested.
   if(mBob)
   {
      const float cVerticalBobDelta = 0.0025f;
      const float cHorizontalBobDelta = 0.00125f;
      const float cVerticalBobAmount = 0.5f;
      const float cHorizontalBobAmount = 0.5f;

      // Go back to original position
      mCameraLoc += cVerticalBobAmount*lookupSin(mBobOffsetVertical)*mCameraUp;
      mCameraLoc += cHorizontalBobAmount*lookupSin(mBobOffsetHorizontal)*mCameraRight;
      mMoved=true;
      mLastMovedTime = mTimer.getElapsedMilliseconds(); 

      mBobOffsetVertical += cVerticalBobDelta*elapsed;
      while(mBobOffsetVertical > cTwoPi)
         mBobOffsetVertical -= cTwoPi;
      mBobOffsetHorizontal += cHorizontalBobDelta*elapsed;
      while(mBobOffsetHorizontal > cTwoPi)
         mBobOffsetHorizontal -= cTwoPi;

      mCameraLoc += cVerticalBobAmount*lookupSin(mBobOffsetVertical)*mCameraUp;
      mCameraLoc += cHorizontalBobAmount*lookupSin(mBobOffsetHorizontal)*mCameraRight;
   }

   return(true);
} // BCamera::positionForStrategy

//==============================================================================
// BCamera::movementTypeName
//==============================================================================
char* BCamera::movementTypeName(void)
{
   static char none[] = "None";
   static char strategy[] = "Strategy";
   static char track[] = "Track";
   static char unknown[] = "Unknown";
   switch (mMovementType)
   {
      case cStrategyMovement:        return(strategy);
      case cTrackMovement:           return(track);
      case cNoMovement:              return(none);
   }
   return(unknown);
} // BCamera::movementTypeName

//==============================================================================
// BCamera::decrementMovementType
//==============================================================================
void BCamera::decrementMovementType(void)
{
   mMovementType--;
   if (mMovementType < 0)
      mMovementType=cNumberBaseMovementTypes-1;
} // BCamera::decrementMovementType

//==============================================================================
// BCamera::incrementMovementType
//==============================================================================
void BCamera::incrementMovementType(void)
{
   mMovementType++;
   if (mMovementType > cNumberBaseMovementTypes-1)
      mMovementType=0;
} // BCamera::incrementMovementType

//==============================================================================
// BCamera::debugList
//==============================================================================
void BCamera::debugList(void)
{
   blog("CAMERA:"); 
   blog("  getCameraLoc                       =(%0.1f,%0.1f,%0.1f).", mCameraLoc.x, mCameraLoc.y, mCameraLoc.z); 
   blog("  getCameraDir                       =(%0.1f,%0.1f,%0.1f).", mCameraDir.x, mCameraDir.y, mCameraDir.z); 
   blog("  getCameraUp                        =(%0.1f,%0.1f,%0.1f).", mCameraUp.x, mCameraUp.y, mCameraUp.z); 
   blog("  getCameraRight                     =(%0.1f,%0.1f,%0.1f).", mCameraRight.x, mCameraRight.y, mCameraRight.z); 
   blog("  getBoundingBoxMin                  =(%0.1f,%0.1f,%0.1f).", mBoundingBoxMin.x, mBoundingBoxMin.y, mBoundingBoxMin.z); 
   blog("  getBoundingBoxMax                  =(%0.1f,%0.1f,%0.1f).", mBoundingBoxMax.x, mBoundingBoxMax.y, mBoundingBoxMax.z); 
   blog("  boundingBoxOn                   =%s.", mBoundingBoxOn?"true":"false"); 
   blog("  pitchLimitOn                    =%s.", mPitchLimitOn?"true":"false"); 
   blog("  pitchMin                        =%0.2f (%0.1f deg).", mPitchMin, 180.0f*mPitchMin/cPi);
   blog("  pitchMax                        =%0.2f (%0.1f deg).", mPitchMax, 180.0f*mPitchMax/cPi);
   blog("  desiredPosition                 =(%0.1f,%0.1f,%0.1f).", mDesiredPosition.x, mDesiredPosition.y, mDesiredPosition.z); 
   blog("  desiredPerspective              =(%0.1f,%0.1f,%0.1f).", mDesiredPerspective.x, mDesiredPerspective.y, mDesiredPerspective.z); 
   blog("  desiredRelativePosition         =(%0.1f,%0.1f,%0.1f).", mDesiredRelativePosition.x, mDesiredRelativePosition.y, mDesiredRelativePosition.z); 
   blog("  mDesiredRelativePerspective =(%0.1f,%0.1f,%0.1f).", mDesiredRelativePerspective.x, mDesiredRelativePerspective.y, mDesiredRelativePerspective.z); 
   blog("  positioningType                 =%d.", mPositioningType);
   blog("  maximumPositionDifference       =%0.1f", mMaximumPositionDifference);
   blog("  maximumPositionChange           =%0.1f", mMaximumPositionChange);
   blog("  positionChangeSpeed             =%0.1f", mPositionChangeSpeed);
   blog("  positionInertia                 =%0.1f", mPositionInertia);
   blog("  moving                          =%s.", mMoving?"true":"false"); 
   blog("  limitedLastTime                 =%s.", mLimitedLastTime?"true":"false"); 
   blog("  lastPositionDifferenceLength    =%0.1f.", lastPositionDifferenceLength);
   blog("  mSpeed                      =%0.1f.", mSpeed);
   blog("  mTurnspeed                  =%0.1f.", mTurnspeed);
   blog("  movementType                    =%d.", mMovementType);
} // BCamera::debugList


//==============================================================================
// BCamera::save
//==============================================================================
bool BCamera::save(BChunkWriter *chunkWriter, bool scenario)
{
   // Check writer validity.
   if(!chunkWriter)
   {
      BASSERT(0);
      return(false);
   }

   // Write tag.
   long mainHandle;
   long result = chunkWriter->writeTagPostSized(BCHUNKTAG("CM"), mainHandle);
   if(!result)
   {
      {setBlogError(4190); blogerror("BCamera::save -- error writing tag.");}
      return(false);
   }

   // Version.
   result = chunkWriter->writeDWORD(msSaveVersion);
   if(!result)
   {
      {setBlogError(4191); blogerror("BCamera::save -- error writing version.");}
      return(false);
   }

   // Basics.
   result=chunkWriter->writeVector(mCameraLoc);
   SAVE_CHECK_RESULT
   result=chunkWriter->writeVector(mCameraDir);
   SAVE_CHECK_RESULT
   result=chunkWriter->writeVector(mCameraUp);
   SAVE_CHECK_RESULT
   result=chunkWriter->writeVector(mCameraRight);
   SAVE_CHECK_RESULT
   result=chunkWriter->writeBYTE(mPositioningType);
   SAVE_CHECK_RESULT
   if (!scenario)
   {
      result=chunkWriter->writeBYTE(mMovementType);
      SAVE_CHECK_RESULT
   }
   
   // Bounding box.
   result=chunkWriter->writeVector(mBoundingBoxMin);
   SAVE_CHECK_RESULT
   result=chunkWriter->writeVector(mBoundingBoxMax);
   SAVE_CHECK_RESULT
   result=chunkWriter->writeBool(mBoundingBoxOn);
   SAVE_CHECK_RESULT

   // Pitch limits.
   result=chunkWriter->writeFloat(mPitchMin);
   SAVE_CHECK_RESULT
   result=chunkWriter->writeFloat(mPitchMax);
   SAVE_CHECK_RESULT
   result=chunkWriter->writeBool(mPitchLimitOn);
   SAVE_CHECK_RESULT

   // "Zoom" limits.
   result=chunkWriter->writeFloat(mZoomDistMin);
   SAVE_CHECK_RESULT
   result=chunkWriter->writeFloat(mZoomDistMax);
   SAVE_CHECK_RESULT
   result=chunkWriter->writeBool(mZoomLimitOn);
   SAVE_CHECK_RESULT

   //FOV.
   result=chunkWriter->writeFloat(mFOV);
   SAVE_CHECK_RESULT

   // Positioning.
   result=chunkWriter->writeVector(mDesiredPosition);
   SAVE_CHECK_RESULT
   result=chunkWriter->writeVector(mDesiredPerspective);
   SAVE_CHECK_RESULT
   result=chunkWriter->writeVector(mDesiredRelativePosition);
   SAVE_CHECK_RESULT
   result=chunkWriter->writeVector(mDesiredRelativePerspective);
   SAVE_CHECK_RESULT
   result=chunkWriter->writeFloat(mMaximumPositionDifference);
   SAVE_CHECK_RESULT
   result=chunkWriter->writeFloat(mMaximumPositionChange);
   SAVE_CHECK_RESULT
   result=chunkWriter->writeFloat(mPositionChangeSpeed);
   SAVE_CHECK_RESULT
   result=chunkWriter->writeFloat(mPositionInertia);
   SAVE_CHECK_RESULT
   result=chunkWriter->writeBool(mMoving);
   SAVE_CHECK_RESULT
   result=chunkWriter->writeBool(mLimitedLastTime);
   SAVE_CHECK_RESULT
   result=chunkWriter->writeFloat(lastPositionDifferenceLength);
   SAVE_CHECK_RESULT
   result=chunkWriter->writeFloat(mSpeed);
   SAVE_CHECK_RESULT
   result=chunkWriter->writeFloat(mTurnspeed);
   SAVE_CHECK_RESULT
   result=chunkWriter->writeBool(mMoved);
   SAVE_CHECK_RESULT
   result=chunkWriter->writeBool(mBob);
   SAVE_CHECK_RESULT
   result=chunkWriter->writeFloat(mBobOffsetVertical);
   SAVE_CHECK_RESULT
   result=chunkWriter->writeFloat(mBobOffsetHorizontal);
   SAVE_CHECK_RESULT
   result=chunkWriter->writeLong(mOverride);
   SAVE_CHECK_RESULT

   // Finish chunk.   
   result = chunkWriter->writeSize(mainHandle);
   if(!result)
   {
      {setBlogError(4192); blogerror("BCamera::save -- failed to write chunk size!");}
      return(false);
   }

   return(true);
}


//==============================================================================
// BCamera::load
//==============================================================================
bool BCamera::load(BChunkReader *chunkReader, bool scenario)
{
   // Check reader validity.
   if(!chunkReader)
   {
      BASSERT(0);
      return(false);
   }

   // Read tag.
   long result = chunkReader->readExpectedTag(BCHUNKTAG("CM"));
   if(!result)
   {
      {setBlogError(4193); blogerror("BCamera::load -- error reading tag.");}
      return(false);
   }

   // Version.
   DWORD version;
   result = chunkReader->readDWORD(&version);
   if(!result)
   {
      {setBlogError(4194); blogerror("BCamera::load -- error reading version.");}
      return(false);
   }

   // Basics.
   result=chunkReader->readVector(&mCameraLoc);
   LOAD_CHECK_RESULT
   result=chunkReader->readVector(&mCameraDir);
   LOAD_CHECK_RESULT
   result=chunkReader->readVector(&mCameraUp);
   LOAD_CHECK_RESULT
   result=chunkReader->readVector(&mCameraRight);
   LOAD_CHECK_RESULT
   result=chunkReader->readBYTE(&mPositioningType);
   LOAD_CHECK_RESULT
   if (!scenario || (version == 0))
   {
      result=chunkReader->readBYTE((BYTE *)&mMovementType);
      LOAD_CHECK_RESULT
      mMovementType = cStrategyMovement;
   }
   
   // Bounding box.
   result=chunkReader->readVector(&mBoundingBoxMin);
   LOAD_CHECK_RESULT
   result=chunkReader->readVector(&mBoundingBoxMax);
   LOAD_CHECK_RESULT
   result=chunkReader->readBool(&mBoundingBoxOn);
   LOAD_CHECK_RESULT

   // Pitch limits.
   result=chunkReader->readFloat(&mPitchMin);
   LOAD_CHECK_RESULT
   result=chunkReader->readFloat(&mPitchMax);
   LOAD_CHECK_RESULT
   result=chunkReader->readBool(&mPitchLimitOn);
   LOAD_CHECK_RESULT

   // "Zoom" limits.
   result=chunkReader->readFloat(&mZoomDistMin);
   LOAD_CHECK_RESULT
   result=chunkReader->readFloat(&mZoomDistMax);
   LOAD_CHECK_RESULT
   result=chunkReader->readBool(&mZoomLimitOn);
   LOAD_CHECK_RESULT

   //FOV.
   if (version >= 3)
   {
      result=chunkReader->readFloat(&mFOV);
      LOAD_CHECK_RESULT
      if (version < 4)
         mFOV*=cRadiansPerDegree;
   }
   else
      mFOV=40.0f;

   //Sun Direction, Sun Color, Ambient Color.
   if ((version >= 4) && (version <= 5))
   {
      BVector tempVec;
      DWORD tempDWORD;
      result=chunkReader->readVector(&tempVec);
      LOAD_CHECK_RESULT
      result=chunkReader->readDWORD(&tempDWORD);
      LOAD_CHECK_RESULT
      result=chunkReader->readDWORD(&tempDWORD);
      LOAD_CHECK_RESULT
   }

   // Attachments.
   if(version<2)
   {
      long temp;
      result=chunkReader->readLong(&temp);
      LOAD_CHECK_RESULT
      result=chunkReader->readLong(&temp);
      LOAD_CHECK_RESULT
      result=chunkReader->readLong(&temp);
      LOAD_CHECK_RESULT
   }

   // Positioning.
   result=chunkReader->readVector(&mDesiredPosition);
   LOAD_CHECK_RESULT
   result=chunkReader->readVector(&mDesiredPerspective);
   LOAD_CHECK_RESULT
   result=chunkReader->readVector(&mDesiredRelativePosition);
   LOAD_CHECK_RESULT
   result=chunkReader->readVector(&mDesiredRelativePerspective);
   LOAD_CHECK_RESULT
   result=chunkReader->readFloat(&mMaximumPositionDifference);
   LOAD_CHECK_RESULT
   result=chunkReader->readFloat(&mMaximumPositionChange);
   LOAD_CHECK_RESULT
   result=chunkReader->readFloat(&mPositionChangeSpeed);
   LOAD_CHECK_RESULT
   result=chunkReader->readFloat(&mPositionInertia);
   LOAD_CHECK_RESULT
   result=chunkReader->readBool(&mMoving);
   LOAD_CHECK_RESULT
   result=chunkReader->readBool(&mLimitedLastTime);
   LOAD_CHECK_RESULT
   result=chunkReader->readFloat(&lastPositionDifferenceLength);
   LOAD_CHECK_RESULT
   result=chunkReader->readFloat(&mSpeed);
   LOAD_CHECK_RESULT
   result=chunkReader->readFloat(&mTurnspeed);
   LOAD_CHECK_RESULT
   result=chunkReader->readBool(&mMoved);
   LOAD_CHECK_RESULT
   result=chunkReader->readBool(&mBob);
   LOAD_CHECK_RESULT
   result=chunkReader->readFloat(&mBobOffsetVertical);
   LOAD_CHECK_RESULT
   result=chunkReader->readFloat(&mBobOffsetHorizontal);
   LOAD_CHECK_RESULT
   if (version >= 5)
   {
      result=chunkReader->readLong(&mOverride);
      LOAD_CHECK_RESULT
   }

   // Validate our reading of the chunk.
   result = chunkReader->validateChunkRead(BCHUNKTAG("CM"));
   if(!result)
   {
      {setBlogError(4195); blogerror("BCamera::load -- did not read chunk properly!");}
      return(false);
   }

   // jce 5/8/2001 -- E3 HACK: just whack the moved flag back to true.  It really shouldn't
   // be saved at all.
   mMoved=true;
   mLastMovedTime = mTimer.getElapsedMilliseconds(); 

   return(true);
}


//==============================================================================
// BCamera::setBob(bool bob)
//==============================================================================
void BCamera::setBob(bool bob)
{
   // Reset bob offsets if going from bobbing off to on
   if(!mBob && bob)
   {
      mBobOffsetVertical = 0.0f;
      mBobOffsetHorizontal = 0.0f;
   }
   mBob = bob;
} // BCamera::setBob

//==============================================================================
// BCamera::lookAtEntity
//==============================================================================
void BCamera::lookAtEntity(const BVector &entityDir, BVector entityPos, float cameraDistance, float cameraHeight, float lookAheadDistance, float lookAtHeight, bool followUnitHeight, bool keepCameraAboveGround, bool smoothMove, float time)
{
   keepCameraAboveGround;
   followUnitHeight;

   // Calculate a position in front of the entity.
   // This point is what the camera will be positioned
   // from and oriented to look at.
   BVector vDir = entityDir;
   vDir.y = 0.0f;
   vDir.normalize();

   BVector lookAtPos = entityPos + (vDir * lookAheadDistance);

   if (smoothMove)
   {
      // Smoothly move between the old and new look at position.
      if (mHaveLastLookAtPos)
      {
         BVector lookAtPosVector = lookAtPos - mLastLookAtPos;
         float lookAtDistance = lookAtPosVector.length();

         BVector entityPosVector = entityPos - mLastEntityPos;
         float entityDistance = entityPosVector.length();

         float maxDistance = entityDistance + (mLookAtCameraSpeed * time);

         if (lookAtDistance > maxDistance)
         {
            lookAtPosVector.normalize();
            lookAtPos = mLastLookAtPos + (lookAtPosVector * maxDistance);
         }
      }

      mLastLookAtPos=lookAtPos;
      mLastEntityPos=entityPos;
      mLastEntityDir=entityDir;
      mHaveLastLookAtPos=true;
   }

   // Position the camera a fixed distance away.
   BVector cameraPos = getCameraLoc();
   BVector entityToCameraVector = cameraPos - lookAtPos;
   entityToCameraVector.y = 0.0f;
   entityToCameraVector.normalize();

   BVector newCameraPos = lookAtPos + (entityToCameraVector * cameraDistance);
   newCameraPos.y += cameraHeight;
   setCameraLoc(newCameraPos);

   // Look at the entity (offset by how high up on the entity to look at)
   lookAtPos.y += lookAtHeight;
   BVector cameraToEntityVector = lookAtPos - newCameraPos;
   cameraToEntityVector.normalize();

   BVector upVector(0.0f, 1.0f, 0.0f);

   BVector rightVector = upVector.cross(cameraToEntityVector);
   rightVector.normalize();

   upVector = cameraToEntityVector.cross(rightVector);
   upVector.normalize();

   setCameraDir(cameraToEntityVector);
   setCameraRight(rightVector);
   setCameraUp(upVector);
}

//==============================================================================
// BCamera::adjustLastLookAtPos
//==============================================================================
void BCamera::adjustLastLookAtPos(float lookAheadDistanceChange)
{
   if (!mHaveLastLookAtPos)
      return;
   mLastLookAtPos += (mLastEntityDir * lookAheadDistanceChange);
}

//==============================================================================
// BCamera::setMovementType
//==============================================================================
void BCamera::setMovementType( BYTE v ) 
{ 
   mMovementType=v; 
}


//==============================================================================
// BCamera::lookAtPosFixedY
//
// Attempts to look at the given point while not reorienting the camera or
// changing the height (y value of position)
//==============================================================================
bool BCamera::lookAtPosFixedY(const BVector &pos)
{
   // We're only allowed to move the camera around in the x and z directions, so
   // we'll find the point in the camera's current y plane that lets us see the requested point.
   BVector iPoint;
   bool intersects=lineIntersectsPlane(getCameraLoc(), cYAxisVector, pos, getCameraDir(), iPoint);

   // If no intersection, we're in an undefined situation (looking parallel to xz plane), so don't move at all.
   if(!intersects)
      return(false);

   // Set the new location.
   setCameraLoc(iPoint);
   return(true);   
}


//==============================================================================
// BCamera::beginShake
//
// MS 9/15/2003: duration and trailOffDuration are specified in seconds, and
// conservationFactor is a value between 0.0 and 1.0. Strength should be >= 0.
//==============================================================================
void BCamera::beginShake(float duration, float strength, float trailOffDuration, float conservationFactor)
{
   beginShake((DWORD)(duration * 1000.0f), strength, (DWORD)(trailOffDuration * 1000.0f), conservationFactor);
}

//==============================================================================
// BCamera::beginShake
//
// Halwes - 8/10/2007: duration and trailOffDuration are specified in milliseconds, and
// conservationFactor is a value between 0.0 and 1.0. Strength should be >= 0.
//==============================================================================
void BCamera::beginShake(DWORD duration, float strength, DWORD trailOffDuration, float conservationFactor)
{
   mShakeTime = timeGetTime() + duration;
   mShakeStrength = strength;
   mShakeTrailOffDuration = trailOffDuration;
   mShakeConservationFactor = conservationFactor;
   mAccumulatedShake.zero();
}

//==============================================================================
// BCamera::checkCameraShake
//==============================================================================
void BCamera::checkCameraShake(void)
{
   if (mShakeTime == 0)
      return;

   // this will be used to dampen the shake as it's trailing off
   float trailOffMultiplier = 1.0f;

   if (timeGetTime() > mShakeTime)
   {
      if(timeGetTime() > mShakeTime + mShakeTrailOffDuration)
      {
         mShakeTime = 0;
         mShakeStrength = 0;
         moveRight(-1.0f * mAccumulatedShake.x);
         moveWorldForward(-1.0f * mAccumulatedShake.z);
         mAccumulatedShake.zero();
         return;
      }
      else
      {
         if(mShakeTrailOffDuration > 0)
            trailOffMultiplier = (float)(mShakeTime + mShakeTrailOffDuration - timeGetTime()) / mShakeTrailOffDuration;
      }
   }

   float realShakeStrength = mShakeStrength * trailOffMultiplier * trailOffMultiplier;

   float r1,r2;
   r1 = getRandRangeFloat(cUnsyncedRand, -realShakeStrength, realShakeStrength) - mAccumulatedShake.x * mShakeConservationFactor;
   r2 = getRandRangeFloat(cUnsyncedRand, -realShakeStrength, realShakeStrength) - mAccumulatedShake.z * mShakeConservationFactor;
   moveRight(r1);
   moveWorldForward(r2);
   mAccumulatedShake.x += r1;
   mAccumulatedShake.z += r2;
}

//==============================================================================
//==============================================================================
void BCamera::beginCameraEffect(DWORD curTime, BCameraEffectData* pCamEffectData, const BVector* loc, uint viewportIndex)
{
   // Don't stomp on existing mode camera effect for a non-mode camera effect
   if (mpCurrentCamEffectData && mpCurrentCamEffectData->mModeCameraEffect && !pCamEffectData->mModeCameraEffect)
      return;

   mEffectStartTime = curTime;
   mpCurrentCamEffectData = pCamEffectData;
   mCamEffectViewportIndex = viewportIndex;
   if (loc)
      mCurrentCamEffectLocation = *loc;
   else
      mCurrentCamEffectLocation = BVector(0.0f, 0.0f, 0.0f);
}

//==============================================================================
//==============================================================================
bool BCamera::updateCameraEffect(DWORD curTime, BVector hoverPoint)
{
   if (!mpCurrentCamEffectData)
      return false;

   float time = (curTime - mEffectStartTime) * 0.001f;

   BToneMapParams params = gToneMapManager.getParams(mCamEffectViewportIndex);

   BVector vecResult;
   BVector2 vec2Result;
   float floatResult;

   // Color xform
   if (mpCurrentCamEffectData->mColorTransformFactorTable.interp(time, floatResult))
      params.mColorTransformFactor = floatResult;
   if (mpCurrentCamEffectData->mColorTransformRTable.interp(time, vecResult))
      params.mRTransform = vecResult;
   if (mpCurrentCamEffectData->mColorTransformGTable.interp(time, vecResult))
      params.mGTransform = vecResult;
   if (mpCurrentCamEffectData->mColorTransformBTable.interp(time, vecResult))
      params.mBTransform = vecResult;

   // Blur
   if (mpCurrentCamEffectData->mBlurFactorTable.interp(time, floatResult))
      params.mBlurFactor = floatResult;
   if (mpCurrentCamEffectData->mUserHoverPointAs3DPosition)
      mCurrentCamEffectLocation = hoverPoint;
   if (mpCurrentCamEffectData->mUse3DPosition && mpCurrentCamEffectData->mRadialBlur)
   {
      // Calc 2D pos from 3D
      float x, y;
      gRender.getViewParams().calculateWorldToScreen(mCurrentCamEffectLocation, x, y);
      x /= gRender.getViewParams().getViewportWidth();
      y /= gRender.getViewParams().getViewportHeight();

      params.mBlurX = Math::Clamp(x, 0.0f, 1.0f);
      params.mBlurY = Math::Clamp(y, 0.0f, 1.0f);
   }
   else if (mpCurrentCamEffectData->mBlurXYTable.interp(time, vec2Result))
   {
      params.mBlurX = vec2Result.x;
      params.mBlurY = vec2Result.y;
   }

   if (mpCurrentCamEffectData->mRadialBlurFactorTable.interp(time, vec2Result))
   {
      params.mRadialBlurScale = vec2Result.x;
      params.mRadialBlurMax = vec2Result.y;
   }

   params.mRadialBlur = mpCurrentCamEffectData->mRadialBlur;

   // Set tonemapper params
   gToneMapManager.setParams(params, mCamEffectViewportIndex);

   // TODO - Fov support
   //if (mpCurrentCamEffectData->mFOVTable.interp(time, floatResult))
   //   setFOV(floatResult);

   // Yaw / Pitch
   float currentPitch = 0.0f;
   if (getXZPitch(currentPitch))
   {
      // Reverse this because getXZPitch returns a value in the opposite
      // direction that pitch() takes
      currentPitch = -currentPitch;

      // Yaw
      if (mpCurrentCamEffectData->mYawTable.interp(time, floatResult))
      {
         BVector dir;
         dir=cZAxisVector;
         dir.rotateXZ(floatResult*cRadiansPerDegree);
         dir.normalize();
         setCameraDir(dir);
         setCameraUp(cYAxisVector);
         calcCameraRight();

         pitch(currentPitch);
      }

      // Pitch
      if (mpCurrentCamEffectData->mPitchTable.interp(time, floatResult))
      {
         pitch(floatResult * cRadiansPerDegree - currentPitch);
      }
   }

   // Camera pos
   if (mpCurrentCamEffectData->mZoomTable.interp(time, floatResult))
   {
      BVector pos = hoverPoint - (getCameraDir()*floatResult);
      setCameraLoc(pos);
   }

   // We're done interpolating through this effect, so unset it
   if (time >= mpCurrentCamEffectData->getMaxTime())
      mpCurrentCamEffectData = NULL;

   return true;
}

//==============================================================================
//==============================================================================
void BCamera::clearCameraEffect(DWORD curTime, DWORD duration, uint viewportIndex)
{
   mClearCamEffectData.makeClearData(duration, viewportIndex);

   beginCameraEffect(curTime, &mClearCamEffectData, NULL, viewportIndex);
}

//==============================================================================
// BCamera::saveFull
//==============================================================================
bool BCamera::saveFull(long dirID, const char *filename)
{
   // Check param.
   if(!filename)
      return(false);

   // Open the file for write.
   BFile file;
   
   // Bail if open failed.
   //TODO FIXME
   if (file.openWriteable(dirID, BString(filename), BFILE_OPEN_ENABLE_BUFFERING) == false)
      return(false);

   BFileSystemStream fileStream;
   fileStream.open(&file, BString(filename));

   // Create a chunk writer.
   BChunkWriter* pChunkWriter = getChunkWriter(&fileStream, true);
   if(!pChunkWriter)
   {
      BASSERT(0);
      return(false);
   }

   // Save.
   bool ok=save(pChunkWriter, true);

   // Clean up.
   releaseChunkWriter(pChunkWriter);
   
   // Done.
   return(ok);
}


//==============================================================================
// BCamera::loadFull
//==============================================================================
bool BCamera::loadFull(long dirID, const char *filename)
{
   // Check param.
   if(!filename)
      return(false);

   // Open the file for read.
   BFile file;

   //TODO FIXME
   // Bail if open failed.
   if(file.openReadOnly(dirID, BString(filename)) == false)
      return(false);

   BFileSystemStream fileStream;
   fileStream.open(&file, BString(filename));

   // Create a chunk reader.
   BChunkReader* pChunkReader = getChunkReader(&fileStream, true);
   if(!pChunkReader)
   {
      BASSERT(0);
      return(false);
   }

   // Load.
   bool ok=load(pChunkReader, true);

   // Clean up.
   releaseChunkReader(pChunkReader);
   
   // Done.
   return(ok);
}

//==============================================================================
// BCamera::getXZZoom
//==============================================================================
bool BCamera::getXZZoom(float &dist)
{
   return(::getXZZoom(mCameraDir, mCameraLoc, dist));
}


//==============================================================================
// BCamera::getXZPitch
//==============================================================================
bool BCamera::getXZPitch(float &angle)
{
   return(::getXZPitch(mCameraDir, mCameraUp, angle));
}

//==============================================================================
//==============================================================================
float BCamera::getXZYaw()
{
   return mCameraDir.getAngleAroundY();
}

//==============================================================================
// eof: camera.cpp
//==============================================================================
