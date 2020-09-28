//------------------------------------------------------------------------------------------------------------------------
//
// File: spawn.cpp
//
// Copyright (c) 2007, Ensemble Studios
//
//------------------------------------------------------------------------------------------------------------------------
#include "xcore.h"
#include "spawn.h"
#include "string\tokenize.h"

BSpawnCommand::BSpawnCommand()
{
}

BSpawnCommand::BSpawnCommand(const char* pCmd)
{
   set(pCmd);
}

void BSpawnCommand::clear()
{
   mExec.empty();
   mArgs.resize(0);
}

bool BSpawnCommand::set(const char* pExec, const char* pArg0, const char* pArg1, const char* pArg2, const char* pArg3)
{
   if (!pExec)
   {
      clear();
      return false;
   }
   
   mExec = pExec;
   mArgs.resize(0);
   mArgs.pushBack(pExec);
   if (pArg0) mArgs.pushBack(pArg0);
   if (pArg1) mArgs.pushBack(pArg1);
   if (pArg2) mArgs.pushBack(pArg2);
   if (pArg3) mArgs.pushBack(pArg3);
      
   return true;
}

bool BSpawnCommand::set(const char* pExec, const BArgList& args)
{
   if (!pExec)
   {
      clear();
      return false;
   }

   mExec = pExec;
   mArgs = args;
   
   return true;
}

bool BSpawnCommand::set(const char* pCmd)
{
   clear();
   
   if (!pCmd)
      return false;
      
   if (!tokenizeString(pCmd, mArgs))
      return false;
      
   if (mArgs.isEmpty())      
      return false;
      
   mExec = mArgs[0];
      
   return true;
}

intptr_t BSpawnCommand::run(int mode)
{
#ifdef XBOX
   return -1;
#else   
   if ((mExec.isEmpty()) || (mArgs.isEmpty()))
      return -1;
      
   BDynamicArray<const char*> ptrs(mArgs.getSize());
   
   for (uint i = 0; i < mArgs.getSize(); i++)
      ptrs[i] = mArgs[i].getPtr();
   ptrs.pushBack(NULL);
   
   intptr_t status = _spawnvp(mode, mExec.getPtr(), ptrs.getPtr());
   
   return status;
#endif   
}
