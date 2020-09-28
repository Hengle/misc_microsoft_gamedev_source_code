// win_hdr.h
#pragma once
#ifndef WIN_HDR_H
#define WIN_HDR_H

#ifndef WIN32
	#error undefined WIN32 preprocessor symbol
#endif

#ifndef WINVER
	#define WINVER 0x0400
#endif

#ifndef _WIN32_WINNT 
	#define _WIN32_WINNT 0x0400
#endif

#ifndef NO_WIN32_LEAN_AND_MEAN
	#ifndef WIN32_LEAN_AND_MEAN
	#define WIN32_LEAN_AND_MEAN
	#endif
#endif

#ifndef _WIN32_WINDOWS
	#define _WIN32_WINDOWS 0x0410
#endif

#include <windows.h>

#endif // WIN_HDR_H
