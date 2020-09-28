// File: line_parser.h
#pragma once
#ifndef LINEPARSER_H
#define LINEPARSER_H

#include "common/utils/string.h"

namespace gr
{
	class LineParser
	{
	public:
		LineParser(const char* pBuf, int bufLen = -1) :
			mpBuf(DebugNull(pBuf)),
			mPcur(pBuf),
			mLen(bufLen),
			mRemaining(bufLen)
		{
			if (mLen < 0)
				mLen = strlen(pBuf);
		}
		
		// true if eof
		bool getString(BigString& buf)
		{
			if (eof())
				return true;

			int len = 0;

			buf.clear();

			while (len < buf.maxLen())
			{
				const int c = getChar();
				if (c == FAILURE)
					break;

				if (c == '\n')
					break;
				else if (c == '\r')
				{
					if (peekChar() == '\n')
						getChar();
					break;
				}
	        
				buf.appendChar(c, len);
				len++;
			}

			return false;
		}
		
		// FAILURE == eof
		int getChar(void) 
		{
			if (0 == mRemaining)
				return FAILURE;

			const int c = *mPcur++;
			mRemaining--;
	    
			return c;
		}

		// FAILURE == eof
		int peekChar(void) const
		{
			if (0 == mRemaining)
				return FAILURE;

			return *mPcur;
		}
		
		// true == eof
		bool eof(void) const
		{
			return 0 == mRemaining;
		}
	
	private:
		const char* mpBuf;
		const char* mPcur;
		int mLen, mRemaining;
	};

}  // namespace gr

#endif // LINEPARSER_H
