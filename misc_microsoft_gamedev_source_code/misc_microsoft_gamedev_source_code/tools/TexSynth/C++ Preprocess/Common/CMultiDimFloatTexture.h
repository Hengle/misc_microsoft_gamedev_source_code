// ----------------------------------------------------------

#ifndef __MULTIDIMFLOATTEXTURE__
#define __MULTIDIMFLOATTEXTURE__

// ----------------------------------------------------------

#include <fstream>
#include <iostream>
#include <vector>

#include <cassert>

// ----------------------------------------------------------

#include "TMultiDimTexture.h"

// ----------------------------------------------------------

class MultiDimFloatTexture : public MultiDimTexture<float>
{
public:

  // ------

  // Construct an empty texture (init to zero)
  MultiDimFloatTexture(int w,int h,int numc)
    : MultiDimTexture<float>(w,h,numc) {}

  // ------

  // Construct a float texture from an RGB texture
  MultiDimFloatTexture(CTexture *tex)
    : MultiDimTexture<float>(tex) {}

  // ------

  // Load from file
  MultiDimFloatTexture(const std::string& file)
    : MultiDimTexture<float>(file) {}

  // ------

  // Duplicate a texture
  MultiDimFloatTexture(MultiDimFloatTexture *tex)
    : MultiDimTexture<float>(tex) {}

  // ------

  MultiDimFloatTexture(const std::vector<std::string>& files,int numcomp=0)
    : MultiDimTexture<float>(files,numcomp) {}

  MultiDimFloatTexture(
    const std::vector<const CTexture *>& textures,
    int numcomp=0,
    const std::vector<float>& weights=std::vector<float>())
    : MultiDimTexture<float>(textures,numcomp,weights) {}

  // ------

  static MultiDimFloatTexture *makeGaussKernel(int filtersz,float sigma) 
  {
    MultiDimFloatTexture *kernel=new MultiDimFloatTexture(filtersz,filtersz,1);
    float sum=0;
    for (int j=0;j<filtersz;j++) {
      for (int i=0;i<filtersz;i++)  {
        float di=(float)(i-filtersz/2);
        float dj=(float)(j-filtersz/2);
        float u2= ( di*di + dj*dj ) / ((float)filtersz*filtersz);
        kernel->set(i,j,0) = exp( - u2 / (2.0f*sigma*sigma) );
        sum += kernel->get(i,j,0);
      }
    }
    // -> normalize
    for (int j=0;j<filtersz;j++)
      for (int i=0;i<filtersz;i++)
        kernel->set(i,j,0) = kernel->get(i,j,0) / sum;
    return kernel;
  }

  // ------

  // Apply a Gaussian filter of size filtersz - separable implementation
  MultiDimFloatTexture *MultiDimFloatTexture::applyGaussianFilter_Separable(int filtersz) const
  {
    // -> compute Gauss kernel
    MultiDimFloatTexture *kernel=new MultiDimFloatTexture(filtersz,1,1);
    float sum=0;
    for (int i=0;i<filtersz;i++)
    {
      float di=(float)(i-filtersz/2);
      float u2= ( di*di ) / ((float)filtersz*filtersz);
      float sigma=1.0f/3.0f; // > 3 std dev => assume 0
      kernel->set(i,0,0) = exp( - u2 / (2.0f*sigma*sigma) );
      sum += kernel->get(i,0,0);
    }

    // -> normalize
    for (int i=0;i<filtersz;i++)
      kernel->set(i,0,0) = kernel->get(i,0,0) / sum;

    // -> alloc result and tmp buffer (initialized to 0)
    MultiDimFloatTexture *tmp=new MultiDimFloatTexture(width(),height(),numComp());
    MultiDimFloatTexture *res=new MultiDimFloatTexture(width(),height(),numComp());

    // -> convolve - pass 1  (treat texture as toroidal)
    for (int v=0;v<height();v++)
    {
      for (int u=0;u<width();u++)
      {
        for (int i=0;i<filtersz;i++)
        {
          int x = u + i - filtersz/2;
          int y = v;
          
          for (int c=0;c<numComp();c++)
            tmp->set(u,v,c) += ((float)getmod(x,y,c)) * kernel->get(i,0,0);
        }
      }
    }
    // -> convolve - pass 2  (treat texture as toroidal)
    for (int v=0;v<height();v++)
    {
      for (int u=0;u<width();u++)
      {
        for (int j=0;j<filtersz;j++)
        {
          int x = u;
          int y = v + j - filtersz/2;
          for (int c=0;c<numComp();c++)
            res->set(u,v,c) += ((float)tmp->getmod(x,y,c)) * kernel->get(j,0,0);
        }
      }
    }
      
    delete (kernel);
    delete (tmp);
    return (res);
  }

  // Apply a Gaussian filter of size filtersz
  MultiDimFloatTexture *MultiDimFloatTexture::applyGaussianFilter(int filtersz) const
  {
    // -> compute Gauss kernel
    MultiDimFloatTexture *kernel=new MultiDimFloatTexture(filtersz,filtersz,1);
    float sum=0;
    for (int j=0;j<filtersz;j++)
    {
      for (int i=0;i<filtersz;i++)
      {
        float di=(float)(i-filtersz/2);
        float dj=(float)(j-filtersz/2);
        float u2= ( di*di + dj*dj ) / ((float)filtersz*filtersz);
        float sigma=1.0f/3.0f; // > 3 std dev => assume 0
        kernel->set(i,j,0) = exp( - u2 / (2.0f*sigma*sigma) );
        sum += kernel->get(i,j,0);
      }
    }
/*  // DEBUG
    for (int j=0;j<filtersz;j++)
      for (int i=0;i<filtersz;i++)
        kernel->set(i,j,0) = kernel->get(i,j,0) * 255.0f;
    CTexture *tk=kernel->toRGBTexture();
    static char str[256];
    sprintf(str,"kernel_%d.png",filtersz);
    CTexture::saveTexture(tk,str);
    delete (tk);
    for (int j=0;j<filtersz;j++)
      for (int i=0;i<filtersz;i++)
        kernel->set(i,j,0) = kernel->get(i,j,0) / 255.0f;
*/
    // -> normalize
    for (int j=0;j<filtersz;j++)
      for (int i=0;i<filtersz;i++)
        kernel->set(i,j,0) = kernel->get(i,j,0) / sum;

    // -> convolve - treat texture as toroidal
    MultiDimFloatTexture *tex=new MultiDimFloatTexture(width(),height(),numComp());
    for (int v=0;v<height();v++)
    {
      for (int u=0;u<width();u++)
      {
        for (int j=0;j<filtersz;j++)
        {
          for (int i=0;i<filtersz;i++)
          {
            int x = u + i - filtersz/2;
            int y = v + j - filtersz/2;
            for (int c=0;c<numComp();c++)
              tex->set(u,v,c) += ((float)getmod(x,y,c)) * kernel->get(i,j,0);
          }
        }
      }
    }
    delete (kernel);
    return (tex);
  }


  // Compute next MIP-map level using a Gaussian filter of size filtersz.
  MultiDimFloatTexture *computeNextMIPMapLevel_GaussianFilter(int filtersz) const
  {
    MultiDimFloatTexture *filtered=applyGaussianFilter_Separable(filtersz);
    int       w=max(1,width() /2);
    int       h=max(1,height()/2);
    MultiDimFloatTexture *tex=new MultiDimFloatTexture(w,h,numComp());
    for (int v=0;v<h;v++)
    {
      for (int u=0;u<w;u++)
      {
        for (int c=0;c<numComp();c++)
        {
          tex->set(u,v,c)=filtered->get(u*2,v*2,c);
        }
      }
    }
    delete (filtered);
    return (tex);
  }

  MultiDimFloatTexture *computeNextMIPMapLevel_BoxFilter() const
  {
    int       w=max(1,m_iW >> 1);
    int       h=max(1,m_iH >> 1);

    MultiDimFloatTexture *tex = new MultiDimFloatTexture(w,h,m_iNumComp);

    for (int j=0;j<h;j++)
      for (int i=0;i<w;i++)
        for (int c=0;c<m_iNumComp;c++)
          tex->set(i,j,c) = 0.25f*(
            get(i*2  ,j*2  ,c)
          + get(i*2+1,j*2  ,c)
          + get(i*2  ,j*2+1,c)
          + get(i*2+1,j*2+1,c));

    return (tex);
  }

};

// ----------------------------------------------------------

#define FLOAT_2_UCHAR(F)  ((unsigned char)(  min( max((F),0.0f),255.0f) ))

// ----------------------------------------------------------

#endif

// ----------------------------------------------------------
