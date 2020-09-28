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
// AkActionExcept.h
//
//////////////////////////////////////////////////////////////////////
#ifndef _ACTION_EXCEPT_H_
#define _ACTION_EXCEPT_H_

#include <AK/Tools/Common/AkArray.h>
#include "AkAction.h"
#include "AkPoolSizes.h"

typedef AkArray<AkUniqueID, AkUniqueID, ArrayPoolDefault, LIST_POOL_BLOCK_SIZE / sizeof(AkUniqueID) > ExceptionList;

class CAkActionExcept : public CAkAction
{

public:
	//Constructor
	CAkActionExcept(
		AkActionType in_eActionType,	//Type of action
		AkUniqueID in_ulID
		);

	//Destructor
	virtual ~CAkActionExcept();

	//Add an element ID in the exception list of the action
	virtual AKRESULT AddException(
		const AkUniqueID in_ulElementID	//Element ID to be added in the exception list
		);

	//Remove an element ID in the exception list of the actions
	virtual void RemoveException(
		const AkUniqueID in_ulElementID	//Element ID to be removed in the exception list
		);

	//Remove all exceptions from the exception list
	virtual void ClearExceptions();

	virtual AKRESULT SetExceptParams(AkUInt8*& io_rpData, AkUInt32& io_rulDataSize );

protected:

	ExceptionList m_listElementException; //List of exection (applicable in the situation of an All_except action type)
};
#endif
