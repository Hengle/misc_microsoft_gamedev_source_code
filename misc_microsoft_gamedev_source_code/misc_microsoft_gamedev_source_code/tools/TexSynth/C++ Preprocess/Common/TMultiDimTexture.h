// ----------------------------------------------------------

#ifndef __MultiDimTexture__
#define __MultiDimTexture__

// ----------------------------------------------------------

#include <fstream>
#include <iostream>
#include <vector>
#include <string>

#include <cassert>

#include "CTexture.h"

#ifndef NO_LIBHH
#include "Hh.h"
#else
#include <assert.h>
#define assertx assert
#endif

// ----------------------------------------------------------

template <class T_ValueType> class MultiDimTexture
{
protected:

  int         m_iNumComp;
  int         m_iW;
  int         m_iH;
  std::string m_Name;

  T_ValueType *m_Data;

public:

  // Destructor
  ~MultiDimTexture()
  { delete [](m_Data); }

  // ------

  // Construct an empty texture (init to zero)
  MultiDimTexture(int w,int h,int numc)
    : m_iW(w), m_iH(h), m_iNumComp(numc)
  { m_Data = new T_ValueType[w*h*numc]; memset(m_Data,0,sizeof(T_ValueType)*w*h*numc); }

  // ------

  // Construct a T_ValueType texture from an RGB texture
  MultiDimTexture(CTexture *tex)
    : m_iW(tex->getWidth()), m_iH(tex->getHeight()), m_iNumComp(3)
  { 
    m_Data = new T_ValueType[m_iW*m_iH*m_iNumComp];
    for (int j=0;j<height();j++)
      for (int i=0;i<width();i++)
        for (int c=0;c<numComp();c++)
          set(i,j,c)=(T_ValueType)tex->get(i,j,c);
    m_Name=std::string(tex->getName());
  }

  // ------

  // Load from file
  MultiDimTexture(const std::string& file)
  {
    load(file);
  }

  // ------

  // Duplicate a texture
  MultiDimTexture(MultiDimTexture *tex)
    : m_iW(tex->width()), m_iH(tex->height()), m_iNumComp(tex->numComp())
  { 
    m_Data = new T_ValueType[m_iW*m_iH*m_iNumComp];
    for (int j=0;j<height();j++)
      for (int i=0;i<width();i++)
        for (int c=0;c<numComp();c++)
          set(i,j,c)=(T_ValueType)tex->get(i,j,c);
    m_Name=std::string(tex->getName());
  }

  // ------

  // Load a multi-channel T_ValueType texture from a list of RGB texture files
  // if numcomp == 0, number of channels = files.size()*3
  // otherwise, the number of channels is clamped to numcomp
  // TODO: load, and then call the method below
  MultiDimTexture(const std::vector<std::string>& files,int numcomp=0)
  {
    assertx(!files.empty());

    CTexture *first=CTexture::loadTexture(files[0].c_str()); // inefficient (loaded twice), but not a big issue here
    m_Name=std::string(first->getName());
    m_iW=first->getWidth();
    m_iH=first->getHeight();
    if (numcomp > 0)
      m_iNumComp=numcomp;
    else
      m_iNumComp=(int)files.size()*3;
    delete (first);

    m_Data = new T_ValueType[m_iW*m_iH*m_iNumComp]; 
    memset(m_Data,0,sizeof(T_ValueType)*m_iW*m_iH*m_iNumComp);

    int firstcomp=0;
    for (int f=0;f<(int)files.size();f++)
    {
      CTexture *tex=CTexture::loadTexture(files[f].c_str());
      assertx(tex->getWidth() == m_iW && tex->getHeight() == m_iH);
      for (int j=0;j<m_iH;j++)
      {
        for (int i=0;i<m_iW;i++)
        {
          for (int c=0;c<3;c++)
          {
            if (firstcomp+c < m_iNumComp)
              set(i,j,firstcomp+c) = tex->get(i,j,c);
          }
        }
      }
      firstcomp+=3;
      delete (tex);
    }
  }

  
  // Load a multi-channel T_ValueType texture from a list of RGB textures
  // if numcomp == 0, number of channels = textures.size()*3
  // otherwise, the number of channels is clamped to numcomp
  // Textures are not deleted

  MultiDimTexture(
    const std::vector<const CTexture *>& textures,
    int numcomp=0,
    const std::vector<T_ValueType>& scales=std::vector<T_ValueType>())
  {
    assertx(!textures.empty());
    assertx(scales.empty()
      || (numcomp != 0 && scales.size() == numcomp)
      || (numcomp == 0 && scales.size() == textures.size()*3));

    const CTexture *first=textures[0];
    m_Name=std::string(first->getName());
    m_iW=first->getWidth();
    m_iH=first->getHeight();
    if (numcomp > 0)
      m_iNumComp=numcomp;
    else
      m_iNumComp=(int)textures.size()*3;

    m_Data = new T_ValueType[m_iW*m_iH*m_iNumComp]; 
    memset(m_Data,0,sizeof(T_ValueType)*m_iW*m_iH*m_iNumComp);

    int firstcomp=0;
    for (int f=0;f<(int)textures.size();f++)
    {
      const CTexture *tex=textures[f];
      assertx(tex->getWidth() == m_iW && tex->getHeight() == m_iH);
      if (scales.empty())
      {
        for (int j=0;j<m_iH;j++)
        {
          for (int i=0;i<m_iW;i++)
          {
            for (int c=0;c<3;c++)
            {
              if (firstcomp+c < m_iNumComp)
                set(i,j,firstcomp+c) = tex->get(i,j,c);
            }
          }
        }
      }
      else
      {
        for (int j=0;j<m_iH;j++)
        {
          for (int i=0;i<m_iW;i++)
          {
            for (int c=0;c<3;c++)
            {
              if (firstcomp+c < m_iNumComp)
                set(i,j,firstcomp+c) = scales[firstcomp+c]*((T_ValueType)tex->get(i,j,c));
            }
          }
        }
      }
      firstcomp+=3;
    }
  }

  // ------

  // Emptied the texture
  void      zero()              {memset(m_Data,0,sizeof(T_ValueType)*m_iW*m_iH*m_iNumComp);}
  // Width
  int       width()      const {return (m_iW);}
  int       getWidth()   const {return (m_iW);} // for backward compatibility with CTexture
  // Height
  int       height()     const {return (m_iH);}
  int       getHeight()  const {return (m_iH);} // for backward compatibility with CTexture
  // Number of components
  int       numComp() const {return (m_iNumComp);}
  // Direct access to data
  T_ValueType *data()             {return (m_Data);}

  // ------

  // Retrieve a value
  T_ValueType get(int i,int j,int c)    const
  {
    assertx(i >= 0 && j >=0 && c >=0 && i < m_iW && j < m_iH && c < m_iNumComp);
    return (m_Data[(j*m_iW + i)*m_iNumComp + c]);
  }

  // Retrieve a value, modulo domain
  T_ValueType getmod(int i,int j,int c) const
  {
    while (i < 0)     i+=m_iW;
    while (i >= m_iW) i-=m_iW;
    while (j < 0)     j+=m_iH;
    while (j >= m_iH) j-=m_iH;
    return (m_Data[(j*m_iW + i)*m_iNumComp + c]);
  }

  // Retrieve a value, bilinear interp
  T_ValueType getlin(float i,float j,int c) const
  {
    int    inti=(int)i;
    float interpi=i-inti;
    int    intj=(int)j;
    float interpj=j-intj;

    T_ValueType c00=getmod(inti,intj,c);
    T_ValueType c01=getmod(inti,intj+1,c);
    T_ValueType c10=getmod(inti+1,intj,c);
    T_ValueType c11=getmod(inti+1,intj+1,c);

    T_ValueType cA=c00*(1.0f-interpi) + c10*interpi;
    T_ValueType cB=c01*(1.0f-interpi) + c11*interpi;

    return (T_ValueType(cA*(1.0-interpj)+cB*interpj));
  }
 
  // Retrieve a value, bilinear interp, texture lookup style
  T_ValueType tex2D(float i,float j,int c) const {return (getlin(i*m_iW-.5f,j*m_iH-.5f,c));}

  // Retrieve a pointer to a pixel (channels are stored sequentially)
  const T_ValueType *getpix(int i,int j) const
  {
    return &(m_Data[(j*m_iW + i)*m_iNumComp ]);
  }

  T_ValueType *getpix(int i,int j)
  {
    return &(m_Data[(j*m_iW + i)*m_iNumComp ]);
  }

  // Retrieve a pointer to a pixel, modulo space (channels are stored sequentially)
  const T_ValueType *getpixmod(int i,int j) const
  {
    while (i < 0)     i+=m_iW;
    while (i >= m_iW) i-=m_iW;
    while (j < 0)     j+=m_iH;
    while (j >= m_iH) j-=m_iH;
    return &(m_Data[(j*m_iW + i)*m_iNumComp ]);
  }

  T_ValueType *getpixmod(int i,int j) 
  {
    while (i < 0)     i+=m_iW;
    while (i >= m_iW) i-=m_iW;
    while (j < 0)     j+=m_iH;
    while (j >= m_iH) j-=m_iH;
    return &(m_Data[(j*m_iW + i)*m_iNumComp ]);
  }

  // Set a value
  T_ValueType& set(int i,int j,int c)    const
  {
    assert(i >= 0 && j >=0 && c >=0 && i < m_iW && j < m_iH && c < m_iNumComp);
    return (m_Data[(j*m_iW + i)*m_iNumComp + c]);
  }

  // ------

  void  save(const std::string& file) const
  {
    FILE *f=fopen(file.c_str(),"wb");
    if (f == NULL)
      throw CLibTextureException("MultiDimTexture::save - Cannot save '%s'",file.c_str());
    // save dimensions
    fwrite(&m_iW,sizeof(int),1,f);
    fwrite(&m_iH,sizeof(int),1,f);
    fwrite(&m_iNumComp,sizeof(int),1,f);
    // save data
    size_t to_write=size_t(m_iW)*size_t(m_iH)*size_t(m_iNumComp);
    size_t num=0;
    do {
      size_t remains=to_write-num;
      size_t to_be_written=min(size_t(64000),remains);
      size_t written=fwrite(m_Data+num,sizeof(T_ValueType),to_be_written,f);
      assertx(written == to_be_written);
      num+=written;
    } while (num < to_write);
    assertx(to_write == num);
    // close
    fclose(f);
  }

  // ------

  void  load(const std::string& file)
  {
    FILE *f=fopen(file.c_str(),"rb");
    if (f == NULL)
      throw CLibTextureException("MultiDimTexture::load - Cannot load '%s'",file.c_str());
    m_Name=file;
    std::cerr << "Loading multi-dim T_ValueType texture '" << file.c_str() << "' ... ";
    // load dimensions
    fread(&m_iW,sizeof(int),1,f);
    fread(&m_iH,sizeof(int),1,f);
    fread(&m_iNumComp,sizeof(int),1,f);
    // load data
    m_Data=new T_ValueType[m_iW*m_iH*m_iNumComp];
    size_t to_read=size_t(m_iW)*size_t(m_iH)*size_t(m_iNumComp);
    size_t num=0;
    do {
      size_t remains=to_read-num;
      size_t to_be_read=min(size_t(64000),remains);
      size_t read=fread(m_Data+num,sizeof(T_ValueType),to_be_read,f);
      assertx(read == to_be_read);
      num+=read;
    } while (num < to_read);
    assertx(to_read == num);
    // close
    fclose(f);
    std::cerr << "done." << std::endl;
  }

  // ------

  // Extract a sub-tile of the texture
  template <class T_Texture> T_Texture *extract(int x,int y,int w,int h) const
  {
    T_Texture *res=new T_Texture(w,h,m_iNumComp);
    for (int j=0;j<h;j++) {
      for (int i=0;i<w;i++) {
        int xs=x+i;
        if (xs < 0 || xs >= m_iW)
          continue;
        int ys=y+j;
        if (ys < 0 || ys >= m_iH)
          continue;
        for (int c=0;c<m_iNumComp;c++)
          res->set(i,j,c)=get(xs,ys,c);
      }
    }
    return (res);
  }

  // Fill with a given value
  void      fill(T_ValueType v)
  {
    for (int j=0;j<m_iH;j++)
      for (int i=0;i<m_iW;i++)
        for (int c=0;c<m_iNumComp;c++)
          set(i,j,c)=v;
  }

  // Extract a sub-tile, assuming modulo texture domain
  template <class T_Texture> T_Texture *extractmod(int x,int y,int w,int h) const
  {
    T_Texture *res=new T_Texture(w,h,m_iNumComp);
    for (int j=0;j<h;j++)
    {
      for (int i=0;i<w;i++)
      {
        int xs=x+i;
        int ys=y+j;
        for (int c=0;c<m_iNumComp;c++)
          res->set(i,j,c)=getmod(xs,ys,c);
      }
    }
    return (res);
  }

  // Copy a tile into the texture
  void copy(int x,int y,const MultiDimTexture *tex)
  {
    assertx(tex->numComp() == numComp());
    for (int j=0;j<tex->height();j++)
    {
      for (int i=0;i<tex->width();i++)
      {
        int xd=i+x;
        int yd=j+y;
        if ((xd >= 0 && xd < width())
          &&  (yd >= 0 && yd < height()))
        {
          for (int c=0;c<numComp();c++)
            set(xd,yd,c)=tex->get(i,j,c);
        }
      }
    }
  }

  // ------

  // Convert into an RGB texture
  CTexture *toRGBTexture(int firstcomp=0,T_ValueType scale=1.0f) const
  {
    CTexture *tex=new CTexture(m_iW,m_iH,false);
    for (int j=0;j<m_iH;j++)
    {
      for (int i=0;i<m_iW;i++)
      {
        for (int c=0;c<min(m_iNumComp-firstcomp,3);c++)
          tex->set(i,j,c)=(unsigned char)min(255.0f,max(0.0f,get(i,j,firstcomp+c)*scale));
      }
    }
    return (tex);
  }

  // ------

  // Convert into an texture of a different type

  template <class T_OtherValueType>
    MultiDimTexture<T_OtherValueType> *toTextureType()
  {
    MultiDimTexture<T_OtherValueType> *tex=new MultiDimTexture<T_OtherValueType>(m_iW,m_iH,m_iNumComp);
    for (int j=0;j<m_iH;j++)
    {
      for (int i=0;i<m_iW;i++)
      {
        for (int c=0;c<m_iNumComp;c++)
          tex->set(i,j,c)=(T_OtherValueType)get(i,j,firstcomp+c);
      }
    }
    return (tex);
  }

  // -------

  const char *getName() const {return (m_Name.c_str());}
  void        setName(const char *n)  {m_Name = std::string(n);}

};

// ----------------------------------------------------------

#endif

// ----------------------------------------------------------
