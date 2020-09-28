//==============================================================================
// minimap.cpp
//
// The minimap manager/renderer.
//
// Copyright (c) 2006 Ensemble Studios
//==============================================================================

//==============================================================================
// Includes
//==============================================================================
#include "common.h"
#include "minimap.h"
#include "unit.h"
#include "color.h"
#include "renderDraw.h"
#include "TerrainSimRep.h"
#include "asyncFileManager.h"
#include "render.h"
#include "camera.h"
#include "config.h"
#include "configsgame.h"
#include "usermanager.h"
#include "user.h"
#include "mathutil.h"
#include "D3DTexture.h"
#include "game.h"
#include "uigame.h"
#include "image.h"
#include "protoobject.h"
#include "renderToTextureXbox.h"
#include "visiblemap.h"
#include "worldVisibility.h"
#include "world.h"
#include "alert.h"
#include "reloadManager.h"

//==============================================================================
// Defines
//==============================================================================
#define BMINIMAP_VISIBILITY_TEXTURE_SIZE  512
#define BMINIMAP_VISIBILITY_TEXTURE_MAX   (BMINIMAP_VISIBILITY_TEXTURE_SIZE - 1)
#define BMINIMAP_VISIBILITY_TEXTURE_FSIZE BMINIMAP_VISIBILITY_TEXTURE_SIZE.0f
#define BMINIMAP_VISIBILITY_TEXTURE_FMAX  (BMINIMAP_VISIBILITY_TEXTURE_FSIZE - 1.0f)

//==============================================================================
// Globals
//==============================================================================
//BMiniMap gMiniMap;

//==============================================================================
// Prototypes
//==============================================================================
void fillTexture(D3DXVECTOR4 *pOut, CONST D3DXVECTOR2 *pTexCoord, CONST D3DXVECTOR2 *pTexelSize, LPVOID pData);
void createCircleStencil(D3DXVECTOR4 *pOut, CONST D3DXVECTOR2 *pTexCoord, CONST D3DXVECTOR2 *pTexelSize, LPVOID pData);

//==============================================================================
// Constructors/Destructors
//==============================================================================
BMiniMap::BMiniMap() :
   BEventReceiver(),
   mVisibilityIndex(0),
   mX1(-1.0f),
   mY1(-1.0f),
   mX2(-1.0f),
   mY2(-1.0f),
   mBorderMinUV(0.0f),
   mBorderMaxUV(1.0f),
   mOpacity(0.5f),
   mX(-1.0f),
   mY(-1.0f),
   mWidth(-1.0f),
   mHeight(-1.0f),
   mHWidth(-1.0f),
   mHHeight(-1.0f),
   mRotationAngle(-1.0f),
   mScale(-1.0f),
   mBorderTextureHandle(cInvalidManagedTextureHandle),
   mIconTextureHandle(cInvalidManagedTextureHandle),
   mRadarSweepTextureHandle(cInvalidManagedTextureHandle),
   mOverlayTextureHandle(cInvalidManagedTextureHandle),
   mMapTextureHandle(cInvalidManagedTextureHandle),
   mBackgroundTexture(NULL),
   mVisibilityTexture(NULL),
   mCircleStencilTexture(NULL),
   mBPTVertexDecl(NULL),
   mBPSDVertexDecl(NULL),
   mBPDVertexDecl(NULL),
   mBPSVertexDecl(NULL),
   mBPSTDVertexDecl(NULL),
   mBackgroundColor(0.0f, 0.5f, 0.5f, 1.0f),
   mVisibilityColor(1.0f, 1.0f, 1.0f, 1.0f),
   mInitialized(false),
   mUseBorderTexture(false),
   mRadarRotation(0.0f),
   mbVisible(true)
{
   mScaleMatrix.makeZero();
   mVisibilityGeneratorScaleMatrix.makeZero();
   mRotationMatrix.makeZero();
   mPositionMatrix.makeZero();
}

BMiniMap::~BMiniMap()
{
}

//==============================================================================
// BMiniMap::gameInit
//==============================================================================
void BMiniMap::gameInit(void)
{
   // init event receiver and command listener
   eventReceiverInit(cThreadIndexRender);
   commandListenerInit();

   // load fx file
   reloadInit();
   loadEffect();
}

//==============================================================================
// BMiniMap::gameDeinit
//==============================================================================
void BMiniMap::gameDeinit(void)
{
   gRenderThread.blockUntilGPUIdle();         

   commandListenerDeinit();
   eventReceiverDeinit();
}

//==============================================================================
// Initializer/Deinitializer
//==============================================================================
void BMiniMap::init(float x1, float y1, float x2, float y2, float borderMinUV, float borderMaxUV, float opacity)
{
   SCOPEDSAMPLE(BMiniMap_init)
   ASSERT_MAIN_THREAD
   BDEBUG_ASSERT(gRenderThread.getInitialized());

   deinit();

   // Precalculate some constants
   mX1            = x1;
   mY1            = y1;
   mX2            = x2;
   mY2            = y2;
   mBorderMinUV   = borderMinUV;
   mBorderMaxUV   = borderMaxUV;
   mOpacity       = opacity;
   mX             = Math::Lerp(x1, x2, 0.5f);
   mY             = Math::Lerp(y1, y2, 0.5f);
   mWidth         = x2 - x1 + 1.0f;
   mHeight        = y2 - y1 + 1.0f;
   mHWidth        = mWidth * 0.5f;
   mHHeight       = mHeight * 0.5f;
   mRotationAngle = 0.0f;
   mScale         = BMINIMAP_VISIBILITY_TEXTURE_FSIZE / (gTerrainSimRep.getNumXDataTiles() * gTerrainSimRep.getDataTileScale());

   mUseBackgroundColor=gUIGame.getMinimapBackgroundColorActive();
   BVector color=gUIGame.getMinimapBackgroundColor();
   mBackgroundColor.x=color.x;
   mBackgroundColor.y=color.y;
   mBackgroundColor.z=color.z;

   color=gUIGame.getMinimapVisibilityColor();
   mVisibilityColor.x=color.x;
   mVisibilityColor.y=color.y;
   mVisibilityColor.z=color.z;

   BMatrix  preScaleMatrix;
   BMatrix  yTranslateMatrix;
   BMatrix  yFlipMatrix;

   // Precalculate visibility scale
   preScaleMatrix.makeScale(mScale, mScale, 1.0f);
   yTranslateMatrix.makeTranslate(0.0f, BMINIMAP_VISIBILITY_TEXTURE_FMAX, 0.0f);
   yFlipMatrix.makeScale(1.0f, -1.0f, 1.0f);
   yFlipMatrix.mult(yFlipMatrix, yTranslateMatrix);
   mVisibilityGeneratorScaleMatrix.mult(preScaleMatrix, yFlipMatrix);

   // Set scale matrix
   const float uScale = mWidth / (gTerrainSimRep.getNumXDataTiles() * gTerrainSimRep.getDataTileScale());
   preScaleMatrix.makeScale(uScale, uScale, 1.0f);
   yTranslateMatrix.makeTranslate(0.0f, mHeight - 1.0f, 0.0f);
   yFlipMatrix.makeScale(1.0f, -1.0f, 1.0f);
   yFlipMatrix.mult(yFlipMatrix, yTranslateMatrix);
   mScaleMatrix.mult(preScaleMatrix, yFlipMatrix);

   // Set position matrix
   mPositionMatrix.makeTranslate(mX1, mY1, 0.0f);

   // Set rotation matrix
   mRotationMatrix.makeIdentity();

   mRadarRotationMatrix.makeIdentity();

   // get the border texture
   mBorderTextureHandle = gUIGame.getMinimapBorderTextureHandle();
   mUseBorderTexture = gUIGame.getMinimapBorderActive();

   // get the icon texture
   mIconTextureHandle = gUIGame.getMinimapIconTextureHandle();

   mRadarSweepTextureHandle = gUIGame.getMinimapRadarTextureHandle(); 
   mOverlayTextureHandle = gUIGame.getMinimapOverlayTextureHandle();

   mFlareIcon = gUIGame.getMinimapIcon("Flare");

   bool success = mFlareArray.init(gUIGame.getMinimapNumFlares());
   success;
   BDEBUG_ASSERT(success);

   mInitialized = true;
}

//==============================================================================
//==============================================================================
void BMiniMap::deinit(void)
{
   ASSERT_MAIN_THREAD
   
   // Clear arrays
   mUnitArray.clear();
   mIconUnitArray.clear();
   mVisibilityArray[0].clear();
   mVisibilityArray[1].clear();
   mFlareArray.clear();
   mBlockerArray.clear();

   if (!mInitialized)
      return;

   mInitialized = false;
}

//==============================================================================
//==============================================================================
void BMiniMap::reloadInit(void)
{
   BReloadManager::BPathArray paths;
   BString effectFilename;
   eFileManagerError result = gFileManager.getDirListEntry(effectFilename, gRender.getEffectCompilerDefaultDirID());
   BVERIFY(cFME_SUCCESS == result);
   strPathAddBackSlash(effectFilename);
   effectFilename += "minimap\\minimap*.bin";
   paths.pushBack(effectFilename);

   gReloadManager.registerClient(paths, BReloadManager::cFlagSynchronous, mEventHandle, eEventClassReloadEffects);
}

//==============================================================================
//==============================================================================
void BMiniMap::reloadDeinit(void)
{
   gReloadManager.deregisterClient(mEventHandle);
}

//==============================================================================
// Reveal a part of the minimap
//==============================================================================
void BMiniMap::reveal(BVector position, float LOS)
{
   if (!mInitialized)
      return;
   BPSVertex   visibilityVertex;
   visibilityVertex.position = XMHALF2(position.x, position.z);
   visibilityVertex.size     = LOS * mScale * 2.0f;
   mVisibilityArray[mVisibilityIndex].pushBack(visibilityVertex);
}

//==============================================================================
// Reveal a part of the minimap
//==============================================================================
void BMiniMap::block(BVector position, float LOS)
{
   if (!mInitialized)
      return;
   BPSVertex   blockVertex;
   blockVertex.position = XMHALF2(position.x, position.z);
   blockVertex.size     = LOS * mScale * 2.0f;
   mBlockerArray.pushBack(blockVertex);
}

//==============================================================================
// Add a new unit to the system
//==============================================================================
void BMiniMap::addUnit(const BObject& unit)
{
   ASSERT_MAIN_THREAD

   if (!mInitialized)
      return;

   bool                 tagged = unit.getFlagSensorLockTagged();      
   const BProtoObject*  proto = unit.getProtoObject();   
   const BVector        position = unit.getPosition();
   const BPlayerID      playerID = unit.getPlayerID();
   // Halwes - 5/2/2007 - Use flare icon for sensor lock power until art provides a new icon
   long                 iconIndex=-1;
   if (tagged)
      iconIndex=gUIGame.getMinimapIconIndex("Flare");
   //DCP 07/16/07: Check for proto before using.
   else if (proto)
      iconIndex=gUIGame.getMinimapIconIndex(proto->getMiniMapIcon());
   XMHALF2              halfPosition = XMHALF2(position.x, position.z);
   // Halwes - 5/2/2007 - Assign yellow to sensor locked units for now
   //D3DCOLOR             color = (playerID == 0) ? packRGBA(proto->getMinimapColor(), 1.0f) : gWorld->getPlayerColor(unit.getPlayerID());
   D3DCOLOR             color ;
   if (playerID == 0)
   {
      color = packRGBA(proto->getMinimapColor(), 1.0f);
   }
   else if (tagged)
   {
      color = packRGBA(cColorYellow, 1.0f);
   }
   else
   {
      color = gWorld->getPlayerColor(unit.getColorPlayerID(), BWorld::cPlayerColorContextMinimap);
   }
       
   if (iconIndex != -1)
   {
      BMinimapIcon icon = gUIGame.getMinimapIcon(iconIndex);

      // Init new vertex
      BPSTDVertex          iconVertex;
      iconVertex.position  = halfPosition;
      // Halwes 5/2/2007 - Hard code icon size for sensor locked unit for now
      iconVertex.size      = XMHALF2(tagged ? gUIGame.getMinimapFlareBigSize() : proto->getMiniMapIconSize(), icon.mSize);
      iconVertex.UV        = XMHALF2(icon.mU, icon.mV);
      iconVertex.diffuse   = color;

      // Add it to the icon array
      mIconUnitArray.pushBack(iconVertex);
   }
   /*
   // Don't add units that don't have an icon specified
   else
   {
      // Init new vertex
      BPSDVertex           unitVertex;
      unitVertex.position  = halfPosition;
      unitVertex.size      = 1.0f;
      unitVertex.diffuse   = color;
   
      // Add it to the unit array
      mUnitArray.pushBack(unitVertex);
   }
   */

   //DCP 07/16/07: Check for proto first since getLOS just assumes it's valid.
   if ((gUserManager.getUser(BUserManager::cPrimaryUser)->getTeamID() == unit.getTeamID()) && proto)
   {
      // Add it to the visibility array
      BPSVertex            visibilityVertex;
      visibilityVertex.position = halfPosition;
      visibilityVertex.size     = unit.getLOS() * mScale * 2.0f;
      mVisibilityArray[mVisibilityIndex].pushBack(visibilityVertex);
   }
}

//==============================================================================
// Update the system
//==============================================================================
void BMiniMap::update(void)
{
   ASSERT_MAIN_THREAD

   if (!mInitialized)
      return;

   BVector  cameraDir;
   BCamera  *camera = gUserManager.getUser(BUserManager::cPrimaryUser)->getCamera();

   if (_fabs(camera->getCameraDir().y) < _fabs(camera->getCameraUp().y))
      cameraDir = BVector(camera->getCameraDir().x, 0.0f, camera->getCameraDir().z);
   else
      cameraDir = BVector(camera->getCameraUp().x, 0.0f, camera->getCameraUp().z);

   float theta = circleTan(cameraDir.x, cameraDir.z) - cPiOver2;

   if (theta < cTwoPi)
      theta = theta + cTwoPi;
   if (theta > cTwoPi)
      theta = theta - cTwoPi;

   if (theta != mRotationAngle)
   {
      mRotationAngle = theta;

      BMatrix  preRotationTransform;
      preRotationTransform.makeTranslate(-mHWidth, -mHHeight, 0.0f);

      BMatrix  rotationTransform;
      rotationTransform.makeRotateZ(mRotationAngle);

      BMatrix  postRotationTransform;
      postRotationTransform.makeTranslate(mHWidth, mHHeight, 0.0f);

      mRotationMatrix.mult(preRotationTransform, rotationTransform);
      mRotationMatrix.mult(mRotationMatrix, postRotationTransform);
   }

   mRadarRotation += 0.025f;
   if (mRadarRotation >= cTwoPi)
      mRadarRotation = 0.0f;

   BMatrix  preRadarTransform;
   preRadarTransform.makeTranslate(-mHWidth, -mHHeight, 0.0f);

   BMatrix  radarTransform;
   radarTransform.makeRotateZ(mRadarRotation);

   BMatrix  postRadarTransform;
   postRadarTransform.makeTranslate(mHWidth, mHHeight, 0.0f);

   mRadarRotationMatrix.mult(preRadarTransform, radarTransform);
   mRadarRotationMatrix.mult(mRadarRotationMatrix, postRadarTransform);
      
   updateFlares();
}

//==============================================================================
// Update flares
//==============================================================================
void BMiniMap::updateFlares(void)
{
   ASSERT_MAIN_THREAD

   const long numFlares = mFlareArray.getSize();
   for (long i = 0; i < numFlares; i++)
   {
      if (mFlareArray.isInUse(i))
      {
         BMiniMapFlare *flare = mFlareArray.get(i);
         if (flare->update())
         {
            BPSTDVertex           iconVertex;

            // Init new vertex
            iconVertex.position  = XMHALF2(flare->mPosition.x, flare->mPosition.z);
            iconVertex.size      = XMHALF2(flare->mSize, mFlareIcon.mSize);
            iconVertex.UV        = XMHALF2(mFlareIcon.mU, mFlareIcon.mV);
            iconVertex.diffuse   = flare->mColor;

            // Add it to the unit array
            mIconUnitArray.pushBack(iconVertex);
         }
         else
            mFlareArray.release(i);
      }
   }
}

//==============================================================================
// Prep minimap for next game update
//==============================================================================
void BMiniMap::prep(void)
{
   ASSERT_MAIN_THREAD

   if (!mInitialized)
      return;

   SCOPEDSAMPLE(MiniMapPrep);

   // Toggle index
   mVisibilityIndex = 1 - mVisibilityIndex;

   // Free up arrays
   mUnitArray.clear();
   mIconUnitArray.clear();
   mVisibilityArray[mVisibilityIndex].clear();
   mBlockerArray.clear();
}

//==============================================================================
// Render the minimap
//==============================================================================
void BMiniMap::render(void)
{   
   ASSERT_MAIN_THREAD

   if (!mInitialized)
      return;

   if (!mbVisible)
      return;

   bool blocked = gUserManager.getPrimaryUser()->getPlayer()->getFlagMinimapBlocked();

   renderBackground();

   if (!blocked)
   {
      if (!gConfig.isDefined(cConfigNoFogMask) && !(gWorld->getFlagAllVisible() && gWorld->getFlagShowSkirt()))
         renderVisibility();
      renderUnits();
      renderIcons();  
   }

   renderOverlay();
   renderBorder();

   if (!blocked)
      renderRadarSweep();
}

//==============================================================================
// Render background
//==============================================================================
void BMiniMap::renderBackground(void)
{
   ASSERT_MAIN_THREAD
   if (mBPTVertexDecl)
   {
      // Begin packet
      BDrawBackgroundPacket *packet = reinterpret_cast<BDrawBackgroundPacket *>(gRenderThread.submitCommandBegin(mCommandHandle, cMMCommandDrawBackground, sizeof(BDrawBackgroundPacket)));
         // Set packet
         mPositionMatrix.getD3DXMatrix(packet->worldMatrix);
      // Send packet
      gRenderThread.submitCommandEnd(sizeof(BDrawBackgroundPacket));
   }
}

//==============================================================================
// Render visibility
//==============================================================================
void BMiniMap::generateVisibility(void)
{
   ASSERT_MAIN_THREAD
   
   SCOPEDSAMPLE(BMiniMapGenVisibility);

   const long  oldIndex = 1 - mVisibilityIndex;
   const long  numVisibility = mVisibilityArray[mVisibilityIndex].size();
   const long  numFog = mVisibilityArray[oldIndex].size();
   const long  numTotal = numVisibility + numFog;
   const long  numBlocker = mBlockerArray.size();

   if (numTotal && mBPSVertexDecl)
   {
      BPSVertex  *blockerVB;
      BPSVertex  *visibilityVB;
      BPSVertex  *fogVB;

      // Copy the vertex lists to dynamic VBs
      if (numFog)
      {
         fogVB = reinterpret_cast<BPSVertex *>(gRenderDraw.lockDynamicVB(numFog, sizeof(BPSVertex)));
            memcpy(fogVB, mVisibilityArray[oldIndex].getData(), numFog * sizeof(BPSVertex));
         gRenderDraw.unlockDynamicVB();
      }
      else
      {
         fogVB = NULL;
      }

      if (numVisibility)
      {
         visibilityVB = reinterpret_cast<BPSVertex *>(gRenderDraw.lockDynamicVB(numVisibility, sizeof(BPSVertex)));
            memcpy(visibilityVB, mVisibilityArray[mVisibilityIndex].getData(), numVisibility * sizeof(BPSVertex));
         gRenderDraw.unlockDynamicVB();
      }
      else
      {
         visibilityVB = NULL;
      }

      if (numBlocker)
      {
         blockerVB = reinterpret_cast<BPSVertex *>(gRenderDraw.lockDynamicVB(numBlocker, sizeof(BPSVertex)));
            memcpy(blockerVB, mBlockerArray.getData(), numBlocker * sizeof(BPSVertex));
         gRenderDraw.unlockDynamicVB();
      }
      else
      {
         blockerVB = NULL;
      }

      // Begin packet
      BGenerateVisibilityPacket *packet = reinterpret_cast<BGenerateVisibilityPacket *>(gRenderThread.submitCommandBegin(mCommandHandle, cMMCommandGenerateVisibility, sizeof(BGenerateVisibilityPacket)));
         // Set packet
         packet->numVisibility   = numVisibility;
         packet->visibilityVB    = visibilityVB;
         packet->numFog          = numFog;
         packet->fogVB           = fogVB;
         packet->numBlocker      = numBlocker;
         packet->blockerVB       = blockerVB;
         mVisibilityGeneratorScaleMatrix.getD3DXMatrix(packet->worldMatrix);
      // Send packet
      gRenderThread.submitCommandEnd(sizeof(BGenerateVisibilityPacket));
   }
}

//==============================================================================
// Render visibility
//==============================================================================
void BMiniMap::renderVisibility(void)
{
   ASSERT_MAIN_THREAD

   if (mBPSVertexDecl)
   {
      BMatrix  worldMatrix;

      worldMatrix.mult(mRotationMatrix, mPositionMatrix);

      // Begin packet
      BDrawVisibilityPacket *packet = reinterpret_cast<BDrawVisibilityPacket *>(gRenderThread.submitCommandBegin(mCommandHandle, cMMCommandDrawVisibility, sizeof(BDrawVisibilityPacket)));
         // Set packet
         worldMatrix.getD3DXMatrix(packet->worldMatrix);
      // Send packet
      gRenderThread.submitCommandEnd(sizeof(BDrawVisibilityPacket));
   }
}

//==============================================================================
// Render border
//==============================================================================
void BMiniMap::renderBorder(void)
{
   ASSERT_MAIN_THREAD

   if (mBPTVertexDecl)
   {
      // Generate view rect
      const BColor   viewRectColor(1.0f, 0.0f, 0.0f);
      BMatrix  viewMatrix;
      BVector  viewVec[4];
      BVector  viewRect[4];
      const BCamera  *camera = gUserManager.getUser(BUserManager::cPrimaryUser)->getCamera();
      const BVector  cameraPos = camera->getCameraLoc();      
      camera->getViewMatrix(viewMatrix);

      D3DXMATRIX  d3dViewMatrix;
      viewMatrix.getD3DXMatrix(d3dViewMatrix);
      float fov = camera->getFOV();
      const BRenderViewParams &params = gRender.getViewParams();
      const float viewportWidth = float(params.getViewportWidth() - 1);
      const float viewportHeight = float(params.getViewportHeight() - 1);         


      params.calculateWorldRay(0.0f, 0.0f, d3dViewMatrix, fov, viewVec[0]);
      params.calculateWorldRay(viewportWidth, 0.0f, d3dViewMatrix, fov, viewVec[1]);
      params.calculateWorldRay(viewportWidth, viewportHeight, d3dViewMatrix, fov, viewVec[2]);
      params.calculateWorldRay(0.0f, viewportHeight, d3dViewMatrix, fov, viewVec[3]);

      bool success = true;


      success &= cameraPos.projectYPlane(viewVec[0], viewRect[0], 0.0f);
      success &= cameraPos.projectYPlane(viewVec[1], viewRect[1], 0.0f);
      success &= cameraPos.projectYPlane(viewVec[2], viewRect[2], 0.0f);
      success &= cameraPos.projectYPlane(viewVec[3], viewRect[3], 0.0f);

      BMatrix  viewRectMatrix;
      viewRectMatrix.mult(mScaleMatrix, mRotationMatrix);
      viewRectMatrix.mult(viewRectMatrix, mPositionMatrix);

      BPDVertex   *viewRectVB = reinterpret_cast<BPDVertex *>(gRenderDraw.lockDynamicVB(5, sizeof(BPDVertex)));
         viewRectVB[0].position = XMHALF2(viewRect[0].x, viewRect[0].z);
         viewRectVB[0].diffuse = packRGBA(viewRectColor, 0.5f);
         viewRectVB[1].position = XMHALF2(viewRect[1].x, viewRect[1].z);
         viewRectVB[1].diffuse = packRGBA(viewRectColor, 0.5f);
         viewRectVB[2].position = XMHALF2(viewRect[2].x, viewRect[2].z);
         viewRectVB[2].diffuse = packRGBA(viewRectColor, 0.8f);
         viewRectVB[3].position = XMHALF2(viewRect[3].x, viewRect[3].z);
         viewRectVB[3].diffuse = packRGBA(viewRectColor, 0.8f);
         viewRectVB[4].position = XMHALF2(viewRect[0].x, viewRect[0].z);
         viewRectVB[4].diffuse = packRGBA(viewRectColor, 0.5f);
      gRenderDraw.unlockDynamicVB();

      // Begin packet
      BDrawBorderPacket *packet = reinterpret_cast<BDrawBorderPacket *>(gRenderThread.submitCommandBegin(mCommandHandle, cMMCommandDrawBorder, sizeof(BDrawBorderPacket)));
         // Set packet
         packet->drawViewRect = success;
         viewRectMatrix.getD3DXMatrix(packet->viewRectMatrix);
         packet->viewRectVB = viewRectVB;
         mPositionMatrix.getD3DXMatrix(packet->worldMatrix);
      // Send packet
      gRenderThread.submitCommandEnd(sizeof(BDrawBorderPacket));
   }
}

//==============================================================================
// Render Radar Sweep
//==============================================================================
void BMiniMap::renderRadarSweep(void)
{
   ASSERT_MAIN_THREAD
   if (mBPTVertexDecl == NULL)
      return;
            
   // Begin packet
   BDrawBorderPacket *packet = reinterpret_cast<BDrawBorderPacket *>(gRenderThread.submitCommandBegin(mCommandHandle, cMMCommandDrawRadar, sizeof(BDrawBorderPacket)));
   // Set packet
   packet->drawViewRect = false;      
   BMatrix mtx;
   mtx.mult(mRadarRotationMatrix, mPositionMatrix);
   //mPositionMatrix.getD3DXMatrix(packet->worldMatrix);
   mtx.getD3DXMatrix(packet->worldMatrix);
   // Send packet
   gRenderThread.submitCommandEnd(sizeof(BDrawBorderPacket));
}

//==============================================================================
// render Overlay
//==============================================================================
void BMiniMap::renderOverlay(void)
{
   int xOffset = 10;
   int yOffset = 10;
   gUI.renderTexture(mOverlayTextureHandle, (long) mX1-xOffset, (long) mY1-yOffset, (long) mX2+xOffset, (long) mY2+yOffset, true, cDWORDWhite, false);
}

//==============================================================================
// Render units
//==============================================================================
void BMiniMap::renderUnits(void)
{
   ASSERT_MAIN_THREAD

   long  numUnits = mUnitArray.size();

   if (numUnits && mBPSDVertexDecl)
   {
      BMatrix  worldMatrix;
      worldMatrix.mult(mScaleMatrix, mRotationMatrix);
      worldMatrix.mult(worldMatrix, mPositionMatrix);

      // Copy the vertex list to a dynamic vb
      BPSDVertex  *unitVB = reinterpret_cast<BPSDVertex *>(gRenderDraw.lockDynamicVB(numUnits, sizeof(BPSDVertex)));
         memcpy(unitVB, mUnitArray.getData(), numUnits * sizeof(BPSDVertex));
      gRenderDraw.unlockDynamicVB();

      // Begin packet
      BDrawUnitsPacket *packet = reinterpret_cast<BDrawUnitsPacket *>(gRenderThread.submitCommandBegin(mCommandHandle, cMMCommandDrawUnits, sizeof(BDrawUnitsPacket)));
         // Set packet
         packet->numUnits  = numUnits;
         worldMatrix.getD3DXMatrix(packet->worldMatrix);
         packet->unitVB    = unitVB;
      // Send packet
      gRenderThread.submitCommandEnd(sizeof(BDrawUnitsPacket));
   }
}

//==============================================================================
// Render icons
//==============================================================================
void BMiniMap::renderIcons(void)
{
   ASSERT_MAIN_THREAD

   long  numIcons = mIconUnitArray.size();

   if (numIcons && mBPSTDVertexDecl)
   {
      BMatrix  worldMatrix;
      worldMatrix.mult(mScaleMatrix, mRotationMatrix);
      worldMatrix.mult(worldMatrix, mPositionMatrix);

      // Copy the vertex list to a dynamic vb
      BPSTDVertex  *iconVB = reinterpret_cast<BPSTDVertex *>(gRenderDraw.lockDynamicVB(numIcons, sizeof(BPSTDVertex)));
         memcpy(iconVB, mIconUnitArray.getData(), numIcons * sizeof(BPSTDVertex));
      gRenderDraw.unlockDynamicVB();

      // Begin packet
      BDrawIconsPacket *packet = reinterpret_cast<BDrawIconsPacket *>(gRenderThread.submitCommandBegin(mCommandHandle, cMMCommandDrawIcons, sizeof(BDrawIconsPacket)));
         // Set packet
         packet->numIcons  = numIcons;
         worldMatrix.getD3DXMatrix(packet->worldMatrix);
         packet->iconVB    = iconVB;
      // Send packet
      gRenderThread.submitCommandEnd(sizeof(BDrawIconsPacket));
   }
}

//==============================================================================
// Reset the visibility texture
//==============================================================================
void BMiniMap::reset(void)
{
   ASSERT_MAIN_THREAD

   if (!mInitialized)
      return;

   // Reset some arrays
   mUnitArray.clear();
   mIconUnitArray.clear();
   mVisibilityArray[0].clear();
   mVisibilityArray[1].clear();
   mBlockerArray.clear();

   // Send a command to clear the visibility texture
   gRenderThread.submitCommand(mCommandHandle, cMMCommandReset);
}

//==============================================================================
// Draw the border
//==============================================================================
void BMiniMap::workerDrawBorder(const BDrawBorderPacket *borderData)
{
   ASSERT_RENDER_THREAD

   if (borderData->drawViewRect)
   {
      // Set params
      BD3D::mpDev->SetVertexDeclaration(mBPDVertexDecl);

      // Draw
      mColoredBlendedSpriteTechnique.beginRestoreDefaultState();
         mColoredBlendedSpriteTechnique.beginPass();
            mColorScaleParam = D3DXVECTOR4(1.0f, 1.0f, 1.0f, mOpacity);
            mTransformParam = borderData->viewRectMatrix;
            mColoredBlendedSpriteTechnique.commit();
            BD3D::mpDev->DrawVerticesUP(D3DPT_LINESTRIP, 5, borderData->viewRectVB, sizeof(BPDVertex));
         mColoredBlendedSpriteTechnique.endPass();
      mColoredBlendedSpriteTechnique.end();
   }

   if (mUseBorderTexture && mBorderTextureHandle!=cInvalidManagedTextureHandle)
   {
      BD3DTextureManager::BManagedTexture *borderTexture = gD3DTextureManager.getManagedTextureByHandle(mBorderTextureHandle);
      if (borderTexture->getStatus() == BD3DTextureManager::BManagedTexture::cStatusLoaded)
      {
         BPTVertex   *VB;

         // Set params
         BD3D::mpDev->SetVertexDeclaration(mBPTVertexDecl);

         // Draw
         mTexturedBlendedSpriteTechnique.beginRestoreDefaultState();
            mTexturedBlendedSpriteTechnique.beginPass();

               mColorScaleParam = D3DXVECTOR4(1.0f, 1.0f, 1.0f, 1.0f);
               mTransformParam = borderData->worldMatrix;
               mLinearTextureParam = borderTexture->getD3DTexture().getBaseTexture();
               mTexturedBlendedSpriteTechnique.commit();
               BD3D::mpDev->BeginVertices(D3DPT_TRIANGLESTRIP, 4, sizeof(BPTVertex), (void **) &VB);
                  VB->position   = XMHALF2(0.0f, 0.0f);
                  VB->uv         = XMHALF2(mBorderMinUV, mBorderMinUV);
                  VB++;
                  VB->position   = XMHALF2(mWidth, 0.0f);
                  VB->uv         = XMHALF2(mBorderMaxUV, mBorderMinUV);
                  VB++;
                  VB->position   = XMHALF2(0.0f, mHeight);
                  VB->uv         = XMHALF2(mBorderMinUV, mBorderMaxUV);
                  VB++;
                  VB->position   = XMHALF2(mWidth, mHeight);
                  VB->uv         = XMHALF2(mBorderMaxUV, mBorderMaxUV);
               BD3D::mpDev->EndVertices();

            mTexturedBlendedSpriteTechnique.endPass();
         mTexturedBlendedSpriteTechnique.end();
      }
   }

   // Reset a few things
   BD3D::mpDev->SetVertexDeclaration(NULL);
}

//==============================================================================
// Draw the border
//==============================================================================
void BMiniMap::workerDrawRadar(const BDrawBorderPacket *borderData)
{
   ASSERT_RENDER_THREAD

   if (mRadarSweepTextureHandle==cInvalidManagedTextureHandle)
      return;

   BD3DTextureManager::BManagedTexture *pTexture = gD3DTextureManager.getManagedTextureByHandle(mRadarSweepTextureHandle);
   if (pTexture->getStatus() != BD3DTextureManager::BManagedTexture::cStatusLoaded)
      return; 

   BPTVertex   *VB;
   // Set params
   BD3D::mpDev->SetVertexDeclaration(mBPTVertexDecl);

   // Draw
   mTexturedBlendedSpriteTechnique.beginRestoreDefaultState();
   mTexturedBlendedSpriteTechnique.beginPass();

   float xOffset = 0.0f;
   float yOffset = 0.0f;
   float width  = mWidth + xOffset;
   float height = mHeight+ yOffset;

   mColorScaleParam = D3DXVECTOR4(1.0f, 1.0f, 1.0f, 1.0f);
   mTransformParam = borderData->worldMatrix;
   mLinearTextureParam = pTexture->getD3DTexture().getBaseTexture();
   mTexturedBlendedSpriteTechnique.commit();
   BD3D::mpDev->BeginVertices(D3DPT_TRIANGLESTRIP, 4, sizeof(BPTVertex), (void **) &VB);
   VB->position   = XMHALF2(0.0f, 0.0f);
   VB->uv         = XMHALF2(mBorderMinUV, mBorderMinUV);
   VB++;
   VB->position   = XMHALF2(width, 0.0f);
   VB->uv         = XMHALF2(mBorderMaxUV, mBorderMinUV);
   VB++;
   VB->position   = XMHALF2(0.0f, height);
   VB->uv         = XMHALF2(mBorderMinUV, mBorderMaxUV);
   VB++;
   VB->position   = XMHALF2(width, height);
   VB->uv         = XMHALF2(mBorderMaxUV, mBorderMaxUV);
   BD3D::mpDev->EndVertices();

   mTexturedBlendedSpriteTechnique.endPass();
   mTexturedBlendedSpriteTechnique.end();

   // Reset a few things
   BD3D::mpDev->SetVertexDeclaration(NULL);
}

//==============================================================================
// Draw the units
//==============================================================================
void BMiniMap::workerDrawUnits(const BDrawUnitsPacket *unitData)
{
   ASSERT_RENDER_THREAD

   // Set params
   mColorScaleParam = D3DXVECTOR4(1.0f, 1.0f, 1.0f, 1.0f /*mOpacity*/);
   mTransformParam = unitData->worldMatrix;
   BD3D::mpDev->SetVertexDeclaration(mBPSDVertexDecl);

   // Draw
   mColoredBlendedPointSpriteTechnique.beginRestoreDefaultState();
      mColoredBlendedPointSpriteTechnique.beginPass();
         mColoredBlendedPointSpriteTechnique.commit();
         BD3D::mpDev->DrawVerticesUP(D3DPT_POINTLIST, unitData->numUnits, unitData->unitVB, sizeof(BPSDVertex));
      mColoredBlendedPointSpriteTechnique.endPass();
   mColoredBlendedPointSpriteTechnique.end();

   // Reset a few things
   BD3D::mpDev->SetVertexDeclaration(NULL);
}

//==============================================================================
// Draw the icons
//==============================================================================
void BMiniMap::workerDrawIcons(const BDrawIconsPacket *iconData)
{
   ASSERT_RENDER_THREAD

   if (mIconTextureHandle==cInvalidManagedTextureHandle)
      return;

   BD3DTextureManager::BManagedTexture *iconTexture = gD3DTextureManager.getManagedTextureByHandle(mIconTextureHandle);

   if (iconTexture->getStatus() == BD3DTextureManager::BManagedTexture::cStatusLoaded)
   {
      // Set params
      mTextureAtlasScaleParam = 1.0f / (float) iconTexture->getWidth();
      mColorScaleParam = D3DXVECTOR4(1.0f, 1.0f, 1.0f, 1.0f /*mOpacity*/);
      mTransformParam = iconData->worldMatrix;
      mLinearTextureParam = iconTexture->getD3DTexture().getBaseTexture();
      BD3D::mpDev->SetVertexDeclaration(mBPSTDVertexDecl);

      // Draw
      mTexturedBlendedColoredPointSpriteTechnique.beginRestoreDefaultState();
         mTexturedBlendedColoredPointSpriteTechnique.beginPass();
            mTexturedBlendedColoredPointSpriteTechnique.commit();
            BD3D::mpDev->DrawVerticesUP(D3DPT_POINTLIST, iconData->numIcons, iconData->iconVB, sizeof(BPSTDVertex));
         mTexturedBlendedColoredPointSpriteTechnique.endPass();
      mTexturedBlendedColoredPointSpriteTechnique.end();

      // Reset a few things
      BD3D::mpDev->SetVertexDeclaration(NULL);
   }
}

//==============================================================================
// Draw the background
//==============================================================================
void BMiniMap::workerDrawBackground(const BDrawBackgroundPacket *borderData)
{
   ASSERT_RENDER_THREAD

   if(mUseBackgroundColor)
   {
      BPSVertex   *VB;

      // Set params
      mColorScaleParam = D3DXVECTOR4(1.0f, 1.0f, 1.0f, mOpacity);
      BD3D::mpDev->SetVertexDeclaration(mBPSVertexDecl);

      // Draw
      mTexturedBlendedPointSpriteTechnique.beginRestoreDefaultState();
         mTexturedBlendedPointSpriteTechnique.beginPass();

            mTransformParam = borderData->worldMatrix;
            mLinearTextureParam = mBackgroundTexture;
            mTexturedBlendedPointSpriteTechnique.commit();
            BD3D::mpDev->BeginVertices(D3DPT_POINTLIST, 1, sizeof(BPSVertex), (void **) &VB);
               VB->position   = XMHALF2(mHWidth, mHHeight);
               VB->size       = mWidth;
            BD3D::mpDev->EndVertices();

         mTexturedBlendedPointSpriteTechnique.endPass();
      mTexturedBlendedPointSpriteTechnique.end();
   }

   // Reset a few things
   BD3D::mpDev->SetVertexDeclaration(NULL);
}

//==============================================================================
// Draw the visibility
//==============================================================================
void BMiniMap::workerDrawVisibility(const BDrawVisibilityPacket *visibilityData)
{
   ASSERT_RENDER_THREAD
      
   BPTVertex   *VB;

   // Set params
   BD3D::mpDev->SetVertexDeclaration(mBPTVertexDecl);

   // Draw
   mDrawVisibilityTechnique.beginRestoreDefaultState();
      mDrawVisibilityTechnique.beginPass();

         mColorScaleParam = D3DXVECTOR4(0.0f, 0.0f, 0.0f, mOpacity);
         mTransformParam = visibilityData->worldMatrix;
#if defined(BVISIBLEMAP_DEBUG)
         mLinearTextureParam = gVisibleMap.getTexture();
#else
         mLinearTextureParam = mVisibilityTexture;
#endif
         mLinearStencilParam = mCircleStencilTexture;
         mDrawVisibilityTechnique.commit();
         BD3D::mpDev->BeginVertices(D3DPT_TRIANGLESTRIP, 4, sizeof(BPTVertex), (void **) &VB);
            VB->position   = XMHALF2(0.0f, 0.0f);
            VB->uv         = XMHALF2(0.0f, 0.0f);
            VB++;
            VB->position   = XMHALF2(mWidth, 0.0f);
            VB->uv         = XMHALF2(1.0f, 0.0f);
            VB++;
            VB->position   = XMHALF2(0.0f, mHeight);
            VB->uv         = XMHALF2(0.0f, 1.0f);
            VB++;
            VB->position   = XMHALF2(mWidth, mHeight);
            VB->uv         = XMHALF2(1.0f, 1.0f);
         BD3D::mpDev->EndVertices();

      mDrawVisibilityTechnique.endPass();
   mDrawVisibilityTechnique.end();

   // Reset a few things
   BD3D::mpDev->SetVertexDeclaration(NULL);
}

//==============================================================================
// Generate the visibility
//==============================================================================
void BMiniMap::workerGenerateVisibility(const BGenerateVisibilityPacket *visibilityData)
{
   ASSERT_RENDER_THREAD
   
   SCOPEDSAMPLE(MiniMapGenerateVisibility);

   BPSVertex                  *VB;
   BMatrix                    ident;
   BRenderToTextureHelperXbox renderTarget(BMINIMAP_VISIBILITY_TEXTURE_SIZE, BMINIMAP_VISIBILITY_TEXTURE_SIZE, D3DFMT_A8R8G8B8);

   ident.makeIdentity();

   // Set params
   BD3D::mpDev->SetVertexDeclaration(mBPSVertexDecl);

   // Draw
   renderTarget.createDeviceObjects();
   renderTarget.begin(mVisibilityTexture);
      mVisibilityGeneratorTechnique.beginRestoreDefaultState();

         mVisibilityGeneratorTechnique.beginPass(0);
            mLinearTextureParam = mVisibilityTexture;
            mTransformParam = ident;
            mColorScaleParam = D3DXVECTOR4(1.0f, 1.0f, 1.0f, 1.0f);
            mVisibilityGeneratorTechnique.commit();
            BD3D::mpDev->BeginVertices(D3DPT_POINTLIST, 1, sizeof(BPSVertex), (void **) &VB);
               VB->position   = XMHALF2(BMINIMAP_VISIBILITY_TEXTURE_FMAX * 0.5f, BMINIMAP_VISIBILITY_TEXTURE_FMAX * 0.5f);
               VB->size       = BMINIMAP_VISIBILITY_TEXTURE_FSIZE;
            BD3D::mpDev->EndVertices();
         mVisibilityGeneratorTechnique.endPass();

         mVisibilityGeneratorTechnique.beginPass(1);
            mPointTextureParam = mCircleStencilTexture;
            mTransformParam = visibilityData->worldMatrix;
            if (visibilityData->numFog)
            {
               mColorScaleParam = D3DXVECTOR4(1.0f, 1.0f, 1.0f, 0.5f);
               mVisibilityGeneratorTechnique.commit();
               BD3D::mpDev->DrawVerticesUP(D3DPT_POINTLIST, visibilityData->numFog, visibilityData->fogVB, sizeof(BPSVertex));
            }

            if (visibilityData->numVisibility)
            {
               mColorScaleParam = D3DXVECTOR4(1.0f, 1.0f, 1.0f, 1.0f / 255.0f);
               mVisibilityGeneratorTechnique.commit();
               BD3D::mpDev->DrawVerticesUP(D3DPT_POINTLIST, visibilityData->numVisibility, visibilityData->visibilityVB, sizeof(BPSVertex));
            }
         mVisibilityGeneratorTechnique.endPass();

         if (visibilityData->numBlocker)
         {
            mVisibilityGeneratorTechnique.beginPass(2);
               mColorScaleParam = D3DXVECTOR4(1.0f, 1.0f, 1.0f, 0.5f);
               mVisibilityGeneratorTechnique.commit();
               BD3D::mpDev->DrawVerticesUP(D3DPT_POINTLIST, visibilityData->numBlocker, visibilityData->blockerVB, sizeof(BPSVertex));
            mVisibilityGeneratorTechnique.endPass();
         }

      mVisibilityGeneratorTechnique.end();
      renderTarget.resolve(mVisibilityTexture);
   renderTarget.end();

   // Reset a few things
   BD3D::mpDev->SetVertexDeclaration(NULL);
}

//==============================================================================
// Process a worker thread command
//==============================================================================
void BMiniMap::processCommand(const BRenderCommandHeader &header, const uchar *pData)
{
   ASSERT_RENDER_THREAD

   switch (header.mType)
   {
      case cMMCommandDrawBackground:
      {
         BDEBUG_ASSERT(header.mLen == sizeof(BDrawBackgroundPacket));
         workerDrawBackground(reinterpret_cast<const BDrawBackgroundPacket *>(pData));
         break;
      }

      case cMMCommandDrawUnits:
      {
         BDEBUG_ASSERT(header.mLen == sizeof(BDrawUnitsPacket));
         workerDrawUnits(reinterpret_cast<const BDrawUnitsPacket *>(pData));
         break;
      }

      case cMMCommandDrawIcons:
      {
         BDEBUG_ASSERT(header.mLen == sizeof(BDrawIconsPacket));
         workerDrawIcons(reinterpret_cast<const BDrawIconsPacket *>(pData));
         break;
      }

      case cMMCommandGenerateVisibility:
      {
         BDEBUG_ASSERT(header.mLen == sizeof(BGenerateVisibilityPacket));
         workerGenerateVisibility(reinterpret_cast<const BGenerateVisibilityPacket *>(pData));
         break;
      }

      case cMMCommandDrawBorder:
      {
         BDEBUG_ASSERT(header.mLen == sizeof(BDrawBorderPacket));
         workerDrawBorder(reinterpret_cast<const BDrawBorderPacket *>(pData));
         break;
      }

      case cMMCommandDrawRadar:
         {
            BDEBUG_ASSERT(header.mLen == sizeof(BDrawBorderPacket));
            workerDrawRadar(reinterpret_cast<const BDrawBorderPacket *>(pData));
            break;
         }

      case cMMCommandDrawVisibility:
      {
         BDEBUG_ASSERT(header.mLen == sizeof(BDrawVisibilityPacket));
         workerDrawVisibility(reinterpret_cast<const BDrawVisibilityPacket *>(pData));
         break;
      }

      case cMMCommandReset:
      {
         D3DXFillTexture(mVisibilityTexture, fillTexture, &mVisibilityColor);
         break;
      }

      case cMMCommandApplyVisibilityTexture:
      {
         //BWorldVisibility::getInstance().setVisibilityTexture(mVisibilityTexture, BMINIMAP_VISIBILITY_TEXTURE_SIZE);
         break;
      }
   };
}

//==============================================================================
// Process frameBegin
//==============================================================================
void BMiniMap::frameBegin(void)
{
   ASSERT_RENDER_THREAD
}

//==============================================================================
// Process frameEnd
//==============================================================================
void BMiniMap::frameEnd(void)
{
   ASSERT_RENDER_THREAD
}

//==============================================================================
// Process deinitDeviceData
//==============================================================================
void BMiniMap::deinitDeviceData()
{
   ASSERT_RENDER_THREAD

   if (mCircleStencilTexture)
   {
      mCircleStencilTexture->Release();
      mCircleStencilTexture = NULL;
   }

   if (mVisibilityTexture)
   {
      //BWorldVisibility::getInstance().setVisibilityTexture(NULL, BMINIMAP_VISIBILITY_TEXTURE_SIZE);

      mVisibilityTexture->Release();
      mVisibilityTexture = NULL;
   }

   if (mBackgroundTexture)
   {
      mBackgroundTexture->Release();
      mBackgroundTexture = NULL;
   }
}

//==============================================================================
// Process initDeviceData
//==============================================================================
void BMiniMap::initDeviceData()
{
   ASSERT_RENDER_THREAD

  // create background texture
   HRESULT hres = gRenderDraw.createTexture(256, 256, 1, 0, D3DFMT_A1R5G5B5, D3DPOOL_DEFAULT, &mBackgroundTexture, NULL);
   if (FAILED(hres))
   {
      BFATAL_FAIL("BMiniMap::initDeviceData: createTexture() failed");
   }
   D3DXFillTexture(mBackgroundTexture, createCircleStencil, &mBackgroundColor);

   // create visibility texture
   hres = gRenderDraw.createTexture(BMINIMAP_VISIBILITY_TEXTURE_SIZE, BMINIMAP_VISIBILITY_TEXTURE_SIZE, 1, 0, D3DFMT_A8, D3DPOOL_DEFAULT, &mVisibilityTexture, NULL);
   if (FAILED(hres))
   {
      BFATAL_FAIL("BMiniMap::initDeviceData: createTexture() failed");
   }
   D3DXFillTexture(mVisibilityTexture, fillTexture, &mVisibilityColor);

   // create circle stencil texture
   hres = gRenderDraw.createTexture(128, 128, 1, 0, D3DFMT_A8, D3DPOOL_DEFAULT, &mCircleStencilTexture, NULL);
   if (FAILED(hres))
   {
      BFATAL_FAIL("BMiniMap::initDeviceData: createTexture() failed");
   }
   D3DXFillTexture(mCircleStencilTexture, createCircleStencil, &mVisibilityColor);

   //BWorldVisibility::getInstance().setVisibilityTexture(mVisibilityTexture, BMINIMAP_VISIBILITY_TEXTURE_SIZE);
}

//==============================================================================
// Process received events
//==============================================================================
bool BMiniMap::receiveEvent(const BEvent &event, BThreadIndex threadIndex)
{
   switch (event.mEventClass)
   {
      case cEventClassAsyncFile:
      {
         ASSERT_RENDER_THREAD
         setupEffect(event);
         break;
      }

      case cEventClassClientRemove:
      {
         ASSERT_RENDER_THREAD
         killEffect();

         break;
      }

      case eEventClassReloadEffects:
      {
         loadEffect();
         break;
      }
   }

   return true;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BMiniMap::loadEffect()
{
   ASSERT_MAIN_OR_WORKER_THREAD

   BAsyncFileManager::BRequestPacket* pPacket = gAsyncFileManager.newRequestPacket();
   pPacket->setDirID(gRender.getEffectCompilerDefaultDirID());
   pPacket->setFilename("minimap\\minimap.bin");
   pPacket->setReceiverHandle(mEventHandle);
   pPacket->setSynchronousReply(true);
   pPacket->setDiscardOnClose(true);
   gAsyncFileManager.submitRequest(pPacket);
}

//==============================================================================
// Initialize effect
//==============================================================================
void BMiniMap::setupEffect(const BEvent &event)
{
   ASSERT_RENDER_THREAD

   BAsyncFileManager::BRequestPacket *packet = reinterpret_cast<BAsyncFileManager::BRequestPacket *>(event.mpPayload);

   if (!packet->getSucceeded())
   {
      trace("BMiniMap::receiveEvent: Async load of file %s failed", packet->getFilename().c_str());
   }
   else
   {
      HRESULT hres = mEffect.createFromCompiledData(BD3D::mpDev, packet->getData());
      if (FAILED(hres))
      {
         trace("BMiniMap::receiveEvent: Effect creation of file %s failed", packet->getFilename().c_str());
      }

      // Get techniques and params
      mColoredBlendedPointSpriteTechnique = mEffect.getTechnique("coloredBlendedPointSprite");
      mColoredBlendedSpriteTechnique = mEffect.getTechnique("coloredBlendedSprite");
      mTexturedBlendedSpriteTechnique = mEffect.getTechnique("texturedBlendedSprite");
      mTextureAdditiveBlendedSpriteTechnique = mEffect.getTechnique("texturedAdditiveBlendedSprite");
      mTexturedBlendedPointSpriteTechnique = mEffect.getTechnique("texturedBlendedPointSprite");
      mTexturedBlendedColoredPointSpriteTechnique = mEffect.getTechnique("texturedBlendedColoredPointSprite");
      mVisibilityGeneratorTechnique = mEffect.getTechnique("visibilityGenerator");
      mDrawVisibilityTechnique = mEffect.getTechnique("drawVisibility");
      mTransformParam = mEffect("transform");
      mLinearTextureParam = mEffect("linearTexture");
      mLinearStencilParam = mEffect("linearStencil");
      mPointTextureParam = mEffect("pointTexture");
      mColorScaleParam = mEffect("colorScale");
      mTextureAtlasScaleParam = mEffect("textureAtlasScale");

      // Create vertex decl
      const D3DVERTEXELEMENT9 PSD_vertexElements[] =
      {
         { 0, 0, D3DDECLTYPE_FLOAT16_2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
         { 0, 4, D3DDECLTYPE_FLOAT1, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_PSIZE, 0 },
         { 0, 8, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0 },
         D3DDECL_END()
      };
      BD3D::mpDev->CreateVertexDeclaration(PSD_vertexElements, &mBPSDVertexDecl);
 
      const D3DVERTEXELEMENT9 PD_vertexElements[] =
      {
         { 0, 0, D3DDECLTYPE_FLOAT16_2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
         { 0, 4, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0 },
         D3DDECL_END()
      };
      BD3D::mpDev->CreateVertexDeclaration(PD_vertexElements, &mBPDVertexDecl);

      const D3DVERTEXELEMENT9 PT_vertexElements[] =
      {
         { 0, 0, D3DDECLTYPE_FLOAT16_2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
         { 0, 4, D3DDECLTYPE_FLOAT16_2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
         D3DDECL_END()
      };
      BD3D::mpDev->CreateVertexDeclaration(PT_vertexElements, &mBPTVertexDecl);

      const D3DVERTEXELEMENT9 PS_vertexElements[] =
      {
         { 0, 0, D3DDECLTYPE_FLOAT16_2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
         { 0, 4, D3DDECLTYPE_FLOAT1, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_PSIZE, 0 },
         D3DDECL_END()
      };
      BD3D::mpDev->CreateVertexDeclaration(PS_vertexElements, &mBPSVertexDecl);

      const D3DVERTEXELEMENT9 PISD_vertexElements[] =
      {
         { 0, 0, D3DDECLTYPE_FLOAT16_2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
         { 0, 4, D3DDECLTYPE_FLOAT16_2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_PSIZE, 0 },
         { 0, 8, D3DDECLTYPE_FLOAT16_2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
         { 0, 12, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0 },
         D3DDECL_END()
      };
      BD3D::mpDev->CreateVertexDeclaration(PISD_vertexElements, &mBPSTDVertexDecl);
   }
}

//==============================================================================
// Kill effect
//==============================================================================
void BMiniMap::killEffect(void)
{
   ASSERT_RENDER_THREAD

   if (mBPSTDVertexDecl)
   {
      mBPSTDVertexDecl->Release();
      mBPSTDVertexDecl = NULL;
   }
   if (mBPSVertexDecl)
   {
      mBPSVertexDecl->Release();
      mBPSVertexDecl = NULL;
   }
   if (mBPTVertexDecl)
   {
      mBPTVertexDecl->Release();
      mBPTVertexDecl = NULL;
   }
   if (mBPDVertexDecl)
   {
      mBPDVertexDecl->Release();
      mBPDVertexDecl = NULL;
   }
   if (mBPSDVertexDecl)
   {
      mBPSDVertexDecl->Release();
      mBPSDVertexDecl = NULL;
   }

   mDrawVisibilityTechnique.clear();
   mVisibilityGeneratorTechnique.clear();
   mTexturedBlendedPointSpriteTechnique.clear();
   mTexturedBlendedColoredPointSpriteTechnique.clear();
   mTexturedBlendedSpriteTechnique.clear();
   mTextureAdditiveBlendedSpriteTechnique.clear();
   mColoredBlendedSpriteTechnique.clear();
   mColoredBlendedPointSpriteTechnique.clear();
   mEffect.clear();
}

//==============================================================================
// Trigger a flare
//==============================================================================
void BMiniMap::triggerFlare(BVector flarePosition, long playerID, int flareType)
{
   ASSERT_MAIN_THREAD

   if (!mInitialized)
      return;

   if (mFlareArray.getNumberAllocated() < mFlareArray.getSize())
   {
      uint index = 0;
      BMiniMapFlare *newFlare = mFlareArray.acquire(index);
      if (newFlare)
      {
         const DWORD gameTime    = gWorld->getGametime();
         newFlare->mPosition     = flarePosition;
         newFlare->mSize         = gUIGame.getMinimapFlareStartSize();
         newFlare->mEndTime      = gameTime + gUIGame.getMinimapFlareDuration();
         newFlare->mColor        = gWorld->getPlayerColor(playerID, BWorld::cPlayerColorContextMinimap);
         newFlare->mLastTime     = gameTime;
         newFlare->mPulseScale   = gUIGame.getMinimapFlareStartSpeed();

         //gUIGame.playSound(BSoundManager::cSoundFlare);
         switch (flareType)
         {
         case BUIGame::cFlareLook: gUIGame.playSound(BSoundManager::cSoundFlareLook); break;
            case BUIGame::cFlareHelp: gUIGame.playSound(BSoundManager::cSoundFlareHelp); break;
            case BUIGame::cFlareMeet: gUIGame.playSound(BSoundManager::cSoundFlareMeet); break;
            case BUIGame::cFlareAttack: gUIGame.playSound(BSoundManager::cSoundFlareAttack); break;
            default: gUIGame.playSound(BSoundManager::cSoundFlare); break;
         }
      }
   }
}

//==============================================================================
// Add an alert to the minimap
//==============================================================================
void BMiniMap::showAlert(const BAlert &alert)
{
   ASSERT_MAIN_THREAD

   if (!mInitialized)
      return;

   BPSTDVertex          iconVertex;

   // Init new vertex
   iconVertex.position  = XMHALF2(alert.getLocation().x, alert.getLocation().z);
   iconVertex.size      = XMHALF2(gUIGame.getMinimapAlertSize(), 64.0f);
   iconVertex.UV        = XMHALF2(0.0f, 0.0f);
   iconVertex.diffuse   = cDWORDWhite;

   // Add it to the icon array
   mIconUnitArray.pushBack(iconVertex);
}

//==============================================================================
// This creates a circular stencil texture
//==============================================================================
void createCircleStencil(D3DXVECTOR4 *pOut, CONST D3DXVECTOR2 *pTexCoord, CONST D3DXVECTOR2 *pTexelSize, LPVOID pData)
{
   const D3DXVECTOR2 center(0.5f, 0.5f);
   const D3DXVECTOR2 delta = *pTexCoord - center;
   const float distanceSqr = delta.x * delta.x + delta.y * delta.y;
   const float maxDistanceSqr = 0.5f * 0.5f;

   if (distanceSqr < maxDistanceSqr)
   {
      *pOut = *((D3DXVECTOR4 *) pData);
   }
   else
   {
      *pOut = D3DXVECTOR4(0.0f, 0.0f, 0.0f, 0.0f);
   }
}

//==============================================================================
// This fills a texture
//==============================================================================
void fillTexture(D3DXVECTOR4 *pOut, CONST D3DXVECTOR2 *pTexCoord, CONST D3DXVECTOR2 *pTexelSize, LPVOID pData)
{
   *pOut = *((D3DXVECTOR4 *) pData);
}

//==============================================================================
// Minimap flare update. Returns false if it's time to kill it.
//==============================================================================
bool BMiniMap::BMiniMapFlare::update(void)
{
   const DWORD gameTime = gWorld->getGametime();
   if (gameTime >= mEndTime)
      return false;

   mSize += mPulseScale * ((float) (gameTime - mLastTime));
   mLastTime = gameTime;

   if (mPulseScale == gUIGame.getMinimapFlareGrowSpeed())
   {
      if (mSize >= gUIGame.getMinimapFlareBigSize())
      {
         mSize = gUIGame.getMinimapFlareBigSize();
         mPulseScale = gUIGame.getMinimapFlareShrinkSpeed();
      }
   }
   else
   {
      if (mSize <= gUIGame.getMinimapFlareSmallSize())
      {
         mSize = gUIGame.getMinimapFlareSmallSize();
         mPulseScale = gUIGame.getMinimapFlareGrowSpeed();
      }
   }

   return true;
}

//==============================================================================
//==============================================================================
void BMiniMap::applyVisibilityTexture()
{
   ASSERT_MAIN_THREAD
   gRenderThread.submitCommand(mCommandHandle, cMMCommandApplyVisibilityTexture);      
}
