// --------------------------------------------------------------------------

#include "CQuantizer.h"

#include <float.h>

#include <algorithm>

#include "Hh.h"

// --------------------------------------------------------------------------


Quantizer::Quantizer(const MultiDimFloatTexture *tex,int qbits,float percent)
{
  assertx(percent > 0.0f && percent <= 100.0f);

  m_Texture   = tex;
  m_Quantized = new MultiDimTexture<int>(tex->width(),tex->height(),tex->numComp());

  m_iQBits  = qbits;

  m_Mean  .allocate(tex->numComp());
  m_StdDev.allocate(tex->numComp());
  m_Center.allocate(tex->numComp());
  m_Radius.allocate(tex->numComp());

  // -> compute std dev and mean of every components

  TableAllocator<float,true> sum   (tex->numComp());
  TableAllocator<float,true> sumsq (tex->numComp());

  for (int j=0;j<tex->height();j++)
  {
    for (int i=0;i<tex->width();i++)
    {
      for (int c=0;c<tex->numComp();c++)
      {
        float  f  = tex->get(i,j,c);
        sum[c]   += f;
        sumsq[c] += f*f;
      }
    }
  }
  
  float num=(float)(tex->width()*tex->height());

  for (int c=0;c<tex->numComp();c++)
  {
    m_Mean[c]      = sum[c]/num;
    float sqstddev = (sumsq[c] - sum[c]*sum[c]/num)/(num-1.0f);
    if (sqstddev > FLT_EPSILON)
      m_StdDev[c] = sqrt(sqstddev);
    else
      m_StdDev[c] = 0.0f;
  }

  // -> for every component 
  //      compute center and radius so that 'percent' samples are within quantization space
  for (int c=0;c<tex->numComp();c++)
  {
    vector<float> samples;
    samples.resize(tex->width()*tex->height());
    // add samples to array
    for (int j=0;j<tex->height();j++)
      for (int i=0;i<tex->width();i++)
        samples[i+j*tex->width()]=tex->get(i,j,c);
    // sort array
    sort(samples.begin(),samples.end());
    // find left and right elements so that 'percent' elements are inside
    float pout= ((100.0f-percent)/2)/100.0f;
    int left  = (int)((samples.size()-1)*pout);
    int right = (int)((samples.size()-1)*(1.0f - pout));
    m_Center[c] = (samples[right]+samples[left])/2.0f;
    m_Radius[c] =  samples[right]-samples[left];
  }

  // -> quantize
  int num_outside=0;
  for (int j=0;j<tex->height();j++)
  {
    for (int i=0;i<tex->width();i++)
    {
      if (!quantizePixel( tex->numComp(),
                          tex->getpix(i,j),
                          m_Quantized->getpix(i,j) )
         )
         num_outside++;
    }
  }

  float percent_out = num_outside*100.0f/(tex->width()*tex->height());

  static char str[256];
  sprintf(str,"Quantizer: num outside = %.2f%%\n",percent_out);
  cerr << str;
  OutputDebugStringA("\n------------>>> ");
  OutputDebugStringA(str);

}


// --------------------------------------------------------------------------


Quantizer::~Quantizer()
{
  delete (m_Quantized);
}


// --------------------------------------------------------------------------


MultiDimFloatTexture *Quantizer::unquantize()
{
  MultiDimFloatTexture *unq=new MultiDimFloatTexture(
    m_Quantized->width(),m_Quantized->height(),m_Quantized->numComp());

  for (int j=0;j<m_Quantized->height();j++)
  {
    for (int i=0;i<m_Quantized->width();i++)
    {
      unquantizePixel(
        m_Quantized->numComp(),
        m_Quantized->getpix(i,j),
        unq->getpix(i,j));
    }
  }

  return (unq);
}


// --------------------------------------------------------------------------


CTexture *Quantizer::error()
{
  TemporaryObject<MultiDimFloatTexture> unq=unquantize();

  CTexture *errtex = new CTexture(m_Texture->width(),m_Texture->height(),false);

  for (int j=0;j<m_Texture->height();j++)
  {
    for (int i=0;i<m_Texture->width();i++)
    {
      float err=0.0f;
      for (int c=0;c<m_Texture->numComp();c++)
      {
        float d = (m_Texture->get(i,j,c) - unq->get(i,j,c));
        err    += d*d;
      }
      err = sqrt(err) / float(m_Texture->numComp());
      errtex->set(i,j,0) = (unsigned char)max(min(255.0f,err),0.0f);
      errtex->set(i,j,1) = (unsigned char)max(min(255.0f,err*10.0f),0.0f);
      errtex->set(i,j,2) = (unsigned char)max(min(255.0f,err*1000.0f),0.0f);
    }
  }

  return (errtex);
}


// --------------------------------------------------------------------------


bool Quantizer::quantizePixel(
                         int          numcomp,
                         const float *values,
                         int         *_quantized
                         )
{
  bool    valid=true;
  assertx(m_iQBits <= 32);

  int maxint=(1 << m_iQBits);
  
  for (int i=0;i<numcomp;i++)
  {
    float ctrval = values[i] - m_Center[i];
    ctrval       = ctrval / m_Radius[i];
    ctrval       = (ctrval + 1.0f)/2.0f;

    _quantized[i]= (int)(ctrval * (maxint-1));
    if (_quantized[i] > maxint-1)
    {
      valid=false;
      _quantized[i]=maxint-1;
    }
    if (_quantized[i] < 0)
    {
      valid=false;
      _quantized[i]=0;
    }

    assertx(_quantized[i] >= 0 && _quantized[i] <= maxint-1 );
  }
  return (valid);
}


// --------------------------------------------------------------------------


void Quantizer::unquantizePixel(
                           int          numcomp,
                           const int   *quantized,
                           float       *_unquantized
                           )
{
  assertx(m_iQBits <= 32);

  unsigned int maxint=(1 << m_iQBits);
  for (int i=0;i<numcomp;i++)
  {
    assertx( quantized[i] >= 0 && quantized[i] <= (int)maxint-1 );

    float ctrval= ( float(quantized[i]) / float(maxint-1) )*2.0f - 1.0f;
    ctrval          = ctrval * m_Radius[i];
    _unquantized[i] = ctrval + m_Center[i];
  }
}


// --------------------------------------------------------------------------
