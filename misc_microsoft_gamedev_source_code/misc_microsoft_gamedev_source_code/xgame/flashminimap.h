//============================================================================
// flashminimap.h
//
//============================================================================

#pragma once 
#include "flashscene.h"
#include "flashgateway.h"
#include "d3dtexturemanager.h"
#include "visual.h"
#include "flashminimaprenderer.h"
#include "maximumsupportedplayers.h"


class BObject;
class BSquad;

class BFlashMinimapIconLookup
{
public:
   BFlashMinimapIconLookup(): mProtoID(-1), mIndex(-1), mIconDataIndexUsesFlash(false), mIconDataIndex2UsesFlash(false) {};
   ~BFlashMinimapIconLookup(){};

   int mProtoID;
   int mIndex;
   int mIconDataIndex;
   int mIconDataIndex2;
   bool mIconDataIndexUsesFlash: 1;
   bool mIconDataIndex2UsesFlash: 1;
};


//============================================================================
// class BFlashMinimap
//============================================================================
class BFlashMinimap : public BFlashScene
{
public:
   BFlashMinimap();
   virtual ~BFlashMinimap();

   enum 
   {
      cFlagInitialized = 0,
      cFlagTotal
   };

   enum BMinimapFloatDatao
   {
      eFloatMapViewX,
      eFloatMapViewY,
      eFloatMapViewWidth,
      eFloatMapViewHeight,
      eFloatMapViewMinPct,
      eFloatMapViewMaxPct,
      
      eFloatDataTotal,
   };

   enum BMinimapHalf4Data
   {
      eHalf4Icon1,
      eHalf4Icon2,
      eHalf4Icon3,
      eHalf4Icon4,
      eHalf4Icon5,
      eHalf4Icon6,
      eHalf4Icon7,
      eHalf4Icon8,
      eHalf4Icon9,

      eHalf4DataTotal,
   };

   enum BMinimapControls
   {      
      eControlTotal
   };

   enum BMinimapASFunctions
   {
      eMinimapASFunctionSetPosition,
      eMinimapASFunctionSetRotation,
      eMinimapASFunctionSetScale,
      eMinimapASFunctionSetNotification,      
      eMinimapASFunctionTotal,
   };

   enum BMinimapItemType
   {
      cMinimapItemTypeObject,
      cMinimapItemTypeUnit,
      cMinimapItemTypePlatoon,
      cMinimapItemTypeSquad,
      cMinimapItemTypeFlare,
      cMinimapItemTypeAlert,
      cMinimapItemTypeCenterPoint,
      cMinimapItemTypeSensorLock,
      cMinimapItemTypeFocusPosition,
      cMinimapItemType,
   };

   bool init(const char* filename, const char* datafile);
   bool initAfterLoad();
   void deinit();
   void enter();
   void leave();
   BFlashMovieInstance*  getMovie() { return mpMovie; }

   void update(float elapsedTIme);
   void renderBegin();
   void render();
   void renderEnd();
   bool handleInput(int port, BInputEventType event, BInputControlType controlType, BInputEventDetail& detail);
   bool receiveEvent(const BEvent& event, BThreadIndex threadIndex);
   void setDimension(int x, int y, int width, int height);
   void setVisible(bool bON);
   void setRotationOffset(float degrees);
   float getRotationOffset() const { return mRotationOffset; }

   bool getFullZoomOut() const { return mbFullZoomOut; }
   void setFullZoomOut(bool bFullZoom) { mbFullZoomOut = bFullZoom; updateMap(); }

   void renderIconAnimations();
   void workerRenderIconAnimations();
   void releaseGPUHeapTextures();
   
   void reset();
   void commit();
   void generateVisibility();

   void addItem(int type, void* pData = NULL);
   void addFocusPositions();
   void addFlare(BVector pos, int playerID, int flareType);
   void addAlert(BVector pos);
   void addReveal(BVector pos, float lineOfSight);
   void addReveal(BVector pos, float width, float height);
   void addBlock(BVector pos, float lineOfSight);
   void addBlock(BVector pos, float width, float height);
   void setMapTexture(BManagedTextureHandle mapHandle) { mMapTextureHandle = mapHandle; }

   BManagedTextureHandle getRenderTargetTexture();
   IDirect3DTexture9* getRenderTargetTexturePtr();

   void  repositionMap(float x, float y, float w, float h);
   float getMapViewCenterX() const { return mMapViewCenterX; }
   void  setMapViewCenterX(float centerX) { mMapViewCenterX=centerX; }

   float getMapViewCenterY() const { return mMapViewCenterY; }
   void  setMapViewCenterY(float centerY) { mMapViewCenterY=centerY; }

   float getMapViewWidth() const { return mMapViewWidth; }
   void  setMapViewWidth(float width) { mMapViewWidth=width;}

   float getMapViewHeight() const { return mMapViewHeight; }
   void  setMapViewHeight(float height) { mMapViewHeight = height; }

   void  setMapSkirtMirroring(bool value) { mbMapSkirtMirroring = value; }
   bool  getMapSkirtMirroring() const     { return mbMapSkirtMirroring; }

   int getFlashMovieX() const { return mX; }
   int getFlashMovieY() const { return mY; }
   int getFlashMovieW() const { return mWidth; }
   int getFlashMovieH() const { return mHeight; }


   void setupScaleMatrix();
   void initMapPosition();
   void calulateRotationMatrix();

   void setMapFlash(bool flashOn);

private:

   void addCenterPoint();
   void computeIconPosition(const BVector& camerPos, const BVector& position, BVector& minimapIconPosition);
   void reveal(BFlashMinimapVisibilityItem vis);
   void block(BFlashMinimapVisibilityItem vis);
   void reveal(BVector pos, float lineOfSight);
   void reveal(BVector pos, float width, float height);
   void block(BVector pos, float lineOfSight);
   void block(BVector pos, float width, float height);
   void alert(BVector pos);
   void commitItems();
   void commitReveals();
   void commitBlocks();
   void commitAlerts();
   void clear();   
   void clearItems();   
   void clearReveals();   
   void clearBlocks();   

   void initLookups();
   void initMap();
   void initStaticIconTexture();
   void updateMap();
   void updateRotation();
   void updateZoom();
   void updateFlares();
   void updateIconPositions(BDynamicSimArray<BFlashMinimapItem>& icons);
   void renderIcons();
   void renderStaticIcons();
   void renderMap();
   void renderFlares();

   // Data

   BDynamicSimArray<BFlashMinimapIconLookup> mIconLookup;
   BDynamicSimArray<BFlashMinimapItem> mIcons;
   BDynamicSimArray<BFlashMinimapItem> mStaticIcons;
   BDynamicSimArray<BFlashMinimapItem> mFlares;
   BDynamicSimArray<BFlashMinimapVisibilityItem> mVisibility[2];
   BDynamicSimArray<BFlashMinimapVisibilityItem> mVisibilityBlockers;
   
   class BQueuedItem
   {
      public:
         BQueuedItem() : mType(-1), mpData(NULL) {}
         BQueuedItem(int type, void *pData) { mType = type; mpData = pData; }

         int mType;
         void *mpData;
   };
   BDynamicSimArray<BQueuedItem> mQueuedItems;

   BDynamicSimArray<BFlashMinimapVisibilityItem> mQueuedReveals;
   BDynamicSimArray<BFlashMinimapVisibilityItem> mQueuedBlocks;
   BDynamicSimArray<BVector> mQueuedAlerts;

   BFlashMinimapItem                   mMap;

   BMatrix                    mScaleMatrix;
   BMatrix                    mPositionMatrix;
   BMatrix                    mRotationMatrix;
   BMatrix                    mVisibilityScaleMatrix;

   BVector                    mCameraPos;
   BVector                    mFocusPos[cMaximumSupportedPlayers];
   
   BFlashMovieInstance*       mpMovie;
   BFlashMovieInstance*       mpIconMovie;
   BFlashGateway::BDataHandle mDataHandle;
   BFlashGateway::BDataHandle mIconDataHandle;

   BManagedTextureHandle      mMapTextureHandle;
   BManagedTextureHandle      mMapBackgroundTextureHandle;
   BManagedTextureHandle      mMapMaskTextureHandle;
   BManagedTextureHandle      mIconTextureHandle;
   BManagedTextureHandle      mVisMaskTextureHandle;
   BManagedTextureHandle      mStaticIconTextureHandle;
   
   // flash movie params
   int mX;
   int mY;
   int mWidth;
   int mHeight;

   // Map Specifics
   float mWorldToMapConversionFactor;
   float mMapViewCenterX;
   float mMapViewCenterY;
   float mMapViewWidth;
   float mMapViewHeight;
   float mMapViewHalfWidth;
   float mMapViewHalfHeight;
   float mMapViewIconStickyOffset;
   float mRotationAngle;
   float mMapFogScalar;
   float mMapSkirtFogScalar;
   float mRotationSpeed;
   float mPanSpeed;
   float mFullZoomOut;
   float mIconScaleX;
   float mIconScaleY;
   float mDefaultCameraYawRadians;

   BFlashPropertyHandle mFlareIconHandle;
   BFlashPropertyHandle mFlareDurationHandle;   
   XMHALF2 mFlareIconSize;

   BFlashPropertyHandle mAlertIconHandle;
   BFlashPropertyHandle mAlertDurationHandle;
   bool                 mAlertIconUsesFlash : 1;
   XMHALF2 mAlertIconSize;
   UTBitVector<8> mAlertStates;  

   BFlashPropertyHandle mSquadIconUVHandle;
   bool                 mSquadIconUsesFlash : 1;
   int   mSquadIconProtoID;
   float mSquadIconBaseSize;

   BFlashPropertyHandle mCenterPointIconUVHandle;
   bool                 mCenterPointUsesFlash : 1;
   int   mCenterPointIconProtoID;

   BFlashPropertyHandle mSensorLockIconHandle;
   bool                 mSensorLockUsesFlash : 1;
   int mSensorLockIconProtoID;

   float mVisibilityScale;
   int   mVisibilityIndex;

   float mWorldToMapRatioX;
   float mWorldToMapRatioY;
   float mZoom;
   float mRotationOffset;

   bool  mbVisible : 1;
   bool  mbFullZoomOut : 1;
   bool  mbShowFocusPoints : 1;
   bool  mbFirstRender : 1;
   bool  mbForceRotationUpdate : 1;
   bool  mbMapSkirtMirroring : 1;
};