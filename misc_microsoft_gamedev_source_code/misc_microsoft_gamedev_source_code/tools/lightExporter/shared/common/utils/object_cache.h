//-----------------------------------------------------------------------------
// File: object_cache.h
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#pragma once
#ifndef OBJECT_CACHE_H
#define OBJECT_CACHE_H

#include "utils.h"

#include <algorithm>
#include <map>

namespace gr
{
	// Simple wrapper around std::map to facilitate the unique storage of 
	// keyed items.
	template <
		typename KeyType, 
		typename ObjectType, 
		typename KeyCompType = std::less<KeyType> 
		>
	class ObjectCache 
	{
	public:
		typedef KeyType			Key;
		typedef ObjectType	Object;
		typedef KeyCompType KeyComp;
		typedef std::map<Key, Object, KeyComp> Container;
		
		ObjectCache(bool clearContents = false) : mAutoClearContents(clearContents)
		{
		}

		virtual ~ObjectCache()
		{
			if (mAutoClearContents)
				clear();
		}

		Object& get(const Key& key)
		{
			Container::iterator it(mObjects.find(key));
			
			if (it != mObjects.end())
				return it->second;
						
			std::pair<Container::iterator, bool> res = 
				mObjects.insert(std::make_pair(key, createObject(key)));
			
			Assert(res->second);

			return (res.first)->second;
		}

		// true of object found and erased
		bool erase(const Key& key)
		{
			Container::iterator it(mObjects.find(key));
			if (it != mObjects.end())
			{
				destroyObject(it->second);
				mObjects.erase(res);
				return true;
			}
			return false;
		}

		// NULL if object not found
		Object* find(const Key& key) 
		{
			Container::iterator it(mObjects.find(key));
			if (it == mObjects.end())
				return NULL;
			
			return &it->second;
		}

		// true if insertion made
		bool insert(const Key& key, const Object& object)
		{
			std::pair<Container::iterator, bool> res = 
				mObjects.insert(std::make_pair(key, object));
			return res.second;
		}

		void clear(void)
		{
			for (Container::iterator it = mObjects.begin(); it != mObjects.end(); ++it)
				destroyObject(it->second);
			mObjects.clear();
		}

		const Container& getObjects(void) const {	return mObjects; }
					Container& getObjects(void)				{	return mObjects; }
		
		virtual Object createObject(const Key& key) = 0;
		virtual void destroyObject(Object& obj) = 0;
		
	protected:

		Container mObjects;
		bool mAutoClearContents;
	};

} // namespace gr

#endif // OBJECT_CACHE_H
