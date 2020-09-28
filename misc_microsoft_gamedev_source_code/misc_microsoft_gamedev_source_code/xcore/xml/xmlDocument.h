//============================================================================
//
//  File: xmlDocument.h
//
//  Copyright (c) 2005-2007, Ensemble Studios
//
//  This is a simple wrapper around the ATG XML parser that offers a simple
//  read-only document view.
//
//  This class is really only intended to be used in tools where speed/memory
//  utilization isn't very important.
//
//  Important: Use the BXMLReader class in xmlReader.h for reading XML files
//  in the game! BXMLReader supports both XML and XMB files, BXMLDocument doesn't!
//
//============================================================================
#pragma once
#include "AtgXmlParser.h"
#include "math\vector.h"

//============================================================================
// class BXMLDocument
//============================================================================
class BXMLDocument : public ATG::ISAXCallback
{
public:
   typedef int BNodeIndex;
   typedef BDynamicArray<BNodeIndex> BNodeIndexArray;
   
   struct BAttribute
   {
      // Having both Unicode and ANSI reps in memory is wasteful, but it sure makes things simpler and potentially faster.
      BString  mName;
      BUString mUName;
      
      BString  mText;   
      BUString mUText;   
      
      inline const BString& getName(void) const { return mName; }
      inline const BString& getText(void) const { return mText; }

      inline const BUString& getUName(void) const { return mUName; }
      inline const BUString& getUText(void) const { return mUText; }
   };
   typedef BDynamicArray<BAttribute> BAttributeArray;
         
   class BNode
   {
      friend class BXMLDocument;
      
   public:
      BNode() : mpReader(NULL), mParentNode(cInvalidIndex), mTextIsCDATA(false) { }
            
      void clear(void)
      {
         mpReader = NULL;
         mParentNode = cInvalidIndex;
         mName.empty();
         mUName.empty();
         mText.empty();
         mUText.empty();
         mAttributes.clear();
         mChildren.clear();
         mTextIsCDATA = false;
      }
      
      inline BXMLDocument* getReader(void) const { return mpReader; }
      
      inline const BNode* getParentNode(void) const;
      inline BNodeIndex getParentNodeIndex(void) const { return mParentNode; }
            
      inline const BString& getName(void) const { return mName; }
      inline const BString& getText(void) const { return mText; }
      
      inline const BUString& getUName(void) const { return mUName; }
      inline const BUString& getUText(void) const { return mUText; }
      
      bool getTextAsBool(bool& value) const;
      bool getTextAsInt(int& value) const;
      bool getTextAsInt64(int64& value) const;
      bool getTextAsUInt64(uint64& value) const;
      bool getTextAsDWORD(DWORD& value) const;
      bool getTextAsFloat(float& value) const;
      bool getTextAsVector(BVector& value) const;
                              
      inline uint getNumChildren(void) const { return mChildren.getSize(); }
      const BNode* getChild(uint index) const;
      BNodeIndex getChildNodeIndex(uint index) const;
                        
      inline uint getNumAttributes(void) const { return mAttributes.getSize(); }
      inline const BAttribute& getAttribute(uint index) const { return mAttributes[index]; }
      
      const BNode* findChild(const char* pName) const;
      const BNode* findChild(const WCHAR* pName) const;
      
      const BAttribute* findAttribute(const char* pName) const;
      const BAttribute* findAttribute(const WCHAR* pName) const;
            
      inline const BAttributeArray& getAttributeArray(void) const { return mAttributes; }
      inline const BNodeIndexArray& getChildrenArray(void) const { return mChildren; }

      // Be sure to initialize the return value to something value!
      bool getAttributeAsBool(const char* pName, bool& value) const;
      bool getAttributeAsInt(const char* pName, int& value) const;
      bool getAttributeAsInt64(const char* pName, int64& value) const;
      bool getAttributeAsUInt64(const char* pName, uint64& value) const;
      bool getAttributeAsFloat(const char* pName, float& value) const;
      bool getAttributeAsString(const char* pName, BString& value) const;
      bool getAttributeAsString(const char* pName, BUString& value) const;
      
      // Be sure to initialize the return value to something value!      
      bool getChildAsBool(const char* pName, bool& value) const;
      bool getChildAsInt(const char* pName, int& value) const;
      bool getChildAsInt64(const char* pName, int64& value) const;
      bool getChildAsUInt64(const char* pName, uint64& value) const;
      bool getChildAsFloat(const char* pName, float& value) const;
      bool getChildAsString(const char* pName, BString& value) const;
      bool getChildAsString(const char* pName, BUString& value) const;
      
      bool getTextIsCData(void) const { return mTextIsCDATA; }
      void setTextIsCData(bool flag) { mTextIsCDATA = flag; }
                  
   protected:
      BXMLDocument* mpReader;
      BNodeIndex mParentNode;
      
      BString mName;
      BUString mUName;
      
      BString mText;
      BUString mUText;
      
      BAttributeArray mAttributes;
      BNodeIndexArray mChildren;
      
      bool mTextIsCDATA : 1;
   };

   BXMLDocument();
   ~BXMLDocument();

   // This method only supports ANSI and UTF-16.
   HRESULT parse(const char* pFilename);
   
   // This method supports ANSI, UTF-8, and UTF-16.
   // Internally it converts UTF-8 to UTF-16 before parsing.
   HRESULT parse(const char* pBuf, uint bufSize);
   
   void clear(void);
         
   inline const BString& getErrorMessage(void) const { return mErrorMessage; }
   
   inline bool getValid(void) const { return mNodePool.getSize() > 0; }        
   
   // getRoot() cannot return NULL if parse() returned S_OK.
   inline const BNode* getRoot(void) const { return mNodePool.empty() ? NULL : mNodePool.getPtr(); }
   
   typedef BDynamicArray<BNode> BNodeArray;
   
   inline const BNodeArray& getNodePool(void) const   { return mNodePool; }
   inline       BNodeArray& getNodePool(void)         { return mNodePool; }
   
   inline uint getNumNodes(void) const { return mNodePool.getSize(); }
   
   inline const BNode* getNode(BNodeIndex index) const { return (index == cInvalidIndex) ? NULL : &mNodePool[index]; }
   inline       BNode* getNode(BNodeIndex index)       { return (index == cInvalidIndex) ? NULL : &mNodePool[index]; }
         
   inline bool getHasUnicodeChars(void) const { return mHasUnicodeChars; }         
               
private:
   ATG::XMLParser mParser;
   
   BNodeArray mNodePool;
         
   BNodeIndexArray mNodeStack;
   
   BString mErrorMessage;
   
   bool mHasUnicodeChars;
             
   BXMLDocument(const BXMLDocument&);
   BXMLDocument& operator= (const BXMLDocument&);
   
   bool convertUTF8ToUTF16(BByteArray& srcFileData);
   
   virtual HRESULT StartDocument();
   virtual HRESULT EndDocument();

   virtual HRESULT ElementBegin( CONST WCHAR* strName, UINT NameLen, CONST ATG::XMLAttribute *pAttributes, UINT NumAttributes );
   virtual HRESULT ElementContent( CONST WCHAR *strData, UINT DataLen, BOOL More );
   virtual HRESULT ElementEnd( CONST WCHAR *strName, UINT NameLen );

   virtual HRESULT CDATABegin( );
   virtual HRESULT CDATAData( CONST WCHAR *strCDATA, UINT CDATALen, BOOL bMore );
   virtual HRESULT CDATAEnd( );

   virtual VOID Error( HRESULT hError, CONST CHAR *strMessage );
   
   static void createString(BUString& ustr, BString& str, CONST WCHAR* pBuf, UINT bufLen);
};

typedef BXMLDocument BSimpleXMLReader;

