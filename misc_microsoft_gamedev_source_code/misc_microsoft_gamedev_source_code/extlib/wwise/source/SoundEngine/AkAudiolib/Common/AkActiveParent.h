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
// AkActiveParent.h
//
//////////////////////////////////////////////////////////////////////
#ifndef _AK_ACTIVE_PARENT_H_
#define _AK_ACTIVE_PARENT_H_

#include "AkParameterNode.h"
#include "AkParentNode.h"

template <class T> class CAkActiveParent : public CAkParentNode<T>
{
public:

	CAkActiveParent(AkUniqueID in_ulID)
	:CAkParentNode<T>(in_ulID)
	{
	}

	virtual ~CAkActiveParent()
	{
	}

	AKRESULT Init()
	{ 
		return CAkParentNode<T>::Init();
	}

	virtual AKRESULT ExecuteAction( ActionParams& in_rAction )
	{
		AKRESULT eResult = AK_Success;
		if( CAkParentNode<T>::IsActiveOrPlaying() )
		{
			if( in_rAction.bIsMasterCall )
			{
				bool bPause = false;
				if( in_rAction.eType == ActionParamType_Resume )
				{
					bPause = true;
				}
				this->PauseTransitions( bPause );
			}
			for( AkMapChildID::Iterator iter = this->m_mapChildId.Begin(); iter != this->m_mapChildId.End(); ++iter )
			{
				CAkAudioNode* pNode = (*iter).item;
				if( !(in_rAction.bIsFromBus) || !pNode->ParentBus() )
				{
					eResult = pNode->ExecuteAction( in_rAction );
					if(eResult != AK_Success)
					{
						break;
					}
				}
			}
		}
		return eResult;
	}

	virtual AKRESULT ExecuteActionExcept( ActionParamsExcept& in_rAction )
	{
		AKRESULT eResult = AK_Success;

		if( in_rAction.pGameObj == NULL )
		{
			bool bPause = false;
			if( in_rAction.eType == ActionParamType_Resume )
			{
				bPause = true;
			}
			this->PauseTransitions( bPause );
		}

		for( AkMapChildID::Iterator iter = this->m_mapChildId.Begin(); iter != this->m_mapChildId.End(); ++iter )
		{
			CAkAudioNode* pNode = (*iter).item;
			if( !(in_rAction.bIsFromBus) || !pNode->ParentBus())
			{
				if( !this->IsException( pNode->ID(), *(in_rAction.pExeceptionList) ) )
				{
					eResult = pNode->ExecuteActionExcept( in_rAction );
					if(eResult != AK_Success)
					{
						break;
					}
				}
			}
		}
		return eResult;
	}

	virtual AKRESULT PlayToEnd( CAkRegisteredObj * in_pGameObj , AkUniqueID in_NodeID, AkPlayingID in_PlayingID /* = AK_INVALID_PLAYING_ID */ )
	{
		AKRESULT eResult = AK_Success;
		for( AkMapChildID::Iterator iter = this->m_mapChildId.Begin(); iter != this->m_mapChildId.End(); ++iter )
		{
			CAkAudioNode* pNode = (*iter).item;

			eResult = pNode->PlayToEnd( in_pGameObj, in_NodeID, in_PlayingID );
			if(eResult != AK_Success)
			{
				break;
			}
		}
		return eResult;
	}

	virtual void ParamNotification( NotifParams& in_rParams )
	{
		for( AkMapChildID::Iterator iter = this->m_mapChildId.Begin(); iter != this->m_mapChildId.End(); ++iter )
		{
			if( !in_rParams.bIsFromBus || (*iter).item->ParentBus() == NULL )
			{
				if( (*iter).item->IsPlaying() )
				{
					(*iter).item->ParamNotification( in_rParams );
				}
			}
		}
	}

	virtual void MuteNotification(AkUInt8 in_cMuteLevel, AkMutedMapItem& in_rMutedItem, bool	in_bIsFromBus = false)
	{
		for( AkMapChildID::Iterator iter = this->m_mapChildId.Begin(); iter != this->m_mapChildId.End(); ++iter )
		{
			// Avoid notifying those who does not want to be notified
			if(!in_bIsFromBus || (*iter).item->ParentBus() == NULL)
			{
				if( (*iter).item->IsPlaying() )
				{
					(*iter).item->MuteNotification(in_cMuteLevel, in_rMutedItem, in_bIsFromBus);
				}
			}
		}
	}

	virtual void MuteNotification(AkUInt8 in_cMuteLevel, CAkRegisteredObj * in_pGameObj, AkMutedMapItem& in_rMutedItem, bool in_bPrioritizeGameObjectSpecificItems = false)
	{
		for( AkMapChildID::Iterator iter = this->m_mapChildId.Begin(); iter != this->m_mapChildId.End(); ++iter )
		{
			if( (*iter).item->IsPlaying() )
			{
				(*iter).item->MuteNotification(in_cMuteLevel, in_pGameObj, in_rMutedItem, in_bPrioritizeGameObjectSpecificItems);
			}
		}
	}

	virtual void PositioningChangeNotification(
		AkReal32			in_RTPCValue,	// Value
		AkRTPC_ParameterID	in_ParameterID,	// RTPC ParameterID, must be a positioning ID.
		CAkRegisteredObj *		in_GameObj,		// Target Game Object
		void*				in_pExceptArray = NULL
		)
	{
		for( AkMapChildID::Iterator iter = this->m_mapChildId.Begin(); iter != this->m_mapChildId.End(); ++iter )
		{
			if( !( (*iter).item->Has3DParams() ) )
			{
				if( (*iter).item->IsPlaying() )
				{
					(*iter).item->PositioningChangeNotification( in_RTPCValue, in_ParameterID, in_GameObj, in_pExceptArray );
				}
			}
		}
	}

	virtual void RecalcNotification()
	{
		for( AkMapChildID::Iterator iter = this->m_mapChildId.Begin(); iter != this->m_mapChildId.End(); ++iter )
		{
			if( (*iter).item->IsPlaying() )
			{
				(*iter).item->RecalcNotification();
			}
		}
	}

	virtual void NotifyBypass(
		AkUInt32 in_bitsFXBypass,
		AkUInt32 in_uTargetMask,
		CAkRegisteredObj * in_pGameObj,
		void* in_pExceptArray = NULL
		)
	{
		for( AkMapChildID::Iterator iter = this->m_mapChildId.Begin(); iter != this->m_mapChildId.End(); ++iter )
		{
			CAkParameterNode* pParameterNode = (CAkParameterNode*)((*iter).item);
			if( pParameterNode->IsPlaying() && !pParameterNode->GetFxParentOverride() )
			{
				pParameterNode->NotifyBypass( in_bitsFXBypass, in_uTargetMask, in_pGameObj, in_pExceptArray );
			}
		}
	}

	virtual void NotifUnsetRTPC( AkPluginID in_FXID, AkRTPC_ParameterID in_ParamID, AkUniqueID in_RTPCCurveID )
	{
		for( AkMapChildID::Iterator iter = this->m_mapChildId.Begin(); iter != this->m_mapChildId.End(); ++iter )
		{
			if( (*iter).item->IsPlaying() )
			{
				(*iter).item->NotifUnsetRTPC( in_FXID, in_ParamID, in_RTPCCurveID );
			}
		}
	}

	virtual void UpdateRTPC( AkRTPCFXSubscription& in_rSubsItem )
	{
		for( AkMapChildID::Iterator iter = this->m_mapChildId.Begin(); iter != this->m_mapChildId.End(); ++iter )
		{
			if( (*iter).item->IsPlaying() )
			{
				(*iter).item->UpdateRTPC( in_rSubsItem );
			}
		}
	}

	virtual AKRESULT PrepareData()
	{
		AKRESULT eResult = AK_Success;
		for( AkMapChildID::Iterator iter = this->m_mapChildId.Begin(); iter != this->m_mapChildId.End(); ++iter )
		{
			eResult = (*iter).item->PrepareData();
			if( eResult != AK_Success )
			{
				// Here unprepare what have been prepared up to now.
				for( AkMapChildID::Iterator iterFlush = this->m_mapChildId.Begin(); iterFlush != iter; ++iterFlush )
				{
					(*iterFlush).item->UnPrepareData();
				}
				break;
			}
		}

		return eResult;
	}

	virtual void UnPrepareData()
	{
		for( AkMapChildID::Iterator iter = this->m_mapChildId.Begin(); iter != this->m_mapChildId.End(); ++iter )
		{
			(*iter).item->UnPrepareData();
		}
	}

#ifndef AK_OPTIMIZED
	virtual void InvalidatePaths()
	{
		for( AkMapChildID::Iterator iter = this->m_mapChildId.Begin(); iter != this->m_mapChildId.End(); ++iter )
		{
			if( (*iter).item->IsPlaying() )
			{
				(*iter).item->InvalidatePaths();
			}
		}
	}

	virtual void UpdateFxParam(
		AkPluginID		in_FXID,
		AkUInt32	   	in_uFXIndex,
		AkPluginParamID	in_ulParamID,
		void*			in_pvParamsBlock,
		AkUInt32			in_ulParamBlockSize
		)
	{
		for( AkMapChildID::Iterator iter = this->m_mapChildId.Begin(); iter != this->m_mapChildId.End(); ++iter )
		{
			if( (*iter).item->IsPlaying() )
			{
				(*iter).item->UpdateFxParam( in_FXID, in_uFXIndex, in_ulParamID, in_pvParamsBlock, in_ulParamBlockSize );
			}
		}
	}

#endif //AK_OPTIMIZED

};

#endif
