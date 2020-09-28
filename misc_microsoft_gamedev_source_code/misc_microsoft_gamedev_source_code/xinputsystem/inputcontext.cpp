//==============================================================================
// inputcontext.cpp
//
// Copyright (c) 2004, Ensemble Studios
//==============================================================================

// Includes
#include "common.h"
#include "inputcontext.h"
#include "inputcontrol.h"
#include "xmlreader.h"

//==============================================================================
// BInputContext::BInputContext
//==============================================================================
BInputContext::BInputContext() :
   mName(),
   mPriority(0),
   mControlCount(0),
   mControlList(NULL)
{
}

//==============================================================================
// BInputContext::~BInputContext
//==============================================================================
BInputContext::~BInputContext()
{
   clear();
}

//==============================================================================
// BInputContext::setup
//==============================================================================
bool BInputContext::setup(BXMLNode node, IInputControlEventHandler* inputControlEventHandler)
{
   if(mControlList)
   {
      delete[]mControlList;
      mControlList=NULL;
   }

   BSimString name;
   if(node.getAttribValue("name", &name))
      mName=name;

   mControlCount=node.getNumberChildren();

   mControlList = new BInputControl[mControlCount];
   if(!mControlList)
   {
      BASSERT(0);
      return false;
   }

   for(long i=0; i<mControlCount; i++)
   {
      BXMLNode child(node.getChild(i));
      mControlList[i].setup(child, inputControlEventHandler);
   }

   return true;
}

//==============================================================================
// BInputContext::handleInput
//==============================================================================
bool BInputContext::handleInput(long port, long event, long controlType, BInputEventDetail& detail)
{
   for(long i=0; i<mControlCount; i++)
   {
      if(mControlList[i].handleInput(port, event, controlType, detail))
         return true;
   }
   return false;
}

//==============================================================================
// BInputContext::reset
//==============================================================================
void BInputContext::reset()
{
   for(long i=0; i<mControlCount; i++)
   {
      mControlList[i].reset();
   }
}

//==============================================================================
// BInputContext::clear
//==============================================================================
void BInputContext::clear()
{
   if(mControlList)
   {
      delete[]mControlList;
      mControlList=NULL;
   }
}
