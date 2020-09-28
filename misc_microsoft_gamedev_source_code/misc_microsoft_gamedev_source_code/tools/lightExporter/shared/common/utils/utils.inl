//-----------------------------------------------------------------------------
// File: utils.inl
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

namespace gr 
{
	namespace Utils 
	{
		template<class T> inline void ClearObj(T& obj)
		{
			memset(&obj, 0, sizeof(obj));
		}

		template<class T> inline bool IsAligned(T p, size_t alignment) 
		{ 
			return 0 == (reinterpret_cast<uint>(p) & (alignment - 1)); 
		}
	  
		template<class T> inline T AlignUp(T p, size_t alignment)
		{
			Assert(Math::IsPow2(alignment));
			uint temp = reinterpret_cast<uint>(p);
			temp = (temp + alignment - 1) & (~(alignment - 1));
			return reinterpret_cast<T>(temp);
		}
	   
		template<class T> inline int BytesToAlignUp(T p, size_t alignment)
		{
			return AlignUp(p, alignment) - p;
		}
		
		template<class T> inline void* WriteValue(void* pDst, const T& v)
		{
			T* p = reinterpret_cast<T*>(pDst);
			*p++ = v;
			return p;
		}
		
		inline int ConvertHexChar(int c)
		{
			if ((c >= '0') && (c <= '9'))
				return c - '0';
			else if ((c >= 'A') && (c <= 'F'))
				return c - 'A' + 10;
			else if ((c >= 'a') && (c <= 'f'))
				return c - 'a' + 10;
			
			return -1;
		}
	} // namespace Utils
} // namespace gr
