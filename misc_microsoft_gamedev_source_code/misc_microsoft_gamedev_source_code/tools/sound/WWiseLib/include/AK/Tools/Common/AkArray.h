//////////////////////////////////////////////////////////////////////
//
// Copyright 2006-2008 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#ifndef _AKARRAY_H
#define _AKARRAY_H

#include <AK/Tools/Common/AkObject.h>
#include <AK/Tools/Common/AKAssert.h>
#include <new>
#undef new

extern AkMemPoolId g_DefaultPoolId;
extern AkMemPoolId g_LEngineDefaultPoolId;

#define AK_DEFINE_ARRAY_POOL( _name_, _poolID_ )	\
struct _name_										\
{													\
	static AkMemPoolId Get()						\
	{												\
		return _poolID_;							\
	}												\
};

AK_DEFINE_ARRAY_POOL( ArrayPoolDefault, g_DefaultPoolId )
AK_DEFINE_ARRAY_POOL( ArrayPoolLEngineDefault, g_LEngineDefaultPoolId )

/// Specific implementation of array
template <class T, class ARG_T, class U_POOL, unsigned long TGrowBy = 1> class AkArray
{
public:
	/// Constructor
	AkArray()
		: m_pItems( NULL )
		, m_pItemsEnd( NULL )
		, m_ulReserved( 0 )
	{
	}

	/// Destructor
	~AkArray()
	{
		AKASSERT( m_pItems == NULL );
		AKASSERT( m_pItemsEnd == NULL );
		AKASSERT( m_ulReserved == 0 );
	}

	/// Iterator
	struct Iterator
	{
		T* pItem;	///< Pointer to the item in the array.

		/// ++ operator
		Iterator& operator++()
		{
			AKASSERT( pItem );
			++pItem;
			return *this;
		}

		/// -- operator
        Iterator& operator--()
		{
			AKASSERT( pItem );
			--pItem;
			return *this;
		}

		/// * operator
		T& operator*()
		{
			AKASSERT( pItem );
			return *pItem;
		}

		/// == operator
		bool operator ==( const Iterator& in_rOp ) const
		{
			return ( pItem == in_rOp.pItem );
		}

		/// != operator
		bool operator !=( const Iterator& in_rOp ) const
		{
			return ( pItem != in_rOp.pItem );
		}
	};

	/// Returns the iterator to the first item of the array, will be End() if the array is empty.
	Iterator Begin() const
	{
		Iterator returnedIt;
		returnedIt.pItem = m_pItems;
		return returnedIt;
	}

	/// Returns the iterator to the end of the array
	Iterator End() const
	{
		Iterator returnedIt;
		returnedIt.pItem = m_pItemsEnd;
		return returnedIt;
	}

	/// Returns the iterator th the specified item, will be End() if the item is not found
	Iterator FindEx( ARG_T in_Item ) const
	{
		Iterator it = Begin();

		for ( Iterator itEnd = End(); it != itEnd; ++it )
		{
			if ( *it == in_Item )
				break;
		}

		return it;
	}

	/// Erase the specified iterator from the array
	Iterator Erase( Iterator& in_rIter )
	{
		AKASSERT( m_pItems != NULL );

		// Move items by 1

		T * pItemLast = m_pItemsEnd - 1;

		for ( T * pItem = in_rIter.pItem; pItem < pItemLast; pItem++ )
			pItem[ 0 ] = pItem[ 1 ];

		// Destroy the last item

		pItemLast->~T();

		m_pItemsEnd--;

		return in_rIter;
	}

	/// Erase the item at the specified index
    void Erase( unsigned int in_uIndex )
	{
		AKASSERT( m_pItems != NULL );

		// Move items by 1

		T * pItemLast = m_pItemsEnd - 1;

		for ( T * pItem = m_pItems+in_uIndex; pItem < pItemLast; pItem++ )
			pItem[ 0 ] = pItem[ 1 ];

		// Destroy the last item

		pItemLast->~T();

		m_pItemsEnd--;
	}

	/// Erase the specified iterator in the array. but it dos not guarantee the ordering in the array.
	/// This version should be used only when the order in the array is not an issue.
    Iterator EraseSwap( Iterator& in_rIter )
	{
		AKASSERT( m_pItems != NULL );

        if ( Length( ) > 1 )
        {
            // Swap last item with this one.
            *in_rIter.pItem = Last( );
        }
        
        // Destroy.
        AKASSERT( Length( ) > 0 );
        Last( ).~T();

        m_pItemsEnd--;

		return in_rIter;
	}

	/// Pre-Allocate a number of spaces in the array
	AKRESULT Reserve( AkUInt32 in_ulReserve )
	{
		AKASSERT( m_pItems == NULL );
		AKASSERT( in_ulReserve || TGrowBy );

		if ( in_ulReserve )
		{
			m_pItemsEnd = m_pItems = (T *) AkAlloc( U_POOL::Get(), sizeof( T ) * in_ulReserve );
			if ( m_pItems == NULL )
				return AK_Fail;

			m_ulReserved = in_ulReserve;
		}

		return AK_Success;
	}

	/// Term the array. Must be called before destroying the object.
	void Term()
	{
		if ( m_pItems )
		{
			RemoveAll();
			AkFree( U_POOL::Get(), m_pItems );
			m_pItems = NULL;
			m_pItemsEnd = NULL;
			m_ulReserved = 0;
		}
	}

	/// Returns the numbers of items in the array.
	AkUInt32 Length() const
	{
		return (AkUInt32)( m_pItemsEnd - m_pItems );
	}

	/// Returns true if the number items in the array is 0, false otherwise.
	bool IsEmpty() const
	{
		return m_pItemsEnd == m_pItems;
	}
	
	/// Returns a pointer to the specified item in the list if it exists, NULL if not found.
	T* Exists(ARG_T in_Item) const
	{
		Iterator it = FindEx( in_Item );
		return ( it != End() ) ? it.pItem : NULL;
	}

	/// Add an item in the array, without filling it.
	/// Returns a pointer to the location to be filled.
	T * AddLast()
	{
		size_t cItems = Length();

#if( defined WIN32 || defined XBOX360 )
#pragma warning( push )
#pragma warning( disable : 4127 )
#endif
		if ( ( cItems >= m_ulReserved ) && TGrowBy > 0 )
		{
			if ( !GrowArray() ) 
				return NULL;
		}
#if( defined WIN32 || defined XBOX360 )
#pragma warning( pop )
#endif

		// have we got space for a new one ?
		if(  cItems < m_ulReserved )
		{
			::new( m_pItemsEnd ) T; // placement new
			return m_pItemsEnd++;
		}

		return NULL;
	}

	/// Add an item in the array, and fills it with the provided item.
	T * AddLast(ARG_T in_rItem)
	{
		T * pItem = AddLast();
		if ( pItem )
			*pItem = in_rItem;
		return pItem;
	}

	/// Returns a reference to the last item in the array.
	T& Last()
	{
		AKASSERT( m_pItemsEnd - m_pItems );

		return *( m_pItemsEnd - 1 );
	}

	/// Removes the last item from the array.
    void RemoveLast()
    {
        AKASSERT( m_pItemsEnd - m_pItems );
        ( m_pItemsEnd - 1 )->~T();
        m_pItemsEnd--;
    }

	/// Removes the specified item if found in the array.
	AKRESULT Remove(ARG_T in_rItem)
	{
		Iterator it = FindEx( in_rItem );
		if ( it != End() )
		{
			Erase( it );
			return AK_Success;
		}

		return AK_Fail;
	}

	/// Fast remove of the specified item in the array.
	/// This method do not guarantee keeping ordering of the array.
	AKRESULT RemoveSwap(ARG_T in_rItem)
	{
		Iterator it = FindEx( in_rItem );
		if ( it != End() )
		{
			EraseSwap( it );
			return AK_Success;
		}

		return AK_Fail;
	}

	/// Removes all items in the array
	void RemoveAll()
	{
		for ( Iterator it = Begin(), itEnd = End(); it != itEnd; ++it )
			(*it).~T();
		m_pItemsEnd = m_pItems;
	}

	/// Operator [], return a reference to the specified index.
    T& operator[](unsigned int uiIndex)
    {
        AKASSERT( m_pItems );
        AKASSERT( uiIndex >= 0 &&
                  uiIndex < Length() );
        return m_pItems[uiIndex];
    }

	/// Insert an item at the specified position without filling it.
	/// Returns the pointer to the item to be filled.
	T * Insert(unsigned int in_uIndex)
	{
        AKASSERT( m_pItems );
        AKASSERT( in_uIndex >= 0 &&
                  in_uIndex <= Length() );

		size_t cItems = Length();

#if( defined WIN32 || defined XBOX360 )
#pragma warning( push )
#pragma warning( disable : 4127 )
#endif
		if ( ( cItems >= m_ulReserved ) && TGrowBy > 0 )
		{
			if ( !GrowArray() ) 
				return NULL;
		}
#if( defined WIN32 || defined XBOX360 )
#pragma warning( pop )
#endif

		// have we got space for a new one ?
		if(  cItems < m_ulReserved )
		{
			::new( m_pItemsEnd ) T; // placement new

			// Move items by 1

			T * pItemLast = m_pItemsEnd++;
			for ( T * pItem = pItemLast; pItem > ( m_pItems + in_uIndex ); --pItem )
				pItem[ 0 ] = pItem[ -1 ];

			// Reinitialize item at index

			( m_pItems + in_uIndex )->~T();
			::new( m_pItems + in_uIndex ) T; // placement new

			return m_pItems + in_uIndex;
		}

		return NULL;
	}

	/// Resize the array.
	bool GrowArray( AkUInt32 in_uGrowBy = TGrowBy )
	{
		AKASSERT( in_uGrowBy );
		
		AkUInt32 ulNewReserve = m_ulReserved + in_uGrowBy;
		T * pNewItems = (T *) AkAlloc( U_POOL::Get(), sizeof( T ) * ulNewReserve );
		if ( !pNewItems ) 
			return false;

		// Copy all elements in new array, destroy old ones

		size_t cItems = Length();

		if ( m_pItems ) 
		{
			for ( size_t i = 0; i < cItems; ++i )
			{
				::new( pNewItems + i ) T; // placement new

				pNewItems[ i ] = m_pItems[ i ];
	            
				m_pItems[ i ].~T();
			}

			AkFree( U_POOL::Get(), m_pItems );
		}

		m_pItems = pNewItems;
		m_pItemsEnd = pNewItems + cItems;
		m_ulReserved = ulNewReserve;

		return true;
	}

	/// Resize the array to the specified size.
	bool Resize(AkUInt32 in_uiSize)
	{
		size_t cItems = Length();
		if (in_uiSize < cItems)
		{
			//Destroy superfluous elements
			for(size_t i = in_uiSize - 1 ; i < cItems; i++)
			{
				m_pItems[ i ].~T();
			}
			m_pItemsEnd = m_pItems + in_uiSize;
			return true;
		}

		if ( in_uiSize >= m_ulReserved )
		{
			if ( !GrowArray(in_uiSize - cItems) ) 
				return false;
		}

		//Create the missing items.
		for(size_t i = cItems; i < in_uiSize; i++)
		{
			::new( m_pItems + i ) T; // placement new
		}

		m_pItemsEnd = m_pItems + in_uiSize;
		return true;
	}

protected:
	AkUInt32	m_ulReserved;	///< how many we can have at most (currently allocated).
	T *         m_pItems;		///< pointer to the beginning of the array.
	T *         m_pItemsEnd;	///< pointer to the next allocatable item in the array.
};

#endif
