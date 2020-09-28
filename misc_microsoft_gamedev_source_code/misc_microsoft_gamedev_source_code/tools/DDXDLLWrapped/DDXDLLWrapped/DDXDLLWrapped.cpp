
#include <windows.h>
#include <assert.h>
#include "DDXDLLWrapped.h"

#include "estypes.h"
#include "DDXDLL.h"




//--------------------------------------------------------------------
//BOOL APIENTRY DllMain( HANDLE hModule, 
//                      DWORD  ul_reason_for_call, 
//                      LPVOID lpReserved )
//{
//   return TRUE;
//}
//--------------------------------------------------------------------
using namespace System;

namespace DDXDLL_CLI
{
   public ref class DDXDLL_Interface
   {
   public:
      static IDDXDLL7 *gIDDXDLLInterface = NULL;
      static HINSTANCE hDLL = NULL;
      static DDX_DLL_ENTRYPOINT_FUNC lpGetInterface;

      static public void init()
      {
         hDLL = LoadLibrary("ddx7.dll");
         assert(hDLL && "ddx7.dll not found or not loaded");

         lpGetInterface = (DDX_DLL_ENTRYPOINT_FUNC)GetProcAddress(hDLL, DDX_DLL_ENTRYPOINT);
         assert(lpGetInterface&& "DDX_DLL_ENTRYPOINT not found or not loaded");

         gIDDXDLLInterface = lpGetInterface(DDX_DLL_INTERFACE_VERSION);
         assert(gIDDXDLLInterface&& "DDX_DLL_INTERFACE_VERSION not found or not loaded");
      }
      static public void release()
      {
         lpGetInterface = NULL;

         if(gIDDXDLLInterface)
         {
            gIDDXDLLInterface->release();
            gIDDXDLLInterface = NULL;
         }
         
         FreeLibrary(hDLL);
         
         
      }

      static public bool loadDDXTextureInfo(const char* pFilename, int &outWidth, int &outHeight, int& format, int& numMips, int& fullTextureSize )
      {
         
         //go do disk and load the file
         HANDLE hObject =CreateFile(pFilename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
         if(hObject == INVALID_HANDLE_VALUE)
         {
            DWORD errCode = GetLastError();
            assert(hObject != INVALID_HANDLE_VALUE && "File not found" && errCode);
         }

         

         DWORD fileSize = GetFileSize(hObject, 0);


         DWORD nNumberOfBytesToRead = fileSize;
         //BYTE* pDDXData = new BYTE[nNumberOfBytesToRead];
         IntPtr pData = System::Runtime::InteropServices::Marshal::AllocHGlobal(nNumberOfBytesToRead);
         BYTE* pDDXData = (BYTE*)(pData.ToPointer());

         DWORD DDXMemSize;
         BOOL wholeFileRead =  ReadFile(hObject, pDDXData, nNumberOfBytesToRead, &DDXMemSize, NULL);
         assert(wholeFileRead && "Could not read file");

         BOOL closedOK = CloseHandle(hObject);
         assert(closedOK && "Could not close");

         IDDXBuffer *ddxBuff;

         BDDXTextureInfo texInfo;
         gIDDXDLLInterface->unpackDDX(&ddxBuff,texInfo, pDDXData, DDXMemSize, true, false);

         //BDDXDesc desc;
         //bool okdesc = gIDDXDLLInterface->getDesc(lpBuffer,NumberOfBytesRead,desc);
         //assert(okdesc && "Could Not Get Description");

         //delete [] pDDXData;
         System::Runtime::InteropServices::Marshal::FreeHGlobal(pData);

         outWidth = texInfo.mWidth;
         outHeight = texInfo.mHeight;
         format = texInfo.mDataFormat;
         numMips = texInfo.mNumMipChainLevels;
         fullTextureSize = ddxBuff->getSize();

         ddxBuff->release();
        
         return true;
      }

   };
}
