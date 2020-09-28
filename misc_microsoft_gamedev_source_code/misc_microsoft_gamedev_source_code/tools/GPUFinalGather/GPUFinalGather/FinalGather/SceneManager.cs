using System;
using Microsoft.DirectX;
using Microsoft.DirectX.Direct3D;
using Microsoft.DirectX.DirectInput;
using System.Drawing;
using System.Diagnostics;

using Rendering;
using EditorCore;

namespace FinalGather
{
   class SceneManager
   {
      public class SimpleScene
      {
         const int numfloors = 3;
         BRenderDebug2DQuad[] mFloor = null;
         BRenderDebugSphere mSphere = null;
         int mNumSceneObjects = numfloors + 1;

         Vector3 mWorldMin = Vector3.Empty;
         Vector3 mWorldMax = Vector3.Empty;

         public Vector3 getWorldMin()
         {
            return mWorldMin;
         }
         public Vector3 getWorldMax()
         {
            return mWorldMax;
         }
         public int getNumObjects()
         {
            return mNumSceneObjects;
         }

         public void initScene()
         {
            mWorldMin = new Vector3(0, -1, 0);
            mWorldMax = new Vector3(30, numfloors * 2 + 2, 30);

            //CLM temp, we should be rendering terrain here...
            Color[] FloorColors = new Color[numfloors] { Color.Red, Color.Green, Color.Blue };
            mFloor = new BRenderDebug2DQuad[numfloors];
            float fheight = 0;
            int heightInc = 2;
            for (int i = 0; i < numfloors; i++)
            {
               Vector3 off = new Vector3(0, fheight, 0);
               mFloor[i] = new BRenderDebug2DQuad(new Vector2(mWorldMin.X, mWorldMin.Z), new Vector2(mWorldMax.X, mWorldMax.Z), FloorColors[i].ToArgb(), true, off);
               fheight += heightInc;
            }


            mSphere = new BRenderDebugSphere(3, 1, Color.Orange.ToArgb(), new Vector3(15, fheight + 1, 15));

            
         }
         public void destroy()
         {
            for (int i = 0; i < numfloors; i++)
            {
               if (mFloor[i] != null)
               {
                  mFloor[i].destroy();
                  mFloor[i] = null;
               }
            }
            mSphere.destroy();
            mSphere = null;
         }
         public void renderSceneObject(int index)
         {
            if (index < 0 || index >= mNumSceneObjects)
               return;

            if (index >= numfloors)
            {
               mSphere.render(false, false, true);
            }
            else
            {
               mFloor[index].render(false, false, true);
            }
         }
         public void renderScene()
         {
            for (int i = 0; i < mNumSceneObjects; i++)
               renderSceneObject(i);
         }
      }
   }
}