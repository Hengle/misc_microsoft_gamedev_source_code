//==============================================================================
// gamerPicManager.h
//
// Copyright (c) 2007 Ensemble Studios
//==============================================================================

#pragma once

#include "asynctaskmanager.h"
#include "xbox.h"

// Forward Declarations
class BGamerPicManager;

// Global variable for the one BGamerPicManager object
extern BGamerPicManager gGamerPicManager;

//-------------------------------------------------------------------------------------------------------------------------------------
class BGamerPic
{
public:
   BGamerPic();
   ~BGamerPic();

   bool init(XUID xuid, DWORD port, int width, int height);

public :
   XUID mXuid;

   int   mBufferSize;
   byte* mpTextureBuffer;
   int   mWidth;
   int   mHeight;
   int   mBytesPerPixel;            // because we are using D3DFMT_LIN_A8R8G8B8 as the format
   int   mPitch;

   DWORD mPort;

   bool  mIsLocalUser:1;

};

//==============================================================================
// BReadGamerPicKeyAsyncTask
//==============================================================================
class BReadGamerPicKeyAsyncTask : public BAsyncTask
{
public:
   BReadGamerPicKeyAsyncTask();
   ~BReadGamerPicKeyAsyncTask();

   bool startRead(XUID mXuid, DWORD requestingPort);
   void processResult();

public:
   XUID mXuid;
   DWORD mRequestingPort;

   DWORD mProfileKeyID;

   DWORD mProfileKeySize;
   BYTE* mpProfileKeyData;
};


//==============================================================================
// BReadLocalGamerPicAsyncTask - read a player's gamer pic
//==============================================================================
class BReadLocalGamerPicAsyncTask : public BAsyncTask
{
public:
   BReadLocalGamerPicAsyncTask();
   ~BReadLocalGamerPicAsyncTask();

   bool startReadRemote(XUID xuid, BYTE* profileGamerKey, DWORD profileGamerKeySize);
   bool startReadLocal(XUID mXuid, DWORD port);
   void processResult();

public:
   BGamerPic*  mpGamerPic;

   // This is for remote gamer pics
   DWORD mProfileKeySize;
   BYTE* mpProfileKeyData;

};


//==============================================================================
// BGamerPicManager
//==============================================================================
class BGamerPicManager : public BAsyncNotify
{

public:
   BGamerPicManager();
   ~BGamerPicManager();

   void reset(bool includeLocal=true);

   DWORD getLastUpdate() const { return mLastPicUpdate; }

   BGamerPic* getGamerPic(XUID xuid);
   BGamerPic* getGamerPic(const char* xuidString);
   void addGamerPic(BGamerPic* gamerPic);

   IDirect3DTexture9* createTexture(const char* xuidString);

   void removeGamerPic(XUID xuid);
   bool readGamerPic(XUID xuid, DWORD port, BOOL isLocal);

   // Async Task Notifications
   void notify(DWORD eventID, void* pTask);

   DWORD                mLastPicUpdate;

protected:
   BCriticalSection     mCrit;
   BSmallDynamicSimArray<BGamerPic*> mGamerPics;
};
