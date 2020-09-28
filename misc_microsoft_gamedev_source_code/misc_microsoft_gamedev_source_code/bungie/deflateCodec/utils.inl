//============================================================================
// utils.inl
// Copyright (c) 1996-2006, Ensemble Studios
//
// rgeldreich@ensemblestudios.com
//============================================================================

namespace ens
{
   namespace Utils 
   {
	   template<class T> inline T AlignUp(T p, uint alignment)
	   {
		   assert(Math::IsPow2(alignment));
		   BCOMPILETIMEASSERT(sizeof(p) == sizeof(uint));
		   uint temp = reinterpret_cast<uint>(p);
		   temp = (temp + alignment - 1) & (~(alignment - 1));
		   return reinterpret_cast<T>(temp);
	   }
   	
      template<class T> inline T AlignDown(T p, uint alignment)
      {
         assert(Math::IsPow2(alignment));
         BCOMPILETIMEASSERT(sizeof(p) == sizeof(uint));
         uint temp = reinterpret_cast<uint>(p);
         temp = temp & (~(alignment - 1));
         return reinterpret_cast<T>(temp);
      }
      
      // This function should only be used with cached memory!
      // Do not use this function to copy into write combined memory (dynamic VB, IB, etc.).
      inline PVOID FastMemCpy(PVOID dst, const VOID *src, SIZE_T count)
      {
   #ifdef _XBOX
         return XMemCpy(dst, src, count);
   #else
         return memcpy(dst, src, count);
   #endif      
      }
      
      // This function should only be used with cached memory!
      inline PVOID FastMemSet(PVOID dest, int c, SIZE_T count)
      {
   #ifdef _XBOX
         return XMemSet(dest, c, count);
   #else
         return memset(dest, c, count);
   #endif   
      }
      
      inline void TouchCacheLine(int offset, const void* base)
      {
         offset;
         base;

   #ifdef _XBOX      
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
               		         
   } // namespace Utils
   
} // namespace ens