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

//////////////////////////////////////////////////////////////////////
//
// AkKeyArray.h
//
//////////////////////////////////////////////////////////////////////
#ifndef _KEYARRAY_H_
#define _KEYARRAY_H_

#include <AK/Tools/Common/AkArray.h>
#include "AkKeyDef.h"
#include "AkPoolSizes.h"

// The Key list is simply a list that may be referenced using a key
// NOTE : 
template <class T_KEY, class T_ITEM, AkUInt32 TGrowBy = ( DEFAULT_POOL_BLOCK_SIZE / sizeof(MapStruct<T_KEY, T_ITEM>) ) > 
class CAkKeyArray : public AkArray< MapStruct<T_KEY, T_ITEM>, const MapStruct<T_KEY, T_ITEM>&, ArrayPoolDefault, TGrowBy>
{
public:
//====================================================================================================
// Return NULL if the Key does not exisis
// Return T_ITEM* otherwise
//====================================================================================================
	T_ITEM* Exists( T_KEY in_Key )
	{
		typename CAkKeyArray< T_KEY, T_ITEM, TGrowBy >::Iterator it = this->FindEx( in_Key );
		return ( it != this->End() ) ? &(it.pItem->item) : NULL;
	}

public:
//====================================================================================================
// Sets the item referenced by the specified key and item
// Return AK_Fail if the list max size was exceeded
//====================================================================================================
	T_ITEM * Set( T_KEY in_Key, T_ITEM in_Item )
	{
		T_ITEM* pSearchedItem = Exists( in_Key );
		if( pSearchedItem )
		{
			*pSearchedItem = in_Item;
		}
		else
		{
			MapStruct<T_KEY, T_ITEM> * pStruct = this->AddLast();
			if ( pStruct )
			{
				pStruct->key = in_Key;
				pStruct->item = in_Item;
				pSearchedItem = &( pStruct->item );
			}
		}
		return pSearchedItem;
	}

	T_ITEM * Set( T_KEY in_Key )
	{
		T_ITEM* pSearchedItem = Exists( in_Key );
		if( !pSearchedItem )
		{
			MapStruct<T_KEY, T_ITEM> * pStruct = this->AddLast();
			if ( pStruct )
			{
				pStruct->key = in_Key;
				pSearchedItem = &( pStruct->item );
			}
		}
		return pSearchedItem;
	}

	typename CAkKeyArray< T_KEY, T_ITEM, TGrowBy >::Iterator FindEx( T_KEY in_Item ) const
	{
		typename CAkKeyArray< T_KEY, T_ITEM, TGrowBy >::Iterator it = this->Begin();

		typename CAkKeyArray< T_KEY, T_ITEM, TGrowBy >::Iterator itEnd = this->End();
		for ( ; it != itEnd; ++it )
		{
			if ( (*it).key == in_Item )
				break;
		}

		return it;
	}

//====================================================================================================
//	Remove the item referenced by the specified key
//====================================================================================================

	void Unset( T_KEY in_Key )
	{
		typename CAkKeyArray< T_KEY, T_ITEM, TGrowBy >::Iterator it = FindEx( in_Key );
		if( it != this->End() )
		{
			this->Erase( it );
		}
	}

//====================================================================================================
//	More efficient version of Unset when order is unimportant
//====================================================================================================

	void UnsetSwap( T_KEY in_Key )
	{
		typename CAkKeyArray< T_KEY, T_ITEM, TGrowBy >::Iterator it = FindEx( in_Key );
		if( it != this->End() )
		{
			this->EraseSwap( it );
		}
	}
};

#endif //_KEYARRAY_H_
