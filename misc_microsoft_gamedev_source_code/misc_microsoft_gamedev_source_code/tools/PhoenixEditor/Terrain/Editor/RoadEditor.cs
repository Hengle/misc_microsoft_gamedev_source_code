using System;
using System.IO;
using System.Drawing;
using System.Drawing.Imaging;
using System.Collections.Generic;
using System.Text;
using Microsoft.DirectX;
using Microsoft.DirectX.Direct3D;
using Microsoft.DirectX.DirectInput;
using System.Windows.Forms;
using System.Threading;
using System;
using System.Diagnostics;

using Rendering;
using EditorCore;
using SimEditor;

////


//------------------------------------------
namespace Terrain
{
   public class roadControlPointUVs
   {
      //minX, minZ, maxX, maxZ
      public static Vector4 cMainRoadUVs = new Vector4(0.058f, 0, 0.254f, 1.0f);                  //main drag

      public static Vector4 c4IntersectUVsCenter      = new Vector4(0.39f,  0.058f, 0.586f, 0.254f);
      public static Vector4 c4IntersectUVsLeftLip     = new Vector4(0.342f, 0.058f, 0.39f, 0.254f);
      public static Vector4 c4IntersectUVsRightLip    = new Vector4(0.586f, 0.058f, 0.635f, 0.254f);
      public static Vector4 c4IntersectUVsTopLip      = new Vector4(0.39f, 0.009f, 0.586f, 0.058f);
      public static Vector4 c4IntersectUVsBottomLip   = new Vector4(0.39f, 0.254f, 0.586f, 0.303f);

      #region 3 way T intsersectionUVs
      public static Vector4 c3TIntersectUVsCenter = new Vector4(0.39f, 0.41f, 0.586f, 0.605f);
      public static Vector4 c3TIntersectUVsLeftLip = new Vector4(0.341f, 0.41f, 0.39f, 0.605f);
      public static Vector4 c3TIntersectUVsRightLip = new Vector4(0.586f, 0.41f, 0.635f, 0.605f);
      public static Vector4 c3TIntersectUVsTopLip = new Vector4(0.390f, 0.361f, 0.586f, 0.410f);
      #endregion

      #region 3 way Y IntersectionUVs
      public static Vector2 c3YIntersectUVsCenterA = new Vector2(0.390f, 0.715f);//topLeft
      public static Vector2 c3YIntersectUVsCenterB = new Vector2(0.586f, 0.715f);//topRight
      public static Vector2 c3YIntersectUVsCenterC = new Vector2(0.488f, 0.891f);//bottomCenter
      public static Vector4 c3YIntersectUVsTopLip = new Vector4(0.390f, 0.664f, 0.586f, 0.715f);

      public static Vector2 c3YIntersectUVsLeftLeft = new Vector2(0.318f, 0.785f);
      public static Vector2 c3YIntersectUVsLeftRight = new Vector2(0.488f, 0.891f);
      public static Vector2 c3YIntersectUVsLeftTop = new Vector2(0.351f, 0.752f);
      public static Vector2 c3YIntersectUVsLeftBottom = new Vector2(0.454f, 0.924f);

      public static Vector2 c3YIntersectUVsRightLeft     = new Vector2(0.488f, 0.891f);
      public static Vector2 c3YIntersectUVsRightRight = new Vector2(0.660f, 0.786f);
      public static Vector2 c3YIntersectUVsRightTop    = new Vector2(0.627f, 0.754f);
      public static Vector2 c3YIntersectUVsRightBottom   = new Vector2(0.522f, 0.924f);
      #endregion


   }
   //---------------------------------------
   //---------------------------------------
   //---------------------------------------
   //---------------------------------------

   public class roadControlPoint
   {
      public enum eControlPointType
      {
         cAngled =0,
         cTightCurve =1,
         cSplinePoint =2,
      }
      public enum eControlPointIntersectType
      {
         cNone = 0,
         c2Way = 1,
         c3WayT = 2,
         c3WayY = 3,
         c4Way = 4,
      }
      static public int[] cTypeColors = new int[] { unchecked((int)0x33FFFF00), unchecked((int)0x33FF00FF), unchecked((int)0x3300FFFF) };
      public int mGridX = 0;
      public int mGridY = 0;
      public eControlPointType mControlPointType = eControlPointType.cAngled;
      public eControlPointIntersectType mControlPointIntersectType = eControlPointIntersectType.cNone;
      public BRenderDebugSphere mVisSphere = null;
      public bool mIsSelected = false;
      List<roadSegment> mRoadSegmentsUsingMe = new List<roadSegment>();
      VertexBuffer mVisVB = null;
      
      float mLipOffset = 0.25f;

      bool mSnapToHeight = false;
      public int mNumVerts = 0;
      public List<VertexTypes.Pos_uv0> mVerts = new List<VertexTypes.Pos_uv0>();

      Vector2 minBounds = new Vector2(float.MaxValue,float.MaxValue);
      Vector2 maxBounds = new Vector2(float.MinValue,float.MinValue);

      //angle constants
      public static Vector2 cYIntersect90DegTol = new Vector2(70, 110);
      public static Vector2 cYIntersect180DegTol = new Vector2(160, 190);
      public static Vector2 cYIntersect45DegTol = new Vector2(25, 65);
      public static Vector2 cYIntersect135DegTol = new Vector2(115, 155);
      public static Vector2 cYIntersect225DegTol = new Vector2(205, 245);


      //-------------------------------------
      public roadControlPoint()
      {
         mGridX = 0;
         mGridY = 0;
         mControlPointType = eControlPointType.cAngled;
      }
      public roadControlPoint(int x, int y, eControlPointType cpt)
      {
         mGridX = x;
         mGridY = y;
         mControlPointType = cpt;
      }
      ~roadControlPoint()
      {
         destroy();
      }
      //-------------------------------------
      public void destroy()
      {
         mRoadSegmentsUsingMe.Clear();
         if(mVisSphere!=null)
         {
            mVisSphere.destroy();
            mVisSphere = null;
         }
         

         if (mVisVB != null)
         {
            mVisVB.Dispose();
            mVisVB = null;
         }
      }
      public void render()
      {
         if (mVisVB!=null)
         {
            //BRenderDevice.getDevice().SetRenderState(RenderStates.FillMode, (int)FillMode.WireFrame);
            //CLM do this the stupid way for now, just render this as a seperate draw call...
            //draw line
            BRenderDevice.getDevice().VertexDeclaration = VertexTypes.Pos_uv0.vertDecl;
            BRenderDevice.getDevice().SetStreamSource(0, mVisVB, 0);
            int numPrimitives = mNumVerts / 3;
            BRenderDevice.getDevice().DrawPrimitives(PrimitiveType.TriangleList, 0, numPrimitives);
            //BRenderDevice.getDevice().SetRenderState(RenderStates.FillMode, (int)FillMode.Solid);
         }

      }
      public void renderDebugPrims()
      {
         if (mVisSphere != null)
         {
            Vector3 pointCenter = TerrainGlobals.getTerrain().getPostDeformPos(mGridX, mGridY);
            pointCenter.Y = TerrainGlobals.getTerrain().getHeightAtXZ(mGridX, mGridY); 
            BRenderDevice.getDevice().Transform.World = Matrix.Translation(pointCenter);
            mVisSphere.render(true, !mIsSelected);
            BRenderDevice.getDevice().Transform.World = Matrix.Identity;
         }
      }
      //-------------------------------------
      public bool rayCastIntersect(Vector3 rayOrig, Vector3 rayDir, float roadWidth)
      {
         Vector3 pointCenter = TerrainGlobals.getTerrain().getPostDeformPos(mGridX, mGridY);
         float tVal = 0;
         return BMathLib.raySphereIntersect(pointCenter, 1, rayOrig, rayDir, ref tVal);
      }
      public bool rayCastDrag(Vector3 []points,float roadWidth)
      {
         Vector3 pointCenter = TerrainGlobals.getTerrain().getPostDeformPos(mGridX, mGridY);
         return (BMathLib.eVolumeIntersectionType.cVIT_Empty != BMathLib.boxSphereIntersect(pointCenter,1.5f,
                     points[0], points[1], points[2], points[3], points[4], points[5], points[6], points[7]));

      }
      public bool pointSphereIntersect(Vector3 worldPos, float roadWidth)
      {
         Vector3 pointCenter = TerrainGlobals.getTerrain().getPostDeformPos(mGridX, mGridY);
         return BMathLib.pointSphereIntersect(ref pointCenter, roadWidth,ref pointCenter);
      }
      public bool pointIntersect2D(int gridX, int gridZ, float roadWidth)
      {
         float xDif = gridX * TerrainGlobals.getTerrain().getTileScale() - mGridX * TerrainGlobals.getTerrain().getTileScale();
         float yDif = gridZ * TerrainGlobals.getTerrain().getTileScale() - mGridY * TerrainGlobals.getTerrain().getTileScale();

         float dist = (float)Math.Sqrt(xDif * xDif + yDif * yDif);
         return dist < roadWidth*1.5f;
      }
      //-------------------------------------
      public void updateVisual(float radius, bool updateDebugVis)
      {
         updateVisual(radius, updateDebugVis,false);
      }
      public void updateVisual(float radius, bool updateDebugVis, bool snapToHeight)
      {
         mSnapToHeight = snapToHeight;
         if(updateDebugVis)
         {
            int col = cTypeColors[(int)mControlPointType];
            //if (numRoadSegmentsUsingMe() > 2)
            //   col = cTypeColors[0];
            mVisSphere = new BRenderDebugSphere(0.5f, 2, col);
         }
         

         if(numRoadSegmentsUsingMe()<3)
         {
            if(mVisVB!=null)
            {
               mVisVB.Dispose();
               mVisVB = null;
            }
         }
         else
         {
            updateMultiSegmentVisual();
         }
      }
      void addVert(List<VertexTypes.Pos_uv0> verts, float x, float z, float u, float v)
      {
         float h = mSnapToHeight? TerrainGlobals.getTerrain().getHeightAtXZ(x, z) + 0.1f : 0;
         verts.Add(new VertexTypes.Pos_uv0(x, h, z, u,v));

         if (x <minBounds.X) minBounds.X = x;
         if (z <minBounds.Y) minBounds.Y = z;
         if (x > maxBounds.X) maxBounds.X = x;
         if (z > maxBounds.Y) maxBounds.Y = z;
      }
      void updateMultiSegmentIntersect4(List<VertexTypes.Pos_uv0> verts)
      {
         Vector3 worldPt = TerrainGlobals.getTerrain().getPostDeformPos(mGridX, mGridY);
         float width = mRoadSegmentsUsingMe[0].mOwnerRoad.getRoadWidth();

         //align myself to my root segment.
         Vector3 ray = Vector3.Empty;
         Vector3 tang = Vector3.Empty;
         float length = 0;
         mRoadSegmentsUsingMe[0].giveRayLengthTan(ref ray, ref length, ref tang);

         if (mRoadSegmentsUsingMe[0].mPointB == this)
            ray = -ray;

         ray *= width;
         tang *= width;

         Vector3 center = Vector3.Empty;
         Vector3 inner = Vector3.Empty;
         Vector3 outer = Vector3.Empty;
         mRoadSegmentsUsingMe[0].calcTangentPoints(this, out center, out inner, out outer);

         Vector3 topLeft = inner - ray;
         Vector3 bottomLeft = inner + ray;
         Vector3 topRight = outer - ray;
         Vector3 bottomRight = outer + ray;

         Vector3 lipOffset = ray * mLipOffset;

         //top 'lip'
         addVert(verts, topRight.X - lipOffset.X,  topRight.Z - lipOffset.Z,           roadControlPointUVs.c4IntersectUVsTopLip.Z, roadControlPointUVs.c4IntersectUVsTopLip.Y);
         addVert(verts, topLeft.X - lipOffset.X,   topLeft.Z - lipOffset.Z,            roadControlPointUVs.c4IntersectUVsTopLip.X, roadControlPointUVs.c4IntersectUVsTopLip.Y);
         addVert(verts, topLeft.X,                 topLeft.Z,                          roadControlPointUVs.c4IntersectUVsTopLip.X, roadControlPointUVs.c4IntersectUVsTopLip.W);

         addVert(verts, topRight.X - lipOffset.X,  topRight.Z - lipOffset.Z,           roadControlPointUVs.c4IntersectUVsTopLip.Z, roadControlPointUVs.c4IntersectUVsTopLip.Y);
         addVert(verts, topLeft.X,                 topLeft.Z,                          roadControlPointUVs.c4IntersectUVsTopLip.X, roadControlPointUVs.c4IntersectUVsTopLip.W);
         addVert(verts, topRight.X,                topRight.Z,                         roadControlPointUVs.c4IntersectUVsTopLip.Z, roadControlPointUVs.c4IntersectUVsTopLip.W);

         //bottom 'lip'
         addVert(verts, bottomRight.X + lipOffset.X,  bottomRight.Z + lipOffset.Z,     roadControlPointUVs.c4IntersectUVsBottomLip.Z, roadControlPointUVs.c4IntersectUVsBottomLip.W);
         addVert(verts, bottomLeft.X + lipOffset.X,   bottomLeft.Z + lipOffset.Z,      roadControlPointUVs.c4IntersectUVsBottomLip.X, roadControlPointUVs.c4IntersectUVsBottomLip.W);
         addVert(verts, bottomLeft.X,                 bottomLeft.Z,                    roadControlPointUVs.c4IntersectUVsBottomLip.X, roadControlPointUVs.c4IntersectUVsBottomLip.Y);

         addVert(verts, bottomRight.X + lipOffset.X,  bottomRight.Z + lipOffset.Z,     roadControlPointUVs.c4IntersectUVsBottomLip.Z, roadControlPointUVs.c4IntersectUVsBottomLip.W);
         addVert(verts, bottomLeft.X,                 bottomLeft.Z,                    roadControlPointUVs.c4IntersectUVsBottomLip.X, roadControlPointUVs.c4IntersectUVsBottomLip.Y);
         addVert(verts, bottomRight.X,                bottomRight.Z,                   roadControlPointUVs.c4IntersectUVsBottomLip.Z, roadControlPointUVs.c4IntersectUVsBottomLip.Y);

         lipOffset = tang * mLipOffset;

         //left 'lip'
         addVert(verts, topLeft.X,                    topLeft.Z,                       roadControlPointUVs.c4IntersectUVsLeftLip.Z, roadControlPointUVs.c4IntersectUVsLeftLip.Y);
         addVert(verts, topLeft.X - lipOffset.X,      topLeft.Z - lipOffset.Z,         roadControlPointUVs.c4IntersectUVsLeftLip.X, roadControlPointUVs.c4IntersectUVsLeftLip.Y);
         addVert(verts, bottomLeft.X,                 bottomLeft.Z,                    roadControlPointUVs.c4IntersectUVsLeftLip.Z, roadControlPointUVs.c4IntersectUVsLeftLip.W);

         addVert(verts, topLeft.X - lipOffset.X,      topLeft.Z - lipOffset.Z,         roadControlPointUVs.c4IntersectUVsLeftLip.X, roadControlPointUVs.c4IntersectUVsLeftLip.Y);
         addVert(verts, bottomLeft.X - lipOffset.X,   bottomLeft.Z - lipOffset.Z,      roadControlPointUVs.c4IntersectUVsLeftLip.X, roadControlPointUVs.c4IntersectUVsLeftLip.W);
         addVert(verts, bottomLeft.X,                 bottomLeft.Z,                    roadControlPointUVs.c4IntersectUVsLeftLip.Z, roadControlPointUVs.c4IntersectUVsLeftLip.W);

         //right 'lip'
         addVert(verts, topRight.X,                   topRight.Z,                      roadControlPointUVs.c4IntersectUVsRightLip.X, roadControlPointUVs.c4IntersectUVsRightLip.Y);
         addVert(verts, topRight.X + lipOffset.X,     topRight.Z + lipOffset.Z,        roadControlPointUVs.c4IntersectUVsRightLip.Z, roadControlPointUVs.c4IntersectUVsRightLip.Y);
         addVert(verts, bottomRight.X,                bottomRight.Z,                   roadControlPointUVs.c4IntersectUVsRightLip.X, roadControlPointUVs.c4IntersectUVsRightLip.W);

         addVert(verts, topRight.X + lipOffset.X,     topRight.Z + lipOffset.Z,        roadControlPointUVs.c4IntersectUVsRightLip.Z, roadControlPointUVs.c4IntersectUVsRightLip.Y);
         addVert(verts, bottomRight.X + lipOffset.X,  bottomRight.Z + lipOffset.Z,     roadControlPointUVs.c4IntersectUVsRightLip.Z, roadControlPointUVs.c4IntersectUVsRightLip.W);
         addVert(verts, bottomRight.X,                bottomRight.Z,                   roadControlPointUVs.c4IntersectUVsRightLip.X, roadControlPointUVs.c4IntersectUVsRightLip.W);


         //center square
         addVert(verts, topRight.X,  topRight.Z, roadControlPointUVs.c4IntersectUVsCenter.Z, roadControlPointUVs.c4IntersectUVsCenter.Y);
         addVert(verts, topLeft.X,  topLeft.Z, roadControlPointUVs.c4IntersectUVsCenter.X, roadControlPointUVs.c4IntersectUVsCenter.Y);
         addVert(verts, bottomLeft.X,  bottomLeft.Z, roadControlPointUVs.c4IntersectUVsCenter.X, roadControlPointUVs.c4IntersectUVsCenter.W);

         addVert(verts, topRight.X,  topRight.Z, roadControlPointUVs.c4IntersectUVsCenter.Z, roadControlPointUVs.c4IntersectUVsCenter.Y);
         addVert(verts, bottomLeft.X,  bottomLeft.Z, roadControlPointUVs.c4IntersectUVsCenter.X, roadControlPointUVs.c4IntersectUVsCenter.W);
         addVert(verts, bottomRight.X,  bottomRight.Z, roadControlPointUVs.c4IntersectUVsCenter.Z, roadControlPointUVs.c4IntersectUVsCenter.W);

      }
      void updateMultiSegmentIntersect3(List<VertexTypes.Pos_uv0> verts)
      {
         Vector3 worldPt = TerrainGlobals.getTerrain().getPostDeformPos(mGridX, mGridY);
         float width = mRoadSegmentsUsingMe[0].mOwnerRoad.getRoadWidth();

         //align myself to my root segment.
         Vector3 ray = Vector3.Empty;
         Vector3 tang = Vector3.Empty;
         float length = 0;
         mRoadSegmentsUsingMe[0].giveRayLengthTan(ref ray, ref length, ref tang);


    
         ////get the angle between myself, and my two neighbors
         if (mRoadSegmentsUsingMe[0].mPointB == this)
            ray = -ray;
         Vector3 ray0 = ray;

         Vector3 ray1 = mRoadSegmentsUsingMe[1].giveRay();
         if (mRoadSegmentsUsingMe[1].mPointB == this)
            ray1 = -ray1;

         Vector3 ray2 = mRoadSegmentsUsingMe[2].giveRay();
         if (mRoadSegmentsUsingMe[2].mPointB == this)
            ray2 = -ray2;

         float ray01Angle = Geometry.RadianToDegree(BMathLib.vec3vec3Angle_0to180(ray0, ray1));
         float ray02Angle = Geometry.RadianToDegree(BMathLib.vec3vec3Angle_0to180(ray0, ray2));


         //start building geometry
         

         Vector3 center = Vector3.Empty;
         Vector3 inner = Vector3.Empty;
         Vector3 outer = Vector3.Empty;
         mRoadSegmentsUsingMe[0].calcTangentPoints(this, out center, out inner, out outer);




         #region Y-INTERSECTIONS
         if (ray01Angle < cYIntersect135DegTol.Y && ray01Angle > cYIntersect135DegTol.X && ray02Angle < cYIntersect135DegTol.Y && ray02Angle > cYIntersect135DegTol.X)//Leaf of T intersection
         {
            //create our angled rays that mirror us at 120 degrees
            Vector3 rightRay = Vector3.Normalize(ray - tang);
            Vector3 leftRay = Vector3.Normalize(ray + tang);

            ray *= width;

            Vector3 lipOffset = ray * mLipOffset;
            Vector3 topLeft = inner - ray;
            Vector3 topRight = outer - ray;
            Vector3 bottomLeft = inner + ray;
            Vector3 bottomRight = outer + ray;
            Vector3 topCenter = center - ray;


            //bottom 'lip'
            addVert(verts, bottomRight.X + lipOffset.X, bottomRight.Z + lipOffset.Z,   roadControlPointUVs.c3YIntersectUVsTopLip.Z, roadControlPointUVs.c3YIntersectUVsTopLip.Y);
            addVert(verts, bottomLeft.X + lipOffset.X, bottomLeft.Z + lipOffset.Z,     roadControlPointUVs.c3YIntersectUVsTopLip.X, roadControlPointUVs.c3YIntersectUVsTopLip.Y);
            addVert(verts, bottomLeft.X, bottomLeft.Z,                                 roadControlPointUVs.c3YIntersectUVsTopLip.X, roadControlPointUVs.c3YIntersectUVsTopLip.W);

            addVert(verts, bottomRight.X + lipOffset.X, bottomRight.Z + lipOffset.Z,   roadControlPointUVs.c3YIntersectUVsTopLip.Z, roadControlPointUVs.c3YIntersectUVsTopLip.Y);
            addVert(verts, bottomLeft.X, bottomLeft.Z,                                 roadControlPointUVs.c3YIntersectUVsTopLip.X, roadControlPointUVs.c3YIntersectUVsTopLip.W);
            addVert(verts, bottomRight.X, bottomRight.Z,                               roadControlPointUVs.c3YIntersectUVsTopLip.Z, roadControlPointUVs.c3YIntersectUVsTopLip.W);

            //right 'lip'
            Vector3 centerLine = leftRay * width*2;// new Vector3(-leftRay.Z * width, 0, leftRay.X * width); // BMathLib.Normalize(topCenter - bottomRight);
            tang =  new Vector3(-centerLine.Z, centerLine.Y, centerLine.X);
            tang = -Vector3.Normalize(tang) * width * mLipOffset;
            if (mRoadSegmentsUsingMe[0].mPointA == this)
               tang = -tang;

            Vector3 topRightPt = centerLine + topCenter;
            addVert(verts, topRightPt.X, topRightPt.Z,                     roadControlPointUVs.c3YIntersectUVsRightTop.X, roadControlPointUVs.c3YIntersectUVsRightTop.Y);
            addVert(verts, topRightPt.X + tang.X, topRightPt.Z + tang.Z,   roadControlPointUVs.c3YIntersectUVsRightRight.X, roadControlPointUVs.c3YIntersectUVsRightRight.Y);
            addVert(verts, topCenter.X, topCenter.Z,                       roadControlPointUVs.c3YIntersectUVsRightLeft.X, roadControlPointUVs.c3YIntersectUVsRightLeft.Y);

            addVert(verts, topCenter.X, topCenter.Z,                       roadControlPointUVs.c3YIntersectUVsRightLeft.X, roadControlPointUVs.c3YIntersectUVsRightLeft.Y);
            addVert(verts, topRightPt.X + tang.X, topRightPt.Z + tang.Z,   roadControlPointUVs.c3YIntersectUVsRightRight.X, roadControlPointUVs.c3YIntersectUVsRightRight.Y);
            addVert(verts, topCenter.X + tang.X, topCenter.Z + tang.Z,     roadControlPointUVs.c3YIntersectUVsRightBottom.X, roadControlPointUVs.c3YIntersectUVsRightBottom.Y);



            //left 'lip'
            centerLine = rightRay * width*2;// BMathLib.Normalize(topCenter - bottomLeft);
            tang = new Vector3(-centerLine.Z, centerLine.Y, centerLine.X);
            tang = -Vector3.Normalize(tang) * width * mLipOffset;
            if (mRoadSegmentsUsingMe[0].mPointA == this)
               tang = -tang;

            Vector3 topLefttPt = centerLine + topCenter;
            addVert(verts, topLefttPt.X, topLefttPt.Z,                     roadControlPointUVs.c3YIntersectUVsLeftTop.X, roadControlPointUVs.c3YIntersectUVsLeftTop.Y);
            addVert(verts, topLefttPt.X - tang.X, topLefttPt.Z - tang.Z,   roadControlPointUVs.c3YIntersectUVsLeftLeft.X, roadControlPointUVs.c3YIntersectUVsLeftLeft.Y);
            addVert(verts, topCenter.X, topCenter.Z,                       roadControlPointUVs.c3YIntersectUVsLeftRight.X, roadControlPointUVs.c3YIntersectUVsLeftRight.Y);

            addVert(verts, topCenter.X, topCenter.Z,                       roadControlPointUVs.c3YIntersectUVsLeftRight.X, roadControlPointUVs.c3YIntersectUVsLeftRight.Y);
            addVert(verts, topLefttPt.X - tang.X, topLefttPt.Z - tang.Z,   roadControlPointUVs.c3YIntersectUVsLeftLeft.X, roadControlPointUVs.c3YIntersectUVsLeftLeft.Y);
            addVert(verts, topCenter.X - tang.X, topCenter.Z - tang.Z,     roadControlPointUVs.c3YIntersectUVsLeftBottom.X, roadControlPointUVs.c3YIntersectUVsLeftBottom.Y);



            //center triangles
            //left supporting triangle
            addVert(verts, topCenter.X, topCenter.Z,     roadControlPointUVs.c3YIntersectUVsCenterC.X, roadControlPointUVs.c3YIntersectUVsCenterC.Y);
            addVert(verts, bottomLeft.X, bottomLeft.Z,   roadControlPointUVs.c3YIntersectUVsCenterA.X, roadControlPointUVs.c3YIntersectUVsCenterA.Y);
            addVert(verts, topLefttPt.X, topLefttPt.Z,   roadControlPointUVs.c3YIntersectUVsLeftTop.X, roadControlPointUVs.c3YIntersectUVsLeftTop.Y);
            //right supporting triangle
            addVert(verts, topCenter.X, topCenter.Z,     roadControlPointUVs.c3YIntersectUVsCenterC.X, roadControlPointUVs.c3YIntersectUVsCenterC.Y);
            addVert(verts, bottomRight.X, bottomRight.Z, roadControlPointUVs.c3YIntersectUVsCenterB.X, roadControlPointUVs.c3YIntersectUVsCenterB.Y);
            addVert(verts, topRightPt.X, topRightPt.Z,   roadControlPointUVs.c3YIntersectUVsRightTop.X, roadControlPointUVs.c3YIntersectUVsRightTop.Y);
            //main center Tri
            addVert(verts, topCenter.X, topCenter.Z,     roadControlPointUVs.c3YIntersectUVsCenterC.X, roadControlPointUVs.c3YIntersectUVsCenterC.Y);
            addVert(verts, bottomLeft.X, bottomLeft.Z,   roadControlPointUVs.c3YIntersectUVsCenterA.X, roadControlPointUVs.c3YIntersectUVsCenterA.Y);
            addVert(verts, bottomRight.X, bottomRight.Z, roadControlPointUVs.c3YIntersectUVsCenterB.X, roadControlPointUVs.c3YIntersectUVsCenterB.Y);

            return;//UGG!
         }
         #endregion

         #region T-INTERSECTIONS
         //Make this more generic so that it's the default value.
         //if(mRoadSegmentsUsingMe[0] == segment)
         //if ( (ray01Angle < cYIntersect90DegTol.Y && ray01Angle > cYIntersect90DegTol.X && ray02Angle < cYIntersect90DegTol.Y && ray02Angle > cYIntersect90DegTol.X) ||  //leaf of T intersection
         //     ((ray01Angle < cYIntersect180DegTol.Y && ray01Angle > cYIntersect180DegTol.X && ray02Angle < cYIntersect90DegTol.Y && ray02Angle > cYIntersect90DegTol.X) || //Stem of T intersection
         //      (ray01Angle < cYIntersect90DegTol.Y && ray01Angle > cYIntersect90DegTol.X && ray02Angle < cYIntersect180DegTol.Y && ray02Angle > cYIntersect180DegTol.X)))
         
         {
            //determine what our 3 way intersect set looks like...
            bool bottomLip = false;
            bool topLip = false;
            bool leftLip = false;
            bool rightLip = false;

            for (int i = 0; i < 3; i++)
            {
               roadSegment segment = getRoadSegmentUsingMe(i);

               Vector3 raySeg = segment.giveRay();
               if (segment.mPointB == this)
                  raySeg = -raySeg;
               float raySegAngle = Geometry.RadianToDegree(BMathLib.vec3vec3Angle_0to180(ray0, raySeg));


               if (mRoadSegmentsUsingMe[0] == segment)
               {
                  bottomLip = true;
               }
               else if (segment.giveClosestCCWSegment(this) == mRoadSegmentsUsingMe[0])
               {
                  if (raySegAngle > cYIntersect90DegTol.Y) //180
                  {
                     topLip = true;
                  }
                  else
                  {
                     rightLip = true;
                  }

               }
               else if (segment.giveClosestCWSegment(this) == mRoadSegmentsUsingMe[0])
               {
                  if (raySegAngle > cYIntersect90DegTol.Y) //180
                  {
                     topLip = true;
                  }
                  else
                  {
                     leftLip = true;
                  }
               }
            }
      

            ray *= width;
            tang *= width;

            Vector3 lipOffset = ray * mLipOffset;

            Vector3 topLeft = inner - ray;
            Vector3 bottomLeft = inner + ray;
            Vector3 topRight = outer - ray;
            Vector3 bottomRight = outer + ray;

            //construct our general mapping
            List<VertexTypes.Pos_uv0> vertMapping = new List<VertexTypes.Pos_uv0>();
            //top 'lip'
            vertMapping.Add(new VertexTypes.Pos_uv0(bottomRight.X + lipOffset.X, 0, bottomRight.Z + lipOffset.Z,  roadControlPointUVs.c3TIntersectUVsTopLip.Z, roadControlPointUVs.c3TIntersectUVsTopLip.Y));
            vertMapping.Add(new VertexTypes.Pos_uv0(bottomLeft.X + lipOffset.X, 0, bottomLeft.Z + lipOffset.Z,    roadControlPointUVs.c3TIntersectUVsTopLip.X, roadControlPointUVs.c3TIntersectUVsTopLip.Y));
            vertMapping.Add(new VertexTypes.Pos_uv0(bottomLeft.X, 0, bottomLeft.Z,                                roadControlPointUVs.c3TIntersectUVsTopLip.X, roadControlPointUVs.c3TIntersectUVsTopLip.W));

            vertMapping.Add(new VertexTypes.Pos_uv0(bottomRight.X + lipOffset.X, 0, bottomRight.Z + lipOffset.Z,  roadControlPointUVs.c3TIntersectUVsTopLip.Z, roadControlPointUVs.c3TIntersectUVsTopLip.Y));
            vertMapping.Add(new VertexTypes.Pos_uv0(bottomLeft.X, 0, bottomLeft.Z,                                roadControlPointUVs.c3TIntersectUVsTopLip.X, roadControlPointUVs.c3TIntersectUVsTopLip.W));
            vertMapping.Add(new VertexTypes.Pos_uv0(bottomRight.X, 0, bottomRight.Z,                              roadControlPointUVs.c3TIntersectUVsTopLip.Z, roadControlPointUVs.c3TIntersectUVsTopLip.W));

            //[03.28.07] CLM NOTE : Y values for UV flipped from here down, didn't pay enough attention to the +/- thing above (topleft bottomright etc) , so, quick hack fix...
            //left 'lip'
            lipOffset = tang * 0.25f;
            vertMapping.Add(new VertexTypes.Pos_uv0(topLeft.X, topLeft.Y, topLeft.Z,                              roadControlPointUVs.c3TIntersectUVsLeftLip.Z, roadControlPointUVs.c3TIntersectUVsLeftLip.W));
            vertMapping.Add(new VertexTypes.Pos_uv0(topLeft.X - lipOffset.X, topLeft.Y, topLeft.Z - lipOffset.Z,  roadControlPointUVs.c3TIntersectUVsLeftLip.X, roadControlPointUVs.c3TIntersectUVsLeftLip.W));
            vertMapping.Add(new VertexTypes.Pos_uv0(bottomLeft.X, bottomLeft.Y, bottomLeft.Z,                     roadControlPointUVs.c3TIntersectUVsLeftLip.Z, roadControlPointUVs.c3TIntersectUVsLeftLip.Y));

            vertMapping.Add(new VertexTypes.Pos_uv0(topLeft.X - lipOffset.X, topLeft.Y, topLeft.Z - lipOffset.Z,           roadControlPointUVs.c3TIntersectUVsLeftLip.X, roadControlPointUVs.c3TIntersectUVsLeftLip.W));
            vertMapping.Add(new VertexTypes.Pos_uv0(bottomLeft.X - lipOffset.X, bottomLeft.Y, bottomLeft.Z - lipOffset.Z,  roadControlPointUVs.c3TIntersectUVsLeftLip.X, roadControlPointUVs.c3TIntersectUVsLeftLip.Y));
            vertMapping.Add(new VertexTypes.Pos_uv0(bottomLeft.X, bottomLeft.Y, bottomLeft.Z,                              roadControlPointUVs.c3TIntersectUVsLeftLip.Z, roadControlPointUVs.c3TIntersectUVsLeftLip.Y));

            //right 'lip'
            vertMapping.Add(new VertexTypes.Pos_uv0(topRight.X, topRight.Y, topRight.Z,                                    roadControlPointUVs.c3TIntersectUVsRightLip.X, roadControlPointUVs.c3TIntersectUVsRightLip.W));
            vertMapping.Add(new VertexTypes.Pos_uv0(topRight.X + lipOffset.X, topRight.Y, topRight.Z + lipOffset.Z,        roadControlPointUVs.c3TIntersectUVsRightLip.Z, roadControlPointUVs.c3TIntersectUVsRightLip.W));
            vertMapping.Add(new VertexTypes.Pos_uv0(bottomRight.X, bottomRight.Y, bottomRight.Z,                           roadControlPointUVs.c3TIntersectUVsRightLip.X, roadControlPointUVs.c3TIntersectUVsRightLip.Y));

            vertMapping.Add(new VertexTypes.Pos_uv0(topRight.X + lipOffset.X, topRight.Y, topRight.Z + lipOffset.Z,           roadControlPointUVs.c3TIntersectUVsRightLip.Z, roadControlPointUVs.c3TIntersectUVsRightLip.W));
            vertMapping.Add(new VertexTypes.Pos_uv0(bottomRight.X + lipOffset.X, bottomRight.Y, bottomRight.Z + lipOffset.Z,  roadControlPointUVs.c3TIntersectUVsRightLip.Z, roadControlPointUVs.c3TIntersectUVsRightLip.Y));
            vertMapping.Add(new VertexTypes.Pos_uv0(bottomRight.X, bottomRight.Y, bottomRight.Z,                              roadControlPointUVs.c3TIntersectUVsRightLip.X, roadControlPointUVs.c3TIntersectUVsRightLip.Y));

            //center square
            vertMapping.Add(new VertexTypes.Pos_uv0(topRight.X, topRight.Y, topRight.Z,                                       roadControlPointUVs.c3TIntersectUVsCenter.Z, roadControlPointUVs.c3TIntersectUVsCenter.W));
            vertMapping.Add(new VertexTypes.Pos_uv0(topLeft.X, topLeft.Y, topLeft.Z,                                          roadControlPointUVs.c3TIntersectUVsCenter.X, roadControlPointUVs.c3TIntersectUVsCenter.W));
            vertMapping.Add(new VertexTypes.Pos_uv0(bottomLeft.X, bottomLeft.Y, bottomLeft.Z,                                 roadControlPointUVs.c3TIntersectUVsCenter.X, roadControlPointUVs.c3TIntersectUVsCenter.Y));

            vertMapping.Add(new VertexTypes.Pos_uv0(topRight.X, topRight.Y, topRight.Z,                                       roadControlPointUVs.c3TIntersectUVsCenter.Z, roadControlPointUVs.c3TIntersectUVsCenter.W));
            vertMapping.Add(new VertexTypes.Pos_uv0(bottomLeft.X, bottomLeft.Y, bottomLeft.Z,                                 roadControlPointUVs.c3TIntersectUVsCenter.X, roadControlPointUVs.c3TIntersectUVsCenter.Y));
            vertMapping.Add(new VertexTypes.Pos_uv0(bottomRight.X, bottomRight.Y, bottomRight.Z,                              roadControlPointUVs.c3TIntersectUVsCenter.Z, roadControlPointUVs.c3TIntersectUVsCenter.Y));


            Matrix rotMat = Matrix.Identity;
            if (bottomLip && leftLip && rightLip)
            {
               rotMat *= Matrix.Identity;   
            }
            else if (topLip && leftLip && rightLip)
            {
               rotMat *= Matrix.RotationY(Geometry.DegreeToRadian(180));
            }
            else if (topLip && bottomLip && rightLip)
            {
               rotMat *= Matrix.RotationY(Geometry.DegreeToRadian(90));
            }
            else if (topLip && bottomLip && leftLip)
            {
               rotMat *= Matrix.RotationY(Geometry.DegreeToRadian(270));
            }
            rotMat *= Matrix.Translation(worldPt);

            for (int k = 0; k < vertMapping.Count;k++ )
            {
               Vector4 ka = new Vector4(vertMapping[k].x - worldPt.X, 0, vertMapping[k].z - worldPt.Z, 1);
               
               ka.Transform(rotMat);
               addVert(verts, ka.X, ka.Z, vertMapping[k].u0, vertMapping[k].v0);
            }
         }

        
         #endregion

       

         #region R-INTERSECTIONS
         #endregion



      }
      void updateMultiSegmentVisual()
      {
         mVerts.Clear();
         if(numRoadSegmentsUsingMe()==4)
            updateMultiSegmentIntersect4(mVerts);
         else if (numRoadSegmentsUsingMe() == 3)
            updateMultiSegmentIntersect3(mVerts);

         mNumVerts = mVerts.Count;
         if (mVisVB != null)
         {
            mVisVB.Dispose();
            mVisVB = null;
         }

         if (mNumVerts == 0)
            return;

         mVisVB = new VertexBuffer(typeof(VertexTypes.Pos_uv0), mNumVerts, BRenderDevice.getDevice(), Usage.None, VertexTypes.Pos_uv0.FVF_Flags, Pool.Managed);

         GraphicsStream gStream = mVisVB.Lock(0, 0, LockFlags.None);
         for (int i = 0; i < mVerts.Count; i++)
            gStream.Write(mVerts[i]);
         mVisVB.Unlock();
      }
      //-------------------------------------
      public void addRoadSegmentUsingMe(roadSegment rs)
      {
         if (!mRoadSegmentsUsingMe.Contains(rs))
            mRoadSegmentsUsingMe.Add(rs);
       
         updateVisual(1,false);
      }
      public void removeRoadSegmentUsingMe(roadSegment rs)
      {
         for (int i = 0; i < mRoadSegmentsUsingMe.Count; i++)
         {
            if (mRoadSegmentsUsingMe[i] == rs)
            {
               mRoadSegmentsUsingMe.RemoveAt(i);
               return;
            }
         }
      }
      public int numRoadSegmentsUsingMe()
      {
         return mRoadSegmentsUsingMe.Count;
      }
      public int segmentUsingMeIndex(roadSegment rs)
      {
         return mRoadSegmentsUsingMe.IndexOf(rs);
      }
      public roadSegment getRoadSegmentUsingMe(int index)
      {
         if (index < 0 || index >= mRoadSegmentsUsingMe.Count)
            return null;

         return mRoadSegmentsUsingMe[index];
      }

      //-------------------------------------
      public void multiSegmentIntersectPoints(roadSegment segment,out Vector3 newCenter , out Vector3 centerPos, out Vector3 centerNeg, out Vector3 veloRay)
      {
         newCenter = Vector3.Empty;
         centerPos = Vector3.Empty;
         centerNeg = Vector3.Empty;

         float width = mRoadSegmentsUsingMe[0].mOwnerRoad.getRoadWidth();
         //align myself to my root segment.
         Vector3 ray = Vector3.Empty;
         Vector3 tang = Vector3.Empty;
         float length = 0;
         mRoadSegmentsUsingMe[0].giveRayLengthTan(ref ray, ref length, ref tang);

         if (mRoadSegmentsUsingMe[0].mPointB == this)
            ray = -ray;

         veloRay = Vector3.Empty;
         

         Vector3 center = giveWorldSpacePos();
         if (numRoadSegmentsUsingMe() == 3)
         {
            Vector3 ray0 = ray;
            ray *= width;
            tang *= width;

            Vector3 ray1 = mRoadSegmentsUsingMe[1].giveRay();
            if (mRoadSegmentsUsingMe[1].mPointB == this)
               ray1 = -ray1;

            Vector3 ray2 = mRoadSegmentsUsingMe[2].giveRay();
            if (mRoadSegmentsUsingMe[2].mPointB == this)
               ray2 = -ray2;

            float ray01Angle = Geometry.RadianToDegree(BMathLib.vec3vec3Angle_0to180(ray0, ray1));
            float ray02Angle = Geometry.RadianToDegree(BMathLib.vec3vec3Angle_0to180(ray0, ray2));

            


            if (ray01Angle < cYIntersect135DegTol.Y && ray01Angle > cYIntersect135DegTol.X && ray02Angle < cYIntersect135DegTol.Y && ray02Angle > cYIntersect135DegTol.X) //Y intersect
            {
               Vector3 rightRay = Vector3.Normalize(ray - tang);
               Vector3 leftRay = Vector3.Normalize(ray + tang);

               Vector3 bottomCenter = center + ray;
               Vector3 bottomRight = bottomCenter + tang;
               Vector3 bottomLeft = bottomCenter - tang;
               Vector3 topCenter = center - ray;
               if(mRoadSegmentsUsingMe[0].mPointA==this)
               {
                  if (mRoadSegmentsUsingMe[0] == segment)
                  {
                     newCenter = center + ray;
                     newCenter += ray * mLipOffset;

                     centerPos = newCenter + tang;
                     centerNeg = newCenter - tang;

                     veloRay = ray;
                  }
                  else if (segment.giveClosestCWSegment(this) == mRoadSegmentsUsingMe[0])
                  {
                     Vector3 centerLine = leftRay * width * 2;// new Vector3(-leftRay.Z * width, 0, leftRay.X * width); // BMathLib.Normalize(topCenter - bottomRight);
                     tang = new Vector3(-centerLine.Z, centerLine.Y, centerLine.X);
                     tang = -Vector3.Normalize(tang) * width * mLipOffset;
                     if (mRoadSegmentsUsingMe[0].mPointA == this)
                        tang = -tang;
                     Vector3 topRightPt = centerLine + topCenter;

                     Vector3 outA = new Vector3(topRightPt.X + tang.X, topRightPt.Y, topRightPt.Z + tang.Z);
                     Vector3 outB = new Vector3(topCenter.X + tang.X, topCenter.Y, topCenter.Z + tang.Z);

                     newCenter = (outB + (centerLine * 0.5f));// +tang;

                     centerPos = outB;
                     centerNeg = outA;

                     veloRay = tang;
                  }
                  else if (segment.giveClosestCCWSegment(this) == mRoadSegmentsUsingMe[0])
                  {
                     Vector3 centerLine = rightRay * width * 2;// new Vector3(-leftRay.Z * width, 0, leftRay.X * width); // BMathLib.Normalize(topCenter - bottomRight);
                     tang = new Vector3(-centerLine.Z, centerLine.Y, centerLine.X);
                     tang = -Vector3.Normalize(tang) * width * mLipOffset;
                     if (mRoadSegmentsUsingMe[0].mPointA == this)
                        tang = -tang;
                     Vector3 topLeftPt = centerLine + topCenter;

                     Vector3 outA = new Vector3(topLeftPt.X - tang.X, topLeftPt.Y, topLeftPt.Z - tang.Z);
                     Vector3 outB = new Vector3(topCenter.X - tang.X, topCenter.Y, topCenter.Z - tang.Z);

                     newCenter = (outB + (centerLine * 0.5f));// +tang;
                      
                     centerPos = outA;
                     centerNeg = outB;
                     veloRay = tang;
                  }
               }
               else
               {
                  if (mRoadSegmentsUsingMe[0] == segment)
                  {
                     newCenter = center + ray;
                     newCenter += ray * mLipOffset;

                     centerPos = newCenter + tang;
                     centerNeg = newCenter - tang;

                     veloRay = -ray;
                  }
                  else if (segment.giveClosestCCWSegment(this) == mRoadSegmentsUsingMe[0])
                  {
                     //right 'lip'
                     Vector3 centerLine = leftRay * width * 2;// new Vector3(-leftRay.Z * width, 0, leftRay.X * width); // BMathLib.Normalize(topCenter - bottomRight);
                     tang = new Vector3(-centerLine.Z, centerLine.Y, centerLine.X);
                     tang = -Vector3.Normalize(tang) * width * mLipOffset;
                     if (mRoadSegmentsUsingMe[0].mPointA == this)
                        tang = -tang;
                     Vector3 topRightPt = centerLine + topCenter;

                     Vector3 outA = new Vector3(topRightPt.X + tang.X, topRightPt.Y, topRightPt.Z + tang.Z);
                     Vector3 outB = new Vector3(topCenter.X + tang.X, topCenter.Y, topCenter.Z + tang.Z);

                     newCenter = (outB + (centerLine *  0.5f));// +tang;

                     centerPos = outB;
                     centerNeg = outA;

                     veloRay = tang;
                  }
                  else if (segment.giveClosestCWSegment(this) == mRoadSegmentsUsingMe[0])
                  {
                     Vector3 centerLine = rightRay * width * 2;// new Vector3(-leftRay.Z * width, 0, leftRay.X * width); // BMathLib.Normalize(topCenter - bottomRight);
                     tang = new Vector3(-centerLine.Z, centerLine.Y, centerLine.X);
                     tang = -Vector3.Normalize(tang) * width * mLipOffset;
                     if (mRoadSegmentsUsingMe[0].mPointA == this)
                        tang = -tang;
                     Vector3 topLeftPt = centerLine + topCenter;

                     Vector3 outA = new Vector3(topLeftPt.X - tang.X, topLeftPt.Y, topLeftPt.Z - tang.Z);
                     Vector3 outB = new Vector3(topCenter.X - tang.X, topCenter.Y, topCenter.Z - tang.Z);

                     newCenter = (outB + (centerLine * 0.5f));// +tang;

                     centerPos = outA;
                     centerNeg = outB;
                     veloRay = tang;
                  }
               }
              
            }
            else //if (  (ray01Angle < 110 && ray01Angle > 70 && ray02Angle < 110 && ray02Angle > 70) || 
            //      (ray01Angle < 190 && ray01Angle > 165 && ray02Angle < 110 && ray02Angle > 70) || //Stem of T intersection
            //      (ray01Angle < 110 && ray01Angle > 70 && ray02Angle < 190 && ray02Angle > 165))
            {
               int segInd = segmentUsingMeIndex(segment);
               Vector3 raySeg = segment.giveRay();
               if (segment.mPointB == this)
                  raySeg = -raySeg;
               float raySegAngle = Geometry.RadianToDegree(BMathLib.vec3vec3Angle_0to180(ray0, raySeg));

               if (mRoadSegmentsUsingMe[0].mPointA == this)
               {
                  if (mRoadSegmentsUsingMe[0] == segment)
                  {
                     newCenter = center + ray;
                     newCenter += ray * mLipOffset;

                     centerPos = newCenter + tang;
                     centerNeg = newCenter - tang;
                     veloRay = -ray;
                  }
                  else if (segment.giveClosestCWSegment(this) == mRoadSegmentsUsingMe[0])
                  {
                     if (raySegAngle > cYIntersect90DegTol.Y) //180
                     {
                        newCenter = center - ray;
                        newCenter -= ray * mLipOffset;

                        centerPos = newCenter - tang;
                        centerNeg = newCenter + tang;

                        veloRay = -ray;
                     }
                     else
                     {
                        newCenter = center + tang;
                        newCenter += tang * mLipOffset;

                        centerPos = newCenter - ray;
                        centerNeg = newCenter + ray;

                        veloRay = -tang;
                     }
                  }
                  else if (segment.giveClosestCCWSegment(this) == mRoadSegmentsUsingMe[0])
                  {
                     if (raySegAngle > cYIntersect90DegTol.Y) //180
                     {
                        newCenter = center - ray;
                        newCenter -= ray * mLipOffset;

                        centerPos = newCenter - tang;
                        centerNeg = newCenter + tang;

                        veloRay = ray;
                     }
                     else
                     {
                        newCenter = center - tang;
                        newCenter -= tang * mLipOffset;

                        centerPos = newCenter + ray;
                        centerNeg = newCenter - ray;

                        veloRay = tang;
                     }
                     
                  }
               }
               else
               {

                  if (mRoadSegmentsUsingMe[0] == segment)
                  {
                     newCenter = center + ray;
                     newCenter += ray * mLipOffset;

                     centerPos = newCenter + tang;
                     centerNeg = newCenter - tang;

                     veloRay = -ray;
                  }
                  else if (segment.giveClosestCCWSegment(this) == mRoadSegmentsUsingMe[0])
                  {
                     if (raySegAngle > cYIntersect90DegTol.Y) //180
                     {
                        newCenter = center - ray;
                        newCenter -= ray * mLipOffset;

                        centerPos = newCenter - tang;
                        centerNeg = newCenter + tang;

                        veloRay = ray;
                     }
                     else
                     {
                        newCenter = center + tang;
                        newCenter += tang * mLipOffset;

                        centerPos = newCenter - ray;
                        centerNeg = newCenter + ray;

                        veloRay = -tang;
                     }
                    
                  }
                  else if (segment.giveClosestCWSegment(this) == mRoadSegmentsUsingMe[0])
                  {
                     if (raySegAngle > cYIntersect90DegTol.Y) //180
                     {
                        newCenter = center - ray;
                        newCenter -= ray * mLipOffset;

                        centerPos = newCenter - tang;
                        centerNeg = newCenter + tang;

                        veloRay = ray;
                     }
                     else
                     {
                        newCenter = center - tang;
                        newCenter -= tang * mLipOffset;
                        centerPos = newCenter + ray;
                        centerNeg = newCenter - ray;

                        veloRay = -tang;
                     }
                  }
               }
            }
         }
         else if(numRoadSegmentsUsingMe()==4)
         {
            ray *= width;
            tang *= width;
            if(mRoadSegmentsUsingMe[0] == segment)
            {
               newCenter = center + ray;
               newCenter += ray * mLipOffset;

               centerPos = newCenter + tang;
               centerNeg = newCenter - tang;

               veloRay = -ray;
            }
            else if(this == mRoadSegmentsUsingMe[0].mPointB)
            {
               if (segment.giveClosestCCWSegment(this) == mRoadSegmentsUsingMe[0])
               {
                  newCenter = center + tang;
                  newCenter += tang * mLipOffset;
                  centerPos = newCenter - ray;
                  centerNeg = newCenter + ray;
                  veloRay = tang;
               }
               else if (segment.giveClosestCWSegment(this) == mRoadSegmentsUsingMe[0])
               {
                  newCenter = center - tang;
                  newCenter -= tang * mLipOffset;
                  centerPos = newCenter + ray;
                  centerNeg = newCenter - ray;
                  veloRay = -tang;
               }
               else
               {
                  newCenter = center - ray;
                  newCenter -= ray * mLipOffset;
                  centerPos = newCenter - tang;
                  centerNeg = newCenter + tang;
                  veloRay = ray;
               }

            }
            else if (this == mRoadSegmentsUsingMe[0].mPointA)
            {
               if (segment.giveClosestCWSegment(this) == mRoadSegmentsUsingMe[0])
               {
                  newCenter = center + tang;
                  newCenter += tang * mLipOffset;
                  centerPos = newCenter - ray;
                  centerNeg = newCenter + ray;
                  veloRay = tang;
               }
               else if (segment.giveClosestCCWSegment(this) == mRoadSegmentsUsingMe[0])
               {
                  newCenter = center - tang;
                  newCenter -= tang * mLipOffset;
                  centerPos = newCenter + ray;
                  centerNeg = newCenter - ray;
                  veloRay = -tang;
               }
               else
               {
                  newCenter = center - ray;
                  newCenter -= ray * mLipOffset;
                  centerPos = newCenter - tang;
                  centerNeg = newCenter + tang;
                  veloRay = ray;
               }

            }
            
                        
         }
      }
      //-------------------------------------
      public Vector3 giveWorldSpacePos()
      {
         return TerrainGlobals.getTerrain().getPostDeformPos(mGridX, mGridY);
      }

      //-------------------------------------
      public List<BTerrainQuadNode> findOwningQN()
      {
         List<BTerrainQuadNode> mQuadNodesTouched = new List<BTerrainQuadNode>();
         float rcpTile = 1 / TerrainGlobals.getTerrain().getTileScale();
         TerrainGlobals.getTerrain().getQuadNodeRoot().getVertBoundsIntersection(mQuadNodesTouched, (int)(Math.Floor(minBounds.X * rcpTile)),
                                                                                                   (int)(Math.Floor(minBounds.Y * rcpTile)),
                                                                                                   (int)(Math.Floor(maxBounds.X * rcpTile)),
                                                                                                   (int)(Math.Floor(maxBounds.Y * rcpTile)));

         return mQuadNodesTouched;
      }
   }
    //---------------------------------------
   //---------------------------------------
   //---------------------------------------
   public class roadSegSpline
   {
      public void createFromRoadSegment(roadSegment rs)
      {
         centerA = rs.mPointA.giveWorldSpacePos();
         centerA = rs.mPointA.giveWorldSpacePos();

         centerA = TerrainGlobals.getTerrain().getPostDeformPos(rs.mPointA.mGridX, rs.mPointA.mGridY);
         velocityA = rs.giveSplineVelocity(rs.mPointA) * rs.giveLength();
         centerB = TerrainGlobals.getTerrain().getPostDeformPos(rs.mPointB.mGridX, rs.mPointB.mGridY);
         velocityB = rs.giveSplineVelocity(rs.mPointB) * rs.giveLength();
      }

      public void copyTo(ref roadSegSpline output)
      {
         output.centerA= centerA;
         output.velocityA = velocityA;
         output.centerB = centerB;
         output.velocityB = velocityB;
      }

      public Vector3 getPos(float time)
      {
         return GetPositionOnCubic(centerA, velocityA, centerB, velocityB, time);
      }
      public Vector3 getTang(float time, float timeDelta)
      {
         return getTangentOnCubic(centerA, velocityA, centerB, velocityB, time, timeDelta);
      }
      public Vector3 getTangPos(float time, float deltaTime, bool posTangOrNegTang)
      {
         Vector3 pos = getPos(time);
         Vector3 tang = getTang(time, deltaTime);
         tang *= posTangOrNegTang ? 1 : -1;
         return pos + tang;
      }

      Vector3 GetPositionOnCubic(Vector3 startPos, Vector3 startVel, Vector3 endPos, Vector3 endVel, float time)
      {
         time = BMathLib.Saturate(time);
         Matrix hermite = BMathLib.matrixFrom16floats(2.0f, -2.0f, 1.0f, 1.0f,
                                                      -3.0f, 3.0f, -2.0f, -1.0f,
                                                       0.0f, 0.0f, 1.0f, 0.0f,
                                                       1.0f, 0.0f, 0.0f, 0.0f
                                                    );


         Matrix m = new Matrix();

         BMathLib.setRow(ref m, 0, startPos);
         BMathLib.setRow(ref m, 1, endPos);
         BMathLib.setRow(ref m, 2, startVel);
         BMathLib.setRow(ref m, 3, endVel);
         m.M44 = 1;

         m = Matrix.Multiply(hermite, m);

         Vector4 timeVector = new Vector4(time * time * time, time * time, time, 1.0f);
         Vector4 result = Vector4.Transform(timeVector, m); // vector * matrix
         return new Vector3(result.X, result.Y, result.Z);
      }
      Vector3 getTangentOnCubic(Vector3 startPos, Vector3 startVel, Vector3 endPos, Vector3 endVel, float time, float timeDelta)
      {
         Vector3 tang = Vector3.Empty;
         Vector3 myPos = GetPositionOnCubic(startPos, startVel, endPos, endVel, time);

         Vector3 nextPos = GetPositionOnCubic(startPos, startVel, endPos, endVel, time + timeDelta);
         if (time == 1)
         {
            nextPos = myPos;
            myPos = GetPositionOnCubic(startPos, startVel, endPos, endVel, time - timeDelta);
         }

         Vector3 dir = BMathLib.Normalize(nextPos - myPos);

         tang = BMathLib.Cross(dir, BMathLib.unitY);

         return tang;
      }



      public Vector3 centerA;
      public Vector3 velocityA;
      public Vector3 centerB;
      public Vector3 velocityB;
   }
   //---------------------------------------
   public class roadSegment
   {
      public Road mOwnerRoad = null;
      public roadControlPoint mPointA = null;
      public roadControlPoint mPointB = null;
      const float cSquareRadToDiamRatio = 1.3f;

      public int mNumVerts = 0;
      public int mStartVertInParentSegment = 0;

      public bool isLine = false;

      

      Vector3 mDirectRay = Vector3.Empty;
      float mLength = 0;
      Vector3 mDirectTangent = Vector3.Empty;
      int mNumIterations = 0;

      bool mSnapToHeight = false;
      //--------------------------
      


      ~roadSegment()
      {
         destroy();
      }
      public void destroy()
      {
         if (mPointA!=null)
            mPointA.removeRoadSegmentUsingMe(this);
         if (mPointB!=null)
            mPointB.removeRoadSegmentUsingMe(this);   
      }
      public void init(Road ownerRoad, roadControlPoint a, roadControlPoint b)
      {
         mOwnerRoad = ownerRoad;

         mPointA = a;
         mPointB = b;

         mPointA.addRoadSegmentUsingMe(this);
         mPointB.addRoadSegmentUsingMe(this);

         giveRayLengthTan(ref mDirectRay, ref mLength, ref mDirectTangent);
         mNumIterations = (int)(mLength  * mOwnerRoad.getWorldTesselationSize());    //CLM calculate this number to ensure proper worldspace segment spacing
      }
      public void update()
      {
         giveRayLengthTan(ref mDirectRay, ref mLength, ref mDirectTangent);
         mNumIterations = (int)(mLength* mOwnerRoad.getWorldTesselationSize()); //CLM calculate this number to ensure proper worldspace segment spacing
         if (mNumIterations < 2)
            mNumIterations = 1;
      }
      public void render()
      {
        //CLM moved up in the pipe so the parent can control us...

      }
      //--------------------------

      //these two calls will give us the neighbor road segment from our position
      //essential in finding intersections w/ neighboring nodes
      //CLM CHANGE THESE TO NOT CARE ABOUT THE ROOT!!!
      public roadSegment giveClosestCCWSegment(roadControlPoint rcp)
      {
         //RETURN MAX(ALL ANGLES FROM ME)
         //get the direction of the ROOT segment, going away from RCP
         roadSegment rootSegment = rcp.getRoadSegmentUsingMe(0);
         Vector3 rootRay = BMathLib.unitZ;// rcp == rootSegment.mPointA ? rootSegment.mDirectRay : -rootSegment.mDirectRay;
         Vector3 myRay = rcp == mPointA ? mDirectRay : -mDirectRay;

         float biggestAngle = 0;
         roadSegment biggestAngleSegment = this;

         for (int i = 0; i < rcp.numRoadSegmentsUsingMe(); i++)
         {
            roadSegment rs = rcp.getRoadSegmentUsingMe(i);
            if (rs == this)
               continue;

            Vector3 rsRay = rcp == rs.mPointA ? rs.mDirectRay : -rs.mDirectRay;
            float angle = BMathLib.vec2vec2Angle_0to360(BMathLib.vec3tovec2XZPlane(myRay), BMathLib.vec3tovec2XZPlane(rsRay));
            if (angle > biggestAngle)
            {
               biggestAngle = angle;
               biggestAngleSegment = rs;
            }

         }

         //get the direction of the ROOT segment, going away from RCP
         //roadSegment rootSegment = rcp.getRoadSegmentUsingMe(0);
         //Vector3 rootRay = rcp == rootSegment.mPointA ? rootSegment.mDirectRay : -rootSegment.mDirectRay;
         //Vector3 myRay = rcp == mPointA ? mDirectRay : -mDirectRay;

         //float maxAngle = BMathLib.vec2vec2Angle_0to360(BMathLib.vec3tovec2XZPlane(rootRay), BMathLib.vec3tovec2XZPlane(myRay));

         //float biggestAngle = 0;
         //roadSegment biggestAngleSegment = rootSegment;
         ////loop through the rest of the road segments on this control point
         ////find the closest to our angle, w/o going over
         //for(int i=0;i<rcp.numRoadSegmentsUsingMe();i++)
         //{
         //   roadSegment rs = rcp.getRoadSegmentUsingMe(i);
         //   if (rs == this)
         //      continue;

         //   Vector3 rsRay = rcp == rs.mPointA ? rs.mDirectRay : -rs.mDirectRay;
         //   float angle = BMathLib.vec2vec2Angle_0to360(BMathLib.vec3tovec2XZPlane(rootRay), BMathLib.vec3tovec2XZPlane(rsRay));
         //   if (angle > maxAngle)
         //      continue;

         //   if(angle > biggestAngle)
         //   {
         //      biggestAngle = angle;
         //      biggestAngleSegment = rs;
         //   }
         //}

         return biggestAngleSegment;
      }
      public roadSegment giveClosestCWSegment(roadControlPoint rcp)
      {
         //RETURN MAX(ALL ANGLES FROM ME)
         //get the direction of the ROOT segment, going away from RCP
         roadSegment rootSegment = rcp.getRoadSegmentUsingMe(0);
         Vector3 rootRay = BMathLib.unitZ;// rcp == rootSegment.mPointA ? rootSegment.mDirectRay : -rootSegment.mDirectRay;
         Vector3 myRay = rcp == mPointA ? mDirectRay : -mDirectRay;

         float biggestAngle = 360;
         roadSegment biggestAngleSegment = this;

         for (int i = 0; i < rcp.numRoadSegmentsUsingMe(); i++)
         {
            roadSegment rs = rcp.getRoadSegmentUsingMe(i);
            if (rs == this)
               continue;

            Vector3 rsRay = rcp == rs.mPointA ? rs.mDirectRay : -rs.mDirectRay;
            float angle = BMathLib.vec2vec2Angle_0to360(BMathLib.vec3tovec2XZPlane(myRay), BMathLib.vec3tovec2XZPlane(rsRay));
            if (angle < biggestAngle)
            {
               biggestAngle = angle;
               biggestAngleSegment = rs;
            }
             
         }

         //float maxAngle = BMathLib.vec2vec2Angle_0to360(BMathLib.vec3tovec2XZPlane(rootRay), BMathLib.vec3tovec2XZPlane(myRay));

         //float biggestAngle = 360;
         //roadSegment biggestAngleSegment = rootSegment;
         ////loop through the rest of the road segments on this control point
         ////find the closest to our angle, w/o going over
         //for (int i = 0; i < rcp.numRoadSegmentsUsingMe(); i++)
         //{
         //   roadSegment rs = rcp.getRoadSegmentUsingMe(i);
         //   if (rs == this)
         //      continue;

        
         //   Vector3 rsRay = rcp == rs.mPointA ? rs.mDirectRay : -rs.mDirectRay;
         //   float angle = BMathLib.vec2vec2Angle_0to360(BMathLib.vec3tovec2XZPlane(rootRay), BMathLib.vec3tovec2XZPlane(rsRay));
         //   if (angle < maxAngle)
         //      continue;

         //   if (angle < biggestAngle)
         //   {
         //      biggestAngle = angle;
         //      biggestAngleSegment = rs;
         //   }
         //}

         return biggestAngleSegment;
      }
      //--------------------------
      void addVert(List<VertexTypes.Pos_uv0> verts, float x, float z, float u, float v)
      {
         float h = mSnapToHeight ? TerrainGlobals.getTerrain().getHeightAtXZ(x, z) + 0.1f : 0;
         verts.Add(new VertexTypes.Pos_uv0(x, h, z, u, v));
      }
      void splineEdge34Intersects(List<VertexTypes.Pos_uv0> verts, roadControlPoint rcp, ref roadSegSpline rss, ref float uvVal)
      {
         Vector3 newCenter = Vector3.Empty;
         Vector3 centerTangPos = Vector3.Empty;
         Vector3 centerTangNeg = Vector3.Empty;
         Vector3 veloRay = Vector3.Empty;
         rcp.multiSegmentIntersectPoints(this, out newCenter, out centerTangPos, out centerTangNeg, out veloRay);

         float mUvIterPerSegmentIter = mOwnerRoad.getWorldTesselationSize() * 0.25f;
        
         if (rcp == mPointA)//CLM flip points if we're the start of a segment (needed for tri-stripping!!!!
         {
            uvVal -= mUvIterPerSegmentIter * 0.5f;// uVdiff;
            
            if (rcp.getRoadSegmentUsingMe(0).mPointA == rcp)
            {
               addVert(verts, centerTangPos.X, centerTangPos.Z, roadControlPointUVs.cMainRoadUVs.X, uvVal);
               addVert(verts, centerTangNeg.X, centerTangNeg.Z, roadControlPointUVs.cMainRoadUVs.Z, uvVal); uvVal+=mUvIterPerSegmentIter;
            }
            else
            {
               addVert(verts, centerTangNeg.X, centerTangNeg.Z, roadControlPointUVs.cMainRoadUVs.X, uvVal);
               addVert(verts, centerTangPos.X, centerTangPos.Z, roadControlPointUVs.cMainRoadUVs.Z, uvVal); uvVal += mUvIterPerSegmentIter;
            }
         }
         else   //Identical to the main segment 
         {

            uvVal -= mUvIterPerSegmentIter * 0.5f;// uVdiff;
            

            
            if(rcp.getRoadSegmentUsingMe(0).mPointB==rcp)
            {
               addVert(verts, centerTangPos.X, centerTangPos.Z, roadControlPointUVs.cMainRoadUVs.X, uvVal);
               addVert(verts, centerTangNeg.X, centerTangNeg.Z, roadControlPointUVs.cMainRoadUVs.Z, uvVal); uvVal += mUvIterPerSegmentIter;
            }
            else
            {
               addVert(verts, centerTangNeg.X, centerTangNeg.Z, roadControlPointUVs.cMainRoadUVs.X, uvVal);
               addVert(verts, centerTangPos.X, centerTangPos.Z, roadControlPointUVs.cMainRoadUVs.Z, uvVal); uvVal += mUvIterPerSegmentIter;
            }
            


         //   verts.Add(new VertexTypes.Pos_uv0(rcpWS.X, rcpWS.Y, rcpWS.Z, roadControlPointUVs.cMainRoadUVs.Z, roadControlPointUVs.cMainRoadUVs.Y));
         }
         

      }
      void splineGive12Edge(List<VertexTypes.Pos_uv0> verts, roadControlPoint rcp,  ref roadSegSpline rss,ref float uvVal)
      {
         float aDist = mPointAOffsetPOS < mPointAOffsetNEG ? mPointAOffsetPOS : mPointAOffsetNEG;
         float bDist = mPointBOffsetPOS < mPointBOffsetNEG ? mPointBOffsetPOS : mPointBOffsetNEG;

         float mDist = Math.Abs(bDist - aDist);

         float nLength = mDist * mLength;
         int nItrations = (int)(mLength * mOwnerRoad.getWorldTesselationSize());    //CLM calculate this number to ensure proper worldspace segment spacing

         float timeItr = (1 / (float)(mNumIterations - 1));
         Vector3 lastPt = Vector3.Empty;
         Vector3 tang = Vector3.Empty;
         float tme = 0;
         float stTime = 0;
         if (rcp == mPointA)
         {
            stTime = 0;
            lastPt = mPointA.giveWorldSpacePos();
            tme = aDist;// aDist + timeItr;
         }
         else if (rcp == mPointB)
         {
            stTime = bDist;// -timeItr;// 1 - timeItr;
            tme =  1;
            lastPt = GetPositionOnCubic(rss.centerA, rss.velocityA, rss.centerB, rss.velocityB, stTime);

         }
         float mUvIterPerSegmentIter = mOwnerRoad.getWorldTesselationSize() * 0.25f;
         Vector3 nextSplinePoint = GetPositionOnCubic(rss.centerA, rss.velocityA, rss.centerB, rss.velocityB, tme);
         

         //compute our tangent points for this segment
         //FRONT TANGENT
         if (rcp == mPointA)
         {
            tang = new Vector3(-rss.velocityA.Z, rss.velocityA.Y, rss.velocityA.X);
            tang.Normalize();
         }
         else
         {
            tang = getTangentOnCubic(rss.centerA, rss.velocityA, rss.centerB, rss.velocityB, stTime, timeItr);
         }
         tang *= mOwnerRoad.getRoadWidth();
         addVert(verts, lastPt.X + tang.X, lastPt.Z + tang.Z, roadControlPointUVs.cMainRoadUVs.X, uvVal);
         addVert(verts, lastPt.X - tang.X, lastPt.Z - tang.Z, roadControlPointUVs.cMainRoadUVs.Z, uvVal); uvVal += mUvIterPerSegmentIter;

         //BACK TANGENT
         if (rcp == mPointB)
         {
            tang = new Vector3(-rss.velocityB.Z, rss.velocityB.Y, rss.velocityB.X);
            tang.Normalize();
         }
         else
         {
            tang = getTangentOnCubic(rss.centerA, rss.velocityA, rss.centerB, rss.velocityB, tme, timeItr);
         }
         tang *= mOwnerRoad.getRoadWidth();
         addVert(verts, nextSplinePoint.X + tang.X, nextSplinePoint.Z + tang.Z, roadControlPointUVs.cMainRoadUVs.X, uvVal);
         addVert(verts, nextSplinePoint.X - tang.X, nextSplinePoint.Z - tang.Z, roadControlPointUVs.cMainRoadUVs.Z, uvVal); uvVal += mUvIterPerSegmentIter;

      }

      void angledGive2Edge(List<VertexTypes.Pos_uv0> verts, roadControlPoint rcp, ref roadSegSpline rss, ref float uvVal)
      {
        

         //find the other road segment
         roadSegment rsB = this == rcp.getRoadSegmentUsingMe(0) ? rcp.getRoadSegmentUsingMe(1) : rcp.getRoadSegmentUsingMe(0);

         //find the shared control point
         roadControlPoint centerPoint = rcp;

         //find the end points of each segment (ie the other points)
         roadControlPoint segAEnd = mPointA == centerPoint ? mPointB : mPointA;
         roadControlPoint segBEnd = rsB.mPointA == centerPoint ? rsB.mPointB : rsB.mPointA;

         //grab worldspace values
         Vector3 centerPointWS = TerrainGlobals.getTerrain().getPostDeformPos(centerPoint.mGridX, centerPoint.mGridY);
         Vector3 segAEndWS = TerrainGlobals.getTerrain().getPostDeformPos(segAEnd.mGridX, segAEnd.mGridY);
         Vector3 segBEndWS = TerrainGlobals.getTerrain().getPostDeformPos(segBEnd.mGridX, segBEnd.mGridY);

         //create the prime two rays (and their tangents)
         Vector3 segARay = BMathLib.Normalize(centerPointWS - segAEndWS);
         Vector3 segBRay = BMathLib.Normalize(centerPointWS - segBEndWS);

         //CLM don't allow acute angles less than 45 deg
         //THIS IS A LIMITATION OF THE SYSTEM!
         float angle = Geometry.RadianToDegree(BMathLib.vec3vec3Angle_0to180(segARay, segBRay));
         if(angle < 60)
         {
            splineGive12Edge(verts, rcp, ref rss, ref uvVal);
            return;
         }


         Vector3 segATan = new Vector3(-segARay.Z, segARay.Y, segARay.X);
         segATan = Vector3.Normalize(segATan) * mOwnerRoad.getRoadWidth();
         Vector3 segBTan = new Vector3(-segBRay.Z, segBRay.Y, segBRay.X);
         segBTan = Vector3.Normalize(segBTan) * mOwnerRoad.getRoadWidth();

         float segALen = Vector3.Length(centerPointWS - segAEndWS);

         //calculate the 2 tangent points of segment A;
         Vector3 segAEndInner = new Vector3(segAEndWS.X - segATan.X, segAEndWS.Y, segAEndWS.Z - segATan.Z);
         Vector3 segAEndOuter = new Vector3(segAEndWS.X + segATan.X, segAEndWS.Y, segAEndWS.Z + segATan.Z);

         //for S&G calculate the normal of the center point
         //(inverted to ensure normal points proper direction)
         Vector3 centerNormal = Vector3.Normalize(segARay + segBRay);
         if (centerNormal == Vector3.Empty)
            centerNormal = segATan;

         //create the center normal as a line segment
         Vector2 centerLineOuter = new Vector2(centerPointWS.X + centerNormal.X * 10, centerPointWS.Z + centerNormal.Z * 10);
         Vector2 centerLineInner = new Vector2(centerPointWS.X - centerNormal.X * 10, centerPointWS.Z - centerNormal.Z * 10);

         //get our intersection points on the line
         Vector2 intersection = new Vector2();
         Vector2 segRay2D = new Vector2(segARay.X, segARay.Z);

         Vector2 ln0 = new Vector2(segAEndInner.X, segAEndInner.Z);
         Vector2 ln1 = ln0 + (20 * segALen * segRay2D);
         bool hit = BMathLib.lineLineIntersect2D(ref ln0, ref  ln1, ref centerLineInner, ref centerLineOuter, out intersection) == BMathLib.eLineIntersectResult.INTERESECTING;
         Vector3 innerPt = new Vector3(intersection.X, centerPointWS.Y, intersection.Y);
         if (!hit)
         {
            return;// Debug.Assert(hit);
         }

         ln0 = new Vector2(segAEndOuter.X, segAEndOuter.Z);
         ln1 = ln0 + (20 * segALen * segRay2D);
         hit = BMathLib.lineLineIntersect2D(ref ln0, ref ln1, ref centerLineInner, ref centerLineOuter, out intersection) == BMathLib.eLineIntersectResult.INTERESECTING;
         Vector3 outerPt = new Vector3(intersection.X, centerPointWS.Y, intersection.Y);
         if (!hit)
         {
            return;// Debug.Assert(hit);
         }





         //now, calculate an extra point so we can fix some UV stretching issues
         Vector3 segAStartInner = new Vector3(centerPointWS.X - segATan.X, centerPointWS.Y, centerPointWS.Z - segATan.Z);
         Vector3 segAStartOuter = new Vector3(centerPointWS.X + segATan.X, centerPointWS.Y, centerPointWS.Z + segATan.Z);
         Vector3 addSegPt = Vector3.Empty;
         float U = 0;

         float mUvIterPerSegmentIter = mOwnerRoad.getWorldTesselationSize() * 0.25f;

         ////Do we need to swap Inner & Outer?
         ////Do an angle check.(outer pt will always be smaller angle to end points than inner point)
         float innrAngle = BMathLib.vec3vec3Angle_0to180(BMathLib.Normalize(innerPt - segAEndWS), BMathLib.Normalize(innerPt - segBEndWS));
         float outrAngle =BMathLib.vec3vec3Angle_0to180(BMathLib.Normalize(outerPt - segAEndWS), BMathLib.Normalize(outerPt - segBEndWS));
         if (innrAngle < outrAngle)
         {
            BMathLib.pointClosestToLine(ref segAStartInner, ref segAEndInner, ref outerPt, out U);
            addSegPt = segAStartInner + ((segAEndInner - segAStartInner) * U);
            
            if (rcp == mPointB)
            {
               addVert(verts, outerPt.X, outerPt.Z, roadControlPointUVs.cMainRoadUVs.X, uvVal); //uvVal += mUvIterPerSegmentIter;
               addVert(verts, addSegPt.X, addSegPt.Z, roadControlPointUVs.cMainRoadUVs.Z, uvVal); uvVal += mUvIterPerSegmentIter;
               addVert(verts, innerPt.X, innerPt.Z, roadControlPointUVs.cMainRoadUVs.Z, uvVal); uvVal += mUvIterPerSegmentIter;
            }
            else
            {
               addVert(verts, innerPt.X, innerPt.Z, roadControlPointUVs.cMainRoadUVs.X, uvVal); uvVal += mUvIterPerSegmentIter;
               addVert(verts, addSegPt.X, addSegPt.Z, roadControlPointUVs.cMainRoadUVs.X, uvVal);
               addVert(verts, outerPt.X, outerPt.Z, roadControlPointUVs.cMainRoadUVs.Z, uvVal); uvVal += mUvIterPerSegmentIter;
            }
          
         }
         else
         {

            BMathLib.pointClosestToLine(ref segAStartOuter, ref segAEndOuter, ref innerPt, out U);
            addSegPt = segAStartOuter + ((segAEndOuter - segAStartOuter) * U);

            //add our verts to our tristrip
            
            if (rcp == mPointB)
            {
               addVert(verts, addSegPt.X, addSegPt.Z, roadControlPointUVs.cMainRoadUVs.X, uvVal); //uvVal += mUvIterPerSegmentIter;
               addVert(verts, innerPt.X, innerPt.Z, roadControlPointUVs.cMainRoadUVs.Z, uvVal); uvVal += mUvIterPerSegmentIter;
               addVert(verts, outerPt.X, outerPt.Z, roadControlPointUVs.cMainRoadUVs.X, uvVal); uvVal += mUvIterPerSegmentIter;

            }
            else
            {

               
               addVert(verts, outerPt.X, outerPt.Z, roadControlPointUVs.cMainRoadUVs.Z, uvVal); uvVal += mUvIterPerSegmentIter;
               addVert(verts, innerPt.X, innerPt.Z, roadControlPointUVs.cMainRoadUVs.X, uvVal); 
               addVert(verts, addSegPt.X, addSegPt.Z, roadControlPointUVs.cMainRoadUVs.Z, uvVal); uvVal += mUvIterPerSegmentIter;
               
               
            }
         
         }






      }

      void updateEdgeVisual(List<VertexTypes.Pos_uv0> verts, roadControlPoint rcp, ref roadSegSpline rss, ref float uvVal)
      {
         int numSeg = rcp.numRoadSegmentsUsingMe();
         switch(numSeg)
         {
            //----------------------------------------------
            case 1:        //end of the road
               splineGive12Edge(verts, rcp, ref rss, ref uvVal);// ref rss.centerA, ref rss.velocityA, ref rss.centerB, ref rss.velocityB);
               break;
               //----------------------------------------------
            case 2://2 INTERSECTIONS
               switch (rcp.mControlPointType)
               {
                  case roadControlPoint.eControlPointType.cAngled:
                     angledGive2Edge(verts, rcp, ref rss, ref uvVal);
                     break;
                  case roadControlPoint.eControlPointType.cTightCurve:

                     break;
                  case roadControlPoint.eControlPointType.cSplinePoint:
                     splineGive12Edge(verts, rcp, ref rss, ref uvVal);

                     break;

               }
               break;

            //----------------------------------------------
            case 3://3 INTERSECTIONS
               switch (rcp.mControlPointType)
               {
                  case roadControlPoint.eControlPointType.cAngled:
                     splineEdge34Intersects(verts, rcp, ref rss, ref uvVal);
                     break;
                  case roadControlPoint.eControlPointType.cTightCurve:

                     break;
                  case roadControlPoint.eControlPointType.cSplinePoint:
                     splineEdge34Intersects(verts, rcp, ref rss, ref uvVal);

                     break;

               }
               break;
            //----------------------------------------------
            default:
            case 4: //4 INTERSECTIONS
               switch (rcp.mControlPointType)
               {
                  case roadControlPoint.eControlPointType.cAngled:
                     splineEdge34Intersects(verts, rcp, ref rss, ref uvVal);
                     break;
                  case roadControlPoint.eControlPointType.cTightCurve:

                     break;
                  case roadControlPoint.eControlPointType.cSplinePoint:
                     splineEdge34Intersects(verts, rcp, ref rss, ref uvVal);

                     break;

               }
               break;
         }
      }

      void mainSection(List<VertexTypes.Pos_uv0> verts, roadSegSpline rss, ref float uvVal)
      {
         
         //figure out our stride etc..
         float aDist = mPointAOffsetPOS < mPointAOffsetNEG ? mPointAOffsetPOS : mPointAOffsetNEG;
         float bDist = mPointBOffsetPOS < mPointBOffsetNEG ? mPointBOffsetPOS : mPointBOffsetNEG;

         float mDist = Math.Abs(bDist - aDist);

         float nLength = mDist * mLength;
         int nItrations = (int)(mLength * mOwnerRoad.getWorldTesselationSize());    //CLM calculate this number to ensure proper worldspace segment spacing

         int startIter = (int)((aDist * mLength) * mOwnerRoad.getWorldTesselationSize());
         int endIter = (int)((bDist * mLength) * mOwnerRoad.getWorldTesselationSize());
         int deltaItr = endIter - startIter;

         float timeItr = (1 / (float)(nItrations - 1));
         
         Vector3 nextSplinePoint = rss.getPos(startIter * timeItr);
         Vector3 tang = Vector3.Empty;


         float mUvIterPerSegmentIter = mOwnerRoad.getWorldTesselationSize() * 0.25f;
         float uvItr = mUvIterPerSegmentIter;

         
         
         if (deltaItr == 0 || deltaItr == 1)
         {
            float tme = 0;
            nextSplinePoint = rss.getPos(aDist);
            tang = rss.getTang(tme, timeItr) * mOwnerRoad.getRoadWidth();
            addVert(verts, nextSplinePoint.X + tang.X, nextSplinePoint.Z + tang.Z, roadControlPointUVs.cMainRoadUVs.X, uvVal);
            addVert(verts, nextSplinePoint.X - tang.X, nextSplinePoint.Z - tang.Z, roadControlPointUVs.cMainRoadUVs.Z, uvVal); uvVal += uvItr;
 
            tme = 1;
            nextSplinePoint = rss.getPos(bDist);
            tang = rss.getTang(tme, timeItr) * mOwnerRoad.getRoadWidth();
            addVert(verts, nextSplinePoint.X + tang.X, nextSplinePoint.Z + tang.Z, roadControlPointUVs.cMainRoadUVs.X, uvVal);
            addVert(verts, nextSplinePoint.X - tang.X, nextSplinePoint.Z - tang.Z, roadControlPointUVs.cMainRoadUVs.Z, uvVal); uvVal += uvItr;

         }
         else
         {
            for (int i = startIter; i < endIter; i++)
            {
               float tme = aDist + ((i * timeItr) * mDist);
               nextSplinePoint = rss.getPos(tme);

               tang = rss.getTang(tme, timeItr) * mOwnerRoad.getRoadWidth();
               addVert(verts, nextSplinePoint.X + tang.X, nextSplinePoint.Z + tang.Z, roadControlPointUVs.cMainRoadUVs.X, uvVal); //uvVal += uvItr;
               addVert(verts, nextSplinePoint.X - tang.X,  nextSplinePoint.Z - tang.Z, roadControlPointUVs.cMainRoadUVs.Z, uvVal); uvVal += uvItr;

            }
         }
      }
      
      //-----------------
      roadSegSpline giveSegSpline()
      {
         roadSegSpline rss = new roadSegSpline();
         rss.createFromRoadSegment(this);

         return rss;
      }
      roadSegSpline giveTangSpline(bool posOrNeg)
      {
         roadSegSpline rss = giveSegSpline();
         return segSplineToTangSpline(rss,posOrNeg);
      }
      roadSegSpline segSplineToTangSpline(roadSegSpline rss, bool posOrNeg)
      {
         roadSegSpline tss = new roadSegSpline();
         rss.copyTo(ref tss);
         
         float timeItr = (1 / (float)(mNumIterations - 1));
         Vector3 tang0 = rss.getTang(0, timeItr) * mOwnerRoad.getRoadWidth();
         
         if (posOrNeg)
         {
            tss.centerA = rss.centerA + tang0;
            tss.centerB = rss.centerB + tang0;
         }
         else
         {
            tss.centerA = rss.centerA - tang0;
            tss.centerB = rss.centerB - tang0;
         }
         return tss;
      }
      //-----------------
      float giveEdgeDist(roadControlPoint rcp, bool posOrNeg, ref roadSegSpline rss, out Vector3 intPoint)
      {
         float timeItr = (1 / (float)(mNumIterations));
         if (rcp.numRoadSegmentsUsingMe() == 0)
         {
            intPoint = Vector3.Empty;
            return 0;
         }

         if (rcp.numRoadSegmentsUsingMe() == 1)
         {
            intPoint = Vector3.Empty;
            return rcp == mPointA?0:1;
         } 

         if (rcp.numRoadSegmentsUsingMe() == 2)
         {
            intPoint = Vector3.Empty;
            return rcp == mPointA ? timeItr : (1 - timeItr);
         }

        
         if(rcp.numRoadSegmentsUsingMe()==3 || rcp.numRoadSegmentsUsingMe()==4 )
         {
            Vector3 newCenter = Vector3.Empty;
            Vector3 centerTangPos = Vector3.Empty;
            Vector3 centerTangNeg = Vector3.Empty;
            Vector3 veloRay = Vector3.Empty;
            rcp.multiSegmentIntersectPoints(this, out newCenter, out centerTangPos, out centerTangNeg,out veloRay);

           // float len = Vector3.Length(newCenter - rcp.giveWorldSpacePos());

            Vector3 otherRCP = rcp == mPointA? mPointB.giveWorldSpacePos():mPointA.giveWorldSpacePos();
            float d = Vector3.Length(newCenter - otherRCP);

            if (mLength - d > mOwnerRoad.getWorldTesselationSize())
               d += mOwnerRoad.getWorldTesselationSize();

            float k = (d) / mLength;


            if (rcp == mPointA)
            {
               k = 1 - k;
               k *= 1.25f; //CLM MAGIC NUMBER TO KEEP POLLIES FROM OVERLAPPING @ ROOT
            }
           
            intPoint = newCenter;
            return k;
         
         }

         

         intPoint = Vector3.Empty;
         return rcp == mPointA ? timeItr : (1 - timeItr);

      }

      //---------------------
      public Vector3 giveSplineVelocity(roadControlPoint rcp)
      {
         //THIS IS THE MAIN LOGIC SYSTEM FOR ALL VELOCITY VALUES
         
         Vector3 velocity = Vector3.Empty;

         if (rcp.mControlPointType != roadControlPoint.eControlPointType.cSplinePoint /*&& rcp.numRoadSegmentsUsingMe() < 3*/)
         {
            velocity = mDirectRay;
         }
         else if(mPointA.numRoadSegmentsUsingMe()<2 && mPointB.numRoadSegmentsUsingMe()<2)
         {
            //i'm a segment
            velocity = mDirectRay;
         }
         else if(mPointA == rcp && mPointA.numRoadSegmentsUsingMe() < 2 || mPointB == rcp && mPointB.numRoadSegmentsUsingMe() < 2)
         {
            //i'm an end node
            velocity = mDirectRay;
         }
         else if (rcp.numRoadSegmentsUsingMe() == 2)
         {
            //we're an 'in the middle' segment
            //CLM we'll have to deal with discontinuities later
            int aIndex = 1 - rcp.segmentUsingMeIndex(this);
            roadSegment rsA = rcp.getRoadSegmentUsingMe(aIndex);
            roadControlPoint minPoint = rcp == rsA.mPointA ? rsA.mPointB : rsA.mPointA;
            Vector3 minPointWS = TerrainGlobals.getTerrain().getPostDeformPos(minPoint.mGridX, minPoint.mGridY);

            Vector3 myWorldPos = rcp.giveWorldSpacePos();

            // split the angle 
            if (this.mPointA == rcp)
            {
               velocity = BMathLib.Normalize(mPointB.giveWorldSpacePos() - myWorldPos) - BMathLib.Normalize(minPointWS - myWorldPos);
            }
            else if (this.mPointB == rcp)
            {
               velocity = BMathLib.Normalize(minPointWS - myWorldPos) - BMathLib.Normalize(mPointA.giveWorldSpacePos() - myWorldPos);
            }

            velocity.Normalize();
         }
         else if( rcp.numRoadSegmentsUsingMe() == 3 || rcp.numRoadSegmentsUsingMe() ==4)
         {
            //use the multi-segement stitching verts to create a vector for linking.
            roadSegment rcpRoot = rcp.getRoadSegmentUsingMe(0);
            Vector3 worldPt = rcp.giveWorldSpacePos();
      
            Vector3 newCenter = Vector3.Empty;
            Vector3 centerTangPos = Vector3.Empty;
            Vector3 centerTangNeg = Vector3.Empty;
            
            rcp.multiSegmentIntersectPoints(this, out newCenter, out centerTangPos, out centerTangNeg, out velocity);


            Vector3 tVelo = rcp == mPointA? tVelo = BMathLib.Normalize(newCenter - worldPt) : tVelo = BMathLib.Normalize(worldPt - newCenter);

            if (BMathLib.Dot(ref tVelo, ref velocity) < 0)
               velocity = -velocity;
         }

        
         return velocity;
      }
      protected Vector3 GetPositionOnCubic(Vector3 startPos, Vector3 startVel, Vector3 endPos, Vector3 endVel, float time)
      {
         time = BMathLib.Saturate(time);
         Matrix hermite = BMathLib.matrixFrom16floats(2.0f, -2.0f, 1.0f, 1.0f,
                                                      -3.0f, 3.0f, -2.0f, -1.0f,
                                                       0.0f, 0.0f, 1.0f, 0.0f,
                                                       1.0f, 0.0f, 0.0f, 0.0f
                                                    );


         Matrix m = new Matrix();

         BMathLib.setRow(ref m, 0, startPos);
         BMathLib.setRow(ref m, 1, endPos);
         BMathLib.setRow(ref m, 2, startVel);
         BMathLib.setRow(ref m, 3, endVel);
         m.M44 = 1;

         m = Matrix.Multiply(hermite, m);

         Vector4 timeVector = new Vector4(time * time * time, time * time, time, 1.0f);
         Vector4 result = Vector4.Transform(timeVector, m); // vector * matrix
         return new Vector3(result.X, result.Y, result.Z);
      }
      Vector3 getTangentOnCubic(Vector3 startPos, Vector3 startVel, Vector3 endPos, Vector3 endVel, float time, float timeDelta)
      {
         Vector3 tang = Vector3.Empty;
         Vector3 myPos = GetPositionOnCubic(startPos, startVel, endPos, endVel, time);

         Vector3 nextPos = GetPositionOnCubic(startPos, startVel, endPos, endVel, time + timeDelta);
         if (time == 1)
            nextPos = -GetPositionOnCubic(startPos, startVel, endPos, endVel, time - timeDelta);

         Vector3 dir = BMathLib.Normalize(nextPos - myPos);

         tang = BMathLib.Cross(dir, BMathLib.unitY);

         return tang;
      }

      //---------------------

      
      Vector3 mPointAintersectPOS;
      float mPointAOffsetPOS = 0;
      Vector3 mPointAintersectNEG;
      float mPointAOffsetNEG = 0;
      float mPointAOffsetMax = 0;

      Vector3 mPointBintersectPOS;
      float mPointBOffsetPOS = 0;
      Vector3 mPointBintersectNEG;
      float mPointBOffsetNEG = 0;
      float mPointAOffsetMin = 0;

    
      //---------------------
      void updateVisualsSplineLine(List<VertexTypes.Pos_uv0> verts)
      {
         Vector3 centerTangPos = Vector3.Empty;
         Vector3 centerTangNeg = Vector3.Empty;
         Vector3 veloRay = Vector3.Empty;

         float uvVal = roadControlPointUVs.cMainRoadUVs.Z;

         roadSegSpline rss = giveSegSpline();

         mPointAOffsetPOS = giveEdgeDist(mPointA, true, ref rss, out mPointAintersectPOS);
         mPointAOffsetNEG = giveEdgeDist(mPointA, false, ref rss, out mPointAintersectNEG);
         mPointAOffsetMax = Math.Max(mPointAOffsetPOS, mPointAOffsetNEG);
         if (mPointA.numRoadSegmentsUsingMe() > 2) mPointA.multiSegmentIntersectPoints(this, out rss.centerA, out centerTangPos, out centerTangNeg, out veloRay);//modify position distances iff multi intersect

         mPointBOffsetPOS = giveEdgeDist(mPointB, true, ref rss, out mPointBintersectPOS);
         mPointBOffsetNEG = giveEdgeDist(mPointB, false, ref rss, out mPointBintersectNEG);
         mPointAOffsetMin = Math.Max(mPointBOffsetPOS, mPointBOffsetNEG);
         if (mPointB.numRoadSegmentsUsingMe() > 2) mPointB.multiSegmentIntersectPoints(this, out rss.centerB, out centerTangPos, out centerTangNeg, out veloRay);//modify position distances iff multi intersect

         if (mPointAOffsetMax != 0) 
            updateEdgeVisual(verts, mPointA, ref rss, ref uvVal);
         
         mainSection(verts, rss, ref uvVal);

         if (mPointAOffsetMin != 1) 
            updateEdgeVisual(verts, mPointB, ref rss, ref uvVal);
         

        
         rss = null;
      }
      public void updateVisuals(ref int myStartVert, ref List<VertexTypes.Pos_uv0> verts, bool snapToHeight)
      {
         if (mPointA == null || mPointB == null)
            return;

         mSnapToHeight = snapToHeight;

         update();

         mStartVertInParentSegment = myStartVert;

         mNumVerts = verts.Count;
         updateVisualsSplineLine(verts);
         mNumVerts = verts.Count - mNumVerts;
         
         myStartVert += mNumVerts;
      }
      
      //--------------------------
      public bool usesPoint(roadControlPoint pt)
      {
         return (mPointA == pt || mPointB == pt);
      }
      public void calcTangentPoints(roadControlPoint rcp, out Vector3 center, out Vector3 inner, out Vector3 outer)
      {
         Vector3 startPt = TerrainGlobals.getTerrain().getPostDeformPos(mPointA.mGridX, mPointA.mGridY);
         Vector3 endPt = TerrainGlobals.getTerrain().getPostDeformPos(mPointB.mGridX, mPointB.mGridY);
         Vector3 ray = Vector3.Normalize(endPt - startPt);
         Vector3 tang = new Vector3(-ray.Z, ray.Y, ray.X);
         tang = Vector3.Normalize(tang);
         tang *= mOwnerRoad.getRoadWidth();

         Vector3 worldPt = TerrainGlobals.getTerrain().getPostDeformPos(rcp.mGridX, rcp.mGridY);
         inner = new Vector3(worldPt.X - tang.X, worldPt.Y, worldPt.Z - tang.Z);
         outer = new Vector3(worldPt.X + tang.X, worldPt.Y, worldPt.Z + tang.Z);

         center = TerrainGlobals.getTerrain().getPostDeformPos(rcp.mGridX, rcp.mGridY);
      }
      public void giveRayLengthTan(ref Vector3 ray, ref float length, ref Vector3 tang)
      {
         Vector3 startPt = TerrainGlobals.getTerrain().getPostDeformPos(mPointA.mGridX, mPointA.mGridY);
         Vector3 endPt = TerrainGlobals.getTerrain().getPostDeformPos(mPointB.mGridX, mPointB.mGridY);
         length = Vector3.Length(endPt - startPt);
         ray = Vector3.Normalize(endPt - startPt);
         tang = new Vector3(-ray.Z, ray.Y, ray.X);
         tang = Vector3.Normalize(tang);
      }
      public Vector3 giveRay()
      {
         Vector3 startPt = TerrainGlobals.getTerrain().getPostDeformPos(mPointA.mGridX, mPointA.mGridY);
         Vector3 endPt = TerrainGlobals.getTerrain().getPostDeformPos(mPointB.mGridX, mPointB.mGridY);
         return Vector3.Normalize(endPt - startPt);
      }
      public float giveLength()
      {
         return mLength;
      }


   }
   //---------------------------------------
   //---------------------------------------
   //---------------------------------------
   //---------------------------------------
   public class Road
   {
      List<roadControlPoint> mControlPoints = new List<roadControlPoint>();
      List<roadControlPoint> mSelectedControlPoints = new List<roadControlPoint>();
      List<roadSegment> mRoadSegments = new List<roadSegment>();

      public VertexBuffer mSegmentVB = null;
      public int mNumVerts = 0;
      public List<VertexTypes.Pos_uv0> mVerts = new List<VertexTypes.Pos_uv0>();

      TextureHandle mTexture = null;
      float mRoadWidth = 1;
      float mIterationWorldLength = 0.75f;
      string mRoadTextureName = null;

      
      

      public Road()
      {
      }
      public Road(string roadTextureName, float roadWidth)
      {
         setRoadTexture(roadTextureName);
         setRoadWidth(roadWidth);
      }
      ~Road()
      {
         destroy();
      }
      //--------------------------------
      public void init()
      {

      }
      public void destroy()
      {
         if (mSegmentVB != null)
         {
            mSegmentVB.Dispose();
            mSegmentVB = null;
         }
         mControlPoints.Clear();
         mTexture = null;
      }
      public void render(bool renderDebugs)
      {
         

         BRenderDevice.getDevice().SetTexture(0, mTexture.mTexture);
         BRenderDevice.getDevice().SetSamplerState(0, SamplerStageStates.AddressU, (int)TextureAddress.Wrap);
         BRenderDevice.getDevice().SetSamplerState(0, SamplerStageStates.AddressV, (int)TextureAddress.Wrap);
         BRenderDevice.getDevice().VertexDeclaration = VertexTypes.Pos_uv0.vertDecl;
         BRenderDevice.getDevice().SetStreamSource(0, mSegmentVB, 0);
        // BRenderDevice.getDevice().SetRenderState(RenderStates.FillMode, (int)FillMode.WireFrame);
         for (int i = 0; i < mRoadSegments.Count; i++)
         {
            if (mRoadSegments[i].mNumVerts < 2)
               continue;
     
               BRenderDevice.getDevice().DrawPrimitives(PrimitiveType.TriangleStrip, mRoadSegments[i].mStartVertInParentSegment, mRoadSegments[i].mNumVerts - 2);
          
         }

    //     BRenderDevice.getDevice().SetRenderState(RenderStates.FillMode, (int)FillMode.Solid);

         for (int i = 0; i < mControlPoints.Count; i++)
            mControlPoints[i].render();
       
         if(renderDebugs)
         {
            for (int i = 0; i < mControlPoints.Count; i++)
               mControlPoints[i].renderDebugPrims();
         }
      }
      //--------------------------------
      public void updateControlPoints()
      {
      }
      public void rebuildVisuals()
      {
         rebuildVisuals(false);
      }
      public void rebuildVisuals(bool snapToHeight)
      {
         mNumVerts = 0;
         mVerts.Clear();
         for (int i = 0; i < mRoadSegments.Count; i++)
         {
            mRoadSegments[i].updateVisuals(ref mNumVerts, ref mVerts, snapToHeight);
         }
         setVertsToSegmentStream(mVerts);

         for (int i = 0; i < mControlPoints.Count; i++)
            mControlPoints[i].updateVisual(mRoadWidth, false, snapToHeight);
      }
      void setVertsToSegmentStream(List<VertexTypes.Pos_uv0> verts)
      {
         mNumVerts = verts.Count;
         
         if (mSegmentVB != null)
         {
            mSegmentVB.Dispose();
            mSegmentVB = null;
         }

         if (verts.Count == 0)
            return;

         mSegmentVB = new VertexBuffer(typeof(VertexTypes.Pos_uv0), mNumVerts, BRenderDevice.getDevice(), Usage.None, VertexTypes.Pos_uv0.FVF_Flags, Pool.Managed);

         GraphicsStream gStream = mSegmentVB.Lock(0, 0, LockFlags.None);
         for (int i = 0; i < verts.Count; i++)
            gStream.Write(verts[i]);
         mSegmentVB.Unlock();

      }
      //--------------------------------
      public int addPoint(int gridX, int gridY, roadControlPoint.eControlPointType type)
      {
         mControlPoints.Add(new roadControlPoint(gridX, gridY, type));
         mControlPoints[mControlPoints.Count - 1].updateVisual(mRoadWidth,true);
         return mControlPoints.Count - 1;
      }
      //--------------------------------
      public void removePoint(int index)
      {
         removePoint(mControlPoints[index]);
      }
      public void removePoint(roadControlPoint rcp)
      {
         for (int i = 0; i < mRoadSegments.Count;i++ )
         {
            if (mRoadSegments[i].usesPoint(rcp))
            {
               mRoadSegments[i].destroy();
               mRoadSegments.RemoveAt(i);
               i--;
            }
         }
         mControlPoints.Remove(rcp);

         rebuildVisuals();
      }
      public void removeDanglingControlPoints()
      {
         for(int i=0;i<mControlPoints.Count;i++)
         {
            bool used = false;

            for (int k = 0; k < mRoadSegments.Count; k++)
            {
               if (mRoadSegments[k].usesPoint(mControlPoints[i]))
               {
                  used = true;
                  break;
               }
            }
            if (!used)
            {
               removePoint(mControlPoints[i]);
               i--;
            }
            
         }
      }

      //--------------------------------
      public void movePoint(roadControlPoint rcp, int gridOffsetX, int gridOffsetY)
      {
         rcp.mGridX = (int)BMathLib.Clamp(rcp.mGridX + gridOffsetX, 0, TerrainGlobals.getTerrain().getNumXVerts() - 2);
         rcp.mGridY = (int)BMathLib.Clamp(rcp.mGridY + gridOffsetY, 0, TerrainGlobals.getTerrain().getNumXVerts() - 2);
      }
      public void moveSelectedPoints(int gridOffsetX, int gridOffsetY)
      {
         for(int i=0;i<mSelectedControlPoints.Count;i++)
         {
            movePoint(mSelectedControlPoints[i], gridOffsetX, gridOffsetY);
         }
         removeDanglingControlPoints();
      }
      public void changeSelectedPointType(roadControlPoint.eControlPointType type)
      {
         for (int i = 0; i < mSelectedControlPoints.Count; i++)
         {
            mSelectedControlPoints[i].mControlPointType = type;
            mSelectedControlPoints[i].updateVisual(mRoadWidth,true);
         }
      }
      public int getNumSelectedpoints()
      {
         return mSelectedControlPoints.Count;
      }
      public roadControlPoint getSelectedControlPoint(int index)
      {
         if (index < 0 || index >= mSelectedControlPoints.Count)
            return null;
         return mSelectedControlPoints[index];
      }
      //--------------------------------
      public int giveControlPointIndex(int gridX, int gridY)
      {
         for(int i=0;i<mControlPoints.Count;i++)
         {
            if(mControlPoints[i].mGridX == gridX && mControlPoints[i].mGridY==gridY)
            {
               return i;
            }
         }
         return -1;
      }
      public int giveControlPointIntersect2D(int gridX, int gridY)
      {
         for(int i=0;i<mControlPoints.Count;i++)
         {
            if (mControlPoints[i].pointIntersect2D(gridX,gridY, mRoadWidth))
            {
               return i;
            }
         }
         return -1;
      }
      public roadControlPoint getPoint(int index)
      {
         return mControlPoints[index];
      }
      //--------------------------------
      public bool addSegment(Point a, Point b,roadControlPoint.eControlPointType type)
      {
         bool closedLineSeg = true;
         roadSegment rs = new roadSegment();
         int indexA = giveControlPointIntersect2D(a.X, a.Y);
         int indexB = giveControlPointIntersect2D(b.X, b.Y);


         if (indexA == -1)
            indexA = addPoint(a.X, a.Y, type);

         if (indexB == -1)
         {
            closedLineSeg = false;
            indexB = addPoint(b.X, b.Y, type);
         }

         if (indexA == indexB || 
            mControlPoints[indexA].numRoadSegmentsUsingMe() >= 4 ||
            mControlPoints[indexB].numRoadSegmentsUsingMe() >= 4)
            return closedLineSeg;

         rs.init(this, mControlPoints[indexA],mControlPoints[indexB]);

       //  rs.updateVisuals();
         mRoadSegments.Add(rs);
         return closedLineSeg;
      }
      public roadSegment getRoadSegment(int index)
      {
         if (index < 0 || index >= mRoadSegments.Count)
            return null;
         return mRoadSegments[index];
      }
      //--------------------------------
      public void selectPointsRayCast(Vector3 rayOrig, Vector3 rayDir, bool clearFirst)
      {
         if (clearFirst)
            clearSelectedPoints();

         for (int i = 0; i < mControlPoints.Count; i++)
         {
            if (mControlPoints[i].rayCastIntersect(rayOrig, rayDir, mRoadWidth))
            {
               mControlPoints[i].mIsSelected = true;
               mSelectedControlPoints.Add(mControlPoints[i]);
            }
         }
      }
      public void selectPointsDrag(Vector3[] points, bool clearFirst)
      {
         if (clearFirst)
            clearSelectedPoints(); 

         for (int i = 0; i < mControlPoints.Count; i++)
         {
            if (mControlPoints[i].rayCastDrag(points,mRoadWidth))
            {
               mControlPoints[i].mIsSelected = true;
               mSelectedControlPoints.Add(mControlPoints[i]);
            }
         }
      }
      public void clearSelectedPoints()
      {
         for (int i = 0; i < mSelectedControlPoints.Count;i++ )
            mSelectedControlPoints[i].mIsSelected = false;

            mSelectedControlPoints.Clear();
      }
      public void removeSelectedPoints()
      {
         for(int i=0;i<mSelectedControlPoints.Count;i++)
         {
            removePoint(mSelectedControlPoints[i]);
         }
         mSelectedControlPoints.Clear();

         removeDanglingControlPoints();
      }
      public void splitSelectedPoints()
      {
         for(int i=0;i<mSelectedControlPoints.Count;i++)
         {
            roadControlPoint rcp = mSelectedControlPoints[i];
            if (rcp.numRoadSegmentsUsingMe() == 1)
               continue;
            for (int c = 0; c <rcp.numRoadSegmentsUsingMe() ; c++)
            {
               Vector3 dir = rcp.getRoadSegmentUsingMe(c).giveRay();
               if (rcp == rcp.getRoadSegmentUsingMe(c).mPointA)
                  dir = -dir;
               Vector3 nPos = new Vector3(rcp.mGridX, 0, rcp.mGridY);
               nPos += -dir*3;
               int mGridX = (int)(nPos.X);
               int mGridY = (int)(nPos.Z);
               int index = addPoint(mGridX, mGridY, rcp.mControlPointType);

               mControlPoints[index].addRoadSegmentUsingMe(rcp.getRoadSegmentUsingMe(c));
               if (rcp.getRoadSegmentUsingMe(c).mPointA == rcp)
                  rcp.getRoadSegmentUsingMe(c).mPointA = mControlPoints[index];
               else
                  rcp.getRoadSegmentUsingMe(c).mPointB = mControlPoints[index];
            }
            mControlPoints.Remove(rcp);
         }
         mSelectedControlPoints.Clear();
      }
      public void collapseSelectedPoints()
      {
         if (mSelectedControlPoints.Count > 4)
            return;

        
         Point avgPos = new Point();
         for (int c = 0; c < mSelectedControlPoints.Count; c++)
         {
            avgPos.X += mSelectedControlPoints[c].mGridX;
            avgPos.Y += mSelectedControlPoints[c].mGridY;
         }
         avgPos.X /= mSelectedControlPoints.Count;
         avgPos.Y /= mSelectedControlPoints.Count;

         int index = addPoint(avgPos.X, avgPos.Y, roadControlPoint.eControlPointType.cAngled);
         for (int k = 0; k < mSelectedControlPoints.Count; k++)
         {
            roadControlPoint rcp = mSelectedControlPoints[k];
            for (int c = 0; c < rcp.numRoadSegmentsUsingMe(); c++)
            {
               mControlPoints[index].addRoadSegmentUsingMe(rcp.getRoadSegmentUsingMe(c));
               if (rcp.getRoadSegmentUsingMe(c).mPointA == rcp)
                  rcp.getRoadSegmentUsingMe(c).mPointA = mControlPoints[index];
               else
                  rcp.getRoadSegmentUsingMe(c).mPointB = mControlPoints[index];
            }
            mControlPoints.Remove(rcp);
            index--;
         }

         //remove any road segments whom A & B point to the same thing
         for(int i=0;i<mRoadSegments.Count;i++)
         {
            if(mRoadSegments[i].mPointA == mRoadSegments[i].mPointB)
            {
               mRoadSegments[i].mPointA.removeRoadSegmentUsingMe(mRoadSegments[i]);
               mRoadSegments.RemoveAt(i);
               i--;
            }
         }

         mSelectedControlPoints.Clear();
      }
      public bool isSelectedPointRaycast(Vector3 rayOrig, Vector3 rayDir)
      {
         for (int i = 0; i < mSelectedControlPoints.Count; i++)
         {
            if (mSelectedControlPoints[i].rayCastIntersect(rayOrig, rayDir, mRoadWidth))
               return true;
         }
         return false;
      }
      //--------------------------------
      public void setRoadTexture(string filename)
      {
         mRoadTextureName = filename;
         string fullPath = CoreGlobals.getWorkPaths().mRoadsPath + "\\" + filename + "_df.tga";
         mTexture = BRenderDevice.getTextureManager().getTexture(fullPath);
         if(mTexture.mTexture==null)
         {
            MessageBox.Show("Road texture " + fullPath  + " not found, displaying default texture for this road.");
            mTexture.mTexture = BRenderDevice.getTextureManager().getDefaultTexture(TextureManager.eDefaultTextures.cDefTex_Green);
         }
      }
      public string getRoadTextureName()
      {
         return mRoadTextureName;
      }
      public void setRoadWidth(float width)
      {
         mRoadWidth = width;

         //for(int i=0;i<mControlPoints.Count;i++)
         //   mControlPoints[mControlPoints.Count - 1].updateVisual(mRoadWidth);
      }
      public float getRoadWidth()
      {
         return mRoadWidth;
      }
      public void setWorldTesselationSize(float sze)
      {
         mIterationWorldLength = sze;
      }
      public float getWorldTesselationSize()
      {
         return mIterationWorldLength;
      }
      public int getNumControlPoints()
      {
         return mControlPoints.Count;
      }
      public int getNumRoadSegments()
      {
         return mRoadSegments.Count;
      }
      //--------------------------------
      
   }
   //---------------------------------------
   //---------------------------------------
   //---------------------------------------
   //---------------------------------------
   public class RoadManager
   {
      static List<Road> mRoads = new List<Road>();

      //Add data
      static bool mLeftMouseDown = false;
      static bool mRightMouseDown = false;
      static bool mLineStarted = false;
      static Point mLineStartPoint = new Point();
      static VertexBuffer mAddModePlacementLine = null;
      static BRenderDebugSphere mPrevPointVisSphere = null;
      static bool mCreateAsContinuousSegment = false;
      static bool mSnapToPrimeAngles = false;

      //selection
      static bool mDragging = false;
      static VertexBuffer m2DSelectionBox = null;
      static Point mPressPoint = new Point();
      static Point mReleasePoint = new Point();

      //movement
      static Point mLastIntersectPoint = new Point();
      static bool mDraggingObjects = false;

      //current states
      static string mCurrRoadTextureName = null;
      static float mCurrRoadWidth = 1.0f;
      static roadControlPoint.eControlPointType mCurrContolType = roadControlPoint.eControlPointType.cAngled;
      static int mCurrRoadIndex = -1;

      

      //---------------------------------------
      ~RoadManager()
      {
         destroy();
      }
      //---------------------------------------
      static public void destroy()
      {
         for (int i = 0; i < mRoads.Count; i++)
            deleteRoad(i);

         mRoads.Clear();
      }
      static public void input(BTerrainEditor.eEditorMode mode)
      {
         bool left = UIManager.GetMouseButtonDown(UIManager.eMouseButton.cLeft);
         bool right = UIManager.GetMouseButtonDown(UIManager.eMouseButton.cRight);
         bool bMouseMoved = UIManager.Moved(0);
         bool bClickedLeft = (!left && mLeftMouseDown);
         bool bClickedRight = (!right && mRightMouseDown);
         bool bDraggingLeft = (left && mLeftMouseDown);
         
         if(mode == BTerrainEditor.eEditorMode.cModeRoadAdd)
         {
            if (bClickedRight)
            {
               mLineStarted = false;
            }
            else if (bClickedLeft)
            {
               //we're starting!
               if (mLineStarted == false)
               {
                  mLineStarted = true;
                  mLineStartPoint.X = TerrainGlobals.getEditor().mVisTileIntetersectionX;
                  mLineStartPoint.Y = TerrainGlobals.getEditor().mVisTileIntetersectionZ;
               }
               else
               {
                  Point b = new Point();
                  b.X = TerrainGlobals.getEditor().mVisTileIntetersectionX;
                  b.Y = TerrainGlobals.getEditor().mVisTileIntetersectionZ;
                  if (mSnapToPrimeAngles)
                     b = capLockMovementToPrimeAngles(mLineStartPoint, b);

                  addSegment(mLineStartPoint, b);

                  if (mCreateAsContinuousSegment)
                     mLineStartPoint = b;
                  else
                     mLineStarted = false;
               }
            }

            if (mLineStarted)
               updateAddLine();

         }

         if (mode == BTerrainEditor.eEditorMode.cModeRoadEdit)
         {
            if (UIManager.GetAsyncKeyStateB(Key.Delete))
            {
               deleteSelectedPoints();
               CoreGlobals.getEditorMain().mIGUI.roadSelectionCallback();
            }

            //SELECT MODE (right mouse button)
            UIManager.GetCursorPos(ref mReleasePoint);
            bDraggingLeft &= mReleasePoint != mPressPoint;
            if (left && !mLeftMouseDown)
            {
               UIManager.GetCursorPos(ref mPressPoint);
               //did we click on a selected object?
               if(isSelectedPointRaycast())
               {
                  mLastIntersectPoint.X = TerrainGlobals.getEditor().mVisTileIntetersectionX;
                  mLastIntersectPoint.Y = TerrainGlobals.getEditor().mVisTileIntetersectionZ;
                  mDraggingObjects = true;
               }
            }
            else if (bClickedLeft)  //The mouse has been released
            {
               UIManager.GetCursorPos(ref mReleasePoint);

               if (mDraggingObjects)
               {

               }
               else
               {
                  if (mDragging)
                     selectControlPointsDragBox(UIManager.GetAsyncKeyStateB(Key.LeftShift));
                  else
                     selectControlPointsRayCast(UIManager.GetAsyncKeyStateB(Key.LeftShift));

                  CoreGlobals.getEditorMain().mIGUI.roadSelectionCallback();
               }
               mDraggingObjects = false;
               mDragging = false;
            }
            else if (bDraggingLeft) 
            {
               if (mDraggingObjects)
               {
                  moveSelectedPoints();
                  mLastIntersectPoint.X = TerrainGlobals.getEditor().mVisTileIntetersectionX;
                  mLastIntersectPoint.Y = TerrainGlobals.getEditor().mVisTileIntetersectionZ;
                  UIManager.GetCursorPos(ref mPressPoint);
               }
               else
               {
                  mDragging = true;
                  update2DSelectionBox();
               }
            }
         }

         
         //mark our current state for next frame
         mLeftMouseDown = left;
         mRightMouseDown = right;
      }
      static public void update()
      {
      }
      static public void render(bool renderDebugs)
      {
         BRenderDevice.getDevice().VertexShader = null;
         BRenderDevice.getDevice().PixelShader = null;
         BRenderDevice.getDevice().SetRenderState(RenderStates.CullMode, (int)Cull.None);
       //  BRenderDevice.getDevice().SetRenderState(RenderStates.ZEnable, false);


         BRenderDevice.getDevice().SetRenderState(RenderStates.AlphaBlendEnable, true);
         BRenderDevice.getDevice().SetRenderState(RenderStates.SourceBlend, (int)Blend.SourceAlpha);
         BRenderDevice.getDevice().SetRenderState(RenderStates.DestinationBlend, (int)Blend.InvSourceAlpha);

         for(int i=0;i<mRoads.Count;i++)
         {
            mRoads[i].render(renderDebugs);
         }
         BRenderDevice.getDevice().SetRenderState(RenderStates.AlphaBlendEnable, false);
         BRenderDevice.getDevice().Transform.World = Matrix.Identity;
         BRenderDevice.getDevice().SetRenderState(RenderStates.CullMode, (int)Cull.CounterClockwise);
        // BRenderDevice.getDevice().SetRenderState(RenderStates.ZEnable, true);

         renderAddLine();

         render2DSelectionBox();
      }
      //-----------------------------
      static void updateAddLine()
      {
         if (!mLineStarted)
            return;

         if(mPrevPointVisSphere==null)
            mPrevPointVisSphere = new BRenderDebugSphere(mCurrRoadWidth, 2, roadControlPoint.cTypeColors[(int)mCurrContolType]);

         if (mAddModePlacementLine == null)
            mAddModePlacementLine = new VertexBuffer(typeof(VertexTypes.Pos_Color), 2, BRenderDevice.getDevice(), Usage.None, VertexTypes.Pos_Color.FVF_Flags, Pool.Managed);

         Vector3 startPt = TerrainGlobals.getTerrain().getPostDeformPos(mLineStartPoint.X, mLineStartPoint.Y);
         Vector3 endPt = TerrainGlobals.getEditor().mBrushIntersectionPoint;

         if(mSnapToPrimeAngles)
         {
            Point kp = new Point( mLastIntersectPoint.X = TerrainGlobals.getEditor().mVisTileIntetersectionX,mLastIntersectPoint.Y = TerrainGlobals.getEditor().mVisTileIntetersectionZ);
            Point pt = capLockMovementToPrimeAngles(mLineStartPoint, kp);
            endPt = TerrainGlobals.getTerrain().getPostDeformPos(pt.X, pt.Y);
         }

         //do a 'preview' : see if we're going to snap to a point
         if (mCurrRoadIndex != -1)
         {
            int existIndex = mRoads[mCurrRoadIndex].giveControlPointIntersect2D(TerrainGlobals.getEditor().mVisTileIntetersectionX, TerrainGlobals.getEditor().mVisTileIntetersectionZ);
            if (existIndex != -1)
               endPt = TerrainGlobals.getTerrain().getPostDeformPos(mRoads[mCurrRoadIndex].getPoint(existIndex).mGridX, mRoads[mCurrRoadIndex].getPoint(existIndex).mGridY);
         }
         


         VertexTypes.Pos_Color[] verts = new VertexTypes.Pos_Color[]
         {
            new VertexTypes.Pos_Color(startPt.X,startPt.Y,startPt.Z,System.Drawing.Color.Yellow.ToArgb()),
            new VertexTypes.Pos_Color(endPt.X,endPt.Y,endPt.Z,System.Drawing.Color.Yellow.ToArgb()),
         };

         GraphicsStream gStream = mAddModePlacementLine.Lock(0, 0, LockFlags.None);
         gStream.Write(verts);
         mAddModePlacementLine.Unlock();
         verts = null;

      }
      static private void renderAddLine()
      {
         if (mLineStarted)
         {
            //draw sphere
            BRenderDevice.getDevice().Transform.World = Matrix.Translation(TerrainGlobals.getTerrain().getPostDeformPos(mLineStartPoint.X, mLineStartPoint.Y));
            mPrevPointVisSphere.render(true, false);
            BRenderDevice.getDevice().Transform.World = Matrix.Identity;

            //draw line
            BRenderDevice.getDevice().VertexShader = null;
            BRenderDevice.getDevice().PixelShader = null;

            BRenderDevice.getDevice().SetTexture(0, null);
            BRenderDevice.getDevice().VertexDeclaration = VertexTypes.Pos_Color.vertDecl;
            BRenderDevice.getDevice().SetStreamSource(0, mAddModePlacementLine, 0);
            BRenderDevice.getDevice().DrawPrimitives(PrimitiveType.LineList, 0, 1);


         }
      }
      //-----------------------------
      static bool checkMinDist(Point a, Point b)
      {
         int c = a.X - b.X;
         int d = a.Y - b.Y;
         double f = Math.Sqrt(c*c + d*d);

         return f> 2;
      }
      //-----------------------------
      static public void addSegment(Point a, Point b)
      {
         if (!checkMinDist(a, b))
            return;

         mCurrRoadIndex = giveRoadIndex(mCurrRoadTextureName);
         if (mCurrRoadIndex == -1)
         {
            mCurrRoadIndex = addNewRoad(mCurrRoadTextureName, mCurrRoadWidth);
         }
         mLineStarted = !mRoads[mCurrRoadIndex].addSegment(a, b, mCurrContolType);

         mRoads[mCurrRoadIndex].rebuildVisuals();
      }
      static public int addNewRoad(string roadTextureName, float roadWidth)
      {
         mRoads.Add(new Road(roadTextureName, roadWidth));
         return mRoads.Count - 1;
      }
      static public void deleteRoad(int roadIndex)
      {
         mRoads[roadIndex].destroy();
         mRoads.RemoveAt(roadIndex);
      }
      //-----------------------------
      static public void setRoadTexture(string fname)
      {
         mCurrRoadTextureName = fname;
         mCurrRoadIndex = giveRoadIndex(mCurrRoadTextureName);
      }
      static public int giveRoadIndex(string fname)
      {
         for(int i=0;i<mRoads.Count;i++)
         {
            if (mRoads[i].getRoadTextureName() == fname)
               return i;
         }
         return -1;
      }
      static public Road giveRoad(int roadIndex)
      {
         if (roadIndex < 0 || roadIndex >= mRoads.Count)
            return null;

         return mRoads[roadIndex];
      }
      static public int giveNumRoads()
      {
         return mRoads.Count;
      }
      //-----------------------------
      static void selectControlPointsRayCast(bool addToList)
      {
         Vector3 orig = TerrainGlobals.getEditor().getRayPosFromMouseCoords(false);
         Vector3 dir = TerrainGlobals.getEditor().getRayPosFromMouseCoords(true) - orig;
         dir = BMathLib.Normalize(dir);

         for (int i = 0; i < mRoads.Count; i++)
         {
            if (!addToList)
               mRoads[i].clearSelectedPoints();

            mRoads[i].selectPointsRayCast(orig, dir, !addToList);
         }
      }
      static double Distance(Point A, Point B)
      {
         double val = ((A.X - B.X) * (A.X - B.X)) + ((A.Y - B.Y) * (A.Y - B.Y));
         return Math.Sqrt(val);
      }
      static double Area(Point A, Point B)
      {
         double val = Math.Abs(A.X - B.X) * Math.Abs(A.Y - B.Y);
         return val;
      }
      static bool IsGoodBox(Point A, Point B)
      {
         double distance = Distance(A, B);
         double area = Area(A, B);
         //double ratio = Ratio(A, B);
         if ((distance > 20) && (area > 10))// && (ratio < 100))
            return true;
         else
            return false;
      }
      static void selectControlPointsDragBox(bool addToList)
      {
         if (!IsGoodBox(mPressPoint, mReleasePoint))
         {
            return;
         }

         Vector3[] points = new Vector3[8];

         Point min = mPressPoint;
         Point max = mReleasePoint;

         if (min.X > max.X)
         {
            min.X = max.X;
            max.X = mPressPoint.X;
         }
         if (min.Y > max.Y)
         {
            min.Y = max.Y;
            max.Y = mPressPoint.Y;
         }

         Point mid0 = new Point();
         Point mid1 = new Point();
         mid1.X = min.X;
         mid1.Y = max.Y;
         mid0.X = max.X;
         mid0.Y = min.Y;


         //TODO : this sucks. VERY ineffecient. We should only be doing 4 unprojects, and interpolating the rest
         //we're currently doing 8..
         points[0] = BRenderDevice.getRayPosFromMouseCoords(false, min);
         points[4] = BRenderDevice.getRayPosFromMouseCoords(true, min);

         points[1] = BRenderDevice.getRayPosFromMouseCoords(false, mid0);
         points[5] = BRenderDevice.getRayPosFromMouseCoords(true, mid0);

         points[2] = BRenderDevice.getRayPosFromMouseCoords(false, mid1);
         points[6] = BRenderDevice.getRayPosFromMouseCoords(true, mid1);

         points[3] = BRenderDevice.getRayPosFromMouseCoords(false, max);
         points[7] = BRenderDevice.getRayPosFromMouseCoords(true, max);

         
        //pass points off to each sub road
         for (int i = 0; i < mRoads.Count; i++)
         {
            if (!addToList)
               mRoads[i].clearSelectedPoints();

            mRoads[i].selectPointsDrag( points, !addToList);
         }

      }
      public static void clearSelectedPoints()
      {
         mLineStarted = false;
         for (int i = 0; i < mRoads.Count; i++)
            mRoads[i].clearSelectedPoints();
      }
      public static void deleteSelectedPoints()
      {
         for (int i = 0; i < mRoads.Count; i++)
         {
            mRoads[i].removeSelectedPoints();
            if (mRoads[i].getNumControlPoints() == 0)
            {
               deleteRoad(i);
               i--;
            }
         }
      }
      static bool isSelectedPointRaycast()
      {
         Vector3 orig = TerrainGlobals.getEditor().getRayPosFromMouseCoords(false);
         Vector3 dir = TerrainGlobals.getEditor().getRayPosFromMouseCoords(true) - orig;
         dir = BMathLib.Normalize(dir);

         for (int i = 0; i < mRoads.Count; i++)
         {
            if (mRoads[i].isSelectedPointRaycast(orig, dir))
               return true;
         }
         return false;
      }
      //------------------------------
      static void update2DSelectionBox()
      {
         Point tPt = new Point();
         Point kPt = mPressPoint;
         UIManager.GetCursorPos(ref tPt);
         BRenderDevice.getScreenToD3DCoords(ref tPt);
         BRenderDevice.getScreenToD3DCoords(ref kPt);

         if (m2DSelectionBox == null)
            m2DSelectionBox = new VertexBuffer(typeof(VertexTypes.PosW_color), 8, BRenderDevice.getDevice(), Usage.None, VertexTypes.PosW_color.FVF_Flags, Pool.Managed);

         VertexTypes.PosW_color[] verts = new VertexTypes.PosW_color[]
         {
            new VertexTypes.PosW_color(kPt.X,kPt.Y,0,1,System.Drawing.Color.GreenYellow.ToArgb()),
            new VertexTypes.PosW_color(tPt.X,kPt.Y,0,1,System.Drawing.Color.GreenYellow.ToArgb()),

            new VertexTypes.PosW_color(tPt.X,kPt.Y,0,1,System.Drawing.Color.GreenYellow.ToArgb()),
            new VertexTypes.PosW_color(tPt.X,tPt.Y,0,1,System.Drawing.Color.GreenYellow.ToArgb()),

            new VertexTypes.PosW_color(tPt.X,tPt.Y,0,1,System.Drawing.Color.GreenYellow.ToArgb()),
            new VertexTypes.PosW_color(kPt.X,tPt.Y,0,1,System.Drawing.Color.GreenYellow.ToArgb()),

            new VertexTypes.PosW_color(kPt.X,tPt.Y,0,1,System.Drawing.Color.GreenYellow.ToArgb()),
            new VertexTypes.PosW_color(kPt.X,kPt.Y,0,1,System.Drawing.Color.GreenYellow.ToArgb()),
         };

         GraphicsStream gStream = m2DSelectionBox.Lock(0, 0, LockFlags.None);
         gStream.Write(verts);
         m2DSelectionBox.Unlock();
         verts = null;

      }
      static void render2DSelectionBox()
      {
         if (mDragging)
         {
            BRenderDevice.getDevice().VertexShader = null;
            BRenderDevice.getDevice().PixelShader = null;

            BRenderDevice.getDevice().SetTexture(0, null);
            BRenderDevice.getDevice().VertexDeclaration = VertexTypes.PosW_color.vertDecl;
            BRenderDevice.getDevice().SetStreamSource(0, m2DSelectionBox, 0);
            BRenderDevice.getDevice().DrawPrimitives(PrimitiveType.LineList, 0, 4);
         }
      }
      //------------------------------
      static public void setContinuousCreationMode(bool onOff)
      {
         mCreateAsContinuousSegment = onOff;
      }
      static public void setSnapToPrimeMode(bool onOff)
      {
         mSnapToPrimeAngles = onOff;
      }
      static public void setCurrentAddModeType(roadControlPoint.eControlPointType type)
      {
         mCurrContolType = type;
         if (mPrevPointVisSphere != null)
            mPrevPointVisSphere.destroy();

            mPrevPointVisSphere = new BRenderDebugSphere(0.5f, 2, roadControlPoint.cTypeColors[(int)mCurrContolType]);
      }
      //------------------------------
      static Point capLockMovementToPrimeAngles(Point anchor, Point mover)
      { 
         //NOTE although we're going to VEC3s we're still in GRID SPACE
         //do angle checkes between NORTH
         Vector3 north = BMathLib.unitZ;
         Vector3 east = BMathLib.unitX;

         Vector3 root = new Vector3(anchor.X, 0, anchor.Y);
         Vector3 leaf = new Vector3(mover.X, 0, mover.Y);
         Vector3 diff = leaf - root;
         float len = diff.Length();
         diff.Normalize();

         float angle = BMathLib.vec2vec2Angle_0to360(BMathLib.vec3tovec2XZPlane(north), BMathLib.vec3tovec2XZPlane(diff));

         Vector3 clampedPoint = new Vector3(0, 0, 1);

         //UGGG Shouldn't be done by hand!! set angle derivation by 22.5deg
         if (angle >= 337.5 || angle < 22.5f)            //0/360  
            clampedPoint = new Vector3(0, 0, 1);

         else if (angle >= 22.5f && angle < 67.5f)       //45
            clampedPoint = new Vector3(1, 0, 1);
         else if (angle >= 67.5f && angle < 112.5f)      //90
            clampedPoint = new Vector3(1, 0, 0);
         else if (angle >= 112.5f && angle < 157.5f)     //135
            clampedPoint = new Vector3(1, 0, -1);

         else if (angle >= 157.5f && angle < 202.5f)     //180
            clampedPoint = new Vector3(0, 0, -1);

         else if (angle >= 202.5f && angle < 247.5f)     
            clampedPoint = new Vector3(-1, 0, -1);
         else if (angle >= 247.5f && angle < 292.5f)     //270
            clampedPoint = new Vector3(-1, 0, 0);
         else if (angle >= 292.5f && angle < 337.5f)     
            clampedPoint = new Vector3(-1, 0, 1);

         clampedPoint = BMathLib.Normalize(clampedPoint) * len;

         //relocate this new vector
         //CLM THIS HAS ALIGNMENT ISSUES!! (but it gets them close enough...)
         Point res = new Point((int)(anchor.X + clampedPoint.X), (int)(anchor.Y + clampedPoint.Z));
         return res;
      }
      static void moveSelectedPoints()
      {

         //get our current intersect tiles
         Point currIntersectTile = new Point();
         currIntersectTile.X = TerrainGlobals.getEditor().mVisTileIntetersectionX;
         currIntersectTile.Y = TerrainGlobals.getEditor().mVisTileIntetersectionZ;

         //get the difference
         Point diffTiles = new Point();
         diffTiles.X = currIntersectTile.X - mLastIntersectPoint.X;
         diffTiles.Y = currIntersectTile.Y - mLastIntersectPoint.Y;
        
         //now, move the objects
         for(int i=0;i<mRoads.Count;i++)
         {
            mRoads[i].moveSelectedPoints(diffTiles.X, diffTiles.Y);
            mRoads[i].rebuildVisuals();
         }
      }
      static public int getNumSelectedPoints()
      {
         int numPoints = 0;
         for (int i = 0; i < mRoads.Count; i++)
         {
            numPoints += mRoads[i].getNumSelectedpoints();
         }
         return numPoints;
      }
      static public int getNumSelectedRoads()
      {
         int numRoads = 0;
         for (int i = 0; i < mRoads.Count; i++)
         {
            if (mRoads[i].getNumSelectedpoints() > 0)
               numRoads++;
         }
         return numRoads;
      }
      static public Road getSelectedRoad(int index)
      {
         if (index < 0)
            return null;

         int numRoads = 0;
         for (int i = 0; i < mRoads.Count; i++)
         {
            if (mRoads[i].getNumSelectedpoints() > 0)
            {
               if (numRoads == index)
                  return mRoads[i];
               numRoads++;
            }
         }
         return null;
      }
      public static void changeSelectedPointType(roadControlPoint.eControlPointType type)
      {
         for(int i=0;i<mRoads.Count;i++)
         {
            mRoads[i].changeSelectedPointType(type);
            mRoads[i].rebuildVisuals();
         }
      }
      public static void changeSelectedRoadWidth(float width)
      {
         for (int i = 0; i < mRoads.Count; i++)
         {
            if (mRoads[i].getNumSelectedpoints() > 0)
            {
               mRoads[i].setRoadWidth(width);
            }
         }
      }
      public static void changeSelectedRoadWorldTesselation(float tessSize)
      {
         for (int i = 0; i < mRoads.Count; i++)
         {
            if (mRoads[i].getNumSelectedpoints() > 0)
            {
               mRoads[i].setWorldTesselationSize(tessSize);
            }
         }
      }
      public static void rebuildSelectedRoadVisuals()
      {
         for (int i = 0; i < mRoads.Count; i++)
         {
            if (mRoads[i].getNumSelectedpoints() > 0)
            {
               mRoads[i].rebuildVisuals();
            }
         }
      }
      public static void splitSelectedPoints()
      {
         for (int i = 0; i < mRoads.Count; i++)
         {
            if (mRoads[i].getNumSelectedpoints() > 0)
            {
               mRoads[i].splitSelectedPoints();
               mRoads[i].rebuildVisuals();
            }
         }
      }
      public static void collapseSelectedPoints()
      {
         for (int i = 0; i < mRoads.Count; i++)
         {
            if (mRoads[i].getNumSelectedpoints() > 0)
            {
               mRoads[i].collapseSelectedPoints();
               mRoads[i].rebuildVisuals();
            }
         }
      }
      public static void snapSelectedRoadToTerrain()
      {
         for (int i = 0; i < mRoads.Count; i++)
         {
            if (mRoads[i].getNumSelectedpoints() > 0)
            {
               mRoads[i].rebuildVisuals(true);
            }
         }
      }
      public static void smoothSelectedRoadTerrain()
      {
         for (int i = 0; i < mRoads.Count; i++)
         {
            if (mRoads[i].getNumSelectedpoints() > 0)
            {
            }
         }
      }
      public static void maskSelectedRoadTerrain()
      {
         for (int i = 0; i < mRoads.Count; i++)
         {
            if (mRoads[i].getNumSelectedpoints() > 0)
            {
            }
         }
      }
      //------------------------------

   }
}