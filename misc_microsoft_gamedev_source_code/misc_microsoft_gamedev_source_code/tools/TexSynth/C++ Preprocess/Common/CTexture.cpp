//---------------------------------------------------------------------------
#ifdef WIN32
#include <windows.h>
#endif
//---------------------------------------------------------------------------
#include <math.h>
//---------------------------------------------------------------------------
#include <GL/gl.h>
#include <GL/glu.h>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include "CLibTextureException.h"
#include "CTexture.h"
//---------------------------------------------------------------------------
// SAT: added define to not include JPG lib
#define NO_JPG
#ifndef NO_JPG
#include "CTextureJPG.h"
#endif
//---------------------------------------------------------------------------
#define NO_PNG
#ifndef NO_PNG
#include "CTexturePNG.h"
#endif
//---------------------------------------------------------------------------
#ifndef NO_TGA
#include "CTextureTGA.h"
#endif
//---------------------------------------------------------------------------
#include "OSAdapter.h"
//---------------------------------------------------------------------------
#ifndef max
# define max(a,b) ((a)>(b)?(a):(b))
#endif
#define CLAMP(m,M,v) (((v) < (m)) ? (m) : ( ((v) > (M)) ? (M) : (v)  ))
#define CLAMP_BYTE(x) (min(255,max(0,x)))
//---------------------------------------------------------------------------
#include <strsafe.h>
//---------------------------------------------------------------------------
CTexture::CTexture()
{
  m_iWidth=0;
  m_iHeight=0;
  m_Data=NULL;
  m_bAlpha=false;
  m_iNbComp=3;
  m_szName[0]='\0';
  //  m_bLuminance=false;
  m_bDataOwner=true;
}
//---------------------------------------------------------------------------
CTexture::CTexture(int w,int h,bool alpha)
{
  m_iWidth=w;
  m_iHeight=h;
  m_bAlpha=alpha;
  m_iNbComp=(m_bAlpha?4:3);
  m_Data=new unsigned char[w*h*m_iNbComp];
  memset(m_Data,0,w*h*m_iNbComp);
  m_szName[0]='\0';
  m_bDataOwner=true;
}
//---------------------------------------------------------------------------
CTexture::CTexture(const char *name,int w,int h,bool alpha,unsigned char *data)
{
  m_iWidth=w;
  m_iHeight=h;
  m_Data=data;
  m_bAlpha=alpha;
  m_iNbComp=(m_bAlpha?4:3);
  strncpy(m_szName,name,TEXTURE_NAME_SIZE);
  m_bDataOwner=false;
}
//---------------------------------------------------------------------------
CTexture::CTexture(const CTexture& tex)
{
  m_iWidth=tex.getWidth();
  m_iHeight=tex.getHeight();
  StringCchCopyA(m_szName,TEXTURE_NAME_SIZE,tex.getName());
  m_Data=new unsigned char[tex.memsize()];
  memcpy(m_Data,tex.getData(),tex.memsize());
  m_bAlpha=tex.isAlpha();
  m_iNbComp=(m_bAlpha?4:3);
  m_bDataOwner=true;
}
//---------------------------------------------------------------------------
CTexture::CTexture(const CTexture *ptex)
{
  m_iWidth=ptex->getWidth();
  m_iHeight=ptex->getHeight();
  StringCchCopyA(m_szName,TEXTURE_NAME_SIZE,ptex->getName());
  m_Data=new unsigned char[ptex->memsize()];
  memcpy(m_Data,ptex->getData(),ptex->memsize());
  m_bAlpha=ptex->isAlpha();
  m_iNbComp=(m_bAlpha?4:3);
  m_bDataOwner=true;
}
//---------------------------------------------------------------------------
CTexture::~CTexture()
{
  if (m_bDataOwner)
    unload();
}
//---------------------------------------------------------------------------
void CTexture::setName(const char *name) {StringCchCopyA(m_szName,TEXTURE_NAME_SIZE,name);}
//---------------------------------------------------------------------------
void CTexture::unload()
{
  if (!m_bDataOwner)
    throw CLibTextureException("Cannot unload a reference texture: it is not the texture object that has loaded data !");
  if (m_Data != NULL)
  {
    delete[] (m_Data);
    m_Data=NULL;
  }
}
//---------------------------------------------------------------------------
void  CTexture::convert2Alpha(unsigned char r,
			      unsigned char g,
			      unsigned char b,
			      unsigned char tolerence)
{
  computeAlpha(CTexture::alpha_color(r,g,b,tolerence));
}
//---------------------------------------------------------------------------
void CTexture::loadAlphaMap(const char *n)
{
  CTexture *tex=loadTexture(n);
  loadAlphaMap(tex);
  delete (tex);
}
//---------------------------------------------------------------------------
void CTexture::loadAlphaMap(CTexture *altex)
{
  unsigned char *ad=altex->getData();
  int t;

  if (altex->getWidth() != getWidth() || altex->getHeight() != getHeight())
    throw CLibTextureException("Alpha map and texture should have same size !");
  if (altex->isAlpha())
    t=4;
  else
    t=3;
  if (!isAlpha())
    convert2Alpha(0,0,0,0);
  for (int i=0;i<getWidth();i++)
  {
    for (int j=0;j<getHeight();j++)
    {
      // use red as alpha
      m_Data[(i+j*getWidth())*4+3]=ad[(i+j*getWidth())*t];
    }
  }
  m_bAlpha=true;
  m_iNbComp=4;
}
//---------------------------------------------------------------------------
void CTexture::computeAlpha(const CTexture::alpha_functor& af)
{
  int i,t=m_iNbComp;
  unsigned char *data;
	
  data=new unsigned char[m_iHeight*m_iWidth*4];
  for (i=0;i<m_iHeight*m_iWidth;i++)
  {
    data[i*4]=m_Data[i*t];
    data[i*4+1]=m_Data[i*t+1];
    data[i*4+2]=m_Data[i*t+2];
	  data[i*4+3]=af.computeAlpha(m_Data[i*t],m_Data[i*t+1],m_Data[i*t+2]);
  }
  delete [](m_Data);
  m_bAlpha=true;
  m_iNbComp=4;
  m_Data=data;
}
//---------------------------------------------------------------------------
CTexture *CTexture::computeNextMIPMapLevel(const mipmap_functor& mip) const
{
  CTexture *tex;
  int       w=max(1,m_iWidth/2);
  int       h=max(1,m_iHeight/2);
  int       t=m_iNbComp;
  unsigned char*data=new unsigned char[w*h*t];
  for (int j=0;j<h;j++)
  {
    for (int i=0;i<w;i++)
    {
      unsigned char r[4]={m_Data[(i*2+j*2*m_iWidth)*t  ],
                m_Data[((i*2+1)+j*2*m_iWidth)*t  ],
                m_Data[((i*2+1)+(j*2+1)*m_iWidth)*t  ],
                m_Data[(i*2+(j*2+1)*m_iWidth)*t  ]};
      unsigned char g[4]={m_Data[(i*2+j*2*m_iWidth)*t+1],
                m_Data[((i*2+1)+j*2*m_iWidth)*t+1],
                m_Data[((i*2+1)+(j*2+1)*m_iWidth)*t+1],
                m_Data[(i*2+(j*2+1)*m_iWidth)*t+1]};
      unsigned char b[4]={m_Data[(i*2+j*2*m_iWidth)*t+2],
                m_Data[((i*2+1)+j*2*m_iWidth)*t+2],
                m_Data[((i*2+1)+(j*2+1)*m_iWidth)*t+2],
                m_Data[(i*2+(j*2+1)*m_iWidth)*t+2]};
      unsigned char a[4]={0,0,0,0};
      if (isAlpha())
      {
        a[0]=m_Data[(i*2+j*2*m_iWidth)*4+3];
        a[1]=m_Data[((i*2+1)+j*2*m_iWidth)*4+3];
        a[2]=m_Data[((i*2+1)+(j*2+1)*m_iWidth)*4+3];
        a[3]=m_Data[(i*2+(j*2+1)*m_iWidth)*4+3];
      }
      unsigned char _r,_g,_b,_a;
      mip.computeAverage(r,g,b,a,_r,_g,_b,_a);
      data[(i+j*w)*t  ]=_r;
      data[(i+j*w)*t+1]=_g;
      data[(i+j*w)*t+2]=_b;
      if (isAlpha())
        data[(i+j*w)*4+3]=_a;
    }
  }
  tex=new CTexture("MIP-map",w,h,isAlpha(),data);
  tex->setDataOwner(true);
  return (tex);
}
//---------------------------------------------------------------------------
void CTexture::computeNormals(double scale,double norme)
{
  unsigned char *d=computeNormalsFromRGB(scale,norme);
  delete (m_Data);
  m_Data=d;
}
//---------------------------------------------------------------------------
void CTexture::convertToNormal(double scale,double norme)
{
  computeNormals(scale,norme);
}
// --------------------------------------------------------
unsigned char *CTexture::computeNormalsFromRGB(double scale,double norme)
{
  int            t,i,j,sz;
  vertex         du,dv,n;
  double         c0,cu,cv;
  unsigned char *data;
  int w,h;

  w=m_iWidth;
  h=m_iHeight;
  t=m_iNbComp;
  data=new unsigned char[t*w*h];
  
  sz=w*h*t;
  for (i=0;i<w;i++)
  {
    for (j=0;j<h;j++)
    {
      c0=( ((double)m_Data[((i+j*w)*t) % sz])
        +((double)m_Data[((i+j*w)*t+1) % sz])
        +((double)m_Data[((i+j*w)*t+2) % sz]))/3.0;
      cu=( ((double)m_Data[(((i+1)+j*w)*t) % sz])
        +((double)m_Data[(((i+1)+j*w)*t+1) % sz])
        +((double)m_Data[(((i+1)+j*w)*t+2) % sz]))/3.0;
      cv=( ((double)m_Data[((i+(j+1)*w)*t) % sz])
        +((double)m_Data[((i+(j+1)*w)*t+1) % sz])
        +((double)m_Data[((i+(j+1)*w)*t+2) % sz]))/3.0;
      du=vertex(1.0,
        0.0,
        (c0-cu)*scale/255.0);
      dv=vertex(0.0,
        1.0,
        (c0-cv)*scale/255.0);
      n=du.cross(dv);
      n.normalize();
      data[((i+j*w)*t  )%sz]=(unsigned char)CLAMP(0,255,
        (128.0 + n.x*norme*127.0));
      data[((i+j*w)*t+1)%sz]=(unsigned char)CLAMP(0,255,
        (128.0 + n.y*norme*127.0));
      data[((i+j*w)*t+2)%sz]=(unsigned char)CLAMP(0,255,
        (128.0 + n.z*norme*127.0));
      if (isAlpha())
        data[((i+j*w)*t+3)%sz]=(unsigned char)m_Data[((i+j*w)*t+3) % sz];
    }
  }
  return (data);
}
//---------------------------------------------------------------------------
void CTexture::computeEdges()
{
  int            t,i,j,sz;
  unsigned char *data;
  int            w,h;

  w=m_iWidth;
  h=m_iHeight;
  t=m_iNbComp;
  data=new unsigned char[t*w*h];
  
  sz=w*h*t;
  for (i=0;i<w;i++)
  {
    for (j=0;j<h;j++)
    {
      for (int c=0;c<3;c++)
      {
        int c0 =getclp(i  ,j  ,c);
        int c0l=getclp(i-1,j  ,c);
        int c0r=getclp(i+1,j  ,c);
        int c0t=getclp(i  ,j-1,c);
        int c0b=getclp(i  ,j+1,c);
        int lr =abs(c0-c0l)+abs(c0-c0r);
        int tb =abs(c0-c0t)+abs(c0-c0b);
        data[((i+j*w)*t+c)%sz]=(unsigned char)CLAMP(0,255,(max(lr,tb)));
      }
      if (isAlpha())
        data[((i+j*w)*t+3)%sz]=(unsigned char)get(i,j,3);
    }
  }
  delete (m_Data);
  m_Data=data;
}
//---------------------------------------------------------------------------
int CTexture::memsize() const
{
  return (m_iNbComp*m_iWidth*m_iHeight);
}
//---------------------------------------------------------------------------
void CTexture::flipY()
{
  int           i,j;
  unsigned char pix[4];
  int           t=m_iNbComp;

  for (i=0;i<m_iWidth;i++)
  {
    for (j=0;j<m_iHeight/2;j++)
    {
      memcpy(pix,&(m_Data[(i+j*m_iWidth)*t]),t);
      memcpy(&(m_Data[(i+j*m_iWidth)*t]),&(m_Data[(i+(m_iHeight-j-1)*m_iWidth)*t]),t);
      memcpy(&(m_Data[(i+(m_iHeight-j-1)*m_iWidth)*t]),pix,t);
    }
  }
}
//---------------------------------------------------------------------------
void CTexture::flipX()
{
  int           i,j;
  unsigned char pix[4];
  int           t=m_iNbComp;

  for (i=0;i<m_iWidth/2;i++)
  {
    for (j=0;j<m_iHeight;j++)
    {
      memcpy(pix,&(m_Data[(i+j*m_iWidth)*t]),t);
      memcpy(&(m_Data[(i+j*m_iWidth)*t]),&(m_Data[((m_iWidth-i-1)+j*m_iWidth)*t]),t);
      memcpy(&(m_Data[((m_iWidth-i-1)+j*m_iWidth)*t]),pix,t);
    }
  }
}
//---------------------------------------------------------------------------
void CTexture::copy(int x,int y,const CTexture *tex,const copy_functor& cpy)
{
  int t=m_iNbComp;
  
  for (int j=0;j<tex->getHeight();j++)
  {
    for (int i=0;i<tex->getWidth();i++)
    {
      int xd=i+x;
      int yd=j+y;
      if ((xd >= 0 && xd < getWidth())
      &&  (yd >= 0 && yd < getHeight()))
        {
          unsigned char r=0,g=0,b=0,a=0,_r=0,_g=0,_b=0,_a=0;
          r=tex->get(i,j,0);
          g=tex->get(i,j,1);
          b=tex->get(i,j,2);
          if (tex->isAlpha())
            a=tex->get(i,j,3);
          cpy.copyPixel(r,g,b,a,_r,_g,_b,_a);
          m_Data[(xd+yd*m_iWidth)*t  ]=_r;
          m_Data[(xd+yd*m_iWidth)*t+1]=_g;
          m_Data[(xd+yd*m_iWidth)*t+2]=_b;
          if (isAlpha() && tex->isAlpha())
              m_Data[(i+j*m_iWidth)*t+3]=_a;
        }
    }
  }
}
//---------------------------------------------------------------------------
void CTexture::copymod(int x,int y,const CTexture *tex,const copy_functor& cpy)
{
  int t=m_iNbComp;
  
  for (int j=0;j<tex->getHeight();j++)
  {
    for (int i=0;i<tex->getWidth();i++)
    {
      int xd=((i+x) + tex->getWidth() /2) % tex->getWidth();
      int yd=((j+y) + tex->getHeight()/2) % tex->getHeight();

      unsigned char r=0,g=0,b=0,a=0,_r=0,_g=0,_b=0,_a=0;
      r=tex->get(i,j,0);
      g=tex->get(i,j,1);
      b=tex->get(i,j,2);
      if (tex->isAlpha())
        a=tex->get(i,j,3);
      cpy.copyPixel(r,g,b,a,_r,_g,_b,_a);
      m_Data[(xd+yd*m_iWidth)*t  ]=_r;
      m_Data[(xd+yd*m_iWidth)*t+1]=_g;
      m_Data[(xd+yd*m_iWidth)*t+2]=_b;
      if (isAlpha() && tex->isAlpha())
        m_Data[(i+j*m_iWidth)*t+3]=_a;
    }
  }
}
//---------------------------------------------------------------------------
CTexture *CTexture::difference(const CTexture *tex)
{
  if (tex->getWidth() != getWidth()
    ||tex->getHeight() != getHeight()
    ||tex->isAlpha() != isAlpha())
    throw CLibTextureException("CTexture::difference - cannot compare %s with %s",getName(),tex->getName());
  
  CTexture *diff=new CTexture(this);

  for (int i=0;i<getWidth();i++)
  {
    for (int j=0;j<getHeight();j++)
    {
      float dr,dg,db,da=0.0;
      dr=abs((float)get(i,j,0) - (float)tex->get(i,j,0));
      dg=abs((float)get(i,j,1) - (float)tex->get(i,j,1));
      db=abs((float)get(i,j,2) - (float)tex->get(i,j,2));
      diff->set(i,j,0)=(unsigned char)CLAMP_BYTE(dr);
      diff->set(i,j,1)=(unsigned char)CLAMP_BYTE(dg);
      diff->set(i,j,2)=(unsigned char)CLAMP_BYTE(db);
      if (isAlpha())
      {
        da=abs((float)get(i,j,3) - (float)tex->get(i,j,3));
        diff->set(i,j,3)=(unsigned char)CLAMP_BYTE(da);
      }
    }
  }
  return (diff);
}
//---------------------------------------------------------------------------

void CTexture::convert(const convert_functor& cnv)
{
  for (int i=0;i<getWidth();i++)
  {
    for (int j=0;j<getHeight();j++)
    {
      unsigned char r=0,g=0,b=0,a=0,_r=0,_g=0,_b=0,_a=0;
      r=get(i,j,0);
      g=get(i,j,1);
      b=get(i,j,2);
      if (isAlpha())
        a=get(i,j,3);
      cnv.convertPixel(r,g,b,a,_r,_g,_b,_a);
      set(i,j,0)=_r;
      set(i,j,1)=_g;
      set(i,j,2)=_b;
      if (isAlpha())
        set(i,j,3)=_a;
    }
  }
}
//---------------------------------------------------------------------------
void CTexture::convert_RGB_HSV::convertPixel(unsigned char r,unsigned char g,unsigned char b,unsigned char a,
                                   unsigned char& _h,unsigned char& _s,unsigned char& _v,unsigned char& _a) const
{
  _a=a;
  float fr=((float)r)/255.0f;
  float fg=((float)g)/255.0f;
  float fb=((float)b)/255.0f;
  float x  = min(fr,min(fg,fb));
  float fv = max(fr,max(fg,fb));

  float f = (fr == x) ? fg - fb : ((fg == x) ? fb - fr : fr - fg);  
  int i = (fr == x) ? 3 : ((fg == x) ? 5 : 1);  

  float fh = i - f /(fv - x);
  float fs = (fv - x)/fv;

  _h=(unsigned char)CLAMP_BYTE(fh*255.0f/6.0f);
  _s=(unsigned char)CLAMP_BYTE(fs*255.0f);
  _v=(unsigned char)CLAMP_BYTE(fv*255.0f);
}
//---------------------------------------------------------------------------
void CTexture::convert_HSV_RGB::convertPixel(unsigned char h,unsigned char s,unsigned char v,unsigned char a,
                                   unsigned char& _r,unsigned char& _g,unsigned char& _b,unsigned char& _a) const
{
  _a=a;

 float fh=h*6.0f/255.0f;
 float fs=((float)s)/255.0f;
 float fv=((float)v)/255.0f;
 int i = (int)(fh);
 float f = (fh - i);
 if (!(i & 1)) 
   f = 1 - f;
 float m = fv * (1 - fs);
 float n = fv * (1 - fs * f);  
 switch (i) 
 {
  case 6:  
  case 0: 
    _r=v; 
    _g=(unsigned char)(CLAMP_BYTE(n*255.0)); 
    _b=(unsigned char)(CLAMP_BYTE(m*255.0));
    break;
  case 1:
    _r=(unsigned char)(CLAMP_BYTE(n*255.0));  
    _g=v;
    _b=(unsigned char)(CLAMP_BYTE(m*255.0));
    break;
  case 2:
    _r=(unsigned char)(CLAMP_BYTE(m*255.0));  
    _g=v;
    _b=(unsigned char)(CLAMP_BYTE(n*255.0));
    break;
  case 3:
    _r=(unsigned char)(CLAMP_BYTE(m*255.0));  
    _g=(unsigned char)(CLAMP_BYTE(n*255.0));
    _b=v;
    break;
  case 4:  
    _r=(unsigned char)(CLAMP_BYTE(n*255.0));  
    _g=(unsigned char)(CLAMP_BYTE(m*255.0));
    _b=v;
    break;
  case 5:
    _r=v; 
    _g=(unsigned char)(CLAMP_BYTE(m*255.0)); 
    _b=(unsigned char)(CLAMP_BYTE(n*255.0));
    break;
 }  
}
//---------------------------------------------------------------------------
void CTexture::convert_RGB_YUV::convertPixel(unsigned char r,unsigned char g,unsigned char b,unsigned char a,
                                   unsigned char& _y,unsigned char& _u,unsigned char& _v,unsigned char& _a) const
{
  static float vls[3][3]={
    { 0.299f,  0.587f,  0.114f},
    {-0.147f, -0.289f,  0.437f},
    { 0.615f, -0.515f, -0.100f}};

  _a=a;
  float fr=((float)r);
  float fg=((float)g);
  float fb=((float)b);

  float fy= 16.0f + (vls[0][0]*fr + vls[0][1]*fg + vls[0][2]*fb);
  float fu=128.0f + (vls[1][0]*fr + vls[1][1]*fg + vls[1][2]*fb);
  float fv=128.0f + (vls[2][0]*fr + vls[2][1]*fg + vls[2][2]*fb);

  assert(fy >= 0.0 && fy <= 255.0);
  assert(fu >= 0.0 && fu <= 255.0);
  assert(fv >= 0.0 && fv <= 255.0);

  _y=(unsigned char)CLAMP_BYTE(fy);
  _u=(unsigned char)CLAMP_BYTE(fu);
  _v=(unsigned char)CLAMP_BYTE(fv);
}
//---------------------------------------------------------------------------
void CTexture::convert_YUV_RGB::convertPixel(unsigned char y,unsigned char u,unsigned char v,unsigned char a,
                                   unsigned char& _r,unsigned char& _g,unsigned char& _b,unsigned char& _a) const
{
  static float vls[3][3]={
    { 1.0f,    0.0f,  1.140f},
    { 1.0f, -0.394f, -0.581f},
    { 1.0f,  2.028f,  0.0f}};

  _a=a;
  float fy=((float)y) - 16.0f;
  float fu=((float)u) - 128.0f;
  float fv=((float)v) - 128.0f;

  float fr=vls[0][0]*fy + vls[0][1]*fu + vls[0][2]*fv;
  float fg=vls[1][0]*fy + vls[1][1]*fu + vls[1][2]*fv;
  float fb=vls[2][0]*fy + vls[2][1]*fu + vls[2][2]*fv;

  _r=(unsigned char)CLAMP_BYTE(fr);
  _g=(unsigned char)CLAMP_BYTE(fg);
  _b=(unsigned char)CLAMP_BYTE(fb);
}
//---------------------------------------------------------------------------
void CTexture::convert_one_channel::convertPixel(unsigned char sr,unsigned char sg,unsigned char sb,unsigned char sa,
                                                 unsigned char& dr,unsigned char& dg,unsigned char& db,unsigned char& da) const
{
  unsigned char v=(unsigned char)(min(255,max(0,(int)(sr*fr + sg*fg + sb*fb + sa*fa))));
  dr=v;dg=v;db=v;da=v;
}
//---------------------------------------------------------------------------
CTexture *CTexture::extract(int x,int y,int w,int h)
{
  CTexture *tex=new CTexture(w,h,isAlpha());
  unsigned char *data=tex->getData();
  int t=m_iNbComp;
  for (int i=0;i<w;i++)
  {
    for (int j=0;j<h;j++)
    {
      int xs=x+i;
      if (xs < 0 || xs >= m_iWidth)
        continue;
      int ys=y+j;
      if (ys < 0 || ys >= m_iHeight)
        continue;
      data[(i+j*w)*t  ]=m_Data[(xs+ys*m_iWidth)*t  ];
      data[(i+j*w)*t+1]=m_Data[(xs+ys*m_iWidth)*t+1];
      data[(i+j*w)*t+2]=m_Data[(xs+ys*m_iWidth)*t+2];
      if (isAlpha())
        data[(i+j*w)*4+3]=m_Data[(xs+ys*m_iWidth)*4+3];
    }
  }
  return (tex);
}
//---------------------------------------------------------------------------
CTexture *CTexture::extractmod(int x,int y,int w,int h)
{
  CTexture *tex=new CTexture(w,h,isAlpha());
  unsigned char *data=tex->getData();
  int t=m_iNbComp;
  for (int i=0;i<w;i++)
  {
    for (int j=0;j<h;j++)
    {
      int xs=x+i;
      int ys=y+j;
      data[(i+j*w)*t  ]=getmod(xs,ys,0);
      data[(i+j*w)*t+1]=getmod(xs,ys,1);
      data[(i+j*w)*t+2]=getmod(xs,ys,2);
      if (isAlpha())
        data[(i+j*w)*4+3]=getmod(xs,ys,3);
    }
  }
  return (tex);
}
//---------------------------------------------------------------------------
unsigned char CTexture::getclp(int i,int j,int c) const
{
  return (m_Data[(min(m_iWidth-1,max(0,i))+(min(m_iHeight-1,max(0,j)))*m_iWidth)*(m_iNbComp)+c]);
}
//---------------------------------------------------------------------------
unsigned char CTexture::getlin(float i,float j,int c) const
{
  int    inti=(int)i;
  float interpi=i-inti;
  int    intj=(int)j;
  float interpj=j-intj;
  
  float c00=getmod(inti,intj,c);
  float c01=getmod(inti,intj+1,c);
  float c10=getmod(inti+1,intj,c);
  float c11=getmod(inti+1,intj+1,c);

  float cA=c00*(1.0f-interpi) + c10*interpi;
  float cB=c01*(1.0f-interpi) + c11*interpi;

  return ((unsigned char)(cA*(1.0-interpj)+cB*interpj));
}
//---------------------------------------------------------------------------
CTexture *CTexture::loadTexture(const char *n)
{
  const char *nos=OSAdapter::convertName(n);
  int l=(int)strlen(nos);
  const char *ext=nos+l-4;

  if (0) {}
#ifndef NO_JPG
  else if (!stricmp(ext,".jpg"))
  {
    return (new CTextureJPG(nos));
  }
#endif
#ifndef NO_PNG
  else if (!stricmp(ext,".png"))
  {
    return (new CTexturePNG(nos));
  }
#endif
#ifndef NO_TGA
  else if (!stricmp(ext,".tga"))
  {
     return (new CTextureTGA(nos));
  }
#endif
  else
    throw CLibTextureException("(load) Unknown file type (%s)",nos);
  return (NULL);
}
//---------------------------------------------------------------------------
void CTexture::saveTexture(const CTexture *tex,const char *n)
{
  const char *nos=OSAdapter::convertName(n);
  int l=(int)strlen(nos);
  const char *ext=nos+l-4;
  if (0) {}
#ifndef NO_JPG
  else if (!stricmp(ext,".jpg"))
  {
    CTextureJPG *tmp=new CTextureJPG(nos,
				     tex->getWidth(),
				     tex->getHeight(),
				     tex->isAlpha(),
				     tex->getData());
    tmp->save();
    delete (tmp);
  } 
#endif
#ifndef NO_PNG
  else if (!stricmp(ext,".png"))
  {
    CTexturePNG *tmp=new CTexturePNG(nos,
				     tex->getWidth(),
				     tex->getHeight(),
				     tex->isAlpha(),
				     tex->getData());
    tmp->save();
    delete (tmp);
  } 
#endif
#ifndef NO_TGA
  else if (!stricmp(ext,".tga"))
  {
     CTextureTGA *tmp=new CTextureTGA(nos,
        tex->getWidth(),
        tex->getHeight(),
        tex->isAlpha(),
        tex->getData());
     tmp->save();
     delete (tmp);
  } 
#endif
  else
    throw CLibTextureException("(save) Unknown file type (%s)",nos);    
}
//---------------------------------------------------------------------------
