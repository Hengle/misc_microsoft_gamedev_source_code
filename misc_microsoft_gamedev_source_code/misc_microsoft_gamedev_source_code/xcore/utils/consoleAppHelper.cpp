//------------------------------------------------------------------------------------------------------------------------
//
//  File: consoleAppHelper.cpp
//
//  Copyright (c) 2005-2007, Ensemble Studios
//
//------------------------------------------------------------------------------------------------------------------------
#include "xcore.h"
#include "consoleAppHelper.h"
#include <conio.h>

bool                 BConsoleAppHelper::mQuietFlag;
BString              BConsoleAppHelper::mLogFilename;
BString              BConsoleAppHelper::mErrorLogFilename;
bool                 BConsoleAppHelper::mErrorMessageBox;
FILE*                BConsoleAppHelper::mpLogFile;
FILE*                BConsoleAppHelper::mpErrorLogFile;
uint                 BConsoleAppHelper::mTotalErrorMessages;
uint                 BConsoleAppHelper::mTotalWarningMessages; 
bool                 BConsoleAppHelper::mPauseOnWarnings;
bool                 BConsoleAppHelper::mPauseFlag;
bool                 BConsoleAppHelper::mAppend;
HWND                 BConsoleAppHelper::mhWnd;           
HINSTANCE            BConsoleAppHelper::mhInstance;        
LPDIRECT3D9          BConsoleAppHelper::mpD3D;          
LPDIRECT3DDEVICE9    BConsoleAppHelper::mpD3DDevice;
HMODULE              BConsoleAppHelper::mD3D9DLL;
BNonRecursiveSpinlock BConsoleAppHelper::mConsolePrintMutex;
HANDLE               BConsoleAppHelper::mConOutHandle;
bool                 BConsoleAppHelper::mIsSTDInRedirected;
bool                 BConsoleAppHelper::mIsSTDOutRedirected;
bool                 BConsoleAppHelper::mIsSTDErrRedirected;

void BConsoleAppHelper::setup(void)
{
   gConsoleOutput.init(consoleOutputFunc, 0);
}

void BConsoleAppHelper::determineRedirection(void)
{
   DWORD stdInType  = GetFileType(GetStdHandle(STD_INPUT_HANDLE));
   DWORD stdOutType = GetFileType(GetStdHandle(STD_OUTPUT_HANDLE));
   DWORD stdErrType = GetFileType(GetStdHandle(STD_ERROR_HANDLE));
   
   mIsSTDInRedirected  = (stdInType  != FILE_TYPE_CHAR) && (stdInType != FILE_TYPE_UNKNOWN);
   mIsSTDOutRedirected = (stdOutType != FILE_TYPE_CHAR) && (stdOutType != FILE_TYPE_UNKNOWN);
   mIsSTDErrRedirected = (stdErrType != FILE_TYPE_CHAR) && (stdErrType != FILE_TYPE_UNKNOWN);

   mConOutHandle = CreateFile("CONOUT$", GENERIC_WRITE, FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
}

bool BConsoleAppHelper::init(BCommandLineParser::BStringArray& args, int argc, const char *argv[])
{
   //extern int __argc;          /* count of cmd line args */
   //extern char ** __argv;      /* pointer to table of cmd line args */
   
   determineRedirection();
   
   if (argv)
      BCommandLineParser::createBStringArray(args, argc, argv);
   
   gConsoleOutput.init(consoleOutputFunc, 0);
   
   const BCLParam clParams[] =
   {
      {"quiet",            cCLParamTypeFlag,       &mQuietFlag },
      {"errormessagebox",  cCLParamTypeFlag,       &mErrorMessageBox },
      {"logfile",          cCLParamTypeBStringPtr, &mLogFilename },
      {"errorlogfile",     cCLParamTypeBStringPtr, &mErrorLogFilename },
      {"pauseonwarnings",  cCLParamTypeFlag,       &mPauseOnWarnings},
      {"pause",            cCLParamTypeFlag,       &mPauseFlag },
      {"append",           cCLParamTypeFlag,       &mAppend },
      { NULL }
   };

   BCommandLineParser parser(clParams);

   const bool success = parser.parse(args, true, true);
   if (!success)
   {
      gConsoleOutput.error("%s\n", parser.getErrorString());
      return false;
   }
   
   if (!openLogFile(NULL))
      return false;
      
   if (!openErrorLogFile(NULL))
      return false;
   
   return true;
}

void BConsoleAppHelper::deinit(void)
{
   if ( (mPauseFlag) || ((mPauseOnWarnings) && ((mTotalWarningMessages) || (mTotalErrorMessages))) )
      pause();
   
   deinitD3D();
      
   closeLogFile();
   closeErrorLogFile();
}

void BConsoleAppHelper::printHelp(void)
{
   gConsoleOutput.printf("Other options:\n\n");
   gConsoleOutput.printf(" -quiet                  Disable console output\n");
   gConsoleOutput.printf(" -logfile filename       Create log file\n");
   gConsoleOutput.printf(" -errorlogfile filename  Create error log file\n");
   gConsoleOutput.printf(" -errormessagebox        Display error message boxes\n");
   gConsoleOutput.printf(" -pauseonwarnings        Wait for a keypress if any warnings or errors occur\n");
   gConsoleOutput.printf(" -append                 Append to existing log files\n");
}


void BConsoleAppHelper::pause(void)
{
   if (mIsSTDInRedirected)
      return;
   
   uint prevTotalWarningMessages = mTotalWarningMessages;
   gConsoleOutput.warning("Press a key to continue.\n");
   mTotalWarningMessages = prevTotalWarningMessages;
   
   for ( ; ; )
   {
      if (_getch() != -1)
         break;
   }
}

bool BConsoleAppHelper::openLogFile(const char* pFilename, bool forceAppend)
{
   closeLogFile();
   
   if (!pFilename)
   {
      if (mLogFilename.isEmpty())
         return true;
      pFilename = mLogFilename.getPtr();  
   }
   
   mpLogFile = NULL;
   fopen_s(&mpLogFile, pFilename, (mAppend || forceAppend) ? "a+" : "w+");
   if (!mpLogFile)
   {
      gConsoleOutput.error("Unable to open log file: %s\n", pFilename);
      return false;
   }

   return true;
}

void BConsoleAppHelper::closeLogFile(void)
{
   if (mpLogFile)
      fclose(mpLogFile);
   mpLogFile = NULL;
}

bool BConsoleAppHelper::openErrorLogFile(const char* pFilename, bool forceAppend)
{
   closeErrorLogFile();

   if (!pFilename)
   {
      if (mErrorLogFilename.isEmpty())
         return true;
      pFilename = mErrorLogFilename.getPtr();  
   }

   mpErrorLogFile = NULL;
   fopen_s(&mpErrorLogFile, pFilename, (mAppend || forceAppend) ? "a+" : "w+");
   if (!mpErrorLogFile)
   {
      gConsoleOutput.error("Unable to open error log file: %s\n", pFilename);
      return false;
   }

   return true;
}

void BConsoleAppHelper::closeErrorLogFile(void)
{
   if (mpErrorLogFile)
      fclose(mpErrorLogFile);
   mpErrorLogFile = NULL;
}

void BConsoleAppHelper::consoleOutputFunc(void* data, BConsoleMessageCategory category, const char* pMessage)
{
   data;

   char buf[8192];

   static const char* pCategoryStrings[] = { "", "Debug: ", "", "Warning: ", "Error: ", "", "" };
   const uint cNumCategoryStrings = sizeof(pCategoryStrings)/sizeof(pCategoryStrings[0]);
   cNumCategoryStrings;

   BDEBUG_ASSERT((category >= 0) && (category < cNumCategoryStrings));

   StringCchPrintfA(buf, sizeof(buf), "%s%s", pCategoryStrings[category], pMessage, pCategoryStrings[category]);
   
   mConsolePrintMutex.lock();
         
   if ((!mQuietFlag) || (category == cMsgWarning) || (category == cMsgError))
   {
      if (mIsSTDOutRedirected)
      {
         printf("%s", buf);
      }
      else
      {
         HANDLE currentConsoleHandle = INVALID_HANDLE_VALUE;
         if ((category == cMsgWarning) || (category == cMsgError))
         {
            currentConsoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);

            const DWORD attr = ((category == cMsgWarning) ? FOREGROUND_GREEN : 0) | FOREGROUND_RED | FOREGROUND_INTENSITY;

            if (INVALID_HANDLE_VALUE != currentConsoleHandle)
               SetConsoleTextAttribute(currentConsoleHandle, (WORD)attr);
         }

         printf("%s", buf);

         if (INVALID_HANDLE_VALUE != currentConsoleHandle)
            SetConsoleTextAttribute(currentConsoleHandle, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
      }            
   }

   if (mpLogFile)
   {
      fprintf(mpLogFile, "%s", buf);
      //fflush(mpLogFile);
   }

   if ((mpErrorLogFile) && (category == cMsgError))
   {
      fprintf(mpErrorLogFile, "%s", buf);
      //fflush(mpErrorLogFile);
   }
      
   if ((mErrorMessageBox) && (category == cMsgError) && (buf[0]))
   {
      char filename[_MAX_PATH];
      
      if (0 == GetModuleFileName(GetModuleHandle(NULL), filename, sizeof(filename)))
         strcpy_s(filename, sizeof(filename), "Error");

      MessageBox(NULL, buf, filename, MB_OK | MB_ICONERROR);
   }

   if (category == cMsgError)
      mTotalErrorMessages++;
   else if (category == cMsgWarning)
      mTotalWarningMessages++;

   mConsolePrintMutex.unlock();      
}

//----------------------------------------------------------------

bool BConsoleAppHelper::initD3D(void)
{
   if (mpD3DDevice)
      return true;
   
   return createD3DREF();
}

//----------------------------------------------------------------

void BConsoleAppHelper::deinitD3D(void)
{
   if (!mpD3DDevice)
      return;

   destroyD3DREF();
}

//----------------------------------------------------------------

static LRESULT CALLBACK WindowProc( HWND   hWnd, UINT   msg, WPARAM wParam, LPARAM lParam )
{
   switch( msg )
   {	
   case WM_CLOSE:
   case WM_DESTROY:
      PostQuitMessage(0);
      break;
   default:
      return DefWindowProc( hWnd, msg, wParam, lParam );
   }

   return 0;
}

//----------------------------------------------------------------

bool BConsoleAppHelper::createD3DREF(void)
{
   BDEBUG_ASSERT((!mpD3D) && (!mpD3DDevice));
   
   WNDCLASSEX winClass; 
   MSG        uMsg;

   memset(&uMsg,0,sizeof(uMsg));

   mhInstance = (HINSTANCE)GetModuleHandle(0) ;

   winClass.lpszClassName = "DDXCONV_WINDOWS_CLASS";
   winClass.cbSize        = sizeof(WNDCLASSEX);
   winClass.style         = CS_HREDRAW | CS_VREDRAW;
   winClass.lpfnWndProc   = WindowProc;
   winClass.hInstance     = mhInstance;
   winClass.hIcon	        = LoadIcon(NULL, IDI_WINLOGO);
   winClass.hIconSm	     = LoadIcon(NULL, IDI_WINLOGO);
   winClass.hCursor       = LoadCursor(NULL, IDC_ARROW);
   winClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
   winClass.lpszMenuName  = NULL;
   winClass.cbClsExtra    = 0;
   winClass.cbWndExtra    = 0;

   if( !RegisterClassEx(&winClass) )
   {
      gConsoleOutput.error("RegisterClassEx() failed.");
      return false;
   }

   int nDefaultWidth = 32;
   int nDefaultHeight = 32;
   DWORD dwWindowStyle = 0;//WS_OVERLAPPEDWINDOW | WS_VISIBLE;
   RECT rc;        
   SetRect( &rc, 0, 0, nDefaultWidth, nDefaultHeight );        
   AdjustWindowRect( &rc, dwWindowStyle, false);

   mhWnd = CreateWindowEx( WS_EX_NOACTIVATE, "DDXCONV_WINDOWS_CLASS", 
      "ddxconv",
      dwWindowStyle,
      0, 0, (rc.right-rc.left), (rc.bottom-rc.top), NULL, NULL, mhInstance, NULL );

   if (!mhWnd)
   {
      gConsoleOutput.error("Unable to create dummy window.");
      goto failed;
   }
   
   UpdateWindow( mhWnd );

   mD3D9DLL = LoadLibrary("d3d9.dll");
   if (mD3D9DLL == NULL)
   {
      gConsoleOutput.error("Unable to load D3D9.DLL!\n");
      goto failed;
   }
   
   typedef IDirect3D9* (WINAPI *PDirect3DCreate9)(UINT);
   
   PDirect3DCreate9 pDirect3DCreate9 = (PDirect3DCreate9)GetProcAddress(mD3D9DLL, "Direct3DCreate9");
   if (!pDirect3DCreate9)
   {
      gConsoleOutput.error("Unable to load D3D9.DLL!\n");
      goto failed;
   }
   
   mpD3D = (*pDirect3DCreate9)(D3D_SDK_VERSION);
   if (!mpD3D)
   {
      gConsoleOutput.error("Direct3DCreate9() failed!\n");
      goto failed;
   }

   D3DDISPLAYMODE d3ddm;

   mpD3D->GetAdapterDisplayMode( D3DADAPTER_DEFAULT, &d3ddm );

   D3DPRESENT_PARAMETERS d3dpp;
   ZeroMemory( &d3dpp, sizeof(d3dpp) );

   d3dpp.Windowed               = TRUE;
   d3dpp.SwapEffect             = D3DSWAPEFFECT_DISCARD;
   d3dpp.BackBufferFormat       = D3DFMT_A8R8G8B8;//d3ddm.Format;
   d3dpp.EnableAutoDepthStencil = TRUE;
   d3dpp.AutoDepthStencilFormat = D3DFMT_D16;
   d3dpp.PresentationInterval   = D3DPRESENT_INTERVAL_IMMEDIATE;

   HRESULT hres = mpD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_REF, 
      mhWnd,
      D3DCREATE_SOFTWARE_VERTEXPROCESSING,
      &d3dpp, &mpD3DDevice );

   if (FAILED(hres))
   {
      gConsoleOutput.error("D3D CreateDevice() failed: 0x%X\n", hres);
      goto failed;
   }

   return true;
   
failed:
   if (mpD3D)
   {
      mpD3D->Release();
      mpD3D = NULL;
   }

   if (mD3D9DLL)
   {
      FreeLibrary(mD3D9DLL);
      mD3D9DLL = NULL;
   }

   if (mhWnd)
   {
      DestroyWindow(mhWnd);
      mhWnd = NULL;
   }
   
   UnregisterClass( "DDXCONV_WINDOWS_CLASS", mhInstance );
   
   return false;
}

//-------------------------------------------------

void BConsoleAppHelper::destroyD3DREF(void)
{
   if (mpD3DDevice != NULL)
      mpD3DDevice->Release();

   if (mpD3D != NULL)
      mpD3D->Release();

   if (mhWnd != NULL)
   {
      DestroyWindow(mhWnd);
      mhWnd = NULL;
   }
   
   if (mD3D9DLL)
   {
      FreeLibrary(mD3D9DLL);
      mD3D9DLL = NULL;
   }
   
   UnregisterClass( "DDXCONV_WINDOWS_CLASS", mhInstance );
}

//-------------------------------------------------

bool BConsoleAppHelper::checkOutputFileAttribs(const char* pDstFilename, bool P4CheckOut)
{
   for ( ; ; )
   {
      DWORD dstFileAttr = GetFileAttributes(pDstFilename);
      if (dstFileAttr == INVALID_FILE_ATTRIBUTES)
         break;

      if ((dstFileAttr & FILE_ATTRIBUTE_READONLY) == 0)
         break;

      if (P4CheckOut)
      {
         gConsoleOutput.warning("Checking out read-only file: %s\n", pDstFilename);

         BString cmdLine;
         cmdLine.format("p4 edit \"%s\"", pDstFilename);

         system(cmdLine);
      }

      dstFileAttr = GetFileAttributes(pDstFilename);
      if (dstFileAttr == INVALID_FILE_ATTRIBUTES)
         break;

      if ((dstFileAttr & FILE_ATTRIBUTE_READONLY) == 0)
         break;

      if (getErrorMessageBox())
      {
         BFixedString256 msg(cVarArg, "Can't save to read-only file %s. Check to see if the file is checked out. Retry saving?", pDstFilename);

         if (IDRETRY != MessageBox(NULL, msg.c_str(), "Error", MB_RETRYCANCEL))
         {
            gConsoleOutput.error("Unable to overwrite read-only file: %s\n", pDstFilename);
            return false;
         }
      }
      else
      {
         gConsoleOutput.error("Unable to overwrite read-only file: %s\n", pDstFilename);
         return false;
      }
   }   

   return true;   
}
