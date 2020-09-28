//==============================================================================
// Pair.hpp
//
// Copyright (c) 2000-2003, Ensemble Studios
//==============================================================================
#ifndef ___Pair___
#define ___Pair___

namespace Etl
{

template <class T1, class T2>
struct Pair
{
public:
   Pair() {}
   Pair(const T1& f) : first(f) {}
   Pair(const T1& f, const T2& s) : first(f), second(s) {}
   inline Pair<T1, T2> operator = (const Pair<T1, T2>& p) 
   { first = p.first; second = p.second; return *this; }
   T1 first;
   T2 second;
};

}

#endif
