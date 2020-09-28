//============================================================================
//
//  File: simpleXMLReader.cpp
//
//  Copyright (c) 2005, Ensemble Studios
//
//============================================================================
#include "xcore.h"
#include "simpleXMLReader.h"

const uint cDefaultNodePoolReserveSize = 256;

//============================================================================
// BSimpleXMLReader::BNode::getParentNode
//============================================================================
const BSimpleXMLReader::BNode* BSimpleXMLReader::BNode::getParentNode(void) const 
{ 
   return mpReader ? mpReader->getNode(mParentNode) : NULL; 
}

//============================================================================
// BSimpleXMLReader::BNode::getChild
//============================================================================
const BSimpleXMLReader::BNode* BSimpleXMLReader::BNode::getChild(uint index) const
{
   BDEBUG_ASSERT(index < mChildren.getSize());
   
   return mpReader->getNode(mChildren[index]);
}

//============================================================================
// BSimpleXMLReader::BNode::findChild
//============================================================================
const BSimpleXMLReader::BNode* BSimpleXMLReader::BNode::findChild(const char* pName) const
{
   for (uint i = 0; i < mChildren.getSize(); i++)
   {
      const BNode* pNode = getChild(i);
      if (pNode->mName == pName)
         return pNode;
   }         
   return NULL;         
}

//============================================================================
// BSimpleXMLReader::BNode::findAttribute
//============================================================================
const BSimpleXMLReader::BAttribute* BSimpleXMLReader::BNode::findAttribute(const char* pName) const
{
   for (uint i = 0; i < mAttributes.getSize(); i++)
      if (mAttributes[i].mName == pName)
         return &mAttributes[i];
   return NULL;
}

//============================================================================
// BSimpleXMLReader::BNode::getTextAsBool
//============================================================================
bool BSimpleXMLReader::BNode::getTextAsBool(bool& value) const
{
   if (!mText.length())
      return false;
   
   value = (mText.asLong() != 0);
   return true;
}

//============================================================================
// BSimpleXMLReader::BNode::getTextAsInt
//============================================================================
bool BSimpleXMLReader::BNode::getTextAsInt(int& value) const
{
   if (!mText.length())
      return false;
   
   value = mText.asLong();
   return true;
}

//============================================================================
// BSimpleXMLReader::BNode::getTextAsFloat
//============================================================================
bool BSimpleXMLReader::BNode::getTextAsFloat(float& value) const
{
   if (!mText.length())
      return false;

   value = mText.asFloat();
   return true;
}

//============================================================================
// BSimpleXMLReader::BNode::getAttributeAsBool
//============================================================================
bool BSimpleXMLReader::BNode::getAttributeAsBool(const char* pName, bool& value) const
{
   const BAttribute* pAttrib = findAttribute(pName);
   if (!pAttrib)
      return false;
   
   value = (pAttrib->mText.asLong() != 0);
   return true;
}

//============================================================================
// BSimpleXMLReader::BNode::getAttributeAsInt
//============================================================================
bool BSimpleXMLReader::BNode::getAttributeAsInt(const char* pName, int& value) const
{
   const BAttribute* pAttrib = findAttribute(pName);
   if (!pAttrib)
      return false;

   value = pAttrib->mText.asLong();
   return true;
}

//============================================================================
// BSimpleXMLReader::BNode::getAttributeAsFloat
//============================================================================
bool BSimpleXMLReader::BNode::getAttributeAsFloat(const char* pName, float& value) const
{
   const BAttribute* pAttrib = findAttribute(pName);
   if (!pAttrib)
      return false;

   value = pAttrib->mText.asFloat();
   return true;
}

//============================================================================
// BSimpleXMLReader::BNode::getAttributeAsString
//============================================================================
bool BSimpleXMLReader::BNode::getAttributeAsString(const char* pName, BString& value) const
{
   const BAttribute* pAttrib = findAttribute(pName);
   if (!pAttrib)
      return false;

   value = pAttrib->mText;
   return true;
}
 
//============================================================================
// BSimpleXMLReader::BNode::getChildAsBool
//============================================================================
bool BSimpleXMLReader::BNode::getChildAsBool(const char* pName, bool& value) const
{
   const BNode* pChild = findChild(pName);
   if (!pChild)
      return false;
   return pChild->getTextAsBool(value);
}

//============================================================================
// BSimpleXMLReader::BNode::getChildAsInt
//============================================================================
bool BSimpleXMLReader::BNode::getChildAsInt(const char* pName, int& value) const
{
   const BNode* pChild = findChild(pName);
   if (!pChild)
      return false;
   return pChild->getTextAsInt(value);
}

//============================================================================
// BSimpleXMLReader::BNode::getChildAsFloat
//============================================================================
bool BSimpleXMLReader::BNode::getChildAsFloat(const char* pName, float& value) const
{
   const BNode* pChild = findChild(pName);
   if (!pChild)
      return false;
   return pChild->getTextAsFloat(value);
}

//============================================================================
// BSimpleXMLReader::BNode::getChildAsString
//============================================================================
bool BSimpleXMLReader::BNode::getChildAsString(const char* pName, BString& value) const
{
   const BNode* pChild = findChild(pName);
   if (!pChild)
      return false;
   value = pChild->mText;
   return true;
}

//============================================================================
// BSimpleXMLReader::BSimpleXMLReader
//============================================================================
BSimpleXMLReader::BSimpleXMLReader()
{
   mParser.RegisterSAXCallbackInterface(this);
}

//============================================================================
// BSimpleXMLReader::~BSimpleXMLReader
//============================================================================
BSimpleXMLReader::~BSimpleXMLReader()
{

}

//============================================================================
// BSimpleXMLReader::parse
//============================================================================
HRESULT BSimpleXMLReader::parse(const char* pFilename)
{
   mNodePool.resize(0);
   mNodePool.reserve(cDefaultNodePoolReserveSize);
   
   mNodeStack.clear();
   mErrorMessage.empty();
   
   HRESULT hres = mParser.ParseXMLFile(pFilename);
   if (FAILED(hres))
      mNodePool.resize(0);
   else if (mNodePool.empty())
      return E_FAIL;      
      
   return hres;      
}

//============================================================================
// BSimpleXMLReader::parse
//============================================================================
HRESULT BSimpleXMLReader::parse(const char* pBuf, uint bufSize)
{
   mNodePool.resize(0);
   mNodePool.reserve(cDefaultNodePoolReserveSize);
   
   mNodeStack.clear();
   mErrorMessage.empty();
   
   HRESULT hres = mParser.ParseXMLBuffer(pBuf, bufSize);
   if (FAILED(hres))
      mNodePool.resize(0);
   else if (mNodePool.empty())
      return E_FAIL;
         
   return hres;
}

//============================================================================
// BSimpleXMLReader::clear
//============================================================================
void BSimpleXMLReader::clear(void)
{
   mNodePool.clear();
   mNodeStack.clear();
   mErrorMessage.empty();
}

//============================================================================
// BSimpleXMLReader::StartDocument
//============================================================================
HRESULT BSimpleXMLReader::StartDocument()
{
   mNodeStack.clear();
      
   return S_OK;
}

//============================================================================
// BSimpleXMLReader::EndDocument
//============================================================================
HRESULT BSimpleXMLReader::EndDocument()
{
   if (mNodeStack.getSize() != 0)
      return E_FAIL;
   
   return S_OK;
}

//============================================================================
// BSimpleXMLReader::createString
//============================================================================
void BSimpleXMLReader::createString(BString& str, CONST WCHAR* pBuf, UINT bufLen)
{
   if ((!pBuf) || (!bufLen))
   {
      str.empty();
      return;
   }
   
   const long numCharsNeeded = WideCharToMultiByte(CP_ACP, 0, pBuf, bufLen, NULL, 0, NULL, NULL);
   if (numCharsNeeded <= 0)
   {
      str.empty();
      return;
   }

   str.makeRawString(numCharsNeeded);
   
   char* pDstBuf = str.getString();

   const long numCharsWritten = WideCharToMultiByte(CP_ACP, 0, pBuf, bufLen, pDstBuf, numCharsNeeded, NULL, NULL);     
   BASSERT(numCharsWritten == numCharsNeeded);
   
   pDstBuf[numCharsWritten] = 0;
}

//============================================================================
// BSimpleXMLReader::ElementBegin
//============================================================================
HRESULT BSimpleXMLReader::ElementBegin( CONST WCHAR* strName, UINT NameLen, CONST ATG::XMLAttribute *pAttributes, UINT NumAttributes )
{
   const BNodeIndex nodeIndex = mNodePool.getSize();
   BNode& node = *mNodePool.enlarge(1);
   
   node.mpReader = this;
   
   createString(node.mName, strName, NameLen);
   
   node.mAttributes.resize(NumAttributes);
   for (uint i = 0; i < NumAttributes; i++)
   {
      BAttribute& attrib = node.mAttributes[i];
      createString(attrib.mName, pAttributes[i].strName, pAttributes[i].NameLen);
      createString(attrib.mText, pAttributes[i].strValue, pAttributes[i].ValueLen);
   }
         
   if (mNodeStack.getSize())
   {
      node.mParentNode = mNodeStack.back();   
      
      getNode(node.mParentNode)->mChildren.pushBack(nodeIndex);
                  
      mNodeStack.pushBack(nodeIndex);
   }
   else
   {
      node.mParentNode = cInvalidIndex;
            
      mNodeStack.pushBack(nodeIndex);
   }
   
   return S_OK;
}

//============================================================================
// BSimpleXMLReader::ElementContent
//============================================================================
HRESULT  BSimpleXMLReader::ElementContent( CONST WCHAR *strData, UINT DataLen, BOOL More )
{
   More;
   
   BDEBUG_ASSERT(mNodeStack.getSize());
   
   BNode* pNode = getNode(mNodeStack.back());
   
   BString data;
   createString(data, strData, DataLen);
   
   pNode->mText += data;

   return S_OK;
}

//============================================================================
// BSimpleXMLReader::ElementEnd
//============================================================================
HRESULT BSimpleXMLReader::ElementEnd( CONST WCHAR *strName, UINT NameLen )
{
   strName;
   NameLen;
   
   if (mNodeStack.empty())
      return E_FAIL;
      
   mNodeStack.popBack();
   return S_OK;
}

//============================================================================
// BSimpleXMLReader::CDATABegin
//============================================================================
HRESULT BSimpleXMLReader::CDATABegin( )
{
   return E_FAIL;
}

//============================================================================
// BSimpleXMLReader::CDATAData
//============================================================================
HRESULT BSimpleXMLReader::CDATAData( CONST WCHAR *strCDATA, UINT CDATALen, BOOL bMore )
{
   strCDATA;
   CDATALen;
   bMore;
   
   return E_FAIL;
}

//============================================================================
// BSimpleXMLReader::CDATAEnd
//============================================================================
HRESULT BSimpleXMLReader::CDATAEnd( )
{
   return E_FAIL;
}

//============================================================================
// BSimpleXMLReader::Error
//============================================================================
VOID BSimpleXMLReader::Error( HRESULT hError, CONST CHAR *strMessage )
{
   hError;
   strMessage;
   
   mErrorMessage.set(strMessage);
}

