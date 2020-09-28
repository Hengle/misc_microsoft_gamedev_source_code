#ifndef _SEMAPHORE_HPP_
#define _SEMAPHORE_HPP_
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

#include "Assembly.hpp"

    /********************************************************************/
    /*                                                                  */
    /*   Constants exported from the class.                             */
    /*                                                                  */
    /*   The semaphore constants indicate the limits of a semaphore.    */
    /*                                                                  */
    /********************************************************************/

CONST SBIT32 DefaultMaxThreads		  = 256;

    /********************************************************************/
    /*                                                                  */
    /*   A control semaphore.                                           */
    /*                                                                  */
    /*   This class implements a semaphore abstraction.                 */
    /*                                                                  */
    /********************************************************************/
    
// rg [8/25/06] - HACK HACK    
class SEMAPHORE : public ASSEMBLY
{
   //
   //   Private data.
   //
   HANDLE mSemaphore;
   
public:
   //
   //   Public functions.
   //
   SEMAPHORE( SBIT32 NewMaxThreads = DefaultMaxThreads )
   {
      mSemaphore = CreateSemaphore(NULL, 0, 0x7FFFFFFF, NULL);
      if (!mSemaphore)
      {
         // FIXME
      }
   }

   VOID Signal( SBIT32 Count = 1 )
   {
      if (mSemaphore)
         ReleaseSemaphore(mSemaphore, Count, NULL);
   }

   BOOLEAN Wait( SBIT32 Timeout = INFINITE )
   {
      if (!mSemaphore)
         return False;
      HRESULT hres = WaitForSingleObject(mSemaphore, Timeout);
      if (hres == WAIT_OBJECT_0)
         return True;
      return False;
   }

   ~SEMAPHORE( VOID )
   {
      if (mSemaphore)
         CloseHandle(mSemaphore);  
   }

private:
   
   //
   //   Disabled operations.
   //
   SEMAPHORE( CONST SEMAPHORE & Copy );

   VOID operator=( CONST SEMAPHORE & Copy );
};
  
#if 0
class SEMAPHORE : public ASSEMBLY
    {
        //
        //   Private data.
        //
		SBIT32						  MaxThreads;
        VOLATILE HANDLE               Semaphore;
        VOLATILE SBIT32               Signals;
        VOLATILE SBIT32               Waiting;

    public:
        //
        //   Public functions.
        //
        SEMAPHORE( SBIT32 NewMaxThreads = DefaultMaxThreads );

        VOID Signal( SBIT32 Count = 1 );

        BOOLEAN Wait( SBIT32 Timeout = INFINITE );

        ~SEMAPHORE( VOID );

		//
		//   Public inline functions.
		//
		INLINE SBIT32 GetMaxThreads( VOID )
			{ return MaxThreads; }

		INLINE SBIT32 GetWaiting( VOID )
			{ return Waiting; }

	private:
		//
		//   Private functions.
		//
		BOOLEAN UpdateSemaphore( VOID );

        //
        //   Disabled operations.
        //
        SEMAPHORE( CONST SEMAPHORE & Copy );

        VOID operator=( CONST SEMAPHORE & Copy );
    };
#endif

#endif