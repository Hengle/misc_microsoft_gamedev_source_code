//============================================================================
//
//  BGameStrings.cpp
//
//  Copyright (c) 2003, Ensemble Studios
//
//============================================================================


//============================================================================
//  INCLUDES
//============================================================================
#include "xsystem.h"
#include "GameStrings.h"
#include "xmlreader.h"

#include "config.h"
#include "econfigenum.h"
#include "workdirsetup.h"

//============================================================================
//  PRIVATE CONSTANTS
//============================================================================


//============================================================================
//  PRIVATE GLOBALS
//============================================================================


//============================================================================
//  CONSTRUCTION/DESTRUCTION
//============================================================================
BGameStrings::BGameStrings()
{
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BGameStrings::~BGameStrings()
{
   // clean up the game strings, we don't want the system squawking at us.
   // jce [10/7/2008] -- not sure why this is here at all since the destructor of mStrings should nuke it, 
   // but it wasn't working because setNumber(0, true) doesn't do anything so I'm taking it out.
   // jce [10/7/2008] -- also, this class seems to be unused :)
   //mStrings.setNumber(0, true);        // remove all the entries and deconstruct them.
   
   mIndexTable.clear();
}


//============================================================================
//  METHODS
//============================================================================


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const BSimString& BGameStrings::getString  (long stringID)
{
   if (!mIndexTable.validIndex(stringID))
      return sEmptySimString;

   long strIndex = mIndexTable[stringID];

   if (strIndex < 0)
      return sEmptySimString;
   
   // get the string
   BSimString & string = mStrings[strIndex];

   return string;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BGameStrings::hasString(long stringID)
{
   if (!mIndexTable.validIndex(stringID))
      return false;

   long strIndex = mIndexTable[stringID];
   if (strIndex < 0)
      return false;

   return true;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BGameStrings::loadStringFile(long dirID, const BSimString &filename)
{
   mFilename = filename;

   // Read in the xml file.
   BXMLReader xmlReader;

   bool ok = xmlReader.load(dirID, filename.getPtr());
   if(!ok)
   {
      {setBlogError(4040); blogerror("%s(1): Failed to load!", filename.getPtr());}
      return(false);
   }
   
   // Grab the root root of the tree.
   BXMLNode root(xmlReader.getRootNode());
   if(!root)
   {
      {setBlogError(4041); blogerror("%s(1): Failed to load!", filename.getPtr());}
      return(false);
   }

   // We want this to be "StringTable"...
   if(root.getName().compare(B("StringTable"))!=0)
   {
      {setBlogError(4042); blogerror("%s(1): expected <StringTable>", filename.getPtr());}
      return(false);
   }

   if (root.getNumberChildren() < 1)
   {
      {setBlogError(4043); blogerror("%s(1): expected <Language> tag", filename.getPtr());}
      BASSERT(0);
      return(false);
   }

   // Grab the first child which should be Language
   long stringNode = 0;
   BXMLNode node(root.getChild(stringNode));

   // Grab the language we are using
   BSimString language;
   if (!node.getAttribValue("name", &language))
   {
      {setBlogError(4044); blogerror("%s(1): 'name' attribute not found in <Language> tag", filename.getPtr());}
      BASSERT(0);
      return(false);
   }

   mLanguage.set(language);

   // Initialize our index array
   mIndexTable.clear();
   //mStrings.setNumber(0, true);        // remove all the entries and deconstruct them.
   // jce [10/7/2008] -- setNumber's force parameter doesn't work (it did in age3).  Replacing with clear()
   mStrings.clear();


   long numChildren = node.getNumberChildren();
   for(long i=0; i<numChildren; i++)
   {
      // Get pointer to child node.
      BXMLNode strNode(node.getChild(i));
      
      // Grab name.
      const BPackedString name(strNode.getName());
      
      // Does it match something we care about?
      if(name.compare(B("String")) == 0)
      {
		  // get the ID
		  long id = 0;
        ok = strNode.getAttribValueAsLong("_locID", id);
        if (!ok)
           ok = strNode.getAttribValueAsLong("id", id);
		  if (ok)
		  {

            // fixme - we will want to do some id validation using this mode.
            BSimString str;
            strNode.getText(str);

            // add to the hashtable
            long index = mStrings.add(str);

            long currentSize = mIndexTable.getNumber();
            if (currentSize <= id)
            {
               long newSize = id+100;
               mIndexTable.setNumber(newSize);      // give us some growing room

               // initialize the array
               for (long j=currentSize; j<newSize; j++)
               {
                  mIndexTable[j] = -1;
               }
            }

            // add to the table
            mIndexTable.setAt(id, index);
		  }
      }
	  else
         blog("%s(1): unknown tag '%s'", filename.getPtr(), name.getPtr());
   }

   return(true);

}

