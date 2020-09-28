//============================================================================
// xdebugtext.cpp
// Copyright (c) 2002, Blue Shift, Inc.
// Copyright (c) 2005, Ensemble Studios
//============================================================================
#include "xcore.h"

#ifndef BUILD_FINAL

#include "xdebugtext.h"
#include "bitmapfont9x14.h"
#include <xgraphics.h>

namespace 
{
	class Render_Pixel_Functor
	{
		uchar* m_Psurf;
		uint m_width;
		uint m_height;
		uint m_pitch;
		D3DFORMAT m_format;
	public:
		Render_Pixel_Functor(uchar* pSurf, uint width, uint height, uint pitch, D3DFORMAT format) : 
			m_Psurf(pSurf), 
			m_width(width),
			m_height(height),
			m_pitch(pitch),
			m_format(format)
		{ 
		}

		void operator()(uint x, uint y, uint c) const
		{
			if ((x >= m_width) || (y >= m_height))
				return;
			
			const uint ofs = XGAddress2DTiledOffset(x, y, m_width, sizeof(DWORD));
			uchar* Ppixel = reinterpret_cast<uchar*>(m_Psurf + ofs * sizeof(DWORD));
			if (c)
			{
			   if (m_format == D3DFMT_LE_X2R10G10B10)
			   {
               Ppixel[0] = 0xFF;
               Ppixel[1] = 0xFF;
               Ppixel[2] = 0xFF;
               Ppixel[3] = 63;
			   }
			   else
			   {
				   Ppixel[0] = 0xFF;
				   Ppixel[1] = 0xFF;
				   Ppixel[2] = 0xFF;
				   Ppixel[3] = 0xFF;
				}
			}
			else
			{
            Ppixel[0] = 0;
            Ppixel[1] = 0;
            Ppixel[2] = 0;
            Ppixel[3] = 0;
			}
		}
	};
}  // end anonymous namespace

Xbox_Debug_Text::Xbox_Debug_Text() :
	m_font(Font9x14::MakeBitmapFont()),
	m_pSurf(NULL),
	m_pitch(0),
	m_width(0),
	m_height(0),
   m_x_scale(0),
   m_y_scale(0),
   m_titlesafe_x_ofs(0),
   m_titlesafe_y_ofs(0),
   m_num_cells_x(0),
   m_num_cells_y(0),
   m_format(D3DFMT_LE_X8R8G8B8)
{
}

void Xbox_Debug_Text::set_buffer(uchar* pSurf, uint width, uint height, uint pitch, D3DFORMAT format)
{
   m_pSurf = pSurf;
   m_width = width;
   m_height = height;
   m_pitch = pitch;
   m_format = format;
         
   m_x_scale = 1;//(m_width + 639) / 640;
   m_y_scale = 1;//(m_height + 479) / 480;
   
   const uint char_width = m_font.char_width * m_x_scale;
   const uint char_height = m_font.cell_height * m_y_scale;
   
   m_titlesafe_x_ofs = static_cast<uint>((m_width * .075f) / char_width);
   m_titlesafe_y_ofs = static_cast<uint>((m_height * .075f) / char_height);

   if ((!m_x_scale) || (!m_y_scale))
   {
      m_num_cells_x = m_num_cells_y = 1;
   }
   else
   {
      m_num_cells_x = (m_width / char_width) - m_titlesafe_x_ofs * 2;
      m_num_cells_y = (m_height / char_height)  - m_titlesafe_y_ofs * 2;
   }
}

void Xbox_Debug_Text::render_raw(uint cell_x, uint cell_y, const char* pBuf)
{
   if (!m_pSurf)
      return;
   
   __try
   {
	   m_font.render(
		   pBuf, 
		   (cell_x + m_titlesafe_x_ofs) * m_font.char_width * m_x_scale, 
		   (cell_y + m_titlesafe_y_ofs) * m_font.cell_height * m_y_scale, 
		   m_x_scale,
		   m_y_scale,
		   Render_Pixel_Functor(m_pSurf, m_width, m_height, m_pitch, m_format));
   }
    __except(EXCEPTION_EXECUTE_HANDLER)
   {
   }		   
}

uint Xbox_Debug_Text::render_cooked(uint cell_x, uint cell_y, const char* pBuf)
{
   if (!m_pSurf)
      return 0;
      
   uint maxCharsPerLine = m_num_cells_x - cell_x;
	uint num_lines = 0;
		
	char line_buf[2048];
	char* pDst = line_buf;
	*pDst = '\0';
	
   const char* pSrc = pBuf;
	while (*pSrc)  
	{
	   const uint curSize = pDst - line_buf;
		const bool force_flush = (curSize == (sizeof(line_buf) - 1)) || (curSize >= maxCharsPerLine);

		if ((pSrc[0] == '\n') || (force_flush))
		{
			*pDst = '\0';
			render_raw(cell_x, cell_y, line_buf);
			num_lines++;
			pDst = line_buf;
			*pDst = '\0';
			cell_y++;
			if (!force_flush)
				pSrc++;
		}
		else 
		{
			if (*pSrc == '\t')
				*pDst++ = ' ';
			else
				*pDst++ = *pSrc;
			pSrc++;
		}
	}

	if (line_buf[0])
	{
		*pDst = '\0';
		render_raw(cell_x, cell_y, line_buf);
		num_lines++;
	}

	return num_lines;
}

uint Xbox_Debug_Text::render_fmt(uint cell_x, uint cell_y, const char *Pfmt, ...)
{
   char buf[2048];
   
	va_list args;
	va_start(args, Pfmt);
#ifdef XBOX	
   _vsnprintf_s(buf, sizeof(buf), Pfmt, args);
#else	
   buf[sizeof(buf) - 1] = '\0';
	_vsnprintf(buf, sizeof(buf) - 1, Pfmt, args);
#endif	
	va_end(args);		

	return render_cooked(cell_x, cell_y, buf);
}

Xbox_Debug_Text G_debug_text;

#endif