// File: clientAdvertisement.h
#pragma once

class BClientAdvertisementManager
{
public:
   static bool create(const char* pAddress);
   static bool destroy(void);
   static bool discover(BDynamicArray<BString>& clients);
   
private:
   static BCriticalSection mMutex;
   static BString          mClientAdvertisementFilename;   
}; 
