//============================================================================
// damagetemplatemananger.h
//
// Copyright (c) 2004-2007 Ensemble Studios
//============================================================================
#pragma once


//============================================================================
// Includes
#include "visualmanager.h"

#include "physics.h"

//============================================================================
//  Forward Declarations
class BDamageTemplate;
class BWorld;
class BObject;

//============================================================================
// Const declarations

//============================================================================
// BDamageTemplateManager
//============================================================================
class BDamageTemplateManager : public BDamageTemplateInterface, public hkpBreakOffPartsListener
{
   public:
      BDamageTemplateManager();
      ~BDamageTemplateManager();


      void                          init(BWorld *pWorld);
      void                          reset( void );

      long                          findDamageTemplate(const BCHAR_T* pFileName);
      long                          createDamageTemplate(const BCHAR_T* pFileName, long modelindex);
      long                          getOrCreateDamageTemplate(const BCHAR_T* pFileName, long modelindex);
      const BDamageTemplate *       getDamageTemplate(long id) const;
      long                          getNumberDamageTemplates() const { return mDamageTemplates.getNumber(); }
      
      #ifndef BUILD_FINAL
      void                          reInitDamageTrackers(long index);
      #endif

      void                          ensureEmpty()  { BASSERT(mDamageTemplates.getNumber() == 0); BASSERT(mDamageNameTable.isEmpty()); }
      long                          getDamageTemplateCount() const   { return mDamageTemplates.getNumber(); }

      hkpBreakOffPartsUtil*         getBreakOffPartsUtil( void )  { return mBreakOffPartsUtil; }

	   /// hkpBreakOffPartsListener implementation
	   virtual hkResult breakOffSubPart(   const ContactImpulseLimitBreachedEvent& event, hkArray<hkpShapeKey>& keysBrokenOffOut, hkpPhysicsSystem& systemOut );

   protected:
      BSmallDynamicSimArray<BDamageTemplate*>      mDamageTemplates;
      BStringTable<short, false>                   mDamageNameTable;


      hkpBreakOffPartsUtil*       mBreakOffPartsUtil;
};


//============================================================================
// the global manager
//============================================================================
extern BDamageTemplateManager gDamageTemplateManager;


