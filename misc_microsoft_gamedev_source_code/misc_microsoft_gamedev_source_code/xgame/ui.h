//==============================================================================
// ui.h
//
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================
#pragma once

// Includes
#include "d3dTextureManager.h"
#include "xmlreader.h"

// Forward declarations
class BRumbleEvent;
class BCameraEffectData;

// Constants
const float cTitleSafeArea=0.9f;

//==============================================================================
// BUIRect
//==============================================================================
class BUIRect
{
   public:
      BUIRect() : mX1(0), mY1(0), mX2(0), mY2(0) {}
      long              mX1;
      long              mY1;
      long              mX2;
      long              mY2;
};

//==============================================================================
// BUITexture
//==============================================================================
class BUITexture
{
   public:
      BManagedTextureHandle      mTextureHandle;
      long                       mRefCount;
};

//==============================================================================
// BUI
//==============================================================================
class BUI
{
   public:
      void              init();
      void              deinit();
      void              update();

      BManagedTextureHandle    loadTexture(const char* pFileName);
      void              unloadTexture(BManagedTextureHandle textureHandle);
      void              renderTexture(BManagedTextureHandle textureHandle, long x1, long y1, long x2, long y2, bool useAlpha=false, DWORD color=cDWORDWhite, bool bUseOverBright = false);
      void              renderTexture(BManagedTextureHandle textureHandle, long x1, long y1, long x2, long y2, float u0, float v0, float u1, float v1, bool useAlpha = false, DWORD color = cDWORDWhite, bool bUseOverBright = false);

      bool              loadSize(BXMLNode node, long*  pX, long*  pY);
      bool              loadPosition(BXMLNode node, BUIRect*  pRect);

      void              playRolloverSound();
      void              playClickSound();
      void              playCantDoSound();
      void              playPowerNotEnoughResourcesSound();

      void              playConfirmSound();
      void              playCancelSound();

      int               playRumbleEvent(int eventType, bool loop=false);
      int               playRumbleEvent(int eventType, int leftRumbleType, float leftStrength, int rightRumbleType, float rightStrength, float duration, bool loop);
      int               playRumbleEvent(int eventType, const BRumbleEvent* pRumbleData, bool loop);
      void              stopRumble(int rumbleID);

      void              doCameraShake(BVector location, float strength, float duration, bool onlyIfSelected, BEntityID unitID);
      void              doCameraEffect(BVector location, BCameraEffectData* pCamEffect);

      uint              wordWrapText(const BUString& text, uint maxLines, BSmallDynamicSimArray<BUString>& lines);

      void              forceUnloadAllTextures();

      float mfSafeX1;
      float mfSafeY1;
      float mfSafeX2;
      float mfSafeY2;
      float mfSafeWidth;
      float mfSafeHeight;
      float mfCenterX;
      float mfCenterY;
      float mfWidth;
      float mfHeight;

      long mlSafeX1;
      long mlSafeY1;
      long mlSafeX2;
      long mlSafeY2;
      long mlSafeWidth;
      long mlSafeHeight;
      long mlCenterX;
      long mlCenterY;
      long mlWidth;
      long mlHeight;

      BDynamicSimArray<BUITexture> mTextureList;
      BDynamicSimArray<BUITexture> mUnloadList;

};

extern BUI gUI;