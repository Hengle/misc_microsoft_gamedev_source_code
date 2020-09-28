using System;
using System.IO;
using System.Drawing;
using System.Collections.Generic;
using System.Text;
using System.Windows.Forms;

using Microsoft.DirectX;
using Microsoft.DirectX.Direct3D;
using System.Collections;

////
//---------------------------------------
namespace EditorCore
{


   #region Frustrum
   public class BFrustum
   {
      //Microsoft.DirectX.
      Plane[] m_planes = new Plane[6];

      public void update(Microsoft.DirectX.Direct3D.Device device)
      {
         Matrix g_matView;
         Matrix g_matProj;
         Matrix res;

         g_matView = device.GetTransform(TransformType.View);
         g_matProj = device.GetTransform(TransformType.Projection);
         res = Matrix.Multiply(g_matView, g_matProj);

         // left
         m_planes[0].A = res.M14 + res.M11;
         m_planes[0].B = res.M24 + res.M21;
         m_planes[0].C = res.M34 + res.M31;
         m_planes[0].D = res.M44 + res.M41;

         // right
         m_planes[1].A = res.M14 - res.M11;
         m_planes[1].B = res.M24 - res.M21;
         m_planes[1].C = res.M34 - res.M31;
         m_planes[1].D = res.M44 - res.M41;

         // top
         m_planes[2].A = res.M14 - res.M12;
         m_planes[2].B = res.M24 - res.M22;
         m_planes[2].C = res.M34 - res.M32;
         m_planes[2].D = res.M44 - res.M42;

         // bottom
         m_planes[3].A = res.M14 + res.M12;
         m_planes[3].B = res.M24 + res.M22;
         m_planes[3].C = res.M34 + res.M32;
         m_planes[3].D = res.M44 + res.M42;

         // near
         m_planes[4].A = res.M13;
         m_planes[4].B = res.M23;
         m_planes[4].C = res.M33;
         m_planes[4].D = res.M43;

         // far
         m_planes[5].A = res.M14 - res.M13;
         m_planes[5].B = res.M24 - res.M23;
         m_planes[5].C = res.M34 - res.M33;
         m_planes[5].D = res.M44 - res.M43;

         for (int i = 0; i < 6; i++)
         {
            m_planes[i].Normalize();
         }




      }
      public bool AABBVisible(Vector3 min, Vector3 max)
      {
         Vector3 c = Vector3.Multiply(min + max, 0.5f);
         Vector3 d = max - c;
         //D3DXVECTOR3 c = (D3DXVECTOR3((float*)(&max)) + D3DXVECTOR3((float*)(&min))) / 2.f;
         //D3DXVECTOR3 d = D3DXVECTOR3((float*)(&max)) - c;    // half-diagonal
         //Vector3 c = Vector3.Multiply((new Vector3(max.X,max.X,max.X)) + (new Vector3(min.X,min.X,min.X)) , 0.5f);
         //Vector3 d = new Vector3(max.X,max.X,max.X) - c;    // half-diagonal

         for (int i = 0; i < 6; i++)
         {
            Vector3 p;
            p.X = m_planes[i].A;//[0];
            p.Y = m_planes[i].B;//[1];
            p.Z = m_planes[i].C;//[2];

            float NP = (float)(d.X * Math.Abs(p.X) + d.Y * Math.Abs(p.Y) + d.Z * Math.Abs(p.Z));

            float MP = c.X * p.X + c.Y * p.Y + c.Z * p.Z + m_planes[i].D;//[3];

            if ((MP + NP) < 0.0f) return false; // behind clip plane
         }

         return true;
      }
      public bool SphereVisible(Vector3 center, float rad)
      {
         // various distances
         float fDistance;

         // calculate our distances to each of the planes
         for (int i = 0; i < 6; ++i)
         {
            // find the distance to this plane
            fDistance = m_planes[i].A * center.X + m_planes[i].B * center.Y + m_planes[i].C * center.Z + m_planes[i].D;

            // if this distance is < -sphere.radius, we are outside
            if (fDistance < -rad)
               return (false);

            // else if the distance is between +- radius, then we intersect
            if ((float)Math.Abs(fDistance) < rad)
               return (true);
         }

         // otherwise we are fully in view
         return (true);
      }



   }
   #endregion



   #region BoundingBox
   public class BBoundingBox
   {
      public BBoundingBox()
      {
         empty();
      }
      public BBoundingBox(Vector3 _min, Vector3 _max)
      {
         min = _min;
         max = _max;
      }

      public Vector3 min;
      public Vector3 max;

      public void addPoint(Vector3 point)
      {
         addPoint(point.X, point.Y, point.Z);
      }
      public void addPoint(float x, float y, float z)
      {
         if (x < min.X)
            min.X = x;
         if (y < min.Y)
            min.Y = y;
         if (z < min.Z)
            min.Z =z;

         if (x > max.X)
            max.X = x;
         if (y > max.Y)
            max.Y = y;
         if (z > max.Z)
            max.Z = z;
      }
      public void addBox(BBoundingBox box)
      {
         if (box.min.X < min.X)
            min.X = box.min.X;
         if (box.min.Y < min.Y)
            min.Y = box.min.Y;
         if (box.min.Z < min.Z)
            min.Z = box.min.Z;

         if (box.max.X > max.X)
            max.X = box.max.X;
         if (box.max.Y > max.Y)
            max.Y = box.max.Y;
         if (box.max.Z > max.Z)
            max.Z = box.max.Z;
      }

      public Vector3 getCenter()
      {
         Vector3 center;
         center = (max - min);
         center.Scale(0.5f);
         center.Add(min);

         return (center);
      }

      public enum eCornerIndex
      {
         cMinX_MinY_MinZ =0,
         cMinX_MinY_MaxZ = 1,
         cMinX_MaxY_MinZ = 2,
         cMinX_MaxY_MaxZ = 3,
         cMaxX_MinY_MinZ = 4,
         cMaxX_MinY_MaxZ = 5,
         cMaxX_MaxY_MinZ = 6,
         cMaxX_MaxY_MaxZ = 7,
      }
      public Vector3 getCorner(eCornerIndex cornerIndex)
      {
         switch(cornerIndex)
         {
            case eCornerIndex.cMinX_MinY_MinZ:
               return new Vector3(min.X, min.Y, min.Z);
               break;
            case eCornerIndex.cMinX_MinY_MaxZ:
               return new Vector3(min.X, min.Y, max.Z);
               break;
            case eCornerIndex.cMinX_MaxY_MinZ:
               return new Vector3(min.X, max.Y, min.Z);
               break;
            case eCornerIndex.cMinX_MaxY_MaxZ:
               return new Vector3(min.X, max.Y, max.Z);
               break;
            case eCornerIndex.cMaxX_MinY_MinZ:
               return new Vector3(max.X, min.Y, min.Z);
               break;
            case eCornerIndex.cMaxX_MinY_MaxZ:
               return new Vector3(max.X, min.Y, max.Z);
               break;
            case eCornerIndex.cMaxX_MaxY_MinZ:
               return new Vector3(max.X, max.Y, min.Z);
               break;
            case eCornerIndex.cMaxX_MaxY_MaxZ:
               return new Vector3(max.X, max.Y, max.Z);
               break;
         }
         return Vector3.Empty;
      }

      public float getMaxSize()
      {
         float sizeX = max.X - min.X;
         float sizeY = max.Y - min.Y;
         float sizeZ = max.Z - min.Z;

         float maxSize = float.MinValue;
         maxSize = Math.Max(maxSize, sizeX);
         maxSize = Math.Max(maxSize, sizeY);
         maxSize = Math.Max(maxSize, sizeZ);

         return (maxSize);
      }

      public void empty()
      {
         min = new Vector3(float.MaxValue, float.MaxValue, float.MaxValue);
         max = new Vector3(float.MinValue, float.MinValue, float.MinValue);
      }

      public bool intersect(BBoundingBox other)
      {
         return BMathLib.aabbsIntersect(ref min, ref max, ref other.min, ref other.max);
      }
      public bool intersect(Vector3 rayOrig, Vector3 rayDir)
      {
         Vector3 coord = Vector3.Empty;
         float t = 0;
         return BMathLib.ray3AABB(ref coord, ref t, ref rayOrig, ref rayDir, ref min, ref max);
         
      }

      public bool isEmpty()
      {
         if ((max.X < min.X) || (max.Y < min.Y) || (max.Z < min.Z))
            return true;
         else
            return false;
      }
   }

   public class BOrientedBoundingBox
   {
      public Vector3 mCenter;
      public Vector3 mExtents;
      public Quaternion mOrientation;

      public void constructFromAABB(Vector3 min, Vector3 max, Matrix orientation)
      {
         mOrientation = Quaternion.RotationMatrix(orientation);
         mCenter = Vector3.Multiply((min + max), 0.5f);
         mExtents = Vector3.Multiply((max - min), 0.5f);
      }

      public void construct(Vector3 min, Vector3 max, Matrix matrix)
      {
         mExtents = Vector3.Multiply((max - min), 0.5f);

         Vector3 vecX = new Vector3(1.0f, 0.0f, 0.0f);
         Vector3 vecY = new Vector3(0.0f, 1.0f, 0.0f);
         Vector3 vecZ = new Vector3(0.0f, 0.0f, 1.0f);
         vecX.TransformNormal(matrix);
         vecY.TransformNormal(matrix);
         vecZ.TransformNormal(matrix);
         mExtents.X *= vecX.Length();
         mExtents.Y *= vecY.Length();
         mExtents.Z *= vecZ.Length();

         mCenter = Vector3.Multiply((min + max), 0.5f);
         mCenter.TransformCoordinate(matrix);

         vecX.Normalize();
         vecY.Normalize();
         vecZ.Normalize();
         Matrix matrix3 = Matrix.Identity;

         matrix3.M11 = vecX.X; matrix3.M12 = vecX.Y; matrix3.M13 = vecX.Z;
         matrix3.M21 = vecY.X; matrix3.M22 = vecY.Y; matrix3.M23 = vecY.Z;
         matrix3.M31 = vecZ.X; matrix3.M32 = vecZ.Y; matrix3.M33 = vecZ.Z;

         mOrientation = Quaternion.RotationMatrix(matrix3);

         Matrix matrix2 = Matrix.RotationQuaternion(mOrientation);

         //mOrientation.Normalize();
      }

      public void constructFromDirExtents(Vector3 dir, Vector3 extents, Vector3 center)
      {
         mOrientation = Quaternion.RotationMatrix(BMathLib.makeRotateMatrix(new Vector3(0,1,0),dir));
         mCenter = center;
         mExtents = extents;
      }
      public void constructFromPoints(List<Vector3> points)
      {
         if(points.Count==0)
            return;
          
          Vector3 CenterOfMass = Vector3.Empty;
          
          // Compute the center of mass and inertia tensor of the points.
          for (int i = 0; i < points.Count; i++)
             CenterOfMass += points[i];

          CenterOfMass= Vector3.Multiply(CenterOfMass, 1.0f / (float)points.Count);

	      // Compute the inertia tensor of the points around the center of mass.
	      // Using the center of mass is not strictly necessary, but will hopefully
	      // improve the stability of finding the eigenvectors.
          Vector3 XXYYZZ = Vector3.Empty;
          Vector3 XYXZ_YZ = Vector3.Empty;

          for (int i = 0; i < points.Count; i++)
          {
             Vector3 Point = CenterOfMass - points[i];

             XXYYZZ = XXYYZZ + BMathLib.Vec3Mul(Point, Point);

              Vector3 XXY = new Vector3(Point.X, Point.X, Point.Y);
              Vector3 YZZ = new Vector3(Point.Y, Point.Z, Point.Z);

              XYXZ_YZ = XYXZ_YZ + BMathLib.Vec3Mul(XXY, YZZ);
          }
          
          Vector3 v1 = Vector3.Empty;
          Vector3 v2 = Vector3.Empty;
          Vector3 v3 = Vector3.Empty;
          
          // Compute the eigenvectors of the inertia tensor.
          BMathLib.CalculateEigenVectorsFromCovarianceMatrix( XXYYZZ.X, XXYYZZ.Y, XXYYZZ.Z,
                                                     XYXZ_YZ.X, XYXZ_YZ.Y, XYXZ_YZ.Z,
                                                     ref v1, ref v2, ref v3 );
          
          // Put them in a matrix.
          Matrix R = Matrix.Identity;
          BMathLib.setRow(ref R, 0, v1);
          BMathLib.setRow(ref R, 1, v2);
          BMathLib.setRow(ref R, 2, v3);
          BMathLib.setRow(ref R, 4, Vector3.Empty);
         
          

          // Multiply by -1 to convert the matrix into a right handed coordinate 
          // system (Det ~= 1) in case the eigenvectors form a left handed 
          // coordinate system (Det ~= -1) because XMQuaternionRotationMatrix only 
          // works on right handed matrices.
          float Det =  R.Determinant;
        
          if ( Det<0 )
          {
             BMathLib.setRow(ref R, 0, -BMathLib.getRow(R, 0));
             BMathLib.setRow(ref R, 1, -BMathLib.getRow(R, 1));
             BMathLib.setRow(ref R, 2, -BMathLib.getRow(R, 2));
          }
          
          // Get the rotation quaternion from the matrix.
         Quaternion Orientation = Quaternion.RotationMatrix(R);
         
          // Make sure it is normal (in case the vectors are slightly non-orthogonal).
          Orientation.Normalize();

          // Rebuild the rotation matrix from the quaternion.
          R = Matrix.RotationQuaternion( Orientation );

          // Build the rotation into the rotated space.
          Matrix InverseR = Matrix.TransposeMatrix(R);
          
          // Find the minimum OBB using the eigenvectors as the axes.
          Vector3 vMin, vMax;
          
          vMin = vMax = Vector3.TransformNormal(points[0],InverseR);

          for (int i = 1; i < points.Count; i++)
          {
              Vector3 Point = Vector3.TransformNormal( points[i], InverseR );
              
              vMin = Vector3.Minimize( vMin, Point );
              vMax = Vector3.Maximize( vMax, Point );
          }
          
          // Rotate the center into world space.
          Vector3 Center = (vMin + vMax) * 0.5f;
          Center = Vector3.TransformNormal( Center, R );

          
          // Store center, extents, and orientation.
          mCenter= Center;
          mExtents= (vMax - vMin) * 0.5f ;
          mOrientation = Orientation ;

      }

      public void computeAABB(ref BBoundingBox box)
      {
         Vector3[] corners = new Vector3[8];
         computeCorners(corners);

         // empty first
         box.empty();

         // now fill
         for (int i = 0; i < 8; i++ )
            box.addPoint(corners[i]);
      }

      private void computeCorners(Vector3[] corners)
      {
         Matrix matrix = Matrix.RotationQuaternion(mOrientation);

         // Prescale axes by extents.
         Vector3[] scaledAxes = new Vector3[3];
         Vector3[] axes = new Vector3[3];

         axes[0].X = matrix.M11; axes[0].Y = matrix.M12; axes[0].Z = matrix.M13;
         axes[1].X = matrix.M21; axes[1].Y = matrix.M22; axes[1].Z = matrix.M23;
         axes[2].X = matrix.M31; axes[2].Y = matrix.M32; axes[2].Z = matrix.M33;

         scaledAxes[0] = mExtents.X * axes[0];
         scaledAxes[1] = mExtents.Y * axes[1];
         scaledAxes[2] = mExtents.Z * axes[2];

         // Compute points.
         corners[0] = mCenter - scaledAxes[0] - scaledAxes[1] - scaledAxes[2];
         corners[1] = mCenter - scaledAxes[0] - scaledAxes[1] + scaledAxes[2];
         corners[2] = mCenter - scaledAxes[0] + scaledAxes[1] - scaledAxes[2];
         corners[3] = mCenter - scaledAxes[0] + scaledAxes[1] + scaledAxes[2];
         corners[4] = mCenter + scaledAxes[0] - scaledAxes[1] - scaledAxes[2];
         corners[5] = mCenter + scaledAxes[0] - scaledAxes[1] + scaledAxes[2];
         corners[6] = mCenter + scaledAxes[0] + scaledAxes[1] - scaledAxes[2];
         corners[7] = mCenter + scaledAxes[0] + scaledAxes[1] + scaledAxes[2];
      }
   }


   public class BTileBoundingBox
   {
      public int minX = Int32.MaxValue;
      public int minZ = Int32.MaxValue;
      public int maxX = 0;
      public int maxZ = 0;

      public void addPoint(int X, int Z)
      {
         if (X < minX)
            minX = X;
         if (Z < minZ)
            minZ = Z;

         if (X > maxX)
            maxX = X;
         if (Z > maxZ)
            maxZ = Z;
      }

      public void addTileBox(BTileBoundingBox other)
      {
         if (other.minX < minX)
            minX = other.minX;
         if (other.minZ < minZ)
            minZ = other.minZ;

         if (other.minX > maxX)
            maxX = other.minX;
         if (other.minZ > maxZ)
            maxZ = other.minZ;
      }

      public bool isPointInside(int x, int z)
      {
         if (x < minX)
            return false;
         if (x > maxX)
            return false;
         if (z < minZ)
            return false;
         if (z > maxZ)
            return false;
         return true;
      }

      public void empty()
      {
         minX = Int32.MaxValue;
         minZ = Int32.MaxValue;
         maxX = -Int32.MaxValue;
         maxZ = -Int32.MaxValue;
      }

      public bool isEmpty()
      {
         if ((maxX < minX) || (maxZ < minZ))
            return true;
         else
            return false;
      }
   }

   #endregion


   #region MathLib

   static public class BMathLib
   {

      static public float cPI = 3.141592654f;
      static public float c2PI = 6.283185307f;
      static public float c1DIVPI = 0.318309886f;
      static public float c1DIV2PI = 0.159154943f;
      static public float cPIDIV2 = 1.570796327f;
      static public float cPIDIV4 = 0.785398163f;


      static public float cNearlyInfinite = 1.0e+37f;

      static public float cFloatCompareEpsilon = 1e-20f;
      static public float cTinyEpsilon = 0.000125f;

      static public bool isPow2(int val)
      {
         return (val & (val - 1)) == 0;
      }
      

      static public bool compare(float input, float valToCompareTo)
      {
         return (Math.Abs(valToCompareTo - input) < cTinyEpsilon);
         
      }
      static public float Clamp(float input, float min, float max)
      {
         if (input < min)
            input = min;
         else if (input > max)
            input = max;

         return (input);
      }
      static public int FloatToIntRound(float f)
      {
         return (int)((f < 0.0f) ? -Math.Floor(-f + .5f) : Math.Floor(f + .5f));
      }
      static public float Saturate(float input)
      {
         input = Clamp(input, 0.0f, 1.0f);

         return (input);
      }
      static public float LERP(float a, float b, float alpha)
      {
         return ((a) + (((b) - (a)) * (alpha)));
      }
      static public Vector3 LERP(Vector3 a, Vector3 b, float alpha)
      {
         return ((a) + (((b) - (a)) * (alpha)));
      }
      static public float areaOfTriangle(ref Vector3 p1, ref Vector3 p2, ref Vector3 p3)
      {
         float halfp;
         float a, b, c;

         a = Vector3.Subtract(p1, p2).Length();
         b = Vector3.Subtract(p2, p3).Length();
         c = Vector3.Subtract(p3, p1).Length();

         halfp = (a + b + c) * 0.5f;

         return (float)Math.Sqrt(halfp * (halfp - a) * (halfp - b) * (halfp - c));
      }
      static public Vector3 centroidOfTriangle(ref Vector3 p1, ref Vector3 p2, ref Vector3 p3)
      {
	      float TotalArea=0;
	      Vector3 Centre= Vector3.Empty;

	      
		      Vector3 MidPoint = (p1 + p2 + p3) * (1.0f/3.0f);
		      float TArea = areaOfTriangle(ref p1 , ref p2 , ref p3);

		      MidPoint*=TArea;		// Wieght current triangle midpoint by area.
		      TotalArea += TArea;
		      Centre    += MidPoint;
	      

         if (TotalArea > BMathLib.cFloatCompareEpsilon) 		// Triangle or greater
         {
            Centre*=(1.0f / TotalArea);
            return Centre;
         }
         else 							// all points coincident or colinear
         {
            Centre = p1 + p2 + p3;

            Centre *= 1.0f/3;

            return MidPoint;// Centre;
         }
      }
      static public  int nexPow2(int value)
      {
         int c = 0;
         while (Math.Pow(2, c) < value)
         {
            c++;
         }
         return (int)Math.Pow(2, c);
      }

      //float16 helpers
      static public float16[] Float32To16Array(float[]array)
      {
         float16[] ar = new float16[array.Length];
         for (int i = 0; i < array.Length; i++)
            ar[i]=new float16(array[i]);

         return ar;
      }
      static public float[] Float16To32Array(float16[] array)
      {
         float[] ar = new float[array.Length];
         for (int i = 0; i < array.Length; i++)
            ar[i] = (array[i].toFloat());

         return ar;
      }



      #region Vec3 & Matrix helpers

      static public Vector3 unitX = new Vector3(1, 0, 0);
      static public Vector3 unitY = new Vector3(0, 1, 0);
      static public Vector3 unitZ = new Vector3(0, 0, 1);

      static public float Dot(ref Vector3 a, ref Vector3 b)
      {
         return a.X * b.X + a.Y * b.Y + a.Z * b.Z;
      }
      static public Vector3 Normalize(Vector3 a)
      {
         Vector3 b = a;
         float len = a.Length();// Math.Sqrt((float)(b.X * b.X + b.Y * b.Y + b.Z * b.Z));
         if (Math.Abs(len) > 0.0001)
         {
            b.X /= len;
            b.Y /= len;
            b.Z /= len;
         }
         return b;
      }
      static public Vector3 Cross(Vector3 a, Vector3 b)
      {
         Vector3 q = new Vector3();
         q.X = a.Y * b.Z - a.Z * b.Y;
         q.Y = a.Z * b.X - a.X * b.Z;
         q.Z = a.X * b.Y - a.Y * b.X;
         return q;
      }

      static Vector3 _op_CrossRef = new Vector3();
      static public void CrossRef(ref Vector3 a, ref Vector3 b, ref Vector3 res)
      {
         res.X = a.Y * b.Z - a.Z * b.Y;
         res.Y = a.Z * b.X - a.X * b.Z;
         res.Z = a.X * b.Y - a.Y * b.X;
         //return _op_CrossRef;
      }

      static public void Vec3Set(ref Vector3 q, float x, float y, float z)
      {
         q.X = x;
         q.Y = y;
         q.Z = z;
      }
      static public Vector3 Vec3Mul(Vector3 a, Vector3 b)
      {
         return new Vector3(a.X * b.X, a.Y * b.Y, a.Z * b.Z);
      }
      static public Vector3 Vec3Div(Vector3 a, Vector3 b)
      {
         return new Vector3(a.X / b.X, a.Y / b.Y, a.Z / b.Z);
      }
      static public Vector3 Vec3RCP(Vector3 a)
      {
         return new Vector3(1.0f/a.X,1.0f/ a.Y,1.0f/ a.Z);
      }
      static public Vector3 Vec3Splat(Vector3 a, int axis)
      {
         if (axis == 1) return new Vector3(a.Y, a.Y, a.Y);
         else if (axis == 2) return new Vector3(a.Z, a.Z, a.Z);
         
         return new Vector3(a.X, a.X, a.X);
      }
      static public Vector3 Vec3Select(Vector3 a, Vector3 b, Vector3 selectors)
      {
         return new Vector3(selectors.X > 0 ? b.X : a.X, selectors.Y > 0 ? b.Y : a.Y, selectors.Z > 0 ? b.Z : a.Z);
      }
      static public bool Vec3Greater(Vector3 a, Vector3 b)
      {
         return (a.X > b.X && a.Y > b.Y && a.Z > b.Z);
      }
      static public bool Vec3Less(Vector3 a, Vector3 b)
      {
         return (a.X < b.X && a.Y < b.Y && a.Z < b.Z);
      }
      static public Vector3 Vec3LessOrEqual(Vector3 V1, Vector3 V2)
      {
         Vector3 Result;
         Result.X = (V1.X <= V2.X) ? float.MaxValue : 0;
         Result.Y = (V1.Y <= V2.Y) ? float.MaxValue : 0;
         Result.Z = (V1.Z <= V2.Z) ? float.MaxValue : 0;
         return Result;

      }
      static public Vector3 Vec3Abs(Vector3 a)
      {
         return new Vector3(Math.Abs(a.X), Math.Abs(a.Y), Math.Abs(a.Z));
      }
      static public Vector3 Vec3InBounds(Vector3 V, Vector3 bounds)
      {
         Vector3 Control;

         Control.X = (V.X <= bounds.X && V.X >= -bounds.X) ? float.MaxValue : 0;
         Control.Y = (V.Y <= bounds.Y && V.Y >= -bounds.Y) ? float.MaxValue : 0;
         Control.Z = (V.Z <= bounds.Z && V.Z >= -bounds.Z) ? float.MaxValue : 0;
         

         return Control;

      }
      static public bool Vec3AnyPos(Vector3 a)
      {
         if(a.X > 0 || a.Y > 0 || a.Z > 0)
            return true;

         return false;
      }
      static public Vector3 projVec3toVec3(Vector3 src, Vector3 dst)
      {
         float k = Dot(ref src,ref  dst);
         float d = dst.Length();
         d *= d;
         return dst * d;
      }
      static public void SetByDim(ref Vector3 vector, int dimension, float val)
      {
         if (dimension == 0)
            vector.X = val;
         if (dimension == 1)
            vector.Y = val;
         if (dimension == 2)
            vector.Z = val;
      }
      static public float GetByDim(ref Vector3 vector, int dimension)
      {
         if (dimension == 0)
            return vector.X;
         if (dimension == 1)
            return vector.Y;
         if (dimension == 2)
            return vector.Z;
         return 0;
      }

      static public Vector2 vec3tovec2XZPlane(Vector3 a)
      {
         return new Vector2(a.X, a.Z);
      }

      static public float vec3vec3Angle_0to180(Vector3 a, Vector3 b)
      {
         return (float)Math.Acos(Vector3.Dot(a, b));
      }

      static public float vec2vec2Angle_0to360(Vector2 a, Vector2 b)
      {
         Vector2 aTang = new Vector2(-a.Y, a.X);
         aTang.Normalize();

         float angle = (float)Geometry.RadianToDegree((float)Math.Acos(Vector2.Dot(a, b)));

         if (Vector2.Dot(aTang, b) > 0)
            angle = (180-angle)+180;

         return angle;
      }

      static public Vector4 vec4Transform(ref Vector3 input, ref Matrix mat)
      {
         Vector4 v = new Vector4(input.X, input.Y, input.Z, 1);
         return vec4Transform(ref v, ref mat);
      }
      static public Vector4 vec4Transform(ref Vector4 input,ref Matrix mat)
      {
         Vector4 outV = Vector4.Empty;
         outV.X = input.X * mat.M11 + input.Y * mat.M21 + input.Z * mat.M31 + input.W * mat.M41;
         outV.Y = input.X * mat.M12 + input.Y * mat.M22 + input.Z * mat.M32 + input.W * mat.M42;
         outV.Z = input.X * mat.M13 + input.Y * mat.M23 + input.Z * mat.M33 + input.W * mat.M43;
         outV.W = input.X * mat.M14 + input.Y * mat.M24 + input.Z * mat.M34 + input.W * mat.M44;

         return outV;
      }


      //matrix helpers
      static public Matrix matrixFrom16floats(float _00, float _01, float _02, float _03,
                                             float _10, float _11, float _12, float _13,
                                             float _20, float _21, float _22, float _23,
                                             float _30, float _31, float _32, float _33)
      {
         Matrix m = new Matrix();
         m.M11 = _00;
         m.M12 = _01;
         m.M13 = _02;
         m.M14 = _03;

         m.M21 = _10;
         m.M22 = _11;
         m.M23 = _12;
         m.M24 = _13;

         m.M31 = _20;
         m.M32 = _21;
         m.M33 = _22;
         m.M34 = _23;

         m.M41 = _30;
         m.M42 = _31;
         m.M43 = _32;
         m.M44 = _33;
         return m;
      }
      static public Vector3 getRow(Matrix x, int row)
      {
         if (row == 1)      return new Vector3(x.M21, x.M22, x.M23);
         else if (row == 2) return new Vector3(x.M31, x.M32, x.M33);
         else if (row == 3) return new Vector3(x.M41, x.M42, x.M43);

         return new Vector3(x.M11, x.M12, x.M13);
      }
      static public void setRow(ref Matrix x, int row, Vector3 val)
      {
         if(row==0){          x.M11 = val.X; x.M12 = val.Y; x.M13 = val.Z;}
         else if (row == 1) { x.M21 = val.X; x.M22 = val.Y; x.M23 = val.Z; }
         else if (row == 2) { x.M31 = val.X; x.M32 = val.Y; x.M33 = val.Z; }
         else if (row == 3) { x.M41 = val.X; x.M42 = val.Y; x.M43 = val.Z; }
      }
      static public Vector3 getCol(Matrix x, int col)
      {
         if (col == 1) return new Vector3(x.M12, x.M22, x.M32);
         else if (col == 2) return new Vector3(x.M13, x.M23, x.M33);
         else if (col == 3) return new Vector3(x.M14, x.M24, x.M34);

         return new Vector3(x.M11, x.M21, x.M31);
      }
      static public void setCol(ref Matrix x, int col, Vector3 val)
      {
         if (col == 0)      { x.M11 = val.X; x.M21 = val.Y; x.M31 = val.Z; }
         else if (col == 1) { x.M12 = val.X; x.M22 = val.Y; x.M32 = val.Z; }
         else if (col == 2) { x.M13 = val.X; x.M23 = val.Y; x.M33 = val.Z; }
         else if (col == 3) { x.M14 = val.X; x.M24 = val.Y; x.M34 = val.Z; }
      }

      static public Matrix makeRotateXZMatrix(float rads)
      {
         Matrix rotmat = Matrix.Identity;
         float sinval = (float)Math.Sin(rads);
         float cosval = (float)Math.Cos(rads);
         rotmat.M11 = cosval;
         rotmat.M31 = sinval;
         rotmat.M13 = -sinval;
         rotmat.M33 = cosval;
         return rotmat;
      }

      static public Matrix makeRotateArbitraryMatrix(float rads, Vector3 axis)
      {
         float c = (float)Math.Cos(rads);
         float omc = 1.0f - c;
         float s = (float)Math.Sin(rads);

         float A = axis.X;
         float B = axis.Y;
         float C = axis.Z;

         Matrix m = new Matrix();

         m.M11 = A*A*omc+c;
         float AB = A*B;
         m.M21 = AB*omc-C*s;
         float AC = A*C;
         m.M31 = AC*omc+B*s;
         m.M41 = 0.0f;

         m.M12 = AB*omc+C*s;
         m.M22 = B*B*omc+c;
         float BC = B*C;
         m.M32 = BC*omc-A*s;
         m.M42 = 0.0f;

         m.M13 = AC*omc-B*s;
         m.M23 = BC*omc+A*s;
         m.M33 = C*C*omc+c;
         m.M43 = 0.0f;

         m.M14 = 0.0f;
         m.M24 = 0.0f;
         m.M34 = 0.0f;
         m.M44 = 0.0f;

         return m;
      }

      static public Matrix makeRotateMatrix2(Vector3 forward, Vector3 right)
      {

         Vector3 up = Vector3.Cross(forward, right);
         Matrix rotationMatrix = Matrix.Identity;

         forward = Vector3.Normalize(forward);
         up = Vector3.Normalize(up);
         right = Vector3.Normalize(right);

         rotationMatrix.M31 = forward.X;
         rotationMatrix.M32 = forward.Y;
         rotationMatrix.M33 = forward.Z;

         rotationMatrix.M21 = up.X;
         rotationMatrix.M22 = up.Y;
         rotationMatrix.M23 = up.Z;

         rotationMatrix.M11 = right.X;
         rotationMatrix.M12 = right.Y;
         rotationMatrix.M13 = right.Z;

         return rotationMatrix;
      }

      static public Matrix makeRotateMatrix(Vector3 fromVec, Vector3 toVec) 
      {

         Vector3 from = Vector3.Normalize(fromVec);
         Vector3 to = Vector3.Normalize(toVec);

         float e, h, f;
         Matrix mtx = Matrix.Identity;

        Vector3 v = Vector3.Cross(from, to);
        e = Vector3.Dot(from, to);
        f = (e < 0)? -e:e;
        if (f > 1.0 - 0.000001f)     /* "from" and "to"-vector almost parallel */
        {
          Vector3 u = Vector3.Empty;
          v = Vector3.Empty; /* temporary storage vectors */
          Vector3 x = Vector3.Empty;       /* vector most nearly orthogonal to "from" */
          float c1, c2, c3; /* coefficients for later use */
          int i, j;

          x.X = (from.X > 0.0)? from.X : -from.X;
          x.Y = (from.Y > 0.0)? from.Y : -from.Y;
          x.Z = (from.Z > 0.0)? from.Z : -from.Z;

          if (x.X < x.Y)
          {
            if (x.X < x.Z)
            {
               x.X = 1.0f; x.Y = x.Z = 0.0f;
            }
            else
            {
               x.Z = 1.0f; x.X = x.Y = 0.0f;
            }
          }
          else
          {
             if (x.Y < x.Z)
            {
               x.Y = 1.0f; x.X = x.Z = 0.0f;
            }
            else
            {
               x.Z = 1.0f; x.X = x.Y = 0.0f;
            }
          }

          u.X = x.X - from.X; u.Y = x.Y - from.Y; u.Z = x.Z - from.Z;
          v.X = x.X - to.X; v.Y = x.Y - to.Y; v.Z = x.Z - to.Z;

          c1 = 2.0f / Vector3.Dot(u, u);
          c2 = 2.0f / Vector3.Dot(v, v);
          c3 = c1 * c2  * Vector3.Dot(u, v);

          for (i = 0; i < 3; i++) 
          {
             setCol(ref mtx, i, new Vector3(
                           -c1 * GetByDim(ref u, i) * GetByDim(ref u, 0)
                           - c2 * GetByDim(ref v, i) * GetByDim(ref v, 0)
                           + c3 * GetByDim(ref v, i) * GetByDim(ref u, 0),

                           -c1 * GetByDim(ref u, i) * GetByDim(ref u, 1)
                           - c2 * GetByDim(ref v, i) * GetByDim(ref v, 1)
                           + c3 * GetByDim(ref v, i) * GetByDim(ref u, 1),

                           -c1 * GetByDim(ref u, i) * GetByDim(ref u, 2)
                           - c2 * GetByDim(ref v, i) * GetByDim(ref v, 2)
                           + c3 * GetByDim(ref v, i) * GetByDim(ref u, 2)));
            
          }
          setCol(ref mtx, 0, getCol(mtx, 0) + new Vector3(1, 0, 0));
          setCol(ref mtx, 1, getCol(mtx, 1) + new Vector3(0, 1, 0));
          setCol(ref mtx, 2, getCol(mtx, 2) + new Vector3(0, 0, 1));
        }
        else  /* the most common case, unless "from"="to", or "from"=-"to" */
        {
             /* unoptimized version - a good compiler will optimize this. */
             /* h = (1.0 - e)/DOT(v, v); old code */
             h = 1.0f/(1.0f + e);      /* optimization by Gottfried Chen */
             setCol(ref mtx,0,new Vector3(e + h * v.X * v.X,
                                           h * v.X * v.Y - v.Z,
                                           h * v.X * v.Z + v.Y));

             setCol(ref mtx,1,new Vector3( h * v.X * v.Y + v.Z,
                                          e + h * v.Y * v.Y,
                                          h * v.Y * v.Z - v.X));

             setCol(ref mtx,2, new Vector3( h * v.X * v.Z - v.Y,
                                       h * v.Y * v.Z + v.X,
                                       e + h * v.Z * v.Z));
        }

        return mtx;
      }


      #endregion  

      #region intersection


      enum eIntersectionType
      {
         cIT_Empty = 0,
         cIT_Segment,
         cIT_Ray,
         cIT_Point
      };
      public enum eVolumeIntersectionType
      {
         cVIT_Empty = 0,
         cVIT_Contains,
         cVIT_Spans,
        };
      
      static public bool spheresIntersect(ref Vector3 center1, float radius1, ref Vector3 center2, float radius2)
      {
         //-- Get distance between the centers.
         float dx = center1.X - center2.X;
         float dy = center1.Y - center2.Y;
         float dz = center1.Z - center2.Z;
         float distanceSqr = (dx * dx) + (dy * dy) + (dz * dz);

         //-- Check the distance.
         float combinedRadius = radius1 + radius2;
         return (distanceSqr <= (combinedRadius * combinedRadius));
      }
      static public bool sphereAABBIntersect(Vector3 sphereCenter, float sphereRadius, Vector3 boxMin, Vector3 boxMax)
      {
         // FROM GRAPHICS GEMS I - p.335
         float minSqDistance = 0.0f;

         if (sphereCenter.X < boxMin.X)
            minSqDistance += (sphereCenter.X - boxMin.X) * (sphereCenter.X - boxMin.X);
         else if (sphereCenter.X > boxMax.X)
            minSqDistance += (sphereCenter.X - boxMax.X) * (sphereCenter.X - boxMax.X);

         if (sphereCenter.Y < boxMin.Y)
            minSqDistance += (sphereCenter.Y - boxMin.Y) * (sphereCenter.Y - boxMin.Y);
         else if (sphereCenter.Y > boxMax.Y)
            minSqDistance += (sphereCenter.Y - boxMax.Y) * (sphereCenter.Y - boxMax.Y);

         if (sphereCenter.Z < boxMin.Z)
            minSqDistance += (sphereCenter.Z - boxMin.Z) * (sphereCenter.Z - boxMin.Z);
         else if (sphereCenter.Z > boxMax.Z)
            minSqDistance += (sphereCenter.Z - boxMax.Z) * (sphereCenter.Z - boxMax.Z);

         if (minSqDistance <= (sphereRadius * sphereRadius))
            return true;
         else
            return false;
      }
      static public bool spherePlaneIntersect(Vector3 sphereOrig, float sphereRad, Plane pln)
      {
         float d = pointPlaneDistance(sphereOrig, pln);
         if (d > sphereRad)
            return false;

         return true;
      }


      static public float pointPlaneDistance(Vector3 point, Plane pln)
      {
         return pln.Dot(point);
      }
      static public float pointLineDistance(Vector3 A, Vector3 B, Vector3 P)
      {
         Vector3 AP = P - A;
         Vector3 AB = B - A;

         Vector3 ABxAP = Vector3.Cross(AP, AB);
         float a = ABxAP.Length();

         float d = AB.Length();

         return a / d;

      }
      static public bool pointSphereIntersect(ref Vector3 sphereCenter, float sphereRadius, ref Vector3 pt)
      {
         Vector3 diff = sphereCenter - pt;
         if (diff.LengthSq() <= (sphereRadius * sphereRadius))
         {
            return (true);
         }
         else
         {
            return (false);
         }
      }
      static public bool pointCylinderIntersect(ref Vector3 sphereCenter, float sphereRadius, ref Vector3 pt)
      {
         Vector3 diff = sphereCenter - pt;
         diff.Y = 0.0f;
         if (diff.LengthSq() <= (sphereRadius * sphereRadius))
         {
            return (true);
         }
         else
         {
            return (false);
         }
      }
      static public bool pointBoxIntersect(ref Vector3 tA, ref Vector3 tB, ref Vector3 tC, ref Vector3 tD,
                             ref Vector3 bA, ref Vector3 bB, ref Vector3 bC, ref Vector3 bD,
                             ref Vector3 pt)
      {
         Plane[] planes = new Plane[6];

         planes[0] = Plane.FromPoints(tA, tC, tB);
         planes[1] = Plane.FromPoints(tA, tB, bA);
         planes[2] = Plane.FromPoints(tA, bA, tC);
         planes[3] = Plane.FromPoints(tB, tD, bB);
         planes[4] = Plane.FromPoints(tD, tC, bD);
         planes[5] = Plane.FromPoints(bA, bB, bC);

         for (int i = 0; i < 6; i++)
         {
            planes[i].Normalize();
            float fDistance = planes[i].A * pt.X + planes[i].B * pt.Y + planes[i].C * pt.Z + planes[i].D;

            // if this distance is < 0, we're on the backside
            if (fDistance < 0)
               return false;
         }
         return true;
      }
      static public Vector3 pointClosestToLine(ref Vector3 lineA, ref Vector3 lineB, ref Vector3 pt, out float U)
      {
         float mag = Vector3.Length(lineA - lineB);

         U = (((pt.X - lineA.X) * (lineB.X - lineA.X)) +
                    ((pt.Y - lineA.Y) * (lineB.Y - lineA.Y)) +
                    ((pt.Z - lineA.Z) * (lineB.Z - lineA.Z))) /
                    (mag*mag);

         if (U < 0.0f || U > 1.0f)
         {
            return Vector3.Empty;   // closest point does not fall within the line segment
         }

         
         return lineA + U * (lineB - lineA);
      }

      static public bool pointInsideConvexPolygon(ref Vector3 pt, ref Vector3[] vertex)
      {
         /*
          * computing the sum of the angles between the test point and every pair of edge points p[i]->p[i+1]. 
          * This sum will only be 2pi if both the point is on the plane of the polygon AND on the interior. 
          * The angle sum will tend to 0 the further away from the polygon point q becomes.
          */
         double m1, m2;
         double anglesum = 0; //IN RADIANS!!
         double costheta = 0;
         Vector3 p1 = Vector3.Empty;
         Vector3 p2 = Vector3.Empty;

         for (uint i = 0; i < (uint)vertex.Length; i++)
         {
            p1.X = vertex[i].X - pt.X;
            p1.Y = vertex[i].Y - pt.Y;
            p1.Z = vertex[i].Z - pt.Z;
            p2.X = vertex[(i + 1) % vertex.Length].X - pt.X;
            p2.Y = vertex[(i + 1) % vertex.Length].Y - pt.Y;
            p2.Z = vertex[(i + 1) % vertex.Length].Z - pt.Z;


            if (pointLineDistance(pt, vertex[i], vertex[(i + 1) % vertex.Length]) < cTinyEpsilon)//if we're on the line of the poly, we're inside...
               return true;

            m1 = p1.LengthSq();
            m2 = p2.LengthSq();
            if (m1 * m2 <= cTinyEpsilon)
               return true;//(cPI * 2); /* We are on a node, consider this inside */
            else
               costheta = (p1.X * p2.X + p1.Y * p2.Y + p1.Z * p2.Z) / (m1 * m2);

            anglesum += Math.Acos(costheta);
         }

         return (Math.Abs(anglesum -(2*cPI)) < cTinyEpsilon);

      }


      static public bool aabbsIntersect(ref Vector3 aMin, ref Vector3 aMax, ref Vector3 bMin, ref Vector3 bMax)
      {
         //A.LO<=B.HI && A.HI>=B.LO

         if (aMin.X < bMax.X && aMin.Y < bMax.Y && aMin.Z < bMax.Z &&
            bMin.X < aMax.X && bMin.Y < aMax.Y && bMin.Z < aMax.Z)
            return true;

         return false;
      }

      static public eVolumeIntersectionType boxAABBIntersect(Vector3 AABBmin, Vector3 AABBmax,
                             Vector3 tA, Vector3 tB, Vector3 tC, Vector3 tD,
                             Vector3 bA, Vector3 bB, Vector3 bC, Vector3 bD)
      {
         Plane[] planes = new Plane[6];

         planes[0] = Plane.FromPoints(tA, tC, tB);
         planes[1] = Plane.FromPoints(tA, tB, bA);
         planes[2] = Plane.FromPoints(tA, bA, tC);
         planes[3] = Plane.FromPoints(tB, tD, bB);
         planes[4] = Plane.FromPoints(tD, tC, bD);
         planes[5] = Plane.FromPoints(bA, bB, bC);

         Vector3 c = Vector3.Multiply(AABBmin + AABBmax, 0.5f);
         Vector3 d = AABBmax - c;
         float r = Vector3.Length(AABBmax - AABBmin) * 0.5f;
         bool spanning = false;
         for (int i = 0; i < 6; i++)
         {
            Vector3 p;
            p.X = planes[i].A;//[0];
            p.Y = planes[i].B;//[1];
            p.Z = planes[i].C;//[2];

            float NP = (float)(d.X * Math.Abs(p.X) + d.Y * Math.Abs(p.Y) + d.Z * Math.Abs(p.Z));

            float MP = c.X * p.X + c.Y * p.Y + c.Z * p.Z + planes[i].D;//[3];

            if ((MP + NP) < 0.0f) return eVolumeIntersectionType.cVIT_Empty; // behind clip plane


            if (pointPlaneDistance(c, planes[i]) < r)//(MP + NP) < r) 
               spanning = true;// return eVolumeIntersectionType.cVIT_Spans; // behind clip plane
         }

         if(spanning)
            return eVolumeIntersectionType.cVIT_Spans;

         return eVolumeIntersectionType.cVIT_Contains;
      }
      static public eVolumeIntersectionType boxSphereIntersect(Vector3 center, float rad,
                             Vector3 tA, Vector3 tB, Vector3 tC, Vector3 tD,
                             Vector3 bA, Vector3 bB, Vector3 bC, Vector3 bD)
      {
         Plane[] planes = new Plane[6];

         planes[0] = Plane.FromPoints(tA, tC, tB);
         planes[1] = Plane.FromPoints(tA, tB, bA);
         planes[2] = Plane.FromPoints(tA, bA, tC);
         planes[3] = Plane.FromPoints(tB, tD, bB);
         planes[4] = Plane.FromPoints(tD, tC, bD);
         planes[5] = Plane.FromPoints(bA, bB, bC);
         
         // various distances
         float fDistance;
         bool spanning = false;

         // calculate our distances to each of the planes
         for (int i = 0; i < 6; ++i)
         {
            // find the distance to this plane
            fDistance = planes[i].A * center.X + planes[i].B * center.Y + planes[i].C * center.Z + planes[i].D;

            // if this distance is < -sphere.radius, we are outside
            if (fDistance < -rad)
               return (eVolumeIntersectionType.cVIT_Empty);

            // else if the distance is between +- radius, then we intersect
            if ((float)Math.Abs(fDistance) < rad)
               spanning = true;// return (eVolumeIntersectionType.cVIT_Spans);
         }

         if(spanning)
            return (eVolumeIntersectionType.cVIT_Spans);

         // otherwise we are fully in view
         return (eVolumeIntersectionType.cVIT_Contains);
      }

      static public bool raySegmentIntersectionTriangle(Vector3[] vertex, ref Vector3 origin, ref Vector3 direction, bool segment, ref Vector3 iPoint)
      {
         //   if (segment)

         // Moller/Trumbore method -- supposedly quick without requiring the normal.
         // Also computes the barycentric coordinates of the intersection point if we wanted to give 
         // those back for some reason.

         // Get vectors along the two triangle edges sharing vertex 0.
         Vector3 edge1;
         //edge1 = vertex[1] - vertex[0];
         edge1.X = vertex[1].X - vertex[0].X;
         edge1.Y = vertex[1].Y - vertex[0].Y;
         edge1.Z = vertex[1].Z - vertex[0].Z;
         Vector3 edge2;
         //edge2 = vertex[2] - vertex[0];
         edge2.X = vertex[2].X - vertex[0].X;
         edge2.Y = vertex[2].Y - vertex[0].Y;
         edge2.Z = vertex[2].Z - vertex[0].Z;

         // Calc determinant.
         Vector3 pvec;
         pvec = Vector3.Cross(direction, edge2);
         float det = BMathLib.Dot(ref edge1, ref pvec);

         // If determinant is near, the ray is in the plane of the triangle.  In that case there is no intersection.
         if ((det > -cFloatCompareEpsilon) && (det < cFloatCompareEpsilon))
            return (false);

         // Get reciprocal.
         float recipDet = 1.0f / det;

         // Calc dist from vertex 0 to origin
         Vector3 tvec;
         //tvec = origin - vertex[0];
         tvec.X = origin.X - vertex[0].X;
         tvec.Y = origin.Y - vertex[0].Y;
         tvec.Z = origin.Z - vertex[0].Z;

         // Get u param of barycentric coords.
         float u = BMathLib.Dot(ref tvec, ref pvec) * recipDet;


         // See if it's inside triangle.
         if (u < -cFloatCompareEpsilon || u > 1.0f + cFloatCompareEpsilon)
            return (false);

         // Calc vector for v portion.
         Vector3 qvec;
         qvec = Vector3.Cross(tvec, edge1);

         // Calc v.
         float v = BMathLib.Dot(ref direction, ref qvec) * recipDet;

         // See if it's inside triangle.
         if (v < -cFloatCompareEpsilon || u + v > 1.0f + cFloatCompareEpsilon)
            return (false);

         // Compute t.
         float t = BMathLib.Dot(ref edge2, ref qvec) * recipDet;

         // See if we're off the ray/segment.
         if (t < -cFloatCompareEpsilon || (segment && t > 1.0f + cFloatCompareEpsilon))
            return (false);

         // Get intersection point.
         iPoint = origin + (t * direction);

         return true;
      }
      static public bool ray3AABB(ref Vector3 coord, ref float t, ref Vector3 rayOrig, ref Vector3 rayDir, ref Vector3 boxmin, ref Vector3 boxmax)
      {

         //enum 
         //{ 
         //   NumDim = 3, 
         //   Right = 0, 
         //   Left = 1, 
         //   Middle = 2 
         //}
         int NumDim = 3;
         int Right = 0;
         int Left = 1;
         int Middle = 2;

         bool inside = true;
         int[] quadrant = new int[NumDim];
         float[] candidatePlane = new float[NumDim];

         for (int i = 0; i < NumDim; i++)
         {
            if (GetByDim(ref rayOrig, i) < GetByDim(ref boxmin, i))
            {
               quadrant[i] = Left;
               candidatePlane[i] = GetByDim(ref boxmin, i);
               inside = false;
            }
            else if (GetByDim(ref rayOrig, i) > GetByDim(ref boxmax, i))
            {
               quadrant[i] = Right;
               candidatePlane[i] = GetByDim(ref boxmax, i);
               inside = false;
            }
            else
            {
               quadrant[i] = Middle;
            }
         }

         if (inside)
         {
            coord = rayOrig;
            t = 0.0f;
            return true;
         }

         float[] maxT = new float[NumDim];
         for (int i = 0; i < NumDim; i++)
         {
            if ((quadrant[i] != Middle) && (GetByDim(ref rayDir, i) != 0.0f))
               maxT[i] = (candidatePlane[i] - GetByDim(ref rayOrig, i)) / GetByDim(ref rayDir, i);
            else
               maxT[i] = -1.0f;
         }

         int whichPlane = 0;
         for (int i = 1; i < NumDim; i++)
            if (maxT[whichPlane] < maxT[i])
               whichPlane = i;

         if (maxT[whichPlane] < 0.0f)
            return false;

         for (int i = 0; i < NumDim; i++)
         {
            if (i != whichPlane)
            {
               SetByDim(ref coord, i, GetByDim(ref rayOrig, i) + maxT[whichPlane] * GetByDim(ref rayDir, i));

               if ((GetByDim(ref coord, i) < GetByDim(ref boxmin, i)) || (GetByDim(ref coord, i) > GetByDim(ref boxmax, i)))
               {
                  return false;
               }
            }
            else
            {
               SetByDim(ref coord, i, candidatePlane[i]);
            }
         }

         t = maxT[whichPlane];
         return true;
      }
      static public bool rayPlaneIntersect(Plane pl,Vector3 orig, Vector3 dir,bool oneSided, ref float tVal)
      {
         
         Vector3 pn = new Vector3(pl.A, pl.B, pl.C);
         float k = Vector3.Dot(pn, dir);
         if (k == 0) //ray is parallel
            return false;
         if (oneSided && k > 0)
            return false;

         float v0 = -(Vector3.Dot(pn, orig) + pl.D);
         float t = v0 / k;

         if (t < 0)
            return false;

         tVal = t;

         return true;
      }
      static public bool raySphereIntersect(Vector3 sphereOrig, float sphereRad, Vector3 r0, Vector3 rD, ref float tVal)
      {
         Vector3 l = sphereOrig - r0;
         float s = Vector3.Dot(l, rD);
         float l2 = Vector3.Dot(l, l);
         float r2 = sphereRad * sphereRad;

         float m2 = l2 - s * s;// m2 is squared distance from the center of the sphere to the projection.

         // If the ray origin is outside the sphere and the center of the sphere is 
         // behind the ray origin there is no intersection.
         if (s < 0 && l2 > r2)
            return false;

         // If the squared distance from the center of the sphere to the projection
         // is greater than the radius squared the ray will miss the sphere.

         if (m2 > r2)
            return false;

         //the ray hits the sphere...
         float q = (float)Math.Sqrt((double)(r2 -m2));
         float t1 = s - q;
         float t2 = s + q;

         bool OriginInside = l2 < r2;
         tVal = OriginInside ? t2 : t1;

         return true;
      }
      static public bool rayCylindirIntersect(Vector3 r0, Vector3 rD, Vector3 cylPos, Vector3 cylAxis, float cylRad)
      {
         Vector3 rayB = r0 + rD*1000;
         Vector3 cyl0 = cylPos;
         Vector3 cyl1 = cylPos + cylAxis;
         float dist = lineLineDistance(r0, rayB, cyl0, cyl1);
         if (dist == -1 || dist > cylRad)
            return false;

         return true;
      }


      static public float rayPlaneAngle(Vector3 r0, Vector3 rD, Plane pl)
      {
         float tVal = 0;
         float angle = 0;
         if (rayPlaneIntersect(pl, r0, rD, false, ref tVal))
         {
            angle = 90 - vec3vec3Angle_0to180(r0, new Vector3(pl.A, pl.B, pl.C));
         }
         return angle;
      }

      
      static public bool rayOOBBIntersect(Vector3 center, Vector3 extents, Quaternion orientation, Vector3 r0, Vector3 rD, ref float tVal)
      {
         //CLM - This is horrid. Convert OBB to 12 triangles. Do Ray-tri intersection tests.
         Matrix k = Matrix.RotationQuaternion(orientation);

         Vector3 min = (center - extents);
         Vector3 max = (center + extents);

         Vector3[] cubeVertices =
		   {
			   new Vector3(min.X,max.Y,min.Z),
			   new Vector3(max.X,max.Y,min.Z),
			   new Vector3(min.X,min.Y,min.Z),
			   new Vector3(max.X,min.Y,min.Z),

			   new Vector3(min.X,max.Y,max.Z),
			   new Vector3(min.X,min.Y,max.Z),   
			   new Vector3(max.X,max.Y,max.Z),  
			   new Vector3(max.X,min.Y,max.Z),    
		   };
         for (int i = 0; i < cubeVertices.Length; i++)
         {
            Vector4 v = Vector3.Transform(cubeVertices[i], k);
            cubeVertices[i].X = v.X;
            cubeVertices[i].Y = v.Y;
            cubeVertices[i].Z = v.Z;
          //  cubeVertices[i] += translation;
         }

         int[] cubeIndices =
		   {
			   0, 1, 2, 
            2, 1, 3, 

			   4, 6, 5, 
            5, 6, 7, 

			   4, 6, 0,
            0, 6, 1,

			   5, 2, 7,
            7, 2, 3,

			   1, 6, 3,
            3, 6, 7, 

			   0, 2, 4,
            4, 2, 5  
		   };

         bool hit = false;
         int count = cubeIndices.Length / 3;
         Vector3 hitPt = Vector3.Empty;
         Vector3 []verts = new Vector3[3];
         tVal = float.MaxValue;
         for (int i = 0; i < count;i++ )
         {
            verts[0] = cubeVertices[cubeIndices[i * 3]];
            verts[1] = cubeVertices[cubeIndices[i * 3 + 1]];
            verts[2] = cubeVertices[cubeIndices[i * 3 + 2]];
            if(raySegmentIntersectionTriangle(verts, ref r0, ref rD, false, ref hitPt))
            {
               Vector3 d = hitPt - r0;
               if (d.Length() < tVal)
                  tVal = d.Length();
               hit = true;
            }
            
         }



         return hit;
      }

      static public float lineLineDistance(Vector3 lnA0,Vector3 lnA1,Vector3 lnB0,Vector3 lnB1)
      {

         Vector3 p13,p43,p21;
         
          
         p13 = lnA0 - lnB0;
         p43 = lnB1 - lnB0;

         if (Math.Abs(p43.X)  < cFloatCompareEpsilon && Math.Abs(p43.Y)  < cFloatCompareEpsilon && Math.Abs(p43.Z)  < cFloatCompareEpsilon)
            return(-1);

         p21 = lnA1 - lnA0;

         if (Math.Abs(p21.X) < cFloatCompareEpsilon && Math.Abs(p21.Y) < cFloatCompareEpsilon && Math.Abs(p21.Z) < cFloatCompareEpsilon)
            return(-1);

         float d1343 = p13.X * p43.X + p13.Y * p43.Y + p13.Z * p43.Z;
         float d4321 = p43.X * p21.X + p43.Y * p21.Y + p43.Z * p21.Z;
         float d1321 = p13.X * p21.X + p13.Y * p21.Y + p13.Z * p21.Z;
         float d4343 = p43.X * p43.X + p43.Y * p43.Y + p43.Z * p43.Z;
         float d2121 = p21.X * p21.X + p21.Y * p21.Y + p21.Z * p21.Z;

         float denom = d2121 * d4343 - d4321 * d4321;

         if (Math.Abs(denom) < cFloatCompareEpsilon)
            return(-1);

         float numer = d1343 * d4321 - d1321 * d4343;

         float mua = numer / denom;
         float mub = (d1343 + d4321 * (mua)) / d4343;

         if (mua < 0 || mua > 1 || mub < 0 || mub > 1)
            return -1;

         Vector3 pa = lnA0 + mua * p21;
         Vector3 pb = lnB0 + mub * p43;
         Vector3 c = pa - pb;
         
         return c.Length();
         
      }
      public enum eLineIntersectResult { PARALLEL, COINCIDENT, NOT_INTERESECTING, INTERESECTING };
      static public eLineIntersectResult lineLineIntersect2D(ref Vector2 lnA0,ref Vector2 lnA1,ref Vector2 lnB0,ref Vector2 lnB1, out Vector2 intersection)
      {
         float uA=0;
         float uB=0;
         return lineLineIntersect2D(ref lnA0,ref lnA1,ref lnB0,ref lnB1,out intersection,out uA,out uB);

      }
      static public eLineIntersectResult lineLineIntersect2D(ref Vector2 lnA0,ref Vector2 lnA1,ref Vector2 lnB0,ref Vector2 lnB1, 
                                                               out Vector2 intersection, out float uA, out float uB)
      {


         /*
          * Notes:
          * The denominators for the equations for ua and ub are the same.
          * If the denominator for the equations for ua and ub is 0 then the two lines are parallel.
          * If the denominator and numerator for the equations for ua and ub are 0 then the two lines are coincident.
          * The equations apply to lines, if the intersection of line segments is required then it is only necessary to test 
          * if ua and ub lie between 0 and 1. 
          * Whichever one lies within that range then the corresponding line segment contains the intersection point. 
          * If both lie within the range of 0 to 1 then the intersection point is within both line segments. 
          */

         float denom = ((lnB1.Y - lnB0.Y) * (lnA1.X - lnA0.X)) -
                      ((lnB1.X - lnB0.X) * (lnA1.Y - lnA0.Y));

         float nume_a = ((lnB1.X - lnB0.X) * (lnA0.Y - lnB0.Y)) -
                       ((lnB1.Y - lnB0.Y) * (lnA0.X - lnB0.X));

         float nume_b = ((lnA1.X - lnA0.X) * (lnA0.Y - lnB0.Y)) -
                       ((lnA1.Y - lnA0.Y) * (lnA0.X - lnB0.X));

        if(denom == 0.0f)
        {
           intersection = Vector2.Empty;
           uA = 0;
           uB = 0;
            if(nume_a == 0.0f && nume_b == 0.0f)
            {
                return eLineIntersectResult.COINCIDENT;
            }
            return eLineIntersectResult.PARALLEL;
        }

        float ua = nume_a / denom;
        float ub = nume_b / denom;

        if(ua >= 0.0f && ua <= 1.0f && ub >= 0.0f && ub <= 1.0f)
        {
            // Get the intersection point.
           intersection.X = lnA0.X + ua * (lnA1.X - lnA0.X);
           intersection.Y = lnA0.Y + ua * (lnA1.Y - lnA0.Y);

           uA = ua;
           uB = ub;
            return eLineIntersectResult.INTERESECTING;
        }

        intersection = Vector2.Empty;
        uA = 0;
        uB = 0;
        return eLineIntersectResult.NOT_INTERESECTING;
    
      }

      static public Matrix giveProjectMatrixForPlane(Plane pl)
      {
         Matrix m = new Matrix();

         m.M11 = pl.B * pl.B + pl.C * pl.C;
         m.M12 = -pl.A * pl.B;
         m.M13 = -pl.A * pl.C;
         m.M14 = 0;

         m.M21 = -pl.B * pl.A;
         m.M22 = pl.A * pl.A + pl.C * pl.C;
         m.M23 = -pl.B * pl.C;
         m.M24 = 0;

         m.M31 = -pl.C * pl.A;
         m.M32 = -pl.B * pl.C;
         m.M33 = pl.A * pl.A + pl.B * pl.B;
         m.M34 = 0;

         m.M41 = 0;
         m.M42 = 0;
         m.M43 = 0;
         m.M44 = 1;

         return m;
      }

      //CLM this will split a polygon and return the verts on the negative side of the plane
      //http://local.wasp.uwa.edu.au/~pbourke/geometry/polygonmesh/source1.c
      static public int splitPolygon(ref Vector3 []p/*p must be of length 4!!!*/, Plane pl, ref float []uVals)
      {
         double[] side = new double[3]; 
         Vector3 q;
         p[3] = Vector3.Empty;

         /*
            Evaluate the equation of the plane for each vertex
            If side < 0 then it is on the side to be retained
            else it is to be clipped
         */
         side[0] = pl.Dot(p[0]);//pl.A * p[0].X + pl.B * p[0].Y + pl.C * p[0].Z + pl.D;
         side[1] = pl.Dot(p[1]);//pl.A * p[1].X + pl.B * p[1].Y + pl.C * p[1].Z + pl.D;
         side[2] = pl.Dot(p[2]);//pl.A * p[2].X + pl.B * p[2].Y + pl.C * p[2].Z + pl.D;

         /* Are all the vertices are on the clipped side */
         if (side[0] >= 0 && side[1] >= 0 && side[2] >= 0)
            return (0);

         /* Are all the vertices on the not-clipped side */
         if (side[0] <= 0 && side[1] <= 0 && side[2] <= 0)
            return (3);

         /* Is p0 the onlY point on the clipped side */
         if (side[0] >= 0 && side[1] <= 0 && side[2] <= 0)
         {
            q.X = (float)(p[0].X - side[0] * (p[2].X - p[0].X) / (side[2] - side[0]));
            q.Y = (float)(p[0].Y - side[0] * (p[2].Y - p[0].Y) / (side[2] - side[0]));
            q.Z = (float)(p[0].Z - side[0] * (p[2].Z - p[0].Z) / (side[2] - side[0]));
            p[3] = q;
            q.X = (float)(p[0].X - side[0] * (p[1].X - p[0].X) / (side[1] - side[0]));
            q.Y = (float)(p[0].Y - side[0] * (p[1].Y - p[0].Y) / (side[1] - side[0]));
            q.Z = (float)(p[0].Z - side[0] * (p[1].Z - p[0].Z) / (side[1] - side[0]));
            p[0] = q;
            return (4);
         }
       /* Is p1 the onlY point on the clipped side */
         if (side[1] >= 0 && side[0] <= 0 && side[2] <= 0) {
            p[3] = p[2];
            q.X = (float)(p[1].X - side[1] * (p[2].X - p[1].X) / (side[2] - side[1]));
            q.Y = (float)(p[1].Y - side[1] * (p[2].Y - p[1].Y) / (side[2] - side[1]));
            q.Z = (float)(p[1].Z - side[1] * (p[2].Z - p[1].Z) / (side[2] - side[1]));
            p[2] = q;
            q.X = (float)(p[1].X - side[1] * (p[0].X - p[1].X) / (side[0] - side[1]));
            q.Y = (float)(p[1].Y - side[1] * (p[0].Y - p[1].Y) / (side[0] - side[1]));
            q.Z = (float)(p[1].Z - side[1] * (p[0].Z - p[1].Z) / (side[0] - side[1]));
            p[1] = q;
            return(4);
         }

         /* Is p2 the onlY point on the clipped side */
         if (side[2] >= 0 && side[0] <= 0 && side[1] <= 0) {
            q.X = (float)(p[2].X - side[2] * (p[0].X - p[2].X) / (side[0] - side[2]));
            q.Y = (float)(p[2].Y - side[2] * (p[0].Y - p[2].Y) / (side[0] - side[2]));
            q.Z = (float)(p[2].Z - side[2] * (p[0].Z - p[2].Z) / (side[0] - side[2]));
            p[3] = q;
            q.X = (float)(p[2].X - side[2] * (p[1].X - p[2].X) / (side[1] - side[2]));
            q.Y = (float)(p[2].Y - side[2] * (p[1].Y - p[2].Y) / (side[1] - side[2]));
            q.Z = (float)(p[2].Z - side[2] * (p[1].Z - p[2].Z) / (side[1] - side[2]));
            p[2] = q;
            return(4);
         }

         /* Is p0 the onlY point on the not-clipped side */
         if (side[0] <= 0 && side[1] >= 0 && side[2] >= 0) {
            q.X = (float)(p[0].X - side[0] * (p[1].X - p[0].X) / (side[1] - side[0]));
            q.Y = (float)(p[0].Y - side[0] * (p[1].Y - p[0].Y) / (side[1] - side[0]));
            q.Z = (float)(p[0].Z - side[0] * (p[1].Z - p[0].Z) / (side[1] - side[0]));
            p[1] = q;
            q.X = (float)(p[0].X - side[0] * (p[2].X - p[0].X) / (side[2] - side[0]));
            q.Y = (float)(p[0].Y - side[0] * (p[2].Y - p[0].Y) / (side[2] - side[0]));
            q.Z = (float)(p[0].Z - side[0] * (p[2].Z - p[0].Z) / (side[2] - side[0]));
            p[2] = q;
            return(3);
         }

         /* Is p1 the onlY point on the not-clipped side */
         if (side[1] <= 0 && side[0] >= 0 && side[2] >= 0) {
            q.X = (float)(p[1].X - side[1] * (p[0].X - p[1].X) / (side[0] - side[1]));
            q.Y = (float)(p[1].Y - side[1] * (p[0].Y - p[1].Y) / (side[0] - side[1]));
            q.Z = (float)(p[1].Z - side[1] * (p[0].Z - p[1].Z) / (side[0] - side[1]));
            p[0] = q;
            q.X = (float)(p[1].X - side[1] * (p[2].X - p[1].X) / (side[2] - side[1]));
            q.Y = (float)(p[1].Y - side[1] * (p[2].Y - p[1].Y) / (side[2] - side[1]));
            q.Z = (float)(p[1].Z - side[1] * (p[2].Z - p[1].Z) / (side[2] - side[1]));
            p[2] = q;
            return(3);
         }

         /* Is p2 the onlY point on the not-clipped side */
         if (side[2] <= 0 && side[0] >= 0 && side[1] >= 0) {
            q.X = (float)(p[2].X - side[2] * (p[1].X - p[2].X) / (side[1] - side[2]));
            q.Y = (float)(p[2].Y - side[2] * (p[1].Y - p[2].Y) / (side[1] - side[2]));
            q.Z = (float)(p[2].Z - side[2] * (p[1].Z - p[2].Z) / (side[1] - side[2]));
            p[1] = q;
            q.X = (float)(p[2].X - side[2] * (p[0].X - p[2].X) / (side[0] - side[2]));
            q.Y = (float)(p[2].Y - side[2] * (p[0].Y - p[2].Y) / (side[0] - side[2]));
            q.Z = (float)(p[2].Z - side[2] * (p[0].Z - p[2].Z) / (side[0] - side[2]));
            p[0] = q;
            return(3);
         }

         /* Shouldn't get here */
         return(-1);
      
      }
      static public void clipPolygon(ref Vector3 []verts, Plane plane, List<Vector3> results, List<float> resultUs)
      {
         float[] dist = new float[3];
         dist[0] = BMathLib.pointPlaneDistance(verts[0], plane);
         dist[1] = BMathLib.pointPlaneDistance(verts[1], plane);
         dist[2] = BMathLib.pointPlaneDistance(verts[2], plane);
         results.AddRange(verts);
         resultUs.Add(0);
         resultUs.Add(0);
         resultUs.Add(0);
         //early outs.
         if (
            (dist[0] == 0 && dist[1] == 0) ||  //2 pts coplanar
            (dist[0] == 0 && dist[2] == 0) ||  //2 pts coplanar
            (dist[2] == 0 && dist[1] == 0) ||  //2 pts coplanar
            (dist[0] == 0 && dist[1] < 0 && dist[2] < 0) || //1 point coplanar, 
            (dist[0] == 0 && dist[1] > 0 && dist[2] > 0) || //1 point coplanar, 
            (dist[1] == 0 && dist[2] < 0 && dist[0] < 0) || //1 point coplanar, 
            (dist[1] == 0 && dist[2] > 0 && dist[0] > 0) || //1 point coplanar, 
            (dist[2] == 0 && dist[0] < 0 && dist[1] < 0) || //1 point coplanar, 
            (dist[2] == 0 && dist[0] > 0 && dist[1] > 0) || //1 point coplanar, 
            (dist[0] <= 0 && dist[1] <= 0 && dist[2] <= 0)     //3 points on one side
            )
            return;

         if (dist[0] >= 0 && dist[1] >= 0 && dist[2] >= 0)   //we're all on the positive side
         {
            results.Clear();
            resultUs.Clear();
            return;
         }

         

         Vector3 ray =Vector3.Empty;
         Vector3 orig =Vector3.Empty;
         
         float tVal=0;
         orig=verts[0];
         ray = BMathLib.Normalize(verts[1] - verts[0]);
         if (rayPlaneIntersect(plane, orig, ray, false, ref tVal))
         {
            results.Add(ray * tVal);
            resultUs.Add(tVal);
         }

         ray = BMathLib.Normalize(verts[2] - verts[0]);
         if (rayPlaneIntersect(plane, orig, ray, false, ref tVal))
         {
            results.Add(ray * tVal);
            resultUs.Add(tVal);
         }

         orig = verts[1];
         ray = BMathLib.Normalize(verts[2] - verts[1]);
         if (rayPlaneIntersect(plane, orig, ray, false, ref tVal))
         {
            results.Add(ray * tVal);
            resultUs.Add(tVal);
         }

         for(int i=0;i<results.Count;i++)
         {
            if(BMathLib.pointPlaneDistance(results[i],plane) >0)
            {
               results.RemoveAt(i);
               resultUs.RemoveAt(i);
               i--;
            }
            for (int c = i+1; c < results.Count; c++)
            {
               if (results[i]==results[c])
               {
                  results.RemoveAt(c);
                  resultUs.RemoveAt(c);
               }
            }
         }
         
      }

      #endregion

      #region linearAlgebra


      static public bool SolveCubic(float e, float f, float g, ref float t, ref float u, ref float v)
      {
         float p, q, h, rc, d, theta, costh3, sinth3;

          p = f - e * e / 3.0f;
          q = g - e * f / 3.0f + e * e * e * 2.0f / 27.0f;
          h = q * q / 4.0f + p * p * p / 27.0f;
          
          if (h > 0.0) 
          {
              return false; // only one real root
          }

          if ((h == 0.0) && (q == 0.0)) // all the same root
          {
              t = - e / 3;
              u = - e / 3;
              v = - e / 3;
              
              return true;
          }

          d = (float)Math.Sqrt(q * q / 4.0f - h);
          if (d < 0)
             rc = -(float)Math.Pow(-d, 1.0f / 3.0f);
          else
             rc = (float)Math.Pow(d, 1.0f / 3.0f);

          theta = (float)Math.Acos(-q / (2.0f * d));
          costh3 = (float)Math.Cos(theta / 3.0f);
          sinth3 = (float)(Math.Sqrt(3.0f) * Math.Sin(theta / 3.0f));
          t = 2.0f * rc * costh3 - e / 3.0f;  
          u = -rc * (costh3 + sinth3) - e / 3.0f;
          v = -rc * (costh3 - sinth3) - e / 3.0f;
          
          return true;
      }

      static public Vector3 CalculateEigenVector(float m11, float m12, float m13, float m22, float m23, float m33, float e)
      {
         Vector3 vTmp;
         float f1, f2, f3;

         vTmp.X = (float)(m12 * m23 - m13 * (m22 - e));
         vTmp.Y = (float)(m13 * m12 - m23 * (m11 - e));
         vTmp.Z = (float)((m11 - e) * (m22 - e) - m12 * m12);

         if ((vTmp.X == 0.0) && (vTmp.Y == 0.0) && (vTmp.Z == 0.0)) // planar or linear
         {
            // we only have one equation - find a valid one
            if ((m11 - e != 0.0) || (m12 != 0.0) || (m13 != 0.0))
            {
               f1 = m11 - e; f2 = m12; f3 = m13;
            }
            else if ((m12 != 0.0) || (m22 - e != 0.0) || (m23 != 0.0))
            {
               f1 = m12; f2 = m22 - e; f3 = m23;
            }
            else if ((m13 != 0.0) || (m23 != 0.0) || (m33 - e != 0.0))
            {
               f1 = m13; f2 = m23; f3 = m33 - e;
            }
            else
            {
               // error, we'll just make something up - we have NO context
               f1 = 1.0f; f2 = 0.0f; f3 = 0.0f;
            }

            if (f1 == 0.0f)
               vTmp.X = 0.0f;
            else
               vTmp.X = 1.0f;

            if (f2 == 0.0f)
               vTmp.Y = 0.0f;
            else
               vTmp.Y = 1.0f;

            if (f3 == 0.0f)
            {
               vTmp.Z = 0.0f;
               // recalculate y to make equation work
               if (m12 != 0.0f)
                  vTmp.Y = (float)(-f1 / f2);
            }
            else
            {
               vTmp.Z = (float)((f2 - f1) / f3);
            }
         }

         if (vTmp.LengthSq() > 1e-5f)
         {
            return Normalize(vTmp);
         }
         else
         {
            // Multiply by a value large enough to make the vector non-zero.
            vTmp *= 1e5f;
            return Normalize(vTmp);
         }
      }

      static public bool CalculateEigenVectors(float m11, float m12, float m13, float m22, float m23, float m33, float e1, float e2, float e3,
                                                ref Vector3 pV1, ref Vector3 pV2, ref Vector3 pV3)
      {
         Vector3 vTmp, vUp, vRight;

         bool v1z, v2z, v3z, e12, e13, e23;

         vUp.X = vUp.Z = 0.0f; vUp.Y = 1.0f;
         vRight.X = 1.0f; vRight.Y = vRight.Z = 0.0f;

         pV1 = CalculateEigenVector(m11, m12, m13, m22, m23, m33, e1);
         pV2 = CalculateEigenVector(m11, m12, m13, m22, m23, m33, e2);
         pV3 = CalculateEigenVector(m11, m12, m13, m22, m23, m33, e3);

         v1z = v2z = v3z = false;

         if ((pV1.X == 0.0) && (pV1.Y == 0.0) && (pV1.Z == 0.0)) v1z = true;
         if ((pV2.X == 0.0) && (pV2.Y == 0.0) && (pV2.Z == 0.0)) v2z = true;
         if ((pV3.X == 0.0) && (pV3.Y == 0.0) && (pV3.Z == 0.0)) v3z = true;

         e12 = (Math.Abs(Vector3.Dot(pV1, pV2)) > 0.1f); // check for non-orthogonal vectors
         e13 = (Math.Abs(Vector3.Dot(pV1, pV3)) > 0.1f);
         e23 = (Math.Abs(Vector3.Dot(pV2, pV3)) > 0.1f);

         if ((v1z && v2z && v3z) || (e12 && e13 && e23) ||
             (e12 && v3z) || (e13 && v2z) || (e23 && v1z)) // all eigenvectors are 0- any basis set
         {
            pV1.X = pV2.Y = pV3.Z = 1.0f;
            pV1.Y = pV1.Z = pV2.X = pV2.Z = pV3.X = pV3.Y = 0.0f;
            return true;
         }

         if (v1z && v2z)
         {
            vTmp = Vector3.Cross(vUp, pV3);
            if (vTmp.LengthSq() < 1e-5f)
            {
               vTmp = Vector3.Cross(vRight, pV3);
            }
            pV1 = Normalize(vTmp);
            pV2 = Vector3.Cross(pV3,pV1);
            return true;
         }

         if (v3z && v1z)
         {
            vTmp = Vector3.Cross(vUp, pV2);
            if (vTmp.LengthSq() < 1e-5f)
            {
               vTmp = Vector3.Cross(vRight, pV2);
            }
            pV3 = Normalize(vTmp);
            pV1 = Vector3.Cross(pV2, pV3);
            return true;
         }

         if (v2z && v3z)
         {
            vTmp = Vector3.Cross(vUp, pV1);
            if (vTmp.LengthSq() < 1e-5f)
            {
               vTmp = Vector3.Cross(vRight, pV1);
            }
            pV2 = Normalize(vTmp);
            pV3 = Vector3.Cross(pV1, pV2);
            return true;
         }

         if ((v1z) || e12)
         {
            pV1 = Vector3.Cross(pV2, pV3);
            return true;
         }

         if ((v2z) || e23)
         {
            pV2 = Vector3.Cross(pV3, pV1);
            return true;
         }

         if ((v3z) || e13)
         {
            pV3 = Vector3.Cross(pV1, pV2);
            return true;
         }

         return true;
      }

      static public bool CalculateEigenVectorsFromCovarianceMatrix(float Cxx, float Cyy, float Czz, float Cxy, float Cxz, float Cyz,
                                                              ref Vector3 pV1, ref Vector3 pV2, ref Vector3 pV3)
      {

         // Calculate the eigenvalues by solving a cubic equation.
         float e = -(Cxx + Cyy + Czz);
         float f = Cxx * Cyy + Cyy * Czz + Czz * Cxx - Cxy * Cxy - Cxz * Cxz - Cyz * Cyz;
         float g = Cxy * Cxy * Czz + Cxz * Cxz * Cyy + Cyz * Cyz * Cxx - Cxy * Cyz * Cxz * 2.0f - Cxx * Cyy * Czz;

         float ev1 = 0;
         float ev2 = 0;
         float ev3 = 0;

         if (!SolveCubic(e, f, g, ref ev1, ref ev2, ref ev3))
         {
            // set them to arbitrary orthonormal basis set
            pV1.X = pV2.Y = pV3.Z = 1.0f;
            pV1.Y = pV1.Z = pV2.X = pV2.Z = pV3.Z = pV3.Y = 0.0f;
            return false;
         }

         return CalculateEigenVectors(Cxx, Cxy, Cxz, Cyy, Cyz, Czz, ev1, ev2, ev3, ref pV1, ref pV2, ref pV3);
      }


     
      #endregion
   }
    

   #endregion

   #region math types
   public class float3
   {
      public float3()
      {
         X = 0;
         Y = 0;
         Z = 0;
      }
      public float3(float x, float y, float z)
      {
         X = x;
         Y = y;
         Z = z;
      }
      public float3(Vector3 inpt)
      {
         X = inpt.X;
         Y = inpt.Y;
         Z = inpt.Z;
      }
      ~float3()
      {
         xyz = null;
      }

      float[] xyz = new float[3];

      public static float3 Empty { get { return new float3(); } }

      public float X
      {
         get
         {
            return xyz[0];
         }
         set
         {
            xyz[0] = value;
         }
      }
      public float Y
      {
         get
         {
            return xyz[1];
         }
         set
         {
            xyz[1] = value;
         }
      }
      public float Z
      {
         get
         {
            return xyz[2];
         }
         set
         {
            xyz[2] = value;
         }
      }
      //-----------------------
      public static float3 operator -(float3 a, float3 b)
      {
         return new float3(a.X - b.X, a.Y - b.Y, a.Z - b.Z);
      }
      public static float3 operator +(float3 a, float3 b)
      {
         return new float3(a.X + b.X, a.Y + b.Y, a.Z + b.Z);
      }
      public static float3 operator *(float3 a, float3 b)
      {
         return new float3(a.X * b.X, a.Y * b.Y, a.Z * b.Z);
      }
      public static float3 operator *(float3 a, float b)
      {
         return new float3(a.X * b, a.Y * b, a.Z * b);
      }
      public static float3 operator /(float3 a, float3 b)
      {
         return new float3(a.X / b.X, a.Y / b.Y, a.Z / b.Z);
      }

      //-----------------------
      public float Length()
      {
         return (float)Math.Sqrt(LengthSq());
      }
      public float LengthSq()
      {
         return (X * X + Y * Y + Z * Z);
      }
      //-----------------------
      public Vector3 toVec3()
      {
         return new Vector3(X, Y, Z);
      }
      //-------------------------------
      // Static Globals
      public static float Dot(ref float3 a, ref float3 b)
      {
         return a.X * b.X + a.Y * b.Y + a.Z * b.Z;
      }
      public static float3 Normalize(float3 a)
      {
         float3 b = a;
         float len = a.Length();// Math.Sqrt((float)(b.X * b.X + b.Y * b.Y + b.Z * b.Z));
         if (Math.Abs(len) > 0.0001)
         {
            b.X /= len;
            b.Y /= len;
            b.Z /= len;
         }
         return b;
      }
      public static float Length(float3 a)
      {
         return (float)Math.Sqrt(a.LengthSq());
      }
      public static float LengthSq(float3 a)
      {
         return a.LengthSq();
      }
      static public float3 Cross(float3 a, float3 b)
      {
         float3 q = new float3();
         q.X = a.Y * b.Z - a.Z * b.Y;
         q.Y = a.Z * b.X - a.X * b.Z;
         q.Z = a.X * b.Y - a.Y * b.X;
         return q;
      }
      static public float3 Mul(float3 a, float3 b)
      {
         return new float3(a.X * b.X, a.Y * b.Y, a.Z * b.Z);
      }
      static public float3 Subtract(float3 a, float3 b)
      {
         return new float3(a.X - b.X, a.Y - b.Y, a.Z - b.Z);
      }
   };
   public class float2
   {
      public float2()
      {
         X = 0;
         Y = 0;
      }
      public float2(float x, float y)
      {
         X = x;
         Y = y;
      }

      ~float2()
      {
         xy = null;
      }
      float[] xy = new float[2];

      public float X
      {
         get
         {
            return xy[0];
         }
         set
         {
            xy[0] = value;
         }
      }
      public float Y
      {
         get
         {
            return xy[1];
         }
         set
         {
            xy[1] = value;
         }
      }

   };
   public unsafe class float16
   {
      private ushort mData;

      public float16()
      {
         mData = 0;
      }
      public float16(float f)
      {
         fromFloat(f, true);
      }
      public float16(float f, bool noAbnormalValues)
      {
         fromFloat(f, noAbnormalValues);
      }

      public void fromFloat(float f)
      {
         fromFloat(f, true);
      }
      public void fromFloat(float f, bool noAbnormalValues)
      {

         uint i = floatToUInt(f);

         uint sign = 0x8000 & (i >> 16);
         int exp = (int)((255 & (i >> 23)) - 112);
         uint man = unchecked((uint)0x7FFFFF) & i;

         // max. possible exponent value indicates Inf/NaN
         if (143 == exp)
         {
            if (noAbnormalValues)
            {
               mData= 0;
               return;
            }

            if (0 == man)
            {
               // output infinity
               mData= (ushort)(sign | 0x7C00);
               return;
            }
            else
            {
               // output NaN
               mData = (ushort)(sign | (man >> 13) | 0x7C00);
               return;
            }
         }
         else if (exp <= 0)
         {
            if (noAbnormalValues)
            {
               mData = 0;
               return ;
            }

            // too small
            if (exp < -10)
            {
               mData = (ushort)(sign);
               return;
            }

            // output denormal
            man = (man | 0x800000) >> (1 - exp);

            // Round 
            if ((man & 0x1000) != 0)
               man += 0x2000;

            mData = (ushort)(sign | (man >> 13));
            return;
         }

         // round
         if ((man & 0x1000) != 0)
         {
            man += 0x2000;
            if (man > 0x7FFFFF)
            {
               exp++;
               man = 0;
            }
         }

         if (exp > 30)
         {
            mData = (ushort)(sign | 0x7C00);
            return;
         }

         mData = (ushort)(sign | (exp << 10) | (man >> 13));
      }
      public float toFloat()
      {
         bool noAbnormalValues = true;
         ushort h = mData;

         uint sign = 0x80000000 & ((uint)h << 16);
         uint exp = (uint)(31 & (h >> 10));
         uint man = (uint)(1023 & h);

         if (exp == 0)
         {
            if (man == 0)
            {
               // +-0
               return IntToFloat((int)sign);
            }
            else
            {
               // normalize denormal
               while (0 == (man & 0x400))
               {
                  exp--;
                  man <<= 1;
               }

               exp++;
               man &= unchecked((uint)(~0x400));
            }
         }
         else if (exp == 31)
         {
            if (noAbnormalValues)
               return 0;

            if (man != 0)
            {
               // NaN
               return IntToFloat((int)(sign | (man << 13) | 0x7F800000));
            }
            else
            {
               // +-inf
               return IntToFloat((int)(sign | 0x7F800000));
            }
         }

         return IntToFloat((int)(sign | ((exp + 112) << 23) | (man << 13)));
      }
      public byte[] toByteArray()
      {
         return BitConverter.GetBytes(mData);
      }
      public ushort getInternalDat()
      {
         return mData;
      }

      //utils
      private unsafe uint floatToUInt(float f)
      {
         byte[] b = BitConverter.GetBytes(f);
         uint i = BitConverter.ToUInt32(b, 0);
         return i;
      }
      private unsafe float IntToFloat(int i)
      {
         byte[] b = BitConverter.GetBytes(i);
         float f = BitConverter.ToSingle(b, 0);
         return f;
      }


      //operators
      static public bool operator ==(float16 a, float16 b)
      {
         return b.mData == a.mData;
      }
      static public bool operator !=(float16 a, float16 b)
      {
         return b.mData != a.mData;
      }

      //http://www.cs.umd.edu/class/spring2003/cmsc311/Notes/BinMath/addFloat.html

   }
   public class floatN
   {
      public floatN(uint numDimentions)
      {
         mDimensions = numDimentions;
         mDataMembers = new float[mDimensions];
      }

      public static floatN Empty(uint numDimentions) 
      {
          return new floatN(numDimentions); 
      }


      float[] mDataMembers = null;
      uint mDimensions = 1;

      //members
      public float[] Array
      {
         get 
         {
            float[] k = new float[mDimensions];
            mDataMembers.CopyTo(k, 0);
            return k; 
         }
         set
         {
            if (value.Length == mDimensions)
            {
               value.CopyTo(mDataMembers, 0);
            }
            else
            {
               throw new ArgumentException("Dimentions do not match!");
            }
         }
      }
      public float this[uint index]
      {
         get
         {
            if(index < mDimensions)
            {
               return mDataMembers[index];
            }
            else
               throw new ArgumentException("Index out of range exception");
            
         }
         set
         {
            if (index < mDimensions)
            {
               mDataMembers[index] = value;
            }
            else
               throw new ArgumentException("Index out of range exception");
         }
      }
      public void set(float[] inArray)
      {
         if (inArray.Length != mDimensions)
            return;
         inArray.CopyTo(mDataMembers, 0);
      }
      public void set(List<float> inArray)
      {
         if (inArray.Count != mDimensions)
            return;
         inArray.CopyTo(mDataMembers, 0);
      }
         
      //-----------------------
      public static floatN operator -(floatN a, floatN b)
      {
         if(a.mDimensions != b.mDimensions)
            throw new ArgumentException("Dimensions of vectors do not match");

         floatN k = new floatN(a.mDimensions);
         for (uint i = 0; i < a.mDimensions; i++)
            k[i] = a[i] - b[i];
         return k;
      }
      public static floatN operator +(floatN a, floatN b)
      {
         if (a.mDimensions != b.mDimensions)
            throw new ArgumentException("Dimensions of vectors do not match");

         floatN k = new floatN(a.mDimensions);
         for (uint i = 0; i < a.mDimensions; i++)
            k[i] = a[i] + b[i];
         return k;
      }
      public static floatN operator *(floatN a, floatN b)
      {
         if (a.mDimensions != b.mDimensions)
            throw new ArgumentException("Dimensions of vectors do not match");

         floatN k = new floatN(a.mDimensions);
         for (uint i = 0; i < a.mDimensions; i++)
            k[i] = a[i] * b[i];
         return k;
      }
      public static floatN operator *(floatN a, float b)
      {
         floatN k = new floatN(a.mDimensions);
         for (uint i = 0; i < a.mDimensions; i++)
            k[i] = a[i] * b;
         return k;
      }
      public static floatN operator /(floatN a, float b)
      {
         floatN k = new floatN(a.mDimensions);
         for (uint i = 0; i < a.mDimensions; i++)
            k[i] = a[i] / b;
         return k;
      }
      public static floatN operator /(floatN a, floatN b)
      {
         if (a.mDimensions != b.mDimensions)
            throw new ArgumentException("Dimensions of vectors do not match");

         floatN k = new floatN(a.mDimensions);
         for (uint i = 0; i < a.mDimensions; i++)
            k[i] = a[i] / b[i];
         return k;
      }
      //-----------------------

      //operations
      public uint NumDimentions()
      {
         return mDimensions;
      }
      public float Length()
      {
         return (float)Math.Sqrt(LengthSq());
      }
      public float LengthSq()
      {
         float sum = 0;
         for (uint i = 0; i < mDimensions; i++)
            sum += mDataMembers[i] * mDataMembers[i];
         return sum;
      }

      //-------------------------------
      // Static Globals
      public static float Dot(ref floatN a, ref floatN b)
      {
         if (a.mDimensions != b.mDimensions)
            throw new ArgumentException("Dimensions of vectors do not match");

         float sum = 0;
         for (uint i = 0; i < a.mDimensions; i++)
            sum += a[i] * b[i];

         return sum;
      }
      public static floatN Normalize(floatN a)
      {
         floatN b = a;
         float len= a.Length();
        
         if (Math.Abs(len) > 0.0001)
         {
            float lenRCP = 1.0f / len;
            b = b * lenRCP;
         }
         return b;
      }
      public static floatN Sqrt(floatN a)
      {
         floatN b = a;
         for (uint i = 0; i < a.mDimensions; i++)
            b[i] = (float)Math.Sqrt(a[i]);
         return b;
      }
      public static floatN Pow(floatN a, int power)
      {
         floatN b = a;
         for (uint i = 0; i < a.mDimensions; i++)
            b[i] = (float)Math.Pow(a[i],power);
         return b;
      }
      public static float Length(floatN a)
      {
         return (float)Math.Sqrt(a.LengthSq());
      }
      public static float LengthSq(floatN a)
      {
         return a.LengthSq();
      }
      static public floatN Mul(floatN a, floatN b)
      {
         return a*b;
      }
      static public floatN Subtract(floatN a, floatN b)
      {
         return a-b;
      }

   };
   public class MatrixMxN
   {
      public MatrixMxN(uint numRows, uint numCols )
      {
         mColumns = numCols;
         mRows = numRows;
         mDataMembers = new float[numRows, numCols];
      }
      ~MatrixMxN()
      {
         mDataMembers = null;
      }
      //-------------------------------------
      public float this[uint row, uint col]
      {
         get
         {
            if (col < mColumns && row < mRows)
            {
               return mDataMembers[row, col];
            }
            else
               throw new ArgumentException("Index out of range exception");

         }
         set
         {
            if (col < mColumns && row < mRows)
            {
               mDataMembers[row, col] = value;
            }
            else
               throw new ArgumentException("Index out of range exception");
         }
      }
      //-------------------------------------
      float[,] mDataMembers;
      uint mColumns;    //width
      uint mRows;       //height
      //-------------------------------------
      public void set(float[,] inArray)
      {
         mRows = (uint)inArray.GetLength(0);
         mColumns = (uint)inArray.GetLength(1);
         mDataMembers = new float[mRows, mColumns];
         for (int y = 0; y < mRows; y++)
         {
            for (int x = 0; x < mColumns; x++)
            {
               mDataMembers[y, x] = inArray[y, x];
            }
         }
      }
      public void copyTo(MatrixMxN dstMat)
      {
         dstMat.set(mDataMembers);
      }
      //-----------------------
      public static MatrixMxN operator -(MatrixMxN a, MatrixMxN b)
      {
         if (a.mColumns != b.mColumns || a.mRows != b.mRows)
            throw new ArgumentException("Dimensions of vectors do not match");

         MatrixMxN t = new MatrixMxN(a.mRows, a.mColumns);
         for (uint i = 0; i < a.mRows; i++)
         {
            for (uint j = 0; j < a.mColumns; j++)
            {
               t[i, j] = a[i, j] - b[i, j];
            }
         }

         return t;
      }
      public static MatrixMxN operator +(MatrixMxN a, MatrixMxN b)
      {
         if (a.mColumns != b.mColumns || a.mRows != b.mRows)
            throw new ArgumentException("Dimensions of vectors do not match");

         MatrixMxN t = new MatrixMxN(a.mRows,a.mColumns);
         for (uint i = 0; i < a.mRows; i++)
         {
            for (uint j = 0; j < a.mColumns; j++)
            {
               t[i, j] = a[i, j] + b[i, j];
            }
         }

         return t;
      }
      public static MatrixMxN operator *(MatrixMxN a, float b)
      {
         MatrixMxN t = new MatrixMxN(a.mRows, a.mColumns);
         for (uint i = 0; i < a.mRows; i++)
         {
            for (uint j = 0; j < a.mColumns; j++)
            {
               t[i, j] = a[i, j] * b;
            }
         }

         return t;
      }
      public static floatN operator *(MatrixMxN a, floatN X)
      {
         if (a.mColumns != X.NumDimentions())
            throw new ArgumentException("Dimensions of Matricies do not match for matrix multiplication");

         floatN t = new floatN(a.mRows);
         for (uint i = 0; i < a.mRows; i++)
         {
            for (uint j = 0; j < a.mColumns; j++)
            {
               t[i] += X[j] * a[i,j];
            }
         }

         return t;
      }
      public static MatrixMxN operator *(MatrixMxN Left, MatrixMxN Right)
      {
         if (Left.mColumns != Right.mRows)
            throw new ArgumentException("Dimensions of Matricies do not match for matrix multiplication");

         MatrixMxN C = new MatrixMxN(Left.mRows, Right.mColumns);
         float val;

         /* iterate through each element of the result matrix */
         for (uint row = 0; row < C.mRows; row++)
         {
            for (uint col = 0; col < C.mColumns; col++)
            {
               /* multiply the corresponding row of left by the corresponding column of right */
               val = 0;
               for (uint pos = 0; pos < Left.mColumns; pos++)
                  val += Left[row, pos] * Right[pos, col];
               C[row, col] = val;
            }
         }

         return C;
      }
      public static MatrixMxN operator /(MatrixMxN a, float b)
      {
         MatrixMxN t = new MatrixMxN(a.mRows,a.mColumns);
         for (uint i = 0; i < a.mRows; i++)
         {
            for (uint j = 0; j < a.mColumns; j++)
            {
               t[i, j] = a[i, j] / b;
            }
         }

         return t;
      }
      //-------------------------------------
      public static MatrixMxN Empty(uint numRows, uint numCols)
      {
         return new MatrixMxN(numRows, numCols);
      }
      public static MatrixMxN Identity(uint numRows, uint numCols)
      {
         MatrixMxN T = new MatrixMxN(numRows, numCols);
         T.Identity();
         return T;
      }
      //-------------------------------------
      static public MatrixMxN sub(MatrixMxN inMat, float scalar)
      {
         MatrixMxN bmat = new MatrixMxN(inMat.mRows,inMat.mColumns);
         inMat.copyTo(bmat);
         bmat.sub(scalar);
         return bmat;
      }
      static public MatrixMxN mul(MatrixMxN Left, MatrixMxN Right)
      {
         return Left * Right;
      }
      static public MatrixMxN mul(MatrixMxN a, float b)
      {
         return a * b;
      }
      static public floatN mul(MatrixMxN a, floatN X)
      {
         return a * X;
      }
      static public void Transpose(MatrixMxN src,ref MatrixMxN dst)
      {
         dst = Transpose(src);
      }
      static public MatrixMxN Transpose(MatrixMxN src)
      {
         MatrixMxN t = new MatrixMxN(src.mRows, src.mColumns);
         src.copyTo(t);
         t.Transpose();
         return t;
      }
      static public MatrixMxN Minor(MatrixMxN matrix, int iRow, int iCol)
      {
         MatrixMxN minor = new MatrixMxN(matrix.mRows - 1, matrix.mColumns - 1);
         uint m = 0, n = 0;
         for (uint i = 0; i < matrix.mRows; i++)
         {
            if (i == iRow)
               continue;
            n = 0;
            for (uint j = 0; j < matrix.mColumns; j++)
            {
               if (j == iCol)
                  continue;
               minor[m, n] = matrix[i, j];
               n++;
            }
            m++;
         }
         return minor;
      }
      static public void calculateEigenSystem(MatrixMxN A, List<float> eigenValues, List<floatN> eigenVectors)
      {
         //ugg, can't find anything better, use the POWER METHOD
         uint numValues = A.mColumns;
         MatrixMxN B = new MatrixMxN(A.mRows, A.mColumns);
         MatrixMxN I = MatrixMxN.Identity(A.mRows, A.mColumns);
         A.copyTo(B);
         for(uint f=0;f<numValues;f++)
         {
            floatN X = new floatN(B.mColumns);
            for (uint i = 0; i < B.mColumns; i++)
               X[i] = 1.0f;

            float lastEigenValue = 0;
            float currEigenValue = float.MaxValue;
            for (int k = 1; k < 100; k++)
            {
               floatN Y = B * X;

               float maxComp = Y[0];
               for (uint t = 0; t < Y.NumDimentions(); t++)
                  if (Math.Abs(Y[t]) > Math.Abs(maxComp))
                     maxComp = Y[t];

               currEigenValue = maxComp;

               X = Y * (1.0f / currEigenValue);


               if (Math.Abs(currEigenValue - lastEigenValue) < BMathLib.cTinyEpsilon)
                  break;
               lastEigenValue = currEigenValue;
            }
            eigenValues.Add(currEigenValue);
            eigenVectors.Add(X);

            B = A - (I * currEigenValue);
         }
      }
      //-------------------------------------
      public void Identity()
      {
         for (int x = 0; x < mRows; x++)
         {
            for (int z = 0; z < mColumns; z++)
            {
               if (x == z)
                  mDataMembers[x, z] = 1;
               else
                  mDataMembers[x, z] = 0;
            }
         }
      }
      public void Empty()
      {
         for (int x = 0; x < mRows; x++)
         {
            for (int z = 0; z < mColumns; z++)
            {
               mDataMembers[x, z] = 0;
            }
         }
      }
      public void Transpose()
      {
         MatrixMxN t = new MatrixMxN(mRows, mColumns);
         copyTo(t);
         for (uint x = 0; x < mRows; x++)
         {
            for (uint z = 0; z < mColumns; z++)
            {
               mDataMembers[x, z] = t[z, x];
            }
         }
         t = null;
      }
      public void mul(float scale)
      {
         for (uint i = 0; i < mRows; i++)
         {
            for (uint j = 0; j < mColumns; j++)
            {
               mDataMembers[i, j] *= scale;
            }
         }
      }
      public void mul(MatrixMxN mat)
      {
         MatrixMxN C = MatrixMxN.mul(this, mat);
         C.copyTo(this);
         C = null;
      }
      public void sub(float scale)
      {
         for (uint i = 0; i < mRows; i++)
         {
            for (uint j = 0; j < mColumns; j++)
            {
               mDataMembers[i, j] -= scale;
            }
         }
      }

      

      public void decompositionQR(ref MatrixMxN q, ref MatrixMxN r)
      {
         MatrixMxN q_copy  = new MatrixMxN(mRows, mColumns);
         MatrixMxN r_copy  = new MatrixMxN(mRows, mColumns);
         MatrixMxN R       = new MatrixMxN(mRows, mColumns);
         MatrixMxN Rt      = new MatrixMxN(mRows, mColumns);

         q.Identity();
         copyTo(r);

         uint row, col;
         float x, y, xydist, negsine, cosine;

          /* find each of the Givens rotation matrices */
         for (col = 0; col < mColumns - 1; col++)
          {
             for (row = col + 1; row < mRows; row++)
              {
                  /* only compute a rotation matrix if the corresponding entry in
                     the matrix is nonzero */
                  y = r[row, col];
                  if (y != 0.0)
                  {
                      /* find the values of -sin and cos */
                      x = r[col, col];
                      xydist = (float)(Math.Sqrt(x * x + y * y));
                      negsine = y / xydist;
                      cosine = x / xydist;
                      
                      /* calculate the R and R^t matrices */
                      R.Identity();
                      R[col, col] = cosine;
                      R[col, row] = negsine;
                      R[row, col] = -negsine;
                      R[row, row] = cosine;
                      MatrixMxN.Transpose(R, ref Rt);

                      /* "add" R to the q and r matrices */
                      q.copyTo(q_copy);
                      r.copyTo(r_copy);
                      q = MatrixMxN.mul(q_copy, Rt);
                      r = MatrixMxN.mul(R, r_copy);
                  }
              }
          }

          /* free memory */
          q_copy = null;
          r_copy = null;
          R = null;
          Rt = null;
      }
      public float reduceToRowEchelonForm()
      {
        int B, C, X;
        float Pivot, Temp;
        float Determinant;
        int PivotRow, PivotCol;

        PivotRow = 0;
        PivotCol = 0;
        Determinant = 1.0f;
        do
        {
          // Find largest pivot.  Search for a number below
          // the current pivot with a greater absolute value.
          X = PivotRow;
          for(C=PivotRow; C<mRows; C++)
            if(Math.Abs(mDataMembers[C, PivotCol])>Math.Abs(mDataMembers[X, PivotCol]))
              X = C;

          if(X != PivotRow)
          {
            // If here, there is a better pivot choice somewhere
            // below the current pivot.
            // Interchange the pivot row with the row
            // containing the largest pivot, X.
            for(B=0; B<mColumns; B++)
            {
              Temp = mDataMembers[PivotRow, B];
              mDataMembers[PivotRow, B]= mDataMembers[X, B];
              mDataMembers[X, B]= Temp;
            }

            Determinant = -Determinant;
          }

          Pivot = mDataMembers[PivotRow, PivotCol];
          Determinant = Pivot*Determinant;

          if(Pivot!= 0.0)
          {
            // Introduce a '1' at the pivot point
            for(B=0; B<mColumns; B++)
              mDataMembers[PivotRow, B]=mDataMembers[PivotRow, B]/Pivot;

            for(B=0; B<mRows; B++)
            {
              // Eliminate (make zero) all elements above
              // and below the pivot.
              // Skip over the pivot row when we come to it.
              if(B != PivotRow)
              {
                Pivot = mDataMembers[B, PivotCol];
                for(C=PivotRow; C<mColumns; C++)
                  mDataMembers[B, C]= mDataMembers[B,C]-Pivot*mDataMembers[PivotRow, C];
              }
            }
            PivotRow++; // Next row
          }
          PivotCol++;  // Next column
        }
        while((PivotRow<mRows)&&(PivotCol<mColumns)); // Reached an edge yet?

        return Determinant;
      
      }
      
      
   };
   #endregion

   #region SPLINE

   public class RoundNonuniformSpline
   {
      public void init()
      {
      }

      // adds node and updates segment length
      public void addNode(Vector3 pos)
      {
        if (mNodes.Count == 0)
          mMaxDistance = 0.0f;
        else
        {
           mNodes[mNodes.Count - 1].mDistance = Vector3.Length(mNodes[mNodes.Count - 1].mPosition - pos);
           mMaxDistance += mNodes[mNodes.Count - 1].mDistance;
        }
        splineControlNode scn = new splineControlNode();
        scn.mPosition = pos;
        mNodes.Add(scn);
        
      }

      // called after all nodes added. This function calculates the node velocities
      public virtual void buildSpline()
      {
        for (int i = 1; i<mNodes.Count-1; i++)
        {
           // split the angle 
           mNodes[i].mVelocity = BMathLib.Normalize(mNodes[i + 1].mPosition - mNodes[i].mPosition) -
                              BMathLib.Normalize(mNodes[i - 1].mPosition - mNodes[i].mPosition);
           mNodes[i].mVelocity.Normalize();
        }
        // calculate start and end velocities
        mNodes[0].mVelocity = getStartVelocity(0);
        mNodes[mNodes.Count - 1].mVelocity = getEndVelocity(mNodes.Count - 1);
      }


      // spline access function. time is 0 -> 1
      public Vector3 getPosition(float time)
      {
         time = BMathLib.Saturate(time);
        float distance = time * mMaxDistance;
        float currentDistance = 0.0f;
        int i = 0;
        while (currentDistance + mNodes[i].mDistance < distance)
        {
          currentDistance += mNodes[i].mDistance;
          i++;
        }
        float t = distance - currentDistance;
        t /= mNodes[i].mDistance; // scale t in range 0 - 1
        Vector3 startVel = mNodes[i].mVelocity * mNodes[i].mDistance;
        Vector3 endVel = mNodes[i+1].mVelocity * mNodes[i].mDistance;

        return GetPositionOnCubic(mNodes[i].mPosition, startVel,
                               mNodes[i+1].mPosition, endVel, t);
      }
      public Vector3 getTangent(float time, float timeDelta)
      {
         
         Vector3 tang = Vector3.Empty;
         Vector3 myPos = getPosition(time);

         Vector3 nextPos = getPosition(time + timeDelta);   
         if (time==1)
            nextPos = -getPosition(time - timeDelta);
         
         Vector3 dir = BMathLib.Normalize(nextPos - myPos);

         tang = BMathLib.Cross(dir, BMathLib.unitY);

         return tang;
      }

      public float getTimeAtNode(int nodeIndex)
      {
         if (nodeIndex < 0 || nodeIndex >= mNodes.Count)
            return 0;
         //sum the distances up to this point..
         float currentDistance = 0.0f;
         for (int i = 0; i < nodeIndex; i++)
            currentDistance += mNodes[i].mDistance;

         return currentDistance / mMaxDistance;
      }
      
      // internal. Based on Equation 14 
      protected Vector3 getStartVelocity(int index)
      {
        Vector3 temp = 3.0f*(mNodes[index+1].mPosition - mNodes[index].mPosition)  * (1.0f/mNodes[index].mDistance);
        return (temp - mNodes[index+1].mVelocity)*0.5f;
      }

      // internal. Based on Equation 15 
      protected Vector3 getEndVelocity(int index)
      {
        Vector3 temp = 3.0f*(mNodes[index].mPosition - mNodes[index-1].mPosition) * (1.0f/mNodes[index-1].mDistance);
        return (temp - mNodes[index-1].mVelocity)*0.5f;
      }

      // cubic curve defined by 2 positions and 2 velocities
      protected Vector3 GetPositionOnCubic(Vector3 startPos, Vector3 startVel, Vector3 endPos, Vector3 endVel, float time)
      {
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

         m = Matrix.Multiply(hermite,m);

        Vector4 timeVector = new Vector4(time*time*time, time*time, time, 1.0f);
         Vector4 result = Vector4.Transform(timeVector, m); // vector * matrix
         return new Vector3(result.X, result.Y, result.Z);
      }

      //--------------------
      protected class splineControlNode
      {
         public Vector3 mPosition;
         public Vector3 mVelocity;
         public float mDistance;
      };

      protected List<splineControlNode> mNodes = new List<splineControlNode>();
      protected float mMaxDistance;
   };

   public class SmoothNonuniformSpline : RoundNonuniformSpline
   {

      public override void buildSpline()
      {
         base.buildSpline();
         Smooth(); Smooth(); Smooth();
      }
     
     void Smooth()
      {
        Vector3 newVel;
        Vector3 oldVel = getStartVelocity(0);
        for (int i = 1; i < mNodes.Count - 1; i++)
        {
          // Equation 12
          newVel = getEndVelocity(i)*mNodes[i].mDistance +
                   getStartVelocity(i) * mNodes[i - 1].mDistance;
          newVel *= 1.0f / (mNodes[i - 1].mDistance + mNodes[i].mDistance);
          mNodes[i - 1].mVelocity = oldVel;
          oldVel = newVel;
        }
        mNodes[mNodes.Count - 1].mVelocity = getEndVelocity(mNodes.Count - 1);
        mNodes[mNodes.Count - 2].mVelocity = oldVel;
      }
   };


   #endregion

   #region SORT & SEARCH

   public class Sort
   {
      #region interfaces
      public interface ISorter
      {
         void Sort(IList list);
      }
      public interface ISwap
      {
         void Swap(IList array, int left, int right);
         void Set(IList array, int left, int right);
         void Set(IList array, int left, object obj);
      }
      public class ComparableComparer : IComparer
      {
         public int Compare(IComparable x, Object y)
         {
            return x.CompareTo(y);
         }

         int IComparer.Compare(Object x, Object y)
         {
            return this.Compare((IComparable)x, y);
         }
      }
      public class DefaultSwap : ISwap
      {
         public void Swap(IList array, int left, int right)
         {
            object swap = array[left];
            array[left] = array[right];
            array[right] = swap;
         }

         public void Set(IList array, int left, int right)
         {
            array[left] = array[right];
         }

         public void Set(IList array, int left, object obj)
         {
            array[left] = obj;
         }
      }
      public abstract class SwapSorter : ISorter
      {
         private IComparer comparer;
         private ISwap swapper;

         public SwapSorter()
         {
            this.comparer = new ComparableComparer();
            this.swapper = new DefaultSwap();
         }

         public SwapSorter(IComparer comparer, ISwap swapper)
         {
            if (comparer == null)
               throw new ArgumentNullException("comparer");
            if (swapper == null)
               throw new ArgumentNullException("swapper");

            this.comparer = comparer;
            this.swapper = swapper;
         }

         /// <summary>
         /// Gets or sets the <see cref="IComparer"/> object
         /// </summary>
         /// <value>
         /// Comparer object
         /// </value>
         /// <exception cref="ArgumentNullException">
         /// Set property, the value is a null reference
         /// </exception>
         public IComparer Comparer
         {
            get
            {
               return this.comparer;
            }
            set
            {
               if (value == null)
                  throw new ArgumentNullException("comparer");
               this.comparer = value;
            }
         }

         /// <summary>
         /// Gets or set the swapper object
         /// </summary>
         /// <value>
         /// The <see cref="ISwap"/> swapper.
         /// </value>
         /// <exception cref="ArgumentNullException">Swapper is a null reference</exception>
         public ISwap Swapper
         {
            get
            {
               return this.swapper;
            }
            set
            {
               if (value == null)
                  throw new ArgumentNullException("swapper");
               this.swapper = value;
            }
         }

         public abstract void Sort(IList list);
      }

      #endregion
      #region BUBBLESORT
      /// <summary>
      /// Bubble sort sequential algorithm
      /// </summary>
      /// <remarks>
      /// <para>
      /// Bubble sort is a sequential sorting algorithm that runs in
      /// <em>O(n^2)</em>, where <em>n</em> is the number of elements in the 
      /// list.
      /// </para>
      /// <para>
      /// Source: <a href="http://www.cs.rit.edu/~atk/Java/Sorting/sorting.html">
      /// http://www.cs.rit.edu/~atk/Java/Sorting/sorting.html</a>
      /// </para>
      /// </remarks>
      public class BubbleSorter : SwapSorter
      {
         public BubbleSorter()
            : base()
         {
         }

         public BubbleSorter(IComparer comparer, ISwap swapper)
            : base(comparer, swapper)
         {
         }

         public override void Sort(IList list)
         {
            for (int i = list.Count; --i >= 0; )
            {
               for (int j = 0; j < i; j++)
               {
                  if (Comparer.Compare(list[j], list[j + 1]) > 0)
                     Swapper.Swap(list, j, j + 1);
               }
            }
         }
      }
      #endregion
      #region BIDIRECTIONALBUBBLESORT
      public class BiDirectionalBubbleSort : SwapSorter
      {
         public BiDirectionalBubbleSort() : base() { }

         public BiDirectionalBubbleSort(IComparer comparer, ISwap swapper)
            : base(comparer, swapper)
         {
         }

         public override void Sort(IList list)
         {
            int j;
            int limit;
            int st;
            bool flipped;

            st = -1;
            limit = list.Count;
            flipped = true;
            while (st < limit & flipped)
            {
               flipped = false;
               st++;
               limit--;
               for (j = st; j < limit; j++)
               {
                  if (Comparer.Compare(list[j], list[j + 1]) > 0)
                  {
                     Swapper.Swap(list, j, j + 1);
                     flipped = true;
                  }
               }

               if (flipped)
               {
                  for (j = limit - 1; j >= st; j--)
                  {
                     if (Comparer.Compare(list[j], list[j + 1]) > 0)
                     {
                        Swapper.Swap(list, j, j + 1);
                        flipped = true;
                     }
                  }
               }
            }
         }
      }
      #endregion
      #region SHELLSORT
      public class ShellSort : SwapSorter
      {
         public ShellSort() : base() { }

         public ShellSort(IComparer comparer, ISwap swapper)
            : base(comparer, swapper)
         { }

         public override void Sort(IList list)
         {
            int h;
            int i;
            int j;
            object b;
            bool loop = true;

            h = 1;

            while (h * 3 + 1 <= list.Count)
            {
               h = 3 * h + 1;
            }
            while (h > 0)
            {
               for (i = h - 1; i < list.Count; i++)
               {
                  b = list[i];
                  j = i;
                  loop = true;
                  while (loop)
                  {
                     if (j >= h)
                     {
                        if (Comparer.Compare(list[j - h], b) > 0)
                        {
                           Swapper.Set(list, j, j - h);
                           j = j - h;
                        }
                        else
                        {
                           loop = false;
                        }
                     }
                     else
                     {
                        loop = false;
                     }
                  }
                  Swapper.Set(list, j, b);
               }
               h = h / 3;
            }
         }
      }
      #endregion
      #region SELECTIONSORT
      public class SelectionSort : SwapSorter
      {
         public SelectionSort() : base() { }

         public SelectionSort(IComparer comparer, ISwap swapper)
            : base(comparer, swapper)
         {
         }

         public override void Sort(IList list)
         {
            int i;
            int j;
            int min;

            for (i = 0; i < list.Count; i++)
            {
               min = i;
               for (j = i + 1; j < list.Count; j++)
               {
                  if (Comparer.Compare(list[j], list[min]) < 0)
                  {
                     min = j;
                  }
               }
               Swapper.Swap(list, min, i);
            }
         }
      }
      #endregion
      #region QUICKSORT

      public class QuickSorter : SwapSorter
      {
         public QuickSorter()
            : base()
         { }

         public QuickSorter(IComparer comparer, ISwap swapper)
            : base(comparer, swapper)
         { }

         /// <summary>
         /// Sorts the array.
         /// </summary>
         /// <param name="array">The array to sort.</param>
         public override void Sort(IList array)
         {
            Sort(array, 0, array.Count - 1);
         }

         public void Sort(IList array, int lower, int upper)
         {
            // Check for non-base case
            if (lower < upper)
            {
               // Split and sort partitions
               int split = Pivot(array, lower, upper);
               Sort(array, lower, split - 1);
               Sort(array, split + 1, upper);
            }
         }

         #region Internal
         internal int Pivot(IList array, int lower, int upper)
         {
            // Pivot with first element
            int left = lower + 1;
            object pivot = array[lower];
            int right = upper;

            // Partition array elements
            while (left <= right)
            {
               // Find item out of place
               while ((left <= right) && (Comparer.Compare(array[left], pivot) <= 0))
               {
                  ++left;
               }

               while ((left <= right) && (Comparer.Compare(array[right], pivot) > 0))
               {
                  --right;
               }

               // Swap values if necessary
               if (left < right)
               {
                  Swapper.Swap(array, left, right);
                  ++left;
                  --right;
               }
            }

            // Move pivot element
            Swapper.Swap(array, lower, right);
            return right;
         }
         #endregion
      }
      #endregion
      #region FASTQUICKSORT
      public class FastQuickSorter : SwapSorter
      {
         public FastQuickSorter()
            : base()
         { }

         public FastQuickSorter(IComparer comparer, ISwap swapper)
            : base(comparer, swapper)
         { }

         public override void Sort(IList list)
         {
            QuickSort(list, 0, list.Count - 1);
            InsertionSort(list, 0, list.Count - 1);
         }

         #region Internal
         /// <summary>
         /// This is a generic version of C.A.R Hoare's Quick Sort 
         /// algorithm.  This will handle arrays that are already
         /// sorted, and arrays with duplicate keys.
         /// </summary>
         /// <remarks>
         /// If you think of a one dimensional array as going from
         /// the lowest index on the left to the highest index on the right
         /// then the parameters to this function are lowest index or
         /// left and highest index or right.  The first time you call
         /// this function it will be with the parameters 0, a.length - 1.
         /// </remarks>
         /// <param name="list">list to sort</param>
         /// <param name="lo0">left boundary of array partition</param>
         /// <param name="hi0">right boundary of array partition</param>
         internal void QuickSort(IList list, int l, int r)
         {
            int M = 4;
            int i;
            int j;
            Object v;

            if ((r - l) > M)
            {
               i = (r + l) / 2;
               if (Comparer.Compare(list[l], list[i]) > 0)
                  Swapper.Swap(list, l, i);     // Tri-Median Methode!
               if (Comparer.Compare(list[l], list[r]) > 0)
                  Swapper.Swap(list, l, r);
               if (Comparer.Compare(list[i], list[r]) > 0)
                  Swapper.Swap(list, i, r);

               j = r - 1;
               Swapper.Swap(list, i, j);
               i = l;
               v = list[j];
               for (; ; )
               {
                  while (Comparer.Compare(list[++i], v) > 0)
                  { }

                  while (Comparer.Compare(list[--j], v) < 0)
                  { }

                  if (j < i)
                     break;
                  Swapper.Swap(list, i, j);
               }
               Swapper.Swap(list, i, r - 1);
               QuickSort(list, l, j);
               QuickSort(list, i + 1, r);
            }
         }


         internal void InsertionSort(IList list, int lo0, int hi0)
         {
            int i;
            int j;
            Object v;

            for (i = lo0 + 1; i <= hi0; i++)
            {
               v = list[i];
               j = i;
               while ((j > lo0) && (Comparer.Compare(list[j - 1], v) > 0))
               {
                  Swapper.Set(list, j, j - 1);
                  j--;
               }
               list[j] = v;
            }
         }
         #endregion

      }
      #endregion
      #region SHAKERSORT
      public class ShakerSort : SwapSorter
      {
         public ShakerSort() : base() { }

         public ShakerSort(IComparer comparer, ISwap swapper)
            : base(comparer, swapper)
         {
         }

         public override void Sort(IList list)
         {
            int i;
            int j;
            int k;
            int min;
            int max;

            i = 0;
            k = list.Count - 1;
            while (i < k)
            {
               min = i;
               max = i;
               for (j = i + 1; j <= k; j++)
               {
                  if (Comparer.Compare(list[j], list[min]) < 0)
                  {
                     min = j;
                  }
                  if (Comparer.Compare(list[j], list[max]) > 0)
                  {
                     max = j;
                  }
               }
               Swapper.Swap(list, min, i);
               if (max == i)
               {
                  Swapper.Swap(list, min, k);
               }
               else
               {
                  Swapper.Swap(list, max, k);
               }
               i++;
               k--;
            }
         }
      }
      #endregion
      #region SHEARSORT
      /// <summary>
      /// Shear sort parralel algorithm
      /// </summary>
      /// <remarks>
      /// <para>
      /// Source: <a href="http://www.cs.rit.edu/~atk/Java/Sorting/sorting.html">
      /// http://www.cs.rit.edu/~atk/Java/Sorting/sorting.html</a>
      /// </para>
      /// </remarks>
      public class ShearSorter : SwapSorter
      {
         private int rows;
         private int cols;
         private int log;

         public ShearSorter()
            : base()
         {
         }

         public ShearSorter(IComparer comparer, ISwap swapper)
            : base(comparer, swapper)
         {
         }

         public override void Sort(IList list)
         {
            int pow = 1, div = 1;

            for (int i = 1; i * i <= list.Count; ++i)
            {
               if (list.Count % i == 0)
                  div = i;
            }

            this.rows = div;
            this.cols = list.Count / div;
            for (this.log = 0; pow <= this.rows; ++this.log)
               pow = pow * 2;

            int[] h = new int[this.rows];
            for (int i = 0; i < this.rows; ++i)
               h[i] = i * this.cols;

            for (int k = 0; k < this.log; ++k)
            {
               for (int j = 0; j < this.cols / 2; ++j)
               {
                  for (int i = 0; i < this.rows; i++)
                     SortPart1(list, i * this.cols, (i + 1) * this.cols, 1, (i % 2 == 0 ? true : false));
                  for (int i = 0; i < this.rows; i++)
                     SortPart2(list, i * this.cols, (i + 1) * this.cols, 1, (i % 2 == 0 ? true : false));
               }
               for (int j = 0; j < this.rows / 2; j++)
               {
                  for (int i = 0; i < this.cols; i++)
                     SortPart1(list, i, this.rows * this.cols + i, this.cols, true);
                  for (int i = 0; i < this.cols; i++)
                     SortPart2(list, i, this.rows * this.cols + i, this.cols, true);
               }
            }
            for (int j = 0; j < this.cols / 2; j++)
            {
               for (int i = 0; i < this.rows; i++)
                  SortPart1(list, i * this.cols, (i + 1) * this.cols, 1, true);
               for (int i = 0; i < this.rows; i++)
                  SortPart2(list, i * this.cols, (i + 1) * this.cols, 1, true);
            }
            for (int i = 0; i < this.rows; i++)
               h[i] = -1;
         }

         internal void SortPart1(IList list, int Lo, int Hi, int Nx, bool Up)
         {
            int c;
            for (int j = Lo; j + Nx < Hi; j += 2 * Nx)
            {
               c = Comparer.Compare(list[j], list[j + Nx]);
               if ((Up && c > 0) || !Up && c < 0)
                  Swapper.Swap(list, j, j + Nx);
            }
         }

         internal void SortPart2(IList list, int Lo, int Hi, int Nx, bool Up)
         {
            int c;
            for (int j = Lo + Nx; j + Nx < Hi; j += 2 * Nx)
            {
               c = Comparer.Compare(list[j], list[j + Nx]);
               if ((Up && c > 0) || !Up && c < 0)
                  Swapper.Swap(list, j, j + Nx);
            }
         }

      }
      #endregion
      #region EVENODDSEARCH
      /// <summary>
      /// Odd-Even Transport sort parralel algorithm
      /// </summary>
      /// <remarks>
      /// <para>
      /// Source: <a href="http://www.cs.rit.edu/~atk/Java/Sorting/sorting.html">
      /// http://www.cs.rit.edu/~atk/Java/Sorting/sorting.html</a>
      /// </para>
      /// </remarks>
      public class OddEvenTransportSorter : SwapSorter
      {
         public OddEvenTransportSorter()
            : base()
         {
         }

         public OddEvenTransportSorter(IComparer comparer, ISwap swapper)
            : base(comparer, swapper)
         {
         }

         public override void Sort(IList list)
         {
            for (int i = 0; i < list.Count / 2; ++i)
            {
               for (int j = 0; j + 1 < list.Count; j += 2)
               {
                  if (Comparer.Compare(list[j], list[j + 1]) > 0)
                     Swapper.Swap(list, j, j + 1);
               }

               for (int j = 1; j + 1 < list.Count; j += 2)
               {
                  if (Comparer.Compare(list[j], list[j + 1]) > 0)
                     Swapper.Swap(list, j, j + 1);
               }
            }
         }
      }
      #endregion
      #region INPLACEMERGESORT
      public class InPlaceMergeSort : SwapSorter
      {
         public InPlaceMergeSort() : base() { }

         public InPlaceMergeSort(IComparer comparer, ISwap swapper)
            : base(comparer, swapper)
         { }

         public override void Sort(IList list)
         {
            Sort(list, 0, list.Count - 1);
         }

         private void Sort(IList list, int fromPos, int toPos)
         {
            int end_low;
            int start_high;
            int i;
            object tmp;
            int mid;

            if (fromPos < toPos)
            {
               mid = (fromPos + toPos) / 2;

               Sort(list, fromPos, mid);
               Sort(list, mid + 1, toPos);

               end_low = mid;
               start_high = mid + 1;

               while (fromPos <= end_low & start_high <= toPos)
               {
                  if (Comparer.Compare(list[fromPos], list[start_high]) < 0)
                  {
                     fromPos++;
                  }
                  else
                  {
                     tmp = list[start_high];
                     for (i = start_high - 1; i >= fromPos; i--)
                     {
                        Swapper.Set(list, i + 1, list[i]);
                     }
                     Swapper.Set(list, fromPos, tmp);
                     fromPos++;
                     end_low++;
                     start_high++;
                  }
               }
            }
         }
      }
      #endregion
      #region HEAPSORT
      public class HeapSort : SwapSorter
      {
         public HeapSort() : base() { }

         public HeapSort(IComparer comparer, ISwap swapper)
            : base(comparer, swapper)
         {
         }

         public override void Sort(IList list)
         {
            int n;
            int i;

            n = list.Count;
            for (i = n / 2; i > 0; i--)
            {
               DownHeap(list, i, n);
            }
            do
            {
               Swapper.Swap(list, 0, n - 1);
               n = n - 1;
               DownHeap(list, 1, n);
            } while (n > 1);
         }

         private void DownHeap(IList list, int k, int n)
         {
            int j;
            bool loop = true;

            while ((k <= n / 2) && loop)
            {
               j = k + k;
               if (j < n)
               {
                  if (Comparer.Compare(list[j - 1], list[j]) < 0)
                  {
                     j++;
                  }
               }
               if (Comparer.Compare(list[k - 1], list[j - 1]) >= 0)
               {
                  loop = false;
               }
               else
               {
                  Swapper.Swap(list, k - 1, j - 1);
                  k = j;
               }
            }
         }
      }
      #endregion
      #region INSERTIONSORT
      public class InsertionSort : SwapSorter
      {
         public InsertionSort() : base() { }

         public InsertionSort(IComparer comparer, ISwap swapper)
            : base(comparer, swapper)
         { }

         public override void Sort(IList list)
         {
            int i;
            int j;
            object b;

            for (i = 1; i < list.Count; i++)
            {
               j = i;
               b = list[i];
               while ((j > 0) && (Comparer.Compare(list[j - 1], b) > 0))
               {
                  Swapper.Set(list, j, list[j - 1]);
                  --j;
               }
               Swapper.Set(list, j, b);
            }
         }
      }
      #endregion
      #region RADIXSORT

      public class RadixSort
      {


         // ================================================================================================
         // flip a float for sorting
         //  finds SIGN of fp number.
         //  if it's 1 (negative float), it flips all bits
         //  if it's 0 (positive float), it flips the sign only
         // ================================================================================================
         UInt32 FloatFlip(UInt32 f)
         {
            UInt32 mask = (UInt32)(-(Int32)(f >> 31) | 0x80000000);
            return f ^ mask;
         }

         void FloatFlipX(ref UInt32 f)
         {
            UInt32 mask = (UInt32)(-(Int32)(f >> 31) | 0x80000000);
            f ^= mask;
         }
         // ================================================================================================
         // flip a float back (invert FloatFlip)
         //  signed was flipped from above, so:
         //  if sign is 1 (negative), it flips the sign bit back
         //  if sign is 0 (positive), it flips all bits back
         // ================================================================================================
         UInt32 IFloatFlip(UInt32 f)
         {
            UInt32 mask = (UInt32)(((f >> 31) - 1) | 0x80000000);
            return f ^ mask;
         }

         public unsafe void Sort(float[] farray)
         {
            //static void RadixSort11(real32 *farray,real32 *buffer, UInt32 elements,int* order,int *tempOrder)

            UInt32 i;
            UInt32 elements = (UInt32)farray.Length;
            UInt32[] buffer = new UInt32[elements];
            //  UInt32 *sort = (UInt32*)buffer;
            //  UInt32 *array = (UInt32*)farray;
            UInt32[] order = new UInt32[elements];
            UInt32[] tempOrder = new UInt32[elements];


            // 3 histograms on the stack:
            UInt32 kHist = 2048;
            UInt32[] b0 = new uint[kHist * 3];

            // UInt32 *b1 = b0 + kHist;
            // UInt32 *b2 = b1 + kHist;
            fixed (float* pfarray = farray)
            fixed (UInt32* sort = buffer, bk0 = b0)
            {
               UInt32* b1 = bk0 + kHist;
               UInt32* b2 = b1 + kHist;
               UInt32* array = (UInt32*)pfarray;

               //this tradeoff creates a massive speed increase..
               //but may be an issue if the memory is fragmented?
               //	for (i = 0; i < kHist * 3; i++)
               //		b0[i] = 0;

               // memset(b0, 0, kHist * 12);

               // 1.  parallel histogramming pass
               //
               for (i = 0; i < elements; i++)
               {

                  //pf(array);

                  UInt32 fi = FloatFlip(array[i]);

                  b0[(fi & 0x7FF)]++;
                  b1[(fi >> 11 & 0x7FF)]++;
                  b2[(fi >> 22)]++;
               }

               // 2.  Sum the histograms -- each histogram entry records the number of values preceding itself.
               {
                  UInt32 sum0 = 0, sum1 = 0, sum2 = 0;
                  UInt32 tsum;
                  for (i = 0; i < kHist; i++)
                  {

                     tsum = b0[i] + sum0;
                     b0[i] = sum0 - 1;
                     sum0 = tsum;

                     tsum = b1[i] + sum1;
                     b1[i] = sum1 - 1;
                     sum1 = tsum;

                     tsum = b2[i] + sum2;
                     b2[i] = sum2 - 1;
                     sum2 = tsum;
                  }
               }

               // byte 0: floatflip entire value, read/write histogram, write out flipped
               for (i = 0; i < elements; i++)
               {
                  UInt32 fi = array[i];
                  FloatFlipX(ref fi);
                  UInt32 pos = (fi & 0x7FF);

                  // pf2(array);
                  sort[++b0[pos]] = fi;
                  order[b0[pos]] = i;
               }

               // byte 1: read/write histogram, copy
               //   sorted -> array
               for (i = 0; i < elements; i++)
               {
                  UInt32 si = sort[i];
                  UInt32 pos = (si >> 11 & 0x7FF);
                  //  pf2(sort);

                  array[++b1[pos]] = si;
                  tempOrder[b1[pos]] = order[i];

               }

               // byte 2: read/write histogram, copy & flip out
               //   array -> sorted
               for (i = 0; i < elements; i++)
               {
                  UInt32 ai = array[i];
                  UInt32 pos = (ai >> 22);

                  //  pf2(array);
                  sort[++b2[pos]] = IFloatFlip(ai);
                  order[b2[pos]] = tempOrder[i];
               }

               // to write original:
               // memcpy(array, buffer, elements * 4);
               System.Buffer.BlockCopy(buffer, 0, farray, 0, (int)(elements * 4));

            }

         }
         public void Sort(int[] a)
         {

            // our helper array
            int[] t = new int[a.Length];

            // number of bits our group will be long
            int r = 4; // try to set this also to 2, 8 or 16 to see if it is quicker or not

            // number of bits of a C# int (sizeof works with unsafe code so I did this way...)
            int b = (int)Math.Ceiling(Math.Log(int.MaxValue, 2)) + 1;

            // counting and prefix arrays (note dimensions 2^r which is the number of all possible values of a r-bit number)
            int[] count = new int[1 << r];
            int[] pref = new int[1 << r];

            // number of groups
            int groups = (int)Math.Ceiling((double)b / (double)r);

            // the mask to identify groups
            int mask = (1 << r) - 1;

            // the algorithm:
            for (int c = 0, shift = 0; c < groups; c++, shift += r)
            {
               // reset count array
               for (int j = 0; j < count.Length; j++)
               {
                  count[j] = 0;
               }
               // counting elements of the c-th group
               for (int i = 0; i < a.Length; i++)
               {
                  count[(a[i] >> shift) & mask]++;
               }
               // calculating prefixes
               pref[0] = 0;
               for (int i = 1; i < count.Length; i++)
               {
                  pref[i] = pref[i - 1] + count[i - 1];
               }
               // from a[] to t[] elements ordered by c-th group
               for (int i = 0; i < a.Length; i++)
               {
                  t[pref[(a[i] >> shift) & mask]++] = a[i];
               }
               // a[]=t[] and start again until the last group
               a = (int[])t.Clone();
            }
            // a is sorted
         }
      }

      #endregion
   }


   #endregion


   #region MinMaxWeighted Quantizer

   public class Weighted_MinMax_Quantizer
   {
   
      public class Weighted_Vec
      {
         public Vector3 vec = Vector3.Empty;
         public float weight = 0;
         public int cell_index = 0;

         public Weighted_Vec() { }
         public Weighted_Vec(Vector3 v, float w, int c)
         {
            vec=(v);
            weight=(w);
            cell_index=(c);
         }
      };
      
   
      protected List<Weighted_Vec> m_vecs = new List<Weighted_Vec>();

      protected Vector3 m_centroid = Vector3.Empty;
      protected float m_total_weight = 0;

      protected List<Vector3> m_cells = new List<Vector3>();

      // -1 if no split can be found
      protected int find_split(ref float md2)
      {
         float max_dist2 = 0.0f;
         int split_vec = -1;

         for (int j = 0; j < m_vecs.Count; j++)
         {
            Weighted_Vec vec = m_vecs[j];
            Vector3 cell = m_cells[vec.cell_index];

            float dist2 = Vector3.LengthSq(cell-vec.vec) * vec.weight;
            if (dist2 > max_dist2)
            {
               max_dist2 = dist2;
               split_vec = j;
            }
         }

         md2 = max_dist2;

         return split_vec;
      }

      protected void reassign_vecs()
      {
         Vector3 new_cell = m_cells[m_cells.Count-1];
         int new_cell_index = m_cells.Count-1;

         for (int i = 0; i < m_vecs.Count; i++)
         {
            Weighted_Vec weighted_vec = m_vecs[i];				

            if (weighted_vec.cell_index != new_cell_index)
            {
               Vector3 orig_cell = m_cells[weighted_vec.cell_index];

               if (Vector3.LengthSq(weighted_vec.vec-orig_cell) > Vector3.LengthSq(weighted_vec.vec-new_cell))
                  weighted_vec.cell_index = new_cell_index;
            }
         }
      }

      protected void calc_centroids()
      {
         List<float> cell_weight = new List<float>(m_cells.Count);

         int i;

         for (i = 0; i < m_cells.Count; i++)
         {
            m_cells[i] = Vector3.Empty;
            cell_weight.Add(0);
         }

         for (i = 0; i < m_vecs.Count; i++)
         {
            Weighted_Vec weighted_vec = m_vecs[i];				

            int cell_index = weighted_vec.cell_index;

            m_cells[cell_index] += weighted_vec.vec * weighted_vec.weight;
            cell_weight[cell_index] += weighted_vec.weight;
         }

         for (i = 0; i < m_cells.Count; i++)
         {
            if (cell_weight[i] == 0.0f)
               continue;

            //assert(cell_weight[i] > 0.0f);
            m_cells[i] = m_cells[i]*(1.0f / cell_weight[i]);
         }
      }

   
      public Weighted_MinMax_Quantizer()
      {
         m_centroid = Vector3.Empty;
      }

      public void insert(Vector3 v, float w)
      {
         m_vecs.Add(new Weighted_Vec(v, w, 0));

         m_centroid += v * w;
         m_total_weight += w;
      }

      public int num_input_vecs()
      {
         return m_vecs.Count;
      }

      public float total_input_weight() 
      {
         return m_total_weight;
      }

      public Weighted_Vec input_vec(int i) 
      {
         return m_vecs[i];
      }

      public int num_output_cells()
      {
         return m_cells.Count;
      }

      public Vector3 output_cell(int i)
      {
         return m_cells[i];
      }
      
      public int find_best_cell(Vector3 cell)
      {
         float bestDist2 = 1e+30f;
         int bestCell=0;
         for (int i = 0; i < m_cells.Count; i++)
         {
            float dist2 = Vector3.LengthSq(cell-m_cells[i]);
            if (dist2 < bestDist2)
            {
               bestDist2 = dist2;
               bestCell = i;
            }
         }
         return bestCell;
      }

      public void quantize(float thresh_dist2, int max_output_cells)
      {
         if ((0==num_input_vecs()) || (m_cells.Count!=0))
            return;

         //assert(m_total_weight > 0.0f);

         m_cells.Add(Vector3.Empty);
         m_cells[m_cells.Count-1] = m_centroid* ((1.0f / m_total_weight));

         int k = 1;
         for ( ; ; )
         {
            float max_dist2=0;
            int split_vec_index = find_split(ref max_dist2);

            if (split_vec_index == -1)
               break;



            //if ((max_dist2 < thresh_dist2) && (k >= num_cells))
            if (max_dist2 < thresh_dist2) 
            {
               break;
            }

            m_vecs[split_vec_index].cell_index = m_cells.Count;

            m_cells.Add( m_vecs[split_vec_index].vec );

            //assert(m_cells.back().dist2(m_vecs[split_vec_index].vec) == 0.0f);

            reassign_vecs();

            calc_centroids();

            k++;
            if (k == max_output_cells)         
               break;
           // debugPrintf("Cells: %i\n", k);
         }
      }
      
      
      //private static void debugPrintf(const char* pMsg, ...)
      //{
      //   va_list args;
      //   va_start(args, pMsg);
      //   char buf[512];
      //   vsprintf_s(buf, sizeof(buf), pMsg, args);
      //   va_end(args);
      //   OutputDebugString(buf);
      //}
   };
   #endregion



   namespace Statistics
   {
      
      public class DataSet 
      {
         private floatN mSum;
         private floatN mSquaredSum;
         private int mCount;
         private List<floatN> mDataElements = new List<floatN>();
         private uint mVectorDim = 1;
        
         public DataSet(uint vectorDim)
         {
            mVectorDim = vectorDim;
            Clear();
         }
         public DataSet(uint vectorDim, floatN[] values)
         {
            Clear();
            mVectorDim = vectorDim;
            AddRange(values);
         }
         public DataSet(uint vectorDim, IEnumerable<floatN> values)
         {
            Clear();
            mVectorDim = vectorDim;
            AddRange(values);
         }
         //--------------------------
         public DataSet clone()
         {
            DataSet b = new DataSet(mVectorDim);
            for (uint i = 0; i < mDataElements.Count; i++)
            {
               floatN gfv = new floatN(mVectorDim);
               for (uint k = 0; k < mVectorDim; k++)
                  gfv[k] = mDataElements[(int)i][k];
               b.Add(gfv);
            }

            return b;
         }
         private float[,] toMatrix()
         {
            float[,] mat = new float[mDataElements.Count, mVectorDim];
            for (uint i = 0; i < mDataElements.Count; i++)
            {
               for (uint j = 0; j < mVectorDim; j++)
               {
                  mat[i, j] = mDataElements[(int)i][j];
               }
            }
            return mat;
         }

         //--------------------------
         private void Clear()
         {
            mSum = floatN.Empty(mVectorDim);
            mSquaredSum = floatN.Empty(mVectorDim);
            mCount = 0;
            mDataElements.Clear();
         }
         public void Add(floatN val)
         {
            if (val.NumDimentions() != mVectorDim)
               return;
            mSum = mSum + val;
            mSquaredSum += val * val;
            mCount++;
            mDataElements.Add(val);
         }
         public void AddRange(floatN[] values)
         {
            for (int i = 0; i < values.Length; i++)
            {
               if (values[i].NumDimentions() != mVectorDim)
                  continue;
               Add(values[i]);
               mDataElements.Add(values[i]);
            }

         }
         public void AddRange(IEnumerable<floatN> values)
         {
            foreach (floatN v in values)
            {
               if (v.NumDimentions() != mVectorDim)
                  continue;
               Add(v);
            }
         }
         //--------------------------
         public floatN Mean
         {
            get
            {
               if (mCount <= 0) return floatN.Empty(mVectorDim);
               return (mSum / (float)mCount);
            }
         }
         public floatN MeanSquared
         {
            get
            {
               if (mCount <= 0) return floatN.Empty(mVectorDim);

               return (mSquaredSum / (float)mCount);
            }
         }
         public floatN Variance
         {
            get
            {
               if (mCount <= 0) return floatN.Empty(mVectorDim);

               floatN mean = Mean;

               floatN sumate = floatN.Empty(mVectorDim);
               for (int i = 0; i < mDataElements.Count; i++)
                  sumate += floatN.Pow(mDataElements[i] - mean,2);
               floatN var = sumate / (mDataElements.Count - 1);

               return var;
            }
         }
         public floatN Sigma
         {
            get
            {
               return floatN.Sqrt(Variance);
            }
         }

         /// Gets the mean error estimate defined as the square root of the ratio of 
         /// the standard deviation to the number of values added to the accumulator.
         public floatN ErrorEstimate
         {
            get
            {
               if (mCount <= 0) return floatN.Empty(mVectorDim);

               return Sigma / (float)Math.Sqrt(mCount);
            }
         }

         public float Covariance(uint elementAIndex, uint elementBIndex)
         {
               if (mCount <= 0) return 0;

               floatN mean = Mean;

               float sumate = 0;
               floatN v;
               for (uint i = 0; i < mDataElements.Count; i++)
               { 
                  v = mDataElements[(int)i] - mean;
                  sumate += v[elementAIndex] * v[elementBIndex];
               }
               float var = sumate / (mDataElements.Count - 1);

               return var;
         }
         public float[,] CovarianceMatrix()
         {
            float[,] cM = new float[mVectorDim, mVectorDim];

            for (uint x = 0; x < mVectorDim; x++)
            {
               for (uint y = 0; y < mVectorDim; y++)
               {
                  cM[x, y] = Covariance(x, y);
               }
            }
            return cM;
         }


         private void rotate(ref float v1,ref float v2, float tau, float vsin)
         {
             float t1=v1, t2=v2;
             v1-=vsin*(t2+t1*tau);
             v2+=vsin*(t1-t2*tau);
         }
         private float dot(float[,] a,int t,float [] b)
         {
            float sum = 0;
            for (int i = 0; i < b.Length; i++)
               sum += a[t,i] * b[i];
            return sum;
         }

         public void PCA(/*List<floatN> eigenVectors, List<float> eigenValues*/)
         {
            DataSet ds = clone();
            floatN mean = Mean;
            //step 1, subtract mean from data
            for (uint i = 0; i < ds.mDataElements.Count; i++)
            {
               for (uint j = 0; j < ds.mVectorDim; j++)
               {
                  ds.mDataElements[(int)i][j] -= mean[j];
               }
            }

            //step 2, create covariance matrix //CLM is this from our subtracted data? or from our origional data?
            float[,] covMatR = ds.CovarianceMatrix(); 

            //step3, find eigenValues & vectors for our covariance matrix
            MatrixMxN K = new MatrixMxN((uint)covMatR.GetLength(0), (uint)covMatR.GetLength(0));//covariance matrix is always square
            K.set(covMatR);

            List<float> eigenValues = new List<float>();
            List<floatN> eigenVectors = new List<floatN>();
            MatrixMxN.calculateEigenSystem(K, eigenValues, eigenVectors);


            #region HOPPE
            //...

        //    //insert our elements into a matrix
        //    float[,] mat = toMatrix();
            
        //    //step 1 : subtract mean from all pieces of data
        //    for (int i = 0; i < mDataElements.Count; i++)
        //    {
        //       for (int j = 0; j < mVectorDim; j++)
        //       {
        //          mat[i, j] -= mean[j];
        //       }
        //    }



        //    int height = mDataElements.Count;
        //    int width = mVectorDim;

        //    float[,] a = new float[width, width];
        //    float[,] t = new float[width, width];
            
        //    for (int i = 0 ;i < height; i++)
        //    {
        //       for (int c0 = 0; c0 < width; c0++)
        //         {
        //             float mi_i_c0=mat[i,c0];
        //             for (int c1 = 0; c1 < c0; c1++)
        //             {
        //                 t[c0,c1] += mi_i_c0*mat[i,c1];
        //             }
        //         } 
        //     }

        //     for (int c0 = 0; c0 < width; c0++)
        //    {
        //       for(int c1=0; c1 < c0; c1++)
        //       {
        //            a[c0,c1]=a[c1,c0]=(float)(t[c0,c1]/height);
        //       }
        //    }


        //    float[] val = new float[width];
        //    for (int i = 0; i < width; i++) { val[i] = a[i, i]; }

        //    float[,] mo = new float[width, width];
        //    for (int i = 0; i < width; i++)
        //    {
        //       for (int j = 0; j < width; j++) 
        //       {
        //          mo[i,j] = i == j ? 1.0f : 0.0f; 
        //       }
        //    } 





        //    //------------------- 
        
        //   int iter=0;
        //   for (;;iter++) 
        //   {
              
        //     float sum=0.0f;
        //     for (int i = 0; i < width ; i++) 
        //     {
        //        for (int j = i + 1; j < width - 1; j++) 
        //       {
        //          sum+=Math.Abs(a[i,j]); 
        //       } 
        //     } 
        //     if (sum==0) break;

        //     for (int i = 0; i < width - 1; i++)
        //       {
        //          for (int j = i + 1; j < width - 1; j++)
        //          {
        //               float thresh=1e2f*Math.Abs(a[i,j]);
        //               if (Math.Abs(val[i])+thresh==Math.Abs(val[i]) &&
        //                   Math.Abs(val[j])+thresh==Math.Abs(val[j]))
        //               {
        //                   a[i,j]=0.0f;
        //               }
        //               else if (Math.Abs(a[i,j])>0.0f)
        //               {
        //                   float vtan;
        //                    {
        //                       float dd=val[j]-val[i];
        //                       if (Math.Abs(dd)+thresh==Math.Abs(dd))
        //                       {
        //                           vtan=a[i,j]/dd;
        //                       } 
        //                       else
        //                       {
        //                           float theta=0.5f*dd/a[i,j];
        //                           vtan=(float)(1.0f/(Math.Abs(theta)+Math.Sqrt(1.0f+Math.Pow(theta,2))));
        //                           if (theta<0.0f) vtan=-vtan;
        //                       }
        //                   }
        //                   float vcos=(float)(1.0f/Math.Sqrt(1.0f+Math.Pow(vtan,2)));
        //                   float vsin=vtan*vcos;
        //                   float tau=vsin/(1.0f+vcos);
        //                   val[i]-=vtan*a[i,j];
        //                   val[j]+=vtan*a[i,j];
        //                   a[i,j]=0.0f;
        //                   for(int k=0;k<i;k++) 
        //                  {
        //                       rotate(ref a[k,i],ref a[k,j],tau,vsin);
        //                   } 
        //                   for(int k=i+1;k<j-1;k++) 
        //                  {
        //                       rotate(ref a[i,k],ref a[k,j],tau,vsin);
        //                   }
        //                   for (int k = j + 1; k < width - 1; k++) 
        //                  {
        //                       rotate(ref a[i,k],ref a[j,k],tau,vsin);
        //                   }
        //                   for (int k = 0; k < width; k++) 
        //                  {
        //                       rotate(ref mo[i,k],ref mo[j,k],tau,vsin);
        //                   } 
        //               }
        //           } 
        //       } 
        //      // assertx(iter<n*20);
        //   }
        //   //SSTAT(Spca_iter,iter+1);
    
        //    //----------------
        //     // Convert variances to standard deviations.
        //   for (int i = 0; i < width; i++)
        //  {
        //   if (val[i]<0.0f) 
        //   {
        ////       assertw1(val[i]>=-1e-5f);
        //   //    if (val[i]<-1e-5f) { SSTAT(Spca_negv,val[i]); }
        //       val[i]=0.0f;
        //   }
        //   val[i]=(float)(Math.Sqrt(val[i]));
        // } 

        //  // Insertion sort: eigenvalues in descending order.
        // for (int i = 0; i < width; i++)
        //  {
        //      int imax=i;
        //      float vmax=val[i];
        //      for (int j = i + 1; j < width - 1; j++)
        //      {
        //         if (val[j] >= vmax)
        //         {
        //            imax = j;
        //            vmax = val[j];
        //         }
        //      }
        //      if (imax==i) continue;
        //      float kt = val[i];
        //       val[i] = val[imax];
        //       val[imax] = kt;

        //       for (int j = 0; j < width; j++) 
        //      {
        //         kt = mo[i,j];
        //         mo[i,j] = mo[imax,j];
        //         mo[imax,j] = kt;

        //      };
        //  }
        //  // Orient eigenvectors canonically
        //  float[] all1 = new float[width];
        //  for (int q = 0; q < width; q++)
        //     all1[q]=1.0f;

        //  for (int i = 0; i < width; i++)
        //  {
        //      if (dot(mo,i,all1)<0.0f)
        //      {
        //         for (int c = 0; c < width; c++)
        //         {
        //            mo[i,c]*=-1; 
        //         } 
        //      }
        //  }

        //  float[,] eigenVectors = mo;
            //  float[] eigenValues = val; 
            #endregion
         }
      };

      public class Accumulator
      {

         private double sum;
         private double squaredSum;
         private int count;
         //------------------------------------------
         public Accumulator()
         {
            this.Clear();
         }
         public Accumulator(double[] values)
         {
            this.Clear();
            this.AddRange(values);
         }
         public Accumulator(IEnumerable<double> values)
         {
            this.Clear();
            this.AddRange(values);
         }
         //------------------------------------------
         public void Add(double value)
         {
            sum += value;
            squaredSum += value * value;
            count++;
         }
         public void AddRange(double[] values)
         {
            for (int i = 0; i < values.Length; i++) this.Add(values[i]);
         }
         public void AddRange(IEnumerable<double> values)
         {
            foreach (double v in values)
               Add(v);
         }
         //------------------------------------------
         public void Clear()
         {
            this.sum = 0d;
            this.squaredSum = 0d;
            this.count = 0;
         }
         //------------------------------------------

         public int Count
         {
            get { return count; }
         }
         public double Sum
         {
            get { return sum; }
         }
         public double SquaredSum
         {
            get { return squaredSum; }
         }
         public double Mean
         {
            get
            {
               if (count <= 0) throw new InvalidOperationException(
                    "#E00 No mean available. The accumulator is empty.");

               return (sum / count);
            }
         }
         /// Gets the arithmetic mean of the squared values added to the accumulator. Note that this is not equal to the squared mean of the values.
         public double MeanSquared
         {
            get
            {
               if (count <= 0) throw new InvalidOperationException(
                    "#E00 No mean available. The accumulator is empty.");

               return (squaredSum / count);
            }
         }
         // The larger the Variance, the more 'scattered' the data.
         public double Variance
         {
            get
            {
               if (count <= 0) return 0;

               double mean = this.Mean;
               return (squaredSum / count - mean * mean);
            }
         }
         public double Sigma
         {
            get
            {
               return Math.Sqrt(this.Variance);
            }
         }

         /// Gets the mean error estimate defined as the square root of the ratio of 
         /// the standard deviation to the number of values added to the accumulator.
         public double ErrorEstimate
         {
            get
            {
               if (count <= 0) throw new InvalidOperationException(
                    "#E00 No error estimate available. The accumulator is empty.");

               return Sigma / Math.Sqrt(count);
            }
         }


   }
   }
}