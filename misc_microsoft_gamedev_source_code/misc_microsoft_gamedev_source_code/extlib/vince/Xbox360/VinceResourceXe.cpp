//-----------------------------------------------------------------------------
// File: VinceResource.cpp
// Based on AtgResource.cpp from Xenon XDK samples
//
// Desc: Loads resources from an XPR (Xbox Packed Resource) file.  
//
// Created 2004/08/23 Rich Bonny <rbonny@microsoft.com>
//         2005/04/05 Some updates from the Atg version 
//                    have been picked up and incorporated.
//
// MICROSOFT CONFIDENTIAL.  DO NOT DISTRIBUTE.
// Microsoft Game Studios Tools and Technology (TnT)
// Copyright (c) 2005 Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

#include "VinceControl.h"

#ifdef _VINCE_
#ifndef NO_VINCE_SURVEYS

#include <XTL.h>
#include "VinceResourceXe.h"
#include <assert.h>
#include <tchar.h>
#include <D3DX9.h>
#include <XGraphics.h>
#include "TnTUtil.h"
#include "VinceUtil.h"

namespace Vince
{
	//--------------------------------------------------------------------------------------
	// Magic values to identify XPR files
	//--------------------------------------------------------------------------------------
	struct XPR_HEADER
	{
		DWORD dwMagic;
		DWORD dwHeaderSize;
		DWORD dwDataSize;
	};

	#define XPR2_MAGIC_VALUE 0x58505232
	const DWORD eXALLOCAllocatorId_VinceResource = 127;

	//--------------------------------------------------------------------------------------
	// Name: VincePackedResource
	//--------------------------------------------------------------------------------------
	VincePackedResource::VincePackedResource()
	{
		m_pSysMemData       = NULL;
		m_dwSysMemDataSize  = 0L;
		m_pVidMemData       = NULL;
		m_dwVidMemDataSize  = 0L;
		m_pResourceTags     = NULL;
		m_dwNumResourceTags = 0L;
	}


	//--------------------------------------------------------------------------------------
	// Name:~ VincePackedResource
	//--------------------------------------------------------------------------------------
	VincePackedResource::~VincePackedResource()
	{
		Destroy();
	}


	//--------------------------------------------------------------------------------------
	// Name: GetData
	// Desc: Loads all the texture resources from the given XPR.
	//--------------------------------------------------------------------------------------
	VOID* VincePackedResource::GetData( const CHAR* strName ) const
	{
		if( NULL==m_pResourceTags || NULL==strName )
			return NULL;

		for( DWORD i=0; i < m_dwNumResourceTags; i++ )
		{
			if( !_stricmp( strName, m_pResourceTags[i].strName ) )
			{
				return &m_pSysMemData[m_pResourceTags[i].dwOffset];
			}
		}

		return NULL;
	}


	//--------------------------------------------------------------------------------------
	// Name: AllocateContiguousMemory()
	// Desc: Wrapper for XMemAlloc
	//--------------------------------------------------------------------------------------
	static __forceinline void* AllocateContiguousMemory( DWORD Size,
														DWORD Alignment, // XALLOC_PHYSICAL_ALIGNMENT_16, etc.
														DWORD Protection = XALLOC_MEMPROTECT_WRITECOMBINE )
	{
		return XMemAlloc( Size, MAKE_XALLOC_ATTRIBUTES( 0, 0, 0, 0,
														eXALLOCAllocatorId_VinceResource,
														Alignment,
														Protection,
														0,
														XALLOC_MEMTYPE_PHYSICAL ) );
	}


	//--------------------------------------------------------------------------------------
	// Name: FreeContiguousMemory()
	// Desc: Wrapper for XMemFree
	//--------------------------------------------------------------------------------------
	static __forceinline VOID FreeContiguousMemory( VOID* pData )
	{
		return XMemFree( pData, MAKE_XALLOC_ATTRIBUTES( 0, 0, 0, 0,
														eXALLOCAllocatorId_VinceResource,
														0, 0, 0,
														XALLOC_MEMTYPE_PHYSICAL ) );
	}


	//--------------------------------------------------------------------------------------
	// Name: Create
	// Desc: Loads all the texture resources from the given XPR.
	//--------------------------------------------------------------------------------------
	HRESULT VincePackedResource::Create( const CHAR* strFilename )
	{
		// Find the media file
		const char* strResourcePath = GetFullFileName(strFilename, false);
		if( !VinceFileExists(strResourcePath, false) )
		{
			SAFE_DELETE_ARRAY(strResourcePath);
			return E_FAIL;
		}

		// Open the file
		DWORD dwNumBytesRead;
		HANDLE hFile = CreateFile( strResourcePath, GENERIC_READ, FILE_SHARE_READ, NULL,
								OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, NULL );
		// Don't need file name string anymore
		SAFE_DELETE_ARRAY(strResourcePath);
		if( hFile == INVALID_HANDLE_VALUE )
		{
			// AtgUtil_PrintError( "File <%s> not found\n", strFilename );
			return E_FAIL;
		}

		// Read in and verify the XPR magic header
		XPR_HEADER xprh;
		ReadFile( hFile, &xprh, sizeof(XPR_HEADER), &dwNumBytesRead, NULL );
	    
		if( xprh.dwMagic != XPR2_MAGIC_VALUE )
		{
			// AtgUtil_PrintError( "Invalid Xbox Packed Resource (.xpr) file: Magic = 0x%08lx\n", xprh.dwMagic );
			CloseHandle( hFile );
			return E_FAIL;
		}

		// Compute memory requirements
		m_dwSysMemDataSize = xprh.dwHeaderSize;
		m_dwVidMemDataSize = xprh.dwDataSize;

		// Allocate memory
		m_pSysMemData = new BYTE[m_dwSysMemDataSize];
		m_pVidMemData = (BYTE*)AllocateContiguousMemory( m_dwVidMemDataSize, XALLOC_PHYSICAL_ALIGNMENT_4K );

		// Read in the data from the file
		ReadFile( hFile, m_pSysMemData, m_dwSysMemDataSize, &dwNumBytesRead, NULL );
		ReadFile( hFile, m_pVidMemData, m_dwVidMemDataSize, &dwNumBytesRead, NULL );

		// Done with the file
		CloseHandle( hFile );
	    
		// Extract resource table from the header data
		m_dwNumResourceTags = *(DWORD*)(m_pSysMemData+0);
		m_pResourceTags     = (VINCERESOURCE*)(m_pSysMemData+4);

		// Patch up the resources
		for( DWORD i=0; i<m_dwNumResourceTags; i++ )
		{
			m_pResourceTags[i].strName = (CHAR*)( m_pSysMemData + (DWORD)m_pResourceTags[i].strName );

			// Fixup the texture memory
			if( (m_pResourceTags[i].dwType & 0xffff0000) == (VINCERESOURCETYPE_TEXTURE & 0xffff0000) )
			{
				D3DTexture* pTexture = (D3DTexture*)&m_pSysMemData[m_pResourceTags[i].dwOffset];
	    
				// Adjust Base address according to where memory was allocated
				XGOffsetBaseTextureAddress( pTexture, m_pVidMemData, m_pVidMemData );
			}
		}

		return S_OK;
	}


	//--------------------------------------------------------------------------------------
	// Name: GetResourceTags
	// Desc: Retrieves the resource tags
	//--------------------------------------------------------------------------------------
	VOID VincePackedResource::GetResourceTags( DWORD* pdwNumResourceTags,
											VINCERESOURCE** ppResourceTags ) const
	{
		if( pdwNumResourceTags )
			(*pdwNumResourceTags) = m_dwNumResourceTags;

		if( ppResourceTags )
			(*ppResourceTags) = m_pResourceTags;
	}


	//--------------------------------------------------------------------------------------
	// Name: Destroy
	// Desc: Cleans up the packed resource data
	//--------------------------------------------------------------------------------------
	VOID VincePackedResource::Destroy() 
	{
		delete[] m_pSysMemData;
		m_pSysMemData = NULL;
		m_dwSysMemDataSize = 0L;
	    
		if( m_pVidMemData != NULL )
			FreeContiguousMemory( m_pVidMemData );
		m_pVidMemData      = NULL;
		m_dwVidMemDataSize = 0L;
	    
		m_pResourceTags     = NULL;
		m_dwNumResourceTags = 0L;
	}

}	// namespace

#endif // !NO_VINCE_SURVEYS
#endif // _VINCE_