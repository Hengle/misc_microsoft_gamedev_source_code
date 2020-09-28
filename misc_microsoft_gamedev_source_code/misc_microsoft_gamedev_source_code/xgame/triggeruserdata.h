//==============================================================================
// triggeruserdata.h
//
// Copyright (c) 2006 Ensemble Studios
//==============================================================================
#pragma once

#include "triggerscript.h"
#include "trigger.h"
#include "triggereffect.h"
#include "savegame.h"
#include "gamefilemacros.h"

// xcore
#include "containers\staticArray.h"

// xsystem
#include "xmlreader.h"



//==============================================================================
// class BTriggerUserTable
// Grid of tab separated data.  Not currently used
//==============================================================================
class BTriggerUserTable
{
public:
   bool load(BString filename);
   bool loadXML(BString filename);
   BSimString getRow(uint i);
   BSimString mFileName;
   int mUserClassType;
private:
   BDynamicSimArray<BSimString> mData;

};

//==============================================================================
// class BTriggerUserTableXML
// 'Grid' of XML Data
//==============================================================================
class BTriggerUserTableXML
{
public:
   bool load(BXMLNode root);

   BXMLNode getRow(uint i);
   long getNumRows();
   BSimString mTableName;
   int mUserClassType;
private:
   BXMLNode mRoot;
};

//==============================================================================
// class BTriggerUserTablesFile
// Collection of N tables that live in the same XML File
//==============================================================================
class BTriggerUserTablesFile
{
public:
   bool load(BSimString filename);
   BSimString mFileName;
   BDynamicSimArray<BTriggerUserTableXML*> mTables;

private:
   BXMLReader reader;
};

//==============================================================================
// class BTriggerTableManager
// Manages and owns all table files.
//==============================================================================
class BTriggerTableManager
{
public:     
   void preloadTables();
   BTriggerUserTablesFile* getOrCreateTableFile(BSimString filename);
   BTriggerUserTableXML* getTable(BSimString filename, BSimString tableName);
   BDynamicSimArray<BTriggerUserTablesFile*> mTableFiles;

   BTriggerUserTableXML* getTable(uint tableID);
   int getTableID(BTriggerUserTableXML* pTable);

   BDynamicSimArray<BTriggerUserTableXML*> mTablesByID;

   void reset();
};

//==============================================================================
// class BTriggerUserField
// field of a user defined class
//==============================================================================
class BTriggerUserField
{
public:
	BSimString mName;
   BTriggerVarType mType;
	bool load(BXMLNode xmlNode);
};


//==============================================================================
// class BTriggerUserClass
// user defined class
//==============================================================================
class BTriggerUserClass
{
public:
   int mDBID;
	BSimString mName;
   BDynamicSimArray<BTriggerUserField*> mFields;
	bool load(BXMLNode xmlNode);
};



//==============================================================================
// class BTriggerUserClass
// owns all user classes
//==============================================================================
class BTriggerUserClassManager
{
public:
   bool load();
   BDynamicSimArray<BTriggerUserClass*> mTriggerUserClasses;
   int getUserClassID(BSimString name);
};