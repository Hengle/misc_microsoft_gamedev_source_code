//////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006-2008 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

/// \file 
/// Memory Manager namespace.

#ifndef _AKMEMORYMGR_H_
#define _AKMEMORYMGR_H_

#include <AK/SoundEngine/Common/AkTypes.h>
#include <AK/SoundEngine/Common/AkSoundEngineExport.h>

#ifndef AK_OPTIMIZED

/// Set the debug name for a pool. This is the name shown in the Memory tab of the Advanced Profiler.
#define AK_SETPOOLNAME( _poolid, _name )				\
	if( AK_INVALID_POOL_ID != _poolid )					\
	{													\
		AK::MemoryMgr::SetPoolName( _poolid, _name );	\
	}

//#define AK_MEMDEBUG

#else
#define AK_SETPOOLNAME(_poolid,_name)
#endif

/// Memory pool attributes.
/// Block allocation type determines the method used to allocate
/// a memory pool (3 bits). Block management type determines the
/// method used to manage memory blocks (1 bit). Note that
/// the list of values in this enum is platform-dependant.
/// \sa
/// - AkMemoryMgr::CreatePool()
/// - AK::Comm::DEFAULT_MEMORY_POOL_ATTRIBUTES
enum AkMemPoolAttributes
{
	AkNoAlloc		= 0,	///< Block allocation type: No need to allocate
	AkMalloc		= 1,	///< Block allocation type: Use AK::AllocHook()

#if defined(WIN32)
	AkVirtualAlloc	= 2,	///< Block allocation type: Use AK::VirtualAllocHook() (Win32 & Xbox360 only)
	AkAllocMask = AkNoAlloc | AkMalloc | AkVirtualAlloc,					///< Block allocation type mask (3 bits)
#elif defined(XBOX360)
	AkVirtualAlloc	= 2,	///< Block allocation type: Use AK::VirtualAllocHook() (Win32 & Xbox360 only)
	AkPhysicalAlloc	= 3,	///< Block allocation type: Use AK::PhysicalAllocHook() (Xbox360 only)
	AkAllocMask = AkNoAlloc | AkMalloc | AkVirtualAlloc | AkPhysicalAlloc,	///< Block allocation type mask (3 bits)
#elif defined (AK_PS3)
	AkAllocMask = AkNoAlloc | AkMalloc,										///< Block allocation type mask (3 bits)
#elif defined(RVL_OS)
	AkMallocMEM2	= 4,	///< Block allocation type: Use AK::AllocMEM2Hook() (Wii only)
	AkAllocMask = AkNoAlloc | AkMalloc | AkMallocMEM2,						///< Block allocation type mask (3 bits)
#endif

	AkFixedSizeBlocksMode	= 1<<3,			///< Block management type: Fixed-size blocks. Get blocks through GetBlock/ReleaseBlock API
	AkBlockMgmtMask	= AkFixedSizeBlocksMode	///< Block management type mask (1 bit)
};

namespace AK
{   
	/// External allocation hook for the Memory Manager. Called by the Audiokinetic 
	/// implementation of the Memory Manager when creating a pool of type AkMalloc.
	/// \aknote This needs to be defined by the client. \endaknote
	/// \return A pointer to the start of the allocated memory (NULL if the system is out of memory)
	/// \sa
	/// - \ref memorymanager
	/// - AK::VirtualAllocHook()
	/// - AK::FreeHook()
	extern void * AllocHook( 
		size_t in_size			///< Number of bytes to allocate
		);

	/// External deallocation hook for the Memory Manager. Called by the Audiokinetic 
	/// implementation of the Memory Manager when destroying a pool of type AkMalloc.
	/// \aknote This needs to be defined by the client. \endaknote
	/// \sa 
	/// - \ref memorymanager
	/// - AK::VirtualAllocHook()
	/// - AK::AllocHook()
	extern void	FreeHook( 
		void * in_pMemAddress	///< Pointer to the start of memory allocated with AllocHook
		);

#if defined(WIN32) || defined(XBOX360)
	/// External allocation hook for the Memory Manager. Called by the Audiokinetic 
	/// implementation of the Memory Manager when creating a pool of type AkVirtualAlloc.
	/// \aknote This needs to be defined by the client, who must allocate memory using VirtualAlloc. \endaknote
	/// \return A pointer to the start of the allocated memory (NULL if the system is out of memory)
	/// \sa 
	/// - \ref memorymanager
	/// - AK::AllocHook()
	/// - AK::FreeHook()
	/// - AK::VirtualFreeHook()
	extern void * VirtualAllocHook( 
		void * in_pMemAddress,		///< Parameter for VirtualAlloc
		size_t in_size,				///< Number of bytes to allocate
		DWORD in_dwAllocationType,	///< Parameter for VirtualAlloc
		DWORD in_dwProtect			///< Parameter for VirtualAlloc
		);

	/// External deallocation hook for the Memory Manager. Called by the Audiokinetic 
	/// implementation of the Memory Manager when destroying a pool of type AkVirtualAlloc.
	/// \aknote This needs to be defined by the client, who must deallocate memory using VirtualFree. \endaknote
	/// \sa 
	/// - \ref memorymanager
	/// - AK::VirtualAllocHook()
	/// - AK::AllocHook()
	/// - AK::FreeHook()
	extern void	VirtualFreeHook( 
		void * in_pMemAddress,	///< Pointer to the start of memory allocated with VirtualAllocHook
		size_t in_size,			///< Parameter for VirtualFree
		DWORD in_dwFreeType		///< Parameter for VirtualFree
		);
#endif // WIN32 || XBOX360

#if defined(XBOX360)
	/// External allocation hook for the Memory Manager. Called by the Audiokinetic 
	/// implementation of the Memory Manager when creating a pool of type AkPhysicalAlloc.
	/// \aknote This needs to be defined by the client, who must allocate memory using XPhysicalAlloc. \endaknote
	/// \return A pointer to the start of the allocated memory. NULL if the system is out of memory.
	/// \sa 
	/// - \ref memorymanager
	/// - AK::AllocHook()
	/// - AK::PhysicalFreeHook()
	extern void * PhysicalAllocHook( 
		size_t in_size,					///< Number of bytes to allocate
		ULONG_PTR in_ulPhysicalAddress, ///< Parameter for XPhysicalAlloc
		ULONG_PTR in_ulAlignment,		///< Parameter for XPhysicalAlloc
		DWORD in_dwProtect				///< Parameter for XPhysicalAlloc
		);

	/// External deallocation hook for the Memory Manager. Called by the Audiokinetic 
	/// implementation of the Memory Manager when destroying a pool of type AkPhysicalAlloc.
	/// \aknote This needs to be defined by the client, and memory must be deallocated using XPhysicalFree. \endaknote
	/// \sa 
	/// - \ref memorymanager
	/// - AK::FreeHook()
	/// - AK::PhysicalAllocHook()
	extern void	PhysicalFreeHook( 
		void * in_pMemAddress	///< Pointer to the start of memory allocated with PhysicalAllocHook
		);
#endif // XBOX360

#if defined(RVL_OS)
	/// External allocation hook for the Memory Manager. Called by the Audiokinetic 
	/// implementation of the Memory Manager when creating a pool of type AkMallocMEM2.
	/// \aknote This needs to be defined by the client. \endaknote
	/// \return A pointer to the start of the allocated memory (NULL if the system is out of memory)
	/// \sa
	/// - \ref memorymanager
	/// - AK::FreeMEM2Hook()
	extern void * AllocMEM2Hook( 
		size_t in_size			///< Number of bytes to allocate
		);

	/// External deallocation hook for the Memory Manager. Called by the Audiokinetic 
	/// implementation of the Memory Manager when destroying a pool of type AkMallocMEM2.
	/// \aknote This needs to be defined by the client. \endaknote
	/// \sa 
	/// - \ref memorymanager
	/// - AK::AllocMEM2Hook()
	extern void	FreeMEM2Hook( 
		void * in_pMemAddress	///< Pointer to the start of memory allocated with AllocHook
		);
#endif

	/// Memory Manager namespace.
	/// \remarks The functions in this namespace are thread-safe, unless stated otherwise.
	/// \sa
	/// - \ref memorymanager
	namespace MemoryMgr
	{
		/// Memory pool statistics. 
		/// \sa 
		/// - AK::MemoryMgr::GetPoolStats()
		/// - \ref memorymanager
		struct PoolStats
		{
			// Current state
			AkUInt32 uReserved;		///< Reserved memory (in bytes)
			AkUInt32 uUsed;			///< Used memory (in bytes)
			AkUInt32 uMaxFreeBlock;	///< Size of biggest free block (in bytes)

			// Statistics
			AkUInt32 uAllocs;		///< Number of Alloc calls since initialization
			AkUInt32 uFrees;		///< Number of Free calls since initialization
			AkUInt32 uPeakUsed;		///< Peak used memory (in bytes)
		};

		/// Query whether the Memory Manager has been sucessfully initialized.
		/// \warning This function is not thread-safe. It should not be called at the same time as MemoryMgr::Init or MemoryMgr::Term.
		/// \return True if the Memory Manager is initialized, False otherwise
		/// \sa 
		/// - AK::MemoryMgr::Init()
		/// - \ref memorymanager
		extern AKMEMORYMGR_API bool IsInitialized();

		/// Terminate the Memory Manager.
		/// \warning This function is not thread-safe. 
		/// \sa
		/// - \ref memorymanager
	    extern AKMEMORYMGR_API void Term();

		////////////////////////////////////////////////////////////////////////
		/// @name Memory Pools
		//@{

		/// Create a new memory pool.
		/// \return The ID of the created memory pool, or AK_INVALID_POOL_ID if creation failed
		/// \aktip
		/// Refer to \ref memorymanager_pools for information about pool resource overhead.
		/// \endaktip
		/// \sa
		/// - \ref memorymanager
		extern AKMEMORYMGR_API AkMemPoolId CreatePool(
			void *			in_pMemAddress,		///< Memory address of the pool, or NULL if it should be allocated
			AkUInt32		in_uMemSize,		///< Size of the pool (in bytes)
			AkUInt32		in_uBlockSize,		///< Size of a block (in bytes)
			AkUInt32		in_eAttributes,		///< Memory pool attributes: use values of AkMemPoolAttributes.	
			AkUInt32        in_uBlockAlign = 0	///< Alignment of memory blocks
			);

		/// Set the name of a memory pool.
		/// \return AK_Success if successful
		/// \sa
		/// - \ref memorymanager
		extern AKMEMORYMGR_API AKRESULT SetPoolName( 
			AkMemPoolId     in_poolId,			///< ID of memory pool
			AkTChar*        in_pszPoolName		///< Pointer to name string
			);

		/// Get the name of a memory pool.
		/// \return A pointer to the name of the memory pool (NULL if the operation failed)
		/// \sa
		/// - \ref memorymanager
		extern AKMEMORYMGR_API AkTChar* GetPoolName( 
			AkMemPoolId     in_poolId			///< ID of memory pool
			);
		
        /// Enables or disables error notifications posted by a memory pool.
        /// The notifications are enabled by default when creating a pool.
        /// They are always disabled in the AK_OPTIMIZED build.
        /// \return AK_Success if the pool exists
		/// \sa
		/// - \ref memorymanager
		extern AKMEMORYMGR_API AKRESULT SetMonitoring(
            AkMemPoolId     in_poolId,			///< ID of memory pool
            bool            in_bDoMonitor       ///< Enables error monitoring (has no effect in AK_OPTIMIZED build)
            );

	    /// Destroy a memory pool.
		/// \return AK_Success if successful
		/// \sa
		/// - \ref memorymanager
	    extern AKMEMORYMGR_API AKRESULT DestroyPool(
			AkMemPoolId     in_poolId			///< ID of memory pool
		    );

        /// Get a memory pool's statistics.
		/// \sa
		/// - \ref memorymanager
	    extern AKMEMORYMGR_API AKRESULT GetPoolStats(
			AkMemPoolId     in_poolId,			///< ID of memory pool
			PoolStats&      out_stats			///< Returned statistics structure
		    );

		/// Get the current number of memory pools.
		/// \return The current number of memory pools
		/// \sa
		/// - \ref memorymanager
		extern AKMEMORYMGR_API AkInt32 GetNumPools();

		/// Get the maximum number of memory pools.
		/// \return The maximum number of memory pools
		/// \sa
		/// - \ref memorymanager
		extern AKMEMORYMGR_API AkInt32 GetMaxPools();

		/// Test the validity of a pool ID.
		/// This is used to verify the validity of a memory pool ID.
		/// \return AK_Success if the pool exists, AK_InvalidID otherwise
		/// \sa
		/// - \ref memorymanager
		extern AKMEMORYMGR_API AKRESULT CheckPoolId(
			AkMemPoolId		in_poolId			///< ID of memory pool to test
			);

		/// Get pool attributes.
		/// \return The memory pool's attributes.
		/// \sa
		/// - \ref memorymanager
		extern AKMEMORYMGR_API AkMemPoolAttributes GetPoolAttributes(
			AkMemPoolId		in_poolId			///< ID of memory pool
			);

		//@}

		////////////////////////////////////////////////////////////////////////
		/// @name Memory Allocation
		//@{

#if defined (AK_MEMDEBUG)
		/// Allocate memory from a pool: debug version.
		/// \return A pointer to the start of the allocated memory (NULL if the system is out of memory)
		/// \sa
		/// - \ref memorymanager
	    extern AKMEMORYMGR_API void * dMalloc(
			AkMemPoolId in_poolId,				///< ID of the memory pool
		    size_t		in_uSize,				///< Number of bytes to allocate
		    AkTChar*	in_pszFile,				///< Debug file name
		    AkUInt32	in_uLine				///< Debug line number
			);
#else
		/// Allocate memory from a pool.
		/// \return A pointer to the start of the allocated memory (NULL if the system is out of memory)
		/// \sa
		/// - \ref memorymanager
	    extern AKMEMORYMGR_API void * Malloc(
			AkMemPoolId in_poolId,				///< ID of the memory pool
		    size_t		in_uSize 				///< Number of bytes to allocate
		    );
#endif
		/// Free memory from a pool.
		/// \return AK_Success if successful
		/// \sa
		/// - \ref memorymanager
	    extern AKMEMORYMGR_API AKRESULT Free(
			AkMemPoolId in_poolId,				///< ID of the memory pool
			void *		in_pMemAddress			///< Pointer to the start of memory allocated with Malloc
		    );

		/// Allocate memory from a pool, overriding the pool's default memory alignment. Needs to be used
		/// in conjunction with AK::MemoryMgr::Falign.
		/// \return A pointer to the start of the allocated memory (NULL if the system is out of memory)
		/// \sa
		/// - \ref memorymanager
	    extern AKMEMORYMGR_API void * Malign(
			AkMemPoolId in_poolId,				///< ID of the memory pool
		    size_t		in_uSize, 				///< Number of bytes to allocate
		    AkUInt32	in_uAlignment 			///< Alignment (in bytes)
		    );

		/// Free memory from a pool, overriding the pool's default memory alignment. Needs to be used in
		/// conjunction with AK::MemoryMgr::Malign.
		/// 
		/// \return AK_Success if successful
		/// \sa
		/// - \ref memorymanager
	    extern AKMEMORYMGR_API AKRESULT Falign(
			AkMemPoolId in_poolId,				///< ID of the memory pool
			void *		in_pMemAddress			///< Pointer to the start of memory allocated with Malloc
		    );

		//@}

		////////////////////////////////////////////////////////////////////////
		/// @name Fixed-Size Blocks Memory Allocation Mode
		//@{

		/// Get a block from a Fixed-Size Block type pool. To be used with pools created with AkFixedSizeBlocksMode
		/// block management type, along with any of the block allocation types.
		/// \return A pointer to the start of the allocated memory (NULL if the system is out of memory)
		///			The size of the memory block is always in_uBlockSize, specified in AK::MemoryMgr::CreatePool.
		/// \warning This method is not thread-safe. Fixed-Size Block pool access must be protected.
		/// \sa AK::MemoryMgr::CreatePool
		/// \sa AkMemPoolAttributes
		/// - \ref memorymanager
	    extern AKMEMORYMGR_API void * GetBlock(
			AkMemPoolId in_poolId				///< ID of the memory pool
		    );

		/// Free memory from a Fixed-Size Block type pool.
		/// \return AK_Success if successful
		/// \warning This method is not thread-safe. Fixed-Size Block pool access must be protected.
		/// \sa
		/// - \ref memorymanager
	    extern AKMEMORYMGR_API AKRESULT ReleaseBlock(
			AkMemPoolId in_poolId,				///< ID of the memory pool
			void *		in_pMemAddress			///< Pointer to the start of memory allocated with Malloc
		    );

		//@}
    }
}

#endif // _AKMEMORYMGR_H_
