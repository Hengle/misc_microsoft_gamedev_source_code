/*

  Quantizer class 

  Take a multi-dimensional float texture and quantize it with q bits so that each value
  falls between -2^(q-1) , 2^(q-1)-1

  Sylvain Lefebvre - (c) Microsoft Corp. - 2005-09-29
*/

// --------------------------------------------------------------------------
#ifndef __QUANTIZER__
#define __QUANTIZER__
// --------------------------------------------------------------------------

#include "mem_tools.h"

// --------------------------------------------------------------------------

#include "CMultiDimFloatTexture.h"

// --------------------------------------------------------------------------

class Quantizer
{
protected:
  
  const MultiDimFloatTexture   *m_Texture;
  MultiDimTexture<int>         *m_Quantized;

  TableAllocator<float>         m_StdDev;
  TableAllocator<float>         m_Mean;
  TableAllocator<float>         m_Center;
  TableAllocator<float>         m_Radius;

  int                           m_iQBits;

  bool quantizePixel(
    int          numcomp,
    const float *values,
    int         *_quantized);

  void unquantizePixel(
    int          numcomp,
    const int   *quantized,
    float       *_unquantized);

public:
  
  /**
    Create the quantizer and immediatly quantize the given texture.
    Quantization is performed so that 'percent' samples are within the quantization
    space (center +/- radius).
    Result can be retrieved with quantized()
  */
  Quantizer(const MultiDimFloatTexture *,int qbits,float percent);
  ~Quantizer();
  
  const MultiDimTexture<int> *quantized()    const {return (m_Quantized);}
  
  MultiDimFloatTexture       *unquantize();
  CTexture                   *error();

  float                       center(int c) const {return (m_Center[c]);}
  float                       radius(int c) const {return (m_Radius[c]);}

};

// --------------------------------------------------------------------------
#endif
// --------------------------------------------------------------------------
