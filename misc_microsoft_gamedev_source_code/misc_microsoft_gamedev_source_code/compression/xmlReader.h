//===================================================================================================================================================
//
//  File: xmlReader.h
//
//  Copyright (c) 2005-2007, Ensemble Studios
//
//  Notes:
//  Unlike the original Wrench/Phoenix XML reader class, BXMLNode and BXMLAttribute objects are intended to be passed by VALUE. 
//  BXMLAttribute and BXMLNode are only 64-bits.
//
//  Templated string "get" accessors accept any type of BStringTemplate (BString, BSimString, BUString, etc.) or BFixedString<n> string types.
//
//  If a get accessor method fails because the object is invalid, it'll assert in debug builds. The returned value will NOT be set in this case, so 
//  it's up to the caller to ensure that the return value has been set to a suitable default. (This emulates the old BXMLReader behavior.)
//
//  Element/attribute name fields are always stored as ANSI strings in binary XML files, so the name field can be quickly accessed.
//  Text fields may be packed or compressed, so many accessors require a temporary string buffer parameter in case they must unpack the string.
//  Only text fields may be UTF-16.
//
//===================================================================================================================================================
#pragma once
#include "xml\xmxData.h"
#include "xml\xmlDocument.h"
#include "utils\packedString.h"
#include "math\halfFloat.h"

class BXMLReader;
class BXMLNode;
class BXMLAttribute;

#define XML_READER_DEBUG_LOGGING 0

//============================================================================
// eXMLFileType
//============================================================================
enum eXMLFileType
{
   cCFTInvalid,
   cXFTXML,
   cXFTXMB
};

//============================================================================
// class BXMLAttribute
//============================================================================
class BXMLAttribute
{
public:
   enum { cMaxNodeIndex = 8388608 };
   enum { cMaxAttributeIndex = 511 };
   
   inline BXMLAttribute() : mpReader(NULL), mNodeIndex(0), mAttributeIndex(0) { }
   inline BXMLAttribute(const BXMLReader* pReader, uint nodeIndex, uint attributeIndex) { BDEBUG_ASSERT(nodeIndex < cMaxNodeIndex && attributeIndex < cMaxAttributeIndex); mpReader = pReader; mNodeIndex = nodeIndex; mAttributeIndex = attributeIndex; }
   
   inline void set(const BXMLReader* pReader, uint nodeIndex, uint attributeIndex) { BDEBUG_ASSERT(nodeIndex < cMaxNodeIndex && attributeIndex < cMaxAttributeIndex); mpReader = pReader; mNodeIndex = nodeIndex; mAttributeIndex = attributeIndex; }
   inline void setInvalid(void) { mpReader = NULL; mAttributeIndex = 0; mNodeIndex = 0; }
            
   inline bool                    getValid(void) const { return NULL != mpReader; }
   
   // bool operator can be used in if statements to determine if the node is valid.
   inline operator bool() const { return getValid(); }
   
   inline const BXMLReader*      getReader(void) const { return mpReader; }
   inline uint                   getNodeIndex(void) const { return mNodeIndex; }
   inline uint                   getAttributeIndex(void) const { return mAttributeIndex; }
   
   // A BPackedString is always internally a 32-bit pointer to a const char*, and can be passed by value.
   inline BPackedString          getName(void) const { return BPackedString(getNamePtr()); }

   // Name fields are always ANSI. Never returns NULL.
   const char*                   getNamePtr(void) const;
      
   // This method will set str to the name. StringType can be one of the BStringTemplates, or a BFixedString.
   // Internally, the name is always ANSI so it makes no sense to pass a Unicode string to this method.
   // Generally, getName() or getNamePtr() should be used instead of this method.
   template<typename StringType>
   StringType&                   getName(StringType& str) const;

   // Sets str to the attribute's value.               
   template<typename StringType>
   StringType&                   getValue(StringType& str) const;
   
   // getValuePtr() will either return a pointer directly to the string, or it will
   // convert the variant data from binary to string form into the supplied string buffer and 
   // return a pointer to it. Never returns NULL.
   template<typename StringType>
   const typename StringType::charType*   getValuePtr(StringType& str) const;
   
   // Returns true if the node's text is a zero length string.
   bool                          isValueEmpty(void) const;
   
   long                          compareValue(const char* pStr, bool caseSensitive = false) const;
   
   // Important: You should initialize the output parameter to something valid before calling these get methods!
   // These methods do not change the return parameter on failure.
   // Out of range input values are clamped.
   bool                          getValueAsInt8(int8& val) const;
   bool                          getValueAsUInt8(uint8& val) const;
   bool                          getValueAsInt16(int16& val) const;
   bool                          getValueAsUInt16(uint16& val) const;
   bool                          getValueAsInt32(int32& val) const;
   bool                          getValueAsUInt32(uint32& val) const;
   bool                          getValueAsInt64(int64& val) const;
   bool                          getValueAsUInt64(uint64& val) const;
   bool                          getValueAsInt(int& val) const { return getValueAsInt32(val); }
   bool                          getValueAsUInt(uint& val) const { return getValueAsUInt32(val); }
   bool                          getValueAsLong(long& val) const;
   bool                          getValueAsFloat(float& val) const;
   bool                          getValueAsHalfFloat(BHalfFloat& val) const;
   bool                          getValueAsAngle(float& val) const;
   bool                          getValueAsDWORD(DWORD& val) const;
   bool                          getValueAsBool(bool& val) const;
   
   // On failure, the return vector will be set to (0,0,0)
   bool                          getValueAsVector(BVector& val) const;

private:   
   const BXMLReader*             mpReader;
   
   uint                          mNodeIndex      : 23;
   uint                          mAttributeIndex : 9;
};

//============================================================================
// class BXMLNode
//============================================================================
class BXMLNode
{
public:
   inline BXMLNode() : mpReader(NULL), mNodeIndex(0) { }
   inline BXMLNode(const BXMLReader* pReader, uint nodeIndex) { mpReader = pReader; mNodeIndex = nodeIndex; }

   inline void                   set(const BXMLReader* pReader, uint nodeIndex) { mpReader = pReader; mNodeIndex = nodeIndex; }   
   inline void                   setInvalid(void) { mpReader = NULL; mNodeIndex = 0; }
   
   inline bool                   getValid(void) const { return NULL != mpReader; }
   
   // bool operator can be used in if statements to determine if the node is valid.
   inline operator bool() const { return getValid(); }   
   
   inline const BXMLReader*      getReader(void) const { return mpReader; }
   inline uint                   getNodeIndex(void) const { return mNodeIndex; }
   
   // A BPackedString is always internally a 32-bit pointer to a const char*, and can be passed by value.
   inline BPackedString          getName(void) const { return BPackedString(getNamePtr()); }
   
   // Name fields are always ANSI. Never returns NULL.
   const char*                   getNamePtr(void) const;
   
   // This method will set str to the name. StringType can be one of the BStringTemplates, or a BFixedString.
   // Internally, the name is always ANSI so it makes no sense to pass a Unicode string to this method.
   // Generally, getName() or getNamePtr() should be used instead of this method.
   template<typename StringType>
   StringType&                   getName(StringType& str) const;

   // Sets str to the node's text.
   template<typename StringType>
   StringType&                   getText(StringType& str) const;
   
   // Sets str to the node's text. 
   template<typename StringType>
   bool                          getTextAsString(StringType& str) const;
   
   // getTextPtr() will either return a pointer directly to the string, or it will
   // convert the variant data from binary to string form into the supplied string buffer and 
   // return a pointer to it. Never returns NULL.
   template<typename StringType>
   typename const StringType::charType* getTextPtr(StringType& str) const;
   
   // Returns true if the node's text is a zero length string.
   bool                          isTextEmpty(void) const;
   
   long                          compareText(const char* pStr, bool caseSensitive = false) const;
   
   // Important: You should initialize the output parameter to something valid before calling these get methods!
   // These methods do not change the return parameter on failure.
   // Out of range input values are clamped.
   bool                          getTextAsFloat(float& out) const;
   bool                          getTextAsHalfFloat(BHalfFloat& out) const;
   bool                          getTextAsAngle(float& out) const;
   bool                          getTextAsBool(bool& out) const;
   bool                          getTextAsInt(int& out) const;
   bool                          getTextAsUInt(uint& out) const { return getTextAsUInt32(out); }
   bool                          getTextAsLong(long& out) const;
   bool                          getTextAsDWORD(DWORD& out) const;
   bool                          getTextAsInt8(int8& out) const;
   bool                          getTextAsUInt8(uint8& out) const;
   bool                          getTextAsInt16(int16& out) const;
   bool                          getTextAsUInt16(uint16& out) const;
   bool                          getTextAsInt32(int32& out) const;
   bool                          getTextAsUInt32(uint32& out) const;
   bool                          getTextAsUInt64(uint64& out) const;
         
   // On failure, the return vector will be set to (0,0,0)
   bool                          getTextAsVector(BVector& out) const;

   long                          getAttributeCount(void) const;
   
   // Returns the indicated node's attribute. Asserts if index is out of range.
   BXMLAttribute                 getAttribute(long index) const;

   // Returns an invalid BXMLAttribute object if the attribute cannot be found. (Use getValid() to determine if the returned object is valid.)
   BXMLAttribute                 getAttribute(const char* pName) const;
   
   // Returns true if the attribute can be found.
   // If pAttrib is not NULL, it will be set to the found attribute, or set to an invalid object if the attribute was not found.
   bool                          getAttribute(const char* pName, BXMLAttribute* pAttrib) const;
   
   // Sets str to the attribute's value, if found. Otherwise str is empties.
   // Sets string to empty if the attribute cannot be found.
   template<typename StringType>
   StringType&                   getAttribValue(const char* pName, StringType& str) const;
   
   // Returns true if the attribute is found. Optionally sets pString to the attribute's value.
   // Sets string to empty if the attribute cannot be found.
   template<typename StringType>
   bool                          getAttribValue(const char* pName, StringType* pString = NULL) const;
   
   // Returns true and sets str to the attribute's value if the attribute can be found.
   // Sets string to empty if the attribute cannot be found.
   template<typename StringType>
   bool                          getAttribValueAsString(const char* pName, StringType& str) const;

   // Important: You should initialize the output parameter to something valid before calling these get methods!
   // These methods do not change the return parameter on failure.
   // Out of range input values are clamped.
   bool                          getAttribValueAsBool(const char* pName, bool& out) const;
   bool                          getAttribValueAsLong(const char* pName, long& out) const;
   bool                          getAttribValueAsFloat(const char* pName, float& out) const;
   bool                          getAttribValueAsHalfFloat(const char* pName, BHalfFloat& out) const;
   bool                          getAttribValueAsAngle(const char* pName, float& out) const;
   bool                          getAttribValueAsDWORD(const char* pName, DWORD& out) const;
   bool                          getAttribValueAsInt8(const char* pName, int8& out) const;
   bool                          getAttribValueAsUInt8(const char* pName, uint8& out) const;
   bool                          getAttribValueAsInt16(const char* pName, int16& out) const;
   bool                          getAttribValueAsUInt16(const char* pName, uint16& out) const;
   bool                          getAttribValueAsInt32(const char* pName, int32& out) const;
   bool                          getAttribValueAsUInt32(const char* pName, uint32& out) const;
   bool                          getAttribValueAsUInt64(const char* pName, uint64& out) const;
   bool                          getAttribValueAsInt(const char* pName, int& out) const { return getAttribValueAsInt32(pName, out); }
   bool                          getAttribValueAsUInt(const char* pName, uint& out) const { return getAttribValueAsUInt32(pName, out); }
   
   // On failure, the return vector will be set to (0,0,0)
   bool                          getAttribValueAsVector(const char* pName, BVector& out) const;

   // Returns parent node. 
   // Returns an invalid BXMLNode object for the root. (Use getValid() to determine if the returned object is valid.)
   BXMLNode                      getParent(void) const;
   
   // Returns the number of child nodes.
   long                          getNumberChildren(void) const;
   
   // Returns child node. Asserts if index is out of range.
   BXMLNode                      getChild(long index) const;

   // Returns an invalid BXMLNode object if the child cannot be found. (Use getValid() to determine if the returned object is valid.)
   BXMLNode                      getChildNode(const char* pName) const;

   // Returns true if child can be found.
   // *pNode will be set to an invalid object on failure. (Use getValid() to determine if the returned object is valid.)
   bool                          getChild(const char* pName, BXMLNode* pNode = NULL) const;
   
   // Important: You should initialize the output parameter to something valid before calling these get methods!
   // These methods do not change the return parameter on failure.
   // Out of range input values are clamped. 
   // getChildValue() is overloaded to work with many types.
   bool                          getChildValue(const char* pName, DWORD& value) const;
   bool                          getChildValue(const char* pName, bool& value) const;
   bool                          getChildValue(const char* pName, long& value) const;
   bool                          getChildValue(const char* pName, float& value) const;
   bool                          getChildValue(const char* pName, BHalfFloat& value) const;
   bool                          getChildValue(const char* pName, int8& value) const;
   bool                          getChildValue(const char* pName, uint8& value) const;
   bool                          getChildValue(const char* pName, int16& value) const;
   bool                          getChildValue(const char* pName, uint16& value) const;
   
   // The int32/uint32 versions will also with int/uint.
   bool                          getChildValue(const char* pName, int32& value) const;
   bool                          getChildValue(const char* pName, uint32& value) const;
   
   // Returns true if child can be found.
   // Optionally copies child's value to pString.
   // Sets string to empty if the child cannot be found.
   template<typename StringType>
   bool                          getChildValue(const char* pName, StringType* pString = NULL) const;

   // Unimplemented.
   long                          getLineNumber(void) const { return 0; }      
   void                          logInfo(const char* pMsg, ...) const { pMsg; }
      
private:
   const BXMLReader* mpReader;
   uint              mNodeIndex;   
};

//============================================================================
// enum eXMLReaderLoadFlags
//============================================================================
enum eXMLReaderLoadFlags
{
   XML_READER_IGNORE_BINARY            = 1,
   XML_READER_LOAD_DISCARD_ON_CLOSE    = 2
};

//============================================================================
// class BXMLReader
//============================================================================
class BXMLReader 
{
   friend class BXMLAttribute;
   friend class BXMLNode;
   
   BXMLReader(const BXMLReader&);
   BXMLReader& operator= (const BXMLReader&);
   
public:
   BXMLReader();
   ~BXMLReader();

   void                    reset(void);
      
   bool                    load(long dirID, const char* pFilename, uint loadFlags = 0, IStreamFactory* pStreamFactory = NULL);
      
   bool                    load(BStream* pStream, eXMLFileType fileType);
   
   inline bool             getValid(void) const { return mValid; }
         
   BXMLNode                getRootNode(void) const;
   
   inline long             getDirID(void) const { return mDirID; }
   inline const BString&   getFilename(void) const { return mFilename; }
   inline int              getLineNumber(void) const { return 0; }
   
   inline eXMLFileType     getFileType(void) const { return mFileType; }
   inline BXMLDocument*    getXMLDocument(void) const { return mpXMLDoc; }
   
   inline BPackedXMXData*  getXMXData(void) const { return mpXMXData; }
   
   static bool             getXMBEnabled(void) { return mXMBEnabled; }
   static void             setXMBEnabled(bool enabled) { mXMBEnabled = enabled; }
         
   static IStreamFactory*  getStreamFactory(void) { return mpStreamFactory; }
   static void             setStreamFactory(IStreamFactory* pStreamFactory) { mpStreamFactory = pStreamFactory; }
   
private:
   long                    mDirID;
   BString                 mFilename;
      
   BXMLDocument*           mpXMLDoc;   
   BPackedXMXData*         mpXMXData;
      
   eXMLFileType            mFileType;
   bool                    mValid : 1;
   
   BConstDataBuffer        mXMXVariantData;
   
   mutable BDynamicArray<char> mStringBuffer;
   
   const char*             getStringBufferPtr(void) const  { return (const char*)mStringBuffer.getPtr(); }
         char*             getStringBufferPtr(void)        { return (char*)mStringBuffer.getPtr(); }
         
   const WCHAR*            getStringBufferWPtr(void) const  { return (const WCHAR*)mStringBuffer.getPtr(); }
         WCHAR*            getStringBufferWPtr(void)        { return (WCHAR*)mStringBuffer.getPtr(); }
   
   inline const BXMLDocument::BNode&         getDocNode(uint nodeIndex) const { return *mpXMLDoc->getNode(nodeIndex); }
   inline const BXMLDocument::BAttribute&    getDocAttribute(uint nodeIndex, uint attributeIndex) const { return mpXMLDoc->getNode(nodeIndex)->getAttribute(attributeIndex); }
   
   inline const BPackedXMXData::BNode&       getXMXNode(uint nodeIndex) const { return mpXMXData->getNode(nodeIndex); }
   inline const BPackedXMXData::BAttribute&  getXMXAttribute(uint nodeIndex, uint attributeIndex) const { return mpXMXData->getNode(nodeIndex).mAttributes[attributeIndex]; }
         
   bool                    loadXML(const BYTE* pData, uint dataLen);
   bool                    loadXMB(const BYTE* pData, uint dataLen);
   bool                    loadFromStream(BStream* pStream, eXMLFileType fileType);

#if XML_READER_DEBUG_LOGGING
   void                    debugLog(const char* pMsg, ...) const;
#else   
   void                    debugLog(const char* pMsg, ...) const { pMsg; }
#endif   
      
   static IStreamFactory*  mpStreamFactory;
   static bool             mXMBEnabled;
};

#include "xmlReader.inl"
