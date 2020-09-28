
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Drawing.Imaging;
using System.Drawing.Drawing2D;
using System.IO;
using System.Text;
using System.Windows.Forms;

using EditorCore;

namespace graphapp
{
   public class GraphBasedMask : IMask
   {
      ArrayBasedMask mOutputMask = null;
      MemoryStream mGraphMemoryStream = null;
      int mCapacity = 2049 * 2049;

      public GraphBasedMask()
         : base()
      {
        
         mGraphMemoryStream = new MemoryStream();
      }
      public GraphBasedMask(BinaryReader b, long length)
         : base()
      {
         mGraphMemoryStream = new MemoryStream(b.ReadBytes((int)length));

      }

      public float GetMaskWeight(long index)
      {
         if (mOutputMask == null)
            if(mGraphMemoryStream != null)
               loadAndExecute();
            else
               return 0.0f;

         return mOutputMask.GetValue(index);
      }
      public void SetMaskWeight(long index, float value)
      {

      }
      public IMask Clone()
      {
         GraphBasedMask newCopy = new GraphBasedMask();
         mGraphMemoryStream.WriteTo(newCopy.mGraphMemoryStream);
         return newCopy;
      }
      public void Set(IMask m)
      {
         GraphBasedMask sameTypeMask = m as GraphBasedMask;
         if (sameTypeMask != null)
         {
            //base.Set(sameTypeMask);
         }
      }

      public void Clear()
      {
         if (mOutputMask!=null)
            mOutputMask.Clear();
      }
      public void ResetIterator()
      {
         if (mOutputMask == null)
            return;

         mOutputMask.ResetIterator();
      }
      public bool MoveNext(out long index, out float value)
      {
         if(mOutputMask==null)
         {
            index = 0;
            value = 0;
            return false;
         }

         return mOutputMask.MoveNext(out index,out value);
      }

      public bool HasData()
      {
         if (mOutputMask == null)
            return false;

         return mOutputMask.HasData();
      }

      public bool loadAndExecute()
      {
         if (mGraphMemoryStream == null)
            return false;

         mGraphMemoryStream.Seek(0, SeekOrigin.Begin);
         int tWidth = CoreGlobals.getEditorMain().mITerrainShared.getNumXVerts();
         int tHeight = CoreGlobals.getEditorMain().mITerrainShared.getNumXVerts();

         DAGCanvas dg = new DAGCanvas(512, 512);
         dg.loadCanvasFromMemoryStream(mGraphMemoryStream);
         DAGMask resMask = dg.execute(tWidth, tHeight);

         if (resMask == null)
         {
            MessageBox.Show("There was an error computing Mask");
            return false;
         }

         mOutputMask = new ArrayBasedMask(mCapacity);

      
         for (int x = 0; x < tWidth; x++)
         {
            for (int y = 0; y < tHeight; y++)
            {
               mOutputMask.SetMaskWeight(x * tWidth+y, resMask[x, y]);
            }
         }
         dg.newCanvas();
         dg = null;
         mGraphMemoryStream.Seek(0, SeekOrigin.Begin);
         return true;
      }
      public MemoryStream GraphMemStream
      {
         get { return mGraphMemoryStream; }
      }
      public ArrayBasedMask getOutputMask()
      {
         return mOutputMask;
      }
      public void clearMemStream()
      {
         mGraphMemoryStream.Close();
         mGraphMemoryStream.Dispose();
         mGraphMemoryStream = null;
         mGraphMemoryStream = new MemoryStream();

      }
   }
}