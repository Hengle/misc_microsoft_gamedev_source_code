//============================================================================
// damageaction.cpp
//
// Copyright (c) 2004-2007 Ensemble Studios
//============================================================================

#include "common.h"
#include "damageaction.h"
#include "damagetemplatemanager.h"

#include "grannymanager.h"
#include "grannymodel.h"
#include "shape.h"
#include "gamedirectories.h"
#include "database.h"
#include "particlegateway.h"
#include "visual.h"
#include "unit.h"
#include "grannyinstance.h"
#include "physicsinfo.h"
#include "physicsobjectblueprint.h"
#include "world.h"
#include "worldsoundmanager.h"
#include "unitactionphysics.h"
#include "syncmacros.h"
#include "configsgame.h"
#include "terraineffectmanager.h"

#include <Physics/Collide/Shape/Convex/Box/hkpBoxShape.h>
#include <Physics/Collide/Shape/Convex/ConvexTranslate/hkpConvexTranslateShape.h>
#include <Physics/Collide/Shape/Compound/Collection/List/hkpListShape.h>
#include <Physics/Collide/Shape/Compound/Tree/Mopp/hkpMoppUtility.h>
#include <Physics/Collide/Shape/Compound/Tree/Mopp/hkpMoppBvTreeShape.h>
#include <Physics/Dynamics/Constraint/Bilateral/LimitedHinge/hkpLimitedHingeConstraintData.h>
#include <Physics/Dynamics/Entity/hkpRigidBody.h>



#define  DO_NOT_USE_MOPP

#define  DEFAULT_RELEASE_EFFECT_LIFETIME          3.0f
#define  DEFAULT_ATTACHPARTICLE_LIFETIME          -1.0f
#define  DEFAULT_THROW_PART_LIFETIME              10.0f

#define  PART_UNDEFINED_SHAPE_BOUNDING_BOX_FACTOR  0.75f


BVector computeImpulseForce(BVector pDir, float force, bool ballisticHit);


//============================================================================
// BDamagePart
//============================================================================

//============================================================================
// BDamagePart::BDamagePart
//============================================================================
BDamagePart::BDamagePart()
{
   reset();
}


//============================================================================
// BDamagePart::~BDamagePart
//============================================================================
BDamagePart::~BDamagePart()
{
}


//============================================================================
// BDamagePart::reset
//============================================================================
void BDamagePart::reset( void )
{
   mBoneHandle = -1;
   mMeshIndex = -1;
   mMin.zero();
   mMax.zero();
   mCenterOffset.zero();
}





//============================================================================
// BDamageAction
//============================================================================

//============================================================================
// BDamageAction::BDamageAction
//============================================================================
BDamageAction::BDamageAction()
{
}


//============================================================================
// BDamageAction::~BDamageAction
//============================================================================
BDamageAction::~BDamageAction()
{
}


//============================================================================
// BDamageActionThrowPart
//============================================================================

//============================================================================
// BDamageActionThrowPart::BDamageActionThrowPart
//============================================================================
BDamageActionThrowPart::BDamageActionThrowPart()
{
   mType = cThrowPart;
   reset();
}


//============================================================================
// BDamageActionThrowPart::~BDamageActionThrowPart
//============================================================================
BDamageActionThrowPart::~BDamageActionThrowPart()
{
   reset();
}


//============================================================================
// BDamageActionThrowPart::reset
//============================================================================
void BDamageActionThrowPart::reset( void )
{
   mBPID = -1;
   mReleaseEffectID = -1;
   mStreamerEffectID = -1;
   mImpactSoundSetID = -1;
   mTerrainEffectID = -1;
   mForceMultiplier = 1.0f;
   mDisregardForce = false;

   long count = mParts.getNumber();
   for (long i = 0; i < count; i++)
   {
      delete mParts.get(i);
   }

   mParts.clear();
}


//============================================================================
// BDamageActionThrowPart::getPart
//============================================================================
const BDamagePart* BDamageActionThrowPart::getPart( long index ) const
{
   if (index < 0 || index >= mParts.getNumber())
      return NULL;

   return mParts.get(index);
}

//============================================================================
// BDamageActionThrowPart::load
//============================================================================
bool BDamageActionThrowPart::load( const BXMLNode &root, long modelIndex)
{
   reset();

   BGrannyModel *pGrannyModel = gGrannyManager.getModel(modelIndex, true);
   if(!pGrannyModel)
      return false;

   //-- there may be an optional physics blueprint declaration
   BSimString blueprintName;
   if (root.getAttribValue("blueprint", &blueprintName))
   {
      mBPID = gPhysics->getPhysicsObjectBlueprintManager().getOrCreate(BStrConv::toA(blueprintName));
   }
   else
   {
      mBPID = gPhysics->getPhysicsObjectBlueprintManager().getOrCreate("part");
   }

   //-- there may be optional releaseeffect, streamereffect or impactsound specified
   BSimString releaseEffectName;
   if (root.getAttribValueAsString("releaseeffect", releaseEffectName))
   {
      gParticleGateway.getOrCreateData(releaseEffectName, (BParticleEffectDataHandle&)mReleaseEffectID);
   }

   BSimString streamerEffectName;
   if (root.getAttribValueAsString("streamereffect", streamerEffectName))
   {
      gParticleGateway.getOrCreateData(streamerEffectName, (BParticleEffectDataHandle&)mStreamerEffectID);
   }

   BSimString impactSoundName;
   if (root.getAttribValueAsString("impactsound", impactSoundName))
   {
      mImpactSoundSetID = gDatabase.getImpactSoundSetIndex(impactSoundName.getPtr());
   }

   BSimString terrainEffectName;
   if (root.getAttribValueAsString("terraineffect", terrainEffectName))
   {
      mTerrainEffectID = gTerrainEffectManager.getOrCreateTerrainEffect(terrainEffectName, false);
   }

   float forceMultiplier;
   if (root.getAttribValueAsFloat("forcemultiplier", forceMultiplier))
   {
      mForceMultiplier = forceMultiplier;
   }

   bool disregardForce;
   if (root.getAttribValueAsBool("diregardforce", disregardForce))
   {
      mDisregardForce = disregardForce;
   }



/*
   mTransferFlagObject = root.getAttribute("transferFlagObject");
*/



   // Read all parts
   //
   BSimString tempStr;
   const BSimString &partNameListStr = root.getText(tempStr);
   if (!partNameListStr.isEmpty())
   {
      BSimString partName;
      long strLen = partNameListStr.length();
      long loc = partName.copyTok(partNameListStr, strLen, -1, B(","));
      while (loc != -1)
      {
         long count = pGrannyModel->getNumMeshes();
         for (long i= 0; i < count; i++)
         {
            BSimString meshName;
            if (pGrannyModel->getMeshName(i, meshName))
            {
               if (meshName.compare(partName) != 0)
                  continue;


               BDamagePart *pDamagePart = new BDamagePart();
               if (pDamagePart)
               {
                  BVector min, max, centerOffset;
                  int boneHandle;

                  //-- figure out the bounds
                  pGrannyModel->getMeshBounds(i, min, max);

                  //-- find part center offset in model space
                  centerOffset = (min + max) / 2.0f;

                  //-- find bone linked to
                  boneHandle = pGrannyModel->getBoneHandle(pGrannyModel->getMeshBone(i));

                  pDamagePart->mMeshIndex = i;
                  pDamagePart->mMin = min;
                  pDamagePart->mMax = max;
                  pDamagePart->mCenterOffset = centerOffset;
                  pDamagePart->mBoneHandle = boneHandle;

                  mParts.add(pDamagePart);
               }

               break;
            }
         }

         loc = partName.copyTok(partNameListStr, strLen, loc+1, B(","));
      }
   }

   // Verify the that we have the valid number of parts
   long partCount = mParts.getNumber();

   if(partCount < 1)
   {
      char buf[512];
      sprintf_s(buf, sizeof(buf), "BDamageActionThrowPart::load: Invalid number of parts found for model \"%s\".  The number of parts must be >= 1 for a throwpart action.  Part Names: %s\n", BStrConv::toA(pGrannyModel->getFilename()), BStrConv::toA(partNameListStr));
      gConsoleOutput.output(cMsgError, buf);
      BASSERTM(false, buf);
      return false;
   }

   return (true);
}

//============================================================================
// BDamageActionThrowPart::execute
//============================================================================
void BDamageActionThrowPart::execute(BUnit* pUnit, bool bVisible, const BVector *pModelSpacePoint, float force, bool ballisticHit, const BVector* pOverrideForceDir, BEntityID* outEntityID ) const
{
   // do nothing if disabled
   if (!gConfig.isDefined(cConfigEnableThrowPart))
      return;

   BASSERTM(getPartCount() >= 1, "Invalid number of parts in throw BDamageActionThrowPart");

   BVisual *pVisual = pUnit->getVisual();
   if (!pVisual)
      return;

   if (!pVisual->mpInstance)
      return;

   doExecuteSilent(pUnit);

   BBitArray mask = ((BGrannyInstance*)(pVisual->mpInstance))->getMeshRenderMask();

   BVector centerOffset(cOriginVector);
   BVector orientedLocation;

   long partcount = getPartCount();

   syncUnitDetailData("BDamageActionThrowPart::execute bVisible", bVisible);
   syncUnitDetailData("BDamageActionThrowPart::execute partcount", partcount);

   if(partcount == 1)
   {
      // Throw single part
      const BDamagePart *pPart = getPart(0);
      BASSERT(pPart);

      long meshIndex = pPart->mMeshIndex;

      if(pPart->mBoneHandle != -1)
      {
         BMatrix boneMatrix;
         if (pVisual->getBone(pPart->mBoneHandle, NULL, &boneMatrix))
         {
            boneMatrix.transformVectorAsPoint(pPart->mCenterOffset, centerOffset);
            syncUnitDetailData("BDamageActionThrowPart::execute pPart->mCenterOffset", pPart->mCenterOffset);
            syncUnitDetailData("BDamageActionThrowPart::execute centerOffset1", centerOffset);
         }
         else
         {
           centerOffset = pPart->mCenterOffset;
           syncUnitDetailData("BDamageActionThrowPart::execute centerOffset2", centerOffset);
         }
      }
      else
      {
         centerOffset = pPart->mCenterOffset;
         syncUnitDetailData("BDamageActionThrowPart::execute centerOffset3", centerOffset);
      }

      BMatrix matrix;
      pUnit->getRotation(matrix);
      matrix.transformVector(centerOffset, orientedLocation);

      //-- if we are visible then we actually throw a part off
      BASSERT(bVisible); // we can't actually check visilbity here since thrown parts are currently synchronous
      if (bVisible)
      {
         // Create shape
         BVector halfExtends;
         halfExtends = (pPart->mMax - pPart->mMin) * 0.5f;
         halfExtends.scale(PART_UNDEFINED_SHAPE_BOUNDING_BOX_FACTOR);
         syncUnitDetailData("BDamageActionThrowPart::execute halfExtents", halfExtends);
         const hkpShape* pShape = (hkpShape*) new hkpBoxShape(halfExtends);

         // Set physics params
         BPhysicsMatrix rot;
         rot.makeIdentity();
         rot.setForward(pUnit->getForward());
         rot.setUp(pUnit->getUp());
         rot.setRight(pUnit->getRight());

         BPhysicsObjectParams physicsparams;
         physicsparams.position = pUnit->getPosition();
         physicsparams.rotation = rot;
         physicsparams.centerOffset = centerOffset;
         physicsparams.pHavokShape = pShape;
         physicsparams.shapeHalfExtents = halfExtends;

         if(mBPID != -1)
         {
            const BPhysicsObjectBlueprint* pBluePrint = gPhysics->getPhysicsObjectBlueprintManager().get(mBPID);

            physicsparams.mass = pBluePrint->getMass();
            physicsparams.friction = pBluePrint->getFriction();
            physicsparams.restitution = pBluePrint->getRestitution();
            physicsparams.centerOfMassOffset = pBluePrint->getCenterOfMassOffset();
            physicsparams.angularDamping = pBluePrint->getAngularDamping();
            physicsparams.linearDamping = pBluePrint->getLinearDamping();
         }
         else
         {
            physicsparams.mass = 100.0f;
            physicsparams.restitution = 0.5f;
            physicsparams.friction = 0.5f;
            physicsparams.centerOfMassOffset = BVector(0.0f, 0.0f, 0.0f);
            physicsparams.angularDamping = 0.4f;
            physicsparams.linearDamping = 0.0f;
         }
         physicsparams.collisionFilterInfo = gWorld->getPhysicsWorld()->getUncollidableCollisionFilterInfo();
         physicsparams.userdata = 0;
         physicsparams.breakable = false;
         physicsparams.fixed = false;

        
         // Compute impulse force
         //BVector impulseForceWorldSpace = computeImpulseForce(orientedLocation, force, ballisticHit);
         BVector impulseForceWorldSpace;
         if (pOverrideForceDir && !mDisregardForce)
            impulseForceWorldSpace = computeImpulseForce(*pOverrideForceDir, force * mForceMultiplier, ballisticHit);
         else
            impulseForceWorldSpace = computeImpulseForce(orientedLocation, force * mForceMultiplier, ballisticHit);

         //-- calculate the impulse point (world space)
         BVector impulsePointWorldSpace = orientedLocation + pUnit->getPosition();
         
         BVector offsetPoint;

         //-- process any model space offset that we have
         if (pModelSpacePoint)
         {
            matrix.transformVectorAsPoint(*pModelSpacePoint, offsetPoint);
            impulsePointWorldSpace += offsetPoint;
         }

         // do an additional offset
         const float cMaxAdditionalOffset = 0.1f;
         BVector additionalOffset(getRandRangeFloat(cSimRand, -cMaxAdditionalOffset, cMaxAdditionalOffset), getRandRangeFloat(cSimRand, -cMaxAdditionalOffset, cMaxAdditionalOffset), getRandRangeFloat(cSimRand, -cMaxAdditionalOffset, cMaxAdditionalOffset));
         matrix.transformVectorAsPoint(additionalOffset, offsetPoint);
         impulsePointWorldSpace += offsetPoint;

         
         BBitArray newPartRenderMask = mask;
         newPartRenderMask.clear();
         newPartRenderMask.setBit(meshIndex);

         syncUnitDetailData("BDamageActionThrowPart::execute impulseForce", impulseForceWorldSpace);
         syncUnitDetailData("BDamageActionThrowPart::execute impulsePoint", impulsePointWorldSpace);

         // do the thing
         BEntityID id = pUnit->createAndThrow(impulseForceWorldSpace, impulsePointWorldSpace, physicsparams, newPartRenderMask, DEFAULT_THROW_PART_LIFETIME);
         pShape->removeReference(); // remove reference to shape as the rigid body now references it
         if ((id != cInvalidObjectID))
         {
            if (outEntityID)
               *outEntityID = id;
            pUnit->addThrownPart(id);
            BObject* pObject = gWorld->getObjectManager()->getObject(id);
            BVisual *pVisual = pObject->getVisual();

            // Preserve obsurable state
            pObject->setFlagObscurable(pUnit->getFlagObscurable());

            BUnitActionPhysics* pAction = reinterpret_cast<BUnitActionPhysics*>(pObject->getActionByType(BAction::cActionTypeUnitPhysics));
            if (pAction)
            {
               // Set impact sound set and add a collision listener
               if (mImpactSoundSetID != -1)
               {
                  BPhysicsObject* pPhysicsObject = pObject->getPhysicsObject();
                  BASSERT(pPhysicsObject);
                  pAction->setImpactSoundSet(mImpactSoundSetID);
                  pPhysicsObject->addGameCollisionListener(pAction);
                  pAction->setFlagCollisionListener(true);
               }

               // Set collision filter to change once the part is outside the unit it is being spawned in.
               pAction->enableDynamicCollisionFilter(gWorld->getPhysicsWorld()->createCollisionFilterInfo(BPhysicsWorld::cLayerObjects), pUnit->getID());
            }

            if (pVisual)
            {
               // add the streamer effect
               if (mStreamerEffectID != -1 && ballisticHit)
               {
                  BMatrix worldMatrix;
                  pObject->getWorldMatrix(worldMatrix);

                  BMatrix tranformMatrix;
                  tranformMatrix.makeTranslate(centerOffset);

                  DWORD playerColor = gWorld->getPlayerColor(pObject->getPlayerID(), BWorld::cPlayerColorContextObjects);

                  ((BVisualItem*)pVisual)->addAttachment(cVisualAssetParticleSystem,   //visualAssetType
                                             0,                            //animationTrack
                                             1,                            //animType
                                             0.0f,                         //tweenTime
                                             mStreamerEffectID, -1, -1.0f, true, playerColor, worldMatrix, &tranformMatrix, true);

                  // Need to update transform here or else the transform will never get set since
                  // the physics object has animations disabled.
                  ((BVisualItem*)pVisual)->updateAttachmentTransforms(true);
               }
            }

            //-- Play the piece thrown off sound
            BCueIndex cueIndex = pUnit->getProtoObject()->getSound(cObjectSoundPieceThrownOff);
            if(cueIndex != cInvalidCueIndex)
               gWorld->getWorldSoundManager()->addSound(pUnit, -1, cueIndex, true, cInvalidCueIndex, true, true);
         }
      }
   }
   else
   {
      // Throw a bunch of attached parts which break upon impact
      //

      // Compute composite center offset
      centerOffset.zero();

      for (long partindex = 0; partindex < partcount; partindex++)
      {
         const BDamagePart *pPart = getPart(partindex);
         BASSERT(pPart);

         BVector partCenterOffset(cOriginVector);
         if(pPart->mBoneHandle != -1)
         {
            BMatrix boneMatrix;
            if (pVisual->getBone(pPart->mBoneHandle, NULL, &boneMatrix))
               boneMatrix.transformVectorAsPoint(pPart->mCenterOffset, partCenterOffset);
            else
               partCenterOffset = pPart->mCenterOffset;
         }
         else
         {
            partCenterOffset = pPart->mCenterOffset;
         }

         centerOffset += partCenterOffset;
      }

      centerOffset.scale(1.0f / partcount);

      BMatrix matrix;
      pUnit->getRotation(matrix);
      matrix.transformVector(centerOffset, orientedLocation);



      //-- if we are visible then we actually throw a part off
      if (bVisible)
      {
         // Create shape
         BDynamicSimArray<hkpShape*> translateShapes;

         for (long partindex = 0; partindex < partcount; partindex++)
         {
            const BDamagePart *pPart = getPart(partindex);
            BASSERT(pPart);

            BVector partCenterOffset(cOriginVector);
            if(pPart->mBoneHandle != -1)
            {
               BMatrix boneMatrix;
               if (pVisual->getBone(pPart->mBoneHandle, NULL, &boneMatrix))
                  boneMatrix.transformVectorAsPoint(pPart->mCenterOffset, partCenterOffset);
               else
                  partCenterOffset = pPart->mCenterOffset;
            }
            else
            {
               partCenterOffset = pPart->mCenterOffset;
            }
            
      
            BVector halfExtends;
            halfExtends = (pPart->mMax - pPart->mMin) * 0.5f;
            halfExtends.scale(PART_UNDEFINED_SHAPE_BOUNDING_BOX_FACTOR);

//-- FIXING PREFIX BUG ID 5277
            const hkpConvexShape* boxShape = new hkpBoxShape( hkVector4( halfExtends.x, halfExtends.y, halfExtends.z) , 0 );
//--
            hkpConvexTranslateShape* translateShape = new hkpConvexTranslateShape( boxShape, hkVector4( partCenterOffset.x, partCenterOffset.y, partCenterOffset.z ));
            translateShape->setUserData(pPart->mMeshIndex);

            boxShape->removeReference();

            translateShapes.add(translateShape);
         }

         // Create list shape
         hkpListShape* listShape = new hkpListShape(translateShapes.getPtr(), partcount);
         BASSERT(listShape);

         for (int i =0; i < translateShapes.getNumber(); i++) 
         { 
            translateShapes[i]->removeReference();
         }

#ifndef DO_NOT_USE_MOPP
         // Create mopp around the list shape
         hkpMoppCompilerInput mci;
         hkpMoppCode* code = hkpMoppUtility::buildCode( listShape, mci);

         hkpMoppBvTreeShape* moppTreeShape = new hkpMoppBvTreeShape( listShape, code );
         BASSERT(moppTreeShape);

         code->removeReference();
         listShape->removeReference();
#endif


         // Set physics params
         BPhysicsMatrix rot;
         rot.makeIdentity();
         rot.setForward(pUnit->getForward());
         rot.setUp(pUnit->getUp());
         rot.setRight(pUnit->getRight());

         BPhysicsObjectParams physicsparams;
         physicsparams.position = pUnit->getPosition();
         physicsparams.rotation = rot;
         physicsparams.centerOffset = BVector(0.0f, 0.0f, 0.0f);

#ifndef DO_NOT_USE_MOPP
         physicsparams.pHavokShape = moppTreeShape;
#else
         physicsparams.pHavokShape = listShape;
#endif

         if(mBPID != -1)
         {
            const BPhysicsObjectBlueprint* pBluePrint = gPhysics->getPhysicsObjectBlueprintManager().get(mBPID);

            physicsparams.mass = pBluePrint->getMass();
            physicsparams.friction = pBluePrint->getFriction();
            physicsparams.restitution = pBluePrint->getRestitution();
            physicsparams.centerOfMassOffset = pBluePrint->getCenterOfMassOffset();
            physicsparams.angularDamping = pBluePrint->getAngularDamping();
            physicsparams.linearDamping = pBluePrint->getLinearDamping();
         }
         else
         {
            physicsparams.mass = 100.0f;
            physicsparams.restitution = 0.8f;
            physicsparams.friction = 0.5f;
            physicsparams.centerOfMassOffset = BVector(0.0f, 0.0f, 0.0f);
            physicsparams.angularDamping = 0.4f;
            physicsparams.linearDamping = 0.0f;
         }
         physicsparams.collisionFilterInfo = gWorld->getPhysicsWorld()->getUncollidableCollisionFilterInfo();
         physicsparams.userdata = 0;
         physicsparams.breakable = true;
         physicsparams.fixed = false;



         // Compute impulse force
         //BVector impulseForceWorldSpace = computeImpulseForce(orientedLocation, force, ballisticHit);
         BVector impulseForceWorldSpace;
         if (pOverrideForceDir && !mDisregardForce)
            impulseForceWorldSpace = computeImpulseForce(*pOverrideForceDir, force * mForceMultiplier, ballisticHit);
         else
            impulseForceWorldSpace = computeImpulseForce(orientedLocation, force * mForceMultiplier, ballisticHit);

         //-- calculate the impulse point (world space)
         BVector impulsePointWorldSpace = orientedLocation + pUnit->getPosition();

            
         // Build the render mask for the new part
         BBitArray newPartRenderMask = mask;
         newPartRenderMask.clear();

         for (long partindex = 0; partindex < partcount; partindex++)
         {
            const BDamagePart *pPart = getPart(partindex);
            newPartRenderMask.setBit(pPart->mMeshIndex);
         }


         // do the thing
         BEntityID id = pUnit->createAndThrow(impulseForceWorldSpace, impulsePointWorldSpace, physicsparams, newPartRenderMask, DEFAULT_THROW_PART_LIFETIME);
         listShape->removeReference(); // remove reference to shape as the rigid body now references it
         if ((id != cInvalidObjectID))
         {
            if (outEntityID)
               *outEntityID = id;
            pUnit->addThrownPart(id);
            BObject* pObject = gWorld->getObjectManager()->getObject(id);
            BVisual *pVisual = pObject->getVisual();    
            
            // Preserve obsurable state
            pObject->setFlagObscurable(pUnit->getFlagObscurable());
            
            BUnitActionPhysics* pAction = reinterpret_cast<BUnitActionPhysics*>(pObject->getActionByType(BAction::cActionTypeUnitPhysics));
            if (pAction)
            {
               bool addListener = false;

               // Set impact sound set
               if (mImpactSoundSetID != -1)
               {
                  pAction->setImpactSoundSet(mImpactSoundSetID);
                  addListener = true;
               }

               // set terrain effect
               if (mTerrainEffectID != -1)
               {
                  pAction->setTerrainEffect(mTerrainEffectID);
                  addListener = true;
               }

               // add listener if needed
               if(addListener)
               {
                  BPhysicsObject* pPhysicsObject = pObject->getPhysicsObject();
                  BASSERT(pPhysicsObject);
                  pPhysicsObject->addGameCollisionListener(pAction);
                  pAction->setFlagCollisionListener(true);
               }

               // Set collision filter to change once the part is outside the unit it is being spawned in.
               pAction->enableDynamicCollisionFilter(gWorld->getPhysicsWorld()->createCollisionFilterInfo(BPhysicsWorld::cLayerObjects), pUnit->getID());
            }

#ifndef DO_NOT_USE_MOPP
            moppTreeShape->setUserData((long)pObject);
#else
            listShape->setUserData((long)pObject);
#endif

            if (pVisual)
            {
               // add the streamer effect
               if (mStreamerEffectID != -1 && ballisticHit)
               {
                  BMatrix worldMatrix;
                  pObject->getWorldMatrix(worldMatrix);

                  BMatrix tranformMatrix;
                  tranformMatrix.makeTranslate(centerOffset);

                  DWORD playerColor = gWorld->getPlayerColor(pObject->getPlayerID(), BWorld::cPlayerColorContextObjects);

                  ((BVisualItem*)pVisual)->addAttachment(cVisualAssetParticleSystem,   //visualAssetType
                                             0,                            //animationTrack
                                             1,                            //animType
                                             0.0f,                         //tweenTime
                                             mStreamerEffectID, -1, -1.0f, true, playerColor, worldMatrix, &tranformMatrix, true);

                  // Need to update transform here or else the transform will never get set since
                  // the physics object has animations disabled.
                  ((BVisualItem*)pVisual)->updateAttachmentTransforms(true);
               }
            }

            if (gConfig.isDefined(cConfigEnableSubbreakage))
            {
               hkpRigidBody *pRigidBody = pObject->getPhysicsObject()->getRigidBody();
               for (long partindex = 1; partindex < partcount; partindex++)
               {
                  gDamageTemplateManager.getBreakOffPartsUtil()->markPieceBreakable(pRigidBody, partindex, 100.0f );
               }
            }
         }

         //-- Play the piece thrown off sound
         BCueIndex cueIndex = pUnit->getProtoObject()->getSound(cObjectSoundPieceThrownOff);
         if(cueIndex != cInvalidCueIndex)
            gWorld->getWorldSoundManager()->addSound(pUnit, -1, cueIndex, true, cInvalidCueIndex, true, true);
      }
   }

   // now add release effect
   if (mReleaseEffectID != -1)
   {
      BMatrix worldMatrix;
      pUnit->getWorldMatrix(worldMatrix);

      DWORD playerColor = gWorld->getPlayerColor(pUnit->getPlayerID(), BWorld::cPlayerColorContextObjects);

      BMatrix tranformMatrix;
      tranformMatrix.makeTranslate(centerOffset);

      // orient the release effect to point in the same direction as impulse
      BVector forward, up, right;

      up = orientedLocation;
      up.normalize();

      if(up.dot(cYAxisVector) > 0.707f)
      {
         // Calc up
         forward.assignCrossProduct(up, cXAxisVector);
         forward.normalize();

         // Calc right   
         right.assignCrossProduct(forward, up);
         right.normalize();
      }
      else
      {
         // Calc right   
         right.assignCrossProduct(cYAxisVector, up);
         right.normalize();

         // Calc up
         forward.assignCrossProduct(up, right);
         forward.normalize();
      }

      BMatrix orientMatrix;
      orientMatrix.makeOrient(forward, up, right);

      tranformMatrix.mult(orientMatrix, tranformMatrix);

      ((BVisualItem*)pVisual)->addAttachment(cVisualAssetParticleSystem,   //visualAssetType
                                             0,                            //animationTrack
                                             1,                            //animType
                                             0.0f,                         //tweenTime
                                             mReleaseEffectID, -1, DEFAULT_RELEASE_EFFECT_LIFETIME, false, playerColor, worldMatrix, &tranformMatrix, true);
   }
}

//============================================================================
// BDamageActionThrowPart::doExecuteSilent
//============================================================================
void BDamageActionThrowPart::doExecuteSilent(BUnit* pUnit) const
{
   // do nothing if disabled
   if (!gConfig.isDefined(cConfigEnableThrowPart))
      return;

   BVisual *pVisual = pUnit->getVisual();
   if (!pVisual)
      return;

   BGrannyInstance* pGrannyInstance = (BGrannyInstance*)(pVisual->mpInstance);
   if (!pGrannyInstance)
      return;

   BBitArray mask = pGrannyInstance->getMeshRenderMask();

   long partcount = getPartCount();
   if(partcount == 1)
   {
      // Turn off single part
      const BDamagePart *pPart = getPart(0);
      mask.clearBit(pPart->mMeshIndex);
   }
   else
   {
      // Turn off all parts
      for (long partindex = 0; partindex < partcount; partindex++)
      {
         const BDamagePart *pPart = getPart(partindex);
         mask.clearBit(pPart->mMeshIndex);
      }
   }
            
   //-- set our mask state to make sure we don't render parts we threw off
   pGrannyInstance->setMeshRenderMask(mask);
}

//============================================================================
// BDamageActionThrowPart::undoExecuteSilent
//============================================================================
void BDamageActionThrowPart::undoExecuteSilent(BUnit* pUnit) const
{
   // do nothing if disabled
   if (!gConfig.isDefined(cConfigEnableThrowPart))
      return;

   BVisual *pVisual = pUnit->getVisual();
   if (!pVisual)
      return;

   BGrannyInstance* pGrannyInstance = (BGrannyInstance*)(pVisual->mpInstance);
   if (!pGrannyInstance)
      return;

   BBitArray mask = pGrannyInstance->getMeshRenderMask();

   long partcount = getPartCount();
   if(partcount == 1)
   {
      // Turn on single part
      const BDamagePart *pPart = getPart(0);
      mask.setBit(pPart->mMeshIndex);
   }
   else
   {
      // Turn on all parts
      for (long partindex = 0; partindex < partcount; partindex++)
      {
         const BDamagePart *pPart = getPart(partindex);
         mask.setBit(pPart->mMeshIndex);
      }
   }
            
   //-- set our mask state to make sure we don't render parts we threw off
   pGrannyInstance->setMeshRenderMask(mask);
}


//============================================================================
// BDamageActionSwapPart
//============================================================================

//============================================================================
// BDamageActionSwapPart::BDamageActionSwapPart
//============================================================================
BDamageActionSwapPart::BDamageActionSwapPart()
{
   mType = cSwapPart;
   reset();
}


//============================================================================
// BDamageActionSwapPart::~BDamageActionSwapPart
//============================================================================
BDamageActionSwapPart::~BDamageActionSwapPart()
{
   reset();
}


//============================================================================
// BDamageActionSwapPart::reset
//============================================================================
void BDamageActionSwapPart::reset( void )
{
   long count = mParts.getNumber();
   for (long i = 0; i < count; i++)
   {
      delete mParts.get(i);
   }

   mParts.clear();
}


//============================================================================
// BDamageActionSwapPart::getPart
//============================================================================
const BDamagePart* BDamageActionSwapPart::getPart( long index ) const
{
   if (index < 0 || index >= mParts.getNumber())
      return NULL;

   return mParts.get(index);
}

//============================================================================
// BDamageActionSwapPart::load
//============================================================================
bool BDamageActionSwapPart::load( const BXMLNode &root, long modelIndex)
{
   reset();

   BGrannyModel *pGrannyModel = gGrannyManager.getModel(modelIndex, true);
   if(!pGrannyModel)
      return false;


   // Read all parts
   //
   BSimString tempStr;
   const BSimString &partNameListStr = root.getText(tempStr);
   if (!partNameListStr.isEmpty())
   {
      BSimString partName;
      long strLen = partNameListStr.length();
      long loc = partName.copyTok(partNameListStr, strLen, -1, B(","));
      while (loc != -1)
      {
         long count = pGrannyModel->getNumMeshes();
         for (long i= 0; i < count; i++)
         {
            BSimString meshName;
            if (pGrannyModel->getMeshName(i, meshName))
            {
               if (meshName.compare(partName) != 0)
                  continue;


               BDamagePart *pDamagePart = new BDamagePart();
               if (pDamagePart)
               {
                  BVector min, max, centerOffset;
                  int boneHandle;

                  //-- figure out the bounds
                  pGrannyModel->getMeshBounds(i, min, max);

                  //-- find part center offset in model space
                  centerOffset = (min + max) / 2.0f;

                  //-- find bone linked to
                  boneHandle = pGrannyModel->getBoneHandle(pGrannyModel->getMeshBone(i));

                  pDamagePart->mMeshIndex = i;
                  pDamagePart->mMin = min;
                  pDamagePart->mMax = max;
                  pDamagePart->mCenterOffset = centerOffset;
                  pDamagePart->mBoneHandle = boneHandle;

                  mParts.add(pDamagePart);
               }

               break;
            }
         }

         loc = partName.copyTok(partNameListStr, strLen, loc+1, B(","));
      }
   }

   // Verify the that we have the valid number of parts
   long partCount = mParts.getNumber();
   if(partCount != 2)
   {
      char buf[512];
      sprintf_s(buf, sizeof(buf), "BDamageActionSwapPart::load: Invalid number of parts specified for model \"%s\".  The number of parts for a swappart action must be 2.  Part Names: %s\n", BStrConv::toA(pGrannyModel->getFilename()), BStrConv::toA(partNameListStr));
      gConsoleOutput.output(cMsgError, buf);
      BASSERTM(false, buf);
      return false;
   }

   return true;
}

//============================================================================
// BDamageActionSwapPart::execute
//============================================================================
void BDamageActionSwapPart::execute(BUnit* pUnit, bool bVisible, const BVector *pModelSpacePoint, float force, bool ballisticHit, const BVector* pOverrideForceDir, BEntityID* outEntityID ) const
{
   doExecuteSilent(pUnit);
}

//============================================================================
// BDamageActionSwapPart::doExecuteSilent
//============================================================================
void BDamageActionSwapPart::doExecuteSilent(BUnit* pUnit) const
{
   BASSERTM(getPartCount() == 2, "Invalid number of parts in swap BDamageActionSwapPart");

   BVisual *pVisual = pUnit->getVisual();
   if (!pVisual)
      return;

   BGrannyInstance* pGrannyInstance = (BGrannyInstance*)(pVisual->mpInstance);
   if (!pGrannyInstance)
      return;

   BBitArray mask = pGrannyInstance->getMeshRenderMask();

   long fromMeshIndex = getPart(0)->mMeshIndex;
   long toMeshIndex = getPart(1)->mMeshIndex;

   //-- turn off this part in the main model
   mask.clearBit(fromMeshIndex);

   //-- turn on this part in the main model
   mask.setBit(toMeshIndex);

   //-- set our mask state to make sure we don't render parts we threw off
   pGrannyInstance->setMeshRenderMask(mask);
}

//============================================================================
// BDamageActionSwapPart::undoExecuteSilent
//============================================================================
void BDamageActionSwapPart::undoExecuteSilent(BUnit* pUnit) const
{
   BASSERTM(getPartCount() == 2, "Invalid number of parts in swap BDamageActionSwapPart");

   BVisual *pVisual = pUnit->getVisual();
   if (!pVisual)
      return;

   BGrannyInstance* pGrannyInstance = (BGrannyInstance*)(pVisual->mpInstance);
   if (!pGrannyInstance)
      return;

   BBitArray mask = pGrannyInstance->getMeshRenderMask();

   long fromMeshIndex = getPart(0)->mMeshIndex;
   long toMeshIndex = getPart(1)->mMeshIndex;

   //-- turn on this part in the main model
   mask.setBit(fromMeshIndex);

   //-- turn off this part in the main model
   mask.clearBit(toMeshIndex);

   //-- set our mask state to make sure we don't render parts we threw off
   pGrannyInstance->setMeshRenderMask(mask);
}

//============================================================================
// BDamageActionHidePart
//============================================================================

//============================================================================
// BDamageActionHidePart::BDamageActionHidePart
//============================================================================
BDamageActionHidePart::BDamageActionHidePart()
{
   mType = cHidePart;
   reset();
}

//============================================================================
// BDamageActionHidePart::~BDamageActionHidePart
//============================================================================
BDamageActionHidePart::~BDamageActionHidePart()
{
   reset();
}

//============================================================================
// BDamageActionHidePart::reset
//============================================================================
void BDamageActionHidePart::reset( void )
{
   mAllParts = false;
   mParts.clear();
}

//============================================================================
// BDamageActionHidePart::load
//============================================================================
bool BDamageActionHidePart::load( const BXMLNode &root, long modelIndex)
{
   reset();

   BGrannyModel *pGrannyModel = gGrannyManager.getModel(modelIndex, true);
   if(!pGrannyModel)
      return false;


   // Read all parts
   //
   BSimString tempStr;
   const BSimString &partNameListStr = root.getText(tempStr);
   if (!partNameListStr.isEmpty())
   {
      if (partNameListStr == "all")
      {
         mAllParts = true;
      }
      else
      {
         BSimString partName;
         long strLen = partNameListStr.length();
         long loc = partName.copyTok(partNameListStr, strLen, -1, B(","));
         while (loc != -1)
         {
            long count = pGrannyModel->getNumMeshes();
            for (long i= 0; i < count; i++)
            {
               BSimString meshName;
               if (pGrannyModel->getMeshName(i, meshName))
               {
                  if (meshName.compare(partName) == 0)
                     mParts.add(i);
               }
            }

            loc = partName.copyTok(partNameListStr, strLen, loc+1, B(","));
         }
      }
   }

   return true;
}

//============================================================================
// BDamageActionHidePart::execute
//============================================================================
void BDamageActionHidePart::execute(BUnit* pUnit, bool bVisible, const BVector *pModelSpacePoint, float force, bool ballisticHit, const BVector* pOverrideForceDir, BEntityID* outEntityID ) const
{
   doExecuteSilent(pUnit);
}

//============================================================================
// BDamageActionHidePart::doExecuteSilent
//============================================================================
void BDamageActionHidePart::doExecuteSilent(BUnit* pUnit) const
{
   BVisual *pVisual = pUnit->getVisual();
   if (!pVisual)
      return;

   if (mAllParts)
   {
      pVisual->setGrannyMeshMask(false);
   }
   else
   {
      BGrannyInstance* pGrannyInstance = (BGrannyInstance*)(pVisual->mpInstance);
      if (pGrannyInstance)
      {
         BBitArray mask = pGrannyInstance->getMeshRenderMask();
         for (long i = 0; i < mParts.getNumber(); ++i)
            mask.clearBit(mParts[i]);
         pGrannyInstance->setMeshRenderMask(mask);
      }
   }
}

//============================================================================
// BDamageActionHidePart::undoExecuteSilent
//============================================================================
void BDamageActionHidePart::undoExecuteSilent(BUnit* pUnit) const
{
   BVisual *pVisual = pUnit->getVisual();
   if (!pVisual)
      return;

   if (mAllParts)
   {
      pVisual->setGrannyMeshMask(false);
   }
   else
   {
      BGrannyInstance* pGrannyInstance = (BGrannyInstance*)(pVisual->mpInstance);
      if (pGrannyInstance)
      {
         BBitArray mask = pGrannyInstance->getMeshRenderMask();
         for (long i = 0; i < mParts.getNumber(); ++i)
            mask.clearBit(mParts[i]);
         pGrannyInstance->setMeshRenderMask(mask);
      }
   }
}


//============================================================================
// BDamageActionThrowAttachment
//============================================================================

//============================================================================
// BDamageActionThrowAttachment::BDamageActionThrowAttachment
//============================================================================
BDamageActionThrowAttachment::BDamageActionThrowAttachment()
{
   mType = cThrowAttachment;
   reset();
}


//============================================================================
// BDamageActionThrowAttachment::~BDamageActionThrowAttachment
//============================================================================
BDamageActionThrowAttachment::~BDamageActionThrowAttachment()
{
   reset();
}


//============================================================================
// BDamageActionThrowAttachment::reset
//============================================================================
void BDamageActionThrowAttachment::reset( void )
{
   mBPID = -1;
   mReleaseEffectID = -1;
   mStreamerEffectID = -1;
   mImpactSoundSetID = -1;
   mForceMultiplier = 1.0f;
   mDisregardForce = false;

   mBoneHandle = -1;
}


//============================================================================
// BDamageActionThrowAttachment::load
//============================================================================
bool BDamageActionThrowAttachment::load( const BXMLNode &root, long modelIndex)
{
   reset();

   BGrannyModel *pGrannyModel = gGrannyManager.getModel(modelIndex, true);
   if(!pGrannyModel)
      return false;


   //-- there may be an optional physics blueprint declaration
   BSimString blueprintName;
   if (root.getAttribValue("blueprint", &blueprintName))
   {
      mBPID = gPhysics->getPhysicsObjectBlueprintManager().getOrCreate(BStrConv::toA(blueprintName));
   }
   else
   {
      mBPID = gPhysics->getPhysicsObjectBlueprintManager().getOrCreate("part");
   }

   //-- there may be optional releaseeffect or streamereffect specified
   BSimString releaseEffectName;
   if (root.getAttribValueAsString("releaseeffect", releaseEffectName))
   {
      gParticleGateway.getOrCreateData(releaseEffectName, (BParticleEffectDataHandle&)mReleaseEffectID);
   }

   BSimString streamerEffectName;
   if (root.getAttribValueAsString("streamereffect", streamerEffectName))
   {
      gParticleGateway.getOrCreateData(streamerEffectName, (BParticleEffectDataHandle&)mStreamerEffectID);
   }

   BSimString impactSoundName;
   if (root.getAttribValueAsString("impactsound", impactSoundName))
   {
      mImpactSoundSetID = gDatabase.getImpactSoundSetIndex(impactSoundName.getPtr());
   }

   //-- read bone name (only one attachment per throwattachment action is valid)
   BSimString szBone;
   if (root.getText(szBone))
   {
      mBoneHandle = pGrannyModel->getBoneHandle(BStrConv::toA(szBone));

      if (mBoneHandle == -1)
      {
         gConsoleOutput.output(cMsgError, "BDamageActionThrowAttachment::load: Invalid bone name \"%s\" for model \"%s\" in throwattachment action.\n", BStrConv::toA(szBone), BStrConv::toA(pGrannyModel->getFilename()));
         return (false);
      }
   }

   float forceMultiplier;
   if (root.getAttribValueAsFloat("forcemultiplier", forceMultiplier))
   {
      mForceMultiplier = forceMultiplier;
   }

   bool disregardForce;
   if (root.getAttribValueAsBool("diregardforce", disregardForce))
   {
      mDisregardForce = disregardForce;
   }


   return (true);
}

//============================================================================
// BDamageActionThrowAttachment::execute
//============================================================================
void BDamageActionThrowAttachment::execute(BUnit* pUnit, bool bVisible, const BVector *pModelSpacePoint, float force, bool ballisticHit, const BVector* pOverrideForceDir, BEntityID* outEntityID ) const
{
   BASSERTM(mBoneHandle != -1, "Invalid bone for action BDamageActionThrowAttachment");
   
   BVisual *pVisual = pUnit->getVisual();
   if (!pVisual)
      return;

   BVector centerOffset(0.0f, 0.0f, 0.0f);
   BVector orientedLocation;


   BMatrix boneMatrix;
   pVisual->getBone(mBoneHandle, NULL, &boneMatrix);

   boneMatrix.transformVectorAsPoint(centerOffset, centerOffset);


   BMatrix matrix;
   pUnit->getRotation(matrix);
   matrix.transformVector(centerOffset, orientedLocation);



   // Find attachment on bone
   BVisualItem *pAttachment = NULL;

   uint32 numAttachments = static_cast<uint32>(pVisual->mAttachments.getNumber());
   for (uint32 i = 0; i < numAttachments; i++)
   {
      BVisualItem* pCurAttachment = pVisual->mAttachments[i];
      if(pCurAttachment->mToBoneHandle == mBoneHandle)
      {
         // Disregard non model attachments
         if (pCurAttachment->mModelAsset.mType != cVisualAssetGrannyModel)
            continue;

         pAttachment = pCurAttachment;
         break;
      }
   }


   if (pAttachment != NULL)
   {
      //-- turn off attachments
      pAttachment->setFlag(BVisualItem::cFlagVisible, false);

      //-- if we are visible then we actually throw a part off
      if (bVisible)
      {
         BVector attachmentCenterOffset;
         attachmentCenterOffset = (pAttachment->getMaxCorner() + pAttachment->getMinCorner()) * 0.5f;

         // Create shape
         BVector halfExtends;
         halfExtends = (pAttachment->getMaxCorner() - pAttachment->getMinCorner()) * 0.5f;
         halfExtends.scale(PART_UNDEFINED_SHAPE_BOUNDING_BOX_FACTOR);
         syncUnitDetailData("BDamageActionThrowAttachment::execute halfExtents", halfExtends);
         const hkpShape* pShape = (hkpShape*) new hkpBoxShape(halfExtends);

               
         // Build matrix for attachment
         BMatrix unitWorldMat;
         pUnit->getWorldMatrix(unitWorldMat);

         BMatrix finalMatrix;
         finalMatrix.mult(pAttachment->mMatrix, unitWorldMat);

         // Set physics params
         BPhysicsMatrix rot;
         rot.makeIdentity();
         rot.setForward(finalMatrix.getRow(2));
         rot.setUp(finalMatrix.getRow(1));
         rot.setRight(finalMatrix.getRow(0));

         BPhysicsObjectParams physicsparams;
         physicsparams.position = finalMatrix.getRow(3);
         physicsparams.rotation = rot;
         physicsparams.centerOffset = attachmentCenterOffset;
         physicsparams.pHavokShape = pShape;
         physicsparams.shapeHalfExtents = halfExtends;

         if(mBPID != -1)
         {
            const BPhysicsObjectBlueprint* pBluePrint = gPhysics->getPhysicsObjectBlueprintManager().get(mBPID);

            physicsparams.mass = pBluePrint->getMass();
            physicsparams.friction = pBluePrint->getFriction();
            physicsparams.restitution = pBluePrint->getRestitution();
            physicsparams.centerOfMassOffset = pBluePrint->getCenterOfMassOffset();
            physicsparams.angularDamping = pBluePrint->getAngularDamping();
            physicsparams.linearDamping = pBluePrint->getLinearDamping();
         }
         else
         {
            physicsparams.mass = 100.0f;
            physicsparams.restitution = 0.5f;
            physicsparams.friction = 0.5f;
            physicsparams.centerOfMassOffset = BVector(0.0f, 0.0f, 0.0f);
            physicsparams.angularDamping = 0.4f;
            physicsparams.linearDamping = 0.0f;
         }
         physicsparams.collisionFilterInfo = gWorld->getPhysicsWorld()->getUncollidableCollisionFilterInfo();
         physicsparams.userdata = 0;
         physicsparams.breakable = false;
         physicsparams.fixed = false;


         // Compute impulse force
         //BVector impulseForceWorldSpace = computeImpulseForce(orientedLocation, force, ballisticHit);
         BVector impulseForceWorldSpace;
         if (pOverrideForceDir && !mDisregardForce)
            impulseForceWorldSpace = computeImpulseForce(*pOverrideForceDir, force * mForceMultiplier, ballisticHit);
         else
            impulseForceWorldSpace = computeImpulseForce(orientedLocation, force * mForceMultiplier, ballisticHit);


         // Throw attachment
         BEntityID id = pUnit->createAndThrowAttachment(pAttachment, impulseForceWorldSpace, physicsparams, pUnit->getPlayerID(), unitWorldMat, attachmentCenterOffset, DEFAULT_THROW_PART_LIFETIME);

         if ((id != cInvalidObjectID))
         {
            BObject* pObject = gWorld->getObjectManager()->getObject(id);
            BVisual *pVisual = pObject->getVisual();

            // Preserve obsurable state
            pObject->setFlagObscurable(pUnit->getFlagObscurable());

            BUnitActionPhysics* pAction = reinterpret_cast<BUnitActionPhysics*>(pObject->getActionByType(BAction::cActionTypeUnitPhysics));
            if (pAction)
            {
               // Set impact sound set and add a collision listener
               if (mImpactSoundSetID != -1)
               {
                  BPhysicsObject* pPhysicsObject = pObject->getPhysicsObject();
                  BASSERT(pPhysicsObject);
                  pAction->setImpactSoundSet(mImpactSoundSetID);
                  pPhysicsObject->addGameCollisionListener(pAction);
                  pAction->setFlagCollisionListener(true);
               }

               // Set collision filter to change once the part is outside the unit it is being spawned in.
               pAction->enableDynamicCollisionFilter(gWorld->getPhysicsWorld()->createCollisionFilterInfo(BPhysicsWorld::cLayerObjects), pUnit->getID());
            }

            if (pVisual)
            {
               // add the streamer effect
               if (mStreamerEffectID != -1 && ballisticHit)
               {
                  BMatrix worldMatrix;
                  pObject->getWorldMatrix(worldMatrix);

                  BMatrix tranformMatrix;
                  tranformMatrix.makeTranslate(attachmentCenterOffset);

                  DWORD playerColor = gWorld->getPlayerColor(pObject->getPlayerID(), BWorld::cPlayerColorContextObjects);                  

                  ((BVisualItem*)pVisual)->addAttachment(cVisualAssetParticleSystem,   //visualAssetType
                                             0,                            //animationTrack
                                             1,                            //animType
                                             0.0f,                         //tweenTime
                                             mStreamerEffectID, -1, -1.0f, true, playerColor, worldMatrix, &tranformMatrix, true);

                  // Need to update transform here or else the transform will never get set since
                  // the physics object has animations disabled.
                  ((BVisualItem*)pVisual)->updateAttachmentTransforms(true);
               }
            }
         }
      }
   }


   // now add release effect
   if (mReleaseEffectID != -1)
   {
      BMatrix worldMatrix;
      pUnit->getWorldMatrix(worldMatrix);

      BMatrix tranformMatrix;
      tranformMatrix.makeTranslate(centerOffset);

      // orient the release effect to point in the same direction as impulse
      BVector forward, up, right;

      up = orientedLocation;
      up.normalize();

      if(up.dot(cYAxisVector) > 0.707f)
      {
         // Calc up
         forward.assignCrossProduct(up, cXAxisVector);
         forward.normalize();

         // Calc right   
         right.assignCrossProduct(forward, up);
         right.normalize();
      }
      else
      {
         // Calc right   
         right.assignCrossProduct(cYAxisVector, up);
         right.normalize();

         // Calc up
         forward.assignCrossProduct(up, right);
         forward.normalize();
      }

      BMatrix orientMatrix;
      orientMatrix.makeOrient(forward, up, right);

      tranformMatrix.mult(orientMatrix, tranformMatrix);

      DWORD playerColor = gWorld->getPlayerColor(pUnit->getPlayerID(), BWorld::cPlayerColorContextObjects);                  

      ((BVisualItem*)pVisual)->addAttachment(cVisualAssetParticleSystem,   //visualAssetType
                                             0,                            //animationTrack
                                             1,                            //animType
                                             0.0f,                         //tweenTime
                                             mReleaseEffectID, mBoneHandle, DEFAULT_RELEASE_EFFECT_LIFETIME, false, playerColor, worldMatrix, &tranformMatrix, true);
   }
}

//============================================================================
// BDamageActionThrowAttachment::doExecuteSilent
//============================================================================
void BDamageActionThrowAttachment::doExecuteSilent(BUnit* pUnit) const
{   
   BASSERTM(mBoneHandle != -1, "Invalid bone for action BDamageActionThrowAttachment");
   
   BVisual *pVisual = pUnit->getVisual();
   if (!pVisual)
      return;

   // Find attachment on bone
   BVisualItem *pAttachment = NULL;

   uint32 numAttachments = static_cast<uint32>(pVisual->mAttachments.getNumber());
   for (uint32 i = 0; i < numAttachments; i++)
   {
      BVisualItem* pCurAttachment = pVisual->mAttachments[i];
      if(pCurAttachment->mToBoneHandle == mBoneHandle)
      {
         // Disregard non model attachments
         if (pCurAttachment->mModelAsset.mType != cVisualAssetGrannyModel)
            continue;

         pAttachment = pCurAttachment;
         break;
      }
   }


   if (pAttachment != NULL)
   {
      //-- turn off attachments
      pAttachment->setFlag(BVisualItem::cFlagVisible, false);
   }
}

//============================================================================
// BDamageActionThrowAttachment::undoExecuteSilent
//============================================================================
void BDamageActionThrowAttachment::undoExecuteSilent(BUnit* pUnit) const
{
   BASSERTM(mBoneHandle != -1, "Invalid bone for action BDamageActionThrowAttachment");
   
   BVisual *pVisual = pUnit->getVisual();
   if (!pVisual)
      return;

   // Find attachment on bone
   BVisualItem *pAttachment = NULL;

   uint32 numAttachments = static_cast<uint32>(pVisual->mAttachments.getNumber());
   for (uint32 i = 0; i < numAttachments; i++)
   {
      BVisualItem* pCurAttachment = pVisual->mAttachments[i];
      if(pCurAttachment->mToBoneHandle == mBoneHandle)
      {
         // Disregard non model attachments
         if (pCurAttachment->mModelAsset.mType != cVisualAssetGrannyModel)
            continue;

         pAttachment = pCurAttachment;
         break;
      }
   }


   if (pAttachment != NULL)
   {
      //-- turn off attachments
      pAttachment->setFlag(BVisualItem::cFlagVisible, true);
   }
}


//============================================================================
// BDamageActionAttachParticle
//============================================================================

//============================================================================
// BDamageActionAttachParticle::BDamageActionAttachParticle
//============================================================================
BDamageActionAttachParticle::BDamageActionAttachParticle()
{
   mType = cAttachParticle;
   reset();
}


//============================================================================
// BDamageActionAttachParticle::~BDamageActionAttachParticle
//============================================================================
BDamageActionAttachParticle::~BDamageActionAttachParticle()
{
   reset();
}


//============================================================================
// BDamageActionAttachParticle::reset
//============================================================================
void BDamageActionAttachParticle::reset( void )
{
   mEffectID = -1;
   mLifeSpan = DEFAULT_ATTACHPARTICLE_LIFETIME;
   mBoneHandle = -1;
   mCenterOffset.zero();
}


//============================================================================
// BDamageActionAttachParticle::load
//============================================================================
bool BDamageActionAttachParticle::load( const BXMLNode &root, long modelIndex)
{
   reset();

   BGrannyModel *pGrannyModel = gGrannyManager.getModel(modelIndex, true);
   if(!pGrannyModel)
      return false;


   //-- Read effect name
   BSimString effectName;
   if (root.getAttribValueAsString("effect", effectName))
   {
      gParticleGateway.getOrCreateData(effectName, (BParticleEffectDataHandle&)mEffectID);
   }
   else
   {
      gConsoleOutput.output(cMsgError, "BDamageActionAttachParticle::load: No effect listed for attachparticle action\n");
      return(false);
   }

   //-- Read lifespan (optional)
   float lifespan;
   if(root.getAttribValueAsFloat("lifespan", lifespan))
   {
      mLifeSpan = lifespan;
   }


   //-- read bone or mesh name
   BSimString szBoneOrMeshName;
   if (root.getText(szBoneOrMeshName))
   {
      mBoneHandle = pGrannyModel->getBoneHandle(BStrConv::toA(szBoneOrMeshName));

      if (mBoneHandle == -1)
      {
         // look for a mesh name, since it isn't a bone name
         //
         long count = pGrannyModel->getNumMeshes();
         for (long i= 0; i < count; i++)
         {
            BSimString meshName;
            if (pGrannyModel->getMeshName(i, meshName))
            {
               if (meshName.compare(szBoneOrMeshName) != 0)
                  continue;

               //-- figure out the bounds
               BVector min, max;
               pGrannyModel->getMeshBounds(i, min, max);

               //-- find center offset in model space
               mCenterOffset = (min + max) / 2.0f;

               //-- find bone linked to
               mBoneHandle = pGrannyModel->getBoneHandle(pGrannyModel->getMeshBone(i));
               break;
            }
         }
      }

      // Verify the that we have a bone
      if(mBoneHandle == -1)
      {
         char buf[512];
         sprintf_s(buf, sizeof(buf), "BDamageActionAttachParticle::load: Bone or mesh name \"%s\" not found for model: %s\n", BStrConv::toA(szBoneOrMeshName), BStrConv::toA(pGrannyModel->getFilename()));
         gConsoleOutput.output(cMsgError, buf);
         BASSERTM(false, buf);
         return false;
      }
   }
   else
   {      
      char buf[512];
      sprintf_s(buf, sizeof(buf), "BDamageActionAttachParticle::load: Bone or mesh name is not specified for model: %s\n", BStrConv::toA(pGrannyModel->getFilename()));
      gConsoleOutput.output(cMsgError, buf);
      BASSERTM(false, buf);
      return false;
   }

   return true;
}

//============================================================================
// BDamageActionAttachParticle::execute
//============================================================================
void BDamageActionAttachParticle::execute(BUnit* pUnit, bool bVisible, const BVector *pModelSpacePoint, float force, bool ballisticHit, const BVector* pOverrideForceDir, BEntityID* outEntityID ) const
{
   doExecuteSilent(pUnit);
}


//============================================================================
// BDamageActionAttachParticle::doExecuteSilent
//============================================================================
void BDamageActionAttachParticle::doExecuteSilent(BUnit* pUnit) const
{
   BASSERT(mEffectID != -1);

   BVisual *pVisual = pUnit->getVisual();
   if (!pVisual)
      return;

   BMatrix worldMatrix;
   pUnit->getWorldMatrix(worldMatrix);

   BMatrix tranformMatrix;
   tranformMatrix.makeTranslate(mCenterOffset);

   DWORD playerColor = gWorld->getPlayerColor(pUnit->getPlayerID(), BWorld::cPlayerColorContextObjects);                  

   ((BVisualItem*)pVisual)->addAttachment(cVisualAssetParticleSystem,   //visualAssetType
                                          0,                            //animationTrack
                                          1,                            //animType
                                          0.0f,                         //tweenTime
                                          mEffectID, mBoneHandle, mLifeSpan, false, playerColor, worldMatrix, &tranformMatrix, true);
}

//============================================================================
// BDamageActionAttachParticle::undoExecuteSilent
//============================================================================
void BDamageActionAttachParticle::undoExecuteSilent(BUnit* pUnit) const
{
   BVisual *pVisual = pUnit->getVisual();
   if (!pVisual)
      return;

   // Find the attachment attached to the given bone, with the given effect
   bool bFound = false;
   long attachmentIndex = -1;
   long numAttachments = ((BVisualItem*)pVisual)->mAttachments.getNumber();

   for(long i = 0; i < numAttachments; i++)
   {
      BVisualItem *pAttachment = ((BVisualItem*)pVisual)->mAttachments[i];
      BASSERT(pAttachment);

      if((pAttachment->getFlag(BVisualItem::cFlagUser)) && 
         (pAttachment->mModelAsset.mType == cVisualAssetParticleSystem) &&
         (pAttachment->mModelAsset.mIndex == mEffectID) &&
         (pAttachment->mToBoneHandle == mBoneHandle))
      {
         attachmentIndex = i;
         bFound = true;
         break;
      }
   }

   // Remove the attachment if we have found it
   if(bFound)
   {
      BVisualItem *pAttachment = ((BVisualItem*)pVisual)->mAttachments[attachmentIndex];
      BVisualItem::releaseInstance(pAttachment);
      ((BVisualItem*)pVisual)->mAttachments.removeIndex(attachmentIndex, false);
   }
}



/*
//============================================================================
// BDamageActionUVOffset
//============================================================================

//============================================================================
// BDamageActionUVOffset::BDamageActionUVOffset
//============================================================================
BDamageActionUVOffset::BDamageActionUVOffset()
{
   mType = cUVOffset;
   reset();
}


//============================================================================
// BDamageActionUVOffset::~BDamageActionUVOffset
//============================================================================
BDamageActionUVOffset::~BDamageActionUVOffset()
{
   reset();
}


//============================================================================
// BDamageActionUVOffset::reset
//============================================================================
void BDamageActionUVOffset::reset( void )
{
   mChannel = 0;
   mUOffset = 0.0f;
   mVOffset = 0.0f;
}


//============================================================================
// BDamageActionUVOffset::load
//============================================================================
bool BDamageActionUVOffset::load( const BXMLNode &root, long modelIndex)
{
   reset();

   if(!root.getAttribValueAsUInt("channel", mChannel))
   {
      gConsoleOutput.output(cMsgError, "BDamageActionUVOffset::load: No channel specification\n");
      return(false);
   }

   if(!root.getAttribValueAsFloat("uOfs", mUOffset))
   {
      gConsoleOutput.output(cMsgError, "BDamageActionUVOffset::load: No U Offset specification\n");
      return(false);
   }

   if(!root.getAttribValueAsFloat("vOfs", mVOffset))
   {
      gConsoleOutput.output(cMsgError, "BDamageActionUVOffset::load: No V Offset specification\n");
      return(false);
   }

   return true;
}

//============================================================================
// BDamageActionUVOffset::execute
//============================================================================
void BDamageActionUVOffset::execute(BUnit* pUnit, bool bVisible, const BVector *pModelSpacePoint, float force, bool ballisticHit, const BVector* pOverrideForceDir, BEntityID* outEntityID ) const
{
   BVisual *pVisual = pUnit->getVisual();
   if (!pVisual)
      return;

   BVisualModelUVOffsets uvOffsets = pVisual->getUVOffsets();

   uvOffsets.mUVOffsets[mChannel] = BVec2(mUOffset,mVOffset);

   pVisual->setUVOffsets(uvOffsets);
}
*/





//============================================================================
// BDamageActionMultiframeTextureIndex
//============================================================================

//============================================================================
// BDamageActionMultiframeTextureIndex::BDamageActionMultiframeTextureIndex
//============================================================================
BDamageActionMultiframeTextureIndex::BDamageActionMultiframeTextureIndex()
{
   mType = cMultiframeTextureIndex;
   reset();
}


//============================================================================
// BDamageActionMultiframeTextureIndex::~BDamageActionMultiframeTextureIndex
//============================================================================
BDamageActionMultiframeTextureIndex::~BDamageActionMultiframeTextureIndex()
{
   reset();
}


//============================================================================
// BDamageActionMultiframeTextureIndex::reset
//============================================================================
void BDamageActionMultiframeTextureIndex::reset( void )
{
   mToIndex = 0;
   mFromIndex = 0;
}


//============================================================================
// BDamageActionMultiframeTextureIndex::load
//============================================================================
bool BDamageActionMultiframeTextureIndex::load( const BXMLNode &root, long modelIndex)
{
   reset();

   if(!root.getAttribValueAsUInt("fromindex", mFromIndex))
   {
      gConsoleOutput.output(cMsgError, "BDamageActionMultiframeTextureIndex::load: No fromindex specification\n");
      return(false);
   }

   if(!root.getAttribValueAsUInt("toindex", mToIndex))
   {
      gConsoleOutput.output(cMsgError, "BDamageActionMultiframeTextureIndex::load: No toindex specification\n");
      return(false);
   }

   return true;
}

//============================================================================
// BDamageActionMultiframeTextureIndex::execute
//============================================================================
void BDamageActionMultiframeTextureIndex::execute(BUnit* pUnit, bool bVisible, const BVector *pModelSpacePoint, float force, bool ballisticHit, const BVector* pOverrideForceDir, BEntityID* outEntityID ) const
{
   doExecuteSilent(pUnit);
}

//============================================================================
// BDamageActionMultiframeTextureIndex::doExecuteSilent
//============================================================================
void BDamageActionMultiframeTextureIndex::doExecuteSilent(BUnit* pUnit) const
{
   pUnit->setMultiframeTextureIndex(mToIndex);
}

//============================================================================
// BDamageActionMultiframeTextureIndex::undoExecuteSilent
//============================================================================
void BDamageActionMultiframeTextureIndex::undoExecuteSilent(BUnit* pUnit) const
{
   pUnit->setMultiframeTextureIndex(mFromIndex);
}



//==============================================================================
//==============================================================================
BVector computeImpulseForce(BVector pDir, float force, bool bBallisticHit)
{
   // Compute impulse
   //
   /*
   static s_velocityFactor = 150;
   static s_directionScalar = 15;

   // Factor in unit velocity
   BVector velocity = getVelocity();
   velocity.scale(s_velocityFactor);

   BVector direction = dmg.mDirection;
   direction.scale(s_directionScalar);

   BVector impulse = velocity + direction;
   */

   //-- calculate the impulse for this mesh
   BVector impulseForceWorldSpace = pDir;

   if (!impulseForceWorldSpace.safeNormalize())
   {
      //-- choose a random dir
      impulseForceWorldSpace.set(getRandRangeFloat(cSimRand, -1.0f, 1.0f), getRandRangeFloat(cSimRand, -1.0f, 1.0f), getRandRangeFloat(cSimRand, -1.0f, 1.0f));
   }

   // Randomize impulse direction
   const float cImpulseDirOffsetMultiplierMin = 0.6f;
   const float cImpulseDirOffsetMultiplierMax = 1.4f;
   impulseForceWorldSpace.x *= getRandRangeFloat(cSimRand, cImpulseDirOffsetMultiplierMin, cImpulseDirOffsetMultiplierMax);
   impulseForceWorldSpace.y *= getRandRangeFloat(cSimRand, cImpulseDirOffsetMultiplierMin, cImpulseDirOffsetMultiplierMax);
   impulseForceWorldSpace.z *= getRandRangeFloat(cSimRand, cImpulseDirOffsetMultiplierMin, cImpulseDirOffsetMultiplierMax);

   // give impulse an upward component
   impulseForceWorldSpace += BVector(0.0f, 1.0f, 0.0f);

   // for non-ballistic hits (e.g., cav unit chopping at the building), decrease the force
   const float cNonBallisticHitForceMultiplier = 0.2f;
   if(bBallisticHit)
      impulseForceWorldSpace.scale(force);
   else
      impulseForceWorldSpace.scale(force * cNonBallisticHitForceMultiplier);

   return(impulseForceWorldSpace);
}


//============================================================================
// BDamageActionPlaySound
//============================================================================

//============================================================================
// BDamageActionPlaySound::BDamageActionPlaySound
//============================================================================
BDamageActionPlaySound::BDamageActionPlaySound()
{
   mType = cPlaySound;
   reset();
}


//============================================================================
// BDamageActionPlaySound::~BDamageActionPlaySound
//============================================================================
BDamageActionPlaySound::~BDamageActionPlaySound()
{
   reset();
}


//============================================================================
// BDamageActionPlaySound::reset
//============================================================================
void BDamageActionPlaySound::reset( void )
{
   mSoundID = cInvalidCueIndex;
   mStopSoundID = cInvalidCueIndex;
}


//============================================================================
// BDamageActionPlaySound::load
//============================================================================
bool BDamageActionPlaySound::load( const BXMLNode &root, long modelIndex)
{
   reset();

   //-- Read effect name
   BSimString soundName;
   if (root.getText(soundName))
   {
      if(soundName.isEmpty() == false)
      {
         BCueIndex cue = gSoundManager.getCueIndex(soundName);                  
         BASSERT(cue != cInvalidCueIndex);
         mSoundID = cue;

         //if we have a soundcue, check for a stop sound
         BSimString stopSoundName;
         if (root.getAttribValue("stopSoundName", &stopSoundName))
         {
            mStopSoundID = gSoundManager.getCueIndex(stopSoundName);
         }
      }
   }
   else
   {
      gConsoleOutput.output(cMsgError, "BDamageActionPlaySound::load: No sound listed for playsound action\n");
      return(false);
   }

   return true;
}

//============================================================================
// BDamageActionPlaySound::execute
//============================================================================
void BDamageActionPlaySound::execute(BUnit* pUnit, bool bVisible, const BVector *pModelSpacePoint, float force, bool ballisticHit, const BVector* pOverrideForceDir, BEntityID* outEntityID ) const
{
   doExecuteSilent(pUnit);
}


//============================================================================
// BDamageActionPlaySound::doExecuteSilent
//============================================================================
void BDamageActionPlaySound::doExecuteSilent(BUnit* pUnit) const
{
   BASSERT(mSoundID != -1);

   if (mSoundID != cInvalidCueIndex)
   {
      if(mStopSoundID == cInvalidCueIndex)  //fire and forget sound
      {
         gWorld->getWorldSoundManager()->addSound(pUnit, -1, mSoundID, true, cInvalidCueIndex, true, true);
      }
      else   //we have an continuous exist sound
      {  
         BRTPCInitArray rtpc;
         rtpc.add( BRTPCInit(AK::GAME_PARAMETERS::FOW_DISTANCE, 0.0f) );
         gWorld->getWorldSoundManager()->addSound(pUnit, -1, mSoundID, false, mStopSoundID, false, false, &rtpc);
      }
   }
}

//============================================================================
// BDamageActionPlaySound::undoExecuteSilent
//============================================================================
void BDamageActionPlaySound::undoExecuteSilent(BUnit* pUnit) const
{
   if (mStopSoundID != cInvalidCueIndex)
   {
      gWorld->getWorldSoundManager()->addSound(pUnit, -1, mStopSoundID, false, cInvalidCueIndex, false, false);
   }
}



//============================================================================
// eof: damageaction.cpp
//============================================================================


