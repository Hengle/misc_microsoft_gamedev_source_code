// File: fileUtils.h
#pragma once

#ifdef XBOX  
class BFileUtils
{
public:
   // Game data directory (on the DVD, Xbox HD, or DVD emulation). This directory cannot be written to!
   static const BCHAR_T*   getXboxGamePath();

   static bool             isXboxGamePath(const BString& path);

   // Directory on cache partition.
   static const BCHAR_T*   getXboxTempPath(void);

   static bool             loadFile   (long dirID, const BString& filename, void** ppFileData, unsigned long* pFileSize);
   static void             unloadFile (void* pFileData);

   static bool             loadFilePhysicalMemory   (long dirID, const BString& filename, void** ppFileData, unsigned long* pFileSize, DWORD dataAlignment=0);
   static void             unloadFilePhysicalMemory (void* pFileData);
};

#undef GetModuleFileName
DWORD GetModuleFileName(HMODULE hModule, LPTSTR lpFilename, DWORD nSize);
#endif