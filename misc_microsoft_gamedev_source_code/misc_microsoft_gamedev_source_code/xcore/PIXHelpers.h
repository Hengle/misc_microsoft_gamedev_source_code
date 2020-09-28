// File: PIXHelpers.h
#pragma once

#ifdef XBOX
   #define PIXSetMarkerNoColor(x, ...) PIXSetMarker(0xFFFFFFFF, x, ##__VA_ARGS__)
   #define PIXBeginNamedEventNoColor(x, ...) PIXBeginNamedEvent(0xFFFFFFFF, x, ##__VA_ARGS__)
#else
   // Disable PIX macros.
   #define PIXBeginNamedEvent 0&&
   #define PIXEndNamedEvent()
   #define PIXSetMarker 0&&
   #define PIXNameThread 0&&
   #define PIXSetMarkerNoColor 0&&
   #define PIXBeginNamedEventNoColor 0&&
   #define PIXSetMarkerNoColor 0&&
   #define PIXBeginNamedEventNoColor 0&& 
#endif

struct BScopedPIXNamedEvent
{
   BScopedPIXNamedEvent(const char* pName, DWORD color = 0xFFFFFFFF)
   {
      pName;
      color;
      PIXBeginNamedEvent(color, pName);
   }

   ~BScopedPIXNamedEvent()
   {
      PIXEndNamedEvent();
   }
};
