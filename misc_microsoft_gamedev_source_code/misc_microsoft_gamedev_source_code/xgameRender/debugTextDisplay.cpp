//=============================================================================
//
//  debugTextDisplay.cpp
//
//  Copyright (c) 2006, Ensemble Studios
//
//=============================================================================
#include "xgameRender.h"
#include "debugTextDisplay.h"
#include "BD3D.h"

static const float cTitleSafeArea = .88f;

//=============================================================================
// BDebugTextDisplay::BDebugTextDisplay
//=============================================================================
BDebugTextDisplay::BDebugTextDisplay(BHandle fontHandle) :
   mFontHandle(fontHandle),
   mColor(cColorWhite)
{
   mConsoleOutput.init(ConsoleOutputFunc, this);
   
   mYH = gFontManager.getLineHeight();
   
   resetPos();
}

//=============================================================================
// BDebugTextDisplay::setLine
//=============================================================================
void BDebugTextDisplay::setLine(uint i) 
{ 
   mSY = BD3D::mD3DPP.BackBufferHeight * (1.0f - cTitleSafeArea) * .5f + mYH * i;
}

//=============================================================================
// BDebugTextDisplay::getTotalLines
//=============================================================================
uint BDebugTextDisplay::getTotalLines(void)
{
   return (uint)((BD3D::mD3DPP.BackBufferHeight * cTitleSafeArea) / mYH);
}

//=============================================================================
// BDebugTextDisplay::resetPos
//=============================================================================
void BDebugTextDisplay::resetPos(void) 
{ 
   mSX = getOriginX();
   mSY = getOriginY();
}

//=============================================================================
// BDebugTextDisplay::getOriginX
//=============================================================================
float BDebugTextDisplay::getOriginX(void) const
{
   return BD3D::mD3DPP.BackBufferWidth * (1.0f - cTitleSafeArea) * .5f;
}

//=============================================================================
// BDebugTextDisplay::getOriginY
//=============================================================================
float BDebugTextDisplay::getOriginY(void) const
{
   return BD3D::mD3DPP.BackBufferHeight * (1.0f - cTitleSafeArea) * .5f;
}

//=============================================================================
// BDebugTextDisplay::printf
//=============================================================================
void BDebugTextDisplay::printf(const char* pFmt, ...)
{
   BDEBUG_ASSERT(pFmt);
   
   BFixedString256 buf;

   va_list args;
   va_start(args, pFmt);

   buf.formatArgs(pFmt, args);
   va_end(args);

   print(mColor, buf.c_str());
}

//=============================================================================
// BDebugTextDisplay::printf
//=============================================================================
void BDebugTextDisplay::printf(BColor c, const char* pFmt, ...)
{
   BDEBUG_ASSERT(pFmt);
      
   BFixedString256 buf;

   va_list args;
   va_start(args, pFmt);

   buf.formatArgs(pFmt, args);
   va_end(args);

   print(c, buf.c_str());
}

//=============================================================================
// BDebugTextDisplay::print
//=============================================================================
void BDebugTextDisplay::print(BColor color, const char* p)
{
   BDEBUG_ASSERT(p);

   gFontManager.drawText(mFontHandle, mSX, mSY, p, color.asDWORD());

   mSY += mYH; 
}

//=============================================================================
// BDebugTextDisplay::print
//=============================================================================
float BDebugTextDisplay::getLineLength(const char* pFmt, ...)
{
//-- FIXING PREFIX BUG ID 6333
   const BFont2* pFont = gFontManager.getFont(mFontHandle);
//--
   if (!pFont)
      return 0.0f;
      
   BFixedString256 buf;

   va_list args;
   va_start(args, pFmt);

   buf.formatArgs(pFmt, args);
   va_end(args);

   return pFont->getTextWidth(buf.c_str());
}

