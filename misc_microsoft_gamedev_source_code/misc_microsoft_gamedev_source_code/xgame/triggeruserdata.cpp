//==============================================================================
// triggeruserdata.h
//
// 
//
// Copyright (c) 2007 Ensemble Studios
//==============================================================================

// Includes
#include "common.h"
#include "triggeruserdata.h"
#include "gamedirectories.h"
#include "triggermanager.h"

// xsystem
#include "xmlreader.h"
#include "xmlwriter.h"


//==============================================================================
// BTriggerUserClassManager::load
//==============================================================================
bool BTriggerUserClassManager::load()
{
   if(mTriggerUserClasses.size() > 0)
      return true; //already loaded

   BXMLReader reader;
   if (!reader.load(cDirData, "userclasses.xml", XML_READER_LOAD_DISCARD_ON_CLOSE))
      return false;
   BXMLNode rootNode(reader.getRootNode());

   int classCount = rootNode.getNumberChildren();
   mTriggerUserClasses.reserve(classCount);

   for (int i = 0; i < classCount; i++)
   {
      BXMLNode node(rootNode.getChild(i));
      BTriggerUserClass *pTriggerUserClass = new BTriggerUserClass();
      if (!pTriggerUserClass)
         return false;

      if (!pTriggerUserClass->load(node))
      {
         delete pTriggerUserClass;
         return false;
      }
      mTriggerUserClasses.add(pTriggerUserClass);

		//if(pProtoConcept->mID > mLargestDBID)
		//	mLargestDBID = pProtoConcept->mID;
   }

   return true;

}

//==============================================================================
// BTriggerUserClassManager::getUserClassID
//==============================================================================
int BTriggerUserClassManager::getUserClassID(BSimString name)
{
   for (uint i = 0; i < mTriggerUserClasses.size(); i++)
   {
      if(mTriggerUserClasses[i]->mName.compare(name) == 0)
      {
         return i;
      }
   }
   return -1;
}

//==============================================================================
// BTriggerUserClass::load
//==============================================================================
bool BTriggerUserField::load(BXMLNode root)
{
   root.getAttribValueAsString("Name", mName);
   BSimString varNodeText;
   root.getAttribValueAsString("Type", varNodeText);
   mType = gTriggerManager.getTriggerVarTypeEnum(varNodeText);      

   return true;
}

//==============================================================================
// BTriggerUserClass::load
//==============================================================================
bool BTriggerUserClass::load(BXMLNode root)
{
//   initValues();

   root.getAttribValueAsInt("DBID", mDBID);

   root.getAttribValueAsString("Name", mName);
  
   // Iterate through children
   long numNodes = root.getNumberChildren();
   for (long i = 0; i < numNodes; i++)
   {
      BXMLNode node(root.getChild(i));            
      BTriggerUserField *pTriggerUserField = new BTriggerUserField();
      if (!pTriggerUserField)
         return false;
      if (!pTriggerUserField->load(node))
      {
         delete pTriggerUserField;
         return false;
      }
      mFields.add(pTriggerUserField);	
	}
	return true;
}


//==============================================================================
// BTriggerUserTable::load
//==============================================================================
bool BTriggerUserTable::load(BString filename)
//bool BScenario::preloadVisFiles(const char* pVisFileList)
{
   //need to use data sub folder

   BFile tableFile;
   if (!tableFile.open(cDirData, filename, BFILE_OPEN_ENABLE_BUFFERING | BFILE_OPEN_DISCARD_ON_CLOSE))
   {      
      return false;
   }
   mFileName = filename;
   mUserClassType = 0;

   BStream* pStream = tableFile.getStream();
      
   BString str; 
   //4 blank lines for now.
   pStream->readLine(str); //version
   pStream->readLine(str);
   pStream->readLine(str);
   pStream->readLine(str);

   uint currCount =0;
   for ( ; ; currCount++)
   {
      if (!pStream->readLine(str))
         break;           
      if (!str.isEmpty())
      {         
         //BSimString token;
         //long strLen = str.length();
         //long loc = token.copyTok(str, strLen, -1, B("\t"));
         //while (loc != -1)
         //{
         //   //protoSquadList.add(gDatabase.getProtoSquad(token));
         //   loc = token.copyTok(str, strLen, loc+1, B("\t"));
         //}
         //pVar->asProtoSquadList()->writeVar(protoSquadList);
         mData.push_back(str);
      }     
      //str.standardizePath();      
      //int i = str.findRight(".vis");
      //if (i > 0)
      //   str.crop(0, i - 1);      
      //i = str.findLeft("art\\");
      //if (i != -1)
      //   str.crop(i + 4, str.length() - 1);      
      if (str.isEmpty())
         continue;         
   }            
   tableFile.close();   
   return true;
}

//==============================================================================
// BTriggerUserTable::getRow
//==============================================================================
BSimString BTriggerUserTable::getRow(uint i)
{   
   return mData.get(i);
}

//==============================================================================
// BTriggerUserTableXML::load
//==============================================================================
bool BTriggerUserTableXML::load(BXMLNode root)  
{
   root.getAttribValueAsString("Name", mTableName);

   BSimString typeName;
   root.getAttribValueAsString("Type", typeName);
   mUserClassType = gTriggerManager.getUserClassManager()->getUserClassID(typeName);  
   mRoot = root;

	return true;
}

//==============================================================================
// BTriggerUserTableXML::getNumRows()
//==============================================================================
long BTriggerUserTableXML::getNumRows()
{
   return mRoot.getNumberChildren();
}

//==============================================================================
// BTriggerUserTableXML::getRow(uint i)
//==============================================================================
BXMLNode BTriggerUserTableXML::getRow(uint i)
{
   uint numNodes = mRoot.getNumberChildren();
   if(numNodes == 0 || i >= numNodes)
   {
      BXMLNode empty;
      return empty;
   }
   BXMLNode node(mRoot.getChild(i));  
   return node;
}



//==============================================================================
// BTriggerUserTablesFile::load(BSimString filename)
//==============================================================================
bool BTriggerUserTablesFile::load(BSimString filename)
{
   BFile tableFile;
   if (!reader.load(cDirAIData, filename, XML_READER_LOAD_DISCARD_ON_CLOSE))
      return false;
   BXMLNode rootNode(reader.getRootNode());
   mFileName = filename;
   
   // Iterate through children
   long numNodes = rootNode.getNumberChildren();
   for (long i = 0; i < numNodes; i++)
   {
      BXMLNode node(rootNode.getChild(i));            
      BTriggerUserTableXML* pTable = new BTriggerUserTableXML();
      pTable->load(node);
      mTables.push_back(pTable);
	}
   return true;
}

//==============================================================================
// BTriggerTableManager::reset()
//==============================================================================
void BTriggerTableManager::reset()
{
   //stuff is cached so it might be better not to reset this system

   //clear all table files?
   //clear tables by id
}

//==============================================================================
// findUserDataFiles
//==============================================================================
static bool CALLBACK findUserDataFiles(const BString& path, void* pParam)
{
   BDynamicSimArray<BSimString>* fileList = (BDynamicSimArray<BSimString>*)pParam;
   if(!fileList)
      return false;
   fileList->add(path);
   return true;
}

//==============================================================================
// BTriggerTableManager::preloadTables
//==============================================================================
void BTriggerTableManager::preloadTables()
{
   BDynamicSimArray<BSimString> fileList;
   gFileManager.findFiles(cDirAIData, "*.ai", BFFILE_WANT_FILES|BFFILE_TRY_XMB, findUserDataFiles, &fileList, false);
   
   BSimString name;
   for(uint i=0; i<fileList.size(); i++)
   {
      strPathGetFilename(fileList[i], name);
      getOrCreateTableFile(name);
   }

}

//==============================================================================
// BTriggerTableManager::getOrCreateTableFile(BSimString filename)
//==============================================================================
BTriggerUserTablesFile* BTriggerTableManager::getOrCreateTableFile(BSimString filename)
{
   BTriggerUserTablesFile* pTableFile = NULL;
   //get
   for(uint i=0; i<mTableFiles.size(); i++)
   {
      if(mTableFiles[i]->mFileName.compare(filename) == 0)
      {
         pTableFile = mTableFiles[i];
         break;
      }
   }
   //or create
   if(pTableFile == NULL)
   {
      //load table file
      pTableFile = new BTriggerUserTablesFile();
      if(pTableFile->load(filename) == false)
      {
         return NULL;
      }
      for(uint j=0; j<pTableFile->mTables.size(); j++)
      {
         mTablesByID.push_back(pTableFile->mTables[j]);
      }
      mTableFiles.push_back(pTableFile);     
   }
   return pTableFile;
}

//==============================================================================
// BTriggerTableManager::getTable(BSimString filename, BSimString tableName)
//==============================================================================
BTriggerUserTableXML* BTriggerTableManager::getTable(BSimString filename, BSimString tableName)
{
   BTriggerUserTablesFile* pTableFile = getOrCreateTableFile(filename);
   if(pTableFile != NULL)
   {
      for(uint j=0; j<pTableFile->mTables.size(); j++)
      {
         if(pTableFile->mTables[j]->mTableName.compare(tableName) == 0)
         {
            return pTableFile->mTables[j];
         }
      }
   }
   return NULL;
}

//==============================================================================
// BTriggerTableManager::getTable(uint tableID)
//==============================================================================
BTriggerUserTableXML* BTriggerTableManager::getTable(uint tableID)
{
   if(tableID < 0 || tableID > mTablesByID.size())
      return NULL;
   return mTablesByID[tableID];
}

//==============================================================================
// BTriggerTableManager::getTableID(BTriggerUserTableXML* pTable)
//==============================================================================
int BTriggerTableManager::getTableID(BTriggerUserTableXML* pTable)
{
   for(uint i=0; i<mTablesByID.size(); i++)
   {
      if(mTablesByID[i] == pTable)
         return i;
   }
   return -1;
}