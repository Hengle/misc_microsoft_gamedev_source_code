using System;
using System.Collections.Generic;
using Microsoft.DirectX;   //CLM UGG! I wish we could get rid of this..

/*
 * CLM [07.05.07]
 * This class defines a generic Quadtree container class, as well as a generic QuadtreeNode class.
 * DO NOT USE QuadTreeTemplate<T> or QuadTreeCellTemplate<T> DIRECTLY!!
 * Instead, ensure you overload it with a base class. (see SpatialQuadTree for an example)
 */
namespace EditorCore
{
   /*
 * DO NOT USE QuadTreeTemplate<T> or QuadTreeCellTemplate<T> DIRECTLY!!
 * Instead, ensure you overload it with a base class. (see SpatialQuadTree for an example)
 */
   public enum eQNKidEnum
   {
      cTopLeft = 0,
      cTopRight = 1,
      cBotLeft = 2,
      cBotRight = 3,
      cNumQuadTreeKids
   };

   public class QuadTreeCellTemplate<T> where T: ICloneable, IComparable, new()
   {
      public uint mXLocCode=0;   //used for neighbor searching.
      public uint mZLocCode=0;   //used for neighbor searching.
      public uint mLevel=0;     //smallest cell has level 0

      public T mParent = default(T);
      public T[] mChildren = null;

      
      //------------------------------------------------------------
      public QuadTreeCellTemplate()
      {

      }
      ~QuadTreeCellTemplate()
      {
         destroy();
      }
      //------------------------------------------------------------
      public void destroy()
      {
         mParent = default(T);
         if(mChildren!=null)
         {
            for (uint q = 0; q < (int)eQNKidEnum.cNumQuadTreeKids; q++)
            {
               if (mChildren[q] != null)
               {
                  mChildren[q] = default(T);
               }
            }
            mChildren = null;
         }
      }
      //------------------------------------------------------------
   };

   public class QuadTreeTemplate<T> where T : QuadTreeCellTemplate<T>,ICloneable,IComparable, new()
   {
      int mMaxNumLevels = 1;
      int mRootLevel = 0;  //mMaxNumLevels-1;

      protected T mRootCell = null;
      protected T[,] mLeafCellArray = null;

      protected uint mNumXLeafCells = 0;
      protected uint mNumZLeafCells = 0;

      
      //------------------------------------------------------------
      protected QuadTreeTemplate()
      {
      }
      ~QuadTreeTemplate()
      {
         destroy();
      }

      //------------------------------------------------------------
      protected void destroy()
      {
         if (mRootCell!=null)
         {
            mRootCell.destroy();
            mRootCell = null;
         }
         //the above will actually delete the memory for the 2D array
         //so we just need to clear the pointers.
         if (mLeafCellArray!=null)
         {
            for (uint i = 0; i < mNumXLeafCells; i++)
            {
               for (uint j = 0; j < mNumZLeafCells; j++)
               {
                  //mLeafCellArray[i, j].destroy();
                  mLeafCellArray[i, j] = null;
               }
            }
            mLeafCellArray = null;
         }
      }
      //------------------------------------------------------------
      protected void createTree(uint numXLeafCells, uint numZLeafCells)
      {

         mNumXLeafCells = numXLeafCells;
         mNumZLeafCells = numZLeafCells;

         //we keep a seperate pointer list to our leaf nodes
         //frequently, it's easier to traverse this list, than it is to 
         //issue a recursive queary to identify leaves
         mLeafCellArray = new T[mNumXLeafCells, mNumZLeafCells];
         for (uint i = 0; i < mNumXLeafCells; i++)
         {
            for (uint j = 0; j < mNumZLeafCells; j++)
            {
               mLeafCellArray[i, j] = new T();
               mLeafCellArray[i, j].mLevel = 0;
               mLeafCellArray[i, j].mParent = null;
               mLeafCellArray[i, j].mXLocCode = i;
               mLeafCellArray[i, j].mZLocCode = j;
               mLeafCellArray[i, j].mChildren = null;
            }
         }


         //recursivly create our upper containers
         T[,] lowerNodes = mLeafCellArray;
         T[,] containerNodes = null;
         mMaxNumLevels = 1;
         while (numXLeafCells != 2)
         {
            uint lowerLengthsX = numXLeafCells;
            uint lowerLengthsZ = numZLeafCells;

            numXLeafCells /= 2;
            numZLeafCells /= 2;

            if (lowerLengthsX % 2 == 1) numXLeafCells++;
            if (lowerLengthsZ % 2 == 1) numZLeafCells++;

            if (numXLeafCells % 2 == 1) numXLeafCells++;
            if (numZLeafCells % 2 == 1) numZLeafCells++;

            containerNodes = new T[(numXLeafCells), (numZLeafCells)];
            for (uint i = 0; i < numXLeafCells; i++)
            {
               for (uint j = 0; j < numZLeafCells; j++)
               {
                  containerNodes[i, j] = new T();
                  containerNodes[i, j].mLevel = (uint)mMaxNumLevels;
                  containerNodes[i, j].mParent = null;
                  containerNodes[i, j].mXLocCode = i;
                  containerNodes[i, j].mZLocCode = j;

                  if (containerNodes[i, j].mChildren == null)
                     containerNodes[i, j].mChildren = new T[(int)eQNKidEnum.cNumQuadTreeKids];

                  uint k = i * 2;
                  uint l = j * 2;
                  uint m = k + 1;
                  uint n = l + 1;
                  containerNodes[i, j].mChildren[(int)eQNKidEnum.cTopLeft] = (k >= lowerLengthsX || n >= lowerLengthsZ) ? null : lowerNodes[k, n];
                  containerNodes[i, j].mChildren[(int)eQNKidEnum.cTopRight] = (m >= lowerLengthsX || n >= lowerLengthsZ) ? null : lowerNodes[m, n];
                  containerNodes[i, j].mChildren[(int)eQNKidEnum.cBotLeft] = (k >= lowerLengthsX || l >= lowerLengthsZ) ? null : lowerNodes[k, l];
                  containerNodes[i, j].mChildren[(int)eQNKidEnum.cBotRight] = (m >= lowerLengthsX || l >= lowerLengthsZ) ? null : lowerNodes[m, l];

                  for (uint q = 0; q < (int)eQNKidEnum.cNumQuadTreeKids; q++)
                  {
                     if (containerNodes[i, j].mChildren[q] != null)
                        containerNodes[i, j].mChildren[q].mParent = containerNodes[i, j];
                  }
               }
            }
            lowerNodes = containerNodes;
            mMaxNumLevels++;
         }

         mMaxNumLevels++;

         //Create Root
         mRootLevel = mMaxNumLevels - 1;
         mRootCell = new T();
         mRootCell.mLevel = (uint)mRootLevel;
         mRootCell.mParent = null;
         mRootCell.mXLocCode = 0;
         mRootCell.mZLocCode = 0;

         if (mRootCell.mChildren == null)
            mRootCell.mChildren = new T[(int)eQNKidEnum.cNumQuadTreeKids];

         mRootCell.mChildren[(int)eQNKidEnum.cTopLeft] = lowerNodes[0, 1];
         mRootCell.mChildren[(int)eQNKidEnum.cTopRight] = lowerNodes[1, 1];
         mRootCell.mChildren[(int)eQNKidEnum.cBotLeft] = lowerNodes[0, 0];
         mRootCell.mChildren[(int)eQNKidEnum.cBotRight] = lowerNodes[1, 0];
         for (uint q = 0; q < (int)eQNKidEnum.cNumQuadTreeKids; q++)
         {
            if (mRootCell.mChildren[q] != null)
               mRootCell.mChildren[q].mParent = mRootCell;
         }
      }

      //------------------------------------------------------------
      public uint getNumXLeafCells() { return mNumXLeafCells; }
      public uint getNumZLeafCells() { return mNumZLeafCells; }
      //------------------------------------------------------------
      public T getLeafCellAtGridLoc(uint xLoc, uint zLoc)
      {
         if (zLoc < 0 || zLoc >= mNumZLeafCells || xLoc < 0 || xLoc >= mNumXLeafCells)
            return default(T);

         return mLeafCellArray[xLoc, zLoc];
      }
      public T getLeftNeighborToCell(T targetCell)
      {
         if (targetCell.mXLocCode == 0)
            return default(T);

         return mLeafCellArray[targetCell.mXLocCode - 1, targetCell.mZLocCode];
      }
      public T getRightNeighborToCell(T targetCell)
      {
         if (targetCell.mXLocCode >= mNumXLeafCells)
            return default(T);

         return mLeafCellArray[targetCell.mXLocCode + 1, targetCell.mZLocCode];
      }
      public T getBottomNeighborToCell(T targetCell)
      {
         if (targetCell.mZLocCode == 0)
            return default(T);

         return mLeafCellArray[targetCell.mXLocCode, targetCell.mZLocCode - 1];
      }
      public T getTopNeighborToCell(T targetCell)
      {
         if (targetCell.mZLocCode >= mNumZLeafCells)
            return default(T);

         return mLeafCellArray[targetCell.mXLocCode, targetCell.mZLocCode + 1];
      }
    };

   ///////-------------------------------------------------
   ///////-------------------------------------------------
   ///////-------------------------------------------------
    /*
     * CLM [07.05.07]
     * This class creates a spatial quadnode system. With sphere, ray, point, and AABB intersection operations
     * Override this for any terrain type systems you want
     * It's up to the base class to extend any raycast operations (for getting the intersection point of terrain explicitly, for example)
     * 
   */
   public class SpatialQuadTreeCell : QuadTreeCellTemplate<SpatialQuadTreeCell>, IComparable, ICloneable
   {
      public SpatialQuadTreeCell()
      {

      }
      ~SpatialQuadTreeCell()
      {
         destroy();
      }
      
      new public void destroy()
      {
         base.destroy();
      }
      //--------------------------------------------------------------------
      float3 mAABBMin;
      float3 mAABBMax;
      public void setBounds(float3 min, float3 max)
      {
         mAABBMin = min;
         mAABBMax = max;
         float3 center = (max + min) * 0.5f;

         if (mChildren != null)
         {
            if (mChildren[(int)eQNKidEnum.cBotLeft]!=null)
               mChildren[(int)eQNKidEnum.cBotLeft].setBounds(min, center);
            if (mChildren[(int)eQNKidEnum.cBotRight] != null)
               mChildren[(int)eQNKidEnum.cBotRight].setBounds(new float3(center.X, min.Y, min.Z), new float3(max.X, max.Y, center.Z));
            if (mChildren[(int)eQNKidEnum.cTopLeft] != null)
               mChildren[(int)eQNKidEnum.cTopLeft].setBounds(new float3(min.X, min.Y, center.Z), new float3(center.X, max.Y, max.Z));
            if (mChildren[(int)eQNKidEnum.cTopRight] != null)
                mChildren[(int)eQNKidEnum.cTopRight].setBounds(center, max);
         }
      }
      public float3 getAABBCenter()
      {
         return (mAABBMax-mAABBMin)*0.5f;
      }
      public float3 getAABBMin() { return mAABBMin; }
      public float3 getAABBMax() { return mAABBMax; }
      public void updateBoundsFromChildren()
      {
         
         if (mChildren != null)
         {
            BBoundingBox aabb = new BBoundingBox();
            for (uint i = 0; i < (int)eQNKidEnum.cNumQuadTreeKids; i++)
            {
               if (mChildren[i] == null)
                  continue;

               mChildren[i].updateBoundsFromChildren();

               float3 min = mChildren[i].getAABBMin();               aabb.addPoint(min.X, min.Y, min.Z);
               min = mChildren[i].getAABBMax();               aabb.addPoint(min.X, min.Y, min.Z);
            }

            mAABBMin = new float3(aabb.min);
            mAABBMax = new float3(aabb.max);
            aabb = null;
         }
         

         
      }
      //--------------------------------------------------------------------
      public object Clone()
      {
         return null;
      }
      public int CompareTo(object obj)
      {
         return -1;
      }
      //--------------------------------------------------------------------
      public bool intersectsSphere(float3 sphereCenter, float sphereRadius)
      {
         return BMathLib.sphereAABBIntersect(sphereCenter.toVec3(), sphereRadius, mAABBMin.toVec3(), mAABBMax.toVec3());
      }
      public bool intersectsRay(float3 rayOrig, float3 rayDir)
      {
         Vector3 tc = Vector3.Empty;
         float tt = 0;
         Vector3 origin = rayOrig.toVec3();
         Vector3 dir = rayDir.toVec3();
         Vector3 mn = mAABBMin.toVec3();
         Vector3 mx = mAABBMax.toVec3();
         return BMathLib.ray3AABB(ref tc, ref tt, ref origin, ref dir, ref mn, ref mx);
      }
      public bool intersectsAABB(float3 bMin, float3 bMax)
      {
         if (mAABBMin.X < bMax.X && mAABBMin.Y < bMax.Y && mAABBMin.Z < bMax.Z &&
            bMin.X < mAABBMax.X && bMin.Y < mAABBMax.Y && bMin.Z < mAABBMax.Z)
            return true;

         return false;
      }
      public bool containsPoint(float3 point)
      {
         if(point.X <= mAABBMax.X && point.Y <= mAABBMax.Y && point.Z <= mAABBMax.Z && 
            point.X >= mAABBMin.X && point.Y >= mAABBMin.Y && point.Z >= mAABBMin.Z)
         return true;

         return false;
      }
      public bool intersectsFrustum(BFrustum frust)
      {
         return frust.AABBVisible(mAABBMin.toVec3(),mAABBMax.toVec3());
      }

      public void getLeafChildrenIntersectingSphere(List<SpatialQuadTreeCell> nodes, float3 sphereCenter, float sphereRadius)
      {
         if (intersectsSphere(sphereCenter, sphereRadius))
         {
            if (mChildren != null)
            {
               for (uint i = 0; i < (int)eQNKidEnum.cNumQuadTreeKids; i++)
                  if (mChildren[i]!=null)
                     mChildren[i].getLeafChildrenIntersectingSphere(nodes, sphereCenter, sphereRadius);
            }
            else
            {
               nodes.Add(this);
            }
         }
      }
      public void getLeafChildrenIntersectingRay(List<SpatialQuadTreeCell> nodes, float3 rayOrig, float3 rayDir)
      {
         if(intersectsRay(rayOrig,rayDir))
         {
            if(mChildren!=null)
            {
               for (uint i = 0; i < (int)eQNKidEnum.cNumQuadTreeKids; i++)
                  if (mChildren[i] != null)
                     mChildren[i].getLeafChildrenIntersectingRay(nodes, rayOrig, rayDir);
            }
            else
            {
               nodes.Add(this);
            }
         }
      }
      public void getLeafChildrenIntersectingAABB(List<SpatialQuadTreeCell> nodes, float3 aabbMin, float3 aabbMax)
      {
         if (intersectsAABB(aabbMin, aabbMax))
         {
            if (mChildren != null)
            {
               for (uint i = 0; i < (int)eQNKidEnum.cNumQuadTreeKids; i++)
                  if (mChildren[i] != null)
                     mChildren[i].getLeafChildrenIntersectingAABB(nodes, aabbMin, aabbMax);
            }
            else
            {
               nodes.Add(this);
            }
         }
      }
      public void getLeafChildContainingPoint(SpatialQuadTreeCell node, float3 point)
      {
         if (containsPoint(point))
         {
            if (mChildren != null)
            {
               for (uint i = 0; i < (int)eQNKidEnum.cNumQuadTreeKids; i++)
                  if (mChildren[i] != null)
                     mChildren[i].getLeafChildContainingPoint(node, point);
            }
            else
            {
               node=this;
            }
         }
      }
      public void getLeafChildrenIntersectingFrustum(List<SpatialQuadTreeCell> nodes,BFrustum frust)
      {
         if (intersectsFrustum(frust))
         {
            if (mChildren != null)
            {
               for (uint i = 0; i < (int)eQNKidEnum.cNumQuadTreeKids; i++)
                  if (mChildren[i] != null)
                     mChildren[i].getLeafChildrenIntersectingFrustum(nodes, frust);
            }
            else
            {
               nodes.Add(this);
            }
         }
      }
      //--------------------------------------------------------------------
      public object mExternalData;
   };

   public class SpatialQuadTree : QuadTreeTemplate<SpatialQuadTreeCell>
   {
      //--------------------------------------------------------------------
      public void createSpatialQuadTree(float3 minTreeBounds, float3 maxTreeBounds, uint numXLeafCells, uint numZLeafCells)
      {
        // if(maxTreeDepth==0)
        //    return;

         float3 diff = maxTreeBounds - minTreeBounds;

         //uint numXLeafCells = (uint)2<< (int)(maxTreeDepth-1);//Math.Pow(2, maxTreeDepth-1);
         //uint numZLeafCells = (uint)Math.Pow(2, maxTreeDepth-1);

         base.createTree(numXLeafCells, numZLeafCells);

         //walk our tree from top down, setting bounding boxes.
         mRootCell.setBounds(minTreeBounds, maxTreeBounds);
      }
      //--------------------------------------------------------------------
      public void getLeafCellsIntersectingSphere(List<SpatialQuadTreeCell> nodes, float3 sphereCenter, float sphereRadius)
      {
         mRootCell.getLeafChildrenIntersectingSphere(nodes, sphereCenter, sphereRadius);
      }
      public void getLeafCellsIntersectingAABB(List<SpatialQuadTreeCell> nodes, float3 aabbMin, float3 aabbMax)
      {
         mRootCell.getLeafChildrenIntersectingAABB(nodes, aabbMin, aabbMax);
      }
      public void getLeafCellsIntersectingRay(List<SpatialQuadTreeCell> nodes, float3 rayOrig, float3 rayDir)
      {
         mRootCell.getLeafChildrenIntersectingRay(nodes, rayOrig, rayDir);
      }
      public void getLeafCellsContainingPoint(SpatialQuadTreeCell node, float3 point)
      {
         mRootCell.getLeafChildContainingPoint(node, point);
      }
      public void getVisibleNodes(List<SpatialQuadTreeCell> nodes, BFrustum frust)
      {
         mRootCell.getLeafChildrenIntersectingFrustum(nodes, frust);
      }
      //--------------------------------------------------------------------

   };
}