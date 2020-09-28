//-----------------------------------------------------------------------------
// File: string.h
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#pragma once
#include "common/core/types.h"
#include "stream.h"
#include <string>

namespace gr
{
	template<int Size>
	struct String
	{
		enum { BufSize = Size };
		enum { MaxLen = Size - 1 };
		
		String& setTerminator(int ofs)
		{
			safeTerminator(*this, ofs);
			return *this;
		}

    String()
		{
			StaticAssert(BufSize >= 2);
			setTerminator(0);
		}

		String(const char* pStr)
		{
			StaticAssert(BufSize >= 2);
			Assert(pStr);
			const int l = Math::Min<int>(static_cast<int>(strlen(pStr)), maxLen());
			memcpy(mBuf, pStr, l);
			setTerminator(l);
		}

		String(EVarArg e, const char* pFmt, ...)
		{
			va_list args;
			va_start(args, pFmt);
			_vsnprintf(mBuf, bufSize(), pFmt, args);
			va_end(args);
			Assert(len() <= maxLen());
		}

		int bufSize(void) const
		{
			return static_cast<int>(BufSize);
		}

		int maxLen(void) const 
		{ 
			return static_cast<int>(MaxLen); 
		}
				
		int len(void) const
		{
			return DebugRangeIncl<int>(static_cast<int>(strlen(mBuf)), maxLen());
		}

		bool empty(void) const
		{
			return 0 == len();
		}
		
		operator	const char*() const { return mBuf; }	
		operator				char*()				{	return mBuf; }	
		
		operator std::string() const { return std::string(c_str()); }
		
		const char* c_str() const			{ return mBuf; }

		char  operator[] (int i) const	{ return mBuf[DebugRange(i, Size)]; }
		char& operator[] (int i)				{ return mBuf[DebugRange(i, Size)]; }

		bool operator== (const String& b) const		{ return strcmp(mBuf, b.mBuf) == 0;	}
		bool operator== (const char* pStr) const	{ return strcmp(mBuf, pStr) == 0;	}
		
		friend bool operator== (const char* pStr, const String& b) { return strcmp(pStr, b.mBuf) == 0; }

		bool operator!= (const String& b) const	{ return strcmp(mBuf, b.mBuf) != 0;	}
		bool operator<	(const String& b) const { return strcmp(mBuf, b.mBuf)  < 0;	}
		bool operator<= (const String& b) const { return strcmp(mBuf, b.mBuf) <= 0;	}
		bool operator>	(const String& b) const { return strcmp(mBuf, b.mBuf)  > 0;	}
		bool operator>= (const String& b) const { return strcmp(mBuf, b.mBuf) >= 0;	}

		String& operator= (const char* pStr) 
		{
			Assert(pStr);
			const int l = Math::Min<int>((int)strlen(pStr), maxLen());
			memcpy(mBuf, pStr, l);
			setTerminator(l);
			return *this;
		}

		String& tolower(void) 
		{
			strlwr(mBuf);
			return *this;
		}

		String& clear(void) 
		{
			memset(mBuf, 0, sizeof(mBuf));
			return *this;
		}

		String& truncate(int ofs = 0)
		{
			return setTerminator(ofs);
		}
		
		String& removePath(void)
		{
			const char *p = mBuf + len() - 1;
			while (p >= mBuf)
			{
				if ((*p == ':') || (*p == '\\') || (*p == '/'))
					break;
				p--;
			}
			memmove(mBuf, p + 1, strlen(p + 1) + 1);
			Assert(strlen(mBuf) <= maxLen());
			return *this;
		}

		String& removeFilename(void)
		{
			char *p = mBuf + len() - 1;
			while (p >= mBuf)
			{
				if ((*p == ':') || (*p == '\\') || (*p == '/'))
					break;
				p--;
			}
			p[1] = '\0';
			Assert(strlen(mBuf) <= maxLen());
			return *this;
		}

		String& removeExtension(void)
		{
			char *p = mBuf + len() - 1;
			while (p >= mBuf)
			{
				if ((*p == ':') || (*p == '\\') || (*p == '/'))
					break;
				else if (*p == '.')
				{
					*p = '\0';
					break;
				}
				p--;
			}
			Assert(strlen(mBuf) <= maxLen());
			return *this;
		}

		String getFilename(void) const
		{
			return String(*this).removePath();
		}

		String getPath(void) const
		{
			return String(*this).removeFilename();
		}
		
		String getExtension(void) const
		{
			String ret;
			const char *p = mBuf + len() - 1;
			while (p >= mBuf)
			{
				if ((*p == ':') || (*p == '\\') || (*p == '/'))
					break;
				else if (*p == '.')
				{
					ret.append(p + 1);
					break;
				}
				p--;
			}
			Assert(strlen(ret.mBuf) <= maxLen());
			return ret;
		}

		String& appendChar(uint8 c, int ofs = -1)
		{
			const int d = (ofs < 0) ? len() : ofs;
			
			Assert(d < maxLen());
			
			if (d < maxLen())
			{
        (*this)[d] = c;
				safeTerminator(*this, d + 1);
			}

			return *this;
		}

		String& append(const char* pSrc, int maxChars = INT_MAX)
		{
			const int curLen = len();
			const int srcLen = static_cast<int>(strlen(pSrc));
			
			const int numChars = Math::Clamp(Math::Min(srcLen, maxChars), 0, maxLen() - curLen);

			if (numChars)
			{
				memcpy(mBuf + curLen, pSrc, numChars);
				setTerminator(curLen + numChars);
			}

			return *this;
		}

		String& sprintf(const char* pFmt, ...)
		{
			va_list args;
			va_start(args, pFmt);
			_vsnprintf(mBuf, bufSize(), pFmt, args);
			va_end(args);
			Assert(len() <= maxLen());
			return *this;
		}

		String& trim(void) 
		{
			int i, j;

			for (i = len() - 1; i >= 0; i--)
			{
				if (' ' != mBuf[i])
					break;
			}

			mBuf[i + 1] = '\0';
			
			for (j = 0; ' ' == mBuf[j]; j++) ;
			
			memmove(mBuf, mBuf + j, (i + 1) - j + 1);

			return *this;
		}

		String& convertToPathname(void)
		{
			if (!len())
				*this = ".";
			
     	const int lastChar = mBuf[len() - 1];
			if ((lastChar != ':') && (lastChar != '/') && (lastChar != '\\'))
				append("\\");

			return *this;
		}

		String operator+ (const char* pStr) const
		{
			return String(*this).append(String(pStr));
		}

		template<int RHSize> String operator+ (const String<RHSize>& b) const
		{
			return String(*this).append(b);
		}

		String& operator+= (const char* pStr)
		{
			return append(String(pStr));
		}

		template<int B>	String& operator+= (const String<B>& b)
		{
			return append(b);
		}
		
		friend Stream& operator<< (Stream& dst, const String& src)
		{
			dst.writeObj(src.len()).writeBytes(src.mBuf, src.len());
			return dst;
		}
		
		friend Stream& operator>> (Stream& src, String& dst)
		{
			int len;
			src >> len;
			Verify((len >= 0) && (len <= dst.maxLen()));
			dst.setTerminator(len);
			src.readBytes(dst.mBuf, len);
			return src;
		}
		
		Stream& readLine(Stream& src)
		{
			const int maxLen = this->maxLen();
			
			int ofs = 0;
			bool firstChar = true;
			while (ofs < maxLen)
			{
				const int c = src.getc();

				// 0D 0A \r\n
				if ((c < 0) || (c == '\r')) 
					break;
				else if (c == '\n')
				{
					if (!firstChar)
						break;
				}
				else
					(*this)[ofs++] = c;
				
				firstChar = false;
			}

			setTerminator(ofs);
	    
			return src;
		}

		// TODO: Add more methods that safely operate on the string data.

	private:
		char mBuf[BufSize];

		template<class DstType>
		static inline DstType& safeTerminator(DstType& dst, int ofs)
		{
			dst[ofs] = '\0';
			return dst;
		}

		template<class DstType, class SrcType>
		static inline DstType& safeCharMove(
			DstType& dst, int dstOfs,
			const SrcType& src, int srcOfs, 
			int len,
			bool terminate = false)
		{
			DebugRange<int>(srcOfs, sizeof(src));
			DebugRange<int>(dstOfs, sizeof(dst));
			Assert(len >= 0);
			
			if (len > 0)
			{
				const int maxLen = Math::Min<int>(sizeof(src) - srcOfs, sizeof(dst) - dstOfs);
				Assert(len <= maxLen);
				memmove(static_cast<char*>(dst) + dstOfs, static_cast<const char*>(src) + srcOfs, len);
			}

			safeTerminator(dst, dstOfs + len);
			
			return dst;
		}
	};

	typedef String<64> SmallString;
	typedef String<256> BigString;
	
} // namespace gr











