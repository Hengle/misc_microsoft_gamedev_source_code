/* 
 * 
 * Confidential Information of Telekinesys Research Limited (t/a Havok).  Not for disclosure or distribution without Havok's
 * prior written consent.This software contains code, techniques and know-how which is confidential and proprietary to Havok.
 * Level 2 and Level 3 source code contains trade secrets of Havok. Havok Software (C) Copyright 1999-2007 Telekinesys Research Limited t/a Havok. All Rights Reserved. Use of this software is subject to the terms of an end user license agreement.
 * 
 */

#ifndef HKBASE_STRINGMAP_H
#define HKBASE_STRINGMAP_H

#include <Common/Base/Container/StringMap/hkStringMapBase.h>

/// Map strings to integers or pointers.
/// Note that neither the keys nor values are copied so the values
/// must exist for the lifetime of this object.
template <typename V>
class hkStringMap
{
	public:

		HK_DECLARE_NONVIRTUAL_CLASS_ALLOCATOR( HK_MEMORY_CLASS_MAP, hkStringMap );

			/// Iterator class
			/// All iterators are invalidated after a mutating operation. i.e. insertion,removal
		typedef hkStringMapBase::Iterator Iterator;

		hkStringMap()
		{
		}

			///	Insert key with associated value val.
			/// If key already exists it is overwritten. The key string is not
			/// copied and must exist for the lifetime of the entry.
		HK_FORCE_INLINE void insert( const char* key, V val )
		{
			m_map.insert( key, hkUlong(val) );
		}

			/// Return the iterator associated with key. Check with isValid().
		HK_FORCE_INLINE Iterator findKey( const char* key ) const
		{
			return m_map.findKey( key );
		}

			/// Return if this map contains the given key.
		HK_FORCE_INLINE hkBool hasKey( const char* key ) const
		{
			return m_map.hasKey( key );
		}

			/// Return the value associated with key or def if not present.
		HK_FORCE_INLINE V getWithDefault( const char* key, V def ) const
		{
			return (V)m_map.getWithDefault( key, hkUlong(def) );
		}

			/// If key present, write value into out and return HK_SUCCESS. Else return HK_FAILURE.
		hkResult get( const char* key, V* out ) const
		{
			hkUlong tmp;
			if( m_map.get( key, &tmp ) == HK_SUCCESS )
			{
				*out = V(tmp);
				return HK_SUCCESS;
			}
			return HK_FAILURE;

		}

			/// Remove pair at "it".
		void remove( Iterator it )
		{
			m_map.remove( it );
		}

			/// If key present, remove it and return HK_SUCCESS. Otherwise return HK_FAILURE.
		hkResult remove( const char* key )
		{
			return m_map.remove( key );
		}

			/// Return the number of elements in this map.
		int getSize() const
		{
			return m_map.getSize();
		}

			/// Perform internal consistency check.
		hkBool isOk() const
		{
			return m_map.isOk();
		}

			/// Get an iterator over the keys of this map.
		Iterator getIterator() const
		{
			return m_map.getIterator();
		}

			/// Get the key at iterator i.
			/// Do not modify this key directly. If you must change a key, remove and re-add it.
		char* getKey( Iterator i ) const
		{
			return (char*)m_map.getKey(i);
		}

			/// Get the value at iterator i.
		V getValue( Iterator i ) const
		{
			return (V)m_map.getValue(i);
		}

			/// Overwrite the value at iterator i.
		void setValue( Iterator i, V v )
		{
			m_map.setValue( i, hkUlong(v) );
		}

			/// Get the next iterator after i.
		Iterator getNext( Iterator i ) const
		{
			return m_map.getNext(i);
		}

			/// Return if the iterator has reached the end.
		hkBool isValid( Iterator i ) const
		{
			return m_map.isValid(i);
		}

			/// Remove all keys from the map.
		void clear()
		{
			m_map.clear();
		}

			/// Swap all data with another map.
		void swap( hkStringMap& other )
		{
			m_map.swap(other.m_map);
		}

	protected:

		hkStringMapBase m_map;
};

#endif // HKBASE_STRINGMAP_H

/*
* Havok SDK - PUBLIC RELEASE, BUILD(#20070919)
*
* Confidential Information of Havok.  (C) Copyright 1999-2007 
* Telekinesys Research Limited t/a Havok. All Rights Reserved. The Havok
* Logo, and the Havok buzzsaw logo are trademarks of Havok.  Title, ownership
* rights, and intellectual property rights in the Havok software remain in
* Havok and/or its suppliers.
*
* Use of this software for evaluation purposes is subject to and indicates 
* acceptance of the End User licence Agreement for this product. A copy of 
* the license is included with this software and is also available from salesteam@havok.com.
*
*/
