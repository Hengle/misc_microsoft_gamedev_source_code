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
   #region Terrain Texturing Brushes




   //------------------------------------------
   public abstract class BTerrainPaintBrush : BTerrainBrush
   {
     
      public abstract bool applyToDstImg(BTerrainQuadNode node, int terrainGridX, int terrainGridZ, bool selectionMode, bool alternate);

      public abstract void render();
      //brush modifiers
      public abstract void rotateBrush(float delta);
      public abstract int getMaskWidth();
      
   };
   //-------------------------------------------
   //-------------------------------------------
   public class BTerrainSplatBrush : BTerrainPaintBrush
      {
         public BrushStroke mStroke = new BrushStroke();
         public BTerrainSplatBrush()
         {
            mTexArrayIndex = 0;
            mMskImg = null;
            mMskImgWidth = 512;
            mMskImgHeight = 512;
            m_bApplyOnSelection = true;
         }
         //------------------------------------------
         ~BTerrainSplatBrush()
         {
            if (mMskImg != null)
            {

               mMskImg = null;
            }
         }

         public bool init(int textureArrayIndex, Texture mskImg)
         {
            mTexArrayIndex = textureArrayIndex;

            SurfaceDescription mskDesc;
            mskDesc = mskImg.GetLevelDescription(0);

            //get out image descriptions
            mMskImgWidth = (uint)mskDesc.Width;
            mMskImgHeight = (uint)mskDesc.Height;

            mMskImg = mskImg;

            mFirstDrawn = true;
            return true;
         }
         //------------------------------------------
         public override bool applyToDstImg(BTerrainQuadNode node, int terrainGridX, int terrainGridZ, bool selectionMode, bool alternate)
         {
            if(!selectionMode)
               return applyToDstLayer(node, terrainGridX, terrainGridZ, selectionMode, alternate);

            return applyMask(node, terrainGridX, terrainGridZ, alternate);
         }
         //------------------------------------------
         //called to apply a texutre directly to a masked area
         public unsafe bool applyToMask(BTerrainQuadNode node)
         {
            bool changed = false;



            BTerrainQuadNodeDesc desc = node.getDesc();

            changed = Texturing_LayerEditor.setIndexToMaskedArea(node, desc.mMinXVert, desc.mMinZVert, desc.mMaxXVert, desc.mMaxZVert,
                               (char)mTexArrayIndex);


            if (changed)
            {
               //node.getTextureData().free();
               for (int i = 0; i < Terrain.BTerrainTexturing.cMaxNumLevels; i++)
               {
                  node.getTextureData(i).free();
               }
            }

            return changed;
         }
         //------------------------------------------
         //called to create the mask form the texture
         public unsafe bool applyMask(BTerrainQuadNode node,  int x, int z, bool alternate)
         {
            UInt32* MskPtr = mStroke.getBits();
            mMskImgWidth = (uint)mStroke.cBufferSize;
            mMskImgHeight = (uint)mStroke.cBufferSize;
            bool changed = Texturing_LayerEditor.setMaskAlphaToTextureBlending(node, BTerrainTexturing.getAlphaTextureWidth(), BTerrainTexturing.getAlphaTextureWidth(),
                                                                  MskPtr, mMskImgWidth, mMskImgHeight,mStroke.mAlphaValue,
                                                                  x, z, alternate);
            mStroke.unlockBits();

            return changed;
         }
         //------------------------------------------
         //called to apply the texture to the terrain
         public unsafe bool applyToDstLayer(BTerrainQuadNode node, int terrainGridX, int terrainGridZ, bool selectionMode, bool alternate)
         {
            UInt32* MskPtr = mStroke.getBits();
            mMskImgWidth = (uint)mStroke.cBufferSize;
            mMskImgHeight = (uint)mStroke.cBufferSize;

            bool changed = Texturing_LayerEditor.setMaskAlphaToLayer(node, MskPtr, mMskImgWidth, mMskImgHeight, mStroke.mAlphaValue, terrainGridX, terrainGridZ, (char)mTexArrayIndex);

           
            mStroke.unlockBits();

            if (changed)
            {
               //node.getTextureData().free();
               for (int i = 0; i < Terrain.BTerrainTexturing.cMaxNumLevels; i++)
               {
                  node.getTextureData(i).free();
               }
            }

            return true;
         }
      
         //------------------------------------------
         public override void rotateBrush(float delta)
         {

         }
         //------------------------------------------
         public override int getMaskWidth()
         {
            return (int)mStroke.cBufferSize;// (int)mMskImgWidth;
         }
         //------------------------------------------
         protected Texture mMskImg;

         protected uint mMskImgWidth;
         protected uint mMskImgHeight;
         protected int mTexArrayIndex;
         public bool mFirstDrawn;

         public override void render()
         {
            BRenderDevice.getDevice().SetRenderState(RenderStates.AlphaBlendEnable, true);
            BRenderDevice.getDevice().SetRenderState(RenderStates.SourceBlend, (int)Microsoft.DirectX.Direct3D.Blend.SourceAlpha);
            BRenderDevice.getDevice().SetRenderState(RenderStates.DestinationBlend, (int)Microsoft.DirectX.Direct3D.Blend.InvSourceAlpha);

            Texture mask = TerrainGlobals.getTerrainFrontEnd().getSelectedMaskTexture();
            SurfaceDescription sd = mask.GetLevelDescription(0);

            float validRadius = sd.Width / 2;
            List<BTerrainQuadNode> nodes = new List<BTerrainQuadNode>();
            int closestX = 0;
            int closestZ = 0;
            {

               // in this node, find the CLOSEST vertex to this actual position.
               BTerrainEditor.findClosestVertex(ref closestX, ref closestZ, ref TerrainGlobals.getEditor().mBrushIntersectionPoint, TerrainGlobals.getEditor().mBrushIntersectionNode);

               TerrainGlobals.getTerrain().getQuadNodeRoot().getTileBoundsIntersection(nodes, (int)(closestX - validRadius),
                                                                                        (int)(closestX + validRadius),
                                                                                        (int)(closestZ - validRadius),
                                                                                        (int)(closestZ + validRadius));
            }
            for (int i = 0; i < nodes.Count; i++)
               nodes[i].renderCursor();


            BRenderDevice.getDevice().SetRenderState(RenderStates.AlphaBlendEnable, false);
         }
      };
   //-------------------------------------------
   public class BTerrainDecalBrush : BTerrainPaintBrush
   {
      public BrushStroke mStroke = new BrushStroke();
      protected int mDecalArrayIndex;
      protected int mDecalRadius=1;
      protected int mDecalWidth = 0;
      protected int mDecalHeight = 0;

      public BTerrainDecalBrush()
      {
      }
      ~BTerrainDecalBrush()
      {
      }
      public bool init(int decalIndex)
      {
         mDecalArrayIndex = decalIndex;
         
         BTerrainActiveDecalContainer decal = TerrainGlobals.getTexturing().getActiveDecal(TerrainGlobals.getTerrainFrontEnd().SelectedDecalIndex);
         SurfaceDescription sd = decal.mTexChannels[0].mTexture.GetLevelDescription(0);

         mDecalRadius = sd.Width / 2;
         mDecalWidth = sd.Width;
         mDecalHeight = sd.Height;
         return true;
      }
      public override bool applyToDstImg(BTerrainQuadNode node, int x, int z, bool selectionMode, bool alternate)
      {
         BTerrainLayerContainer layers = node.mLayerContainer;
         BTerrainActiveDecalContainer decal = TerrainGlobals.getTexturing().getActiveDecal(TerrainGlobals.getTerrainFrontEnd().SelectedDecalIndex);

         bool changed = false;



         return changed;
      }

      //------------------------------------------
      public override void rotateBrush(float delta)
      {
         mStroke.mRotationAmt += delta;
      }
      //------------------------------------------
      public override int getMaskWidth()
      {
         return (int)mStroke.cBufferSize;// (int)mMskImgWidth;
      }

      public int getDecalWidth()
      {
         return (int)(mDecalWidth );
      }
      public int getDecalHeight()
      {
         return (int)(mDecalHeight);
      }   
      //------------------------------------------

      public override void render()
      {
         BRenderDevice.getDevice().SetRenderState(RenderStates.AlphaBlendEnable, true);
         BRenderDevice.getDevice().SetRenderState(RenderStates.SourceBlend, (int)Microsoft.DirectX.Direct3D.Blend.SourceAlpha);
         BRenderDevice.getDevice().SetRenderState(RenderStates.DestinationBlend, (int)Microsoft.DirectX.Direct3D.Blend.InvSourceAlpha);

         
         List<BTerrainQuadNode> nodes = new List<BTerrainQuadNode>();
         int closestX = 0;
         int closestZ = 0;
         {

            // in this node, find the CLOSEST vertex to this actual position.
            BTerrainEditor.findClosestVertex(ref closestX, ref closestZ, ref TerrainGlobals.getEditor().mBrushIntersectionPoint, TerrainGlobals.getEditor().mBrushIntersectionNode);

            TerrainGlobals.getTerrain().getQuadNodeRoot().getTileBoundsIntersection(nodes, (int)(closestX - mDecalRadius),
                                                                                     (int)(closestX + mDecalRadius),
                                                                                     (int)(closestZ - mDecalRadius),
                                                                                     (int)(closestZ + mDecalRadius));
         }
         for (int i = 0; i < nodes.Count; i++)
            nodes[i].renderCursor();


         BRenderDevice.getDevice().SetRenderState(RenderStates.AlphaBlendEnable, false);
      }
   };










   public class GDIBrushSplats
   {

      unsafe public struct PixelData24
      {
         public byte blue;
         public byte green;
         public byte red;
      }
      unsafe public struct PixelData32
      {
         public byte blue;
         public byte green;
         public byte red;
         public byte alpha;
      }
      //This sucks.
      static public Bitmap generateAlpha(Bitmap source)
      {

         Bitmap output = new Bitmap(source.Width, source.Height, PixelFormat.Format32bppArgb);
         Rectangle r = new Rectangle(0, 0, source.Width, source.Height);
         unsafe
         {
            BitmapData sourceData = source.LockBits(r, ImageLockMode.ReadOnly, source.PixelFormat);
            BitmapData outputData = output.LockBits(r, ImageLockMode.WriteOnly, output.PixelFormat);

            if (sourceData.PixelFormat == PixelFormat.Format24bppRgb)
            {
               PixelData24* sourceBase = (PixelData24*)sourceData.Scan0;
               PixelData32* outputBase = (PixelData32*)outputData.Scan0;
               int width = source.Width;
               int height = source.Height;
               for (int x = 0; x < width; x++)
               {
                  for (int y = 0; y < height; y++)
                  {
                     PixelData24* pSourcePixel = sourceBase + y * width + x;
                     PixelData32* pOututPixel = outputBase + y * width + x;
                     byte blue = pSourcePixel->blue;
                     pOututPixel->alpha = (byte)(blue);
                     pOututPixel->red = (byte)(blue);
                     pOututPixel->blue = (byte)(blue);
                     pOututPixel->green = (byte)(blue);
                  }
               }
            }
            else if (sourceData.PixelFormat == PixelFormat.Format32bppArgb)
            {
               PixelData32* sourceBase = (PixelData32*)sourceData.Scan0;
               PixelData32* outputBase = (PixelData32*)outputData.Scan0;
               int width = source.Width;
               int height = source.Height;
               for (int x = 0; x < width; x++)
               {
                  for (int y = 0; y < height; y++)
                  {
                     PixelData32* pSourcePixel = sourceBase + y * width + x;
                     PixelData32* pOututPixel = outputBase + y * width + x;
                     byte alpha = pSourcePixel->alpha;
                     pOututPixel->alpha = (byte)(alpha);
                     pOututPixel->red = (byte)(alpha);
                     pOututPixel->blue = (byte)(alpha);
                     pOututPixel->green = (byte)(alpha);
                  }
               }
            }
            source.UnlockBits(sourceData);
            output.UnlockBits(outputData);
         }
         return output;
      }
   }

   public class BrushStroke
   {
      //used specifically for the dynamic brush
      public float mSpacing = 0;
      public float mMinSize = 100;
      public float mMaxSize = 100; // percent
      public float mAngleJitter = 0;//360;
      public float mPositionJitter = 0;
      public float mScatterCount = 1;

      //used more for direct brush access info
      public float mBrushRadius = 1;
      public float mAlphaValue = 1.0f;
      public float mRotationAmt = 0;

      public Graphics mG = null;
      float xLastDraw = -1;
      float yLastDraw = -1;

      public int cBufferSize = 512 + 1;
      Pen p = new Pen(Color.Black);
      Image mImage = null;
      Bitmap mFrameBuffer = null;

      public void SetDefaults()
      {
         mSpacing = 0;
         mMinSize = 100;
         mMaxSize = 100;
         mAngleJitter = 0;
         mPositionJitter = 0;
         mScatterCount = 1;
         mBrushRadius = 1;
         mAlphaValue = 1;
         mRotationAmt = 0;
         mG = null;
      }

      public int EstimateInfluence()
      {
         float radius;
         //radius = (mMaxSize / 2 + mPositionJitter);
         //radius = (mBrushRadius * mMaxSize / 10 + mPositionJitter);
         radius = (2 * (mBrushRadius + 1) * mLastRandomSizeMultiplier + mPositionJitter);
         return (int)radius;
      }


      public bool brushInput(float x, float y, bool newStroke)
      {
         //x = (int)x;
         //y = (int)y;
         if (mG == null)
         {
            xLastDraw = x;
            yLastDraw = y;
            clear();
            draw(x, y, mFrameBuffer);
            return true;
         }
         else
         {
            float distance = (float)Math.Sqrt(
            Math.Pow((x - xLastDraw), 2) +
            Math.Pow((y - yLastDraw), 2));

            if (distance > mSpacing)
            {
               xLastDraw = x;
               yLastDraw = y;
               clear();
               draw(x, y, mFrameBuffer);
               return true;
            }
         }
         return false;
      }
      private void clear()
      {
         if (mFrameBuffer != null)
            mFrameBuffer.Dispose();
         mFrameBuffer = new Bitmap(cBufferSize, cBufferSize);

      }
      //private Graphics getBuffer(Bitmap bitmap)
      //{

      //}
      public void draw(float x, float y, Bitmap buffer)
      {
         draw(x, y, buffer, false);
      }

      public void draw(float x, float y, Bitmap buffer, bool bOffset)
      {
         mG = Graphics.FromImage(buffer);
         mG.TranslateTransform(cBufferSize / 2, cBufferSize / 2);
         for (int k = 0; k < mScatterCount; k++)
         {

            if (bOffset)
            {
               x += Rand.mRandom.Next((int)mPositionJitter) - Rand.mRandom.Next((int)mPositionJitter);
               y += Rand.mRandom.Next((int)mPositionJitter) - Rand.mRandom.Next((int)mPositionJitter);

            }
            else
            {
               x = cBufferSize / 2 + Rand.mRandom.Next((int)mPositionJitter) - Rand.mRandom.Next((int)mPositionJitter);
               y = cBufferSize / 2 + Rand.mRandom.Next((int)mPositionJitter) - Rand.mRandom.Next((int)mPositionJitter);
            }

            //sizeRand is a percentage of 100
            float sizeRand = Rand.mRandom.Next((int)mMinSize, (int)mMaxSize) * 0.01f;

            //CLM the 4 here is a magic number that fixes the innacuracies between this system and the TextureManipulation class
            //This really needs to be fixed to a proper calculation instead of using a magic number...
            float size = (mBrushRadius * sizeRand) * 4;

            mLastRandomSizeMultiplier = sizeRand;



            mG.ResetTransform();
            mG.TranslateTransform(x, y);
            float rotAmt = (mRotationAmt * (180.0f / (float)(Math.PI)));
            mG.RotateTransform(rotAmt + Rand.mRandom.Next((int)mAngleJitter)); // 28 degrees
            mG.CompositingMode = CompositingMode.SourceOver;
            mG.CompositingQuality = CompositingQuality.HighQuality;
            mG.InterpolationMode = InterpolationMode.Bicubic;

            mG.DrawImage(mImage, -size / 2, -size / 2, size, size);
         }
         mG.Dispose();
      }

      float mLastRandomSizeMultiplier = 0;
      float mLastRandomXPos = 0;
      float mLastRandomYPos = 0;

      public Bitmap RenderPreview(int width, int height)
      {
         Bitmap buffer = new Bitmap(cBufferSize, cBufferSize);

         for (int x = 0; x < width - 30; x++)
         {
            //double phase = x / ((float)width / 10) + 1.3;
            //int y = (int)(Math.Sin(phase) * height / 10) + height / 2;
            //mBrushStroke.brushInput(x, y, newStroke);
            //newStroke = false;
            //int y =  cBufferSize / 2;// / 2;
            int y = height / 2;

            //*20 wtf?
            float distance = 0.25f * (float)Math.Sqrt(
            Math.Pow((x - xLastDraw), 2) +
            Math.Pow((y - yLastDraw), 2));

            if (distance > mSpacing)
            {
               xLastDraw = x;
               yLastDraw = y;
               draw(x, y, buffer, true);

            }
         }


         return buffer;
      }

      public BrushStroke()
      {
         SetBrushImage(CoreGlobals.getWorkPaths().mBrushMasks + @"\default.bmp");
      }
      public void SetBrushImage(string file)
      {
         Bitmap source = null;
         if (file.Contains(".bmp"))
         {
            source = (Bitmap)Bitmap.FromFile(file);
         }
         else if (file.Contains(".tga"))
         {
            Texture tex = TextureLoader.FromFile(BRenderDevice.getDevice(), file);
            Image img = Image.FromStream(TextureLoader.SaveToStream(ImageFileFormat.Bmp, tex));
            source = new Bitmap(img);

            tex.Dispose();
            tex = null;
            img = null;
         }

         SetBrushImage(source);
      }
      public void SetBrushImage(Bitmap bitmap)
      {
         try
         {

            mImage = GDIBrushSplats.generateAlpha(bitmap);
         }
         catch (System.Exception ex)
         {
            MessageBox.Show(ex.ToString());


         }
      }
      public void SetBrushImageNoAlpha(Bitmap bitmap)
      {
         mImage = new Bitmap(bitmap);
      }

      BitmapData sourceData = null;
      unsafe public UInt32* getBits()
      {
         Rectangle r = new Rectangle(0, 0, mFrameBuffer.Width, mFrameBuffer.Height);
         sourceData = mFrameBuffer.LockBits(r, ImageLockMode.ReadWrite, mFrameBuffer.PixelFormat);
         return (UInt32*)sourceData.Scan0;
      }
      public void unlockBits()
      {
         mFrameBuffer.UnlockBits(sourceData);
      }
   }





   public class Rand
   {
      static public Random mRandom = new Random(System.DateTime.Now.Millisecond);
      static int Get()
      {
         return mRandom.Next();
      }
      static public bool Chance(int percent)
      {
         return percent > mRandom.Next(100);
      }
   }

   #endregion

   //---------------------------------------------
   //---------------------------------------------
   //---------------------------------------------
   //---------------------------------------------
   //---------------------------------------------
   //---------------------------------------------
   //---------------------------------------------




}