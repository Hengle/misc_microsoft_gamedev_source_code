/***********************************************************************
  The content of this file includes source code for the sound engine
  portion of the AUDIOKINETIC Wwise Technology and constitutes "Level
  Two Source Code" as defined in the Source Code Addendum attached
  with this file.  Any use of the Level Two Source Code shall be
  subject to the terms and conditions outlined in the Source Code
  Addendum and the End User License Agreement for Wwise(R).

  Version: v2008.2.1 Patch 4  Build: 2821
  Copyright (c) 2006-2008 Audiokinetic Inc.
 ***********************************************************************/

//////////////////////////////////////////////////////////////////////
//
// AkParentNode.h
//
//////////////////////////////////////////////////////////////////////
#ifndef _AK_PARENT_NODE_H_
#define _AK_PARENT_NODE_H_

#include "AkAudioLibExport.h"
#include "AkKeyArray.h"
#include "AkParameterNode.h"
#include "AkAudioLibIndex.h"
#include "AkBankFloatConversion.h"

extern AkMemPoolId g_DefaultPoolId;

typedef CAkKeyArray<AkUniqueID, CAkAudioNode*> AkMapChildID;

template <class T> class CAkParentNode : public T
{
public:
	//Constructor
	CAkParentNode(AkUniqueID in_ulID)
	:T(in_ulID)
	{
	}

	//Destructor
	virtual ~CAkParentNode()
	{
		m_mapChildId.Term();
	}

	AKRESULT Init()
	{
		return T::Init();
	}

	// Get the total number of children
    //
    // Return - AkUInt16 - Number of inputs
    virtual AkUInt16 Children()
	{
		return static_cast<AkUInt16>( m_mapChildId.Length() );
	}

	// Check if the specified child can be connected
    //
    // Return - bool - True if possible
    virtual AKRESULT CanAddChild(
        CAkAudioNode * in_pAudioNode // Audio node to connect on
        ) = 0;

    // Add a Child
    //
    // Return - AKRESULT - AK_NotCompatible : The output is not compatible with the Node
	//					 - AK_Success if everything succeed
    //                   - AK_MaxReached if the maximum of Input is reached
    virtual AKRESULT AddChild(AkUniqueID in_ulID)
	{
		AKASSERT(g_pIndex);
		if(!in_ulID)
		{
			return AK_InvalidID;
		}
		
		CAkAudioNode* pAudioNode = g_pIndex->m_idxAudioNode.GetPtrAndAddRef(in_ulID);
		if ( !pAudioNode )
			return AK_IDNotFound;

		AKRESULT eResult = CanAddChild(pAudioNode);
		if(eResult == AK_Success)
		{
			if(! m_mapChildId.Set( in_ulID, pAudioNode ) )
			{
				eResult = AK_Fail;
			}
			else
			{
				pAudioNode->Parent(this);
				this->AddRef();
			}
		}
		pAudioNode->Release();
		return eResult;
	}

	// Remove a child from the list
	//
    // Return - AKRESULT - AK_Success if all went right
    virtual AKRESULT RemoveChild(AkUniqueID in_ulID)
	{
		// IMPORTANT NOTICE :: CAkRanSeqCntr Has ist own version of RemoveChild() implemented, not calling this one
		// The reason for that is that the this->Release() must be the last line of the last function called on the
		// Object.
		AKASSERT(in_ulID);
		AKRESULT eResult = AK_Success;
		CAkAudioNode** l_pANPtr = m_mapChildId.Exists(in_ulID);
		if( l_pANPtr )
		{
			(*l_pANPtr)->Parent( NULL );
			m_mapChildId.Unset( in_ulID );
			this->Release();
		}
		return eResult;
	}

	// Remove all inputs
	//
    // Return - AKRESULT - AK_Success if all went right
	virtual AKRESULT RemoveAllChildren()
	{
		while( m_mapChildId.Length() > 0 )
		{
			RemoveChild( m_mapChildId.Begin().pItem->key );
		}
		return AK_Success;
	}

	AKRESULT SetChildren( AkUInt8*& io_rpData, AkUInt32& io_rulDataSize )
	{
		AKRESULT eResult = AK_Success;
		//Process Child list
		AkUInt32 ulNumChilds = READBANKDATA( AkUInt32, io_rpData, io_rulDataSize );
		if( ulNumChilds )
		{
			eResult = m_mapChildId.Reserve( ulNumChilds );
			if( eResult != AK_Success )
				return eResult;
				
			for( AkUInt32 i = 0; i < ulNumChilds; ++i )
			{
				AkUInt32 ulChildID = READBANKDATA( AkUInt32, io_rpData, io_rulDataSize );
				eResult = AddChild( ulChildID );
				if( eResult != AK_Success )
				{
					break;
				}
			}
		}
		return eResult;
	}

	virtual AKRESULT GetChildren( AkUInt32& io_ruNumItems, AkObjectInfo* out_aObjectInfos, AkUInt32& in_uIndex_Out, AkUInt32 in_uDepth )
	{
		AKRESULT eResult = AK_Success;

		AkMapChildID::Iterator iter = m_mapChildId.Begin();
		for( ; iter != m_mapChildId.End(); ++iter )
		{
			if( io_ruNumItems != 0 )
			{
				out_aObjectInfos[in_uIndex_Out].objID = (*iter).key;
				out_aObjectInfos[in_uIndex_Out].parentID = (*iter).item->Parent()->ID();
				out_aObjectInfos[in_uIndex_Out].iDepth = in_uDepth;
			}

			++in_uIndex_Out;
			if( io_ruNumItems != 0 && 
				( in_uIndex_Out >= io_ruNumItems ) )
				break; //exit for() loop
			else
				eResult = (*iter).item->GetChildren( io_ruNumItems, out_aObjectInfos, in_uIndex_Out, ++in_uDepth );

			if( eResult != AK_Success )
				break;
		}

		return eResult;
	}

protected:
	AkMapChildID m_mapChildId; // List of nodes connected to this one

};

#endif
