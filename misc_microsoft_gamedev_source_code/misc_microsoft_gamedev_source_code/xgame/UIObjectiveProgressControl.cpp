//============================================================================
// UIObjectiveProgressControl.cpp
// Ensemble Studios (c) 2008
//============================================================================
#include "common.h"
#include "UIObjectiveProgressControl.h"
#include "world.h"
#include "objectivemanager.h"
#include "usermanager.h"
#include "user.h"
#include "uimanager.h"
#include "UITalkingHeadControl.h"

GFIMPLEMENTVERSION(BUIObjectiveProgressControl, 1);

//============================================================================
//============================================================================
BUIObjectiveProgressControl::BUIObjectiveProgressControl( void )
{
}

//============================================================================
//============================================================================
BUIObjectiveProgressControl::~BUIObjectiveProgressControl( void )
{
}

//============================================================================
//============================================================================
bool BUIObjectiveProgressControl::init( IUIControlEventHandler* parent, const char* controlPath, int controlID, BXMLNode* initData )
{
   bool result = __super::init( parent, controlPath, controlID, initData );

   for( int i = 0; result && i < NUM_LABELS; ++i )
   {
      BString labelPath;
      labelPath.format( "mLabel%d", i + 1 );
      result = mObjectiveLabels[i].init( this, mScriptPrefix + labelPath.getPtr() );
      if( result )
         mObjectiveLabels[i].hide();
   }

   return result;
}

//============================================================================
//============================================================================
void BUIObjectiveProgressControl::update( void )
{
   uint32 nTime = gWorld->getGametime();

   for( uint32 objIdx = 0; objIdx < mObjectives.getSize(); ++objIdx )
   {
      SObjective& obj = mObjectives[objIdx];
      
      int minIncrement = obj.nLabelIndex >= 0 ? 1 : obj.nMinIncrement;

      if( obj.pObjective &&
          ((obj.pObjective->getFinalCount() > 0 && obj.pObjective->getCurrentCount() - obj.nLastCount >= minIncrement) || 
           (obj.pObjective->getFinalCount() <= 0 && obj.pObjective->isCompleted() && obj.nLabelIndex == -1)) )
      {
         if( obj.pObjective->getFinalCount() > 0 )
            obj.nLastCount = obj.pObjective->getCurrentCount();
         
         obj.nFadeTime = nTime + obj.nDuration;
         
         if( obj.nLabelIndex < 0 )
            obj.nLabelIndex = getNextLabelIndex();

         if( obj.nLabelIndex >= 0 )
         {
            BUILabelControl& label = mObjectiveLabels[obj.nLabelIndex];

            if( obj.pObjective->getFinalCount() > 0 )
            {
               BUString progressText;
               progressText.format( L"%s   %d / %d", obj.pObjective->getTrackerText().getPtr(), obj.pObjective->getCurrentCount(), obj.pObjective->getFinalCount() );
               label.setText( progressText );
            }
            else
            {
               label.setText( obj.pObjective->getTrackerText() );
            }

            if( obj.pObjective->isCompleted() )
               obj.pObjective = NULL;
            
            label.show();
         }
      }
      else if( obj.nLabelIndex >= 0 && obj.nFadeTime < nTime )
      {
         mObjectiveLabels[obj.nLabelIndex].hide();
         obj.nLabelIndex = -1;
         
         if( obj.pObjective && obj.pObjective->isCompleted() )
            obj.pObjective = NULL;
      }
   }
}

//============================================================================
//============================================================================
void BUIObjectiveProgressControl::trackObjective( BObjective* pObjective )
{
   if( pObjective && pObjective->getTrackerText().length() > 0 )
   {
      for( uint i = 0; i < mObjectives.size(); ++i )
      {
         if( mObjectives[i].pObjective == pObjective )
            return;
      }

      mObjectives.add( SObjective( pObjective ) );
   }
}

//============================================================================
//============================================================================
int BUIObjectiveProgressControl::getNextLabelIndex( void )
{
   for( int i = 0; i < NUM_LABELS; ++i )
   {
      if( mObjectiveLabels[i].isShown() == false )
         return i;
   }
   return -1;
}

//============================================================================
//============================================================================
BUIObjectiveProgressControl::SObjective::SObjective( BObjective* objective ) :
   pObjective( objective ),
   nLastCount( objective ? objective->getCurrentCount() : 0 ),
   nFadeTime( 0 ),
   nLabelIndex( -1 ),
   nDuration( objective ? objective->getTrackerDuration() : 0 ),
   nMinIncrement( objective ? objective->getMinTrackerIncrement() : 0 )
{
}

//============================================================================
//============================================================================
bool BUIObjectiveProgressControl::save(BStream* pStream, int saveType) const
{
   for (int i=0; i<NUM_LABELS; i++)
   {
      const BUILabelControl& label = mObjectiveLabels[i];
      GFWRITEVAL(pStream, bool, label.isShown());
      const BUString& text = label.getText();
      GFWRITESTRING(pStream, BUString, text, 200);
   }
   GFWRITECLASSARRAY(pStream, saveType, mObjectives, uint8, 100);
   return true;
}

//============================================================================
//============================================================================
bool BUIObjectiveProgressControl::load(BStream* pStream, int saveType)
{
   for (int i=0; i<NUM_LABELS; i++)
   {
      bool isShown;
      GFREADVAR(pStream, bool, isShown);
      BUString text;
      GFREADSTRING(pStream, BUString, text, 200);
      if (isShown)
      {
         BUILabelControl& label = mObjectiveLabels[i];
         label.setText(text);
         label.show();
      }
   }
   GFREADCLASSARRAY(pStream, saveType, mObjectives, uint8, 100);
   return true;
}

//============================================================================
//============================================================================
bool BUIObjectiveProgressControl::SObjective::save(BStream* pStream, int saveType) const
{
   // pObjective
   BObjectiveID objectiveID = (pObjective ? pObjective->getID() : -1);
   GFWRITEVAR(pStream, BObjectiveID, objectiveID);

   GFWRITEVAR(pStream, int, nLastCount);
   GFWRITEVAR(pStream, uint32, nFadeTime);
   GFWRITEVAR(pStream, int, nLabelIndex);
   GFWRITEVAR(pStream, int, nDuration);
   GFWRITEVAR(pStream, int, nMinIncrement);

   return true;
}

//============================================================================
//============================================================================
bool BUIObjectiveProgressControl::SObjective::load(BStream* pStream, int saveType)
{
   // pObjective
   BObjectiveID objectiveID;
   GFREADVAR(pStream, BObjectiveID, objectiveID);
   if (objectiveID != -1)
      pObjective = gWorld->getObjectiveManager()->getObjective(gWorld->getObjectiveManager()->getIndexFromID(objectiveID));

   GFREADVAR(pStream, int, nLastCount);
   GFREADVAR(pStream, uint32, nFadeTime);
   GFREADVAR(pStream, int, nLabelIndex);
   GFREADVAR(pStream, int, nDuration);
   GFREADVAR(pStream, int, nMinIncrement);

   return true;
}
