//============================================================================
//
// File: binaryDataTree.cpp
// Copyright (c) 2007, Ensemble Studios
//
//============================================================================
#include "xcore.h"
#include "binaryDataTree.h"
#include "string\strHelper.h"
#include "hash\crc.h"
#include "nameValueMap.h"

namespace BBinaryDataTree
{
   BTypeDesc gNullTypeDesc(cTypeNull, 0, 1, false);
   BTypeDesc gBoolTypeDesc(cTypeBool, 1, 1, true);
   BTypeDesc gUInt8TypeDesc(cTypeInt, 1, 1, true), gInt8TypeDesc(cTypeInt, 1, 1, false);
   BTypeDesc gUInt16TypeDesc(cTypeInt, 2, 2, true), gInt16TypeDesc(cTypeInt, 2, 2, false);
   BTypeDesc gUInt32TypeDesc(cTypeInt, 4, 4, true), gInt32TypeDesc(cTypeInt, 4, 4, false);
   BTypeDesc gUInt64TypeDesc(cTypeInt, 8, 8, true), gInt64TypeDesc(cTypeInt, 8, 8, false);
   BTypeDesc gFloatTypeDesc(cTypeFloat, 4, 4, false), gDoubleTypeDesc(cTypeFloat, 8, 8, false);
   BTypeDesc gStringTypeDesc(cTypeString, 1, 1, true), gUStringTypeDesc(cTypeString, 2, 2, true);
   BTypeDesc gFloatVectorTypeDesc(cTypeFloat, 4, 16, false);
      
   BDocumentBuilder::BDocumentBuilder() :
      mpRootNode(NULL)
   {
      clear();
   }
            
   void BDocumentBuilder::clear()
   {
      mBuildNodes.clear();
      mUserSections.clear();
      
      mpRootNode = mBuildNodes.acquire(true);
      mpRootNode->clear();
      mpRootNode->mNameValues.grow().mName.set("root");
   }
   
   uint BDocumentBuilder::getNumNodes() const
   {
      return mBuildNodes.getNumberAllocated();
   }

   bool BDocumentBuilder::packName(const BString& name, BStringHashMap& nameHash, BByteArray& nameData, uint& nameOfs)
   {
      BStringHashMap::const_iterator it = nameHash.find(name);
      if (it != nameHash.end())
      {
         nameOfs = it->second;
         return true;
      }
      
      nameOfs = nameData.getSize();
      nameData.pushBack((const BYTE*)name.getPtr(), name.length() + 1);
      
      nameHash.insert(name, nameOfs);
      return true;
   }
   
   bool BDocumentBuilder::packVariant(const BBuildVariantData& variant, BBuildVariantDataHashMap& variantHash, bool bigEndian, BByteArray& variantData, uint& variantOfs)
   {
      variantOfs = UINT_MAX;
      
      if (variant.mData.isEmpty())
         return true;
      
      if (variant.mTypeDesc.mClass == cTypeString)
      {
#if DIRECT_STRING_VARIANTS
         if ((variant.mTypeDesc.mSizeInBytes == 1) && (variant.mData.getSize() <= 5))
            return true;
#endif            
      }
      else if ((variant.mArraySize == 1) && (variant.mData.getSize() <= 4))
         return true;
      
      BBuildVariantDataHashMap::const_iterator it = variantHash.find(variant);
      if (it != variantHash.end())
      {
         variantOfs = it->second;
         return true;
      }

      const uint writeSize = (variant.mData.getSize() >= (1U << BPackedNameValue::cSizeBits));
               
      uint alignment = variant.mTypeDesc.mAlignment;
      if (writeSize)
         alignment = Math::Max(alignment, sizeof(uint32));

      uint bytesToAlign = Utils::BytesToAlignUpValue(variantData.getSize(), alignment);

      if (writeSize)
      {
         if (bytesToAlign < sizeof(uint32))
            bytesToAlign += alignment;
      }

      for (uint j = 0; j < bytesToAlign; j++)
         variantData.pushBack(0);

      variantOfs = variantData.getSize();

      variantData.append(variant.mData);

      BYTE* p = variantData.getPtr() + variantOfs;

      if (writeSize)
      {
         uint32* q = (uint32*)(p) - 1;
         *q = variant.mData.getSize();
         if (bigEndian != cBigEndianNative)
            EndianSwitchDWords((DWORD*)q, 1);
      }

      if (bigEndian != cBigEndianNative)
      {
         if (variant.mTypeDesc.mSizeInBytes == 2)
            EndianSwitchWords((WORD*)p, variant.mData.getSize() >> 1U);
         else if (variant.mTypeDesc.mSizeInBytes == 4)
            EndianSwitchDWords((DWORD*)p, variant.mData.getSize() >> 2U);
         else if (variant.mTypeDesc.mSizeInBytes == 8)
            EndianSwitchQWords((uint64*)p, variant.mData.getSize() >> 3U);
      }               
      
      variantHash.insert(variant, variantOfs);
            
      return true;
   }
   
   bool BDocumentBuilder::createNewNodes(BDynamicArray<BBuildNode>& newNodes)
   {
      UIntArray nodeStack;
      if (mpRootNode)
      {
         nodeStack.pushBack(getNodeIndex(mpRootNode));
         newNodes.pushBack(*mpRootNode);
         newNodes[0].mpParent = (BBuildNode*)UINT_MAX;
      }

      while (!nodeStack.isEmpty())
      {
         BNodeIndex parentNodeIndex = nodeStack.back();
         nodeStack.popBack();
         
         const uint numChildren = newNodes[parentNodeIndex].mChildren.getSize();           
         if (!numChildren)
            continue;
         
         const uint firstChildIndex = newNodes.getSize();
         newNodes.enlarge(numChildren);

         for (uint i = 0; i < numChildren; i++)
         {
            BNodeIndex newChildNodeIndex = firstChildIndex + i;
            const BBuildNode* pChildNode = newNodes[parentNodeIndex].mChildren[i];

            BBuildNode& newChildNode = newNodes[newChildNodeIndex];

            newChildNode = *pChildNode;
            newChildNode.mpParent = (BBuildNode*)parentNodeIndex;

            newNodes[parentNodeIndex].mChildren[i] = (BBuildNode*)newChildNodeIndex;

            if (!newChildNode.mChildren.isEmpty())
               nodeStack.pushBack(newChildNodeIndex);
         }
         
      }

      BDEBUG_ASSERT(newNodes.getSize() == mBuildNodes.getNumberAllocated());
      
      return true;
   }
   
   bool BDocumentBuilder::createPackedNodes(
      bool bigEndian, 
      BDynamicArray<BPackedNameValue>& packedNameValues, 
      BDynamicArray<BPackedNode>& packedNodes,
      BByteArray& nameData, BByteArray& variantData)
   {
      BStringHashMap nameHash;
      BBuildVariantDataHashMap variantHash;
      
      BDynamicArray<BBuildNode> newNodes;
      if (!createNewNodes(newNodes))
         return false;
         
      packedNodes.resize(newNodes.getSize());
      
      for (uint i = 0; i < newNodes.getSize(); i++)
      {
         const BBuildNode& node = newNodes[i];
         BPackedNode& packedNode = packedNodes[i];
         
         uint parentIndex = (uint)node.mpParent;
         if (parentIndex == UINT_MAX)
            packedNode.mParentIndex = UINT16_MAX;
         else
         {
            if (parentIndex > UINT16_MAX)
               return false;
            BDEBUG_ASSERT(parentIndex < newNodes.getSize());
            packedNode.mParentIndex = static_cast<uint16>(parentIndex);
         }
         
         if (node.mChildren.isEmpty())
         {
            packedNode.mChildNodeIndex = UINT16_MAX;
            packedNode.mNumChildrenNodes = 0;
         }
         else
         {
            const uint childIndex = (uint)node.mChildren[0];
            if (childIndex >= UINT16_MAX)
               return false;
            BDEBUG_ASSERT(childIndex < newNodes.getSize());
            packedNode.mChildNodeIndex = static_cast<uint16>(childIndex);
            packedNode.mNumChildrenNodes = static_cast<uint8>(Math::Min(0xFFU, node.mChildren.getSize()));
         }
         
         const uint numNameValues = node.mNameValues.getSize();
         packedNode.mNumNameValues = static_cast<uint8>(Math::Min(0xFFU, numNameValues));
         
         const uint firstNameValueIndex = packedNameValues.getSize();
         packedNameValues.enlarge(numNameValues);
         
         if (firstNameValueIndex >= UINT16_MAX)
            return false;
            
        packedNode.mNameValueOfs = static_cast<uint16>(firstNameValueIndex);
        
         for (uint i = 0; i < numNameValues; i++)
         {
            const BBuildNameValue& buildNameValue = node.mNameValues[i];
            const BString& name = buildNameValue.mName;
            const BBuildVariantData& variant = buildNameValue.mVariant;
            
            BPackedNameValue& packedNameValue = packedNameValues[firstNameValueIndex + i];
            
            uint nameOfs;
            if (!packName(name, nameHash, nameData, nameOfs))                        
               return false;
            if (nameOfs > UINT16_MAX)
               return false;
            packedNameValue.mNameOfs = static_cast<uint16>(nameOfs);
            
            packedNameValue.mFlags = 0;
            packedNameValue.mValue = 0;
            
            packedNameValue.mFlags |= (variant.mTypeDesc.mClass << BPackedNameValue::cTypeShift);
            
            if (i == (numNameValues - 1))
               packedNameValue.mFlags |= BPackedNameValue::cLastNameValueMask;
            
            if (variant.mTypeDesc.mClass == cTypeNull)
               continue;
               
            uint variantOfs;
            if (!packVariant(variant, variantHash, bigEndian, variantData, variantOfs))
               return false;
                        
            const bool directEncoding = (variantOfs == UINT_MAX);
            
            if (directEncoding)   
            {
               BDEBUG_ASSERT(variant.mArraySize == 1);
               
               packedNameValue.mFlags |= BPackedNameValue::cDirectEncodingMask;
               
               switch (variant.mTypeDesc.mClass)
               {
                  case cTypeBool:
                  {
                     packedNameValue.mValue = variant.asBool();
                     break;
                  }
                  case cTypeInt:
                  {
                     if (variant.mTypeDesc.mIsUnsigned)
                        packedNameValue.mValue = (uint32)variant.asUnsigned();
                     else
                        packedNameValue.mValue = (uint32)((int32)variant.asSigned());
                     break;
                  }
                  case cTypeFloat:
                  {
                     float v = variant.asFloat();
                     packedNameValue.mValue = *reinterpret_cast<const uint32*>(&v);
                     break;
                  }
#if DIRECT_STRING_VARIANTS
                  case cTypeString:
                  {
                     BDEBUG_ASSERT(variant.mData.getSize());
                     const uint strLen = variant.mData.getSize() - 1;
                     for (uint i = 0; i < strLen; i++)
                        packedNameValue.mValue |= (variant.mData[i] << (i * 8U));
                     break;
                  }
#endif                  
                  default:
                  {
                     BDEBUG_ASSERT(false);
                     return false;
                  }
               }
            }
            else
            {
               packedNameValue.mValue = variantOfs;
            }
            
            const uint packedSize = Math::Min(variant.mData.getSize(), (1U << BPackedNameValue::cSizeBits) - 1U);
            packedNameValue.mFlags |= (packedSize << BPackedNameValue::cSizeShift);
            
            const uint typeSizeInBytesLog2 = Math::iLog2(variant.mTypeDesc.mSizeInBytes);
            packedNameValue.mFlags |= (typeSizeInBytesLog2 << BPackedNameValue::cTypeSizeInBytesLog2Shift);
            
            if (variant.mTypeDesc.mIsUnsigned)
               packedNameValue.mFlags |= BPackedNameValue::cTypeIsUnsignedMask;
         }
      }
      
      if (bigEndian != cBigEndianNative)      
      {
         for (uint i = 0; i < packedNodes.getSize(); i++)
            packedNodes[i].endianSwitch();
         
         for (uint i = 0; i < packedNameValues.getSize(); i++)
            packedNameValues[i].endianSwitch();
      }
      
      return true;
   }
   
   bool BDocumentBuilder::serialize(BByteArray& data, bool bigEndian)
   {
      if (mBuildNodes.getNumberAllocated() >= UINT16_MAX)
         return false;
         
      BByteArray nameData;
      BByteArray variantData;
      
      BDynamicArray<BPackedNameValue> packedNameValues;
      BDynamicArray<BPackedNode> packedNodes;
      if (!createPackedNodes(bigEndian, packedNameValues, packedNodes, nameData, variantData))
         return false;
           
      if (mUserSections.getSize() > cMaxUserSections)
         return false;
            
      BPackedHeader header;
      Utils::ClearObj(header);
      
      header.mSig = static_cast<uint8>(bigEndian ? BPackedHeader::cBigEndianSig : BPackedHeader::cLittleEndianSig);
      header.mHeaderDWORDs = static_cast<uint8>(sizeof(header) / sizeof(DWORD));
            
      header.mNumUserSections = static_cast<uint8>(mUserSections.getSize());
           
      uint curOfs = sizeof(header) + sizeof(BPackedSection) * mUserSections.getSize();
      
      header.mBaseSectionSizes[cNodeSectionIndex] = packedNodes.getSizeInBytes();
      const uint nodeOfs = packedNodes.getSizeInBytes() ? curOfs : 0;
      curOfs += packedNodes.getSizeInBytes();
      BDEBUG_ASSERT((curOfs & 3) == 0);
                  
      header.mBaseSectionSizes[cNameValueSectionIndex] = packedNameValues.getSizeInBytes();
      const uint nameValueOfs = packedNameValues.getSizeInBytes() ? curOfs : 0;
      curOfs += packedNameValues.getSizeInBytes();
      BDEBUG_ASSERT((curOfs & 3) == 0);
      
      header.mBaseSectionSizes[cNameDataSectionIndex] = nameData.getSizeInBytes();
      const uint nameDataOfs = nameData.getSizeInBytes() ? curOfs : 0;
      curOfs += nameData.getSizeInBytes();
      
      if (variantData.getSize())
         curOfs += Utils::BytesToAlignUpValue(curOfs, 16);
      
      header.mBaseSectionSizes[cValueDataSectionIndex] = variantData.getSizeInBytes();
      const uint valueDataOfs = variantData.getSizeInBytes() ? curOfs : 0;
      curOfs += variantData.getSizeInBytes();

      BDynamicArray<BPackedSection> sections;
      sections.resize(mUserSections.getSize());
                  
      for (uint i = 0; i < mUserSections.getSize(); i++)
      {
         BPackedSection& section = sections[i];
           
         section.mID = mUserSections[i].mID;
         if (mUserSections[i].mData.isEmpty())
         {
            section.mOfs = 0;
            section.mSize = 0;
         }
         else
         {
            curOfs += Utils::BytesToAlignUpValue(curOfs, 16);
            
            section.mSize = mUserSections[i].mData.getSizeInBytes();
            section.mOfs = curOfs;
            curOfs += section.mSize;
         }
      }
      
      data.resize(curOfs);
      BYTE* pDst = data.getPtr();
         
      Utils::FastMemCpy(pDst + sizeof(header), sections.getPtr(), sections.getSizeInBytes());   
      
      if (bigEndian != cBigEndianNative)
      {
         BPackedSection* pPackedSections = reinterpret_cast<BPackedSection*>(pDst + sizeof(header));
         for (uint i = 0; i < sections.getSize(); i++)
            pPackedSections[i].endianSwitch();
      }
      
      if (packedNodes.getSize())
         Utils::FastMemCpy(pDst + nodeOfs, packedNodes.getPtr(), packedNodes.getSizeInBytes());
      
      if (packedNameValues.getSize())
         Utils::FastMemCpy(pDst + nameValueOfs, packedNameValues.getPtr(), packedNameValues.getSizeInBytes());
      
      if (nameData.getSize())
         Utils::FastMemCpy(pDst + nameDataOfs, nameData.getPtr(), nameData.getSizeInBytes());
         
      if (variantData.getSize())
         Utils::FastMemCpy(pDst + valueDataOfs, variantData.getPtr(), variantData.getSizeInBytes());
       
      for (uint i = 0; i < mUserSections.getSize(); i++)
      {
         if (mUserSections[i].mData.getSize())
            Utils::FastMemCpy(pDst + sections[i].mOfs, mUserSections[i].mData.getPtr(), mUserSections[i].mData.getSizeInBytes());
      }
      
      header.mDataSize = data.getSize() - sizeof(header);
      header.mDataCRC32 = calcCRC32(pDst + sizeof(header), data.getSize() - sizeof(header));
      if (bigEndian != cBigEndianNative)
         header.endianSwitch();
      header.mHeaderCRC8 = (uint8)calcCRC16(&header, sizeof(header));
            
      Utils::FastMemCpy(pDst, &header, sizeof(header));
      
      return true;
   }
   
   bool BDocumentBuilder::deserialize(const BConstDataBuffer& data)
   {
      clear();
      if (!deserializeInternal(data))
      {
         clear();
         return false;
      }
      return true;
   }
         
   bool BDocumentBuilder::deserializeInternal(const BConstDataBuffer& data)
   {
      const BYTE* pSrc = data.getPtr();
      const uint srcLen = data.getLen();
      
      if (srcLen < sizeof(BPackedHeader))
         return false;
      
      BPackedHeader header(*reinterpret_cast<const BPackedHeader*>(pSrc));
      
      bool bigEndian = false;
      if (header.mSig == BPackedHeader::cLittleEndianSig)
         bigEndian = false;
      else if (header.mSig == BPackedHeader::cBigEndianSig)
         bigEndian = true;
      else
         return false;

      const bool switchEndianness = (cBigEndianNative != bigEndian);
            
      if (header.mHeaderDWORDs != sizeof(BPackedHeader) / sizeof(DWORD))
         return false;
         
      BPackedHeader tempHeader(header);
      tempHeader.mHeaderCRC8 = 0;
      if (((uint8)calcCRC16(&tempHeader, sizeof(tempHeader)) != header.mHeaderCRC8))
         return false;
      
      if (switchEndianness)
         header.endianSwitch();
         
      if (srcLen < (sizeof(BPackedHeader) + sizeof(BPackedSection) * header.mNumUserSections))
         return false;
      
      const uint totalSize = header.mDataSize + sizeof(BPackedHeader);
      if (srcLen < totalSize)
         return false;
         
      if (calcCRC32(pSrc + sizeof(BPackedHeader), header.mDataSize) != header.mDataCRC32)
         return false;
         
      BDynamicArray<BPackedSection> sections(header.mNumUserSections, reinterpret_cast<const BPackedSection*>(pSrc + sizeof(BPackedHeader)));
      if (switchEndianness)
      {
         for (uint i = 0; i < header.mNumUserSections; i++)
            sections[i].endianSwitch();  
      }
      
      for (uint i = 0; i < header.mNumUserSections; i++)
         if ((sections[i].mOfs + sections[i].mSize) > srcLen)
            return false;   
      
      uint curOfs = sizeof(header) + sizeof(BPackedSection) * header.mNumUserSections;
      
      const uint nodeOfs = header.mBaseSectionSizes[cNodeSectionIndex] ? curOfs : 0;
      curOfs += header.mBaseSectionSizes[cNodeSectionIndex];
      
      const uint nameValueOfs = header.mBaseSectionSizes[cNameValueSectionIndex] ? curOfs : 0;
      curOfs += header.mBaseSectionSizes[cNameValueSectionIndex];
      
      const uint nameDataOfs = header.mBaseSectionSizes[cNameDataSectionIndex] ? curOfs : 0;
      curOfs += header.mBaseSectionSizes[cNameDataSectionIndex];

      if (header.mBaseSectionSizes[cValueDataSectionIndex])
         curOfs += Utils::BytesToAlignUpValue(curOfs, 16);

      const uint valueDataOfs = header.mBaseSectionSizes[cValueDataSectionIndex] ? curOfs : 0;
      curOfs += header.mBaseSectionSizes[cValueDataSectionIndex];
      if (curOfs > srcLen)
         return false;

      const BYTE* pNameData   = nameDataOfs ? (pSrc + nameDataOfs) : NULL;
      const BYTE* pValueData  = valueDataOfs ? (pSrc + valueDataOfs) : NULL;

      const BPackedNode* pNodes = nodeOfs ? reinterpret_cast<const BPackedNode*>(pSrc + nodeOfs) : NULL;
      const uint numNodes = header.mBaseSectionSizes[cNodeSectionIndex] / sizeof(BPackedNode);
      BDynamicArray<BPackedNode> nodes;
      if (numNodes)
         nodes.pushBack(pNodes, numNodes);
      if (switchEndianness)
      {
         for (uint i = 0; i < numNodes; i++)
            nodes[i].endianSwitch();
      }
      
      const BPackedNameValue* pNameValues = nameValueOfs ? reinterpret_cast<const BPackedNameValue*>(pSrc + nameValueOfs) : NULL;
      const uint numNameValues = header.mBaseSectionSizes[cNameValueSectionIndex] / sizeof(BPackedNameValue);
      BDynamicArray<BPackedNameValue> nameValues;
      if (numNameValues)
      {
         nameValues.pushBack(pNameValues, numNameValues);
         if (switchEndianness)
         {
            for (uint i = 0; i < numNameValues; i++)
               nameValues[i].endianSwitch();
         }
      }
                  
      mBuildNodes.init(numNodes, true);
      BDynamicArray<BBuildNode*> nodePtrs(numNodes);
      BDynamicArray<uint> nodeIndices(numNodes);
      for (uint i = 0; i < numNodes; i++)
      {
         uint nodeIndex;
         BBuildNode* pBuildNode = mBuildNodes.acquire(nodeIndex, true);
         BDEBUG_ASSERT(pBuildNode);

         nodePtrs[i] = pBuildNode;
         nodeIndices[i] = nodeIndex;
      }
      
      for (uint i = 0; i < numNodes; i++)
      {
         const BPackedNode& packedNode = nodes[i];
         
         BBuildNode& buildNode = *nodePtrs[i];
         buildNode.clear();
         
         if (UINT16_MAX == packedNode.mParentIndex)
         {
            mpRootNode = &buildNode;
            buildNode.mpParent = NULL;
         }
         else
            buildNode.mpParent = nodePtrs[packedNode.mParentIndex];
         
         uint numChildNodes = packedNode.mNumChildrenNodes;
         if (numChildNodes == 0xFF)
         {
            uint childNodeIndex = packedNode.mChildNodeIndex;
                        
            for ( ; ; )
            {
               if ((childNodeIndex + numChildNodes) > numNodes)
                  return false;
               else if ((childNodeIndex + numChildNodes) == numNodes)
                  break;
               
               if (nodes[childNodeIndex + numChildNodes].mParentIndex != i)
                  break;
               
               numChildNodes++;
            }
         }
         buildNode.mChildren.resize(numChildNodes);
         for (uint j = 0; j < numChildNodes; j++)
            buildNode.mChildren[j] = nodePtrs[packedNode.mChildNodeIndex + j];
         
         uint numNameValues = packedNode.mNumNameValues;
         if (numNameValues == 0xFF)
         {
            uint nameValueIndex = packedNode.mNameValueOfs;
            
            for ( ; ; )
            {
               if ((nameValueIndex + numNameValues) >= numNameValues)
                  return false;
               
               if (nameValues[nameValueIndex + numNameValues].mFlags & BPackedNameValue::cLastNameValueMask)
                  break;
               
               numNameValues++;
            }
         }
         if (!numNameValues)
            return false;
         
         buildNode.mNameValues.resize(numNameValues);
         
         for (uint j = 0; j < numNameValues; j++)
         {
            const BPackedNameValue& nameValue = nameValues[packedNode.mNameValueOfs + j];
            BBuildNameValue& buildNameValue = buildNode.mNameValues[j];
            BBuildVariantData& variantData = buildNameValue.mVariant;
            
            if (j == (numNameValues - 1))
            {
               if ((nameValue.mFlags & BPackedNameValue::cLastNameValueMask) == 0)
                  return false;
            }
                        
            if ((!pNameData) || (nameValue.mNameOfs >= header.mBaseSectionSizes[cNameDataSectionIndex]))
               return false;
                        
            buildNameValue.mName.set((const char*)pNameData + nameValue.mNameOfs);
                        
            const bool directEncoding = (nameValue.mFlags & BPackedNameValue::cDirectEncodingMask) != 0;
            
            uint totalDataSize = (nameValue.mFlags & BPackedNameValue::cSizeMask) >> BPackedNameValue::cSizeShift;
            
            if (totalDataSize == ((1U << BPackedNameValue::cSizeBits) - 1U))
            {
               if (directEncoding)
                  return false;
                  
               if ((!pValueData) || (nameValue.mValue >= header.mBaseSectionSizes[cValueDataSectionIndex]))
                  return false;
                                 
               totalDataSize = ((DWORD*)(pValueData + nameValue.mValue))[-1];
               if (switchEndianness)
                  EndianSwitchDWords((DWORD*)&totalDataSize, 1);
                  
               if (totalDataSize < (1U << BPackedNameValue::cSizeBits))
                  return false;
            }
                                                                       
            variantData.mTypeDesc.mClass = static_cast<eTypeClass>((nameValue.mFlags & BPackedNameValue::cTypeMask) >> BPackedNameValue::cTypeShift);
            if (variantData.mTypeDesc.mClass >= cNumTypes)
               return false;
            
            if (variantData.mTypeDesc.mClass == cTypeNull)
            {
               if (totalDataSize)
                  return false;
               variantData.mTypeDesc.mSizeInBytes = 0;
            }
            else
            {
               if (!totalDataSize)
                  return false;
                  
               variantData.mTypeDesc.mSizeInBytes = static_cast<uint16>(1U << ((nameValue.mFlags & BPackedNameValue::cTypeSizeInBytesLog2Mask) >> BPackedNameValue::cTypeSizeInBytesLog2Shift));
            }
            
            if (variantData.mTypeDesc.mSizeInBytes > sizeof(uint64))
               return false;
            
            variantData.mArraySize = 0;
            if (variantData.mTypeDesc.mSizeInBytes)
            {
               if ((totalDataSize % variantData.mTypeDesc.mSizeInBytes) != 0)
                  return false;
               variantData.mArraySize = totalDataSize / variantData.mTypeDesc.mSizeInBytes;
            }
               
            variantData.mTypeDesc.mIsUnsigned = (nameValue.mFlags & BPackedNameValue::cTypeIsUnsignedMask) != 0;

            variantData.mData.resize(0);                                                
            
            if (variantData.mTypeDesc.mClass == cTypeNull)                                                
            {
            
            }
            else if (!directEncoding)
            {
               if ((nameValue.mValue + totalDataSize) > header.mBaseSectionSizes[cValueDataSectionIndex])
                  return false;
               
               if ((nameValue.mValue & 127) == 0)
                  variantData.mTypeDesc.mAlignment = 128;   
               else if ((nameValue.mValue & 63) == 0)
                  variantData.mTypeDesc.mAlignment = 64;   
               else if ((nameValue.mValue & 31) == 0)
                  variantData.mTypeDesc.mAlignment = 32;
               else if ((nameValue.mValue & 15) == 0)
                  variantData.mTypeDesc.mAlignment = 16;
               else if ((nameValue.mValue & 7) == 0)
                  variantData.mTypeDesc.mAlignment = 8;
               else if ((nameValue.mValue & 3) == 0)
                  variantData.mTypeDesc.mAlignment = 4;
               else if ((nameValue.mValue & 1) == 0)
                  variantData.mTypeDesc.mAlignment = 2;
               
               variantData.mData.pushBack(pValueData + nameValue.mValue, totalDataSize);
               
               if (variantData.mTypeDesc.mClass == cTypeString)
               {
                  if (variantData.mTypeDesc.mSizeInBytes == sizeof(char))
                  {
                     variantData.mTypeDesc.mAlignment = sizeof(char);
                     if (!BStringArrayHelpers::getSize((const char*)variantData.mData.getPtr(), variantData.mData.getSizeInBytes(), variantData.mArraySize))
                        return false;
                  }
                  else if (variantData.mTypeDesc.mSizeInBytes == sizeof(wchar_t))
                  {
                     if (switchEndianness)
                        EndianSwitchWords((WORD*)variantData.mData.getPtr(), variantData.mData.getSizeInBytes() >> 1U);
                     
                     variantData.mTypeDesc.mAlignment = sizeof(wchar_t);
                     if (!BStringArrayHelpers::getSize((const wchar_t*)variantData.mData.getPtr(), variantData.mData.getSizeInBytes(), variantData.mArraySize))
                        return false;
                  }                        
                  else
                     return false;
               }
               else 
               {
                  if (switchEndianness)
                  {
                     if (variantData.mTypeDesc.mSizeInBytes == sizeof(WORD))
                        EndianSwitchWords((WORD*)variantData.mData.getPtr(), variantData.mData.getSize() >> 1U);
                     else if (variantData.mTypeDesc.mSizeInBytes == sizeof(DWORD))
                        EndianSwitchDWords((DWORD*)variantData.mData.getPtr(), variantData.mData.getSize() >> 2U);
                     else if (variantData.mTypeDesc.mSizeInBytes == sizeof(uint64))
                        EndianSwitchQWords((uint64*)variantData.mData.getPtr(), variantData.mData.getSize() >> 3U);
                  }
               }                        
            }
            else
            {
               switch (variantData.mTypeDesc.mClass)
               {
                  case cTypeNull:
                  case cTypeBool:
                  {
                     if (variantData.mArraySize != 1)
                        return false;
                        
                     if (totalDataSize != sizeof(BYTE))
                        return false;
                     
                     variantData.mData.pushBack(nameValue.mValue ? 1 : 0);
                     break;
                  }
                  case cTypeInt:
                  {
                     if (variantData.mArraySize != 1)
                        return false;
                        
                     if (variantData.mTypeDesc.mSizeInBytes == sizeof(BYTE))
                     {
                        variantData.mData.pushBack((BYTE)nameValue.mValue);
                        variantData.mTypeDesc.mAlignment = sizeof(BYTE);
                     }
                     else if (variantData.mTypeDesc.mSizeInBytes == sizeof(WORD))
                     {
                        variantData.mData.enlarge(sizeof(WORD));
                        *(WORD*)variantData.mData.getPtr() = (WORD)nameValue.mValue;
                        variantData.mTypeDesc.mAlignment = sizeof(WORD);
                     }
                     else if (variantData.mTypeDesc.mSizeInBytes == sizeof(DWORD))
                     {
                        variantData.mData.enlarge(sizeof(DWORD));
                        *(DWORD*)variantData.mData.getPtr() = (DWORD)nameValue.mValue;
                        variantData.mTypeDesc.mAlignment = sizeof(DWORD);
                     }
                     else 
                        return false;
                                          
                     break;
                  }
                  case cTypeFloat:
                  {
                     if (variantData.mArraySize != 1)
                        return false;
                        
                     if (totalDataSize != sizeof(DWORD))
                        return false;
                        
                     variantData.mData.enlarge(sizeof(DWORD));
                     *(DWORD*)variantData.mData.getPtr() = (DWORD)nameValue.mValue;
                     
                     break;
                  }
                  case cTypeString:
                  {
                     variantData.mArraySize = 1;
                     
                     if (totalDataSize > 5)
                        return false;
                        
                     BYTE buf[5];
                     
                     Utils::ClearObj(buf);
                     DWORD value = nameValue.mValue;
                     for (uint i = 0; i < 4; i++)
                     {
                        buf[i] = (BYTE)(value & 0xFF);
                        value >>= 8;
                     }
                     
                     variantData.mData.pushBack(buf, totalDataSize);
                     
                     variantData.mTypeDesc.mAlignment = sizeof(char);
                     
                     break;
                  }
                  default:
                     return false;
               }
            }
            
            variantData.mTypeDesc.mAlignment = Math::Max(variantData.mTypeDesc.mSizeInBytes, variantData.mTypeDesc.mAlignment);
         }
      }
      
      mUserSections.resize(header.mNumUserSections);
      for (uint i = 0; i < mUserSections.getSize(); i++)
      {
         mUserSections[i].mID = sections[i].mID;
         
         if (sections[i].mSize)
         {
            if ((sections[i].mOfs + sections[i].mSize) > srcLen)
               return false;
            
            mUserSections[i].mData.resize(0);
            mUserSections[i].mData.pushBack(pSrc + sections[i].mOfs, sections[i].mSize);
         }
      }
            
      return true;
   }
   
   bool BDocumentBuilder::dumpNodeToXML(BBuildNode* pNode, BTextDispatcher& textDispatcher, uint indent, bool writeTextDescAttributes) const
   {
      const BBuildNode&          node = *pNode;
      const BBuildNameValue&     nameValue = node.mNameValues[0];
      const BString&             name = nameValue.mName;
      const BBuildVariantData&   variantData = nameValue.mVariant;
      
      for (uint i = 0; i < indent; i++)
         textDispatcher.printf("  ");
      
      const uint numChildren = node.mChildren.getSize();
      const uint numAttributes = node.mNameValues.getSize() - 1;
      
      textDispatcher.printf(numAttributes ? "<%s " : "<%s", name.getPtr());
      
      for (uint i = 0; i < numAttributes; i++)
      {
         const BBuildNameValue&     attrNameValue = node.mNameValues[i + 1];
         const BString&             attrName = attrNameValue.mName;
         const BBuildVariantData&   attrVariantData = attrNameValue.mVariant;
         
         BString data;
         for (uint j = 0; j < attrVariantData.mArraySize; j++)
         {
            data.append(attrVariantData.asString(j));
            if (j < (attrVariantData.mArraySize - 1))
               data.append(",");
         }
         
         escapeXMLString(data);
         
         textDispatcher.printf("%s=\"%s\"", attrName.getPtr(), data.getPtr());
         
         if (i < (numAttributes - 1))
            textDispatcher.printf(" ");
      }
      
      if (writeTextDescAttributes)
      {
         const BTypeDesc& typeDesc = variantData.mTypeDesc;
         const char* p = "?";
         BString type;
         switch (typeDesc.mClass)
         {
            case cTypeNull:
            {
               p = "null";
               break;
            }
            case cTypeBool:
            {
               p = "bool";
               break;
            }
            case cTypeInt:
            {
               type.format(typeDesc.mIsUnsigned ? "uint%u" : "int%u", typeDesc.mSizeInBytes * 8U);
               p = type.getPtr();
               break;
            }
            case cTypeFloat: 
            {
               if (typeDesc.mSizeInBytes == 4)
                  p = "float";
               else
                  p = "double";
               break;
            }
            case cTypeString: 
            {
               if (typeDesc.mSizeInBytes == 2)
                  p = "ustring";
               else
                  p = "string";
               break;
            }
         }
         
         if (cTypeNull != typeDesc.mClass)
            textDispatcher.printf(" dataType=\"%s\"", p);
         if ((cTypeNull != typeDesc.mClass) && (typeDesc.mAlignment > typeDesc.mSizeInBytes))
            textDispatcher.printf(" alignment=\"%u\"", typeDesc.mAlignment);
         if (variantData.mArraySize > 1)
            textDispatcher.printf(" arraySize=\"%u\"", variantData.mArraySize);
      }            
      
      BString data;
      for (uint j = 0; j < variantData.mArraySize; j++)
      {
         data.append(variantData.asString(j));
         if (j < (variantData.mArraySize - 1))
            data.append(",");
      }
      
      escapeXMLString(data);

      if ((data.isEmpty()) && (!numChildren))
      {
         textDispatcher.printf(" />\n");
      }
      else if (!numChildren)
      {
         textDispatcher.printf(">%s</%s>\n", data.getPtr(), name.getPtr());
      }
      else
      {
         if (data.length())
            textDispatcher.printf(">%s\n", data.getPtr());
         else
            textDispatcher.printf(">\n");
         
         const bool doIndent = indent < 20;
         if (doIndent)
            indent++;

         for (uint childIndex = 0; childIndex < numChildren; childIndex++)
         {
            if (!dumpNodeToXML(node.mChildren[childIndex], textDispatcher, indent, writeTextDescAttributes))
               return false;
         }

         if (doIndent)
            indent--;
         
         for (uint i = 0; i < indent; i++)
            textDispatcher.printf("  ");
            
         textDispatcher.printf("</%s>\n", name.getPtr());
      }
            
      return true;
   }
   
   bool BDocumentBuilder::dumpToXML(BTextDispatcher& textDispatcher, bool writeTypeDescAttributes)
   {
      return dumpNodeToXML(mpRootNode, textDispatcher, 0, writeTypeDescAttributes);
   }
   
   BDocumentBuilder::BUserSectionIndex BDocumentBuilder::addUserSection(uint32 id, const void* pData, uint dataLen)
   {
      BDocumentBuilder::BUserSectionIndex sectionIndex = mUserSections.getSize();
      
      BUserSection& section = mUserSections.grow();
      section.mID = id;
      if (dataLen)
      {
         section.mData.resize(0);
         section.mData.pushBack(static_cast<const BYTE*>(pData), dataLen);
      }
      
      return sectionIndex;
   }
   
   BDocumentBuilder::BNodeIndex BDocumentBuilder::getNodeIndex(const BBuildNode* pNode) const
   {
      BNodeIndex index;
      bool success = mBuildNodes.getIndex(pNode, index);
      success;
      BDEBUG_ASSERT(success);
      return index;
   }
   
   bool BDocumentBuilder::isValidNode(const BBuildNode* pNode) const
   {
      uint index;
      if (!mBuildNodes.getIndex(pNode, index))
         return false;
      return mBuildNodes.isValidIndex(index) != FALSE;
   }

   bool BDocumentBuilder::deleteNode(BNode node)
   {
      if (!node.isValid())
         return false;

      BBuildNode* pBuildNode = node.mpNode;
      if (pBuildNode == mpRootNode)
         return false;

      if (!pBuildNode->mChildren.isEmpty())
      {
         BBuildNode::BBuildNodePtrArray children;
         pBuildNode->mChildren.swap(children);
         for (uint i = 0; i < children.getSize(); i++)
            if (!deleteNode(BNode(this, pBuildNode->mChildren[i])))
               return false;
      }

      if (!pBuildNode->mpParent)
      {
         BDEBUG_ASSERT(mpRootNode == pBuildNode);
         mpRootNode = NULL;
      }
      else
      {
         BBuildNode* pParentBuildNode = pBuildNode->mpParent;

         uint index;
         bool success = pParentBuildNode->mChildren.find(pBuildNode, index);
         if (success)
            pParentBuildNode->mChildren.removeIndex(index, true);
      }

      pBuildNode->clear();
      mBuildNodes.release(pBuildNode);
      return true;
   }
   
   bool BDocumentBuilder::BBuildVariantData::set(
      const void* pValueData, uint valueDataSizeInBytes, 
      const BTypeDesc& typeDesc, uint arraySize)
   {
      if (typeDesc.mSizeInBytes > sizeof(uint64))
         return false;
                       
      if (!pValueData)
      {
         if (valueDataSizeInBytes)
            return false;
      }
      
      if (valueDataSizeInBytes)
      {
         if (!arraySize)
            return false;
      }
      else
      {
         if (arraySize)
            return false;
      }
      
      switch (typeDesc.mClass)
      {
         case cTypeNull:
         {
            if (valueDataSizeInBytes)
               return false;
            break;
         }
         case cTypeString:
         {
            uint size = 0;
            
            bool success = false;
            if (typeDesc.mSizeInBytes == 1)
               success = BStringArrayHelpers::getSize(static_cast<const char*>(pValueData), valueDataSizeInBytes, size);
            else if (typeDesc.mSizeInBytes == 2)
               success = BStringArrayHelpers::getSize(static_cast<const wchar_t*>(pValueData), valueDataSizeInBytes, size);
            
            if (!success)
               return false;
            
            if (size != arraySize)
               return false;
         
            break;
         }
         case cTypeInt:
         case cTypeFloat:
         case cTypeBool:
         {
            if (typeDesc.mSizeInBytes * arraySize != valueDataSizeInBytes)
               return false;
            break;
         }
      }
   
      mTypeDesc = typeDesc;
      mArraySize = arraySize;
      if (valueDataSizeInBytes)
      {
         mData.resize(0);
         mData.pushBack(static_cast<const BYTE*>(pValueData), valueDataSizeInBytes);
      }
            
      return true;
   }      
         
   bool BDocumentBuilder::BBuildNameValue::set(
      const char* pName, 
      const void* pValueData, uint valueDataSizeInBytes, 
      const BTypeDesc& typeDesc, uint arraySize)
   {
      if (!pName)
         pName = "";
   
      if (!mVariant.set(pValueData, valueDataSizeInBytes, typeDesc, arraySize))
         return false;
            
      mName.set(pName);
      
      return true;
   }      
   
   BDocumentBuilder::BNode BDocumentBuilder::BNode::addChild(
      const char* pName, 
      const void* pValueData, uint valueDataSizeInBytes, 
      const BTypeDesc& typeDesc, uint arraySize)
   {  
      BDEBUG_ASSERT(isValid());
      BDEBUG_ASSERT(mpBuilder->isValidNode(mpNode));
      if (!isValid())
         return BNode();
                     
      uint nodeIndex;
      BBuildNode* pNode = mpBuilder->mBuildNodes.acquire(nodeIndex, true);
      pNode->clear();
      pNode->mpParent = mpNode;
      
      BBuildNameValue& nameValue = pNode->mNameValues.grow();
      
      bool success = nameValue.set(pName, pValueData, valueDataSizeInBytes, typeDesc, arraySize);
      if (!success)
      {
         pNode->clear();
         mpBuilder->mBuildNodes.release(nodeIndex);
         return BNode();
      }
            
      mpNode->mChildren.pushBack(pNode);
                        
      return BNode(mpBuilder, pNode);
   }      
   
   BDocumentBuilder::BNode BDocumentBuilder::BNode::addChild(const char* pName)
   {
      return addChild(pName, NULL, 0, gNullTypeDesc, 0);
   }
         
   BDocumentBuilder::BNode BDocumentBuilder::BNode::addChild(const char* pName, const char* pValue)
   {
      BDEBUG_ASSERT(pValue);
      const uint sizeInBytes = strlen(pValue) + 1;
      return addChild(pName, pValue, sizeInBytes, gStringTypeDesc, 1);
   }
   
   BDocumentBuilder::BNode BDocumentBuilder::BNode::addChild(const char* pName, const wchar_t* pValue)
   {
      BDEBUG_ASSERT(pValue);
      const uint sizeInBytes = (wcslen(pValue) + 1) * sizeof(wchar_t);
      return addChild(pName, pValue, sizeInBytes, gUStringTypeDesc, 1);
   }
   
   BDocumentBuilder::BNode BDocumentBuilder::BNode::addChild(const char* pName, const void* pValueData, uint dataLen)
   {
      return addChild(pName, pValueData, dataLen, gUInt8TypeDesc, dataLen);
   }
   
   BDocumentBuilder::BNode BDocumentBuilder::BNode::addChild(const char* pName, const BDynamicArray<BString>& strings)
   {
      BByteArray buf;
      for (uint i = 0; i < strings.getSize(); i++)
         buf.pushBack((const BYTE*)strings[i].getPtr(), strings[i].length() + 1);
      return addChild(pName, buf.getSize() ? buf.getPtr() : NULL, buf.getSizeInBytes(), gStringTypeDesc, strings.getSize());
   }

   BDocumentBuilder::BValue BDocumentBuilder::BNode::addAttribute(
      const char* pName, 
      const void* pValueData, uint valueDataSizeInBytes, 
      const BTypeDesc& typeDesc, uint arraySize)
   {
      BDEBUG_ASSERT(isValid());
      BDEBUG_ASSERT(mpBuilder->isValidNode(mpNode));
      if (!isValid())
         return BValue();
      
      const uint nameValueIndex = mpNode->mNameValues.getSize();
      
      BBuildNameValue& nameValue = mpNode->mNameValues.grow();
      if (!nameValue.set(pName, pValueData, valueDataSizeInBytes, typeDesc, arraySize))
      {
         mpNode->mNameValues.popBack();
         return BValue();
      }
      
      return BValue(mpNode, nameValueIndex);
   }

   BDocumentBuilder::BValue BDocumentBuilder::BNode::addAttribute(const char* pName)
   {
      return addAttribute(pName, NULL, 0, gNullTypeDesc, 0);
   }
   
   BDocumentBuilder::BValue BDocumentBuilder::BNode::addAttribute(const char* pName, const char* pValue)
   {
      BDEBUG_ASSERT(pValue);
      const uint sizeInBytes = strlen(pValue) + 1;
      return addAttribute(pName, pValue, sizeInBytes, gStringTypeDesc, 1);
   }
   
   BDocumentBuilder::BValue BDocumentBuilder::BNode::addAttribute(const char* pName, const wchar_t* pValue)
   {
      BDEBUG_ASSERT(pValue);
      const uint sizeInBytes = (wcslen(pValue) + 1) * sizeof(wchar_t);
      return addAttribute(pName, pValue, sizeInBytes, gUStringTypeDesc, 1);
   }
   
   BDocumentBuilder::BValue BDocumentBuilder::BNode::addAttribute(const char* pName, const void* pValueData, uint dataLen)
   {
      return addAttribute(pName, pValueData, dataLen, gUInt8TypeDesc, dataLen);
   }
   
   BDocumentBuilder::BValue BDocumentBuilder::BNode::addAttribute(const char* pName, const BDynamicArray<BString>& strings)
   {
      BByteArray buf;
      for (uint i = 0; i < strings.getSize(); i++)
         buf.pushBack((const BYTE*)strings[i].getPtr(), strings[i].length() + 1);
      return addAttribute(pName, buf.getSize() ? buf.getPtr() : NULL, buf.getSizeInBytes(), gStringTypeDesc, strings.getSize());
   }
   
   bool BDocumentBuilder::BNode::deleteAttribute(BAttributeIndex attributeIndex)
   {
      if (!isValid())
         return false;

      BDEBUG_ASSERT((attributeIndex + 1) < mpNode->mNameValues.getSize());

      return mpNode->mNameValues.removeIndex(attributeIndex + 1, true);
   }
            
   // BDocumentBuilder::BValue
   
   const char* BDocumentBuilder::BValue::getName() const
   {
      BDEBUG_ASSERT(mpNode);
      if (!mpNode)
         return "";
      return mpNode->mNameValues[mNameValueIndex].mName.getPtr();
   }

   const BBinaryDataTree::BTypeDesc& BDocumentBuilder::BValue::getTypeDesc() const
   {
      BDEBUG_ASSERT(mpNode);
      return mpNode->mNameValues[mNameValueIndex].mVariant.mTypeDesc;
   }

   uint BDocumentBuilder::BValue::getDataSizeInBytes() const
   {
      BDEBUG_ASSERT(mpNode);
      if (!mpNode)
         return 0;
      return mpNode->mNameValues[mNameValueIndex].mVariant.mData.getSizeInBytes();
   }
   
   const void* BDocumentBuilder::BValue::getDataPtr() const
   {
      BDEBUG_ASSERT(mpNode);
      if (!mpNode)
         return NULL;
      const BByteArray& data = mpNode->mNameValues[mNameValueIndex].mVariant.mData;
      if (data.isEmpty())
         return NULL;
      return data.getPtr();
   }

   uint BDocumentBuilder::BValue::getArraySize() const
   {
      BDEBUG_ASSERT(mpNode);
      if (!mpNode)
         return 0;
      return mpNode->mNameValues[mNameValueIndex].mVariant.mArraySize;
   }
   
   uint64 BDocumentBuilder::BValue::asUnsigned(uint arrayIndex) const
   {
      BDEBUG_ASSERT(mpNode);
      if (!mpNode)
         return 0;
      const BBuildVariantData& variantData = mpNode->mNameValues[mNameValueIndex].mVariant;
      return variantData.asUnsigned(arrayIndex);
   }
   
   int64 BDocumentBuilder::BValue::asSigned(uint arrayIndex) const
   {
      BDEBUG_ASSERT(mpNode);
      if (!mpNode)
         return 0;
      const BBuildVariantData& variantData = mpNode->mNameValues[mNameValueIndex].mVariant;
      return variantData.asSigned(arrayIndex);
   }
   
   double BDocumentBuilder::BValue::asDouble(uint arrayIndex) const
   {
      BDEBUG_ASSERT(mpNode);
      if (!mpNode)
         return 0;
      const BBuildVariantData& variantData = mpNode->mNameValues[mNameValueIndex].mVariant;
      return variantData.asDouble(arrayIndex);
   }
   
   float BDocumentBuilder::BValue::asFloat(uint arrayIndex) const
   {
      BDEBUG_ASSERT(mpNode);
      if (!mpNode)
         return 0;
      const BBuildVariantData& variantData = mpNode->mNameValues[mNameValueIndex].mVariant;
      return variantData.asFloat(arrayIndex);
   }
   
   bool BDocumentBuilder::BValue::asBool(uint arrayIndex) const
   {
      BDEBUG_ASSERT(mpNode);
      if (!mpNode)
         return 0;
      const BBuildVariantData& variantData = mpNode->mNameValues[mNameValueIndex].mVariant;
      return variantData.asBool(arrayIndex);
   }
   
   const char* BDocumentBuilder::BValue::asString(uint arrayIndex, bool convertToString) const
   {
      BDEBUG_ASSERT(mpNode);
      if (!mpNode)
         return "";
      const BBuildVariantData& variantData = mpNode->mNameValues[mNameValueIndex].mVariant;
      return variantData.asString(arrayIndex, convertToString);
   }
   
   const wchar_t* BDocumentBuilder::BValue::asUString(uint arrayIndex, bool convertToString) const
   {
      BDEBUG_ASSERT(mpNode);
      if (!mpNode)
         return L"";
      const BBuildVariantData& variantData = mpNode->mNameValues[mNameValueIndex].mVariant;
      return variantData.asUString(arrayIndex, convertToString);
   }
   
   bool BDocumentBuilder::BValue::setName(const char* pName)
   {
      BDEBUG_ASSERT(mpNode);
      if (!mpNode)
         return false;
      BBuildNameValue& buildNameValue = mpNode->mNameValues[mNameValueIndex];
      buildNameValue.mName.set(pName ? pName : "");
      return true;
   }
   
   bool BDocumentBuilder::BValue::set(const char* pName, const void* pValueData, uint valueDataSizeInBytes, const BTypeDesc& typeDesc, uint arraySize)
   {
      BDEBUG_ASSERT(mpNode);
      if (!mpNode)
         return false;
      BBuildNameValue& buildNameValue = mpNode->mNameValues[mNameValueIndex];
      return buildNameValue.set(pName, pValueData, valueDataSizeInBytes, typeDesc, arraySize);
   }
   
   bool BDocumentBuilder::BValue::set(const void* pValueData, uint valueDataSizeInBytes, const BTypeDesc& typeDesc, uint arraySize)
   {
      BDEBUG_ASSERT(mpNode);   
      if (!mpNode)
         return false;
      BBuildNameValue& buildNameValue = mpNode->mNameValues[mNameValueIndex];
      return buildNameValue.mVariant.set(pValueData, valueDataSizeInBytes, typeDesc, arraySize);
   }
   
   bool BDocumentBuilder::BValue::set(const char* pValue)
   {
      BDEBUG_ASSERT(mpNode);
      if (!mpNode)
         return false;
      BDEBUG_ASSERT(pValue);
      const uint sizeInBytes = strlen(pValue) + 1;
      return set(pValue, sizeInBytes, gStringTypeDesc, 1);
   }
   
   bool BDocumentBuilder::BValue::set(const wchar_t* pValue)
   {
      BDEBUG_ASSERT(mpNode);
      if (!mpNode)
         return false;
      BDEBUG_ASSERT(pValue);
      const uint sizeInBytes = (wcslen(pValue) + 1) * sizeof(wchar_t);
      return set(pValue, sizeInBytes, gUStringTypeDesc, 1);
   }
   
   bool BDocumentBuilder::BValue::set(const void* pValueData, uint dataLen)
   {
      return set(pValueData, dataLen, gUInt8TypeDesc, dataLen);
   }
   
   bool BDocumentBuilder::BValue::set(const BDynamicArray<BString>& strings)
   {
      BByteArray buf;
      for (uint i = 0; i < strings.getSize(); i++)
         buf.pushBack((const BYTE*)strings[i].getPtr(), strings[i].length() + 1);
      return set(buf.getSize() ? buf.getPtr() : NULL, buf.getSizeInBytes(), gStringTypeDesc, strings.getSize());
   }
   
   // BDocumentBuilder::BBuildVariantData

   uint64 BDocumentBuilder::BBuildVariantData::asUnsigned(uint arrayIndex) const
   {
      BDEBUG_ASSERT(arrayIndex < mArraySize);
      
      uint64 uvalue = 0;
                  
      switch (mTypeDesc.mClass)
      {
         case cTypeInt:
         {
            const void* pData = mData.getPtr() + mTypeDesc.mSizeInBytes * arrayIndex;
            
            if (mTypeDesc.mIsUnsigned)
            {
               switch (mTypeDesc.mSizeInBytes)
               {
                  case 1: uvalue = *(const uint8*)pData; break;
                  case 2: uvalue = *(const uint16*)pData; break;
                  case 4: uvalue = *(const uint32*)pData; break;
                  case 8: uvalue = *(const uint64*)pData; break;
               }
            }
            else
            {
               int64 value = 0;
               switch (mTypeDesc.mSizeInBytes)
               {
                  case 1: value = *(const int8*)pData; break;
                  case 2: value = *(const int16*)pData; break;
                  case 4: value = *(const int32*)pData; break;
                  case 8: value = *(const int64*)pData; break;
               }
               uvalue = (value < 0) ? 0 : value;
            }               
            break;
         }
         case cTypeFloat:
         {
            const void* pData = mData.getPtr() + mTypeDesc.mSizeInBytes * arrayIndex;
            double fvalue;
            if (mTypeDesc.mSizeInBytes == 4)
               fvalue = *(const float*)pData;
            else
               fvalue = *(const double*)pData;
            if ((fvalue >= 0.0f) && (fvalue <= UINT64_MAX))
               uvalue = (uint64)fvalue;
            break;               
         }
         case cTypeBool:
         {
            const void* pData = mData.getPtr() + mTypeDesc.mSizeInBytes * arrayIndex;
            uvalue = *(const bool*)pData;
            break;
         }
         case cTypeString:
         {
            if (mTypeDesc.mSizeInBytes == 1)
            {
               const char* pStr = BStringArrayHelpers::getString((const char*)mData.getPtr(), mData.getSizeInBytes(), arrayIndex);
               if (pStr)
               {
                  int64 value = _atoi64(pStr);
                  uvalue = (value < 0) ? 0 : value;
               }
            }
            else
            {
               const wchar_t* pStr = BStringArrayHelpers::getString((const wchar_t*)mData.getPtr(), mData.getSizeInBytes(), arrayIndex);
               if (pStr)
               {
                  int64 value = _wtoi64(pStr);
                  uvalue = (value < 0) ? 0 : value;
               }
            }
            break;
         }
      }
      return uvalue;
   }
   
   int64 BDocumentBuilder::BBuildVariantData::asSigned(uint arrayIndex) const
   {
      BDEBUG_ASSERT(arrayIndex < mArraySize);
      
      int64 value = 0;
                  
      switch (mTypeDesc.mClass)
      {
         case cTypeInt:
         {
            const void* pData = mData.getPtr() + mTypeDesc.mSizeInBytes * arrayIndex;
            
            if (mTypeDesc.mIsUnsigned)
            {
               uint64 uvalue = 0;
               switch (mTypeDesc.mSizeInBytes)
               {
                  case 1: uvalue = *(const uint8*)pData; break;
                  case 2: uvalue = *(const uint16*)pData; break;
                  case 4: uvalue = *(const uint32*)pData; break;
                  case 8: uvalue = *(const uint64*)pData; break;
               }
               if (uvalue <= INT64_MAX)
                  value = uvalue;
            }
            else
            {
               switch (mTypeDesc.mSizeInBytes)
               {
                  case 1: value = *(const int8*)pData; break;
                  case 2: value = *(const int16*)pData; break;
                  case 4: value = *(const int32*)pData; break;
                  case 8: value = *(const int64*)pData; break;
               }
            }               
            break;
         }
         case cTypeFloat:
         {
            const void* pData = mData.getPtr() + mTypeDesc.mSizeInBytes * arrayIndex;
            double fvalue;
            if (mTypeDesc.mSizeInBytes == 4)
               fvalue = *(const float*)pData;
            else
               fvalue = *(const double*)pData;
            if ((fvalue >= INT64_MIN) && (fvalue <= INT64_MAX))
               value = (int64)fvalue;
            break;               
         }
         case cTypeBool:
         {
            const void* pData = mData.getPtr() + mTypeDesc.mSizeInBytes * arrayIndex;
            value = *(const bool*)pData;
            break;
         }
         case cTypeString:
         {
            if (mTypeDesc.mSizeInBytes == 1)
            {
               const char* pStr = BStringArrayHelpers::getString((const char*)mData.getPtr(), mData.getSizeInBytes(), arrayIndex);
               if (pStr)
                  value = _atoi64(pStr);
            }
            else
            {
               const wchar_t* pStr = BStringArrayHelpers::getString((const wchar_t*)mData.getPtr(), mData.getSizeInBytes(), arrayIndex);
               if (pStr)
                  value = _wtoi64(pStr);
            }
            break;
         }
      }
      return value;
   }
   
   double BDocumentBuilder::BBuildVariantData::asDouble(uint arrayIndex) const
   {
      BDEBUG_ASSERT(arrayIndex < mArraySize);
      
      double fvalue = 0.0f;
                  
      switch (mTypeDesc.mClass)
      {
         case cTypeInt:
         {
            const void* pData = mData.getPtr() + mTypeDesc.mSizeInBytes * arrayIndex;
            
            if (mTypeDesc.mIsUnsigned)
            {
               switch (mTypeDesc.mSizeInBytes)
               {
                  case 1: fvalue = (double)*(const uint8*)pData; break;
                  case 2: fvalue = (double)*(const uint16*)pData; break;
                  case 4: fvalue = (double)*(const uint32*)pData; break;
                  case 8: fvalue = (double)*(const uint64*)pData; break;
               }
            }
            else
            {
               switch (mTypeDesc.mSizeInBytes)
               {
                  case 1: fvalue = (double)*(const int8*)pData; break;
                  case 2: fvalue = (double)*(const int16*)pData; break;
                  case 4: fvalue = (double)*(const int32*)pData; break;
                  case 8: fvalue = (double)*(const int64*)pData; break;
               }
            }               
            break;
         }
         case cTypeFloat:
         {
            const void* pData = mData.getPtr() + mTypeDesc.mSizeInBytes * arrayIndex;
            if (mTypeDesc.mSizeInBytes == 4)
               fvalue = *(const float*)pData;
            else
               fvalue = *(const double*)pData;
            break;               
         }
         case cTypeBool:
         {
            const void* pData = mData.getPtr() + mTypeDesc.mSizeInBytes * arrayIndex;
            fvalue = *(const bool*)pData;
            break;
         }
         case cTypeString:
         {
            if (mTypeDesc.mSizeInBytes == 1)
            {
               const char* pStr = BStringArrayHelpers::getString((const char*)mData.getPtr(), mData.getSizeInBytes(), arrayIndex);
               if (pStr)
               {
                  _CRT_DOUBLE d;
                  int result = _atodbl(&d, (char*)pStr);
                  if (!result)
                     fvalue = d.x;
               }
            }
            else
            {
               const wchar_t* pStr = BStringArrayHelpers::getString((const wchar_t*)mData.getPtr(), mData.getSizeInBytes(), arrayIndex);
               if (pStr)
                  fvalue = _wtof(pStr);
            }
            break;
         }
      }
      return fvalue;
   }
   
   float BDocumentBuilder::BBuildVariantData::asFloat(uint arrayIndex) const
   {
      if ((mTypeDesc.mClass == cTypeFloat) && (mTypeDesc.mSizeInBytes == 4))
      {
         BDEBUG_ASSERT(arrayIndex < mArraySize);
         
         const void* pData = mData.getPtr() + mTypeDesc.mSizeInBytes * arrayIndex;
         return *(const float*)pData;
      }
      else
         return (float)asDouble(arrayIndex);
   }
   
   bool BDocumentBuilder::BBuildVariantData::asBool(uint arrayIndex) const
   {
      return asSigned(arrayIndex) != 0;
   }
   
   const char* BDocumentBuilder::BBuildVariantData::asString(uint arrayIndex, bool convertToString) const
   {
      BDEBUG_ASSERT(arrayIndex < mArraySize);
      
      const char* pStr = NULL;
                  
      switch (mTypeDesc.mClass)
      {
         case cTypeNull:
         {
            pStr = "";
            break;
         }
         case cTypeInt:
         {
            if (convertToString)
            {
               const void* pData = mData.getPtr() + mTypeDesc.mSizeInBytes * arrayIndex;
               
               if (mTypeDesc.mIsUnsigned)
               {
                  uint64 uvalue = 0;
                  switch (mTypeDesc.mSizeInBytes)
                  {
                     case 1: uvalue = *(const uint8*)pData; break;
                     case 2: uvalue = *(const uint16*)pData; break;
                     case 4: uvalue = *(const uint32*)pData; break;
                     case 8: uvalue = *(const uint64*)pData; break;
                  }
                  mString.format("%I64u", uvalue);
               }
               else
               {
                  int64 value = 0;
                  switch (mTypeDesc.mSizeInBytes)
                  {
                     case 1: value = *(const int8*)pData; break;
                     case 2: value = *(const int16*)pData; break;
                     case 4: value = *(const int32*)pData; break;
                     case 8: value = *(const int64*)pData; break;
                  }
                  mString.format("%I64i", value);
               }  
               pStr = mString.getPtr();             
            }               
            break;
         }
         case cTypeFloat:
         {
            if (convertToString)
            {
               const void* pData = mData.getPtr() + mTypeDesc.mSizeInBytes * arrayIndex;
               double fvalue;
               if (mTypeDesc.mSizeInBytes == 4)
                  fvalue = *(const float*)pData;
               else
                  fvalue = *(const double*)pData;
               if (!fvalue)
                  mString.format("%1.1f", fvalue);
               else
                  mString.format("%f", fvalue);
               pStr = mString.getPtr();
            }               
            break;               
         }
         case cTypeBool:
         {
            if (convertToString)
            {
               const void* pData = mData.getPtr() + mTypeDesc.mSizeInBytes * arrayIndex;
               mString.format("%i", *(const bool*)pData);
               pStr = mString.getPtr();
            }               
            break;
         }
         case cTypeString:
         {
            if (mTypeDesc.mSizeInBytes == 1)
            {
               pStr = BStringArrayHelpers::getString((const char*)mData.getPtr(), mData.getSizeInBytes(), arrayIndex);
            }
            else
            {
               if (convertToString)
               {
                  mString.set(BStringArrayHelpers::getString((const wchar_t*)mData.getPtr(), mData.getSizeInBytes(), arrayIndex));
                  pStr = mString.getPtr();
               }
            }
            break;
         }
      }
      return pStr;
   }
   
   const wchar_t* BDocumentBuilder::BBuildVariantData::asUString(uint arrayIndex, bool convertToString) const
   {
      BDEBUG_ASSERT(arrayIndex < mArraySize);
      
      const wchar_t* pStr = NULL;
                  
      switch (mTypeDesc.mClass)
      {
         case cTypeNull:
         {
            pStr = L"";
            break;
         }
         case cTypeInt:
         {
            if (convertToString)
            {
               const void* pData = mData.getPtr() + mTypeDesc.mSizeInBytes * arrayIndex;
               
               if (mTypeDesc.mIsUnsigned)
               {
                  uint64 uvalue = 0;
                  switch (mTypeDesc.mSizeInBytes)
                  {
                     case 1: uvalue = *(const uint8*)pData; break;
                     case 2: uvalue = *(const uint16*)pData; break;
                     case 4: uvalue = *(const uint32*)pData; break;
                     case 8: uvalue = *(const uint64*)pData; break;
                  }
                  mUString.format(L"%I64u", uvalue);
               }
               else
               {
                  int64 value = 0;
                  switch (mTypeDesc.mSizeInBytes)
                  {
                     case 1: value = *(const int8*)pData; break;
                     case 2: value = *(const int16*)pData; break;
                     case 4: value = *(const int32*)pData; break;
                     case 8: value = *(const int64*)pData; break;
                  }
                  mUString.format(L"%I64i", value);
               }  
               pStr = mUString.getPtr();             
            }               
            break;
         }
         case cTypeFloat:
         {
            if (convertToString)
            {
               const void* pData = mData.getPtr() + mTypeDesc.mSizeInBytes * arrayIndex;
               double fvalue;
               if (mTypeDesc.mSizeInBytes == 4)
                  fvalue = *(const float*)pData;
               else
                  fvalue = *(const double*)pData;
               if (!fvalue)
                  mUString.format(L"%1.1f", fvalue);
               else
                  mUString.format(L"%f", fvalue);
               pStr = mUString.getPtr();
            }               
            break;               
         }
         case cTypeBool:
         {
            if (convertToString)
            {
               const void* pData = mData.getPtr() + mTypeDesc.mSizeInBytes * arrayIndex;
               mUString.format(L"%i", *(const bool*)pData);
               pStr = mUString.getPtr();
            }
            break;
         }
         case cTypeString:
         {
            if (mTypeDesc.mSizeInBytes == 1)
            {
               if (convertToString)
               {
                  mUString.set(BStringArrayHelpers::getString((const char*)mData.getPtr(), mData.getSizeInBytes(), arrayIndex));
                  pStr = mUString.getPtr();
               }
            }
            else
            {
               pStr = BStringArrayHelpers::getString((const wchar_t*)mData.getPtr(), mData.getSizeInBytes(), arrayIndex);
            }
            break;
         }
      }
      return pStr;
   }
   
   bool BDocumentBuilder::BNode::getNameValueMap(BNameValueMap& nameValueMap, bool asAttributes) const
   {
      BDEBUG_ASSERT(isValid());
      
      const uint num = asAttributes ? getNumAttributes() : getNumChildren();
      for (uint i = 0; i < num; i++)
      {
         BValue value(asAttributes ? getAttribute(i) : getChild(i));
         
         const char* pName = value.getName();
         
         const BTypeDesc& typeDesc = value.getTypeDesc();
         switch (typeDesc.mClass)
         {
            case cTypeNull:
            {
               break;
            }
            case cTypeBool:
            {
               nameValueMap.set(pName, (bool)value);
               break;
            }
            case cTypeInt:
            {
               if (typeDesc.mIsUnsigned)
               {
                  if (typeDesc.mSizeInBytes <= sizeof(uint8))
                     nameValueMap.set(pName, (uint8)value);
                  else if (typeDesc.mSizeInBytes <= sizeof(uint16))
                     nameValueMap.set(pName, (uint16)value);
                  else if (typeDesc.mSizeInBytes <= sizeof(uint32))
                     nameValueMap.set(pName, (uint32)value);
                  else
                     nameValueMap.set(pName, (uint64)value);
               }
               else
               {
                  if (typeDesc.mSizeInBytes <= sizeof(int8))
                     nameValueMap.set(pName, (int8)value);
                  else if (typeDesc.mSizeInBytes <= sizeof(int16))
                     nameValueMap.set(pName, (int16)value);
                  else if (typeDesc.mSizeInBytes <= sizeof(int32))
                     nameValueMap.set(pName, (int32)value);
                  else
                     nameValueMap.set(pName, (int64)value);
               }
               break;
            }
            case cTypeFloat:
            {
               if (typeDesc.mSizeInBytes == sizeof(float))
                  nameValueMap.set(pName, (float)value);
               else
                  nameValueMap.set(pName, (double)value);
               break;
            }
            case cTypeString:
            {
               // Name value maps don't support UTF-16 yet!
               nameValueMap.set(pName, (const char*)value);
               break;
            }
            default:
            {
               BDEBUG_ASSERT(0);
               return false;
            }
         }
      }
      
      return true;
   }
   
   bool BDocumentBuilder::BNode::addNameValueMap(const BNameValueMap& nameValueMap, bool asAttributes)
   {
      BDEBUG_ASSERT(isValid());

      for (uint i = 0; i < nameValueMap.getNumNameValues(); i++)
      {
         const BString& name = nameValueMap.getName(i);
         const BNameValueMap::eDataType dataType = nameValueMap.getType(i);

         const void* pData = nameValueMap.getDataPtr(i);
         uint dataSize = nameValueMap.getDataSizeInBytes(i);
         BTypeDesc* pTypeDesc = NULL;
         const uint arraySize = 1;

         switch (dataType)
         {
            case BNameValueMap::cTypeString: pTypeDesc = &gStringTypeDesc; break;
            case BNameValueMap::cTypeFloat: pTypeDesc = &gFloatTypeDesc; break;
            case BNameValueMap::cTypeDouble: pTypeDesc = &gDoubleTypeDesc; break;
            case BNameValueMap::cTypeInt8: pTypeDesc = &gInt8TypeDesc; break;
            case BNameValueMap::cTypeUInt8: pTypeDesc = &gUInt8TypeDesc; break;
            case BNameValueMap::cTypeInt16: pTypeDesc = &gInt16TypeDesc; break;
            case BNameValueMap::cTypeUInt16: pTypeDesc = &gUInt16TypeDesc; break;
            case BNameValueMap::cTypeInt32: pTypeDesc = &gInt32TypeDesc; break;
            case BNameValueMap::cTypeUInt32: pTypeDesc = &gUInt32TypeDesc; break;
            case BNameValueMap::cTypeInt64: pTypeDesc = &gInt64TypeDesc; break;
            case BNameValueMap::cTypeUInt64: pTypeDesc = &gUInt64TypeDesc; break;
            case BNameValueMap::cTypeBool: pTypeDesc = &gBoolTypeDesc; break;
            default:
            {
               BDEBUG_ASSERT(0);
               return false;
            }
         }

         BDEBUG_ASSERT(pTypeDesc);

         if (asAttributes)
         {
            BValue value;
            if (findAttribute(name, value))
               value.set(name, pData, dataSize, *pTypeDesc, arraySize);
            else
               addAttribute(name, pData, dataSize, *pTypeDesc, arraySize);
         }
         else
         {
            BNode node;
            if (findChild(name, node))
               node.set(name, pData, dataSize, *pTypeDesc, arraySize);
            else
               addChild(name, pData, dataSize, *pTypeDesc, arraySize);
         }
      }
      
      return true;
   }
}


