//==============================================================================
// gamerPicManager.cpp
//
// Copyright (c) 2007 Ensemble Studios
//==============================================================================

// Includes
#include "xgameRender.h"
#include "gamerPicManager.h"
#include "xbox.h"
#include "render.h"


// Globals
BGamerPicManager gGamerPicManager;

//==============================================================================
// BGamerPic::BGamerPic
//==============================================================================
BGamerPic::BGamerPic() :
mXuid(0),
mpTextureBuffer(NULL),
mWidth(0),
mHeight(0),
mPort(XUSER_INDEX_NONE),
mIsLocalUser(true)
{
}

//==============================================================================
// BGamerPic::~BGamerPic
//==============================================================================
BGamerPic::~BGamerPic()
{
   if (mpTextureBuffer)
   {
      delete mpTextureBuffer;
      mpTextureBuffer=NULL;
   }
}

//==============================================================================
// BGamerPic::~BGamerPic
//==============================================================================
bool BGamerPic::init(XUID xuid, DWORD port, int width, int height)
{
   mXuid = xuid;
   mPort = port;

   if (mpTextureBuffer)
   {
      delete mpTextureBuffer;
      mpTextureBuffer=NULL;
   }

   // create a byte array buffer large enough to handle the gamerpic
   mWidth=width;
   mHeight=height;
   mBytesPerPixel=4;                            // from D3DFMT_LIN_A8R8G8B8
   mBufferSize=mHeight*mWidth*mBytesPerPixel;
   mPitch=mWidth*mBytesPerPixel;

   // allocate the buffer
   mpTextureBuffer = new byte[mBufferSize];
   ZeroMemory(mpTextureBuffer, mBufferSize);

   // hResult = gRenderDraw.createTexture(mWidth, mHeight, 1, 0, D3DFMT_LIN_A8R8G8B8, 0, &(mpTextureBuffer), NULL);
   return true;
}

//-------------------------------------------------------------------------------------

//==============================================================================
// BReadLocalGamerPicAsyncTask::BReadLocalGamerPicAsyncTask
//==============================================================================
BReadLocalGamerPicAsyncTask::BReadLocalGamerPicAsyncTask() :
   BAsyncTask(),
   mpProfileKeyData(NULL),
   mpGamerPic(NULL)
{
}

//==============================================================================
// BReadLocalGamerPicAsyncTask::~BReadLocalGamerPicAsyncTask
//==============================================================================
BReadLocalGamerPicAsyncTask::~BReadLocalGamerPicAsyncTask()
{
   if (mpProfileKeyData)
   {
      delete[] mpProfileKeyData;
      mpProfileKeyData = NULL;
   }
}

//==============================================================================
// BReadLocalGamerPicAsyncTask::startReadRemote
//==============================================================================
bool BReadLocalGamerPicAsyncTask::startReadRemote(XUID xuid, BYTE* profileGamerKey, DWORD profileGamerKeySize)
{
   if (mpProfileKeyData)
   {
      delete[] mpProfileKeyData;
      mpProfileKeyData=NULL;
   }

   // NOTE: We now have ownership of the buffer passed into us and we are responsible for deleting it.
   mProfileKeySize = profileGamerKeySize;
   mpProfileKeyData = profileGamerKey;

   int height = 64;
   int width  = 64;

   mpGamerPic = new BGamerPic();
   if (!mpGamerPic->init(xuid, XUSER_INDEX_NONE, width, height))
   {
      delete mpGamerPic;
      mpGamerPic=NULL;
      return false;
   }

   mpGamerPic->mIsLocalUser=false;

   // Read the gamerpic texture locally
   ZeroMemory( &mOverlapped , sizeof( mOverlapped  ) );


   DWORD dwResult = ERROR_SUCCESS;
   dwResult = XUserReadGamerPictureByKey(&( ( PXUSER_READ_PROFILE_SETTING_RESULT )mpProfileKeyData )->pSettings->data,
                                          FALSE,
                                          mpGamerPic->mpTextureBuffer,
                                          mpGamerPic->mPitch,
                                          mpGamerPic->mHeight,
                                          &mOverlapped );

   if( dwResult != ERROR_IO_PENDING )
   {
      // delete the gamer pick
      delete mpGamerPic;
      mpGamerPic = NULL;

      return false;
   }

   return true;
}


//==============================================================================
// BReadLocalGamerPicAsyncTask::startRead
//==============================================================================
bool BReadLocalGamerPicAsyncTask::startReadLocal(XUID xuid, DWORD port)
{
   int height = 64;
   int width  = 64;

   mpGamerPic = new BGamerPic();
   if (!mpGamerPic->init(xuid, port, width, height))
   {
      delete mpGamerPic;
      mpGamerPic=NULL;
      return false;
   }

   mpGamerPic->mIsLocalUser=true;

   // Read the gamerpic texture locally
   ZeroMemory( &mOverlapped , sizeof( mOverlapped  ) );

   DWORD dwResult = ERROR_SUCCESS;
   dwResult = XUserReadGamerPicture(mpGamerPic->mPort,
                                    FALSE,               // ask for the 64x64 version
                                    mpGamerPic->mpTextureBuffer,
                                    mpGamerPic->mPitch,
                                    mpGamerPic->mHeight,
                                    &mOverlapped);


   if( dwResult != ERROR_IO_PENDING )
   {
      // delete the gamer pick
      delete mpGamerPic;
      mpGamerPic = NULL;

      return false;
   }

   return true;
}

//==============================================================================
// BReadLocalGamerPicAsyncTask::processResult
//==============================================================================
void BReadLocalGamerPicAsyncTask::processResult()
{
   BAsyncTask::processResult();

   DWORD result = getResult();
   if (result != ERROR_SUCCESS)
   {
      DWORD extendedError = XGetOverlappedExtendedError(mpOverlapped);

      trace("read gamer pic async, extended error is %d",extendedError);

      delete mpGamerPic;
      mpGamerPic=NULL;

      return;
   }

   gGamerPicManager.addGamerPic(mpGamerPic);
   return;
}

//-------------------------------------------------------------------------------------

//==============================================================================
// BReadGamerPicKeyAsyncTask::BReadGamerPicKeyAsyncTask
//==============================================================================
BReadGamerPicKeyAsyncTask::BReadGamerPicKeyAsyncTask() :
   BAsyncTask(),
   mProfileKeyID(XPROFILE_GAMERCARD_PICTURE_KEY),
   mpProfileKeyData(NULL)
{
}

//==============================================================================
// BReadGamerPicKeyAsyncTask::~BReadGamerPicKeyAsyncTask
//==============================================================================
BReadGamerPicKeyAsyncTask::~BReadGamerPicKeyAsyncTask()
{
   if (mpProfileKeyData)
   {
      delete[] mpProfileKeyData;
      mpProfileKeyData = NULL;
   }
}

//==============================================================================
// BReadGamerPicKeyAsyncTask::startRead
//==============================================================================
bool BReadGamerPicKeyAsyncTask::startRead(XUID xuid, DWORD requestingPort)
{
   mXuid = xuid;
   mRequestingPort = requestingPort;

   // first get the profile data to be able to read the gamer pic

   HRESULT hResult = S_OK;
   mProfileKeySize = 0;

   // Determine the required size of the key buffer
   DWORD dwResult = XUserReadProfileSettingsByXuid(
      0, mRequestingPort,
      1, &mXuid,
      1, &mProfileKeyID,
      &mProfileKeySize,
      NULL,
      NULL
      );

   if( dwResult != ERROR_INSUFFICIENT_BUFFER )
      hResult = HRESULT_FROM_WIN32( dwResult );

   if (!SUCCEEDED(hResult))
      return false;

   // allocate the buffer for the profile data
   mpProfileKeyData = new BYTE[mProfileKeySize];

   // Read the gamerpic key
   ZeroMemory( &mOverlapped, sizeof( mOverlapped ) );
   dwResult = XUserReadProfileSettingsByXuid(
      0, mRequestingPort,
      1, &mXuid,
      1, &mProfileKeyID,
      &mProfileKeySize,
      ( PXUSER_READ_PROFILE_SETTING_RESULT )mpProfileKeyData,
      &mOverlapped
      );

   if( dwResult != ERROR_IO_PENDING )
   {
      hResult = HRESULT_FROM_WIN32( dwResult );
   }

   if (!SUCCEEDED(hResult))
   {
      delete[] mpProfileKeyData;
      mpProfileKeyData=NULL;
      return false;
   }

   return true;
}

//==============================================================================
// BReadGamerPicKeyAsyncTask::processResult
//==============================================================================
void BReadGamerPicKeyAsyncTask::processResult()
{
   BAsyncTask::processResult();

   // 
   if (getResult() != ERROR_SUCCESS)
   {
      // delete the gamer pick
      delete[] mpProfileKeyData;
      mpProfileKeyData = NULL;
      return;
   }

   // we were successful, kick off the task to get the picture now.


   // NOTE: We are transferring the profile data buffer over the the BReadLocalGamerPicAsyncTask object
   // so we can set our pointer to NULL, the BReadLocalGamerPicAsyncTask will be responsible for deleting this
   // buffer when it is done with it.
   BYTE* pProfileKeyData = mpProfileKeyData;
   mpProfileKeyData=NULL;

   // create a new task to read the actual gamer pic
   BReadLocalGamerPicAsyncTask* pTask = new BReadLocalGamerPicAsyncTask();
   if (!pTask->startReadRemote(mXuid, pProfileKeyData, mProfileKeySize))
   {
      delete pTask;
      return;
   }

   gAsyncTaskManager.addTask(pTask);

   return;
}

//-------------------------------------------------------------------------------------


//==============================================================================
// BGamerPicManager::BGamerPicManager
//==============================================================================
BGamerPicManager::BGamerPicManager() :
mLastPicUpdate(0)
{
}

//==============================================================================
// BGamerPicManager::~BGamerPicManager
//==============================================================================
BGamerPicManager::~BGamerPicManager()
{
   // clean up
   reset();
}

//==============================================================================
// BGamerPicManager::reset
//==============================================================================
void BGamerPicManager::reset(bool includeLocal)
{
   BScopedCriticalSection crit(mCrit);
   for (int i=mGamerPics.getNumber()-1; i>=0; i--)
   {
      if (mGamerPics[i] == NULL)
         continue;

      if (!includeLocal && mGamerPics[i]->mIsLocalUser)
         continue;

      // remove it
      BGamerPic* pic = mGamerPics[i];
      delete pic;
      mGamerPics[i]=NULL;
      mGamerPics.removeIndex(i);
   }
}

//==============================================================================
// BGamerPicManager::readGamerPic
//==============================================================================
bool BGamerPicManager::readGamerPic(XUID xuid, DWORD port, BOOL isLocal)
{
   if (isLocal)
   {
      BReadLocalGamerPicAsyncTask* pTask = new BReadLocalGamerPicAsyncTask();
      if (!pTask->startReadLocal(xuid, port))
      {
         delete pTask;
         return false;
      }

      gAsyncTaskManager.addTask(pTask);
   }
   else
   {
      BReadGamerPicKeyAsyncTask* pTask=new BReadGamerPicKeyAsyncTask();
      if (!pTask->startRead(xuid, port))
      {
         delete pTask;
         return false;
      }

      gAsyncTaskManager.addTask(pTask);

   }

   return true;
}

//==============================================================================
// BGamerPicManager::notify
//==============================================================================
void BGamerPicManager::notify(DWORD eventID, void* pTask)
{
}

//==============================================================================
// BGamerPicManager::createTexture
//==============================================================================
IDirect3DTexture9* BGamerPicManager::createTexture(const char* xuidString)
{
   ASSERT_RENDER_THREAD
   BScopedCriticalSection crit(mCrit);

//-- FIXING PREFIX BUG ID 6544
   const BGamerPic* gamerPic = getGamerPic(xuidString);
//--
   if (!gamerPic)
      return NULL;

   IDirect3DTexture9* pD3DTexture = NULL;
   if (FAILED(gRenderDraw.createTexture(gamerPic->mWidth, gamerPic->mHeight, 1, 0, D3DFMT_LIN_A8R8G8B8, 0, &pD3DTexture, NULL)))
      return NULL;

   // copy over the data for this gamer pic
   D3DLOCKED_RECT rect = {0};
   pD3DTexture->LockRect( 0, &rect, NULL, 0 );
   Utils::FastMemCpy(rect.pBits, gamerPic->mpTextureBuffer, gamerPic->mBufferSize);
   pD3DTexture->UnlockRect(0);

   return pD3DTexture;
}

//==============================================================================
// BGamerPicManager::getGamerPic
//==============================================================================
BGamerPic* BGamerPicManager::getGamerPic(const char* xuidString)
{
   // fixme - use this format:
//   mHack.format("img://gamerPic:%I64x", user->getXuid());

   BScopedCriticalSection crit(mCrit);
   XUID xuid;
   xuid = _strtoui64(xuidString, NULL, 16);

   for (int i=0; i<mGamerPics.getNumber(); i++)
   {
      if (mGamerPics[i] == NULL)
         continue;

      if (mGamerPics[i]->mXuid == xuid)
         return mGamerPics[i];
   }

   return NULL;
}


//==============================================================================
// BGamerPicManager::notify
//==============================================================================
BGamerPic* BGamerPicManager::getGamerPic(XUID xuid)
{
   BScopedCriticalSection crit(mCrit);
   for (int i=0; i<mGamerPics.getNumber(); i++)
   {
      if (mGamerPics[i] == NULL)
         continue;

      if (mGamerPics[i]->mXuid == xuid)
         return mGamerPics[i];
   }

   return NULL;
}


//==============================================================================
// BGamerPicManager::notify
//==============================================================================
void BGamerPicManager::removeGamerPic(XUID xuid)
{
   BScopedCriticalSection crit(mCrit);
   for (int i=0; i<mGamerPics.getNumber(); i++)
   {
      if (mGamerPics[i] == NULL)
         continue;

      if (mGamerPics[i]->mXuid != xuid)
         continue;

      // remove it
      BGamerPic* pic = mGamerPics[i];
      delete pic;
      mGamerPics[i]=NULL;
      mGamerPics.removeIndex(i);
      break;
   }
}


//==============================================================================
// BGamerPicManager::notify
//==============================================================================
void BGamerPicManager::addGamerPic(BGamerPic* gamerPic)
{
   BScopedCriticalSection crit(mCrit);

   removeGamerPic(gamerPic->mXuid);    // remove any duplicates
   
   mGamerPics.add(gamerPic);

   mLastPicUpdate = timeGetTime();
}
