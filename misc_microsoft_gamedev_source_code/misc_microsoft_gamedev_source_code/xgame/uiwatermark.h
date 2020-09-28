//==============================================================================
// uiwatermark.h
//
// manages and renders game watermark
//
// Copyright (c) 2007 Microsoft Corporation
//==============================================================================

#pragma once
struct BTLVertex;
class BWatermarkUI
{
protected:
   bool bVBCreationPending;
   void getUserXuid(PXUID pXuid);
   void initialize();
   void load();

   void updateXuid();
   void buildVertexBuffer();
   void addTickToVertexBuffer(BTLVertex *pVertex, float angle, bool highTick);
   void workerRenderInitialize(void *pData);

   float mCenterX, mCenterY;        // Minimap center coordinates
   float mRadius;                   // Minimap radius

   float mTextStartY;               // Text watermark start Y

   uint  mTickCount;                // Maximum potential number of ticks around the dial (per layer) - not XUID dependent
   uint  mActualTickCount;          // Actual number of ticks on the dial - XUID dependent
   uint8  mXuidHash;                // The XUID will be XORed with mXuidHash * 0x0101010101010101
   uint8  mXuidHashUpdateCounter;   // mXuidHash will update only once HASH_UPDATE_FRAMES frames
   BOOL  mForceBuildVertexBuffer;   

   DWORD mColor;
   XUID  mUserXUID;                 // This frame's XUID
   XUID  mOldUserXUID;              // Last frame's XUID
   uint  countSetBits(XUID xuid, UINT lowBits);

   IDirect3DVertexBuffer9 *mpVertexBuffer;

   bool  mVisible;

public:
   void initFromMinimap();
   void setVisible(bool val) { mVisible=val; }
   void render();
   BWatermarkUI();
   ~BWatermarkUI();
};

extern BWatermarkUI gWatermarkUI;
