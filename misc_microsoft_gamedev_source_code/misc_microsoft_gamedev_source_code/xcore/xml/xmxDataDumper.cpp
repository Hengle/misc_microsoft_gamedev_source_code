//============================================================================
//
// File: xmxDataDumper.cpp
// Copyright (c) 2005-2006, Ensemble Studios
//
//============================================================================
#include "xcore.h"
#include "xmxDataDumper.h"
#include "string\strHelper.h"

//============================================================================
// BXMXDataDumper::BXMXDataDumper
//============================================================================
BXMXDataDumper::BXMXDataDumper() :
   mpTextDispatcher(NULL),
   mpXMXData(NULL),
   mIndent(0),
   mIndentSize(2)
{
}

//============================================================================
// BXMXDataDumper::dumpNode
//============================================================================
bool BXMXDataDumper::dumpNode(uint nodeIndex)
{
   const BXMXDataType::BNode& node = mpXMXData->getNode(nodeIndex);
   
   const uint cStrSize = 8192;
   typedef BFixedString<cStrSize> BStringType;
   std::auto_ptr<BStringType> pName(new BStringType);
   std::auto_ptr<BStringType> pText(new BStringType);
   std::auto_ptr<BStringType> pStr(new BStringType);
   std::auto_ptr<BStringType> pAttrName(new BStringType);
   std::auto_ptr<BStringType> pAttrText(new BStringType);
         
   bool unicode;
   if (!BXMXVariantHelpers::unpackVariantToString(pName->getPtr(), pName->getBufSize(), unicode, BConstDataBuffer(mpXMXData->getVariantData().getPtr(), mpXMXData->getVariantData().getSize()), node.mName, false))
      return false;
   
   if (!BXMXVariantHelpers::unpackVariantToString(pText->getPtr(), pText->getBufSize(), unicode, BConstDataBuffer(mpXMXData->getVariantData().getPtr(), mpXMXData->getVariantData().getSize()), node.mText, false))
      return false;
      
   escapeXMLString(*pText);      

   //const uint nameLen = pName->getLen();
   const uint textLen = pText->getLen();
   
   const uint numChildren = node.mChildren.getSize();
   const uint numAttributes = node.mAttributes.getSize();
   
   const uint numIndentChars = mIndent * mIndentSize;
   if (numIndentChars >= cStrSize)
      return false;
   
   pStr->appendChars(' ', numIndentChars, 0);
   
   pStr->formatAppend(numAttributes ? "<%s " : "<%s", pName->getPtr());
   
   for (uint attrIndex = 0; attrIndex < numAttributes; attrIndex++)
   {
      const BXMXDataType::BAttribute& attribute = node.mAttributes[attrIndex];
      
      if (!BXMXVariantHelpers::unpackVariantToString(pAttrName->getPtr(), pAttrName->getBufSize(), unicode, BConstDataBuffer(mpXMXData->getVariantData().getPtr(), mpXMXData->getVariantData().getSize()), attribute.mName, false))
         return false;         
      
      if (!BXMXVariantHelpers::unpackVariantToString(pAttrText->getPtr(), pAttrName->getBufSize(), unicode, BConstDataBuffer(mpXMXData->getVariantData().getPtr(), mpXMXData->getVariantData().getSize()), attribute.mText, false))
         return false;         
      
      escapeXMLString(*pAttrText);         
         
      pStr->formatAppend("%s=\"%s\"", pAttrName->getPtr(), pAttrText->getPtr());
      
      if (attrIndex < (numAttributes - 1))
         pStr->append(" ");
   }
   
   if ((!textLen) && (!numChildren))
   {
      pStr->append(" />\n");

      mpTextDispatcher->printf("%s", pStr->getPtr());
   }
   else if (!numChildren)
   {
      pStr->formatAppend(">%s</%s>\n", pText->getPtr(), pName->getPtr());
                  
      mpTextDispatcher->printf("%s", pStr->getPtr());
   }
   else 
   {
      if (textLen)
         pStr->formatAppend(">%s\n", pText->getPtr());
      else
         pStr->append(">\n");
      
      mpTextDispatcher->printf("%s", pStr->getPtr());
      
      pStr->empty();
      
      mIndent++;
      
      for (uint childIndex = 0; childIndex < numChildren; childIndex++)
      {
         if (!dumpNode(node.mChildren[childIndex]))
            return false;
      }
      
      mIndent--;
      
      pStr->appendChars(' ', numIndentChars, 0);
      pStr->formatAppend("</%s>\n", pName->getPtr());
      
      mpTextDispatcher->printf("%s", pStr->getPtr());
   }
   
   return true;
}

//============================================================================
// BXMXDataDumper::dump
//============================================================================
bool BXMXDataDumper::dump(BTextDispatcher& textDispatcher, const BConstDataBuffer& buf)
{
   mpTextDispatcher = &textDispatcher;
   
   textDispatcher.printf("<?xml version=\"1.0\"?>\n");
   
   mIndent = 0;

   BByteArray tempBuf;
   tempBuf.reserve(buf.getLen());
   tempBuf.pushBack(buf.getPtr(), buf.getLen());
   
   mpXMXData = reinterpret_cast<BXMXDataType*>(tempBuf.getPtr());
   
   if (!mpXMXData->unpack(BDataBuffer(tempBuf.getPtr(), tempBuf.getSize())))
      return false;
      
   if (!mpXMXData->getNumNodes())      
      return false;

   if (!dumpNode(0))
      return false;
   
   return true;
}
