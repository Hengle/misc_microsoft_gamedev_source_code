#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <psapi.h>
#include <tchar.h>

#include <vector>
#include <string>
#include <algorithm>

class BProcessEnumerator
{
public:
   BProcessEnumerator()
   {
   }
   
   struct BProcessDesc
   {
      std::string mName;
      DWORD mProcessID;
   };
   typedef std::vector<BProcessDesc> BProcessDescArray;
   
   bool enumerate(BProcessDescArray& results)
   {
      results.resize(0);
      
      // Get the list of process identifiers.

      DWORD aProcesses[1024], cbNeeded, cProcesses;
      unsigned int i;

      if ( !EnumProcesses( aProcesses, sizeof(aProcesses), &cbNeeded ) )
         return false;

      // Calculate how many process identifiers were returned.

      cProcesses = cbNeeded / sizeof(DWORD);

      // Print the name and process identifier for each process.

      for (i = 0; i < cProcesses; i++)
      {
         if (aProcesses[i] != 0)
         {
            results.resize(results.size() + 1);
            
            getProcessNameAndID(aProcesses[i], results.back());
         }
      }
      
      return true;
   }
   
private:
   void getProcessNameAndID( DWORD processID, BProcessDesc& desc )
   {
      char szProcessName[MAX_PATH] = "<unknown>";

      // Get a handle to the process.

      HANDLE hProcess = OpenProcess( PROCESS_QUERY_INFORMATION |
         PROCESS_VM_READ,
         FALSE, processID );

      // Get the process name.

      if (NULL != hProcess )
      {
         HMODULE hMod;
         DWORD cbNeeded;

         if ( EnumProcessModules( hProcess, &hMod, sizeof(hMod), 
            &cbNeeded) )
         {
            GetModuleBaseNameA( hProcess, hMod, szProcessName, 
               sizeof(szProcessName)/sizeof(char) );
         }
      }

      desc.mProcessID = processID;
      _strlwr_s(szProcessName, sizeof(szProcessName));
      desc.mName = szProcessName;
      
      CloseHandle( hProcess );
   }
};   

class BWindowEnumerator
{
public:
   BWindowEnumerator()
   {
   }

   struct BWindowDesc
   {
      HWND        mHWnd;
      std::string mName;
      DWORD       mThreadID;
      DWORD       mProcessID;
   };
   typedef std::vector<BWindowDesc> BWindowDescArray;

   bool enumerate(BWindowDescArray& results)
   {
      results.resize(0);
      
      BOOL result = EnumWindows(EnumWindowsProc, (LPARAM)&results);
      if (!result)
         return false;

      return true;
   }
   
private:
   static BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam)
   {
      BWindowDescArray* pArray = (BWindowDescArray*)lParam;
      
      pArray->resize(pArray->size() + 1);
      
      BWindowDesc& desc = pArray->back();
      desc.mHWnd = hwnd;
      
      char buf[512];
      GetWindowTextA(hwnd, buf, sizeof(buf));
      _strlwr_s(buf, sizeof(buf));
      
      desc.mName = buf;
      desc.mThreadID = GetWindowThreadProcessId(hwnd, &desc.mProcessID);
      return TRUE;
   }
};   

static void psExit(int status)
{
   printf("psWait: Exit status: %i\n", status);
   exit(status);
}

static void usage()
{
   printf("psWait built " __DATE__ " " __TIME__ "\n");
   printf("Waits for process to exit.\n");
   printf("\n");
   printf("Usage: psWait processString [-t timeout] [-w windowString]\n");
   printf("processString     Substring to look for in module name\n");
   printf("-t timeout        Timeout in seconds. 0 = exit immediately after testing.\n");
   printf("-w windowString   Also filters by top-level window titles instead of process module names.\n");
   printf("                  To disable filtering by module name, specify an empty string for processString: \"\"\n");
   psExit(EXIT_FAILURE);
}

int main(int argC, char** argV)
{
   if (argC < 2)
      usage();
   
   char processName[MAX_PATH] = { '\0' };
   char windowName[MAX_PATH] = { '\0' };
   UINT timeout = 0xFFFFFFFF;
   bool waitForWindow = false;
   bool gotProcessName = true;
   bool listFlag = false;
   
   for (int i = 1; i < argC; i++)
   {
      const char* pArg = argV[i];
      if ((pArg[0] == '-') || (pArg[0] == '/'))
      {
         switch (tolower(pArg[1]))
         {
            case 'l':
            {
               listFlag = true;
               break;
            }
            case 't':
            {
               if (i == (argC - 1))
                  usage();
               
               i++;
               timeout = atoi(argV[i]);
                                 
               break;
            }
            case 'w':
            {
               if (i == (argC - 1))
                  usage();
               i++;
               
               strcpy_s(windowName, sizeof(windowName), argV[i]);
               _strlwr_s(windowName, sizeof(windowName));
               
               waitForWindow = true;
               break;
            }
            default:
            {
               usage();
            }
         }  
      }
      else
      {
         strcpy_s(processName, sizeof(processName), pArg);
         _strlwr_s(processName, sizeof(processName));
         
         gotProcessName = true;
      }
   }
   
   if (!gotProcessName)
      usage();
      
   const DWORD startTime = GetTickCount();   
   
   int exitStatus = EXIT_SUCCESS;
   for ( ; ; )
   {
      bool processFound = false;
      
      if (waitForWindow)
      {
         BWindowEnumerator windowEnumerator;
         BWindowEnumerator::BWindowDescArray windows;
         windowEnumerator.enumerate(windows);
          
         UINT i;
         for (i = 0; i < windows.size(); i++)
         {
            const BWindowEnumerator::BWindowDesc& desc = windows[i];

            if (strstr(desc.mName.c_str(), windowName) != NULL)
            {
               DWORD processID;
               DWORD threadId = GetWindowThreadProcessId(desc.mHWnd, &processID);
               threadId;

               char szProcessName[MAX_PATH] = "<unknown>";

               // Get a handle to the process.

               HANDLE hProcess = OpenProcess( PROCESS_QUERY_INFORMATION |
                  PROCESS_VM_READ,
                  FALSE, processID );

               // Get the process name.

               if (NULL != hProcess )
               {
                  HMODULE hMod;
                  DWORD cbNeeded;

                  if ( EnumProcessModules( hProcess, &hMod, sizeof(hMod), 
                     &cbNeeded) )
                  {
                     GetModuleBaseNameA( hProcess, hMod, szProcessName, 
                        sizeof(szProcessName)/sizeof(char) );
                  }
               }

               CloseHandle( hProcess );

               _strlwr_s(szProcessName, sizeof(szProcessName));
               
               if (processName[0])
               {                                                      
                  if (strstr(szProcessName, processName) != NULL)
                     processFound = true;
               }
               else
                  processFound = true;
               
               if (processFound)
               {
                  printf("psWait: Found matching window \"%s\", HWND: 0x%08X, ProcessID 0x%08X, Module Name: \"%s\"\n", desc.mName.c_str(), desc.mHWnd, desc.mProcessID, szProcessName);
                  break;
               }
            }
         }
      }
      else
      {
         if (!processName[0])
            usage();
            
         BProcessEnumerator psEnumerator;
         BProcessEnumerator::BProcessDescArray processes;
         psEnumerator.enumerate(processes);
                           
         UINT i;
         if (listFlag)
         {
            for (i = 0; i < processes.size(); i++)
               printf("%s\n", processes[i].mName.c_str());
         }               
            
         for (i = 0; i < processes.size(); i++)
         {
            const BProcessEnumerator::BProcessDesc& desc = processes[i];
            
            if (strstr(desc.mName.c_str(), processName) != NULL)
            {
               printf("psWait: Found matching Process ID 0x%08X, Module Name: \"%s\"\n", processes[i].mProcessID, processes[i].mName.c_str());
               processFound = true;
               break;
            }
         }
      }         
      
      if (!processFound)
         printf("psWait: Process \"%s\" not found\n", processName);
      
      if (!timeout)
      {
         exitStatus = processFound ? EXIT_SUCCESS : EXIT_FAILURE;
         break;
      }
      else if (!processFound)
         break;
                                 
      Sleep(5000);
      DWORD elapsedTime = (GetTickCount() - startTime) / 1000;
            
      if (elapsedTime > timeout)
      {
         printf("psWait: Timed out\n");
         exitStatus = EXIT_FAILURE;
         break;
      }
   }

   psExit(exitStatus);
}

