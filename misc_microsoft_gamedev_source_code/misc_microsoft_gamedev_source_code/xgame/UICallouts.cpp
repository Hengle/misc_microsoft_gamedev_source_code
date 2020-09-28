//============================================================================
// uicallouts.cpp
// Ensemble Studios (c) 2007
//============================================================================

#include "common.h"
#include "uicallouts.h"

//-- render
#include "render.h"
#include "renderDraw.h"
#include "renderThread.h"

#include "camera.h"
#include "user.h"
#include "usermanager.h"
#include "player.h"
#include "uigame.h"
#include "visualmanager.h"
#include "database.h"
#include "world.h"
#include "hpbar.h"
#include "FontSystem2.h"
#include "timermanager.h"
#include "configsgame.h"
#include "gamefile.h"
#include "protosquad.h"

GFIMPLEMENTVERSION(BUICallouts, 1);
enum
{
   cSaveMarkerUICallouts1=10000,
   cSaveMarkerUICallouts2,
   cSaveMarkerUICallout,
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BUICallouts::BUICallouts():
mpMovie(NULL),
mPanelVisible(true),
mCalloutsVisible(false),
mNextCalloutID(0),
mX(0),
mY(0),
mWidth(0),
mHeight(0)
{
   for (int i=0; i<cNumCallouts; i++)
      mCalloutWidgets[i]=-1;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BUICallouts::~BUICallouts()
{
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BUICallouts::init(const char* filename, const char* datafile)
{
   if (!loadData(datafile))
      return false;

   gFlashGateway.getOrCreateData(filename, cFlashAssetCategoryInGame, mDataHandle);
   mpMovie = gFlashGateway.createInstance(mDataHandle, false);
   gFlashGateway.registerEventHandler(mpMovie, mSimEventHandle);

   initResolution();

   setFlag(cFlagInitialized, true);
   return true;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BUICallouts::deinit()
{
   if (mpMovie)
   {
      gFlashGateway.unregisterEventHandler(mpMovie, mSimEventHandle);
      gFlashGateway.releaseInstance(mpMovie);
   }
   mpMovie=NULL;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BUICallouts::enter()
{
   if (!getFlag(cFlagInitialized))
      return;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BUICallouts::leave()
{
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BUICallouts::update(float elapsedTime)
{   
   static bool bInitHack = false;
   if (bInitHack)
   {
      initResolution();
      bInitHack = false;
   }
//   mCalloutsVisible = updateCallouts();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BUICallouts::renderBegin()
{   
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BUICallouts::render()
{   
   SCOPEDSAMPLEID(FlashUIGame, 0xFFFF0000);

   mCalloutsVisible = updateCallouts();

   if(mpMovie && mPanelVisible && mCalloutsVisible)
      mpMovie->render();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BUICallouts::renderEnd()
{
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BUICallouts::handleInput(int port, BInputEventType event, BInputControlType controlType, BInputEventDetail& detail)
{
   return false;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BUICallouts::receiveEvent(const BEvent& event, BThreadIndex threadIndex)
{
   return false;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BManagedTextureHandle BUICallouts::getRenderTargetTexture()
{
   if (mpMovie)
      return mpMovie->mRenderTargetHandle;

   return cInvalidManagedTextureHandle;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BUICallouts::setDimension(int x, int y, int width, int height)
{
   if (!mpMovie)
      return;

   mWidth = width;
   mHeight = height;
   mX = x;
   mY = y;

   mpMovie->setDimension(x, y, width, height);
}

//==============================================================================
// BUICallouts::createCalloutObject
//==============================================================================
bool BUICallouts::createCalloutObject(BUICallout* pCallout, BVector position)
{
   // create an object at this location too
   BObjectCreateParms parms;
   parms.mProtoObjectID = gDatabase.getProtoObject("fx_arrowPointer_01");
   if (parms.mProtoObjectID == -1)
      return false;

   parms.mPlayerID = 0; // gaia
   parms.mPosition = position;
   parms.mNoTieToGround = true;

   BObject* pObject = gWorld->createObject(parms);
   if (!pObject)
      return false;

   pObject->setFlagNoRenderDuringCinematic(true);

   //if (pObject)
   //{
   //   pObject->setFlagVisibility(true);
   //   pObject->setFlagLOS(false);
   //   pObject->setFlagDopples(false);
   //}

   // save off the entityID of the 3D callout object
   pCallout->mCalloutEntityID=pObject->getID();

   return true;
}

//==============================================================================
// BUICallouts::addCallout
//==============================================================================
int BUICallouts::addCallout(BEntityID entity, uint type, long locStringID)
{
   // Clear any callouts already on this entity
   uint numCallouts = mCallouts.getSize();
   for (uint i = 0; i < numCallouts; i++)
   {
      BUICallout* pCallout = mCallouts[i];
      if (pCallout && (entity == pCallout->mEntityID))
      {
         setCalloutVisible(pCallout, false);
         freeCalloutWidget(pCallout);
         mCallouts.removeIndex(i);      
         break;
      }
   }

   // If an object callout and unit entity clear any callouts on the parent squad
   if ((type == BUICallout::cCalloutTypeObject) && (entity.getType() == BEntity::cClassTypeUnit))
   {
//-- FIXING PREFIX BUG ID 2991
      const BUnit* pUnit = gWorld->getUnit(entity);
//--
      if (pUnit)
      {
         BEntityID parentSquadID = pUnit->getParentID();
         uint numCallouts = mCallouts.getSize();
         for (uint i = 0; i < numCallouts; i++)
         {
            BUICallout* pCallout = mCallouts[i];
            if (pCallout && (pCallout->mEntityID == parentSquadID))
            {
               setCalloutVisible(pCallout, false);
               freeCalloutWidget(pCallout);
               mCallouts.removeIndex(i);
               break;
            }
         }
      }
   }

   BUICallout* pCallout = new BUICallout();
   pCallout->mID = mNextCalloutID;
   mNextCalloutID++;
   pCallout->mEntityID=entity;
   pCallout->mLocStringIndex = gDatabase.getLocStringIndex(locStringID);

   // attach this callout to an available callout widget
   if (!assignCalloutWidget(pCallout))
   {
      delete pCallout;
      return -1;
   }

   setCalloutText(pCallout);

   BVector position = cInvalidVector;

   // get the type
   long entityType = (long) entity.getType();
   switch (entityType)
   {
      case BEntity::cClassTypeUnit:
         {
            BUnit* pObject = gWorld->getUnit(entity);
            if (pObject)
            {
               float w, h;
               gHPBar.getHPBarPosition(*pObject, position, w, h);
            }
            pCallout->mType=BUICallout::cCalloutTypeUnit;

         }
         break;
      case BEntity::cClassTypeSquad:
         {
            BSquad* pObject = gWorld->getSquad(entity);
            if (pObject)
            {
               gHPBar.getSquadHPBarPosition(pObject, position);
            }
            pCallout->mType=BUICallout::cCalloutTypeSquad;
         }
         break;
   }

   if (position == cInvalidVector)
   {
      freeCalloutWidget(pCallout);
      delete pCallout;
      return -1;
   }

//    if (!createCalloutObject(pCallout, position))
//    {
//       freeCalloutWidget(pCallout);
//       delete pCallout;
//       return -1;
//    }

   mCallouts.add(pCallout);
   return pCallout->mID;
}

//==============================================================================
// BUICallouts::addCallout
//==============================================================================
int BUICallouts::addCallout(BVector area, int type, long locStringID)
{
   BUICallout* pCallout = new BUICallout();
   pCallout->mID = mNextCalloutID;
   mNextCalloutID++;
   pCallout->mType=BUICallout::cCalloutTypeLocation;
   pCallout->mLocation=area;
   pCallout->mLocStringIndex = gDatabase.getLocStringIndex(locStringID);

   // attach this callout to an available callout widget
   if (!assignCalloutWidget(pCallout))
   {
      delete pCallout;
      return -1;
   }

   setCalloutText(pCallout);

   BVector position = area;
   position.y += 5.0f;

//    if (!createCalloutObject(pCallout, position))
//    {
//       freeCalloutWidget(pCallout);
//       delete pCallout;
//       return -1;
//    }


   mCallouts.add(pCallout);
   return pCallout->mID;
}

//==============================================================================
// BUICallouts::removeCallout
//==============================================================================
bool BUICallouts::removeCallout(int id)
{
   bool freed = false;
   for (int i=0; i<mCallouts.getNumber(); i++)
   {
      BUICallout* pCallout = mCallouts[i];
      if (!pCallout)
         continue;

      if (pCallout->mID != id)
         continue;

      setCalloutVisible(pCallout, false);
      freeCalloutWidget(pCallout);

      mCallouts.removeIndex(i);

      freed = true;
      break;
   }
   return freed;
}

//==============================================================================
// BUICallouts::assignCalloutWidget
//==============================================================================
bool BUICallouts::assignCalloutWidget(BUICallout* pCallout)
{
   for (int i=0; i<cNumCallouts; i++)
   {
      if (mCalloutWidgets[i] != -1)
         continue;

      mCalloutWidgets[i]=pCallout->mID;
      pCallout->mUICalloutID=i;

      return true;
   }

   return false;
}

//==============================================================================
// BUICallouts::freeCalloutWidget
//==============================================================================
bool BUICallouts::freeCalloutWidget(BUICallout* pCallout)
{
   int widgetID = pCallout->mUICalloutID;
   if ( (widgetID <0 ) || (widgetID>=cNumCallouts))
      return false;

   if (mCalloutWidgets[widgetID] != pCallout->mID)
      return false;

   // we have a match, free it up
   mCalloutWidgets[widgetID] = -1;

   BObject* pObject = gWorld->getObject(pCallout->mCalloutEntityID);
   if (pObject)
   {
      pObject->destroy();
      gWorld->releaseObject(pObject);
   }
   delete pCallout;

   return true;
}



//==============================================================================
// BUICallouts::updateCallouts
//==============================================================================
bool BUICallouts::updateCallouts()
{
   if (!mpMovie)
      return false;

   bool needsRender = false;
   for (int i=0; i<mCallouts.getNumber(); i++)
   {
      BUICallout* pCallout = mCallouts[i];
      if (!pCallout)
         continue;

      BVector baseLoc;

      bool doUpdatePointer = false;
      switch (pCallout->mType)
      {
         case BUICallout::cCalloutTypeLocation:
            {
               //baseLoc = pCallout->mLocation;

//-- FIXING PREFIX BUG ID 2994
//                const BObject* pObject = gWorld->getObject(pCallout->mCalloutEntityID);
//--
//                if (!pObject)
//                {
//                   // turn off the callout
//                   setCalloutVisible(pCallout, false);
//                   break;
//                }
// 
//                doUpdatePointer = true;
// 
//                baseLoc = pObject->getPosition();
//                baseLoc.y += (pObject->getVisualRadius()*2.0f);

               baseLoc=pCallout->mLocation;
               doUpdatePointer=true;
            }
            break;
         case BUICallout::cCalloutTypeUnit:
            {
               BObject * pBaseObject = gWorld->getUnit(pCallout->mEntityID);
               if (!pBaseObject)
               {
                  // turn off the callout
                  // setCalloutVisible(pCallout, false);
                  removeCallout(pCallout->mID);          // If we don't have the entity we are connected to, we need to remove ourselves.
                  break;
               }

//                BObject* pObject = gWorld->getObject(pCallout->mCalloutEntityID);
//                if (!pObject)
//                {
//                   // turn off the callout
//                   setCalloutVisible(pCallout, false);
//                   break;
//                }

               // adjust for the movement of the squad
//                BVector newPosition = pObject->getPosition();
//                float w, h;
//                gHPBar.getHPBarPosition(*pBaseObject, newPosition, w, h);
//                #ifdef SYNC_Unit
//                   if (pObject->isClassType(BEntity::cClassTypeUnit))
//                      syncUnitData("BUICallouts::updateCallouts cCalloutTypeUnit", newPosition);
//                #endif
//                pObject->setPosition(newPosition);

               BVector newPosition = pBaseObject->getPosition();
               float w, h;
               gHPBar.getHPBarPosition(*pBaseObject, newPosition, w, h);
               if (newPosition==cInvalidVector)
               {
                  baseLoc = pBaseObject->getVisualCenter();
                  baseLoc.y += pBaseObject->getVisualRadius();
               }
               else
               {
                  BVector distance = newPosition-pBaseObject->getPosition();
                  baseLoc = pBaseObject->getPosition()+distance/2;
               }


               doUpdatePointer = true;
            }
            break;
         case BUICallout::cCalloutTypeSquad:
            {
               BSquad * pSquad = gWorld->getSquad(pCallout->mEntityID);
               if (!pSquad)
               {
                  // turn off the callout
                  // setCalloutVisible(pCallout, false);
                  removeCallout(pCallout->mID);          // If we don't have the entity we are connected to, we need to remove ourselves.
                  break;
               }

//                BObject* pObject = gWorld->getObject(pCallout->mCalloutEntityID);
//                if (!pObject)
//                {
//                   // turn off the callout
//                   setCalloutVisible(pCallout, false);
//                   break;
//                }
// 
//                // adjust for the movement of the squad
//                BVector newPosition = pObject->getPosition();
//                gHPBar.getSquadHPBarPosition(pSquad, newPosition);
//                #ifdef SYNC_Unit
//                   if (pObject->isClassType(BEntity::cClassTypeUnit))
//                      syncUnitData("BUICallouts::updateCallouts cCalloutTypeSquad", newPosition);
//                #endif
//                pObject->setPosition(newPosition);

               gHPBar.getSquadHPBarPosition(pSquad, baseLoc);
               if (baseLoc == cInvalidVector)
                  break;

               const BProtoSquad* pProto = pSquad->getProtoSquad();
               if (pProto)
               {
                  baseLoc -= pProto->getHPBarOffset()/2;
               }
               else
               {
                  baseLoc.y -= 5.0f/2;
               }


               doUpdatePointer = true;

            }
            break;
      }

      if (!doUpdatePointer)
         continue;

      // Convert anchor to screen point.
      if(!gRender.getViewParams().isPointOnScreen(baseLoc))
      {
         // turn this callout off
         setCalloutVisible(pCallout, false);
         continue;
      }

      long renderX = 0;
      long renderY = 0;

      gRender.getViewParams().calculateWorldToScreen(baseLoc, renderX, renderY);
      
      const float authoredWidth = 1280.0f;
      const float authoredHeight = 720.0f;      
            
      float finalX = ( ((float)(renderX + abs(mX))) / mWidth) * authoredWidth;
      float finalY = ( ((float)(renderY + abs(mY))) / mHeight) * authoredHeight;

      renderX = Math::FloatToIntRound(finalX);
      renderY = Math::FloatToIntRound(finalY);
      
      // set the location of this callout
      setCalloutPosition(pCallout, renderX, renderY);

      if (pCallout->mVisible)
         needsRender=true;
   }

   return needsRender;
}

//==============================================================================
// BUICallouts::setCalloutVisible
//==============================================================================
void BUICallouts::setCalloutVisible(BUICallout* pCallout, bool bVisible)
{
   if (!pCallout)
      return;

   // look at the cache
   if (pCallout->mVisible == bVisible)
      return;

   pCallout->mVisible=bVisible;
   GFxValue values[2];
   values[0].SetNumber(pCallout->mUICalloutID);
   values[1].SetBoolean(bVisible);
   mpMovie->invokeActionScript("setCalloutVisible", values, 2); 
}

//==============================================================================
// BUICallouts::setCalloutText
//==============================================================================
void BUICallouts::setCalloutText(BUICallout* pCallout)
{
   if (!pCallout)
      return;

   GFxValue values[2];
   values[0].SetNumber(pCallout->mUICalloutID);
   values[1].SetStringW(gDatabase.getLocStringFromIndex(pCallout->mLocStringIndex));
   mpMovie->invokeActionScript("setCalloutText", values, 2); 
}


//==============================================================================
// BUICallouts::setCalloutPosition
//==============================================================================
void BUICallouts::setCalloutPosition(BUICallout* pCallout, int x, int y)
{
   if (!pCallout)
      return;

   // check position and visibility
   if ( (pCallout->mX==x) && (pCallout->mY==y) && pCallout->mVisible)
      return;

   // update state
   pCallout->mX=x;
   pCallout->mY=y;
   pCallout->mVisible=true;

   GFxValue values[3];
   values[0].SetNumber(pCallout->mUICalloutID);
   values[1].SetNumber(x);
   values[2].SetNumber(y);
   mpMovie->invokeActionScript("setCalloutPosition", values, 3); 
}

//==============================================================================
//==============================================================================
bool BUICallouts::save(BStream* pStream, int saveType)
{
   //BFlashMovieInstance*       mpMovie;
   //BFlashGateway::BDataHandle mDataHandle;
   uint16 calloutCount = (uint16)mCallouts.size();
   GFWRITEVAR(pStream, uint16, calloutCount);
   GFVERIFYCOUNT(calloutCount, 10000);
   for (uint16 i=0; i<calloutCount; i++)
   {
      BUICallout* pCallout = mCallouts[i];
      if (pCallout)
      {
         GFWRITEVAR(pStream, uint16, i);
         pCallout->save(pStream, saveType);
         if (pCallout->mCalloutEntityID != cInvalidObjectID)
         {
            BVector position;
//-- FIXING PREFIX BUG ID 2995
            const BEntity* pEntity = gWorld->getEntity(pCallout->mCalloutEntityID);
//--
            position = (pEntity ? pEntity->getPosition() : cOriginVector);
            GFWRITEVECTOR(pStream, position);
         }
      }
   }
   GFWRITEVAL(pStream, uint16, UINT16_MAX);
   GFWRITEMARKER(pStream, cSaveMarkerUICallouts1);

   GFWRITEVAL(pStream, uint8, cNumCallouts);
   GFWRITEPTR(pStream, cNumCallouts * sizeof(long), mCalloutWidgets);
   GFWRITEVAR(pStream, int, mNextCalloutID);
   GFWRITEBITBOOL(pStream, mPanelVisible);
   GFWRITEBITBOOL(pStream, mCalloutsVisible);
   GFWRITEMARKER(pStream, cSaveMarkerUICallouts2);
   return true;
}

//==============================================================================
//==============================================================================
bool BUICallouts::load(BStream* pStream, int saveType)
{
   uint16 calloutCount;
   GFREADVAR(pStream, uint16, calloutCount);
   GFVERIFYCOUNT(calloutCount, 10000);
   if (!mCallouts.setNumber(calloutCount))
      return false;
   for (uint16 i=0; i<calloutCount; i++)
      mCallouts[i] = NULL;
   uint16 loadedCount = 0;
   uint16 calloutIndex;
   GFREADVAR(pStream, uint16, calloutIndex);
   while (calloutIndex != UINT16_MAX)
   {
      GFVERIFYCOUNT(calloutIndex, calloutCount-1);
      GFVERIFYCOUNT(loadedCount, calloutCount-1);
      BUICallout* pCallout = new BUICallout();
      if (!pCallout)
         return false;

      if (!pCallout->load(pStream, saveType))
      {
         delete pCallout;
         return false;
      }

      if (pCallout->mCalloutEntityID != cInvalidObjectID)
      {
         BVector position;
         GFREADVECTOR(pStream, position);
         pCallout->mCalloutEntityID = cInvalidObjectID;
         createCalloutObject(pCallout, position);
      }
      setCalloutText(pCallout);
      pCallout->mX=0;
      pCallout->mY=0;
      pCallout->mVisible=false;
      mCallouts[calloutIndex] = pCallout;
      GFREADVAR(pStream, uint16, calloutIndex);
   }
   GFREADMARKER(pStream, cSaveMarkerUICallouts1);

   uint8 numCallouts;
   GFREADVAR(pStream, uint8, numCallouts);
   GFVERIFYCOUNT(numCallouts, 20);
   for (uint8 i=0; i<numCallouts; i++)
   {
      long callOutWidget;
      GFREADVAR(pStream, long, callOutWidget);
      if (i < cNumCallouts)
         mCalloutWidgets[i] = callOutWidget;
   }

   GFREADVAR(pStream, int, mNextCalloutID);
   GFREADBITBOOL(pStream, mPanelVisible);
   GFREADBITBOOL(pStream, mCalloutsVisible);
   GFREADMARKER(pStream, cSaveMarkerUICallouts2);
   return true;
}

//==============================================================================
//==============================================================================
bool BUICallout::save(BStream* pStream, int saveType)
{
   GFWRITEVAR(pStream, int, mID);
   GFWRITEVAR(pStream, uint, mType);
   GFWRITEVECTOR(pStream, mLocation);
   GFWRITEVAR(pStream, BEntityID, mEntityID);
   GFWRITEVAR(pStream, BEntityID, mCalloutEntityID);
   GFWRITEVAR(pStream, long, mLocStringIndex);
   GFWRITEVAR(pStream, int, mUICalloutID);
   GFWRITEVAR(pStream, int, mX);
   GFWRITEVAR(pStream, int, mY);
   GFWRITEVAR(pStream, bool, mVisible);
   GFWRITEMARKER(pStream, cSaveMarkerUICallout);
   return true;
}

//==============================================================================
//==============================================================================
bool BUICallout::load(BStream* pStream, int saveType)
{
   GFREADVAR(pStream, int, mID);
   GFREADVAR(pStream, uint, mType);
   GFREADVECTOR(pStream, mLocation);
   GFREADVAR(pStream, BEntityID, mEntityID);
   GFREADVAR(pStream, BEntityID, mCalloutEntityID);
   GFREADVAR(pStream, long, mLocStringIndex);
   GFREADVAR(pStream, int, mUICalloutID);
   GFREADVAR(pStream, int, mX);
   GFREADVAR(pStream, int, mY);
   GFREADVAR(pStream, bool, mVisible);
   GFREADMARKER(pStream, cSaveMarkerUICallout);
   return true;
}
