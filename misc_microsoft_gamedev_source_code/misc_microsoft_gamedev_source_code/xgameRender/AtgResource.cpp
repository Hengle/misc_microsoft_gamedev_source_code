//--------------------------------------------------------------------------------------
// AtgResource.cpp
//
// Loads resources from an XPR (Xbox Packed Resource) file
//
// Xbox Advanced Technology Group.
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#include "xgameRender.h"
#include "AtgResource.h"
#include "asyncFileManager.h"
#include "fileUtils.h"
#include "memory\alignedAlloc.h"

namespace ATG
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

const DWORD XPR2_MAGIC_VALUE = 0x58505232;

const DWORD eXALLOCAllocatorId_AtgResource = 127;


//--------------------------------------------------------------------------------------
// Name: PackedResource
//--------------------------------------------------------------------------------------
PackedResource::PackedResource()
{
    m_pSysMemData       = NULL;
    m_dwSysMemDataSize  = 0L;
    m_pVidMemData       = NULL;
    m_dwVidMemDataSize  = 0L;
    m_pResourceTags     = NULL;
    m_dwNumResourceTags = 0L;
}


//--------------------------------------------------------------------------------------
// Name: PackedResource
//--------------------------------------------------------------------------------------
PackedResource::~PackedResource()
{
    Destroy();
}


//--------------------------------------------------------------------------------------
// Name: GetData
// Desc: Loads all the texture resources from the given XPR.
//--------------------------------------------------------------------------------------
VOID* PackedResource::GetData( const CHAR* strName ) const
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
static __forceinline void* AllocateContiguousMemory( DWORD Size, DWORD Alignment,
                                                     DWORD Protection = XALLOC_MEMPROTECT_WRITECOMBINE )
{
    return XMemAlloc( Size, MAKE_XALLOC_ATTRIBUTES( 0, 0, 0, 0, eXALLOCAllocatorId_AtgResource,
                                                    Alignment, Protection, 0,
                                                    XALLOC_MEMTYPE_PHYSICAL ) );
}


//--------------------------------------------------------------------------------------
// Name: FreeContiguousMemory()
// Desc: Wrapper for XMemFree
//--------------------------------------------------------------------------------------
static __forceinline VOID FreeContiguousMemory( VOID* pData )
{
    return XMemFree( pData, MAKE_XALLOC_ATTRIBUTES( 0, 0, 0, 0, eXALLOCAllocatorId_AtgResource,
                                                    0, 0, 0, XALLOC_MEMTYPE_PHYSICAL ) );
}


//--------------------------------------------------------------------------------------
// Name: Create
// Desc: Loads all the texture resources from the given XPR.
//--------------------------------------------------------------------------------------
HRESULT PackedResource::Create( long dirID, const CHAR* strFilename )
{
    // Open the file 
   unsigned long dwNumBytesRead;
  
   void* pData = NULL;
   if (!BFileUtils::loadFile(dirID, strFilename, &pData, &dwNumBytesRead))
   {
      tracenocrlf( "File <%s> not found\n", strFilename );
      return E_FAIL;
   }
   
   const uchar* pCurFileOfs = reinterpret_cast<const uchar*>(pData);
   uint fileLeft = dwNumBytesRead;
   
    // Read in and verify the XPR magic header
    XPR_HEADER xprh;
    if (fileLeft < sizeof(XPR_HEADER))
    {
       tracenocrlf( "Invalid Xbox Packed Resource (.xpr) file\n");
       BFileUtils::unloadFile(pData);
       return E_FAIL;
    }
      
    memcpy(&xprh, pCurFileOfs, sizeof(XPR_HEADER));
    pCurFileOfs += sizeof(XPR_HEADER);
    fileLeft -= sizeof(XPR_HEADER);
        
    if( xprh.dwMagic != XPR2_MAGIC_VALUE )
    {
        tracenocrlf( "Invalid Xbox Packed Resource (.xpr) file: Magic = 0x%08lx\n", xprh.dwMagic );
        BFileUtils::unloadFile(pData);
        return E_FAIL;
    }

    // Compute memory requirements
    m_dwSysMemDataSize = xprh.dwHeaderSize;
    m_dwVidMemDataSize = xprh.dwDataSize;

    // Allocate memory
    m_pSysMemData = new BYTE[m_dwSysMemDataSize];
    m_pVidMemData = (BYTE*)AllocateContiguousMemory( m_dwVidMemDataSize, XALLOC_PHYSICAL_ALIGNMENT_4K );

    // Read in the data from the file
    BVERIFY(fileLeft >= m_dwSysMemDataSize+m_dwVidMemDataSize);
            
    memcpy(m_pSysMemData, pCurFileOfs, m_dwSysMemDataSize);
    pCurFileOfs += m_dwSysMemDataSize;
    
    memcpy(m_pVidMemData, pCurFileOfs, m_dwVidMemDataSize);
                
    BFileUtils::unloadFile(pData);
        
    // Extract resource table from the header data
    m_dwNumResourceTags = *(DWORD*)(m_pSysMemData+0);
    m_pResourceTags     = (RESOURCE*)(m_pSysMemData+4);

    // Patch up the resources
    for( DWORD i=0; i<m_dwNumResourceTags; i++ )
    {
        m_pResourceTags[i].strName = (CHAR*)( m_pSysMemData + (DWORD)m_pResourceTags[i].strName );

        // Fixup the texture memory
        if( (m_pResourceTags[i].dwType & 0xffff0000) == (RESOURCETYPE_TEXTURE & 0xffff0000) )
        {
            D3DTexture* pTexture = (D3DTexture*)&m_pSysMemData[m_pResourceTags[i].dwOffset];
    
            // Adjust Base address according to where memory was allocated
            XGOffsetBaseTextureAddress( pTexture, m_pVidMemData, m_pVidMemData );

#if _XDK_VER < 1838
            // By default, put textures in video memory:
            pTexture->MoveResourceMemory( D3DMEM_VRAM );
#endif            
        }
    }

    return S_OK;
}


//--------------------------------------------------------------------------------------
// Name: GetResourceTags
// Desc: Retrieves the resource tags
//--------------------------------------------------------------------------------------
VOID PackedResource::GetResourceTags( DWORD* pdwNumResourceTags,
                                      RESOURCE** ppResourceTags ) const
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
VOID PackedResource::Destroy() 
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

} // namespace ATG
