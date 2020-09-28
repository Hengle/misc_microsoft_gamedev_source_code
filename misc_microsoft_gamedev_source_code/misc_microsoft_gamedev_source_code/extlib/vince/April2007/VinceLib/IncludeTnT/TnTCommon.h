#pragma once

#ifdef _XBOX
#include <xtl.h>
#include <winsockx.h>
#else // NOT _XBOX
#include <windows.h>
#include <winsock.h>
#endif // _XBOX

#include <vector>
#include <string>
