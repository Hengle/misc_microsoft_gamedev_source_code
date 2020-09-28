//============================================================================
// cost.h
//
// Copyright (c) 1999-2007 Ensemble Studios
//============================================================================
#pragma once

#include "rockall/code/rockall/interface/SingleSizeHeap.hpp"
#include "xmlreader.h"
#include "gamefilemacros.h"

// Forward declarations

//============================================================================
// BCost
//============================================================================
class BCost
{
   public:

      enum { cMaxNumResources = 4, };

                           BCost ();
                           BCost (const BCost& cost);
                           ~BCost();

      // Operators
      const BCost&         operator =  (const BCost& cost);
      const BCost&         operator *= (const float scale);

      bool                 isAtLeast(const BCost& cost) const;

      // Modifiers
      void                 setAll  (float v);
      void                 multiply(float v);
      void                 set     (long resourceID, float v);
      void                 set     (const BCost*  pCost);
      void                 add     (long resourceID, float v);
      void                 add     (const BCost*  pCost);
      void                 add     (const BCost*  pCost, float percentage);
      void                 subtract(const BCost*  pCost);
      void                 subtractDeductableOnly(const BCost*  pCost);

      void                 zero    ();

      // Accessors
      float                get(long resourceID) const;

      float                getTotal() const           {return mTotal; }

      // Saving/Loading
      bool                 load(BXMLNode node);

      GFDECLAREVERSION();
      bool save(BStream* pStream, int saveType) const;
      bool load(BStream* pStream, int saveType);

      // Static Interface
      static void          initAllocator();
      static void          deinitAllocator();
      static long          getNumberResources() { return msNumResources; };
      static void          setNumberResources(long numResources) { msNumResources = numResources; }
      static bool          isValidResourceID(long resourceID) { return (resourceID >= 0 && resourceID < msNumResources); }

   protected:
      static long          msNumResources;            // Number of resources for all instances
      
      float                mpCosts[BCost::cMaxNumResources];   // Array of individual resource costs
      float                mTotal;                             // Total of all resources

   private:
      void                 init();
      void                 kill();
      void                 updateTotal();

};