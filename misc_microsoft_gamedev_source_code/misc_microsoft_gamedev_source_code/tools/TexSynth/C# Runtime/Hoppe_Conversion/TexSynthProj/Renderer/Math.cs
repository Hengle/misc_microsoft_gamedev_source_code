using System;
using System.IO;
using System.Drawing;
using System.Collections.Generic;
using System.Text;
using System.Windows.Forms;

using Microsoft.DirectX;
using Microsoft.DirectX.Direct3D;
using Microsoft.DirectX.DirectInput;
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
          Vector3 XX_YY_ZZ = Vector3.Empty;
          Vector3 XY_XZ_YZ = Vector3.Empty;

          for (int i = 0; i < points.Count; i++)
          {
             Vector3 Point = CenterOfMass - points[i];

             XX_YY_ZZ = XX_YY_ZZ + BMathLib.Vec3Mul(Point, Point);

              Vector3 XXY = new Vector3(Point.X, Point.X, Point.Y);
              Vector3 YZZ = new Vector3(Point.Y, Point.Z, Point.Z);

              XY_XZ_YZ = XY_XZ_YZ + BMathLib.Vec3Mul(XXY, YZZ);
          }
          
          Vector3 v1 = Vector3.Empty;
          Vector3 v2 = Vector3.Empty;
          Vector3 v3 = Vector3.Empty;
          
          // Compute the eigenvectors of the inertia tensor.
          BMathLib.CalculateEigenVectorsFromCovarianceMatrix( XX_YY_ZZ.X, XX_YY_ZZ.Y, XX_YY_ZZ.Z,
                                                     XY_XZ_YZ.X, XY_XZ_YZ.Y, XY_XZ_YZ.Z,
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

      static float cPI               =3.141592654f;
      static float c2PI              =6.283185307f;
      static float c1DIVPI           =0.318309886f;
      static float c1DIV2PI          =0.159154943f;
      static float cPIDIV2           =1.570796327f;
      static float cPIDIV4           =0.785398163f;

      

      public static float cFloatCompareEpsilon = 1e-20f;
      public static float cTinyEpsilon = 0.000125f;

      static public float log2(float v)
      {
         return (float)(Math.Log(v) / Math.Log(2.0));
      }
      static public float Clamp(float input, float min, float max)
      {
         if (input < min)
            input = min;
         else if (input > max)
            input = max;

         return (input);
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
   
      static public float vec3vec3Angle(Vector3 a, Vector3 b)
      {
         return (float)Math.Acos(Vector3.Dot(a, b));
      }
      static public float rayPlaneAngle(Vector3 r0, Vector3 rD, Plane pl)
      {
         float tVal = 0;
         float angle = 0;
         if(rayPlaneIntersect(pl,r0, rD, false, ref tVal))
         {
            angle = 90 - vec3vec3Angle(r0, new Vector3(pl.A, pl.B, pl.C));
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
         public void Sort(int [] a)
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
}