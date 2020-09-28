/* 
 * Confidential Information of Telekinesys Research Limited (t/a Havok).  Not for disclosure or distribution without Havok's
 * prior written consent.This software contains code, techniques and know-how which is confidential and proprietary to Havok.
 * Level 2 and Level 3 source code contains trade secrets of Havok. Havok Software (C) Copyright 1999-2007 Telekinesys Research Limited t/a Havok. All Rights Reserved. Use of this software is subject to the terms of an end user license agreement.
 * 
 */
#include <ContentTools/Common/Filters/FilterTutorial/hctFilterTutorial.h>

// Keycode
#include <Common/Base/keycode.cxx>

// DLL main
HINSTANCE hInstance;
static BOOL CommonControlsInitialized = FALSE;

BOOL WINAPI DllMain(HINSTANCE hinstDLL, ULONG fdwReason, LPVOID lpvReserved)
{
	switch ( fdwReason )
	{
		case DLL_PROCESS_ATTACH:
		{
			hInstance = hinstDLL;				// Hang on to this DLL's instance handle.

			if (!CommonControlsInitialized) 
			{

				CommonControlsInitialized = TRUE;
				InitCommonControls();			// Initialize Win32 controls
			}
			break;
		}

		case DLL_PROCESS_DETACH:
		{
			if (CommonControlsInitialized)
			{
				CommonControlsInitialized = FALSE;
			}
		}

		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
			break;
	}		
	return TRUE;
}

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
