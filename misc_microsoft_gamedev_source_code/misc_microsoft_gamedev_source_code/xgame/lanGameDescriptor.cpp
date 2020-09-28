//==============================================================================
// lanGameDescriptor.cpp
//
// Copyright (c) 2006-2008, Ensemble Studios
//==============================================================================

// Includes
#include "Common.h"
#include "lanGameDescriptor.h"

//==============================================================================
//
//==============================================================================
BLanGameDescriptor::BLanGameDescriptor() :
   mChecksum(0),
   mLocal(false),
   mGameType(0)
{
   Utils::FastMemSet(&mXnAddr, 0, sizeof(mXnAddr));
   Utils::FastMemSet(&mXnKID, 0, sizeof(mXnKID));
   Utils::FastMemSet(&mXnKey, 0, sizeof(mXnKey));
   Utils::FastMemSet(mName, 0, sizeof(mName));
   if (XNetRandom(mNonce, NONCE_SIZE) != 0)
   {
      BASSERT(false);
      //not sure why this would ever fail
   }
   mUpdateTime = timeGetTime();
}

//==============================================================================
//
//==============================================================================
uint64 BLanGameDescriptor::getNonce()
{
//-- FIXING PREFIX BUG ID 3532
   const uint64* tmp = reinterpret_cast<const uint64*>(&mNonce);
//--
   return *tmp;
}

//==============================================================================
//
//==============================================================================
void BLanGameDescriptor::setNonce(uint64 n)
{
   uint64* tmp = (uint64*)&mNonce;
   *tmp = n;
}

//==============================================================================
//
//==============================================================================
BLanGameDescriptor::BLanGameDescriptor(MessageLANDiscoveryResponse* pMsgResponse) :
   mChecksum(0),
   mLocal(true),
   mGameType(0)
{
   BASSERT(pMsgResponse);
   Utils::FastMemSet(&mXnAddr, 0, sizeof(mXnAddr));
   Utils::FastMemSet(&mXnKID, 0, sizeof(mXnKID));
   Utils::FastMemSet(&mXnKey, 0, sizeof(mXnKey));
   Utils::FastMemSet(mName, 0, sizeof(mName));

   Utils::FastMemCpy(&mXnAddr, &pMsgResponse->mXnAddr, sizeof(mXnAddr));
   Utils::FastMemCpy(&mXnKey, &pMsgResponse->mXnKey, sizeof(mXnKey));
   Utils::FastMemCpy(&mXnKID, &pMsgResponse->mXnKID, sizeof(mXnKID));
   Utils::FastMemCpy(&mName, &pMsgResponse->mName, sizeof(mName));
   Utils::FastMemCpy(&mNonce, &pMsgResponse->mNonce, NONCE_SIZE);
   mChecksum = pMsgResponse->mChecksum;

   mUpdateTime = timeGetTime();
}

//==============================================================================
//Takes the current descriptor lan-related data and packs it into the buffer you pass in
// It returns the size of the structure it packed into the buffer
//==============================================================================
DWORD BLanGameDescriptor::serializeLANData(MessageLANDiscoveryResponse* pMsgResponse)
{
   Utils::FastMemCpy(&pMsgResponse->mXnAddr, &mXnAddr, sizeof(mXnAddr));
   Utils::FastMemCpy(&pMsgResponse->mXnKey, &mXnKey, sizeof(mXnKey));
   Utils::FastMemCpy(&pMsgResponse->mXnKID, &mXnKID, sizeof(mXnKID));
   Utils::FastMemCpy(&pMsgResponse->mName, &mName, sizeof(mName));
   Utils::FastMemCpy(&pMsgResponse->mNonce, &mNonce, NONCE_SIZE);
   pMsgResponse->mChecksum = mChecksum;
   mUpdateTime = timeGetTime();

   return 0;//todo - drop return val
}

//==============================================================================
//
//==============================================================================
bool BLanGameDescriptor::isFromSameSenderAs(BLanGameDescriptor* pDesc)
{
   return (memcmp(pDesc->mNonce, mNonce, NONCE_SIZE) == 0);
}

//==============================================================================
//
//==============================================================================
//Returns true if this descriptor has been around longer than the expireTime (in ms)
bool BLanGameDescriptor::hasExpired(DWORD expireTime)
{
   return ((timeGetTime() - mUpdateTime) > expireTime);
}

//==============================================================================
//
//==============================================================================
void BLanGameDescriptor::setName(const BSimString& name)
{
   if (name.length() <= 0)
      mName[0] = 0;
   else
   {   
      strCopy(mName, _MAX_PATH, name.getPtr());
      mName[_MAX_PATH-1] = 0;
   }
   mUpdateTime = timeGetTime();
}

//==============================================================================
//
//==============================================================================
const BCHAR_T* BLanGameDescriptor::getName(void) const
{
   return(mName);
}

//==============================================================================
//
//==============================================================================
void BLanGameDescriptor::setLocal(bool local)
{
   mLocal = local;
   mUpdateTime = timeGetTime();
}

//==============================================================================
//
//==============================================================================
bool BLanGameDescriptor::getLocal(void) const
{
   return (mLocal);
}

//==============================================================================
//
//==============================================================================
void BLanGameDescriptor::setChecksum(DWORD checksum)
{
   mChecksum = checksum;
   mUpdateTime = timeGetTime();
}

//==============================================================================
//
//==============================================================================
DWORD BLanGameDescriptor::getChecksum() const
{
   return (mChecksum);
}

//==============================================================================
//
//==============================================================================
void BLanGameDescriptor::setXNKID(const XNKID& newKid)
{
   mXnKID = newKid;
   //Utils::FastMemCpy(&mXnKID, &newKid, sizeof(XNKID)); 
   mUpdateTime = timeGetTime();
}

//==============================================================================
//
//==============================================================================
void BLanGameDescriptor::setXNKEY(const XNKEY& newKey)
{
   mXnKey = newKey;
   mUpdateTime = timeGetTime();
}

//==============================================================================
//
//==============================================================================
void BLanGameDescriptor::setXnAddr(const XNADDR& xnaddr)
{
   mXnAddr = xnaddr;
   mUpdateTime = timeGetTime();
}

//==============================================================================
//
//==============================================================================
WORD BLanGameDescriptor::getPort() const
{
   return mXnAddr.wPortOnline;
}

//==============================================================================
//
//==============================================================================
const IN_ADDR& BLanGameDescriptor::getAddress() const
{
   return mXnAddr.ina;
}

//==============================================================================
//
//==============================================================================
const IN_ADDR& BLanGameDescriptor::getTranslatedAddress() const
{
   return mXnAddr.inaOnline;
}
