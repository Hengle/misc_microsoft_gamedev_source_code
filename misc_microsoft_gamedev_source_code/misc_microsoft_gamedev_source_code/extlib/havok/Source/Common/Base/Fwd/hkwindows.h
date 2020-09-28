/* 
 * 
 * Confidential Information of Telekinesys Research Limited (t/a Havok).  Not for disclosure or distribution without Havok's
 * prior written consent.This software contains code, techniques and know-how which is confidential and proprietary to Havok.
 * Level 2 and Level 3 source code contains trade secrets of Havok. Havok Software (C) Copyright 1999-2007 Telekinesys Research Limited t/a Havok. All Rights Reserved. Use of this software is subject to the terms of an end user license agreement.
 * 
 */
#ifndef HK_BASE_WINDOWS_H
#define HK_BASE_WINDOWS_H

// remember if we defined symbols so we can reset them afterward

#ifndef _WIN32_WINNT
#	define _WIN32_WINNT 0x0403
#	define HK_DEFINED_WIN32_WINNT
#endif
#ifndef WIN32_LEAN_AND_MEAN
#	define WIN32_LEAN_AND_MEAN
#	define HK_DEFINED_WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>

#ifdef HK_DEFINED_WIN32_WINNT
#	undef _WIN32_WINNT
#	undef HK_DEFINED_WIN32_WINNT
#endif
#ifdef HK_DEFINED_WIN32_LEAN_AND_MEAN
#	undef WIN32_LEAN_AND_MEAN
#	undef HK_DEFINED_WIN32_LEAN_AND_MEAN
#endif

#endif // HK_BASE_WINDOWS_H

/*
* Havok SDK - PUBLIC RELEASE, BUILD(#20070919)
*
* Confidential Information of Havok.  (C) Copyright 1999-2007 
* Telekinesys Research Limited t/a Havok. All Rights Reserved. The Havok
* Logo, and the Havok buzzsaw logo are trademarks of Havok.  Title, ownership
* rights, and intellectual property rights in the Havok software remain in
* Havok and/or its suppliers.
*
* Use of this software for evaluation purposes is subject to and indicates 
* acceptance of the End User licence Agreement for this product. A copy of 
* the license is included with this software and is also available from salesteam@havok.com.
*
*/
