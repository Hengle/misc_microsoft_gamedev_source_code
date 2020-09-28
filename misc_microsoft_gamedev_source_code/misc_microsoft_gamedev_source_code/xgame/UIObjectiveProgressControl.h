//============================================================================
// UIObjectiveProgressControl.h
// Ensemble Studios (c) 2008
//============================================================================
#pragma once
#include "UIControl.h"
#include "UILabelControl.h"
#include "gamefilemacros.h"

class BObjective;

class BUIObjectiveProgressControl : public BUIControl
{
public:
   enum Events { eObjectiveProgressControlEvent = UIObjectiveProgressControlID };

   IMPLEMENT_CONTROL_TYPE_ACCESSORS( UIOptionsMenuItemControl );
public:
   BUIObjectiveProgressControl( void );
   virtual ~BUIObjectiveProgressControl( void );

   virtual bool init( IUIControlEventHandler* parent, const char* controlPath, int controlID = -1, BXMLNode* initData = NULL );

   virtual void update( void );
   
   virtual void trackObjective( BObjective* pObjective );

   GFDECLAREVERSION();
   bool save(BStream* pStream, int saveType) const;
   bool load(BStream* pStream, int saveType);

protected:
   static const long NUM_LABELS = 4;
   BUILabelControl mObjectiveLabels[NUM_LABELS];

   struct SObjective
   {
      bool save(BStream* pStream, int saveType) const;
      bool load(BStream* pStream, int saveType);

      SObjective( BObjective* objective = NULL );
      BObjective* pObjective;
      int nLastCount;
      uint32 nFadeTime;
      int nLabelIndex;
      int nDuration;
      int nMinIncrement;
   };

   int getNextLabelIndex( void );

   BDynamicArray<SObjective> mObjectives;
};