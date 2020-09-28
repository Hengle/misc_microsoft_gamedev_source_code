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
// AkKeyList.h
//
//////////////////////////////////////////////////////////////////////
#ifndef _KEYLIST_H_
#define _KEYLIST_H_

#include "AkList2.h"
#include "AkKeyDef.h"

// The Key list is simply a list that may be referenced using a key
// NOTE : 
template <class T_KEY, class T_ITEM, ListAllocFlag TAllocFlag> 
class CAkKeyList : public CAkList2< MapStruct<T_KEY, T_ITEM>, const MapStruct<T_KEY, T_ITEM>&, TAllocFlag >
{
public:
//====================================================================================================
// Return NULL if the Key does not exisis
// Return T_ITEM* otherwise
//====================================================================================================
	T_ITEM* Exists(T_KEY in_Key)
	{
		typename CAkKeyList<T_KEY,T_ITEM,TAllocFlag>::ListItem* pCurrent = this->m_pFirst;
		while(pCurrent != NULL)
		{
			if(pCurrent->Item.key == in_Key)
			{
				return &(pCurrent->Item.item);
			}
			pCurrent = pCurrent->pNextListItem;
		}
		return NULL;
	}

public:
//====================================================================================================
// Sets the item referenced by the specified key and item
// Return AK_Fail if the list max size was exceeded
//====================================================================================================
	T_ITEM * Set(T_KEY in_Key, T_ITEM in_Item)
	{
		T_ITEM* pSearchedItem = Exists(in_Key);
		if(pSearchedItem)
		{
			*pSearchedItem = in_Item;
		}
		else
		{
			//pSearchedItem = AddKeylistItem(in_Key, in_Item);
			MapStruct<T_KEY, T_ITEM> * pStruct = this->AddLast();
			if ( pStruct )
			{
				pStruct->key = in_Key;
				pStruct->item = in_Item;
				pSearchedItem = &(pStruct->item);
			}
		}
		return pSearchedItem;
	}

	T_ITEM * Set(T_KEY in_Key)
	{
		T_ITEM* pSearchedItem = Exists(in_Key);
		if(!pSearchedItem)
		{
			MapStruct<T_KEY, T_ITEM> * pStruct = this->AddLast();
			if ( pStruct )
			{
				pStruct->key = in_Key;
				pSearchedItem = &(pStruct->item);
			}
		}
		return pSearchedItem;
	}

	typename CAkKeyList<T_KEY,T_ITEM,TAllocFlag>::IteratorEx FindEx(T_KEY in_Key)
	{
		typename CAkKeyList<T_KEY,T_ITEM,TAllocFlag>::IteratorEx it = this->BeginEx();
		for ( ; it != this->End(); ++it )
		{
			if ( (*it).key == in_Key )
			{
				break;
			}
		}
		return it;
	}
//====================================================================================================
//	Remove the item referenced by the specified key
//====================================================================================================

	void Unset(T_KEY in_Key)
	{
		typename CAkKeyList<T_KEY,T_ITEM,TAllocFlag>::IteratorEx it = this->BeginEx();
		for ( ; it != this->End(); ++it )
		{
			if ( (*it).key == in_Key )
			{
				Erase( it );
				break;
			}
		}
	}
};

#endif //_KEYLIST_H_
