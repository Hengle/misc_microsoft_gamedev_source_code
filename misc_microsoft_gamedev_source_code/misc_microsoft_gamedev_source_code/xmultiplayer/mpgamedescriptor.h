//==============================================================================
// mpgamedescriptor.h
//
// Copyright (c) 2003, Ensemble Studios
//==============================================================================

#pragma once

#ifndef __MPGAMEDESCRIPTOR_H__
#define __MPGAMEDESCRIPTOR_H__

//==============================================================================
// Includes
#include "Socket.h"

//==============================================================================
// Forward declarations

//==============================================================================
// Const declarations


//==============================================================================
class BMPGameDescriptor
{
public:
   BMPGameDescriptor();
   BMPGameDescriptor(const BSimString& name, bool local);

   void                 setName(const BSimString& name);
   const BCHAR_T*         getName(void) const;

   void                 setLocal(bool local);
   bool                 getLocal(void) const;

   void                 setPort(long port);
   long                 getPort(void) const;

   void                 setRestoredFilename(const BSimString& filename);
   const BCHAR_T*         getRestoredFilename(void) const;

   void                 setChecksum(DWORD checksum);
   DWORD                getChecksum(void) const;

   void                 setAddress(const SOCKADDR_IN& address);
   const SOCKADDR_IN&   getAddress(void) const;

   void                 setTranslatedAddress(const SOCKADDR_IN& address);
   const SOCKADDR_IN&   getTranslatedAddress(void) const;

   void                 setAdvertiseGame(bool val) { mAdvertise = val; }
   bool                 getAdvertiseGame() const { return mAdvertise; }

   void                 setGameType(long gameType) { mGameType = gameType; }
   long                 getGameType(void) { return mGameType; }

protected:
   bool                 mLocal;
   bool                 mAdvertise;
   long                 mPort;
   DWORD                mChecksum;
   BCHAR_T                mFileName[_MAX_PATH];
   BCHAR_T                mName[_MAX_PATH];
   SOCKADDR_IN          mAddress;
   SOCKADDR_IN          mTranslatedAddress;
   long                 mGameType;
};

typedef BDynamicSimArray<BMPGameDescriptor> BMPGameDescriptorArray;
typedef BDynamicSimArray<BMPGameDescriptor*> BMPGameDescriptorPtrArray;

#endif //__MPGAMEDESCRIPTOR_H__
