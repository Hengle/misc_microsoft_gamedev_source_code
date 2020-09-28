//-----------------------------------------------------------------------------
// File: hdr_screenshot.h
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#pragma once
#ifndef HDR_SCREENSHOT_H
#define HDR_SCREENSHOT_H

#include "common/math/half_float.h"
#include "common/math/vector.h"

namespace gr
{
	class HDRScreenshot
	{
		struct Color
    {
      uchar v[4];
      
      uchar operator[] (int i) const { return v[DebugRange(i, 4)]; }
      uchar& operator[] (int i) { return v[DebugRange(i, 4)]; }
    };

    // Hacked from "Real Pixels", Graphics Gems 2
    typedef uchar BYTE;
    typedef BYTE COLR[4];
     
    static Color intensityToRGBE(const Vec4& i)
    {
      Color ret;
      ret[0] = ret[1] = ret[2] = ret[3] = 0;

      float v = i.maxComponent();
      if (v >= 1e-32)
      {
        int e;
        v = frexp(v, &e) * 255.9999f / v;
        ret[0] = Math::FloatToIntTrunc(i[0] * v);
        ret[1] = Math::FloatToIntTrunc(i[1] * v);
        ret[2] = Math::FloatToIntTrunc(i[2] * v);
        ret[3] = e + 128;
      }

      return ret;
    }
    
    #define  MINELEN  8 /* minimum scanline length for encoding */
    #define  MINRUN   4 /* minimum run length */

    static int fwritecolrs(COLR* scanline, int len, FILE* fp)    /* write out a colr scanline */
    {
      int  i, j, beg, cnt;
      int  c2;
      
      if (len < MINELEN)    /* too small to encode */
        return(fwrite((char *)scanline,sizeof(COLR),len,fp) - len);
      if (len > 32767)    /* too big! */
        return(-1);
      putc(2, fp);      /* put magic header */
      putc(2, fp);
      putc(len>>8, fp);
      putc(len&255, fp);
              /* put components separately */
      for (i = 0; i < 4; i++) {
          for (j = 0; j < len; j += cnt) {  /* find next run */
        for (beg = j; beg < len; beg += cnt) {
            for (cnt = 1; cnt < 127 && beg+cnt < len &&
              scanline[beg+cnt][i] == scanline[beg][i]; cnt++)
          ;
            if (cnt >= MINRUN)
          break;      /* long enough */
        }
        if (beg-j > 1 && beg-j < MINRUN) {
            c2 = j+1;
            while (scanline[c2++][i] == scanline[j][i])
          if (c2 == beg) {  /* short run */
              putc(128+beg-j, fp);
              putc(scanline[j][i], fp);
              j = beg;
              break;
          }
        }
        while (j < beg) {   /* write out non-run */
            if ((c2 = beg-j) > 128) c2 = 128;
            putc(c2, fp);
            while (c2--)
          putc(scanline[j++][i], fp);
        }
        if (cnt >= MINRUN) {    /* write out run */
            putc(128+cnt, fp);
            putc(scanline[beg][i], fp);
        } else
            cnt = 0;
          }
      }
      return(ferror(fp) ? -1 : 0);
    }
  
    static void writeRadianceHalfFloat(
      FILE* pFile,
      const D3DSURFACE_DESC& desc,
      const uchar* pBits,
      const int pitch)
    {
      std::vector<Color> scanline(desc.Width);
            
      for (int y = 0; y < desc.Height; y++)
      {
        const ushort* pSrc = reinterpret_cast<const ushort*>(pBits + pitch * y);

        for (int x = 0; x < desc.Width; x++)
        {
          Vec4 v(0);
          
          for (int i = 0; i < 3; i++)
          {
            const float val = Half::HalfToFloat(*pSrc);
            if (Math::IsValidFloat(val))
              v[i] = Math::ClampLow(val, 0.0f);
            pSrc++;
          }
          pSrc++; // skip alpha

          scanline[x] = intensityToRGBE(v);
        }

        Verify(fwritecolrs((COLR*)&scanline[0], desc.Width, pFile) >= 0);
      }
    }

	public:
	  static writeRadianceFromSurf(FILE* pFile, IDirect3DSurface9* pSrcSurf)
    {
      //IDirect3DSurface9* pSrcSurf = gShaderManager.getSurface(eABuffer);
      
      D3DSURFACE_DESC desc;
      pSrcSurf->GetDesc(&desc);

      Verify(desc.Format == D3DFMT_A16B16G16R16F);

      IDirect3DSurface9* pDstSurf = D3D::createOffscreenPlainSurface(
        desc.Width,
        desc.Height,
        desc.Format,
        D3DPOOL_SYSTEMMEM);
                
      D3D::getRenderTargetData(pSrcSurf, pDstSurf);

      D3DLOCKED_RECT rect;
      pDstSurf->LockRect(&rect, NULL, D3DLOCK_READONLY);

      const uchar* pBits = reinterpret_cast<const uchar*>(rect.pBits);
      const int pitch = rect.Pitch;
      
      char buf[256];
      
      sprintf(buf, "#?RADIANCE\nFORMAT=32-bit_rle_rgbe\n\n-Y %i +X %i\n",
        desc.Height,
        desc.Width);

      Verify(1 == fwrite(buf, strlen(buf), 1, pFile));

      writeRadianceHalfFloat(
        pFile,
        desc,
        pBits,
        pitch);

      pDstSurf->UnlockRect();
      
      pDstSurf->Release();
    }
				
	};

} // namespace gr

#endif // HDR_SCREENSHOT_H
