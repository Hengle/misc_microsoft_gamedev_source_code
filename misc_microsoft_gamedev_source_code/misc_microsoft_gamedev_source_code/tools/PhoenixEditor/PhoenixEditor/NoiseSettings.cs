using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using System.IO;
using System.Drawing.Drawing2D;
using System.Drawing.Imaging;

using Terrain;


namespace PhoenixEditor
{
   public partial class NoiseSettings : Xceed.DockingWindows.ToolWindow //: Form
   {
      public NoiseSettings()
      {
         InitializeComponent();

         Init();
      }

      public void Init()
      {
         mBuffer = new Bitmap(pictureBox1.Width / 4, pictureBox1.Height / 4, System.Drawing.Imaging.PixelFormat.Format24bppRgb);
         mFloats = new float[pictureBox1.Width / 4, pictureBox1.Height / 4];         
         
         mFunction = (BasicNoise) TerrainGlobals.getTerrainFrontEnd().getNoiseFunction();


         this.floatSlider1.Constraint = mFunction.mScaleX;
         this.floatSlider1.ValueName = "Scale X";
         this.floatSlider2.Constraint = mFunction.mScaleY;
         this.floatSlider2.ValueName = "Scale Z";

         this.floatSlider3.Constraint = mFunction.mPersistance;
         this.floatSlider3.ValueName = "Persistance";

         this.floatSlider4.Constraint = mFunction.mOctaves;
         this.floatSlider4.ValueName = "Octaves";
         this.floatSlider4.mbIntStep = true;

         this.floatSlider5.Constraint = mFunction.mFreq;
         this.floatSlider5.ValueName = "Frequency";
         this.floatSlider6.Constraint = mFunction.mAmplitude;
         this.floatSlider6.ValueName = "Amplitude";

      }

      //public NoiseFunction NoiseFunction
      //{
      //   set
      //   {

      //   }
      //   get
      //   {

      //   }
      //}
   


      public void render()
      {
         render(mBuffer);
         pictureBox1.Image = mBuffer;
         pictureBox1.SizeMode = PictureBoxSizeMode.StretchImage;
      }

      BasicNoise mFunction = null;
      Bitmap mBuffer = null;
      float[,] mFloats;

      unsafe public struct PixelData24
      {
         public byte blue;
         public byte green;
         public byte red;
      }

      public void render(Bitmap source)
      {
         float max = float.MinValue;
         float min = float.MaxValue;
         int width = source.Width;
         int height = source.Height;

         for (int x = 0; x < width; x++)
         {
            for (int y = 0; y < height; y++)
            {
               float v = mFunction.compute(x, y);

               if (v > max) max = v;
               if (v < min) min = v;

               mFloats[x, y] = v;
            }

         }

         //adjust range
         float scale = 1 / (max - min);
         float offset = -min * scale;

         Rectangle r = new Rectangle(0, 0, source.Width, source.Height);
         unsafe
         {
            BitmapData sourceData = source.LockBits(r, ImageLockMode.ReadOnly, source.PixelFormat);
            PixelData24* sourceBase = (PixelData24*)sourceData.Scan0;

            for (int x = 0; x < width; x++)
            {
               for (int y = 0; y < height; y++)
               {
                  PixelData24* pSourcePixel = sourceBase + y * width + x;

                  float v = offset + scale * mFloats[x, y];
                  byte val = (byte)(v * 255);

                  pSourcePixel->red = val;
                  pSourcePixel->green = val;
                  pSourcePixel->blue = val;
               }
            }
            source.UnlockBits(sourceData);
         }
      }

      private void timer1_Tick(object sender, EventArgs e)
      {
         render();
      }

      private void AbsCheckBox_CheckedChanged(object sender, EventArgs e)
      {
         mFunction.mbAbs = AbsCheckBox.Checked;
      }

      private void CosCheckBox_CheckedChanged(object sender, EventArgs e)
      {
         mFunction.mbCos = CosCheckBox.Checked;

      }

      private void AlwaysPoscheckBox_CheckedChanged(object sender, EventArgs e)
      {
         mFunction.mbAlwaysPos = AlwaysPoscheckBox.Checked;
      }
   }
}