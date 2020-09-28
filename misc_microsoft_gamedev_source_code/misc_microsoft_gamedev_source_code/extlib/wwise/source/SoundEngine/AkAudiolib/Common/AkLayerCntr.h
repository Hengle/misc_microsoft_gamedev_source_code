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
// AkLayerCntr.h
//
//////////////////////////////////////////////////////////////////////

#ifndef _AKLAYERCNTR_H_
#define _AKLAYERCNTR_H_

#include "AkMultiPlayNode.h"
#include <AK/Tools/Common/AkArray.h>

class CAkLayer;

// class corresponding to a Layer container
//
// Author:  dmouraux
class CAkLayerCntr
	: public CAkMultiPlayNode
{
	DECLARE_BASECLASS( CAkContainerBase );

public:

	// Thread safe version of the constructor
	static CAkLayerCntr* Create( AkUniqueID in_ulID = 0 );

	//Return the node category
	virtual AkNodeCategory NodeCategory();

 	// Call a play on the definition directly
	//
    // Return - AKRESULT - Ak_Success if succeeded
	virtual AKRESULT Play( AkPBIParams& in_rPBIParams );

	AKRESULT SetInitialValues( AkUInt8* in_pData, AkUInt32 in_ulDataSize );

    virtual AKRESULT AddChild(AkUniqueID in_ulID);
    virtual AKRESULT RemoveChild(AkUniqueID in_ulID);

	AKRESULT AddLayer( AkUniqueID in_LayerID );
	AKRESULT RemoveLayer( AkUniqueID in_LayerID );

protected:

    CAkLayerCntr( AkUniqueID in_ulID );
	virtual ~CAkLayerCntr();

	typedef AkArray< CAkLayer*, CAkLayer*, ArrayPoolDefault, LIST_POOL_BLOCK_SIZE/sizeof(CAkLayer*) > LayerList;
	LayerList m_layers;
};

#endif // _AKLAYERCNTR_H_
