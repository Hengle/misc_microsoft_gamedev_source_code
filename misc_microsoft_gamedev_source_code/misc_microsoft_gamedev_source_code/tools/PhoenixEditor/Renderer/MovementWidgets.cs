using System;
using System.Collections.Generic;
using System.Text;
using Rendering;
using Microsoft.DirectX;
using Microsoft.DirectX.Direct3D;
using System.IO;
using System.Drawing;



using Rendering;
using EditorCore;

//CLM [04.29.06] This file to contain interface widgets

namespace SimEditor
{
   //CLM [04.29.06] This class is a 3DSM gimmick widget. All intersection and rendering is self contained.
   public class Widget
   {
      public Vector3 mTranslation;
      protected bool mXaxisSelected = false;
      protected bool mYaxisSelected = false;
      protected bool mZaxisSelected = false;

      protected Vector3 mVisibleScale = new Vector3(1, 1, 1);

      protected bool mLocked = false;

      public virtual void init()
      {
      }
      public virtual void destroy()
      {
      }

      public virtual bool input() { return false; }
      public virtual void update(Vector3 trans) { }
      public virtual void render() { }


      public virtual void calculateScale() 
      {
         if (mLocked) return;

         //use a rough estimate to ensure we're the same size on the screen (relatively)
         float mRadius = 12.3f;
         Vector3 v = CoreGlobals.getEditorMain().mITerrainShared.getCameraPos() - mTranslation;
         float d = v.Length();
         mVisibleScale.X = d / mRadius;
         mVisibleScale.Y = d / mRadius;
         mVisibleScale.Z = d / mRadius;   
      }
      
      public bool isLocked()
      {
         return mLocked;
      }
      public void lockIntersection()
      {
         mLocked = true;
      }
      public void unlockIntersection()
      {
         mLocked = false;
      }
      public void setSelected(bool x, bool y, bool z)
      {
         mXaxisSelected = x;
         mYaxisSelected = y;
         mZaxisSelected = z;
      }
      public void invertSelected()
      {
         mXaxisSelected = !mXaxisSelected;
         mYaxisSelected = !mYaxisSelected;
         mZaxisSelected = !mZaxisSelected;
      }
      public virtual bool testIntersection() { return false; }

      public void clear()
      {
         mXaxisSelected = true;
         mYaxisSelected = true;
         mZaxisSelected = true;
         mLocked = false;
      }
      public Vector3 getPosition()
      {
         return mTranslation;
      }
   }

   //CLM [04.29.06] This class is a 3DSM gimmick widget. All intersection and rendering is self contained.
   public class TranslationWidget : Widget
   {
      float[] mHitDist = new float[3];

      const float mRodLength = 2.0f;
      const float mRodWidth = 0.3f;


      //visuals
      BRenderDebugCylinder mAxisRodRed;
      BRenderDebugCylinder mAxisRodGreen;
      BRenderDebugCylinder mAxisRodBlue;
      BRenderDebugCylinder mAxisRodSelected;

      BRenderDebugCone mAxisConeRed;
      BRenderDebugCone mAxisConeGreen;
      BRenderDebugCone mAxisConeBlue;

      BRenderDebug2DQuad mAxisQuad;
      BRenderDebug2DQuad mAxisQuadSelected;

      BRenderDebug2DQuad mAxisXZQuad;
      BRenderDebug2DQuad mAxisXZQuadSelected;

      Vector3 mConeTranslation = new Vector3(0, 1.9f, 0);


      public TranslationWidget()
      {

      }
      ~TranslationWidget()
      {
         destroy();
      }

      public override void init()
      {

         //create our visual representations
         mAxisRodRed       = new BRenderDebugCylinder(mRodLength, 0.02f, System.Drawing.Color.Red.ToArgb());
         mAxisRodGreen     = new BRenderDebugCylinder(mRodLength, 0.02f, System.Drawing.Color.Green.ToArgb());
         mAxisRodBlue      = new BRenderDebugCylinder(mRodLength, 0.02f, System.Drawing.Color.Blue.ToArgb());
         mAxisRodSelected  = new BRenderDebugCylinder(mRodLength, 0.02f, System.Drawing.Color.Yellow.ToArgb());

         mAxisConeRed      = new BRenderDebugCone(0.7f, mRodWidth, System.Drawing.Color.Red.ToArgb(),false,true);
         mAxisConeGreen = new BRenderDebugCone(0.7f, mRodWidth, System.Drawing.Color.Green.ToArgb(), false,true);
         mAxisConeBlue     = new BRenderDebugCone(0.7f, mRodWidth, System.Drawing.Color.Blue.ToArgb(),false,true);

         float hr = mRodLength/3.0f;
         mAxisQuad = new BRenderDebug2DQuad(new Vector2(0, 0), new Vector2(hr, hr), Color.White.ToArgb(), false);
         mAxisQuadSelected = new BRenderDebug2DQuad(new Vector2(0, 0), new Vector2(hr, hr), unchecked((int)0x88FFFFFF), true);

         mAxisXZQuad = new BRenderDebug2DQuad(new Vector2(0, 0), new Vector2(mRodLength * 0.75f, mRodLength * 0.75f), Color.White.ToArgb(), false);
         mAxisXZQuadSelected = new BRenderDebug2DQuad(new Vector2(0, 0), new Vector2(mRodLength * 0.75f, mRodLength * 0.75f), unchecked((int)0x88FFFFFF), true);

      }
      public override void destroy()
      {
         mAxisRodRed.destroy();
         mAxisRodGreen.destroy();
         mAxisRodBlue.destroy();
         mAxisRodSelected.destroy();

         mAxisConeRed.destroy();
         mAxisConeGreen.destroy();
         mAxisConeBlue.destroy();

         mAxisQuad.destroy();
         mAxisQuadSelected.destroy();

         mAxisXZQuad.destroy();
         mAxisXZQuadSelected.destroy();
      }

      //selection testing
      private BBoundingBox axisRodBox(int axis)
      {
         BBoundingBox bbox = new BBoundingBox();
         bbox.min.X = -mRodWidth;
         bbox.min.Y = -mRodWidth;
         bbox.min.Z = -mRodWidth;

         if(axis ==0)   //X
         {
            bbox.max.X = mRodLength + 0.7f; ;
            bbox.max.Y = mRodWidth;
            bbox.max.Z = mRodWidth;
         }
         else if (axis == 1)   //Y
         {
            bbox.max.X = mRodWidth;
            bbox.max.Y = mRodLength + 0.7f;  ;
            bbox.max.Z = mRodWidth;
         }
         else if (axis == 2)   //Z
         {
            bbox.max.X = mRodWidth;
            bbox.max.Y = mRodWidth;
            bbox.max.Z = mRodLength + 0.7f; ;
         }

         bbox.min *= mVisibleScale.Y;
         bbox.max *= mVisibleScale.Y;

         return bbox;
      }
      private BBoundingBox axisPlaneBox(int axis)
      {
         BBoundingBox bbox = new BBoundingBox();
         bbox.min.X = -0.1f;
         bbox.min.Y = -0.1f;
         bbox.min.Z = -0.1f;

         float hl = mRodLength / 3.0f;
         if (axis == 0)   //X
         {
            bbox.max.X = hl;
            bbox.max.Y = hl;
            bbox.max.Z = 0.1f;
         }
         else if (axis == 1)   //Y
         {
            bbox.max.X = mRodLength * 0.75f;
            bbox.max.Y = 0.1f;
            bbox.max.Z = mRodLength * 0.75f;
         }
         else if (axis == 2)   //Z
         {
            bbox.max.X = 0.1f;
            bbox.max.Y = hl;
            bbox.max.Z = hl;
         }

         bbox.min *= mVisibleScale.Y;
         bbox.max *= mVisibleScale.Y;

         return bbox;
      }
      private bool testPlanesIntersection(Vector3 r0, Vector3 rD)
      {
         mHitDist[0] = float.MaxValue;
         mHitDist[1] = float.MaxValue;
         mHitDist[2] = float.MaxValue;

         Vector3 tc = Vector3.Empty;
         Vector3 tOrg = r0 - mTranslation;
         BBoundingBox yxBox = axisPlaneBox(0);
         if (!BMathLib.ray3AABB(ref tc, ref mHitDist[0], ref tOrg, ref rD, ref yxBox.min, ref yxBox.max))
            mHitDist[0] = float.MaxValue;

         BBoundingBox zxBox = axisPlaneBox(1);
         if(!BMathLib.ray3AABB(ref tc, ref mHitDist[1], ref tOrg, ref rD, ref zxBox.min, ref zxBox.max))
            mHitDist[1] = float.MaxValue;

         BBoundingBox yzBox = axisPlaneBox(2);
         if(!BMathLib.ray3AABB(ref tc, ref mHitDist[2], ref tOrg, ref rD, ref yzBox.min, ref yzBox.max))
            mHitDist[2] = float.MaxValue;

         if (mHitDist[0] < mHitDist[1] && mHitDist[0] < mHitDist[2])
            mXaxisSelected = mYaxisSelected = true;
         else if (mHitDist[1] < mHitDist[0] && mHitDist[1] < mHitDist[2])
            mXaxisSelected = mZaxisSelected = true;
         else if (mHitDist[2] < mHitDist[1] && mHitDist[2] < mHitDist[0])
            mZaxisSelected = mYaxisSelected = true;

         return mXaxisSelected | mYaxisSelected | mZaxisSelected;
      }
      private bool testRodsIntersection(Vector3 r0, Vector3 rD)
      {
         Vector3 coord = Vector3.Empty;
         float t = 0;

         Vector3 tOrg = r0 - mTranslation;

         BBoundingBox xBox = axisRodBox(0);
         mXaxisSelected = BMathLib.ray3AABB(ref coord, ref t, ref tOrg, ref rD, ref xBox.min, ref xBox.max);

         BBoundingBox yBox = axisRodBox(1);
         mYaxisSelected = BMathLib.ray3AABB(ref coord, ref t, ref tOrg, ref rD, ref yBox.min, ref yBox.max);

         BBoundingBox zBox = axisRodBox(2);
         mZaxisSelected = BMathLib.ray3AABB(ref coord, ref t, ref tOrg, ref rD, ref zBox.min, ref zBox.max);

         return mXaxisSelected | mYaxisSelected | mZaxisSelected;
      }


      public override bool testIntersection()
      {
         if (mLocked) return true;

         mXaxisSelected = mYaxisSelected = mZaxisSelected = false;

         //cast a ray
         //find what axis it's hitting..
         Point cursorPos = new Point();
         UIManager.GetCursorPos(ref cursorPos);
         Vector3 orig = BRenderDevice.getRayPosFromMouseCoords(false, cursorPos);
         Vector3 dir = BRenderDevice.getRayPosFromMouseCoords(true, cursorPos) - orig;
         dir =BMathLib.Normalize(dir);

         //test intersection against our entire widget first
            BBoundingBox myBox = new BBoundingBox();
            myBox.min = mTranslation - (new Vector3(mRodWidth, mRodWidth, mRodWidth)*mVisibleScale.Y);
            myBox.max = mTranslation + (new Vector3(mRodLength + 0.5f, mRodLength + 0.5f, mRodLength + 0.5f) * mVisibleScale.Y);
            if (!myBox.intersect(orig, dir))
               return false;
            
         //if we didn't hit a plane, check our boxes
         if (!testPlanesIntersection(orig, dir))
            testRodsIntersection(orig, dir);
          

         return mXaxisSelected | mYaxisSelected | mZaxisSelected;
      }
    
      
      public void capMovement(ref Vector3 desiredVector)
      {
         desiredVector.X = desiredVector.X * (int)(mXaxisSelected ? 1 : 0);
         desiredVector.Y = desiredVector.Y * (int)(mYaxisSelected ? 1 : 0);
         desiredVector.Z = desiredVector.Z * (int)(mZaxisSelected ? 1 : 0);
      }


      public override bool input()
      {
         return true;
      }

      public override void update(Vector3 trans)
      {
         mTranslation = trans;
         calculateScale();

      }
      public override void render()
      {
         BRenderDevice.getDevice().VertexShader = null;
         BRenderDevice.getDevice().PixelShader = null;

         BRenderDevice.getDevice().SetRenderState(RenderStates.ZEnable, false);
         BRenderDevice.getDevice().VertexDeclaration = VertexTypes.Pos_Color.vertDecl;         
         BRenderDevice.getDevice().SetTexture(0, null);

         //render rods and cones
         BRenderDevice.getDevice().Transform.World = Matrix.Scaling(mVisibleScale) * Matrix.RotationZ(Geometry.DegreeToRadian((float)-90)) * Matrix.Translation(mTranslation); ;
         if (mXaxisSelected) mAxisRodSelected.render();
         else mAxisRodRed.render();
         BRenderDevice.getDevice().Transform.World = Matrix.Scaling(mVisibleScale) * Matrix.Translation(mConeTranslation * mVisibleScale.Y) * Matrix.RotationZ(Geometry.DegreeToRadian((float)-90)) * Matrix.Translation(mTranslation);
         mAxisConeRed.render();

         BRenderDevice.getDevice().Transform.World = Matrix.Scaling(mVisibleScale) * Matrix.RotationX(Geometry.DegreeToRadian((float)90)) * Matrix.Translation(mTranslation); ;
         if (mZaxisSelected) mAxisRodSelected.render();
         else mAxisRodBlue.render();
         BRenderDevice.getDevice().Transform.World = Matrix.Scaling(mVisibleScale) * Matrix.Translation(mConeTranslation * mVisibleScale.Y) * Matrix.RotationX(Geometry.DegreeToRadian((float)90)) * Matrix.Translation(mTranslation);
         mAxisConeBlue.render();



         //render planes

         BRenderDevice.getDevice().Transform.World = Matrix.Scaling(mVisibleScale) * Matrix.RotationX(Geometry.DegreeToRadian((float)-90)) * Matrix.Translation(mTranslation);
         if (mXaxisSelected && mYaxisSelected) mAxisQuadSelected.render(true);
         mAxisQuad.render();

         BRenderDevice.getDevice().Transform.World = Matrix.Scaling(mVisibleScale) * Matrix.RotationZ(Geometry.DegreeToRadian((float)90)) * Matrix.Translation(mTranslation);
         if (mYaxisSelected && mZaxisSelected) mAxisQuadSelected.render(true);
         mAxisQuad.render();

         BRenderDevice.getDevice().Transform.World = Matrix.Scaling(mVisibleScale) * Matrix.Translation(mTranslation);
         if (mXaxisSelected && mZaxisSelected) mAxisXZQuadSelected.render(true);
         mAxisXZQuad.render();

         //render Y arrow
         BRenderDevice.getDevice().Transform.World = Matrix.Scaling(mVisibleScale) * Matrix.Translation(mTranslation);
         if (mYaxisSelected) mAxisRodSelected.render();
         else mAxisRodGreen.render();
         BRenderDevice.getDevice().Transform.World = Matrix.Scaling(mVisibleScale) * Matrix.Translation(mConeTranslation * mVisibleScale.Y) * Matrix.Translation(mTranslation);
         mAxisConeGreen.render();

         //reset
         
         BRenderDevice.getDevice().SetRenderState(RenderStates.ZEnable, true);
         BRenderDevice.getDevice().Transform.World = Matrix.Identity;


         
      }
   }

   //CLM [05.01.06] This class is a 3DSM gimmick widget. All intersection and rendering is self contained.
   public class RotationWidget : Widget
   {
      BRenderDebug2DCircle mXAxisCircle = null;
      BRenderDebug2DCircle mYAxisCircle = null;
      BRenderDebug2DCircle mZAxisCircle = null;

      BRenderDebug2DCircle mAxisCircleSelected = null;

      BRenderDebugSphere mSphere = null;

      Vector3 hitPt = Vector3.Empty;

      public RotationWidget()
      {
       

      }

      ~RotationWidget()
      {
         destroy();
      }

      public override void init()
      {
         mXAxisCircle = new BRenderDebug2DCircle(1, Color.Red.ToArgb());
         mYAxisCircle = new BRenderDebug2DCircle(1, Color.Green.ToArgb());
         mZAxisCircle = new BRenderDebug2DCircle(1, Color.Blue.ToArgb());

         mAxisCircleSelected = new BRenderDebug2DCircle(1, Color.Yellow.ToArgb());

         mSphere = new BRenderDebugSphere(1, unchecked((int)0x88FFFFFF));
      }
      public override void destroy()
      {

      }


      public override bool testIntersection()
      {
         if (mLocked) return true;

         mXaxisSelected = mYaxisSelected = mZaxisSelected = false;

         //find what axis it's hitting..
         Point cursorPos = new Point();
         UIManager.GetCursorPos(ref cursorPos);
         Vector3 orig = BRenderDevice.getRayPosFromMouseCoords(false, cursorPos);
         Vector3 dir = BRenderDevice.getRayPosFromMouseCoords(true, cursorPos) - orig;
         dir=BMathLib.Normalize(dir);

         //test intersection against our entire widget first
         float tVal = 0;
         if (!BMathLib.raySphereIntersect(mTranslation, mVisibleScale.Y, orig, dir, ref tVal))
            return false;

         //we hit the sphere. Find out what axis we're interested in
         Vector3 pt = orig + (dir * tVal);

         Plane xPlane = Plane.FromPoints(mTranslation, mTranslation + new Vector3(0, 1, 0), mTranslation + new Vector3(0, 0, 1));
         Plane yPlane = Plane.FromPoints(mTranslation, mTranslation + new Vector3(1, 0, 0), mTranslation + new Vector3(0, 0, 1));
         Plane zPlane = Plane.FromPoints(mTranslation, mTranslation + new Vector3(1, 0, 0), mTranslation + new Vector3(0, 1, 0));

         float tol = 0.35f * mVisibleScale.Y;
         float yDist = Math.Abs(BMathLib.pointPlaneDistance(pt, yPlane));// < tol;
         float xDist = Math.Abs(BMathLib.pointPlaneDistance(pt, xPlane));// < tol;
         float zDist = Math.Abs(BMathLib.pointPlaneDistance(pt, zPlane));// < tol;

         mXaxisSelected = mYaxisSelected = mZaxisSelected = false;


         if (xDist < yDist && xDist < zDist)
            mXaxisSelected = xDist < tol;
         else if (yDist < xDist && yDist < zDist)
            mYaxisSelected = yDist < tol;
         else if (zDist < xDist && zDist < yDist)
            mZaxisSelected = zDist < tol;


         if (mYaxisSelected) { mXaxisSelected = mZaxisSelected = false; }
         if (mZaxisSelected) { mYaxisSelected = mXaxisSelected = false; }
         if (mXaxisSelected) { mYaxisSelected = mZaxisSelected = false; } 

         return mXaxisSelected | mYaxisSelected | mZaxisSelected;
      }
      public void capRotation(ref Vector3 desiredVector)
      {
         desiredVector.X = desiredVector.X * (int)(mXaxisSelected ? 1 : 0);
         desiredVector.Y = desiredVector.Y * (int)(mYaxisSelected ? 1 : 0);
         desiredVector.Z = desiredVector.Z * (int)(mZaxisSelected ? 1 : 0);
      }

      public override bool input()
      {
         return true;
      }

      public override void update(Vector3 trans)
      {
         mTranslation = trans;
         calculateScale();
      }

      public override void render()
      {
         BRenderDevice.getDevice().VertexShader = null;
         BRenderDevice.getDevice().PixelShader = null;
         BRenderDevice.getDevice().VertexDeclaration = VertexTypes.Pos_Color.vertDecl;
         BRenderDevice.getDevice().SetTexture(0, null);

         BRenderDevice.clear(true, false, 0, 1.0f, 0);

         BRenderDevice.getDevice().RenderState.CullMode = Cull.Clockwise;
         BRenderDevice.getDevice().Transform.World = Matrix.Scaling(mVisibleScale*0.98f) * Matrix.Translation(mTranslation);
         mSphere.render(false,true);

         BRenderDevice.getDevice().SetRenderState(RenderStates.ZEnable, true);

         BRenderDevice.getDevice().RenderState.CullMode = Cull.CounterClockwise;

         BRenderDevice.getDevice().Transform.World = Matrix.Scaling(mVisibleScale) * Matrix.RotationX(Geometry.DegreeToRadian((float)-90)) * Matrix.Translation(mTranslation);
         if (mZaxisSelected) mAxisCircleSelected.render();
         else mZAxisCircle.render();

         BRenderDevice.getDevice().Transform.World = Matrix.Scaling(mVisibleScale) * Matrix.RotationZ(Geometry.DegreeToRadian((float)90)) * Matrix.Translation(mTranslation);
         if (mXaxisSelected) mAxisCircleSelected.render();
         else mXAxisCircle.render();

         BRenderDevice.getDevice().Transform.World = Matrix.Scaling(mVisibleScale) * Matrix.Translation(mTranslation);
         if (mYaxisSelected) mAxisCircleSelected.render();
         else mYAxisCircle.render();

         

         //reset
         BRenderDevice.getDevice().Transform.World = Matrix.Identity;
         
      }
   }

   
}