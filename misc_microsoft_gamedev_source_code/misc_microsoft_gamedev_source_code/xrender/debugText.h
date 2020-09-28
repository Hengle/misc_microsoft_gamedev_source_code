// File: debugText.h
#pragma once

#ifndef BUILD_FINAL
class BDebugText
{
public:
   static void init(void);
   static void deinit(void);
   
   static void renderRaw(uint cell_x, uint cell_y, const char* pBuf);

   // Supports CR, tab. Returns number of lines rendered.
   static uint renderCooked(uint cell_x, uint cell_y, const char* pBuf);

   static uint renderFmt(uint cell_x, uint cell_y, const char *pFmt, ...);
};
#endif