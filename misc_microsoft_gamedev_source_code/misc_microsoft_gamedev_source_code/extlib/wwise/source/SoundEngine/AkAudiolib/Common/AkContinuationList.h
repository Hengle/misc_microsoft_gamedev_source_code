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
// AkContinuationList.h
//
//////////////////////////////////////////////////////////////////////
#ifndef _CONTINUATION_LIST_H_
#define _CONTINUATION_LIST_H_

#include "AkParameters.h"
#include <AK/Tools/Common/AkArray.h>
#include "AkPoolSizes.h"
#include "AkSmartPtr.h"

extern AkMemPoolId g_DefaultPoolId;

class CAkRanSeqCntr;
class CAkMultiPlayNode;
class CAkContinuationList;
class CAkContainerBaseInfo;

// This is the number of buffers we wait in a continuation list before
// attempting to play the next item when an item fails. The goal is to
// avoid making lots of very rapid attempts that keep failing.
#ifdef RVL_OS
#define AK_WAIT_BUFFERS_AFTER_PLAY_FAILED		(80)
#else
#define AK_WAIT_BUFFERS_AFTER_PLAY_FAILED		(10)
#endif

class CAkContinueListItem
{
public:
	//Constructor
	CAkContinueListItem();

	//Destructor
	~CAkContinueListItem();

// Members are public since they all have to be used by everybody
public:
	CAkRanSeqCntr* m_pContainer;			// Pointer to the container
	CAkContainerBaseInfo* m_pContainerInfo;	// Container info
	AkLoop m_LoopingInfo;					// Looping info
	CAkMultiPlayNode* m_pMultiPlayNode;
	CAkContinuationList* m_pAlternateContList;
};

class CAkContinuationList : public CAkObject
{

protected:
	CAkContinuationList():m_iRefCount(1){}
	virtual ~CAkContinuationList();

public:
	//MUST Assign the returned value of create into a CAkSmartPtr.
	static CAkContinuationList* Create();

	typedef AkArray<CAkContinueListItem, const CAkContinueListItem&, ArrayPoolDefault, DEFAULT_POOL_BLOCK_SIZE/sizeof(CAkContinueListItem)> AkContinueListItem;
	AkContinueListItem m_listItems;
	void AddRef();
	void Release();

private:
	void Term();
	AkInt32 m_iRefCount;
};

#endif
