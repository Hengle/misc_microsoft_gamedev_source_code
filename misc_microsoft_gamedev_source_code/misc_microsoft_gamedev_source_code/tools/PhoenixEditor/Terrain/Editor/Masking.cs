using System;
using System.IO;
using System.Drawing;
using System.Collections.Generic;
using System.Text;
using Microsoft.DirectX;
using Microsoft.DirectX.Direct3D;
using Microsoft.DirectX.DirectInput;
using System.Windows.Forms;
using System.Threading;
using System.Drawing.Drawing2D;
using System.Drawing.Imaging;
////
using Rendering;
using EditorCore;



//------------------------------------------
namespace Terrain
{

   public class Masking
   {

      static IMask mCurrSelectionMask = new ArrayBasedMask(2049 * 2049);
      static IMask mBaseMaskingMask = null;//new ArrayBasedMask(2049 * 2049);
      static public BTileBoundingBox mCurrSelectionMaskExtends = new BTileBoundingBox();

      static public IMask getCurrSelectionMaskWeights() { return mCurrSelectionMask; }
      static public IMask getBaseMaskingMaskWeights() { return mBaseMaskingMask; }
      
      static public void InitMasking(int maxCapacity)
      {
         MaskFactory.mMaxCapacity = maxCapacity;
         mCurrSelectionMask = MaskFactory.GetNewMask();
      }


      static public void setBaseMaskToCurrent()
      {
         if (mBaseMaskingMask == null)
         {
            //mCurrSelectionMask.Clear();
            //rebuildVisualsAfterSelection(); 
            return;
         }
         mCurrSelectionMask = mBaseMaskingMask;
         mBaseMaskingMask = null;
         rebuildVisualsAfterSelection(); 
      }
      static public void setCurrentMaskToBase()
      {
         mBaseMaskingMask = mCurrSelectionMask;
         mCurrSelectionMask = MaskFactory.GetNewMask();
         rebuildVisualsAfterSelection(); 
      }

      static public void setCurrSelectionMaskWeights(IMask mask)
      {
         mCurrSelectionMask = mask;
         mCurrSelectionMaskExtends.empty();

         int stride = TerrainGlobals.getTerrain().getNumZVerts();
         long id;
         float value;
         mCurrSelectionMask.ResetIterator();
         while (mCurrSelectionMask.MoveNext(out id, out value))  //it.MoveNext() == true)
         {
            int x = (int)(id / stride);
            int z = (int)(id - x * stride);
            extendCurrSelectionMask(x, z);
         }
         rebuildVisualsAfterSelection();         
      }
      static public float combineValueWithBaseMask(float BaseValue, float NewValue)
      {
         return Math.Min(BaseValue, NewValue);
      }
      static public void addSelectedVert(int x, int z, float selAmt)
      {
         int i = x * TerrainGlobals.getTerrain().getNumXVerts() + z;   

         float factor = 1.0f;

         if(checkBaseWritePermission(x,z, out factor))
         {
            selAmt = Masking.combineValueWithBaseMask(factor, selAmt);
            extendCurrSelectionMask(x, z);
            mCurrSelectionMask.SetMaskWeight((int)i, selAmt);
         }
      }
      static public void addSelectedVert(long id, float selAmt)
      {
         float factor = 1.0f;

         if (checkBaseWritePermission(id, out factor))
         {
            selAmt = Masking.combineValueWithBaseMask(factor, selAmt);
            int stride = TerrainGlobals.getTerrain().getNumZVerts();
            int x = (int)(id / stride);
            int z = (int)(id - x * stride);
            extendCurrSelectionMask(x, z);
            mCurrSelectionMask.SetMaskWeight(id, selAmt);
         }
      }
      static public bool checkBaseWritePermission(int x, int z, out float value)
      {
         value = 1.0f;
         if (mBaseMaskingMask != null)
         {
            int i = x * TerrainGlobals.getTerrain().getNumXVerts() + z;
            return checkBaseWritePermission(i, out value);
         }
         return true;
      }
      static public bool checkBaseWritePermission(long id, out float value)
      {
         value = 1.0f;
         if (mBaseMaskingMask != null)
         {            
            value = mBaseMaskingMask.GetMaskWeight(id);
            if (value == 0)
               return false;
         }
         return true;
      }

      static public void invertSelectionMask()
      {
         mCurrSelectionMaskExtends.empty();
         int imgWidth = TerrainGlobals.getTerrain().getNumXVerts();

         for (int x = 0; x < imgWidth; x++)
         {
            for (int y = 0; y < imgWidth; y++)
            {
               int index = x * imgWidth + y;
               float weight = mCurrSelectionMask.GetMaskWeight(index);
               addSelectedVert(x, y, 1 - weight);
            }
         }
         rebuildVisualsAfterSelection();
      }

      static public void clearBaseMaskingMask()
      {
         mBaseMaskingMask = null;
      }

      static public bool isEmpty()
      {
         return !mCurrSelectionMask.HasData();
      }
      static public void clearSelectionMask()
      {

         if (mCurrSelectionMask.HasData())
         {

            if (CoreGlobals.getEditorMain().mIMaskPickerUI == null)
            {
               CoreGlobals.getEditorMain().mIGUI.ShowDialog("MaskLayers");
            }
            CoreGlobals.getEditorMain().mIMaskPickerUI.SetLastMask(mCurrSelectionMask);
         }

         mCurrSelectionMask.Clear();

         if(TerrainGlobals.getTerrain().getQuadNodeRoot()!=null)
         {
            List<BTerrainQuadNode> nodes = new List<BTerrainQuadNode>();
            TerrainGlobals.getTerrain().getQuadNodeRoot().getTileBoundsIntersection(nodes, mCurrSelectionMaskExtends.minX, mCurrSelectionMaskExtends.maxX,
                                                                                     mCurrSelectionMaskExtends.minZ, mCurrSelectionMaskExtends.maxZ);

            for (int i = 0; i < nodes.Count; i++)
            {
               nodes[i].mDirty = true;
            }
            TerrainGlobals.getTerrain().rebuildDirtyPostDeform(BRenderDevice.getDevice());
         }

         // Reset extends
         mCurrSelectionMaskExtends.empty();

      }
      
      static public bool hasSelectionMaskData(int x, int z)
      {
         return (mCurrSelectionMaskExtends.isPointInside(x, z));
      }
      static public bool isPointSelected(int x, int z, ref float selAmt)
      {
         return isPointSelected(x * TerrainGlobals.getTerrain().getNumXVerts() + z, ref selAmt);
      }
      static public bool isPointSelected(long index, ref float selAmt)
      {
         selAmt = mCurrSelectionMask.GetMaskWeight(index);
        
         return (selAmt != 0);
      }

      static private void extendCurrSelectionMask(int x, int z)
      {
         mCurrSelectionMaskExtends.addPoint(x, z);
      }

      static public void rebuildVisualsAfterSelection()
      {

         TerrainGlobals.getTerrain().getQuadNodeRoot().clearVisibleDatHandle();
         List<BTerrainQuadNode> nodes = new List<BTerrainQuadNode>();


         TerrainGlobals.getTerrain().getQuadNodeRoot().getTileBoundsIntersection(nodes, mCurrSelectionMaskExtends.minX, mCurrSelectionMaskExtends.maxX,
                                                                                  mCurrSelectionMaskExtends.minZ, mCurrSelectionMaskExtends.maxZ);


         for (int k = 0; k < nodes.Count; k++)
         {
            nodes[k].mDirty = true;
            BTerrainQuadNodeDesc desc = nodes[k].getDesc();
            
            BTerrain.computeBasis(TerrainGlobals.getEditor().getDetailPoints(), TerrainGlobals.getEditor().getNormals(),
                                                 TerrainGlobals.getTerrain().getTileScale(), TerrainGlobals.getTerrain().getNumXVerts(),
                                                 desc.mMinXVert, desc.mMaxXVert, desc.mMinZVert, desc.mMaxZVert);
   
         }

         TerrainGlobals.getTerrain().rebuildDirtyPostDeform(BRenderDevice.getDevice());
      }

      //used for when d3d needs to lock a resource of a chunk. Test this early so we don't lock per vertex.
      static public bool doesAreaContainSelection(int minx, int maxx, int minz, int maxz)
      {
         for(int i=minx;i<maxx;i++)
         {
            for(int j=minz;j<maxz;j++)
            {
               long index = j + TerrainGlobals.getTerrain().getNumXVerts() * i;
               float selAmt=0;
               if (isPointSelected(index, ref selAmt))
                  return true;
            }
         }
         return false;
      }

      //used as a filter.
      static public void createSelectionMaskFromTerrain(float minHeight,float maxHeight,float slopeValue, float range)
      {

         //clearSelectionMask();

         int totalNumVerts = TerrainGlobals.getTerrain().getNumXVerts();
         int i = -1;

         //range = 0.1f;

         for (int x = 0; x < totalNumVerts; x++)
         {
            for (int z = 0; z < totalNumVerts; z++)
            {
               i++;
               if (TerrainGlobals.getTerrain().getPostDeformPos(x, z).Y < minHeight ||
                  TerrainGlobals.getTerrain().getPostDeformPos(x, z).Y > maxHeight)
                  continue;

               float factor = Vector3.Dot(TerrainGlobals.getTerrain().getPostDeformNormal(x, z), BMathLib.unitY);
               if (factor < 0 || Math.Abs(slopeValue - factor) > range)
                  continue;

               addSelectedVert(x, z, factor);
                

            }
         }
         smoothFilter();
      }

      static public void createSelectionMaskFromTerrain(ComponentMaskSettings maskSettings)
      {             
         int totalNumVerts = TerrainGlobals.getTerrain().getNumXVerts();
         int i = -1;
       
         for (int x = 0; x < totalNumVerts; x++)
         {
            for (int z = 0; z < totalNumVerts; z++)
            {
               float baseValue;
               if (checkBaseWritePermission(x, z, out baseValue) == false)
               {
                  continue;
               }

               i++;

               float yValue = TerrainGlobals.getTerrain().getPostDeformPos(x, z).Y;
               float factor = 1.0f;

               float minHeight = maskSettings.mMinHeight;
               float maxHeight = maskSettings.mMaxHeight;

               //Apply 2d height noise to min/max height values
               if (maskSettings.mbUseMinHeight && maskSettings.mMinNoiseFunction != null)
               {
                  minHeight += maskSettings.mMinNoiseFunction.Evaluate(x, z);
               }
               if (maskSettings.mbUseMaxHeight && maskSettings.mMaxNoiseFunction != null)
               {
                  maxHeight += maskSettings.mMaxNoiseFunction.Evaluate(x, z);
               }              

               //Apply height clamp
               if ((maskSettings.mbUseMinHeight && yValue < minHeight) ||
                   (maskSettings.mbUseMaxHeight && yValue > maxHeight))
               {
                  continue;
               }

               //Apply gradient
               if (maskSettings.mbUseMinHeight && maskSettings.mMinGradient != null)
               {
                  maskSettings.mMinGradient.mStartPoint = minHeight;
                  maskSettings.mMinGradient.mEndPoint = maxHeight;
                     
                  factor -= maskSettings.mMinGradient.Evaluate(yValue); 
               }
               if (maskSettings.mbUseMaxHeight && maskSettings.mMaxGradient != null)
               {
                  maskSettings.mMaxGradient.mStartPoint = minHeight;
                  maskSettings.mMaxGradient.mEndPoint = maxHeight;

                  factor -= maskSettings.mMaxGradient.Evaluate(yValue); 
               }

               if (factor <= 0)
               {
                  continue;
               }

               if(maskSettings.mbUseSlope)
               {
                  factor = Vector3.Dot(TerrainGlobals.getTerrain().getPostDeformNormal(x, z), BMathLib.unitY);
                  if ( factor < 0 || Math.Abs(maskSettings.mSlopeValue - factor) > maskSettings.mSlopeRange)
                     continue;
               }

               if(maskSettings.mUseGradiant)
               {
                  Vector2 v = TerrainGlobals.getTerrain().getGradiant(x, z);
                  factor = BMathLib.Clamp(v.Length(),0,1);
                  if (factor < 0)
                     continue;
               }

               addSelectedVert(x, z, factor);

            }
         }
         if (maskSettings.mbPostSmooth)
         {
            smoothFilter();
         }
      }


      //filter functions applied to the mask to vary it.
      public class ConvMatrix
      {
         public enum eCoeffSpot
         {
            cTopLeft=0,
            cTopCenter,
            cTopRight,
            cMidLeft,
            cMidCenter,
            cMidRight,
            cBotLeft,
            cBotCenter,
            cBotRight
            
         };
         public ConvMatrix()
         {
         }
         public ConvMatrix(float topLeft, float topCenter, float topRight, float midLeft, float midCenter, float midRight, float botLeft, float botCenter, float botRight)
         {
            mFilterCoeffs = new float[9];
            mFilterCoeffs[0] = topLeft;
            mFilterCoeffs[1] = topCenter;
            mFilterCoeffs[2] = topRight;
            mFilterCoeffs[3] = midLeft;
            mFilterCoeffs[4] = midCenter;
            mFilterCoeffs[5] = midRight;
            mFilterCoeffs[6] = botLeft;
            mFilterCoeffs[7] = botCenter;
            mFilterCoeffs[8] = botRight;

         }
         public float[] mFilterCoeffs = null;
         public float mFactor = 1;
         public float mOffset = 0;
         public void SetAll(float nVal)
         {
            if(mFilterCoeffs==null)
               mFilterCoeffs = new float[9];

            for (int i = 0; i < 9; i++)
               mFilterCoeffs[i] = nVal;
         }
      }

      static private void tapNeightbor(int k, ref int x, ref int z)
      {
         int[] neightbors = new int[16] { +1, +0, 
                                          -1, +0,
                                          +1, +1,
                                          -1, +1,
                                          +1, -1,
                                          -1, -1,
                                          +0, +1,
                                          +0, -1 };

         x = x + neightbors[k * 2];
         z = z + neightbors[k * 2 +1];
         BMathLib.Clamp(x, 0, TerrainGlobals.getTerrain().getNumXVerts()-1);
         BMathLib.Clamp(z, 0, TerrainGlobals.getTerrain().getNumXVerts()-1);
      }
      static private void applyConvMatrix(ConvMatrix m )
      {
         int[] neightbors = new int[] {   -1, 1,          //top left
                                           0, 1,          //top center
                                           1, 1,          //top right

                                          -1, 0,          //mid left
                                           0, 0,          //mid center
                                           1, 0,          //mid right

                                          -1, -1,          //bot left
                                           0, -1,          //bot center
                                           1, -1,          //bot right
                                          };

         int imgWidth = TerrainGlobals.getTerrain().getNumXVerts();
         float[] tempImgArray = new float[imgWidth * imgWidth];
         
         for (int x = 0; x < imgWidth; x++)
         {
            for (int y = 0; y < imgWidth; y++)
            {
               float total = 0;
               for (int k = 0; k < 9; k++)
               {
                  int xIndex = x + neightbors[k * 2];
                  int zIndex = y + neightbors[k * 2 + 1];
                  if(xIndex < 0 || xIndex > imgWidth-1 || zIndex < 0 || zIndex > imgWidth - 1 )
                     continue;
                  


                  int index = xIndex * imgWidth + zIndex;
                  float amt = 0;
                  isPointSelected(index, ref amt);
                  total += m.mFilterCoeffs[k] * amt;

               }
               total = total / m.mFactor + m.mOffset;

               if (total > 1.0f) 
                  total = 1.0f;
               if (total < 0) 
                  total = 0;

               tempImgArray[x * imgWidth + y] = total;
            }
         }

         //send our mask back
         mCurrSelectionMask.Clear();
         for (int x = 0; x < imgWidth; x++)
         {
            for (int y = 0; y < imgWidth; y++)
            {
               int indx = x * imgWidth + y;
               if (tempImgArray[indx] != 0)
                  addSelectedVert(x, y, tempImgArray[indx]);
            }
         }


         tempImgArray = null;

      }

      static public void smoothFilter()
      {

         //use a gaussin matrix
         ConvMatrix filter = new ConvMatrix(1,2,1,2,4,2,1,2,1);
         filter.mFactor = 16;
         filter.mOffset = 0;

         applyConvMatrix(filter);

         rebuildVisualsAfterSelection();
      }
      static public void detectEdges()
      {
         ConvMatrix simpleEdgeFilter = new ConvMatrix(0, -1, 0, -1, 4, -1, 0, -1, 0);
         ConvMatrix fancyEdgeFilter = new ConvMatrix( 1.0f / 6.0f, 4.0f / 6.0f, 1.0f / 6.0f, 4.0f / 6.0f, -20.0f/6.0f, 4.0f / 6.0f, 1.0f / 6.0f, 4.0f / 6.0f, 1.0f / 6.0f );

         applyConvMatrix(fancyEdgeFilter);

         rebuildVisualsAfterSelection();
      }
      static public void sharpenFilter()
      {
         ConvMatrix filter = new ConvMatrix(0, -2, 0, -2, 11, -2, 0, -2, 0);
         filter.mFactor = 3;
         filter.mOffset = 0;


         applyConvMatrix(filter);

         rebuildVisualsAfterSelection();
      }

      static public void expandSelection(int numVerts)
      {
         ConvMatrix filter = new ConvMatrix(1, 1, 1, 1, 0, 1, 1, 1, 1);
         filter.mFactor = 1;
         filter.mOffset = 0;

         for (int i = 0; i < numVerts;i++ )
            applyConvMatrix(filter);

         rebuildVisualsAfterSelection();

      }
      static public void contractSelection(int numVerts)
      {
         Masking.invertSelectionMask();
         ConvMatrix filter = new ConvMatrix(1, 1, 1, 1, 0, 1, 1, 1, 1);
         filter.mFactor = 1;
         filter.mOffset = 0;

         for (int i = 0; i < numVerts; i++)
            applyConvMatrix(filter);


         Masking.invertSelectionMask();
      }

      static public void scaleSelection(float amt, bool addOrPercent)
      {
         int totalNumVerts = TerrainGlobals.getTerrain().getNumXVerts();
         
         float fact =0;
         for (int x = 0; x < totalNumVerts; x++)
         {
            for (int z = 0; z < totalNumVerts; z++)
            {
               if(Masking.isPointSelected(x,z,ref fact))
               {
                  if (addOrPercent)
                     fact = BMathLib.Clamp(fact + amt, 0, 1);
                  else
                     fact = BMathLib.Clamp(fact * amt, 0, 1);
                  Masking.addSelectedVert(x, z, fact);
               }
            }
         }
         rebuildVisualsAfterSelection();
      }
      // Gaussian blur
      static private double[] Kernel(int size, float sigma, float sqrSigma)
      {
         // check for evem size and for out of range
         if (size % 2 == 0)
            size = size + 1;

         if ( (size < 3) || (size > 101))
         {
            throw new ArgumentException();
         }

         // raduis
         int r = size / 2;
         // kernel
         double[] kernel = new double[size];

         // compute kernel
         for (int x = -r, i = 0; i < size; x++, i++)
         {
            kernel[i] = Math.Exp(x * x / (-2 * sqrSigma)) / (Math.Sqrt(2 * Math.PI) * sigma);
         }

         return kernel;
      }

      static public void blurSelection(int numVerts, float sigma)
      {
         //get our kernel 
         double[] kernel = Kernel(numVerts, sigma, sigma * sigma);

         //temp array
         int imgWidth = TerrainGlobals.getTerrain().getNumXVerts();
         float[] tempImgArray = new float[imgWidth * imgWidth];
         int half = numVerts>>1;
         //apply us horizontally first
         for(int z=0;z<imgWidth;z++)
         {
            for(int x=0;x<imgWidth;x++)   
            {
               float total = 0;
               int rootIndex = x * imgWidth + z;
               for(int ww=0;ww<numVerts;ww++)
               {
                  int xIndex = x + (ww-half);
                  
                  if (xIndex < 0 || xIndex > imgWidth - 1 )
                     continue;
                  
                    int index = xIndex * imgWidth + z;
                     float amt = 0;
                     isPointSelected(index, ref amt);

                     total += amt * (float)kernel[ww];
               }

               if (total > 1.0f)
                  total = 1.0f;
               if (total < 0)
                  total = 0;
               tempImgArray[rootIndex] = total;
            }
         }

         mCurrSelectionMask.Clear();

         
         for (int x = 0; x < imgWidth; x++)
         {
            for (int z = 0; z < imgWidth; z++)   
            {
               float total = 0;
               int rootIndex = x * imgWidth + z;
               for (int ww = 0; ww < numVerts; ww++)
               {
                  int zIndex = z + (ww - half);

                  if (zIndex < 0 || zIndex > imgWidth - 1)
                     continue;

                  int index = x * imgWidth + zIndex;
                  
                  total += tempImgArray[index] * (float)kernel[ww];
               }

               if (total > 1.0f)
                  total = 1.0f;
               if (total < 0)
                  total = 0;

               addSelectedVert(x, z, total);
            }
         }



         tempImgArray = null;


         rebuildVisualsAfterSelection();
      }

      static public Image GetScaledImageFromFile(string filename, int xScale, int yScale)
      {
         Bitmap bitmap = new Bitmap(filename);
         Image myThumbnail = bitmap.GetThumbnailImage(xScale, yScale, myCallback, IntPtr.Zero);
         return myThumbnail;
      }
      static public Image.GetThumbnailImageAbort myCallback = null;
      static public bool ThumbnailCallback()
      {
         return false;
      }
      unsafe public struct PixelData24
      {
         public byte blue;
         public byte green;
         public byte red;
      }
      unsafe public struct PixelData32Rgb
      {
         public byte blue;
         public byte green;
         public byte red;
         public byte notused;
      }
      unsafe public struct PixelData32PArgb
      {
         public byte blue;
         public byte green;
         public byte red;
         public byte A;
      }
      public static IMask CreateMask(Image source1)
      {
         if(myCallback == null)
            myCallback = new Image.GetThumbnailImageAbort(ThumbnailCallback);

         IMask mask = MaskFactory.GetNewMask();
         Bitmap source = (Bitmap)(source1.Clone());
         Rectangle r = new Rectangle(0, 0, source.Width, source.Height);

         
         int formatSize = Image.GetPixelFormatSize(source.PixelFormat);
         unsafe
         {
            
            BitmapData sourceData = source.LockBits(r, ImageLockMode.ReadWrite, source.PixelFormat);
            if (sourceData.PixelFormat == PixelFormat.Format24bppRgb)
            {
               PixelData24* sourceBase = (PixelData24*)sourceData.Scan0;
               int width = source.Width;
               int height = source.Height;
               for (int x = 0; x < width; x++)
               {
                  for (int y = 0; y < height; y++)
                  {
                     PixelData24* pSourcePixel = sourceBase + y * width + x;
                     byte blue = pSourcePixel->blue;
                     //pOututPixel->alpha = (byte)(blue);
                     //pOututPixel->red = (byte)(blue);
                     //pOututPixel->blue = (byte)(blue);
                     //pOututPixel->green = (byte)(blue);
                     long index = y * width + x;
                     float value = blue / (float)byte.MaxValue;
                     //mask.Add(index, value);
                     mask.SetMaskWeight(index, value);
                  }
               }
               source.UnlockBits(sourceData);
            }
            else if (sourceData.PixelFormat == PixelFormat.Format32bppRgb)
            {
               PixelData32Rgb* sourceBase = (PixelData32Rgb*)sourceData.Scan0;
               int width = source.Width;
               int height = source.Height;
               for (int x = 0; x < width; x++)
               {
                  for (int y = 0; y < height; y++)
                  {
                     PixelData32Rgb* pSourcePixel = sourceBase + y * width + x;
                     byte blue = pSourcePixel->blue;
                     //pOututPixel->alpha = (byte)(blue);
                     //pOututPixel->red = (byte)(blue);
                     //pOututPixel->blue = (byte)(blue);
                     //pOututPixel->green = (byte)(blue);
                     long index = y * width + x;
                     float value = blue / (float)byte.MaxValue;
                     //mask.Add(index, value);
                     mask.SetMaskWeight(index, value);
                  }
               }
               source.UnlockBits(sourceData);
            }
            else if (sourceData.PixelFormat == PixelFormat.Format32bppPArgb)
            {
               PixelData32PArgb* sourceBase = (PixelData32PArgb*)sourceData.Scan0;
               int width = source.Width;
               int height = source.Height;
               for (int x = 0; x < width; x++)
               {
                  for (int y = 0; y < height; y++)
                  {
                     PixelData32PArgb* pSourcePixel = sourceBase + y * width + x;
                     byte blue = pSourcePixel->blue;
                     //pOututPixel->alpha = (byte)(blue);
                     //pOututPixel->red = (byte)(blue);
                     //pOututPixel->blue = (byte)(blue);
                     //pOututPixel->green = (byte)(blue);
                     long index = y * width + x;
                     float value = blue / (float)byte.MaxValue;
                     //mask.Add(index, value);
                     mask.SetMaskWeight(index, value);
                  }
               }
               source.UnlockBits(sourceData);
            }         
         }
         return mask;
      }

      public static Image ExportMask(IMask mask)
      {

         int width = TerrainGlobals.getTerrain().getNumXVerts();
         int height = TerrainGlobals.getTerrain().getNumZVerts();
         Rectangle r = new Rectangle(0, 0, width, height);

         Bitmap destination = new Bitmap(width, height, PixelFormat.Format24bppRgb);

         unsafe
         {
            BitmapData outputData = destination.LockBits(r, ImageLockMode.WriteOnly, destination.PixelFormat);
            if (outputData.PixelFormat == PixelFormat.Format24bppRgb)
            {
               PixelData24* outputBase = (PixelData24*)outputData.Scan0;

               for (int x = 0; x < width; x++)
               {
                  for (int y = 0; y < height; y++)
                  {
                     PixelData24* pPixel = outputBase + y * width + x;
                     long index = y * width + x;
                     float value = mask.GetMaskWeight(index);                     
                     byte outputValue = (byte)(value * byte.MaxValue);
                     pPixel->blue = outputValue;
                     pPixel->red = outputValue;
                     pPixel->green = outputValue;

                  }
               }
               destination.UnlockBits(outputData);
            }           
         }
         return (Image)destination;
      }




      public static void MaskCurrentTexture(bool bUseTextureFade)
      {
         int totalNumVerts = TerrainGlobals.getTerrain().getNumXVerts();
         int i = -1;

         int selTexIndex = TerrainGlobals.getTerrainFrontEnd().SelectedTextureIndex;
       
         BTerrainActiveTextureContainer activeTex = TerrainGlobals.getTexturing().getActiveTexture(TerrainGlobals.getTerrainFrontEnd().SelectedTextureIndex);

         if (activeTex.mMatHasMask == false)
         {
            return;
         }
         
         Image img = Image.FromFile(activeTex.getMaskFileName());
         if (img == null)
            return;
         IMask m = CreateMask(img);
         if (m == null)
            return;
         img.Dispose();

         for (int x = 0; x < totalNumVerts; x++)
         {
            for (int z = 0; z < totalNumVerts; z++)
            {
               float baseValue;
               float factor = 0;
               if (checkBaseWritePermission(x, z, out baseValue) == false)
               {
                  continue;
               }
               BTerrainTextureVector vec = TerrainGlobals.getEditor().giveTextureDataAtVertex(x, z);

               if(vec.containsID(selTexIndex, BTerrainTexturingLayer.eLayerType.cLayer_Splat))
               {
                  int splatindex = vec.giveLayerIndex(selTexIndex, BTerrainTexturingLayer.eLayerType.cLayer_Splat);

                  float layeralpha = vec.mLayers[splatindex].mAlphaContrib / 255f;
                  float maskAlpha = 0;
                  if (layeralpha == 0)
                     continue;


                  maskAlpha = m.GetMaskWeight(((z % 64) * 64) + (x % 64));
                  //layeralpha = m.GetMaskWeight((( x) * totalNumVerts) + ( z));
                  if (bUseTextureFade)
                  {
                     maskAlpha = maskAlpha * layeralpha;
                  }
                  if (maskAlpha == 0)
                     continue;

                  i++;

                  addSelectedVert(x, z, maskAlpha);
               }
            }
         }    
      }

   }

   public interface OneDimensionalFunction
   {
      float Evaluate(float inputValue);
   }

   public interface TwoDimensionalFunction
   {
      float Evaluate(float inputValueA, float inputValueB);
   }

   public class GradientFunction : OneDimensionalFunction
   {
      public float mStartPoint;
      public float mEndPoint;
      public float mStartValue;
      public float mEndValue;
      public float Evaluate(float inputValue)
      {
         if (inputValue <= mStartPoint)
            return mStartValue;
         else if (inputValue >= mEndPoint)
            return mEndValue;
         else
            return ((inputValue - mStartPoint) / (mEndPoint - mStartPoint)) * (mEndValue - mStartValue) + mStartValue;
      }
   }
   public class NoiseFunction : TwoDimensionalFunction
   {
      public float Evaluate(float inputValueA, float inputValueB)
      {
         return 0;// 3f * (float)NoiseGeneration.RigedMultiFractal.getValue(inputValueA, inputValueB, 0);
      }
   }


   //to be normal or not?
   //how to scale?

   public class AbstractImage : TwoDimensionalFunction
   {
      virtual public float Evaluate(float inputValueA, float inputValueB)
      {
         //float value;
         //value = 0.5f * ((float)NoiseGeneration.RigedMultiFractal.getValue(inputValueA, inputValueB, 0) + 0.5f);
         //value =  Math.Max(0,((float)NoiseGeneration.RigedMultiFractal.getValue(inputValueA, inputValueB, 0) ));
         //if (value < 0)
         //{
         //   value = 0;
         //}
         //if (value > 1)
         //{
         //   value = 1;
         //}
         //return value;
         return 1.0f;
      }
      public float mMinA = 0;
      public float mMinB = 0;
      public float mMaxA = 300;
      public float mMaxB = 300;

      //virtual public Image GetThumbail(int width, int height)
      //{
      //   Image thumbnail = new Bitmap(width, height);
      //   float xRate = width / (mMaxA);
      //   float yRate = height / mMaxB;
      //}

   }
   public class LinearGradientImage : AbstractImage
   {
      override public float Evaluate(float inputValueA, float inputValueB)
      {
         if (inputValueB > mMaxB)
            return 1.0f;

         if (inputValueB < mMinB)
            return 0.0f;
         return 1.0f - ((mMaxB - inputValueB) / (mMaxB-mMinB));
      }
   }
   public class FractalImage : AbstractImage
   {
      override public float Evaluate(float inputValueA, float inputValueB)
      {
         NoiseGeneration.RigedMultiFractal rv = new NoiseGeneration.RigedMultiFractal();
         float value;
         value = 0.5f * ((float)rv.getValue(inputValueA, inputValueB, 0) + 0.5f);
         value = Math.Max(0, ((float)rv.getValue(inputValueA, inputValueB, 0)));
         if (value < 0)
         {
            value = 0;
         }
         if (value > 1)
         {
            value = 1;
         }
         return value;
      }
   }

   public class MasklImage : AbstractImage
   {
      override public float Evaluate(float inputValueA, float inputValueB)
      {
         if (mMask == null)
            return 0;

         inputValueA = (int)inputValueA;
         inputValueB = (int)inputValueB;

         long index = (long)((inputValueB * mWidth) + inputValueA);

         float value = mMask.GetMaskWeight(index);
         if (value != 0)
         {
            return value;
         }
         return value;

      }
      int mHeight = 0;
      int mWidth = 0;
      public void SetMask(IMask mask, int height, int width)
      {
         mMask = mask;
         mHeight = height;
         mWidth = width;
         mMaxA = mHeight;
         mMaxB = mWidth;
      }

      IMask mMask = null;

      public void Load()
      {
         OpenFileDialog d = new OpenFileDialog();
         d.Filter = "Image (*.bmp)|*.bmp";
         if (d.ShowDialog() == DialogResult.OK)
         {
            Image loadedImage = Masking.GetScaledImageFromFile(d.FileName, TerrainGlobals.getTerrain().getNumXVerts(), TerrainGlobals.getTerrain().getNumZVerts());

            IMask mask = Masking.CreateMask(loadedImage);

            //MasklImage newImage = new MasklImage();
            this.SetMask(mask, TerrainGlobals.getTerrain().getNumXVerts(), TerrainGlobals.getTerrain().getNumZVerts());


            //TerrainGlobals.getEditor().mCurrentAbstractImage = newImage;
         }
      }
   }


   public class ComponentMaskSettings
   {
      public bool mbUseSlope = false;
      public float mSlopeValue = 0;
      public float mSlopeRange = 0.05f;

      public bool mbUseMinHeight = false;
      public float mMinHeight = 0;
      public GradientFunction mMinGradient = null;
      public NoiseFunction mMinNoiseFunction = null;

      public bool mbUseMaxHeight = false;
      public float mMaxHeight = 0;
      public GradientFunction mMaxGradient = null;
      public NoiseFunction mMaxNoiseFunction = null;

      public bool mUseGradiant = false;

      public bool mbPostSmooth = true;
   }


}