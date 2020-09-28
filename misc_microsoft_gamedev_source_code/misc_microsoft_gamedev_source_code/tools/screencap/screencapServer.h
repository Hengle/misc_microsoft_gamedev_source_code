// File: screencapServer.h
#pragma once

class BScreenCapServer
{
public:
   static bool          init(void);
   static bool          deinit(void);
   
   static bool          getInitialized(void);
   
   static unsigned int  getNumServerAddresses(void);
   static const char*   getServerAddress(unsigned int index);
         
   static bool          beginCapture(const char* pServerAddress, float updateRate);
   static bool          endCapture(void);
   
   static bool          getCapturing(void);
};





