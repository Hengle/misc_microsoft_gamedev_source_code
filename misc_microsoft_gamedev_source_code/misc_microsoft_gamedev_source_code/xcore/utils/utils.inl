//============================================================================
//
// File: utils.inl
// Copyright (c) 2005-2006, Ensemble Studios
//
//============================================================================

// Disable PREfast warnings
#pragma warning( push )
#pragma warning( disable : 25042)

namespace Utils 
{
	template<class T> inline void ClearObj(T& obj, int val)
	{
		FastMemSet(&obj, val, sizeof(obj));
	}

	template<class T> inline bool IsAligned(T p, uint alignment) 
	{ 
		return 0 == (reinterpret_cast<uint>(p) & (alignment - 1)); 
	}
	
   inline bool IsValueAligned(uint p, uint alignment) 
   { 
      return 0 == (p & (alignment - 1)); 
   }
	
	template<class T> inline T AlignUp(T p, uint alignment)
	{
		BDEBUG_ASSERT(Math::IsPow2(alignment));
		BCOMPILETIMEASSERT(sizeof(p) == sizeof(uint));
		uint temp = reinterpret_cast<uint>(p);
		temp = (temp + alignment - 1) & (~(alignment - 1));
		return reinterpret_cast<T>(temp);
	}
	
   template<class T> inline T AlignDown(T p, uint alignment)
   {
      BDEBUG_ASSERT(Math::IsPow2(alignment));
      BCOMPILETIMEASSERT(sizeof(p) == sizeof(uint));
      uint temp = reinterpret_cast<uint>(p);
      temp = temp & (~(alignment - 1));
      return reinterpret_cast<T>(temp);
   }
	
   template<class T> inline T AlignUpValue(T p, size_t alignment)
   {
      unsigned int temp = static_cast<unsigned int>(p);
      temp = (temp + alignment - 1) & (~(alignment - 1));
      return static_cast<T>(temp);
   }
   
   template<class T> inline T BytesToAlignUpValue(T p, size_t alignment)
   {
      return AlignUpValue(p, alignment) - p;
   }
	
   template<class T> inline T RoundUp(T p, uint alignment)
   {
      BDEBUG_ASSERT(Math::IsPow2(alignment));
      uint temp = p;
      temp = (temp + alignment - 1) & (~(alignment - 1));
      return temp;
   }
	
	template<class T> inline int BytesToAlignUp(T p, uint alignment)
	{
		return (uchar*)AlignUp(p, alignment) - (uchar*)p;
	}
	
	template<class T> inline void* WriteValue(__out_bcount_full_opt(sizeof(T)) void* pDst, const T& v)
	{
      const uchar* pSrcBytes = reinterpret_cast<const uchar*>(&v);
      uchar* pDstBytes = reinterpret_cast<uchar*>(pDst);

      for (uint i = 0; i < sizeof(T); i++)
         pDstBytes[i] = pSrcBytes[i];
         
		return reinterpret_cast<T*>(pDst) + 1; 
	}
	
   template<> inline void* WriteValue<BYTE>(void* pDst, const BYTE& v)
   {
      const uchar* pSrcBytes = reinterpret_cast<const uchar*>(&v);
      uchar* pDstBytes = reinterpret_cast<uchar*>(pDst);

      pDstBytes[0] = pSrcBytes[0];

      return reinterpret_cast<BYTE*>(pDst) + 1; 
   }
	
   template<> inline void* WriteValue<WORD>(void* pDst, const WORD& v)
   {
      const uchar* pSrcBytes = reinterpret_cast<const uchar*>(&v);
      uchar* pDstBytes = reinterpret_cast<uchar*>(pDst);

      pDstBytes[0] = pSrcBytes[0];
      pDstBytes[1] = pSrcBytes[1];

      return reinterpret_cast<WORD*>(pDst) + 1; 
   }
	
   template<> inline void* WriteValue<DWORD>(void* pDst, const DWORD& v)
   {
      const uchar* pSrcBytes = reinterpret_cast<const uchar*>(&v);
      uchar* pDstBytes = reinterpret_cast<uchar*>(pDst);
      
      pDstBytes[0] = pSrcBytes[0];
      pDstBytes[1] = pSrcBytes[1];
      pDstBytes[2] = pSrcBytes[2];
      pDstBytes[3] = pSrcBytes[3];

      return reinterpret_cast<DWORD*>(pDst) + 1; 
   }
	
   template<class T> inline void* WriteValueByteSwapped(__out_bcount_full_opt(sizeof(T)) void* pDst, const T& v)
   {
      const uchar* pSrcBytes = reinterpret_cast<const uchar*>(&v);
      uchar* pDstBytes = reinterpret_cast<uchar*>(pDst);
      
      for (uint i = 0; i < sizeof(T); i++)
         pDstBytes[i] = pSrcBytes[sizeof(T) - 1 - i];
      
      return reinterpret_cast<T*>(pDst) + 1; 
   }
	
   template<class T> inline void* WriteValueLittleEndian(void* pDst, const T& v)
   {
      if (cLittleEndianNative)
         return WriteValue(pDst, v);
      else
         return WriteValueByteSwapped(pDst, v);
   }
	
   template<class T> inline void* WriteValueBigEndian(void* pDst, const T& v)
   {
      if (cLittleEndianNative)
         return WriteValueByteSwapped(pDst, v);
      else
         return WriteValue(pDst, v);
   }
   
   template<class T> inline void* WriteValue(void* pDst, const T& v, bool bigEndian)
   {
      if (bigEndian)
         return WriteValueBigEndian(pDst, v);
      else
         return WriteValueLittleEndian(pDst, v);
   }
   
   template<class T> inline const void* ReadValue(const void* pSrc, T& v)
   {
      const uchar* pSrcBytes = reinterpret_cast<const uchar*>(pSrc);
      uchar* pDstBytes = reinterpret_cast<uchar*>(&v);

      for (uint i = 0; i < sizeof(T); i++)
         pDstBytes[i] = pSrcBytes[i];

      return reinterpret_cast<const T*>(pSrc) + 1; 
   }
   
   template<> inline const void* ReadValue<BYTE>(const void* pSrc, BYTE& v)
   {
      const uchar* pSrcBytes = reinterpret_cast<const uchar*>(pSrc);
      uchar* pDstBytes = reinterpret_cast<uchar*>(&v);

      pDstBytes[0] = pSrcBytes[0];

      return reinterpret_cast<const BYTE*>(pSrc) + 1; 
   }
   
   template<> inline const void* ReadValue<WORD>(const void* pSrc, WORD& v)
   {
      const uchar* pSrcBytes = reinterpret_cast<const uchar*>(pSrc);
      uchar* pDstBytes = reinterpret_cast<uchar*>(&v);

      pDstBytes[0] = pSrcBytes[0];
      pDstBytes[1] = pSrcBytes[1];

      return reinterpret_cast<const WORD*>(pSrc) + 1; 
   }
   
   template<> inline const void* ReadValue<DWORD>(const void* pSrc, DWORD& v)
   {
      const uchar* pSrcBytes = reinterpret_cast<const uchar*>(pSrc);
      uchar* pDstBytes = reinterpret_cast<uchar*>(&v);

      pDstBytes[0] = pSrcBytes[0];
      pDstBytes[1] = pSrcBytes[1];
      pDstBytes[2] = pSrcBytes[2];
      pDstBytes[3] = pSrcBytes[3];

      return reinterpret_cast<const DWORD*>(pSrc) + 1; 
   }
   
   template<class T> inline const void* ReadValueByteSwapped(const void* pSrc, T& v)
   {
      const uchar* pSrcBytes = reinterpret_cast<const uchar*>(pSrc);
      uchar* pDstBytes = reinterpret_cast<uchar*>(&v);

      for (uint i = 0; i < sizeof(T); i++)
         pDstBytes[i] = pSrcBytes[sizeof(T) - 1 - i];

      return reinterpret_cast<const T*>(pSrc) + 1; 
   }
   
   template<class T> inline const void* ReadValueLittleEndian(const void* pSrc, T& v)
   {
      if (cLittleEndianNative)
         return ReadValue(pSrc, v);
      else
         return ReadValueByteSwapped(pSrc, v);
   }
   
   template<class T> inline const void* ReadValueBigEndian(const void* pSrc, T& v)
   {
      if (cLittleEndianNative)
         return ReadValueByteSwapped(pSrc, v);
      else
         return ReadValue(pSrc, v);
   }
                  
   template<class T> inline const void* ReadValue(const void* pSrc, T& v, bool bigEndian)
   {
      if (bigEndian)
         return ReadValueBigEndian(pSrc, v);
      else
         return ReadValueLittleEndian(pSrc, v);
   }
   
   template<class T> inline const T GetValue(const void* pSrc)                    
   { 
      T temp; ReadValue(pSrc, temp); return temp; 
   }
   
   template<class T> inline const T GetValue(const void* pSrc, bool bigEndian)    
   { 
      T temp; ReadValue(pSrc, temp, bigEndian); return temp; 
   }
   
   template<class T> inline const T GetValueByteSwapped(const void* pSrc)         
   { 
      T temp; ReadValueByteSwapped(pSrc, temp); return temp; 
   }
   
   template<class T> inline const T GetValueLittleEndian(const void* pSrc)        
   { 
      T temp; ReadValueLittleEndian(pSrc, temp); return temp; 
   }
   
   template<class T> inline const T GetValueBigEndian(const void* pSrc)           
   { 
      T temp; ReadValueBigEndian(pSrc, temp); return temp; 
   }
   
   inline WORD CreateWORD(BYTE lo, BYTE hi) 
   { 
      return lo | ((WORD)hi << 8); 
   }
   
   inline DWORD CreateDWORD(BYTE lo, BYTE b, BYTE c, BYTE hi) 
   { 
      return lo | ((DWORD)b << 8) | ((DWORD)c << 16) | ((DWORD)hi << 24); 
   }
   
   inline uint ExtractLowByteFromWORD(WORD w) { return w & 0xFF; }
   
   inline uint ExtractHighByteFromWORD(WORD w) { return (w >> 8) & 0xFF; }
   
   template<class T> inline void* writeObj(void* pDst, const T& obj)
   {
      const uint size = sizeof(obj);
      FastMemCpy(pDst, &obj, size);
      return static_cast<uchar*>(pDst) + size;
   }
   
   template<class T> inline const void* readObj(const void* pSrc, T& obj)
   {
      const uint size = sizeof(obj);
      FastMemCpy(&obj, pSrc, size);
      return static_cast<const uchar*>(pSrc) + size;
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
	
   template<typename T> inline void BubbleSort(T pStart, T pEnd)
   {
      BDEBUG_ASSERT(pEnd >= pStart);

      const uint size = pEnd - pStart;

      for (uint j = 1; j < size; j++)
      {
         bool swapFlag = false;
         for (uint i = 0; i < size - j; i++)
         {
            if (pStart[i] > pStart[i + 1])
            {
               std::swap(pStart[i], pStart[i + 1]);
               swapFlag = true;
            }
         }
         if (!swapFlag)
            break;
      }           
   }
   
   template<typename T, typename F> inline void BubbleSortElemFunc(T pStart, T pEnd, F elemFunc)
   {
      BDEBUG_ASSERT(pEnd >= pStart);

      const uint size = pEnd - pStart;

      for (int j = 1; j < size; j++)
      {
         bool swapFlag = false;
         for (int i = 0; i < size - j; i++)
         {
            if (elemFunc(pStart[i]) > elemFunc(pStart[i + 1]))
            {
               std::swap(pStart[i], pStart[i + 1]);
               swapFlag = true;
            }
         }
         if (!swapFlag)
            break;
      }           
   }
   
   template<typename T, typename F> inline void BubbleSortPredFunc(T pStart, T pEnd, F predFunc)
   {
      BDEBUG_ASSERT(pEnd >= pStart);

      const uint size = pEnd - pStart;

      for (int j = 1; j < size; j++)
      {
         bool swapFlag = false;
         for (int i = 0; i < size - j; i++)
         {
            if (predFunc(pStart[i + 1], pStart[i]))
            {
               std::swap(pStart[i], pStart[i + 1]);
               swapFlag = true;
            }
         }
         if (!swapFlag)
            break;
      }           
   }
   
   template<typename T> inline void ShellSort(T pStart, T pEnd) 
   {
      BDEBUG_ASSERT(pEnd >= pStart);
      const int n = pEnd - pStart;

      int h = 1;
      if (n >= 16)
      {
         while (h < n) h = 3 * h + 1;
         h /= 3; h /= 3;
      }

      while (h > 0) 
      {
         for (int i = h; i < n; i++) 
            for (int j = i - h; (j >= 0) && (pStart[j] > pStart[j + h]); j -= h)
               std::swap(pStart[j + h], pStart[j]);
         h /= 3;
      }
   }

   template<typename T, typename F> inline void ShellSortElemFunc(T pStart, T pEnd, F elemFunc) 
   {
      BDEBUG_ASSERT(pEnd >= pStart);
      const int n = pEnd - pStart;

      int h = 1;
      if (n >= 16)
      {
         while (h < n) h = 3 * h + 1;
         h /= 3; h /= 3;
      }

      while (h > 0) 
      {
         for (int i = h; i < n; i++) 
            for (int j = i - h; (j >= 0) && (elemFunc(pStart[j]) > elemFunc(pStart[j + h])); j -= h)
               std::swap(pStart[j + h], pStart[j]);
         h /= 3;
      }
   }

   template<typename T, typename F> inline void ShellSortPredFunc(T pStart, T pEnd, F predFunc) 
   {
      BDEBUG_ASSERT(pEnd >= pStart);
      const int n = pEnd - pStart;

      int h = 1;
      if (n >= 16)
      {
         while (h < n) h = 3 * h + 1;
         h /= 3; h /= 3;
      }

      while (h > 0) 
      {
         for (int i = h; i < n; i++) 
            for (int j = i - h; (j >= 0) && (predFunc(pStart[j + h], pStart[j])); j -= h)
               std::swap(pStart[j + h], pStart[j]);
         h /= 3;
      }
   }
   
   inline bool IsVirtualMemory(void* p, uint len)
   {
#ifdef XBOX    
      if ( ((uint)p + len - 1) & 0x80000000 )
         return false;
         
      return true;
#else
      p;
      len;
      return true;
#endif
   }
   
   // This function should only be used with cached memory!
   // Do not use this function to copy into write combined memory (dynamic VB, IB, etc.).
   inline PVOID FastMemCpy(__out_bcount_full_opt(count) PVOID dst, __in_bcount_opt(count) const VOID *src, SIZE_T count)
   {
#ifdef XBOX
      if (IsVirtualMemory(dst, count))
         return XMemCpy(dst, src, count);
      else
         return memcpy(dst, src, count);
#else
      return memcpy(dst, src, count);
#endif      
   }
   
   // This function should only be used with cached memory!
   inline PVOID FastMemSet(__out_bcount_full_opt(count) PVOID dest, int c, SIZE_T count)
   {
#ifdef XBOX
      if (IsVirtualMemory(dest, count))
         return XMemSet(dest, c, count);
      else
         return memset(dest, c, count);
#else
      return memset(dest, c, count);
#endif   
   }
   
   inline void TouchCacheLine(int offset, const void* base)
   {
      offset;
      base;

#ifdef XBOX      
      __dcbt(offset, base);
#endif
   }
   
   inline BPrefetchState BeginPrefetch(const void* p, uint cacheLinesToReadAhead)
   {
      BPrefetchState state = Utils::AlignDown(p, 128);   
      TouchCacheLine(cacheLinesToReadAhead << 7, p);
      return state;
   }
   
   inline BPrefetchState BeginPrefetch(const void* p, const void* pEnd, uint cacheLinesToReadAhead)
   {
      BPrefetchState state = Utils::AlignDown(p, 128);  
      const uint ofs = (cacheLinesToReadAhead << 7);
      if (((const uchar*)state + ofs) < (const uchar*)pEnd)
         TouchCacheLine(ofs, p);
      return state;
   }
   
   inline BPrefetchState UpdatePrefetch(BPrefetchState state, const void* p, uint cacheLinesToReadAhead)
   {
      BPrefetchState newState = Utils::AlignDown(p, 128);
      if (newState > state) 
         TouchCacheLine(cacheLinesToReadAhead << 7, p);
      return newState;
   }
   
   inline BPrefetchState UpdatePrefetch(BPrefetchState state, const void* p, const void* pEnd, uint cacheLinesToReadAhead)
   {
      BPrefetchState newState = Utils::AlignDown(p, 128);
      if (newState > state) 
      {
         const uint ofs = (cacheLinesToReadAhead << 7);
         if (((const uchar*)newState + ofs) < (const uchar*)pEnd)
            TouchCacheLine(ofs, p);
      }
      return newState;
   }

   inline BPrefetchState BeginPrefetchLargeStruct(const void* p, const void* pEnd, uint cacheLinesToReadAhead)
   {
      BPrefetchState state = Utils::AlignDown(p, 128);  

      const uint ofs = (cacheLinesToReadAhead << 7);

      if (((const uchar*)state + ofs) < (const uchar*)pEnd)
      {
         TouchCacheLine(ofs, state);

         state = (const uchar*)state + ofs + 128;
      }

      return state;
   }
   
   inline BPrefetchState BeginPrefetchLargeStruct(const void* p, uint cacheLinesToReadAhead)
   {
      BPrefetchState state = Utils::AlignDown(p, 128);  

      const uint ofs = (cacheLinesToReadAhead << 7);

      TouchCacheLine(ofs, state);

      state = (const uchar*)state + ofs + 128;
      
      return state;
   }

   inline BPrefetchState UpdatePrefetchLargeStruct(BPrefetchState state, const void* p, const void* pEnd, uint cacheLinesToReadAhead)
   {
      p = Utils::AlignDown(p, 128);

      const uint ofs = (cacheLinesToReadAhead << 7);
      const uchar* pDesiredState = (const uchar*)p + ofs;

      if (pDesiredState > state)
      {
         if ((uchar*)p > (uchar*)state)
         {
            state = (uchar*)p + ofs;
            pDesiredState = (const uchar*)state + ofs;
         }

         uint numBytes = (uint)((const uchar*)pDesiredState - (const uchar*)state);

         for (uint i = 0; i < numBytes; i += 128)
         {
            const uchar* pTouch = i + (const uchar*)state;

            if (pTouch < (const uchar*)pEnd)
            {
               TouchCacheLine(0, pTouch);
            }
         }        

         state = pDesiredState;          
      }

      return state;
   }   

   inline BPrefetchState UpdatePrefetchLargeStruct(BPrefetchState state, const void* p, uint cacheLinesToReadAhead)
   {
      p = Utils::AlignDown(p, 128);

      const uint ofs = (cacheLinesToReadAhead << 7);
      const uchar* pDesiredState = (const uchar*)p + ofs;

      if (pDesiredState > state)
      {
         if ((uchar*)p > (uchar*)state)
         {
            state = (uchar*)p + ofs;
            pDesiredState = (const uchar*)state + ofs;
         }

         uint numBytes = (uint)((const uchar*)pDesiredState - (const uchar*)state);

         for (uint i = 0; i < numBytes; i += 128)
         {
            const uchar* pTouch = i + (const uchar*)state;

            TouchCacheLine(0, pTouch);
         }        

         state = pDesiredState;          
      }

      return state;
   }   
   
   inline bool IsValidBool(const bool& b)
   {
      const uchar* pByte = reinterpret_cast<const uchar*>(&b);
      return (pByte[0] == 0) || (pByte[0] == 1);
   }        		         
   
   inline uint64 GetCounter(void) { LARGE_INTEGER result; QueryPerformanceCounter(&result); return *reinterpret_cast<const uint64*>(&result); }
   inline uint64 GetCounterFrequency(void) { LARGE_INTEGER result; QueryPerformanceFrequency(&result); return *reinterpret_cast<const uint64*>(&result); }
   inline double GetCounterFrequencyAsDouble(void) { LARGE_INTEGER result; QueryPerformanceFrequency(&result); return static_cast<double>(*reinterpret_cast<const uint64*>(&result)); }
   inline double GetCounterOneOverFrequency(void) { return 1.0f / GetCounterFrequencyAsDouble(); }

#pragma warning(disable:4706)   
   inline uint CountLeadingZeros64(uint64 value)
   {
#ifdef XBOX   
      return _CountLeadingZeros64(value);
#else      
      uint64 temp;
      uint result = 64U;

      if (temp = (value >> 32U)) { result -= 32U; value = temp; }
      if (temp = (value >> 16U)) { result -= 16U; value = temp; }
      if (temp = (value >>  8U)) { result -=  8U; value = temp; }
      if (temp = (value >>  4U)) { result -=  4U; value = temp; }
      if (temp = (value >>  2U)) { result -=  2U; value = temp; }
      if (temp = (value >>  1U)) { result -=  1U; value = temp; }
         
      if (value & 1U) 
         result--;

      return result;
#endif
   }

   inline uint CountLeadingZeros32(uint value)
   {
#ifdef XBOX   
      return _CountLeadingZeros64((uint64)value) - 32;
#else      
      uint temp;
      uint result = 32U;

      if (temp = (value >> 16U)) { result -= 16U; value = temp; }
      if (temp = (value >>  8U)) { result -=  8U; value = temp; }
      if (temp = (value >>  4U)) { result -=  4U; value = temp; }
      if (temp = (value >>  2U)) { result -=  2U; value = temp; }
      if (temp = (value >>  1U)) { result -=  1U; value = temp; }

      if (value & 1U) 
         result--;

      return result;
#endif
   }
#pragma warning(default:4706)   
#pragma warning(pop)

   inline uint CountBits(uint value)
   {
      value = ((value & 0xAAAAAAAAL) >>  1) + (value & 0x55555555L);
      value = ((value & 0xCCCCCCCCL) >>  2) + (value & 0x33333333L);
      value = ((value & 0xF0F0F0F0L) >>  4) + (value & 0x0F0F0F0FL);
      value = ((value & 0xFF00FF00L) >>  8) + (value & 0x00FF00FFL);
      value = ((value & 0xFFFF0000L) >> 16) + (value & 0x0000FFFFL);
      return value;
   }
   
} // namespace Utils
