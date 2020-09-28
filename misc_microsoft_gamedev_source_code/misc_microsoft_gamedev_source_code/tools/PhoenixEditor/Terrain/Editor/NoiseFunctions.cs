using System;

namespace NoiseGeneration
{
   //general
   static public class Noise
   {
      const int X_NOISE_GEN = 1619;
      const int Y_NOISE_GEN = 31337;
      const int Z_NOISE_GEN = 6971;
      const int SEED_NOISE_GEN = 1013;
      const int SHIFT_NOISE_GEN = 8;


      public enum eNoiseQuality
      {

         /// Generates coherent noise quickly.  When a coherent-noise function with
         /// this quality setting is used to generate a bump-map image, there are
         /// noticeable "creasing" artifacts in the resulting image.  This is
         /// because the derivative of that function is discontinuous at integer
         /// boundaries.
         cQuality_Fast = 0,

         /// Generates standard-quality coherent noise.  When a coherent-noise
         /// function with this quality setting is used to generate a bump-map
         /// image, there are some minor "creasing" artifacts in the resulting
         /// image.  This is because the second derivative of that function is
         /// discontinuous at integer boundaries.
         cQuality_Std = 1,

         /// Generates the best-quality coherent noise.  When a coherent-noise
         /// function with this quality setting is used to generate a bump-map
         /// image, there are no "creasing" artifacts in the resulting image.  This
         /// is because the first and second derivatives of that function are
         /// continuous at integer boundaries.
         cQuality_Best = 2

      };

      /// A <i>gradient</i>-noise function generates better-quality noise than a
      /// <i>value</i>-noise function.  Most noise modules use gradient noise for
      /// this reason, although it takes much longer to calculate.
      /// 
      /// Generates a gradient-coherent-noise value from the coordinates of a
      /// three-dimensional input value.
      ///
      /// @param x The @a x coordinate of the input value.
      /// @param y The @a y coordinate of the input value.
      /// @param z The @a z coordinate of the input value.
      /// @param seed The random number seed.
      /// @param noiseQuality The quality of the coherent-noise.
      /// The return value ranges from -1.0 to +1.0.
      static public double GradientCoherentNoise3D(double x, double y, double z, int seed, eNoiseQuality noiseQuality)
      {
         // Create a unit-length cube aligned along an integer boundary.  This cube
         // surrounds the input point.
         int x0 = (int)(Math.Floor(x));
         int x1 = x0 + 1;
         int y0 = (int)(Math.Floor(y));
         int y1 = y0 + 1;
         int z0 = (int)(Math.Floor(z));
         int z1 = z0 + 1;

         // Map the difference between the coordinates of the input value and the
         // coordinates of the cube's outer-lower-left vertex onto an S-curve.
         double xs = 0;
         double ys = 0;
         double zs = 0;
         switch (noiseQuality)
         {
            case eNoiseQuality.cQuality_Fast:
               xs = (x - (double)x0);
               ys = (y - (double)y0);
               zs = (z - (double)z0);
               break;
            case eNoiseQuality.cQuality_Std:
               xs = Interp.SCurve3(x - (double)x0);
               ys = Interp.SCurve3(y - (double)y0);
               zs = Interp.SCurve3(z - (double)z0);
               break;
            case eNoiseQuality.cQuality_Best:
               xs = Interp.SCurve5(x - (double)x0);
               ys = Interp.SCurve5(y - (double)y0);
               zs = Interp.SCurve5(z - (double)z0);
               break;
         }

         // Now calculate the noise values at each vertex of the cube.  To generate
         // the coherent-noise value at the input point, interpolate these eight
         // noise values using the S-curve value as the interpolant (trilinear
         // interpolation.)
         double n0, n1, ix0, ix1, iy0, iy1;
         n0 = GradientNoise3D(x, y, z, x0, y0, z0, seed);
         n1 = GradientNoise3D(x, y, z, x1, y0, z0, seed);
         ix0 = Interp.LinearInterp(n0, n1, xs);
         n0 = GradientNoise3D(x, y, z, x0, y1, z0, seed);
         n1 = GradientNoise3D(x, y, z, x1, y1, z0, seed);
         ix1 = Interp.LinearInterp(n0, n1, xs);
         iy0 = Interp.LinearInterp(ix0, ix1, ys);
         n0 = GradientNoise3D(x, y, z, x0, y0, z1, seed);
         n1 = GradientNoise3D(x, y, z, x1, y0, z1, seed);
         ix0 = Interp.LinearInterp(n0, n1, xs);
         n0 = GradientNoise3D(x, y, z, x0, y1, z1, seed);
         n1 = GradientNoise3D(x, y, z, x1, y1, z1, seed);
         ix1 = Interp.LinearInterp(n0, n1, xs);
         iy1 = Interp.LinearInterp(ix0, ix1, ys);

         return Interp.LinearInterp(iy0, iy1, zs);
      }
      /// Generates a gradient-noise value from the coordinates of a
      /// three-dimensional input value and the integer coordinates of a
      /// nearby three-dimensional value.
      ///
      /// @param fx The floating-point @a x coordinate of the input value.
      /// @param fy The floating-point @a y coordinate of the input value.
      /// @param fz The floating-point @a z coordinate of the input value.
      /// @param ix The integer @a x coordinate of a nearby value.
      /// @param iy The integer @a y coordinate of a nearby value.
      /// @param iz The integer @a z coordinate of a nearby value.
      /// @param seed The random number seed.
      /// @pre The difference between @a fx and @a ix must be less than or equal to one.
      /// @pre The difference between @a fy and @a iy must be less than or equal to one.
      /// @pre The difference between @a fz and @a iz must be less than or equal to one.
      /// The return value ranges from -1.0 to +1.0.
      static public double GradientNoise3D(double fx, double fy, double fz, int ix, int iy, int iz, int seed)
      {
         // Randomly generate a gradient vector given the integer coordinates of the
         // input value.  This implementation generates a random number and uses it
         // as an index into a normalized-vector lookup table.
         int vectorIndex = (int)((
             X_NOISE_GEN * ix
           + Y_NOISE_GEN * iy
           + Z_NOISE_GEN * iz
           + SEED_NOISE_GEN * seed)
           & 0xffffffff);
         vectorIndex ^= (vectorIndex >> SHIFT_NOISE_GEN);
         vectorIndex &= 0xff;

         double xvGradient = Utils.g_randomVectors[(vectorIndex << 2)];
         double yvGradient = Utils.g_randomVectors[(vectorIndex << 2) + 1];
         double zvGradient = Utils.g_randomVectors[(vectorIndex << 2) + 2];

         // Set up us another vector equal to the distance between the two vectors
         // passed to this function.
         double xvPoint = (fx - (double)ix);
         double yvPoint = (fy - (double)iy);
         double zvPoint = (fz - (double)iz);

         // Now compute the dot product of the gradient vector with the distance
         // vector.  The resulting value is gradient noise.  Apply a scaling value
         // so that this noise value ranges from -1.0 to 1.0.
         return ((xvGradient * xvPoint)
           + (yvGradient * yvPoint)
           + (zvGradient * zvPoint)) * 2.12;
      }

      /// Generates an integer-noise value from the coordinates of a three-dimensional input value.
      /// @param x The integer @a x coordinate of the input value.
      /// @param y The integer @a y coordinate of the input value.
      /// @param z The integer @a z coordinate of the input value.
      /// @param seed A random number seed.
      /// The return value ranges from 0 to 2147483647.
      static public int IntValueNoise3D(int x, int y, int z, int seed)
      {
         // All constants are primes and must remain prime in order for this noise
         // function to work correctly.
         int n = (int)((
             X_NOISE_GEN * x
           + Y_NOISE_GEN * y
           + Z_NOISE_GEN * z
           + SEED_NOISE_GEN * seed)
           & 0x7fffffff);
         n = (n >> 13) ^ n;
         return (int)((n * (n * n * 60493 + 19990303) + 1376312589) & 0x7fffffff);
      }
      static public double ValueNoise3D(int x, int y, int z, int seed)
      {
         return 1.0 - ((double)IntValueNoise3D(x, y, z, seed) / 1073741824.0);
      }
   }

   //perlin based
   public class Perlin
   {
      /// Noise module that outputs 3-dimensional Perlin noise.

      /// Perlin noise is the sum of several coherent-noise functions of
      /// ever-increasing frequencies and ever-decreasing amplitudes.
      /// An important property of Perlin noise is that a small change in the
      /// input value will produce a small change in the output value, while a
      /// large change in the input value will produce a random change in the
      /// output value.
      /// 
      /// This noise module outputs Perlin-noise values that usually range from
      /// -1.0 to +1.0, but there are no guarantees that all output values will
      /// exist within that range.
      /// 
      /// The persistence value determines how quickly the amplitudes diminish
      /// for successive octaves.  The amplitude of the first octave is 1.0.
      /// The amplitude of each subsequent octave is equal to the product of the
      /// previous octave's amplitude and the persistence value.  So a
      /// persistence value of 0.5 sets the amplitude of the first octave to
      /// 1.0; the second, 0.5; the third, 0.25; etc.
      /// 
      /// The lacunarity specifies the frequency multipler between successive
      /// octaves.
      /// The effect of modifying the lacunarity is subtle; you may need to play
      /// with the lacunarity value to determine the effects.  For best results,
      /// set the lacunarity to a number between 1.5 and 3.5.
      public ClampedDouble mFrequency = new ClampedDouble(0.001, 0.15, 0.05);
      public ClampedDouble mLacunarity = new ClampedDouble(1.5, 3.5, 2.0);   //[1.5,3.5]
      public ClampedDouble mPersistence = new ClampedDouble(0.0, 1.0, 0.5); //[0.0,1.0]
      public ClampedInt mOctaveCount = new ClampedInt(0, 30, 6);
      public Noise.eNoiseQuality mQuality = Noise.eNoiseQuality.cQuality_Fast;
      public int mSeed = 0;
      public int mMaxOctave = 30;

      //[-1,1]
      public double getValue(double x, double y, double z)
      {
         double value = 0.0;
         double signal = 0.0;
         double curPersistence = 1.0;
         double nx, ny, nz;
         int seed;

         x *= mFrequency.Value;
         y *= mFrequency.Value;
         z *= mFrequency.Value;

         for (int curOctave = 0; curOctave < mOctaveCount.Value; curOctave++)
         {

            // Make sure that these floating-point values have the same range as a 32-
            // bit integer so that we can pass them to the coherent-noise functions.
            nx = Utils.MakeInt32Range(x);
            ny = Utils.MakeInt32Range(y);
            nz = Utils.MakeInt32Range(z);

            // Get the coherent-noise value from the input value and add it to the
            // final result.
            seed = (int)((mSeed + curOctave) & 0xffffffff);
            signal = Noise.GradientCoherentNoise3D(nx, ny, nz, seed, mQuality);
            value += signal * curPersistence;

            // Prepare the next octave.
            x *= mLacunarity.Value;
            y *= mLacunarity.Value;
            z *= mLacunarity.Value;
            curPersistence *= mPersistence.Value;
         }

         return value;
      }

   }

   public class Voronoi
   {
      /// In mathematics, a Voronoi cell is a region containing all the
      /// points that are closer to a specific seed point than to any
      /// other seed point.  These cells mesh with one another, producing
      /// polygon-like formations.
      /// By default, this noise module randomly places a seed point within
      /// each unit cube.  By modifying the frequency of the seed points,
      /// an application can change the distance between seed points.  The
      /// higher the frequency, the closer together this noise module places
      /// the seed points, which reduces the size of the cells.  
      /// This noise module assigns each Voronoi cell with a random constant
      /// value from a coherent-noise function.  The displacement value
      /// controls the range of random values to assign to each cell.  The
      /// range of random values is +/- the displacement value.  
      /// 
      /// Voronoi cells are often used to generate cracked-mud terrain
      /// formations or crystal-like textures
      /// 
      public ClampedDouble mDisplacement = new ClampedDouble(0, 1.0, 1.0);
      public ClampedDouble mFrequency = new ClampedDouble(0.01, 0.5, 0.05);
      public int mSeed = 0;
      public bool mEnableDistance = true;


      public double getValue(double x, double y, double z)
      {
         // This method could be more efficient by caching the seed values.  Fix
         // later.

         x *= mFrequency.Value;
         y *= mFrequency.Value;
         z *= mFrequency.Value;

         int xInt = (int)(Math.Floor(x));
         int yInt = (int)(Math.Floor(y));
         int zInt = (int)(Math.Floor(z));

         double minDist = 2147483647.0;
         double xCandidate = 0;
         double yCandidate = 0;
         double zCandidate = 0;

         // Inside each unit cube, there is a seed point at a random position.  Go
         // through each of the nearby cubes until we find a cube with a seed point
         // that is closest to the specified position.
         for (int zCur = zInt - 2; zCur <= zInt + 2; zCur++)
         {
            for (int yCur = yInt - 2; yCur <= yInt + 2; yCur++)
            {
               for (int xCur = xInt - 2; xCur <= xInt + 2; xCur++)
               {

                  // Calculate the position and distance to the seed point inside of
                  // this unit cube.
                  double xPos = xCur + Noise.ValueNoise3D(xCur, yCur, zCur, mSeed);
                  double yPos = yCur + Noise.ValueNoise3D(xCur, yCur, zCur, mSeed + 1);
                  double zPos = zCur + Noise.ValueNoise3D(xCur, yCur, zCur, mSeed + 2);
                  double xDist = xPos - x;
                  double yDist = yPos - y;
                  double zDist = zPos - z;
                  double dist = xDist * xDist + yDist * yDist + zDist * zDist;

                  if (dist < minDist)
                  {
                     // This seed point is closer to any others found so far, so record
                     // this seed point.
                     minDist = dist;
                     xCandidate = xPos;
                     yCandidate = yPos;
                     zCandidate = zPos;
                  }
               }
            }
         }

         double value;
         if (mEnableDistance)
         {
            // Determine the distance to the nearest seed point.
            double xDist = xCandidate - x;
            double yDist = yCandidate - y;
            double zDist = zCandidate - z;
            value = (Math.Sqrt(xDist * xDist + yDist * yDist + zDist * zDist)
              ) * Utils.SQRT_3 - 1.0;
         }
         else
         {
            value = 0.0;
         }

         // Return the calculated distance with the displacement value applied.
         return value + (mDisplacement.Value * (double)Noise.ValueNoise3D(
           (int)(Math.Floor(xCandidate)),
           (int)(Math.Floor(yCandidate)),
           (int)(Math.Floor(zCandidate)), mSeed));
      }
   }

   public class Billow
   {

      /// This noise module generates "billowy" noise suitable for clouds and
      /// rocks.
      /// 
      public ClampedDouble mFrequency = new ClampedDouble(0.001, 0.15, 0.05);
      public ClampedDouble mLacunarity = new ClampedDouble(1.5, 3.5, 2.0);   //[1.5,3.5]
      public ClampedDouble mPersistence = new ClampedDouble(0.0, 1.0, 0.5); //[0.0,1.0]
      public ClampedInt mOctaveCount = new ClampedInt(0, 30, 6);
      public Noise.eNoiseQuality mQuality = Noise.eNoiseQuality.cQuality_Fast;
      public int mSeed = 0;
      public int mMaxOctave = 30;


      public double getValue(double x, double y, double z)
      {
         double value = 0.0;
         double signal = 0.0;
         double curPersistence = 1.0;
         double nx, ny, nz;
         int seed;

         x *= mFrequency.Value;
         y *= mFrequency.Value;
         z *= mFrequency.Value;

         for (int curOctave = 0; curOctave < mOctaveCount.Value; curOctave++)
         {

            // Make sure that these floating-point values have the same range as a 32-
            // bit integer so that we can pass them to the coherent-noise functions.
            nx = Utils.MakeInt32Range(x);
            ny = Utils.MakeInt32Range(y);
            nz = Utils.MakeInt32Range(z);

            // Get the coherent-noise value from the input value and add it to the
            // final result.
            seed = (int)((mSeed + curOctave) & 0xffffffff);
            signal = Noise.GradientCoherentNoise3D(nx, ny, nz, seed, mQuality);
            signal = 2.0 * Math.Abs(signal) - 1.0;
            value += signal * curPersistence;

            // Prepare the next octave.
            x *= mLacunarity.Value;
            y *= mLacunarity.Value;
            z *= mLacunarity.Value;
            curPersistence *= mPersistence.Value;
         }
         value += 0.5;

         return value;
      }
   }


   //generic
   public class Spheres
   {
      /// The first sphere has a radius of 1.0.  Each subsequent sphere has a
      /// radius that is 1.0 unit larger than the previous sphere.
      ///
      /// The output value from this noise module is determined by the distance
      /// between the input value and the the nearest spherical surface.  The
      /// input values that are located on a spherical surface are given the
      /// output value 1.0 and the input values that are equidistant from two
      /// spherical surfaces are given the output value -1.0.
      public ClampedDouble mFrequency = new ClampedDouble(0.001, 0.15, 0.05);

      public double getValue(double x, double y, double z)
      {
         x *= mFrequency.Value;
         y *= mFrequency.Value;
         z *= mFrequency.Value;

         double distFromCenter = Math.Sqrt(x * x + y * y + z * z);
         double distFromSmallerSphere = distFromCenter - Math.Floor(distFromCenter);
         double distFromLargerSphere = 1.0 - distFromSmallerSphere;
         double nearestDist = Math.Min(distFromSmallerSphere, distFromLargerSphere);
         return 1.0 - (nearestDist * 4.0); // Puts it in the -1.0 to +1.0 range.
      }
   }

   public class Cylindars
   {
      public ClampedDouble mFrequency = new ClampedDouble(0.001, 1, 0.002);//0.001, 0.15, 0.05);

      public double getValue(double x, double y, double z)
      {
         x *= mFrequency.Value;
         z *= mFrequency.Value;

         double distFromCenter = Math.Sqrt(x * x + z * z);
         double distFromSmallerSphere = distFromCenter - Math.Floor(distFromCenter);
         double distFromLargerSphere = 1.0 - distFromSmallerSphere;
         double nearestDist = Math.Min(distFromSmallerSphere, distFromLargerSphere);
         return 1.0 - (nearestDist * 4.0); // Puts it in the -1.0 to +1.0 range.
      }
   }


   //fractal based
   public class RigedMultiFractal
   {
      // Multifractal code originally written by F. Kenton "Doc Mojo" Musgrave, 1998.  
      /// This noise module, heavily based on the Perlin-noise module, generates
      /// ridged-multifractal noise.  Ridged-multifractal noise is generated in
      /// much of the same way as Perlin noise, except the output of each octave
      /// is modified by an absolute-value function.  Modifying the octave
      /// values in this way produces ridge-like formations.
      /// Ridged-multifractal noise does not use a persistence value.  This is
      /// because the persistence values of the octaves are based on the values
      /// generated from from previous octaves, creating a feedback loop (or
      /// that's what it looks like after reading the code.)
      /// This noise module outputs ridged-multifractal-noise values that
      /// usually range from -1.0 to +1.0, but there are no guarantees that all
      /// output values will exist within that range.

      public ClampedDouble mFrequency = new ClampedDouble(0.001, 0.05, 0.009);
      public ClampedDouble mLacunarity = new ClampedDouble(1.5, 3.5, 2.0);   //[1.5,3.5]
      public ClampedInt mOctaveCount = new ClampedInt(0, 30, 6);
      public Noise.eNoiseQuality mQuality = Noise.eNoiseQuality.cQuality_Std;
      public int mSeed = 0;
      public int mMaxOctave = 30;
      private double[] mSpectralWeights = null;
      public double mOffset = 1.0;
      public double mGain = 2.0;

      //a custom exponent definer.
      public void CalcSpectralWeights(double H) //H == 1.0
      {
         if (mSpectralWeights == null)
            mSpectralWeights = new double[mMaxOctave];

         double frequency = 1.0;
         for (int i = 0; i < mMaxOctave; i++)
         {
            mSpectralWeights[i] = Math.Pow(frequency, -H);
            frequency *= mLacunarity.Value;
         }
      }


      public double getValue(double x, double y, double z)
      {
         if (mSpectralWeights == null)
            CalcSpectralWeights(1.0);

         x *= mFrequency.Value;
         y *= mFrequency.Value;
         z *= mFrequency.Value;

         double signal = 0.0;
         double value = 0.0;
         double weight = 1.0;

         for (int curOctave = 0; curOctave < mOctaveCount.Value; curOctave++)
         {

            // Make sure that these floating-point values have the same range as a 32-
            // bit integer so that we can pass them to the coherent-noise functions.
            double nx, ny, nz;
            nx = Utils.MakeInt32Range(x);
            ny = Utils.MakeInt32Range(y);
            nz = Utils.MakeInt32Range(z);

            // Get the coherent-noise value.
            int seed = (int)((mSeed + curOctave) & 0x7fffffff);
            signal = Noise.GradientCoherentNoise3D(nx, ny, nz, seed, mQuality);

            // Make the ridges.
            signal = Math.Abs(signal);
            signal = mOffset - signal;

            // Square the signal to increase the sharpness of the ridges.
            signal *= signal;

            // The weighting from the previous octave is applied to the signal.
            // Larger values have higher weights, producing sharp points along the
            // ridges.
            signal *= weight;

            // Weight successive contributions by the previous signal.
            weight = signal * mGain;
            if (weight > 1.0)
            {
               weight = 1.0;
            }
            if (weight < 0.0)
            {
               weight = 0.0;
            }

            // Add the signal to the output value.
            value += (signal * mSpectralWeights[curOctave]);

            // Go to the next octave.
            x *= mLacunarity.Value;
            y *= mLacunarity.Value;
            z *= mLacunarity.Value;
         }

         return (value * 1.25) - 1.0;
      }
   }

   public class fBm
   {
      public ClampedDouble mFrequency = new ClampedDouble(0.001, 0.05, 0.009);
      public ClampedDouble mLacunarity = new ClampedDouble(1.5, 3.5, 2.0);   //[1.5,3.5]
      public ClampedInt mOctaveCount = new ClampedInt(0, 30, 6);
      public Noise.eNoiseQuality mQuality = Noise.eNoiseQuality.cQuality_Std;
      public int mSeed = 0;
      public int mMaxOctave = 30;
      private double[] mSpectralWeights = null;
      public double mOffset = 1.0;
      public double mGain = 2.0;

      //a custom exponent definer.
      public void CalcSpectralWeights(double H) //H == 1.0
      {
         if (mSpectralWeights == null)
            mSpectralWeights = new double[mMaxOctave];

         double frequency = 1.0;
         for (int i = 0; i < mMaxOctave; i++)
         {
            mSpectralWeights[i] = Math.Pow(frequency, -H);
            frequency *= mLacunarity.Value;
         }
      }

      public double getValue(double x, double y, double z)
      {
         if (mSpectralWeights == null)
            CalcSpectralWeights(1.0);

         x *= mFrequency.Value;
         y *= mFrequency.Value;
         z *= mFrequency.Value;

         double signal = 0.0;
         double value = 0.0;


         for (int curOctave = 0; curOctave < mOctaveCount.Value; curOctave++)
         {

            // Make sure that these floating-point values have the same range as a 32-
            // bit integer so that we can pass them to the coherent-noise functions.
            double nx, ny, nz;
            nx = Utils.MakeInt32Range(x);
            ny = Utils.MakeInt32Range(y);
            nz = Utils.MakeInt32Range(z);

            // Get the coherent-noise value.
            int seed = (int)((mSeed + curOctave) & 0x7fffffff);
            signal = Noise.GradientCoherentNoise3D(nx, ny, nz, seed, mQuality);

            // Add the signal to the output value.
            value += (signal * mSpectralWeights[curOctave]);

            // Go to the next octave.
            x *= mLacunarity.Value;
            y *= mLacunarity.Value;
            z *= mLacunarity.Value;
         }

         return value;
      }
   }



}