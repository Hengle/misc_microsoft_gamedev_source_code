//============================================================================
//
// File: main.cpp
// Copyright (c) 2005-2006, Ensemble Studios
//
//============================================================================

#include "xcore.h"
#include "xcoreLib.h"

#include <d3d9.h>
#include <d3dx9.h>

#include "DDXDLL.h"
#include "DDXDLLHelper.h"

#include "file\win32FindFiles.h"
#include "resource\ecfUtils.h"
#include "utils\commandLineParser.h"
#include "stream\cfileStream.h"
#include "stream\byteStream.h"
#include "file\win32File.h"
#include "math\halfFloat.h"
#include "xml\xmlDocument.h"
#include "resource\resourceTag.h"

#include "bytePacker.h"
#include "colorUtils.h"
#include "RGBAImage.h"
#include "imageUtils.h"
#include "fileTypes.h"
#include "D3DXTexture.h"
#include "consoleOutput.h"
#include "ImageResample.h"

#include "hdrUtils.h"

#include "DDXSerializedTexture.h"

#include "textureMetadata.h"

#include "pixelFormat.h"

#include <conio.h>

//-------------------------------------------------
// Globals
//-------------------------------------------------

static const uint64 DDX_ID_CHUNK = 0x93F8593B8A8493EF;

const uint cAssetTagCreatorToolVersion = 1;

//-------------------------------------------------
// struct BDDXIDChunkData
//-------------------------------------------------
struct BDDXIDChunkData
{
   BPacked32 mSourceFileID;
   BPacked32 mSourceFileCRC;
   BPacked32 mUpdateIndex;
   
   BDDXIDChunkData() { clear(); }
   
   void clear(void)
   {
      mSourceFileID = 0;
      mSourceFileCRC = 0;
      mUpdateIndex = 0;
   }
};

//-------------------------------------------------

static const uchar gVerticalCrossXYOffsets[] = 
{
   2, 1, // +X
   0, 1, // -X
   1, 0, // +Y
   1, 2, // -Y
   1, 1, // +Z
   1, 3, // -Z
};

//-------------------------------------------------

static bool gQuietFlag;
static FILE* gpLogFile;
static FILE* gpErrorLogFile;
static bool gErrorMessageBox;
static uint gTotalErrorMessages;
static uint gTotalWarningMessages;

static void ConsoleOutputFunc(void* data, BConsoleMessageCategory category, const char* pMessage)
{
   data;
   
   char buf[1024];

   static const char* pCategoryStrings[] = { "", "Debug: ", "", "Warning: ", "Error: " };
   const uint cNumCategoryStrings = sizeof(pCategoryStrings)/sizeof(pCategoryStrings[0]);
   cNumCategoryStrings;

   BDEBUG_ASSERT((category >= 0) && (category < cNumCategoryStrings));

   StringCchPrintfA(buf, sizeof(buf), "%s%s", pCategoryStrings[category], pMessage, pCategoryStrings[category]);
   
   if (!gQuietFlag)
   {
      // rg - This should detect when stdout is redirected.
      
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
   
   if (gpLogFile)
   {
      fprintf(gpLogFile, "%s", buf);
      fflush(gpLogFile);
   }
   
   if ((gpErrorLogFile) && (category == cMsgError))
   {
      fprintf(gpErrorLogFile, "%s", buf);
      fflush(gpErrorLogFile);
   }
   
   if ((gErrorMessageBox) && (category == cMsgError) && (buf[0]))
   {
      MessageBox(NULL, buf, "DDXConv Error", MB_OK | MB_ICONERROR);
   }
   
   if (category == cMsgError)
      gTotalErrorMessages++;
   else if (category == cMsgWarning)
      gTotalWarningMessages++;
}

static void ddxConvWarning(const char* pMsg, ...)
{
   va_list args;
   va_start(args, pMsg);
   char buf[512];
   StringCchVPrintfA(buf, sizeof(buf), pMsg, args);
   va_end(args);

   gConsoleOutput.output(cMsgWarning, "%s", buf);
}

static void ddxConvError(const char* pMsg, ...)
{
   va_list args;
   va_start(args, pMsg);
   char buf[512];
   StringCchVPrintfA(buf, sizeof(buf), pMsg, args);
   va_end(args);

   gConsoleOutput.output(cMsgError, "%s", buf);
}

//-------------------------------------------------

static void ddxConvPrintf(const char* pMsg, ...)
{
   if (gQuietFlag)
      return;
      
   va_list args;
   va_start(args, pMsg);
   char buf[512];
   StringCchVPrintfA(buf, sizeof(buf), pMsg, args);
   va_end(args);

   gConsoleOutput.output(cMsgStatus, "%s", buf);
}

//----------------------------------------------------------------

static HWND                 g_hWnd            = NULL;
static HINSTANCE            hInstance         = NULL;
static LPDIRECT3D9          g_pD3D            = NULL;
static LPDIRECT3DDEVICE9    g_pd3dDevice      = NULL;

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

static bool createD3DREF()
{
   //   Sleep(1 + (rand() % 100));

   WNDCLASSEX winClass; 
   MSG        uMsg;

   memset(&uMsg,0,sizeof(uMsg));

   hInstance = (HINSTANCE)GetModuleHandle(0) ;

   winClass.lpszClassName = "DDXCONV_WINDOWS_CLASS";
   winClass.cbSize        = sizeof(WNDCLASSEX);
   winClass.style         = CS_HREDRAW | CS_VREDRAW;
   winClass.lpfnWndProc   = WindowProc;
   winClass.hInstance     = hInstance;
   winClass.hIcon	        = LoadIcon(NULL, IDI_WINLOGO);
   winClass.hIconSm	     = LoadIcon(NULL, IDI_WINLOGO);
   winClass.hCursor       = LoadCursor(NULL, IDC_ARROW);
   winClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
   winClass.lpszMenuName  = NULL;
   winClass.cbClsExtra    = 0;
   winClass.cbWndExtra    = 0;

   if( !RegisterClassEx(&winClass) )
      return false;

   int nDefaultWidth = 32;
   int nDefaultHeight = 32;
   DWORD dwWindowStyle = 0;//WS_OVERLAPPEDWINDOW | WS_VISIBLE;
   RECT rc;        
   SetRect( &rc, 0, 0, nDefaultWidth, nDefaultHeight );        
   AdjustWindowRect( &rc, dwWindowStyle, false);

   g_hWnd = CreateWindowEx( WS_EX_NOACTIVATE, "DDXCONV_WINDOWS_CLASS", 
      "ddxconv",
      dwWindowStyle,
      0, 0, (rc.right-rc.left), (rc.bottom-rc.top), NULL, NULL, hInstance, NULL );

   if(!g_hWnd)
   {
      ddxConvError("Unable to create dummy window.\n");
      return false;
   }

   UpdateWindow( g_hWnd );

   //CREATE D3D REF OBJECT
   g_pD3D = Direct3DCreate9( D3D_SDK_VERSION );
   if (!g_pD3D)
   {
      ddxConvError("Direct3DCreate9 failed\n");
      return false;
   }

   D3DDISPLAYMODE d3ddm;

   g_pD3D->GetAdapterDisplayMode( D3DADAPTER_DEFAULT, &d3ddm );

   D3DPRESENT_PARAMETERS d3dpp;
   ZeroMemory( &d3dpp, sizeof(d3dpp) );

   d3dpp.Windowed               = TRUE;
   d3dpp.SwapEffect             = D3DSWAPEFFECT_DISCARD;
   d3dpp.BackBufferFormat       = D3DFMT_A8R8G8B8;//d3ddm.Format;
   d3dpp.EnableAutoDepthStencil = TRUE;
   d3dpp.AutoDepthStencilFormat = D3DFMT_D16;
   d3dpp.PresentationInterval   = D3DPRESENT_INTERVAL_IMMEDIATE;

   HRESULT hres = g_pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_REF, 
      g_hWnd,
      D3DCREATE_SOFTWARE_VERTEXPROCESSING,
      &d3dpp, &g_pd3dDevice );

   if (FAILED(hres))
   {
      ddxConvError("CreateDevice failed: 0x%X\n", hres);
   }

   return SUCCEEDED(hres);      
}

//-------------------------------------------------

static void destroyD3DREF()
{
   if( g_pd3dDevice != NULL )
   {
      g_pd3dDevice->Release();
      g_pd3dDevice = NULL;
   }
   
   if( g_pD3D != NULL )
   {
      g_pD3D->Release();
      g_pD3D = NULL;
   }

   if (g_hWnd != NULL)
   {
      DestroyWindow(g_hWnd);
      g_hWnd = NULL;
   }

   if (hInstance)
   {
      UnregisterClass( "DDXCONV_WINDOWS_CLASS", hInstance );
      hInstance = NULL;
   }
}

//-------------------------------------------------
// class BDDXConvCmdLineParams
//-------------------------------------------------
class BDDXConvCmdLineParams
{
public:
   bool mQuietFlag;
   bool mPauseOnWarningsOrErrors;
   
   bool mBestFlag;
   bool mQuickFlag;
   bool mNoAutoMipFlag;
   bool mNormalMapFlag;
   bool mDitherFlag;
   bool mWrapFlag;
   bool mNoPerceptualFlag;
   
   int mMinMipDim;
   bool mSpecifiedMinMipDim;
   int mMipLevels;
   bool mSpecifiedMipLevels;
   
   int mDXTQQuality;
   bool mSpecifiedDXTQQuality;
   
   BCommandLineParser::BStringArray mFileStringArray;
   BString mOutputPathString;
   BString mOutputFilenameString;
   BString mOutputTypeString;
   bool mSpecifiedOutputType;
   BString mAppendString;
   bool mOutSameDirFlag;
   bool mTimestampFlag;
   bool mDeepFlag;
   bool mRecreateFlag;
   bool mNoOverwrite;
   bool mIgnoreErrors;
   
   BString mRegularFormat;
   bool mSpecifiedRegularFormat;
         
   BString mAlphaFormat;
   bool mSpecifiedAlphaFormat;
   
   BString mNormalMapFormat;
   bool mSpecifiedNormalMapFormat;
         
   BString mNormalMapAlphaFormat;
   bool mSpecifiedNormalMapAlphaFormat;
   
   BString mHDRFormat;
   bool mSpecifiedHDRFormat;

   BString mHDRAlphaFormat;
   bool mSpecifiedHDRAlphaFormat;
   
   bool mAutoDetermineNormalMaps;
      
   bool mSimulateFlag;
   bool mStatsFlag;
   
   bool mUseOrigFormat;
   bool mUseNativeData;
   bool mUseNativeDataForNormalMaps;
   bool mForceCompress32BitTextures;
   bool mFileStats;
   bool mXboxFlag;
   BString mLogfile;
   BString mErrorLogfile;
   bool mErrorMessageBox;
   bool mSmartFlag;
   
   bool mIgnoreTextureMetadata;
   bool mPadToPowerOfTwo;
   
   bool mInfoMode;
   bool mHDRConvertMode;
   
   uint mMaxMapSize;
   uint mMaxNormalMapSize;
   BString mTexTypeToProcess;
   
   BString mRulesFilename;
   bool mCompressedProfiles;
   
   bool mAutoDXTQQuality;
   
   BCommandLineParser::BStringArray mExcludeStrings;
   
   uint mNumFileGroups;
   uint mFileGroupIndex;
      
   uint mUpdateIndex;
   bool mSpecifiedUpdateIndex;
   
   bool mDeltaMode;
   
   bool mCheckOut;
   bool mDisableTagChunk;
            
   BDDXConvCmdLineParams() :
      mQuietFlag(false),
      mPauseOnWarningsOrErrors(false),
      mBestFlag(false),
      mQuickFlag(false),
      mNoAutoMipFlag(false),
      mNormalMapFlag(false),
      mDitherFlag(false),
      mWrapFlag(false),
      mNoPerceptualFlag(false),
      mSpecifiedAlphaFormat(false),
      mSpecifiedRegularFormat(false),
      mMinMipDim(1),
      mSpecifiedMinMipDim(false),
      mMipLevels(0),
      mSpecifiedMipLevels(false),
      mDXTQQuality(BDDXDXTQPackParams::cDefaultQualityFactor),
      mSpecifiedDXTQQuality(false),
      mOutSameDirFlag(false),
      mTimestampFlag(false),
      mDeepFlag(false),
      mRecreateFlag(false),
      mAutoDetermineNormalMaps(false),
      mRegularFormat(B("A8B8G8R8")),
      mAlphaFormat(B("A8B8G8R8")),
      mNormalMapFormat(B("A8R8G8B8")),
      mNormalMapAlphaFormat(B("A8R8G8B8")),
      mHDRFormat(B("A16B16G16R16F")),
      mSpecifiedHDRFormat(false),
      mHDRAlphaFormat(B("A16B16G16R16F")),
      mSpecifiedHDRAlphaFormat(false),
      mSpecifiedNormalMapAlphaFormat(false),
      mSpecifiedNormalMapFormat(false),
      mOutputTypeString(B("DDX")),
      mSpecifiedOutputType(false),
      mNoOverwrite(false),
      mIgnoreErrors(false),
      mSimulateFlag(false),
      mStatsFlag(false),
      mUseOrigFormat(false),
      mUseNativeData(false),
      mUseNativeDataForNormalMaps(false),
      mForceCompress32BitTextures(false),
      mFileStats(false),
      mXboxFlag(false),
      mLogfile(B("")),
      mErrorLogfile(B("")),
      mErrorMessageBox(false),
      mSmartFlag(false),
      mIgnoreTextureMetadata(false),
      mPadToPowerOfTwo(true),
      mInfoMode(false),
      mHDRConvertMode(false),
      mMaxMapSize(0),
      mMaxNormalMapSize(0),
      mTexTypeToProcess(""),
      mRulesFilename(""),
      mCompressedProfiles(false),
      mNumFileGroups(1),
      mFileGroupIndex(0),
      mUpdateIndex(0),
      mSpecifiedUpdateIndex(false),
      mDeltaMode(false),
      mCheckOut(false),
      mDisableTagChunk(false),
      mAutoDXTQQuality(false)
   {
      mExcludeStrings.clear();
   }

   bool parse(int argc, const char* const argv[])
   {
      const BCLParam clParams[] =
      {
         {"file", cCLParamTypeBStringArrayPtr, &mFileStringArray },
         {"outpath", cCLParamTypeBStringPtr, &mOutputPathString },
         {"outfile", cCLParamTypeBStringPtr, &mOutputFilenameString },
         {"type", cCLParamTypeBStringPtr, &mOutputTypeString, 1, &mSpecifiedOutputType },
         {"append", cCLParamTypeBStringPtr, &mAppendString },
         {"outsamedir", cCLParamTypeFlag, &mOutSameDirFlag },
         {"nooverwrite", cCLParamTypeFlag, &mNoOverwrite },
         {"timestamp", cCLParamTypeFlag, &mTimestampFlag },
         {"deep", cCLParamTypeFlag, &mDeepFlag },
         {"recreate", cCLParamTypeFlag, &mRecreateFlag },
         {"ignoreerrors", cCLParamTypeFlag, &mIgnoreErrors },
         {"simulate", cCLParamTypeFlag, &mSimulateFlag },
         {"stats", cCLParamTypeFlag, &mStatsFlag },
                  
         {"quiet", cCLParamTypeFlag, &mQuietFlag },
         {"pauseonwarnings", cCLParamTypeFlag, &mPauseOnWarningsOrErrors },
         
         {"best", cCLParamTypeFlag, &mBestFlag },
         {"quick", cCLParamTypeFlag, &mQuickFlag },

         {"noautomip", cCLParamTypeFlag, &mNoAutoMipFlag },
         {"normalmap", cCLParamTypeFlag, &mNormalMapFlag },
         {"dither", cCLParamTypeFlag, &mDitherFlag },
         {"wrap", cCLParamTypeFlag, &mWrapFlag },
         {"noperceptual", cCLParamTypeFlag, &mNoPerceptualFlag },

         {"minmipdim", cCLParamTypeIntPtr, &mMinMipDim, 1, &mSpecifiedMinMipDim },
         {"miplevels", cCLParamTypeIntPtr, &mMipLevels, 1, &mSpecifiedMipLevels },
         {"dxtqquality", cCLParamTypeIntPtr, &mDXTQQuality, 1, &mSpecifiedDXTQQuality },
         
         {"autonormalmap", cCLParamTypeFlag, &mAutoDetermineNormalMaps },
                  
         {"format", cCLParamTypeBStringPtr, &mRegularFormat, 1, &mSpecifiedRegularFormat  },
         {"alphaformat", cCLParamTypeBStringPtr, &mAlphaFormat, 1, &mSpecifiedAlphaFormat },
         {"normalmapformat", cCLParamTypeBStringPtr, &mNormalMapFormat, 1, &mSpecifiedNormalMapFormat  },
         {"normalmapalphaformat", cCLParamTypeBStringPtr, &mNormalMapAlphaFormat, 1, &mSpecifiedNormalMapAlphaFormat  },
         {"hdrformat", cCLParamTypeBStringPtr, &mHDRFormat, 1, &mSpecifiedHDRFormat  },
         {"hdralphaformat", cCLParamTypeBStringPtr, &mHDRAlphaFormat, 1, &mSpecifiedHDRAlphaFormat },
         
         {"usenativedata", cCLParamTypeFlag, &mUseNativeData },
         {"usenativedatanormalmaps", cCLParamTypeFlag, &mUseNativeDataForNormalMaps },
         {"forceCompress32BitTextures", cCLParamTypeFlag, &mForceCompress32BitTextures },
         {"filestats", cCLParamTypeFlag, &mFileStats },
         {"xbox", cCLParamTypeFlag, &mXboxFlag },
         {"logfile", cCLParamTypeBStringPtr, &mLogfile },
         {"errorlogfile", cCLParamTypeBStringPtr, &mErrorLogfile },
         {"errormessagebox", cCLParamTypeFlag, &mErrorMessageBox },
         {"useorigformat", cCLParamTypeFlag, &mUseOrigFormat },         
         {"smart", cCLParamTypeFlag, &mSmartFlag },
         
         {"ignoremetadata", cCLParamTypeFlag, &mIgnoreTextureMetadata },
         {"padtopoweroftwo", cCLParamTypeFlag, &mPadToPowerOfTwo },
         {"info", cCLParamTypeFlag, &mInfoMode },
         {"hdrconvert", cCLParamTypeFlag, &mHDRConvertMode },
         
         {"maxMapSize", cCLParamTypeIntPtr, &mMaxMapSize },
         {"maxNormalMapSize", cCLParamTypeIntPtr, &mMaxNormalMapSize },
         {"texType", cCLParamTypeBStringPtr, &mTexTypeToProcess},
         {"exclude", cCLParamTypeBStringArrayPtr, &mExcludeStrings},
         
         {"rules", cCLParamTypeBStringPtr, &mRulesFilename},
         {"compressedProfiles", cCLParamTypeFlag, &mCompressedProfiles},
         {"numFileGroups", cCLParamTypeIntPtr, &mNumFileGroups},
         {"fileGroup", cCLParamTypeIntPtr, &mFileGroupIndex},
         
         {"updateIndex", cCLParamTypeIntPtr, &mUpdateIndex, 1, &mSpecifiedUpdateIndex},
         
         {"delta", cCLParamTypeFlag, &mDeltaMode},
         {"checkout", cCLParamTypeFlag, &mCheckOut },
         {"noTags", cCLParamTypeFlag, &mDisableTagChunk },
         {"autodxtqquality", cCLParamTypeFlag, &mAutoDXTQQuality }
      };

      const uint numCLParams = sizeof(clParams)/sizeof(clParams[0]);

      BCommandLineParser parser(clParams, numCLParams);

      const bool success = parser.parse(argc, argv);
      
      if (!success)
      {
         ddxConvError("%s\n", parser.getErrorString());
         return false;
      }
      
      if (parser.getUnparsedParams().size())
      {
         ddxConvError("Invalid parameter: %s\n", argv[parser.getUnparsedParams()[0]]);
         return false;
      }
      
      return true;
   }

   void printHelp(void)
   {
      //      --------------------------------------------------------------------------------
      ddxConvPrintf("Usage: ddxconv <options>\n");
      ddxConvPrintf("Supported file formats:\nDDX,DDS,DDT,BMP,TGA,PNG,JPG (DDT is ready only)\n");
      ddxConvPrintf("Options:\n");

      ddxConvPrintf(" Modes:\n");
      ddxConvPrintf(" -info             Read files and display info, but don't convert\n");
      ddxConvPrintf(" -hdrconvert       Convert LDR image to HDR (experimental hack)\n");
      ddxConvPrintf(" -delta            Don't overwrite existing DDX file unless necessary\n");
      ddxConvPrintf("                   Compares update index, DDX formats, metadata ID\n");
      
      ddxConvPrintf(" Path/Filename control:\n");
      ddxConvPrintf(" -file filename    Specify source filename (wildcards okay, may be repeated)\n");
      ddxConvPrintf(" -outpath path     Specify output path\n");
      ddxConvPrintf(" -outfile filename Specify output filename\n");
      ddxConvPrintf(" -append string    Append string to filename\n");
      ddxConvPrintf(" -outsamedir       Write output files to source path\n");
      
      ddxConvPrintf(" -all              Process all supported image files\n");
      ddxConvPrintf(" -timestamp        Compare timestamps and skip files that are not outdated\n");
      ddxConvPrintf(" -deep             Recurse subdirectories\n");
      ddxConvPrintf(" -recreate         Recreate directory structure\n");
      ddxConvPrintf(" -type string      Specified output image type (default is DDX)\n");
      ddxConvPrintf(" -simulate         Only print filenames of files to be processed\n");
      ddxConvPrintf(" -nooverwrite      Don't overwrite existing files\n");
      ddxConvPrintf(" -ignoreerrors     Don't stop on failed files\n");
      ddxConvPrintf(" -stats            Compare output files against source files (LDR and MIP0 only)\n");
      
      ddxConvPrintf(" -numFileGroups #  Set number of file groups\n");
      ddxConvPrintf(" -fileGroup #      Only process files in file group index #\n");
      
      ddxConvPrintf(" -texType string   Restrict processing to texture type (specify suffix: df, nm, etc.)\n");
      ddxConvPrintf(" -exclude string   Exclude all files with string in the filename (multiple OK)\n");
      
      ddxConvPrintf(" -discardalpha     Discard alpha (or save 255's to dest image if it has alpha).\n");
      ddxConvPrintf(" -noautomip        Do not automatically generate mipmap levels. Use original\n");
      ddxConvPrintf("                   file's mipmaps levels (if any).\n");
      ddxConvPrintf(" -minmipdim #      Set minimum mipmap dimension.\n");
      ddxConvPrintf(" -miplevels #      Set number of mipmap chain levels (0=no mipmaps).\n");
      ddxConvPrintf(" -xbox             Create DDX files optimized for Xbox 360\n");
      ddxConvPrintf(" -logfile filename Append to log file\n");
      ddxConvPrintf(" -errorlogfile filename Append to error log file\n");
      ddxConvPrintf(" -errormessagebox  Display error message boxes\n");
      ddxConvPrintf(" -autonormalmap    Automatically detect normal maps\n");
      ddxConvPrintf(" -padtopoweroftwo  Pad non-power of 2 textures\n");
            
      ddxConvPrintf(" -smart            Examining source file to determine settings (use by export script)\n");
      
      ddxConvPrintf(" -maxMapSize #     Restrict maps to specified size or smaller (except cubemaps)\n");
      ddxConvPrintf(" -maxNormalMapSize # Restrict normal maps to specified size or smaller\n");
            
      ddxConvPrintf(" -updateIndex #    Set DDX chunk ID update index value\n");
      ddxConvPrintf(" -checkout         Use P4 to check out output file if read only\n");
      ddxConvPrintf(" -noTags           Don't write resource tag to output DDX file\n");
                        
      ddxConvPrintf("\n");

      ddxConvPrintf("Options valid when converting to DDX:\n");
      ddxConvPrintf(" -useorigformat    Use the DDX format most similar to the source file's\n");
      ddxConvPrintf(" -usenativedata    Copy the source file's data unchanged for all map types\n");
      ddxConvPrintf(" -usenativedatanormalmaps Copy the source file's data unchanged for normal maps\n");
      ddxConvPrintf(" -forceCompress32BitTextures Don't use native data for 32-bit textures\n");
      ddxConvPrintf(" -quick            Lowest quality DXT1/3/5 conversion\n");
      ddxConvPrintf(" -best             Best quality DXT1/3/5 conversion (slower)\n");
      ddxConvPrintf(" -normalmap         Process as normal map.\n");
      ddxConvPrintf(" -dither           Dither image before converting to DXT1, 3, or 5.\n");
      ddxConvPrintf(" -wrap             Use wrap addressing when filtering mipmaps, instead of clamp.\n");
      ddxConvPrintf(" -noperceptual     Don't use perceptual distance metric when convering to DXT.\n");
      ddxConvPrintf(" -rules rules.xml  Rule file\n");
      ddxConvPrintf(" -compressedProfiles Use compressed profiles\n");
      ddxConvPrintf(" -dxtqQuality #    Set DXTQ comp. quality, from 0 (worst) to 6 (best). Default=3\n");
      ddxConvPrintf(" -autodxtqquality  Set DXTQ quality setting depending on texture type.\n");
            
      ddxConvPrintf("\n");

      ddxConvPrintf("Texture format:\n");
      ddxConvPrintf(" -format           Format for LDR files without alpha\n");
      ddxConvPrintf(" -alphaformat      Format for LDR files with alpha\n");
      ddxConvPrintf(" -normalmapformat  Format for normal maps\n");
      ddxConvPrintf(" -normalmapalphaformat Format for normal maps with alpha\n");
      ddxConvPrintf(" -hdrformat        Format for HDR files without alpha\n");
      ddxConvPrintf(" -hdralphaformat   Format for HDR files with alpha\n");
      
      ddxConvPrintf("Supported formats:\n");
      
      for (uint i = 0; i < cDDXDataFormatMax; i++)
         if (i != cDDXDataFormatInvalid)
            ddxConvPrintf("  %s\n", getDDXDataFormatString(static_cast<eDDXDataFormat>(i)));
   }
}; // class BDDXConvCmdLineParams

//-------------------------------------------------
// class BTextureRule
//-------------------------------------------------
class BTextureRule
{
public:
   BTextureRule() : 
      mNormalProfileFileIndex(-1), 
      mCompressedProfileFileIndex(-1),
      mMaxMapSizeOverride(-1), 
      mMaxMapSizeMultiplier(1),
      mMipLevelsOverride(-1),
      mDisableCompressedFormats(false)
   { 
   }
   
   BString mName;
   BString mPathSubstring;
      
   int mNormalProfileFileIndex;
   int mCompressedProfileFileIndex;
      
   int mMaxMapSizeOverride;
   int mMaxMapSizeMultiplier;
   
   int mMipLevelsOverride;
   
   bool mDisableCompressedFormats;
};

typedef BDynamicArray<BTextureRule> BTextureRuleArray;

//-------------------------------------------------
// class BTextureDefinition
//-------------------------------------------------
class BTextureProfile
{
public:
   BTextureProfile() : 
      mAlpha(-1), mHDR(-1), mMaxMapSize(-1), mMipLevels(-1), 
      mFormat(cDDXDataFormatDXT1), mNonCompressedFormat(cDDXDataFormatDXT1), mCompressedFormat(cDDXDataFormatDXT1Q),
      mSpecifiedNonCompressedFormat(false),
      mSpecifiedCompressedFormat(false)
   { 
   }
   
   BString        mName;
   BString        mSuffix;
   BString        mPathSubstring;
   int            mAlpha;
   int            mHDR;
   eDDXDataFormat mFormat;
   eDDXDataFormat mNonCompressedFormat;
   eDDXDataFormat mCompressedFormat;
   int            mMaxMapSize;
   int            mMipLevels;
   BString        mResourceType;
   bool           mSpecifiedNonCompressedFormat;
   bool           mSpecifiedCompressedFormat;
};
typedef BDynamicArray<BTextureProfile> BTextureProfileArray;

//-------------------------------------------------
// class BTextureProfileFile
//-------------------------------------------------
class BTextureProfileFile
{
public:
   BTextureProfileFile() { }
            
   const BString& getName() const { return mName; }
   
   uint getNumProfiles() const { return mProfiles.getSize(); }
   const BTextureProfile& getProfile(uint index) const { return mProfiles[index]; }
         
   bool load(const char* pName, const char* pFilename, bool printNotFoundError = true)
   {
      mName.set(pName);

      BSimpleXMLReader xmlReader;
      HRESULT hres = xmlReader.parse(pFilename);
      if (FAILED(hres))
      {
         if (printNotFoundError)
            ddxConvError("Unable to load texture profile file: %s\n", pFilename);
         return false;
      }

      const BSimpleXMLReader::BNode* pRootNode = xmlReader.getRoot();
      for (uint i = 0; i < pRootNode->getNumChildren(); i++)
      {
         const BSimpleXMLReader::BNode* pNode = pRootNode->getChild(i);

         BTextureProfile& profile = *mProfiles.enlarge(1);

         profile.mName = pNode->getName();

         pNode->getAttributeAsString("filenameSuffix", profile.mSuffix);
         profile.mSuffix.toLower();

         pNode->getAttributeAsString("pathSubstring", profile.mPathSubstring);
         profile.mPathSubstring.toLower();

         BString alphaStr; 
         if (pNode->getAttributeAsString("alpha", alphaStr))
         {
            if (alphaStr == "0")
               profile.mAlpha = FALSE;
            else if (alphaStr == "1")
               profile.mAlpha = TRUE;
            else if (alphaStr == "X")
               profile.mAlpha = -1;
            else
            {  
               ddxConvError("Invalid alpha string \"%s\" in texture profile file: %s\n", alphaStr.getPtr(), pFilename);
               return false;
            }
         }            

         BString hdrStr;
         if (pNode->getAttributeAsString("hdr", hdrStr))
         {
            if (hdrStr == "0")
               profile.mHDR = FALSE;
            else if (hdrStr == "1")
               profile.mHDR = TRUE;
            else if (hdrStr == "X")
               profile.mHDR = -1;
            else
            {  
               ddxConvError("Invalid HDR string \"%s\" in texture profile file: %s\n", hdrStr.getPtr(), pFilename);
               return false;
            }
         }            

         BString formatStr;
         pNode->getChildAsString("format", formatStr);
         if (formatStr.length())
         {
            if (!getTextureFormat(formatStr, profile.mNonCompressedFormat)) 
            {
               ddxConvError("Invalid format \"%s\" in texture profile file: %s\n", formatStr.getPtr(), pFilename);
               return false;
            }
                        
            profile.mSpecifiedNonCompressedFormat = true;
         }

         BString compressedFormatStr;
         pNode->getChildAsString("compressedFormat", compressedFormatStr);
         if (compressedFormatStr.length())
         {
            if (!getTextureFormat(compressedFormatStr, profile.mCompressedFormat)) 
            {
               ddxConvError("Invalid compressed format \"%s\" in texture profile file: %s\n", compressedFormatStr.getPtr(), pFilename);
               return false;
            }
            
            profile.mSpecifiedCompressedFormat = true;
         }            
         
         profile.mFormat = profile.mNonCompressedFormat;

         pNode->getChildAsInt("maxMapSize", profile.mMaxMapSize);
         pNode->getChildAsInt("mipLevels", profile.mMipLevels);
         pNode->getChildAsString("resource", profile.mResourceType);
      }

      ddxConvPrintf("Successfully loaded texture profile file: %s\n", pFilename);

      return true;
   }
   
   int findMatch(const char* pSrcFilename, const char* pSrcFilenameSuffix, const BDDXTextureInfo& textureInfo)
   {
      uint profileEntryIndex;
      for (profileEntryIndex = 0; profileEntryIndex < mProfiles.getSize(); profileEntryIndex++)
      {
         const BTextureProfile& profile = mProfiles[profileEntryIndex];

         if (profile.mSuffix.length())
         {
            if (profile.mSuffix.compare(pSrcFilenameSuffix) != 0)
               continue;
         }

         if (profile.mPathSubstring.length())
         {
            if (strstr(pSrcFilename, profile.mPathSubstring.getPtr()) == NULL)
               continue;
         }

         if (profile.mAlpha != -1)
         {
            if (profile.mAlpha != textureInfo.mHasAlpha)  
               continue;
         }

         if (profile.mHDR != -1)
         {
            if (profile.mHDR != (int)getDDXDataFormatIsHDR(textureInfo.mDataFormat))
               continue;
         }
         
         return profileEntryIndex;
      }
      
      return cInvalidIndex;
   }
   
private:
   static bool getTextureFormat(const char* pStr, eDDXDataFormat& fmt)
   {
      for (uint i = 0; i < cDDXDataFormatMax; i++)
      {
         if (_stricmp(pStr, getDDXDataFormatString(static_cast<eDDXDataFormat>(i))) == 0)
         {
            fmt = static_cast<eDDXDataFormat>(i);
            return true;
         }
      }

      ddxConvError("Invalid texture format: %s\n", pStr);
      return false;
   }
   
   BString              mName;
   BTextureProfileArray mProfiles;
};

typedef BDynamicArray<BTextureProfileFile> BTextureProfileFileArray;

//-------------------------------------------------
// class BDDXConv
//-------------------------------------------------
class BDDXConv
{
   BDDXDLLHelper& mDDXDLLHelper;
   
   BDDXPackParams mProtoPackParams;
   
   BDDXConvCmdLineParams mCmdLineParams;
   uint mUpdateIndex;
   
   BTextureRuleArray mTextureRules;
   BTextureProfileFileArray mTextureProfileFiles;
   
   eDDXDataFormat mRegularFormat;
   eDDXDataFormat mAlphaFormat;
   eDDXDataFormat mNormalMapFormat;
   eDDXDataFormat mNormalMapAlphaFormat;
   eDDXDataFormat mHDRFormat;
   eDDXDataFormat mHDRAlphaFormat;
   
   eFileType mOutputFileType;
   
   uint mNumFilesProcessed;
   uint mNumFilesSkipped;
   uint mNumFailedFiles;
   uint mNumNonImageFiles;
   
   struct BFileStats 
   {
      enum { cMaxResHist = 14 };
      uint mResHist[cMaxResHist];
      uint mFormatHist[cDDXDataFormatMax];
      uint64 mTotalSize;
      uint mNumFiles;
      
      void update(uint res, eDDXDataFormat format, uint64 size)
      {
         uint resLog2 = Math::iLog2(res);
         if (resLog2 < cMaxResHist)
            mResHist[resLog2]++;
         if (format < cDDXDataFormatMax)
            mFormatHist[format]++;
         mTotalSize += size;            
         mNumFiles++;
      }
   };
   
   BFileStats mFileStats[cDDXResourceTypeMax];
   BFileStats mOverallFileStats;
      
   class BFilePath
   {
   public:
      BFilePath() { }
      
      BFilePath(const BString& basePathname, const BString& relPathname, const BString& filename) :
         mBasePathname(basePathname),
         mRelPathname(relPathname),
         mFilename(filename)
      {
      }         
      
      BFilePath(const BFileDesc& fileDesc) :
         mBasePathname(fileDesc.basePathname()),
         mRelPathname(fileDesc.relPathname()),
         mFilename(fileDesc.filename())
      {
      }         
      
      BString& basePathname(void) { return mBasePathname; }
      BString& relPathname(void) { return mRelPathname; }
      BString& filename(void) { return mFilename; }
      
      const BString& basePathname(void) const { return mBasePathname; }
      const BString& relPathname(void) const { return mRelPathname; }
      const BString& filename(void) const { return mFilename; }
      
      BString fullFilename(void) const
      {
         BString filename(mBasePathname);
         strPathAddBackSlash(filename, true);
         filename += mRelPathname;
         strPathAddBackSlash(filename, true);
         filename += mFilename;
         return filename;
      }
   
   private:
      BString mBasePathname;
      BString mRelPathname;
      BString mFilename;
   };
   
   BDynamicArray<BFilePath> mSourceFiles;
   
   LPDIRECT3DDEVICE9 getD3DDevice(void)
   {
      if (!g_pd3dDevice)
      {
         if (!createD3DREF())
         {
            gConsoleOutput.error("Unable to create D3D device! Please try reinstalling the D3D9 SDK.\n");
            exit(100);
         }
      }
      
      return g_pd3dDevice;
   }
        
   static bool getTextureFormat(const char* pStr, eDDXDataFormat& fmt)
   {
      for (uint i = 0; i < cDDXDataFormatMax; i++)
      {
         if (_stricmp(pStr, getDDXDataFormatString(static_cast<eDDXDataFormat>(i))) == 0)
         {
            fmt = static_cast<eDDXDataFormat>(i);
            return true;
         }
      }

      ddxConvError("Invalid texture format: %s\n", pStr);
      return false;
   }
   
   static bool getFileSize(const char* pFilename, uint64& fileSize)
   {
      WIN32_FILE_ATTRIBUTE_DATA attr;
      
      if (0 == GetFileAttributesEx(pFilename, GetFileExInfoStandard, &attr))
         return false;
         
      if (attr.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
         return false;
         
      fileSize = (uint64)attr.nFileSizeLow | ((uint64)attr.nFileSizeHigh << 32U);
      
      return true;
   }
   
   static bool doesFileExist(const char* pFilename)
   {
      const DWORD fullAttributes = GetFileAttributes(pFilename);

      if (fullAttributes == INVALID_FILE_ATTRIBUTES) 
         return false;
      
      return true;
   }
   
   static bool readFileData(const char* pFilename, BByteArray& data)
   {
      BWin32File file;
      if (!file.open(pFilename))
         return false;
      return file.readArray(data);
   }
   
   static bool saveFileData(const char* pFilename, const uchar* pData, uint dataLen)
   {
      BWin32File file;
      if (!file.open(pFilename, BWin32File::cWriteAccess | BWin32File::cSequentialAccess | BWin32File::cCreateAlways))
         return false;
         
      if (!file.write(pData, dataLen))
         return false;

      //pDataHandle->release();

      if (!file.close())
         return false;
         
      return true;
   }         
   
   bool readOnlyFileCheck(const char* pDstFilename)
   {
      for ( ; ; )
      {
         DWORD dstFileAttr = GetFileAttributes(pDstFilename);
         if (dstFileAttr == INVALID_FILE_ATTRIBUTES)
            break;

         if ((dstFileAttr & FILE_ATTRIBUTE_READONLY) == 0)
            break;

         if (mCmdLineParams.mCheckOut)
         {
            ddxConvPrintf("Checking out read-only file: %s\n", pDstFilename);
            
            BString cmdLine;
            cmdLine.format("p4 edit \"%s\"", pDstFilename);
          
            system(cmdLine);
         }
         
         dstFileAttr = GetFileAttributes(pDstFilename);
         if (dstFileAttr == INVALID_FILE_ATTRIBUTES)
            break;

         if ((dstFileAttr & FILE_ATTRIBUTE_READONLY) == 0)
            break;
         
         if (mCmdLineParams.mErrorMessageBox)
         {
            BFixedString256 msg(cVarArg, "Can't save to read-only file %s. The file may not be checked out. Retry saving?", pDstFilename);
            if (IDRETRY != MessageBox(NULL, msg.c_str(), "DDXConv Error", MB_RETRYCANCEL))
            {
               ddxConvError("Unable to overwrite read-only file: %s\n", pDstFilename);
               return false;
            }
         }
         else
         {
            ddxConvError("Unable to overwrite read-only file: %s\n", pDstFilename);
            return false;
         }
      }         

      return true;
   }
   
   static bool isValidNormalMap(const char* pSrcFilename, const BByteArray& textureData, const BDDXTextureInfo& textureInfo, eFileType srcFileType, BTextureMetadata::BTexType texType, const BTextureMetadata::BMetadata& textureMetadata)
   {
      textureMetadata;
      
      if (textureInfo.mDataFormat != cDDXDataFormatA8B8G8R8)
         return false;
      
      char filename[_MAX_PATH];
      StringCchCopy(filename, sizeof(filename), pSrcFilename);
      
      _strlwr_s(filename, sizeof(filename));
      
      float score = 0.0f;
      
      if ((cFileTypeDDT == srcFileType) || (cFileTypeDDX == srcFileType))
      {
         if (textureInfo.mResourceType == cDDXResourceTypeNormalMap)
            score += 1.0f;
         else
            score -= .25f;
      }
      
      if (texType != BTextureMetadata::cTexTypeUnknown)
      {
         if ((texType == BTextureMetadata::cTexTypeNormal) || (texType == BTextureMetadata::cTexTypeDistortion))
            score += 4.0f;
         else
            score -= 2.0f;
      }
            
      if (  (strstr(filename, "normal") != NULL) || 
            (strstr(filename, "nmap") != NULL) || 
            (strstr(filename, "local") != NULL)
          )
      {          
         score += 1.0f;
      }
   
      if (
            (strstr(filename, "diffuse") != NULL) ||
            (strstr(filename, "spec") != NULL)
         )
      {            
         score -= 1.0f;
      }
      
      switch (textureInfo.mOrigDataFormat)
      {
         case cDDXDataFormatDXT1:
         case cDDXDataFormatDXT3:
         case cDDXDataFormatA8:
         //case cDDXDataFormatDXTM:
         //case cDDXDataFormatDXTMA:
         case cDDXDataFormatDXT5Y:
         {
            score -= 1.3f;          
            break;
         }
      }         
         
      const BRGBAImage image((BRGBAColor*)&textureData[0], textureInfo.mWidth, textureInfo.mHeight);
      
      uint numBadPixels = 0;
      for (uint y = 0; y < textureInfo.mHeight; y++)
      {
         for (uint x = 0; x < textureInfo.mWidth; x++)
         {
            const BRGBAColor& color = image(x, y);

            if (color.b < 123)
            {
               numBadPixels++;
               continue;
            }
            
            // Some normal map tools write (0,0,0) for unused texels, so ignore them
            if ((color.r == 128) && (color.g == 128) && (color.b == 128))
               continue;
            
            float r = (color.r - 128) / 127.0f;
            float g = (color.g - 128) / 127.0f;
            float b = (color.b - 128) / 127.0f;

            float l = sqrt(r*r+g*g+b*b);
            if ((l < .94f) || (l > 1.06f))
               numBadPixels++;
         }
      }
      
      const uint totalPixels = textureInfo.mWidth * textureInfo.mHeight;
      float badFraction = float(numBadPixels) / totalPixels;
      score -= Math::Clamp(badFraction - .025f, 0.0f, 1.0f) * 5.0f;
               
      if (score >= 0.0f)
      {
         ddxConvPrintf("Texture appears to be a normal map. (score: %2.2f, bad pixels fraction: %2.2f)\n", score, badFraction);
         return true;
      }
      
      ddxConvPrintf("Texture appears to be a regular map. (score: %2.2f, bad pixels fraction: %2.2f)\n", score, badFraction);
            
      return false;
   }
      
   bool loadDDTFile( 
      const char *pSrcFilename, 
      BByteArray& textureData,
      BDDXTextureInfo& textureInfo,
      const bool convertToABGR)
   {
      BByteArray DDTData;
      if (!readFileData(pSrcFilename, DDTData))
         return false;
         
      if (DDTData.empty())
         return false;         

      IDDXBuffer* pDataHandle = NULL;

      if (!mDDXDLLHelper.getInterface()->DDTtoRGBA8(&pDataHandle, textureInfo, convertToABGR, &DDTData[0], DDTData.size()))
         return false;

      const uint dataSize = pDataHandle->getSize();

      textureData.resize(dataSize);
      memcpy(&textureData[0], pDataHandle->getPtr(), dataSize);

      pDataHandle->release();

      return true;
   }
   
   bool addResourceTagChunk(BECFFileBuilder& ecfFileBuilder, const char* pSrcFilename, const char* pDstFilename)
   {
      pDstFilename;

      if (mCmdLineParams.mDisableTagChunk)
         return true;

      BResourceTagBuilder resourceTagBuilder;

      resourceTagBuilder.setPlatformID(mCmdLineParams.mXboxFlag ? BResourceTagHeader::cPIDXbox : BResourceTagHeader::cPIDPC);

      BString commandLine;
      commandLine.set("DDXCONV -smart -file %s");
      if (mCmdLineParams.mXboxFlag)
         commandLine += " -xbox";
      if (mCmdLineParams.mCompressedProfiles)
         commandLine += " -compressedProfiles";

      resourceTagBuilder.setCreatorToolInfo(commandLine, cAssetTagCreatorToolVersion);

      resourceTagBuilder.setSourceFilename(pSrcFilename);
      if (!resourceTagBuilder.setSourceDigestAndTimeStamp(pSrcFilename))
      {
         gConsoleOutput.error("Failed computing source file digest for file: %s\n", pSrcFilename);
         return false;
      }
            
      if (!resourceTagBuilder.finalize())
      {
         gConsoleOutput.error("Failed computing resource tag for file: %s\n", pSrcFilename);
         return false;
      }

      const BByteArray& tagData = resourceTagBuilder.getFinalizedData();

      BECFChunkData& resourceChunk = ecfFileBuilder.addChunk((uint64)cResourceTagECFChunkID, tagData);
      resourceChunk.setResourceBitFlag(cECFChunkResFlagIsResourceTag, true);

      return true;
   }
         
   bool saveDDXFile(const char* pSrcFilename, const char* pDstFilename, const uchar* pData, const uint dataSize, const BDDXTextureInfo& textureInfo, eDDXPlatform platform, const BTextureMetadata::BMetadata& sourceMetadata)
   {
      if ((!pDstFilename) || (!pData))
         return false;

      if (!Math::IsPow2(textureInfo.mWidth) || !Math::IsPow2(textureInfo.mHeight) || (textureInfo.mWidth < 1) || (textureInfo.mHeight < 1))
         return false;

      if (textureInfo.mResourceType == cDDXResourceTypeCubeMap)      
      {
         if (textureInfo.mWidth != textureInfo.mHeight)
            return false;
      }
      
      IDDXBuffer* pDataHandle = NULL;

      if (!mDDXDLLHelper.getInterface()->packDDX(&pDataHandle, pData, dataSize, textureInfo, platform))
         return false;
         
      BByteStream byteStream(pDataHandle->getPtr(), pDataHandle->getSize());
      
      BECFFileBuilder ecfBuilder;
      if (!ecfBuilder.readFromStream(byteStream))
      {
         pDataHandle->release();
         return false;
      }
                  
      pDataHandle->release();
      pDataHandle = NULL;
      
      BDDXIDChunkData IDChunkData;
      IDChunkData.clear();
      IDChunkData.mSourceFileID = sourceMetadata.mID;
      IDChunkData.mSourceFileCRC = sourceMetadata.mFileCRC;
      IDChunkData.mUpdateIndex = mUpdateIndex;
      
      ecfBuilder.addChunk(DDX_ID_CHUNK, reinterpret_cast<const BYTE*>(&IDChunkData), sizeof(IDChunkData));
      
      if (!addResourceTagChunk(ecfBuilder, pSrcFilename, pDstFilename))
         return false;
      
      BByteArray newDDXFile;
      
      if (!ecfBuilder.writeToFileInMemory(newDDXFile))
         return false;
            
      if (!saveFileData(pDstFilename, newDDXFile.getPtr(), newDDXFile.getSize()))
         return false;
         
      return true;
   }
   
   static bool readDDXFileID(const char* pSrcFilename, BDDXIDChunkData& idChunk)
   {
      idChunk.clear();

      BByteArray DDXData;
      if (!readFileData(pSrcFilename, DDXData))
         return false;

      if (DDXData.empty())
         return false;

      BECFFileReader ecfReader(BConstDataBuffer(DDXData.getPtr(), DDXData.getSize()));
      
      uint chunkLen = 0;
      const BDDXIDChunkData* pChunk = (const BDDXIDChunkData*)ecfReader.getChunkDataByID(DDX_ID_CHUNK, chunkLen);
      if ((pChunk) && (chunkLen == sizeof(BDDXIDChunkData)))
      {
         idChunk = *pChunk;
         return true;
      }
      
      return false;
   }      

   bool loadDDXFile(
      const char* pSrcFilename, 
      BByteArray& textureData, 
      BDDXTextureInfo& textureInfo,
      BDDXDesc* pDDXDesc = NULL)
   {
      BByteArray DDXData;
      if (!readFileData(pSrcFilename, DDXData))
         return false;
         
      if (DDXData.empty())
         return false;
            
      IDDXBuffer* pDataHandle = NULL;
      const bool unpackAllMips = true;
      const bool platformSpecificData = false;
            
      if (pDDXDesc)
      {
         if (!mDDXDLLHelper.getInterface()->getDesc(DDXData.getPtr(), DDXData.getSize(), *pDDXDesc))
            return false;
      }            
      
      if (!mDDXDLLHelper.getInterface()->unpackDDX(&pDataHandle, textureInfo, &DDXData[0], DDXData.size(), unpackAllMips, platformSpecificData))
         return false;

      const uint dataSize = pDataHandle->getSize();

      textureData.resize(dataSize);
      memcpy(&textureData[0], pDataHandle->getPtr(), dataSize);

      pDataHandle->release();

      return true;
   }
         
   static bool loadTGAFile(
      const char* pSrcFilename, 
      BTextureMetadata::BTexType texType,
      BByteArray& textureData, BDDXTextureInfo& textureInfo)
   {
      texType;
      
      BCFileStream stream;
      if (!stream.open(pSrcFilename))
         return false;
         
      BRGBAImage image;
      BPixelFormat origFormat;

      if (!BImageUtils::readTGA(stream, image, &origFormat))
         return false;
      
      textureInfo.mHasAlpha = origFormat.getMappedAlpha();
      textureInfo.mDataFormat = cDDXDataFormatA8B8G8R8;
         
      if ( (Math::IsPow2(image.getHeight())) && (!Math::IsPow2(image.getWidth())) && (image.getHeight() * 6 == image.getWidth()) )
      {
         textureInfo.mWidth = image.getHeight();
         textureInfo.mHeight = image.getHeight();
         textureInfo.mHasAlpha = origFormat.getMappedAlpha();
         textureInfo.mResourceType = cDDXResourceTypeCubeMap;
                  
         ddxConvPrintf("Processing TGA file as a cube map.\n");
         
         // DDX expects:
         // +X
         // -X
         // +Y
         // -Y
         // +Z
         // -Z
         // Texture:
         // -X
         // +Z
         // +X
         // -Z
         // +Y
         // -Y
         for (uint dstFaceIndex = 0; dstFaceIndex < 6; dstFaceIndex++)
         {
            const uint textureDataOfs = textureData.getSize();
            textureDataOfs;
            BRGBAColor* pTextureData = reinterpret_cast<BRGBAColor*>(textureData.enlarge(sizeof(DWORD) * image.getHeight() * image.getHeight()));
                                    
            const uint srcFaceIndex = "204513"[dstFaceIndex] - '0';
            
            for (uint y = 0; y < image.getHeight(); y++)
               for (uint x = 0; x < image.getHeight(); x++)
                  pTextureData[x + y * image.getHeight()] = image(srcFaceIndex * image.getHeight() + x, y);
         }
      }
      else if ( (Math::IsPow2(image.getHeight())) && (!Math::IsPow2(image.getWidth())) && (image.getHeight() / 4 == image.getWidth() / 3) )
      {
         const uint faceSize = image.getHeight() / 4;

         textureInfo.mWidth = faceSize;
         textureInfo.mHeight = faceSize;
         textureInfo.mHasAlpha = origFormat.getMappedAlpha();
         textureInfo.mResourceType = cDDXResourceTypeCubeMap;

         ddxConvPrintf("Processing file as a vertical cross cube map.\n");

         //    0  1  2
         //0     +Y
         //1  -X +Z +X
         //2     -Y
         //3     -Z

         for (uint dstFaceIndex = 0; dstFaceIndex < 6; dstFaceIndex++)
         {
            const uint textureDataOfs = textureData.getSize();
            textureDataOfs;
            BRGBAColor* pTextureData = reinterpret_cast<BRGBAColor*>(textureData.enlarge(sizeof(DWORD) * faceSize * faceSize));
                        
            const bool flip = (dstFaceIndex == 5);

            const uint faceXOffset = gVerticalCrossXYOffsets[dstFaceIndex*2+0];
            const uint faceYOffset = gVerticalCrossXYOffsets[dstFaceIndex*2+1];

            for (uint y = 0; y < faceSize; y++)
               for (uint x = 0; x < faceSize; x++)
               {
                  BRGBAColor& srcPixel = image(faceXOffset * faceSize + x, faceYOffset * faceSize + y);

                  if (flip)
                     pTextureData[(faceSize - 1 - x) + (faceSize - 1 - y) * faceSize] = srcPixel;
                  else
                     pTextureData[x + y * faceSize] = srcPixel;
               }
         }
      }
      else
      {
         textureInfo.mWidth = image.getWidth();
         textureInfo.mHeight = image.getHeight();
         
         textureData.resize(sizeof(DWORD) * image.getWidth() * image.getHeight());
         memcpy(textureData.getPtr(), &image(0, 0), textureData.getSizeInBytes());
      }
                     
      return true;
   }
   
   static bool saveTGAFile(
      const char* pDstFilename,
      const uchar* pData,
      const uint dataSize,
      const BDDXTextureInfo& textureInfo,
      const uint maxMipLevelsToSave,
      const BTextureMetadata::BMetadata& textureMetadata)
   {
      if (textureInfo.mDataFormat != cDDXDataFormatA8B8G8R8)
         return false;
         
      maxMipLevelsToSave;
      
      BRGBAImage tempImage;
      
      BDDXSerializedTexture srcTexture(pData, dataSize, textureInfo);
      const uint levelBytesPerLine = (srcTexture.getWidth(0) * getDDXDataFormatBitsPerPixel(textureInfo.mDataFormat)) / 8;
      
      if (textureInfo.mResourceType == cDDXResourceTypeCubeMap)
      {
         tempImage.setSize(textureInfo.mHeight * 6, textureInfo.mHeight);
         
         for (uint dstFaceIndex = 0; dstFaceIndex < 6; dstFaceIndex++)
         {
            //const uint srcFaceIndex = "204513"[dstFaceIndex] - '0';
            const uint srcFaceIndex = "140523"[dstFaceIndex] - '0';
            
            const uchar* pLevelData = srcTexture.getSurfaceData(srcFaceIndex, 0);
            
            for (uint y = 0; y < textureInfo.mHeight; y++)
               memcpy(&tempImage(dstFaceIndex * textureInfo.mHeight, y), pLevelData + y * levelBytesPerLine, levelBytesPerLine);
         }
      }
      else
      {
         tempImage.setSize(textureInfo.mWidth, textureInfo.mHeight);
         
         memcpy(&tempImage(0, 0), pData, levelBytesPerLine * textureInfo.mHeight);
      }

      BCFileStream dstStream;
      if (!dstStream.open(pDstFilename, cSFWritable|cSFSeekable))
         return false;
            
      BTGAImageType tgaImageType = textureInfo.mHasAlpha ? cTGAImageTypeBGRA : cTGAImageTypeBGR;
                     
      if (!BImageUtils::writeTGA(dstStream, tempImage, tgaImageType))
         return false;
      
      BTextureMetadata::BMetadata dstTextureMetadata(textureMetadata);
      if (!textureMetadata.mValid)
      {
         dstTextureMetadata.mMipmaps = (textureInfo.mNumMipChainLevels > 0);
         dstTextureMetadata.mAlpha = textureInfo.mHasAlpha != 0;
         dstTextureMetadata.mTilable = false;
      }

// This was used during the conversion to DDX. A lot of UI textures where marked as needing mips.
//#define FORCE_MIPS_HACK
#ifdef FORCE_MIPS_HACK      
      bool uiTexture = (strstr(pDstFilename, "art\\ui") != NULL);
      if (uiTexture)
      {
         if (dstTextureMetadata.mMipmaps)
         {
            dstTextureMetadata.mMipmaps = false;
            gConsoleOutput.output(cMsgWarning, "Forcing texture metadata to NOT have mipmaps\n");
         }
      }
      else if (!dstTextureMetadata.mMipmaps)
      {
         dstTextureMetadata.mMipmaps = true;
         gConsoleOutput.output(cMsgWarning, "Forcing texture metadata to have mipmaps\n");
      }
#endif      
      
      ddxConvPrintf("Writing Texture Metadata: Mips: %i, Alpha: %i, Tilable: %i\n", dstTextureMetadata.mMipmaps, dstTextureMetadata.mAlpha, dstTextureMetadata.mTilable);
      if (!dstTextureMetadata.write(dstStream))
         return false;
      
      if (!dstStream.close())
         return false;
         
      return true;
   }
   
   static bool loadHDRFile(
      const char* pSrcFilename, 
      eFileType srcFileType,
      BTextureMetadata::BTexType texType,
      BByteArray& textureData, BDDXTextureInfo& textureInfo)
   {
      texType;

      BCFileStream stream;
      if (!stream.open(pSrcFilename, cSFReadable | cSFSeekable))
         return false;

      BRGBAFImage image;

      switch (srcFileType)
      {
         case cFileTypeTIF:
         {
            if (!BImageUtils::readTiff(stream, image))
               return false;
            break;               
         }
         case cFileTypeHDR:
         {
            if (!BImageUtils::readHDR(stream, image))
               return false;
            break;               
         }               
         default:
            return false;
      }         

      // Don't currently supported HDR image alpha, but we could.
      textureInfo.mHasAlpha = false;
      textureInfo.mDataFormat = cDDXDataFormatA16B16G16R16F;

      if ( (Math::IsPow2(image.getHeight())) && (!Math::IsPow2(image.getWidth())) && (image.getHeight() * 6 == image.getWidth()) )
      {
         textureInfo.mWidth = image.getHeight();
         textureInfo.mHeight = image.getHeight();
         textureInfo.mHasAlpha = false;
         textureInfo.mResourceType = cDDXResourceTypeCubeMap;

         ddxConvPrintf("Processing file as a cube map.\n");

         // DDX expects:
         // +X
         // -X
         // +Y
         // -Y
         // +Z
         // -Z
         // Texture:
         // -X
         // +Z
         // +X
         // -Z
         // +Y
         // -Y
         for (uint dstFaceIndex = 0; dstFaceIndex < 6; dstFaceIndex++)
         {
            const uint textureDataOfs = textureData.getSize();
            textureDataOfs;
            BRGBAColor16* pTextureData = reinterpret_cast<BRGBAColor16*>(textureData.enlarge(sizeof(uint64) * image.getHeight() * image.getHeight()));

            const uint srcFaceIndex = "204513"[dstFaceIndex] - '0';

            for (uint y = 0; y < image.getHeight(); y++)
               for (uint x = 0; x < image.getHeight(); x++)
               {
                  BRGBAColorF& srcPixel = image(srcFaceIndex * image.getHeight() + x, y);
                  
                  BRGBAColor16 dstPixel;
                  for (uint c = 0; c < 4; c++)
                     dstPixel[c] = HalfFloat::FloatToHalfLittleEndian(srcPixel[c]);
                  
                  pTextureData[x + y * image.getHeight()] = dstPixel;
               }
         }
      }
      else if ( (Math::IsPow2(image.getHeight())) && (!Math::IsPow2(image.getWidth())) && (image.getHeight() / 4 == image.getWidth() / 3) )
      {
         const uint faceSize = image.getHeight() / 4;
         
         textureInfo.mWidth = faceSize;
         textureInfo.mHeight = faceSize;
         textureInfo.mHasAlpha = false;
         textureInfo.mResourceType = cDDXResourceTypeCubeMap;

         ddxConvPrintf("Processing file as a vertical cross cube map.\n");
       
         //    0  1  2
         //0     +Y
         //1  -X +Z +X
         //2     -Y
         //3     -Z
         
         for (uint dstFaceIndex = 0; dstFaceIndex < 6; dstFaceIndex++)
         {
            const uint textureDataOfs = textureData.getSize();
            textureDataOfs;
            BRGBAColor16* pTextureData = reinterpret_cast<BRGBAColor16*>(textureData.enlarge(sizeof(uint64) * faceSize * faceSize));
                        
            const bool flip = (dstFaceIndex == 5);

            const uint faceXOffset = gVerticalCrossXYOffsets[dstFaceIndex*2+0];
            const uint faceYOffset = gVerticalCrossXYOffsets[dstFaceIndex*2+1];
            
            for (uint y = 0; y < faceSize; y++)
               for (uint x = 0; x < faceSize; x++)
               {
                  BRGBAColorF& srcPixel = image(faceXOffset * faceSize + x, faceYOffset * faceSize + y);

                  BRGBAColor16 dstPixel;
                  for (uint c = 0; c < 4; c++)
                     dstPixel[c] = HalfFloat::FloatToHalfLittleEndian(srcPixel[c]);
                  
                  if (flip)
                     pTextureData[(faceSize - 1 - x) + (faceSize - 1 - y) * faceSize] = dstPixel;
                  else
                     pTextureData[x + y * faceSize] = dstPixel;
               }
         }
      }
      else 
      {
         textureInfo.mWidth = image.getWidth();
         textureInfo.mHeight = image.getHeight();

         textureData.resize(sizeof(uint64) * image.getWidth() * image.getHeight());
         
         BRGBA16Image halfImage((BRGBAColor16*)textureData.getPtr(), image.getWidth(), image.getHeight());
         
         BHDRUtils::packFloatToHalfImage(image, halfImage);
      }

      return true;
   }      
   
   static bool saveHDRFile(
      const char* pDstFilename,
      const uchar* pData,
      const uint dataSize,
      const BDDXTextureInfo& textureInfo,
      const uint maxMipLevelsToSave,
      const BTextureMetadata::BMetadata& textureMetadata)
   {
      dataSize;
      maxMipLevelsToSave;
      
      const float gamma = 2.2f;
      
      BCFileStream dstStream;
      if (!dstStream.open(pDstFilename, cSFWritable|cSFSeekable))
         return false;
      
      BRGBAFImage floatImage;
      
      BDDXSerializedTexture srcTexture(pData, dataSize, textureInfo);
                 
      if (textureInfo.mResourceType == cDDXResourceTypeCubeMap)
      {
         floatImage.setSize(textureInfo.mHeight * 6, textureInfo.mHeight);

         for (uint dstFaceIndex = 0; dstFaceIndex < 6; dstFaceIndex++)
         {
            const uint srcFaceIndex = "140523"[dstFaceIndex] - '0';

            const uchar* pLevelData = srcTexture.getSurfaceData(srcFaceIndex, 0);
                        
            BRGBAFImage faceFloatImage;
            if (textureInfo.mDataFormat == cDDXDataFormatA16B16G16R16F)
            {
               BRGBA16Image halfImage((BRGBAColor16*)pLevelData, textureInfo.mHeight, textureInfo.mHeight);
               BHDRUtils::unpackHalfFloatImage(halfImage, faceFloatImage);
            }
            else if (textureInfo.mDataFormat == cDDXDataFormatA8B8G8R8)
            {
               BRGBAImage rgb8Image((BRGBAColor*)pLevelData, textureInfo.mHeight, textureInfo.mHeight);
               BHDRUtils::convertRGB8ToFloatImage(rgb8Image, faceFloatImage, gamma);
            }
            else
            {
               return false;
            }

            for (uint y = 0; y < textureInfo.mHeight; y++)
               memcpy(&floatImage(dstFaceIndex * textureInfo.mHeight, y), faceFloatImage.getScanlinePtr(y), textureInfo.mHeight * sizeof(BRGBAColorF));
         }
      }
      else
      {
         if (textureInfo.mDataFormat == cDDXDataFormatA16B16G16R16F)
         {
            BRGBA16Image halfImage((BRGBAColor16*)pData, textureInfo.mWidth, textureInfo.mHeight);
            BHDRUtils::unpackHalfFloatImage(halfImage, floatImage);
         }
         else if (textureInfo.mDataFormat == cDDXDataFormatA8B8G8R8)
         {
            BRGBAImage rgb8Image((BRGBAColor*)pData, textureInfo.mWidth, textureInfo.mHeight);

            BHDRUtils::convertRGB8ToFloatImage(rgb8Image, floatImage, gamma);
         }
         else
         {
            return false;
         }
      }
      
      if (!BImageUtils::writeHDR(dstStream, floatImage))
         return false;
               
      BTextureMetadata::BMetadata dstTextureMetadata(textureMetadata);
      if (!textureMetadata.mValid)
      {
         dstTextureMetadata.mMipmaps = (textureInfo.mNumMipChainLevels > 0);
         dstTextureMetadata.mAlpha = textureInfo.mHasAlpha != 0;
         dstTextureMetadata.mTilable = false;
      }
      
      ddxConvPrintf("Writing Texture Metadata: Mips: %i, Alpha: %i, Tilable: %i\n", dstTextureMetadata.mMipmaps, dstTextureMetadata.mAlpha, dstTextureMetadata.mTilable);
      if (!dstTextureMetadata.write(dstStream))
         return false;         
         
      return true;
   }      
         
   static void printPackParams(const BDDXPackParams& packParams)
   {
      ddxConvPrintf("Packer Parameters:\n");
      ddxConvPrintf("        Max MipChain Levels: %i\n", packParams.mMipChainLevels);
      ddxConvPrintf("                Data Format: %s\n", getDDXDataFormatString(packParams.mDataFormat));
      ddxConvPrintf("              Resource type: %s\n", getDDXResourceTypeString(packParams.mResourceType));

      ddxConvPrintf("                      Flags: ");
      if (packParams.mPackerFlags & BDDXPackParams::cUseWrapFiltering) ddxConvPrintf("usewrapfiltering ");
      if (packParams.mPackerFlags & BDDXPackParams::cRenormalize)    ddxConvPrintf("renormalize ");
      if (packParams.mPackerFlags & BDDXPackParams::cDXTDithering)   ddxConvPrintf("dxtdithering ");
      if (packParams.mPackerFlags & BDDXPackParams::cGenerateMips)   ddxConvPrintf("generatemips ");
      if (packParams.mPackerFlags & BDDXPackParams::cDXTFast)        ddxConvPrintf("fastDXT ");
      if (packParams.mPackerFlags & BDDXPackParams::cDXTBest)        ddxConvPrintf("bestDXT ");
      if (packParams.mPackerFlags & BDDXPackParams::cPerceptual)     ddxConvPrintf("perceptual ");
      ddxConvPrintf("\n");

      //ddxConvPrintf("   DXTQ Color Codebook Size: %i\n", packParams.mDXTQParams.mColorCodebookSize);
      //ddxConvPrintf("DXTQ Selector Codebook Size: %i\n", packParams.mDXTQParams.mSelectorCodebookSize);
   }
   
   static void printTextureInfo(const BDDXTextureInfo& textureInfo)
   {
      ddxConvPrintf("           Dimension: %ix%i\n", textureInfo.mWidth, textureInfo.mHeight);
      ddxConvPrintf("       Mipchain Size: %i\n", textureInfo.mNumMipChainLevels);
      ddxConvPrintf("              Format: %s\n", getDDXDataFormatString(textureInfo.mDataFormat));
      ddxConvPrintf("           Has Alpha: %i\n", textureInfo.mHasAlpha);
      ddxConvPrintf("        ResourceType: %s\n", getDDXResourceTypeString(textureInfo.mResourceType));
      ddxConvPrintf("         Orig Format: %s\n", getDDXDataFormatString(textureInfo.mOrigDataFormat));
      ddxConvPrintf("           HDR Scale: %f\n", textureInfo.mHDRScale);
   }
   
   static void printTextureMetadata(const BTextureMetadata::BMetadata& textureMetadata)
   {
      if (textureMetadata.mValid)
      {
         ddxConvPrintf("Read texture metadata: Size: %u, CRC: 0x%08X, ID: 0x%08X\n  Mips: %i, Alpha: %i, Tilable: %i, ConvertHDR: %i, HDRScale: %2.3f\n", 
            textureMetadata.mFileSize,
            textureMetadata.mFileCRC,
            textureMetadata.mID, 
            textureMetadata.mMipmaps, 
            textureMetadata.mAlpha, 
            textureMetadata.mTilable, 
            textureMetadata.mConvertToHDR, 
            textureMetadata.mConvertToHDRScale);
      }            
      else
      {
         ddxConvPrintf("No texture metadata found.\n");
      }
   }         
   
   bool loadImageFile(const char* pSrcFilename, const eFileType srcFileType, BByteArray& nativeTextureData, BDDXTextureInfo& nativeTextureInfo, BTextureMetadata::BTexType& texType, bool D3DXGenerateMips, BDDXDesc* pDDXDesc = NULL)
   {
      texType = BTextureMetadata::determineTextureType(pSrcFilename);         
            
      bool readStatus = false;      
      switch (srcFileType)
      {
         case cFileTypeDDX:
         {
            readStatus = loadDDXFile(pSrcFilename, nativeTextureData, nativeTextureInfo, pDDXDesc);
            break;
         }
         case cFileTypeDDT:
         {
            readStatus = loadDDTFile(pSrcFilename, nativeTextureData, nativeTextureInfo, false);
            break;
         }
         case cFileTypeTGA:
         {
            readStatus = loadTGAFile(pSrcFilename, texType, nativeTextureData, nativeTextureInfo);
            break;
         }
         case cFileTypeHDR:
         case cFileTypeTIF:
         {
            readStatus = loadHDRFile(pSrcFilename, srcFileType, texType, nativeTextureData, nativeTextureInfo);
            break;
         }
         case cFileTypeDDS:
         case cFileTypeBMP:
         case cFileTypePNG:
         case cFileTypeJPG:
         {
            BD3DXTexture texture(getD3DDevice());
            readStatus = texture.loadD3DXFile(pSrcFilename, nativeTextureData, nativeTextureInfo, D3DXGenerateMips, false);
            break;
         }
      }
      
      return readStatus;
   }      
   
   bool displayFileInfo(const char* pSrcFilename, const eFileType srcFileType)
   {
      BByteArray nativeTextureData;
      BDDXTextureInfo nativeTextureInfo;
      BTextureMetadata::BTexType texType;

      BDDXDesc ddxDesc;
      if (!loadImageFile(pSrcFilename, srcFileType, nativeTextureData, nativeTextureInfo, texType, false, &ddxDesc))
      {
         ddxConvError("Unable to read file %s\n", pSrcFilename);
         return false;
      }
            
      BTextureMetadata::BMetadata textureMetadata;
      
      if (!mCmdLineParams.mIgnoreTextureMetadata)  
      {
         textureMetadata.read(pSrcFilename);

         printTextureMetadata(textureMetadata);
      }                  
      
      if (srcFileType == cFileTypeDDX)
      {
         BDDXIDChunkData idChunk;
         readDDXFileID(pSrcFilename, idChunk);
            
         ddxConvPrintf("\nDDX desc:\n");
         ddxConvPrintf("      Source File ID: 0x%08X\n", (uint)idChunk.mSourceFileID);
         ddxConvPrintf("     Source File CRC: 0x%08X\n", (uint)idChunk.mSourceFileCRC);
         ddxConvPrintf("        Update Index: %u\n", (uint)idChunk.mUpdateIndex);
         ddxConvPrintf("            Platform: %s\n", getDDXPlatformString(ddxDesc.mPlatform));
         ddxConvPrintf("     Creator version: %u\n", ddxDesc.mCreatorVersion);
         ddxConvPrintf("Min required version: %u\n", ddxDesc.mMinRequiredVersion);
         ddxConvPrintf("               Width: %u\n", ddxDesc.mWidth);
         ddxConvPrintf("              Height: %u\n", ddxDesc.mHeight);
         ddxConvPrintf("     Total data size: %u\n", ddxDesc.mTotalDataSize);
         ddxConvPrintf("      Mip chain size: %u\n", ddxDesc.mMipChainSize);
         
         ddxConvPrintf("         Data format: %s (%u)\n", getDDXDataFormatString(ddxDesc.mDataFormat), ddxDesc.mDataFormat);
         ddxConvPrintf("     Mip 0 data size: %u\n", ddxDesc.mMip0DataSize);
         ddxConvPrintf(" Mip chain data size: %u\n", ddxDesc.mMipChainDataSize);
         ddxConvPrintf("        Header flags: 0x%08X\n", ddxDesc.mHeaderFlags);
         ddxConvPrintf("       Resource type: %s\n", getDDXResourceTypeString(ddxDesc.mResourceType));
         ddxConvPrintf("\n");
      }
            
      ddxConvPrintf("Texture info:\n");
      printTextureInfo(nativeTextureInfo);
      
      return true;
   }
         
   void convertLDRToHDRImage(BRGBAFImage& dstImage, const BRGBAImage& srcImage, float scale)
   {
      dstImage.setSize(srcImage.getWidth(), srcImage.getHeight());
      
      float g[256];
      for (uint i = 0; i < 256; i++)
         g[i] = powf(i * (1.0f / 255.0f), 2.2f);

      for (uint y = 0; y < srcImage.getHeight(); y++)
      {
         for (uint x = 0; x < srcImage.getWidth(); x++)
         {
            BRGBAColor s(srcImage(x, y));
            BRGBAColorF d;

            float l = g[s.a];

            d.r = g[s.r] * l * scale;
            d.g = g[s.g] * l * scale;
            d.b = g[s.b] * l * scale;
            d.a = 1.0f;

            dstImage(x, y) = d;            
         }
      }
   }
   
   bool hdrConvertFile(const char* pSrcFilename, const eFileType srcFileType, const char* pDstFilename, const eFileType dstFileType)
   {
      if (!readOnlyFileCheck(pDstFilename))
         return false;
         
      if (srcFileType != cFileTypeTGA)
      {
         ddxConvError("This mode only supports .TGA source files.\n");
         return false;
      }         
      
      if (dstFileType != cFileTypeHDR)
      {
         ddxConvError("This mode only supports .HDR destination files.\n");
         return false;
      }
         
      BCFileStream srcStream;
      if (!srcStream.open(pSrcFilename))
      {  
         ddxConvError("Unable to read source file: %s\n", pSrcFilename);
         return false;
      }

      BRGBAImage srcImage;
      BPixelFormat origFormat;

      if (!BImageUtils::readTGA(srcStream, srcImage, &origFormat))
      {
         ddxConvError("Failed reading TGA file: %s\n", pSrcFilename);
         return false;
      }
      
      const float scale = 32.0f;         
      
      BRGBAFImage dstImage;
      
      convertLDRToHDRImage(dstImage, srcImage, scale);
      
      BCFileStream dstStream;
      if (!dstStream.open(pDstFilename, cSFWritable | cSFSeekable))
      {
         ddxConvError("Can't open file %s\n", pDstFilename);
         return false;
      }
      
      if (!BImageUtils::writeHDR(dstStream, dstImage))
      {
         ddxConvPrintf("Error writing file %s\n", pDstFilename);
         return false;
      }
      
      if (!dstStream.close())
      {
         ddxConvPrintf("Error writing file %s\n", pDstFilename);
         return false;
      }
      
      return true;
   }
   
   bool downsampleTexture(BDDXTextureInfo& textureInfo, BByteArray& textureData, const BTextureMetadata::BMetadata& textureMetadata, uint maxMapSize)
   {
      if (textureInfo.mDataFormat != cDDXDataFormatA8B8G8R8)
         return false;
      if (textureInfo.mResourceType == cDDXResourceTypeCubeMap)
         return false;
         
      const uint dstWidth = Math::Min<uint>(maxMapSize, textureInfo.mWidth);
      const uint dstHeight = Math::Min<uint>(maxMapSize, textureInfo.mHeight);
      
      BRGBAImage srcImage((BRGBAColor*)textureData.getPtr(), textureInfo.mWidth, textureInfo.mHeight);
      BRGBAImage dstImage(dstWidth, dstHeight);
      
      const bool normalMap = (textureInfo.mResourceType == cDDXResourceTypeNormalMap);
      
      if (normalMap)
      {
         BImageResampler resampler(1.0f);
         const bool resampleStatus = resampler.resample(srcImage, dstImage, dstWidth, dstHeight, 3, textureMetadata.mTilable, false, "gaussian", .85f);
         if (!resampleStatus)
            return false;
         BImageUtils::renormalizeImage(dstImage, dstImage);
      }
      else
      {
         BImageResampler resampler;
         const bool resampleStatus = resampler.resample(srcImage, dstImage, dstWidth, dstHeight, 4, textureMetadata.mTilable, true);
         if (!resampleStatus)
            return false;
      }
            
      textureInfo.mWidth = dstWidth;
      textureInfo.mHeight = dstHeight;
      textureInfo.mNumMipChainLevels = 0;
      
      textureData.resize(dstWidth * dstHeight * sizeof(DWORD));
      memcpy(textureData.getPtr(), dstImage.getPtr(), textureData.getSizeInBytes());
            
      return true;
   }
   
   BString getFilenameSuffix(const BString& srcFilename)
   {
      BString suffix;
      strPathGetFilename(srcFilename, suffix);
      strPathRemoveExtension(suffix);
      
      int l = suffix.length();
      if (suffix.length() < 3)
         suffix.empty();
      else if (suffix.getPtr()[l - 3] == '_')
         suffix.crop(l - 3, l - 1);
      else 
         suffix.empty();
         
      return suffix;
   }
   
   bool findTextureProfile(BTextureProfile& textureProfile, bool& foundTextureProfile, const char* pSrcFilename, const BDDXTextureInfo& textureInfo)
   {
      foundTextureProfile = false;
      
      BString overrideProfileFilename;
      strPathGetDirectory(BString(pSrcFilename), overrideProfileFilename, true);
      overrideProfileFilename += "textureProfiles.txp";
      
      BTextureProfileFile overrideProfiles;
      const bool hasOverrideProfiles = overrideProfiles.load("OverrideProfile", overrideProfileFilename, false);
      if (hasOverrideProfiles)
         ddxConvPrintf("Found local texture override profiles: %s\n", overrideProfileFilename.getPtr());
      
      BString srcFilename(pSrcFilename);
      srcFilename.toLower();
      
      const BString srcFilenameSuffix(getFilenameSuffix(srcFilename));
                  
      if (!mTextureRules.getSize())
         return true;
         
      uint ruleIndex;
      for (ruleIndex = 0; ruleIndex < mTextureRules.getSize(); ruleIndex++)
      {
         if (mTextureRules[ruleIndex].mPathSubstring.length() == 0)
            break;
         
         if (strstr(srcFilename.getPtr(), mTextureRules[ruleIndex].mPathSubstring.getPtr()) != NULL)
            break;
      }

      if (ruleIndex == mTextureRules.getSize())
      {
         ddxConvError("Unable to find texture rule for file: %s\n", pSrcFilename);
         return false;
      }
      
      const BTextureRule& textureRule = mTextureRules[ruleIndex];

      BTextureProfileFile* pProfileFile = NULL;
      if ((mCmdLineParams.mCompressedProfiles) && (textureRule.mCompressedProfileFileIndex != -1))
         pProfileFile = &mTextureProfileFiles[textureRule.mCompressedProfileFileIndex];
      else if (textureRule.mNormalProfileFileIndex != -1)
         pProfileFile = &mTextureProfileFiles[textureRule.mNormalProfileFileIndex];
      else
      {
         ddxConvError("Can't find a profile file for texture rule %s\n", textureRule.mName.getPtr());
         return false;
      }

      ddxConvPrintf("Using rule \"%s\", base profile file \"%s\"\n", textureRule.mName.getPtr(), pProfileFile->getName().getPtr());
      ddxConvPrintf("  MaxMapSizeOverride: %i, MaxMapSizeMultiplier: %i, MipLevelsOveride: %i\n", textureRule.mMaxMapSizeOverride, textureRule.mMaxMapSizeMultiplier, textureRule.mMipLevelsOverride);

      int profileEntryIndex = pProfileFile->findMatch(srcFilename, srcFilenameSuffix, textureInfo);
      if (profileEntryIndex < 0)
      {
         ddxConvError("Couldn't find a usable base texture profile for file \"%s\" within profile file \"%s\"\n", pSrcFilename, pProfileFile->getName().getPtr());
         return false;      
      }
      
      textureProfile = pProfileFile->getProfile(profileEntryIndex);
         
      ddxConvPrintf("Using base texture profile \"%s\":\n", textureProfile.mName.getPtr());
            
      if ((mCmdLineParams.mCompressedProfiles) && (!textureRule.mDisableCompressedFormats) && (textureProfile.mSpecifiedCompressedFormat))
         textureProfile.mFormat = textureProfile.mCompressedFormat;
                           
      if (hasOverrideProfiles)
      {
         int overrideProfileIndex = overrideProfiles.findMatch(srcFilename, srcFilenameSuffix, textureInfo);
         if (overrideProfileIndex >= 0)
         {
            const BTextureProfile& overrideProfile = overrideProfiles.getProfile(overrideProfileIndex);
            
            ddxConvPrintf("Applying override texture profile \"%s\"\n", overrideProfile.mName.getPtr());
            
            if ((mCmdLineParams.mCompressedProfiles) && (!textureRule.mDisableCompressedFormats) && (overrideProfile.mSpecifiedCompressedFormat)) 
               textureProfile.mFormat = overrideProfile.mCompressedFormat;
            else if (overrideProfile.mSpecifiedNonCompressedFormat)
               textureProfile.mFormat = overrideProfile.mNonCompressedFormat;
            
            if (overrideProfile.mMaxMapSize != -1)
               textureProfile.mMaxMapSize = overrideProfile.mMaxMapSize;
            if (overrideProfile.mMipLevels != -1)
               textureProfile.mMipLevels = overrideProfile.mMipLevels;
         }
         else
         {
            ddxConvPrintf("Unable to find matching override texture profile\n");
         }
      }
      
      if (textureRule.mMaxMapSizeOverride != -1)
         textureProfile.mMaxMapSize = textureRule.mMaxMapSizeOverride;
      else if (textureRule.mMaxMapSizeMultiplier != 1)
         textureProfile.mMaxMapSize *= textureRule.mMaxMapSizeMultiplier;

      if (textureRule.mMipLevelsOverride != -1)
         textureProfile.mMipLevels = textureRule.mMipLevelsOverride;

      if (!Math::IsPow2(textureProfile.mMaxMapSize))
         textureProfile.mMaxMapSize = Math::NextPowerOf2(textureProfile.mMaxMapSize);
      
      ddxConvPrintf("Using Format: %s, MaxMapSize: %i, MipLevels: %i\n", getDDXDataFormatString(textureProfile.mFormat), textureProfile.mMaxMapSize, textureProfile.mMipLevels);
      
      foundTextureProfile = true;
            
      return true;
   }  
   
   bool printStats(const BDDXTextureInfo& textureInfo, const BByteArray* pTextureData, const char* pDstFilename, uint64 dstFileSize)
   {
      BByteArray dstTextureData;
      BDDXTextureInfo dstTextureInfo;

      if (!loadDDXFile(pDstFilename, dstTextureData, dstTextureInfo))
         return false;

      BByteArray decompTextureData;
      BDDXTextureInfo decompTextureInfo;

      IDDXBuffer* pDataHandle = NULL;

      if (!mDDXDLLHelper.getInterface()->unpackTexture(&pDataHandle, &dstTextureData[0], dstTextureData.size(), dstTextureInfo, decompTextureInfo, true, false))
         return false;

      const uint dataSize = pDataHandle->getSize();

      decompTextureData.resize(dataSize);
      memcpy(&decompTextureData[0], pDataHandle->getPtr(), dataSize);

      pDataHandle->release();

      if (decompTextureInfo.mDataFormat == cDDXDataFormatA8B8G8R8)
      {
         const BRGBAImage decompImage((BRGBAColor*)&decompTextureData[0], decompTextureInfo.mWidth, decompTextureInfo.mHeight);
         const BRGBAImage origImage((BRGBAColor*)pTextureData->getPtr(), textureInfo.mWidth, textureInfo.mHeight);

         BImageUtils::BErrorMetrics overallError;
         BImageUtils::BErrorMetrics lumaError;
         BImageUtils::BErrorMetrics rError;
         BImageUtils::BErrorMetrics gError;
         BImageUtils::BErrorMetrics bError;
         BImageUtils::BErrorMetrics aError;
         BImageUtils::computeErrorMetrics(overallError, origImage, decompImage, 0, 3, false);
         BImageUtils::computeErrorMetrics(lumaError, origImage, decompImage, 0, 3, true);
         BImageUtils::computeErrorMetrics(rError, origImage, decompImage, 0, 1, false);
         BImageUtils::computeErrorMetrics(gError, origImage, decompImage, 1, 1, false);
         BImageUtils::computeErrorMetrics(bError, origImage, decompImage, 2, 1, false);
         BImageUtils::computeErrorMetrics(aError, origImage, decompImage, 3, 1, false);

         ddxConvPrintf("    RGB: Max: %u, Mean: %3.2f, MSE: %3.2f, RMSE: %3.2f, PSNR: %2.2f\n", overallError.mMaxError, overallError.mMeanError, overallError.mMSE, overallError.mRMSE, overallError.mPSNR);
         ddxConvPrintf("   Luma: Max: %u, Mean: %3.2f, MSE: %3.2f, RMSE: %3.2f, PSNR: %2.2f\n", lumaError.mMaxError, lumaError.mMeanError, lumaError.mMSE, lumaError.mRMSE, lumaError.mPSNR);
         ddxConvPrintf("      R: Max: %u, Mean: %3.2f, MSE: %3.2f, RMSE: %3.2f, PSNR: %2.2f\n", rError.mMaxError, rError.mMeanError, rError.mMSE, rError.mRMSE, rError.mPSNR);
         ddxConvPrintf("      G: Max: %u, Mean: %3.2f, MSE: %3.2f, RMSE: %3.2f, PSNR: %2.2f\n", gError.mMaxError, gError.mMeanError, gError.mMSE, gError.mRMSE, gError.mPSNR);
         ddxConvPrintf("      B: Max: %u, Mean: %3.2f, MSE: %3.2f, RMSE: %3.2f, PSNR: %2.2f\n", bError.mMaxError, bError.mMeanError, bError.mMSE, bError.mRMSE, bError.mPSNR);
         ddxConvPrintf("      A: Max: %u, Mean: %3.2f, MSE: %3.2f, RMSE: %3.2f, PSNR: %2.2f\n", aError.mMaxError, aError.mMeanError, aError.mMSE, aError.mRMSE, aError.mPSNR);

         if (dstFileSize)
            ddxConvPrintf("Luma PSNR per byte: %f\n", lumaError.mPSNR / dstFileSize);
      }
      
      return true;               
   }       
         
   bool convertFile(const char* pSrcFilename, const eFileType srcFileType, const char* pDstFilename, const eFileType dstFileType, bool& fileSkipped)
   {
      fileSkipped = false;
                           
      BByteArray nativeTextureData;
      BDDXTextureInfo sourceTextureInfo;
      BDDXTextureInfo nativeTextureInfo;
      BTextureMetadata::BTexType texType;
      
      bool D3DXGenerateMips = false;
      if (dstFileType != cFileTypeDDX)
         D3DXGenerateMips = (mProtoPackParams.mPackerFlags & BDDXPackParams::cGenerateMips) != 0;
      
      if (!loadImageFile(pSrcFilename, srcFileType, nativeTextureData, nativeTextureInfo, texType, D3DXGenerateMips))
      {
         ddxConvError("Unable to read file %s\n", pSrcFilename);
         return false;
      }
      
      sourceTextureInfo = nativeTextureInfo;
      
      BTextureMetadata::BMetadata textureMetadata;
                              
      if (!mCmdLineParams.mIgnoreTextureMetadata) 
      {
         textureMetadata.read(pSrcFilename);

         printTextureMetadata(textureMetadata);
      }                  
      
      ddxConvPrintf("\n");
      ddxConvPrintf("Texture filename suffix metadata type: %s\n", BTextureMetadata::gpTexTypeDescriptions[texType]);
                                                            
      if ( (dstFileType != cFileTypeDDX) && (!getDDXDataFormatIsFixedSize(nativeTextureInfo.mDataFormat)) )
      {
         BByteArray DXTTextureData;
         BDDXTextureInfo DXTTextureInfo;
         IDDXBuffer* pDataHandle = NULL;

         if (!mDDXDLLHelper.getInterface()->unpackTexture(&pDataHandle, &nativeTextureData[0], nativeTextureData.size(), nativeTextureInfo, DXTTextureInfo, true, true))
            return false;

         const uint dataSize = pDataHandle->getSize();

         DXTTextureData.resize(dataSize);
         memcpy(&DXTTextureData[0], pDataHandle->getPtr(), dataSize);

         pDataHandle->release();
         
         DXTTextureData.swap(nativeTextureData);
         nativeTextureInfo = DXTTextureInfo;
      }
      
      if ( (mCmdLineParams.mPadToPowerOfTwo) && (!Math::IsPow2(nativeTextureInfo.mWidth) || !Math::IsPow2(nativeTextureInfo.mHeight)) )
      {
         if ( (nativeTextureInfo.mDataFormat == cDDXDataFormatA8B8G8R8) || (nativeTextureInfo.mDataFormat == cDDXDataFormatA8R8G8B8) || (nativeTextureInfo.mDataFormat == cDDXDataFormatA16B16G16R16F) )
         {
            ddxConvWarning("Image dimensions are not a power of 2 -- resizing by padding!\n");

            const uint newWidth = Math::NextPowerOf2(nativeTextureInfo.mWidth);
            const uint newHeight = Math::NextPowerOf2(nativeTextureInfo.mHeight);

            BByteArray newTextureData((newWidth * newHeight * getDDXDataFormatBitsPerPixel(nativeTextureInfo.mDataFormat)) / 8);

            if (nativeTextureInfo.mDataFormat == cDDXDataFormatA16B16G16R16F)
            {
               BRGBA16Image srcImage((BRGBAColor16*)nativeTextureData.getPtr(), nativeTextureInfo.mWidth, nativeTextureInfo.mHeight);
               BRGBA16Image dstImage((BRGBAColor16*)newTextureData.getPtr(), newWidth, newHeight);
               for (uint y = 0; y < nativeTextureInfo.mHeight; y++)
                  for (uint x = 0; x < nativeTextureInfo.mWidth; x++)
                     dstImage(x, y) = srcImage(x, y);
            }
            else
            {
               BRGBAImage srcImage((BRGBAColor*)nativeTextureData.getPtr(), nativeTextureInfo.mWidth, nativeTextureInfo.mHeight);
               BRGBAImage dstImage((BRGBAColor*)newTextureData.getPtr(), newWidth, newHeight);
               for (uint y = 0; y < nativeTextureInfo.mHeight; y++)
                  for (uint x = 0; x < nativeTextureInfo.mWidth; x++)
                     dstImage(x, y) = srcImage(x, y);
            }

            nativeTextureData.swap(newTextureData);
            nativeTextureInfo.mWidth = newWidth;
            nativeTextureInfo.mHeight = newHeight;
            nativeTextureInfo.mNumMipChainLevels = 0;
         }
      }
            
      BByteArray textureData;
      BDDXTextureInfo textureInfo;
      IDDXBuffer* pDataHandle = NULL;
      
      if (!mDDXDLLHelper.getInterface()->unpackTexture(&pDataHandle, &nativeTextureData[0], nativeTextureData.size(), nativeTextureInfo, textureInfo, true, false))
         return false;
         
      if ((textureInfo.mDataFormat != cDDXDataFormatA8B8G8R8) && (textureInfo.mDataFormat != cDDXDataFormatA16B16G16R16F))
         return false;
            
      textureData.resize(pDataHandle->getSize());
      memcpy(&textureData[0], pDataHandle->getPtr(), pDataHandle->getSize());
      pDataHandle->release();

      //---------------------------      
      
      if ((mCmdLineParams.mAutoDetermineNormalMaps) && (textureInfo.mResourceType == cDDXResourceTypeRegularMap))
      {
         if (isValidNormalMap(pSrcFilename, textureData, textureInfo, srcFileType, texType, textureMetadata))
            textureInfo.mResourceType = cDDXResourceTypeNormalMap;
      }
      
      //---------------------------
      
      BDDXTextureInfo tempTextureInfo(textureInfo);
      if ((textureMetadata.mConvertToHDR) && (textureInfo.mDataFormat == cDDXDataFormatA8B8G8R8))
      {
         tempTextureInfo.mHasAlpha = false;
         tempTextureInfo.mDataFormat = cDDXDataFormatA16B16G16R16F;
      }
      
      BTextureProfile profile;
      bool foundProfile = false;
      if (!findTextureProfile(profile, foundProfile, pSrcFilename, tempTextureInfo))
         return false;
         
      //---------------------------
      
      bool downsampledTexture = false;
      const bool normalMap = (textureInfo.mResourceType == cDDXResourceTypeNormalMap);
      uint maxMapSize = 0;
      if ((foundProfile) && (profile.mMaxMapSize > 0))
         maxMapSize = profile.mMaxMapSize;
      else if ((normalMap) && (mCmdLineParams.mMaxNormalMapSize > 0))
         maxMapSize = mCmdLineParams.mMaxNormalMapSize;
      else
         maxMapSize = mCmdLineParams.mMaxMapSize;
      
      if ((maxMapSize > 0) && (Math::Max(textureInfo.mWidth, textureInfo.mHeight) > maxMapSize))
      {
         if (downsampleTexture(textureInfo, textureData, textureMetadata, maxMapSize))
         {
            ddxConvPrintf("Texture has been downsampled to the maximum permitted size for this texture type (%ux%u).\n", maxMapSize, maxMapSize);
            downsampledTexture = true;
         }
      }   
      
      //---------------------------
      
      if ((textureMetadata.mConvertToHDR) && (textureInfo.mDataFormat == cDDXDataFormatA8B8G8R8))
      {
         ddxConvPrintf("Converting LDR image to HDR using intensity scale of %f\n", textureMetadata.mConvertToHDRScale);
         if (!textureInfo.mHasAlpha)
            ddxConvWarning("Source texture should have an alpha channel when converting to HDR!\nUsing an ALL-WHITE alpha channel!\n");
         
         BDDXSerializedTexture srcTexture(textureData.getPtr(), textureData.getSize(), textureInfo);
         
         BByteArray HDRTextureData;

         for (uint faceIndex = 0; faceIndex < srcTexture.getNumFaces(); faceIndex++)
         {
            for (uint mipLevel = 0; mipLevel < textureInfo.mNumMipChainLevels + 1; mipLevel++)
            {
               const uint mipWidth = srcTexture.getWidth(mipLevel);
               const uint mipHeight = srcTexture.getHeight(mipLevel);

               BRGBAImage LDRImage((BRGBAColor*)srcTexture.getSurfaceData(faceIndex, mipLevel), mipWidth, mipHeight);

               BRGBAFImage HDRImage;                  
               convertLDRToHDRImage(HDRImage, LDRImage, textureMetadata.mConvertToHDRScale);
               
               BRGBA16Image halfImage;
               BHDRUtils::packFloatToHalfImage(HDRImage, halfImage, 3);
               
               HDRTextureData.pushBack(reinterpret_cast<const BYTE*>(halfImage.getPtr()), mipWidth * mipHeight * sizeof(BRGBAColor16));
            }
         }               

         textureData.swap(HDRTextureData);
                  
         textureInfo.mHasAlpha = false;
         textureInfo.mDataFormat = cDDXDataFormatA16B16G16R16F;
      }

      const bool HDRImage = (textureInfo.mDataFormat != cDDXDataFormatA8B8G8R8);
            
      ddxConvPrintf("\n");
      ddxConvPrintf("Source texture info:\n");
      printTextureInfo(nativeTextureInfo);

      // Now create a set of pack params for this texture.
      BDDXPackParams localPackParams(mProtoPackParams);   
            
      if ((mCmdLineParams.mAutoDXTQQuality) && (!mCmdLineParams.mSpecifiedDXTQQuality))
      {
         uint dxtqQualityFactor = BDDXDXTQPackParams::cDefaultQualityFactor; 
         
         switch (texType)
         {
            case BTextureMetadata::cTexTypeDiffuse:            dxtqQualityFactor += 0; break;
            case BTextureMetadata::cTexTypeNormal:             dxtqQualityFactor += 0; break;
            case BTextureMetadata::cTexTypeGloss:              dxtqQualityFactor -= 1; break;
            case BTextureMetadata::cTexTypeSpecular:           dxtqQualityFactor -= 1; break;
            case BTextureMetadata::cTexTypeOpacity:            dxtqQualityFactor -= 1; break;
            case BTextureMetadata::cTexTypePixelXForm:         dxtqQualityFactor -= 2; break;
            case BTextureMetadata::cTexTypeEmmissive:          dxtqQualityFactor += 0; break;
            case BTextureMetadata::cTexTypeAmbientOcclusion:   dxtqQualityFactor -= 1; break;
            case BTextureMetadata::cTexTypeEnvironment:        dxtqQualityFactor -= 1; break;
            case BTextureMetadata::cTexTypeEnvironmentMask:    dxtqQualityFactor -= 1; break;
            case BTextureMetadata::cTexTypeUI:                 dxtqQualityFactor += 1; break;
            case BTextureMetadata::cTexTypeEmissivePixelXForm: dxtqQualityFactor += 0; break;
            case BTextureMetadata::cTexTypeEffect:             dxtqQualityFactor -= 1; break;
            case BTextureMetadata::cTexTypeDistortion:         dxtqQualityFactor += 0; break;
         }

         localPackParams.mDXTQParams.mQualityFactor = dxtqQualityFactor;
      }  
            
      // Set the resource type.
      localPackParams.mResourceType = textureInfo.mResourceType;
            
      if (foundProfile)
      {
         if (HDRImage)
         {
            if (!getDDXDataFormatIsHDR(profile.mFormat))
            {
               ddxConvError("Texture is HDR, but the texture profile expects an LDR texture. Please use an LDR image for this profile.\n");
               return false;
            }
         }
         else
         {
            if (getDDXDataFormatIsHDR(profile.mFormat))
            {
               ddxConvError("Texture is LDR, but the texture profile expects an HDR texture. Please use an HDR image for this profile.\n");
               return false;
            }
         } 
         
         if (profile.mResourceType == "cube")
         {
            if (textureInfo.mResourceType != cDDXResourceTypeCubeMap)
            {
               ddxConvError("Texture profile requires this texture to be a cubemap.\n");
               return false;
            }
         }
         else if (profile.mResourceType == "2D")
         {
            if (textureInfo.mResourceType != cDDXResourceTypeRegularMap)
            {
               ddxConvError("Texture profile requires this texture to be a 2D texture.\n");
               return false;
            }
         }
         else if (profile.mResourceType == "normal")
         {
            if (textureInfo.mResourceType != cDDXResourceTypeNormalMap)
            {
               ddxConvError("Texture profile requires this texture to be a normal map.\n");
               return false;
            }
         }
      }
            
      uint64 srcFileSize;
      if (!getFileSize(pSrcFilename, srcFileSize))
         return false;      
         
      mFileStats[localPackParams.mResourceType].update(Math::Max(nativeTextureInfo.mWidth, nativeTextureInfo.mHeight), nativeTextureInfo.mOrigDataFormat, srcFileSize);
      mOverallFileStats.update(Math::Max(nativeTextureInfo.mWidth, nativeTextureInfo.mHeight), nativeTextureInfo.mOrigDataFormat, srcFileSize);

      // Set the number of mip levels to pack
      uint maxMipChainLevels = 0;

      int dimension = Math::Max(textureInfo.mWidth, textureInfo.mHeight);
      while (dimension > mCmdLineParams.mMinMipDim)
      {
         maxMipChainLevels++;
         dimension >>= 1;
      }
                  
      localPackParams.mMipChainLevels = textureInfo.mNumMipChainLevels;

      if (localPackParams.mPackerFlags & BDDXPackParams::cGenerateMips)
      {
         if ((foundProfile) && (profile.mMipLevels >= 0))
            localPackParams.mMipChainLevels = profile.mMipLevels;
         else if (mCmdLineParams.mSpecifiedMipLevels)
            localPackParams.mMipChainLevels = mCmdLineParams.mMipLevels;
         else
            localPackParams.mMipChainLevels = maxMipChainLevels;      
      }
                  
      if (textureMetadata.mValid)
      {
         if (!textureMetadata.mMipmaps)
         {
            localPackParams.mMipChainLevels = 0;
         }
         
         if (textureMetadata.mTilable)
            localPackParams.mPackerFlags |= BDDXPackParams::cUseWrapFiltering;
         else
            localPackParams.mPackerFlags &= ~BDDXPackParams::cUseWrapFiltering;
      }

      localPackParams.mMipChainLevels = Math::Min<uint>(maxMipChainLevels, localPackParams.mMipChainLevels);

      bool useNativeData = mCmdLineParams.mUseNativeData;
      
      if ((mCmdLineParams.mUseNativeDataForNormalMaps) && (textureInfo.mResourceType == cDDXResourceTypeNormalMap))
         useNativeData = true;

      // Can't use native data if we've downsampled the texture.
      if (downsampledTexture)         
         useNativeData = false;
         
      if ((mCmdLineParams.mForceCompress32BitTextures) && (useNativeData))
      {
         if (getDDXDataFormatBitsPerPixel(nativeTextureInfo.mDataFormat) == 32)
         {
            useNativeData = false;
            ddxConvWarning("File is 32-bit/texel, not using native data.");
         }
      }
            
      // Set the data format.
      if (foundProfile)
      {
         if (getDDXDataFormatHasAlpha(profile.mFormat))
         {
            if (!textureInfo.mHasAlpha)
            {
               ddxConvWarning("Texture profile expects a texture with an alpha channel, but source texture doesn't have alpha!\n");
            }
         }
         else
         {
            if (textureInfo.mHasAlpha)
            {
               ddxConvWarning("Texture profile expects a texture without an alpha channel, but source texture has alpha!\nAlpha channel will be ignored.");
            }
         }
         
         localPackParams.mDataFormat = profile.mFormat;
      }      
      else
      {
         if ((mCmdLineParams.mUseOrigFormat) || (useNativeData))
         {
            localPackParams.mDataFormat = nativeTextureInfo.mOrigDataFormat;
         }
         else if (textureInfo.mResourceType == cDDXResourceTypeNormalMap)
         {
            if (textureInfo.mHasAlpha)
               localPackParams.mDataFormat = mNormalMapAlphaFormat;
            else
               localPackParams.mDataFormat = mNormalMapFormat;
         }
         else if (HDRImage)
         {
            if (textureInfo.mHasAlpha)
               localPackParams.mDataFormat = mHDRAlphaFormat;
            else
               localPackParams.mDataFormat = mHDRFormat;
         }
         else if ((textureMetadata.mValid) && (!textureMetadata.mAlpha))
         {
            localPackParams.mDataFormat = mRegularFormat;
         }
         else if (textureInfo.mHasAlpha)
         {
            localPackParams.mDataFormat = mAlphaFormat;
         }
         else
         {
            localPackParams.mDataFormat = mRegularFormat;
         }
      }         
               
      if ((textureInfo.mResourceType == cDDXResourceTypeNormalMap) || 
          (localPackParams.mDataFormat == cDDXDataFormatDXT5N) || 
          (localPackParams.mDataFormat == cDDXDataFormatDXN) ||
          (localPackParams.mDataFormat == cDDXDataFormatDXNQ))
      {
         localPackParams.mPackerFlags &= ~BDDXPackParams::cPerceptual;
         localPackParams.mPackerFlags &= ~BDDXPackParams::cDXTDithering;     
         //localPackParams.mPackerFlags |= BDDXPackParams::cRenormalize; 
      }
      
      ddxConvPrintf("\n");
                  
      BByteArray* pTextureData = &textureData;
      BByteArray LDRTextureData;
      
      if (HDRImage)
      {
         bool fileFormatSupportsHDR = true;
         
         switch (dstFileType)
         {
            case cFileTypeDDX:
            case cFileTypeDDS:
            case cFileTypeHDR:
            {
               break;
            }
            default:
            {
               fileFormatSupportsHDR = false;
               break;
            }
         }

         if ((!fileFormatSupportsHDR) || (!getDDXDataFormatIsHDR(localPackParams.mDataFormat)))
         {
            ddxConvWarning("Input is HDR, but output isn't. Converting image to LDR!\n");
                        
            float gamma = 2.2f;
            float exposure = 0.0f;
            
            BDDXSerializedTexture srcTexture(textureData.getPtr(), textureData.getSize(), textureInfo);
            
            for (uint faceIndex = 0; faceIndex < srcTexture.getNumFaces(); faceIndex++)
            {
               for (uint mipLevel = 0; mipLevel < textureInfo.mNumMipChainLevels + 1; mipLevel++)
               {
                  const uint mipWidth = srcTexture.getWidth(mipLevel);
                  const uint mipHeight = srcTexture.getHeight(mipLevel);
                  
                  BRGBA16Image halfImage((BRGBAColor16*)srcTexture.getSurfaceData(faceIndex, mipLevel), mipWidth, mipHeight);
               
                  BRGBAImage LDRImage;                  
                  BHDRUtils::convertHalfFloatToRGB8Image(halfImage, LDRImage, gamma, exposure);

                  LDRTextureData.pushBack(reinterpret_cast<const BYTE*>(LDRImage.getPtr()), mipWidth * mipHeight * sizeof(BRGBAColor));
               }
            }               

            pTextureData = &LDRTextureData;                  
                                    
            textureInfo.mDataFormat = cDDXDataFormatA8B8G8R8;
         }
      }
      
      if ((dstFileType == cFileTypeDDX) && getDDXDataFormatIsDXTQ(localPackParams.mDataFormat))
      {
         ddxConvPrintf("Using DXTQ quality factor %u\n", localPackParams.mDXTQParams.mQualityFactor);       
         
         if (localPackParams.mResourceType == cDDXResourceTypeCubeMap)
         {
            ddxConvWarning("DXTQ format doesn't currently support cubemaps, using regular DXT!\n");

            localPackParams.mDataFormat = getDDXDXTQBaseFormat(localPackParams.mDataFormat);
         }
      }         
      
      if ((mCmdLineParams.mDeltaMode) && (dstFileType == cFileTypeDDX))
      {
         bool foundExistingFile = false;
         
         BDDXDesc ddxDesc;
         BDDXIDChunkData idChunk;
                  
         BByteArray existingDDXData;
         if (readFileData(pDstFilename, existingDDXData))
         {
            if (mDDXDLLHelper.getInterface()->getDesc(existingDDXData.getPtr(), existingDDXData.getSize(), ddxDesc))
            {
               BECFFileReader ecfReader(BConstDataBuffer(existingDDXData.getPtr(), existingDDXData.getSize()));

               uint chunkLen = 0;
               const BDDXIDChunkData* pChunk = (const BDDXIDChunkData*)ecfReader.getChunkDataByID(DDX_ID_CHUNK, chunkLen);
               if ((pChunk) && (chunkLen == sizeof(BDDXIDChunkData)))
               {
                  idChunk = *pChunk;
                  
                  foundExistingFile = true;
               }               
               else
                  ddxConvWarning("Found existing DDX file, but couldn't locate ID chunk.\n");
            }
            else
               ddxConvWarning("Found existing DDX file, but couldn't get description of file.\n");
         }   
                  
         if (!foundExistingFile)
            ddxConvWarning("Unable to load existing DDX file: %s\n", pDstFilename);
         else
         {
            ddxConvWarning("Found existing DDX file.\n");
            
            bool shouldProcessFile = false;
            
            if (idChunk.mSourceFileCRC != textureMetadata.mFileCRC)
            {
               shouldProcessFile = true;
               ddxConvWarning("Source file's CRC differs from the CRC stored in the DDX file.\n");
            }
            else if (idChunk.mSourceFileID != textureMetadata.mID)
            {
               shouldProcessFile = true;
               ddxConvWarning("Source file's ID differs from the ID stored in the DDX file.\n");
            }
            else if ((mCmdLineParams.mSpecifiedUpdateIndex) && (idChunk.mUpdateIndex != mCmdLineParams.mUpdateIndex))
            {
               shouldProcessFile = true;
               ddxConvWarning("DDX file's update index differs from the update index specified on the command line.\n");
            }
            else if (ddxDesc.mDataFormat != localPackParams.mDataFormat)
            {
               shouldProcessFile = true;
               ddxConvWarning("Source file's data format differs from the existing DDX file's data format.\n");
            }
            else if (ddxDesc.mResourceType != localPackParams.mResourceType)
            {
               shouldProcessFile = true;
               ddxConvWarning("Source file's resource type differs from the existing DDX file's resource type.\n");
            }
            else if (ddxDesc.mCreatorVersion  != BDDXHeader::cDDXVersion)
            {
               shouldProcessFile = true;
               ddxConvWarning("Existing DDX file's creator version differs from the current creator version.\n");
            }
            else if ((ddxDesc.mMipChainSize > 0) && (localPackParams.mMipChainLevels == 0))
            {
               shouldProcessFile = true;
               ddxConvWarning("Source file's mipchain levels differs from the existing DDX file.\n");
            }
            else if ((ddxDesc.mMipChainSize == 0) && (localPackParams.mMipChainLevels > 0))
            {
               shouldProcessFile = true;
               ddxConvWarning("Source file's mipchain levels differs from the existing DDX file.\n");
            }
            else 
            {
               const uint width = useNativeData ? nativeTextureInfo.mWidth : textureInfo.mWidth;
               const uint height = useNativeData ? nativeTextureInfo.mHeight : textureInfo.mHeight;
               if ((width != ddxDesc.mWidth) || (height != ddxDesc.mHeight)) 
               {
                  shouldProcessFile = true;
                  ddxConvWarning("Source file's resolution differs from the existing DDX file.\n");
               }
            }
            
            if (!shouldProcessFile)
            {
               ddxConvWarning("Existing DDX file passed all delta update tests.\n");
               fileSkipped = true;
               return false;
            }
            
            ddxConvWarning("Overwriting existing DDX file.\n");
         }
         
         ddxConvPrintf("\n");
      }
               
      if (!readOnlyFileCheck(pDstFilename))
      {
         return false;
      }
                     
      ddxConvPrintf("Writing File %s, %ux%u\n", pDstFilename, textureInfo.mWidth, textureInfo.mHeight); 
      
      if (mCmdLineParams.mSimulateFlag)
      {
         fileSkipped = true;
         return false;
      }

      // Save file
      bool saveStatus = false;

      switch (dstFileType)
      {
         case cFileTypeDDX:
         {
            ddxConvPrintf("Output platform: %s\n", mCmdLineParams.mXboxFlag ? "Xbox" : "PC");
            
            if (useNativeData)
            {
               ddxConvPrintf("Output texture info:\n");
               printTextureInfo(nativeTextureInfo);
               
               ddxConvPrintf("Writing native texture data.\n");
                                             
               saveStatus = saveDDXFile(pSrcFilename, pDstFilename, &nativeTextureData[0], nativeTextureData.size(), nativeTextureInfo, mCmdLineParams.mXboxFlag ? cDDXPlatformXbox : cDDXPlatformNone, textureMetadata);
            }
            else
            {
               BDDXTextureInfo newTextureInfo;
               IDDXBuffer* pDataHandle = NULL;
               
               printPackParams(localPackParams);

               if (!mDDXDLLHelper.getInterface()->packTexture(&pDataHandle, pTextureData->getPtr(), pTextureData->getSize(), textureInfo, localPackParams, newTextureInfo))
                  return false;
               
               ddxConvPrintf("Output texture info:\n");
               printTextureInfo(newTextureInfo);
               
               ddxConvPrintf("Writing processed texture data.\n");
               
               saveStatus = saveDDXFile(pSrcFilename, pDstFilename, pDataHandle->getPtr(), pDataHandle->getSize(), newTextureInfo, mCmdLineParams.mXboxFlag ? cDDXPlatformXbox : cDDXPlatformNone, textureMetadata);
               
               pDataHandle->release();
            }
            
            break;
         }
         case cFileTypeHDR:
         {
            ddxConvPrintf("Output texture info:\n");
            printTextureInfo(textureInfo);

            saveStatus = saveHDRFile(pDstFilename, pTextureData->getPtr(), pTextureData->getSize(), textureInfo, textureInfo.mNumMipChainLevels + 1, textureMetadata);

            break;
         }
         case cFileTypeTGA:
         {
            ddxConvPrintf("Output texture info:\n");
            printTextureInfo(textureInfo);

            saveStatus = saveTGAFile(pDstFilename, pTextureData->getPtr(), pTextureData->getSize(), textureInfo, textureInfo.mNumMipChainLevels + 1, textureMetadata);

            break;
         }
         case cFileTypeDDS:
         {
            bool useNativeDataDDS = false;
            
            if (useNativeData)
            {
               useNativeDataDDS = true;
               
               if (!BD3DXTexture::isSupportedFormat(cFileTypeDDS, nativeTextureInfo.mDataFormat)) 
                  useNativeDataDDS = false;
            }   
            
            BD3DXTexture d3dxTexture(getD3DDevice());
            if (useNativeDataDDS)
            {
               ddxConvPrintf("Output texture info:\n");
               printTextureInfo(nativeTextureInfo);
               
               ddxConvPrintf("Writing native texture data.\n");
               
               saveStatus = d3dxTexture.saveD3DXFile(pDstFilename, cFileTypeDDS, &nativeTextureData[0], nativeTextureData.size(), nativeTextureInfo, nativeTextureInfo.mNumMipChainLevels + 1);
            }
            else
            {
               BDDXTextureInfo newTextureInfo;
               IDDXBuffer* pDataHandle = NULL;

               //if (localPackParams.mDataFormat == cDDXDataFormatDXTM) 
               //   localPackParams.mDataFormat = cDDXDataFormatDXT1;
               //else if (localPackParams.mDataFormat == cDDXDataFormatDXTMA)
               //   localPackParams.mDataFormat = cDDXDataFormatDXT3;
               
               switch (localPackParams.mDataFormat)
               {
                  case cDDXDataFormatDXT1Q: localPackParams.mDataFormat = cDDXDataFormatDXT1; break;
                  case cDDXDataFormatDXT5Q: localPackParams.mDataFormat = cDDXDataFormatDXT5; break;
                  case cDDXDataFormatDXT5HQ: localPackParams.mDataFormat = cDDXDataFormatA16B16G16R16F; break;
                  case cDDXDataFormatDXN: localPackParams.mDataFormat = cDDXDataFormatDXN; break;
               }
                              
               printPackParams(localPackParams);
               
               if (!mDDXDLLHelper.getInterface()->packTexture(&pDataHandle, pTextureData->getPtr(), pTextureData->getSize(), textureInfo, localPackParams, newTextureInfo))
                  return false;

               ddxConvPrintf("Output texture info:\n");
               printTextureInfo(newTextureInfo);
               
               ddxConvPrintf("Writing processed texture data.\n");
               
               saveStatus = d3dxTexture.saveD3DXFile(pDstFilename, cFileTypeDDS, pDataHandle->getPtr(), pDataHandle->getSize(), newTextureInfo, newTextureInfo.mNumMipChainLevels + 1);

               pDataHandle->release();
            }
            
            break;
         }
         case cFileTypeTIF:
         {
            ddxConvError("Can't write .TIF files yet.\n");
            return false;
         }
         default:
         {
            BD3DXTexture d3dxTexture(getD3DDevice());
            
            ddxConvPrintf("Output texture info:\n");
            printTextureInfo(textureInfo);
            
            saveStatus = d3dxTexture.saveD3DXFile(pDstFilename, dstFileType, pTextureData->getPtr(), pTextureData->getSize(), textureInfo, textureInfo.mNumMipChainLevels + 1);
         }
      }
      
      if (!saveStatus)
      {
         ddxConvError("Unable to save file %s\n", pDstFilename);
         return false;
      }
            
      ddxConvPrintf("\n");
      
      uint64 dstFileSize;
      if (getFileSize(pDstFilename, dstFileSize))
      {
         const uint numFaces = textureInfo.mResourceType == cDDXResourceTypeCubeMap ? 6 : 1;
         ddxConvPrintf("Input file size: %I64u, Ave. bits per texel: %2.2f\n", srcFileSize, (srcFileSize * 8.0f) / (sourceTextureInfo.mWidth * sourceTextureInfo.mHeight * numFaces));
         ddxConvPrintf("Output file size: %I64u, Ave. bits per texel: %2.2f\n", dstFileSize, (dstFileSize * 8.0f) / (textureInfo.mWidth * textureInfo.mHeight * numFaces));
      }
         
      if ((mCmdLineParams.mStatsFlag) && (dstFileType == cFileTypeDDX) && (textureInfo.mDataFormat == cDDXDataFormatA8B8G8R8))
      {
         if (!printStats(textureInfo, pTextureData, pDstFilename, dstFileSize))
            return false;
      }
      
      ddxConvPrintf("\n");            
                  
      return true;
   }   
         
   bool loadProfile(const char* pProfileName, const char* pProfilePath, int& profileFileIndex)
   {
      uint i;
      for (i = 0; i < mTextureProfileFiles.getSize(); i++)
      {
         if (mTextureProfileFiles[i].getName() == pProfileName)
         {
            profileFileIndex = i;
            return true;
         }
      }
      
      profileFileIndex = mTextureProfileFiles.getSize();
      
      BTextureProfileFile& textureProfileFile = *mTextureProfileFiles.enlarge(1);
      
      BString filename;
      filename.format("%s%s.xml", pProfilePath, pProfileName);
      
      return textureProfileFile.load(pProfileName, filename);
   }
   
   bool loadRules(void)
   {
      BSimpleXMLReader xmlReader;
      
      BString profilePath;
      strPathGetDirectory(mCmdLineParams.mRulesFilename, profilePath, true);
      
      HRESULT hres = xmlReader.parse(mCmdLineParams.mRulesFilename);
      if (FAILED(hres))
      {
         ddxConvError("Unable to load XML rules file: %s\n", mCmdLineParams.mRulesFilename.getPtr());
         return false;
      }
                  
      mTextureRules.clear();
      
      const BSimpleXMLReader::BNode* pRootNode = xmlReader.getRoot();
            
      for (uint i = 0; i < pRootNode->getNumChildren(); i++)
      {
         const BSimpleXMLReader::BNode* pNode = pRootNode->getChild(i);
         
         BTextureRule& rule = *mTextureRules.enlarge(1);
         
         rule.mName = pNode->getName();
         pNode->getAttributeAsString("pathSubstring", rule.mPathSubstring);
         rule.mPathSubstring.toLower();
         
         BString profile, compressedProfile;
         
         pNode->getChildAsString("profile", profile);
         if (profile.length())
         {
            if (!loadProfile(profile, profilePath, rule.mNormalProfileFileIndex))
               return false;
         }
         
         pNode->getChildAsString("compressedProfile", profile);
         if (compressedProfile.length())
         {
            if (!loadProfile(compressedProfile, profilePath, rule.mCompressedProfileFileIndex))
               return false;
         }
                           
         rule.mMaxMapSizeOverride = -1;
         pNode->getChildAsInt("maxMapSizeOverride", rule.mMaxMapSizeOverride);
         
         rule.mMaxMapSizeMultiplier = 1;
         pNode->getChildAsInt("maxMapSizeMultiplier", rule.mMaxMapSizeMultiplier);
         
         rule.mMipLevelsOverride = -1;
         pNode->getChildAsInt("mipLevelsOverride", rule.mMipLevelsOverride);
         
         rule.mDisableCompressedFormats = false;
         pNode->getChildAsBool("disableCompressedFormats", rule.mDisableCompressedFormats);
      }
      
      ddxConvPrintf("Successfully loaded rules file: %s\n", mCmdLineParams.mRulesFilename.getPtr());
      
      return true;
   }
   
   bool openLogFile(const char* pFilename)
   {
      closeLogFile();
      
      gpLogFile = NULL;
      fopen_s(&gpLogFile, pFilename, "a+");
      if (!gpLogFile)
      {
         ddxConvError("Unable to open log file: %s\n", pFilename);
         return false;
      }
      
      return true;
   }
         
   void closeLogFile(void)
   {
      if (gpLogFile)
         fclose(gpLogFile);
      gpLogFile = NULL;
   }
   
   bool openErrorLogFile(const char* pFilename)
   {
      closeErrorLogFile();

      gpErrorLogFile = NULL;
      fopen_s(&gpErrorLogFile, pFilename, "a+");
      if (!gpErrorLogFile)
      {
         ddxConvError("Unable to open error log file: %s\n", pFilename);
         return false;
      }

      return true;
   }
   
   void closeErrorLogFile(void)
   {
      if (gpErrorLogFile)
         fclose(gpErrorLogFile);
      gpErrorLogFile = NULL;
   }   
   
   bool processParams(int argc, const char * const argv[])
   {
      if (argc < 2)
      {
         mCmdLineParams.printHelp();
         return false;
      }

      if (!mCmdLineParams.parse(argc, argv))
         return false;
                                   
      gQuietFlag = mCmdLineParams.mQuietFlag;
      gErrorMessageBox = mCmdLineParams.mErrorMessageBox;

      closeLogFile();
      closeErrorLogFile();
      
      if (!mCmdLineParams.mLogfile.isEmpty())
      {  
         if (!openLogFile(mCmdLineParams.mLogfile.getPtr()))
         {
            if (!mCmdLineParams.mIgnoreErrors)
               return false;
         }
      }  
      
      if (!mCmdLineParams.mErrorLogfile.isEmpty())
      {  
         if (!openErrorLogFile(mCmdLineParams.mErrorLogfile.getPtr()))
         {
            if (!mCmdLineParams.mIgnoreErrors)
               return false;
         }
      }  
      
      mUpdateIndex = mCmdLineParams.mUpdateIndex;
            
      if (mCmdLineParams.mSmartFlag)
      {
         mCmdLineParams.mAutoDetermineNormalMaps = true;
                  
         char moduleFilename[MAX_PATH];
         GetModuleFileName(NULL, moduleFilename, MAX_PATH);
         BString rulesFilename(moduleFilename);
         strPathGetDirectory(rulesFilename, rulesFilename, true);
         rulesFilename += "rules.xml";
         
         mCmdLineParams.mRulesFilename = rulesFilename;
         mCmdLineParams.mAutoDXTQQuality = true;
      }
      
      if (mCmdLineParams.mBestFlag)
         mProtoPackParams.mPackerFlags |= BDDXPackParams::cDXTBest;
      else if (mCmdLineParams.mQuickFlag)
         mProtoPackParams.mPackerFlags |= BDDXPackParams::cDXTFast;

      if (mCmdLineParams.mNoAutoMipFlag)
         mProtoPackParams.mPackerFlags &= ~BDDXPackParams::cGenerateMips;
        
      if (mCmdLineParams.mSpecifiedDXTQQuality)
         mProtoPackParams.mDXTQParams.mQualityFactor = mCmdLineParams.mDXTQQuality;

#if 0
      mProtoPackParams.mDXTMParams.mDXTMCodebookSize = Math::Clamp<int>(mCmdLineParams.mDXTMCodebook, 0, BDDXDXTMPackParams::cDXTMMaxCodebookSize);
      mProtoPackParams.mDXTMMipParams.mDXTMCodebookSize = Math::Clamp<int>(mCmdLineParams.mDXTMMipCodebook, 0, BDDXDXTMPackParams::cDXTMMaxCodebookSize);
      if (!mCmdLineParams.mSpecifiedMipCodebook)
         mProtoPackParams.mDXTMMipParams.mDXTMCodebookSize = mProtoPackParams.mDXTMParams.mDXTMCodebookSize;

      mProtoPackParams.mDXTMParams.mDXTMQuality = Math::Clamp<int>(mCmdLineParams.mDXTMQuality, BDDXDXTMPackParams::cDXTMMinQuality, BDDXDXTMPackParams::cDXTMMaxQuality);
      mProtoPackParams.mDXTMMipParams.mDXTMQuality = Math::Clamp<int>(mCmdLineParams.mDXTMMipQuality, BDDXDXTMPackParams::cDXTMMinQuality, BDDXDXTMPackParams::cDXTMMaxQuality);
      if (!mCmdLineParams.mSpecifiedMipQuality)
         mProtoPackParams.mDXTMMipParams.mDXTMQuality = mProtoPackParams.mDXTMParams.mDXTMQuality;

      mProtoPackParams.mDXTMParams.mDXTMCompressionMethod = Math::Clamp<int>(mCmdLineParams.mDXTMMethod, 0, BDDXDXTMPackParams::cDXTMNumCompressionMethods - 1);      
      mProtoPackParams.mDXTMMipParams.mDXTMCompressionMethod = Math::Clamp<int>(mCmdLineParams.mDXTMMipMethod, 0, BDDXDXTMPackParams::cDXTMNumCompressionMethods - 1);      
      if (!mCmdLineParams.mSpecifiedMipMethod)
         mProtoPackParams.mDXTMMipParams.mDXTMCompressionMethod = mProtoPackParams.mDXTMParams.mDXTMCompressionMethod;
#endif         

      if (mCmdLineParams.mNormalMapFlag)   
         mProtoPackParams.mResourceType = cDDXResourceTypeNormalMap;

      if (mCmdLineParams.mDitherFlag)
         mProtoPackParams.mPackerFlags |= BDDXPackParams::cDXTDithering;

      if (mCmdLineParams.mWrapFlag)
         mProtoPackParams.mPackerFlags |= BDDXPackParams::cUseWrapFiltering;

      if (mCmdLineParams.mNoPerceptualFlag)
         mProtoPackParams.mPackerFlags &= ~BDDXPackParams::cPerceptual;
      
      if (!getTextureFormat(mCmdLineParams.mRegularFormat.getPtr(), mRegularFormat)) return false;
      if (!getTextureFormat(mCmdLineParams.mAlphaFormat.getPtr(), mAlphaFormat)) return false;
      if (!getTextureFormat(mCmdLineParams.mNormalMapFormat.getPtr(), mNormalMapFormat)) return false;
      if (!getTextureFormat(mCmdLineParams.mNormalMapAlphaFormat.getPtr(), mNormalMapAlphaFormat)) return false;
      if (!getTextureFormat(mCmdLineParams.mHDRFormat.getPtr(), mHDRFormat)) return false;
      if (!getTextureFormat(mCmdLineParams.mHDRAlphaFormat.getPtr(), mHDRAlphaFormat)) return false;
         
      if (!mCmdLineParams.mSpecifiedAlphaFormat)
         mAlphaFormat = mRegularFormat;
         
      if (!mCmdLineParams.mSpecifiedNormalMapFormat)
         mNormalMapFormat = mRegularFormat;
         
      if (!mCmdLineParams.mSpecifiedNormalMapAlphaFormat)
      {
         if (!mCmdLineParams.mSpecifiedNormalMapFormat)
            mNormalMapAlphaFormat = mAlphaFormat;
         else
            mNormalMapAlphaFormat = mNormalMapFormat;
      }
         
      if (!mCmdLineParams.mSpecifiedHDRAlphaFormat)
         mHDRAlphaFormat = mHDRFormat;

      mOutputFileType = BFileType::determineFromExtension(mCmdLineParams.mOutputTypeString.getPtr());
      if (cFileTypeInvalid == mOutputFileType)
      {
         ddxConvError("Invalid file type: %s\n", mCmdLineParams.mOutputTypeString.getPtr());
         return false;
      }
      
      if (mOutputFileType == cFileTypeDDT)
      {
         ddxConvError("Can't write to DDT files yet!\n");
         return false;
      }
      
      if (mCmdLineParams.mRulesFilename.length())
      {
         if (!loadRules())
            return false;  
      }
                                                                     
      return true;
   }
          
   bool processFiles(void)
   {
      if (mCmdLineParams.mHDRConvertMode)
         mOutputFileType = cFileTypeHDR;
         
      if (!mSourceFiles.size())
      {
         ddxConvWarning("No files found to process.\n");
         return false;
      }
      
      if ((!mCmdLineParams.mOutputFilenameString.isEmpty()) && (mSourceFiles.size() > 1))
      {
         ddxConvWarning("More than one file found -- ignoring output filename %s\n\n", mCmdLineParams.mOutputFilenameString.getPtr());
      }
      
      uint startFileIndex = 0;
      uint endFileIndex = mSourceFiles.getSize();
      if ((mCmdLineParams.mNumFileGroups) && (mSourceFiles.getSize()))
      {
         const uint numFileGroups = Math::Clamp<uint>(mCmdLineParams.mNumFileGroups, 1, mSourceFiles.getSize());
         const uint numFilesPerGroup = Math::Clamp<uint>(mSourceFiles.getSize() / numFileGroups, 1, mSourceFiles.getSize());
         
         if (mCmdLineParams.mFileGroupIndex >= numFileGroups)
         {
            ddxConvError("Invalid file group index\n");
            return false;
         }
         
         for (uint groupIndex = 0; groupIndex < numFileGroups; groupIndex++)
         {      
            endFileIndex = startFileIndex + numFilesPerGroup;
            if (groupIndex == (numFileGroups - 1))
               endFileIndex = mSourceFiles.getSize();
            
            if (groupIndex == mCmdLineParams.mFileGroupIndex)
               break;
            
            startFileIndex += numFilesPerGroup;
         }
      }  
      
      ddxConvPrintf("Processing %u files (%u to %u)\n\n", endFileIndex - startFileIndex, startFileIndex + 1, (endFileIndex - 1) + 1);
            
      for (uint fileIndex = startFileIndex; fileIndex < endFileIndex; fileIndex++)
      {
         const BFilePath& srcFilePath = mSourceFiles[fileIndex];
         BString srcRelPathname(srcFilePath.relPathname());
         BString srcFullFilename(srcFilePath.fullFilename());
         
         const eFileType srcFileType = BFileType::determineFromFilename(srcFullFilename.getPtr());
         if (srcFileType == cFileTypeInvalid)
         {
            mNumNonImageFiles++;
            continue;
         }
                              
         BString srcPath;
         BString srcFilename;
         strPathSplit(srcFullFilename, srcPath, srcFilename);
         
         char srcFName[_MAX_FNAME];
         char srcExt[_MAX_EXT];
         _splitpath_s(srcFilename.getPtr(), NULL, 0, NULL, 0, srcFName, sizeof(srcFName), srcExt, sizeof(srcExt));
               
         BString dstFullFilename;
         
         if ((!mCmdLineParams.mOutputFilenameString.isEmpty()) && (mSourceFiles.size() == 1))
         {
            dstFullFilename = mCmdLineParams.mOutputFilenameString;
            
            mOutputFileType = BFileType::determineFromFilename(dstFullFilename.getPtr());
            if (cFileTypeInvalid == mOutputFileType)
            {
               ddxConvError("Invalid output file type: %s\n", dstFullFilename.getPtr());
               return false;
            }
         }
         else
         {
            dstFullFilename.set((mCmdLineParams.mOutSameDirFlag ? srcPath : mCmdLineParams.mOutputPathString));
                  
            strPathAddBackSlash(dstFullFilename, false);
            
            if ((!mCmdLineParams.mOutSameDirFlag) && (mCmdLineParams.mRecreateFlag))
            {
               dstFullFilename += srcRelPathname;       
               
               strPathAddBackSlash(dstFullFilename, false);  
            }
            
            if ((!mCmdLineParams.mSimulateFlag) && (!mCmdLineParams.mOutSameDirFlag) && (mCmdLineParams.mRecreateFlag))
               strPathCreateFullPath(dstFullFilename);
                     
            dstFullFilename += srcFName;
            dstFullFilename += B(".");
            dstFullFilename += BFileType::getExtension(mOutputFileType);
         }
                           
         WIN32_FILE_ATTRIBUTE_DATA dstFileAttributes;
         const BOOL dstFileExists = GetFileAttributesEx(dstFullFilename.getPtr(), GetFileExInfoStandard, &dstFileAttributes);
                  
         if (dstFileExists)
         {
            if (mCmdLineParams.mNoOverwrite)
            {
               ddxConvPrintf("Skipping already existing file: %s\n", dstFullFilename.getPtr());
               mNumFilesSkipped++;
               continue;
            }
                        
            if (mCmdLineParams.mTimestampFlag)
            {
               WIN32_FILE_ATTRIBUTE_DATA srcFileAttributes;
               const BOOL srcFileExists = GetFileAttributesEx(srcFullFilename.getPtr(), GetFileExInfoStandard, &srcFileAttributes);
               
               if (srcFileExists)
               {
                  LONG timeComp = CompareFileTime(&srcFileAttributes.ftLastWriteTime, &dstFileAttributes.ftLastWriteTime);
                  if (timeComp <= 0)
                  {
                     ddxConvPrintf("Skipping up to date file: %s\n", dstFullFilename.getPtr());
                     mNumFilesSkipped++;
                     continue;
                  }
               }                  
            }
         }         
         
         if (mCmdLineParams.mTexTypeToProcess.length() > 0)
         {
            mCmdLineParams.mTexTypeToProcess.toLower();
            BTextureMetadata::BTexType texType = BTextureMetadata::determineTextureType((BString("_") + mCmdLineParams.mTexTypeToProcess).getPtr());
            BTextureMetadata::BTexType srcTexType = BTextureMetadata::determineTextureType(srcFullFilename.getPtr());
            if ((texType != BTextureMetadata::cTexTypeUnknown) && (texType != srcTexType))
            {
               ddxConvWarning("File is not the proper type, skipping: %s\n", srcFullFilename.getPtr());
               mNumFilesSkipped++;
               continue;
            }
         }   
         
         BString srcFullFilenameLCase(srcFullFilename);
         srcFullFilenameLCase.toLower();
         
         uint i;
         for (i = 0; i < mCmdLineParams.mExcludeStrings.getSize(); i++)
         {  
            BString excludeStr(mCmdLineParams.mExcludeStrings[i]);
            excludeStr.toLower();
                        
            if (strstr(srcFullFilenameLCase, excludeStr) != NULL)
            {
               ddxConvWarning("File matches exclude string, skipping: %s\n", srcFullFilename.getPtr());
               mNumFilesSkipped++;
               break;
            }
         }
         if (i < mCmdLineParams.mExcludeStrings.getSize())
            continue;
                          
         ddxConvPrintf("Processing file %u [%u to %u]: %s\n", 1 + fileIndex, 1 + startFileIndex, endFileIndex, srcFullFilename.getPtr());                                                      
         
         bool success = false;
         bool fileSkipped = false;
         if (mCmdLineParams.mHDRConvertMode)
            success = hdrConvertFile(srcFullFilename.getPtr(), srcFileType, dstFullFilename.getPtr(), mOutputFileType);
         else if (mCmdLineParams.mInfoMode)
            success = displayFileInfo(srcFullFilename.getPtr(), srcFileType);
         else
            success = convertFile(srcFullFilename.getPtr(), srcFileType, dstFullFilename.getPtr(), mOutputFileType, fileSkipped);
         
         if (!success)
         {
            if (fileSkipped)
            {
               mNumFilesSkipped++;
               
               ddxConvWarning("Skipped processing file: %s\n", srcFullFilename.getPtr());
            }
            else
            {
               mNumFailedFiles++;
            
               ddxConvError("Failed processing file: %s\n", srcFullFilename.getPtr());
            
               if (!mCmdLineParams.mIgnoreErrors)
                  return false;
            }                  
         }
         else
         {
            mNumFilesProcessed++;
         }
      }
   
      return true;
   }
   
   bool findFiles(void)
   {
      mSourceFiles.clear();

      int findFilesFlag = BFindFiles::FIND_FILES_WANT_FILES;
      if (mCmdLineParams.mDeepFlag)
         findFilesFlag |= BFindFiles::FIND_FILES_RECURSE_SUBDIRS;

      for (uint fileSpecIndex = 0; fileSpecIndex < mCmdLineParams.mFileStringArray.getSize(); fileSpecIndex++)
      {
         TCHAR fullpath[_MAX_PATH];
         LPTSTR pFilePart = NULL;

         DWORD result = GetFullPathName(mCmdLineParams.mFileStringArray[fileSpecIndex].getPtr(), _MAX_PATH, fullpath, &pFilePart);
         if (0 == result)            
         {
            ddxConvError("Can't resolve path: %s\n", mCmdLineParams.mFileStringArray[fileSpecIndex].getPtr());
            return false;
         }

         BString path;
         BString filename;
         path.set(fullpath, pFilePart - fullpath);
         filename.set(pFilePart);

         const DWORD fullAttributes = GetFileAttributes(fullpath);

         if ((fullAttributes != INVALID_FILE_ATTRIBUTES) && (fullAttributes & FILE_ATTRIBUTE_DIRECTORY))
         {
            // If the full path points to a directory then change the filename part to be a wildcard.
            path.set(fullpath);
            filename.set(B("*.*"));
         }
         else
         {
#if 0         
            // Remove backslash from end of path
            const BCHAR_T lastChar = path.getPtr()[path.length() - 1];
            if ((lastChar == B('\\')) || (lastChar == B('/')))
            {
               path.set(path.getPtr(), path.length() - 1);
            }
#endif            
         }

         BFindFiles findFiles(path, filename, findFilesFlag);

         if (!findFiles.success())
         {
            ddxConvError("Can't find files: %s\n", mCmdLineParams.mFileStringArray[fileSpecIndex].getPtr());
         }

         for (uint fileIndex = 0; fileIndex < findFiles.numFiles(); fileIndex++)
         {
            BFilePath filePath(findFiles.getFile(fileIndex));
            
            bool foundReplacement = false;
            if (fileSpecIndex > 0)
            {
               // This search is slow as hell, but I don't expect anything but the first path to have many files.
               BString filename(filePath.fullFilename());
               strPathRemoveExtension(filename);
               
               for (uint compFileIndex = 0; compFileIndex < mSourceFiles.getSize(); compFileIndex++)
               {
                  BString compFilename(mSourceFiles[compFileIndex].fullFilename());
                  strPathRemoveExtension(compFilename);
                  
                  if (compFilename == filename)
                  {
                     ddxConvWarning("Not processing redundant file \"%s\" because file \"%s\" has the same base filename.\n", 
                        mSourceFiles[compFileIndex].fullFilename().getPtr(), 
                        filePath.fullFilename().getPtr());
                                             
                     mSourceFiles[compFileIndex] = filePath;
                     foundReplacement = true;
                     
                     break;
                  }
               }
            }
            
            if (!foundReplacement)
               mSourceFiles.pushBack(filePath);
         }
      }            

      return true;
   }
   
   void clear(void)
   {
      mSourceFiles.clear();

      mRegularFormat          = cDDXDataFormatA8R8G8B8;
      mAlphaFormat            = cDDXDataFormatA8R8G8B8;
      mNormalMapFormat        = cDDXDataFormatA8R8G8B8;
      mNormalMapAlphaFormat   = cDDXDataFormatA8R8G8B8;
      mHDRFormat              = cDDXDataFormatA16B16G16R16F;
      mHDRAlphaFormat         = cDDXDataFormatA16B16G16R16F;

      mOutputFileType         = cFileTypeDDX;
      
      mNumFilesProcessed      = 0;
      mNumFilesSkipped        = 0;
      mNumFailedFiles         = 0;
      mNumNonImageFiles       = 0;
      
      Utils::ClearObj(mFileStats);
      Utils::ClearObj(mOverallFileStats);
   }
   
   void pause(void)
   {
      ddxConvWarning("Press a key to continue.\n", gTotalWarningMessages, gTotalErrorMessages);
      for ( ; ; )
      {
         if (_getch() != -1)
            break;
      }
   }
   
public:
   BDDXConv(BDDXDLLHelper& DDXDLLHelper) :
      mDDXDLLHelper(DDXDLLHelper)
   {
      clear();
   }
   
   bool process(int argc, const char * const argv[])
   {
      clear();
      
      if (!processParams(argc, argv))
      {
         if ((!mCmdLineParams.mQuietFlag) && (mCmdLineParams.mPauseOnWarningsOrErrors))
         {
            ddxConvWarning("There where %i warnings and %i errors.\n", gTotalWarningMessages, gTotalErrorMessages);
            pause();
         }
         
         return false;
      }
         
      if (mCmdLineParams.mFileStringArray.empty())
      {
         ddxConvError("No files specified to process!\n");
         
         if ((!mCmdLineParams.mQuietFlag) && (mCmdLineParams.mPauseOnWarningsOrErrors))
         {
            ddxConvWarning("There where %i warnings and %i errors.\n", gTotalWarningMessages, gTotalErrorMessages);
            pause();
         }
         
         return false;
      }
      
      if (!findFiles())
      {
         closeLogFile();
         closeErrorLogFile();
         
         if ((!mCmdLineParams.mQuietFlag) && (mCmdLineParams.mPauseOnWarningsOrErrors))
         {
            ddxConvWarning("There where %i warnings and %i errors.\n", gTotalWarningMessages, gTotalErrorMessages);
            pause();
         }
         
         return false;
      }
      
      if (mSourceFiles.empty())
      {
         ddxConvError("No files found to process!\n");
         
         if ((!mCmdLineParams.mQuietFlag) && (mCmdLineParams.mPauseOnWarningsOrErrors))
         {
            ddxConvWarning("There where %i warnings and %i errors.\n", gTotalWarningMessages, gTotalErrorMessages);
            pause();
         }
         
         return false;
      }
       
      bool status = processFiles();
      
      ddxConvPrintf("Files processed successfully: %i, skipped: %i, failed: %i, non-image skipped: %i\n", mNumFilesProcessed, mNumFilesSkipped, mNumFailedFiles, mNumNonImageFiles);
      
      if (mCmdLineParams.mFileStats)
      {
         for (int resTypeIter = -1; resTypeIter < cDDXResourceTypeMax; resTypeIter++)
         {
            const BFileStats& stats = (resTypeIter < 0) ? mOverallFileStats : mFileStats[resTypeIter];
            
            if (resTypeIter < 0)
               ddxConvPrintf("Overall file stats:\n");
            else
               ddxConvPrintf("File stats: %s\n", getDDXResourceTypeString((eDDXResourceType)resTypeIter));
            ddxConvPrintf("    Num files: %u\n", stats.mNumFiles);
            ddxConvPrintf("    Total size: %u\n", stats.mTotalSize);
            ddxConvPrintf("    By resolution:\n");
            for (uint i = 0; i < BFileStats::cMaxResHist; i++)
               ddxConvPrintf("      %04u: %u\n", 1<<i, stats.mResHist[i]);
            ddxConvPrintf("    By format:\n");
            for (uint i = 0; i < cDDXDataFormatMax; i++)
               ddxConvPrintf("      %s: %u\n", getDDXDataFormatString((eDDXDataFormat)i), stats.mFormatHist[i]);
         }
      }
      
      ddxConvPrintf("Total errors: %u, Total warnings: %u\n", gTotalErrorMessages, gTotalWarningMessages);
      
      closeLogFile();
      closeErrorLogFile();
                  
      if ( ((gTotalWarningMessages) || (gTotalErrorMessages)) && (!mCmdLineParams.mQuietFlag) && (mCmdLineParams.mPauseOnWarningsOrErrors))
      {
         ddxConvWarning("There where %i warnings and %i errors.\n", gTotalWarningMessages, gTotalErrorMessages);
         pause();
      }
               
      return status;    
   }
};

//-------------------------------------------------

static bool isQuietModeEnabled(int argc, const char *argv[])
{
   for (int i = 1; i < argc; i++)
      if (_stricmp(argv[i], "-quiet") == 0)
         return true;
   return false;         
}

//-------------------------------------------------

//-------------------------------------------------

static int mainInternal(int argc, const char *argv[])
{
   XCoreCreate();
   
   gConsoleOutput.init(ConsoleOutputFunc, 0);

//extern int lzpTest(int argC, const char* argV[]);
//return lzpTest(argc, argv);
  
   bool bQuietEnabled = isQuietModeEnabled(argc, argv);

   if(!bQuietEnabled)
      ddxConvPrintf("DDXConv Compiled %s %s\n", __DATE__, __TIME__);
            
   BDDXDLLHelper DDXDLL;
   if (!DDXDLL.init())
   {
      ddxConvError("Unable to load DDX.DLL!\n");
      
      XCoreRelease();

      return 100;
   }
               
   if(!bQuietEnabled)
   {
      ddxConvPrintf("Using DDX.DLL version %i\n", DDXDLL.getInterface()->getVersion());
      ddxConvPrintf("\n");
   }
        
   DWORD processAffinity, systemAffinity;
   BOOL result = GetProcessAffinityMask(GetCurrentProcess(), &processAffinity, &systemAffinity);
   if (result)
      SetProcessAffinityMask(GetCurrentProcess(), systemAffinity);
      
   BDDXConv ddxConv(DDXDLL);
   
   DWORD startTime = GetTickCount();
   
   const bool success = ddxConv.process(argc, argv);
   
   DWORD endTime = GetTickCount();
   
   ddxConvPrintf("Total time: %f\n", (endTime - startTime) * .001f);
               
   destroyD3DREF();   
   
   DDXDLL.deinit();
      
   XCoreRelease();
                              
   return success ? EXIT_SUCCESS : EXIT_FAILURE;
}

int main(int argC, const char *argV[])
{
   int status;

#ifndef BUILD_DEBUG   
   __try
#endif   
   {
      status = mainInternal(argC, argV);
   }
#ifndef BUILD_DEBUG   
   __except(EXCEPTION_EXECUTE_HANDLER)
   {
      fprintf(stderr, "DDXConv: Unhandled exception!");
      return 100;
   }
#endif   

   return status;
}

