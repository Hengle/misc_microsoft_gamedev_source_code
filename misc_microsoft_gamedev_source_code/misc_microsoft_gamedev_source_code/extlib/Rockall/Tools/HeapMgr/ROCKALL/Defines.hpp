#ifndef _DEFINES_HPP_
#define _DEFINES_HPP_
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

//				Global defines.

    /********************************************************************/
    /*                                                                  */
    /*   Linkage to the DLL.                                            */
    /*                                                                  */
    /*   We need to compile the class specification slightly            */
    /*   differently if we are creating the heap DLL.                   */
    /*                                                                  */
    /********************************************************************/
#undef COMPILING_ROCKALL_DLL
#ifdef COMPILING_ROCKALL_DLL
	#define ROCKALL_DLL_LINKAGE __declspec(dllexport)
#else
	#ifdef COMPILING_ROCKALL_LIBRARY
		#define ROCKALL_DLL_LINKAGE/**/
	#else
		#define ROCKALL_DLL_LINKAGE __declspec(dllimport)
	#endif
#endif

#endif //_DEFINES_HPP_