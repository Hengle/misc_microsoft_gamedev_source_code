// File: templateIf.h
#pragma once

template <bool> 
struct BSelector 
{
	template<class T0, class T1> 
	struct BType 
	{ 
		typedef T1 BResultType; 
	};
};

template <> 
struct BSelector<true> 
{
	template<class T0, class T1> 
	struct BType 
	{
		typedef T0 BResultType;
	};
};

// BIf is a magical little pice of template craziness. It's a lot more useful than it first appears.
// BIf<bool, T0, T1>::BResultType is typedef'd to T0 if the bool is true, otherwise it's typedef'd to T1.
// This decision is made at compile time, not run-time, opening up a lot of interesting possibilities.

// T0 on true
template <bool i, class T0, class T1> 
struct BIf 
{
   typedef typename BSelector<i>::template BType<T0, T1>::BResultType BResultType;
};

// Converts an int to a type.
// Example:
// http://aszt.inf.elte.hu/~gsd/halado_cpp/ch06s09.html
template <int v> struct BInt2Type { enum { Value = v }; };
template <typename T> struct BType2Type { typedef T OriginalType; };

