//============================================================================
//
//  File: xmlDocument.cpp
//
//  Copyright (c) 2005-2007, Ensemble Studios
//
//============================================================================
#include "xcore.h"
#include "xmlDocument.h"
#include "string\convertToken.h"
#include "string\strHelper.h"
#include "consoleOutput.h"

const uint cDefaultNodePoolReserveSize = 256;

//============================================================================
// BXMLDocument::BNode::getParentNode
//============================================================================
const BXMLDocument::BNode* BXMLDocument::BNode::getParentNode(void) const 
{ 
   return mpReader ? mpReader->getNode(mParentNode) : NULL; 
}

//============================================================================
// BXMLDocument::BNode::getChild
//============================================================================
const BXMLDocument::BNode* BXMLDocument::BNode::getChild(uint index) const
{
   BDEBUG_ASSERT(index < mChildren.getSize());
   
   return mpReader->getNode(mChildren[index]);
}

//============================================================================
// BXMLDocument::BNode::getChild
//============================================================================
BXMLDocument::BNodeIndex BXMLDocument::BNode::getChildNodeIndex(uint index) const
{
   BDEBUG_ASSERT(index < mChildren.getSize());

   return mChildren[index];
}

//============================================================================
// BXMLDocument::BNode::findChild
//============================================================================
const BXMLDocument::BNode* BXMLDocument::BNode::findChild(const char* pName) const
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
// BXMLDocument::BNode::findChild
//============================================================================
const BXMLDocument::BNode* BXMLDocument::BNode::findChild(const WCHAR* pName) const
{
   for (uint i = 0; i < mChildren.getSize(); i++)
   {
      const BNode* pNode = getChild(i);
      if (pNode->mUName == pName)
         return pNode;
   }         
   return NULL;         
}

//============================================================================
// BXMLDocument::BNode::findAttribute
//============================================================================
const BXMLDocument::BAttribute* BXMLDocument::BNode::findAttribute(const char* pName) const
{
   for (uint i = 0; i < mAttributes.getSize(); i++)
      if (mAttributes[i].mName == pName)
         return &mAttributes[i];
   return NULL;
}

//============================================================================
// BXMLDocument::BNode::findAttribute
//============================================================================
const BXMLDocument::BAttribute* BXMLDocument::BNode::findAttribute(const WCHAR* pName) const
{
   for (uint i = 0; i < mAttributes.getSize(); i++)
      if (mAttributes[i].mUName == pName)
         return &mAttributes[i];
   return NULL;
}

//============================================================================
// BXMLDocument::BNode::getTextAsBool
//============================================================================
bool BXMLDocument::BNode::getTextAsBool(bool& value) const
{
   if (!mText.length())
      return false;
   
   return convertTokenToBool(mText, value);
}

//============================================================================
// BXMLDocument::BNode::getTextAsInt
//============================================================================
bool BXMLDocument::BNode::getTextAsInt(int& value) const
{
   if (!mText.length())
      return false;
   
   value = mText.asLong();
   return true;
}

//============================================================================
// BXMLDocument::BNode::getTextAsInt64
//============================================================================
bool BXMLDocument::BNode::getTextAsInt64(int64& value) const
{
   if (!mText.length())
      return false;

   value = mText.asInt64();
   return true;
}

//============================================================================
// BXMLDocument::BNode::getTextAsUInt64
//============================================================================
bool BXMLDocument::BNode::getTextAsUInt64(uint64& value) const
{
   if (!mText.length())
      return false;

   value = mText.asUInt64();
   return true;
}

//============================================================================
// BXMLDocument::BNode::getTextAsDWORD
//============================================================================
bool BXMLDocument::BNode::getTextAsDWORD(DWORD& value) const
{
   if (!mText.length())
      return false;

   // This is not technically correct but I must emulate how the old BXMLReader class worked.
   value = (DWORD)atoi(mText.getPtr());
   
   return true;
}

//============================================================================
// BXMLDocument::BNode::getTextAsFloat
//============================================================================
bool BXMLDocument::BNode::getTextAsFloat(float& value) const
{
   if (!mText.length())
      return false;

   value = mText.asFloat();
   return true;
}

//============================================================================
// BXMLDocument::BNode::getTextAsVector
//============================================================================
bool BXMLDocument::BNode::getTextAsVector(BVector& value) const
{
   if (!mText.length())
   {
      value.set(0.0f, 0.0f, 0.0f);
      return false;
   }

   return mText.convertToVector3(&value.x);
}

//============================================================================
// BXMLDocument::BNode::getAttributeAsBool
//============================================================================
bool BXMLDocument::BNode::getAttributeAsBool(const char* pName, bool& value) const
{
   const BAttribute* pAttrib = findAttribute(pName);
   if ((!pAttrib) || (pAttrib->mText.isEmpty()))
      return false;
   
   return convertTokenToBool(pAttrib->mText, value);
}

//============================================================================
// BXMLDocument::BNode::getAttributeAsInt
//============================================================================
bool BXMLDocument::BNode::getAttributeAsInt(const char* pName, int& value) const
{
   const BAttribute* pAttrib = findAttribute(pName);
   if ((!pAttrib) || (pAttrib->mText.isEmpty()))
      return false;

   value = pAttrib->mText.asLong();
   return true;
}

//============================================================================
// BXMLDocument::BNode::getAttributeAsInt64
//============================================================================
bool BXMLDocument::BNode::getAttributeAsInt64(const char* pName, int64& value) const
{
   const BAttribute* pAttrib = findAttribute(pName);
   if ((!pAttrib) || (pAttrib->mText.isEmpty()))
      return false;

   value = pAttrib->mText.asInt64();
   return true;
}

//============================================================================
// BXMLDocument::BNode::getAttributeAsUInt64
//============================================================================
bool BXMLDocument::BNode::getAttributeAsUInt64(const char* pName, uint64& value) const
{
   const BAttribute* pAttrib = findAttribute(pName);
   if ((!pAttrib) || (pAttrib->mText.isEmpty()))
      return false;

   value = pAttrib->mText.asUInt64();
   return true;
}

//============================================================================
// BXMLDocument::BNode::getAttributeAsFloat
//============================================================================
bool BXMLDocument::BNode::getAttributeAsFloat(const char* pName, float& value) const
{
   const BAttribute* pAttrib = findAttribute(pName);
   if ((!pAttrib) || (pAttrib->mText.isEmpty()))
      return false;

   value = pAttrib->mText.asFloat();
   return true;
}

//============================================================================
// BXMLDocument::BNode::getAttributeAsString
//============================================================================
bool BXMLDocument::BNode::getAttributeAsString(const char* pName, BString& value) const
{
   const BAttribute* pAttrib = findAttribute(pName);
   if (!pAttrib)
   {
      value.empty();
      return false;
   }

   value = pAttrib->mText;
   return true;
}

//============================================================================
// BXMLDocument::BNode::getAttributeAsString
//============================================================================
bool BXMLDocument::BNode::getAttributeAsString(const char* pName, BUString& value) const
{
   const BAttribute* pAttrib = findAttribute(pName);
   if (!pAttrib)
   {
      value.empty();
      return false;
   }

   value = pAttrib->mUText;
   return true;
}
 
//============================================================================
// BXMLDocument::BNode::getChildAsBool
//============================================================================
bool BXMLDocument::BNode::getChildAsBool(const char* pName, bool& value) const
{
   const BNode* pChild = findChild(pName);
   if (!pChild)
      return false;
      
   return pChild->getTextAsBool(value);
}

//============================================================================
// BXMLDocument::BNode::getChildAsInt
//============================================================================
bool BXMLDocument::BNode::getChildAsInt(const char* pName, int& value) const
{
   const BNode* pChild = findChild(pName);
   if (!pChild)
      return false;
      
   return pChild->getTextAsInt(value);
}

//============================================================================
// BXMLDocument::BNode::getChildAsInt64
//============================================================================
bool BXMLDocument::BNode::getChildAsInt64(const char* pName, int64& value) const
{
   const BNode* pChild = findChild(pName);
   if (!pChild)
      return false;

   return pChild->getTextAsInt64(value);
}

//============================================================================
// BXMLDocument::BNode::getChildAsUInt64
//============================================================================
bool BXMLDocument::BNode::getChildAsUInt64(const char* pName, uint64& value) const
{
   const BNode* pChild = findChild(pName);
   if (!pChild)
      return false;

   return pChild->getTextAsUInt64(value);
}

//============================================================================
// BXMLDocument::BNode::getChildAsFloat
//============================================================================
bool BXMLDocument::BNode::getChildAsFloat(const char* pName, float& value) const
{
   const BNode* pChild = findChild(pName);
   if (!pChild)
      return false;
      
   return pChild->getTextAsFloat(value);
}

//============================================================================
// BXMLDocument::BNode::getChildAsString
//============================================================================
bool BXMLDocument::BNode::getChildAsString(const char* pName, BString& value) const
{
   const BNode* pChild = findChild(pName);
   if (!pChild)
   {
      value.empty();
      return false;
   }
   value = pChild->mText;
   return true;
}

//============================================================================
// BXMLDocument::BNode::getChildAsString
//============================================================================
bool BXMLDocument::BNode::getChildAsString(const char* pName, BUString& value) const
{
   const BNode* pChild = findChild(pName);
   if (!pChild)
   {
      value.empty();
      return false;
   }
   value = pChild->mUText;
   return true;
}

//============================================================================
// BXMLDocument::BXMLDocument
//============================================================================
BXMLDocument::BXMLDocument() :
   mHasUnicodeChars(false)
{
   mParser.RegisterSAXCallbackInterface(this);
}

//============================================================================
// BXMLDocument::~BXMLDocument
//============================================================================
BXMLDocument::~BXMLDocument()
{

}

//============================================================================
// BXMLDocument::parse
//============================================================================
HRESULT BXMLDocument::parse(const char* pFilename)
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
      
   mHasUnicodeChars = mParser.IsUnicode() != 0;
      
   return hres;      
}

bool BXMLDocument::convertUTF8ToUTF16(BByteArray& srcFileData)
{
   BDEBUG_ASSERT(srcFileData.getSize() >= 3);
      
   int bufSize = MultiByteToWideChar(
      CP_UTF8, 
      0, 
      (LPCSTR)srcFileData.getPtr() + 3, 
      srcFileData.getSize() - 3,
      NULL,
      0);   

   if (bufSize <= 0)
      return false;

   BByteArray tempBuf((bufSize + 1) * sizeof(WCHAR));

   int numWritten = MultiByteToWideChar(
      CP_UTF8, 
      0, 
      (LPCSTR)srcFileData.getPtr() + 3,  
      srcFileData.getSize() - 3,
      (LPWSTR)tempBuf.getPtr(),
      tempBuf.getSizeInBytes() / sizeof(WCHAR));
   if (numWritten != bufSize)         
      return false;

   srcFileData.resize(0);

   if (cBigEndianNative)
   {
      srcFileData.pushBack(0xFE);
      srcFileData.pushBack(0xFF);
   }
   else
   {
      srcFileData.pushBack(0xFF);
      srcFileData.pushBack(0xFE);
   }

   srcFileData.pushBack(tempBuf.getPtr(), numWritten * sizeof(WCHAR));
   
   return true;
}

//============================================================================
// BXMLDocument::parse
//============================================================================
HRESULT BXMLDocument::parse(const char* pBuf, uint bufSize)
{
   if (!bufSize)
      return E_FAIL;
      
   mNodePool.resize(0);
   mNodePool.reserve(cDefaultNodePoolReserveSize);
   
   mNodeStack.clear();
   mErrorMessage.empty();
   
   BByteArray utf16Bytes;
   if (bufSize >= 3)
   {
      const uchar* pBytes = (const uchar*)pBuf;
      if ((pBytes[0] == 0xEF) && (pBytes[1] == 0xBB) && (pBytes[2] == 0xBF))
      {
         utf16Bytes.pushBack(pBytes, bufSize);
         
         if (!convertUTF8ToUTF16(utf16Bytes))
         {
            return E_FAIL;
         }
      }
   }
   
   HRESULT hres;
   if (utf16Bytes.getSize())
      hres = mParser.ParseXMLBuffer((const char*)utf16Bytes.getPtr(), utf16Bytes.getSize());
   else
      hres = mParser.ParseXMLBuffer(pBuf, bufSize);
      
   if (FAILED(hres))
      mNodePool.resize(0);
   else if (mNodePool.empty())
      return E_FAIL;

   mHasUnicodeChars = mParser.IsUnicode() != 0;
   
   return hres;
}

//============================================================================
// BXMLDocument::clear
//============================================================================
void BXMLDocument::clear(void)
{
   mNodePool.clear();
   mNodeStack.clear();
   mErrorMessage.empty();
}

//============================================================================
// BXMLDocument::StartDocument
//============================================================================
HRESULT BXMLDocument::StartDocument()
{
   mNodeStack.clear();
      
   return S_OK;
}

//============================================================================
// BXMLDocument::EndDocument
//============================================================================
HRESULT BXMLDocument::EndDocument()
{
   if (mNodeStack.getSize() != 0)
      return E_FAIL;
   
   return S_OK;
}

//============================================================================
// BXMLDocument::createString
//============================================================================
void BXMLDocument::createString(BUString& ustr, BString& str, CONST WCHAR* pBuf, UINT bufLen)
{
   if ((!pBuf) || (!bufLen))
   {
      ustr.empty();
      str.empty();
      return;
   }
   
   ustr.makeRawString(bufLen);
   WCHAR* pUDstBuf = ustr.getString();
   memcpy(pUDstBuf, pBuf, bufLen * sizeof(WCHAR));
   pUDstBuf[bufLen] = L'\0';
   
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
// BXMLDocument::ElementBegin
//============================================================================
HRESULT BXMLDocument::ElementBegin( CONST WCHAR* strName, UINT NameLen, CONST ATG::XMLAttribute *pAttributes, UINT NumAttributes )
{
   const BNodeIndex nodeIndex = mNodePool.getSize();
   BNode& node = *mNodePool.enlarge(1);
   
   node.mpReader = this;
   
   createString(node.mUName, node.mName, strName, NameLen);
   
   node.mAttributes.resize(NumAttributes);
   for (uint i = 0; i < NumAttributes; i++)
   {
      BAttribute& attrib = node.mAttributes[i];
      createString(attrib.mUName, attrib.mName, pAttributes[i].strName, pAttributes[i].NameLen);
      createString(attrib.mUText, attrib.mText, pAttributes[i].strValue, pAttributes[i].ValueLen);
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

namespace
{
   template<class T>
   void cleanText(T& str)
   {
      // First, convert CRLF (0D0A) to just (LF) 0A, per the XML specification.
      // Then remove leading/trailing whitespace - if the user really wants whitespace preserved they should use CDATA.
      
      int ofs = 0;
      int len = str.length();
      for ( ; ; )
      {
         if (ofs >= len)
            break;

         int c = str.getChar(ofs);
         int nc = (ofs < (len - 1)) ? str.getChar(ofs + 1) : -1;

         if ((c == 13) && (nc == 10))
         {
            T start(str);

            if (ofs > 0)
               start.crop(0, ofs - 1);
            else
               start.empty();

            T end(str);
            if ((ofs + 2) < len)
               end.crop(ofs + 2, len - 1);
            else
               end.empty();

            str = start + "\n" + end;
            len--;
         }
         else if (c == 13)
         {
            str.setChar(ofs, 10);
         }

         ofs++;
      }

      str.trimLeft(BStringDefaults<T::charType>::getWhiteSpaceString());
      str.trimRight(BStringDefaults<T::charType>::getWhiteSpaceString());

#if 0      
      if ((str.findLeft(10) != -1) || (str.findLeft(13) != -1))
      {
         gConsoleOutput.warning("Encountered XML text with CR/LF: \"%s\"\n", str.getPtr());
      }
#endif      
   }
};

//============================================================================
// BXMLDocument::ElementContent
//============================================================================
HRESULT BXMLDocument::ElementContent( CONST WCHAR *strData, UINT DataLen, BOOL More )
{
   More;
   
   if (!mNodeStack.getSize())
      return E_FAIL;
   
   BNode* pNode = getNode(mNodeStack.back());
   
   BUString udata;
   BString data;
   createString(udata, data, strData, DataLen);
   
   if ((!More) && (!pNode->getTextIsCData()))
   {
      // This won't really do the right thing if the element contained CDATA!
      cleanText(udata);
      cleanText(data);
   }
   
   pNode->mText += data;
   pNode->mUText += udata;

   return S_OK;
}

//============================================================================
// BXMLDocument::ElementEnd
//============================================================================
HRESULT BXMLDocument::ElementEnd( CONST WCHAR *strName, UINT NameLen )
{
   strName;
   NameLen;
   
   if (mNodeStack.empty())
      return E_FAIL;
      
   mNodeStack.popBack();
   return S_OK;
}

//============================================================================
// BXMLDocument::CDATABegin
//============================================================================
HRESULT BXMLDocument::CDATABegin( )
{
   return S_OK;
}

//============================================================================
// BXMLDocument::CDATAData
//============================================================================
HRESULT BXMLDocument::CDATAData( CONST WCHAR *strCDATA, UINT CDATALen, BOOL bMore )
{
   bMore;

   if (!mNodeStack.getSize())
      return E_FAIL;

   BNode* pNode = getNode(mNodeStack.back());

   BUString udata;
   BString data;
   createString(udata, data, strCDATA, CDATALen);

   pNode->mUText += udata;
   pNode->mText += data;
   
   pNode->setTextIsCData(true);

   return S_OK;
}

//============================================================================
// BXMLDocument::CDATAEnd
//============================================================================
HRESULT BXMLDocument::CDATAEnd( )
{
   return S_OK;
}

//============================================================================
// BXMLDocument::Error
//============================================================================
VOID BXMLDocument::Error( HRESULT hError, CONST CHAR *strMessage )
{
   hError;
   strMessage;
   
   mErrorMessage.set(strMessage);
}

