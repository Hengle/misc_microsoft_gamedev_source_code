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
// AkPoolSizes.h
//
// AkAudioLib specific definitions
//
//////////////////////////////////////////////////////////////////////
#ifndef _POOLSIZES_H_
#define _POOLSIZES_H_

#define DEFAULT_POOL_SIZE			(16 * 1024 * 1024)
#define DEFAULT_POOL_BLOCK_SIZE		(64)

#define LIST_POOL_SIZE				(1024 * 1024)
#define LIST_POOL_BLOCK_SIZE		(32)

#define COMMAND_QUEUE_SIZE			(1024 * 256)

// Monitor pool is 256k
#define MONITOR_POOL_SIZE				( 256 * 1024 )
#define MONITOR_POOL_BLOCK_SIZE			64

// Monitor queue pool is a single block of 64k
#define MONITOR_QUEUE_POOL_SIZE			( 64 * 1024 )
#define MONITOR_QUEUE_POOL_BLOCK_SIZE	MONITOR_QUEUE_POOL_SIZE

#endif // _POOLSIZES_H_
