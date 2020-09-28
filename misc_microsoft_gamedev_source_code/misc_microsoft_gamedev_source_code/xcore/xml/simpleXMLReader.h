//============================================================================
//
//  File: simpleXMLReader.h
//
//  Copyright (c) 2005, Ensemble Studios
//
//============================================================================
#pragma once
#include "AtgXmlParser.h"

//============================================================================
// class BSimpleXMLReader
// Note: Doesn't support CDATA.
//============================================================================
class BSimpleXMLReader : public ATG::ISAXCallback
{
public:
   typedef int BNodeIndex;
   typedef BDynamicArray<BNodeIndex> BNodeIndexArray;
   
   struct BAttribute
   {
      BString mName;
      BString mText;   
   };
   typedef BDynamicArray<BAttribute> BAttributeArray;
         
   class BNode
   {
      friend class BSimpleXMLReader;
      
   public:
      BNode() : mpReader(NULL), mParentNode(cInvalidIndex) { }
            
      void clear(void)
      {
         mpReader = NULL;
         mParentNode = cInvalidIndex;
         mName.empty();
         mText.empty();
         mAttributes.clear();
         mChildren.clear();
      }
      
      BSimpleXMLReader* getReader(void) const { return mpReader; }
      
      const BNode* getParentNode(void) const;
      
      const BString& getName(void) const { return mName; }
      const BString& getText(void) const { return mText; }
      bool getTextAsBool(bool& value) const;
      bool getTextAsInt(int& value) const;
      bool getTextAsFloat(float& value) const;
                              
      uint getNumChildren(void) const { return mChildren.getSize(); }
      const BNode* getChild(uint index) const;
                        
      uint getNumAttributes(void) const { return mAttributes.getSize(); }
      const BAttribute& getAttribute(uint index) const { return mAttributes[index]; }
      
      const BNode* findChild(const char* pName) const;
      const BAttribute* findAttribute(const char* pName) const;
                  
      const BAttributeArray& getAttributeArray(void) const { return mAttributes; }
      const BNodeIndexArray& getChildrenArray(void) const { return mChildren; }
            
      bool getAttributeAsBool(const char* pName, bool& value) const;
      bool getAttributeAsInt(const char* pName, int& value) const;
      bool getAttributeAsFloat(const char* pName, float& value) const;
      bool getAttributeAsString(const char* pName, BString& value) const;
      
      bool getChildAsBool(const char* pName, bool& value) const;
      bool getChildAsInt(const char* pName, int& value) const;
      bool getChildAsFloat(const char* pName, float& value) const;
      bool getChildAsString(const char* pName, BString& value) const;
                  
   protected:
      BSimpleXMLReader* mpReader;
      BNodeIndex mParentNode;
      
      BString mName;
      BString mText;
      
      BAttributeArray mAttributes;
      BNodeIndexArray mChildren;
   };

   BSimpleXMLReader();
   ~BSimpleXMLReader();

   HRESULT parse(const char* pFilename);
   HRESULT parse(const char* pBuf, uint bufSize);
   
   void clear(void);
         
   const BString& getErrorMessage(void) const { return mErrorMessage; }
   
   // getRoot() cannot return NULL if parse() returned S_OK.
   const BNode* getRoot(void) const { return mNodePool.empty() ? NULL : mNodePool.getPtr(); }
   
   const BDynamicArray<BNode>& getNodePool(void) const   { return mNodePool; }
         BDynamicArray<BNode>& getNodePool(void)         { return mNodePool; }
   
   const BNode* getNode(BNodeIndex index) const { return (index == cInvalidIndex) ? NULL : &mNodePool[index]; }
         BNode* getNode(BNodeIndex index)       { return (index == cInvalidIndex) ? NULL : &mNodePool[index]; }
            
private:
   ATG::XMLParser mParser;
   
   BDynamicArray<BNode> mNodePool;
         
   BNodeIndexArray mNodeStack;
   
   BString mErrorMessage;
             
   virtual HRESULT StartDocument();
   virtual HRESULT EndDocument();

   virtual HRESULT ElementBegin( CONST WCHAR* strName, UINT NameLen, CONST ATG::XMLAttribute *pAttributes, UINT NumAttributes );
   virtual HRESULT ElementContent( CONST WCHAR *strData, UINT DataLen, BOOL More );
   virtual HRESULT ElementEnd( CONST WCHAR *strName, UINT NameLen );

   virtual HRESULT CDATABegin( );
   virtual HRESULT CDATAData( CONST WCHAR *strCDATA, UINT CDATALen, BOOL bMore );
   virtual HRESULT CDATAEnd( );

   virtual VOID Error( HRESULT hError, CONST CHAR *strMessage );
   
   static void createString(BString& str, CONST WCHAR* pBuf, UINT bufLen);
};
