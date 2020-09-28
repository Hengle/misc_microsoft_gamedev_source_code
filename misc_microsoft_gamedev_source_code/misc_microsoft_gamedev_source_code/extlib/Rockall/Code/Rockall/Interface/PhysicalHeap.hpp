#ifndef _PHYSICAL_HEAP_HPP_
#define _PHYSICAL_HEAP_HPP_

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

#include "RockallFrontEnd.hpp"

    /********************************************************************/
    /*                                                                  */
    /*   A fast heap.                                                   */
    /*                                                                  */
    /*   A fast heap tries to provide very good performance even        */
    /*   if that comes at a significant cost in terms of additional     */
    /*   memory usage.                                                  */
    /*                                                                  */
    /********************************************************************/

class ROCKALL_DLL_LINKAGE PHYSICAL_HEAP : public ROCKALL_FRONT_END
    {
   public:
        //
        //   Public functions.
        //
        PHYSICAL_HEAP
			( 
			int					  MaxFreeSpace = (2 * HalfMegabyte),
			bool					  Recycle = true,
			bool					  SingleImage = false,
			bool					  ThreadSafe = true,
			bool                WriteCombined = false
			);

		virtual void *AlignedNew
			( 
			int						  Size,
			int						  Alignment=4,
			int						  *Space = NULL,
			bool					     Zero = false
			)
		{
			assert(((Alignment+Alignment-1)& (~(Alignment-1)))==Alignment); //Check that using alignment results in proper alignment
			Size = (Size+Alignment-1)& (~(Alignment-1));
			VOID * memory = ROCKALL_FRONT_END::New(Size,Space,Zero);
#ifdef _DEBUG
			unsigned __int64 ptr = (unsigned __int64)memory;
			unsigned __int64 addr = (unsigned __int64)0x0FFFFF;
			assert(ptr > addr);
#endif
			return memory;
		}

        ~PHYSICAL_HEAP( void );

	private:
      //
      //   Disabled operations.
      //
      //   All copy constructors and class assignment 
      //   operations are disabled.
      //
        PHYSICAL_HEAP( const PHYSICAL_HEAP & Copy );

        void operator=( const PHYSICAL_HEAP & Copy );
    };
#endif
