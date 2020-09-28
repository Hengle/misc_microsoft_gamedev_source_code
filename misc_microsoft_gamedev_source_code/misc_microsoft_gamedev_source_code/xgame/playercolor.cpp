//==============================================================================
// playercolor.cpp
//
// Copyright (c) 2003-2006 Ensemble Studios
//==============================================================================

// Includes
#include "common.h"
#include "playercolor.h"
#include "xmlreader.h"
#include "string\converttoken.h"

//=============================================================================
// BPlayerColor::load
//=============================================================================
bool BPlayerColor::load(BXMLNode node)
{
   BSimString val;
   if (node.getAttribValue("objects", &val))
      convertTokenToDWORDColor(val, mObjects, 255);
   else
      mObjects = cDWORDWhite;

   if (node.getAttribValue("corpse", &val))
      convertTokenToDWORDColor(val, mCorpse, 255);
   else
      mCorpse = cDWORDWhite;
   
   if (node.getAttribValue("selection", &val))
      convertTokenToDWORDColor(val, mSelection, 255);
   else
      mSelection = cDWORDWhite;
            
   if (node.getAttribValue("minimap", &val))
      convertTokenToDWORDColor(val, mMinimap, 255);
   else
      mMinimap = cDWORDWhite;
      
   if (node.getAttribValue("ui", &val))
      convertTokenToDWORDColor(val, mUI, 255);
   else
      mUI = cDWORDWhite;
      
   return true;
}
