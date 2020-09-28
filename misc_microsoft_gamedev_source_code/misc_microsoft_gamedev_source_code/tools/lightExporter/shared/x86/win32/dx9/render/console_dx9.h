//-----------------------------------------------------------------------------
// File: console_dx9.h
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#pragma once
#ifndef CONSOLE_DX9_H
#define CONSOLE_DX9_H

#include "common/render/console.h"
#include "d3dfont.h"

namespace gr
{
	class DX9Console : public Console
	{
		CD3DFont* mpFont;
		float mX, mY, mXScale, mYScale, mYSep;
		D3DCOLOR mColor;

		static int fromHexChar(int c)
		{
			if ((c >= '0') && (c <= '9'))
				return c - '0';
			
			c = tolower(c);
			
			if ((c >= 'a') && (c <= 'f'))
				return 10 + c - 'a';

			return 0;
		}

		static int fromHex(const char* pChars)
		{
			return fromHexChar(pChars[0]) * 16 + fromHexChar(pChars[1]);
		}
    			
	public:
		DX9Console(void) : 
			mpFont(NULL), 
			mX(16), 
			mY(16), 
			mXScale(1.0f),
			mYScale(1.0f),
			mYSep(1.0f),
			mColor(0xFFFFFFFF)
		{
		}

		void setFont(CD3DFont* pFont)
		{
			mpFont = pFont;
		}

		void setDisplayParameters(float x, float y, float xScale, float yScale, float ySep, D3DCOLOR color)
		{
			mX = x;
			mY = y;
			mXScale = xScale;
			mYScale = yScale;
			mYSep = ySep;
			mColor = color;
		}
		
		virtual void display(void)
		{
			mpFont->DrawTextBegin();

			float rowHeight = ceil(mpFont->GetRowHeight() * mYScale * mYSep);

			float yOfs = 0;
			for (LineListType::const_iterator it = mLines.begin(); it != mLines.end(); ++it)
			{
				int flags = D3DFONT_FILTERED;
				
				DWORD color = mColor;

				const char* pStr = it->c_str();
				if ((pStr[0] == '#') && (pStr[1] == 'c'))
				{
					Assert(strlen(pStr) >= 8);
					
					// \cFFFFFF
					color = D3DCOLOR_ARGB(255, fromHex(&pStr[2]), fromHex(&pStr[4]), fromHex(&pStr[6]));
					
					pStr += 8;
				}

				mpFont->DrawText(mX, mY + yOfs, color, pStr, flags, mXScale, mYScale);
        yOfs += rowHeight;
			}

			mpFont->DrawTextEnd();
		}
	};

	extern DX9Console& gDX9Console;

} // namespace gr

#endif // CONSOLE_DX9_H
