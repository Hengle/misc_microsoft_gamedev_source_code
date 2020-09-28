//============================================================================
// uiwatermark.cpp
//
// manages and renders game watermark
//
// Copyright (c) 2007 Microsoft Corporation
//============================================================================

#include "common.h"
#include "uiwatermark.h"
#include "fileUtils.h"
#include "gameDirectories.h"
#include "render.h"
#include "primDraw2D.h"
#include "uimanager.h"
#include "FontSystem2.h"
#include "world.h"
#include "uimanager.h"
#include "LiveSystem.h"

BWatermarkUI gWatermarkUI;

#define SAFE_RELEASE(x) { if(x) {(x)->Release(); (x)=NULL; } }
#define HASH_UPDATE_FRAMES 20

#define TEXTSTARTNON640 460
#define TEXTSTART640    300

//==============================================================================
//==============================================================================
BWatermarkUI::BWatermarkUI()
{
   bVBCreationPending = false;
   mCenterX = 175; //195;
   mCenterY = 145;
   mTextStartY = TEXTSTARTNON640;
   mRadius = 100; //75;
   mpVertexBuffer = NULL;
   mColor = 0x80FFFFFF;
   mTickCount = 12;
   mActualTickCount = 0;
   mUserXUID = 0;
   mOldUserXUID = 0;
   mXuidHash = 0;
   mForceBuildVertexBuffer = false;
   mVisible = true;
}

//==============================================================================
//==============================================================================
BWatermarkUI::~BWatermarkUI()
{
   SAFE_RELEASE(mpVertexBuffer);
}

//==============================================================================
//==============================================================================
void BWatermarkUI::initFromMinimap()
{
   mCenterX = gUIManager->getMinimapMapViewCenterX();
   mCenterY = gUIManager->getMinimapMapViewCenterY();
   mRadius = gUIManager->getMinimapMapViewWidth() * 0.5f;
   mForceBuildVertexBuffer = true;
}

//==============================================================================
// Returns number of set bits in the XUID; tests only bottom lowBits bits
//==============================================================================
uint BWatermarkUI::countSetBits(XUID x, UINT lowBits)
{
   uint bitCount = 0;

   x &= ((1 << (uint64) lowBits) - 1);
   while(x)
   {
      bitCount++;
      x &= (x-1);
   }

   return bitCount;
}

//==============================================================================
// Refreshes user Xuid, triggers a VB rebuild if the Xuid changes
//==============================================================================
void BWatermarkUI::updateXuid()
{
   // Get new Xuid
   getUserXuid(&mUserXUID);

#ifndef BUILD_FINAL
   if (!gLiveSystem->isMultiplayerGameActive())
   {
      if (mUserXUID == 0)
         mUserXUID = 1;
   }
#endif

   // If it hasn't changed this frame, we're done
   if (mUserXUID == mOldUserXUID)
   {
      return;
   }

   // If the Xuid changed, and we already kicked off a VB creation request on the render thread, just exit
   if (bVBCreationPending)
   {
      return;
   }

   const BRenderViewParams &params = gRender.getViewParams();     
   if (params.getViewportWidth()==640)
   {
     mTextStartY = TEXTSTART640; 
   }
   else
   {
     mTextStartY = TEXTSTARTNON640;
   }

   // Xuid changed, re-build the VB
   buildVertexBuffer();
   mOldUserXUID = mUserXUID;
}

//==============================================================================
// Initializes rendering resources (tick mark vertex buffer) [Render thread]
//==============================================================================
void BWatermarkUI::workerRenderInitialize(void *pData)
{
   BDEBUG_ASSERT(bVBCreationPending);
   BDEBUG_ASSERT(mpVertexBuffer == NULL);

   // Create vertex buffer
   mActualTickCount = countSetBits(mUserXUID, mTickCount*2);
   uint size = mActualTickCount * 6 * sizeof(BTLVertex);
   if (size == 0)
      return;
   HRESULT hr = gRenderDraw.createVertexBuffer(size, 0, NULL, NULL, &mpVertexBuffer, NULL);

   if (FAILED(hr))
   {
      BASSERT(0);
      return;
   }

   // Fill it out
   BTLVertex *pVBData;
   mpVertexBuffer->Lock(0, size, (void**) &pVBData, 0);

   float fDeltaAngle = 2 * D3DX_PI / mTickCount;
   uint iTicksAdded = 0;

   for (uint i=0; i<mTickCount; i++)
   {
      float fAngle = i*fDeltaAngle;
      if (mUserXUID & ((uint64) 1 << i))
      {
         addTickToVertexBuffer(pVBData + iTicksAdded*6, fAngle, false);
         iTicksAdded++;
      }

      if (mUserXUID & ((uint64) 1 << (i + mTickCount)))
      {
         addTickToVertexBuffer(pVBData + iTicksAdded*6, fAngle, true);
         iTicksAdded++;
      }
   }

   BDEBUG_ASSERT(iTicksAdded == mActualTickCount);
   mpVertexBuffer->Unlock();

   // All done
   bVBCreationPending = false;
}

//==============================================================================
// Initializes rendering resources (tick mark vertex buffer) [Main thread]
//==============================================================================
void BWatermarkUI::buildVertexBuffer()
{
   mForceBuildVertexBuffer = false;
   bVBCreationPending = true;

   if (mpVertexBuffer)
   {
      gRenderThread.blockUntilGPUIdle();
      SAFE_RELEASE(mpVertexBuffer);
   }

   // create vertex buffer
   gRenderThread.submitFunctor(BRenderThread::BFunctor(this, &BWatermarkUI::workerRenderInitialize), NULL);
}

//==============================================================================
// Returns currently signed in user's XUID
//==============================================================================
void BWatermarkUI::getUserXuid(PXUID pXuid)
{   
   for (uint nUser=0; nUser < XUSER_MAX_COUNT; nUser++)
   {
      XUSER_SIGNIN_STATE State = XUserGetSigninState(nUser);

      if (State == eXUserSigninState_SignedInToLive)
      {
         //There is only 1 signed user at a time supported
         XUserGetXUID(nUser, pXuid);
         return;
      }
   }
}

//==============================================================================
// Used to build up vertex buffer - adds one tick
//==============================================================================
void BWatermarkUI::addTickToVertexBuffer(BTLVertex *pVertex, float angle, bool highTick)
{
   D3DXMATRIX matTransform;
   D3DXMATRIX matT;

   if (highTick)
   {
      // High ticks are rotated 180 degrees
      D3DXMatrixRotationZ(&matTransform, D3DX_PI);
      static float radiusDif = 0;
      mRadius += radiusDif;
   }
   else
   {
      D3DXMatrixIdentity(&matTransform);
   }

   // Move ticks 'up' mRadius pixels
   D3DXMatrixTranslation(&matT, 0, -mRadius, 0);
   D3DXMatrixMultiply(&matTransform, &matTransform, &matT);

   // Rotate ticks
   D3DXMatrixRotationZ(&matT, angle);
   D3DXMatrixMultiply(&matTransform, &matTransform, &matT);

   // Translate the whole dial over
   D3DXMatrixTranslation(&matT, mCenterX - 0.5f, mCenterY - 0.5f, 0);
   D3DXMatrixMultiply(&matTransform, &matTransform, &matT);

   // Initialize vertex data
   for (int i=0; i<6; i++)
   {
      pVertex[i].diffuse = mColor;
      pVertex[i].rhw = 1.0f;
      pVertex[i].specular = mColor;
      pVertex[i].tu = 0.0f;
      pVertex[i].tv = 0.0f;
      pVertex[i].z = 0.0f;
   }

   pVertex[0].x =  4;   pVertex[0].y =  1;
   pVertex[1].x =  4;   pVertex[1].y =  5;
   pVertex[2].x = -4;   pVertex[2].y =  5;

   pVertex[3].x =  4;   pVertex[3].y =  1;
   pVertex[4].x = -4;   pVertex[4].y =  5;
   pVertex[5].x = -4;   pVertex[5].y =  1;

   // Transform vertices
   for (int i=0; i<6; i++)
   {
      D3DXVECTOR3 vec;
      D3DXVECTOR4 vOut;
      vec.x = pVertex[i].x;
      vec.y = pVertex[i].y;
      vec.z = pVertex[i].z;

      D3DXVec3Transform(&vOut, &vec, &matTransform);
      pVertex[i].x = vOut.x;
      pVertex[i].y = vOut.y;
      pVertex[i].z = vOut.z;
   }
}

//==============================================================================
// Renders the watermark
//==============================================================================
void BWatermarkUI::render()
{
   updateXuid();

   if (mForceBuildVertexBuffer)
      buildVertexBuffer();

   BHandle fontHandle = gUIGame.getPlayerStatFont();
   if (gRender.getViewParams().getViewportWidth()==640)
      fontHandle = gFontManager.findFont("Denmark 24");
   gFontManager.setFont( fontHandle );

   // Position watermark below player name
   float yh = gFontManager.getLineHeight();
   float sy = gUI.mfSafeY2 - yh - yh;
   //float textWidth = gFontManager.getLineLength("XXXXXXXXX", 9);
   float sx = gUI.mfSafeX2 - 1;//textWidth;

   // Trivially "hash" the XUID so that the numbers look like they are doing something interesting
   uint64 tempValue = 0x0101010101010101 * mXuidHash;
   uint64 hashedXuid = mUserXUID ^ tempValue;

   // Print top 32 bits
   char buffer[12];
   bsnprintf(buffer, sizeof(buffer), "%01X%08X", (mXuidHash >> 4), (uint) (hashedXuid >> 32));
   gFontManager.drawText(fontHandle, sx+1, sy+1, buffer, cDWORDBlack, BFontManager2::cJustifyRight);
   gFontManager.drawText(fontHandle, sx, sy, buffer, cDWORDWhite, BFontManager2::cJustifyRight);
   sy += yh;

   // Print bottom 32 bits
   bsnprintf(buffer, sizeof(buffer), "%01X%08X", (mXuidHash & 0xF), (uint) (hashedXuid));
   gFontManager.drawText(fontHandle, sx+1, sy+1, buffer, cDWORDBlack, BFontManager2::cJustifyRight);
   gFontManager.drawText(fontHandle, sx, sy, buffer, cDWORDWhite, BFontManager2::cJustifyRight);

   // Roll hash
   if ((++mXuidHashUpdateCounter) % HASH_UPDATE_FRAMES == 0)
   {
      mXuidHash++;
      mXuidHash &= 0xFF;
   }

   // Render minimap watermark
   if (mpVertexBuffer && mActualTickCount && gUIManager->getMinimapVisible() && !gWorld->isPlayingCinematic() && mVisible)
   {
      // Set render state
      gRenderDraw.setRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
      gRenderDraw.setRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
      gRenderDraw.setRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
      gRenderDraw.setRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
      gRenderDraw.setRenderState(D3DRS_ALPHATESTENABLE, FALSE);
//      gRenderDraw.setRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
      gRenderDraw.setRenderState(D3DRS_VIEWPORTENABLE, FALSE);

      // Set the stream source to the dynamic vb.
      gRenderDraw.setStreamSource(0, mpVertexBuffer, 0, sizeof(BTLVertex));
      gRenderDraw.setVertexDeclaration(BTLVertex::msVertexDecl);
      gFixedFuncShaders.set(cPosDiffuseVS, cDiffusePS);

      D3DXMATRIX identity;
      D3DXMatrixIdentity(&identity);
      gRenderDraw.setVertexShaderConstantF(0, (const float*)&identity, 4);

      // Draw the list
      gRenderDraw.drawVertices(D3DPT_TRIANGLELIST, 0, mActualTickCount*6);

      // Clean up
      gRenderDraw.setRenderState(D3DRS_VIEWPORTENABLE, TRUE);
      gRenderDraw.clearStreamSource(0);
   }
}