//============================================================================
//
//  FontCharHashTable.h
//
//  Copyright (c) 2003 Ensemble Studios
//
//============================================================================


#ifndef FONTCHARHASHTABLE_H
#define FONTCHARHASHTABLE_H


// Default hash table size
#define DEFAULT_CHAR_HASH_TABLE_SIZE 127


// This class implements an open hash table of character data where each node in the
// table represents a single character.  The table consists of an array of pointers
// to linked lists.
class BFontCharHashTable
{
public:
   BFontCharHashTable(long tableSize = DEFAULT_CHAR_HASH_TABLE_SIZE);
   ~BFontCharHashTable();

   void reset();
   BFontCharacter& getCharacter(WCHAR character);

private:
   struct Node
   {
      BFontCharacter mFontCharacter;
      Node *mpNextNode;
   };

   long mTableSize;
   Node **mpHashTable;
};


#endif
