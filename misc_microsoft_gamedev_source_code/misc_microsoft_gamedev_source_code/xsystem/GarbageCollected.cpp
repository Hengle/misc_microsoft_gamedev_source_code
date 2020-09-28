//==============================================================================
// GarbageCollected.cpp
//
// Copyright (c) 2001-2008, Ensemble Studios
//==============================================================================

// Includes
#include "xsystem.h"
#include "GarbageCollected.h"
#include <winbase.h>

//==============================================================================
// BGarbageCollected::BGarbageCollected
//==============================================================================
BGarbageCollected::BGarbageCollected() :
   mDisposing(false)
{

} // BGarbageCollected::BGarbageCollected

//==============================================================================
// BGarbageCollected::~BGarbageCollected
//==============================================================================
BGarbageCollected::~BGarbageCollected()
{

} // BGarbageCollected::~BGarbageCollected

//==============================================================================
// 
void CALLBACK BGarbageCollected::apcProc(ULONG_PTR dwParam)
{
   delete (BGarbageCollected *)dwParam;   
}

//==============================================================================
// 
HRESULT BGarbageCollected::dispose()
{   
   if (mDisposing)
      return E_FAIL;

   if (!QueueUserAPC(BGarbageCollected::apcProc, GetCurrentThread(), (DWORD)this))   
      return HRESULT_FROM_WIN32(GetLastError());
   
   mDisposing = true;
   return S_OK;
}
