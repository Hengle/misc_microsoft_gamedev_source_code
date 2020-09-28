/*===================================
  texSynth.cs
 * Origional code : Hoppe / Lebreve (MS Research)
 * Ported code : Colt "MainRoach" McAnlis  (MGS Ensemble studios) [12.01.06]
===================================*/
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Collections;
using System.Text;
using System.Windows.Forms;
using System.Diagnostics;
using System;
using Rendering;
using System.IO;
using EditorCore;

namespace TextureSynthesis
{
   class Quantizer
   {
      
        MultiDimFloatTexture         m_Texture;
        MultiDimIntTexture           m_Quantized;

        float         []m_StdDev;
        float         []m_Mean;
        float         []m_Center;
        float         []m_Radius;

        int                m_iQBits;

      
     public float                       center(int c)  {return (m_Center[c]);}
     public float                       radius(int c)  {return (m_Radius[c]);}
        
      public MultiDimIntTexture quantized()    {return (m_Quantized);}


      public Quantizer(MultiDimFloatTexture tex,int qbits,float percent)
      {
        Globals.Assert(percent > 0.0f && percent <= 100.0f);
        ScopeTimer tm = new ScopeTimer("[Quantizer]");
        m_Texture   = tex;
        m_Quantized = new MultiDimIntTexture(tex.width(),tex.height(),tex.numComp());

        m_iQBits  = qbits;

        m_Mean  =new float[tex.numComp()];
        m_StdDev=new float[tex.numComp()];
        m_Center=new float[tex.numComp()];
        m_Radius=new float[tex.numComp()];

        // . compute std dev and mean of every components

        float []sum = new float[tex.numComp()];
        float []sumsq  = new float[tex.numComp()];

        for (int j=0;j<tex.height();j++)
        {
          for (int i=0;i<tex.width();i++)
          {
            for (int c=0;c<tex.numComp();c++)
            {
              float  f  = tex.get(i,j,c);
              sum[c]   += f;
              sumsq[c] += f*f;
            }
          }
        }
        
        float num=(float)(tex.width()*tex.height());

        for (int c=0;c<tex.numComp();c++)
        {
          m_Mean[c]      = sum[c]/num;
          float sqstddev = (sumsq[c] - sum[c]*sum[c]/num)/(num-1.0f);
          if (sqstddev > BMathLib.cFloatCompareEpsilon)
            m_StdDev[c] = (float)Math.Sqrt(sqstddev);
          else
            m_StdDev[c] = 0.0f;
        }

        // . for every component 
        //      compute center and radius so that 'percent' samples are within quantization space
        for (int c=0;c<tex.numComp();c++)
        {
          float []samples = new float[tex.width()*tex.height()];
          // add samples to array
          for (int j=0;j<tex.height();j++)
            for (int i=0;i<tex.width();i++)
              samples[i+j*tex.width()]=tex.get(i,j,c);

          // sort array
        Sort.RadixSort rs = new Sort.RadixSort();
        rs.Sort(samples);//sort(samples.begin(),samples.end());

          // find left and right elements so that 'percent' elements are inside
          float pout= ((100.0f-percent)/2)/100.0f;
          int left  = (int)((samples.Length-1)*pout);
          int right = (int)((samples.Length - 1) * (1.0f - pout));
          m_Center[c] = (samples[right]+samples[left])/2.0f;
          m_Radius[c] =  samples[right]-samples[left];
        }

        // . quantize
        int num_outside=0;
        for (int j=0;j<tex.height();j++)
        {
          for (int i=0;i<tex.width();i++)
          {
             
            if (!quantizePixel( tex.numComp(),tex, tex.getpixIndex(i,j),m_Quantized.getpixIndex(i,j) )    )
            {
               num_outside++;
            }
          }
        }

        float percent_out = num_outside*100.0f/(tex.width()*tex.height());


        if (Globals.PRINT_DEBUG_MSG) Debug.Print("-----------.>> Quantizer: num outside = " + percent_out + "%");
       tm.destroy();
       tm = null;
         
      

      }


      // --------------------------------------------------------------------------


      ~Quantizer()
      {
         m_Texture = null;
        m_Quantized=null;
        m_StdDev = null;
        m_Mean = null;
        m_Center = null;
        m_Radius = null;

      }

      
      unsafe bool quantizePixel(int numcomp,MultiDimFloatTexture tex,int texPixIndex, int quantizedPixIndex)
      {
        bool    valid=true;
        Globals.Assert(m_iQBits <= 32);

        int maxint=(1 << m_iQBits);

        int []_quantized = m_Quantized.getData();
        for (int i=0;i<numcomp;i++)
        {
           float ctrval = tex.getData()[texPixIndex+i] - m_Center[i];
          ctrval       = ctrval / m_Radius[i];
          ctrval       = (ctrval + 1.0f)/2.0f;

          _quantized[quantizedPixIndex+i] = (int)(ctrval * (maxint - 1));
          if (_quantized[quantizedPixIndex + i] > maxint - 1)
          {
            valid=false;
            _quantized[quantizedPixIndex + i] = maxint - 1;
          }
          if (_quantized[quantizedPixIndex + i] < 0)
          {
            valid=false;
            _quantized[quantizedPixIndex + i] = 0;
          }

          Globals.Assert(_quantized[quantizedPixIndex + i] >= 0 && _quantized[quantizedPixIndex + i] <= maxint - 1);
        }
        return (valid);
      }

   }
}