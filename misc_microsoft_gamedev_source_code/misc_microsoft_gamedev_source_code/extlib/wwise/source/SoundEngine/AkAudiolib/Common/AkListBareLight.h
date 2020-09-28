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

// AkListBareLight.h

#ifndef _AKLISTBARELIGHT_H
#define _AKLISTBARELIGHT_H

// this one lets you define the structure
// only requirement is that T must have member pNextLightItem.
// client is responsible for allocation/deallocation of T.

// WATCH OUT !
// - remember that removeall/term can't delete the elements for you.
// - be sure to destroy elements AFTER removing them from the list, as remove will
// access members of the element.
// WARNING : Each AkListBareLight item can be used in only one AkListBareLight at 
//           once since the member pNextLightItem cannot be re-used.

template <class T> class AkListBareLight
{
public:
	struct Iterator
	{
		T* pItem;

		inline Iterator& operator++()
		{
			AKASSERT( pItem );
			pItem = pItem->pNextLightItem;
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
			this->pItem = this->pItem->pNextLightItem;
			
			return *this;
		}
	};

	IteratorEx Erase( const IteratorEx& in_rIter )
	{
		IteratorEx returnedIt;
		returnedIt.pItem = in_rIter.pItem->pNextLightItem;
		returnedIt.pPrevItem = in_rIter.pPrevItem;

		RemoveItem( in_rIter.pItem, in_rIter.pPrevItem );

		return returnedIt;
	}

    IteratorEx Insert( const IteratorEx& in_rIter,
                       T * in_pItem )
	{
        IteratorEx returnedIt;
		if ( AddItem( in_pItem, in_rIter.pItem, in_rIter.pPrevItem ) == NULL )
        {            
            returnedIt.pPrevItem = in_rIter.pPrevItem;
            returnedIt.pItem = NULL;
            return returnedIt;
        }
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

	AkListBareLight()
	{
	}

	~AkListBareLight()
	{
	}

	AKRESULT Init()
	{
		m_pFirst = NULL;

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
			m_pFirst = in_pItem;
			in_pItem->pNextLightItem = NULL;
		}
		else
		{
			in_pItem->pNextLightItem = m_pFirst;
			m_pFirst = in_pItem;
		}
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

		if ( m_pFirst->pNextLightItem == NULL )
		{
			m_pFirst = NULL;
		}
		else
		{
			m_pFirst = m_pFirst->pNextLightItem;
		}

		return AK_Success;
	}

	AkForceInline void RemoveAll()
	{
		// Items being externally managed, all we need to do here is clear our members.
		m_pFirst = NULL;
	}

	AkForceInline T * First()
	{
		return m_pFirst;
	}

	AkForceInline bool IsEmpty() const
	{
		return ( m_pFirst == NULL );
	}

	void RemoveItem( T * in_pItem, T * in_pPrevItem )
	{
		// Is it the first one ?

		if( in_pItem == m_pFirst )
		{
			// new first one is the next one
			m_pFirst = in_pItem->pNextLightItem;
		}
		else
		{
			// take it out of the used space
			in_pPrevItem->pNextLightItem = in_pItem->pNextLightItem;
		}
	}

	void AddItem( T * in_pItem, T * in_pNextItem, T * in_pPrevItem )
	{
		in_pItem->pNextLightItem = in_pNextItem;

		if ( in_pPrevItem == NULL )
            m_pFirst = in_pItem;
        else
            in_pPrevItem->pNextLightItem = in_pItem;

	}

protected:
	T *				m_pFirst;					// top of list
};

#endif // _AKLISTBARELIGHT_H
