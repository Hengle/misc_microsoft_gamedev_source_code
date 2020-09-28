#ifndef _ROCKALL_PHYSICAL_BACK_END_HPP_
#define _ROCKALL_PHYSICAL_BACK_END_HPP_
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

#include <stddef.h>
#include "RockallBackEnd.hpp"

/********************************************************************/
/*                                                                  */
/*   The memory allocation support services.                        */
/*                                                                  */
/*   This allocator provides physical memory for user-requested		*/
/*   memory. System memory still goes through VirtualAlloc          */
/*                                                                  */
/********************************************************************/

class ROCKALL_DLL_LINKAGE ROCKALL_PHYSICAL_BACK_END : public ROCKALL_BACK_END
{
	//
	//   Private static data.
	//
	static ROCKALL_PHYSICAL_BACK_END		  DefaultBaseClass;
	static ROCKALL_PHYSICAL_BACK_END		  DefaultBaseClassWC;

public:
	//
	//   Low level heap interface.
	//
	//   The following group of functions are called by the
	//   heap to aquire or release large blocks of memory.
	//   These functions can be overloaded to enable the
	//   heap work in constrained environments.
	//
	ROCKALL_PHYSICAL_BACK_END( DWORD protect = PAGE_READWRITE );

	virtual void DeleteArea( void *Memory,int Size,bool User );

	// Not overloading this right now because we don't know better right now
	//virtual int NaturalSize( void )	{return ENVIRONMENT::AllocationSize();}

	virtual void *NewArea( int AlignMask,int Size,bool User );

	virtual ~ROCKALL_PHYSICAL_BACK_END( void );

	//
	//   Static public inline functions.
	//
	static ROCKALL_PHYSICAL_BACK_END *RockallPhysicalBackEnd( bool WriteCombined )
	{ return WriteCombined ? &DefaultBaseClassWC : &DefaultBaseClass; }

private:
	//
	//   Disabled operations.
	//
	//   All copy constructors and class assignment 
	//   operations are disabled.
	//
	ROCKALL_PHYSICAL_BACK_END( const ROCKALL_PHYSICAL_BACK_END & Copy );

	void operator=( const ROCKALL_PHYSICAL_BACK_END & Copy );
	
	DWORD mProtect;
};
#endif

