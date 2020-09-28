//============================================================================
// cost.cpp
//
// Copyright (c) 1999-2006 Ensemble Studios
//============================================================================

// Includes
#include "common.h"
#include "cost.h"
#include "database.h"
#include "game.h"
#include "savegame.h"
#include "xmlreader.h"

// Static members
long        BCost::msNumResources = 0;

GFIMPLEMENTVERSION(BCost, 1);

//============================================================================
// BCost::BCost
//============================================================================
BCost::BCost()
{
   init();
}

//============================================================================
// BCost::BCost
//============================================================================
BCost::BCost(const BCost &cost)
{
   init();
   *this = cost;
}
   
//============================================================================
// BCost::~BCost
//============================================================================
BCost::~BCost()
{
   kill();
}

//============================================================================
// BCost::init
//============================================================================
void BCost::init()
{
   Utils::FastMemSet(mpCosts, 0, BCost::cMaxNumResources * sizeof(float));
   mTotal = 0.0f;
}

//============================================================================
// BCost::kill
//============================================================================
void BCost::kill()
{
   Utils::FastMemSet(mpCosts, 0, BCost::cMaxNumResources * sizeof(float));
   mTotal = 0.0f;
}

//============================================================================
// BCost::operator=
//============================================================================
const BCost& BCost::operator=(const BCost& cost)
{
   set(&cost);
   return *this;
}

//============================================================================
// BCost::operator*=
//============================================================================
const BCost& BCost::operator*=(const float scale)
{
   BASSERTM(msNumResources > 0, "BCost::msNumResources <= 0!");
   mTotal = 0.0f;
   for (long resource = 0; resource < msNumResources; resource++)
   {
      mpCosts[resource] *= scale;
      mTotal += mpCosts[resource];
   }
   return *this;
}

//============================================================================
// BCost::isAtLeast
// Returns true if all resource values of the cost are >= the resource values
// of the argument cost.
//============================================================================
bool BCost::isAtLeast(const BCost& cost) const
{
   BASSERTM(msNumResources > 0, "BCost::msNumResources <= 0!");
   for (long i=0; i<msNumResources; i++)
   {
      if (mpCosts[i] < cost.mpCosts[i])
         return(false);
   }
   return(true);
}

//============================================================================
// BCost::setAll
//============================================================================
void BCost::setAll(float v)
{
   BASSERTM(msNumResources > 0, "BCost::msNumResources <= 0!");
   mTotal = 0.0f;
   for (long resource = 0; resource < msNumResources; resource++)
   {
      mpCosts[resource] = v;
      mTotal += v;
   }
}

//============================================================================
// BCost::multiply
//============================================================================
void BCost::multiply(float v)
{
   BASSERTM(msNumResources > 0, "BCost::msNumResources <= 0!");
   mTotal = 0.0f;
   for (long resource = 0; resource < msNumResources; resource++)
   {
      mpCosts[resource] *= v;
      mTotal += v;
   }
}

//============================================================================
// BCost::set
//============================================================================
void BCost::set(long resourceID, float v)
{
   BASSERTM(msNumResources > 0, "BCost::msNumResources <= 0!");
   BASSERTM(resourceID >= 0 && resourceID < msNumResources, "BCost::set() - resourceID is out of range!!");
   if (resourceID >= 0 && resourceID < msNumResources)
   {
      mTotal-=mpCosts[resourceID];
      mpCosts[resourceID]=v;
      mTotal+=v;
   }
}

//============================================================================
// BCost::set
//============================================================================
void BCost::set(const BCost*  pCost)
{
   BASSERTM(pCost, "BCost::set - pCost is NULL!");
   BASSERTM(msNumResources > 0, "BCost::msNumResources <= 0!");

   mTotal = 0.0f;
   if (pCost)
   {
      for (long resource = 0; resource < msNumResources; resource++)
      {
         mpCosts[resource] = pCost->mpCosts[resource];
         mTotal += mpCosts[resource];
      }
   }
}

//============================================================================
// BCost::add
//============================================================================
void BCost::add(long resourceID, float v)
{
   BASSERTM(msNumResources > 0, "BCost::msNumResources <= 0!");
   BASSERTM(resourceID >= 0 && resourceID < msNumResources, "BCost::set() - resourceID is out of range!!");
   if (resourceID >= 0 && resourceID < msNumResources)
   {
      mpCosts[resourceID] += v;
      mTotal += v;
   }
}

//============================================================================
// BCost::add
//============================================================================
void BCost::add(const BCost*  pCost)
{
   BASSERTM(pCost, "BCost::add - pCost is NULL!");
   BASSERTM(msNumResources > 0, "BCost::msNumResources <= 0!");

   mTotal = 0.0f;
   if (pCost)
   {
      for (long resource = 0; resource < msNumResources; resource++)
      {
         mpCosts[resource] += pCost->mpCosts[resource];
         mTotal += mpCosts[resource];
      }
   }
}

//============================================================================
// BCost::add
//============================================================================
void BCost::add(const BCost*  pCost, float percentage)
{
   BASSERTM(pCost, "BCost::add - pCost is NULL!");
   BASSERTM(msNumResources > 0, "BCost::msNumResources <= 0!");

   mTotal = 0.0f;
   if (pCost)
   {
      for (long resource = 0; resource < msNumResources; resource++)
      {
         mpCosts[resource] += (pCost->mpCosts[resource] * percentage);
         mTotal += mpCosts[resource];
      }
   }
}

//============================================================================
// BCost::subtract
//============================================================================
void BCost::subtract(const BCost*  pCost)
{
   BASSERTM(pCost, "BCost::subtract - pCost is NULL!");
   BASSERTM(msNumResources > 0, "BCost::msNumResources <= 0!");

   mTotal = 0.0f;
   if (pCost)
   {
      for (long resource = 0; resource < msNumResources; resource++)
      {
         mpCosts[resource] -= pCost->mpCosts[resource];
         mTotal += mpCosts[resource];
      }
   }
}

//============================================================================
// BCost::subtractDeductableOnly
//============================================================================
void BCost::subtractDeductableOnly(const BCost*  pCost)
{
   BASSERTM(pCost, "BCost::subtract - pCost is NULL!");
   BASSERTM(msNumResources > 0, "BCost::msNumResources <= 0!");

   mTotal = 0.0f;
   if (pCost)
   {
      for (long resource = 0; resource < msNumResources; resource++)
      {
         if (gDatabase.getResourceDeductable(resource))
         {
            mpCosts[resource] -= pCost->mpCosts[resource];
            mTotal += mpCosts[resource];
         }
      }
   }
}

//============================================================================
// BCost::zero
//============================================================================
void BCost::zero()
{
   Utils::FastMemSet(mpCosts, 0, BCost::cMaxNumResources * sizeof(float));
   mTotal = 0.0f;
}

//============================================================================
// BCost::get
//============================================================================
float BCost::get(long resourceID) const
{
   BASSERTM(msNumResources > 0, "BCost::msNumResources <= 0!");
   BASSERTM(resourceID >= 0 && resourceID < msNumResources, "BCost::get() - resourceID is out of range!!");
   if (resourceID >= 0 && resourceID < msNumResources)
      return mpCosts[resourceID];
   return (0.0f);
}

//============================================================================
// BCost::updateTotal
//============================================================================
void BCost::updateTotal()
{
   mTotal = 0.0f;
   BASSERTM(msNumResources > 0, "BCost::msNumResources <= 0!");
   for (long resource = 0; resource < msNumResources; resource++)
      mTotal+=mpCosts[resource];
}

//============================================================================
// BCost::load
//============================================================================
bool BCost::load(BXMLNode node)
{
   //FIXME AJL 4/25/06 - Change the format of the XML files to have a cost
   // node with multiple resource child nodes instead of individual cost 
   // nodes like we have now in objects.xml and techs.xml
   BSimString resourceName;
   if(node.getAttribValue("ResourceType", &resourceName))
   {
      long resourceID=gDatabase.getResource(resourceName);
      BASSERTM(msNumResources > 0, "BCost::msNumResources <= 0");
      BASSERTM(resourceID >= 0 && resourceID < msNumResources, "BCost::load() - resourceID is out of range!!");
      if (resourceID >= 0 && resourceID < msNumResources)
      {
         float amount=0.0f;
         node.getTextAsFloat(amount);
         mpCosts[resourceID]=amount;
         mTotal+=amount;
      }
   }
   return true;
}


//============================================================================
//============================================================================
bool BCost::save(BStream* pStream, int saveType) const
{
   for (int i=0; i<msNumResources; i++)
      GFWRITEVAR(pStream, float, mpCosts[i]);
   return true;
}

//============================================================================
//============================================================================
bool BCost::load(BStream* pStream, int saveType)
{
   mTotal = 0.0f;
   int count = gSaveGame.getNumberResources();
   for (int i=0; i<count; i++)
   {
      float amount;
      GFREADVAR(pStream, float, amount);
      int id = gSaveGame.getResourceID(i);
      BASSERT(id < msNumResources);
      if (id != -1 && id < msNumResources)
      {
         mpCosts[id] = amount;
         mTotal += amount;
      }
   }
   return true;
}
