//==============================================================================
// Hash.hpp
//
// Copyright (c) 2000-2003, Ensemble Studios
//==============================================================================
#ifndef ___Hash___
#define ___Hash___

template <class Key>
struct Hash
{
   long operator () (const Key& k)
   {
      char *pchPos = reinterpret_cast<unsigned char*>(const_cast<Key*>(&k));
      long lValue = 0;

      for (long lIndex = 0; lIndex < sizeof(k); lIndex++)
      {
         long lTmp;

         lValue = (lValue << 4) + pchPos[lIndex];
         lTmp = (lValue & 0xf000000000);

         if (lTmp)
         {
            lValue = lValue ^ (lTmp >> 24);
            lValue = lValue ^ lTmp;
         }
      }
      return lValue;
   }
};

struct HashString
{
   long operator () (const char* p)
   {
      char *pchPos = const_cast<char*>(p);
      long lValue = 0;

      while (*pchPos++)
      {
         long lTmp;

         lValue = (lValue << 4) + *pchPos;
         lTmp = (lValue & 0xf0000000);

         if (lTmp)
         {
            lValue = lValue ^ (lTmp >> 24);
            lValue = lValue ^ lTmp;
         }
      }
      return lValue;
   }
};

#endif
