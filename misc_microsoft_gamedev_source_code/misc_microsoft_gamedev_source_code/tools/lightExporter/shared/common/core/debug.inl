//-----------------------------------------------------------------------------
// File: debug.inl
// Debug related helper functions.
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
namespace gr
{

#if DEBUG
	template<class T> inline T DebugNull(T i)
  {
    Assert(i);
    return i;
  }
  template<class T, class U> inline T DebugRange(T i, U l, U h)
  {
    Assert(i >= l);
    Assert(i < h);
    return i;
  }
  template<class T, class U> inline T DebugRange(T i, U h)
  {
    Assert(i >= 0);
    Assert(i < h);
    return i;
  }
  template<class T, class U> inline T DebugRangeIncl(T i, U l, U h)
  {
    Assert(i >= l);
    Assert(i <= h);
    return i;
  }
  template<class T, class U> inline T DebugRangeIncl(T i, U h)
  {
    Assert(i >= 0);
    Assert(i <= h);
    return i;
  }
#else
	template<class T> inline T DebugNull(T i)
  {
    return i;
  }
  template<class T, class U> inline T DebugRange(T i, U l, U h)
  {
    return i;
  }
  template<class T, class U> inline T DebugRange(T i, U h)
  {
    return i;
  }
  template<class T, class U> inline T DebugRangeIncl(T i, U l, U h)
  {
    return i;
  }
  template<class T, class U> inline T DebugRangeIncl(T i, U h)
  {
    return i;
  }
#endif

} // namespace gr


