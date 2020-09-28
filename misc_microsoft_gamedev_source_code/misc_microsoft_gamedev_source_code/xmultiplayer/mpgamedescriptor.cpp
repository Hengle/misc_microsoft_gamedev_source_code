//==============================================================================
// mpgamedescriptor.cpp
//
// Copyright (c) 2003, Ensemble Studios
//==============================================================================

// Includes
#include "multiplayercommon.h"
#include "mpgamedescriptor.h"
//#include "strHelper.h"

//==============================================================================
// Defines

//==============================================================================
// BMPGameDescriptor::BMPGameDescriptor
//==============================================================================
BMPGameDescriptor::BMPGameDescriptor() :
   mPort(0),
   mChecksum(0),
   mLocal(true),
   mAdvertise(true),
   mGameType(0)
{
   memset(&mAddress, 0, sizeof(mAddress));
   memset(&mTranslatedAddress, 0, sizeof(mTranslatedAddress));
   memset(mName, 0, sizeof(mName));
   memset(mFileName, 0, sizeof(mFileName));
}

//==============================================================================
// BMPGameDescriptor::BMPGameDescriptor
//==============================================================================
BMPGameDescriptor::BMPGameDescriptor(const BSimString& name, bool local) : 
   mPort(0),
   mChecksum(0),
   mLocal(local),
   mAdvertise(true),
   mGameType(0)
{
   setName(name);
   memset(&mAddress, 0, sizeof(mAddress));
   memset(&mTranslatedAddress, 0, sizeof(mTranslatedAddress));
   memset(mFileName, 0, sizeof(mFileName));
}

//==============================================================================
// BMPGameDescriptor::setName
//==============================================================================
void BMPGameDescriptor::setName(const BSimString& name)
{
   if (name.length() <= 0)
      mName[0] = 0;
   else
   {   
      strCopy(mName, _MAX_PATH, name.getPtr());
      mName[_MAX_PATH-1] = 0;
   }
}

//==============================================================================
// BMPGameDescriptor::getName
//==============================================================================
const BCHAR_T* BMPGameDescriptor::getName(void) const
{
   return(mName);
}

//==============================================================================
// BMPGameDescriptor::setLocal
//==============================================================================
void BMPGameDescriptor::setLocal(bool local)
{
   mLocal = local;
}

//==============================================================================
// BMPGameDescriptor::getLocal
//==============================================================================
bool BMPGameDescriptor::getLocal(void) const
{
   return(mLocal);
}

//==============================================================================
// BMPGameDescriptor::setChecksum
//==============================================================================
void BMPGameDescriptor::setChecksum(DWORD checksum)
{
   mChecksum = checksum;
}

//==============================================================================
// BMPGameDescriptor::setPort
//==============================================================================
DWORD BMPGameDescriptor::getChecksum(void) const
{
   return(mChecksum);
}

//==============================================================================
// BMPGameDescriptor::setPort
//==============================================================================
void BMPGameDescriptor::setPort(long port)
{
   mPort = port;
}

//==============================================================================
// BMPGameDescriptor::getPort
//==============================================================================
long BMPGameDescriptor::getPort(void) const
{
   return(mPort);
}

//==============================================================================
// BMPGameDescriptor::setRestoredFilename
//==============================================================================
void BMPGameDescriptor::setRestoredFilename(const BSimString& filename)
{
   if (filename.length() <= 0)
      mFileName[0] = 0;
   else
   {   
      strCopy(mFileName, _MAX_PATH, filename.getPtr());
      mFileName[_MAX_PATH-1] = 0;
   }
}

//==============================================================================
// BMPGameDescriptor::getRestoredFilename
//==============================================================================
const BCHAR_T* BMPGameDescriptor::getRestoredFilename(void) const
{
   return mFileName;
}

//==============================================================================
// BMPGameDescriptor::setAddress
//==============================================================================
void BMPGameDescriptor::setAddress(const SOCKADDR_IN& address)
{
   mAddress = address;
}

//==============================================================================
// BMPGameDescriptor::getAddress
//==============================================================================
const SOCKADDR_IN& BMPGameDescriptor::getAddress(void) const
{
   return(mAddress);
}

//==============================================================================
// BMPGameDescriptor::setTransloatedAddress
//==============================================================================
void BMPGameDescriptor::setTranslatedAddress(const SOCKADDR_IN& address)
{
   mTranslatedAddress = address;
}

//==============================================================================
// BMPGameDescriptor::getTranslatedAddress
//==============================================================================
const SOCKADDR_IN& BMPGameDescriptor::getTranslatedAddress(void) const
{
   return(mTranslatedAddress);
}
