//==============================================================================
// Support.hpp
//
// Copyright (c) 2000-2003, Ensemble Studios
//==============================================================================
#ifndef __Support__
#define __Support__

#include <Etl\Pair.hpp>

namespace Etl
{
template <class T>
inline T Max(const T& a, const T& b) { return (a > b) ? a : b; }

template <class T>
inline T Min(const T& a, const T& b) { return (a < b) ? a : b; }

template <class T>
inline void Swap(T& a, T& b)
{
   T tmp = a; a = b; b = tmp;
}


struct EqualKeyStr
{
   inline bool operator() (const char* a, const char* b) { return (strcmp(a, b) == 0);}
};

template <class T>
struct EqualKeyDefault
{
   inline bool operator() (const T& a, const T& b) const { return (a == b); }
};

template <class T>
struct CompareKeyDefault
{
   inline long operator() (const T& a, const T& b) const
   {
      if (a < b) return -1;
      if (a > b) return 1;
      return 0;
   }
};


struct CompareKeyString
{
   inline long operator() (const char* s1, const char* s2) const { return strcmp(s1, s2); }
};

#ifdef _UNICODE
struct CompareKeyWideString
{
   inline long operator() (const WCHAR* s1, const WCHAR* s2) const { return wcscmp(s1, s2); }
};
#endif

template <class Key, class T, class Cmp>
struct PtrPairKeyCompareDefault
{
   inline long operator() (const Etl::Pair<Key, T>* a, const Etl::Pair<Key, T>* b) const
   {
      Cmp cmp;

      if (cmp(a->first, b->first) < 0) return -1;
      if (cmp(a->first, b->first) > 0) return 1;
      return 0;
   }
};


}

#endif
