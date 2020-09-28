/***********************************************************************
  The content of this file includes source code for the sound engine
  portion of the AUDIOKINETIC Wwise Technology and constitutes "Level
  Two Source Code" as defined in the Source Code Addendum attached
  with this file.  Any use of the Level Two Source Code shall be
  subject to the terms and conditions outlined in the Source Code
  Addendum and the End User License Agreement for Wwise(R).

  Version: v2008.2.1  Build: 2821
  Copyright (c) 2006-2008 Audiokinetic Inc.
 ***********************************************************************/

// AkListBare.h

#ifndef _AKLISTBARE_H
#define _AKLISTBARE_H

// this one lets you define the structure
// only requirement is that T must have member pNextItem (if using the default AkListBareNextItem policy struct).
// client is responsible for allocation/deallocation of T.

// WATCH OUT !
// - remember that removeall/term can't delete the elements for you.
// - be sure to destroy elements AFTER removing them from the list, as remove will
// access members of the element.

template <class T> struct AkListBareNextItem
{
	static AkForceInline T *& Get( T * in_pItem ) 
	{
		return in_pItem->pNextItem;
	}
};

template <class T, class U_NEXTITEM = AkListBareNextItem< T > > class AkListBare
{
public:
	struct Iterator
	{
		T* pItem;

		inline Iterator& operator++()
		{
			AKASSERT( pItem );
			pItem = U_NEXTITEM::Get( pItem );
			return *this;
		}

		inline T * operator*()
		{
			AKASSERT( pItem );
			return pItem;
		}

		bool operator !=( const Iterator& in_rOp ) const
		{
			return ( pItem != in_rOp.pItem );
		}
		
		bool operator ==( const Iterator& in_rOp ) const
		{
			return ( pItem == in_rOp.pItem );
		}
	};

	// The IteratorEx iterator is intended for usage when a possible erase may occurs
	// when simply iterating trough a list, use the simple Iterator, it is faster and lighter.
	struct IteratorEx : public Iterator
	{
		T* pPrevItem;

		IteratorEx& operator++()
		{
			AKASSERT( this->pItem );
			
			pPrevItem = this->pItem;
			this->pItem = U_NEXTITEM::Get( this->pItem );
			
			return *this;
		}
	};

	IteratorEx Erase( const IteratorEx& in_rIter )
	{
		IteratorEx returnedIt;
		returnedIt.pItem = U_NEXTITEM::Get( in_rIter.pItem );
		returnedIt.pPrevItem = in_rIter.pPrevItem;

		RemoveItem( in_rIter.pItem, in_rIter.pPrevItem );

		return returnedIt;
	}

    IteratorEx Insert( const IteratorEx& in_rIter,
                       T * in_pItem )
	{
        IteratorEx returnedIt;
		AddItem( in_pItem, in_rIter.pItem, in_rIter.pPrevItem );
        returnedIt = in_rIter;
        return returnedIt;
	}

	inline Iterator End()
	{
		Iterator returnedIt;
		returnedIt.pItem = NULL;
		return returnedIt;
	}

	inline IteratorEx BeginEx()
	{
		IteratorEx returnedIt;
		
		returnedIt.pItem = m_pFirst;
		returnedIt.pPrevItem = NULL;
		
		return returnedIt;
	}

	inline Iterator Begin()
	{
		Iterator returnedIt;
		
		returnedIt.pItem = m_pFirst;
		
		return returnedIt;
	}

	inline IteratorEx FindEx( T *  in_pItem )
	{
		IteratorEx it = BeginEx();
		for ( ; it != End(); ++it )
		{
			if ( it.pItem == in_pItem )
				break;
		}

		return it;
	}

	AkListBare()
	{
	}

	~AkListBare()
	{
	}

	AKRESULT Init()
	{
		m_pFirst = NULL;
		m_pLast = NULL;
		m_ulNumListItems = 0;

		return AK_Success;
	}

	void Term()
	{
		RemoveAll();
	}

	void AddFirst( T * in_pItem )
	{
		if ( m_pFirst == NULL )
		{
			m_pFirst = m_pLast = in_pItem;
			U_NEXTITEM::Get( in_pItem ) = NULL;
		}
		else
		{
			U_NEXTITEM::Get( in_pItem ) = m_pFirst;
			m_pFirst = in_pItem;
		}

		++m_ulNumListItems;
	}

	void AddLast( T * in_pItem )
	{
		U_NEXTITEM::Get( in_pItem ) = NULL;

		if ( m_pLast == NULL )
		{
			m_pFirst = m_pLast = in_pItem;
		}
		else
		{
			U_NEXTITEM::Get( m_pLast ) = in_pItem;
			m_pLast = in_pItem;
		}

		++m_ulNumListItems;
	}

	AKRESULT Remove( T * in_pItem )
	{
		IteratorEx it = FindEx( in_pItem );
		if ( it != End() )
		{
			Erase( it );
			return AK_Success;
		}

		return AK_Fail;
	}

	AKRESULT RemoveFirst()
	{
		if( m_pFirst == NULL )
			return AK_Fail;

		if ( U_NEXTITEM::Get( m_pFirst ) == NULL )
		{
			m_pFirst = NULL;
			m_pLast = NULL;
		}
		else
		{
			m_pFirst = U_NEXTITEM::Get( m_pFirst );
		}

		--m_ulNumListItems;

		return AK_Success;
	}

	AkForceInline void RemoveAll()
	{
		// Items being externally managed, all we need to do here is clear our members.
		m_pFirst = NULL;
		m_pLast = NULL;
		m_ulNumListItems = 0;
	}

	AkForceInline T * First()
	{
		return m_pFirst;
	}

	AkForceInline T * Last()
	{
		return m_pLast;
	}

	AkForceInline unsigned int Length() const
	{
		return m_ulNumListItems;
	}

	AkForceInline bool IsEmpty() const
	{
		return m_ulNumListItems == 0;
	}

	void RemoveItem( T * in_pItem, T * in_pPrevItem )
	{
		// Is it the first one ?

		if( in_pItem == m_pFirst )
		{
			// new first one is the next one
			m_pFirst = U_NEXTITEM::Get( in_pItem );
		}
		else
		{
			// take it out of the used space
			U_NEXTITEM::Get( in_pPrevItem ) = U_NEXTITEM::Get( in_pItem );
		}

		// Is it the last one ?

		if( in_pItem == m_pLast )
		{
			// new last one is the previous one
			m_pLast = in_pPrevItem;
		}

		--m_ulNumListItems;
	}

	void AddItem( T * in_pItem, T * in_pNextItem, T * in_pPrevItem )
	{
		U_NEXTITEM::Get( in_pItem ) = in_pNextItem;

		if ( in_pPrevItem == NULL )
            m_pFirst = in_pItem;
        else
            U_NEXTITEM::Get( in_pPrevItem ) = in_pItem;

        // Update tail.
        // Note: will never occur since it is used with iteratorEx (have EndEx?).
        if ( in_pNextItem == NULL )   
            m_pLast = in_pItem;

	    ++m_ulNumListItems;
	}

protected:
	T *				m_pFirst;					// top of list
	T *				m_pLast;					// bottom of list
	unsigned int	m_ulNumListItems;			// how many we have
};

#endif // _AKLISTBARE_H
