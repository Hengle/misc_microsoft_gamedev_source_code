//==============================================================================
// interptable.h
//
// Copyright (c) 2004, Ensemble Studios
//==============================================================================

#pragma once 

#ifndef _INTERPTABLE_H_
#define _INTERPTABLE_H_

//==============================================================================
// Includes
#include "xmlReader.h"
#include "string\convertToken.h"
#include "gamefilemacros.h"
#include "stream\stream.h"

//==============================================================================
// Forward declarations

//==============================================================================
// Const declarations

//==============================================================================
// class BInterpTable
//
// This is a templated class that will interpolate a value based on a table
// of key / value / variation triplets.  You must define your interpolation
// function and give it to the typedef when declaring your new type.  The
// current function definition works well for linear interpolation functions
// as the inputs are a normalized key (0.0 - 1.0), the value at the normalized
// 0.0 and the value at the normalized 1.0.
//
// Also note that because of how variations are computed, the type specified
// for the BInterpTable must define operator+(Type) and operator*(float).
// 
// See the bottom of this file for common types of BInterpTables already made.
//==============================================================================

template <class Type, void (*interpFunc)(const float, const Type&, const Type&, Type&)>
class BInterpTable
{
   public:

      // Constructor / deconstructor
      inline                           BInterpTable();
      inline virtual                   ~BInterpTable();

      // Manage key, value, variance triplets
      inline void                      addKeyValueVar(float key, const Type& value, const Type& var);
      inline void                      clear();
      inline const BDynamicSimArray<float> &getKeys() const;
      inline const BDynamicSimArray<Type>  &getValues() const;
      inline const BDynamicSimArray<Type>  &getVariances() const;

      // Interpolation
      inline bool                      interp(float time, float varFactor, Type& result);

   protected:

      // Parallel arrays for storing key (time), value, and variance per step
      BDynamicSimArray<float>              mKeys;
      BDynamicSimArray<Type>               mValues;
      BDynamicSimArray<Type>               mVariances;
};


//==============================================================================
// BInterpTable::BInterpTable
//==============================================================================
template <class Type, void (*interpFunc)(const float, const Type&, const Type&, Type&)>
BInterpTable<Type, interpFunc>::BInterpTable()
{
}


//==============================================================================
// BInterpTable::~BInterpTable
//==============================================================================
template <class Type, void (*interpFunc)(const float, const Type&, const Type&, Type&)>
BInterpTable<Type, interpFunc>::~BInterpTable()
{
}


//==============================================================================
// BInterpTable::addKeyValueVar
//==============================================================================
template <class Type, void (*interpFunc)(const float, const Type&, const Type&, Type&)>
void BInterpTable<Type, interpFunc>::addKeyValueVar(float key, const Type& value, const Type& var)
{
   // Find location to insert
   long insertIndex = 0;
   while ((insertIndex < mKeys.getNumber()) && (key > mKeys[insertIndex]))
      insertIndex++;

   // Insert key/value/variance into arrays, replace if keys are the same
   if ((insertIndex < mKeys.getNumber()) && (key == mKeys[insertIndex]))
   {
      mKeys.setAt(insertIndex, key);
      mValues.setAt(insertIndex, value);
      mVariances.setAt(insertIndex, var);
   }
   else
   {
      mKeys.insertAtIndex(key, insertIndex);
      mValues.insertAtIndex(value, insertIndex);
      mVariances.insertAtIndex(var, insertIndex);
   }
}


//==============================================================================
// BInterpTable::clear
//==============================================================================
template <class Type, void (*interpFunc)(const float, const Type&, const Type&, Type&)>
void BInterpTable<Type, interpFunc>::clear()
{
   mKeys.setNumber(0);
   mValues.setNumber(0);
   mVariances.setNumber(0);
}


//==============================================================================
// BInterpTable::getKeys
//==============================================================================
template <class Type, void (*interpFunc)(const float, const Type&, const Type&, Type&)>
const BDynamicSimArray<float>& BInterpTable<Type, interpFunc>::getKeys() const
{
   return mKeys;
}


//==============================================================================
// BInterpTable::getValues
//==============================================================================
template <class Type, void (*interpFunc)(const float, const Type&, const Type&, Type&)>
const BDynamicSimArray<Type>& BInterpTable<Type, interpFunc>::getValues() const
{
   return mValues;
}


//==============================================================================
// BInterpTable::getVariances
//==============================================================================
template <class Type, void (*interpFunc)(const float, const Type&, const Type&, Type&)>
const BDynamicSimArray<Type>& BInterpTable<Type, interpFunc>::getVariances() const
{
   return mVariances;
}


//==============================================================================
// BInterpTable::interp
//==============================================================================
template <class Type, void (*interpFunc)(const float, const Type&, const Type&, Type&)>
bool BInterpTable<Type, interpFunc>::interp(float time, float varFactor, Type& result)
{
   if (mKeys.getNumber() <= 0)
      return false;

   if (time <= mKeys[0])
   {
      result = mValues[0] + (mVariances[0] * varFactor);
   }
   else if (time >= mKeys[mKeys.getNumber() - 1])
   {
      result = mValues[mKeys.getNumber() - 1] + (mVariances[mKeys.getNumber() - 1] * varFactor);
   }
   else
   {
      if (interpFunc)
      {
         long index = 0;
         while ((index < mKeys.getNumber()) && (time > mKeys[index]))
            index++;

         float relativeTime = (time - mKeys[index - 1]) / (mKeys[index] - mKeys[index - 1]);

         // Get values + variance to interpolate between
         Type val1 = mValues[index - 1] + (mVariances[index - 1] * varFactor);
         Type val2 = mValues[index] + (mVariances[index] * varFactor);

         interpFunc(relativeTime, val1, val2, result);
      }
      else
      {
         BFAIL("You might want to define an interpolation function for this class.");
         return false;
      }
   }

   return true;
}


//==============================================================================
// class BInterpTableNoVar
//
// This is identical to the above class, but without variances.
//==============================================================================

template <class Type, void (*interpFunc)(const float, const Type&, const Type&, Type&), bool (*stringConvertFunc)(const BSimString &token, Type&)>
class BInterpTableNoVar
{
   public:

      // Constructor / deconstructor
      inline                           BInterpTableNoVar();
      inline virtual                   ~BInterpTableNoVar();

      // Manage key, value, variance triplets
      inline void                      addKeyValue(float key, const Type& value);
      inline void                      clear();
      inline const BDynamicSimArray<float> &getKeys() const;
      inline const BDynamicSimArray<Type>  &getValues() const;
      inline void                      setKey(long index, float key);
      inline void                      setValue(long index, const Type& value);
      inline float                     getLastKey() const;
      inline int                       getNumKeys() const;

      inline void                      load(BXMLNode& node);

      // Interpolation
      inline bool                      interp(float time, Type& result);

      inline bool save(BStream* pStream, int saveType) const;
      inline bool load(BStream* pStream, int saveType);

   protected:

      // Parallel arrays for storing key (time), value per step
      BDynamicSimArray<float>              mKeys;
      BDynamicSimArray<Type>               mValues;
};


//==============================================================================
// BInterpTableNoVar::BInterpTableNoVar
//==============================================================================
template <class Type, void (*interpFunc)(const float, const Type&, const Type&, Type&), bool (*stringConvertFunc)(const BSimString&, Type&)>
BInterpTableNoVar<Type, interpFunc, stringConvertFunc>::BInterpTableNoVar()
{
}


//==============================================================================
// BInterpTableNoVar::~BInterpTableNoVar
//==============================================================================
template <class Type, void (*interpFunc)(const float, const Type&, const Type&, Type&), bool (*stringConvertFunc)(const BSimString&, Type&)>
BInterpTableNoVar<Type, interpFunc, stringConvertFunc>::~BInterpTableNoVar()
{
}


//==============================================================================
// BInterpTableNoVar::addKeyValue
//==============================================================================
template <class Type, void (*interpFunc)(const float, const Type&, const Type&, Type&), bool (*stringConvertFunc)(const BSimString&, Type&)>
void BInterpTableNoVar<Type, interpFunc, stringConvertFunc>::addKeyValue(float key, const Type& value)
{
   // Find location to insert
   long insertIndex = 0;
   while ((insertIndex < mKeys.getNumber()) && (key > mKeys[insertIndex]))
      insertIndex++;

   // Insert key/value/variance into arrays, replace if keys are the same
   if ((insertIndex < mKeys.getNumber()) && (key == mKeys[insertIndex]))
   {
      mKeys.setAt(insertIndex, key);
      mValues.setAt(insertIndex, value);
   }
   else
   {
      mKeys.insertAtIndex(key, insertIndex);
      mValues.insertAtIndex(value, insertIndex);
   }
}


//==============================================================================
// BInterpTableNoVar::clear
//==============================================================================
template <class Type, void (*interpFunc)(const float, const Type&, const Type&, Type&), bool (*stringConvertFunc)(const BSimString&, Type&)>
void BInterpTableNoVar<Type, interpFunc, stringConvertFunc>::clear()
{
   mKeys.setNumber(0);
   mValues.setNumber(0);
}


//==============================================================================
// BInterpTableNoVar::getKeys
//==============================================================================
template <class Type, void (*interpFunc)(const float, const Type&, const Type&, Type&), bool (*stringConvertFunc)(const BSimString&, Type&)>
const BDynamicSimArray<float>& BInterpTableNoVar<Type, interpFunc, stringConvertFunc>::getKeys() const
{
   return mKeys;
}


//==============================================================================
// BInterpTableNoVar::getValues
//==============================================================================
template <class Type, void (*interpFunc)(const float, const Type&, const Type&, Type&), bool (*stringConvertFunc)(const BSimString&, Type&)>
const BDynamicSimArray<Type>& BInterpTableNoVar<Type, interpFunc, stringConvertFunc>::getValues() const
{
   return mValues;
}


//==============================================================================
// BInterpTableNoVar::setKey
//==============================================================================
template <class Type, void (*interpFunc)(const float, const Type&, const Type&, Type&), bool (*stringConvertFunc)(const BSimString&, Type&)>
void BInterpTableNoVar<Type, interpFunc, stringConvertFunc>::setKey(long index, float key)
{
   if ((index < 0) || (index >= mKeys.getNumber()))
      return;

   mKeys[index] = key;
}


//==============================================================================
// BInterpTableNoVar::setValue
//==============================================================================
template <class Type, void (*interpFunc)(const float, const Type&, const Type&, Type&), bool (*stringConvertFunc)(const BSimString&, Type&)>
void BInterpTableNoVar<Type, interpFunc, stringConvertFunc>::setValue(long index, const Type& value)
{
   if ((index < 0) || (index >= mValues.getNumber()))
      return;

   mValues[index] = value;
}


//==============================================================================
// BInterpTableNoVar::getLastKey
//==============================================================================
template <class Type, void (*interpFunc)(const float, const Type&, const Type&, Type&), bool (*stringConvertFunc)(const BSimString&, Type&)>
float BInterpTableNoVar<Type, interpFunc, stringConvertFunc>::getLastKey() const
{
   if (mKeys.getNumber() > 0)
      return mKeys[mKeys.getNumber() - 1];
   else
      return 0.0f;
}


//==============================================================================
// BInterpTableNoVar::getNumKeys
//==============================================================================
template <class Type, void (*interpFunc)(const float, const Type&, const Type&, Type&), bool (*stringConvertFunc)(const BSimString&, Type&)>
int BInterpTableNoVar<Type, interpFunc, stringConvertFunc>::getNumKeys() const
{
   return mKeys.getNumber();
}


//==============================================================================
// BInterpTableNoVar::load
//==============================================================================
template <class Type, void (*interpFunc)(const float, const Type&, const Type&, Type&), bool (*stringConvertFunc)(const BSimString&, Type&)>
void BInterpTableNoVar<Type, interpFunc, stringConvertFunc>::load(BXMLNode& node)
{
   clear();

   for (int i = 0; i < node.getNumberChildren(); i++)
   {
      BXMLNode childNode(node.getChild(i));
      const BPackedString& name = childNode.getName();
      
      if (name == "Value")
      {
         float key = 0.0f;
         if (!childNode.getAttribValueAsFloat("time", key))
            continue;

         BSimString tempStr;
         childNode.getText(tempStr);

         Type value;
         if (!stringConvertFunc(tempStr, value))
            continue;

         addKeyValue(key, value);
      }
   }
}


//==============================================================================
// BInterpTableNoVar::interp
//==============================================================================
template <class Type, void (*interpFunc)(const float, const Type&, const Type&, Type&), bool (*stringConvertFunc)(const BSimString&, Type&)>
bool BInterpTableNoVar<Type, interpFunc, stringConvertFunc>::interp(float time, Type& result)
{
   if (mKeys.getNumber() <= 0)
      return false;

   if (time <= mKeys[0])
   {
      result = mValues[0];
   }
   else if (time >= mKeys[mKeys.getNumber() - 1])
   {
      result = mValues[mKeys.getNumber() - 1];
   }
   else
   {
      if (interpFunc)
      {
         long index = 0;
         while ((index < mKeys.getNumber()) && (time > mKeys[index]))
            index++;

         float relativeTime = (time - mKeys[index - 1]) / (mKeys[index] - mKeys[index - 1]);

         // Get values + variance to interpolate between
         Type val1 = mValues[index - 1];
         Type val2 = mValues[index];

         interpFunc(relativeTime, val1, val2, result);
      }
      else
      {
         BFAIL("You might want to define an interpolation function for this class.");
         return false;
      }
   }

   return true;
}

template <class Type, void (*interpFunc)(const float, const Type&, const Type&, Type&), bool (*stringConvertFunc)(const BSimString&, Type&)>
bool BInterpTableNoVar<Type, interpFunc, stringConvertFunc>::save(BStream* pStream, int saveType) const
{
   GFWRITEARRAY(pStream, float, mKeys, uint16, 1000);
   GFWRITEARRAY(pStream, Type, mValues, uint16, 1000);
   return true;
}

template <class Type, void (*interpFunc)(const float, const Type&, const Type&, Type&), bool (*stringConvertFunc)(const BSimString&, Type&)>
bool BInterpTableNoVar<Type, interpFunc, stringConvertFunc>::load(BStream* pStream, int saveType)
{
   GFREADARRAY(pStream, float, mKeys, uint16, 1000);
   GFREADARRAY(pStream, Type, mValues, uint16, 1000);
   return true;
}

//==============================================================================
//==============================================================================
// Common typedefs.
//==============================================================================
//==============================================================================

//==============================================================================
// Linearly interpolated float table
extern void lerpFloats(const float time, const float& val1, const float& val2, float& result);
typedef BInterpTableNoVar<float, lerpFloats, convertSimTokenToFloat>   BFloatLerpTable;

//==============================================================================
// Linearly interpolated BVector table
extern void lerpVector(const float time, const BVector& val1, const BVector& val2, BVector& result);
typedef BInterpTableNoVar<BVector, lerpVector, convertSimTokenToVector>   BVectorLerpTable;

//==============================================================================
// Linearly interpolated BVector2 table
extern void lerpVector2(const float time, const BVector2& val1, const BVector2& val2, BVector2& result);
typedef BInterpTableNoVar<BVector2, lerpVector2, convertSimTokenToVector2>   BVector2LerpTable;

//==============================================================================
// Linearly interpolated BVector4 table
extern void lerpVector4(const float time, const BVector4& val1, const BVector4& val2, BVector4& result);
typedef BInterpTableNoVar<BVector4, lerpVector4, convertSimTokenToVector4>   BVector4LerpTable;

#endif

//==============================================================================
// eof: interptable.h
//==============================================================================
