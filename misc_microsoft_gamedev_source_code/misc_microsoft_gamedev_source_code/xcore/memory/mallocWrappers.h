// File: mallocWrappers.h
#pragma once

// On 360 these functions are simple wrappers around malloc, free, _expand, and _msize.
// On the PC they allocate a bit extra to guarantee alignment because 2005's C RTL on the PC
// doesn't support alignment greater than 8.
// The maximum supported alignment is 16.
void* alignedMalloc(uint size, uint alignment = 0);
void  alignedFree(void* p);
void* alignedRealloc(void* p, uint newSize, uint alignment = 0, bool move = true);
// alignedExpand is the same as calling alignedRealloc() with move set to false.
void* alignedExpand(void* p, uint newSize, uint alignment = 0);
uint  alignedMSize(void* p);
