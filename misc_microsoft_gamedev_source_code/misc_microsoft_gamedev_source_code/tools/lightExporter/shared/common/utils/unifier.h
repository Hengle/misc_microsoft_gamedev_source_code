//-----------------------------------------------------------------------------
// File: unifier.h
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

// A Unifier is a container of unique objects, 
// with a fixed index associated with each.
// In this implementation, the object is stored twice (!) in memory:
// once in a std::hash_map, and another time in a std::vector.
#pragma once
#ifndef UNIFIER_H
#define UNIFIER_H

#include "common/core/core.h"

#include <map>
#include <hash_map>

namespace gr
{
	template<
		class ObjectType, 
		class HashCompareType = stdext::hash_compare<ObjectType, std::less<ObjectType> > 
		>
	class Unifier
	{
	public:
		typedef int Index;
		enum { InvalidIndex = -1 };

		typedef stdext::hash_map<ObjectType, Index, HashCompareType> ObjectHashMap;
		typedef std::vector<ObjectType> ObjectVector;
		
		Unifier()
		{
		}

		virtual ~Unifier()
		{
		}

		int size(void) const
		{
			return static_cast<int>(mObjs.size());
		}

		bool empty(void) const
		{
			return mObjs.empty();
		}

		const ObjectHashMap& getHashMap(void) const
		{
			return mHashMap;
		}

		const ObjectVector& getObjects(void) const
		{
			return mObjs;
		}

		const ObjectType& at(int i) const
		{
			const ObjectType& o = mObjs[DebugRange(i, size())];

	#if DEBUG		
			ObjectHashMap::const_iterator it = mHashMap.find(o);
			Assert(it != mHashMap.end());
			Assert(it->second == i);
	#endif

			return o;
		}

		const ObjectType& operator[] (int i) const
		{
			return at(i);
		}

		void clear(void)
		{
			mHashMap.clear();
			mObjs.clear();
		}

		// first is the object index
		// second is false if insert failed (object already exists)
		typedef std::pair<Index, bool> InsertResult;

		InsertResult insert(const ObjectType& o)
		{
			Index nextIndex = size();
			
			const std::pair<ObjectHashMap::const_iterator, bool> res = 
				mHashMap.insert(std::make_pair(o, nextIndex));

			if (res.second)
				mObjs.push_back(o);
			else
				nextIndex = (*res.first).second;
			
			return std::make_pair(nextIndex, res.second);
		}

		// InvalidIndex if object does not exist in the container
		Index find(const ObjectType& o) const
		{
			ObjectHashMap::const_iterator res = mHashMap.find(o);
			if (res == mHashMap.end())
				return InvalidIndex;
			return res->second;
		}

	protected:
		ObjectHashMap mHashMap;
		ObjectVector mObjs;
	};

	template<
		class ObjectType, 
		class HashCompareType = stdext::hash_compare<ObjectType, std::less<ObjectType> > 
		>
	class IndexedUnifier : public Unifier<ObjectType, HashCompareType>
	{
		typedef Unifier<ObjectType, HashCompareType> Base;
		
	public:
		IndexedUnifier() : 
			Base(), 
			mNextInputIndex(0)
		{
		}

		virtual ~IndexedUnifier()
		{
		}
		
		void clear(void)
		{
			Base::clear();
			mInputToMergedIndices.clear();
			mMergedToInputIndices.clear();
			mNextInputIndex = 0;
		}

		struct InsertResult
		{
			Index objectIndex;
			bool inserted;
			Index origIndex;
			
			InsertResult() 
			{
			}

			InsertResult(Index objI, bool i, Index origI) : objectIndex(objI), inserted(i), origIndex(origI)
			{
			}
		};

		InsertResult insert(const ObjectType& o)
		{
			// first is the object index
			// second is false if insert failed (object already exists)
			std::pair<Index, bool> res = Base::insert(o);

			if (res.second)
			{
				// insertion - object has not been seen before
				mMergedToInputIndices.push_back(mNextInputIndex);
			}
			
			mInputToMergedIndices.push_back(res.first);
			
			const Index origIndex = mNextInputIndex;
			
			mNextInputIndex++;

			return InsertResult(res.first, res.second, origIndex);
		}

		int numInputObjects(void) const 
		{
			return mNextInputIndex;
		}

		// returns the input index of the indicated merged object
		Index mergedIndexToInputIndex(Index mergedIndex) const
		{
			return mMergedToInputIndices[
				DebugRange(mergedIndex, static_cast<int>(mMergedToInputIndices.size()))
			];
		}
		
		// returns the merged index of the indicated input object
		Index inputIndexToMergedIndex(Index inputIndex) const
		{
			return mInputToMergedIndices[DebugRange(inputIndex, static_cast<int>(mInputToMergedIndices.size()))];
		}
		
		// returns input index of the first instance of the indicated input object
		// (i.e. the first time the object was encountered)
		Index inputIndexToFirstInstance(Index inputIndex) const
		{
			const Index mergedIndex = inputIndexToMergedIndex(inputIndex);
			return mergedIndexToInputIndex(mergedIndex);
		}

		// InvalidIndex if object does not exist in the container
		Index find(const ObjectType& o) const
		{
			ObjectHashMap::const_iterator res = mHashMap.find(o);
			if (res == mHashMap.end())
				return InvalidIndex;
			return res->second;
		}

	protected:
		// maps input index to new index
		std::vector<Index> mInputToMergedIndices;
		
		// maps new indices to input index
		std::vector<Index> mMergedToInputIndices;

		int mNextInputIndex;
	};

	template<class ObjectType>
	class MapUnifier
	{
	public:
		typedef int Index;
		enum { InvalidIndex = -1 };

		typedef std::map<ObjectType, Index> ObjectMap;
		typedef std::vector<ObjectType> ObjectVector;

		MapUnifier()
		{
		}

		virtual ~MapUnifier()
		{
		}

		int size(void) const
		{
			return static_cast<int>(mObjs.size());
		}

		bool empty(void) const
		{
			return mObjs.empty();
		}

		const ObjectMap& getMap(void) const
		{
			return mMap;
		}

		const ObjectVector& getObjects(void) const
		{
			return mObjs;
		}

		const ObjectType& at(int i) const
		{
			const ObjectType& o = mObjs[DebugRange(i, size())];

	#if DEBUG		
			ObjectMap::const_iterator it = mMap.find(o);
			Assert(it != mMap.end());
			Assert(it->second == i);
	#endif

			return o;
		}

		const ObjectType& operator[] (int i) const
		{
			return at(i);
		}

		void clear(void)
		{
			mMap.clear();
			mObjs.clear();
		}

		// first is the object index
		// second is false if insert failed (object already exists)
		std::pair<Index, bool> insert(const ObjectType& o)
		{
			Index nextIndex = size();
			
			const std::pair<ObjectMap::const_iterator, bool> res = 
				mMap.insert(std::make_pair(o, nextIndex));

			if (res.second)
				mObjs.push_back(o);
			else
				nextIndex = (*res.first).second;
			
			return std::make_pair(nextIndex, res.second);
		}

		// InvalidIndex if object does not exist in the container
		Index find(const ObjectType& o) const
		{
			ObjectMap::const_iterator res = mMap.find(o);
			if (res == mMap.end())
				return InvalidIndex;
			return res->second;
		}

	protected:
		ObjectMap mMap;
		ObjectVector mObjs;
	};

} // end namespace gr

#endif // UNIFIER_H









