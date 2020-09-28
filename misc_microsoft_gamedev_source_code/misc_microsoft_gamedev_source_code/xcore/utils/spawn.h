//------------------------------------------------------------------------------------------------------------------------
//
// File: spawn.h
//
// Copyright (c) 2007, Ensemble Studios
//
//------------------------------------------------------------------------------------------------------------------------
#pragma once

// Executes a process with optional parameters.
class BSpawnCommand
{
public:
   BSpawnCommand();
   BSpawnCommand(const char* pCmd);
   
   void clear();
   bool isValid() const { return mExec.length() > 0; }

   bool set(const char* pCmd);
   bool set(const char* pExec, const char* pArg0, const char* pArg1 = NULL, const char* pArg2 = NULL, const char* pArg3 = NULL);
   
   typedef BDynamicArray<BString> BArgList;
   
   bool set(const char* pExec, const BArgList& args);
   
   intptr_t run(int mode = _P_WAIT);
   
private:
   
   BString mExec;
   BArgList mArgs;   
};