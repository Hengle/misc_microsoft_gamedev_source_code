
// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the VC6MEM_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// VC6MEM_API functions as being imported from a DLL, wheras this DLL sees symbols
// defined with this macro as being exported.
#ifdef VC6MEM_EXPORTS
#define VC6MEM_API __declspec(dllexport)
#else
#define VC6MEM_API __declspec(dllimport)
#endif

VC6MEM_API void* __cdecl VC6Mem_new( unsigned int size );
VC6MEM_API void  __cdecl VC6Mem_delete(void* p);



