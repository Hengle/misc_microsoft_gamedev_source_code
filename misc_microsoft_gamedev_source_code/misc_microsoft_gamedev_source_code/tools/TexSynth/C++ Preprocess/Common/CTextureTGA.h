//---------------------------------------------------------------------------
#ifndef __CTEXTURETGA__
#define __CTEXTURETGA__
//---------------------------------------------------------------------------
#ifdef WIN32
# include <windows.h>
#endif
//---------------------------------------------------------------------------
#include "CTexture.h"
//---------------------------------------------------------------------------
class CTextureTGA : public CTexture
{
protected:

public:
  CTextureTGA(const char *);
  CTextureTGA(const char *n,int w,int h,bool a,unsigned char *d) : CTexture(n,w,h,a,d) {}
  CTextureTGA(const CTexture& t) : CTexture(t) {}
  CTextureTGA(CTexture *t) : CTexture(t) {}

  void            load();
  void            save();
};
//------------------------------------------------------------------------
#endif
//------------------------------------------------------------------------
