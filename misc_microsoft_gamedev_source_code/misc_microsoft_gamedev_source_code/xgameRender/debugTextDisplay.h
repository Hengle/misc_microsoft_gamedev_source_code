//=============================================================================
//
//  debugTextDisplay.h
//
//  Copyright (c) 2006, Ensemble Studios
//
//=============================================================================
#pragma once
#include "FontSystem2.h"
#include "consoleOutput.h"

//=============================================================================
// class BDebugTextDisplay
// Currently only usable from the sim thread (because the font manager is
// not thread safe).
//=============================================================================
class BDebugTextDisplay
{
public:
   BDebugTextDisplay(BHandle fontHandle);

   // Pixel positioning
   void resetPos(void);
   void setPos(float x, float y) { mSX = x; mSY = y; }
   float getPosX(void) const { return mSX; }
   float getPosY(void) const { return mSY; }
   
   // Returns upper left pixel origin, taking into account the title safe area.
   float getOriginX(void) const;
   float getOriginY(void) const;
   
   // Line positioning
   uint getTotalLines(void);
   float getLineHeight(void) const { return mYH; }
   
   void setLine(uint i);
   void skipLine(int delta = 1) { mSY += mYH * delta; }
      
   // Color
   void setColor(BColor c) { mColor = c; }
   const BColor& getColor(void) const { return mColor; }

   // Printing
   void printf(const char* pFmt, ...);
   void printf(BColor c, const char* pFmt, ...);
   
   void print(BColor c, const char* p);
   void print(const char* p) { print(mColor, p); }
      
   // Line length
   float getLineLength(const char* pFmt, ...); 
   
   BConsoleOutput& getConsoleOutput(void) { return mConsoleOutput; }
   
   BHandle getFontHandle(void) const { return mFontHandle; }
         
private:
   BHandle mFontHandle;
   float mSX;
   float mSY;
   float mYH;
   BColor mColor;
   
   BConsoleOutput mConsoleOutput;
   
   static void ConsoleOutputFunc(void* pData, BConsoleMessageCategory category, const char* pMessage) { static_cast<BDebugTextDisplay*>(pData)->printf("%s", pMessage); }
};
