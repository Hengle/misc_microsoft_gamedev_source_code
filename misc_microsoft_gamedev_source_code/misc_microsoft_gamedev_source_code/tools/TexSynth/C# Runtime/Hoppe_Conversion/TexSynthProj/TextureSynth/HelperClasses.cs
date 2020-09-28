/*===================================
  texSynth.cs
 * Origional code : Hoppe / Lebreve (MS Research)
 * Ported code : Colt "MainRoach" McAnlis (MGS Ensemble studios) [12.01.06]
===================================*/ 
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Drawing.Imaging;
using System.Text;
using System.Windows.Forms;
using System.Collections;
using System.Diagnostics;
using Microsoft.DirectX;
using Microsoft.DirectX.Direct3D;
using System.IO;
using System.Diagnostics;
using Rendering;
using EditorCore;

namespace TextureSynthesis
{
  
   class CTexture
   {
      byte []m_Data = null;
      string m_Name = null;
      int m_iW = 0;
      int m_iH = 0;
      bool m_bAlpha = false;
      int m_iNbComp = 0;

      public CTexture()
      {
      }
      public CTexture(int w, int h, bool alpha)//=false)
      {
         m_iW=w;
         m_iH=h;
         m_bAlpha=alpha;
         m_iNbComp=(m_bAlpha?4:3);
         m_Data=new byte[w*h*m_iNbComp];
        
       //  m_szName[0]='\0';
       //  m_bDataOwner=true;
      }
      unsafe public CTexture(String filename)
      {
         m_Name = filename;
        /* Texture tex = TextureLoader.FromFile(BRenderDevice.getDevice(), filename);
         Debug.Print("CTexture Loaded :" + filename);
         Image img = Image.FromStream(TextureLoader.SaveToStream(ImageFileFormat.Png, tex));
         */
         Bitmap mInternal = new Bitmap(filename);
       //  tex.Dispose();
      //   tex = null;

         Rectangle r = new Rectangle(0, 0, mInternal.Width, mInternal.Height);
         BitmapData sourceData = mInternal.LockBits(r, ImageLockMode.ReadOnly, mInternal.PixelFormat);
         byte* sourceBase = (byte*)sourceData.Scan0;

         m_bAlpha = mInternal.PixelFormat == PixelFormat.Format24bppRgb ? false : true;
         m_iW = mInternal.Width;
         m_iH = mInternal.Height;
         m_iNbComp = (m_bAlpha ? 4 : 3);
         m_Data = new byte[m_iW * m_iH * m_iNbComp];
         for (int i = 0; i < m_iW * m_iH * m_iNbComp;i++ )
         {
            m_Data[i] = sourceBase[i];
         }

            mInternal.UnlockBits(sourceData);
      }
      ~CTexture()
      {
         m_Data =null;
      }

      public int getWidth()
      {
         return m_iW;
      }
      public int getHeight()
      {
         return m_iH;
      }
      public int width()
      {
         return m_iW;
      }
      public int height()
      {
         return m_iH;
      }
      public byte get(int i, int j, int c)
      {
         return m_Data[(i + j * m_iW) * m_iNbComp + c];
      }
      public void set(byte value,int i, int j, int c)
      {
         m_Data[(i + j * m_iW) * m_iNbComp + c] = value;
         
      }
      public string getName()
      {
         return m_Name;
      }

      static public CTexture loadTexture(String name)
      {
         return new CTexture(name);
      }
   }

   class MultiDimTexture<T>
   {
      ~MultiDimTexture()
      {
         m_Data = null;
      }
      protected T[] m_Data = null;
      public T[] getData()
      {
         return m_Data;
      }
      public void set(T val, int i, int j, int k)
      {
         m_Data[(j * m_iW + i) * m_iNumComp + k] = val;
      }
      public T get(int i, int j, int k)
      {
         return m_Data[(j * m_iW + i) * m_iNumComp + k];
      }

      public T getpix(int i, int j)
      {
         return m_Data[(j * m_iW + i) * m_iNumComp];
      }
      public int getpixIndex(int i, int j)
      {
         return (j * m_iW + i) * m_iNumComp;
      }

      // Retrieve a value, modulo domain
      public T getmod(int i, int j, int c)
      {
         while (i < 0) i += m_iW;
         while (i >= m_iW) i -= m_iW;
         while (j < 0) j += m_iH;
         while (j >= m_iH) j -= m_iH;
         return (m_Data[(j * m_iW + i) * m_iNumComp + c]);
      }
      // Retrieve a pointer to a pixel, modulo space (channels are stored sequentially)
      public T getpixmod(int i, int j)
      {
         while (i < 0) i += m_iW;
         while (i >= m_iW) i -= m_iW;
         while (j < 0) j += m_iH;
         while (j >= m_iH) j -= m_iH;
         return (m_Data[(j * m_iW + i) * m_iNumComp]);
      }
      public int getpixmodIndex(int i, int j)
      {
         while (i < 0) i += m_iW;
         while (i >= m_iW) i -= m_iW;
         while (j < 0) j += m_iH;
         while (j >= m_iH) j -= m_iH;
         return ((j * m_iW + i) * m_iNumComp);
      }


      public int getWidth()
      {
         return m_iW;
      }
      public int getHeight()
      {
         return m_iH;
      }
      public int width()
      {
         return m_iW;
      }
      public int height()
      {
         return m_iH;
      }
      public int numComp()
      {
         return m_iNumComp;
      }
      public void setName(String name)
      {
         m_Name = name;
      }
      public string getName()
      {
         return m_Name;
      }

      protected string m_Name = null;
      protected int m_iW = 0;
      protected int m_iH = 0;
      protected int m_iNumComp = 0;
   }
   class MultiDimIntTexture : MultiDimTexture<int>
   {
      public MultiDimIntTexture(int i, int j, int k)
      {
         m_Name = "";
         m_iW = i;
         m_iH = j;
         m_iNumComp = k;
         m_Data = new int[m_iW * m_iH * m_iNumComp];
      }
      ~MultiDimIntTexture()
      {
         m_Data = null;
      }

     

   }
   class MultiDimFloatTexture : MultiDimTexture<float>
   {
 
      public bool load(String filename)
      {
         if (!File.Exists(filename))
            throw new Exception("MultiDimTexture::load - Cannot load " + filename);

         BinaryReader f = new BinaryReader(File.Open(filename, FileMode.Open, FileAccess.Read));

         m_Name = filename;
         if (Globals.PRINT_DEBUG_MSG) Debug.Print("Loading multi-dim T_ValueType texture '" + filename + "' ... ");

         m_iW = f.ReadInt32();
         m_iH = f.ReadInt32();
         m_iNumComp = f.ReadInt32();

         m_Data=new float[m_iW*m_iH*m_iNumComp];

         int to_read = m_iW * m_iH * m_iNumComp;
         int num=0;
         for (int i = 0; i < to_read; i++)
            m_Data[i] = f.ReadSingle();
         /*
            do
            {
               int remains = to_read - num;
               int to_be_read = min(size_t(64000), remains);
               int read = fread(m_Data + num, sizeof(float), to_be_read, f);
                Globals.Assert(read == to_be_read);
               num += read;
            } while (num < to_read);

          Globals.Assert(to_read == num);
          * */
         // close
         f.Close();
         return true;
      }
      public MultiDimFloatTexture(String name)
      {
         m_Name = name;
         load(m_Name);
      }
      public MultiDimFloatTexture(int i, int j, int k)
      {
         m_Name = "";
         m_iW = i;
         m_iH = j;
         m_iNumComp = k;
         m_Data = new float[m_iW * m_iH * m_iNumComp];

      }
      public MultiDimFloatTexture(MultiDimFloatTexture tex)
      {
         m_Name = tex.m_Name;
      }
      ~MultiDimFloatTexture()
      {
         m_Data=null;
      }


      // ------
      public MultiDimFloatTexture computeNextMIPMapLevel_BoxFilter()
      {
         int w = (int)Math.Max(1, m_iW >> 1);
         int h = (int)Math.Max(1, m_iH >> 1);

         MultiDimFloatTexture tex = new MultiDimFloatTexture(w, h, m_iNumComp);

         for (int j = 0; j < h; j++)
            for (int i = 0; i < w; i++)
               for (int c = 0; c < m_iNumComp; c++)
               {
                  float val = 0.25f * (
                             get(i * 2, j * 2, c)
                           + get(i * 2 + 1, j * 2, c)
                           + get(i * 2, j * 2 + 1, c)
                           + get(i * 2 + 1, j * 2 + 1, c));
                  tex.set(val, i, j, c);
               }

         return (tex);
      }
      public MultiDimFloatTexture computeNextMIPMapLevel_GaussianFilter(int filtersz)
      {
         MultiDimFloatTexture filtered = applyGaussianFilter_Separable(filtersz);
         int w = (int)Math.Max(1, m_iW / 2);
         int h = (int)Math.Max(1, m_iH / 2);
         MultiDimFloatTexture tex = new MultiDimFloatTexture(w, h, m_iNumComp);
         for (int v = 0; v < h; v++)
         {
            for (int u = 0; u < w; u++)
            {
               for (int c = 0; c < m_iNumComp; c++)
               {
                  tex.set(filtered.get(u * 2, v * 2, c), u, v, c);
               }
            }
         }
         filtered = null;
         return (tex);
      }
      
      static MultiDimFloatTexture makeGaussKernel(int filtersz, float sigma)
      {
         MultiDimFloatTexture kernel = new MultiDimFloatTexture(filtersz, filtersz, 1);
         float sum = 0;
         for (int j = 0; j < filtersz; j++)
         {
            for (int i = 0; i < filtersz; i++)
            {
               float di = (float)(i - filtersz / 2);
               float dj = (float)(j - filtersz / 2);
               float u2 = (di * di + dj * dj) / ((float)filtersz * filtersz);
               kernel.set((float)Math.Exp(-u2 / (2.0f * sigma * sigma)), i, j, 0);
               sum += kernel.get(i, j, 0);
            }
         }
         // . normalize
         for (int j = 0; j < filtersz; j++)
            for (int i = 0; i < filtersz; i++)
               kernel.set(kernel.get(i, j, 0) / sum,i, j, 0);
            
         return kernel;
      }

      // Apply a Gaussian filter of size filtersz - separable implementation
      private MultiDimFloatTexture applyGaussianFilter_Separable(int filtersz) 
     {
       // . compute Gauss kernel
       MultiDimFloatTexture kernel=new MultiDimFloatTexture(filtersz,1,1);
       float sum=0;
       for (int i=0;i<filtersz;i++)
       {
         float di=(float)(i-filtersz/2);
         float u2= ( di*di ) / ((float)filtersz*filtersz);
         float sigma=1.0f/3.0f; // > 3 std dev => assume 0
         kernel.set((float)Math.Exp( - u2 / (2.0f*sigma*sigma) ),i,0,0);
         sum += kernel.get(i,0,0);
       }

       // . normalize
       for (int i=0;i<filtersz;i++)
          kernel.set(kernel.get(i, 0, 0) / sum,i, 0, 0);

       // . alloc result and tmp buffer (initialized to 0)
       MultiDimFloatTexture tmp = new MultiDimFloatTexture(m_iW, m_iH, m_iNumComp);
       MultiDimFloatTexture res = new MultiDimFloatTexture(m_iW, m_iH, m_iNumComp);

       // . convolve - pass 1  (treat texture as toroidal)
       for (int v = 0; v < m_iH; v++)
       {
          for (int u = 0; u < m_iW; u++)
         {
           for (int i=0;i<filtersz;i++)
           {
             int x = u + i - filtersz/2;
             int y = v;

             for (int c = 0; c < numComp(); c++)
             {
                float val = tmp.get(u, v, c);
                tmp.set(val + ((float)getmod(x, y, c)) * kernel.get(i, 0, 0), u, v, c);
             }
           }
         }
       }
       // . convolve - pass 2  (treat texture as toroidal)
       for (int v = 0; v < m_iH; v++)
       {
          for (int u = 0; u < m_iW; u++)
         {
           for (int j=0;j<filtersz;j++)
           {
             int x = u;
             int y = v + j - filtersz/2;
             for (int c = 0; c < m_iNumComp; c++)
             {
                float val = res.get(u, v, c);
                res.set(val + ((float)tmp.getmod(x, y, c)) * kernel.get(j, 0, 0),u, v, c);
             }
           }
         }
       }
         
       kernel=null;
       tmp = null;
       return (res);
     }
      // ------
        // Extract a sub-tile of the texture
      public MultiDimFloatTexture extract(int x, int y, int w, int h)
     {
        MultiDimFloatTexture res = new MultiDimFloatTexture(w, h, m_iNumComp);
       for (int j=0;j<h;j++) 
       {
         for (int i=0;i<w;i++) 
         {
           int xs=x+i;
           if (xs < 0 || xs >= m_iW)
             continue;
           int ys=y+j;
           if (ys < 0 || ys >= m_iH)
             continue;
           for (int c=0;c<m_iNumComp;c++)
             res.set(get(xs,ys,c),i,j,c);
         }
       }
       return (res);
     }
   }

   class Pair<A, B> : IComparer
   {
      public A first;
      public B second;

      public Pair(A a,B b)
      {
         first = a;
         second = b;
      }
      int IComparer.Compare(object x, object y)
      {
         Pair<int, float> a = (Pair<int, float>)x;
         Pair<int, float> b = (Pair<int, float>)y;
         return (a.second < b.second) ? 1 : 0;
      }
   }

   
   class Window
   {
      private int  m_iLeft;
      private int  m_iRight;
      private int  m_iTop;
      private int  m_iBottom;
      private bool m_bEmpty;

        // empty test
   public  bool empty() {return (m_bEmpty);}


     private Window(int l,int r,int t,int b)
      { 
        m_iLeft=(l); m_iRight=(r);m_iTop=(t);m_iBottom=(b);m_bEmpty=(false) ;
     normalize();}


     public Window()
     {
        m_iLeft=(0); m_iRight=(0); m_iTop=(0); m_iBottom=(0); m_bEmpty=(true);
     }

     public Window( Window w)
     {
       m_iLeft=w.left(); m_iRight=w.right(); m_iTop=w.top(); m_iBottom=w.bottom();
       m_bEmpty=w.m_bEmpty;
       normalize();
     }

      // -----------------------------------------------------

      public void normalize()
      {
         int l = Math.Min(left(), right());
         int r = Math.Max(left(), right());
         int t = Math.Min(top(), bottom());
         int b = Math.Max(top(), bottom());

        m_iLeft   = l;
        m_iRight  = r;
        m_iTop    = t;
        m_iBottom = b;
      }

      // -----------------------------------------------------

      public void print() 
      {
   
        string str="["+left()+","+top()+"] "+width()+"x"+height();
        if (Globals.PRINT_DEBUG_MSG) Debug.Print(str);
      }

      // -----------------------------------------------------

      public bool equals(Window w) 
      {
        return (left() == w.left() && right() == w.right() && top() == w.top() && bottom() == w.bottom());
      }

      // -----------------------------------------------------

      public bool includes(Window w) 
      {
        if ( w.left() >= left() && w.right() <= right()
          && w.top() >= top() && w.bottom() <= bottom())
        {
          return (true);
        }
        else
          return (false);
      }

      // -----------------------------------------------------

      public bool intersects(Window w) 
      {
        if (w.left() > right() || w.right() < left())
          return (false);
        if (w.top() > bottom() || w.bottom() < top())
          return (false);
        return (!includes(w) && !w.includes(this));
      }

      // -----------------------------------------------------

      public Window overlap(Window w) 
      {
        if (intersects(w) || includes(w) || w.includes(this))
        {
          int l = Math.Max(left()  , w.left());
          int r = Math.Min(right(), w.right());
          int t = Math.Max(top(), w.top());
          int b = Math.Min(bottom(), w.bottom());

          return (new Window(l,r,t,b));
        }
        else
          return (new Window());
      }

      // -----------------------------------------------------

      public Window merge(Window w) 
      {
        if (empty()==true)
          return (w);
        if (w.empty()==true)
          return (this);

       int l = Math.Min(left(), w.left());
       int r = Math.Max(right(), w.right());
       int t = Math.Min(top(), w.top());
       int b = Math.Max(bottom(), w.bottom());

        return (new Window(l,r,t,b));
      }

      // -----------------------------------------------------

      public int nestedBorderDistance(Window w) 
      {
        if (includes(w))
        {
           int d = Math.Min(w.left() - left(), Math.Min(right() - w.right(), Math.Min(w.top() - top(), bottom() - w.bottom())));
          Globals.Assert(d >= 0);
          return (d);
        }
        else if (w.includes(this))
        {
           int d = Math.Min(left() - w.left(), Math.Min(w.right() - right(), Math.Min(top() - w.top(), w.bottom() - bottom())));
          Globals.Assert(d >= 0);
          return (d);
        }
      //  assert(false);
        return (0);
      }

      // -----------------------------------------------------

      public Window relativeTo(Window w) 
      {
        if (empty()==true)
          return (new Window());

        Globals.Assert(w.includes(this));

        int l = left()   - w.left();
        int r = right()  - w.left();
        int t = top()    - w.top();
        int b = bottom() - w.top();

        return (new Window(l,r,t,b));
      }

      // -----------------------------------------------------
        
     public Window absoluteFrom( Window w) 
      {
        if (empty()==true)
          return (new Window());

        int l = left()   + w.left();
        int r = right()  + w.left();
        int t = top()    + w.top();
        int b = bottom() + w.top();

        Window res = new Window(l,r,t,b);
        Globals.Assert(w.includes(res));
        return (res);
      }

     // creation utilities

     public static Window LTWH(int l,int t,int w,int h) {return new Window(l,l+w-1,t,t+h-1);}
     public static Window LRTB(int l,int r,int t,int b) {return new Window(l,r,t,b);}
     public static Window LTRB(int l,int t,int r,int b) {return new Window(l,r,t,b);}
     public static Window RTWH(int r,int t,int w,int h) {return new Window(r-w+1,r,t,t+h-1);}
     public static Window LBWH(int l,int b,int w,int h) {return new Window(l,l+w-1,b-h+1,b);}

     // accessors

     public int left()    {return (m_iLeft);}
     public int right()   {return (m_iRight);}
     public int top()     {return (m_iTop);}
     public int bottom()  {return (m_iBottom);}
     public int        width()   {return (m_iRight-m_iLeft+1);}
     public int        height()  {return (m_iBottom-m_iTop+1);}

     public void setLeft(int i)   {m_iLeft  =i; normalize();}
     public void setRight(int i)  {m_iRight =i; normalize();}
     public void setTop(int i)    {m_iTop   =i; normalize();}
     public void setBottom(int i) {m_iBottom=i; normalize();}

   };

   
   class SynthWindows
   {

      List<Window> m_BufferRgn = new List<Window>();   // absolute coordinates of synthesized window
      List<Window> m_Requested = new List<Window>();   // relative coordinates of requested windows
      List<Window> m_Valid = new List<Window>();       // relative coordinates of valid window
      List<Window> m_Cached = new List<Window>();      // absolute coordinates of cached region (valid minus reserved ring)
      List<Window> m_UpdCopySrc = new List<Window>();  // update - source region from previous
      List<Window> m_UpdCopyDst = new List<Window>();  // update - destination region into new
      List<Window> m_UpdHorz = new List<Window>();     // update - horizontal region
      List<Window> m_UpdVert = new List<Window>();     // update - vertical region
      List<bool>   m_HasChanged = new List<bool>();  // indicates if a level has been changed by update

      // used to save window during partial level update
      Window     m_TmpBufferRgn;
      Window     m_TmpValid;
      bool       m_bUpdateReady;
      int        m_iUpdateLevel;

      int        m_iNbLevels;

      int        m_iPrefetchBorder;
      int        m_iBorderL;
      int        m_iBorderR;
      int        m_iBorderT;
      int        m_iBorderB;

     
      public SynthWindows() 
      { 
         m_iNbLevels=(0);
         m_iPrefetchBorder=(0);
         m_iBorderL=(0);
         m_iBorderR=(0);
         m_iBorderT=(0);
         m_iBorderB=(0);
      }

         
      public SynthWindows(Window target,int nblvl,int borderl, int borderr,int bordert, int borderb, int prefetch)
      {
        m_bUpdateReady=false;
        m_iNbLevels=nblvl;
        
        m_iBorderL = borderl;
        m_iBorderR = borderr;
        m_iBorderT = bordert;
        m_iBorderB = borderb;
        m_iPrefetchBorder = prefetch;

        Window requested  = target;
        Window previous   = target;
        for (int l=0 ; l < m_iNbLevels ; l++)
        {
          Window valid        = windowAddPrefetch(previous);
          Window bufferregion = windowAddPadding(valid);

          m_BufferRgn  .Add(bufferregion);
          m_Valid      .Add(valid       .relativeTo(bufferregion));
          m_Requested  .Add(requested   .relativeTo(bufferregion));
          m_Cached     .Add(windowAddPrefetch(requested));

          m_UpdCopySrc .Add(new Window());
          m_UpdCopyDst .Add(new Window());
          m_UpdHorz    .Add(new Window());
          m_UpdVert    .Add(new Window());
          m_HasChanged .Add(true);

          requested = windowCoarserLevel(requested);
          previous  = windowCoarserLevel(bufferregion);
        }
      }


      // ------------------------------------------------------


      SynthWindows(List<Window> requested,int borderl,int borderr, int bordert,int borderb,int prefetch)
      {
        m_bUpdateReady = false;
        m_iNbLevels    = (int)requested.Count;

        m_iBorderL = borderl;
        m_iBorderR = borderr;
        m_iBorderT = bordert;
        m_iBorderB = borderb;
        m_iPrefetchBorder = prefetch;

        int l=0;

        Window reserved=new Window();
        for (int i=0;i<requested.Count;i++)
        {
          Window cached = windowAddPrefetch(requested[i]);
          Window req    = cached.merge(reserved);

          Window valid        = req;
          Window bufferregion = windowAddPadding(valid);

          m_BufferRgn  .Add(bufferregion);
          m_Valid      .Add(valid.relativeTo(bufferregion));
          m_Requested  .Add(requested[i].relativeTo(bufferregion));
          m_Cached     .Add(cached);

          m_UpdCopySrc .Add(new Window());
          m_UpdCopyDst .Add(new Window());
          m_UpdHorz    .Add(new Window());
          m_UpdVert    .Add(new Window());
          m_HasChanged .Add(true);

          reserved  = windowCoarserLevel(bufferregion);
        }

      }

      ~SynthWindows()
      {
         m_BufferRgn = null;
         m_Requested = null;
         m_Valid = null;    
         m_Cached = null;   
         m_UpdCopySrc = null;
         m_UpdCopyDst = null; 
         m_UpdHorz = null;    
         m_UpdVert = null;    
         m_HasChanged = null;

         if(mD3DPixPackOffsets!=null)
         {
            for(int i=0;i<mD3DPixPackOffsets.GetLength(0);i++)
            {
               for (int j = 0; j < mD3DPixPackOffsets.GetLength(1); j++)
               {
                  for (int k = 0; k < mD3DPixPackOffsets.GetLength(2); k++)
                  {
                     if(mD3DPixPackOffsets[i,j,k]!=null)
                     {
                        mD3DPixPackOffsets[i, j, k].Dispose();
                        mD3DPixPackOffsets[i, j, k] = null;
                     }
                  }
               }
            }
            mD3DPixPackOffsets = null;
         }
      }

      // ------------------------------------------------------

      public Window windowCoarserLevel(Window valid)
      {
       int l = (valid.left()  ) / 2;
       int r = (valid.right() ) / 2;
       int t = (valid.top()   ) / 2;
       int b = (valid.bottom()) / 2;

       return (Window.LRTB(l,r,t,b));
      }

      Window windowAddPadding(Window req) 
      {
       int l = (req.left()   - m_iBorderL);
       int r = (req.right()  + m_iBorderR);
       int t = (req.top()    - m_iBorderT);
       int b = (req.bottom() + m_iBorderB);

       return (Window.LRTB(l,r,t,b));
      }

      Window windowAddPrefetch( Window req) 
      {
       int l = (req.left()  - m_iPrefetchBorder);
       int r = (req.right() + m_iPrefetchBorder);
       int t = (req.top()   - m_iPrefetchBorder);
       int b = (req.bottom()+ m_iPrefetchBorder);

       return (Window.LRTB(l,r,t,b));
      }

   // ------------------------------------------------------
   // helper function - [true] if list l contains integer e

      static bool contains(ref List<int> l,int e)
      {
        bool found=false;
        for (int i = 0; i < l.Count;i++ )
        {
           if (l[i] == e)
           {
              found = true;
              break;
           }
        }
        return (found);
      }

   // ------------------------------------------------------
   // helper function - sort list - FIXME: ugly, expensive: do something better
   
      static void sort(ref List<int> l)
      {
        int []tmp = new int[l.Count];
        for (int i=0;i<l.Count;i++)
          tmp[i]=(l[i]);

         Sort.RadixSort rs = new Sort.RadixSort();
         rs.Sort(tmp);

        l.Clear();
        for (int i=0;i<(int)tmp.Length;i++)
          l.Add(tmp[i]);
      }
   
      Pair<Window,Window> computeUpdateRegions(Window valid, Window overlap) 
      {
        Pair<Window,Window> res = new Pair<Window,Window>(null,null);

        if (overlap.empty())
        {
          res.first=valid;
        }
        else
        {
          int w=valid.width() -overlap.width() ;
          int h=valid.height()-overlap.height();
          Globals.Assert(w >= 0 && h >= 0);

          int l=(valid.left() == overlap.left()) ? overlap.right() +1 : valid.left();
          int t=(valid.top()  == overlap.top() ) ? overlap.bottom()+1 : valid.top() ;

          if (w > 0)
          {
            res.first =Window.LTWH(
              l              , valid.top()    ,
              w              , valid.height() );
          }

          if (h > 0)
          {
            if (l > valid.left())
              res.second=Window.LTWH(
              valid.left()      , t             ,
              overlap.width()   , h             );
            else
              res.second=Window.LTWH(
              l+w-1  +1           , t             ,
              overlap.width()     , h             );
          }
        }
        return (res);
      }


      void computeNewRegions(int l,SynthWindows synthwins)
      {
         // compute update regions
         // -> common synthesized part, express in new window coords, to be copied
         Window absSynthCached   =           bufferrgn(l);
         Window absSynthNew      = synthwins.bufferrgn(l);
         Window absOverlap       = absSynthCached.overlap(absSynthNew);

         Window absValidCached   = valid(l)          .absoluteFrom(bufferrgn(l));
         Window absValidNew      = synthwins.valid(l).absoluteFrom(synthwins.bufferrgn(l));
         Window absValidOverlap  = absValidCached    .overlap(absValidNew);

         Pair<Window,Window> rgn = computeUpdateRegions(absValidNew,absValidOverlap);
         m_UpdVert[l]            = rgn.first.relativeTo(synthwins.bufferrgn(l));
         m_UpdHorz[l]            = rgn.second.relativeTo(synthwins.bufferrgn(l));
         m_UpdCopySrc[l]         = absValidOverlap.relativeTo(          bufferrgn(l));
         m_UpdCopyDst[l]         = absValidOverlap.relativeTo(synthwins.bufferrgn(l));

         m_BufferRgn[l]          = synthwins.bufferrgn(l);
         m_Valid[l]              = synthwins.valid(l);
         m_Requested[l]          = synthwins.requested(l);
         m_Cached[l]             = synthwins.m_Cached[l];

      }

         // TODO : get rid of 'contains' => O(n) !!!!

         List<int> update(SynthWindows synthwins,int   lastsynthesizedlevel)
         {
           List<int> tobeupdated = new List<int>();
           int       nbearly=0;

           // STEP 1
           // reuse some coarse levels ?
           for (int l=m_iNbLevels-1 ; l>=lastsynthesizedlevel ; l--)
           {
             // is new requested window included in current valid window ?
             // -> request window in absolute coords.
             Window absreq=synthwins.absRequested(l);
             // -> current valid in absolute coord.
             Window absval=absValid(l);
             // -> if current valid area does not include requested area, level cannot be reused    
             if (! absval.includes(absreq))
             {
               tobeupdated.Insert(0,l);
             }
           }

           // STEP 2
           // -> Early Update: if finest level request is closest than half prefetch, regen !
           if (tobeupdated.Count < 1)
           {
             for (int l=m_iNbLevels-1 ; l>=lastsynthesizedlevel ; l--)
             {
               if (! contains(ref tobeupdated,l) )
               {
                 if (nbearly < 1)
                 {
                   Window absreq=synthwins.absRequested(l);
                   int dist  = absreq.nestedBorderDistance(m_Cached[l]);
                   int thres = m_iPrefetchBorder / 2;
                   //      cerr << l << " - thres " << thres << " - dist " << dist << endl;
                   if (dist < thres)
                   {
                     nbearly++;
                     tobeupdated.Insert(0, l);
                   }
                 }
               }
             }
           }

           // STEP 3
           // -> from finest to coarsest
           // update regions and check parents hold enough data
           //      skip finest
           for (int l=lastsynthesizedlevel;l<m_iNbLevels;l++)
           {
             bool lpresent = contains(ref tobeupdated,l);
             bool ladded   = false;

             if (l > 0 && !lpresent)
             {
               if (contains(ref tobeupdated,l-1))
               {
                 Globals.Assert(bufferrgn(l-1).equals(synthwins.bufferrgn(l-1)));

                 Window needed = windowCoarserLevel(bufferrgn(l-1));
                 if ( ! (absValid(l).includes(needed)) )
                 {
                   tobeupdated.Add(l);
                   ladded=true;
                 }
               }
             }

             if (lpresent || ladded)
               computeNewRegions(l,synthwins);
           }

           for (int l=0;l<m_iNbLevels;l++)
           {
             if (l < lastsynthesizedlevel)
               m_HasChanged[l]=false;
             else if (! contains(ref tobeupdated,l))
             {
               Window absreq    = synthwins.absRequested(l);
               if (absreq.equals(absRequested(l)))
                 m_HasChanged[l]=false;
               else
               {
                 m_Requested[l]   = absreq.relativeTo(bufferrgn(l));
                 m_UpdCopySrc[l]  = new Window();
                 m_UpdCopyDst[l] = new Window();
                 m_UpdHorz[l] = new Window();
                 m_UpdVert[l] = new Window();
                 m_HasChanged[l]=true;
               }
             }
             else
               m_HasChanged[l]=true;
           }

           // sort list

           sort(ref tobeupdated);

           // sanity check

           // -> check enough data in levels
           for (int k = 0;k<tobeupdated.Count;k++)
           {
             if (tobeupdated[k] < m_iNbLevels-1)
             {
               Window coarser_needed = windowCoarserLevel(bufferrgn(tobeupdated[k]));
               Window coarser_valid  = absValid(tobeupdated[k]+1);
               if (! coarser_valid.includes(coarser_needed))
               {
                 // make sure parent also in regen
                 // cerr << "failed(" << (*R) << ") ";
                 MessageBox.Show("(SynthWindows::update) Sanity check failed !");
               }
             }
           }
           // cerr << endl;

           // -> check list is sorted
           int last=-1;
           for (int k = 0; k < tobeupdated.Count; k++)
           {
              if (!(last == -1 || last > (tobeupdated[k])))
                 MessageBox.Show("(SynthWindows::update) List not sorted !");
              last = tobeupdated[k];
           }

           return (tobeupdated);
         }
      
      public List<int> update( Window target)
      {
         SynthWindows synthwins = new SynthWindows(target,m_iNbLevels,m_iBorderL,m_iBorderR,m_iBorderT,m_iBorderB,m_iPrefetchBorder);
         return (update(synthwins,0));
      }
      

      public List<int> update(List<Window> requested,int lastsynthesizedlevel)
      {
         SynthWindows synthwins= new SynthWindows(requested,m_iBorderL,m_iBorderR,m_iBorderT,m_iBorderB,m_iPrefetchBorder);
         if (synthwins.m_iNbLevels != m_iNbLevels)
           throw new Exception("SynthWindows::update - can only update if the number of requested levels is the same !");
         return (update(synthwins,lastsynthesizedlevel));
      }
   
      public void beginUpdateHorz(int l)
      {
         Globals.Assert(!m_bUpdateReady);

         m_iUpdateLevel   = l;
         // save current windows
         m_TmpBufferRgn   = m_BufferRgn[l];
         m_TmpValid       = m_Valid[l];
         // compute windows for update
         Window upd       = m_UpdHorz[l].absoluteFrom(m_BufferRgn[l]);
         Window padded    = windowAddPadding(upd);
         Globals.Assert(m_BufferRgn[l].includes(padded));

         m_BufferRgn[l]   = padded;
         m_Valid[l]       = upd.relativeTo(m_BufferRgn[l]);
         // ready
         m_bUpdateReady   = true;
      }



      public void beginUpdateVert(int l)
      {
         Globals.Assert(!m_bUpdateReady);

         m_iUpdateLevel   = l;
         // save current windows
         m_TmpBufferRgn   = m_BufferRgn[l];
         m_TmpValid       = m_Valid[l];
         // compute windows for update
         Window upd       = m_UpdVert[l].absoluteFrom(m_BufferRgn[l]);
         Window padded    = windowAddPadding(upd);
         Globals.Assert(m_BufferRgn[l].includes(padded));

         m_BufferRgn[l]   = padded;
         m_Valid[l]       = upd.relativeTo(m_BufferRgn[l]);
         // ready
         m_bUpdateReady   = true;
      }


      public void endUpdate()
      {
         Globals.Assert(m_bUpdateReady);

         // restore windows
         m_BufferRgn[m_iUpdateLevel] = m_TmpBufferRgn;
         m_Valid[m_iUpdateLevel]     = m_TmpValid;

         m_bUpdateReady              = false;
      }


      public int nbLevels() { return (m_iNbLevels); }
      public Window bufferrgn(int l)      {return (m_BufferRgn[l]);}
      public Window requested(int l) { return (m_Requested[l]); }
      public Window valid(int l) { return (m_Valid[l]); }
      public Window absRequested(int l) { return (m_Requested[l].absoluteFrom(bufferrgn(l))); }
      public Window absValid(int l) { return (m_Valid[l].absoluteFrom(bufferrgn(l))); }
      public Window updCopySrc(int l) { return (m_UpdCopySrc[l]); }
      public Window updCopyDst(int l) { return (m_UpdCopyDst[l]); }
      public Window updHorz(int l) { return (m_UpdHorz[l]); }
      public Window updVert(int l) { return (m_UpdVert[l]); }
      public bool hasChanged(int l) { return (m_HasChanged[l]); }

      public Window coarserNeeded(int l) { return (windowCoarserLevel(windowAddPadding(absRequested(l)))); }

      public Texture[, ,] mD3DPixPackOffsets = null;

      unsafe public void computeD3DPixPackTextures(Synthesiser m_Synthesizer)
      {
           int sz = 0;
          for (int l = 0; l < Globals.cNumLevels/*m_Synthesizer.getAnalyser().nbLevels()*/; l++)
         {
            int csz = Synthesiser.bufferSizeFromWindow(bufferrgn(l));
            sz = Math.Max(csz, sz);
         }

         mD3DPixPackOffsets = new Texture[m_iNbLevels, Globals.cNumCorrectionPasses, 4];

         for(int l=0;l<m_iNbLevels;l++)
         {
            Window window = bufferrgn(l);
            for (int pass = 0; pass < Globals.cNumCorrectionPasses; pass++)
            {
               for (int sub = 0; sub < 4; sub++)
               {
                  // . which quadrant to update ?
                  int ai = (m_Synthesizer.m_SubpassesOrderingTbl[pass % Globals.SYNTH_ORDER_NUM_PASSES][sub][0] + window.left()) & 1;
                  int aj = (m_Synthesizer.m_SubpassesOrderingTbl[pass % Globals.SYNTH_ORDER_NUM_PASSES][sub][1] + window.top()) & 1;
                  Globals.Assert(ai >= 0 && aj >= 0);


                  Vector4 qszs = m_Synthesizer.bufferSize(sz);

                  mD3DPixPackOffsets[l, pass, sub] = new Texture(BRenderDevice.getDevice(), 5, 5, 1, 0, Format.G16R16F, Pool.Managed);
                  GraphicsStream texstream = mD3DPixPackOffsets[l,pass,sub].LockRectangle(0, LockFlags.None);
                  ushort* data = (ushort*)texstream.InternalDataPointer;
                  int rectPitch = 5 * 2;

                  for (int x = 0; x < 5; x++)
                  {
                     for (int y = 0; y < 5; y++)
                     {
                        int ox = x - 2;
                        int oy = y - 2;
                        int fx = (int)Math.Floor((ox + ai) / 2.0f);
                        int fy = (int)Math.Floor((oy + aj) / 2.0f);
                        Vector4 vq = new Vector4(
                                ((float)(fx + (sz >> 1) * ox) + ai * (sz >> 1)) / qszs.X,
                                ((float)(fy + (sz >> 1) * oy) + aj * (sz >> 1)) / qszs.Y,
                                0, 0);
                        //offs[x + y * 5] = vq;

                        int srcIndex = x * 2 * sizeof(byte) + y * rectPitch;
                        float16 xx = new float16(vq.X);
                        float16 yy = new float16(vq.Y);
                        data[srcIndex] = xx.getInternalDat();
                        data[srcIndex + 1] = yy.getInternalDat();
                     }
                  }

                  mD3DPixPackOffsets[l, pass, sub].UnlockRectangle(0);
               }
            }
         }

         

      }

   }


   class ScopeTimer
   {
      DateTime mStartTime;
      string mLabel = null;
      public ScopeTimer(String ident)
      {
         mLabel = ident;
         mStartTime = DateTime.Now;
      }
      ~ScopeTimer()
      {
     //    destroy();
      }
      public void destroy()
      {
         TimeSpan ts = DateTime.Now - mStartTime;
         if (Globals.PRINT_DEBUG_MSG) Debug.Print(mLabel + " : " + ts.Milliseconds + "ms");
      }

   }

}