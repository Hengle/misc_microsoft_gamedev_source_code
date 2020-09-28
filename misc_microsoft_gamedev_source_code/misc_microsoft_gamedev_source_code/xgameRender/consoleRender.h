//============================================================================
// File: consoleRender.h
//
// Copyright (c) 2007 Ensemble Studios
//============================================================================

#pragma once

#ifndef BUILD_FINAL

#include "threading\eventDispatcher.h"
#include "AtgFont.h"
#include "renderThread.h"

//============================================================================
// BConsoleRender
//============================================================================
class BConsoleRender : public BRenderCommandListener
{
   public:
      BConsoleRender();
      ~BConsoleRender();

      // Render commands
      enum
      {
         cCRRender = 0,
         cCRRenderStatusText
      };

      // Packet of render data
      struct BConsoleRenderPacket
      {
         long category;
         long newCharOffset;
         long newLineOffset;
         bool goToTop;
         bool goToBottom;
      };

      // Init / deinit
      void           init();
      void           deinit();

      // Submit render commands for rendering console and status text
      void           renderSubmit();

      //BRenderCommandListener interface
      virtual void   initDeviceData(void);
      virtual void   frameBegin(void);
      virtual void   processCommand(const BRenderCommandHeader &header, const uchar *pData);
      virtual void   frameEnd(void);
      virtual void   deinitDeviceData(void);


   protected:

      // Rendering funcs
      void           renderRectPixel(float x, float y, float width, float height, DWORD color);
      void           renderRect(float x, float y, float width, float height, DWORD color);
      void           renderBackground();
      void           renderScrollBars();
      void           renderConsoleText(long category, long newCharOffset, long newLineOffset, bool goToTop, bool goToBottom);
      void           renderStatusText();
      void           renderInfoText();

      // Font and console rendering data
      ATG::Font      mRenderFont;
      long           mCharOffset;
      long           mLineOffset;
      long           mPrevCategory;
      long           mPrevLinesInCategory;
};

extern BConsoleRender gConsoleRender;

#endif