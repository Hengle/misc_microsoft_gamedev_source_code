//==============================================================================
// gamecallbacks.cpp
//
// Copyright (c) 2005 Ensemble Studios
//==============================================================================

// Includes
#include "common.h"
#include "gamecallbacks.h"
#include "consolefuncsgame.h"
#include "consolefuncsengine.h"
#include "criticalmessagebox.h"
#include "debugcallstack.h"
#include "game.h"
#include "xmlreader.h"
#include "xsfuncs.h"
#include "xfs.h"
// xgameRender
#include "renderFuncs.h"
#include "modemanager.h"

//==============================================================================
// minimizeFullscreenWindow
//==============================================================================
void CALLBACK minimizeFullscreenWindow(bool bMinimize)
{
   bMinimize;
}

//==============================================================================
// gamePreAssert
//==============================================================================
void gamePreAssert( const char *expression, const char *msg, const char *file, long line, bool fatal, bool noreturn, const BDebugCallstack* pCallstack, void * /*param1*/, void * /*param2*/, const char* cpFullAssertError )
{
   // This function may be called from any thread!
   // The blog functionality is not yet thread safe, so I'm remarking this stuff out.

   X_Exception::DumpFailReport(cpFullAssertError, "PhoenixFailReport.txt");
   //BModeManager::shutdownFPSLogFile(true);


#if 0
   minimizeFullscreenWindow(true);

   {setBlogError(0); blogerrortrace("%s(%d): assertion failed -- exp: %s, msg: %s, fatal: %s -- callstack:", file, line, expression, msg, fatal?"yes":"no");}
   
   //BCommLog::dumpHistory();
   if (pCallstack)   
   {
      const BDebugCallStackEntry *entries = pCallstack->getEntries();
      if(!entries)
         return;

      for(long i=0; i<pCallstack->getCount(); i++)
      {
         setBlogError(0); blogerrortrace("   %s(%d):  function: %s   module: %s", BString(entries[i].mFile).getPtr(), entries[i].mLine, BString(entries[i].mFunctionName).getPtr(), BString(entries[i].mModule).getPtr());
      }
   }      
#endif   
}

//==============================================================================
// gamePostAssert
//==============================================================================
void gamePostAssert( const char * /*expression*/, const char * /*msg*/, const char * /*file*/, long /*line*/, bool /*fatal*/, bool /* noreturn */, const BDebugCallstack* /*callstack*/, void * /*param1*/, void * /*param2*/, const char* /*cpFullAssertError*/ )
{
#if 0
   // This function may be called from any thread!
   minimizeFullscreenWindow(false);
#endif   
}

//==============================================================================
// memoryMessageFunc
//==============================================================================
long CALLBACK memoryMessageFunc(const BCHAR_T* pMessage, const BCHAR_T* pTitle, long numButtons, const BCHAR_T** pButtonText, void* pParam)
{
   pParam;

   minimizeFullscreenWindow(true);

   BDynamicSimArray<BUString> strings;
   BDynamicSimArray<const WCHAR*> pointers;
   if (numButtons)
   {
      strings.resize(numButtons);
      pointers.resize(numButtons);
      for (int i = 0; i < numButtons; i++)
      {
         strings[i].set(pButtonText[i]);
         pointers[i] = strings[i].getPtr();
      }
   }

   return criticalMessageBox(BUString(pMessage), BUString(pTitle), NULL, NULL, numButtons, &pointers[0]);
}

//==============================================================================
// dumpConsoleFuncs
//==============================================================================
static void dumpConsoleFuncs(BXSSyscallModule* sm)
{
   BXSSyscallEntryArray& syscalls = gConsole.getConsoleSyscalls()->getSyscalls();
   for (uint i = 0; i < syscalls.size(); i++)
   {
      const BXSSyscallEntry* pEntry = syscalls[i];

      if ((pEntry) && (pEntry->getHelp()) && (strlen(pEntry->getHelp())))
      {
         if (strstr(pEntry->getHelp(), " xs") == NULL)
         {
            BString helpText(pEntry->getHelp());
            
            int i = helpText.findLeft('(');
            if (i >= 0)
               helpText.crop(0, i - 1);
            
            BString buf;
            buf.format("<cmd>%s", helpText.getPtr());
            gConsole.rawOutput(buf.getPtr());
         }
      }
   }
   
   // This should be data driven, for now screw it.
   gConsole.rawOutput("<cmd>configSetFloat(\"GameSpeed\", .25)");
   gConsole.rawOutput("<cmd>configSetFloat(\"GameSpeed\", .5)");
   gConsole.rawOutput("<cmd>configSetFloat(\"GameSpeed\", 1.0)");
   gConsole.rawOutput("<cmd>configSetFloat(\"GameSpeed\", 2.0)");
   
   gConsole.rawOutput("<cmd>createObjectAtCursor(\"unsc_inf_marine_01\", 1)");
   gConsole.rawOutput("<cmd>createObjectAtCursor(\"unsc_veh_wolverine_01\", 1)");
   gConsole.rawOutput("<cmd>createObjectAtCursor(\"unsc_bldg_supplypad_01\", 1)");
   
   gConsole.rawOutput("<cmd>createObjectAtCursor(\"unsc_inf_marine_01\", 2)");
   gConsole.rawOutput("<cmd>createObjectAtCursor(\"unsc_veh_wolverine_01\", 2)");
   gConsole.rawOutput("<cmd>createObjectAtCursor(\"unsc_bldg_supplypad_01\", 2)");

   gConsole.rawOutput("<cmd>createSquadAtCursor(\"unsc_inf_marine_01\", 1)");
   gConsole.rawOutput("<cmd>createSquadAtCursor(\"unsc_veh_warthog_01\", 1)");
   gConsole.rawOutput("<cmd>createSquadAtCursor(\"unsc_veh_wolverine_01\", 1)");
   gConsole.rawOutput("<cmd>createSquadAtCursor(\"unsc_veh_scorpion_01\", 1)");

   gConsole.rawOutput("<cmd>createSquadAtCursor(\"unsc_inf_marine_01\", 2)");
   gConsole.rawOutput("<cmd>createSquadAtCursor(\"unsc_veh_warthog_01\", 2)");
   gConsole.rawOutput("<cmd>createSquadAtCursor(\"unsc_veh_wolverine_01\", 2)");
   gConsole.rawOutput("<cmd>createSquadAtCursor(\"unsc_veh_scorpion_01\", 2)");

   gConsole.rawOutput("<cmd>createObjectAtCursor(\"test_particle_01\", 1)");
   gConsole.rawOutput("<cmd>createObjectAtCursor(\"test_particle_02\", 1)");
   gConsole.rawOutput("<cmd>createObjectAtCursor(\"test_particle_03\", 1)");
   gConsole.rawOutput("<cmd>createObjectAtCursor(\"test_particle_04\", 1)");
   gConsole.rawOutput("<cmd>createObjectAtCursor(\"test_particle_05\", 1)");
   gConsole.rawOutput("<cmd>createObjectAtCursor(\"test_particle_06\", 1)");
   gConsole.rawOutput("<cmd>createObjectAtCursor(\"test_particle_07\", 1)");
   gConsole.rawOutput("<cmd>createObjectAtCursor(\"test_particle_08\", 1)");
}

//==============================================================================
// registerConsoleFuncs
//==============================================================================
bool registerConsoleFuncs(BXSSyscallModule* sm)
{
   if (sm == NULL)
      return(false);
      
   if(!addEngineFuncs(sm))
      return false;      

   if(!addGameFuncs(sm))
      return false;

   if(!addXSFunctions(sm))
      return false;
      
   if (!addRenderFuncs(sm))
      return false;

   dumpConsoleFuncs(sm);      

   return(true);
}
