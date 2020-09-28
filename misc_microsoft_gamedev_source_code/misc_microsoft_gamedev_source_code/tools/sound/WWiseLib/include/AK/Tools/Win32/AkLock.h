//////////////////////////////////////////////////////////////////////
//
// AkLock.h
//
// AudioKinetic Lock class
//
// Copyright 2006-2008 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#ifndef _AKLOCK_H_
#define _AKLOCK_H_

#include <AK/SoundEngine/Common/AkTypes.h>

//-----------------------------------------------------------------------------
// CAkLock class
//-----------------------------------------------------------------------------
class CAkLock
{
public:
    /// Constructor
	CAkLock() 
    {
        ::InitializeCriticalSection( &m_csLock );
    }

	/// Destructor
	~CAkLock()
    {
        ::DeleteCriticalSection( &m_csLock );
    }

    /// Lock 
    inline AKRESULT Lock( void )
	{
	    ::EnterCriticalSection( &m_csLock );
		return AK_Success;
	}

	/// Unlock
    inline AKRESULT Unlock( void )
	{
	    ::LeaveCriticalSection( &m_csLock );
		return AK_Success;
	}

/*  // Returns AK_Success if lock aquired, AK_Fail otherwise.
	inline AKRESULT Trylock( void )
    {
        if ( ::TryEnterCriticalSection( &m_csLock ) )
            return AK_Success;
        return AK_Fail;
    } */

private:
    CRITICAL_SECTION  m_csLock; ///< Platform specific lock
};

#endif // _AKLOCK_H_
