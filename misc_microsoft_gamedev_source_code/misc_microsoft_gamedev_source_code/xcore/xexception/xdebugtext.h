//============================================================================
// xdebugtext.h
// Copyright (c) 2002, Blue Shift, Inc.
// Copyright (c) 2005, Ensemble Studios
//============================================================================
#pragma once

#ifndef BUILD_FINAL

#include "bitmapfont.h"

class Xbox_Debug_Text
{
	Bitmap_Font m_font;
   uchar* m_pSurf;
   uint m_pitch;
   uint m_width;
   uint m_height;
   D3DFORMAT m_format;
   
   uint m_titlesafe_x_ofs;
   uint m_titlesafe_y_ofs;
   uint m_x_scale;
   uint m_y_scale;
   uint m_num_cells_x;
   uint m_num_cells_y;
      				
public:
	Xbox_Debug_Text();
	
	uint get_num_cells_x(void) const { return m_num_cells_x; }
	uint get_num_cells_y(void) const { return m_num_cells_y; }
	
	void set_buffer(uchar* pBits, uint width, uint height, uint pitch, D3DFORMAT format);
	uchar* get_buffer(void) const { return m_pSurf; }
			
	void render_raw(uint cell_x, uint cell_y, const char* pBuf);
	
	// Supports CR, tab. Returns number of lines rendered.
	uint render_cooked(uint cell_x, uint cell_y, const char* pBuf);
	
	uint render_fmt(uint cell_x, uint cell_y, const char *pFmt, ...);
};

extern Xbox_Debug_Text G_debug_text;

#endif