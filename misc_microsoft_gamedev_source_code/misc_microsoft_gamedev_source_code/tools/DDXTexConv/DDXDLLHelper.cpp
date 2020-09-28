// File: DDXDLLHelper.cpp
#include "xcore.h"
#include "string\fixedString.h"

#include "DDXDLL.h"
#include "DDXDLLHelper.h"

BDDXDLLHelper::BDDXDLLHelper() :
   mDLLHandle(NULL),
   mpIDDXDLL(NULL)
{
}

BDDXDLLHelper::~BDDXDLLHelper()
{
   deinit();
}

bool BDDXDLLHelper::init(void)
{
   deinit();
   
   BFixedString256 path;
   
   GetModuleFileNameA(NULL, path.getPtr(), path.getBufSize() - 1);
   path.removeFilename();
   path.append(DDX_DLL_FILENAME);

   mDLLHandle=LoadLibrary(path.c_str());

   if(mDLLHandle==NULL) 
      return false;
   
   DDX_DLL_ENTRYPOINT_FUNC pCreateDDXDLL = (DDX_DLL_ENTRYPOINT_FUNC)GetProcAddress(mDLLHandle, DDX_DLL_ENTRYPOINT);

   if(!pCreateDDXDLL)
   {
      deinit();
      return false;
   }

   mpIDDXDLL = pCreateDDXDLL(DDX_DLL_INTERFACE_VERSION);
   if(!mpIDDXDLL)
   {
      deinit();
      return false;
   }
   
   if (mpIDDXDLL->getVersion() < 1)
   {
      deinit();
      return false;
   }
      
   return true;
}

bool BDDXDLLHelper::deinit(void)
{
   if (mpIDDXDLL)
   {
      mpIDDXDLL->release();
      mpIDDXDLL = NULL;
   }

   if (mDLLHandle)
   {
      FreeLibrary(mDLLHandle);
      mDLLHandle = NULL;
   }
   
   return true;
}


   