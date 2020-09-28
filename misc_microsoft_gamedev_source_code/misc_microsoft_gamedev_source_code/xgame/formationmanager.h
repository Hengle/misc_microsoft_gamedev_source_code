//==============================================================================
// formationmanager.h
// Copyright (c) 2006 Ensemble Studios
//==============================================================================

#pragma once 

class BFormation2;



//==============================================================================
//==============================================================================
class BFormationManager
{
   public:

      BFormationManager();
      virtual ~BFormationManager();
      
      bool                       init();
      void                       reset();

      BFormation2*               createFormation2();   
      void                       releaseFormation2(BFormation2* pFormation);

   protected:
      long                       mRefCount;
      
      uint                       mIDCounter;
};

extern BFormationManager gFormationManager;

