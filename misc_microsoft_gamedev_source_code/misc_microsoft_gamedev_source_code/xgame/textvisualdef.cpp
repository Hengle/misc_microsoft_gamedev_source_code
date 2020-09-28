//==============================================================================
// textvisualdef.cpp
//
// Copyright (c) 2005, Ensemble Studios
//==============================================================================
#include "common.h"
#include "textvisualdef.h"
#include "xmlreader.h"
#include "textvisualmanager.h"
#include "textvisualeffect.h"
#include "textvisual.h"
#include "fontsystem2.h"
#include "gamedirectories.h"

//==============================================================================
// BTextVisualDefElement::BTextVisualDefElement
//==============================================================================
BTextVisualDefElement::BTextVisualDefElement() :
   mFont(NULL),
   mLifespan(0xFFFFFFFF),
   mFadeOutDuration(1000)
{
}

//==============================================================================
// BTextVisualDefElement::~BTextVisualDefElement
//==============================================================================
BTextVisualDefElement::~BTextVisualDefElement()
{
   // Nuke effects.
   for(long i=0; i<mEffects.getNumber(); i++)
      delete mEffects[i];
   mEffects.setNumber(0);
}

//==============================================================================
// BTextVisualDefElement::load
//==============================================================================
bool BTextVisualDefElement::load(BXMLNode root)
{
   // Sanity.
   if(!root)
      return(false);
    
   // Run through children.  
   for(long i=0; i<root.getNumberChildren(); i++)
   {
      // Get child.
      BXMLNode child(root.getChild(i));
      if(!child)
         continue;

      // Effect, if we create one.
      BTextVisualEffect *effect = NULL;
            
      // Font
      if(child.getName().compare("Font") == 0)
      {
         // Look up font.
         BSimString tempStr;
         mFont = gFontManager.findFont(child.getTextPtr(tempStr));
      }
      else if(child.getName().compare("Lifespan") == 0)
      {
         child.getTextAsDWORD(mLifespan);
      }
      else if(child.getName().compare("FadeOutDuration") == 0)
      {
         child.getTextAsDWORD(mFadeOutDuration);
      }
      else if(child.getName().compare("Position") == 0)
      {  
         // Allocate.
         effect = new BTextVisualEffectPosition;
      }
      else if(child.getName().compare("Color") == 0)
      {  
         // Allocate.
         effect = new BTextVisualEffectColor;
      }
      else if(child.getName().compare("Alpha") == 0)
      {  
         // Allocate.
         effect = new BTextVisualEffectAlpha;
      }
      else if(child.getName().compare("Icon") == 0)
      {
         // Allocate.
         effect = new BTextVisualEffectIcon;
      }
      
      // If it's an effect, load/add.
      if(effect)
      {
         // Load.
         bool ok = effect->load(child);
         
         // Add/nuke based on result.
         if(ok)
         {
            // Add to list.
            mEffects.add(effect);
            
            // Mark as managed by us so it won't get auto-deleted.
            effect->setManagedByDef(true);
         }
         else
         {
            // Failed, so just nuke.
            delete effect;
         }
      }
   }
   
   // Success.
   return(true);
}


//==============================================================================
// BTextVisualDefElement::initVisual
//==============================================================================
void BTextVisualDefElement::initVisual(BTextVisual *vis)
{
   // Sanity.
   if(!vis)
      return;

   // Basic info.
   vis->setFont(mFont);
   vis->setLifespan(mLifespan);
   vis->setFadeoutDuration(mFadeOutDuration);
   
   // Effects.
   for(long i=0; i<mEffects.getNumber(); i++)
      vis->addEffect(mEffects[i]);      
}

//==============================================================================
// BTextVisualDef::BTextVisualDef
//==============================================================================
BTextVisualDef::BTextVisualDef()
{
}


//==============================================================================
// BTextVisualDef::~BTextVisualDef
//==============================================================================
BTextVisualDef::~BTextVisualDef()
{
   // Clean up elements.
   for(long i=0; i<mElements.getNumber(); i++)
      delete mElements[i];
   mElements.setNumber(0);
}


//==============================================================================
// BTextVisualDef::load
//==============================================================================
bool BTextVisualDef::load()
{
   // Try to load.
   BXMLReader reader;
   BString fullname = mFilename;
   fullname += ".textvisual";
   bool ok = reader.load(cDirArt, fullname);
   BXMLNode root(reader.getRootNode());
   
   // See if it worked.
   if(!ok || !root)
   {
      blogtrace("Failed to load text visual %s", mFilename.getPtr());
      return(false);
   }
   
   
   // Walk children.
   for(long i=0; i<root.getNumberChildren(); i++)
   {
      // Get child.
      BXMLNode child(root.getChild(i));
      
      if(child.getName().compare("Visual") == 0)
      {
         // Create new visual element.
         BTextVisualDefElement *element = new BTextVisualDefElement;
         if(!element)
         {
            BFAIL("Failed to allocate element");
            return(false);
         }
         
         // Load it.
         bool ok = element->load(child);
         if(ok)
         {
            // Add to list.
            mElements.add(element);
         }
         else
         {
            // Failed, so just clean up.
            delete element;
         }
      }
   }
   
   
   // Success.
   return(true);
}


//==============================================================================
// BTextVisualDef::create
//==============================================================================
void BTextVisualDef::create(long playerID, const BUString &text, const BVector &anchorPos, BDynamicSimLongArray *resultIDs)
{
   // If we have a result list, clear it.
   if(resultIDs)
      resultIDs->setNumber(0);
   
   // Create each element.
   for(long i=0; i<mElements.getNumber(); i++)
   {
      // Create new visual.
      long newID = gTextVisualManager.createVisual();
      
      // Look it up.
      BTextVisual *vis = gTextVisualManager.getVisual(newID);
      if(!vis)
         continue;
         
      // Basics.
      vis->setText(text);
      vis->setWorldAnchor(anchorPos);
      vis->setPlayerID(playerID);
         
      // Let element do its thing.
      mElements[i]->initVisual(vis);
      
      // Add to results list if we have one.
      if(resultIDs)
         resultIDs->add(newID);
   }
}

//==============================================================================
// eof: textvisualdef.cpp
//==============================================================================
