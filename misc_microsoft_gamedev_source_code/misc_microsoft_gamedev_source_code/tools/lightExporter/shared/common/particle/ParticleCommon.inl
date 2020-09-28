// Particle System Simulator -- Aaron Hill
// Copyright 2003 Blank Cartridge, Inc.

#ifndef _COMMON_INL_
#define _COMMON_INL_

// Macros.


// FIXME: Yikes! Also defined in dxutil.h
#ifndef SAFE_DELETE
	#define SAFE_DELETE(p) { delete (p); (p) = NULL; }
#endif

#ifndef SAFE_DELETE_ARRAY
	#define SAFE_DELETE_ARRAY(p) { delete [] (p); (p) = NULL; }
#endif

#ifndef SAFE_RELEASE
	#define SAFE_RELEASE(p) { if ((p) != NULL) { (p)->Release(); (p) = NULL; } }
#endif

// Inlines.

#endif /* _COMMON_INL_ */