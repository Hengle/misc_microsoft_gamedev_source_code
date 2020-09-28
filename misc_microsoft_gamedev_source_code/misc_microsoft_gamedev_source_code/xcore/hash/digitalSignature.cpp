//==============================================================================
//
// digitalSignature.cpp
// Hash tree digital signature
//
// Copyright (c) 2007, Ensemble Studios
// 
// See:
// http://en.wikipedia.org/wiki/Merkle_tree
// http://en.wikipedia.org/wiki/Lamport_signature
// http://szydlo.com/szydlo-loglog.pdf
//
// rg [3/15/07]:
// This is a pretty simple implementation, but should work well enough for our
// needs. The entire Hash tree is computed in memory for simplicity, because we don't
// really care how long it takes to compute keys or signatures, or how large the secret key file is.
// Instead of tracking the # of used leaf nodes as messages are signed, and never using them 
// again, this implementation starts authentication beginning at a psuedo-random leaf node based off 
// a very long-period PRNG seeded off the message digest. This means, the more messages we 
// sign with the same keys, the more leaf node keys we expose, and the lower the 
// overall quality of the signature. I don't feel this is a big deal because we don't 
// intend on signing many archives (a few dozen at most), and this is intended to slow 
// down hackers, not stop them. Also, we can ensure the number of leaf nodes is sufficiently 
// high relative to the total number of bits we'll ever sign. 
// The one way hash function is SHA1.
//
//==============================================================================
#include "xcore.h"
#include "digitalSignature.h"
#include "math\random.h"
#include "containers\unifier.h"
#include "containers\hashMap.h"

typedef Unifier<uint>         UIntUnifier;
typedef BHashMap<uint, BSHA1> BNodeHashMap;

//==============================================================================
// BDigitalSignature::computeChecksum
//==============================================================================
uchar BDigitalSignature::computeChecksum(const BSHA1& messageDigest)
{
   uchar checkSum = 0;
   
   for (uint i = 0; i < 20; i++)
   {
      uint c = messageDigest[i];
      
      for (uint j = 0; j < 8; j++)
      {
         if (c & 1)
            checkSum++;
         
         c >>= 1;
      }
   }   
   
   return checkSum;
}

//==============================================================================
// BDigitalSignature::computeLeafNode
//==============================================================================
void BDigitalSignature::computeLeafNode(BSHA1& preImage, BSHA1& leafHash, uint leafIndex, const BString& secretKeyPhrase)
{
   BSHA1Gen sha1Gen;
   
   sha1Gen.update(0);
   sha1Gen.update(secretKeyPhrase.getPtr(), secretKeyPhrase.length());
   sha1Gen.update((uchar)((secretKeyPhrase.length() >> 24) & 0xFF));
   sha1Gen.update((uchar)((secretKeyPhrase.length() >> 16) & 0xFF));
   sha1Gen.update((uchar)((secretKeyPhrase.length() >> 8) & 0xFF));
   sha1Gen.update((uchar)(secretKeyPhrase.length() & 0xFF));
      
   sha1Gen.update((uchar)((leafIndex >> 24) & 0xFF));
   sha1Gen.update((uchar)((leafIndex >> 16) & 0xFF));
   sha1Gen.update((uchar)((leafIndex >> 8) & 0xFF));
   sha1Gen.update((uchar)(leafIndex & 0xFF));
         
   preImage = sha1Gen.finalize();
   
   sha1Gen.clear();
   sha1Gen.update(preImage.getBuf(), preImage.size());
   leafHash = sha1Gen.finalize();
}

//==============================================================================
// BDigitalSignature::computeInternalNode
//==============================================================================
void BDigitalSignature::computeInternalNode(BSHA1& nodeHash, const BSHA1& leftHash, const BSHA1& rightHash, uint nodeIndex)
{
   BSHA1Gen sha1Gen;
   
   sha1Gen.update(1);
   sha1Gen.update((uchar)((nodeIndex >> 24) & 0xFF));
   sha1Gen.update((uchar)((nodeIndex >> 16) & 0xFF));
   sha1Gen.update((uchar)((nodeIndex >> 8) & 0xFF));
   sha1Gen.update((uchar)(nodeIndex & 0xFF));
   
   sha1Gen.update(leftHash.getBuf(), leftHash.size());
   sha1Gen.update(rightHash.getBuf(), rightHash.size());
   
   nodeHash = sha1Gen.finalize();
}

//==============================================================================
// BDigitalSignature::BDigitalSignature
//==============================================================================
BDigitalSignature::BDigitalSignature() :
   mTotalLeafNodes(0),
   mFirstLeafNodeIndex(0),
   mTotalNodes(0),
   mTreeHeight(0)
{
}

//==============================================================================
// BDigitalSignature::computeNodeTraversal
//==============================================================================
void BDigitalSignature::computeNodeTraversal(UIntArray& nodeIndices, uint leafNodeIndex)
{
   nodeIndices.resize(0);
   
   uint nodeIndex = leafNodeIndex;
   
   for ( ; ; )
   {
      nodeIndices.pushBack(nodeIndex);
    
      if (nodeIndex == cRootNodeIndex)
         break;
         
      nodeIndex >>= 1;
   }
}

//==============================================================================
// BDigitalSignature::clear
//==============================================================================
void BDigitalSignature::clear(void)
{
   mLeafPreImages.setAll(BSHA1());
   mLeafPreImages.clear();

   mTreeNodes.setAll(BSHA1());
   mTreeNodes.clear();
   
   mTotalLeafNodes = 0;
   mFirstLeafNodeIndex = 0;
   mTotalNodes = 0;
   mTreeHeight = 0;
}

//==============================================================================
// BDigitalSignature::init
//==============================================================================
bool BDigitalSignature::init(const BString& secretKeyPhrase, uint signatureCount)
{
   if ((secretKeyPhrase.length() < 8) || (signatureCount < 16))
      return false;

   mTotalLeafNodes = (uint)pow(2.0f, ceil(log(signatureCount * 168.0f) / log(2.0f)));
   mFirstLeafNodeIndex = mTotalLeafNodes;
   mTotalNodes = mTotalLeafNodes * 2; 
   mTreeHeight = Math::iLog2(mTotalNodes);

   mLeafPreImages.resize(mTotalLeafNodes);
   mTreeNodes.resize(mTotalNodes);

   for (uint leafNodeIndex = 0; leafNodeIndex < mTotalLeafNodes; leafNodeIndex++)
      computeLeafNode(mLeafPreImages[leafNodeIndex], mTreeNodes[mFirstLeafNodeIndex + leafNodeIndex], leafNodeIndex, secretKeyPhrase);

   for (uint internalNodeIndex = mFirstLeafNodeIndex - 1; internalNodeIndex >= cRootNodeIndex; internalNodeIndex--)
      computeInternalNode(mTreeNodes[internalNodeIndex], mTreeNodes[getChildNode(internalNodeIndex, 0)], mTreeNodes[getChildNode(internalNodeIndex, 1)], internalNodeIndex);

   return true;
}

//==============================================================================
// BDigitalSignature::init
//==============================================================================
bool BDigitalSignature::init(BStream& secretKeyData)
{
   uint signatureValue;
   secretKeyData >> signatureValue;

   if (signatureValue != (uint)cDigitalSignatureSig)
      return false;

   uchar treeHeight = 0;
   secretKeyData >> treeHeight;
   mTreeHeight = treeHeight;

   if ((mTreeHeight < 2) || (mTreeHeight > 32))
      return false;

   mTotalNodes = 1 << mTreeHeight;
   mTotalLeafNodes = mTotalNodes >> 1;
   mFirstLeafNodeIndex = mTotalLeafNodes;
   
   secretKeyData >> mLeafPreImages;
   if (mLeafPreImages.getSize() != mTotalLeafNodes)
      return false;
      
   secretKeyData >> mTreeNodes;
   if (mTreeNodes.getSize() != mTotalNodes)
      return false;
   
   secretKeyData >> signatureValue;

   if (signatureValue != (uint)cDigitalSignatureSig)
      return false;
      
   return true;      
}

//==============================================================================
// BDigitalSignature::createKeys
//==============================================================================
bool BDigitalSignature::createKeys(BSHA1& publicKey, BStream& secretKeyData, const BString& secretKeyPhrase, uint signatureCount)
{
   if (!init(secretKeyPhrase, signatureCount))
      return false;
   
   publicKey = mTreeNodes[cRootNodeIndex];
   
   secretKeyData << (uint)cDigitalSignatureSig;
   secretKeyData << (uchar)mTreeHeight;
   
   secretKeyData << mLeafPreImages;
   secretKeyData << mTreeNodes;
   
   secretKeyData << (uint)cDigitalSignatureSig;
   
   clear();
   
   return !secretKeyData.errorStatus();
}

//==============================================================================
// BDigitalSignature::signMessage
//==============================================================================
bool BDigitalSignature::signMessage(BStream& signatureData, const BSHA1& messageDigest, BStream& secretKeyData)
{
   if (!init(secretKeyData))
      return false;
   
   signatureData << (uint)cDigitalSignatureSig;
   signatureData << (uchar)mTreeHeight;
   
   uchar bitsToSign[21];
   for (uint i = 0; i < 20; i++)
      bitsToSign[i] = messageDigest[i];
   bitsToSign[20] = computeChecksum(messageDigest);
   
   Random rand;
   rand.setSeed(messageDigest.getDWORD(0), messageDigest.getDWORD(1), messageDigest.getDWORD(2), messageDigest.getDWORD(3), messageDigest.getDWORD(4), 
               (messageDigest.getDWORD(0) ^ messageDigest.getDWORD(1) ^  messageDigest.getDWORD(2) ^ messageDigest.getDWORD(3) ^ messageDigest.getDWORD(4)) + 1);   

   // Compute the initial leaf node.
   uint leafNodeIndex = mFirstLeafNodeIndex + rand.iRand(0, mTotalLeafNodes);
   
   UIntUnifier nodeIndexUnifier;
   
   UIntArray nodeIndices;      
   // Process all bits in the message digest+checksum
   for (uint bitIndex = 0; bitIndex < 168; bitIndex++)
   {
      const uint bit = (bitsToSign[bitIndex >> 3] & (1U << (bitIndex & 7))) != 0;
         
      UIntUnifier::InsertResult leafInsertResult(nodeIndexUnifier.insert(leafNodeIndex));

      if (leafInsertResult.second)
      {
         // Output the leaf's preimage, or its hash depending on the message bit.
         const BSHA1 hash(bit ? mTreeNodes[leafNodeIndex] : mLeafPreImages[leafNodeIndex - mFirstLeafNodeIndex]);
      
         signatureData << hash;
      }
                  
      computeNodeTraversal(nodeIndices, leafNodeIndex);

      // Now follow the authentication path of the leaf node, outputting sibling hashes as we travel toward the root.      
      for (uint i = 0; i < nodeIndices.getSize(); i++)
      {
         const uint nodeIndex = nodeIndices[i];
         if (nodeIndex == cRootNodeIndex)
            break;

         uint siblingNodeIndex = getSiblingNode(nodeIndex);
         UIntUnifier::InsertResult insertResult(nodeIndexUnifier.insert(siblingNodeIndex));
         
         if (insertResult.second)
         {
            if (siblingNodeIndex >= mFirstLeafNodeIndex)
            {
               int siblingBitIndex = (siblingNodeIndex < leafNodeIndex) ? (bitIndex - 1) : (bitIndex + 1);
               if (siblingBitIndex < 0) 
                  siblingBitIndex += 168;
               else if (siblingBitIndex >= 168)
                  siblingBitIndex -= 168;
                  
               BDEBUG_ASSERT((siblingBitIndex >= 0) && (siblingBitIndex < 168));
               const uint siblingBit = (bitsToSign[siblingBitIndex >> 3] & (1U << (siblingBitIndex & 7))) != 0;
               
               // Output the leaf's preimage, or its hash depending on the message bit.
               const BSHA1 siblingHash(siblingBit ? mTreeNodes[siblingNodeIndex] : mLeafPreImages[siblingNodeIndex - mFirstLeafNodeIndex]);

               signatureData << siblingHash;
            }
            else
               signatureData << mTreeNodes[siblingNodeIndex];
         }
      }
      
      leafNodeIndex++;
      if (leafNodeIndex == mTotalNodes)
         leafNodeIndex = mFirstLeafNodeIndex;
   }         
   
   signatureData << (uint)cDigitalSignatureSig;
        
   clear();
   
   return !signatureData.errorStatus();
}

//==============================================================================
// BDigitalSignature::verifyMessage
//==============================================================================
bool BDigitalSignature::verifyMessage(BStream& signatureData, const BSHA1& publicKey, const BSHA1& messageDigest)
{
   uchar bitsToCheck[21];
   for (uint i = 0; i < 20; i++)
      bitsToCheck[i] = messageDigest[i];
   bitsToCheck[20] = computeChecksum(messageDigest);

   uint signatureValue;
   signatureData >> signatureValue;
   
   if (signatureValue != (uint)cDigitalSignatureSig)
      return false;

   uchar treeHeight = 0;
   signatureData >> treeHeight;
   mTreeHeight = treeHeight;
   
   if ((mTreeHeight < 2) || (mTreeHeight > 32))
      return false;
   
   mTotalNodes = 1 << mTreeHeight;
   mTotalLeafNodes = mTotalNodes >> 1;
   mFirstLeafNodeIndex = mTotalLeafNodes;
         
   Random rand;
   rand.setSeed(messageDigest.getDWORD(0), messageDigest.getDWORD(1), messageDigest.getDWORD(2), messageDigest.getDWORD(3), messageDigest.getDWORD(4), 
               (messageDigest.getDWORD(0) ^ messageDigest.getDWORD(1) ^  messageDigest.getDWORD(2) ^ messageDigest.getDWORD(3) ^ messageDigest.getDWORD(4)) + 1);   

   // Compute the initial leaf node.
   // We should track the leaf nodes that have been used by all previous messages, and never use them again.
   // The tracking of which leaf nodes have been utilized would be just another thing for us to screw up, so instead just choose 
   // a psuedo random initial leaf and hope for the best.
   uint leafNodeIndex = mFirstLeafNodeIndex + rand.iRand(0, mTotalLeafNodes);   
   
   BNodeHashMap nodeHashMap;
   
   UIntArray nodeIndices;    
   // Process all bits in the message digest+checksum  
   for (uint bitIndex = 0; bitIndex < 168; bitIndex++)
   {
      const uint bit = (bitsToCheck[bitIndex >> 3] & (1U << (bitIndex & 7))) != 0;
            
      BSHA1 nodeHash;
                  
      BNodeHashMap::const_iterator leafIt = nodeHashMap.find(leafNodeIndex);
            
      if (leafIt != nodeHashMap.end())
         nodeHash = leafIt->second;
      else
      {
         if ((signatureData.errorStatus()) || (signatureData.bytesLeft() < sizeof(BSHA1)))
            return false;
            
         signatureData >> nodeHash;
         
         if (!bit)
         {
            BSHA1Gen sha1Gen(nodeHash.getBuf(), nodeHash.size());
            
            nodeHash = sha1Gen.finalize();
         }
         
         nodeHashMap.insert(leafNodeIndex, nodeHash);
      }         

      computeNodeTraversal(nodeIndices, leafNodeIndex);         
      
      // Now follow the authentication path of the leaf node, reading sibling hashes as we travel toward the root.
      for (uint i = 0; i < nodeIndices.getSize(); i++)
      {
         const uint nodeIndex = nodeIndices[i];
         if (nodeIndex == cRootNodeIndex)
            break;

         const uint siblingNodeIndex = getSiblingNode(nodeIndex);
         
         BSHA1 siblingHash;
         
         BNodeHashMap::const_iterator it = nodeHashMap.find(siblingNodeIndex);
         
         if (it != nodeHashMap.end())
            siblingHash = it->second;
         else
         {
            if ((signatureData.errorStatus()) || (signatureData.bytesLeft() < sizeof(BSHA1)))
               return false;
               
            signatureData >> siblingHash;
            
            if (siblingNodeIndex >= mFirstLeafNodeIndex)
            {
               int siblingBitIndex = (siblingNodeIndex < leafNodeIndex) ? (bitIndex - 1) : (bitIndex + 1);
               if (siblingBitIndex < 0) 
                  siblingBitIndex += 168;
               else if (siblingBitIndex >= 168)
                  siblingBitIndex -= 168;
               BDEBUG_ASSERT((siblingBitIndex >= 0) && (siblingBitIndex < 168));
               const uint siblingBit = (bitsToCheck[siblingBitIndex >> 3] & (1U << (siblingBitIndex & 7))) != 0;

               if (!siblingBit)
               {
                  BSHA1Gen siblingSHA1Gen(siblingHash.getBuf(), siblingHash.size());

                  siblingHash = siblingSHA1Gen.finalize();
               }
            }
            
            nodeHashMap.insert(siblingNodeIndex, siblingHash);
         }
                             
         const uint parentNodeIndex = getParentNode(siblingNodeIndex);
                  
         if (siblingNodeIndex < nodeIndex)
            computeInternalNode(nodeHash, siblingHash, nodeHash, parentNodeIndex);
         else
            computeInternalNode(nodeHash, nodeHash, siblingHash, parentNodeIndex);
      }
      
      // If the computed root hash isn't what it's supposed to be the message has failed to authenticate.
      if (nodeHash != publicKey)
         return false;
         
      leafNodeIndex++;
      if (leafNodeIndex == mTotalNodes)
         leafNodeIndex = mFirstLeafNodeIndex;
   }      
   
   signatureData >> signatureValue;
   if (signatureValue != (uint)cDigitalSignatureSig)
      return false;
      
   if (signatureData.errorStatus())
      return false;

   return true;
}
