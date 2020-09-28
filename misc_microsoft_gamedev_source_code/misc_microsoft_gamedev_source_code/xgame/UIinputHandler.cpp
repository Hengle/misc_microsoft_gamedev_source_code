//==============================================================================
// modepartyroom.cpp
//
// Copyright (c) Ensemble Studios, 2006-2007
//==============================================================================

// Includes
#include "common.h"
#include "UIInputHandler.h"

// xinput
#include "inputcontext.h"

//==============================================================================
//
//==============================================================================
BUIInputHandler::BUIInputHandler()
{
   for(long i=0; i<mContextList.getNumber(); i++)
      delete mContextList[i];
   mContextList.clear();
   mpActiveContext= NULL;
}

//==============================================================================
//
//==============================================================================
BUIInputHandler::~BUIInputHandler()
{
   for(long i=0; i<mContextList.getNumber(); i++)
      delete mContextList[i];
   mContextList.clear();
   mpActiveContext= NULL;
}

//==============================================================================
// BUIInputHandler::reset()
//==============================================================================
void  BUIInputHandler::reset()
{
   for(long i=0; i<mContextList.getNumber(); i++)
      mContextList[i]->reset();
}


//==============================================================================
// BUIInputHandler::enterContext
//==============================================================================
void BUIInputHandler::enterContext(const BCHAR_T* contextName)
{
   for(long i=0; i<mContextList.getNumber(); i++)
   {
      BInputContext* gamepadContext=mContextList[i];
      if(gamepadContext->getName()==contextName)
      {
         mpActiveContext= gamepadContext;
         break;
      }
   }
}

//==============================================================================
// 
//==============================================================================
bool BUIInputHandler::loadControls(BXMLNode controlsNode, IInputControlEventHandler* handler)
{
   for(long i=0; i<controlsNode.getNumberChildren(); i++)
   {
      BXMLNode child(controlsNode.getChild(i));

      const BPackedString name2(child.getName());

      if(name2==B("Mode"))
      {
         BSimString modeName;
         if(child.getAttribValue("name", &modeName))
         {
            BInputContext* context=NULL;
            for(long j=0; j<mContextList.getNumber(); j++)
            {
               BInputContext* item=mContextList[j];
               if(modeName.compare(item->getName())==0)
               {
                  context=item;
                  break;
               }
            }

            if(context)
            {
               if(!context->setup(child, handler))
                  return false;
            }
            else
            {
               BInputContext* context2=new BInputContext();
               if(!context2)
               {
                  BASSERT(0);
                  return false;
               }

               if(!context2->setup(child, handler))
               {
                  delete context2;
                  return false;
               }

               if(mContextList.add(context2)==-1)
               {
                  delete context2;
                  return false;
               }
            }
         }
      }
   }

   return true;

}

//==============================================================================
//
//==============================================================================
bool BUIInputHandler::loadControls(const BCHAR_T* name, IInputControlEventHandler* handler)
{
   BSimString path;
   path.set(name);

   BXMLReader reader;
   if(!reader.load(cDirProduction, path))
   {
      BASSERT(0);
      return false;
   }

   BXMLNode root(reader.getRootNode());

   return loadControls(root, handler);

}
//==============================================================================
// 
//==============================================================================
bool BUIInputHandler::handleInput(long port, long event, long controlType, BInputEventDetail& detail)
{
   if (mpActiveContext)
   {
      return mpActiveContext->handleInput(port, event, controlType, detail);
   }

   return false;
}
