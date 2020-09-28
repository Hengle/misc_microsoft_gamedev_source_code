//////////////////////////////////////////////////////////////////////
//
// AkObject.h
//
// Base class for object that use dynamic allocation.
// Overloads new and delete to call those of the memory manager.
//
// Copyright 2006-2008 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#ifndef _AK_OBJECT_H_
#define _AK_OBJECT_H_

#include <AK/SoundEngine/Common/AkMemoryMgr.h>

//-----------------------------------------------------------------------------
// Macros
//-----------------------------------------------------------------------------
#if defined (AK_MEMDEBUG)
	#define AkNew(_pool,_what)				new((_pool),__WFILE__,__LINE__) _what
	#define AkAlloc(_pool,_size)			(AK::MemoryMgr::dMalloc((_pool),_size,__WFILE__,__LINE__))
	#ifndef WIDEN2
		#define WIDEN2(x) L ## x
		#define WIDEN(x) WIDEN2(x)
		#define __WFILE__ WIDEN(__FILE__)
	#endif
	#define AkNew2(_ptr,_pool,_type,_what)	{ _ptr = (_type *) AK::MemoryMgr::dMalloc((_pool),sizeof(_type),__WFILE__,__LINE__); if ( _ptr ) ::new( _ptr ) _what; }
#else
	#define AkNew(_pool,_what)				new((_pool)) _what
	#define AkAlloc(_pool,_size)			(AK::MemoryMgr::Malloc((_pool),_size))
	#define AkNew2(_ptr,_pool,_type,_what)	{ _ptr = (_type *) AK::MemoryMgr::Malloc((_pool),sizeof(_type)); if ( _ptr ) ::new( _ptr ) _what; }
#endif

#define AkFree(_pool,_pvmem)			(AK::MemoryMgr::Free((_pool),(_pvmem)))

#define AkDelete2(_pool,_type,_what)	{ _what->~_type(); AK::MemoryMgr::Free( _pool, _what ); }

//-----------------------------------------------------------------------------
// Name: Class CAkObject
// Desc: Base allocator object.
//-----------------------------------------------------------------------------
class CAkObject
{
public:

#if defined (AK_MEMDEBUG)
	static AkForceInline void * operator new(size_t size,AkMemPoolId in_PoolId,AkTChar* ptcFile,AkUInt32 ulLine)
	{
		return AK::MemoryMgr::dMalloc( in_PoolId,size,ptcFile,ulLine );
	}
#else
	/// Member new operator
    static AkForceInline void * operator new(size_t size,AkMemPoolId in_PoolId)
	{
		return AK::MemoryMgr::Malloc( in_PoolId, size );
	}
#endif

	/// Destructor
    virtual ~CAkObject( ) { }

private:
    static void * operator new(size_t size); // Illegal.
};

AkForceInline void AkDelete( AkMemPoolId in_PoolId, CAkObject * in_pObject )
{
	if ( in_pObject )
	{
		in_pObject->~CAkObject();
		AK::MemoryMgr::Free( in_PoolId, in_pObject );
	}
}

#endif // _AK_OBJECT_H_
