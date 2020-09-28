#define _WIN32_WINNT 0x0500 
#include <windows.h>
#include <conio.h>
#include <stdio.h>
#include <d3d9.h>
#include <d3dx9.h>
#include <stdarg.h>

#include "ecore.h"
#include "alignedarray.h"

#include "DDXDLL.h"
#include "DDXDLLHelper.h"
#include "ddxdll.h"

#include "TextureFileConverter.h"
#include "commandLineParser.h"

static HWND                 g_hWnd            = NULL;

static HINSTANCE            hInstance         = NULL;
static LPDIRECT3D9          g_pD3D            = NULL;
static LPDIRECT3DDEVICE9    g_pd3dDevice      = NULL;
static BDDXDLLHelper        g_DDXDLL;

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
      
      break;
   }

   return 0;
}

//----------------------------------------------------------------

static bool createD3DREF()
{
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

   int nDefaultWidth = 16;
   int nDefaultHeight = 16;
   DWORD dwWindowStyle = 0;//WS_OVERLAPPEDWINDOW | WS_VISIBLE;
   RECT rc;        
   SetRect( &rc, 0, 0, nDefaultWidth, nDefaultHeight );        
   AdjustWindowRect( &rc, dwWindowStyle, false);

   g_hWnd = CreateWindowEx( WS_EX_NOACTIVATE, "DDXCONV_WINDOWS_CLASS", 
      "ddxconv",
      dwWindowStyle,
      0, 0, (rc.right-rc.left), (rc.bottom-rc.top), NULL, NULL, hInstance, NULL );

   if(!g_hWnd)
      return false;
 
   UpdateWindow( g_hWnd );

   //CREATE D3D REF OBJECT
   g_pD3D = Direct3DCreate9( D3D_SDK_VERSION );
   if (!g_pD3D)
      return false;

   D3DDISPLAYMODE d3ddm;

   g_pD3D->GetAdapterDisplayMode( D3DADAPTER_DEFAULT, &d3ddm );

   D3DPRESENT_PARAMETERS d3dpp;
   ZeroMemory( &d3dpp, sizeof(d3dpp) );

   d3dpp.Windowed               = TRUE;
   d3dpp.SwapEffect             = D3DSWAPEFFECT_DISCARD;
   d3dpp.BackBufferFormat       = d3ddm.Format;
   d3dpp.EnableAutoDepthStencil = TRUE;
   d3dpp.AutoDepthStencilFormat = D3DFMT_D16;
   d3dpp.PresentationInterval   = D3DPRESENT_INTERVAL_IMMEDIATE;

   HRESULT hres = g_pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_REF, 
      g_hWnd,
      D3DCREATE_SOFTWARE_VERTEXPROCESSING,
      &d3dpp, &g_pd3dDevice );

   return SUCCEEDED(hres);      
}

//-------------------------------------------------

static void destroyD3DREF()
{
   if( g_pd3dDevice != NULL )
      g_pd3dDevice->Release();

   if( g_pD3D != NULL )
      g_pD3D->Release();

   UnregisterClass( "DDXCONV_WINDOWS_CLASS", hInstance );
}

//-------------------------------------------------

static void texConvError(const char* pMsg, ...)
{
   va_list args;
   va_start(args, pMsg);
   char buf[512];
   StringCchVPrintfA(buf, sizeof(buf), pMsg, args);
   va_end(args);

   printf("%s", buf);
}

//-------------------------------------------------

class BCommandLineParams
{
public:
   bool mBestFlag;
   bool mQuickFlag;
   bool mDiscardAlphaFlag;
   bool mNoAutoMipFlag;
   bool mNormalMapFlag;
   bool mDitherFlag;
   bool mWrapFlag;
   bool mNoPerceptualFlag;
   
   int mMinMipDim;
   bool mSpecifiedMinMipDim;
   int mMipLevels;
   bool mSpecifiedMipLevels;
   int mDXTMMethod;
   int mDXTMMipMethod;

   int mDXTMQuality;
   int mDXTMMipQuality;
   bool mSpecifiedMipQuality;
   bool mSpecifiedMipMethod;

   int mDXTMCodebook;
   int mDXTMMipCodebook;
   bool mSpecifiedMipCodebook;

   BString mFileString;
   BString mOutputString;
   BString mAppendString;
   bool mOutSameDirFlag;
   bool mAllFlag;
   bool mTimestampFlag;
   bool mDeepFlag;
   bool mRecreateFlag;

   BString mErrorMessage;

   BString mNormalFormat;
   BString mNormalMipFormat;
   bool mSpecifiedNormalFormat;
   bool mSpecifiedMipFormat;
   
   BString mAlphaFormat;
   BString mAlphaMipFormat;
   bool mSpecifiedAlphaMipFormat;
   bool mSpecifiedAlphaFormat;
   bool mAutoDetermineNormalMaps;

   BCommandLineParams() :
      mBestFlag(false),
      mQuickFlag(false),
      mDiscardAlphaFlag(false),
      mNoAutoMipFlag(false),
      mNormalMapFlag(false),
      mDitherFlag(false),
      mWrapFlag(false),
      mNoPerceptualFlag(false),
      mSpecifiedAlphaMipFormat(false),
      mSpecifiedAlphaFormat(false),
      mSpecifiedNormalMipFormat(false),
      mSpecifiedNormalFormat(false),
      mMinMipDim(0),
      mSpecifiedMinMipDim(false),
      mMipLevels(0),
      mSpecifiedMipLevels(false),
      mDXTMMethod(0),
      mDXTMMipMethod(0),
      mDXTMQuality(92),
      mDXTMMipQuality(92),
      mSpecifiedMipQuality(false),
      mSpecifiedMipMethod(false),
      mDXTMCodebook(0),
      mDXTMMipCodebook(0),
      mSpecifiedMipCodebook(false),
      mOutSameDirFlag(false),
      mAllFlag(false),
      mTimestampFlag(false),
      mDeepFlag(false),
      mRecreateFlag(false),
      mAutoDetermineNormalMaps(false)
   {
   }

   bool parse(int argc, char* argv[])
   {
      const BCLParam clParams[] =
      {
         {"file", cCLParamTypeUserBString, &mFileString },
         {"output", cCLParamTypeUserBString, &mOutputString },
         {"append", cCLParamTypeUserBString, &mAppendString },
         {"outsamedir", cCLParamTypeFlag, &mOutSameDirFlag },
         {"all", cCLParamTypeFlag, &mAllFlag },
         {"timestamp", cCLParamTypeFlag, &mTimestampFlag },
         {"deep", cCLParamTypeFlag, &mDeepFlag },
         {"recreate", cCLParamTypeFlag, &mRecreateFlag },

         {"best", cCLParamTypeFlag, &mBestFlag },
         {"quick", cCLParamTypeFlag, &mQuickFlag },
         {"discardalpha", cCLParamTypeFlag, &mDiscardAlphaFlag },
         {"noautomip", cCLParamTypeFlag, &mNoAutoMipFlag },
         {"normalmap", cCLParamTypeFlag, &mNormalMapFlag },
         {"dither", cCLParamTypeFlag, &mDitherFlag },
         {"wrap", cCLParamTypeFlag, &mWrapFlag },
         {"noperceptual", cCLParamTypeFlag, &mNoPerceptualFlag },

         {"minmipdim", cCLParamTypeUserInt, &mMinMipDim, 1, 0, 0, 0, &mSpecifiedMinMipDim },
         {"miplevels", cCLParamTypeUserInt, &mMipLevels, 1, 0, 0, 0, &mSpecifiedMipLevels },
         
         {"autonormalmap", cCLParamTypeFlag, &mAutoDetermineNormalMaps },
         
         {"p", cCLParamTypeUserInt, &mDXTMMethod },
         {"pm", cCLParamTypeUserInt, &mDXTMMipMethod, 1, 0, 0, 0, &mSpecifiedMipMethod },
         {"q", cCLParamTypeUserInt, &mDXTMQuality },
         {"qm", cCLParamTypeUserInt, &mDXTMMipQuality, 1, 0, 0, 0, &mSpecifiedMipQuality },
         {"c", cCLParamTypeUserInt, &mDXTMCodebook },
         {"cm", cCLParamTypeUserInt, &mDXTMMipCodebook, 1, 0, 0, 0, &mSpecifiedMipCodebook },

         {"format", cCLParamTypeUserBString, &mNormalFormat },
         {"mipformat", cCLParamTypeUserBString, &mNormalMipFormat, 1, 0, 0, 0, &mSpecifiedMipFormat },
         {"alphaformat", cCLParamTypeUserBString, &mAlphaFormat, 1, 0, 0, 0, &mSpecifiedAlphaFormat },
         {"alphamipformat", cCLParamTypeUserBString, &mAlphaFormat, 1, 0, 0, 0, &mSpecifiedAlphaMipFormat },
      };

      const uint numCLParams = sizeof(clParams)/sizeof(clParams[0]));

      BCommandLineParser parser(clParams, numCLParams);

      const bool success = parser.parse(argc, argv);
      mErrorMessage.set(parser.getErrorString());
      return success;
   }

   void printHelp(void)
   {
      //      --------------------------------------------------------------------------------
      printf("Usage: ddxconv <options>\n");
      printf("Supported file formats:\nDDX,DDS,DDT,BMP,TGA,PNG,JPG (DDT is ready only)\n");
      printf("Options:\n");

      printf("  -file filename   Specify source filename (wildcards okay)\n");
      printf("  -output path     Specify output path/filename\n");
      printf("  -append string   Append string to filename\n");
      printf("  -outsamedir      Write output files to source path\n");
      printf("  -all             Process all supported image files\n");
      printf("  -timestamp       Compare timestamps and skip files that are not outdated\n");
      printf("  -deep            Recurse subdirectories\n");
      printf("  -recreate        Recreate directory structure\n");

      printf("  -discardalpha    Discard alpha (or save 255's to dest image if it has alpha).\n");
      printf("  -noautomip       Do not automatically generate mipmap levels. Use original\n");
      printf("                   file's mipmaps levels (if any).\n");
      printf("  -minmipdim #     Set minimum mipmap dimension.\n");
      printf("  -miplevels #     Set number of desired mipmap levels.\n");

      printf("\n");

      printf("Options valid when converting to DDX:\n");
      printf("  -quick           Lowest quality DXT1/3/5 conversion\n");
      printf("  -best            Best quality DXT1/3/5 conversion (slower)\n");
      printf("  -normalmap       Process as normal map.\n");
      printf("  -dither          Dither image before converting to DXT1, 3, or 5.\n");
      printf("  -wrap            Use wrap addressing when filtering mipmaps, instead of clamp.\n");
      printf("  -noperceptual    Don't use perceptual distance metric when convering to DXT.\n");
      printf("  -p[m] #          DXTM compression method (0 or 1, default is 0).\n");
      printf("                   Specify -pm for mipchain compression method.\n");
      printf("  -q[m] #          DXTM compression quality (1 to 99, 99 is best).\n");
      printf("                   Specify -qm for mipchain compression quality.\n");
      printf("  -c[m] #          DXTM codebook size (0-8192, default is 0, try 128-1024).\n");
      printf("                   Specify -cm for mipchain codebook size.\n");
      printf("\n");

      printf("Texture format:\n");
      printf("  -format          Format for files without alpha\n");
      printf("  -mipformat       Mip format for files without alpha\n");
      printf("  -alphaformat     Format for files without alpha\n");
      printf("  -mipalphaformat  Mip format for files without alpha\n");
      printf("Supported formats:\n");
      for (uint i = 0; i < cDDXDataFormatMax; i++)
         printf("  %s\n", getDDXDataFormatString(static_cast<eDDXDataFormat>(i)));
   }
}; // class BCommandLineParams

static eDDXDataFormat getTextureFormat(const char* pStr)
{
   for (uint i = 0; i < cDDXDataFormatMax; i++)
      if (stricmp(pStr, getDDXDataFormatString(static_cast<eDDXDataFormat>(i))) == 0)
         return static_cast<eDDXDataFormat>(i);
   
   return cDDXDataFormatInvalid;
}

//-------------------------------------------------

#if 0
static bool convertTexture(
   const char* pSrcFilename, 
   const char* pDstFilename, 
   BDDXPackParams& packParams,
   bool retrieveDDXSourceImage,
   BTextureFileConverter::BConvertParams::eAlphaMode alphaMode,
   uint minMipDimension,
   int desiredMipChainLevels,
   bool autoDataFormat)
{
   BTextureFileConverter::EFileType srcFileType = BTextureFileConverter::determineFileType(pSrcFilename);
   BTextureFileConverter::EFileType dstFileType = BTextureFileConverter::determineFileType(pDstFilename);
   
   if (srcFileType == BTextureFileConverter::cFileTypeInvalid)
   {
      texConvError("Unsupported source file type!\n");
      return false;
   }
   
   if (dstFileType == BTextureFileConverter::cFileTypeInvalid)
   {
      texConvError("Unsupported source file type!\n");
      return false;
   }
        
   

   BTextureFileConverter::BConvertParams convertParams;
   convertParams.mMinMipDimension = minMipDimension;
   convertParams.mRetrieveDDXSourceImage = retrieveDDXSourceImage;
   convertParams.mAlphaMode = alphaMode;
   convertParams.mDesiredMipChainLevels = desiredMipChainLevels;
   convertParams.mAutoDataFormat = autoDataFormat;
         
   bool success = textureFileConverter.convert(
         pSrcFilename, srcFileType, 
         pDstFilename, dstFileType, 
         convertParams,
         packParams);
   
   return success;
}
#endif

//-------------------------------------------------

static bool doesFileExist(const char* pFilename)
{
   FILE* pFile = fopen(pFilename, "rb");
   if (pFile)
      fclose(pFile);
   return NULL != pFile;
}

int main(int argc, const char *argv[])
{
   printf("ddxconv compiled %s %s\n", __DATE__, __TIME__);
      
   if (!g_DDXDLL.init())
   {
      texConvError("Unable to load DDX.DLL!\n");
      return EXIT_FAILURE;
   }
   
   printf("Using DDX.DLL version %i\n", g_DDXDLL.getInterface()->getVersion());
   
   BCommandLineParams params;
   
   if (argc < 2)
   {
      params.printHelp();
      return EXIT_FAILURE;
   }
   
   if (!params.parse(argc, argv))
   {
      printf("Error: %s\n", params.mErrorMessage.asANSI());
      return EXIT_FAILURE;
   }
      
   if (!createD3DREF())
   {
      texConvError("Unable to create D3D device!\n");
      return EXIT_FAILURE;
   }
   
   BTextureFileConverter textureFileConverter(g_pd3dDevice, g_DDXDLL);
   
#if 0
   bool mBestFlag;
   bool mQuickFlag;
   bool mDiscardAlphaFlag;
   bool mNoAutoMipFlag;
   bool mNormalMapFlag;
   bool mDitherFlag;
   bool mWrapFlag;
   bool mNoPerceptualFlag;

   eDDXDataFormat mDDXFormat;
   int mMinMipDim;
   bool mSpecifiedMinMipDim;
   int mMipLevels;
   bool mSpecifiedMipLevels;
   int mDXTMMethod;
   int mDXTMMipMethod;

   int mDXTMQuality;
   int mDXTMMipQuality;
   bool mSpecifiedMipQuality;

   int mDXTMCodebook;
   int mDXTMMipCodebook;
   bool mSpecifiedMipCodebook;

   BString mFileString;
   BString mOutputString;
   BString mAppendString;
   bool mOutSameDirFlag;
   bool mAllFlag;
   bool mTimestampFlag;
   bool mDeepFlag;
   bool mRecreateFlag;

   BString mErrorMessage;

   BString mNormalFormat;
   BString mAlphaFormat;
#endif   
   
   BDDXPackParams packParams;

//   packParams.mDataFormat = cDDXDataFormatDXT5;
//   packParams.mMipDataFormat = cDDXDataFormatDXT5;
   
   if (params.mBestFlag)
      packParams.mPackerFlags |= BDDXPackParams::cDXTBest;
   else if (params.mQuickFlag)
      packParams.mPackerFlags |= BDDXPackParams::cDXTFast;
      
   if (params.mNoAutoMipFlag)
      packParams.mPackerFlags &= ~BDDXPackParams::cGenerateMips;

   packParams.mDXTMCodebookSize = Math::Clamp(params.mDXTMCodebook, 0, BDDXPackParams::cDXTMMaxCodebookSize);
   packParams.mDXTMMipCodebookSize = Math::Clamp(params.mDXTMMipCodebook, 0, BDDXPackParams::cDXTMMaxCodebookSize);
   if (!params.mSpecifiedMipCodebook)
      packParams.mDXTMMipCodebookSize = packParams.mDXTMCodebookSize;
   
   packParams.mDXTMQuality = Math::Clamp(packParams.mDXTMQuality, BDDXPackParams::cDXTMMinQuality, BDDXPackParams::cDXTMMaxQuality);
   packParams.mDXTMMipQuality = packParams.mDXTMQuality;
   if (!params.mSpecifiedMipQuality)
      packParams.mDXTMMipQuality = packParams.mDXTMQuality;
      
   packParams.mDXTMCompressionMethod = Math::Clamp<uint>(params.mDXTMMethod, 0, BDDXPackParams::cDXTMNumCompressionMethods - 1);      
   packParams.mDXTMMipCompressionMethod = Math::Clamp<uint>(params.mDXTMMipMethod, 0, BDDXPackParams::cDXTMNumCompressionMethods - 1);      
   if (!params.mSpecifiedMipMethod)
      packParams.mDXTMMipCompressionMethod = packParams.mDXTMCompressionMethod;
            
   if (params.mNormalMapFlag)   
      packParams.mResourceType = cDDXResourceTypeNormalMap;
      
   if (params.mDitherFlag)
      packParams.mPackerFlags |= BDDXPackParams::cDXTDithering;
   
   if (params.mWrapFlag)
      packParams.mWrapFlag |= BDDXPackParams::cUseWrapFiltering;
      
   BTextureFileConverter::BConvertParams::eAlphaMode alphaMode = BTextureFileConverter::BConvertParams::cAlphaModeAuto;

   if (params.mDiscardAlphaFlag)    
      alphaMode = BTextureFileConverter::BConvertParams::cAlphaModeDiscard;
      
   if (params.mNoPerceptualFlag)
      packParams.mPackerFlags &= ~BDDXPackParams::cPerceptual;
      
   
         
#if 0   
   
   
   bool retrieveDDXSourceImage = false;
   bool specifiedMipQualityLevel = false;
   bool specifiedQualityLevel = false;
   bool specifiedMipCompressionMethod = false;
   bool specifiedCompressionMethod = false;
   bool specifiedFormat = false;
   bool specifiedMipFormat = false;
   bool specifiedCodebookSize = false;
   bool specifiedMipCodebookSize = false;
   bool autoDataFormat = true;
      
   int minMipDimension = 1;
   int desiredMipChainLevels = -1;
         
   
#endif   
   

#if 0               
   for (int i = 1; i < argc; i++)
   {
      const char* pOption = argv[i];
      if (pOption[0] == '-')
      {
         const uchar optionChar = pOption[1];
         if (stricmp(argv[i], "-best") == 0)
            packParams.mPackerFlags |= BDDXPackParams::cDXTBest;
         else if (stricmp(argv[i], "-quick") == 0)
            packParams.mPackerFlags |= BDDXPackParams::cDXTFast;
         else if (stricmp(argv[i], "-minmipdim") == 0)
         {
            minMipDimension = Math::Clamp<uint>(atoi(&pOption[2]), 1, 8192);
         }
         else if (optionChar == 'g')
         {
            packParams.mPackerFlags &= ~BDDXPackParams::cGenerateMips;
         }
         else if (optionChar == 'c')
         {
            if (pOption[2] == 'm')
            {
               packParams.mDXTMMipCodebookSize = Math::Clamp<uint>(atoi(&pOption[3]), 0, BDDXPackParams::cDXTMMaxCodebookSize);
               specifiedMipCodebookSize = true;
               
               if (!specifiedCodebookSize)
                  packParams.mDXTMCodebookSize = packParams.mDXTMMipCodebookSize;
            }
            else
            {
               packParams.mDXTMCodebookSize = Math::Clamp<uint>(atoi(&pOption[2]), 0, BDDXPackParams::cDXTMMaxCodebookSize);
               specifiedCodebookSize = true;
               
               if (!specifiedMipCodebookSize)
                  packParams.mDXTMMipCodebookSize = packParams.mDXTMCodebookSize;
            }
         }
         else if (optionChar == 'q')  //DXTM COMPRESSION QUALITY
         {
            if (pOption[2] == 'm')
            {
               packParams.mDXTMMipQuality = Math::Clamp<uint>(atoi(&pOption[3]), BDDXPackParams::cDXTMMinQuality, BDDXPackParams::cDXTMMaxQuality);
               if (!specifiedQualityLevel)
                  packParams.mDXTMQuality = packParams.mDXTMMipQuality;
               specifiedMipQualityLevel = true;
            }
            else
            {
               packParams.mDXTMQuality = Math::Clamp<uint>(atoi(&pOption[2]), BDDXPackParams::cDXTMMinQuality, BDDXPackParams::cDXTMMaxQuality);
               if (!specifiedMipQualityLevel)
                  packParams.mDXTMMipQuality = packParams.mDXTMQuality;
               specifiedQualityLevel = true;
            }
         }
         else if (optionChar == 'p')   //DXTM COMPRESSION METHOD
         {
            if (pOption[2] == 'm')
            {
               packParams.mDXTMMipCompressionMethod = Math::Clamp<uint>(atoi(&pOption[3]), 0, BDDXPackParams::cDXTMNumCompressionMethods - 1);
               if (!specifiedCompressionMethod)
                  packParams.mDXTMCompressionMethod = packParams.mDXTMMipCompressionMethod;
               specifiedMipCompressionMethod = true;
            }
            else
            {
               packParams.mDXTMCompressionMethod = Math::Clamp<uint>(atoi(&pOption[2]), 0, BDDXPackParams::cDXTMNumCompressionMethods - 1);
               if (!specifiedMipCompressionMethod)
                  packParams.mDXTMMipCompressionMethod = packParams.mDXTMCompressionMethod;
               specifiedCompressionMethod = true;
            }
         }
         
         else if (optionChar == 'f')   
         {
            uchar c = pOption[2];
            
            eDDXDataFormat* pFormat = &packParams.mDataFormat;
            if (c == 'm')
            {
               c = pOption[3];
               pFormat = &packParams.mMipDataFormat;
               specifiedMipFormat = true;
            }
            else
               specifiedFormat = true;
                                    
            switch (tolower(c))
            {
               case '1': *pFormat = cDDXDataFormatDXT1; break;
               case '3': *pFormat = cDDXDataFormatDXT3; break;
               case '5': *pFormat = cDDXDataFormatDXT5; break;
               case 'l': *pFormat = cDDXDataFormatDXT5Y; break;
               
               case 'x': *pFormat = cDDXDataFormatDXTM; break;
               case 'a': *pFormat = cDDXDataFormatA8; break;
               case 'b': *pFormat = cDDXDataFormatA8R8G8B8; break;
               case 'c': *pFormat = cDDXDataFormatA8B8G8R8; break;
               default:
               {  
                  texConvError("Invalid format: \"%s\"\n", pOption);
                  return EXIT_FAILURE;
               }
            }
            
            if (pFormat == &packParams.mMipDataFormat)
            {
               if (!specifiedFormat)
                  packParams.mDataFormat = packParams.mMipDataFormat;
            }
            else
            {
               if (!specifiedMipFormat)
                  packParams.mMipDataFormat = packParams.mDataFormat;
            }
            
            autoDataFormat = false;
         }
         else if (optionChar == 'd')
         {
            packParams.mPackerFlags |= BDDXPackParams::cDXTDithering;
         }
         else if (optionChar == 'w')
         {
            packParams.mPackerFlags |= BDDXPackParams::cUseWrapFiltering;
         }
         else if (optionChar == 's')
         {
            packParams.mPackerFlags |= BDDXPackParams::cARSwizzle;
         }
         else if (optionChar == 'a')
         {
            if (pOption[2] == '\0')
               alphaMode = BTextureFileConverter::BConvertParams::cAlphaModeDiscard;
            else
            {
               int mode = atoi(&pOption[2]);
               if ((alphaMode >= 0) && (mode < (int)BTextureFileConverter::BConvertParams::cAlphaModeTotal))
                  alphaMode = (BTextureFileConverter::BConvertParams::eAlphaMode)mode;
            }
         }
         else if (optionChar == 'i')
         {
            desiredMipChainLevels = atoi(&pOption[2]);
            minMipDimension = 0;
         }
         else if (optionChar == 'o')
         {
            retrieveDDXSourceImage = true;
            break;
         }
         else if (optionChar == 'n')
         {
            packParams.mResourceType = cDDXResourceTypeNormalMap;
         }
         else
         {
            texConvError("Invalid command line option: \"%s\"\n", pOption);
            printHelp();
            return EXIT_FAILURE;
         }
      }
      else if (!pSrcFilename)
         pSrcFilename = pOption;
      else if (!pDstFilename)
         pDstFilename = pOption;
      else
      {
         texConvError("Invalid command line option: \"%s\"\n", pOption);
         printHelp();
         return EXIT_FAILURE;
      }
   }
   
   if (!pSrcFilename || !pDstFilename)
   {
      texConvError("Must specify source/destination filenames.\n");
      printHelp();
      return EXIT_FAILURE;
   }
   
   if (!doesFileExist(pSrcFilename))
   {
      printf("Unable to open file \"%s\"!\n", pSrcFilename);
      return EXIT_FAILURE;
   }
         
   if (!convertTexture(
      pSrcFilename, 
      pDstFilename, 
      packParams,
      retrieveDDXSourceImage,
      alphaMode,
      minMipDimension,
      desiredMipChainLevels,
      autoDataFormat))
   {
      texConvError("FAILED to convert \"%s\" to \"%s\"!\n", pSrcFilename, pDstFilename);
      return EXIT_FAILURE;
   }
   else
   {
      printf("Successfully converted \"%s\" to \"%s\".\n", pSrcFilename, pDstFilename);
   }
#endif



   

   destroyD3DREF();   
   
   g_DDXDLL.deinit();
      
   return EXIT_SUCCESS;
}


