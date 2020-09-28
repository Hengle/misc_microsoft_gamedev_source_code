//============================================================================
// File: consoleRender.cpp
//
// Copyright (c) 2007 Ensemble Studios
//============================================================================


#include "xgamerender.h"

#ifndef BUILD_FINAL

#include "consoleRender.h"
#include "renderDraw.h"
#include "vertexTypes.h"
#include "fixedFuncShaders.h"
#include "workdirsetup.h"
#include "configsgamerender.h"
#include "console.h"
#include "inputsystem.h"
#include "keyboard.h"
#include "render.h"
#include "configsinput.h"

#define BACKGROUND_COLOR      0xe0404040
#define SCROLLBAR_COLOR       0x40ffffff
#define SCROLLBAR_TICK_COLOR  0xffffffff
#define HEADER_COLOR          0xffffffff
#define ERROR_COLOR           0xffff8080
#define WARNING_COLOR         0xffffff00
#define DEFAULT_COLOR         0xffffffff
#define CONSOLE_SIZE          0.625f      // Percent of screen height
#define START_X               60.0f       // Start x pos for text (in pixels)
#define START_Y               40.0f       // Start x pos for text (in pixels)

BConsoleRender gConsoleRender;

//==============================================================================
// BConsoleRender::BConsoleRender()
//==============================================================================
BConsoleRender::BConsoleRender() :
   mCharOffset(0),
   mLineOffset(0),
   mPrevCategory(0),
   mPrevLinesInCategory(0)
{
}

//==============================================================================
// BConsoleRender::~BConsoleRender()
//==============================================================================
BConsoleRender::~BConsoleRender()
{
}

//==============================================================================
// BConsoleRender::init()
//==============================================================================
void BConsoleRender::init()
{
   ASSERT_THREAD(cThreadIndexSim);
   commandListenerInit();
}

//==============================================================================
// BConsoleRender::deinit()
//==============================================================================
void BConsoleRender::deinit()
{
   ASSERT_THREAD(cThreadIndexSim);
   commandListenerDeinit();
}

//==============================================================================
//BRenderCommandListener interface
//==============================================================================

//==============================================================================
// BConsoleRender::initDeviceData
//==============================================================================
void BConsoleRender::initDeviceData(void)
{
   ASSERT_RENDER_THREAD

   if (gRender.getWidth() == 640)
      mRenderFont.Create(cDirProduction, "Fonts\\courier_12_640.xpr");
   else
      mRenderFont.Create(cDirProduction, "Fonts\\courier_12.xpr");
}

//==============================================================================
// BConsoleRender::frameBegin
//==============================================================================
void BConsoleRender::frameBegin(void)
{
   ASSERT_RENDER_THREAD
}

//==============================================================================
// BConsoleRender::processCommand
//==============================================================================
void BConsoleRender::processCommand(const BRenderCommandHeader &header, const uchar *pData)
{
   ASSERT_RENDER_THREAD

   switch (header.mType)
   {
      // Render console
      case cCRRender:
      {
         SCOPEDSAMPLE(BConsoleRender);

         BDEBUG_ASSERT(header.mLen == sizeof(BConsoleRenderPacket));
         const BConsoleRenderPacket *packet = reinterpret_cast<const BConsoleRenderPacket*>(pData);

         renderBackground();
         renderConsoleText(packet->category, packet->newCharOffset, packet->newLineOffset, packet->goToTop, packet->goToBottom);
         renderScrollBars();
         break;
      }

      // Render status text
      case cCRRenderStatusText:
      {
         renderStatusText();
         renderInfoText();
         break;
      }
   }
}

//==============================================================================
// BConsoleRender::frameEnd
//==============================================================================
void BConsoleRender::frameEnd(void)
{
   ASSERT_RENDER_THREAD
}

//==============================================================================
// BConsoleRender::deinitDeviceData
//==============================================================================
void BConsoleRender::deinitDeviceData(void)
{
   ASSERT_RENDER_THREAD
   mRenderFont.Destroy();
}

//==============================================================================
// BConsoleRender::renderSubmit
//==============================================================================
void BConsoleRender::renderSubmit()
{
   ASSERT_MAIN_THREAD

   // If the console is enabled, send the cCRRender command
   long consoleStringLimit = -1;
   gConfig.get(cConfigConsoleStringLimit, &consoleStringLimit);
   if (gConfig.isDefined(cConfigConsoleRenderEnable) && (consoleStringLimit != 0))
   {
      long mode = 0;
      gConfig.get(cConfigConsoleRenderMode, &mode);

      // Begin packet
      BConsoleRenderPacket *packet = reinterpret_cast<BConsoleRenderPacket *>(gRenderThread.submitCommandBegin(mCommandHandle, cCRRender, sizeof(BConsoleRenderPacket)));
         // Set packet
         packet->category = mode;
         packet->newCharOffset = 0;
         packet->newLineOffset = 0;
         packet->goToTop = false;
         packet->goToBottom = false;
         if (gInputSystem.getKeyboard()->isKeyActive(cKeyLeft))
            packet->newCharOffset--;
         if (gInputSystem.getKeyboard()->isKeyActive(cKeyRight))
            packet->newCharOffset++;
         if (gInputSystem.getKeyboard()->isKeyActive(cKeyPageUp))
            packet->newLineOffset += 10;
         if (gInputSystem.getKeyboard()->isKeyActive(cKeyPageDown))
            packet->newLineOffset -= 10;
         if (gInputSystem.getKeyboard()->isKeyActive(cKeyUp))
            packet->newLineOffset++;
         if (gInputSystem.getKeyboard()->isKeyActive(cKeyDown))
            packet->newLineOffset--;
         if (gInputSystem.getKeyboard()->isKeyActive(cKeyHome))
            packet->goToTop = true;
         if (gInputSystem.getKeyboard()->isKeyActive(cKeyEnd))
            packet->goToBottom = true;
      // Send packet
      gRenderThread.submitCommandEnd(sizeof(BConsoleRenderPacket));
   }

   // Send the status text cCRREnderStatus command
   gRenderThread.submitCommand(mCommandHandle, cCRRenderStatusText);
}

//==============================================================================
// BConsoleRender::renderRect
//==============================================================================
void BConsoleRender::renderRectPixel(float x, float y, float width, float height, DWORD color)
{
   //     0       1280                  -1.0       1.0
   //   0 -----------                 1.0 -----------
   //     |         |                     |         |
   //     |         |         ---->       |         |
   //     |         |                     |         |
   // 720 -----------                -1.0 -----------

   float recipHalfHorizPixels = 1.0f / float(gRender.getWidth() / 2);
   float recipHalfVertPixels = 1.0f / float(gRender.getHeight() / 2);

   // Convert from pixel (0...width, 0...height) to screen (-1...1, -1...1)
   float screenWidth = width * recipHalfHorizPixels;
   float screenHeight = height * recipHalfVertPixels;
   float screenX = (x * recipHalfHorizPixels) - 1.0f;
   float screenY = 1.0f - ((y + height) * recipHalfVertPixels);
   renderRect(screenX, screenY, screenWidth, screenHeight, color);
}

//==============================================================================
// BConsoleRender::renderRect
//==============================================================================
void BConsoleRender::renderRect(float x, float y, float width, float height, DWORD color)
{
   // Make vertex buffer
   BPDVertex* pVB = static_cast<BPDVertex*>(gRenderDraw.lockDynamicVB(3, sizeof(BPDVertex)));

   // Set vertex data
   pVB->pos.x = x;
   pVB->pos.y = y;
   pVB->pos.z = 0;
   pVB->diffuse = color;
   pVB++;

   pVB->pos.x = x + width;
   pVB->pos.y = y;
   pVB->pos.z = 0;
   pVB->diffuse = color;
   pVB++;

   pVB->pos.x = x;
   pVB->pos.y = y + height;
   pVB->pos.z = 0;
   pVB->diffuse = color;

   // Set shaders and render state
   gFixedFuncShaders.set(cPosDiffuseVS, cDiffusePS);
   gFixedFuncShaders.setIdentityTransform();

   DWORD oldAlphaBlend, oldSrcBlend, oldDestBlend, oldZEnable, oldZWrite;
   BD3D::mpDev->GetRenderState(D3DRS_ALPHABLENDENABLE, &oldAlphaBlend);
   BD3D::mpDev->GetRenderState(D3DRS_SRCBLEND, &oldSrcBlend);
   BD3D::mpDev->GetRenderState(D3DRS_DESTBLEND, &oldDestBlend);
   BD3D::mpDev->GetRenderState(D3DRS_ZENABLE, &oldZEnable);
   BD3D::mpDev->GetRenderState(D3DRS_ZWRITEENABLE, &oldZWrite);

   BD3D::mpDev->SetRenderState(D3DRS_ALPHABLENDENABLE, true);
   BD3D::mpDev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
   BD3D::mpDev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
   BD3D::mpDev->SetRenderState(D3DRS_ZENABLE, false);
   BD3D::mpDev->SetRenderState(D3DRS_ZWRITEENABLE, false);


   BD3D::mpDev->SetVertexDeclaration(BPDVertex::msVertexDecl);
   BD3D::mpDev->SetStreamSource(0, gRenderDraw.getDynamicVB(), 0, sizeof(BPDVertex));

   gRenderDraw.unlockDynamicVB();
   BD3D::mpDev->DrawVertices(D3DPT_RECTLIST, 0, 3);

   BD3D::mpDev->SetStreamSource(0, 0, 0, 0);

   // Restore render state
   BD3D::mpDev->SetRenderState(D3DRS_ALPHABLENDENABLE, oldAlphaBlend);
   BD3D::mpDev->SetRenderState(D3DRS_SRCBLEND, oldSrcBlend);
   BD3D::mpDev->SetRenderState(D3DRS_DESTBLEND, oldDestBlend);
   BD3D::mpDev->SetRenderState(D3DRS_ZENABLE, oldZEnable);
   BD3D::mpDev->SetRenderState(D3DRS_ZWRITEENABLE, oldZWrite);
}

//==============================================================================
// BConsoleRender::renderBackground
//==============================================================================
void BConsoleRender::renderBackground()
{
   // Background rect is full width (-1.0...1.0) and CONSOLE_SIZE height
   float consoleHeight = CONSOLE_SIZE * 2.0f;
   renderRect(-1.0f, 1.0f - consoleHeight, 2.0f, consoleHeight, BACKGROUND_COLOR);
   //renderRectPixel(0.0f, 0.0f, 1280.0f, 450.0f, BACKGROUND_COLOR);
}

//==============================================================================
// BConsoleRender::renderScrollBars
//==============================================================================
void BConsoleRender::renderScrollBars()
{
   float screenWidth = (float) gRender.getWidth();
   float screenHeight = (float) gRender.getHeight();
   float fontHeight = mRenderFont.GetFontHeight();
   long numTextLines = long((screenHeight - START_Y) * CONSOLE_SIZE / mRenderFont.GetFontHeight());

   // Horiz scrollbar
   float x, y, width, height;
   width = screenWidth - (START_X * 2);
   height = fontHeight * 0.5f;
   x = START_X;
   y = screenHeight * CONSOLE_SIZE - height;
   renderRectPixel(x, y, width, height, SCROLLBAR_COLOR);

   // Horiz tick
   float horizPercent = max(0.0f, min(float(mCharOffset) / 200.0f, 1.0f));
   width = 10.0f;
   x = START_X * (1.0f - horizPercent) + (screenWidth - START_X - width) * horizPercent;
   renderRectPixel(x, y, width, height, SCROLLBAR_TICK_COLOR);

   // Vert scrollbar
   width = 10.0f;
   height = (numTextLines - 3) * fontHeight;
   x = screenWidth - START_X - width;
   y = START_Y + 2 * fontHeight;
   renderRectPixel(x, y, width, height, SCROLLBAR_COLOR);

   // Vert tick
   float vertPercent = max(0.0f, min(float(mLineOffset) / (mPrevLinesInCategory - (numTextLines - 3)), 1.0f));
   height = fontHeight;
   y = (START_Y + (numTextLines - 1) * fontHeight - height) * (1.0f - vertPercent) + (START_Y + 2 * fontHeight) * vertPercent;
   renderRectPixel(x, y, width, height, SCROLLBAR_TICK_COLOR);
}

//==============================================================================
// BConsoleRender::renderConsoleText
//==============================================================================
void BConsoleRender::renderConsoleText(long category, long newCharOffset, long newLineOffset, bool goToTop, bool goToBottom)
{
   // Reset character and line offsets if the category changed
   if (category != mPrevCategory)
   {
      mCharOffset = 0;
      mLineOffset = 0;
   }
   // Otherwise, update and clamp offsets
   else
   {
      mCharOffset = max(0, mCharOffset + newCharOffset);
      if (goToBottom)
         mLineOffset = 0;
      else
         mLineOffset = max(0, mLineOffset + newLineOffset);
   }

   float screenWidth = (float) gRender.getWidth();
   float screenHeight = (float) gRender.getHeight();
   float x = START_X;
   float y = START_Y;
   DWORD textColor = DEFAULT_COLOR;
   if (category == cMsgError)
      textColor = ERROR_COLOR;
   else if (category == cMsgWarning)
      textColor = WARNING_COLOR;
   float fontHeight = mRenderFont.GetFontHeight();
   float fontWidth = mRenderFont.GetTextWidth(L"=");

   // Calc number of lines that will fit on the screen (subtract 3 for headers + header underline + scrollbar at bottom)
   // Also calc num characters (subtract 1 for scrollbar on right)
   long numMessageLines = long((screenHeight - START_Y) * CONSOLE_SIZE / fontHeight) - 3;
   long numChars = long((screenWidth - (2.0f * START_X)) / fontWidth) - 1;

   ////////////////////////////////////////////////////////////////////////////
   // Construct string of category headers and render
   BUString str;
   for (long i = 0; i < cChannelMax; i++)
   {
      BUString headerStr;
      if (i == category)
         headerStr.format(L" [%S] ", gStoredConsoleOutput.getHeader(i).getPtr());
      else
         headerStr.format(L"  %S  ", gStoredConsoleOutput.getHeader(i).getPtr());
      str.append(headerStr);
   }
   mRenderFont.DrawText(x, y, HEADER_COLOR, str.getPtr());
   y += fontHeight;

   ////////////////////////////////////////////////////////////////////////////
   // Underline headers
   renderRectPixel(START_X, START_Y + fontHeight, screenWidth - (2.0f * START_X), fontHeight, 0xffffffff);
   y += fontHeight;



   ////////////////////////////////////////////////////////////////////////////
   // Get string array for category we're rendering and do any line offset adjustment
   const BRenderStringArray& msgs = gStoredConsoleOutput.getOutputMessageList(category);

   // Reset line offset if we added new lines and we weren't at the bottom
   if (goToTop && !goToBottom)
      mLineOffset = msgs.getNumber() - numMessageLines;
   else
   {
      if ((mPrevLinesInCategory != msgs.getNumber()) && (mLineOffset > 0))
      {
         // Offset by number of new messages
         mLineOffset += (msgs.getNumber() - mPrevLinesInCategory);
      }
   }
   mLineOffset = max(0, min(mLineOffset, msgs.getNumber() - numMessageLines)); // don't scroll up too far

   ////////////////////////////////////////////////////////////////////////////
   // Construct and render message strings
   long numMsgsToRender = min(numMessageLines, msgs.getNumber());
   long startMsg = msgs.getNumber() - numMsgsToRender - mLineOffset;
   startMsg = max(startMsg, 0);

   for (long i = startMsg; i < (startMsg + numMsgsToRender); i++)
   {
      str.format(L"%S", msgs[i].getPtr());

      // Crop beginning of string
      if (mCharOffset > 0)
      {
         if (mCharOffset >= str.length())
            str.empty();
         else
            str.crop(mCharOffset, str.length() - 1);
      }

      // Crop end of string
      if (str.length() > numChars)
      {
         str.crop(0, numChars - 4);
         str.append(L"...");
      }
      mRenderFont.DrawText(x, y, textColor, str.getPtr());
      y += fontHeight;
   }

   ////////////////////////////////////////////////////////////////////////////
   // Cache this frame's category and num of messages
   mPrevCategory = category;
   mPrevLinesInCategory = msgs.getNumber();
}

//==============================================================================
// BConsoleRender::renderStatusText
//==============================================================================
void BConsoleRender::renderStatusText()
{
   // Check that timer is running
   BTimer& timer = gStoredConsoleOutput.getStatusTextTimer();
   if (!timer.isStarted() || timer.isStopped())
      return;

   // If duration is over, stop the timer and return
   if (timer.getElapsedMilliseconds() > gStoredConsoleOutput.getStatusTextDuration())
   {
      timer.stop();
      return;
   }

   // Get text length for centering text
   BUString str;
   str.format(L"%S", gStoredConsoleOutput.getStatusText().getPtr());
   float textWidth = mRenderFont.GetTextWidth(str);

   // Render background rect
   float halfScreenWidth = float(gRender.getWidth()) * 0.5f;
   float halfScreenHeight = float(gRender.getHeight()) * 0.5f;
   renderRectPixel(halfScreenWidth - (textWidth * 0.5f), halfScreenHeight, textWidth, mRenderFont.GetFontHeight(), 0x40c0c0c0);

   // Render
   DWORD color;
   if (gStoredConsoleOutput.getStatusTextCategory() == cMsgError)
      color = ERROR_COLOR;
   else if (gStoredConsoleOutput.getStatusTextCategory() == cMsgWarning)
      color = WARNING_COLOR;
   else
      color = gStoredConsoleOutput.getStatusTextColor();

   mRenderFont.DrawText(halfScreenWidth - (textWidth * 0.5f), halfScreenHeight, color, str.getPtr());
}

//==============================================================================
// BConsoleRender::renderInfoText
//==============================================================================
void BConsoleRender::renderInfoText()
{
   if (gStoredConsoleOutput.getInfoText().length() == 0)
      return;

   // Get text length for centering text
   BUString str;
   str.format(L"%S", gStoredConsoleOutput.getInfoText().getPtr());
   float textWidth = 0.0f;
   float textHeight = 0.0f;
   
   mRenderFont.GetTextExtent(str, &textWidth, &textHeight, FALSE);

   // Render background rect
   float screenWidth = float(gRender.getWidth());
   float screenHeight = float(gRender.getHeight());

   renderRectPixel(screenWidth - textWidth - 70.0f, screenHeight - 60.0f, textWidth, textHeight, 0x40c0c0c0);

   // Render
   DWORD color = gStoredConsoleOutput.getInfoTextColor();

   mRenderFont.DrawText(screenWidth - textWidth - 70.0f, screenHeight - 60.0f, color, str.getPtr());
}

#endif