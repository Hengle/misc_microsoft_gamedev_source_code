#ifndef _SORTED_FIND_HPP_
#define _SORTED_FIND_HPP_
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
    /*   Include files for inherited classes.                           */
    /*                                                                  */
    /*   The include files for inherited classes are required in the    */
    /*   specification of this class.                                   */
    /*                                                                  */
    /********************************************************************/

#include "Lock.hpp"
#include "Vector.hpp"

    /********************************************************************/
    /*                                                                  */
    /*   Constants exported from the class.                             */
    /*                                                                  */
    /*   The sorted find constants specify the initial size of          */
    /*   the list.                                                      */
    /*                                                                  */
    /********************************************************************/

CONST SBIT32 DefaultAlignmentSize	  = 1;
CONST SBIT32 DefaultListSize		  = 1024;

    /********************************************************************/
    /*                                                                  */
    /*   Sorted storage and lookup.                                     */
    /*                                                                  */
    /*   It is not uncommon to want to store sorted data or to try to   */
    /*   find the closest match.  We combine this functionality into    */
    /*   a single combined class.                                       */
    /*                                                                  */
    /********************************************************************/

template <class TYPE,class LOCK=NO_LOCK> class SORTED_FIND : public LOCK
    {
        //
        //   Private data.
        //
        SBIT32                        MaxList;
        SBIT32                        ListUsed;

        VECTOR<TYPE>	              List;

    public:
        //
        //   Public functions.
        //
        SORTED_FIND
			( 
			SBIT32                    NewMaxOrdered = DefaultListSize, 
			SBIT32                    Alignment = DefaultAlignmentSize 
			);

        VOID AddToList( CONST TYPE & Value );

		VIRTUAL SBIT32 CompareValues
			( 
			CONST TYPE				  & Value1,
			CONST TYPE				  & Value2 
			);

		BOOLEAN ExtractValue( SBIT32 Offset,TYPE *Type );

		BOOLEAN FindClosest
			( 
			CONST TYPE				  & Value,
			SBIT32					  *Before,
			SBIT32					  *After
			);

        VOID RemoveFromList( SBIT32 Offset );

        VIRTUAL ~SORTED_FIND( VOID );

		//
		//   Public inline functions.
		//
		SBIT32 GetSize( VOID )
			{ return ListUsed; }

	protected:
		//
		//   Protected inline functions.
		//
		TYPE *GetList( VOID )
			{ return & List[0]; }

	private:
		//
		//   Private functions.
		//
		BOOLEAN FindClosestValues
			( 
			CONST TYPE				  & Value,
			SBIT32					  *Before,
			SBIT32					  *After
			);

        //
        //   Disabled operations.
        //
        SORTED_FIND( CONST SORTED_FIND & Copy );

        VOID operator=( CONST SORTED_FIND & Copy );
    };

    /********************************************************************/
    /*                                                                  */
    /*   Class constructor.                                             */
    /*                                                                  */
    /*   Create a new sorted find list and prepare it for use.  This    */
    /*   call is not thread safe and should only be made in a single    */
    /*   thread environment.                                            */
    /*                                                                  */
    /********************************************************************/

template <class TYPE,class LOCK> SORTED_FIND<TYPE,LOCK>::SORTED_FIND
		( 
		SBIT32						  NewMaxList,
		SBIT32                        Alignment
		) :
		//
		//   Call the constructors for the contained classes.
		//
		List( NewMaxList,Alignment,CacheLineSize )
    {
	//
	//   Lets just make sure that the list size
	//   appears to be reasonable.
	//
    if ( NewMaxList > 0 ) 
        {
		//
		//   Setup the hash table size information.
		//
        MaxList = NewMaxList;
        ListUsed = 0;
        }
    else
        { Failure( "Max size in constructor for SORTED_FIND" ); }
    }

    /********************************************************************/
    /*                                                                  */
    /*   Add to the sorted list.                                        */
    /*                                                                  */
    /*   We add an item to the sorted list if it does not already       */
    /*   exist.  If there is not enough space we expand the size        */
    /*   of the 'List'.                                                 */
    /*                                                                  */
    /********************************************************************/

template <class TYPE,class LOCK> VOID SORTED_FIND<TYPE,LOCK>::AddToList
        (
        CONST TYPE					  & Value 
        )
    {
	AUTO SBIT32 *After;
	AUTO SBIT32 *Before;

	//
	//   Claim an exclusive lock (if enabled).
	//
    ClaimExclusiveLock();

	//
	//   Find the closest match to the value.
	//
	if ( FindClosestValues( Value,& Before,& After ) )
		{
		//
		//   We insert the new value in the sorted
		//   list unless we have an exact match.
		//   If we have an exact match we overwrite
		//   the existing value.
		//
		if ( Before != After )
			{
			//
			//   If the array is full then resize it.
			//   We need to be careful here as a resize
			//   can change the address of the 'List'
			//   array.
			//
			if ( MaxList < ListUsed  )
				{ List.Resize( (MaxList *= ExpandStore) ); }

			//
			//   We have a special case when the list
			//   wraps as this means we insert at the
			//   end of the list.  If not then the normal
			//   case is to copy down the existing 
			//   values and insert the new value.
			//
			if ( Before < After )
				{
				REGISTER SBIT32 Count;

				//
				//   The existing values are copied further
				//   down the list.
				//
				for ( Count=ListUsed ++;Count > After;Count -- )
					{ List[ Count ] = List[ (Count-1) ]; }

				//
				//   Insert the new value.
				//
				List[ After ] = Value;
				}
			else
				{ List[ (ListUsed ++) ] = Value; }
			}
		else
			{ List[ After ] = Value; }
		}
	else
		{ List[ (ListUsed ++) ] = Value; }

	//
	//   Release any lock we got earlier.
	//
	ReleaseExclusiveLock();
    }

    /********************************************************************/
    /*                                                                  */
    /*   Compare two values.          .                                 */
    /*                                                                  */
    /*   Compare two values to understand the relationship.             */
    /*                                                                  */
    /********************************************************************/

template <class TYPE,class LOCK> SBIT32 SORTED_FIND<TYPE,LOCK>::CompareValues
        (
		CONST TYPE					  & Value1,
		CONST TYPE					  & Value2
        )
	{ return ((SBIT32) (Value2 - Value1)); }

    /********************************************************************/
    /*                                                                  */
    /*   Extract a list value.        .                                 */
    /*                                                                  */
    /*   Extact a list value and return for examination.                */
    /*                                                                  */
    /********************************************************************/

template <class TYPE,class LOCK> BOOLEAN SORTED_FIND<TYPE,LOCK>::ExtractValue
        (
		SBIT32						  Offset,
		TYPE						  *Value
        )
    {
	REGISTER BOOLEAN Result;

	//
	//   Claim an shared lock (if enabled).
	//
    ClaimSharedLock();

	//
	//   Extract the value from the list if it
	//   exists.
	//
	if ( (Offset >= 0) && (Offset < ListUsed) )
		{
		(*Value) = List[ Offset ];

		Result = True;
		}
	else
		{ Result = False; }

	//
	//   Release any lock we got earlier.
	//
	ReleaseSharedLock();

	return Result;
    }

    /********************************************************************/
    /*                                                                  */
    /*   Find the closest match.                                        */
    /*                                                                  */
    /*   Find the closest matches in the sorted list.                   */
    /*                                                                  */
    /********************************************************************/

template <class TYPE,class LOCK> BOOLEAN SORTED_FIND<TYPE,LOCK>::FindClosest
        (
		CONST TYPE					  & Value,
        SBIT32						  *Before, 
        SBIT32						  *After 
        )
    {
	REGISTER BOOLEAN Result;

	//
	//   Claim an shared lock (if enabled).
	//
    ClaimSharedLock();

	//
	//   Find the closest matches.
	//
	Result = FindClosestValues( Value,Before,After );

	//
	//   Release any lock we got earlier.
	//
	ReleaseSharedLock();

	return Result;
    }

    /********************************************************************/
    /*                                                                  */
    /*   Find the closest values.                                       */
    /*                                                                  */
    /*   Find the closest matching values in the sorted list.           */
    /*                                                                  */
    /********************************************************************/

template <class TYPE,class LOCK> BOOLEAN SORTED_FIND<TYPE,LOCK>::FindClosestValues
        (
		CONST TYPE					  & Value,
        SBIT32						  *Before, 
        SBIT32						  *After 
        )
	{
	REGISTER BOOLEAN Found = False;

	//
	//   Claim an shared lock (if enabled).
	//
    ClaimSharedLock();

	//
	//   If the list is empty we claerly can't find
	//   anybody who is close so we need to check for
	//   this now.
	//
	if ( ListUsed > 0 )
		{
		REGISTER SBIT32 First = CompareValues( Value,List[ 0 ] );
		REGISTER SBIT32 Last = CompareValues( Value,List[ (ListUsed-1) ] );

		//
		//   We are about to start a classical binary
		//   search so lets set the start and end indicators
		//   at the two ends of the list.
		//
		Before = 0;
		After = (ListUsed-1);

		//
		//   We do something special in this binary search
		//   in that we always indicate the two closest
		//   matches if an exact hit is not found.  We need
		//   to consider the case where there is a wrap around
		//   (this could be very nasty).
		//
		if ( (First > 0) && (Last < 0) )
			{
			REGISTER SBIT32 Current;

			//
			//   We now know that the target value is in the
			//   list somewhere but it is not a special case
			//   of any kind.  So lets go and find it.
			//
			for 
					( 
					Current=(After / 2);
					! Found;
					Current=((Before + After) / 2)
					)
				{
				REGISTER SBIT32 Delta = ((*After) - (*Before));

				//
				//   Lets compare the target value with
				//   the current value.
				//
				switch ( CompareValues( Value,List[ Current ] ) )
					{
					case -1:
						{
						//
						//   The target value is greater
						//   than the current value.  If
						//   we are not within 1 step try
						//   again otherwise exit as we have
						//   found the closest values.
						//
						if ( Delta > 1 )
							{ (*After) = Current; }
						else
							{ Found = True; }
						break;
						}

					case 0:
						{
						//
						//   A direct hit.  We have found
						//   the target value.
						//
						(*Before) = (*After) = Current;

						Found = True;

						break;
						}

					case 1:
						{
						//
						//   The target value is smaller
						//   than the current value.  If
						//   we are not within 1 step try
						//   again otherwise exit as we have
						//   found the closest values.
						//
						if ( Delta > 1 )
							{ (*Before) = Current; }
						else
							{ Found = True; }
						break;
						}
					}
				}
			}
		else
			{
			//
			//   We know that the target value matches
			//   one of the end points or lies outside
			//   the list.  We deduce which is the case
			//   and exit.
			//   
			if ( First == 0 )
				{ (*After) = (*Before); }
			else
				{
				//
				//   It must either match the last value
				//   or be outside the range.
				//
				if ( Last == 0 )
					{ (*Before) = (*After); }
				else
					{
					Before = After;
					After = 0;
					}
				}

			//
			//   Anyway you see it we wil have found the
			//   closest values.
			//
			Found = True;
			}
		}

	//
	//   Release any lock we got earlier.
	//
	ReleaseSharedLock();

	return Found;
	}

    /********************************************************************/
    /*                                                                  */
    /*   Remove a key from the hash table.                              */
    /*                                                                  */
    /*   The supplied key is removed from the hash table (if it exists) */
    /*   and the associated value is deleted.                           */
    /*                                                                  */
    /********************************************************************/

template <class TYPE,class LOCK> VOID SORTED_FIND<TYPE,LOCK>::RemoveFromList
        (
		SBIT32						  Offset
        )
    {
	REGISTER SBIT32 Count;

	if ( (Offset >= 0) && (Offset < (ListUsed --)) )
		{
		//
		//   Claim an exclusive lock (if enabled).
		//
		ClaimExclusiveLock();

		//
		//   The existing values are copied further
		//   up the list.
		//
		for ( Count=Offset;Count < ListUsed;Count ++ )
			{ List[ Count ] = List[ (Count+1) ]; }

		//
		//   Release any lock we got earlier.
		//
		ReleaseExclusiveLock();
		}
    }

    /********************************************************************/
    /*                                                                  */
    /*   Class destructor.                                              */
    /*                                                                  */
    /*   Destory the sorted find.  This call is not thread safe and     */
    /*   should only be made in a single thread environment.            */
    /*                                                                  */
    /********************************************************************/

template <class TYPE,class LOCK> SORTED_FIND<TYPE,LOCK>::~SORTED_FIND( VOID )
    { /* void */ }
#endif
