//============================================================================
//
//  FontCharHashTable.cpp
//
//  Copyright (c) 2003 Ensemble Studios
//
//============================================================================


#include "xgameRender.h"
#include "FontSystem2.h"
#include "FontCharHashTable.h"


BFontCharHashTable::BFontCharHashTable(long tableSize)
{
   // Allocate and initialize the table of pointers
   mTableSize = tableSize;
   mpHashTable = new Node*[mTableSize];
   ZeroMemory(mpHashTable, mTableSize * sizeof(Node*));
}


BFontCharHashTable::~BFontCharHashTable()
{
   reset();
   delete mpHashTable;
}


void BFontCharHashTable::reset()
{
   long nIndx;
   Node *pNode;
   Node *pTemp;

   for (nIndx = 0; nIndx < mTableSize; nIndx++)
   {
      pNode = mpHashTable[nIndx];
      mpHashTable[nIndx] = NULL;

      while (pNode != NULL)
      {
         pTemp = pNode->mpNextNode;
         delete pNode;
         pNode = pTemp;
      }
   }
}


BFontCharacter& BFontCharHashTable::getCharacter(WCHAR character)
{
   // Get the hash value.  Don't do a calculation unless necessary.
   // This is a big speed improvement.
   long hashValue;
   if (character < mTableSize)
      hashValue = character;
   else
   {
      hashValue = (character % mTableSize);
   }

   Node *pNode = mpHashTable[hashValue];

   // Search for the node in the linked list at the given position in the table
   while (pNode != NULL)
   {
      if (pNode->mFontCharacter.mCharacter == character)
         return (pNode->mFontCharacter);
      else
         pNode = pNode->mpNextNode;
   }

   // Node not found so create a new node for the character
   pNode = new Node;
   ZeroMemory(pNode, sizeof(Node));
   pNode->mFontCharacter.mCharacter = character;
   pNode->mpNextNode = mpHashTable[hashValue];
   mpHashTable[hashValue] = pNode;

   return (pNode->mFontCharacter);
}
