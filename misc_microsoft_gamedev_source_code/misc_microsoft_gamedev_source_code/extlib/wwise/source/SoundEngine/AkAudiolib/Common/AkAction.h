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
// AkAction.h
//
//////////////////////////////////////////////////////////////////////
#ifndef _ACTION_H_
#define _ACTION_H_

#include "AkIndexable.h"
#include "AkParameters.h"

//        Define                 Bitfield exclusive
#define ACTION_TYPE_USE_OBJECT			0x01

//        Define                 Bitfield exclusive
#define ACTION_TYPE_SCOPE_MASK			0xFF0

#define ACTION_TYPE_SCOPE_ELEMENT		0x010
#define ACTION_TYPE_SCOPE_ALL			0x020
#define ACTION_TYPE_SCOPE_ALL_EXCEPT	0x040
#define ACTION_TYPE_SCOPE_EVENT			0x080
#define ACTION_TYPE_SCOPE_BUS			0x100

//       Define                  Bitfield NON-exclusive

#define ACTION_TYPE_ACTION				0xFF000
#define ACTION_TYPE_STOP				0x01000
#define ACTION_TYPE_PAUSE				0x02000
#define ACTION_TYPE_RESUME				0x03000
#define ACTION_TYPE_PLAY				0x04000
#define ACTION_TYPE_PLAYANDCONTNUE		0x05000

#define ACTION_TYPE_MUTE				0x06000			
#define ACTION_TYPE_UNMUTE				0x07000
#define ACTION_TYPE_SETPITCH			0x08000
#define ACTION_TYPE_RESETPITCH			0x09000
#define ACTION_TYPE_SETVOLUME			0x0A000
#define ACTION_TYPE_RESETVOLUME			0x0B000
#define ACTION_TYPE_SETLFE				0x0C000
#define ACTION_TYPE_RESETLFE			0x0D000
#define ACTION_TYPE_SETLPF				0x0E000
#define ACTION_TYPE_RESETLPF			0x0F000
#define ACTION_TYPE_USESTATE			0x10000
#define ACTION_TYPE_UNUSESTATE			0x11000
#define ACTION_TYPE_SETSTATE			0x12000

#define ACTION_TYPE_STOPEVENT			0x20000
#define ACTION_TYPE_PAUSEEVENT			0x30000
#define ACTION_TYPE_RESUMEEVENT			0x40000

#define ACTION_TYPE_DUCK				0x50000
#define ACTION_TYPE_SETSWITCH			0x60000
#define ACTION_TYPE_SETRTPC				0x61000
#define ACTION_TYPE_BYPASSFX			0x70000
#define ACTION_TYPE_RESETBYPASSFX		0x80000
#define ACTION_TYPE_BREAK				0x90000
#define ACTION_TYPE_TRIGGER				0xA0000

class CAkRegisteredObj;

//List of all types of actions
enum AkActionType
{
//             LEGEND
//
//	_E   = ElementSpecific
//	_ALL = Global scope(all elements)
//	_AE  = All except
//	_O   = Object Specific
//  _M	 = Main object is affected (AudioNode Definition)

	AkActionType_None				= 0x0000,
	AkActionType_SetState			= ACTION_TYPE_SETSTATE	 | ACTION_TYPE_SCOPE_ALL,

	AkActionType_BypassFX_M			= ACTION_TYPE_BYPASSFX	 | ACTION_TYPE_SCOPE_ELEMENT,
	AkActionType_BypassFX_O			= ACTION_TYPE_BYPASSFX	 | ACTION_TYPE_SCOPE_ELEMENT | ACTION_TYPE_USE_OBJECT,

	AkActionType_ResetBypassFX_M	= ACTION_TYPE_RESETBYPASSFX | ACTION_TYPE_SCOPE_ELEMENT,
	AkActionType_ResetBypassFX_O	= ACTION_TYPE_RESETBYPASSFX | ACTION_TYPE_SCOPE_ELEMENT | ACTION_TYPE_USE_OBJECT ,

	AkActionType_ResetBypassFX_ALL	= ACTION_TYPE_RESETBYPASSFX | ACTION_TYPE_SCOPE_ALL,
	AkActionType_ResetBypassFX_ALL_O= ACTION_TYPE_RESETBYPASSFX | ACTION_TYPE_SCOPE_ALL | ACTION_TYPE_USE_OBJECT,

	AkActionType_ResetBypassFX_AE	= ACTION_TYPE_RESETBYPASSFX | ACTION_TYPE_SCOPE_ALL_EXCEPT,
	AkActionType_ResetBypassFX_AE_O	= ACTION_TYPE_RESETBYPASSFX | ACTION_TYPE_SCOPE_ALL_EXCEPT | ACTION_TYPE_USE_OBJECT,

	AkActionType_SetSwitch			= ACTION_TYPE_SETSWITCH  | ACTION_TYPE_USE_OBJECT,
	AkActionType_SetRTPC			= ACTION_TYPE_SETRTPC	 | ACTION_TYPE_USE_OBJECT,

	AkActionType_UseState_E			= ACTION_TYPE_USESTATE	 | ACTION_TYPE_SCOPE_ELEMENT,
	AkActionType_UnuseState_E		= ACTION_TYPE_UNUSESTATE | ACTION_TYPE_SCOPE_ELEMENT,

	AkActionType_Play				= ACTION_TYPE_PLAY		 | ACTION_TYPE_SCOPE_ELEMENT | ACTION_TYPE_USE_OBJECT,
	AkActionType_PlayAndContinue	= ACTION_TYPE_PLAYANDCONTNUE | ACTION_TYPE_SCOPE_ELEMENT | ACTION_TYPE_USE_OBJECT,

	AkActionType_Stop_E				= ACTION_TYPE_STOP		 | ACTION_TYPE_SCOPE_ELEMENT,
	AkActionType_Stop_E_O			= ACTION_TYPE_STOP		 | ACTION_TYPE_SCOPE_ELEMENT | ACTION_TYPE_USE_OBJECT,

	AkActionType_Stop_ALL			= ACTION_TYPE_STOP		 | ACTION_TYPE_SCOPE_ALL,
	AkActionType_Stop_ALL_O			= ACTION_TYPE_STOP		 | ACTION_TYPE_SCOPE_ALL | ACTION_TYPE_USE_OBJECT,

	AkActionType_Stop_AE			= ACTION_TYPE_STOP		 | ACTION_TYPE_SCOPE_ALL_EXCEPT,
	AkActionType_Stop_AE_O			= ACTION_TYPE_STOP		 | ACTION_TYPE_SCOPE_ALL_EXCEPT | ACTION_TYPE_USE_OBJECT,	

	AkActionType_Pause_E			= ACTION_TYPE_PAUSE		 | ACTION_TYPE_SCOPE_ELEMENT,
	AkActionType_Pause_E_O			= ACTION_TYPE_PAUSE		 | ACTION_TYPE_SCOPE_ELEMENT | ACTION_TYPE_USE_OBJECT,	
												
	AkActionType_Pause_ALL			= ACTION_TYPE_PAUSE		 | ACTION_TYPE_SCOPE_ALL,		
	AkActionType_Pause_ALL_O		= ACTION_TYPE_PAUSE		 | ACTION_TYPE_SCOPE_ALL | ACTION_TYPE_USE_OBJECT,	
												
	AkActionType_Pause_AE			= ACTION_TYPE_PAUSE		 | ACTION_TYPE_SCOPE_ALL_EXCEPT,
	AkActionType_Pause_AE_O			= ACTION_TYPE_PAUSE		 | ACTION_TYPE_SCOPE_ALL_EXCEPT | ACTION_TYPE_USE_OBJECT,

	AkActionType_Resume_E			= ACTION_TYPE_RESUME	 | ACTION_TYPE_SCOPE_ELEMENT,
	AkActionType_Resume_E_O			= ACTION_TYPE_RESUME	 | ACTION_TYPE_SCOPE_ELEMENT | ACTION_TYPE_USE_OBJECT,
												
	AkActionType_Resume_ALL			= ACTION_TYPE_RESUME	 | ACTION_TYPE_SCOPE_ALL,
	AkActionType_Resume_ALL_O		= ACTION_TYPE_RESUME	 | ACTION_TYPE_SCOPE_ALL | ACTION_TYPE_USE_OBJECT,
												
	AkActionType_Resume_AE			= ACTION_TYPE_RESUME	 | ACTION_TYPE_SCOPE_ALL_EXCEPT,
	AkActionType_Resume_AE_O		= ACTION_TYPE_RESUME	 | ACTION_TYPE_SCOPE_ALL_EXCEPT | ACTION_TYPE_USE_OBJECT,

	AkActionType_Break_E			= ACTION_TYPE_BREAK		 | ACTION_TYPE_SCOPE_ELEMENT,
	AkActionType_Break_E_O			= ACTION_TYPE_BREAK		 | ACTION_TYPE_SCOPE_ELEMENT | ACTION_TYPE_USE_OBJECT,
												
	AkActionType_Mute_M				= ACTION_TYPE_MUTE		 | ACTION_TYPE_SCOPE_ELEMENT,
	AkActionType_Mute_O				= ACTION_TYPE_MUTE		 | ACTION_TYPE_SCOPE_ELEMENT | ACTION_TYPE_USE_OBJECT ,

	AkActionType_Unmute_M			= ACTION_TYPE_UNMUTE	 | ACTION_TYPE_SCOPE_ELEMENT,
	AkActionType_Unmute_O			= ACTION_TYPE_UNMUTE	 | ACTION_TYPE_SCOPE_ELEMENT | ACTION_TYPE_USE_OBJECT ,

	AkActionType_Unmute_ALL			= ACTION_TYPE_UNMUTE	 | ACTION_TYPE_SCOPE_ALL,
	AkActionType_Unmute_ALL_O		= ACTION_TYPE_UNMUTE	 | ACTION_TYPE_SCOPE_ALL | ACTION_TYPE_USE_OBJECT ,

	AkActionType_Unmute_AE			= ACTION_TYPE_UNMUTE	 | ACTION_TYPE_SCOPE_ALL_EXCEPT,
	AkActionType_Unmute_AE_O		= ACTION_TYPE_UNMUTE	 | ACTION_TYPE_SCOPE_ALL_EXCEPT | ACTION_TYPE_USE_OBJECT ,

	AkActionType_SetVolume_M		= ACTION_TYPE_SETVOLUME	 | ACTION_TYPE_SCOPE_ELEMENT,
	AkActionType_SetVolume_O		= ACTION_TYPE_SETVOLUME	 | ACTION_TYPE_SCOPE_ELEMENT | ACTION_TYPE_USE_OBJECT ,

	AkActionType_ResetVolume_M		= ACTION_TYPE_RESETVOLUME | ACTION_TYPE_SCOPE_ELEMENT,
	AkActionType_ResetVolume_O		= ACTION_TYPE_RESETVOLUME | ACTION_TYPE_SCOPE_ELEMENT | ACTION_TYPE_USE_OBJECT ,

	AkActionType_ResetVolume_ALL	= ACTION_TYPE_RESETVOLUME | ACTION_TYPE_SCOPE_ALL,
	AkActionType_ResetVolume_ALL_O	= ACTION_TYPE_RESETVOLUME | ACTION_TYPE_SCOPE_ALL | ACTION_TYPE_USE_OBJECT,

	AkActionType_ResetVolume_AE		= ACTION_TYPE_RESETVOLUME | ACTION_TYPE_SCOPE_ALL_EXCEPT,
	AkActionType_ResetVolume_AE_O	= ACTION_TYPE_RESETVOLUME | ACTION_TYPE_SCOPE_ALL_EXCEPT | ACTION_TYPE_USE_OBJECT,

	AkActionType_SetPitch_M			= ACTION_TYPE_SETPITCH	 | ACTION_TYPE_SCOPE_ELEMENT,
	AkActionType_SetPitch_O			= ACTION_TYPE_SETPITCH	 | ACTION_TYPE_SCOPE_ELEMENT | ACTION_TYPE_USE_OBJECT,

	AkActionType_ResetPitch_M		= ACTION_TYPE_RESETPITCH | ACTION_TYPE_SCOPE_ELEMENT,
	AkActionType_ResetPitch_O		= ACTION_TYPE_RESETPITCH | ACTION_TYPE_SCOPE_ELEMENT | ACTION_TYPE_USE_OBJECT,	

	AkActionType_ResetPitch_ALL		= ACTION_TYPE_RESETPITCH | ACTION_TYPE_SCOPE_ALL,
	AkActionType_ResetPitch_ALL_O	= ACTION_TYPE_RESETPITCH | ACTION_TYPE_SCOPE_ALL | ACTION_TYPE_USE_OBJECT,

	AkActionType_ResetPitch_AE		= ACTION_TYPE_RESETPITCH | ACTION_TYPE_SCOPE_ALL_EXCEPT,	
	AkActionType_ResetPitch_AE_O	= ACTION_TYPE_RESETPITCH | ACTION_TYPE_SCOPE_ALL_EXCEPT | ACTION_TYPE_USE_OBJECT,

	AkActionType_SetLFE_M			= ACTION_TYPE_SETLFE	 | ACTION_TYPE_SCOPE_ELEMENT,
	AkActionType_SetLFE_O			= ACTION_TYPE_SETLFE	 | ACTION_TYPE_SCOPE_ELEMENT | ACTION_TYPE_USE_OBJECT ,

	AkActionType_ResetLFE_M			= ACTION_TYPE_RESETLFE	 | ACTION_TYPE_SCOPE_ELEMENT,
	AkActionType_ResetLFE_O			= ACTION_TYPE_RESETLFE	 | ACTION_TYPE_SCOPE_ELEMENT | ACTION_TYPE_USE_OBJECT ,

	AkActionType_ResetLFE_ALL		= ACTION_TYPE_RESETLFE	 | ACTION_TYPE_SCOPE_ALL,
	AkActionType_ResetLFE_ALL_O		= ACTION_TYPE_RESETLFE	 | ACTION_TYPE_SCOPE_ALL | ACTION_TYPE_USE_OBJECT,

	AkActionType_ResetLFE_AE		= ACTION_TYPE_RESETLFE	 | ACTION_TYPE_SCOPE_ALL_EXCEPT,
	AkActionType_ResetLFE_AE_O		= ACTION_TYPE_RESETLFE	 | ACTION_TYPE_SCOPE_ALL_EXCEPT | ACTION_TYPE_USE_OBJECT,

	AkActionType_SetLPF_M			= ACTION_TYPE_SETLPF	 | ACTION_TYPE_SCOPE_ELEMENT,
	AkActionType_SetLPF_O			= ACTION_TYPE_SETLPF	 | ACTION_TYPE_SCOPE_ELEMENT | ACTION_TYPE_USE_OBJECT ,

	AkActionType_ResetLPF_M			= ACTION_TYPE_RESETLPF	 | ACTION_TYPE_SCOPE_ELEMENT,
	AkActionType_ResetLPF_O			= ACTION_TYPE_RESETLPF	 | ACTION_TYPE_SCOPE_ELEMENT | ACTION_TYPE_USE_OBJECT ,

	AkActionType_ResetLPF_ALL		= ACTION_TYPE_RESETLPF	 | ACTION_TYPE_SCOPE_ALL,
	AkActionType_ResetLPF_ALL_O		= ACTION_TYPE_RESETLPF	 | ACTION_TYPE_SCOPE_ALL | ACTION_TYPE_USE_OBJECT,

	AkActionType_ResetLPF_AE		= ACTION_TYPE_RESETLPF	 | ACTION_TYPE_SCOPE_ALL_EXCEPT,
	AkActionType_ResetLPF_AE_O		= ACTION_TYPE_RESETLPF	 | ACTION_TYPE_SCOPE_ALL_EXCEPT | ACTION_TYPE_USE_OBJECT,

	AkActionType_StopEvent			= ACTION_TYPE_STOPEVENT | ACTION_TYPE_SCOPE_EVENT | ACTION_TYPE_USE_OBJECT,
	AkActionType_PauseEvent			= ACTION_TYPE_PAUSEEVENT | ACTION_TYPE_SCOPE_EVENT | ACTION_TYPE_USE_OBJECT,
	AkActionType_ResumeEvent		= ACTION_TYPE_RESUMEEVENT | ACTION_TYPE_SCOPE_EVENT | ACTION_TYPE_USE_OBJECT,

	AkActionType_Duck				= ACTION_TYPE_DUCK | ACTION_TYPE_SCOPE_BUS,

	AkActionType_Trigger			= ACTION_TYPE_TRIGGER,
	AkActionType_Trigger_O			= ACTION_TYPE_TRIGGER | ACTION_TYPE_USE_OBJECT,

//               LEGEND
//
//	_E   = ElementSpecific
//	_ALL = Global scope(all elements)
//	_AE  = All except
//	_O   = Object Specific
//  _M	 = Main object is affected (AudioNode Definition)
};

struct AkPendingAction;

//Action object
class CAkAction : public CAkIndexable
{
public:

	//Thread safe version of the constructor
	static CAkAction* Create(AkActionType in_eActionType, AkUniqueID in_ulID = 0);

	//Constructor
	CAkAction(
		AkActionType in_eActionType,	//Type of action
		AkUniqueID in_ulID
		);

	//Destructor
	virtual ~CAkAction();

	AKRESULT	Init(){ AddToIndex(); return AK_Success; }

	//Get the Action type of the current action
	//
	// Return - AkActionType - Type of action
	AkForceInline AkActionType ActionType() { return m_eActionType; }

	//Set the Action type of the current action
	virtual void ActionType(AkActionType in_ActionType);

	//Set the element ID associated to the Action
	virtual void SetElementID(
		AkUniqueID in_ulElementID//Element ID set as action target
		);

	//Get The element ID associated to the action
	AkForceInline AkUniqueID ElementID() { return m_ulElementID; }

	//Set the Delay associated to the action, in output samples
	virtual void Delay(
		AkInt32 in_Delay, // Custom ID
		AkInt32 in_RangeMin = 0,// Range Min
		AkInt32 in_RangeMax = 0// Range Max
		);

	//Get the Delay associated to the action
	//
	// Return - AkInt32 - Delay associated to the action 0 means no delay, in output samples
	AkInt32 Delay();

	// Execute the Action
	// Must be called only by the AudioThread
	//
	// Return - AKRESULT - AK_Success if all succeeded
	virtual AKRESULT Execute(
		AkPendingAction * in_pAction
		) = 0;

	void AddToIndex();
	void RemoveFromIndex();

	virtual AkUInt32 AddRef();
	virtual AkUInt32 Release();

	AKRESULT SetInitialValues(AkUInt8* pData, AkUInt32 ulDataSize);
	virtual AKRESULT SetActionParams(AkUInt8*& io_rpData, AkUInt32& io_rulDataSize );
	virtual AKRESULT SetExceptParams(AkUInt8*& io_rpData, AkUInt32& io_rulDataSize );
	virtual AKRESULT SetActionSpecificParams(AkUInt8*& io_rpData, AkUInt32& io_rulDataSize );

protected:

	AkUniqueID m_ulElementID;	// Associated element	
	AkActionType m_eActionType;	// Type of action
	RANGED_PARAMETER<AkInt32> m_Delay;// Delay in output samples	
};
#endif
