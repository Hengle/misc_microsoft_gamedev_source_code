//==============================================================================
// minimap.h
//
// The minimap manager/renderer.
//
// Copyright (c) 2006 Ensemble Studios
//==============================================================================

#pragma once

//==============================================================================
// Includes
//==============================================================================
#include "containers\staticArray.h"
#include "threading\eventDispatcher.h"
#include "renderThread.h"
#include "effect.h"
#include "D3DTextureManager.h"
#include "containers\freelist.h"
#include "uigame.h"

//==============================================================================
// Forward definitions
//==============================================================================
class BObject;
class BAlert;

//==============================================================================
// Defines
//==============================================================================
#define BMINIMAP_BLOCKER_ARRAY_SIZE    50
#define BMINIMAP_VISIBILITY_ARRAY_SIZE 2000
#define BMINIMAP_ARRAY_SIZE            2000
#define BMINIMAP_ICON_ARRAY_SIZE       50

//==============================================================================
// Class definition
//==============================================================================
class BMiniMap : public BEventReceiver, BRenderCommandListener
{

public:

   // Constructor/Destructor
   BMiniMap();
   ~BMiniMap();
   
   void gameInit(void);
   void gameDeinit(void);

   // Initializer/Deinitializer
   void init(float x1, float y1, float x2, float y2, float borderMinUV, float borderMaxUV, float opacity);
   void deinit(void);

   // Block a part of the minimap
   void block(BVector position, float LOS);

   // Reveal a part of the minimap
   void reveal(BVector position, float LOS);

   // Add a unit to the minimap
   void addUnit(const BObject &unit);

   // Trigger flare
   void triggerFlare(BVector flarePosition, long playerID, int flareType);

   // Add an alert to the minimap
   void showAlert(const BAlert &alert);

   // Update the minimap
   void update(void);

   // Generate visibility texture
   void generateVisibility(void);

   // Prep minimap for next game update
   void prep(void);

   // Render the minimap
   void render(void);

   // Reset the visibility texture
   void reset(void);

   void setVisible(bool bVisible) { mbVisible = bVisible; };

   void applyVisibilityTexture();

   enum
   {
      eMiniMapFlareIcon = 0,

      eMiniMapNumIcons
   };

protected:

   class BMiniMapFlare
   {

   public:

      bool update(void);

      BVector  mPosition;
      float    mSize;
      DWORD    mColor;
      DWORD    mEndTime;
      DWORD    mLastTime;
      float    mPulseScale;
   };

   struct BPTVertex
   {
      XMHALF2  position;
      XMHALF2  uv;
   };

   struct BPSDVertex
   {
      XMHALF2  position;
      FLOAT    size;
      D3DCOLOR diffuse;
   };

   struct BPSTDVertex
   {
      XMHALF2  position;
      XMHALF2  size;
      XMHALF2  UV;
      D3DCOLOR diffuse;
   };

   struct BPSVertex
   {
      XMHALF2  position;
      FLOAT    size;
   };

   struct BPDVertex
   {
      XMHALF2  position;
      D3DCOLOR diffuse;
   };

   typedef BStaticArray<BPSDVertex, BMINIMAP_ARRAY_SIZE, true, false>  UnitArray;
   typedef BStaticArray<BPSTDVertex, BMINIMAP_ICON_ARRAY_SIZE, true, false>  IconUnitArray;
   typedef BStaticArray<BPSVertex, BMINIMAP_VISIBILITY_ARRAY_SIZE, true, false>  VisibilityArray;
   typedef BStaticArray<BPSVertex, BMINIMAP_BLOCKER_ARRAY_SIZE, true, false>  BlockerArray;   

   struct BDrawBackgroundPacket
   {
      D3DXMATRIX  worldMatrix;
   };

   struct BDrawUnitsPacket
   {
      long        numUnits;
      D3DXMATRIX  worldMatrix;
      BPSDVertex  *unitVB;
   };

   struct BDrawIconsPacket
   {
      long        numIcons;
      D3DXMATRIX  worldMatrix;
      BPSTDVertex *iconVB;
   };

   struct BGenerateVisibilityPacket
   {
      long        numVisibility;
      BPSVertex   *visibilityVB;
      long        numFog;
      BPSVertex   *fogVB;
      long        numBlocker;
      BPSVertex   *blockerVB;
      D3DXMATRIX  worldMatrix;
   };

   struct BDrawVisibilityPacket
   {
      D3DXMATRIX  worldMatrix;
   };

   struct BDrawBorderPacket
   {
      bool        drawViewRect;
      D3DXMATRIX  viewRectMatrix;
      BPDVertex   *viewRectVB;
      D3DXMATRIX  worldMatrix;
   };
   
   enum 
   {
      cMMCommandDrawBackground = 0,
      cMMCommandDrawUnits,
      cMMCommandDrawIcons,
      cMMCommandGenerateVisibility,
      cMMCommandDrawVisibility,
      cMMCommandDrawBorder,
      cMMCommandDrawRadar,
      cMMCommandReset,
      cMMCommandApplyVisibilityTexture,

      cMMCommandMax
   };

   enum 
   {
      eEventClassReloadEffects = cEventClassFirstUser
   };

   void updateFlares(void);

   void renderBackground(void);
   void renderUnits(void);
   void renderIcons(void);
   void renderVisibility(void);
   void renderBorder(void);
   void renderRadarSweep(void);
   void renderOverlay(void);

   //BRenderCommandListener interface
   virtual void initDeviceData(void);
   virtual void frameBegin(void);
   virtual void processCommand(const BRenderCommandHeader &header, const uchar *pData);
   virtual void frameEnd(void);
   virtual void deinitDeviceData(void);

   //BEventReceiver interface
   virtual bool receiveEvent(const BEvent &event, BThreadIndex threadIndex);

   // Effect Loading
   void loadEffect(void);
   void reloadInit(void);
   void reloadDeinit(void);

   // Worker thread functions
   void setupEffect(const BEvent &event);
   
   void killEffect(void);
   void workerDrawBackground(const BDrawBackgroundPacket *backgroundData);
   void workerDrawUnits(const BDrawUnitsPacket *unitData);
   void workerDrawIcons(const BDrawIconsPacket *iconData);
   void workerDrawVisibility(const BDrawVisibilityPacket *visibilityData);
   void workerDrawBorder(const BDrawBorderPacket *borderData);
   void workerDrawRadar(const BDrawBorderPacket *borderData);
   void workerGenerateVisibility(const BGenerateVisibilityPacket *visibilityData);

   // Visibility index
   long     mVisibilityIndex;
   // Minimap rect in screen space
   float    mX1;
   float    mY1;
   float    mX2;
   float    mY2;
   // Minimap border min/max uv
   float    mBorderMinUV;
   float    mBorderMaxUV;
   // Minimap opacity
   float    mOpacity;
   // Minimap position in screen space (center of rect)
   float    mX;
   float    mY;
   // Minimap size in screen space
   float    mWidth;
   float    mHeight;
   float    mHWidth;
   float    mHHeight;
   // Minimap rotation
   float    mRotationAngle;
   float    mRadarRotation;
   // Conversion variables
   float    mScale;
   BMatrix  mScaleMatrix;
   BMatrix  mVisibilityGeneratorScaleMatrix;
   BMatrix  mRotationMatrix;
   BMatrix  mRadarRotationMatrix;
   BMatrix  mPositionMatrix;
   // Textures
   BManagedTextureHandle mBorderTextureHandle;
   BManagedTextureHandle mIconTextureHandle;
   BManagedTextureHandle mRadarSweepTextureHandle;
   BManagedTextureHandle mOverlayTextureHandle;
   BManagedTextureHandle mMapTextureHandle;
   IDirect3DTexture9 *mBackgroundTexture;
   IDirect3DTexture9 *mVisibilityTexture;
   IDirect3DTexture9 *mCircleStencilTexture;
   
   UnitArray         mUnitArray;
   IconUnitArray     mIconUnitArray;
   VisibilityArray   mVisibilityArray[2];
   BlockerArray      mBlockerArray;
   BFreeList<BMiniMapFlare>   mFlareArray;
   BMinimapIcon               mFlareIcon;

   BFXLEffect           mEffect;
   BFXLEffectTechnique  mColoredBlendedPointSpriteTechnique;
   BFXLEffectTechnique  mColoredBlendedSpriteTechnique;
   BFXLEffectTechnique  mTexturedBlendedSpriteTechnique;
   BFXLEffectTechnique  mTextureAdditiveBlendedSpriteTechnique;
   BFXLEffectTechnique  mTexturedBlendedPointSpriteTechnique;
   BFXLEffectTechnique  mTexturedBlendedColoredPointSpriteTechnique;
   BFXLEffectTechnique  mVisibilityGeneratorTechnique;
   BFXLEffectTechnique  mDrawVisibilityTechnique;
   BFXLEffectParam      mTransformParam;
   BFXLEffectParam      mLinearTextureParam;
   BFXLEffectParam      mLinearStencilParam;
   BFXLEffectParam      mPointTextureParam;
   BFXLEffectParam      mColorScaleParam;
   BFXLEffectParam      mTextureAtlasScaleParam;

   IDirect3DVertexDeclaration9   *mBPTVertexDecl;
   IDirect3DVertexDeclaration9   *mBPSDVertexDecl;
   IDirect3DVertexDeclaration9   *mBPDVertexDecl;
   IDirect3DVertexDeclaration9   *mBPSVertexDecl;
   IDirect3DVertexDeclaration9   *mBPSTDVertexDecl;
   
   D3DXVECTOR4          mBackgroundColor;
   D3DXVECTOR4          mVisibilityColor;

   bool                 mInitialized : 1;
   bool                 mUseBorderTexture : 1;
   bool                 mUseBackgroundColor : 1;
   bool                 mbVisible : 1;
};

//extern BMiniMap gMiniMap;