using System;




namespace Terrain
{

   public interface OldNoiseFunction
   {
      float compute(int x, int y);


   }
   

   public class BasicNoise : OldNoiseFunction
   {
      public ConstrainedValue mScaleX = new ConstrainedValue(0.01f, 10, 0.1f);
      public ConstrainedValue mScaleY = new ConstrainedValue(0.01f, 10, 0.1f);
      public ConstrainedValue mAmplitude = new ConstrainedValue(0.01f, 10, 0.1f);

      public ConstrainedValue mPersistance = new ConstrainedValue(0.05f, 1, 0.5f);
      public ConstrainedValue mOctaves = new ConstrainedValue(1f, 12f, 8f);
      public ConstrainedValue mFreq = new ConstrainedValue(0.1f, 5f, 1f);

      public bool mbCos = false;
      public bool mbAbs = false;
      public bool mbAlwaysPos = false;


      public float compute(int x, int y)
      {
         float inputX = mScaleX.Value * x;
         float inputY = mScaleY.Value * y;
         float noise = 0;
         noise = (float)Perlin.FinalNoise(mPersistance.Value, (int)mOctaves.Value, mFreq.Value, inputX, inputY, mbAbs);
         if (mbCos)
            noise = (float)Math.Cos(inputX + noise);
         if (mbAlwaysPos)
            noise = (float)Math.Abs(noise);


         return noise * mAmplitude.Value;
      }
   }

   public class Perlin
   {
      static public float Noise2d(int x, int y)
      {
         int n;
         n = x + y * 57;
         n = (n << 13) ^ n;
         float res = (float)(1.0 - ((n * (n * n * 15731 + 789221)
                                 + 1376312589) & 0x7fffffff) / 1073741824.0);
         return res;
      }
      static float Interpolate(float x, float y, float a)
      {
         float fac1 = (float)(3 * Math.Pow(1 - a, 2) - 2 * Math.Pow(1 - a, 3));
         float fac2 = (float)(3 * Math.Pow(a, 2) - 2 * Math.Pow(a, 3));

         return x * fac1 + y * fac2; //add the weighted factors
      }

      static public float GetValue(float x, float y)
      {
         int Xint = (int)x;
         int Yint = (int)y;
         float Xfrac = x - Xint;
         float Yfrac = y - Yint;

         float x0y0 = Noise2d(Xint, Yint);  //find the noise values of the four corners
         float x1y0 = Noise2d(Xint + 1, Yint);
         float x0y1 = Noise2d(Xint, Yint + 1);
         float x1y1 = Noise2d(Xint + 1, Yint + 1);

         //interpolate between those values according to the x and y fractions
         float v1 = Interpolate(x0y0, x1y0, Xfrac); //interpolate in x direction (y)
         float v2 = Interpolate(x0y1, x1y1, Xfrac); //interpolate in x direction (y+1)
         float fin = Interpolate(v1, v2, Yfrac);  //interpolate in y direction

         return fin;
      }

      static public float FinalNoise(float x, float y)
      {
         float P = 0.5f;
         int octaves = 8;// 4;
         int freq = 1;
         return FinalNoise(P, octaves, freq, x, y, false);
      }

      static public float FinalNoise(float P, int octaves, float freq, float x, float y, bool abs)
      {
         float total = 0.0f;
         float pers = 1;
         for (int i = 0; i < octaves; i++)
         {
            if (abs)
            {
               total += Math.Abs(GetValue(x * freq, y * freq) * pers);
            }
            else
            {
               total += (GetValue(x * freq, y * freq) * pers);
            }
            freq *= 2;
            pers *= P;
         }
         return total;
      }
   }

}