//-----------------------------------------------------------------------------
// File: VinceResourceXe.h
// Based on AtgResource.h from Xenon XDK samples
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

#pragma once

//--------------------------------------------------------------------------------------
// Name tag for resources. An app may initialize this structure, and pass
// it to the resource's Create() function. From then on, the app may call
// GetResource() to retrieve a resource using an ascii name.
//--------------------------------------------------------------------------------------
namespace Vince
{
	//--------------------------------------------------------------------------------------
	// VinceResource_SizeOf()
	//--------------------------------------------------------------------------------------
	DWORD VinceResource_SizeOf( const D3DResource* pResource );


	struct VINCERESOURCE
	{
		DWORD dwType;
		DWORD dwOffset;
		DWORD dwSize;
		CHAR* strName;
	};


	// Resource types
	enum
	{
		VINCERESOURCETYPE_USERDATA        = (('U'<<24)|('S'<<16)|('E'<<8)|('R')),
		VINCERESOURCETYPE_TEXTURE         = (('T'<<24)|('X'<<16)|('2'<<8)|('D')),
		VINCERESOURCETYPE_CUBEMAP         = (('T'<<24)|('X'<<16)|('C'<<8)|('M')),
		VINCERESOURCETYPE_VOLUMETEXTURE   = (('T'<<24)|('X'<<16)|('3'<<8)|('D')),
		VINCERESOURCETYPE_VERTEXBUFFER    = (('V'<<24)|('B'<<16)|('U'<<8)|('F')),
		VINCERESOURCETYPE_INDEXBUFFER     = (('I'<<24)|('B'<<16)|('U'<<8)|('F')),
		VINCERESOURCETYPE_EOF             = 0xffffffff
	};

	//--------------------------------------------------------------------------------------
	// Name: VincePackedResource
	//--------------------------------------------------------------------------------------
	class VincePackedResource
	{
	protected:
		BYTE*           m_pSysMemData;        // Alloc'ed memory for resource headers etc.
		DWORD           m_dwSysMemDataSize;

		BYTE*           m_pVidMemData;        // Alloc'ed memory for resource data, etc.
		DWORD           m_dwVidMemDataSize;
	 
		VINCERESOURCE*  m_pResourceTags;      // Tags to associate names with the resources
		DWORD           m_dwNumResourceTags;  // Number of resource tags

	public:
		// Loads the resources out of the specified bundle
		HRESULT Create( const CHAR* strFilename );

		VOID Destroy();

		// Retrieves the resource tags
		VOID GetResourceTags( DWORD* pdwNumResourceTags, VINCERESOURCE** ppResourceTags ) const;

		// Helper function to make sure a resource is registered
		D3DResource* RegisterResource( D3DResource* pResource ) const
		{ return pResource; }

		// Functions to retrieve resources by their offset
		VOID* GetData( DWORD dwOffset ) const
		{ return &m_pSysMemData[dwOffset]; }

		D3DResource* GetResource( DWORD dwOffset ) const
		{ return RegisterResource( (D3DResource*)GetData(dwOffset) ); }

		D3DTexture* GetTexture( DWORD dwOffset ) const
		{ return (D3DTexture*)GetResource( dwOffset ); }

		D3DCubeTexture* GetCubemap( DWORD dwOffset ) const
		{ return (D3DCubeTexture*)GetResource( dwOffset ); }

		D3DVolumeTexture* GetVolumeTexture( DWORD dwOffset ) const
		{ return (D3DVolumeTexture*)GetResource( dwOffset ); }

		D3DVertexBuffer* GetVertexBuffer( DWORD dwOffset ) const
		{ return (D3DVertexBuffer*)GetResource( dwOffset ); }

		// Functions to retrieve resources by their name
		VOID* GetData( const CHAR* strName ) const;

		D3DResource* GetResource( const CHAR* strName ) const
		{ return RegisterResource( (D3DResource*)GetData( strName ) ); }

		D3DTexture* GetTexture( const CHAR* strName ) const
		{ return (D3DTexture*)GetResource( strName ); }

		D3DCubeTexture* GetCubemap( const CHAR* strName ) const
		{ return (D3DCubeTexture*)GetResource( strName ); }

		D3DVolumeTexture* GetVolumeTexture( const CHAR* strName ) const
		{ return (D3DVolumeTexture*)GetResource( strName ); }

		D3DVertexBuffer* GetVertexBuffer( const CHAR* strName ) const
		{ return (D3DVertexBuffer*)GetResource( strName ); }

		VincePackedResource();
		~VincePackedResource();
	};

}	// namespace VINCE
