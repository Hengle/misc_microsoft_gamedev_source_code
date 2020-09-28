
#include "EditorUtilsDLL.h"
//--------------------------------------------------------------------
const int EU_DLL_VERSION = 1;

//--------------------------------------------------------------------
BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved )
{
    return TRUE;
}
//--------------------------------------------------------------------

extern "C" __declspec(dllexport) DWORD getVersion()
   {
      return EU_DLL_VERSION;
   };


