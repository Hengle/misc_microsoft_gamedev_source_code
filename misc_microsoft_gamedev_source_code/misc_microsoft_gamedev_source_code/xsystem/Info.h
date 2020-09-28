#pragma once

class BInfo 
{
public:

   enum { MaxNicCount = 20 };

   BInfo();

   HRESULT initialize(void);
   long    getInterfaceCount(void);
   DWORD   getInterfaceAddress(long lIndex);
   DWORD   getInterfaceBroadcastAddress(long lIndex);
   DWORD   getInterfaceNetmask(long lIndex);
   bool    isUp(long lIndex);
   bool    isBroadcastSupported(long lIndex);
   bool    isLoopback(long lIndex);
   bool    isMulicastSupported(long lIndex);
   bool    isPPP(long lIndex);

private:

   //FIXME - May need replacement for Xbox
#ifndef XBOX
   INTERFACE_INFO m_InterfaceList[MaxNicCount];
#endif
   long           m_lCount;

};


