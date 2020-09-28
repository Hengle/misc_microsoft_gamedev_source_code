
using Microsoft.DirectX;
using Microsoft.DirectX.Direct3D;
using System.Collections.Generic;
using System;
using System.Drawing;
using System.Drawing.Imaging;


using EditorCore;
using Rendering;

namespace Terrain
{

   public class TerrainFilter
   {
      public virtual void init() { }
      public virtual void destroy() { }

      //mask creation values
      public enum eFilterMaskCreation
      {
         cNoMask = 0,
         cMaskAndErosion = 1,
         cMaskOnly = 2
      };
      public eFilterMaskCreation mMaskCreationType = eFilterMaskCreation.cNoMask;



      protected virtual void MakeUndoData(bool onlyToMaskedVerts)
      {
         if (onlyToMaskedVerts == true)
         {
            TerrainGlobals.getEditor().PushSelectedDetailPoints(true);
         }
         else
         {
            TerrainGlobals.getEditor().PushWholeFrigginMap(true);
         }

      }

      public virtual void apply(bool onlyToMaskedVerts)
      {
         updateTerrainVisuals(onlyToMaskedVerts);
      }

      protected bool isVertSelected(int x, int z, ref float selAmt)
      {
         return isVertSelected(x + TerrainGlobals.getTerrain().getNumXVerts() * z, ref selAmt);
      }
      protected bool isVertSelected(long index, ref float selAmt)
      {
         return Masking.isPointSelected(index, ref selAmt);
      }

      protected void updateTerrainVisuals(bool applyToMaskOnly)
      {
         BTerrain.computeBasis(TerrainGlobals.getEditor().getDetailPoints(), TerrainGlobals.getEditor().getNormals(),
                                                   TerrainGlobals.getTerrain().getTileScale(), TerrainGlobals.getTerrain().getNumXVerts(),
                                                   0, TerrainGlobals.getTerrain().getNumXVerts(), 0, TerrainGlobals.getTerrain().getNumXVerts());
         TerrainGlobals.getTerrain().getQuadNodeRoot().clearVisibleDatHandle();
         List<BTerrainQuadNode> nodes = new List<BTerrainQuadNode>();

         if (applyToMaskOnly)
         {
            TerrainGlobals.getTerrain().getQuadNodeRoot().getTileBoundsIntersection(nodes, Masking.mCurrSelectionMaskExtends.minX, Masking.mCurrSelectionMaskExtends.maxX,
                                                                                     Masking.mCurrSelectionMaskExtends.minZ, Masking.mCurrSelectionMaskExtends.maxZ);

            TerrainGlobals.getEditor().PushSelectedDetailPoints(false);
         }
         else
         {
            TerrainGlobals.getTerrain().getQuadNodeRoot().getTileBoundsIntersection(nodes, 0, TerrainGlobals.getTerrain().getNumXVerts(),
                                                                                     0, TerrainGlobals.getTerrain().getNumXVerts());
            TerrainGlobals.getEditor().PushWholeFrigginMap(false);
         }



         for (int i = 0; i < nodes.Count; i++)
         {
            nodes[i].mDirty = true;
         }
         TerrainGlobals.getTerrain().rebuildDirtyPostDeform(BRenderDevice.getDevice());

      }
      protected float blend(float a, float b, float i)
      {
         return (a * i) + ((1 - i) * b);
      }

      public virtual void previewToBitmap(ref Bitmap bmp)
      {
      }

   }
   unsafe public struct PixelData32
   {

      public byte R;
      public byte G;
      public byte B;
      public byte A;
   }
   class int2
   {
      public int2(int i, int j)
      {
         a = i; b = j;
      }
      public int a, b;
   }
   public enum eFilterOperation
   {
      Add,
      Replace,
      Subtract
   }

   public class MountainRidgeTerrainFilter : TerrainFilter
   {

      private float mScaleHeight = 5.0f;
      public float ScaleHeight { get { return mScaleHeight; } set { mScaleHeight = value; } }
      
      private int mDetail = 28;
      public int Detail { get { return mDetail; } set { mDetail = value; } }

      private float mFrequency = 0.005f;
      public float Frequency { get { return mFrequency; } set { mFrequency = value; } }

      private int mMacroSeed = 0;
      public int MacroSeed { get { return mMacroSeed; } set { mMacroSeed = value; } }

      private int mMicroSeed = 1;
      public int MicroSeed { get { return mMicroSeed; } set { mMicroSeed = value; } }

      private eFilterOperation mAddReplaceSubtract = eFilterOperation.Replace;   //add=0, replace = 1, subtract=2
      public eFilterOperation AddReplaceSubtract { get { return mAddReplaceSubtract; } set { mAddReplaceSubtract = value; } }

      private float mLacunarity = 1.5f;
      public float Lacunarity { get { return mLacunarity; } set { mLacunarity = value; } }

      public override void init()
      {
      }
      public override void destroy()
      {
      }
      public override void apply(bool onlyToMaskedVerts)
      {
         //if (mMaskCreationType != eFilterMaskCreation.cMaskOnly)
         //   MakeUndoData(onlyToMaskedVerts);

         //NoiseGeneration.RigedMultiFractal.mFrequency.Value = mFrequency;
         //NoiseGeneration.RigedMultiFractal.mLacunarity.Value = mLacunarity;// new ClampedDouble(1.5, 3.5, 2.0);   //[1.5,3.5]
         //NoiseGeneration.RigedMultiFractal.mOctaveCount.Value = mDetail;
         //NoiseGeneration.RigedMultiFractal.mSeed = mMicroSeed;

         //int x = TerrainGlobals.getTerrain().getNumXVerts();
         //float scale = TerrainGlobals.getTerrain().getTileScale();
         //for (int i = 0; i < x; i++)
         //{
         //   for (int j = 0; j < x; j++)
         //   {
         //      float amt =1.0f;
         //      if(onlyToMaskedVerts)
         //      {
         //         if (!isVertSelected(i, j, ref amt))
         //            continue;
         //      }

         //      int indx = i + (j * x);
         //      Vector3 p = TerrainGlobals.getTerrain().getPostDeformPos(i, j);
         //      double height = 0;


         //      if(mMacroSeed==0)                  height = NoiseGeneration.RigedMultiFractal.getValue(i, j, TerrainGlobals.getEditor().getDetailPoints()[indx].Y);
         //      else if (mMacroSeed == 1)          height = NoiseGeneration.RigedMultiFractal.getValue(i, TerrainGlobals.getEditor().getDetailPoints()[indx].Y, j);
         //      else if (mMacroSeed == 2)          height = NoiseGeneration.RigedMultiFractal.getValue(TerrainGlobals.getEditor().getDetailPoints()[indx].Y, i, j);

         //      if(mMaskCreationType != eFilterMaskCreation.cNoMask)
         //         Masking.addSelectedVert(j, i, (float)height);
               
         //      height *= mScaleHeight;

         //      if (mMaskCreationType != eFilterMaskCreation.cMaskOnly)
         //         TerrainGlobals.getEditor().getDetailPoints()[indx].Y = addReplaceSubtractResult(TerrainGlobals.getEditor().getDetailPoints()[indx].Y, (float)height, mAddReplaceSubtract,amt);
         //   }
         //}
         //base.apply(onlyToMaskedVerts);
      }

      public override void previewToBitmap(ref Bitmap bmp)
      {
         //NoiseGeneration.RigedMultiFractal.mFrequency.Value = mFrequency;
         //NoiseGeneration.RigedMultiFractal.mLacunarity.Value = mLacunarity;// new ClampedDouble(1.5, 3.5, 2.0);   //[1.5,3.5]
         //NoiseGeneration.RigedMultiFractal.mOctaveCount.Value = mDetail;
         //NoiseGeneration.RigedMultiFractal.mSeed = mMicroSeed;

         //Rectangle r = new Rectangle(0, 0, bmp.Width, bmp.Height);
         //unsafe
         //{
         //   BitmapData sourceData = bmp.LockBits(r, ImageLockMode.ReadOnly, bmp.PixelFormat);
         //   PixelData32* sourceBase = (PixelData32*)sourceData.Scan0;
         //   int width = bmp.Width;
         //   int height = bmp.Height;
         //   for (int x = 0; x < width; x++)
         //   {
         //      for (int y = 0; y < height; y++)
         //      {
         //         PixelData32* pSourcePixel = sourceBase + y * width + x;

         //         double H = 0;
         //         if (mMacroSeed == 0) H = NoiseGeneration.RigedMultiFractal.getValue(x, y, 0);
         //         else if (mMacroSeed == 1) H = NoiseGeneration.RigedMultiFractal.getValue(x, 0, y);
         //         else if (mMacroSeed == 2) H = NoiseGeneration.RigedMultiFractal.getValue(0, x, y);

         //         H = (float)BMathLib.Clamp((float)H, 0.0f, 1.0f);

         //         Byte val = (byte)(H * 255);
         //         pSourcePixel->A = 255;
         //         pSourcePixel->R = val;
         //         pSourcePixel->G = val;
         //         pSourcePixel->B = val;
         //      }
         //   }
         //   bmp.UnlockBits(sourceData);


         //}
      }

      protected float blend(float a, float b, float i)
      {
         return (a * i) + ((1 - i) * b);
      }
      protected float addReplaceSubtractResult(float orig, float input, eFilterOperation ars, float influenceAmt)
      {
         if (ars == eFilterOperation.Add)
            return blend(orig + input, orig, influenceAmt);
         else if (ars == eFilterOperation.Replace)
            return blend( input, orig, influenceAmt);
         else if (ars == eFilterOperation.Subtract)
            return blend( orig - input, orig, influenceAmt);

         return 0;
      }
   }

   public class BillowTerrainFilter : TerrainFilter
   {

      public float mScaleHeight = 5.0f;
      public int mDetail = 28;
      public float mFrequency = 0.005f;
      public int mMacroSeed = 0;
      public int mMicroSeed = 1;
      public int mAddReplaceSubtract = 1;   //add=0, replace = 1, subtract=2
      public float mLacunarity = 1.5f;

      public override void init()
      {
      }
      public override void destroy()
      {
      }
      public override void apply(bool onlyToMaskedVerts)
      {
         if (mMaskCreationType != eFilterMaskCreation.cMaskOnly)
            MakeUndoData(onlyToMaskedVerts);

         NoiseGeneration.Billow bb = new NoiseGeneration.Billow();
         bb.mFrequency.Value = mFrequency;
         bb.mLacunarity.Value = mLacunarity;// new ClampedDouble(1.5, 3.5, 2.0);   //[1.5,3.5]
         bb.mOctaveCount.Value = mDetail;
         bb.mSeed = mMicroSeed;

         int x = TerrainGlobals.getTerrain().getNumXVerts();
         float scale = TerrainGlobals.getTerrain().getTileScale();
         
         for (int i = 0; i < x; i++)
         {
            for (int j = 0; j < x; j++)
            {
               float amt = 1.0f;
               if (onlyToMaskedVerts)
               {
                  if (!isVertSelected(i, j, ref amt))
                     continue;
               }

               int indx = i + (j * x);
               Vector3 p = TerrainGlobals.getTerrain().getPostDeformPos(i, j);
               double height = 0;


               if (mMacroSeed == 0) height = bb.getValue(i, j, TerrainGlobals.getEditor().getDetailPoints()[indx].Y);
               else if (mMacroSeed == 1) height = bb.getValue(i, TerrainGlobals.getEditor().getDetailPoints()[indx].Y, j);
               else if (mMacroSeed == 2) height = bb.getValue(TerrainGlobals.getEditor().getDetailPoints()[indx].Y, i, j);

               if (mMaskCreationType != eFilterMaskCreation.cNoMask)
                  Masking.addSelectedVert(j, i, (float)height);

               height *= mScaleHeight;

               if (mMaskCreationType != eFilterMaskCreation.cMaskOnly)
                  TerrainGlobals.getEditor().getDetailPoints()[indx].Y = addReplaceSubtractResult(TerrainGlobals.getEditor().getDetailPoints()[indx].Y, (float)height, mAddReplaceSubtract, amt);
            }
         }
         base.apply(onlyToMaskedVerts);
      }

      public override void previewToBitmap(ref Bitmap bmp)
      {
         NoiseGeneration.Billow bb = new NoiseGeneration.Billow();
         bb.mFrequency.Value = mFrequency;
         bb.mLacunarity.Value = mLacunarity;// new ClampedDouble(1.5, 3.5, 2.0);   //[1.5,3.5]
         bb.mOctaveCount.Value = mDetail;
         bb.mSeed = mMicroSeed;

         Rectangle r = new Rectangle(0, 0, bmp.Width, bmp.Height);
         unsafe
         {
            BitmapData sourceData = bmp.LockBits(r, ImageLockMode.ReadOnly, bmp.PixelFormat);
            PixelData32* sourceBase = (PixelData32*)sourceData.Scan0;
            int width = bmp.Width;
            int height = bmp.Height;
            for (int x = 0; x < width; x++)
            {
               for (int y = 0; y < height; y++)
               {
                  PixelData32* pSourcePixel = sourceBase + y * width + x;

                  double H = 0;
                  if (mMacroSeed == 0) H = bb.getValue(x, y, 0);
                  else if (mMacroSeed == 1) H = bb.getValue(x, 0, y);
                  else if (mMacroSeed == 2) H = bb.getValue(0, x, y);

                  H = (float)BMathLib.Clamp((float)H, 0.0f, 1.0f);

                  Byte val = (byte)(H * 255);
                  pSourcePixel->A = 255;
                  pSourcePixel->R = val;
                  pSourcePixel->G = val;
                  pSourcePixel->B = val;
               }
            }
            bmp.UnlockBits(sourceData);


         }
      }

      protected float blend(float a, float b, float i)
      {
         return (a * i) + ((1 - i) * b);
      }
      protected float addReplaceSubtractResult(float orig, float input, int ars, float influenceAmt)
      {
         if (ars == 0)
            return blend(orig + input, orig, influenceAmt);
         else if (ars == 1)
            return blend(input, orig, influenceAmt);
         else if (ars == 2)
            return blend(orig - input, orig, influenceAmt);

         return 0;
      }
   }

   public class VoronoiTerrainFilter : TerrainFilter
   {
      public float mScaleHeight = 5.0f;
      public float mDisplacement = 1.0f;
      public float mFrequency = 0.05f;
      public int mMacroSeed = 1;
      public int mMicroSeed = 1;
      public int mAddReplaceSubtract = 1;   //add=0, replace = 1, subtract=2
      

      public override void init()
      {
      }
      public override void destroy()
      {
      }
      public override void apply(bool onlyToMaskedVerts)
      {
         //if (mMaskCreationType != eFilterMaskCreation.cMaskOnly)
         //   MakeUndoData(onlyToMaskedVerts);

         //NoiseGeneration.Voronoi.mFrequency.Value = mFrequency;
         //NoiseGeneration.Voronoi.mDisplacement.Value = mDisplacement;
         //NoiseGeneration.Voronoi.mSeed = mMicroSeed;

         //int x = TerrainGlobals.getTerrain().getNumXVerts();
         //float scale = TerrainGlobals.getTerrain().getTileScale();
         //for (int i = 0; i < x; i++)
         //{
         //   for (int j = 0; j < x; j++)
         //   {
         //      float amt = 1.0f;
         //      if (onlyToMaskedVerts)
         //      {
         //         if (!isVertSelected(i, j, ref amt))
         //            continue;
         //      }

         //      int indx = i + (j * x);
         //      Vector3 p = TerrainGlobals.getTerrain().getPostDeformPos(i, j);
         //      double height = 0;


         //      if (mMacroSeed == 0) height = NoiseGeneration.Voronoi.getValue(i, j, TerrainGlobals.getEditor().getDetailPoints()[indx].Y);
         //      else if (mMacroSeed == 1) height = NoiseGeneration.Voronoi.getValue(i, TerrainGlobals.getEditor().getDetailPoints()[indx].Y, j);
         //      else if (mMacroSeed == 2) height = NoiseGeneration.Voronoi.getValue(TerrainGlobals.getEditor().getDetailPoints()[indx].Y, i, j);

         //      height *= mScaleHeight;

         //      if (mMaskCreationType != eFilterMaskCreation.cNoMask)
         //         Masking.addSelectedVert(j, i, (float)height);

         //      height *= mScaleHeight;

         //      if (mMaskCreationType != eFilterMaskCreation.cMaskOnly)
         //         TerrainGlobals.getEditor().getDetailPoints()[indx].Y = addReplaceSubtractResult(TerrainGlobals.getEditor().getDetailPoints()[indx].Y, (float)height, mAddReplaceSubtract, amt);
         //   }
         //}
         //base.apply(onlyToMaskedVerts);
      }

      public override void previewToBitmap(ref Bitmap bmp)
      {

         //NoiseGeneration.Voronoi.mFrequency.Value = mFrequency;
         //NoiseGeneration.Voronoi.mDisplacement.Value = mDisplacement;
         //NoiseGeneration.Voronoi.mSeed = mMicroSeed;


         //Rectangle r = new Rectangle(0, 0, bmp.Width, bmp.Height);
         //unsafe
         //{
         //   BitmapData sourceData = bmp.LockBits(r, ImageLockMode.ReadOnly, bmp.PixelFormat);
         //   PixelData32* sourceBase = (PixelData32*)sourceData.Scan0;
         //   int width = bmp.Width;
         //   int height = bmp.Height;
         //   for (int x = 0; x < width; x++)
         //   {
         //      for (int y = 0; y < height; y++)
         //      {
         //         PixelData32* pSourcePixel = sourceBase + y * width + x;

         //         double H = 0;
         //         if (mMacroSeed == 0) H = NoiseGeneration.Voronoi.getValue(x, y, 0);
         //         else if (mMacroSeed == 1) H = NoiseGeneration.Voronoi.getValue(x, 0, y);
         //         else if (mMacroSeed == 2) H = NoiseGeneration.Voronoi.getValue(0, x, y);

         //         H = (float)BMathLib.Clamp((float)H, 0.0f, 1.0f);

         //         Byte val = (byte)(H * 255);
         //         pSourcePixel->A = 255;
         //         pSourcePixel->R = val;
         //         pSourcePixel->G = val;
         //         pSourcePixel->B = val;
         //      }
         //   }
         //   bmp.UnlockBits(sourceData);


         //}
      }

      protected float addReplaceSubtractResult(float orig, float input, int ars, float influenceAmt)
      {
         if (ars == 0)
            return blend(orig + input, orig, influenceAmt);
         else if (ars == 1)
            return blend(input, orig, influenceAmt);
         else if (ars == 2)
            return blend(orig - input, orig, influenceAmt);

         return 0;
      }
   }

   public class fBMTerrainFilter : TerrainFilter
   {

      public float mScaleHeight = 5.0f;
      public int mDetail = 28;
      public float mFrequency = 0.005f;
      public int mMacroSeed = 0;
      public int mMicroSeed = 1;
      public int mAddReplaceSubtract = 1;   //add=0, replace = 1, subtract=2
      public float mLacunarity = 1.5f;

      public override void init()
      {
      }
      public override void destroy()
      {
      }
      public override void apply(bool onlyToMaskedVerts)
      {
         //if (mMaskCreationType != eFilterMaskCreation.cMaskOnly)
         //   MakeUndoData(onlyToMaskedVerts);

         //NoiseGeneration.fBm.mFrequency.Value = mFrequency;
         //NoiseGeneration.fBm.mLacunarity.Value = mLacunarity;// new ClampedDouble(1.5, 3.5, 2.0);   //[1.5,3.5]
         //NoiseGeneration.fBm.mOctaveCount.Value = mDetail;
         //NoiseGeneration.fBm.mSeed = mMicroSeed;

         //int x = TerrainGlobals.getTerrain().getNumXVerts();
         //float scale = TerrainGlobals.getTerrain().getTileScale();
         //for (int i = 0; i < x; i++)
         //{
         //   for (int j = 0; j < x; j++)
         //   {
         //      float amt = 1.0f;
         //      if (onlyToMaskedVerts)
         //      {
         //         if (!isVertSelected(i, j, ref amt))
         //            continue;
         //      }

         //      int indx = i + (j * x);
         //      Vector3 p = TerrainGlobals.getTerrain().getPostDeformPos(i, j);
         //      double height = 0;


         //      if (mMacroSeed == 0) height = NoiseGeneration.fBm.getValue(i, j, TerrainGlobals.getEditor().getDetailPoints()[indx].Y);
         //      else if (mMacroSeed == 1) height = NoiseGeneration.fBm.getValue(i, TerrainGlobals.getEditor().getDetailPoints()[indx].Y, j);
         //      else if (mMacroSeed == 2) height = NoiseGeneration.fBm.getValue(TerrainGlobals.getEditor().getDetailPoints()[indx].Y, i, j);

         //      if (mMaskCreationType != eFilterMaskCreation.cNoMask)
         //         Masking.addSelectedVert(j, i, (float)height);

         //      height *= mScaleHeight;

         //      if (mMaskCreationType != eFilterMaskCreation.cMaskOnly)
         //         TerrainGlobals.getEditor().getDetailPoints()[indx].Y = addReplaceSubtractResult(TerrainGlobals.getEditor().getDetailPoints()[indx].Y, (float)height, mAddReplaceSubtract, amt);
         //   }
         //}
         //base.apply(onlyToMaskedVerts);
      }

      public override void previewToBitmap(ref Bitmap bmp)
      {
         //NoiseGeneration.fBm.mFrequency.Value = mFrequency;
         //NoiseGeneration.fBm.mLacunarity.Value = mLacunarity;// new ClampedDouble(1.5, 3.5, 2.0);   //[1.5,3.5]
         //NoiseGeneration.fBm.mOctaveCount.Value = mDetail;
         //NoiseGeneration.fBm.mSeed = mMicroSeed;

         //Rectangle r = new Rectangle(0, 0, bmp.Width, bmp.Height);
         //unsafe
         //{
         //   BitmapData sourceData = bmp.LockBits(r, ImageLockMode.ReadOnly, bmp.PixelFormat);
         //   PixelData32* sourceBase = (PixelData32*)sourceData.Scan0;
         //   int width = bmp.Width;
         //   int height = bmp.Height;
         //   for (int x = 0; x < width; x++)
         //   {
         //      for (int y = 0; y < height; y++)
         //      {
         //         PixelData32* pSourcePixel = sourceBase + y * width + x;

         //         double H = 0;
         //         if (mMacroSeed == 0) H = NoiseGeneration.fBm.getValue(x, y, 0);
         //         else if (mMacroSeed == 1) H = NoiseGeneration.fBm.getValue(x, 0, y);
         //         else if (mMacroSeed == 2) H = NoiseGeneration.fBm.getValue(0, x, y);

         //         H = (float)BMathLib.Clamp((float)H, 0.0f, 1.0f);

         //         Byte val = (byte)(H * 255);
         //         pSourcePixel->A = 255;
         //         pSourcePixel->R = val;
         //         pSourcePixel->G = val;
         //         pSourcePixel->B = val;
         //      }
         //   }
         //   bmp.UnlockBits(sourceData);


         //}
      }

      protected float blend(float a, float b, float i)
      {
         return (a * i) + ((1 - i) * b);
      }
      protected float addReplaceSubtractResult(float orig, float input, int ars, float influenceAmt)
      {
         if (ars == 0)
            return blend(orig + input, orig, influenceAmt);
         else if (ars == 1)
            return blend(input, orig, influenceAmt);
         else if (ars == 2)
            return blend(orig - input, orig, influenceAmt);

         return 0;
      }
   }


   public class TerraceFilter : TerrainFilter
   {
      public int mNumTerraces = 18;
      
      public override void init()
      {
      }
      public override void destroy()
      {
      }
      public override void apply(bool onlyToMaskedVerts)
      {
         if (mMaskCreationType != eFilterMaskCreation.cMaskOnly)
            MakeUndoData(onlyToMaskedVerts);

         int width = TerrainGlobals.getTerrain().getNumXVerts();
         Vector3[] detail = TerrainGlobals.getEditor().getDetailPoints();

         //find max * min Heights
         float maxH = -30000;
         float minH = 30000;
         for (int x = 0; x < width; x++)
         {
            for (int z = 0; z < width; z++)
            {
               
               if (onlyToMaskedVerts)
               {
                  float amt = 1;
                  if (!isVertSelected(x, z, ref amt))
                     continue;
               }
               float H = detail[x + z * width].Y;

               if (H > maxH) maxH = H;
               if (H < minH) minH = H;

            }
         }

         float terrStep = (maxH - minH) / mNumTerraces;

         for (int x = 0; x < width; x++)
         {
            for (int z = 0; z < width; z++)
            {
               float amt = 1.0f;
               if (onlyToMaskedVerts)
               {
                  if (!isVertSelected(x,z, ref amt))
                     continue;
               }
               float H = detail[x + z * width].Y;
               int k = (int)(H / terrStep) + 1;
               float nVal = (k * terrStep);
               detail[x + z * width].Y = blend(nVal, detail[x + z * width].Y, amt);
            }
         }

         base.apply(onlyToMaskedVerts);
      }
   }

   public class PlateauFilter : TerrainFilter
   {
      /*
       * CLM - This is a multifractal plateau filter 
       * This shit needs a ton of work before we roll it out
       */

      public NoiseGeneration.ClampedDouble mbutteMaxPercentThreshold = new NoiseGeneration.ClampedDouble(0.001, 1.0, 0.65);
      public NoiseGeneration.ClampedDouble mbutteMinPercentThreshold = new NoiseGeneration.ClampedDouble(0.001, 0.99, 0.25);
      public bool mHeightsDefined = false;
      public float mMaxHeight = -40;
      public float mMinHeight = 40;

      public override void init()
      {
      }
      public override void destroy()
      {
      }
      public override void apply(bool onlyToMaskedVerts)
      {
         if (mMaskCreationType != eFilterMaskCreation.cMaskOnly)
            MakeUndoData(onlyToMaskedVerts);

         float max = -20;
         float min = 20;

         int width = TerrainGlobals.getTerrain().getNumXVerts();
         Vector3[] detail = TerrainGlobals.getEditor().getDetailPoints();
         if(mHeightsDefined)
         {
            max = mMaxHeight;
            min = mMinHeight;
         }
         else
         {
            for (int x = 0; x < width; x++)
            {
               for (int y = 1; y < width; y++)
               {
                  if (onlyToMaskedVerts)
                  {
                     float amt = 1;
                     if (!isVertSelected(x, y, ref amt))
                        continue;
                  }

                  float curr = detail[x * width + y].Y;
                  if (curr > max)
                     max = curr;
                  if (curr < min)
                     min = curr;

               }
            }
         }
       

         
         float diff = max - min;

         max = (float)(max * mbutteMaxPercentThreshold.Value);
         float tmin = (float)(diff * mbutteMinPercentThreshold.Value);
         

         for (int x = 0; x < width; x++)
         {
            for (int y = 0; y < width; y++)
            {
               float amt = 1.0f;
               if (onlyToMaskedVerts)
               {
                  if (!isVertSelected(y,x, ref amt))
                     continue;
               }

               if (mHeightsDefined)
               {
                  float curr = mMinHeight + (amt * (mMaxHeight - mMinHeight));
                 // if (curr > max)
                  //   curr = max;
                  float eps = (curr / diff);
                  float newHeight = (curr > max ? max : diff * (eps * eps));
                  detail[x * width + y].Y = newHeight;// blend(newHeight, mMinHeight, amt);
               }
               else
               {
                  float curr = detail[x * width + y].Y;

                  float eps = (curr / diff);
                  float newHeight = (curr > max ? max : diff * (eps * eps));
                  detail[x * width + y].Y = blend(newHeight, curr, amt);
               }
            }
         }
         base.apply(onlyToMaskedVerts);
      }
   }

   public class ThermalErosionFilter : TerrainFilter
   {
      /*
       * This algorithm is dealing with longterm thermal erosion. 
       * The material is dissolved because of changes in temperature. 
         The eroded part is falling down in the direction of the greatest gradient.
       */
      public float mTalusAngle = 0;
      public int mType = 0;
      public int mNumIterations = 50;

      struct tNeighbor
      {
         public bool isValid;
         public float hDiff;
      } ;


      public override void init()
      {
      }
      public override void destroy()
      {
      }

      public void apply(bool onlyToMaskedVerts)
      {
         if (mMaskCreationType != eFilterMaskCreation.cMaskOnly)
            MakeUndoData(onlyToMaskedVerts);

         tNeighbor[] neighbors = new tNeighbor[8];			// the 8 neighbors of the current point
         for (int q = 0; q < 8; q++)
            neighbors[q] = new tNeighbor();

         int numLow = 0;						// number of lower neighbors
         float nearestDiff = 0;				// height difference to next lower neighbor [...]
         float moveAmount = 0;				// amount of material to move
         float sumDiff = 0;					// sum of (positive) height differences
         float totalMovedMaterial = 0;		// total material moved per iteration


         int2[] offsets = new int2[]
         {
            new int2(-1,-1),
            new int2( 0,-1),
            new int2( 1,-1),
            new int2(-1, 0),
            new int2(+1, 0),
            new int2(-1, 1),
            new int2( 0, 1),
            new int2( 1, 1),
         };


         int width = TerrainGlobals.getTerrain().getNumXVerts();
         Vector3[] detail = TerrainGlobals.getEditor().getDetailPoints();

         float[] tmpArray = new float[width * width];
         for (int T = 0; T < mNumIterations; T++)
         {
            for (int k = 0; k < width * width; k++)
               tmpArray[k] = 0;

            totalMovedMaterial = 0;

            for (int x = 0; x < width; x++)
            {
               for (int z = 0; z < width; z++)
               {
                  for (int q = 0; q < 8; q++)
                  {
                     int xIndex = x + offsets[q].a;
                     int zIndex = z + offsets[q].b;
                     if (xIndex < 0 || zIndex < 0 || xIndex >= width || zIndex >= width)
                     {
                        neighbors[q].isValid = false;
                     }
                     else
                     {
                        neighbors[q].isValid = true;
                        neighbors[q].hDiff = detail[x * width + z].Y - detail[xIndex * width + zIndex].Y;
                     }
                  }


                  nearestDiff = mType == 0 ? 200000 : 0;
                  numLow = 0;
                  sumDiff = 0;

                  // count number of lower neighbors & find next lower neighbor
                  for (int k = 0; k < 8; k++)
                  {
                     if (neighbors[k].isValid && neighbors[k].hDiff > mTalusAngle)	// <-- "talus angle"
                     {
                        numLow++;

                        if (mType == 0)
                        {
                           if (neighbors[k].hDiff < nearestDiff)
                              nearestDiff = neighbors[k].hDiff;
                        }
                        else
                        {
                           if (neighbors[k].hDiff > nearestDiff)
                              nearestDiff = neighbors[k].hDiff;
                        }

                        sumDiff += neighbors[k].hDiff;
                     }
                  }

                  if (mType != 0)
                     nearestDiff /= 2;


                  // continue if we have lower neighbors...
                  if (numLow > 0)
                  {

                     // amount of material moved from current position to all lower neighbors
                     // (slightly less than nearestDiff)
                     moveAmount = ((float)numLow / (numLow + 1)) * nearestDiff;

                     // subtract (small) random amount: 0%..19%
                     moveAmount -= (float)((float)Rand.mRandom.Next(20)) * moveAmount / 100;

                     // remove material from current position
                     tmpArray[x * width + z] -= moveAmount;

                     for (int q = 0; q < 8; q++)
                     {
                        if (neighbors[q].isValid && neighbors[q].hDiff > mTalusAngle)
                        {
                           int xIndex = x + offsets[q].a;
                           int zIndex = z + offsets[q].b;
                           float amt = neighbors[q].hDiff * moveAmount / sumDiff;
                           tmpArray[xIndex * width + zIndex] += amt;
                        }

                        totalMovedMaterial += moveAmount;		// for statistical reasons... :-)
                     }
                  }

               }
            }

            // write back to original array
            for (int k = 0; k < width * width; k++)
            {
               if (tmpArray[k] > 10 || tmpArray[k] < -10)
                  continue;

               float amt = 1.0f;
               if (onlyToMaskedVerts)
               {
                  if (!isVertSelected(k, ref amt))
                     continue;
               }
               detail[k].Y += tmpArray[k] * amt;

            }
         }

         base.apply(onlyToMaskedVerts);
      }
      
      
   }

   public class BasicHydraulicErosionFilter : TerrainFilter
   {
      /*
       * CLM - this is an implimentation of Benes hydraulic erosion paper
       * "Visual Simulation of Hydraulic Erosion" (http://wscg.zcu.cz/wscg2002/Papers_2002/F23.pdf)
       * This is no where NEAR as accurate as the 89Musgrave paper, but it's implementation is a stepping stone
       * Hopefully this will turn into a wider parameter stochastic erosion filter.
       */
      public override void init()
      {
      }
      public override void destroy()
      {
      }

      class waterDrop
      {
         public float material=0;
         public float water=0;
         public float soil=0;
      }
      struct tNeighbor
      {
         public bool isValid;
         public float hDiff;
      } ;


      public float mRainAmt = 1.0f/150.0f;		// amount of raindrops (1.0 = 100% = (width*height) drops)   1/[50,150]
      public float mRainDropSize = 1.0f/500.0f;	// = 1/1000th of the vertex height        1/[500,1500]               
      public float mDepositionAmt = 0.1f;			// how much soil to deposit (0..1)
      public float mSedimentCapacity = 5.0f;	// maximum sediment per vertex / sediment per unit of water
      public float mSoilSoftness = 0.29f;			// how much sediment to dissolve (0..1)
      public float mEvaporationRate = 1 / 100;   //how fast the raindrop dissolves    1/[50,150]
      public bool mAllowDirtFlowout = false;
      public int mNumIterations = 500;
   
      int2[] offsets = new int2[]
         {
            new int2(-1,-1),
            new int2( 0,-1),
            new int2( 1,-1),
            new int2(-1, 0),
            new int2(+1, 0),
            new int2(-1, 1),
            new int2( 0, 1),
            new int2( 1, 1),

         };

      public void apply(bool onlyToMaskedVerts)
      {
         if (mMaskCreationType != eFilterMaskCreation.cMaskOnly)
            MakeUndoData(onlyToMaskedVerts);

         int width = TerrainGlobals.getTerrain().getNumXVerts();
         Vector3[] detail = TerrainGlobals.getEditor().getDetailPoints();


         tNeighbor[] neighbors = new tNeighbor[8];			// the 8 neighbors of the current point
         for (int q = 0; q < 8; q++)
            neighbors[q] = new tNeighbor();
	      float waterMoved;				// total amount of water to move to neighboring vertices
	      float subWaterMoved;			// amount of water to move to a specific neighbor
	      float sed_capacity;				// sediment capacity
	      float soil_fraction;			// fraction of soil to be moved to neighbor
	      int numLow;						// number of lower neighbors
	      float sumDiff;					// sum of (positive) height differences
	      float avgDiff;					// average height difference

	      int rainOffset = 0;				// for rain model

         waterDrop[] tmpData = new waterDrop[(width + 1) * (width + 1)];
         waterDrop[] backData = new waterDrop[(width + 1) * (width + 1)];

         for (int i = 0; i < width * width; i++)
         {
            tmpData[i] = new waterDrop();
            tmpData[i].material = detail[i].Y;
            tmpData[i].soil = 0;
            tmpData[i].water = 0;

            backData[i] = new waterDrop();
         }

         for(int its=0;its<mNumIterations;its++)
         {
            for (int i = 0; i < width * width; i++)
            {
               backData[i].material = 0;
               backData[i].soil = 0;
               backData[i].water = 0;
            }

            // select positions to drop water onto...
            int numRaindrops = 0;

            if (its < mNumIterations - (mNumIterations / 10))		// before 90% of iterations have passed...
               numRaindrops = (int)(width * width * mRainAmt);

            for (int rd = 0; rd < numRaindrops; rd++)
            {
               float rainPos = ((float)Rand.mRandom.NextDouble()) * (width-1) * (width-1);

               //			tmpData[(int)rainPos].water += mRainDropSize;
               tmpData[(int)rainPos].water += tmpData[(int)rainPos].material * mRainDropSize;
            }

            for (int x = 0; x < width; x++)
            {
               for (int z = 0; z < width; z++)
               {
                  if (tmpData[x * width + z].water > 0)
                  {
                     for (int q = 0; q < 8; q++)
                     {
                        int xIndex = x + offsets[q].a;
                        int zIndex = z + offsets[q].b;
                        if (xIndex < 0 || zIndex < 0 || xIndex >= width || zIndex >= width)
                        {
                           neighbors[q].isValid = false;
                        }
                        else
                        {
                           neighbors[q].isValid = true;
                           neighbors[q].hDiff = (tmpData[x * width + z].water + tmpData[x * width + z].material + tmpData[x * width + z].soil)
                                              - (tmpData[xIndex * width + zIndex].water + tmpData[xIndex * width + zIndex].material + tmpData[xIndex * width + zIndex].soil);
                        }
                     }


                     numLow = 0;
                     sumDiff = 0;

                     // count number of lower neighbors & sum of differences
                     for (int k = 0; k < 8; k++)
                     {
                        if (neighbors[k].isValid && neighbors[k].hDiff > 0)
                        {
                           numLow++;
                           sumDiff += neighbors[k].hDiff;
                        }
                     }

                     // if water can flow out of heightfield...
                     if (mAllowDirtFlowout)
                     {
                        avgDiff = sumDiff / numLow;
                        for (int k = 0; k < 8; k++)
                        {
                           if (!neighbors[k].isValid)
                           {
                              neighbors[k].hDiff = avgDiff / 2;		// or avgDiff or avgDiff*2 ??
                              sumDiff += avgDiff / 2;
                              numLow++;
                           }
                        }
                     }




                     // continue if we have lower neighbors...
					      if (numLow > 0)
					      {
						      // total amount of water to move to neighbors
						      waterMoved = sumDiff / numLow+1;
						      waterMoved = Math.Min(tmpData[x*width+z].water, waterMoved);


						      // iterate over all neighbors...
						      for (int k = 0; k < 8; k++)
						      {
							      //distribute water...
							      if ( (neighbors[k].isValid || mAllowDirtFlowout) && (neighbors[k].hDiff > 0) )
							      {
                              int xIndex = x + offsets[k].a;
                              int zIndex = z + offsets[k].b;

								      // fraction of water to be moved
								      subWaterMoved = waterMoved * neighbors[k].hDiff / sumDiff;

								      // max sediment the current fraction of water can carry
                              sed_capacity = mSedimentCapacity * subWaterMoved;

								      // fraction of existing soil to be moved from old vertex to new vertex
								      soil_fraction = tmpData[x*width+z].soil * subWaterMoved / waterMoved;

								      if (soil_fraction < sed_capacity )	// dissolving (water can still capture material)
								      {
									      // convert material from old vertex to soil (in new vertex):

									      // increase amount of soil in new vertex
									      if (neighbors[k].isValid)
                                    backData[xIndex * width + zIndex].soil += soil_fraction + mSoilSoftness * (sed_capacity - soil_fraction);

									      // remove fraction of soil from old vertex
									      backData[x*width+z].soil -= soil_fraction;

									      // decrease material in old vertex
                                 backData[x * width + z].material -= mSoilSoftness * (sed_capacity - soil_fraction);
								      }
								      else		// deposition (water is saturated)
								      {
									      // move maximum amount of soil to new vertex (maximum = sed_capacity)
									      if (neighbors[k].isValid)
                                    backData[xIndex * width + zIndex].soil += sed_capacity;

									      // remove maximum amount of soil from old vertex (maximum = sed_capacity)
									      backData[x*width+z].soil -= sed_capacity;

									      // increase material in old vertex
									      // (soil_fraction - sed_capacity) = remaining soil w/ respect to new vertex
                                 backData[x * width + z].material += mDepositionAmt * (soil_fraction - sed_capacity);

									      // decrease soil in old vertex
                                 backData[x * width + z].soil -= mDepositionAmt * (soil_fraction - sed_capacity);
								      }

								      // remove water from old vertex
								      backData[x*width+z].water -= subWaterMoved;

								      // move water to new vertex
								      if (neighbors[k].isValid)
                                 backData[xIndex * width + zIndex].water += subWaterMoved;
							      }	
						      }	

					      }	
				      	
                  }
               }
            }

            // write back to tmpData:
            for (int i = 0; i < width * width; i++)			// TODO: remove backData (no longer needed)!!!
            {
               tmpData[i].material += backData[i].material;
               tmpData[i].soil += backData[i].soil;
               tmpData[i].water += backData[i].water;
            }

            //////////////////////////////////////////////////////////////////////////
            // evaporate part of the water
            for (int i = 0; i < width * width; i++)
            {
               if (tmpData[i].water > 0)
               {
                  //				tmpData[i].water -= RAINDROP_SIZE / 150;		// ??
                  tmpData[i].water -= tmpData[i].water * mEvaporationRate;

                  //				if (tmpData[i].water < RAINDROP_SIZE / 100)
                  //					tmpData[i].water = 0;
               }
            }
         }

         //before we write back to the array, attempt to establish equilibrium with the sediment particles still in transport..


         // write back to original array
         for (int i = 0; i < width * width; i++)
         {
            float amt = 1.0f;
            if (onlyToMaskedVerts)
            {
               if (!isVertSelected(i, ref amt))
                  continue;
            }
            detail[i].Y = amt*(tmpData[i].material + tmpData[i].soil);
         }


         tmpData = null;
         backData = null;

         base.apply(onlyToMaskedVerts);
      }
   }

   public class AdvancedHydraulicErosionFilter : TerrainFilter
   {
      #region public params
      float mPower = 1.0f;                   //[0,x]
      public float Power
      {
         get
         {
            return mPower;
         }
         set
         {
            mPower = value;
            if (mPower > 30) mPower = 30;
            if (mPower < 0) mPower = 0;
         }
      }
      float mSoilHardness = 0.3f;             //[0,1]
      public float SoilHardness
      {
         get
         {
            return mSoilHardness;
         }
         set
         {
            mSoilHardness = value;
            if (mSoilHardness > 1) mSoilHardness = 1;
            if (mSoilHardness < 0) mSoilHardness = 0;
         }
      }
      int mSeed = 0;
      public int RandomSeed
      {
         get
         {
            return mSeed;
         }
         set
         {
            mSeed = value;
            mRand = new Random(mSeed);
         }
      }
      float mSoilCarryPercent = 0.5f;        //[0,1]
      public float SoilCarryAmt
      {
         get
         {
            return mSoilCarryPercent;
         }
         set
         {
            mSoilCarryPercent = BMathLib.Clamp(value, 0, 1);
         }
      }
      int mMaxRaindropLife = 32;        //[0,x]
      public int DropLife
      {
         get{
            return mMaxRaindropLife;
         }
         set{
            mMaxRaindropLife = value;
         }
      }
      float mChannelingForce = 28;     //[0,32]
      public float ChannelingForce
      {
         get
         {
            return mChannelingForce;
         }
         set
         {
            mChannelingForce = BMathLib.Clamp(value, 2, 32);
         }
      }

      float mSoilErosionRate = 0.5f;            //scalar that dumbs down the amount of soil we actually transfer
      public float SoilErosionRate
      {
         get
         {
            return mSoilErosionRate;
         }
         set
         {
            mSoilErosionRate = BMathLib.Clamp(value, 0, 1);
         }
      }

      //erosion type values
      public enum eErosionType
      {
         cErosionSimple =0,
         cErosionAdvanced =1,
      }
      public eErosionType mErosionType = eErosionType.cErosionSimple;


      #endregion

      #region local params
      float mNeighborErodeFalloff = 0.85f;   //[0,1]        //closer to 1, means more square effects

      float maxSlope = 0.15f;          //[0,1] slopes outside of this range are clamped


      Random mRand = new Random(0);

      bool doLocalNeighbors = false;

      int mWidth = 256;
      int mHeight = 256;

      Vector3[] mHeights = null;
      float[,] mDeltaHeights = null;
      #endregion

      private class waterDrop
      {
         public float mSoilContained = 0;
         public int mAge = 0;
         public int _x;
         public int _y;
      }
      private class waterDropPathPoint
      {
         public int mGridX;
         public int mGridY;
         public float mHeight;
         public float mDiffFromLastHeight;
         public float mSlopeFromLastHeight;
      }


      public override void  init()
      {
 	       base.init();
          mWidth = TerrainGlobals.getTerrain().getNumXVerts();
          mHeight = TerrainGlobals.getTerrain().getNumXVerts();
          mDeltaHeights = new float[mWidth, mHeight];
         if(mMaskCreationType != eFilterMaskCreation.cNoMask)
         {
            for (int x = 0; x < mWidth; x++)
            {
               for (int y = 0; y < mHeight; y++)
               {
                  mDeltaHeights[x, y] = TerrainGlobals.getTerrain().getPostDeformPos(x,y).Y;
               }
            }
         }
      }
      public override void  destroy()
      {
 	       base.destroy();
          mDeltaHeights = null;
      }

      
      public override void apply(bool onlyToMaskedVerts)
      {
         MakeUndoData(onlyToMaskedVerts);

         mHeights = TerrainGlobals.getEditor().getDetailPoints();
         switch(mErosionType)
         {
            case eErosionType.cErosionSimple:
               applySimple(onlyToMaskedVerts);
               break;
            case eErosionType.cErosionAdvanced:
               applyAdv(onlyToMaskedVerts);
               break;
         };

         base.apply(onlyToMaskedVerts);
         mHeights = null;
      }


      /*
       * Advaced method
       * Finds the path of a drop first, so we can analyize it
       * then does sediment transport as a 2nd step
       * this is slower, as it does n*2 iterations
       * but the control allows us to do some advanced things
       */ 
      public void  applyAdv(bool onlyToMaskedVerts)
      {
         float soil;
         float slope;

         float mSlopeToSoilScalar = 1.0f / mChannelingForce;

         uint passes = (uint)(mWidth * mHeight * mPower);		// scales better than entering passes by hand

         waterDrop wd = new waterDrop();
         
         List<waterDropPathPoint> points = new List<waterDropPathPoint>();

         for (uint i = 0; i < passes; i++)
         {
            wd._x = mRand.Next(mWidth - 2);
            wd._y = mRand.Next(mHeight - 2);

            float amt = 0;
            if (onlyToMaskedVerts)
            {
               if (!Masking.isPointSelected(wd._x, wd._y, ref amt))
                  continue;
            }
            else
            {
               if (Masking.isPointSelected(wd._x, wd._y, ref amt))
                  continue;
            }

            soil = 0.0f;
            wd.mAge = mMaxRaindropLife;
            wd.mSoilContained = 0;

            //let's path the drop first.
            pathDrop(wd, ref points);

            //now that we have the path this drop will follow
            //transfer and move the sediment
            wd.mSoilContained = 0;
            wd._x = points[0].mGridX;
            wd._y = points[0].mGridY;
            int p = 0;
            if (points.Count > 1)
            {
               for (p = 1; p < points.Count - 1; p++)
               {
                  mHeights[wd._x * mWidth + wd._y].Y += soil;
               
                  slope = BMathLib.Clamp(points[p + 1].mSlopeFromLastHeight, 0, maxSlope);

                  soil = ((slope * mSlopeToSoilScalar) * (1.0f - mSoilHardness));

                  mHeights[wd._x * mWidth + wd._y].Y -= soil;
                  wd.mSoilContained += soil;

                  forwardChannel(wd._x, wd._y, points[p + 1].mGridX, points[p + 1].mGridY, soil);

                  wd._x = points[p + 1].mGridX;
                  wd._y = points[p + 1].mGridY;
               }
            }

            //this drop is done. deposit any leftover sediment
            mHeights[points[p].mGridX* mWidth + points[p].mGridY].Y += wd.mSoilContained;
            diffusePoint(points[p].mGridX, points[p].mGridY);

         }

 
      }
      waterDropPathPoint constructPathPoint(int gridX, int gridZ)
      {
         waterDropPathPoint wpp = new waterDropPathPoint();
         wpp.mGridX = gridX;
         wpp.mGridY = gridZ;
         wpp.mHeight = mHeights[gridX * mWidth + gridZ].Y;

         return wpp;
      }
      void pathDrop(waterDrop wd, ref List<waterDropPathPoint> points)
      {
         points.Clear();

         points.Add(constructPathPoint(wd._x, wd._y));

         Point next = new Point();
         float slope = 0;

         while (wd.mAge > 0 && (wd._x >= 1 && wd._y >= 1 && wd._x <= mWidth - 2 && wd._y <= mHeight - 2))
         {
            next.X = wd._x;
            next.Y = wd._y;

            if (TerrainGlobals.getTerrain().isLocalMinima(wd._x, wd._y))
               break;


            int tx = 0;
            int ty = 0;

            findLowestNeighbor(wd._x, wd._y, out tx, out ty, out slope);

            next.X = tx;
            next.Y = ty;


            //move our drop
            wd._x = next.X;
            wd._y = next.Y;

            //age our drop
            wd.mAge--;

            points.Add(constructPathPoint(wd._x, wd._y));
            points[points.Count - 1].mSlopeFromLastHeight = slope;
         }


      }


      /*
       * Simple method
       * Applys erosion and pathing @ the same time
       * No advanced knowlege of the system
       */ 
      public void applySimple(bool onlyToMaskedVerts)
      {
         float soil;
         float slope;
         Point next = new Point();
         float numr;
         float dnmr;

         float mSlopeToSoilScalar = 1.0f / mChannelingForce;

         uint passes = (uint)(mWidth * mHeight * mPower);		// scales better than entering passes by hand

         waterDrop wd = new waterDrop();

         List<waterDropPathPoint> points = new List<waterDropPathPoint>();

         for (uint i = 0; i < passes; i++)
         {
            wd._x = mRand.Next(mWidth - 2);
            wd._y = mRand.Next(mHeight - 2);

            float amt = 0;
            if (onlyToMaskedVerts)
            {
               if (!Masking.isPointSelected(wd._x, wd._y, ref amt))
                  continue;
            }
            else
            {
               if (Masking.isPointSelected(wd._x, wd._y, ref amt))
                  continue;
            }

            soil = 0.0f;
            wd.mAge = mMaxRaindropLife;
            wd.mSoilContained = 0;

            //let's path the drop first.
               while (wd.mAge > 0 && (wd._x >= 1 && wd._y >= 1 && wd._x <= mWidth - 2 && wd._y <= mHeight - 2))
               {
                  next.X = wd._x;
                  next.Y = wd._y;
                  slope = 0;

                  
                  if (TerrainGlobals.getTerrain().isLocalMinima(wd._x, wd._y))
                  {
                     //we're a local minima. Deposit our soil here
                     {
                        mHeights[wd._x * mWidth + wd._y].Y += wd.mSoilContained;//we use the sum of the moved soil to fake it.
                        soil = 0.0f;
                        wd.mSoilContained = 0;
                        diffusePoint(wd._x, wd._y);
                     }

                     break;
                  }
                  else
                  {
                     int tx = 0;
                     int ty = 0;

                     findLowestNeighbor(wd._x, wd._y, out tx, out ty, out slope);

                     next.X = tx;
                     next.Y = ty;
                  }

                
                  mHeights[wd._x * mWidth + wd._y].Y += soil;


                  numr = (mHeights[wd._x * mWidth + wd._y].Y - mHeights[next.X * mWidth + next.Y].Y);
                  dnmr = TerrainGlobals.getTerrain().getTileScale();

                  slope = BMathLib.Clamp((numr / dnmr), 0, maxSlope);


                  // now pick up soil to carry to the next cell
                  soil = ((slope * mSlopeToSoilScalar) * (1.0f - mSoilHardness));

                  mHeights[wd._x * mWidth + wd._y].Y -= soil;
                  wd.mSoilContained += soil;

                  forwardChannel(wd._x, wd._y, next.X, next.Y, soil);

                  //move our drop
                  wd._x = next.X;
                  wd._y = next.Y;

                  //age our drop
                  wd.mAge--;
               }


            {
               mHeights[wd._x * mWidth + wd._y].Y +=  wd.mSoilContained;   //we use the sum of the moved soil to fake it.
               diffusePoint(wd._x, wd._y);
            }
         }

         postMaskOperation();
      }

      void postMaskOperation()
      {
         if (mMaskCreationType == eFilterMaskCreation.cNoMask)
            return;
            
         float tol = 0.1f;
         Masking.clearSelectionMask();
         for (int x = 0; x < mWidth; x++)
         {
            for (int y = 0; y < mHeight; y++)
            {
               if (mDeltaHeights[x, y] > mHeights[x * mWidth + y].Y)
               {
                  float dff = BMathLib.Clamp(mDeltaHeights[x, y] - mHeights[x * mWidth + y].Y, 0, 1);
                  Masking.addSelectedVert(x, y, dff);
               }
               if (mMaskCreationType == eFilterMaskCreation.cMaskOnly)
                  mHeights[x * mWidth + y].Y = mDeltaHeights[x, y];
            }
         }

         mDeltaHeights = null;
      }

      
      void diffusePoint(int x, int z)
      {
      }

      /*
       * This method will modify verts on the forward selection of a moved point
       * We have to fix up the diagnoals that are opposite the winding of the polies
       * otherwise, we get spikes as we move to corners that don't influence the neighbor edge
       * Also, this looks good to widen the sloping kernel.. (if we want it?)
       * 
       */
      void forwardChannel(int _x, int _y, int nX, int nY, float soil)
      {

         float tSoil = soil * mNeighborErodeFalloff;
         //float eSoil = tSoil * mNeighborErodeFalloff * mNeighborErodeFalloff;

         int px = (int)BMathLib.Clamp(_x + 1, 0, TerrainGlobals.getTerrain().getNumXVerts() - 1);
         int py = (int)BMathLib.Clamp(_y + 1, 0, TerrainGlobals.getTerrain().getNumZVerts() - 1);

         int nx = (int)BMathLib.Clamp(_x - 1, 0, TerrainGlobals.getTerrain().getNumXVerts() - 1);
         int ny = (int)BMathLib.Clamp(_y - 1, 0, TerrainGlobals.getTerrain().getNumZVerts() - 1);

         //int px2 = (int)BMathLib.Clamp(_x + 2, 0, TerrainGlobals.getTerrain().getNumXVerts() - 1);
         //int py2 = (int)BMathLib.Clamp(_y + 2, 0, TerrainGlobals.getTerrain().getNumZVerts() - 1);

         //int nx2 = (int)BMathLib.Clamp(_x - 2, 0, TerrainGlobals.getTerrain().getNumXVerts() - 1);
         //int ny2 = (int)BMathLib.Clamp(_y - 2, 0, TerrainGlobals.getTerrain().getNumZVerts() - 1);

         int []localNeighbor = new int[]
         { 
            px, py,     //diagnols
            nx, ny,
            nx, py,
            px, ny,

            _x, py,     //cardinal
            _x, ny, 
            nx, _y,
            px, _y
         };

         int[] localNeighborRel = new int[]
         { 
            _x, py,   px, _y, //diagnols
            _x, ny,   nx, _y,
            _x, py,   nx, _y,
            _x, ny,   px, _y,

            nx, py,   px, py,    //cardinal
            nx, ny,   px, ny,
            nx, py,   nx, ny,
            px, py,   px, ny,

         };

         //int[] extendedNeighborRel = new int[]
         //{ 
         //   _x, py2,   px, py2,   px2, py2,   px2, py,   px2, _y,      //directional
         //   _x, ny2,   nx, ny2,   nx2, ny2,   nx2, ny,   nx2, _y,
         //   _x, py2,   nx, py2,   nx2, py2,   nx2, py,   nx2, _y,
         //   _x, ny2,   px, ny2,   px2, ny2,   px2, ny,   px2, _y,

         //   nx2, py2,   px, py2,   _x, py2,   nx, py2,   nx2, py2,    //cardinal
         //   nx2, ny2,   px, ny2,   _x, ny2,   nx, ny2,   nx2, ny2,    
         //   nx2, ny2,   nx2, ny,   nx2, _y,   nx2, py,   nx2, py2,
         //   px2, ny2,   px2, ny,   px2, _y,   px2, py,   px2, py2,
         //};

         int max = doLocalNeighbors ? 2 : 8;
         for(int i=0;i<8;i++)
         {
            if(nX == localNeighbor[i*2] && nY == localNeighbor[i*2+1])
            {
               mHeights[localNeighborRel[i * 4] * mWidth + localNeighborRel[i*4+1]].Y -= tSoil;
               mHeights[localNeighborRel[i*4+2] * mWidth + localNeighborRel[i*4+3]].Y -= tSoil;
               //if(doExtendedNeighbors)
               //{
               //   mHeights[extendedNeighborRel[i * 10 + 0] * mWidth + extendedNeighborRel[i * 10 + 1]].Y -= eSoil;
               //   mHeights[extendedNeighborRel[i * 10 + 2] * mWidth + extendedNeighborRel[i * 10 + 3]].Y -= eSoil;
               //   mHeights[extendedNeighborRel[i * 10 + 4] * mWidth + extendedNeighborRel[i * 10 + 5]].Y -= eSoil;
               //   mHeights[extendedNeighborRel[i * 10 + 6] * mWidth + extendedNeighborRel[i * 10 + 7]].Y -= eSoil;
               //   mHeights[extendedNeighborRel[i * 10 + 8] * mWidth + extendedNeighborRel[i * 10 + 9]].Y -= eSoil;
               //}
            }
         }

      }

      bool findLowestNeighbor(int _x, int _y, out int nX, out int nZ, out float slope)
      {
         float testSlope = 0;

         nX = _x;
         nZ = _y;
         slope = 0;
         float diff = 0;

         int iS = Math.Max(_x - 1, 0);
         int iE = Math.Min(_x + 2, mWidth);
         int jS = Math.Max(_y - 1, 0);
         int jE = Math.Min(_y + 2, mWidth);

         for (int i = iS; i < iE; i++)
         {
            for (int j = jS; j < jE; j++)
            {
               diff = mHeights[_x * mWidth + _y].Y - mHeights[i * mWidth + j].Y;
           
               if (diff==0)
                  continue;
               
               testSlope = diff / TerrainGlobals.getTerrain().getTileScale();
               if (testSlope > slope  || (testSlope == slope && mRand.NextDouble()<0.5f))
               {
                  nX = i;
                  nZ = j;

                  slope = testSlope;
               }
            }
         }

         return true;
      }
   }

   public class SandDuneFilter : TerrainFilter
   {
      public override void init()
      {
      }
      public override void destroy()
      {
      }
      const float mAvgSand = 0.015f; //Qo
      const float mAvgHop = 40;
      int2[] offsets = new int2[]
         {
            new int2(-1,-1),
            new int2( 0,-1),
            new int2( 1,-1),
            new int2(-1, 0),
            new int2(+1, 0),
            new int2(-1, 1),
            new int2( 0, 1),
            new int2( 1, 1),
         };
      public float findGradiant(int x, int z)
      {
         float grad = 0;
         Vector3[] detail = TerrainGlobals.getEditor().getDetailPoints();
          int width = TerrainGlobals.getTerrain().getNumXVerts();

         float max = -20;
         int maxI = 0;
         float min = 20;
         int minI = 0;
         for (int i = 0; i < 8; i++)
         {

            int xIndex = (int)BMathLib.Clamp(x + offsets[i].a, 0, width - 1);
            int zIndex = (int)BMathLib.Clamp(z + offsets[i].b, 0, width - 1);

            float h = detail[xIndex * width + zIndex].Y;
            if(h>max)
            {
               max = h;
               maxI = i;
            }
            if (h < min)
            {
               min = h;
               minI = i;
            }
         }

         Vector3 a = new Vector3(offsets[maxI].a, max, offsets[maxI].b);
         Vector3 b = new Vector3(offsets[minI].a, min, offsets[minI].b);

         return max-min;
      }


      public override void apply(bool onlyToMaskedVerts)
      {
         if (mMaskCreationType != eFilterMaskCreation.cMaskOnly)
            MakeUndoData(onlyToMaskedVerts);

         int width = TerrainGlobals.getTerrain().getNumXVerts();
         Vector3[] detail = TerrainGlobals.getEditor().getDetailPoints();
         Vector3[] normals = TerrainGlobals.getEditor().getNormals();

         Vector2 windVector = new Vector2((float)Rand.mRandom.NextDouble() * Rand.mRandom.Next(4), (float)Rand.mRandom.NextDouble() * Rand.mRandom.Next(4));

         //lets do Saltation first.
         int numIterations =20;
         for (int i = 0; i < numIterations; i++)
         {
            for (int x = 0; x < width; x++)
            {
               for (int y = 0; y < width; y++)
               {
                  //  float e = detail[x * width + y].Y;
                  //calculate the gradiant of our point
                  float grad = findGradiant(x, y);


                  Vector3 norm = normals[x * width + y];
                  //calculate our 'hop' position
                  int lx = x + (int)((mAvgHop) * (1 - (float)Math.Tanh(Math.Cos(y))));
                  int lz = y + (int)((mAvgHop) * (1 - (float)Math.Tanh(Math.Sin(x))));

                  float dotRes = Vector3.Dot(BMathLib.unitY, norm);
                  float Qs = mAvgSand * (1 + (float)Math.Tanh(dotRes));

                  //
                  detail[x * width + y].Y -= Qs;

                  if (lx >= width || lx < 0 || lz >= width || lz < 0)
                     continue;

                  detail[lx * width + lz].Y += Qs;
               }
            }
         }
      }
   }
 
}