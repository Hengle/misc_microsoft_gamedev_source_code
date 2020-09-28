//============================================================================
// flashminimap.cpp
// Ensemble Studios (c) 2006
//============================================================================

#include "common.h"
#include "flashminimap.h"
#include "flashmanager.h"

//-- data
#include "xmlReader.h"

//-- render
#include "render.h"
#include "renderDraw.h"
#include "renderThread.h"


//-- game classes
#include "ai.h"
#include "user.h"
#include "usermanager.h"
#include "player.h"
#include "uigame.h"
#include "visualmanager.h"
#include "world.h"
#include "camera.h"
#include "mathutil.h"
#include "configsgame.h"
#include "protoobject.h"
#include "protosquad.h"
#include "game.h"
#include "gamesettings.h"
#include "visiblemap.h"

//-- terrain
#include "TerrainVisual.h"
#include "configsgamerender.h"

#include "team.h"

const int cMinimapMaxSquads = 2000;

#define BLACK_MAP

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BFlashMinimap::BFlashMinimap():
mpMovie(NULL),
mRotationAngle(0.0f),
mX(0),
mY(0),
mWidth(100),
mHeight(100),
mMapViewWidth(100),
mMapViewHeight(100),
mMapViewHalfWidth(50),
mMapViewHalfHeight(50),
mMapViewCenterX(0),
mMapViewCenterY(0),
mMapTextureHandle(cInvalidManagedTextureHandle),
mMapMaskTextureHandle(cInvalidManagedTextureHandle),
mMapBackgroundTextureHandle(cInvalidManagedTextureHandle),
mIconTextureHandle(cInvalidManagedTextureHandle),
mWorldToMapConversionFactor(1.0f),
mVisibilityScale(1.0f),
mVisibilityIndex(0),
mVisMaskTextureHandle(cInvalidManagedTextureHandle),
mMapViewIconStickyOffset(0.0f),
mFlareDurationHandle(BFlashProperties::eInvalidFlashPropertyHandle),
mFlareIconHandle(BFlashProperties::eInvalidFlashPropertyHandle),
mSensorLockIconHandle(BFlashProperties::eInvalidFlashPropertyHandle),
mSensorLockIconProtoID(-1),
mAlertDurationHandle(BFlashProperties::eInvalidFlashPropertyHandle),
mAlertIconHandle(BFlashProperties::eInvalidFlashPropertyHandle),
mSquadIconUVHandle(BFlashProperties::eInvalidFlashPropertyHandle),
mSquadIconBaseSize(1.0f),
mSquadIconProtoID(-1),
mCenterPointIconUVHandle(BFlashProperties::eInvalidFlashPropertyHandle),
mCenterPointIconProtoID(-1),
mMapFogScalar(1.0f),
mMapSkirtFogScalar(1.0f),
mCameraPos(XMVectorZero()),
mbVisible(true),
mRotationSpeed(0.3f),
mPanSpeed(0.15f),
mFullZoomOut(1.5f),
mbFullZoomOut(true),
mIconScaleX(1.0f),
mIconScaleY(1.0f),
mDefaultCameraYawRadians(0.0f),
mbShowFocusPoints(true),
mbFirstRender(true),
mRotationOffset(0.0f),
mbForceRotationUpdate(false),
mbMapSkirtMirroring(true),
mAlertIconUsesFlash(false),
mSquadIconUsesFlash(false),
mCenterPointUsesFlash(false),
mSensorLockUsesFlash(false)
{
   initEventHandle();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BFlashMinimap::~BFlashMinimap()
{
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BFlashMinimap::init(const char* filename, const char* datafile)
{
   if (!datafile)
      return false;

   if (!loadData(datafile))
      return false;

   // get properties from the xml file that defined the minimap
   const BFlashProperties* pProps = getProperties();
   BDEBUG_ASSERT(pProps);
   if (!pProps)
      return false;

   // minimap movie
   gFlashGateway.getOrCreateData(filename, cFlashAssetCategoryInGame, mDataHandle);
   mpMovie = gFlashGateway.createInstance(mDataHandle, false);
   gFlashGateway.registerEventHandler(mpMovie, mSimEventHandle);

   BFixedString128 value;
   BFlashPropertyHandle handle = pProps->findStringHandle("IconFlashFile");
   bool result = pProps->getString(handle, value);
   BDEBUG_ASSERT(result);
   if (!result)
      return false;

   gFlashGateway.getOrCreateData(value.c_str(), cFlashAssetCategoryInGame, mIconDataHandle);
   mpIconMovie = gFlashGateway.createInstance(mIconDataHandle, true);
   gFlashGateway.registerEventHandler(mpIconMovie, mSimEventHandle);
      
   /*
   mX = pProps->mX;
   mY = pProps->mY;
   mWidth  = pProps->mWidth;
   mHeight = pProps->mHeight;
   */

   float mapXScalar = 1.0f;
   result = pProps->getFloat(pProps->findHandle("MapX"),  mapXScalar);
   BDEBUG_ASSERT(result);
   if (!result)
      return false;

   float mapYScalar = 1.0f;
   result = pProps->getFloat(pProps->findHandle("MapY"),  mapYScalar);
   BDEBUG_ASSERT(result);
   if (!result)
      return false;

   float mapWidthScalar = 1.0f;
   result = pProps->getFloat(pProps->findHandle("MapWidth"),  mapWidthScalar);
   BDEBUG_ASSERT(result);
   if (!result)
      return false;

   float mapHeightScalar = 1.0f;
   result = pProps->getFloat(pProps->findHandle("MapHeight"),  mapHeightScalar);
   BDEBUG_ASSERT(result);
   if (!result)
      return false;

   float mapViewCenterXScalar = 1.0f;
   result = pProps->getFloat(pProps->findHandle("MapViewX"),  mapViewCenterXScalar);
   BDEBUG_ASSERT(result);
   if (!result)
      return false;

   float mapViewCenterYScalar = 1.0f;
   result = pProps->getFloat(pProps->findHandle("MapViewY"),  mapViewCenterYScalar);
   BDEBUG_ASSERT(result);
   if (!result)
      return false;

   float mapViewWidthScalar = 1.0f;
   result = pProps->getFloat(pProps->findHandle("MapViewWidth"),  mapViewWidthScalar);
   BDEBUG_ASSERT(result);
   if (!result)
      return false;

   float mapViewHeightScalar = 1.0f;
   result = pProps->getFloat(pProps->findHandle("MapViewHeight"), mapViewHeightScalar);
   BDEBUG_ASSERT(result);
   if (!result)
      return false;


   /*
   if (gGame.isSplitScreen())
   {
      if (gGame.isVerticalSplit())
      {
         pProps->getInt(pProps->findHandle("XSV"),  mX);
         pProps->getInt(pProps->findHandle("YSV"),  mY);
         pProps->getInt(pProps->findHandle("WidthSV"),  mWidth);
         pProps->getInt(pProps->findHandle("HeightSV"), mHeight);
         pProps->getFloat(pProps->findHandle("MapViewXSV"),  mMapViewCenterX);
         pProps->getFloat(pProps->findHandle("MapViewYSV"),  mMapViewCenterY);
         pProps->getFloat(pProps->findHandle("MapViewWidthSV"),  mMapViewWidth);
         pProps->getFloat(pProps->findHandle("MapViewHeightSV"), mMapViewHeight);
         pProps->getFloat(pProps->findHandle("IconScaleXSV"), mIconScaleX);
         pProps->getFloat(pProps->findHandle("IconScaleYSV"), mIconScaleY);
      }
      else
      {
         pProps->getInt(pProps->findHandle("XSH"),  mX);
         pProps->getInt(pProps->findHandle("YSH"),  mY);
         pProps->getInt(pProps->findHandle("WidthSH"),  mWidth);
         pProps->getInt(pProps->findHandle("HeightSH"), mHeight);
         pProps->getFloat(pProps->findHandle("MapViewXSH"),  mMapViewCenterX);
         pProps->getFloat(pProps->findHandle("MapViewYSH"),  mMapViewCenterY);
         pProps->getFloat(pProps->findHandle("MapViewWidthSH"),  mMapViewWidth);
         pProps->getFloat(pProps->findHandle("MapViewHeightSH"), mMapViewHeight);
         pProps->getFloat(pProps->findHandle("IconScaleXSH"), mIconScaleX);
         pProps->getFloat(pProps->findHandle("IconScaleYSH"), mIconScaleY);
      }
   }
   else 
   */   

   if (gRender.getAspectRatioMode()==BRender::cAspectRatioMode4x3)
   {
      pProps->getFloat(pProps->findHandle("X640"), mapXScalar);
      pProps->getFloat(pProps->findHandle("Y640"), mapYScalar);
      pProps->getFloat(pProps->findHandle("Width640"), mapWidthScalar);
      pProps->getFloat(pProps->findHandle("Height640"), mapHeightScalar);
      pProps->getFloat(pProps->findHandle("MapViewX640"), mapViewCenterXScalar);
      pProps->getFloat(pProps->findHandle("MapViewY640"), mapViewCenterYScalar);
      pProps->getFloat(pProps->findHandle("MapViewWidth640"),  mapViewWidthScalar);
      pProps->getFloat(pProps->findHandle("MapViewHeight640"), mapViewHeightScalar);

      pProps->getFloat(pProps->findHandle("IconScaleX640"), mIconScaleX);
      pProps->getFloat(pProps->findHandle("IconScaleY640"), mIconScaleY);
   }

   mX = Math::FloatToIntRound(mapXScalar * BD3D::mD3DPP.BackBufferWidth);
   mY = Math::FloatToIntRound(mapYScalar * BD3D::mD3DPP.BackBufferHeight);
   mWidth = Math::FloatToIntRound(mapWidthScalar * BD3D::mD3DPP.BackBufferWidth);
   mHeight = Math::FloatToIntRound(mapHeightScalar * BD3D::mD3DPP.BackBufferHeight);
   mMapViewCenterX = mapViewCenterXScalar * BD3D::mD3DPP.BackBufferWidth;
   mMapViewCenterY = mapViewCenterYScalar * BD3D::mD3DPP.BackBufferHeight;
   mMapViewWidth = mapViewWidthScalar * BD3D::mD3DPP.BackBufferWidth;
   mMapViewHeight = mapViewHeightScalar * BD3D::mD3DPP.BackBufferHeight;

   result = pProps->getFloat(pProps->findHandle("MapViewIconStickyOffset"), mMapViewIconStickyOffset);
   BDEBUG_ASSERT(result);
   if (!result)
      return false;

   result = pProps->getFloat(pProps->findHandle("MapFogScalar"), mMapFogScalar);
   BDEBUG_ASSERT(result);
   if (!result)
      return false;

   result = pProps->getFloat(pProps->findHandle("MapSkirtFogScalar"), mMapSkirtFogScalar);
   BDEBUG_ASSERT(result);
   if (!result)
      return false;

   mFlareDurationHandle = pProps->findHandle("FlareDuration");
   mAlertDurationHandle = pProps->findHandle("AlertDuration");

   pProps->getFloat(pProps->findHandle("RotationSpeed"), mRotationSpeed);
   pProps->getFloat(pProps->findHandle("PanSpeed"), mPanSpeed);
   pProps->getFloat(pProps->findHandle("FullZoomOut"), mFullZoomOut);

   result = pProps->getFloat(pProps->findHandle("SquadIconBaseSize"), mSquadIconBaseSize);
   if (!result)
      return false;
   
   // set the dimensions for the flash movie
   setDimension(mX, mY, mWidth, mHeight);

   // find out the default camera yaw in radians that the user's camera is setup
   // we need this to offset the North Pointer correctly
   float defaultCameraYaw = 0.0f;   
   gConfig.get(cConfigCameraYaw, &defaultCameraYaw);
   mDefaultCameraYawRadians = defaultCameraYaw * cRadiansPerDegree;

   // setup the scale matrix
   setupScaleMatrix();

   mRotationMatrix.makeIdentity();   
   
   initLookups();

   initMapPosition();

   initMap();

   initStaticIconTexture();

   bool coop=false;
   long gametype=BGameSettings::cGameTypeSkirmish;
//-- FIXING PREFIX BUG ID 2586
   const BGameSettings* pSettings = gDatabase.getGameSettings();
//--
   if(pSettings)
   {
      pSettings->getBool(BGameSettings::cCoop, coop);
      pSettings->getLong(BGameSettings::cGameType, gametype);      
   }

   // by default always show focus points
   mbShowFocusPoints = true;
   // don't show focus points in non-coop spc campaign games
   if ((gametype == BGameSettings::cGameTypeCampaign) && (!coop))
      mbShowFocusPoints = false;
   
   setFlag(cFlagInitialized, true);
   return true;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BFlashMinimap::initAfterLoad()
{
   setupScaleMatrix();
   return true;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashMinimap::initMapPosition()
{
   mMap.mColor = cDWORDWhite;
   mMap.mPosition.set(mMapViewCenterX, 0, mMapViewCenterY);
   mMap.mSize  = XMHALF2((float) mMapViewWidth, (float) mMapViewHeight);
   mMap.mType  = (BYTE)-1;
   mMap.mUV    = XMHALF4(0.0f,0.0f,1.0f,1.0f);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashMinimap::setupScaleMatrix()
{
   mMapViewHalfHeight = mMapViewWidth * 0.5f;
   mMapViewHalfWidth  = mMapViewHeight  * 0.5f;

   // setup the scale matrix
   BMatrix preScaleMatrix;
   BMatrix yTranslateMatrix;
   BMatrix yFlipMatrix;

   //-- figure our the conversion scalar that brings our world coords into map coordinates
   mWorldToMapConversionFactor = 1.0f / gTerrainVisual.getTileScale() / gTerrainVisual.getNumXVerts();

   const float cVisMapSize = 512.0f;
   mVisibilityScale = cVisMapSize * mWorldToMapConversionFactor;
   preScaleMatrix.makeScale(cVisMapSize * mWorldToMapConversionFactor, cVisMapSize * mWorldToMapConversionFactor, 1.0f);
   yTranslateMatrix.makeTranslate(0.0f, cVisMapSize-1.0f, 0.0f);
   yFlipMatrix.makeScale(1.0f, -1.0f, 1.0f);
   yFlipMatrix.mult(yFlipMatrix, yTranslateMatrix);
   mVisibilityScaleMatrix.mult(preScaleMatrix, yFlipMatrix);

   //const float uScale = mMapViewWidth / (gTerrainSimRep.getNumXTiles() * gTerrainSimRep.getTileScale());  
   const float scalar = 1.0f; //512.0f * mWorldToMapConversionFactor;
   preScaleMatrix.makeScale(scalar, scalar, 1.0f);   
   yTranslateMatrix.makeTranslate(0,0,0); //(0.0f, 512.0f - 1.0f, 0.0f);
   yFlipMatrix.makeScale(1.0f, -1.0f, 1.0f);
   yFlipMatrix.mult(yFlipMatrix, yTranslateMatrix);
   mScaleMatrix.mult(preScaleMatrix, yFlipMatrix);

}




//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashMinimap::initLookups()
{
   const BFlashProperties* pProps = getProperties();

   for(int i = 0; i < getProtoIconCount(); i++)
   {
      BFlashProtoIcon* pIcon = getProtoIcon(i);
      BDEBUG_ASSERT(pIcon !=NULL);

      BFlashMinimapIconLookup icon;
      icon.mIndex = i;
      
      // NOTE:
      // We are determining whether an icon uses the animated flash texture by looking at the name
      // of the icon rather than having to set explicit flags on each icon instance.  The main
      // reason is because we want to keep the code changes to a minimum.
      if (pIcon->mType.compare("objectIcon") == 0)
      {         
         int protoObjID   = gDatabase.getProtoObject(pIcon->mOwnerName);
         if (protoObjID != -1)
         {
            icon.mProtoID = protoObjID;            
            icon.mIconDataIndex = pProps->findHandle(pIcon->mName.c_str());
            icon.mIconDataIndex2 = pProps->findHandle(pIcon->mName2.c_str());

            
            if (pIcon->mName.findLeft("Anim") != -1)
               icon.mIconDataIndexUsesFlash = true;

            if (pIcon->mName2.findLeft("Anim") != -1)
               icon.mIconDataIndex2UsesFlash = true;

            mIconLookup.add(icon);
         }
      }
      else if (pIcon->mType.compare("miscIcon") == 0)
      {
         if (pIcon->mOwnerName.compare("flare") == 0)
         {
            mFlareIconHandle = pProps->findHandle(pIcon->mName.c_str());            
            mFlareIconSize   = XMHALF2(pIcon->mSize.x, pIcon->mSize.y);
         }
         else if (pIcon->mOwnerName.compare("alert") == 0)
         {
            mAlertIconHandle = pProps->findHandle(pIcon->mName.c_str());

            mAlertIconUsesFlash = false;
            if (pIcon->mName.findLeft("Anim") != -1)
               mAlertIconUsesFlash = true;

            mAlertIconSize   = XMHALF2(pIcon->mSize.x, pIcon->mSize.y);
         }
         else if (pIcon->mOwnerName.compare("squad") == 0)
         {
            mSquadIconUVHandle = pProps->findHandle(pIcon->mName.c_str());

            mSquadIconUsesFlash = false;
            if (pIcon->mName.findLeft("Anim") != -1)
               mSquadIconUsesFlash = true;

            mSquadIconProtoID = i;
         }
         else if (pIcon->mOwnerName.compare("centerpoint") == 0)
         {
            mCenterPointIconUVHandle = pProps->findHandle(pIcon->mName.c_str());

            mCenterPointUsesFlash = false;
            if (pIcon->mName.findLeft("Anim") != -1)
               mCenterPointUsesFlash = true;

            mCenterPointIconProtoID = i;
         }
         else if (pIcon->mOwnerName.compare("sensorlock") == 0)
         {
            mSensorLockIconHandle = pProps->findHandle(pIcon->mName.c_str());

            mSensorLockUsesFlash = false;
            if (pIcon->mName.findLeft("Anim") != -1)
               mSensorLockUsesFlash = true;

            mSensorLockIconProtoID = i;
         }

      }
   }
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashMinimap::initStaticIconTexture()
{
   const BFlashProperties* pProps = getProperties();
   BDEBUG_ASSERT(pProps);
      
   BFixedString128 value;
   BFlashPropertyHandle handle = pProps->findStringHandle("IconStaticTexture");
   pProps->getString(handle, value);
   mStaticIconTextureHandle = gD3DTextureManager.getOrCreateHandle(value, BFILE_OPEN_NORMAL, BD3DTextureManager::cUI, false, cDefaultTextureRed, true, false, "BFlashMinimap");
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashMinimap::initMap()
{
/*
   mMap.mColor = cDWORDWhite;
   mMap.mPosition.set(mMapViewCenterX, 0, mMapViewCenterY);
   mMap.mSize  = XMHALF2((float) mMapViewWidth, (float) mMapViewHeight);
   mMap.mType  = (BYTE)-1;
   mMap.mUV    = XMHALF4(0.0f,0.0f,1.0f,1.0f);
*/
         
   BFixedString128 value;

   const BFlashProperties* pProps = getProperties();
   BDEBUG_ASSERT(pProps);
      
   BFlashPropertyHandle handle = pProps->findStringHandle("MapMaskTexture");
   pProps->getString(handle, value);
   mMapMaskTextureHandle = gD3DTextureManager.getOrCreateHandle(value, BFILE_OPEN_NORMAL, BD3DTextureManager::cUI, false, cDefaultTextureRed, true, false, "BFlashMinimap");
   
   handle = pProps->findStringHandle("MapBackgroundTexture");
   pProps->getString(handle, value);
   mMapBackgroundTextureHandle = gD3DTextureManager.getOrCreateHandle(value, BFILE_OPEN_NORMAL, BD3DTextureManager::cUI, false, cDefaultTextureRed, true, false, "BFlashMinimap");

   handle = pProps->findStringHandle("VisibilityMaskTexture");
   pProps->getString(handle, value);
   mVisMaskTextureHandle = gD3DTextureManager.getOrCreateHandle(value, BFILE_OPEN_NORMAL, BD3DTextureManager::cUI, false, cDefaultTextureRed, true, false, "BFlashMinimap");
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashMinimap::updateMap()
{  
//-- FIXING PREFIX BUG ID 2587
   const BUser* pUser = gUserManager.getUser(BUserManager::cPrimaryUser);
//--


   BVector newCameraPosOnWorld = pUser->getHoverPoint();

   BVector delta = newCameraPosOnWorld - mCameraPos;   
   delta *= mPanSpeed;

   BVector cameraPosOnWorld;
   cameraPosOnWorld = mCameraPos + delta;
   mCameraPos = cameraPosOnWorld;

   long numPlayers = gWorld->getNumberPlayers();
   for (long i=0; i<numPlayers; i++)
   {
      const BPlayer* pPlayer = gWorld->getPlayer(i);
      if (pPlayer)
      {
         BVector focusPos = pPlayer->getLookAtPos();
         if (gGame.isSplitScreen() && i == gUserManager.getSecondaryUser()->getPlayerID())
            focusPos = gUserManager.getSecondaryUser()->getHoverPoint();

         BVector focusDelta = focusPos - mFocusPos[i];
         focusDelta *= mPanSpeed;
         mFocusPos[i] += focusDelta;
      }
   }   
   
   //-- convert world coordinates to our map coordinates in 0..1 range
   float mapU;
   float mapV;
   mapU = cameraPosOnWorld.x * mWorldToMapConversionFactor; //- / gTerrainVisual.getTileScale()) / gTerrainVisual.getNumXVerts();
   mapV = 1.0f - (cameraPosOnWorld.z * mWorldToMapConversionFactor); // --/ gTerrainVisual.getTileScale()) / gTerrainVisual.getNumXVerts();

   if (mbFullZoomOut)
   {
      mapU = 0.5f;
      mapV = 0.5f;
   }
   
   //-- compute the width in uv space first
   float zoomHalfWidth = mZoom * 0.5f;
   float uHalfWidth  = zoomHalfWidth; 
   float vHalfHeight = zoomHalfWidth;

   float u0,v0,u1,v1;
   u0 = mapU;
   v0 = mapV;
   u1 = uHalfWidth;
   v1 = vHalfHeight;

   mMap.mColor.c = gWorld->getPlayerColor(pUser->getPlayerID(), BWorld::cPlayerColorContextMinimap);
   mMap.mPosition.set(mMapViewCenterX, 0, mMapViewCenterY);
   mMap.mSize  = XMHALF2((float) mMapViewWidth, (float) mMapViewHeight);
   mMap.mType  = 0;
   mMap.mUV    = XMHALF4(u0,v0,u1,v1);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashMinimap::updateFlares()
{   
   double curGameTime = gWorld->getGametimeFloat();
   for (uint i = 0; i < mFlares.getSize(); ++i)
   {
      //-- update death
      if (curGameTime >= mFlares[i].mDeathTime)
      {
         mFlares[i] = mFlares.back();
         mFlares.popBack();
         i--;
         continue;
      }      

      computeIconPosition(mCameraPos, mFlares[i].mPosition2, mFlares[i].mPosition);      
   }
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashMinimap::updateIconPositions(BDynamicSimArray<BFlashMinimapItem>& icons)
{   
   for (uint i = 0; i < icons.getSize(); ++i)
   {
      if (icons[i].mType == cMinimapItemTypeCenterPoint)
      {
         //-- if we are zoomed out recompute the center point from world space camera to map space
         if (mbFullZoomOut)
            computeIconPosition(mCameraPos, mCameraPos, icons[i].mPosition);
         else //-- stick it in the center in map space
            icons[i].mPosition = XMVectorSet(mMapViewHalfWidth, 0, mMapViewHalfHeight, 1);

         // done -- go to the next icon
         continue;
      }

      //-- update all others
      computeIconPosition(mCameraPos, icons[i].mPosition2, icons[i].mPosition);      
   }
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashMinimap::deinit()
{
   if (mpMovie)
   {
      gFlashGateway.unregisterEventHandler(mpMovie, mSimEventHandle);
      gFlashGateway.releaseInstance(mpMovie);
   }
   mpMovie = NULL;

   if (mpIconMovie)
   {
      gFlashGateway.unregisterEventHandler(mpIconMovie, mSimEventHandle);
      gFlashGateway.releaseInstance(mpIconMovie);
   }
   mpIconMovie = NULL;

   // we are not responsible for the map texture but BScenario is
   mMapTextureHandle = cInvalidManagedTextureHandle;

   if (mVisMaskTextureHandle != cInvalidManagedTextureHandle)
      gD3DTextureManager.releaseManagedTextureByHandle(mVisMaskTextureHandle);
   mVisMaskTextureHandle=cInvalidManagedTextureHandle;

   if (mMapMaskTextureHandle != cInvalidManagedTextureHandle)
      gD3DTextureManager.releaseManagedTextureByHandle(mMapMaskTextureHandle);
   mMapMaskTextureHandle = cInvalidManagedTextureHandle;

   if (mMapBackgroundTextureHandle != cInvalidManagedTextureHandle)
      gD3DTextureManager.releaseManagedTextureByHandle(mMapBackgroundTextureHandle);
   mMapBackgroundTextureHandle = cInvalidManagedTextureHandle;

   //-- don't release this this a dynamic flash movie texture
   mIconTextureHandle= cInvalidManagedTextureHandle;

   if (mStaticIconTextureHandle != cInvalidManagedTextureHandle)
      gD3DTextureManager.releaseManagedTextureByHandle(mStaticIconTextureHandle);
   mStaticIconTextureHandle = cInvalidManagedTextureHandle;
   
   setFlag(cFlagInitialized, false);

   mbFirstRender = true;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashMinimap::enter()
{
   if (!getFlag(cFlagInitialized))
      return;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashMinimap::leave()
{
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashMinimap::update(float elapsedTime)
{      
   ASSERT_MAIN_THREAD   
   updateZoom();
   updateMap();
   updateFlares();
   updateRotation();   
   updateIconPositions(mIcons);
   updateIconPositions(mStaticIcons);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashMinimap::updateRotation()
{
   if (gGame.isSplitScreen())
      return;

   BVector  cameraDir;
   BCamera  *camera = gUserManager.getUser(BUserManager::cPrimaryUser)->getCamera();

   if (_fabs(camera->getCameraDir().y) < _fabs(camera->getCameraUp().y))
      cameraDir = BVector(camera->getCameraDir().x, 0.0f, camera->getCameraDir().z);
   else
      cameraDir = BVector(camera->getCameraUp().x, 0.0f, camera->getCameraUp().z);

   float theta = circleTan(cameraDir.x, cameraDir.z) - cPiOver2;
      
   float delta = 0.0f;
   float delta2 = 0.0f;
   float chosenDelta = 0.0f;
  
   delta = theta - mRotationAngle;

   if (mRotationAngle > theta)
      delta2 = (theta + cTwoPi) - mRotationAngle;
   else if (theta > mRotationAngle)
      delta2 = (theta - cTwoPi) - mRotationAngle;

   chosenDelta = delta;
   if (fabs(delta2) < fabs(delta))         
      chosenDelta = delta2;
   
   chosenDelta *= mRotationSpeed;

   theta = mRotationAngle + chosenDelta;

   theta = Math::fPosMod(theta, cTwoPi);                  

   if ((theta != mRotationAngle) || mbForceRotationUpdate)
   {        
      mbForceRotationUpdate = false;
      mRotationAngle = theta;

      float offset = cTwoPi - mDefaultCameraYawRadians + mRotationOffset;
      float angle = (mRotationAngle - offset) * cDegreesPerRadian;      

      GFxValue value;
      value.SetNumber((double)angle);
      mpMovie->invokeActionScript(getProtoASFunctionName(eMinimapASFunctionSetRotation), &value, 1);

      calulateRotationMatrix();
   }
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashMinimap::setMapFlash(bool flashOn)
{
   GFxValue value;
   value.SetBoolean(flashOn);
   mpMovie->invokeActionScript("setMapFlash", &value, 1);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashMinimap::repositionMap(float x, float y, float w, float h)
{
   setMapViewCenterX(x);
   setMapViewCenterY(y);
   setMapViewWidth(w);
   setMapViewHeight(h);

   // reinit the map now.
   setupScaleMatrix();
   initMapPosition();
   calulateRotationMatrix();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashMinimap::calulateRotationMatrix()
{
   BMatrix preRotationTransform;
   preRotationTransform.makeTranslate(-mMapViewHalfWidth, -mMapViewHalfHeight, 0.0f);            

   BMatrix postRotationTransform;
   postRotationTransform.makeTranslate(mMapViewHalfWidth, mMapViewHalfHeight, 0.0f);

   BMatrix rotationTransform;
   rotationTransform.makeRotateZ(mRotationAngle);

   mRotationMatrix.mult(preRotationTransform, rotationTransform);
   mRotationMatrix.mult(mRotationMatrix, postRotationTransform);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashMinimap::updateZoom()
{
//-- FIXING PREFIX BUG ID 2589
   const BUser* pUser = gUserManager.getUser(BUserManager::cPrimaryUser);
//--
   BDEBUG_ASSERT(pUser!=NULL);

   float cameraZoom    = pUser->getCameraZoom();

   float cameraZoomMin=1.0f;
   float cameraZoomMax=100.0f;

   //gConfig.get(cConfigCameraZoomMin, &cameraZoomMin);
   //gConfig.get(cConfigCameraZoomMax, &cameraZoomMax);

   float alpha = (cameraZoom - cameraZoomMin) / (cameraZoomMax - cameraZoomMin);
   alpha = Math::Clamp(alpha, 0.0f, 1.0f);

   const BFlashProperties* pProps = getProperties();
   BDEBUG_ASSERT(pProps!=NULL);

   float minScale = 0.5f;
   float maxScale = 1.0f;
   pProps->getFloat(pProps->findHandle("MapViewMinPct"), minScale);
   pProps->getFloat(pProps->findHandle("MapViewMaxPct"), maxScale);

   mZoom = ((maxScale-minScale) * alpha) + minScale;
   
   if (mbFullZoomOut)
      mZoom = mFullZoomOut;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashMinimap::renderBegin()
{   
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashMinimap::render()
{   
   ASSERT_MAIN_THREAD
   SCOPEDSAMPLEID(FlashUIGame, 0xFFFF0000);

   if (mbFirstRender)
   {
      mbFirstRender = false;
      return;
   }

   if (!mbVisible)
      return;   

   bool blocked = gUserManager.getUser(BUserManager::cPrimaryUser)->getPlayer()->getFlagMinimapBlocked();

   mIconTextureHandle = mpIconMovie->mRenderTargetHandle;

   if (!blocked)
      renderMap();

   static bool bRenderMovie = true;
   if (bRenderMovie)
      mpMovie->render();
      
   if (!blocked)
   {
      renderStaticIcons();
      renderIcons();
      renderFlares();   
   }
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashMinimap::renderIconAnimations()
{
   ASSERT_MAIN_THREAD
   if (mpIconMovie)
      mpIconMovie->render();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashMinimap::workerRenderIconAnimations()
{
   ASSERT_RENDER_THREAD

   if (mpIconMovie)
      mpIconMovie->workerRender();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashMinimap::renderEnd()
{
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashMinimap::renderIcons(void)
{
   ASSERT_MAIN_THREAD
   int numIcons = mIcons.getNumber();
   if (numIcons < 1)
      return;

   //-- sort
   mIcons.sort();
   
   uint bytes = numIcons*sizeof(BFlashMinimapItem);
   uchar* pFrameStorage = static_cast<uchar*> (gRenderThread.allocateFrameStorage(bytes));
   XMemCpy(pFrameStorage, mIcons.getData(), bytes);

   BMatrix worldMatrix;
   BMatrix translation;
   translation.makeIdentity();
   translation.makeTranslate(mMapViewCenterX-mMapViewHalfWidth, mMapViewCenterY-mMapViewHalfHeight, 0.0f);
   worldMatrix.mult(mRotationMatrix, translation);
   
   gFlashMinimapRenderer.submitIconRender(worldMatrix, mIconTextureHandle, mpIconMovie->mInstanceSlotIndex, numIcons, pFrameStorage);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashMinimap::renderStaticIcons(void)
{
   ASSERT_MAIN_THREAD
   int numIcons = mStaticIcons.getNumber();
   if (numIcons < 1)
      return;

   //-- sort
   mStaticIcons.sort();
   
   uint bytes = numIcons*sizeof(BFlashMinimapItem);
   uchar* pFrameStorage = static_cast<uchar*> (gRenderThread.allocateFrameStorage(bytes));
   XMemCpy(pFrameStorage, mStaticIcons.getData(), bytes);

   BMatrix worldMatrix;
   BMatrix translation;
   translation.makeIdentity();
   translation.makeTranslate(mMapViewCenterX-mMapViewHalfWidth, mMapViewCenterY-mMapViewHalfHeight, 0.0f);
   worldMatrix.mult(mRotationMatrix, translation);
   
   gFlashMinimapRenderer.submitIconRender(worldMatrix, mStaticIconTextureHandle, -1, numIcons, pFrameStorage);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashMinimap::renderMap(void)
{
   ASSERT_MAIN_THREAD

   uint bytes = sizeof(BFlashMinimapItem);
   uchar* pFrameStorage = static_cast<uchar*> (gRenderThread.allocateFrameStorage(bytes));
   XMemCpy(pFrameStorage, &mMap, bytes);

   BMatrix worldMatrix;   
   worldMatrix.makeIdentity();   
   
   XMCOLOR fogColor;
   fogColor.c = cDWORDPurple;

   float fogMapScalar = mMapFogScalar;
   float fogMapSkirtScalar = mMapSkirtFogScalar;

#ifdef BLACK_MAP
   fogMapSkirtScalar = 0.0f;
   fogMapScalar = 0.0f;
#endif
   
   if (gConfig.isDefined(cConfigNoFogMask) || (gWorld->getFlagAllVisible() && gWorld->getFlagShowSkirt()))
   {
      fogMapSkirtScalar = 1.0f;
      fogMapScalar = 1.0f;
   }   

   DWORD samplingMode = D3DTADDRESS_MIRROR;
   if (!mbMapSkirtMirroring)
      samplingMode = D3DTADDRESS_CLAMP;
   
   gFlashMinimapRenderer.submitMapRender(worldMatrix, mMapTextureHandle, mMapMaskTextureHandle, mMapBackgroundTextureHandle, mRotationAngle, fogColor, fogMapScalar, fogMapSkirtScalar, pFrameStorage, samplingMode);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashMinimap::renderFlares(void)
{
   ASSERT_MAIN_THREAD
   int numFlares = mFlares.getNumber();
   if (numFlares < 1)
      return;

   uint bytes = numFlares*sizeof(BFlashMinimapItem);
   uchar* pFrameStorage = static_cast<uchar*> (gRenderThread.allocateFrameStorage(bytes));
   XMemCpy(pFrameStorage, mFlares.getData(), bytes);

   BMatrix worldMatrix;
   BMatrix translation;
   translation.makeIdentity();
   translation.makeTranslate(mMapViewCenterX-mMapViewHalfWidth, mMapViewCenterY-mMapViewHalfHeight, 0.0f);
   worldMatrix.mult(mRotationMatrix, translation);

   gFlashMinimapRenderer.submitIconRender(worldMatrix, mIconTextureHandle, mpIconMovie->mInstanceSlotIndex, numFlares, pFrameStorage);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BFlashMinimap::handleInput(int port, BInputEventType event, BInputControlType controlType, BInputEventDetail& detail)
{
   return false;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BFlashMinimap::receiveEvent(const BEvent& event, BThreadIndex threadIndex)
{
   if (threadIndex == cThreadIndexSim)
   {
      switch (event.mEventClass)
      {
         case BFlashManager::cFlashEventFileReload:
         {            
            setDimension(mX, mY, mWidth, mHeight);            
         }
         break;
      }
   }
   return false;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BManagedTextureHandle BFlashMinimap::getRenderTargetTexture()
{
   if (mpMovie)
      return mpMovie->mRenderTargetHandle;

   return cInvalidManagedTextureHandle;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
IDirect3DTexture9* BFlashMinimap::getRenderTargetTexturePtr()
{
   if (mpMovie)
      return mpMovie->mpRenderTargetTexture;

   return NULL;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashMinimap::setDimension(int x, int y, int width, int height)
{
   if (mpMovie)
      mpMovie->setDimension(x,y,width,height);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashMinimap::setVisible(bool bON)
{
   mbVisible = bON;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashMinimap::setRotationOffset(float degrees)
{
   if (!getFlag(cFlagInitialized))
      return;

   if (!mpMovie)
      return;

   degrees = Math::Clamp(degrees, -360.0f, 360.0f);
   mRotationOffset = degrees * cRadiansPerDegree;  
   mbForceRotationUpdate = true;   
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashMinimap::reset()
{
   ASSERT_MAIN_THREAD

   // ajl 12/1/08 - Don't reset the rotation offset here otherwise the value will be lost. The minimap is reset multiple times 
   // while the game is initialized. The problem where the value gets lost shows up whe n loading a save game.
   //mRotationOffset = 0.0f;   

   mIcons.resize(0);
   mStaticIcons.resize(0);
   mVisibility[0].resize(0);
   mVisibility[1].resize(0);
   mVisibilityBlockers.resize(0);
   mQueuedItems.resize(0);
   mQueuedReveals.resize(0);
   mQueuedBlocks.resize(0);
   mQueuedAlerts.resize(0);
   for (uint i=0; i<cMaximumSupportedPlayers; i++)
      mFocusPos[i].zero();

   addCenterPoint();
   addFocusPositions();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashMinimap::clear()
{
   clearItems();
   clearReveals();
   clearBlocks();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashMinimap::clearItems()
{
   ASSERT_MAIN_THREAD
   mIcons.resize(0);
   mStaticIcons.resize(0);
   addCenterPoint();
   addFocusPositions();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashMinimap::clearReveals()
{
   ASSERT_MAIN_THREAD

   //-- toggle visibility index between 0 and 1
   mVisibilityIndex = 1 - mVisibilityIndex;

   mVisibility[mVisibilityIndex].resize(0);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashMinimap::commit()
{
   clear();
   commitItems();
   commitReveals();
   commitBlocks();
   commitAlerts();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashMinimap::commitReveals()
{
   for (uint i = 0; i < mQueuedReveals.getSize(); ++i)
   {
      reveal(mQueuedReveals[i]);
   }

   mQueuedReveals.resize(0);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashMinimap::commitAlerts()
{
   for (uint i = 0; i < mQueuedAlerts.getSize(); ++i)
   {
      alert(mQueuedAlerts[i]);
   }

   mQueuedAlerts.resize(0);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashMinimap::clearBlocks()
{
   ASSERT_MAIN_THREAD

   mVisibilityBlockers.resize(0);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashMinimap::commitBlocks()
{
   for (uint i = 0; i < mQueuedBlocks.getSize(); ++i)
   {
      block(mQueuedBlocks[i]);
   }

   mQueuedBlocks.resize(0);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashMinimap::addItem(int type, void* pData)
{   
   ASSERT_MAIN_THREAD

   mQueuedItems.add(BQueuedItem(type, pData));
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashMinimap::commitItems()
{
   for (uint i=0;i<mQueuedItems.getSize(); ++i)
   {
      int type = mQueuedItems[i].mType;
      void *pData = mQueuedItems[i].mpData;
   
//-- FIXING PREFIX BUG ID 2603
      const BUser* pUser = gUserManager.getUser(BUserManager::cPrimaryUser);
//--
//-- FIXING PREFIX BUG ID 2604
      const BUser* pSecondaryUser = gUserManager.getSecondaryUser();
//--

      const BFlashProperties* pProperties = getProperties();
      if (type == cMinimapItemTypeUnit || type == cMinimapItemTypeObject)
      {
         BPlayerID playerID = -1;
//-- FIXING PREFIX BUG ID 2599
         const BProtoObject* pProtoObject = NULL;
//--
//-- FIXING PREFIX BUG ID 2600
         const BUnit* pUnit = NULL;
//--
         BObject* pObject = NULL;
         long protoID = -1;
         BVector itemPosition;
         long teamID = -1;
         float lineofSight = 0.0f;
         BColor bColor;
         bColor.set(-1.0f, -1.0f, -1.0f);
         float size = -1.0f;
         bool revealMap = false;

         bool tagged = false;
         if (type == cMinimapItemTypeUnit)
         {
            pUnit = (BUnit*)pData;
            playerID = pUnit->getColorPlayerID();
            pProtoObject = (BProtoObject*)pUnit->getProtoObject();
            protoID = pUnit->getProtoID();
            
            // jce [10/22/2008] -- changing to interpolated position for smooth changes when subupdating
            //itemPosition = pUnit->getPosition();
            itemPosition = pUnit->getInterpolatedPosition();
            
            teamID = pUnit->getTeamID();
            lineofSight = pUnit->getLOS();
            tagged = pUnit->getFlagSensorLockTagged();
            bColor = pUnit->getIconColor();
            size = pUnit->getIconSize();
            revealMap = pUnit->getFlagLOS();

            if (pUnit->getFlagNoRender())
                  continue;
         }
         else // cMinimapItemTypeObject
         {
            pObject = (BObject*) pData;
            playerID = pObject->getPlayerID();
            pProtoObject = (BProtoObject*) pObject->getProtoObject();         
            protoID = pObject->getProtoID();

            // jce [10/22/2008] -- changing to interpolated position for smooth changes when subupdating
            //itemPosition = pObject->getPosition();
            itemPosition = pObject->getInterpolatedPosition();
            
            teamID = pObject->getTeamID();
            lineofSight = pObject->getLOS();
            tagged = pObject->getFlagSensorLockTagged();
            bColor = pObject->getIconColor();
            size = pObject->getIconSize();
            revealMap = pObject->getFlagLOS();

            if (pObject->getFlagNoRender())
                  continue;
         }
              
         // Unit/Object has been tagged by a sensor lock so override with a sensor lock icon
         if (tagged)
         {
//-- FIXING PREFIX BUG ID 2595
            const BFlashProtoIcon* pProtoIcon = getProtoIcon(mSensorLockIconProtoID);
//--
            BDEBUG_ASSERT(pProtoIcon != NULL);

            BVector unitPos;
            computeIconPosition(mCameraPos,/*pUser->getHoverPoint(),*/ itemPosition, unitPos);

            BFlashMinimapItem item;
            item.mPosition = unitPos;  
            item.mPosition2= itemPosition;
            item.mPriority = pProtoIcon->mPriority;

            if (size > 0.0f )
            {            
               item.mSize = XMHALF2(size*mIconScaleX, size*mIconScaleY);
            }
            else
            {
               float sizeX = XMConvertHalfToFloat(pProtoIcon->mSize.x) * mIconScaleX;
               float sizeY = XMConvertHalfToFloat(pProtoIcon->mSize.y) * mIconScaleY;
               item.mSize = XMHALF2(sizeX, sizeY);
            }

            XMCOLOR color = pProtoIcon->mColor;
            if (bColor.r >= 0.0f)
            {
               color.c = bColor.asDWORD();
            }
            else if ((playerID > 0) && pProtoIcon->mAllowPlayerColor)
            {
               color.c = gWorld->getPlayerColor(playerID, BWorld::cPlayerColorContextMinimap);
            }         

            pProperties->getHalf4(mSensorLockIconHandle, item.mUV);

            item.mColor = color;
            item.mType = cMinimapItemTypeSensorLock;
            if (mSensorLockUsesFlash)
               mIcons.add(item);
            else
               mStaticIcons.add(item);
         }
         else
         {
//-- FIXING PREFIX BUG ID 2598
            const BFlashMinimapIconLookup* pIconLookup = NULL;       
//--
            for (int i = 0; i < mIconLookup.getNumber(); ++i)
            {
               if (mIconLookup[i].mProtoID == protoID)
               {
                  pIconLookup = &mIconLookup[i];

                  if (pIconLookup && pIconLookup->mIconDataIndex != BFlashProperties::eInvalidFlashPropertyHandle)
                  {
//-- FIXING PREFIX BUG ID 2596
                     const BFlashProtoIcon* pProtoIcon = getProtoIcon(pIconLookup->mIndex);
                     bool bIconUsesFlash = pIconLookup->mIconDataIndexUsesFlash;
//--

                     BVector unitPos;
                     computeIconPosition(mCameraPos,/*pUser->getHoverPoint(),*/ itemPosition, unitPos);

                     BFlashMinimapItem item;
                     item.mPosition = unitPos;
                     item.mPosition2 = itemPosition;
                     item.mPriority = pProtoIcon->mPriority;

                     if (size > 0.0f )
                     {                     
                        item.mSize = XMHALF2(size*mIconScaleX, size*mIconScaleY);
                     }
                     else
                     {
                        float sizeX = XMConvertHalfToFloat(pProtoIcon->mSize.x);
                        float sizeY = XMConvertHalfToFloat(pProtoIcon->mSize.y);

                        sizeX *= mIconScaleX;
                        sizeY *= mIconScaleY;

                        item.mSize = XMHALF2(sizeX, sizeY);
                     }

//-- FIXING PREFIX BUG ID 2597
                     const BFlashProtoIcon* pIcon = getProtoIcon(pIconLookup->mIndex);
//--
                     BDEBUG_ASSERT(pIcon!=NULL);

                     XMCOLOR color = pIcon->mColor;
                     if (bColor.r >= 0.0f)
                     {
                        color.c = bColor.asDWORD();
                     }
                     else if ((playerID > 0) && pIcon->mAllowPlayerColor)
                     {
                        color.c = gWorld->getPlayerColor(playerID, BWorld::cPlayerColorContextMinimap);
                     }         

                     pProperties->getHalf4(pIconLookup->mIconDataIndex, item.mUV);

                     //-- do we have a secondary icon we could use?
                     if (pIconLookup->mIconDataIndex2 != BFlashProperties::eInvalidFlashPropertyHandle)
                     {
                        //-- handle a unit
                        if (type == cMinimapItemTypeUnit && pUnit != NULL)
                        {
                           //-- if this is a unit see if we need to show a special icon when its being captured
                           if (pUnit->getFlagBeingCaptured() || pUnit->isExplored(pUser->getTeamID()))
                           {
                              XMHALF4 uv;
                              if (pProperties->getHalf4(pIconLookup->mIconDataIndex2, uv))
                                 item.mUV = uv;

                              bIconUsesFlash = pIconLookup->mIconDataIndex2UsesFlash;
                           }
                        }
                        else if (type == cMinimapItemTypeObject && pObject != NULL)
                        {
                           //-- if this is a unit see if we need to show a special icon when its being captured
                           const BProtoObject* pProtoObject = pObject->getProtoObject();
                           if ((pProtoObject && !pProtoObject->isType(gDatabase.getOTIDIcon())) &&
                              (pObject->getFlagBeingCaptured() || pObject->isExplored(pUser->getTeamID()) || pObject->getDopple()))
                           {
                              XMHALF4 uv;
                              if (pProperties->getHalf4(pIconLookup->mIconDataIndex2, uv))
                                 item.mUV = uv;

                              bIconUsesFlash = pIconLookup->mIconDataIndex2UsesFlash;
                           }
                        }
                     }
                                                         
                     item.mColor= color;
                     item.mType = (BYTE) type;

                     if (bIconUsesFlash)
                        mIcons.add(item);
                     else
                        mStaticIcons.add(item);
                  }
               }
            }
         }

         //-- reveal it 
         if (revealMap)
         {
            if (BTeam::canTeamASeeTeamB(pUser->getTeamID(), teamID))
               reveal(itemPosition, lineofSight);
            else if (gGame.isSplitScreen())
            {
               if (pSecondaryUser && BTeam::canTeamASeeTeamB(pSecondaryUser->getTeamID(), teamID))
                  reveal(itemPosition, lineofSight);
            }
         }
      }  
      else if (type == cMinimapItemTypeSquad)
      {
         // [6/27/2008 xemu] modified code from using an aggregate of all squads in a platoon, to just one squad at a time. 
//-- FIXING PREFIX BUG ID 2601
         const BSquad* pSquad = (BSquad*)pData;     
//--
         const long playerID = pSquad->getPlayerID();
//-- FIXING PREFIX BUG ID 2602
         const BFlashProtoIcon* pProtoIcon = getProtoIcon(mSquadIconProtoID);
//--
         BDEBUG_ASSERT(pProtoIcon!=NULL);
         if (!pProtoIcon)
               continue;

         XMCOLOR color;      
         if (playerID == 0)
            color.c = cDWORDGreen; //packRGBA(cDWORDGreen, 1.0f); //pProtoObject->getMinimapColor(), 1.0f);
         else
            color.c = gWorld->getPlayerColor(playerID, BWorld::cPlayerColorContextMinimap);

         static bool bUseNewPos = true;

         BVector squadPos = XMVectorZero();         
         bool bVisible = false;

         if (pSquad->isVisible(pUser->getTeamID()))
            bVisible = true;
         else if (gGame.isSplitScreen())
         {
            if (pSecondaryUser && pSquad->isVisible(pSecondaryUser->getTeamID()))
               bVisible = true;
         }

         if((pSquad->getFlagCloaked() && !pSquad->getFlagCloakDetected() && pUser->getTeamID() != pSquad->getTeamID()))
         {
            bVisible = false;
         }

         // [9/22/2008 xemu] if a squad has a leader marked as "always visible" for the minimap, then respect that 
         if (!bVisible)
         {
            BUnit *pSquadLeader = pSquad->getLeaderUnit();
            if ((pSquadLeader != NULL) && (pSquadLeader->getProtoObject() != NULL) && (pSquadLeader->getProtoObject()->getFlagAlwaysVisibleOnMinimap()))
            {
               // [9/22/2008 xemu] note that per design request, "always visible" really means "always visible under fog and not blackmap" 
               if (gVisibleMap.isPositionExploredToTeam(pSquadLeader->getPosition(), pUser->getTeamID()))
                  bVisible = true;
            }
         }
         if (!bVisible)
         {
               continue;
         }

         squadPos = pSquad->getInterpolatedPosition();
         squadPos.w = 1.0f;

         BVector unitPos;
         computeIconPosition(mCameraPos, squadPos, unitPos);
            
         BFlashMinimapItem item;
         item.mPosition = unitPos;
         item.mPosition2 = squadPos;
         static float iconSize = 25.0f;

         //-- poor man's version of determining how powerful a platoon's strength
         //-- the more children the bigger the icon gets.  
         float iconSizeMultiplier = pSquad->getProtoSquad()->getMinimapScale();
         /*
         uint squadStrength = pPlatoon->getNumberChildren();
         if (squadStrength > 2)
            iconSizeMultiplier = 1.25f;
         if (squadStrength > 5)
            iconSizeMultiplier = 2.0f;
         */

         item.mSize = XMHALF2(mSquadIconBaseSize*iconSizeMultiplier, mSquadIconBaseSize*iconSizeMultiplier);
         pProperties->getHalf4(mSquadIconUVHandle, item.mUV);      
         item.mColor = color;
         item.mType = cMinimapItemTypeSquad;
         item.mPriority = pProtoIcon->mPriority;

         if (mSquadIconUsesFlash)
            mIcons.add(item);
         else
            mStaticIcons.add(item);
      }
   }

   mQueuedItems.resize(0);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------2
void BFlashMinimap::addFlare(BVector pos, int playerID, int flareType)
{
   ASSERT_MAIN_THREAD

   flareType;
   
   const BFlashProperties* pProperties = getProperties();

   XMCOLOR color;      
   if (playerID == 0)
      color.c = cDWORDGreen; //packRGBA(cDWORDGreen, 1.0f); //pProtoObject->getMinimapColor(), 1.0f);
   else
      color.c = gWorld->getPlayerColor(playerID, BWorld::cPlayerColorContextMinimap);
      
   BVector unitPos;
   computeIconPosition(mCameraPos, pos, unitPos);

   BFlashMinimapItem item;
   
   item.mPosition  = unitPos;
   item.mPosition2 = pos;
   
   // Fixes Bug 15548   
   float sizeX = XMConvertHalfToFloat(mFlareIconSize.x) * mIconScaleX;
   float sizeY = XMConvertHalfToFloat(mFlareIconSize.y) * mIconScaleY;
   item.mSize  = XMHALF2(sizeX, sizeY);
   
   pProperties->getHalf4(mFlareIconHandle, item.mUV);   
   item.mColor    = color;
   item.mType = cMinimapItemTypeFlare;

   float flareDuration = 1.0f;
   pProperties->getFloat(mFlareDurationHandle, flareDuration);
   item.mDeathTime = gWorld->getGametimeFloat() + flareDuration;
   mFlares.add(item);

   switch (flareType)
   {
      case BUIGame::cFlareLook: gUIGame.playSound(BSoundManager::cSoundFlareLook); break;
      case BUIGame::cFlareHelp: gUIGame.playSound(BSoundManager::cSoundFlareHelp); break;
      case BUIGame::cFlareMeet: gUIGame.playSound(BSoundManager::cSoundFlareMeet); break;
      case BUIGame::cFlareAttack: gUIGame.playSound(BSoundManager::cSoundFlareAttack); break;
      default: gUIGame.playSound(BSoundManager::cSoundFlare); break;
   }
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashMinimap::addFocusPositions()
{
   ASSERT_MAIN_THREAD

   if (!mbShowFocusPoints)
      return;

   const BFlashProperties* pProperties = getProperties();
   BPlayerID primaryUserPlayerID = gUserManager.getPrimaryUser()->getPlayerID();
//-- FIXING PREFIX BUG ID 2607
   const BFlashProtoIcon* pProtoIcon = getProtoIcon(mCenterPointIconProtoID);
//--
   if (!pProtoIcon)
      return;
   
   int numPlayers = gWorld->getNumberPlayers();
   for (BPlayerID i=1; i<numPlayers; i++)
   {
      if (i == primaryUserPlayerID)
         continue;
      const BPlayer* pPlayer = gWorld->getPlayer(i);
      if (!pPlayer)
         continue;
      if (!pPlayer->isPlaying())
         continue;

      // mrh 8/12/2008 - Only show the AI Eye for CONTROLLABLE players (i.e., skirmish AI players, or humans, not NPC's)
      if (!pPlayer->isHuman() && !pPlayer->isComputerAI())
         continue;

      if (!gWorld->getFlagAllVisible())
         if (!gGame.isSplitScreen() && !pPlayer->isAlly(primaryUserPlayerID, false))
            continue;

      BVector unitPos;
      computeIconPosition(mCameraPos, mFocusPos[i], unitPos);

      BFlashMinimapItem item;
      item.mPosition = unitPos;
      item.mPosition2 = mFocusPos[i];

      float sizeX = XMConvertHalfToFloat(pProtoIcon->mSize.x) * mIconScaleX;
      float sizeY = XMConvertHalfToFloat(pProtoIcon->mSize.y) * mIconScaleY;

      item.mSize = XMHALF2(sizeX, sizeY);
      pProperties->getHalf4(mCenterPointIconUVHandle, item.mUV);   
      item.mColor = gWorld->getPlayerColor(i, BWorld::cPlayerColorContextMinimap);
      item.mType = cMinimapItemTypeFocusPosition;
      item.mPriority = pProtoIcon->mPriority;
      if (mCenterPointUsesFlash)
         mIcons.add(item);
      else
         mStaticIcons.add(item);
   }
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashMinimap::addCenterPoint()
{
   ASSERT_MAIN_THREAD

   const BFlashProperties* pProperties = getProperties();
   XMCOLOR color = gWorld->getPlayerColor(gUserManager.getPrimaryUser()->getPlayerID(), BWorld::cPlayerColorContextMinimap);

   BVector unitPos = XMVectorSet(mMapViewHalfWidth, 0, mMapViewHalfHeight, 1);

   if (mbFullZoomOut)
   {
//-- FIXING PREFIX BUG ID 2608
      const BUser* pUser = gUserManager.getUser(BUserManager::cPrimaryUser);
//--
      BVector cameraPos = pUser->getHoverPoint();      
      computeIconPosition(cameraPos, cameraPos, unitPos);            
   }

   BFlashMinimapItem item;
   item.mPosition  = unitPos;
   item.mPosition2 = unitPos;

//-- FIXING PREFIX BUG ID 2609
   const BFlashProtoIcon* pProtoIcon = getProtoIcon(mCenterPointIconProtoID);
//--
   if (!pProtoIcon)
      return;

   float sizeX = XMConvertHalfToFloat(pProtoIcon->mSize.x) * mIconScaleX;
   float sizeY = XMConvertHalfToFloat(pProtoIcon->mSize.y) * mIconScaleY;

   item.mSize  = XMHALF2(sizeX, sizeY);
   pProperties->getHalf4(mCenterPointIconUVHandle, item.mUV);   
   item.mColor    = color;
   item.mType     = cMinimapItemTypeCenterPoint;
   item.mPriority = pProtoIcon->mPriority;

   if (mCenterPointUsesFlash)
      mIcons.add(item);
   else 
      mStaticIcons.add(item);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashMinimap::alert(BVector pos)
{
   ASSERT_MAIN_THREAD   
   const BFlashProperties* pProperties = getProperties();

   XMCOLOR color = cDWORDWhite;

   BVector unitPos;
   computeIconPosition(mCameraPos, pos, unitPos);

   BFlashMinimapItem item;
   item.mPosition  = unitPos;
   item.mPosition2 = pos;

   // Fixes Bug 15548
   float sizeX = XMConvertHalfToFloat(mAlertIconSize.x) * mIconScaleX;
   float sizeY = XMConvertHalfToFloat(mAlertIconSize.y) * mIconScaleY;
   item.mSize  = XMHALF2(sizeX, sizeY);

   pProperties->getHalf4(mAlertIconHandle, item.mUV);   
   item.mColor    = color;
   item.mType = cMinimapItemTypeAlert;
   item.mDeathTime = 0.0f;

   if (mAlertIconUsesFlash)
      mIcons.add(item);
   else
      mStaticIcons.add(item);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashMinimap::addAlert(BVector pos)
{
   ASSERT_MAIN_THREAD

   mQueuedAlerts.add(pos);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashMinimap::computeIconPosition(const BVector& camerPos, const BVector& position, BVector& minimapIconPosition)
{
   BVector  cameraPosOnWorld = camerPos; 
   BVector  curPos           = position;
   BVector  unitPos          = position; 

   //-- convert world coordinates to our map coordinates in 0..1 range
   float camU = (cameraPosOnWorld.x * mWorldToMapConversionFactor);
   float camV = 1.0f - (cameraPosOnWorld.z * mWorldToMapConversionFactor);

   //-- if we are forcing a full zoom out pretend that the camera is in the center of the map
   if (mbFullZoomOut)
   {
      camU = 0.5f;
      camV = 0.5f;
   }

   float objectU = (unitPos.x * mWorldToMapConversionFactor);
   float objectV = 1.0f - (unitPos.z * mWorldToMapConversionFactor);

   XMVECTOR objV = XMVectorSet(objectU, 0, objectV, 0);
   XMVECTOR cameraV = XMVectorSet(camU, 0, camV, 0);

   XMVECTOR offset = XMVectorSubtract(objV, cameraV);
   XMVECTOR length = XMVector3Length(offset);

   float  maxUVDistance = mZoom * 0.5f;   
   if (length.x > maxUVDistance)
   {
      float alpha =  maxUVDistance / length.x;
      offset = XMVectorLerp(XMVectorZero(), offset, alpha);
      objV   = XMVectorAdd(cameraV, offset);
      
      //trace("OldPos x=%4.3f z=%4.3f)  => NewPos(x=%4.3f z=%4.3f)  alpha=%4.3f offset(x=%4.3f y=%4.3f)", oldObj.x, oldObj.z, objV.x, objV.z, alpha, offset.x, offset.z);
   }

   offset.x /= maxUVDistance;
   offset.z /= maxUVDistance;

   unitPos.x = mMapViewHalfWidth + ((mMapViewHalfWidth-mMapViewIconStickyOffset) * offset.x);
   unitPos.z = mMapViewHalfHeight+ ((mMapViewHalfHeight-mMapViewIconStickyOffset) * offset.z);

   minimapIconPosition = unitPos;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashMinimap::addReveal(BVector pos, float lineOfSight)
{   
   BFlashMinimapVisibilityItem vis;
   vis.mPosition = pos;
   vis.mSize     = XMHALF2(lineOfSight * mVisibilityScale * 2.0f, 0.0f);
   mQueuedReveals.add(vis);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashMinimap::addReveal(BVector pos, float width, float height)
{
   BFlashMinimapVisibilityItem vis;
   vis.mPosition = pos;
   vis.mSize     = XMHALF2(width * mVisibilityScale, height * mVisibilityScale);
   mQueuedReveals.add(vis);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashMinimap::reveal(BVector pos, float lineOfSight)
{   
   BFlashMinimapVisibilityItem vis;
   vis.mPosition = pos;
   vis.mSize     = XMHALF2(lineOfSight * mVisibilityScale * 2.0f, 0.0f);
   mVisibility[mVisibilityIndex].add(vis);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashMinimap::reveal(BVector pos, float width, float height)
{
   BFlashMinimapVisibilityItem vis;
   vis.mPosition = pos;
   vis.mSize     = XMHALF2(width * mVisibilityScale, height * mVisibilityScale);
   mVisibility[mVisibilityIndex].add(vis);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashMinimap::reveal(BFlashMinimapVisibilityItem vis)
{
   mVisibility[mVisibilityIndex].add(vis);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashMinimap::block(BFlashMinimapVisibilityItem vis)
{
   mVisibilityBlockers.add(vis);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashMinimap::block(BVector pos, float lineOfSight)
{   
   BFlashMinimapVisibilityItem vis;
   vis.mPosition = pos;
   vis.mSize     = XMHALF2(lineOfSight * mVisibilityScale * 2.0f, 0.0f);
   mVisibilityBlockers.add(vis);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashMinimap::block(BVector pos, float width, float height)
{
   BFlashMinimapVisibilityItem vis;
   vis.mPosition = pos;
   vis.mSize     = XMHALF2(width * mVisibilityScale, height * mVisibilityScale);
   mVisibilityBlockers.add(vis);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashMinimap::addBlock(BVector pos, float lineOfSight)
{   
   BFlashMinimapVisibilityItem vis;
   vis.mPosition = pos;
   vis.mSize     = XMHALF2(lineOfSight * mVisibilityScale * 2.0f, 0.0f);
   mQueuedBlocks.add(vis);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashMinimap::addBlock(BVector pos, float width, float height)
{
   BFlashMinimapVisibilityItem vis;
   vis.mPosition = pos;
   vis.mSize     = XMHALF2(width * mVisibilityScale, height * mVisibilityScale);
   mQueuedBlocks.add(vis);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashMinimap::generateVisibility()
{
   ASSERT_MAIN_THREAD
   SCOPEDSAMPLE(BFlashMinimapGenerateVisibility);

   const int oldIndex = 1 - mVisibilityIndex;
   const int numVisibility = mVisibility[mVisibilityIndex].getNumber();
   const int numFog = mVisibility[oldIndex].getNumber();
   const int numTotal = numVisibility + numFog;
   const int numBlocker = mVisibilityBlockers.getNumber();

   if (numTotal <= 0)
      return;

   uchar* pFogFrameStorage = NULL;
   if (numFog)
   {
      uint bytes = numFog*sizeof(BFlashMinimapVisibilityItem);
      pFogFrameStorage = static_cast<uchar*> (gRenderThread.allocateFrameStorage(bytes));
      XMemCpy(pFogFrameStorage, mVisibility[oldIndex].getData(), bytes);
   }
         
   uchar* pVisibilityFrameStorage = NULL;
   if (numVisibility)
   {
      uint bytes = numVisibility*sizeof(BFlashMinimapVisibilityItem);
      pVisibilityFrameStorage = static_cast<uchar*> (gRenderThread.allocateFrameStorage(bytes));
      XMemCpy(pVisibilityFrameStorage, mVisibility[mVisibilityIndex].getData(), bytes);
   }

   uchar* pBlockerFrameStorage = NULL;
   if (numBlocker)
   {
      uint bytes = numBlocker*sizeof(BFlashMinimapVisibilityItem);
      pBlockerFrameStorage = static_cast<uchar*> (gRenderThread.allocateFrameStorage(bytes));
      XMemCpy(pBlockerFrameStorage, mVisibilityBlockers.getData(), bytes);
   }

   gFlashMinimapRenderer.submitGenerateVisibility(mVisibilityScaleMatrix, mVisMaskTextureHandle, numFog, pFogFrameStorage, numVisibility, pVisibilityFrameStorage, numBlocker, pBlockerFrameStorage);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashMinimap::releaseGPUHeapTextures()
{
   ASSERT_MAIN_THREAD
   if (!mpIconMovie)
         return;

   mpIconMovie->releaseGPUHeapTextures();
}
