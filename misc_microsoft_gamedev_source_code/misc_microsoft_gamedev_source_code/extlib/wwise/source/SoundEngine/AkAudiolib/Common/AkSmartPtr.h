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

#pragma once

#include <AK\SoundEngine\Common\AkTypes.h>

template <class T> class CAkSmartPtr
{
public:
	AkForceInline CAkSmartPtr()
		: m_pT( NULL )
	{
	}

    AkForceInline CAkSmartPtr( T* in_pT )
    {
        m_pT = in_pT;
        if (m_pT)
            m_pT->AddRef();
    }

    AkForceInline CAkSmartPtr( const CAkSmartPtr<T>& in_rPtr )
    {
        m_pT = in_rPtr.m_pT;
        if (m_pT)
            m_pT->AddRef();
    }

    ~CAkSmartPtr()
    {
        Release();
    }

    AkForceInline void Release()
    {
        if( m_pT )
		{
			m_pT->Release();
			m_pT = NULL;
		}
    }

    // Assign with no Addref.
    AkForceInline void Attach( T* in_pObj )
    {
        _Assign( in_pObj, false );   
    }

    // Give the pointer without releasing it.
    AkForceInline T* Detach()
    {
        T* pObj = m_pT;
        m_pT = NULL;

        return pObj;
    }

	const CAkSmartPtr<T>& operator=( const CAkSmartPtr<T>& in_pObj )
	{
        _Assign( in_pObj.m_pT );
        return *this;
	}

	const CAkSmartPtr<T>& operator=( T* in_pObj )
	{
        _Assign( in_pObj );
        return *this;
	}

    T& operator*() { return *m_pT; }
    T* operator->() const { return m_pT; }
	operator T*() const { return m_pT; }

	//Operators to pass to functions like QueryInterface and other functions returning an addref'd pointer.
	T** operator &() { return &m_pT; }

    const T& operator*() const { return *m_pT; }

	T* Cast() { return m_pT; }
	const T* Cast() const { return m_pT; }

protected:
    void _Assign( T* in_pObj, bool in_bAddRef = true )
    {
	    if (in_pObj != NULL && in_bAddRef)
		    in_pObj->AddRef();

		// Must use a local pointer since we cannot call Release(); without having set the m_pT to its final value.
		T* l_Ptr = m_pT;
		m_pT = in_pObj;
		if (l_Ptr)
            l_Ptr->Release();
    }

    bool _Compare( const T* in_pObj ) const
    {
        return m_pT == in_pObj;
    }

    T* m_pT;
};
