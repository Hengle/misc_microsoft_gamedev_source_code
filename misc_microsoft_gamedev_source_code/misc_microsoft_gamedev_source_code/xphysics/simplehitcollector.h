//==============================================================================
// simplehitcollector.h
//
// Copyright (c) 2003, Ensemble Studios
//==============================================================================



#ifndef _SIMPLE_HIT_COLLECTOR_H_
#define _SIMPLE_HIT_COLLECTOR_H_

#include "Physics/Collide/Shape/Query/hkpShapeRayCastCollectorOutput.h"

//==============================================================================
// Includes

//==============================================================================
// Forward declarations

//==============================================================================
// Const declarations


class BRayHit
{
public:
   BRayHit() {};
   hkpShapeRayCastCollectorOutput mOutput;
   const hkpCollidable*           mRootCollidable;
   hkpShapeKey                    mShapeKey;
};

//==============================================================================
class BSimpleHitCollector : public hkpRayHitCollector
{
public:

   BEGIN_FLAG_MAP
      ADD_FLAG_ENTRY(cFlagIgnoreBackfaces,1)
      ADD_FLAG_ENTRY(cFlagAbortEarly,2)
   END_FLAG_MAP

   DECLARE_FLAGS;

   BSimpleHitCollector()
   {
      mHits.empty();
      memset(&mFlags, 0, sizeof(mFlags));
   };

	virtual void addRayHit( const hkpCdBody& cdBody, const hkpShapeRayCastCollectorOutput& hitInfo )
   {
      if (getFlag(cFlagAbortEarly) && (mHits.getSize() > 0))
         return;

      BRayHit hit;
      hit.mOutput = hitInfo;
      hit.mRootCollidable = cdBody.getRootCollidable();
      hit.mShapeKey = cdBody.getShapeKey();
      mHits.addToHead(hit);
   }

#if 0
   virtual void addRayHit( const hkpCdBody& cdBody, const hkpShapeRayCastOutput& hitInfo )
   {    

      if (getFlag(cFlagAbortEarly) && (mHits.getSize() > 0))
         return;
/*
      if (getFlag(cFlagIgnoreBackfaces))
      {

         hkpRigidBody* hkGroundRB = static_cast<hkpRigidBody*>(cdBody.getRootCollidable()->getOwner());
         if(!hkGroundRB)
            return;

         const hkTransform &trans = hkGroundRB->getTransform();
         hkpCollidable *hkGround = hkGroundRB->getCollidable();
         if ((hkGround != NULL) && (hkGround->getShape() != NULL))
         {
            long shapeType = hkGround->getShape()->getType();
            if (shapeType == HK_SHAPE_MOPP)
            {
               hkpMoppBvTreeShape *treeShape = (hkpMoppBvTreeShape *)hkGround->getShape();
               const hkpShapeCollection *shapeCollect = treeShape->getShapeCollection();

               hkpShapeCollection::AllocBuffer buffer;
               const hkpShape *subShape = shapeCollect->getChildShape(cdBody.getShapeKey(), buffer);
               if (subShape->getType() == HK_SHAPE_TRIANGLE)
               {

                  hkpTriangleShape *triangleShape = (hkpTriangleShape *)subShape;
                  const hkVector4 *pVerts = triangleShape->getVertices();
                  hkVector4 side1 = pVerts[1];
                  side1.sub4(pVerts[0]);

                  hkVector4 side2 = pVerts[2];
                  side2.sub4(pVerts[0]);

                  hkVector4 normal;
                  normal.setCross(side1, side2);
                  normal.normalize4();

                  //-- transform the normal
                  normal.setRotatedDir(trans.getRotation(), normal);

                  hkVector4 dir;
                  dir = mInput.m_to;
                  dir.sub4(mInput.m_from);
                  dir.normalize4();

                  float dot = dir.dot3(normal);
                  if (dot > 0.0f)
                  {
                     return;
                  }
               }
            }
         }
      }

*/
      BRayHit hit;
      hit.mOutput = hitInfo;
      hit.mRootCollidable = cdBody.getRootCollidable();
      hit.mShapeKey = cdBody.getShapeKey();
      mHits.addToHead(hit);
   };
#endif

   BCopyList<BRayHit> &getHits(void) { return mHits; }
   
   void setRayInputData(hkpWorldRayCastInput & input)
   {
      mInput = input;
   }

protected:

   BCopyList<BRayHit>   mHits;
   hkpWorldRayCastInput  mInput;

}; 


//==============================================================================
#endif // _SIMPLE_HIT_COLLECTOR_H_

//==============================================================================
// eof: simpleraycastcallback.h
//==============================================================================