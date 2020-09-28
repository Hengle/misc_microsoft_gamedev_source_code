//============================================================================
//
//  deviceStateDumper.cpp
//  
//  Copyright (c) 2004-2006, Ensemble Studios
//
//============================================================================
#include "xrender.h"
#include "deviceStateDumper.h"
#include "BD3D.h"

#ifndef BUILD_FINAL

// disable macro redefinition warning
#pragma warning(disable:4005) 

//---------------------------------------------------------------------------------------------------
// Globals
//---------------------------------------------------------------------------------------------------
int BD3DDeviceStateDumper::mCurIndent;

//---------------------------------------------------------------------------------------------------
// Helpers
//---------------------------------------------------------------------------------------------------

struct BStateType
{
   DWORD value;
   char* pDesc;
};

//---------------------------------------------------------------------------------------------------

#define STATE_TYPE_DWORD_INT (BStateType*)(-1)
#define STATE_TYPE_DWORD (BStateType*)(STATE_TYPE_DWORD_INT)

#define STATE_TYPE_FLOAT_INT (BStateType*)(-2)
#define STATE_TYPE_FLOAT (BStateType*)(STATE_TYPE_FLOAT_INT)

#define STATE_TYPE_BOOL_INT (BStateType*)(-3)
#define STATE_TYPE_BOOL (BStateType*)(STATE_TYPE_BOOL_INT)

//---------------------------------------------------------------------------------------------------

struct BStateDesc
{
   DWORD value;
   char* pName;
   BStateType* pType;
};

//---------------------------------------------------------------------------------------------------

#define Q(s) { s, #s },
#define E {0, NULL }

static BStateType gD3DZBUFFERTYPE[] = 
{
   Q(D3DZB_FALSE)
   Q(D3DZB_TRUE)
#ifndef XBOX   
   Q(D3DZB_USEW)
#endif   
   E
};

static BStateType gD3DFILLMODE[] = 
{ 
   Q(D3DFILL_POINT             ) 
   Q(D3DFILL_WIREFRAME         )  
   Q(D3DFILL_SOLID             )
   E
};

#ifndef XBOX
static BStateType gD3DSHADEMODE[] =  
{ 
   Q(D3DSHADE_FLAT               )
   Q(D3DSHADE_GOURAUD            )
   Q(D3DSHADE_PHONG              )
   E
};
#endif

static BStateType gD3DBLEND[] = 
{ 
   Q(   D3DBLEND_ZERO              )
   Q(   D3DBLEND_ONE               ) 
   Q(   D3DBLEND_SRCCOLOR          ) 
   Q(   D3DBLEND_INVSRCCOLOR       ) 
   Q(   D3DBLEND_SRCALPHA          ) 
   Q(   D3DBLEND_INVSRCALPHA       ) 
   Q(   D3DBLEND_DESTALPHA         ) 
   Q(   D3DBLEND_INVDESTALPHA      ) 
   Q(   D3DBLEND_DESTCOLOR         ) 
   Q(   D3DBLEND_INVDESTCOLOR      ) 
   Q(   D3DBLEND_SRCALPHASAT       ) 
#ifndef XBOX   
   Q(   D3DBLEND_BOTHSRCALPHA      ) 
   Q(   D3DBLEND_BOTHINVSRCALPHA   ) 
#endif   
   Q(   D3DBLEND_BLENDFACTOR       ) 
   Q(   D3DBLEND_INVBLENDFACTOR    ) 
   E
};

static BStateType gD3DCULL[] = 
{ 
   Q(      D3DCULL_NONE          )    
   Q(D3DCULL_CW                  )
   Q(D3DCULL_CCW                 )
   E
};

static BStateType gD3DCMPFUNC[]= 
{ 
   Q(D3DCMP_NEVER                )
   Q(D3DCMP_LESS                 )
   Q(D3DCMP_EQUAL                )
   Q(D3DCMP_LESSEQUAL            )
   Q(D3DCMP_GREATER              )
   Q(D3DCMP_NOTEQUAL             )
   Q(D3DCMP_GREATEREQUAL         )
   Q(D3DCMP_ALWAYS               )
   E
};

static BStateType gD3DSTENCILOP[]= 
{ 
   Q(   D3DSTENCILOP_KEEP        )   
   Q(D3DSTENCILOP_ZERO           )
   Q(D3DSTENCILOP_REPLACE        )
   Q(D3DSTENCILOP_INCRSAT        )
   Q(D3DSTENCILOP_DECRSAT        )
   Q(D3DSTENCILOP_INVERT         )
   Q(D3DSTENCILOP_INCR           )
   Q(D3DSTENCILOP_DECR           )
   E
};

static BStateType gD3DBLENDOP[]= 
{ 
   Q(D3DBLENDOP_ADD              ) 
   Q(D3DBLENDOP_SUBTRACT         )
   Q(D3DBLENDOP_REVSUBTRACT      )
   Q(D3DBLENDOP_MIN              )
   Q(D3DBLENDOP_MAX              )
   E
};

static BStateType gD3DTEXTUREOP[]=
{
   Q(D3DTOP_DISABLE              )
   Q(D3DTOP_SELECTARG1           )
   Q(D3DTOP_SELECTARG2           )
   Q(D3DTOP_MODULATE             )
   Q(D3DTOP_MODULATE2X           )
   Q(D3DTOP_MODULATE4X           )
   Q(D3DTOP_ADD                  )
   Q(D3DTOP_ADDSIGNED            )
   Q(D3DTOP_ADDSIGNED2X          )
   Q(D3DTOP_SUBTRACT             )
   Q(D3DTOP_ADDSMOOTH            )
   Q(D3DTOP_BLENDDIFFUSEALPHA    )
   Q(D3DTOP_BLENDTEXTUREALPHA    )
   Q(D3DTOP_BLENDFACTORALPHA     )
   Q(D3DTOP_BLENDTEXTUREALPHAPM  )
   Q(D3DTOP_BLENDCURRENTALPHA    )
   Q(D3DTOP_PREMODULATE            )
   Q(D3DTOP_MODULATEALPHA_ADDCOLOR )
   Q(D3DTOP_MODULATECOLOR_ADDALPHA )
   Q(D3DTOP_MODULATEINVALPHA_ADDCOLOR )
   Q(D3DTOP_MODULATEINVCOLOR_ADDALPHA )
   Q(D3DTOP_BUMPENVMAP                )
   Q(D3DTOP_BUMPENVMAPLUMINANCE       )
   Q(D3DTOP_DOTPRODUCT3               )
   Q(D3DTOP_MULTIPLYADD               )
   Q(D3DTOP_LERP                      )
   E
};

static BStateType gD3DTEXTURETRANSFORMFLAGS[] =
{
      Q(D3DTTFF_DISABLE      )
   Q(D3DTTFF_COUNT1          )
   Q(D3DTTFF_COUNT2          )
   Q(D3DTTFF_COUNT3          )
   Q(D3DTTFF_COUNT4          )
   Q(D3DTTFF_PROJECTED       )
   E
};
   
static BStateType gD3DTEXTUREADDRESS[]=
{
   Q(D3DTADDRESS_WRAP            )
   Q(D3DTADDRESS_MIRROR          )
   Q(D3DTADDRESS_CLAMP           )
   Q(D3DTADDRESS_BORDER          )
   Q(D3DTADDRESS_MIRRORONCE      )
   E
};
   
static BStateType gD3DTEXTUREFILTER[] =
{
   Q(D3DTEXF_NONE            )
   Q(D3DTEXF_POINT           )
   Q(D3DTEXF_LINEAR          )
   Q(D3DTEXF_ANISOTROPIC     )
#ifndef XBOX   
   Q(D3DTEXF_PYRAMIDALQUAD   )
   Q(D3DTEXF_GAUSSIANQUAD    )
#endif   
   E
};

static BStateType gD3DTA[] =
{
   Q(D3DTA_DIFFUSE           )
   Q(D3DTA_CURRENT           )
   Q(D3DTA_TEXTURE           )
   Q(D3DTA_TFACTOR           )
   Q(D3DTA_SPECULAR          )
   Q(D3DTA_TEMP              )
   Q(D3DTA_CONSTANT          )
   Q(D3DTA_COMPLEMENT        )
   Q(D3DTA_ALPHAREPLICATE    )
   E
};

#undef Q
#undef E

//---------------------------------------------------------------------------------------------------

#define Q(s) s, #s

static BStateDesc gTextureBStateDesc[] =
{
      {Q(D3DTSS_COLOROP           ),gD3DTEXTUREOP},
      {Q(D3DTSS_COLORARG1         ),gD3DTA},
      {Q(D3DTSS_COLORARG2         ),gD3DTA},
      {Q(D3DTSS_ALPHAOP           ),gD3DTEXTUREOP },
      {Q(D3DTSS_ALPHAARG1         ),gD3DTA },
      {Q(D3DTSS_ALPHAARG2         ),gD3DTA },
      {Q(D3DTSS_BUMPENVMAT00      ),STATE_TYPE_FLOAT},
      {Q(D3DTSS_BUMPENVMAT01      ),STATE_TYPE_FLOAT},
      {Q(D3DTSS_BUMPENVMAT10      ),STATE_TYPE_FLOAT},
      {Q(D3DTSS_BUMPENVMAT11      ),STATE_TYPE_FLOAT},
      {Q(D3DTSS_TEXCOORDINDEX     ),STATE_TYPE_DWORD},
      {Q(D3DTSS_BUMPENVLSCALE     ),STATE_TYPE_FLOAT},
      {Q(D3DTSS_BUMPENVLOFFSET    ),STATE_TYPE_FLOAT},
      {Q(D3DTSS_TEXTURETRANSFORMFLAGS), gD3DTEXTURETRANSFORMFLAGS},
      {Q(D3DTSS_COLORARG0         ),gD3DTA },
      {Q(D3DTSS_ALPHAARG0         ),gD3DTA },
      {Q(D3DTSS_RESULTARG         ),gD3DTA },
      {Q(D3DTSS_CONSTANT          ), STATE_TYPE_DWORD},
};

static const int NumTextureBStateDesc = sizeof(gTextureBStateDesc)/sizeof(gTextureBStateDesc[0]);

static BStateDesc gSamplerBStateDesc[] =
{
   {Q(D3DSAMP_ADDRESSU       ),gD3DTEXTUREADDRESS},
   {Q(D3DSAMP_ADDRESSV       ),gD3DTEXTUREADDRESS},
   {Q(D3DSAMP_ADDRESSW       ),gD3DTEXTUREADDRESS},
   {Q(D3DSAMP_BORDERCOLOR    ),STATE_TYPE_DWORD},
   {Q(D3DSAMP_MAGFILTER      ),gD3DTEXTUREFILTER},
   {Q(D3DSAMP_MINFILTER      ),gD3DTEXTUREFILTER},
   {Q(D3DSAMP_MIPFILTER      ),gD3DTEXTUREFILTER},
   {Q(D3DSAMP_MIPMAPLODBIAS  ),STATE_TYPE_FLOAT},
   {Q(D3DSAMP_MAXMIPLEVEL    ),STATE_TYPE_DWORD},
   {Q(D3DSAMP_MAXANISOTROPY  ),STATE_TYPE_DWORD},
#ifndef XBOX   
   {Q(D3DSAMP_SRGBTEXTURE    ),STATE_TYPE_BOOL},
   {Q(D3DSAMP_ELEMENTINDEX   ),STATE_TYPE_DWORD},
   {Q(D3DSAMP_DMAPOFFSET     ),STATE_TYPE_DWORD},
#endif   
};
const int NumSamplerBStateDesc = sizeof(gSamplerBStateDesc)/sizeof(gSamplerBStateDesc[0]);

static BStateDesc gRenderBStateDesc[] =
{
   { Q(D3DRS_ZENABLE                ) ,gD3DZBUFFERTYPE },
   { Q(D3DRS_FILLMODE               ) ,gD3DFILLMODE },
   { Q(D3DRS_ZWRITEENABLE           ) ,STATE_TYPE_BOOL},
   { Q(D3DRS_ALPHATESTENABLE        ) ,STATE_TYPE_BOOL},
   { Q(D3DRS_SRCBLEND               ) ,gD3DBLEND },
   { Q(D3DRS_DESTBLEND              ) ,gD3DBLEND },
   { Q(    D3DRS_CULLMODE           )       ,gD3DCULL },
   { Q(    D3DRS_ZFUNC              )       ,gD3DCMPFUNC },
   { Q(    D3DRS_ALPHAREF           )       ,STATE_TYPE_DWORD },
   { Q(    D3DRS_ALPHAFUNC          )       ,gD3DCMPFUNC },
//   { Q(    D3DRS_DITHERENABLE       )       ,STATE_TYPE_BOOL},
   { Q(    D3DRS_ALPHABLENDENABLE   )       ,STATE_TYPE_BOOL},
   { Q(    D3DRS_STENCILENABLE      )       ,STATE_TYPE_BOOL},
   { Q(    D3DRS_STENCILFAIL        )       ,gD3DSTENCILOP},
   { Q(    D3DRS_STENCILZFAIL       )       ,gD3DSTENCILOP},
   { Q(    D3DRS_STENCILPASS        )       ,gD3DSTENCILOP},
   { Q(    D3DRS_STENCILFUNC        )       ,gD3DCMPFUNC},
   { Q(    D3DRS_STENCILREF         )       ,STATE_TYPE_DWORD},
   { Q(    D3DRS_STENCILMASK        )       ,STATE_TYPE_DWORD},
   { Q(    D3DRS_STENCILWRITEMASK   )       ,STATE_TYPE_DWORD},

   { Q(    D3DRS_WRAP0              )       ,STATE_TYPE_BOOL},
   { Q(    D3DRS_WRAP1              )       ,STATE_TYPE_BOOL},
   { Q(    D3DRS_WRAP2              )       ,STATE_TYPE_BOOL},
   { Q(    D3DRS_WRAP3              )       ,STATE_TYPE_BOOL},
   { Q(    D3DRS_WRAP4              )       ,STATE_TYPE_BOOL},
   { Q(    D3DRS_WRAP5              )       ,STATE_TYPE_BOOL},
   { Q(    D3DRS_WRAP6              )       ,STATE_TYPE_BOOL},
   { Q(    D3DRS_WRAP7              )       ,STATE_TYPE_BOOL},
   { Q(    D3DRS_CLIPPLANEENABLE         )  ,STATE_TYPE_BOOL},
   { Q(    D3DRS_POINTSIZE               )  ,STATE_TYPE_FLOAT},
   { Q(    D3DRS_POINTSIZE_MIN           )  ,STATE_TYPE_FLOAT},
   { Q(    D3DRS_POINTSPRITEENABLE       )  ,STATE_TYPE_BOOL},
   { Q(    D3DRS_MULTISAMPLEANTIALIAS    )  ,STATE_TYPE_BOOL},
   { Q(    D3DRS_MULTISAMPLEMASK         )  ,STATE_TYPE_DWORD},
//   { Q(    D3DRS_PATCHEDGESTYLE          )  ,STATE_TYPE_DWORD},
//      { Q(    D3DRS_DEBUGMONITORTOKEN       )  ,STATE_TYPE_DWORD},
   { Q(    D3DRS_POINTSIZE_MAX           )  ,STATE_TYPE_FLOAT},
   { Q(    D3DRS_BLENDOP                 )  ,gD3DBLENDOP},
//   { Q(    D3DRS_POSITIONDEGREE          )  ,STATE_TYPE_DWORD},
//   { Q(    D3DRS_NORMALDEGREE            )  ,STATE_TYPE_DWORD},
   { Q(    D3DRS_SCISSORTESTENABLE       )  ,STATE_TYPE_BOOL},
   { Q(    D3DRS_SLOPESCALEDEPTHBIAS     )  ,STATE_TYPE_FLOAT},
//   { Q(    D3DRS_ANTIALIASEDLINEENABLE   )  ,STATE_TYPE_BOOL},
   { Q(    D3DRS_MINTESSELLATIONLEVEL    )  ,STATE_TYPE_DWORD},
   { Q(    D3DRS_MAXTESSELLATIONLEVEL    )  ,STATE_TYPE_DWORD},
//   { Q(    D3DRS_ADAPTIVETESS_X          )  ,STATE_TYPE_DWORD},
//   { Q(    D3DRS_ADAPTIVETESS_Y          )  ,STATE_TYPE_DWORD},
//   { Q(    D3DRS_ADAPTIVETESS_Z          )  ,STATE_TYPE_DWORD},
//   { Q(    D3DRS_ADAPTIVETESS_W          )  ,STATE_TYPE_DWORD},
//   { Q(    D3DRS_ENABLEADAPTIVETESSELLATION) ,STATE_TYPE_BOOL},
   { Q(    D3DRS_TWOSIDEDSTENCILMODE       ),STATE_TYPE_BOOL},
   { Q(    D3DRS_CCW_STENCILFAIL           ),gD3DSTENCILOP},
   { Q(    D3DRS_CCW_STENCILZFAIL          ),gD3DSTENCILOP},
   { Q(    D3DRS_CCW_STENCILPASS           ),gD3DSTENCILOP},
   { Q(    D3DRS_CCW_STENCILFUNC           ),gD3DCMPFUNC},
   { Q(    D3DRS_COLORWRITEENABLE1         ),STATE_TYPE_DWORD},
   { Q(    D3DRS_COLORWRITEENABLE2         ),STATE_TYPE_DWORD},
   { Q(    D3DRS_COLORWRITEENABLE3         ),STATE_TYPE_DWORD},
   { Q(    D3DRS_BLENDFACTOR               ),STATE_TYPE_DWORD},
//   { Q(    D3DRS_SRGBWRITEENABLE           ),STATE_TYPE_BOOL},
   { Q(    D3DRS_DEPTHBIAS                 ),STATE_TYPE_DWORD},
   { Q(    D3DRS_WRAP8                     ),STATE_TYPE_DWORD},
   { Q(    D3DRS_WRAP9                     ),STATE_TYPE_DWORD},
   { Q(    D3DRS_WRAP10                    ),STATE_TYPE_DWORD},
   { Q(    D3DRS_WRAP11                    ),STATE_TYPE_DWORD},
   { Q(    D3DRS_WRAP12                    ),STATE_TYPE_DWORD},
   { Q(    D3DRS_WRAP13                    ),STATE_TYPE_DWORD},
   { Q(    D3DRS_WRAP14                    ),STATE_TYPE_DWORD},
   { Q(    D3DRS_WRAP15                    ),STATE_TYPE_DWORD},
   { Q(    D3DRS_SEPARATEALPHABLENDENABLE  ),STATE_TYPE_BOOL},
   { Q(    D3DRS_SRCBLENDALPHA             ),gD3DBLEND },
   { Q(    D3DRS_DESTBLENDALPHA            ),gD3DBLEND },
   { Q(    D3DRS_BLENDOPALPHA              ),gD3DBLENDOP}, 

#ifndef XBOX   
   { Q(    D3DRS_SHADEMODE          )       ,gD3DSHADEMODE },
   { Q(    D3DRS_LASTPIXEL          )       ,STATE_TYPE_BOOL},
   { Q(    D3DRS_FOGENABLE          )       ,STATE_TYPE_BOOL},
   { Q(    D3DRS_SPECULARENABLE     )       ,STATE_TYPE_BOOL},
   { Q(    D3DRS_FOGCOLOR           )       ,STATE_TYPE_DWORD },
   { Q(    D3DRS_FOGTABLEMODE       )       ,STATE_TYPE_DWORD},
   { Q(    D3DRS_FOGSTART           )       ,STATE_TYPE_FLOAT},
   { Q(    D3DRS_FOGEND             )       ,STATE_TYPE_FLOAT},
   { Q(    D3DRS_FOGDENSITY         )       ,STATE_TYPE_FLOAT},
   { Q(    D3DRS_RANGEFOGENABLE     )       ,STATE_TYPE_BOOL},
   { Q(    D3DRS_TEXTUREFACTOR      )       ,STATE_TYPE_DWORD},
   { Q(    D3DRS_CLIPPING           )       ,STATE_TYPE_BOOL},
   { Q(    D3DRS_LIGHTING           )       ,STATE_TYPE_BOOL},
   { Q(    D3DRS_AMBIENT            )       ,STATE_TYPE_DWORD},
   { Q(    D3DRS_FOGVERTEXMODE      )       ,STATE_TYPE_DWORD},
   { Q(    D3DRS_COLORVERTEX        )       ,STATE_TYPE_DWORD},
   { Q(    D3DRS_LOCALVIEWER        )       ,STATE_TYPE_BOOL},
   { Q(    D3DRS_NORMALIZENORMALS   )       ,STATE_TYPE_BOOL},
   { Q(    D3DRS_DIFFUSEMATERIALSOURCE   )  ,STATE_TYPE_DWORD},
   { Q(    D3DRS_SPECULARMATERIALSOURCE  )  ,STATE_TYPE_DWORD},
   { Q(    D3DRS_AMBIENTMATERIALSOURCE   )  ,STATE_TYPE_DWORD},
   { Q(    D3DRS_EMISSIVEMATERIALSOURCE  )  ,STATE_TYPE_DWORD},
   { Q(    D3DRS_VERTEXBLEND             )  ,STATE_TYPE_DWORD},
   { Q(    D3DRS_POINTSCALEENABLE        )  ,STATE_TYPE_BOOL},
   { Q(    D3DRS_POINTSCALE_A            )  ,STATE_TYPE_FLOAT},
   { Q(    D3DRS_POINTSCALE_B            )  ,STATE_TYPE_FLOAT},
   { Q(    D3DRS_POINTSCALE_C            )  ,STATE_TYPE_FLOAT},
   { Q(    D3DRS_INDEXEDVERTEXBLENDENABLE)  ,STATE_TYPE_BOOL},
   { Q(    D3DRS_COLORWRITEENABLE        )  ,STATE_TYPE_DWORD},
   { Q(    D3DRS_TWEENFACTOR             )  ,STATE_TYPE_FLOAT},
#endif   

#ifdef XBOX   
   { Q(D3DRS_VIEWPORTENABLE), STATE_TYPE_BOOL },
   { Q(D3DRS_HIGHPRECISIONBLENDENABLE), STATE_TYPE_BOOL },
   { Q(D3DRS_HIGHPRECISIONBLENDENABLE1), STATE_TYPE_BOOL },
   { Q(D3DRS_HIGHPRECISIONBLENDENABLE2), STATE_TYPE_BOOL },
   { Q(D3DRS_HIGHPRECISIONBLENDENABLE3), STATE_TYPE_BOOL },
   { Q(D3DRS_HALFPIXELOFFSET), STATE_TYPE_BOOL },
   { Q(D3DRS_PRIMITIVERESETENABLE), STATE_TYPE_BOOL },
   { Q(D3DRS_PRIMITIVERESETINDEX), STATE_TYPE_DWORD },
   { Q(D3DRS_ALPHATOMASKENABLE), STATE_TYPE_BOOL },
   { Q(D3DRS_ALPHATOMASKOFFSETS), STATE_TYPE_DWORD },
#endif   
};

static const int NumRenderBStateDesc = sizeof(gRenderBStateDesc) / sizeof(gRenderBStateDesc[0]);

#undef Q

//---------------------------------------------------------------------------------------------------
// BD3DDeviceStateDumper::dumpStates
//---------------------------------------------------------------------------------------------------
template<typename GetFunc> 
void BD3DDeviceStateDumper::dumpStates(
   DWORD numGroups,
   const BStateDesc* pBStateDesc, 
   int numBStateDescs, 
   GetFunc getFunc)
{
   for (DWORD groupIter = 0; groupIter < numGroups; groupIter++)
   {
      if (numGroups)
      {
         print("---- %i:\n", groupIter);
         indent(1);
      }

      for (int i = 0; i < numBStateDescs; i++)
      {
         const DWORD curState = getFunc(groupIter, pBStateDesc[i].value);

         if (numGroups > 1)
            print("%s[%i] (%i, 0x%X): ", pBStateDesc[i].pName, groupIter, i, i);
         else
            print("%s (%i, 0x%X): ", pBStateDesc[i].pName, i, i);

         switch (reinterpret_cast<int>(pBStateDesc[i].pType))
         {
         case STATE_TYPE_DWORD_INT:
            {
               print("0x%X\n", curState);
               break;
            }
         case STATE_TYPE_FLOAT_INT:
            {
               print("%f\n", *reinterpret_cast<const float*>(&curState));
               break;
            }
         case STATE_TYPE_BOOL_INT:
            {
               if ((curState >= 0) && (curState <= 1))
                  print("%s\n", curState ? "TRUE" : "FALSE");
               else
                  print("INVALID BOOL: %i\n", curState);
               break;
            }
         default:
            {
               BStateType* pType = pBStateDesc[i].pType;

               while (pType->pDesc)
               {
                  if (pType->value == curState)
                  {
                     print("%s\n", pType->pDesc);
                     break;
                  }
                  pType++;
               }  

               if (!pType->pDesc)                         
                  print("UNRECOGNIZED: %i (0x%X)\n", curState, curState);

               break;
            }
         }
      }

      if (numGroups)
         indent(-1);
   }
}   

//---------------------------------------------------------------------------------------------------
// BD3DDeviceStateDumper::D3DErrCheck
//---------------------------------------------------------------------------------------------------
void BD3DDeviceStateDumper::D3DErrCheck(HRESULT hres)
{
   BVERIFYM(SUCCEEDED(hres), BFixedString256(cVarArg, "BD3DDeviceStateDumper::D3DErrCheck: A D3D call failed, HRES=0x%X", hres).c_str());
}

//---------------------------------------------------------------------------------------------------
// BD3DDeviceStateDumper::indent
//---------------------------------------------------------------------------------------------------
void BD3DDeviceStateDumper::indent(int i)
{
   mCurIndent += i;
}

//---------------------------------------------------------------------------------------------------
// BD3DDeviceStateDumper::print
//---------------------------------------------------------------------------------------------------
void BD3DDeviceStateDumper::print(const char* pFmt, ...)
{
   pFmt;
   
#ifndef BUILD_FINAL
   BFixedString<32768>* pBuf = new BFixedString<32768>();
         
   va_list args;
   va_start(args, pFmt);
   pBuf->formatArgs(pFmt, args);
   va_end(args);
   
   const char* pSrc = pBuf->getPtr();
   
   // Hack hack - shader 3.0 disassemblies can be huge!
   // rg [1/30/06] - Not thread safe
   static BFixedString<1024> curLine;   
   static int curLineOfs = 0;

   while (*pSrc)
   {
      char c = *pSrc++;
      if (c == '\r')
         continue;
         
      curLine[curLineOfs++] = c;                  
      
      if ((c == '\n') || (curLineOfs == (int)curLine.getBufSize() - 1))
      {
         curLine[curLineOfs] = '\0';
         
         for (int i = 0; i < mCurIndent; i++)
            tracenocrlf("   ");
         
         tracenocrlf("%s", curLine.c_str());      
         
         curLineOfs = 0;
      }
   }
   
   delete pBuf;
#endif

#if 0   
   if (curLineOfs)
   {
      for (int i = 0; i < mCurIndent; i++)
         tracenocrlf("   ");
            
      curLine[curLineOfs++] = '\0';
      tracenocrlf("%s", curLine.c_str());      
   }
#endif   
}

//---------------------------------------------------------------------------------------------------
// BD3DDeviceStateDumper::getRenderStateUncachedUntyped
//---------------------------------------------------------------------------------------------------
DWORD BD3DDeviceStateDumper::getRenderStateUncachedUntyped(DWORD s, DWORD t) 
{  
   s; 
   DWORD value;
   BD3D::mpDev->GetRenderState(static_cast<D3DRENDERSTATETYPE>(t), &value); 
   return value;
}
 
//---------------------------------------------------------------------------------------------------
// BD3DDeviceStateDumper::getSamplerStateUncachedUntyped
//---------------------------------------------------------------------------------------------------
DWORD BD3DDeviceStateDumper::getSamplerStateUncachedUntyped(DWORD s, DWORD t) 
{ 
   DWORD value;
   BD3D::mpDev->GetSamplerState(s, static_cast<D3DSAMPLERSTATETYPE>(t), &value); 
   return value;
}

//---------------------------------------------------------------------------------------------------
// BD3DDeviceStateDumper::dumpRenderStates
//---------------------------------------------------------------------------------------------------
void BD3DDeviceStateDumper::dumpRenderStates(void)
{
   print("----------------------- Render States:\n\n");
   dumpStates(1, gRenderBStateDesc, NumRenderBStateDesc, getRenderStateUncachedUntyped);
}

//---------------------------------------------------------------------------------------------------
// BD3DDeviceStateDumper::dumpSamplerStates
//---------------------------------------------------------------------------------------------------
void BD3DDeviceStateDumper::dumpSamplerStates(void)
{
   print("----------------------- Sampler States:\n\n");
   dumpStates(NumSamplers, gSamplerBStateDesc, NumSamplerBStateDesc, getSamplerStateUncachedUntyped);
}

//---------------------------------------------------------------------------------------------------
// BD3DDeviceStateDumper::dumpVShaderConstants
//---------------------------------------------------------------------------------------------------
void BD3DDeviceStateDumper::dumpVShaderConstants(void)
{
   const int MaxVertexShaderConstants = 256;
         
   print("----------------------- Non-Zero Vertex Shader Constants:\n\n");
   
   print("Floats:\n");
   
   indent(1);
   int numZeroRegs = 0;
   for (int i = 0; i < MaxVertexShaderConstants; i++)
   {
      float v[4]={0,0,0,0};
      IDirect3DDevice9 *pDevice = BD3D::mpDev;
      if (pDevice)
      {
         pDevice->GetVertexShaderConstantF(i, v, 1);
         if ((v[0] == 0.0f) && (v[1] == 0.0f) && (v[2] == 0.0f) && (v[3] == 0.0f))
            numZeroRegs++;
         else
            print("[%i]: %f %f %f %f\n", i, v[0], v[1], v[2], v[3]);
      }
   }
   print("Num all-zero regs: %i\n", numZeroRegs);
   indent(-1);
}
//---------------------------------------------------------------------------------------------------
// BD3DDeviceStateDumper::dumpPShaderConstants
//---------------------------------------------------------------------------------------------------
void BD3DDeviceStateDumper::dumpPShaderConstants(void)
{
   const int MaxPixelShaderConstants = 256;   
   
   print("----------------------- Non-Zero Pixel Shader Constants:\n\n");

   print("Floats:\n");
   
   indent(1);
   int numZeroRegs = 0;
   for (int i = 0; i < MaxPixelShaderConstants; i++)
   {
      float v[4] = {0,0,0,0};
      IDirect3DDevice9 *pDevice = BD3D::mpDev;
      if (pDevice)
      {
         pDevice->GetPixelShaderConstantF(i, v, 1);
         if ((v[0] == 0.0f) && (v[1] == 0.0f) && (v[2] == 0.0f) && (v[3] == 0.0f))
            numZeroRegs++;
         else
            print("[%i]: %f %f %f %f\n", i, v[0], v[1], v[2], v[3]);
      }
   }
   print("Num all-zero regs: %i\n", numZeroRegs);
   indent(-1);
}

//---------------------------------------------------------------------------------------------------
// BD3DDeviceStateDumper::dumpVShaderConstants
//---------------------------------------------------------------------------------------------------
void BD3DDeviceStateDumper::dumpVShaderBoolConstants(void)
{
   const int MaxVertexShaderConstants = 16;

   print("----------------------- Vertex Shader Boolean Constants:\n\n");

   print("Bools:\n");

   indent(1);
   for (int i = 0; i < MaxVertexShaderConstants; i++)
   {
      BOOL v[4];
      IDirect3DDevice9 *pDevice = BD3D::mpDev;
      if (pDevice)
      {
         pDevice->GetVertexShaderConstantB(i, v, 1);
         print("[%i]: %i\n", i, v[0]);
      }
   }
   indent(-1);
}
//---------------------------------------------------------------------------------------------------
// BD3DDeviceStateDumper::dumpPShaderConstants
//---------------------------------------------------------------------------------------------------
void BD3DDeviceStateDumper::dumpPShaderBoolConstants(void)
{
   const int MaxPixelShaderConstants = 16;   

   print("----------------------- Pixel Shader Boolean Constants:\n\n");

   print("Bools:\n");

   indent(1);
   for (int i = 0; i < MaxPixelShaderConstants; i++)
   {
      BOOL v[4];
      IDirect3DDevice9 *pDevice = BD3D::mpDev;
      if (pDevice)
      {
         pDevice->GetPixelShaderConstantB(i, v, 1);
         print("[%i]: %i\n", i, v[0]);
      }
   }
   indent(-1);
}

//---------------------------------------------------------------------------------------------------
// BD3DDeviceStateDumper::disassembleShader
//---------------------------------------------------------------------------------------------------
void BD3DDeviceStateDumper::disassembleShader(const DWORD* pTokens)
{
   LPD3DXBUFFER pDisasmBuf = NULL;

   HRESULT hres = D3DXDisassembleShader(pTokens,
      FALSE,
      NULL,
      &pDisasmBuf);

   BFixedString<32768>* pBuffer = new BFixedString<32768>();
   if (SUCCEEDED(hres))
      pBuffer->append(reinterpret_cast<const char*>(pDisasmBuf->GetBufferPointer()), pDisasmBuf->GetBufferSize());

   print("%s\n", pBuffer->c_str());
   
   delete pBuffer;

   if (pDisasmBuf) pDisasmBuf->Release();
}

//---------------------------------------------------------------------------------------------------
// BD3DDeviceStateDumper::dumpVShader
//---------------------------------------------------------------------------------------------------
void BD3DDeviceStateDumper::dumpVShader(void)
{
   print("----------------------- Current Vertex Shader:\n\n");
   
   IDirect3DDevice9 *pDevice = BD3D::mpDev;
   IDirect3DVertexShader9* pShader;
   if (!pDevice)
      print("getDevice() failed!\n");
   
   pDevice->GetVertexShader(&pShader);
   
   print("IDirect3DVertexShader9 0x%X\n", pShader);
   if (pShader)
   {
      indent(1);
      
      UINT size = 0;
      pShader->GetFunction(NULL, &size);
      BDEBUG_ASSERT(size > 0);
      
      BDynamicArray<uchar> data(size);

      pShader->GetFunction(&data[0], &size);
      
      disassembleShader(reinterpret_cast<const DWORD*>(&data[0]));
                        
      pShader->Release();
      
      indent(-1);
   }            
}

//---------------------------------------------------------------------------------------------------
// BD3DDeviceStateDumper::dumpPShader
//---------------------------------------------------------------------------------------------------
void BD3DDeviceStateDumper::dumpPShader(void)
{
   print("----------------------- Current Pixel Shader:\n\n");
   IDirect3DDevice9 *pDevice = BD3D::mpDev;
   IDirect3DPixelShader9* pShader;
   if (!pDevice)
      print("getDevice() failed!\n");
   pDevice->GetPixelShader(&pShader);
   
   print("IDirect3DPixelShader9 0x%X\n", pShader);
   if (pShader)
   {
      indent(1);

      UINT size = 0;
      pShader->GetFunction(NULL, &size);
      BDEBUG_ASSERT(size > 0);

      BDynamicArray<uchar> data(size);

      pShader->GetFunction(&data[0], &size);

      disassembleShader( reinterpret_cast<const DWORD*>(&data[0]));

      pShader->Release();

      indent(-1);
   }            
}

//---------------------------------------------------------------------------------------------------
// BD3DDeviceStateDumper::getVertexDeclarationTypeSize
//---------------------------------------------------------------------------------------------------
int BD3DDeviceStateDumper::getVertexDeclarationTypeSize(DWORD type)
{
   int size = 0;
   switch (type)
   {
      case D3DDECLTYPE_FLOAT1    :  size = 4 * 1; break; // 1D float expanded to (value, 0., 0., 1.)
      case D3DDECLTYPE_FLOAT2    :  size = 4 * 2; break; // 2D float expanded to (value, value, 0., 1.)
      case D3DDECLTYPE_FLOAT3    :  size = 4 * 3; break; // 3D float expanded to (value, value, value, 1.)
      case D3DDECLTYPE_FLOAT4    :  size = 4 * 4; break; // 4D float
      case D3DDECLTYPE_D3DCOLOR  :  size = 4; break; // 4D packed unsigned bytes mapped to 0. to 1. range Input is in D3DCOLOR format (ARGB) expanded to (R, G, B, A)
      case D3DDECLTYPE_UBYTE4    :  size = 4; break; // 4D unsigned byte
      case D3DDECLTYPE_SHORT2    :  size = 4; break; // 2D signed short expanded to (value, value, 0., 1.)
      case D3DDECLTYPE_SHORT4    :  size = 8; break; // 4D signed short
      case D3DDECLTYPE_UBYTE4N   :  size = 4; break; // Each of 4 bytes is normalized by dividing to 255.0
      case D3DDECLTYPE_SHORT2N   :  size = 4; break; // 2D signed short normalized (v[0]/32767.0,v[1]/32767.0,0,1)
      case D3DDECLTYPE_SHORT4N   :  size = 8; break; // 4D signed short normalized (v[0]/32767.0,v[1]/32767.0,v[2]/32767.0,v[3]/32767.0)
      case D3DDECLTYPE_USHORT2N  :  size = 4; break; // 2D unsigned short normalized (v[0]/65535.0,v[1]/65535.0,0,1)
      case D3DDECLTYPE_USHORT4N  :  size = 8; break; // 4D unsigned short normalized (v[0]/65535.0,v[1]/65535.0,v[2]/65535.0,v[3]/65535.0)
      case D3DDECLTYPE_UDEC3     :  size = 4; break; // 3D unsigned 10 10 10 format expanded to (value, value, value, 1)
      case D3DDECLTYPE_DEC3N     :  size = 4; break; // 3D signed 10 10 10 format normalized and expanded to (v[0]/511.0, v[1]/511.0, v[2]/511.0, 1)
      case D3DDECLTYPE_FLOAT16_2 :  size = 4; break; // Two 16-bit floating point values, expanded to (value, value, 0, 1)
      case D3DDECLTYPE_FLOAT16_4 :  size = 8; break; // Four 16-bit floating point values
   }
   return size;
}

//---------------------------------------------------------------------------------------------------
// BD3DDeviceStateDumper::getVertexDeclarationTypeName
//---------------------------------------------------------------------------------------------------
const char* BD3DDeviceStateDumper::getVertexDeclarationTypeName(DWORD type)
{
   switch (type)
   {
      case D3DDECLTYPE_FLOAT1    :  return "FLOAT1";    break; // 1D float expanded to (value, 0., 0., 1.)
      case D3DDECLTYPE_FLOAT2    :  return "FLOAT2";    break; // 2D float expanded to (value, value, 0., 1.)
      case D3DDECLTYPE_FLOAT3    :  return "FLOAT3";    break; // 3D float expanded to (value, value, value, 1.)
      case D3DDECLTYPE_FLOAT4    :  return "FLOAT4";    break; // 4D float
      case D3DDECLTYPE_D3DCOLOR  :  return "D3DCOLOR";  break; // 4D packed unsigned bytes mapped to 0. to 1. range Input is in D3DCOLOR format (ARGB) expanded to (R, G, B, A)
      case D3DDECLTYPE_UBYTE4    :  return "UBYTE4";    break; // 4D unsigned byte
      case D3DDECLTYPE_SHORT2    :  return "SHORT2";    break; // 2D signed short expanded to (value, value, 0., 1.)
      case D3DDECLTYPE_SHORT4    :  return "SHORT4";    break; // 4D signed short
      case D3DDECLTYPE_UBYTE4N   :  return "UBYTE4N";   break; // Each of 4 bytes is normalized by dividing to 255.0
      case D3DDECLTYPE_SHORT2N   :  return "SHORT2N";   break; // 2D signed short normalized (v[0]/32767.0,v[1]/32767.0,0,1)
      case D3DDECLTYPE_SHORT4N   :  return "SHORT4N";   break; // 4D signed short normalized (v[0]/32767.0,v[1]/32767.0,v[2]/32767.0,v[3]/32767.0)
      case D3DDECLTYPE_USHORT2N  :  return "USHORT2N";  break; // 2D unsigned short normalized (v[0]/65535.0,v[1]/65535.0,0,1)
      case D3DDECLTYPE_USHORT4N  :  return "USHORT4N";  break; // 4D unsigned short normalized (v[0]/65535.0,v[1]/65535.0,v[2]/65535.0,v[3]/65535.0)
      case D3DDECLTYPE_UDEC3     :  return "UDEC3";     break; // 3D unsigned 10 10 10 format expanded to (value, value, value, 1)
      case D3DDECLTYPE_DEC3N     :  return "DEC3N";     break; // 3D signed 10 10 10 format normalized and expanded to (v[0]/511.0, v[1]/511.0, v[2]/511.0, 1)
      case D3DDECLTYPE_FLOAT16_2 :  return "FLOAT16_2"; break; // Two 16-bit floating point values, expanded to (value, value, 0, 1)
      case D3DDECLTYPE_FLOAT16_4 :  return "FLOAT16_4"; break; // Four 16-bit floating point values
   }
   
   return "??";
}

//---------------------------------------------------------------------------------------------------
// BD3DDeviceStateDumper::getVertexDeclarationUsageName
//---------------------------------------------------------------------------------------------------
const char* BD3DDeviceStateDumper::getVertexDeclarationUsageName(DWORD usage)
{
   switch (usage)
   {
      case D3DDECLUSAGE_POSITION          : return "POSITION";
      case D3DDECLUSAGE_BLENDWEIGHT       : return "BLENDWEIGHT";
      case D3DDECLUSAGE_BLENDINDICES      : return "BLENDINDICES";
      case D3DDECLUSAGE_NORMAL            : return "NORMAL";
      case D3DDECLUSAGE_PSIZE             : return "PSIZE";
      case D3DDECLUSAGE_TEXCOORD          : return "TEXCOORD";
      case D3DDECLUSAGE_TANGENT           : return "TANGENT";
      case D3DDECLUSAGE_BINORMAL          : return "BINORMAL";
      case D3DDECLUSAGE_TESSFACTOR        : return "TESSFACTOR";
#ifndef XBOX      
      case D3DDECLUSAGE_POSITIONT         : return "POSITIONT";
#endif      
      case D3DDECLUSAGE_COLOR             : return "COLOR";
      case D3DDECLUSAGE_FOG               : return "FOG";
      case D3DDECLUSAGE_DEPTH             : return "DEPTH";
      case D3DDECLUSAGE_SAMPLE            : return "SAMPLE";
   }
   return "??";
}

//---------------------------------------------------------------------------------------------------
// BD3DDeviceStateDumper::dumpVertexDeclaration
//---------------------------------------------------------------------------------------------------
void BD3DDeviceStateDumper::dumpVertexDeclaration(D3DVERTEXELEMENT9* pElements)
{
   print("dumpVertexDeclaration:\n");
   indent(1);
   
   int numElements = 0;
   int totalSize = 0;
   
// rg [2/10/06] - Big old hack here. D3D sometimes gives us a bad pointer, not sure why yet.
__try
{
   
   while (0xFF != pElements->Stream)
   {
      const char* pTypeName = getVertexDeclarationTypeName(pElements->Type);
      const char* pUsageName = getVertexDeclarationUsageName(pElements->Usage);
      const int size = getVertexDeclarationTypeSize(pElements->Type);

      print("  Stream: %i, Ofs: %02i, Typ: %s, Len: %i, Mthd: %i, Usage: %s, UsgIdx: %i\n",
         pElements->Stream,
         pElements->Offset,
         pTypeName,
         size,
         pElements->Method,
         pUsageName, 
         pElements->UsageIndex);

      totalSize += size;

      pElements++;
      
      // Bail if we're off in the woods.
      numElements++;
      if (numElements == 32)
         break;
   }
}
__except(EXCEPTION_EXECUTE_HANDLER)
{
   printf("  Vertex decl dump failed");
}

   print("  Total size: %i\n", totalSize);
   indent(-1);
}

//---------------------------------------------------------------------------------------------------
// BD3DDeviceStateDumper::dumpVertexDecl
//---------------------------------------------------------------------------------------------------
void BD3DDeviceStateDumper::dumpVertexDecl(void)
{
   print("----------------------- Current Vertex Declaration:\n\n");
   
   IDirect3DDevice9 *pDevice = BD3D::mpDev;
   IDirect3DVertexDeclaration9* pDecl;
   if (!pDevice)
      print("getDevice() failed!\n");
   
   pDevice->GetVertexDeclaration(&pDecl);
            
   if (!pDecl)
   {
      print("NULL\n");
   }
   else
   {
      const uint cMaxElements = 256;
      D3DVERTEXELEMENT9 elements[cMaxElements];
      UINT numElements;
               
      pDecl->GetDeclaration(elements, &numElements);
      
      BASSERT(numElements < cMaxElements);
      
      indent(1);
      dumpVertexDeclaration( elements);
      indent(-1);
            
      pDecl->Release();
   }

#if 0   
   DWORD fvf;
   if (FAILED(BD3D::mpDev->GetFVF(&fvf)))
      print("GetFVF() failed!\n");
   else
      print("Current FVF: 0x%X\n", fvf);
#endif      
}

//---------------------------------------------------------------------------------------------------
// BD3DDeviceStateDumper::getFormatString
//---------------------------------------------------------------------------------------------------
#define Q(s) #s

const char* BD3DDeviceStateDumper::getFormatString(D3DFORMAT format)
{
   switch (format)
   {
         case D3DFMT_UNKNOWN: return "D3DFMT_UNKNOWN";
#ifndef XBOX         
         case D3DFMT_R8G8B8: return Q(D3DFMT_R8G8B8);
#endif         
         case D3DFMT_X8R8G8B8: return Q(D3DFMT_X8R8G8B8);
         case D3DFMT_A8R8G8B8: return Q(D3DFMT_A8R8G8B8);
         case D3DFMT_R5G6B5: return Q(D3DFMT_R5G6B5);
         case D3DFMT_X1R5G5B5: return Q(D3DFMT_X1R5G5B5);
         case D3DFMT_A1R5G5B5: return Q(D3DFMT_A1R5G5B5);
         case D3DFMT_A4R4G4B4: return Q(D3DFMT_A4R4G4B4);
#ifndef XBOX         
         case D3DFMT_R3G3B2: return Q(D3DFMT_R3G3B2);
#endif         
         case D3DFMT_A8: return Q(D3DFMT_A8);
#ifndef XBOX         
         case D3DFMT_A8R3G3B2: return Q(D3DFMT_A8R3G3B2);
#endif         
         case D3DFMT_X4R4G4B4: return Q(D3DFMT_X4R4G4B4);
         case D3DFMT_A2B10G10R10: return Q(D3DFMT_A2B10G10R10);
         case D3DFMT_A8B8G8R8: return Q(D3DFMT_A8B8G8R8);
         case D3DFMT_X8B8G8R8: return Q(D3DFMT_X8B8G8R8);
         case D3DFMT_G16R16: return Q(D3DFMT_G16R16);
         case D3DFMT_A2R10G10B10: return Q(D3DFMT_A2R10G10B10);
         case D3DFMT_A16B16G16R16: return Q(D3DFMT_A16B16G16R16);
#ifndef XBOX         
         case D3DFMT_A8P8: return Q(D3DFMT_A8P8);
         case D3DFMT_P8: return Q(D3DFMT_P8);
#endif                  
         case D3DFMT_L8: return Q(D3DFMT_L8);
         case D3DFMT_A8L8: return Q(D3DFMT_A8L8);
#ifndef XBOX         
         case D3DFMT_A4L4: return Q(D3DFMT_A4L4);
#endif         
         case D3DFMT_V8U8: return Q(D3DFMT_V8U8);
         case D3DFMT_L6V5U5: return Q(D3DFMT_L6V5U5);
         case D3DFMT_X8L8V8U8: return Q(D3DFMT_X8L8V8U8);
         case D3DFMT_Q8W8V8U8: return Q(D3DFMT_Q8W8V8U8);
         case D3DFMT_V16U16: return Q(D3DFMT_V16U16);
         case D3DFMT_A2W10V10U10: return Q(D3DFMT_A2W10V10U10);
         case D3DFMT_UYVY: return Q(D3DFMT_UYVY);
         case D3DFMT_R8G8_B8G8: return Q(D3DFMT_R8G8_B8G8);
         case D3DFMT_YUY2: return Q(D3DFMT_YUY2);
         case D3DFMT_G8R8_G8B8: return Q(D3DFMT_G8R8_G8B8);
         case D3DFMT_DXT1: return Q(D3DFMT_DXT1);
         case D3DFMT_DXT2: return Q(D3DFMT_DXT2);
#ifndef XBOX                  
         case D3DFMT_DXT3: return Q(D3DFMT_DXT3);
         case D3DFMT_DXT4: return Q(D3DFMT_DXT4);
#endif         
         case D3DFMT_DXT5: return Q(D3DFMT_DXT5);
#ifndef XBOX         
         case D3DFMT_D16_LOCKABLE: return Q(D3DFMT_D16_LOCKABLE);
#endif         
         case D3DFMT_D32: return Q(D3DFMT_D32);
#ifndef XBOX         
         case D3DFMT_D15S1: return Q(D3DFMT_D15S1);
#endif         
         case D3DFMT_D24S8: return Q(D3DFMT_D24S8);
         case D3DFMT_D24X8: return Q(D3DFMT_D24X8);
#ifndef XBOX         
         case D3DFMT_D24X4S4: return Q(D3DFMT_D24X4S4);
#endif         
         case D3DFMT_D16: return Q(D3DFMT_D16);
#ifndef XBOX         
         case D3DFMT_D32F_LOCKABLE: return Q(D3DFMT_D32F_LOCKABLE);
#endif         
         case D3DFMT_D24FS8: return Q(D3DFMT_D24FS8);
         case D3DFMT_L16: return Q(D3DFMT_L16);
         case D3DFMT_VERTEXDATA: return Q(D3DFMT_VERTEXDATA);
         case D3DFMT_INDEX16: return Q(D3DFMT_INDEX16);
         case D3DFMT_INDEX32: return Q(D3DFMT_INDEX32);
         case D3DFMT_Q16W16V16U16: return Q(D3DFMT_Q16W16V16U16);
#ifndef XBOX         
         case D3DFMT_MULTI2_ARGB8: return Q(D3DFMT_MULTI2_ARGB8);
#endif         
         case D3DFMT_R16F: return Q(D3DFMT_R16F);
         case D3DFMT_G16R16F: return Q(D3DFMT_G16R16F);
         case D3DFMT_A16B16G16R16F: return Q(D3DFMT_A16B16G16R16F);
         case D3DFMT_R32F: return Q(D3DFMT_R32F);
         case D3DFMT_G32R32F: return Q(D3DFMT_G32R32F);
         case D3DFMT_A32B32G32R32F: return Q(D3DFMT_A32B32G32R32F);
#ifndef XBOX         
         case D3DFMT_CxV8U8: return Q(D3DFMT_CxV8U8);
#endif         
   }
   return "INVALID D3DFORMAT";
}

#undef Q

//---------------------------------------------------------------------------------------------------
// BD3DDeviceStateDumper::dumpSurfaceInfo
//---------------------------------------------------------------------------------------------------
void BD3DDeviceStateDumper::dumpSurfaceInfo(IDirect3DSurface9* pSurf)
{
   if (!pSurf)
      print("NULL\n");
   else
   {
      indent(1);
      
      print("IDirect3DSurface9 0x%X:\n", pSurf);
      
      D3DSURFACE_DESC desc;
      
      pSurf->GetDesc(&desc);
      
      print("Width: %i, Height: %i\n", desc.Width, desc.Height);
      
      print("Format: %s\n", getFormatString(desc.Format));
      
      print("Type: ");
      
      switch (desc.Type)
      {
         case D3DRTYPE_SURFACE: print("SURFACE\n"); break;
         case D3DRTYPE_VOLUME: print("VOLUME\n"); break;
         case D3DRTYPE_TEXTURE: print("TEXTURE\n"); break;
         case D3DRTYPE_VOLUMETEXTURE: print("VOLUMETEXTURE\n"); break;
         case D3DRTYPE_CUBETEXTURE: print("CUBETEXTURE\n"); break;
         case D3DRTYPE_VERTEXBUFFER: print("VERTEXBUFFER\n"); break;
         case D3DRTYPE_INDEXBUFFER: print("INDEXBUFFER\n"); break;
         default: print("UNKNOWN\n"); break;
      }
      
      print("Usage: %X ", desc.Usage);
      if (desc.Usage & D3DUSAGE_DEPTHSTENCIL)
         print("DEPTHSTENCIL ");
      if (desc.Usage & D3DUSAGE_RENDERTARGET)
         print("RENDERTARGET");
      print("\n");
      
      print("Pool: %X ", desc.Pool);
      switch (desc.Pool)
      {
         case D3DPOOL_DEFAULT: print("DEFAULT\n"); break;
         case D3DPOOL_MANAGED: print("MANAGED\n"); break;
         case D3DPOOL_SYSTEMMEM: print("SYSTEMMEM\n"); break;
         case D3DPOOL_SCRATCH: print("SCRATCH\n"); break;
         default: print("UNKNOWN\n"); break;
      }
      
      print("MultiSampleType: %X, MultiSampleQuality: %X\n", desc.MultiSampleType, desc.MultiSampleQuality);
            
      indent(-1);
   }  
}

//---------------------------------------------------------------------------------------------------
// BD3DDeviceStateDumper::dumpRenderTargets
//---------------------------------------------------------------------------------------------------
void BD3DDeviceStateDumper::dumpRenderTargets(void)
{
   print("----------------------- Render Targets:\n\n");

   for (int i = 0; i < MaxRenderTargets; i++)
   {
      IDirect3DDevice9 *pDevice = BD3D::mpDev;
      IDirect3DSurface9* pSurf = NULL;
      if (!pDevice)
      {
         print("getDevice() failed!\n");
      }
      else
      {
         const HRESULT hres = pDevice->GetRenderTarget(i, &pSurf);
         if (FAILED(hres))
         {
            print("GetRenderTarget() failed on RT index %i: 0x%X\n", i, hres);
         }
         else
         {

            print("%i: ", i);

            dumpSurfaceInfo( pSurf);

            if (pSurf)
               pSurf->Release();
         }
      }
   }
   
   print("----------------------- Depth/Stencil:\n\n");
               
   IDirect3DDevice9 *pDevice = BD3D::mpDev;
   if (!pDevice)
   {
      print("getDevice() failed!\n");
   }
   else
   {
      IDirect3DSurface9* pSurf = NULL;
      const HRESULT hres = pDevice->GetDepthStencilSurface(&pSurf);
      if (FAILED(hres))
      {
         print("GetDepthStencilSurface() failed: 0x%X\n", hres);
      }
      else
      {
         dumpSurfaceInfo(pSurf);

         if (pSurf)
            pSurf->Release();
      }
   }

   print("----------------------- Current Viewport:\n\n");

   D3DVIEWPORT9 viewport;
   pDevice->GetViewport(&viewport);
   
   print("X: %i, Y: %i, Width: %i, Height: %i, MinZ: %f, MaxZ: %f\n",
      viewport.X,
      viewport.Y,
      viewport.Width,
      viewport.Height,
      viewport.MinZ,
      viewport.MaxZ);
}

//---------------------------------------------------------------------------------------------------
// BD3DDeviceStateDumper::dumpTextures
//---------------------------------------------------------------------------------------------------
void BD3DDeviceStateDumper::dumpTextures(void)
{
   print("----------------------- Textures:\n\n");

   for (int i = 0; i < NumSamplers; i++)
   {
      LPDIRECT3DBASETEXTURE9 pBaseTex = NULL;
       
      BD3D::mpDev->GetTexture(i, &pBaseTex);

      print("%i: 0x%X\n", i, pBaseTex);

      if (pBaseTex)
      {
         indent(1);

         print("Level Count: %i\n", pBaseTex->GetLevelCount());

         D3DRESOURCETYPE type = pBaseTex->GetType();
         switch (type)
         {
            case D3DRTYPE_SURFACE:        print("SURFACE\n"); break;
            case D3DRTYPE_VOLUME:         print("VOLUME\n"); break;
            case D3DRTYPE_TEXTURE:        print("TEXTURE\n"); break;
            case D3DRTYPE_VOLUMETEXTURE:  print("VOLUMETEXTURE\n"); break;
            case D3DRTYPE_CUBETEXTURE:    print("CUBETEXTURE\n"); break;
            default:                      print("Unknown: 0x%X\n", type); break;
         }
         
         if (D3DRTYPE_TEXTURE == type)
         {
            IDirect3DTexture9* pTex = reinterpret_cast<IDirect3DTexture9*>(pBaseTex);
            print("Common: 0x%X\n", pTex->Common);
            print("MipFlush: 0x%X\n", pTex->MipFlush);
            print("Format: 0x%X\n", pTex->Format);
            print("  Width: 0x%X\n", 1+pTex->Format.Size.TwoD.Width);
            print("  Height: 0x%X\n", 1+pTex->Format.Size.TwoD.Height);
            print("  BaseAddress: 0x%X\n", pTex->Format.BaseAddress);
            print("  Pitch: 0x%X\n", pTex->Format.Pitch);
            print("  Tiled: 0x%X\n", pTex->Format.Tiled);
            print("  Endian: 0x%X\n", pTex->Format.Endian);
            print("  DataFormat: 0x%X\n", pTex->Format.DataFormat);
         }

         indent(-1);                         
      }
   }
}

//---------------------------------------------------------------------------------------------------
// BD3DDeviceStateDumper::dumpState
//---------------------------------------------------------------------------------------------------
void BD3DDeviceStateDumper::dumpState(int dumpFlags /* = cDumpAllStates */ )
{
   if (!BD3D::mpDev)
      return;
      
   if (dumpFlags & cDumpRenderStates)
      dumpRenderStates();
      
   if (dumpFlags & cDumpSamplerStates)
      dumpSamplerStates();
      
   if (dumpFlags & cDumpVShaderConstants)
      dumpVShaderConstants();        
      
   if (dumpFlags & cDumpPShaderConstants)
      dumpPShaderConstants();

   if (dumpFlags & cDumpVShaderBoolConstants)
      dumpVShaderBoolConstants();        

   if (dumpFlags & cDumpPShaderBoolConstants)
      dumpPShaderBoolConstants();
      
   if (dumpFlags & cDumpVShader)
      dumpVShader();
      
   if (dumpFlags & cDumpPShader)
      dumpPShader();
      
   if (dumpFlags & cDumpVertexDecl)
      dumpVertexDecl();
   
   if (dumpFlags & cDumpRenderTargets)
      dumpRenderTargets();
    
   if (dumpFlags & cDumpTextures)
      dumpTextures();
   
   print("\n");
}
#endif
