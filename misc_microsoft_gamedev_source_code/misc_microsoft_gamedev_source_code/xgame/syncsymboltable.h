//==============================================================================
// SyncSymbolTable.h
//
// Copyright (c) 1999-2007, Ensemble Studios
//==============================================================================
#pragma once

#include "SymbolTable.h"

class BSymbolEntry;

//==============================================================================
class BSyncSymbolTable : public BSymbolTable
{     
   protected:      
      bool                  saveExtra(BSymbolEntry * pEntry, FILE * pFile) {pEntry;pFile;return TRUE;};
      bool                  loadExtra(BSymbolEntry * pEntry, FILE * pFile) {pEntry;pFile;return TRUE;}; 
      bool                  saveExtra(BSymbolEntry * pEntry, BChunkWriter *writer) {pEntry;writer;return TRUE;};
      bool                  loadExtra(BSymbolEntry * pEntry, BChunkReader *reader) {pEntry;reader;return TRUE;}; 
      long                  checksumExtra(BSymbolEntry * pEntry) const {pEntry;return 0;};
      void                  dumpExtra(void(*output)(const char *), BSymbolEntry * pEntry) const {output;pEntry;};

   public:
      enum 
      {
         cTag = 0x40411901,
         cTableSize = 23
      };
};