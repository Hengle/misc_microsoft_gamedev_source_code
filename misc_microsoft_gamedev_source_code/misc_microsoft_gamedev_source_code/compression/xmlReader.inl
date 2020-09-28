// File: xmlReader.inl

template<typename StringType>
inline StringType& BXMLAttribute::getName(StringType& str) const
{
   const bool unicodeString = (sizeof(StringType::charType) == sizeof(WCHAR));
   unicodeString;
   
   str.set(getNamePtr());
   
#if XML_READER_DEBUG_LOGGING   
   mpReader->debugLog(unicodeString ? "BXMLAttribute::getName: %S" : "BXMLAttribute::getName: %s", str.getPtr());
#endif   

   return str;
}

template<typename StringType>
inline StringType& BXMLAttribute::getValue(StringType& str) const
{
   if (!getValid())
   {
      BDEBUG_ASSERT(false);
      str.empty();
      return str;
   }
   
   const bool unicodeString = (sizeof(StringType::charType) == sizeof(WCHAR));
   if (mpReader->mpXMLDoc)
   {
      if (unicodeString)
         str.set(mpReader->getDocAttribute(mNodeIndex, mAttributeIndex).getUText().getPtr());
      else  
         str.set(mpReader->getDocAttribute(mNodeIndex, mAttributeIndex).getText().getPtr());
         
#if XML_READER_DEBUG_LOGGING   
      mpReader->debugLog(unicodeString ? "BXMLAttribute::getValue: %s %S" : "BXMLAttribute::getValue: %s %s", getNamePtr(), str.getPtr());
#endif   
         
      return str;
   }
   
   const BPackedXMXData::BAttribute& attr = mpReader->getXMXAttribute(mNodeIndex, mAttributeIndex);
   const uint variantValue = attr.mText;
   
   const eXMXVariantType variantType = BXMXVariantHelpers::getVariantType(variantValue);
   const uint variantBits = BXMXVariantHelpers::getVariantBits(variantValue);   

   if ((!BXMXVariantHelpers::isStringVariant(variantValue)) || (!BXMXVariantHelpers::getVariantIsOffset(variantValue)))
   {
      bool unicode;
      if (!BXMXVariantHelpers::unpackVariantToString(mpReader->mStringBuffer.getPtr(), mpReader->mStringBuffer.getSize(), unicode, mpReader->mXMXVariantData, variantValue, unicodeString))
      {
         BDEBUG_ASSERT(0);
         str.empty();
         return str;
      }
      if (unicode)
         str.set((const WCHAR*)mpReader->mStringBuffer.getPtr());
      else
         str.set((const char*)mpReader->mStringBuffer.getPtr());
   }
   else if (variantType == cXMXVTUString)
   {
      str.set((const WCHAR*)(mpReader->mXMXVariantData.getPtr() + variantBits));
   }
   else
   {
      BDEBUG_ASSERT(variantType == cXMXVTString);
      str.set((const char*)(mpReader->mXMXVariantData.getPtr() + variantBits));
   }
   
#if XML_READER_DEBUG_LOGGING   
   mpReader->debugLog(unicodeString ? "BXMLAttribute::getValue: %s %S" : "BXMLAttribute::getValue: %s %s", getNamePtr(), str.getPtr());
#endif      

   return str;
}

template<typename StringType>
inline typename const StringType::charType* BXMLAttribute::getValuePtr(StringType& str) const
{
   if (!getValid())
   {
      BDEBUG_ASSERT(false);
      str.empty();
      return str.getPtr();
   }

   const bool unicodeString = (sizeof(StringType::charType) == sizeof(WCHAR));
   if (mpReader->mpXMLDoc)
   {
      const BXMLDocument::BAttribute& node = mpReader->getDocAttribute(mNodeIndex, mAttributeIndex);
      
      StringType::charType* pStr;
      if (unicodeString)
         pStr = (StringType::charType*)node.getUText().getPtr();
      else  
         pStr = (StringType::charType*)node.getText().getPtr();
         
#if XML_READER_DEBUG_LOGGING   
      mpReader->debugLog(unicodeString ? "BXMLAttribute::getValuePtr: %s %S" : "BXMLAttribute::getValuePtr: %s %s", getNamePtr(), pStr);
#endif   

      return pStr;         
   }

   const BPackedXMXData::BAttribute& attribute = mpReader->getXMXAttribute(mNodeIndex, mAttributeIndex);
   const uint variantValue = attribute.mText;

   const eXMXVariantType variantType = BXMXVariantHelpers::getVariantType(variantValue);
   const uint variantBits = BXMXVariantHelpers::getVariantBits(variantValue);   

   if (BXMXVariantHelpers::getVariantIsOffset(variantValue))
   {
      if ( ((variantType == cXMXVTUString) && ( unicodeString)) || 
         ((variantType == cXMXVTString ) && (!unicodeString)) )
      {
         StringType::charType* pStr = (StringType::charType*)(mpReader->mXMXVariantData.getPtr() + variantBits);
         
#if XML_READER_DEBUG_LOGGING   
         mpReader->debugLog(unicodeString ? "BXMLAttribute::getValuePtr: %s %S" : "BXMLAttribute::getValuePtr: %s %s", getNamePtr(), pStr);
#endif   

         return pStr;
      }
   }

   bool unicode;
   if (!BXMXVariantHelpers::unpackVariantToString(mpReader->mStringBuffer.getPtr(), mpReader->mStringBuffer.getSize(), unicode, mpReader->mXMXVariantData, variantValue, unicodeString))
   {
      BDEBUG_ASSERT(0);
      str.empty();
      return str.getPtr();
   }

   if (unicode)
      str.set(mpReader->getStringBufferWPtr());
   else
      str.set(mpReader->getStringBufferPtr());
      
#if XML_READER_DEBUG_LOGGING   
   mpReader->debugLog(unicodeString ? "BXMLAttribute::getValuePtr: %s %S" : "BXMLAttribute::getValuePtr: %s %s", getNamePtr(), str.getPtr());
#endif         

   return str.getPtr();
}

template<typename StringType>
inline StringType& BXMLNode::getName(StringType& str) const
{
   str.set(getNamePtr());
   
#if XML_READER_DEBUG_LOGGING   
   mpReader->debugLog("BXMLNode::getName: %s", getNamePtr());
#endif            

   return str;
}

template<typename StringType>
inline StringType& BXMLNode::getText(StringType& str) const
{
   const bool unicodeString = (sizeof(StringType::charType) == sizeof(WCHAR));
   unicodeString;
   
   const StringType::charType* p = getTextPtr(str);
   
   if (p != str.getPtr())
      str.set(p);

#if XML_READER_DEBUG_LOGGING   
   mpReader->debugLog(unicodeString ? "BXMLNode::getText: %s %S" : "BXMLNode::getText: %s %s", getNamePtr(), str.getPtr());
#endif               
   
   return str;
}

template<typename StringType>
inline typename const StringType::charType* BXMLNode::getTextPtr(StringType& str) const
{
   if (!getValid())
   {
      BDEBUG_ASSERT(false);
      str.empty();
      return str.getPtr();
   }

   const bool unicodeString = (sizeof(StringType::charType) == sizeof(WCHAR));
   if (mpReader->mpXMLDoc)
   {
      const BXMLDocument::BNode& node = mpReader->getDocNode(mNodeIndex);
      
      StringType::charType* pStr;
      if (unicodeString)
         pStr = (StringType::charType*)node.getUText().getPtr();
      else  
         pStr = (StringType::charType*)node.getText().getPtr();
         
#if XML_READER_DEBUG_LOGGING   
      mpReader->debugLog(unicodeString ? "BXMLNode::getTextPtr: %s %S" : "BXMLNode::getTextPtr: %s %s", getNamePtr(), pStr);
#endif          

      return pStr;  
   }

   const BPackedXMXData::BNode& node = mpReader->getXMXNode(mNodeIndex);
   const uint variantValue = node.mText;

   const eXMXVariantType variantType = BXMXVariantHelpers::getVariantType(variantValue);
   const uint variantBits = BXMXVariantHelpers::getVariantBits(variantValue);   

   if (BXMXVariantHelpers::getVariantIsOffset(variantValue))
   {
      if ( ((variantType == cXMXVTUString) && ( unicodeString)) || 
           ((variantType == cXMXVTString ) && (!unicodeString)) )
      {
         StringType::charType* pStr = (StringType::charType*)(mpReader->mXMXVariantData.getPtr() + variantBits);

#if XML_READER_DEBUG_LOGGING   
         mpReader->debugLog(unicodeString ? "BXMLNode::getTextPtr: %s %S" : "BXMLNode::getTextPtr: %s %s", getNamePtr(), pStr);
#endif       
         
         return pStr;
      }
   }
   
   bool unicode;
   if (!BXMXVariantHelpers::unpackVariantToString(mpReader->mStringBuffer.getPtr(), mpReader->mStringBuffer.getSize(), unicode, mpReader->mXMXVariantData, variantValue, unicodeString))
   {
      BDEBUG_ASSERT(0);
      str.empty();
      return str.getPtr();
   }
   
   if (unicode)
      str.set(mpReader->getStringBufferWPtr());
   else
      str.set(mpReader->getStringBufferPtr());
      
#if XML_READER_DEBUG_LOGGING   
   mpReader->debugLog(unicodeString ? "BXMLNode::getTextPtr: %s %S" : "BXMLNode::getTextPtr: %s %s", getNamePtr(), str.getPtr());
#endif             

   return str.getPtr();
}

template<typename StringType>
inline bool BXMLNode::getTextAsString(StringType& str) const
{
   if (!getValid())
   {
      BDEBUG_ASSERT(false);
      str.empty();
      return false;
   }
   
   getText(str);
   return true;
}

template<typename StringType>
inline StringType& BXMLNode::getAttribValue(const char* pName, StringType& str) const
{
   if ((!getValid()) || (!pName))
   {
      BDEBUG_ASSERT(false);
      str.empty();
      return str;
   }
   
   BXMLAttribute attribute;
   if (!getAttribute(pName, &attribute))
   {
#if XML_READER_DEBUG_LOGGING
      mpReader->debugLog("BXMLReader::getAttribValue<StringType>: %i %s", false, pName);
#endif   
#if 0
#ifdef BUILD_DEBUG
      if (!str.isEmpty())
         trace("BXMLNode::getAttribValue: Setting return string \"%s\" to empty on failure", str.getPtr());
#endif
#endif
      str.empty();
      return str;
   }
   
#if XML_READER_DEBUG_LOGGING
   mpReader->debugLog("BXMLReader::getAttribValue<StringType>: %i %s", true, pName);
#endif      

   attribute.getValue(str);
   return true;
}

template<typename StringType>
inline bool BXMLNode::getAttribValue(const char* pName, StringType* pString) const
{
   if ((!getValid()) || (!pName))
   {
      BDEBUG_ASSERT(false);
      if (pString)      
         pString->empty();
      return false;
   }
   
   BXMLAttribute attribute;
   if (!getAttribute(pName, &attribute))
   {
#if XML_READER_DEBUG_LOGGING
      mpReader->debugLog("BXMLReader::getAttribValue<StringType*>: %i %s", false, pName);
#endif
      if (pString)      
      {
#if 0      
#ifdef BUILD_DEBUG
         if (!pString->isEmpty())
            trace("BXMLNode::getAttribValue: Setting return string \"%s\" to empty on failure", pString->getPtr());
#endif
#endif
         pString->empty();
      }
      return false;
   }
      
   if (pString)      
      attribute.getValue(*pString);
      
#if XML_READER_DEBUG_LOGGING
   mpReader->debugLog("BXMLReader::getAttribValue<StringType*>: %i %s", true, pName);
#endif      

   return true;      
}

template<typename StringType>
inline bool BXMLNode::getAttribValueAsString(const char* pName, StringType& str) const
{
   if ((!getValid()) || (!pName))
   {
      BDEBUG_ASSERT(false);
      str.empty();
      return false;
   }
   
   BXMLAttribute attribute;
   if (!getAttribute(pName, &attribute))
   {
#if XML_READER_DEBUG_LOGGING
      mpReader->debugLog("BXMLReader::getAttribValueAsString: %i %s", false, pName);
#endif

#if 0
#ifdef BUILD_DEBUG
      if (!str.isEmpty())
         trace("BXMLNode::getAttribValueAsString: Setting return string \"%s\" to empty on failure", str.getPtr());
#endif
#endif

      str.empty();
      return false;
   }

   attribute.getValue(str);

#if XML_READER_DEBUG_LOGGING
   mpReader->debugLog("BXMLReader::getAttribValueAsString: %i %s", true, pName);
#endif   
   
   return true;
}

template<typename StringType>
inline bool BXMLNode::getChildValue(const char* pName, StringType* pString) const
{
   if ((!getValid()) || (!pName))
   {
      BDEBUG_ASSERT(false);
      if (pString)
         pString->empty();
      return false;
   }
   
   BXMLNode node;
   if (!getChild(pName, &node))
   {
#if XML_READER_DEBUG_LOGGING
      mpReader->debugLog("BXMLReader::getChildValue<StringType*>: %i %s", false, pName);
#endif
      if (pString)
      {
#if 0      
#ifdef BUILD_DEBUG
         if (!pString->isEmpty())
            trace("BXMLNode::getChildValue: Setting return string \"%s\" to empty on failure", pString->getPtr());
#endif      
#endif
         pString->empty();
      }
      return false;
   }
   
   if (pString)
      node.getText(*pString);
 
#if XML_READER_DEBUG_LOGGING
   mpReader->debugLog("BXMLReader::getChildValue<StringType*>: %i %s", true, pName);
#endif      
   
   return true;
}