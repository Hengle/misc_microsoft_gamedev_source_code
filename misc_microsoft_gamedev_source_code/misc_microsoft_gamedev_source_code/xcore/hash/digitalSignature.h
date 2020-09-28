//==============================================================================
//
// digitalSignature.h
// Hash tree digital signature
//
// Copyright (c) 2007, Ensemble Studios
//
//==============================================================================
#pragma once
#include "hash\bsha1.h"

//==============================================================================
// class BDigitalSignature
//==============================================================================
class BDigitalSignature
{
   BDigitalSignature(const BDigitalSignature&);
   BDigitalSignature& operator= (const BDigitalSignature&);
   
public:
   BDigitalSignature();

   // The secret key data is only a function of the secret key phrase, and is itself unencrypted.
   bool createKeys(BSHA1& publicKey, BStream& secretKeyData, const BString& secretKeyPhrase, uint signatureCount = 1024);

   bool signMessage(BStream& signatureData, const BSHA1& messageDigest, BStream& secretKeyData);

   bool verifyMessage(BStream& signatureData, const BSHA1& publicKey, const BSHA1& messageDigest);
   
private:   
   enum { cDigitalSignatureSig = 0xAAC94350 };
   
   uchar computeChecksum(const BSHA1& messageDigest);
   void computeLeafNode(BSHA1& preImage, BSHA1& leafHash, uint leafIndex, const BString& secretKeyPhrase);
   void computeInternalNode(BSHA1& nodeHash, const BSHA1& leftHash, const BSHA1& rightHash, uint nodeIndex);
   void computeNodeTraversal(UIntArray& nodeIndices, uint leafNodeIndex);
   bool init(const BString& secretKeyPhrase, uint signatureCount);
   bool init(BStream& secretKeyData);
   void clear(void);
   
   enum { cRootNodeIndex = 1 };
   
   static uint getChildNode(uint nodeIndex, uint siblingIndex) { return nodeIndex * 2 + siblingIndex; }
   static uint getParentNode(uint nodeIndex) { BDEBUG_ASSERT(nodeIndex > cRootNodeIndex); return nodeIndex >> 1; }
   static uint getSiblingNode(uint nodeIndex) { BDEBUG_ASSERT(nodeIndex > cRootNodeIndex); if (nodeIndex & 1) return nodeIndex - 1; else return nodeIndex + 1; }
   
   typedef BDynamicArray<BSHA1> HashArray;
   
   uint mTotalLeafNodes;
   uint mFirstLeafNodeIndex;
   uint mTotalNodes;
   uint mTreeHeight;
   
   HashArray mLeafPreImages;
   HashArray mTreeNodes;
};

