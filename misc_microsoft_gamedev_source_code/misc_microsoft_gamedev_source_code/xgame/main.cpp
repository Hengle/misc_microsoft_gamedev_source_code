//==============================================================================
// main.cpp
//
// Copyright (c) 2005 Ensemble Studios
//==============================================================================

// Includes
#include "common.h"
#include "game.h"

// Constants
const char* cGameTitle="Phoenix";

#ifdef XBOX
//==============================================================================
// main
//==============================================================================
void __cdecl main()
{
   __try
   {
      if(gGame.setup(cGameTitle, 1, false))
         gGame.run();
      gGame.shutdown();
   }
   // Catch any assertion exceptions.
   __except(gAssertionSystem.xboxHandleAssertException(GetExceptionInformation()))
   {
   }
}
#endif

#ifndef XBOX
//==============================================================================
// WinMain
//==============================================================================
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR lpCmdLine, int nCmdShow)
{
   lpCmdLine;
   hInstance;
   if(gGame.setup(cGameTitle, nCmdShow, false))
      gGame.run();
   gGame.shutdown();
}
#endif

