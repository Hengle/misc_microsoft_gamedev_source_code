//==============================================================================
// xexInfo.cpp
//
// Copyright (c) 2006 Ensemble Studios
//==============================================================================
#include "xcore.h"
#ifdef XBOX
#ifndef BUILD_FINAL
#include <xbdm.h>
#endif
#include "xexInfo.h"
#include "hash\adler32.h"
#include "file\win32FileUtils.h"

#ifndef INVALID_FILE_ATTRIBUTES
   #define INVALID_FILE_ATTRIBUTES ((DWORD)-1)
#endif

BXEXChecksum::BXEXChecksum()
{
}

BXEXChecksum::~BXEXChecksum()
{
}

bool BXEXChecksum::compute(DWORD& checksum, BStream& stream)
{
   checksum = 0;

   const uint cCheckSumSize = 4096;
   uint size = (uint)Math::Min<uint64>(cCheckSumSize, stream.size());

   uchar buf[cCheckSumSize];

   if (stream.readBytes(buf, size) != size)
      return false;

   checksum = calcAdler32(buf, size);      
   return true;
}

bool BXEXInfo::init(void)
{
#ifdef BUILD_FINAL
   const DWORD fullAttributes = GetFileAttributes("d:\\default.xex");
   if ((fullAttributes == INVALID_FILE_ATTRIBUTES) || (fullAttributes & FILE_ATTRIBUTE_DIRECTORY))
   {
      mLaunchPath.set("d:\\xgameF.xex");
      mModuleName.set("xgameF.xex");
   }
   else
   {
      mLaunchPath.set("d:\\default.xex");
      mModuleName.set("default.xex");
   }
   mModuleFullName = "d:\\";
   mModuleFullName += mModuleName;
   mBaseAddress = 0x82000000;
   mBaseAddress = 0x82000000;
   mSize = 0xFFFFFFF;
   mValid = true;
   return true;
#else
   mLaunchPath.empty();
   mModuleName.empty();
   mBaseAddress = 0x82000000;
   mSize = 0xFFFFFFF;
   mValid = false;

   DM_XBE xbeInfo;
   HRESULT hres = DmGetXbeInfo(NULL, &xbeInfo);
   if (FAILED(hres))
      return false;

   mLaunchPath.set(xbeInfo.LaunchPath);

   strPathGetFilename(BString(xbeInfo.LaunchPath), mModuleName);
   
   mModuleFullName = "d:\\";
   mModuleFullName += mModuleName;

   HRESULT error;
   PDM_WALK_MODULES pWalkMod = NULL;
   DMN_MODLOAD modLoad;

   bool found = false;

   while( XBDM_NOERR == (error = DmWalkLoadedModules(&pWalkMod, &modLoad)) ) 
   {
      BString filename(modLoad.Name);

      if (filename == mModuleName)
      {
         found = true;
         mBaseAddress = reinterpret_cast<DWORD>(modLoad.BaseAddress);
         mSize = modLoad.Size;
#if 0            
         trace("%s Base:%08X Size:%08X TimeStamp:%08X CheckSum: %08X Flags: %08X PDataAddr: %08X, PDataSize: %08X",
            modLoad.Name,
            modLoad.BaseAddress,
            modLoad.Size,
            modLoad.TimeStamp,
            modLoad.CheckSum,
            modLoad.Flags,
            modLoad.PDataAddress,
            modLoad.PDataSize);
#endif
      }            
   }

   if (error != XBDM_ENDOFLIST)
      return false;

   DmCloseLoadedModules(pWalkMod);

   mValid = found;

   return found;
#endif   
}

BXEXInfo::BXEXInfo() :
   mBaseAddress(0x82000000),
   mSize(0xFFFFFFF),
   mValid(false)
{
   if (!init())
   {
      trace("BXEXInfo::init() failed!");
   }
}

BXEXInfo::~BXEXInfo()
{
}
#else
uint gXEXDummy;
#endif // XBOX