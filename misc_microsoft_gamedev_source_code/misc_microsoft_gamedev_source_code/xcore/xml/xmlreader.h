#if 0
//==============================================================================
// xmlreader.h
//
// Copyright (c) 2002, Ensemble Studios
//==============================================================================

#pragma once 

// Includes
#include "string\stringtable.h"
#include "math\vector.h"
#include "xmlfile.h"

// Forward declarations.
class BXMLAttribute;
class BXMLDocument;
class BXMLNode;
class BXMLWriter;

typedef void (*OUTPUT_PROC)(const char *str);
typedef void (*MSXML_MISSING_CALLBACK)(void);

//==============================================================================
// BXMLReader
//==============================================================================
class BXMLReader
{
   public:
      BXMLReader(BIXMLFile* pFile);
      ~BXMLReader();

      // create a tree of XML nodes by reading a file.  Returns true for success, false for failure
      // this is the main thing you call to actually do the work of loading and creating the structure
      
      bool                    loadFileSAX(long dirID, const BCHAR_T* pRelativeFilename, BStringTable<BString> *conversions = NULL, bool bLoadBinary =true);

      // Binary load/save
      bool                    save(long dirID, const BCHAR_T* szFilePath);
      bool                    load(long dirID, const BString& filePath);
      
      void                    setLoadError() { mLoadError=true; }
      bool                    getLoadError() const { return mLoadError; }

      void                    reset();

      BXMLNode*               getRootNode() const { return mRootNode; }

      bool                    readNextChar(BCHAR_T& c);

      bool                    writeXML(long dirID, const BString& filename);
      
      const BString&          getFilename() const {return(mFilename);}
      const BString&          getFullFilename() const {return(mFullFilename);}
      
      void                    output(OUTPUT_PROC outputFn) const;

      bool                    updateBinary(long dirID, const BCHAR_T *pFileName, BStringTable<BString> *conversions = NULL);

      static void             setWorkingFolder(const BCHAR_T *workingFolder) { workingFolder; }
      static const BString&   getWorkingFolder() { return(sEmptyString); }
      //static void             setMSXMLMissingCallback(MSXML_MISSING_CALLBACK callback);

   protected:
      BString                 mFilename;
      BString                 mFullFilename;
      BXMLNode*               mRootNode;
      bool                    mLoadError;
      BIXMLFile*              mpFile;
      DWORD                   mFileSize;
      char*                   mDataBuffer;
      DWORD                   mBytesProcessed;
      DWORD                   mBufferSize;
      DWORD                   mBufferIndex;
      bool                    mUnicode;
      
};

//==============================================================================
// BXMLNode
//==============================================================================
class BXMLNode
{
   public:
      BXMLNode(BXMLReader* pReader, int lineNumber);
      BXMLNode();
      ~BXMLNode();

      bool                    load(BXMLReader* reader, BCHAR_T startChar);

      bool                    write(BXMLWriter& writer);

      void                    setReader(BXMLReader* reader) { mReader=reader; }

      const BString&          getName(void) const { return mName; }
#ifdef UNICODE
      void                    setName(const char* name) { mName.set(name); }
#endif      
      void                    setName(const BCHAR_T* name) { mName.set(name); }

      const BString&          getText(void) const { return mText; }
#ifdef UNICODE      
      void                    setText(const char* text) { mText.set(text); }
#endif      
      void                    setText(const BCHAR_T* text) { mText.set(text); }

      bool                    getTextAsString(BString& out) const;
      bool                    getTextAsFloat(float& out) const;
      bool                    getTextAsAngle(float& out) const;
      bool                    getTextAsBool(bool& out) const;
      bool                    getTextAsLong(long& out) const;
      bool                    getTextAsDWORD(DWORD& out) const;
      bool                    getTextAsHexDWORD(DWORD &out) const;
      bool                    getTextAsVector(BVector& out) const;

      const BSimpleArray<BXMLAttribute*>& getAttributes(void) const { return mAttributes; };
      long                    getAttributeCount( void ) const { return mAttributes.getNumber(); }
      const BXMLAttribute*    getAttribute(long index) const;
      bool                    getAttributeName(long index, const BString** pString = NULL) const;

#ifdef UNICODE
      const BXMLAttribute*    getAttribute(const char* name) const;
      bool                    getAttribute(const char* name, const BXMLAttribute **pAttrib) const;
      const BString*          getAttribValue(const char* name) const;
      bool                    getAttribValue(const char* name, const BString **pString) const;
      bool                    getAttribValueAsString(const char* name, BString& out) const;
      bool                    getAttribValueAsBool(const char* name, bool &out) const;
      bool                    getAttribValueAsLong(const char* name, long &out) const;
      bool                    getAttribValueAsFloat(const char* name, float &out) const;
      bool                    getAttribValueAsAngle(const char* name, float &out) const;
      bool                    getAttribValueAsDWORD(const char* name, DWORD &out) const;
      bool                    getAttribValueAsVector(const char* name, BVector& out) const;
#endif      

      const BXMLAttribute*    getAttribute(const BCHAR_T* name) const;
      bool                    getAttribute(const BCHAR_T* name, const BXMLAttribute **pAttrib) const;
      const BString*          getAttribValue(const BCHAR_T* name) const;
      bool                    getAttribValue(const BCHAR_T* name, const BString **pString) const;
      bool                    getAttribValueAsString(const BCHAR_T* name, BString& out) const;
      bool                    getAttribValueAsBool(const BCHAR_T* name, bool &out) const;
      bool                    getAttribValueAsLong(const BCHAR_T* name, long &out) const;
      bool                    getAttribValueAsFloat(const BCHAR_T* name, float &out) const;
      bool                    getAttribValueAsAngle(const BCHAR_T* name, float &out) const;
      bool                    getAttribValueAsDWORD(const BCHAR_T* name, DWORD &out) const;
      bool                    getAttribValueAsVector(const BCHAR_T* name, BVector& out) const;

      // parent stuff (this can return NULL)
      const BXMLNode*         getParent() const            { return mParentNode; }
      BXMLNode*               getParentRaw()               { return mParentNode; }
      void                    setParent(BXMLNode *pNode)   { mParentNode = pNode; }

      long                    getNumberChildren(void) const { return mChildren.getNumber(); }
      BXMLNode*               getChild(long index) const;
#ifdef UNICODE      
      BXMLNode*               getChildNode(const char* name) const;
#endif      
      BXMLNode*               getChildNode(const BCHAR_T* name) const;
#ifdef UNICODE      
      bool                    getChild(const char* name, const BXMLNode **pNode = NULL) const;
#endif      
      bool                    getChild(const BCHAR_T* name, const BXMLNode **pNode = NULL) const;

      bool                    getChildValue(const char *name, DWORD& value) const;
      bool                    getChildValue(const char *name, bool& value) const;
      bool                    getChildValue(const char *name, long& value) const;
      bool                    getChildValue(const char *name, float& value) const;
      bool                    getChildValue(const char *name, const BString **pString = NULL) const;

#ifdef UNICODE
      bool                    getChildValue(const BCHAR_T* name, DWORD& value) const { return getChildValue(BString(name).asANSI(), value); }
      bool                    getChildValue(const BCHAR_T* name, bool& value) const { return getChildValue(BString(name).asANSI(), value); };
      bool                    getChildValue(const BCHAR_T* name, long& value) const { return getChildValue(BString(name).asANSI(), value); };
      bool                    getChildValue(const BCHAR_T* name, float& value) const { return getChildValue(BString(name).asANSI(), value); };
      bool                    getChildValue(const BCHAR_T* name, const BString **pString = NULL) const { return getChildValue(BString(name).asANSI(), pString); };
#endif      

      bool                    setText(const BCHAR_T* szText, long textLen);
      bool                    setText( const BString& text ) { mText=text; return(true); }

      bool                    appendText(const BCHAR_T* szText, long textLen);

      BXMLAttribute*          addAttribute(long id, const BCHAR_T* szValue);

#ifdef UNICODE
      BXMLAttribute*          addAttribute(const char* szAttributeName, const char* szValue);
      BXMLAttribute*          addAttribute(const char* szAttributeName, long dwValue);
      BXMLAttribute*          addAttribute(const char* szAttributename, float fValue);
#endif      

      BXMLAttribute*          addAttribute(const BCHAR_T* szAttributeName, const BCHAR_T* szValue);
      BXMLAttribute*          addAttribute(const BCHAR_T* szAttributeName, long dwValue);
      BXMLAttribute*          addAttribute(const BCHAR_T* szAttributename, float fValue);

      bool                    addChild(BXMLNode* pNode);
      BXMLNode*               addChild(BXMLDocument* pDocument, const BCHAR_T* szChildName);
      
      bool                    removeChild(long index);
      bool                    removeChild(const BXMLNode *pChild);

      static BXMLNode*        getInstance();
      static void             releaseInstance(BXMLNode* node);

      void                    output(long spacing, OUTPUT_PROC outputFn) const;

      long                    getLineNumber(void) const { return mLineNumber; }      
      BXMLReader*             getReader(void) const { return mReader; }

      // RG [6/14/05 ] - FIXME
      void                    logInfo(const char* pMsg, ...) const { pMsg; }
      long                    getPrependBlankLineCount( void ) const { return(0); }
      void                    setPrependBlankLineCount( long v ) { v; }
      
   protected:
      BXMLReader*             mReader;
      BXMLNode*               mParentNode;
      BString                 mName;
      BString                 mText;
      BSimpleArray<BXMLNode *>      mChildren;
	   BSimpleArray<BXMLAttribute*>  mAttributes; 
	   int                     mLineNumber;
};

//==============================================================================
// BXMLAttribute
//==============================================================================
class BXMLAttribute
{
   public:
      BXMLAttribute();
      ~BXMLAttribute();

      const BString&          getName() const { return mName; }
      const BString&          getName(BXMLReader* pReader) const { pReader; return mName; }

#ifdef UNICODE
      void                    setName(const char* text) { mName.set(text); }
#endif      
      void                    setName(const BCHAR_T* text) { mName.set(text); }

      const BString&          getValue(void) const { return mValue;  }

#ifdef UNICODE
      void                    setValue(const char* val) { mValue.set(val); }
#endif      
      void                    setValue(const BCHAR_T* val) { mValue.set(val); }
     
      bool                    getValueAsLong(long& val) const;
      bool                    getValueAsFloat(float& val) const;
      bool                    getValueAsDWORD(DWORD& val) const;

   protected:
      BString                 mName;
      BString                 mValue;
};

//==============================================================================
// BXMLDocument
//==============================================================================
class BXMLDocument
{
   public:
      BXMLDocument();
      ~BXMLDocument();

      //Node manipulation.
      BXMLNode*               createRootNode(BIXMLFile* pFile, const BCHAR_T* szName);
      bool                    setRootNode(BXMLReader  *pReader);
      BXMLNode*               getRootNode(void) const;
      BXMLNode*               createNode(const BCHAR_T* szNodeName, BXMLNode *pParent /*=NULL*/);

      //Unicode.
      bool                    getUnicode( void ) const { return(mUnicode); }
      void                    setUnicode( bool v ) { mUnicode=v; }

      //Writing.
      bool                    writeToFile(BIXMLFile &file, bool unicode );

      // More Node manipulation
      bool                    addChild(BXMLNode *pParent, const BXMLNode *pChild);

      bool                    HtmlTranslateChar(const BString &src, BString &dest);
      
   protected:

      void                    walk(const BXMLNode *pNode, BIXMLFile &file, int level);
      bool                    validateText(const BCHAR_T* szText);
      bool                    makeSafeXMLString(const BString &szIn, BString &szOut);

      BXMLNode*               mpRootNode;
      bool                    mbOwnRootNode;
      BXMLReader*             mReader;
      bool                    mbOwnReader;
      bool                    mUnicode;
};

#endif