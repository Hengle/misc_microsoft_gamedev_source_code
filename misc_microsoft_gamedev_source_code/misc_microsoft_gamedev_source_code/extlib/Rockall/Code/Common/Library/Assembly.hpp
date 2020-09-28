#ifndef _ASSEMBLY_HPP_
#define _ASSEMBLY_HPP_
//                                        Ruler
//       1         2         3         4         5         6         7         8
//345678901234567890123456789012345678901234567890123456789012345678901234567890

    /********************************************************************/
    /*                                                                  */
    /*   The standard layout.                                           */
    /*                                                                  */
    /*   The standard layout for 'hpp' files for this code is as        */
    /*   follows:                                                       */
    /*                                                                  */
    /*      1. Include files.                                           */
    /*      2. Constants exported from the class.                       */
    /*      3. Data structures exported from the class.                 */
	/*      4. Forward references to other data structures.             */
	/*      5. Class specifications (including inline functions).       */
    /*      6. Additional large inline functions.                       */
    /*                                                                  */
    /*   Any portion that is not required is simply omitted.            */
    /*                                                                  */
    /********************************************************************/

#include "Global.hpp"

    /********************************************************************/
    /*                                                                  */
    /*   Constants exported from the class.                             */
    /*                                                                  */
    /*   The assembly constants indicate the location of the thread     */
    /*   local store.                                                   */
    /*                                                                  */
    /********************************************************************/

#define PcTeb                         0x18
#define IDTeb                         0x24

#ifdef WINDOWS_95
CONST SBIT32 TebSlot				  = 0x88;
#else
CONST SBIT32 TebSlot				  = 0xE10;
#endif

#pragma warning( disable : 4035 )

    /********************************************************************/
    /*                                                                  */
    /*   Assembly language for ultra high performance.                  */
    /*                                                                  */
    /*   We have coded a few functions in assembly language for         */
    /*   ultra high performance.                                        */
    /*                                                                  */
    /********************************************************************/

class ASSEMBLY
    {
	protected:
		//
		//   Protected type definitions.
		//
		typedef struct
			{
	        SBIT32                    First;
	        SBIT32                    Second;
			}
		DOUBLE_SBIT32;

    public:
        //
        //   Public inline functions.
        //
		ASSEMBLY( VOID )
			{ /* void */ }

		STATIC INLINE SBIT32 AtomicAdd
				( 
				VOLATILE SBIT32		  *Address,
				SBIT32				  Value 
				)
			{
			return 
				(
				(SBIT32) InterlockedExchangeAdd
					( 
					((LPLONG) Address),
					((LONG) Value) 
					)
				);
			}

		STATIC INLINE SBIT32 AtomicCompareExchange
				( 
				VOLATILE SBIT32		  *Address,
				SBIT32				  NewValue,
				SBIT32				  Value 
				)
			{
			return 
				(
				(SBIT32) InterlockedCompareExchange
					( 
					((LONG*) Address),
					((LONG) NewValue),
					((LONG) Value)
					)
				);
			}

		STATIC INLINE VOID *AtomicCompareExchangePointer
				( 
				VOLATILE VOID		  **Address,
				VOID				  *NewPointer,
				VOID				  *OldPointer 
				)
			{ 
			return 
				(
				InterlockedCompareExchangePointer
					( 
					((VOLATILE PVOID*) Address),
					NewPointer,
					OldPointer
					)
				); 
			}


		STATIC INLINE SBIT64 AtomicCompareExchange64
				( 
				VOLATILE SBIT64		  *Address,
				SBIT64				  NewValue,
				SBIT64				  Value 
				)
			{
#ifdef ASSEMBLY_X86
			__asm
				{
				mov		esi, Address				// Load the address.
				mov		ebx, dword ptr NewValue[0]	// Load the new value.
				mov		ecx, dword ptr NewValue[4]	// Load the new value.
				mov		eax, dword ptr Value[0]		// Load the value.
				mov		edx, dword ptr Value[4]		// Load the value.
				lock	cmpxchg8b [esi]				// Update the value.
				}
#elif WINDOWS64
			return 
				(
				InterlockedCompareExchange64
					( 
					((VOLATILE SBIT64*) Address),
					NewValue,
					Value
					)
				);
#elif _XENON
			return 
				(
				InterlockedCompareExchange64
					( 
					((LONG64 VOLATILE *) Address),
					NewValue,
					Value
					)
				);
#else
#if defined(_PREFIX_) || defined(_PREFAST_)
				SBIT64 original = *Address;
				if (original == Value)
					*Address = NewValue;
				return original;
#else
#error No Interlocked 64 bit exchange.
#endif
#endif
			}

		STATIC INLINE SBIT32 AtomicDecrement( VOLATILE SBIT32 *Address )
			{ return ((SBIT32) InterlockedDecrement( ((LONG*) Address) )); }

		STATIC INLINE VOID AtomicDoubleDecrement( VOLATILE SBIT32 *Address )
			{
			AUTO DOUBLE_SBIT32 Original;
			AUTO DOUBLE_SBIT32 Update;

			do
				{
				//
				//   Clone the current values.
				//
				(*((SBIT64*) & Original)) = (*((SBIT64*) Address));
				(*((SBIT64*) & Update)) = (*((SBIT64*) & Original));

				//
				//   Update the current values.
				//
				Update.First --;
				Update.Second --;
				}
			while
				( 
				AtomicCompareExchange64
					( 
					((SBIT64*) Address),
					(*((SBIT64*) & Update)),
					(*((SBIT64*) & Original))
					) 
						!= 
				(*((SBIT64*) & Original))
				);
			}

		STATIC INLINE VOID AtomicDoubleIncrement( VOLATILE SBIT32 *Address )
			{
			AUTO DOUBLE_SBIT32 Original;
			AUTO DOUBLE_SBIT32 Update;

			do
				{
				//
				//   Clone the current values.
				//
				(*((SBIT64*) & Original)) = (*((SBIT64*) Address));
				(*((SBIT64*) & Update)) = (*((SBIT64*) & Original));

				//
				//   Update the current values.
				//
				Update.First ++;
				Update.Second ++;
				}
			while
				( 
				AtomicCompareExchange64
					( 
					((SBIT64*) Address),
					(*((SBIT64*) & Update)),
					(*((SBIT64*) & Original))
					) 
						!= 
				(*((SBIT64*) & Original))
				);
			}

		STATIC INLINE SBIT32 AtomicExchange
				( 
				VOLATILE SBIT32		  *Address,
				SBIT32				  NewValue 
				)
			{
			return 
				(
				(SBIT32) InterlockedExchange
					( 
					((LONG*) Address),
					((LONG) NewValue) 
					)
				);
			}

		STATIC INLINE VOID *AtomicExchangePointer
				( 
				VOLATILE VOID		  **Address,
				VOID				  *NewPointer 
				)
			{ 
			return 
				(
				InterlockedCompareExchangePointer
					( 
					((VOLATILE PVOID*) Address),
					NewPointer,
					((PVOID) (*Address))
					)
				); 
			}

		STATIC INLINE SBIT32 AtomicIncrement( VOLATILE SBIT32 *Address )
			{ return ((SBIT32) InterlockedIncrement( ((LONG*) Address) )); }

		STATIC INLINE SBIT32 GetThreadId( VOID )
			{ return ((SBIT32) GetCurrentThreadId()); }

		STATIC INLINE VOID Pause( VOID )
			{
#ifdef ASSEMBLY_X86
			__asm
				{
				pause								// Pause for Jackson MP.
				}
#endif
#ifdef WINDOWS64
			_mm_pause();
#endif
			}

		STATIC INLINE VOID PrefetchL1( VOID *Address )
			{
#ifdef ASSEMBLY_X86
			__asm
				{
				mov			eax,Address				// Load the address.
				prefetcht0	[eax]					// Prefetch into the L1.
				}
#elif WINDOWS64
			_mm_prefetch( ((CHAR*) Address),_MM_HINT_T0 );
#elif POWERPC
			__dcbt(0,Address);
#endif
			}

		STATIC INLINE VOID PrefetchL2( VOID *Address )
			{
#ifdef ASSEMBLY_X86
			__asm
				{
				mov			eax,Address				// Load the address.
				prefetcht1	[eax]					// Prefetch into the L2.
				}
#elif WINDOWS64
			_mm_prefetch( ((CHAR*) Address),_MM_HINT_T1 );
#elif POWERPC
				__dcbt(0,Address);
#endif
			}

		STATIC INLINE VOID PrefetchL3( VOID *Address )
			{
#ifdef ASSEMBLY_X86
			__asm
				{
				mov			eax,Address				// Load the address.
				prefetcht2	[eax]					// Prefetch into the L3.
				}
#elif WINDOWS64
			_mm_prefetch( ((CHAR*) Address),_MM_HINT_T2 );
#endif
			}

		STATIC INLINE VOID PrefetchNta( VOID *Address )
			{
#ifdef ASSEMBLY_X86
			__asm
				{
				mov			eax,Address				// Load the address.
				prefetchnta	[eax]					// Prefetch into the L1.
				}
#elif WINDOWS64
			_mm_prefetch( ((CHAR*) Address),_MM_HINT_NTA );
#endif
			}

#ifdef ASSEMBLY_X86
#ifdef ENABLE_NON_STANDARD_ASSEMBLY
		STATIC INLINE VOID *GetTlsAddress( SBIT32 TlsOffset )
			{
			__asm
				{
				mov		eax,TlsOffset				// Load the TLS offset.
				add		eax,fs:[PcTeb]				// Add TEB base address.
				}
			}
#endif
#endif

		STATIC INLINE VOID *GetTlsValue
				( 
				SBIT32				  TlsIndex,
				SBIT32				  TlsOffset
				)
			{
#ifdef ASSEMBLY_X86
#ifdef ENABLE_NON_STANDARD_ASSEMBLY
			__asm
				{
				mov		edx,TlsOffset				// Load the TLS offset.
				add		edx,fs:[PcTeb]				// Add TEB base address.
				mov		eax,[edx]					// Load TLS value.
				}
#else
			return (TlsGetValue( ((DWORD) TlsIndex) ));
#endif
#else
			return (TlsGetValue( ((DWORD) TlsIndex) ));
#endif
			}

		STATIC INLINE VOID SetTlsValue
				( 
				SBIT32				  TlsIndex,
				SBIT32				  TlsOffset,
				VOID				  *NewPointer 
				)
			{
#ifdef ASSEMBLY_X86
#ifdef ENABLE_NON_STANDARD_ASSEMBLY
			__asm
				{
				mov		edx,TlsOffset				// Load the TLS offset.
				add		edx,fs:[PcTeb]				// Add TEB base address.
				mov		ecx,NewPointer				// Load new TLS value.
				mov		[edx],ecx					// Store new TLS value.
				}
#else
			(VOID) TlsSetValue( ((DWORD) TlsIndex),NewPointer );
#endif
#else
			(VOID) TlsSetValue( ((DWORD) TlsIndex),NewPointer );
#endif
			}

#if 0         
       STATIC INLINE VOID MemoryBarrier ( VOID )
       {
#if POWERPC       
         __lwsync();
#endif
       }
#endif       

		~ASSEMBLY( VOID )
			{ /* void */ }

	private:
        //
        //   Disabled operations.
        //
        ASSEMBLY( CONST ASSEMBLY & Copy );

        VOID operator=( CONST ASSEMBLY & Copy );
    };

#pragma warning( default : 4035 )
#endif
