using System;
using System.IO;
using System.Drawing;
using System.Collections.Generic;
using System.Text;
using Microsoft.DirectX;
using Microsoft.DirectX.Direct3D;
using Microsoft.DirectX.DirectInput;
using System.Windows.Forms;
using Rendering;
using EditorCore;
using SimEditor;
////
//---------------------------------------
namespace Terrain
{
   #region CameraManager

   public class BCameraManager
   {
      bool[] keyDownLastFrame = new bool[256];


      static public Vector3 GameCamLookat = new Vector3(30, 50, 30);
      public Vector3 mEye = new Vector3(-GameCamLookat.X, GameCamLookat.Y, -GameCamLookat.Z);
      public Vector3 mLookAt = new Vector3(GameCamLookat.X, 0, GameCamLookat.Z);
      public Vector3 mUp = new Vector3(0, 1, 0);
      public Vector3 mRight = new Vector3(1, 0, 0);
      Vector3 Dir = new Vector3(0, 0, 1);
      private float mCamDotLimit = 0.99f;

      float mZoom = 1;

      public bool mbInvertZoom = false;

      public float Zoom
      {
         set
         {
            //if (value > 0.2f && value < 10)
            {
               mZoom = value;
            }
         }
         get
         {
            return mZoom;
         }
      }

      public enum eCamSnaps
      {
         cCamSnap_RTS = 0,
         cCamSnap_posY,
         cCamSnap_posX,
         cCamSnap_negX,
         cCamSnap_posZ,
         cCamSnap_negZ,
         cCamSnap_ModelView,
         cCamSnap_Map_Center
      }
      public enum eCamModes
      {
         cCamMode_Free = 0,
         cCamMode_RTS,
         cCamMode_ModelView     // Used in visual editor
      }

      eCamModes mCamMode = eCamModes.cCamMode_Free;


      bool mbFirstMouseIntput = true;
      Vector3 mTranlationInterception = new Vector3();

      bool mTranslationPressed = false;
      int mLastMousePosX = 0;
      int mLastMousePosY = 0;


      BBoundingBox   mModelBoundingBox = new BBoundingBox();
      float          mModelBoundingBoxMaxSize = 0.0f;

      //-----------------------------------------------------------------------------
      public void snapCamera(eCamSnaps snap)
      {
         float worldX = TerrainGlobals.getTerrain().getWorldSizeX();
         float halfWorldX = worldX / 2.0f;
         Vector3 center = new Vector3(halfWorldX, 0, halfWorldX);

         GameCamLookat.X = -30;
         GameCamLookat.Y = 50;
         GameCamLookat.Z = 30;


         switch (snap)
         {
            case eCamSnaps.cCamSnap_RTS:
               mEye.X = mLookAt.X - GameCamLookat.X;
               mEye.Z = mLookAt.Z - GameCamLookat.Z;
               mEye.Y = mLookAt.Y + GameCamLookat.Y;
               break;
            case eCamSnaps.cCamSnap_Map_Center:
               mEye.X = halfWorldX;
               mEye.Z = halfWorldX + 0.5f;
               mEye.Y = mLookAt.Y + GameCamLookat.Y;
               mLookAt = center;
               break;
            case eCamSnaps.cCamSnap_posY:
               mEye.X = halfWorldX;
               mEye.Z = halfWorldX + 0.5f;
               mEye.Y = worldX * 2;
               mLookAt = center;
               break;
            case eCamSnaps.cCamSnap_posX:
               mEye.X = worldX * 2;
               mEye.Z = halfWorldX;
               mEye.Y = 0;
               mLookAt = center;
               break;
            case eCamSnaps.cCamSnap_negX:
               mEye.X = -worldX;
               mEye.Z = halfWorldX;
               mEye.Y = 0;
               mLookAt = center;
               break;
            case eCamSnaps.cCamSnap_posZ:
               mEye.X = halfWorldX;
               mEye.Z = worldX * 2;
               mEye.Y = 0;
               mLookAt = center;
               break;
            case eCamSnaps.cCamSnap_negZ:
               mEye.X = halfWorldX;
               mEye.Z = -worldX;
               mEye.Y = 0;
               mLookAt = center;
               break;

            case eCamSnaps.cCamSnap_ModelView:
               {
                  Vector3 modelCenter = mModelBoundingBox.getCenter();

                  float sizeX = mModelBoundingBox.max.X - mModelBoundingBox.min.X;
                  float sizeY = mModelBoundingBox.max.Y - mModelBoundingBox.min.Y;
                  float sizeZ = mModelBoundingBox.max.Z - mModelBoundingBox.min.Z;

                  float maxSize = float.MinValue;
                  maxSize = Math.Max(maxSize, sizeX);
                  maxSize = Math.Max(maxSize, sizeY);
                  maxSize = Math.Max(maxSize, sizeZ);

                  float cameraDistanceXZ = maxSize * 2.0f;
                  float cameraDistanceY = maxSize * 1.0f;

                  mLookAt = modelCenter;
                  mEye = modelCenter + new Vector3(cameraDistanceXZ, cameraDistanceY, cameraDistanceXZ);
               }

               break;
         }
      }
      //-----------------------------------------------------------------------------
      public void setCamMode(eCamModes mode)
      {
         mCamMode = mode;
      }
      //-----------------------------------------------------------------------------
      public void setModelBoundingBox(BBoundingBox box)
      {
         mModelBoundingBox = box;
         mModelBoundingBoxMaxSize = mModelBoundingBox.getMaxSize();
      }
      //-----------------------------------------------------------------------------
      public void CheckAssignMovement(out Vector3 outVec, Vector3 Pos, Vector3 movAmt)
      {
         outVec.X = 0;
         outVec.Y = 0;
         outVec.Z = 0;
         if (Pos.X + movAmt.X > 0 && Pos.X + movAmt.X < TerrainGlobals.getTerrain().getWorldSizeX())
            outVec.X = movAmt.X;
         if (Pos.Z + movAmt.Z > 0 && Pos.Z + movAmt.Z < TerrainGlobals.getTerrain().getWorldSizeX())
            outVec.Z = movAmt.Z;

      }
      //-----------------------------------------------------------------------------
      public void camUpdate()
      {
         Matrix View;

         View = Matrix.LookAtLH(mEye, mLookAt, mUp);
         BRenderDevice.getDevice().SetTransform(TransformType.View, View);
      }

      public float SmartSpeedupCamera()
      {
         //Vector3 dir = Eye - LookAt;
         //float altidir.Length();
         float min = 10;
         float max = 1000;

         float minmod = 0.5f;
         float maxmod = 12f;

         float clampedRange = Math.Abs(mEye.Y);
         if (clampedRange > max)
            clampedRange = max;
         if (clampedRange < min)
            clampedRange = min;

         float modifier = minmod + (maxmod - minmod) * (clampedRange / max);
         return modifier;

      }


      //-----------------------------------------------------------------------------
      public void CameraMovement()
      {
         //camera controls
         float incAmt = 2 * SmartSpeedupCamera();
         Dir = mEye - mLookAt;
         mRight = Vector3.Cross(Dir, mUp);
         mRight = BMathLib.Normalize(mRight);

         Vector3 ndir = -Dir;
         ndir.Y = 0;
         ndir = BMathLib.Normalize(ndir);

         Vector3 kDir = BMathLib.Normalize(Dir);

         Vector3 inc;
         Matrix matRotation;

         int mouseDeltaX = 0;
         int mouseDeltaY = 0;
         Point currPos = Point.Empty;
         UIManager.GetCursorPos(ref currPos);


         if (mbFirstMouseIntput)
         {
            mLastMousePosX = currPos.X;
            mLastMousePosY = currPos.Y;
            mbFirstMouseIntput = false;
         }
         else
         {
            mouseDeltaX = mLastMousePosX - currPos.X;
            mouseDeltaY = mLastMousePosY - currPos.Y;
            mLastMousePosX = currPos.X;
            mLastMousePosY = currPos.Y;
         }




         switch (mCamMode)
         {
            case eCamModes.cCamMode_Free:
            case eCamModes.cCamMode_RTS:

               if (mCamMode == eCamModes.cCamMode_RTS) //EDGE PUSH MODE
               {
                  const int edgeAmt = 20;
                  Point size = new Point();

                  BRenderDevice.getParentWindowSize(ref size);

                  Point cursorPos = Point.Empty;
                  UIManager.GetCursorPos(ref cursorPos);

                  if (cursorPos.Y < edgeAmt)
                  {
                     CheckAssignMovement(out inc, mLookAt, ndir * incAmt);
                     mEye += inc;
                     mLookAt += inc;
                  }
                  else if (cursorPos.Y > size.Y - edgeAmt)
                  {
                     CheckAssignMovement(out inc, mLookAt, -ndir * incAmt);
                     mEye += inc;
                     mLookAt += inc;
                  }
                  else if (cursorPos.X < edgeAmt)
                  {
                     CheckAssignMovement(out inc, mLookAt, -mRight * incAmt);
                     mEye += inc;
                     mLookAt += inc;
                  }
                  else if (cursorPos.X > size.X - edgeAmt)
                  {
                     CheckAssignMovement(out inc, mLookAt, mRight * incAmt);
                     mEye += inc;
                     mLookAt += inc;
                  }
               }


               if (SimGlobals.getSimMain().isUsingDesignerControls())
               {
                  if (UIManager.GetMouseButtonDown(UIManager.eMouseButton.cMiddle) && (UIManager.GetAsyncKeyStateB(Key.LeftAlt) || UIManager.GetAsyncKeyStateB(Key.RightAlt)))
                  {
                     //Rotation!
                     if (mouseDeltaX != 0 || mouseDeltaY != 0)
                     {
                        matRotation = Matrix.RotationAxis(mRight, Geometry.DegreeToRadian((float)-mouseDeltaY / 5.0f));
                        Dir.TransformCoordinate(matRotation);
                        Vector3 t = Vector3.Normalize(Dir);
                        if (Vector3.Dot(t, mUp) < mCamDotLimit && Vector3.Dot(t, mUp) > -mCamDotLimit)
                           mEye = mLookAt + Dir;

                        matRotation = Matrix.RotationY(Geometry.DegreeToRadian((float)-mouseDeltaX / 5.0f));
                        Dir.TransformCoordinate(matRotation);
                        t = Vector3.Normalize(Dir);
                        if (Vector3.Dot(t, mUp) < mCamDotLimit && Vector3.Dot(t, mUp) > -mCamDotLimit)
                           mEye = mLookAt + Dir;
                     }
                     mTranslationPressed = false;
                  }
                  else if (UIManager.GetMouseButtonDown(UIManager.eMouseButton.cMiddle))
                  {

                     if (mTranslationPressed)
                     {
                        float yE = mEye.Y;
                        float yL = mLookAt.Y;
                        Vector3 r0 = TerrainGlobals.getEditor().getRayPosFromMouseCoords(false);
                        Vector3 rD = TerrainGlobals.getEditor().getRayPosFromMouseCoords(true) - r0;
                        rD.Normalize();
                        Vector3 iPt = new Vector3();
                        BTerrainQuadNode node = null;
                        TerrainGlobals.getTerrain().rayIntersects(ref r0, ref rD, ref iPt, ref node, true);
                        Vector3 diff = mTranlationInterception - iPt;
                        Vector3 eyeDiff = mLookAt - mEye;

                        mLookAt += diff;
                        mEye += diff;

                        mLookAt.Y = yL;
                        mEye.Y = yE;

                     }
                     else
                     {
                        mTranslationPressed = true;
                        Vector3 r0 = TerrainGlobals.getEditor().getRayPosFromMouseCoords(false);
                        Vector3 rD = TerrainGlobals.getEditor().getRayPosFromMouseCoords(true) - r0;
                        rD.Normalize();
                        BTerrainQuadNode node = null;
                        TerrainGlobals.getTerrain().rayIntersects(ref r0, ref rD, ref mTranlationInterception, ref node, true);


                     }
                     /*
                     //TRANSLATION
                     float lSq = Eye.Y * 0.3f;
                     float forwardInc = (float)mouseDeltaY / -20.0f;
                     float rightInc = (float)mouseDeltaX / 20.0f;

                     Vector3 k = ndir * forwardInc + Right * rightInc;

                     CheckAssignMovement(out inc, LookAt, k);
                     Eye += inc;
                     LookAt += inc;
                      * */
                  }
                  else
                  {
                     mTranslationPressed = false;
                  }

                  if ((UIManager.WheelDelta != 0) && (UIManager.GetAsyncKeyStateB(Key.LeftAlt) || UIManager.GetAsyncKeyStateB(Key.RightAlt)))
                  {
                     //ZOOM
                     int sign = UIManager.WheelDelta > 0 ? -1 : 1;
                     float heightDiff = (float)Math.Log(mEye.Y);
                     float lSq = mEye.Y * 0.3f;

                     Zoom = Math.Abs(lSq * (float)(UIManager.WheelDelta / 120f));

                     kDir.Scale(Zoom * sign);

                     mEye += kDir;
                  }


                  //allow WASD || Arrow keys to translate us...
                  if (UIManager.GetAsyncKeyStateB(Key.W) || UIManager.GetAsyncKeyStateB(Key.UpArrow) || UIManager.GetAsyncKeyStateB(Key.NumPad8))
                  {
                     if (UIManager.GetAsyncKeyStateB(Key.LeftAlt) || UIManager.GetAsyncKeyStateB(Key.RightAlt))
                     {
                        //ZOOM
                        int sign = UIManager.WheelDelta > 0 ? -1 : 1;
                        Zoom = 2 * (float)(UIManager.WheelDelta / 120f);

                        kDir.Scale(Zoom * sign);

                        mEye -= kDir;
                     }
                     else
                     {
                        CheckAssignMovement(out inc, mLookAt, ndir * incAmt);
                        mEye += inc;
                        mLookAt += inc;
                     }

                  }
                  if (UIManager.GetAsyncKeyStateB(Key.S) || UIManager.GetAsyncKeyStateB(Key.DownArrow) || UIManager.GetAsyncKeyStateB(Key.NumPad2))
                  {
                     if (UIManager.GetAsyncKeyStateB(Key.LeftAlt) || UIManager.GetAsyncKeyStateB(Key.RightAlt))
                     {
                        //ZOOM
                        int sign = UIManager.WheelDelta > 0 ? -1 : 1;
                        Zoom = 2 * (float)(UIManager.WheelDelta / 120f);

                        kDir.Scale(Zoom * sign);

                        mEye += kDir;
                     }
                     else
                     {
                        CheckAssignMovement(out inc, mLookAt, -ndir * incAmt);
                        mEye += inc;
                        mLookAt += inc;
                     }

                  }
                  if (UIManager.GetAsyncKeyStateB(Key.A) || UIManager.GetAsyncKeyStateB(Key.LeftArrow) || UIManager.GetAsyncKeyStateB(Key.NumPad4))
                  {
                     CheckAssignMovement(out inc, mLookAt, -mRight * incAmt);
                     mEye += inc;
                     mLookAt += inc;
                  }
                  if (UIManager.GetAsyncKeyStateB(Key.D) || UIManager.GetAsyncKeyStateB(Key.RightArrow) || UIManager.GetAsyncKeyStateB(Key.NumPad6))
                  {
                     CheckAssignMovement(out inc, mLookAt, mRight * incAmt);
                     mEye += inc;
                     mLookAt += inc;
                  }
               }
               else   //artist camera controls
               {
                  Dir.Scale(Zoom);
                  Zoom = 1;
                  if ((UIManager.GetAsyncKeyStateB(Key.LeftShift)) || (UIManager.GetAsyncKeyStateB(Key.RightShift)) || (CoreGlobals.getSettingsFile().ArtistModePan == false && UIManager.GetMouseButtonDown(UIManager.eMouseButton.cMiddle)))
                  {
                     if (UIManager.GetAsyncKeyStateB(Key.LeftControl) || UIManager.GetAsyncKeyStateB(Key.RightControl))
                     {
                        if (mbInvertZoom == true)
                        {
                           mouseDeltaY = -1 * mouseDeltaY;
                        }

                        Zoom = 1 + mouseDeltaY / 100f;
                        mEye = mLookAt + Dir;
                     }
                     else if (!UIManager.GetMouseButtonDown(UIManager.eMouseButton.cLeft))
                     {
                        //Rotation!

                        if (mouseDeltaX != 0 || mouseDeltaY != 0)
                        {
                           matRotation = Matrix.RotationAxis(mRight, Geometry.DegreeToRadian((float)-mouseDeltaY / 5.0f));
                           Dir.TransformCoordinate(matRotation);
                           Vector3 t = Vector3.Normalize(Dir);
                           if (Vector3.Dot(t, mUp) < mCamDotLimit && Vector3.Dot(t, mUp) > -mCamDotLimit)
                              mEye = mLookAt + Dir;

                           matRotation = Matrix.RotationY(Geometry.DegreeToRadian((float)-mouseDeltaX / 5.0f));
                           Dir.TransformCoordinate(matRotation);
                           t = Vector3.Normalize(Dir);
                           if (Vector3.Dot(t, mUp) < mCamDotLimit && Vector3.Dot(t, mUp) > -mCamDotLimit)
                              mEye = mLookAt + Dir;
                        }
                     }

                     // Do nothing if control key is pressed
                     if (UIManager.GetAsyncKeyStateB(Key.LeftControl) || UIManager.GetAsyncKeyStateB(Key.RightControl))
                     {
                        return;
                     }
                  }

                  else if (UIManager.GetMouseButtonDown(UIManager.eMouseButton.cMiddle))
                  {

                     if (mTranslationPressed)
                     {
                        float yE = mEye.Y;
                        float yL = mLookAt.Y;
                        Vector3 r0 = TerrainGlobals.getEditor().getRayPosFromMouseCoords(false);
                        Vector3 rD = TerrainGlobals.getEditor().getRayPosFromMouseCoords(true) - r0;
                        rD.Normalize();
                        Vector3 iPt = new Vector3();
                        BTerrainQuadNode node = null;
                        TerrainGlobals.getTerrain().rayIntersects(ref r0, ref rD, ref iPt, ref node, true);
                        Vector3 diff = mTranlationInterception - iPt;
                        Vector3 eyeDiff = mLookAt - mEye;

                        mLookAt += diff;
                        mEye += diff;

                        mLookAt.Y = yL;
                        mEye.Y = yE;

                     }
                     else
                     {
                        mTranslationPressed = true;
                        Vector3 r0 = TerrainGlobals.getEditor().getRayPosFromMouseCoords(false);
                        Vector3 rD = TerrainGlobals.getEditor().getRayPosFromMouseCoords(true) - r0;
                        rD.Normalize();
                        BTerrainQuadNode node = null;
                        TerrainGlobals.getTerrain().rayIntersects(ref r0, ref rD, ref mTranlationInterception, ref node, true);


                     }
                  }
                  else
                  {
                      mTranslationPressed = false;
                     if (UIManager.GetAsyncKeyStateB(Key.W) || UIManager.GetAsyncKeyStateB(Key.UpArrow) || UIManager.GetAsyncKeyStateB(Key.NumPad8))
                     {
                        CheckAssignMovement(out inc, mLookAt, ndir * incAmt);
                        mEye += inc;
                        mLookAt += inc;
                     }
                     if (UIManager.GetAsyncKeyStateB(Key.S) || UIManager.GetAsyncKeyStateB(Key.DownArrow) || UIManager.GetAsyncKeyStateB(Key.NumPad2))
                     {
                        CheckAssignMovement(out inc, mLookAt, -ndir * incAmt);
                        mEye += inc;
                        mLookAt += inc;
                     }
                     if (UIManager.GetAsyncKeyStateB(Key.A) || UIManager.GetAsyncKeyStateB(Key.LeftArrow) || UIManager.GetAsyncKeyStateB(Key.NumPad4))
                     {
                        CheckAssignMovement(out inc, mLookAt, -mRight * incAmt);
                        mEye += inc;
                        mLookAt += inc;
                     }
                     if (UIManager.GetAsyncKeyStateB(Key.D) || UIManager.GetAsyncKeyStateB(Key.RightArrow) || UIManager.GetAsyncKeyStateB(Key.NumPad6))
                     {
                        CheckAssignMovement(out inc, mLookAt, mRight * incAmt);
                        mEye += inc;
                        mLookAt += inc;
                     }
                  }


               }

               if (UIManager.GetAsyncKeyStateB(Key.Space) && (UIManager.GetAsyncKeyStateB(Key.LeftShift) || UIManager.GetAsyncKeyStateB(Key.RightShift)))
               {
                  SimGlobals.getSimMain().LookAtSelectedObject();
               }
               else if(UIManager.GetAsyncKeyStateB(Key.Space))
               {
                  snapCamera(eCamSnaps.cCamSnap_RTS);
               }


               break;


            case eCamModes.cCamMode_ModelView:
               {
                  if (UIManager.GetMouseButtonDown(UIManager.eMouseButton.cMiddle) && (UIManager.GetAsyncKeyStateB(Key.LeftAlt) || UIManager.GetAsyncKeyStateB(Key.RightAlt)))
                  {
                     // Orbit camera
                     if (mouseDeltaX != 0 || mouseDeltaY != 0)
                     {
                        matRotation = Matrix.RotationAxis(mRight, Geometry.DegreeToRadian((float)-mouseDeltaY / 5.0f));
                        Dir.TransformCoordinate(matRotation);
                        Vector3 t = Vector3.Normalize(Dir);
                        if (Vector3.Dot(t, mUp) < mCamDotLimit && Vector3.Dot(t, mUp) > -mCamDotLimit)
                           mEye = mLookAt + Dir;

                        matRotation = Matrix.RotationY(Geometry.DegreeToRadian((float)-mouseDeltaX / 5.0f));
                        Dir.TransformCoordinate(matRotation);
                        t = Vector3.Normalize(Dir);
                        if (Vector3.Dot(t, mUp) < mCamDotLimit && Vector3.Dot(t, mUp) > -mCamDotLimit)
                           mEye = mLookAt + Dir;
                     }
                  }
                  else if (UIManager.GetMouseButtonDown(UIManager.eMouseButton.cMiddle))
                  {
                     // Translate (pan)
                     float distFromModel = Vector3.Length(mEye - mModelBoundingBox.getCenter());
                     float translationFactor = distFromModel / 1400.0f;

                     float upInc = (float)mouseDeltaY * translationFactor;
                     float rightInc = (float)mouseDeltaX * translationFactor;

                     Vector3 up = Vector3.Cross(Dir, mRight);
                     up = BMathLib.Normalize(up);

                     Vector3 tranlation = up * upInc + mRight * rightInc;


                     mEye += tranlation;
                     mLookAt += tranlation;
                  }

                  if (UIManager.WheelDelta != 0)
                  {
                     // Zoom
                     float distFromModel = Vector3.Length(mEye - mModelBoundingBox.getCenter());
                     float translationFactor = -distFromModel / 10.0f;

                     mEye += kDir * (translationFactor * (UIManager.WheelDelta / 120f));
                  }

                  if (UIManager.GetAsyncKeyStateB(Key.Space))
                  {
                     snapCamera(eCamSnaps.cCamSnap_ModelView);
                  }
               }
               break;
         }
      }
   }

   #endregion


}
