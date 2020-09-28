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
// AkFXContext.h
//
// Implementation of FX context interface for source, insert and bus FX.
// These class essentially package up information into a more unified interface for the
// FX API.
//
//////////////////////////////////////////////////////////////////////

#ifndef _AK_FXCONTEXT_H_
#define _AK_FXCONTEXT_H_

#include "AkPBI.h"
#include <AK/Tools/Common/AkObject.h>

//-----------------------------------------------------------------------------
// CAkInsertFXContext class.
//-----------------------------------------------------------------------------
class CAkInsertFXContext :	public CAkObject,
							public AK::IAkEffectPluginContext
							
{
public:
	CAkInsertFXContext( CAkPBI * in_pCtx, AkUInt32 in_uFXIndex );
	virtual ~CAkInsertFXContext();

	// Determine if our effect is Send Mode or Insert
	bool IsSendModeEffect( );

protected:
	CAkPBI * m_pContext;
	AkUInt32 m_uFXIndex;
};

//-----------------------------------------------------------------------------
// CAkBusFXContext class.
//-----------------------------------------------------------------------------
class CAkBusFXContext :	public CAkObject,
						public AK::IAkEffectPluginContext
						
{
public:
	CAkBusFXContext();
	virtual ~CAkBusFXContext();

	// Determine if our effect is Send Mode or Insert
	void SetIsSendModeEffect( bool in_bSendMode );
	bool IsSendModeEffect( );

protected:
	bool m_bBypassed;
	bool m_bSendMode;
};

//-----------------------------------------------------------------------------
// CAkSourceFXContext class.
//-----------------------------------------------------------------------------
class CAkSourceFXContext :	public CAkObject,
							public AK::IAkSourcePluginContext
							
{
public:
	CAkSourceFXContext( CAkPBI *, AK::IAkStreamMgr * in_pStreamMgr );
	virtual ~CAkSourceFXContext();

	// Number of loops set through the context, nothing to do
	AkUInt16 GetNumLoops( );

	// Retrieve streaming manager interface if desired
	AK::IAkStreamMgr * GetStreamMgr( );

protected:
	CAkPBI * m_pContext;
	AK::IAkStreamMgr * m_pStreamMgr;
};

#endif // _AK_FXCONTEXT_H_
