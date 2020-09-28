#ifndef _NEW_HPP_
#define _NEW_HPP_
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
    /*   The placement new and delete macros.                           */
    /*                                                                  */
    /*   The placement new and delete macros allow the constructor      */
    /*   and destructos of a type to be called as needed.               */
    /*                                                                  */
    /********************************************************************/

#define PLACEMENT_NEW( Address,Type )		new( Address ) Type
#define PLACEMENT_DELETE( Address,Type )	(((Type*) Address) -> ~Type())
#ifndef DISABLE_GLOBAL_NEW

    /********************************************************************/
    /*                                                                  */
    /*   The standard functions.                                        */
    /*                                                                  */
    /*   The following are standard functions used by various           */
    /*   classes in multiple applications.                              */
    /*                                                                  */
    /********************************************************************/

VOID* CDECL operator new( size_t Size );
VOID* CDECL operator new[]( size_t Size );
VOID  CDECL operator delete( VOID *Store );
VOID  CDECL operator delete[]( VOID *Store );
#endif
#endif
