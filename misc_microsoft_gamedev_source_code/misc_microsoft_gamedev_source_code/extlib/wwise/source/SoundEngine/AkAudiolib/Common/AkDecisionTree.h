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

#ifndef AK_DECISION_TREE_H_
#define AK_DECISION_TREE_H_

#include <AK/SoundEngine/Common/AkTypes.h>

class AkDecisionTree
{
public:
	struct Node
	{
		AkArgumentValueID key; // Not AkUInt16 to make alignment explicit

		union
		{
			struct
			{
				AkUInt16 uIdx;
				AkUInt16 uCount;
			} children;

			AkUniqueID audioNodeID;
		};
	};

	AkDecisionTree();
	~AkDecisionTree();

	AKRESULT Init( AkUInt32 in_uDepth, void * in_pData, AkUInt32 in_uDataSize );

	AkUniqueID ResolvePath( AkArgumentValueID * in_pPath, AkUInt32 in_cPath );

	AkUInt32 Depth() const { return m_uDepth; }

private:
	AkUniqueID _ResolvePath( Node * in_pRootNode, AkArgumentValueID * in_pPath, AkUInt32 in_cPath, bool & io_bFound );
	Node * BinarySearch( Node * in_pNodes, AkUInt32 in_cNodes, AkArgumentValueID in_key );

	AkUInt32 m_uDepth;
	Node * m_pNodes;
};

#endif
