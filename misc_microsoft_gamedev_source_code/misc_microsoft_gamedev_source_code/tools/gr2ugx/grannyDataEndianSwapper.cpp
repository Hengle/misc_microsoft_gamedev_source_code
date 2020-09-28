//------------------------------------------------------------------------------------------------------------------------
//  File: grannyDataEndianSwapper.cpp
//
//  Copyright (c) 2005-2007, Ensemble Studios
//
//------------------------------------------------------------------------------------------------------------------------
#include "xcore.h"
#include "grannyDataEndianSwapper.h"

void BGrannyDataEndianSwapper::processArray(
      granny_pointer_hash *ObjectHash, 
      char const *Name, int ArrayCount,
      granny_data_type_definition const *Type,
      void *ObjectInit)
{
   Name;
   
   unsigned char *Object = (unsigned char *)ObjectInit;

   if (Object)
   {
      int ObjectSize = GrannyGetTotalObjectSize(Type);

      for(int ArrayIndex = 0; ArrayIndex < ArrayCount; ++ArrayIndex)
      {
         processObject(ObjectHash, Object, Type, false);
         Object += ObjectSize;
      }
   }
}

void BGrannyDataEndianSwapper::processPointers(void** p, uint numPointers)
{
   for (uint i = 0; i < numPointers; i++)
   {
      void* ptr = p[i];
      if (!ptr)
         continue;
               
      ptr = (void*)((BYTE*)ptr - (BYTE*)mpSrc);
      
      BVERIFY((DWORD)ptr < mBufSize);
      
      BVERIFY( ptr == *(void**)computeDst(p + i) );
         
      EndianSwitchDWords((DWORD*)&ptr, 1);
      
      *(void**)computeDst(p + i) = ptr;
      
      //p[i] = (BYTE*)mpSrc + (DWORD)p[i];
   }         
}

void BGrannyDataEndianSwapper::processTransform(granny_transform &Transform)
{
   granny_transform* pDst = (granny_transform*)computeDst(&Transform);
   EndianSwitchWorker(pDst, pDst + 1, "17i");   
}

void BGrannyDataEndianSwapper::processDWORDs(const void* p, uint num)
{
   void* q = computeDst(p);
   EndianSwitchDWords((DWORD*)q, num);
}

void* BGrannyDataEndianSwapper::computeDst(const void* pSrc)
{
   if (!pSrc)
      return NULL;
      
   BVERIFY((pSrc >= mpSrc) && (pSrc < ((const BYTE*)mpSrc + mBufSize)));
   return ((const BYTE*)pSrc - (const BYTE*)mpSrc) + (BYTE*)mpDst;
}
   
void BGrannyDataEndianSwapper::processVariant(const granny_data_type_definition* VariantType, granny_pointer_hash *ObjectHash)
{
   if (!VariantType)
      return;
      
   if (VariantType == mpDataTypeDef)
      return;
   
   const granny_data_type_definition* q = VariantType;
         
   while (q->Type != GrannyEndMember)
   {
      if (!GrannyHashedPointerKeyExists(ObjectHash, q))
      {
         GrannyAddPointerToHash(ObjectHash, q, 0);

         processDWORDs(&q->Type);
         processPointers((void**)&q->Name);
         processPointers((void**)&q->ReferenceType);
         processDWORDs(&q->ArrayWidth);
         processDWORDs(q->Extra, 3);
         
         if (q->ReferenceType)
            processVariant(q->ReferenceType, ObjectHash);
      }

      q++;
   }
}   

void BGrannyDataEndianSwapper::processObject(
   granny_pointer_hash *ObjectHash,
   void const *Object,
   granny_data_type_definition const *ObjectType,
   bool IgnoreHashing)
{
   if (!IgnoreHashing)
   {
      if (GrannyHashedPointerKeyExists(ObjectHash, Object))
         return;

      GrannyAddPointerToHash(ObjectHash, Object, 0);
   }
         
   granny_uint8 *ObjectPtr = (granny_uint8 *)Object;
    
   for (granny_data_type_definition const *Type = ObjectType; Type && Type->Type != GrannyEndMember; ObjectPtr += GrannyGetMemberTypeSize(Type), ++Type)
   {
      switch (Type->Type)
      {
         case GrannyReferenceMember:
         {
             processPointers((void**)ObjectPtr);
             
             void *Object = *(void **)ObjectPtr;
             
             if (Object)
                 processObject(ObjectHash, Object, Type->ReferenceType, false);
             
             break;
         }

         case GrannyReferenceToArrayMember:
         {
             processDWORDs(ObjectPtr);
             processPointers((void**)(ObjectPtr + sizeof(granny_int32)));
            
             int ArrayCount = *(granny_int32 *)ObjectPtr;
                             
             granny_uint8 *Object = *(granny_uint8 **)(ObjectPtr + sizeof(granny_int32));
                 
             processArray(ObjectHash, Type->Name, ArrayCount, Type->ReferenceType, Object);
            
            break;
         } 
         
         case GrannyArrayOfReferencesMember:
         {
             processDWORDs(ObjectPtr);
             processPointers((void**)(ObjectPtr + sizeof(granny_int32)));
             
             int ArrayCount = *(granny_int32 *)ObjectPtr;
                                             
             void **Object = *(void ***)(ObjectPtr + sizeof(granny_int32));

             if (Object)
             {
                 for(int ArrayIndex = 0; ArrayIndex < ArrayCount; ++ArrayIndex)
                 {
                     processPointers(Object);
                     
                     if(*Object)
                     {
                         processObject(ObjectHash, *Object, Type->ReferenceType, false);
                     }
                     
                     ++Object;
                 }
             }
           
            break;  
         } 
        
         case GrannyVariantReferenceMember:
         {
             processPointers((void**)ObjectPtr);
             processPointers((void**)(ObjectPtr + sizeof(granny_data_type_definition *)));
             
             granny_data_type_definition *VariantType = *(granny_data_type_definition **)ObjectPtr;
             
             processVariant(VariantType, ObjectHash);            
                                             
             void *Object = *(void **)(ObjectPtr + sizeof(granny_data_type_definition *));
             
             if(Object)
             {
                 processObject(ObjectHash, Object, VariantType, false);
             }
           
            break;  
         } 

         case GrannyReferenceToVariantArrayMember:
         {
             processPointers((void**)ObjectPtr);
             processDWORDs(ObjectPtr + sizeof(granny_data_type_definition *));
             processPointers((void**)(ObjectPtr + sizeof(granny_data_type_definition *) + sizeof(int)));
                             
             granny_data_type_definition *VariantType = *(granny_data_type_definition **)ObjectPtr;
             
             processVariant(VariantType, ObjectHash);
                                                             
             int Count = *(int *)(ObjectPtr + sizeof(granny_data_type_definition *));
             
             void *Object = *(void **)(ObjectPtr + sizeof(granny_data_type_definition *) + sizeof(int));
             
             processArray(ObjectHash, Type->Name, Count, VariantType, Object);
             
             break;                           
         }
        
         case GrannyInlineMember:
         {
             processObject(ObjectHash, ObjectPtr, Type->ReferenceType, true);
             break;
         } 
       
         case GrannyTransformMember:
         {
             granny_transform &Transform = *(granny_transform *)ObjectPtr;
             
             processTransform(Transform);
             break;
         }

         case GrannyStringMember:
         {
             processPointers((void**)ObjectPtr);
             
             //char *String = *(char **)ObjectPtr;
             
             break;
         } 
         
         case GrannyUInt8Member:
         case GrannyNormalUInt8Member:
             processScalarMember<granny_uint8>(Type, ObjectPtr, "%u");
             break;

         case GrannyInt8Member:
         case GrannyBinormalInt8Member:
             processScalarMember<granny_int8>(Type, ObjectPtr, "%d");
             break;

         case GrannyUInt16Member:
         case GrannyNormalUInt16Member:
             processScalarMember<granny_uint16>(Type, ObjectPtr, "%u");
             break;

         case GrannyInt16Member:
         case GrannyBinormalInt16Member:
             processScalarMember<granny_int16>(Type, ObjectPtr, "%d");
             break;

         case GrannyUInt32Member:
             processScalarMember<granny_uint32>(Type, ObjectPtr, "%u");
             break;

         case GrannyInt32Member:
             processScalarMember<granny_int32>(Type, ObjectPtr, "%d");
             break;

         case GrannyReal32Member:
         {
             processScalarMember<granny_real32>(Type, ObjectPtr, "%f");
             break;
         } 

         default:
         {
             BVERIFY(0);
             break;
         } 
      }
   }
}
     
BGrannyDataEndianSwapper::BGrannyDataEndianSwapper(void* pSrc, void* pDst, uint bufSize, granny_data_type_definition* pDataTypeDef) :
   mpSrc(pSrc),
   mpDst(pDst),
   mBufSize(bufSize),
   mpDataTypeDef(pDataTypeDef)
{
   BDEBUG_ASSERT(pSrc && pDst && bufSize && pDataTypeDef);
   
   GrannyRebasePointers(pDataTypeDef, pSrc, (uint)pSrc, true);
   
   mpPointerHash = GrannyNewPointerHash();
   
   processObject(
      mpPointerHash,
      pSrc,
      pDataTypeDef,
      false);
   
   GrannyDeletePointerHash(mpPointerHash);
   mpPointerHash = NULL;
}
