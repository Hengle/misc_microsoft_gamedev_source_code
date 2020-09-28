/*
   This a generic sorting routine.  You set defines that setup how the sort
   should be performed and then you include this file.  It cleans up all of 
   the stuff that it defines (and you define for it) after it has been 
   included.

   The sort emitted at the statement level.  That is, it doesn't create a new
   function - it generates code right at the point it is included. You can 
   still use a function based sort if you want, of course, just make an
   holder function and then include sort.inl there.

   The sort is a tweaked quicksort that falls back to a heap sort if it
   detects that it is going quadratic.  You can also force it to just use
   heapsort if you want the code size to be smaller.

   It is 2 to 4 times faster than any other sort I've tested (and I tested a
   bunch) on random data.  On highly skewed data (like 99% zeros) and/or 
   already sorted data, we are much faster than qsorts and run about twice
   as fast as other heap sorts that I've tested.  We never go quadratic.


   It's the sweet action!  See the end of the defines for some of the specific
   coolness in the code itself.
   


   Defines:

   SORTINDEX - this the type name of an index to the data that you are sorting.
     This will usually just be a pointer to the type of data you are sorting:

       #define SORTINDEX int *
       
     However, sort.inl allows you to use any type of index - integer offsets
     from the beginning of an array, or whatever.  The fastest code is 
     generated when using pointers to your elements, though.  (Why you ask, 
     would you use anything else, then?  Well, you can use integer offsets to 
     sort a set of arrays, for example.  Like, say you had an array of names, 
     and a separate array of cities - you'd want to use integer offsets so that
     you could swap the elements in both arrays).

     Again, though usually, you'll just use a pointer to the type. Sort.inl
     also defines a macro to use the restrict modifier for relaxed aliasing 
     checks that speeds up sorting (usually by 10%).  So, you will usually want
     to add a SORTRESTRICT to the end of your SORTINDEX definition:

       #define SORTINDEX int * SORTRESTRICT

     If you are sorting structures, then you can also add an alignment macro
     to the end of your SORTINDEX which will let platforms like the PS2 swap
     your structure more rapidly (assuming your structures are aligned). For
     example, let's say your structure is 16 bytes long, you could use:

       #define SORTINDEX mystruct * SORTRESTRICT SORTALIGN(16)



    SORTFIRSTINDEX - this macro should be defined to return the first SORTINDEX.
      It is only called once at the beginning of the routine.  Usually this
      macro will just return a pointer to the base of the array to be sorted,
      or just 0, if you are using integer indexes.

        #define SORTFIRSTINDEX &array[0]



    SORTLENGTH *optional define* - this macro should be defined to return the
      number of elements to sort. It can be left undefined if you define
      SORTLASTINDEX instead.  You can also define both SORTLENGTH *and* 
      SORTLASTINDEX, if you happen to know both ahead of time (or they are 
      constant or whatever).  This macro is only called once at the beginning
      of the routine.

        #define SORTLENGTH count_of_elements



    SORTLASTINDEX *optional define* - this macro should be defined to return the
      the last valid SORTINDEX (so, it is "first+lenfth-1"). It can be left 
      undefined if you define SORTLENGTH instead.  You can also define both 
      SORTLENGTH *and* SORTLASTINDEX, if you happen to know both ahead of time 
      (or they are constant or whatever).  This macro is only called once at the
      beginning of the routine.

        #define SORTLASINDEX &array[ MAX_ELEMENTS - 1 ]



    SORTSWAP - this macro is called with two SORTINDEXes for two elements that
      need to be swapped.  We have a predefined macro (SORTSWAPDEFAULT) that you 
      should almost always use that will do the swap quickly for a specified type, 
      so generally this macro will be defined something like:

        #define SORTSWAP( index1, index2 ) SORTSWAPDEFAULT( index1, index2, float )

      If you are sorting independent arrays, you'd do something like:

        #define SORTSWAP( index1, index2 ) \
          SORTSWAPDEFAULT( &name_array[ index1 ], &name_array[ index2 ], char * ) \
          SORTSWAPDEFAULT( &city_array[ index1 ], &city_array[ index2 ], char * ) \

      There is another predefined macro that you can use when you don't have the type
      of the data, but just a constant length.  If you wanted to use it, use something
      like:

        #define SORTSWAP( index1, index2 ) SORTSWAPLENGTH( index1, index2, sizeof( element ) + 4 )

      Finally, if the length isn't constant (like this a generic routine where
      the length is passed in), then you can use the SORTSWAPGENERIC which loops
      over the dwords until they are all swapped.



     SORTSWAP3 - this macro is just like SORTSWAP, only it takes three elements to
       swap.  Again, we have the default macros, SORTSWAP3DEFAULT, SORTSWAP3LENGTH,
       and SORTSWAP3GENERIC that you can use to rapidly swap memory in a 3 ring.  
       Note that if you write this macro yourself completely, the order of swapping
       is: ind3=ind2; ind2=ind1; ind1=original ind3;



     SORTKEYDECLARE *optional define* - sort.inl does it's compares by comparing 
       a key value against  many other indexed elements.  So, we make a copy of 
       the key from one element and then compare than key to all the others.  We 
       don't know what your key is (it can even be multiple fields), so this 
       macro allows you to define the key variables to compare against.  

       Since we have the indexes that we are comparing to anyway, you don't *have*
       to define this macro at all - you can just compare the values in the two
       elements.  That is, you only need to declare a key if it will be faster than
       repeatedly dereferencing the elements for the comparisons.  Usually the
       smaller and the simpiler the comparisons, the more likely that declaring
       a key is worthwhile.

       So, let's say you have a structure and you have a name and a zip code that 
       you are sorting by. To set this up, you would use something like this:

         #define SORTKEYDECLARE()  char * name_key; char * zip_code;



     SORTKEYSET *optional define* - this macro assigns a given indexed element's 
       key values to the key variables that you defined with SORTKEYDECLARE.  If you
       didn't declare any key variables then you don't need to define this macro
       either. From the name/zip example above, you could use something like this:

         #define SORTKEYSET( index ) name_key = index->name_field;  zip_code = index->zip_field;

       

     SORTKEYISBEFORE, SORTKEYISAFTER, SORTKEYISEQUAL - these three macros do the 
       actual comparison.  You must define all three because different comparisions
       are done at different places throughout the sort.  These macros are passed
       to SORTINDEX values - one that is the key index, and one that is the compare
       index.  If you used the SORTKEYDECLARE and SORTKEYSET macros, then you don't
       use the key index at all - you just use your saved key variables.  So, if
       you were sorting by an integer field, you could use macros like this:

         #define SORTKEYDECLARE() int key_value;
         #define SORTKEYSET( index ) key_value = index->int_field;
         #define SORTKEYISBEFORE( key_index, index ) key_value < index->int_field;
         #define SORTKEYISAFTER( key_index, index ) key_value > index->int_field;
         #define SORTKEYISEQUAL( key_index, index ) key_value == index->int_field;

     
      
      SORTINDEXADD *optional define* - this macro returns a SORTINDEX advanced by 
        an integer amount. You only need to define this macro in unusual situations
        (like when you are sorting a linked list or sorting on-disc data).  By 
        default, it is simply defined as:
        
          #define SORTINDEXADD( index, amt ) index + amt
        
      

      SORTINDEXNEXT *optional define* - this macro returns a SORTINDEX that has been
        advanced to the next element. You only need to define this macro in unusual
        situations. By default, it is simply defined as:
        
          #define SORTINDEXNEXT( index, amt ) index + 1
        
      

      SORTINDEXPREV *optional define* - this macro returns a SORTINDEX that has been
        backed up to the previous element. You only need to define this macro in 
        unusual situations.  By default, it is simply defined as:
        
          #define SORTINDEXPREV( index, amt ) index - 1
        
      

      SORTINDEXDIFF *optional define* - this macro returns the difference between two
        indexes as a count.  By default, it is simply defined as:
        
          #define SORTINDEXDIFF( index1, index2 ) index1 - index2
        
      

      SORTINDEXSMALLER *optional define* - this macro compares one *index* to another.
        You only need to define this macro in unusual situations. By default, it is
        simply defined as:
        
          #define SORTINDEXSMALLER( index1, index2 ) index1 < index2



      SORTTINY *optional define* - this macro tells sort.inl to just use a tiny
        sorter instead of the mega-action.  This macro just generates the simple
        tail heap sort that is only used in quadratic situations otherwise.  It's
        smaller but still fairly fast.  For simple "sort a list of strings" usage,
        normally sort.inl costs about 2K in code space.  But if you define 
        SORTTINY, then we only use about 300 bytes.



      NDEBUG - you don't define this, your building make file or environment does -
        this is just the macro we assume is defined when building in release mode.
        We still work if it isn't defined, just not as quickly.



   Examples:

   Sort a list of integers:

     int numbers[ 64 ];

     // filled numbers here somehow

     #define SORTINDEX int *
     #define SORTFIRSTINDEX numbers
     #define SORTLENGTH 64
   
     #define SORTSWAP( ind1, ind2 ) SORTSWAPDEFAULT( ind1, ind2, int )
     #define SORTSWAP3( ind1, ind2, ind3 ) SORTSWAP3DEFAULT( ind1, ind2, ind3, int )

     #define SORTKEYDECLARE() int key;
     #define SORTKEYSET( key_ind ) key = *key_ind

     #define SORTKEYISBEFORE( key_ind, ind ) key < * ind
     #define SORTKEYISEQUAL( key_ind, ind ) key == * ind
     #define SORTKEYISAFTER( key_ind, ind ) key > * ind

     #include "sort.inl"


   Sort two parallel arrays:

     char * names[ 256 ];
     int zips[ 256 ];

     int my_compare( char * n1, char * n2, int z1, int z2 )
     {
       int i;

       i = strcmp( n1, n2 );
       if ( i == 0 )
         i = z1 - z2;

       return( i );
     }

     // filled arrays here somehow

     #define SORTINDEX int
     #define SORTFIRSTINDEX 0
     #define SORTLENGTH 256
   
     #define SORTSWAP( ind1, ind2 ) SORTSWAPDEFAULT( names + ind1, names + ind2, char * ); \
                                    SORTSWAPDEFAULT( zips + ind1, zips + ind2, int )
     #define SORTSWAP3( ind1, ind2, ind3 ) SORTSWAP3DEFAULT( names + ind1, names + ind2, names + ind3, char * ); \
                                           SORTSWAP3DEFAULT( zips + ind1, zips + ind2, zips + ind3, int )

     #define SORTKEYDECLARE() char * key_name; int key_zip;
     #define SORTKEYSET( key_ind ) key_name = names[ key_ind ]; key_zip = zips[ key_ind ];

     #define SORTKEYISBEFORE( key_ind, ind ) ( my_compare( key_name, names[ ind ], key_zip, zips[ ind ] ) < 0 )
     #define SORTKEYISEQUAL( key_ind, ind ) ( my_compare( key_name, names[ ind ], key_zip, zips[ ind ] ) == 0 )
     #define SORTKEYISAFTER( key_ind, ind ) ( my_compare( key_name, names[ ind ], key_zip, zips[ ind ] ) > 0 )

     #include "sort.inl"


   Generate a generic sorter that uses callbacks like CRT qsort (this is
     slower than a type specific custom sort, btw):

     typedef int compare_func( void * ind1, void * ind2 );

     void generic_qsort( void * base, int count, int element_size, compare_func * comp )
     {
       #define SORTINDEX int
       #define SORTFIRSTINDEX 0
       #define SORTLENGTH count
     
       #define PTRADD( ptr, val ) ( ( (char*) ptr ) + val )

       #define SORTSWAP( ind1, ind2 ) SORTSWAPGENERIC( PTRADD( base, ind1 ), PTRADD( base, ind2 ), element_size ); 
       #define SORTSWAP3( ind1, ind2, ind3 ) SORTSWAP3GENERIC( PTRADD( base, ind1 ), PTRADD( base, ind2 ), PTRADD( base, ind3 ), element_size ); 

       #define SORTKEYISBEFORE( key_ind, ind ) ( comp( PTRADD( base, key_ind ), PTRADD( base, ind ) ) < 0 )
       #define SORTKEYISEQUAL( key_ind, ind ) ( comp( PTRADD( base, key_ind ), PTRADD( base, ind ) ) == 0 )
       #define SORTKEYISAFTER( key_ind, ind ) ( comp( PTRADD( base, key_ind ), PTRADD( base, ind ) ) > 0 )

       #include "sort.inl"
     }
     

        
   Cool algorithm stuff: 

   When we pick our median of three for the pivot value, we sort the values we looked
   at, as long as we are there.  This gives a 8% speed improvement and we don't have 
   to handle groups of 3 specifically since it just happens for free (we do still
   special case groups of 2 and 4, though).

   Our swap helper macros do a lot of trickiness to get the compiler to give optimal 
   register use during swaps for any element size up to 16 bytes (this gives us 80%
   improvments in places).  For elements larger than that we use dwords to swap 
   which is way faster than bytes. 

   The handling of equivalent elements is handled extra carefully to balance 
   needless swaps and extra compares in later stages.  We also use a pivot "area",
   not just a single pivot value.

   We do the standard trick of only semi-recursing on the smaller of the two 
   partitions to avoid the qsort ever using more than log N of stack space.  We
   detect this, however, not by comparing sizes, but by comparing our final pivot
   index to the halfway index that we used in the median of three - this is 
   faster and I haven't seen it done before.

   We flip to heapsort when we detect that we picked a crummy pivot. Instead of
   detecting stack depth (which isn't useful when you are using the recurse on
   the smaller half trick), we check to see if the smaller of the two halves is
   less than 128th of the original length.  This is kind of cool because 128th of
   a partition that is less than 128 elements is zero, so the same check for
   seeing whether we have a bad split prevents heap sort from running on lists
   smaller than 128 elements (where even with quadratic-ish behavior, qsort is fine).

   We never flip to other algorithms on some smallness threshold (like 8 or 
   something).  I never saw this *ever* make any sort faster, once you handled
   3 lists with the median pivot finder, and 2 and 4 list specifically.  Once 
   you handle those cases, it was always a lose to special case other small sizes.

   We keep both left and right pointers during the qsort *and* length values.
   This is kind of silly since you can alway just do (right-left+1) to get the
   length, right?  Well, by keeping both, you never have to do any interior
   loop pointer arithmetic.  That's good when you have an element size that 
   isn't a power of 2.  In these cases, we are many times faster than normal
   sort routines (up to 8 times faster than CRT sort).  We do still have to
   do some pointer arithmetic in the fall through to heap sort, though.

   Our heap sort code is the very smallest and more stripped that I've ever seen.
   It uses no sub-functions (can't, since we are declared inline), but also does
   no duplication of code.  It kind of interlaces the pre-heap setup with the
   code that actually sorts the heap values with some duff-ish weirdness.  It is
   goto and logic weirdness heavy.  But it's fast and very small.
   
   One optimization that I didn't do in heap sort which would make it 20%
   faster in some cases is rather than swap as we move down the heap tree,
   you save the top value and then just copy down the tree.  Saves a little
   memory movement, but it would require being able to make a complete
   copy of one of the elements, which I didn't need to do anywhere else.
   Since heapsort is only the fallback sort (unless you are using the
   SORTTINY define), I didn't want to add any macro concepts just for it.
   If everyone ends up using SORTSMALL all the time, I'll add this logic
   and update the file.
*/


#ifndef SORTINDEX
#error You must define SORTINDEX typename
#endif

#ifndef SORTSWAP
#error You must define SORTSWAP(index1,index2)
#endif

#ifndef SORTSWAP3
#error You must define SORTSWAP3(index1,index2,index3)
#endif

#ifndef SORTFIRSTINDEX
#error You must define SORTFIRSTINDEX first_index
#endif

#if !defined(SORTLENGTH) && !(defined(SORTLASTINDEX))
#error You must define either SORTLENGTH or SORTLASTINDEX (or both)
#endif

#if defined(SORTKEYDECLARE) && !defined(SORTKEYSET)
#error If you define SORTKEYDECLARE, you should define SORTKEYSET
#endif

#ifndef SORTKEYISBEFORE
#error You must define SORTKEYISBEFORE( key_index, index )
#endif

#ifndef SORTKEYISEQUAL
#error You must define SORTKEYISEQUAL( key_index, index )
#endif

#ifndef SORTKEYISAFTER
#error You must define SORTKEYISAFTER( key_index, index )
#endif

#ifndef SORTINDEXADD
#define SORTINDEXADD( ind1, val ) ( ind1 + val )
#endif

#ifndef SORTINDEXDIFF
#define SORTINDEXDIFF( index1, index2 ) ( index1 - index2 )
#endif

#ifndef SORTINDEXNEXT
#define SORTINDEXNEXT( ind ) ( ind + 1 )
#endif

#ifndef SORTINDEXPREV
#define SORTINDEXPREV( ind ) ( ind - 1 )
#endif

#ifndef SORTINDEXSMALLER
#define SORTINDEXSMALLER( ind1, ind2 ) ( ind1 < ind2 )
#endif

#if _MSC_VER >= 1400
  #define SORTRESTRICT __restrict
  #define SORTPACKED
  #define SORTALIGN(num) __declspec( align( num ) )  
  #define SORTU64 unsigned __int64
  #pragma warning( push )
  #pragma warning( disable : 4127 )
#elif defined(__CELLOS_LV2__)
  #define SORTRESTRICT __restrict
  #define SORTPACKED 
  #define SORTALIGN(num) __attribute__((aligned(num)))
  #define SORTU64 unsigned long long
#elif defined(__psp__)
  #define SORTRESTRICT __restrict
  #define SORTPACKED
  #define SORTALIGN(num) __attribute__((aligned(num)))
  #define SORTU64 unsigned long long
#elif defined(GEKKO)
  #define SORTRESTRICT
  #define SORTPACKED
  #define SORTALIGN(num) __attribute__((aligned(num)))
  #define SORTU64 unsigned long long
#elif defined(HOLLYWOOD_REV) || defined(REVOLUTION) 
  #define SORTRESTRICT __restrict
  #define SORTPACKED
  #define SORTALIGN(num) __attribute__((aligned(num)))
  #define SORTU64 unsigned long long
#elif defined(R5900)
  #define SORTRESTRICT __restrict
  #define SORTPACKED __attribute__((__packed__))
  #define SORTALIGN(num) __attribute__((aligned(num)))
  #define SORTU64 unsigned long
#elif defined(__WATCOMC__)
  #define SORTRESTRICT 
  #define SORTPACKED
  #define SORTALIGN(num)
  #define SORTU64 double
#elif defined(__MWERKS__)
  #define SORTRESTRICT 
  #define SORTPACKED
  #define SORTALIGN(num)
  #define SORTU64 unsigned long long
#elif defined(_MACOSX)
  #define SORTRESTRICT __restrict
  #define SORTPACKED
  #define SORTALIGN(num) __attribute__((aligned(num)))
  #define SORTU64 unsigned long long
#elif defined(linux)
  #define SORTRESTRICT __restrict
  #define SORTPACKED 
  #define SORTALIGN(num) __attribute__((aligned(num)))
  #define SORTU64 unsigned long long
#else
  #define SORTRESTRICT 
  #define SORTPACKED
  #define SORTALIGN(num)
  #define SORTU64 unsigned __int64
#endif


// Swapping macros=================================

#define SORTSWAPGENERIC( addr1, addr2, size )      \
do {                                               \
  char* SORTRESTRICT a1=(char* SORTRESTRICT)addr1; \
  char* SORTRESTRICT a2=(char* SORTRESTRICT)addr2; \
  unsigned int l = size>>2;                        \
  while(l)                                         \
  {                                                \
    int __t;                                       \
    --l;                                           \
    __t=((int* SORTRESTRICT)a1)[0];                \
    ((int* SORTRESTRICT)a1)[0]=((int* SORTRESTRICT)a2)[0];  \
    ((int* SORTRESTRICT)a2)[0]=__t;                \
    a1+=4;                                         \
    a2+=4;                                         \
  }                                                \
  l = size&3;                                      \
  while(l)                                         \
  {                                                \
    char __t;                                      \
    --l;                                           \
    __t=a1[0];                                     \
    a1[0]=a2[0];                                   \
    a2[0]=__t;                                     \
    ++a1;                                          \
    ++a2;                                          \
  }                                                \
} while(0)

#define SORTSWAP3GENERIC( addr1, addr2, addr3, size ) \
do {                                               \
  char* SORTRESTRICT a1=(char* SORTRESTRICT)addr1; \
  char* SORTRESTRICT a2=(char* SORTRESTRICT)addr2; \
  char* SORTRESTRICT a3=(char* SORTRESTRICT)addr3; \
  unsigned int l = size>>2;                        \
  while(l)                                         \
  {                                                \
    int __t;                                       \
    --l;                                           \
    __t=((int* SORTRESTRICT)a3)[0];                \
    ((int* SORTRESTRICT)a3)[0]=((int* SORTRESTRICT)a2)[0];  \
    ((int* SORTRESTRICT)a2)[0]=((int* SORTRESTRICT)a1)[0];  \
    ((int* SORTRESTRICT)a1)[0]=__t;                \
    a1+=4;                                         \
    a2+=4;                                         \
    a3+=4;                                         \
  }                                                \
  l = size&3;                                      \
  while(l)                                         \
  {                                                \
    char __t;                                      \
    --l;                                           \
    __t=a1[0];                                     \
    a1[0]=a2[0];                                   \
    a2[0]=a3[0];                                   \
    a3[0]=__t;                                     \
    ++a1;                                          \
    ++a2;                                          \
    ++a3;                                          \
  }                                                \
} while(0)

#define SORTSWAPSPECIFIC(addr1,addr2, kind )                                                              \
do {                                                                                                      \
  kind __t = ((kind* SORTRESTRICT)(void*SORTRESTRICT)addr1)[0];                                           \
  ((kind* SORTRESTRICT)(void*SORTRESTRICT)addr1)[0] = ((kind* SORTRESTRICT)(void*SORTRESTRICT)addr2)[0];  \
  ((kind* SORTRESTRICT)(void*SORTRESTRICT)addr2)[0] = __t;                                                \
} while(0)

#define SORTSWAP3SPECIFIC(addr1,addr2, addr3, kind )                                                      \
do {                                                                                                      \
  kind __t = ((kind* SORTRESTRICT)(void*SORTRESTRICT)addr3)[0];                                           \
  ((kind* SORTRESTRICT)(void*SORTRESTRICT)addr3)[0] = ((kind* SORTRESTRICT)(void*SORTRESTRICT)addr2)[0];  \
  ((kind* SORTRESTRICT)(void*SORTRESTRICT)addr2)[0] = ((kind* SORTRESTRICT)(void*SORTRESTRICT)addr1)[0];  \
  ((kind* SORTRESTRICT)(void*SORTRESTRICT)addr1)[0] = __t;                                                \
} while(0)

#ifdef NDEBUG

#define SORTSWAPDEFAULT( addr1, addr2, kind )                   \
do {                                                            \
  if ( sizeof( kind ) <= 16 )                                   \
    SORTSWAPSPECIFIC( addr1, addr2, kind );                     \
  else                                                          \
    SORTSWAPGENERIC( addr1, addr2, sizeof( kind ) );            \
} while(0)

#define SORTSWAP3DEFAULT( addr1, addr2, addr3, kind )           \
do {                                                            \
  if ( sizeof( kind ) <= 16 )                                   \
    SORTSWAP3SPECIFIC( addr1, addr2, addr3, kind );             \
  else                                                          \
    SORTSWAP3GENERIC( addr1, addr2, addr3, sizeof( kind ) );    \
} while(0)

#define SORTSWAPLENGTH( addr1, addr2, len )                  \
do {                                                         \
  char forceuseconstantforlenparameter[ len ? 1 : - 1 ]={0}; \
  if ( len == 1 )                                            \
    SORTSWAPSPECIFIC( addr1, addr2, char );                  \
  else if ( len == 2 )                                       \
    SORTSWAPSPECIFIC( addr1, addr2, short );                 \
  else if ( len == 3 )                                       \
    SORTSWAPSPECIFIC( addr1, addr2, __ssl3 );                \
  else if ( len == 4 )                                       \
    SORTSWAPSPECIFIC( addr1, addr2, int );                   \
  else if ( len == 5 )                                       \
    SORTSWAPSPECIFIC( addr1, addr2, __ssl5 );                \
  else if ( len == 6 )                                       \
    SORTSWAPSPECIFIC( addr1, addr2, __ssl6 );                \
  else if ( len == 7 )                                       \
    SORTSWAPSPECIFIC( addr1, addr2, __ssl7 );                \
  else if ( len == 8 )                                       \
    SORTSWAPSPECIFIC( addr1, addr2, __ssl8 );                \
  else if ( len == 9 )                                       \
    SORTSWAPSPECIFIC( addr1, addr2, __ssl9 );                \
  else if ( len == 10 )                                      \
    SORTSWAPSPECIFIC( addr1, addr2, __ssl10 );               \
  else if ( len == 11 )                                      \
    SORTSWAPSPECIFIC( addr1, addr2, __ssl11 );               \
  else if ( len == 12 )                                      \
    SORTSWAPSPECIFIC( addr1, addr2, __ssl12 );               \
  else if ( len == 13 )                                      \
    SORTSWAPSPECIFIC( addr1, addr2, __ssl13 );               \
  else if ( len == 14 )                                      \
    SORTSWAPSPECIFIC( addr1, addr2, __ssl14 );               \
  else if ( len == 15 )                                      \
    SORTSWAPSPECIFIC( addr1, addr2, __ssl15 );               \
  else if ( len == 16 )                                      \
    SORTSWAPSPECIFIC( addr1, addr2, __ssl16 );               \
  else                                                       \
    SORTSWAPGENERIC( addr1, addr2, len );                    \
} while(0)


#define SORTSWAP3LENGTH( addr1, addr2, addr3, len )          \
do {                                                         \
  char forceuseconstantforlenparameter[ len ? 1 : - 1 ]={0}; \
  if ( len == 1 )                                            \
    SORTSWAP3SPECIFIC( addr1, addr2, addr3, char );          \
  else if ( len == 2 )                                       \
    SORTSWAP3SPECIFIC( addr1, addr2, addr3, short );         \
  else if ( len == 3 )                                       \
    SORTSWAP3SPECIFIC( addr1, addr2, addr3, __ssl3 );        \
  else if ( len == 4 )                                       \
    SORTSWAP3SPECIFIC( addr1, addr2, addr3, int );           \
  else if ( len == 5 )                                       \
    SORTSWAP3SPECIFIC( addr1, addr2, addr3, __ssl5 );        \
  else if ( len == 6 )                                       \
    SORTSWAP3SPECIFIC( addr1, addr2, addr3, __ssl6 );        \
  else if ( len == 7 )                                       \
    SORTSWAP3SPECIFIC( addr1, addr2, addr3, __ssl7 );        \
  else if ( len == 8 )                                       \
    SORTSWAP3SPECIFIC( addr1, addr2, addr3, __ssl8 );        \
  else if ( len == 9 )                                       \
    SORTSWAP3SPECIFIC( addr1, addr2, addr3, __ssl9 );        \
  else if ( len == 10 )                                      \
    SORTSWAP3SPECIFIC( addr1, addr2, addr3, __ssl10 );       \
  else if ( len == 11 )                                      \
    SORTSWAP3SPECIFIC( addr1, addr2, addr3, __ssl11 );       \
  else if ( len == 12 )                                      \
    SORTSWAP3SPECIFIC( addr1, addr2, addr3, __ssl12 );       \
  else if ( len == 13 )                                      \
    SORTSWAP3SPECIFIC( addr1, addr2, addr3, __ssl13 );       \
  else if ( len == 14 )                                      \
    SORTSWAP3SPECIFIC( addr1, addr2, addr3, __ssl14 );       \
  else if ( len == 15 )                                      \
    SORTSWAP3SPECIFIC( addr1, addr2, addr3, __ssl15 );       \
  else if ( len == 16 )                                      \
    SORTSWAP3SPECIFIC( addr1, addr2, addr3, __ssl16 );       \
  else                                                       \
    SORTSWAP3GENERIC( addr1, addr2, addr3, len );            \
} while(0)

#else

#define SORTSWAPDEFAULT( addr1, addr2, kind )                \
    SORTSWAPGENERIC( addr1, addr2, sizeof( kind ) )

#define SORTSWAPLENGTH( addr1, addr2, len )                  \
    SORTSWAPGENERIC( addr1, addr2, len )

#define SORTSWAP3DEFAULT( addr1, addr2, addr3, kind )        \
    SORTSWAP3GENERIC( addr1, addr2, addr3, sizeof( kind ) )
 
#define SORTSWAP3LENGTH( addr1, addr2, addr3, len )          \
    SORTSWAP3GENERIC( addr1, addr2, addr3, len )

#endif

// ================================================



// Sorting code starts here!

{
  #if !defined(R5900)
    #if defined(_PUSHPOP_SUPPORTED) || PRAGMA_STRUCT_PACKPUSH
      #pragma pack(push,1)
    #else
      #pragma pack(1)
    #endif
  #endif
  typedef struct SORTPACKED __ssl3 { short s; char c; } __ssl3;
  typedef struct SORTPACKED __ssl5 { int i; char c; } __ssl5;
  typedef struct SORTPACKED SORTALIGN(2) __ssl6 { int i; short s; } __ssl6;
  typedef struct SORTPACKED __ssl7 { int i; short s; char c; } __ssl7;
  typedef struct SORTPACKED SORTALIGN(8) __ssl8 { SORTU64 u; } __ssl8;
  typedef struct SORTPACKED __ssl9 { SORTU64 u; char c; } __ssl9;
  typedef struct SORTPACKED SORTALIGN(2) __ssl10 { SORTU64 u; short s; } __ssl10;
  typedef struct SORTPACKED __ssl11 { SORTU64 u; short s; char c; } __ssl11;
  typedef struct SORTPACKED SORTALIGN(4) __ssl12 { SORTU64 u; int i; } __ssl12;
  typedef struct SORTPACKED __ssl13 { SORTU64 u; int i; char c; } __ssl13;
  typedef struct SORTPACKED SORTALIGN(2) __ssl14 { SORTU64 u; int i; short s; } __ssl14;
  typedef struct SORTPACKED __ssl15 { SORTU64 u; int i; short s; char c; } __ssl15;
  typedef struct SORTPACKED SORTALIGN(16) __ssl16 { SORTU64 u1; SORTU64 u2; } __ssl16;
  #if !defined(R5900)
    #if defined(_PUSHPOP_SUPPORTED) || PRAGMA_STRUCT_PACKPUSH
      #pragma pack(pop)
    #else
      #pragma pack()
    #endif
  #endif

  SORTINDEX left;
  SORTINDEX right;
  unsigned int length;
  SORTKEYDECLARE()

#ifndef SORTTINY

  // stack for left, right, and length (we keep track of the length
  //   so that we never subtract pointers - in case of non-power-2
  //   sort sizes - we are *much* faster there)

  SORTINDEX stk[ 32 * 2 ];
  unsigned int stk_len[ 32 ];
  int stk_ptr = 0;

#endif

  #ifdef SORTLENGTH
    length = SORTLENGTH;
    if ( length <= 1 ) goto no_sort;
    left = (SORTINDEX) ( SORTFIRSTINDEX );
    #ifdef SORTLASTINDEX
      right = (SORTINDEX) ( SORTLASTINDEX );
    #else
      right = SORTINDEXADD( left, ( length - 1 ) );
    #endif
  #else
    left = (SORTINDEX) ( SORTFIRSTINDEX );
    right = (SORTINDEX) ( SORTLASTINDEX );
    if ( SORTINDEXSMALLER( right, SORTINDEXNEXT( SORTINDEXNEXT( left ) ) ) ) goto no_sort;
    #ifdef SORTLENGTH
      length = SORTLENGTH;
    #else
      length = SORTINDEXDIFF( right, left ) + 1; 
    #endif
  #endif

#ifndef SORTTINY

  for(;;)
  {
    while ( SORTINDEXSMALLER( left, right ) )
    {
      SORTINDEX piv;
      SORTINDEX lpiv;
      SORTINDEX rpiv;
      unsigned int ll, mpl; // left side, mid+left length

      if ( length == 2 )
      {
        SORTKEYSET( right );
        if ( SORTKEYISBEFORE( right, left ) )
          SORTSWAP( left, right );
        break;
      }
      else
      {
        SORTINDEX mid = SORTINDEXADD( left, ( length >> 1 ) );

        // this chooses our pivot and sorts the three values
        SORTKEYSET( mid );
        if ( SORTKEYISBEFORE( mid, left ) )
        {
          if ( SORTKEYISBEFORE( mid, right ) )
          {
            SORTKEYSET( left );
            if ( SORTKEYISBEFORE( mid, right ) )
              SORTSWAP( left, mid  ); // was TLR
            else
              SORTSWAP3( mid, left, right ); // was TRL
          }
          else
            SORTSWAP( left, right ); // was RTL
        }
        else
        {
          if ( SORTKEYISAFTER( mid, right ) )
          {
            SORTKEYSET( left );
            if ( SORTKEYISBEFORE( mid, right ) )
              SORTSWAP( mid, right ); // was LRT
            else
              SORTSWAP3( left, mid, right ); // was RLT
          }
          // else  // was LTR
        }

        if ( length == 3 )
          break;

        if ( length == 4 )
        {
          // piv used here as a temp
          piv = SORTINDEXNEXT( left );

          SORTKEYSET( piv );
          if ( SORTKEYISAFTER( piv, mid ) )
            if ( SORTKEYISBEFORE( piv, right ) )
              SORTSWAP( piv, mid );
            else
              SORTSWAP3( mid, piv, right );
          else
            if ( SORTKEYISBEFORE( piv, left ) )
              SORTSWAP( left, piv );
          break;
        }
        
        {
          SORTINDEX l; SORTINDEX r;

          ll = 1;
          piv = SORTINDEXNEXT( left );
          l = piv;
          r = right;

          SORTSWAP( mid, piv );
          SORTKEYSET( piv );

//          SORTCODEALIGN
          while ( 1 ) 
          {
            do 
            {
              r = SORTINDEXPREV( r );
              if ( !SORTINDEXSMALLER( l, r ) ) 
                goto parted;
            } while ( SORTKEYISBEFORE( piv, r ) ); 
            do 
            { 
              ++ll;
              l = SORTINDEXNEXT( l );
              if ( !SORTINDEXSMALLER( l, r ) ) 
                goto parted;
            } while ( SORTKEYISAFTER( piv, l ) );  
            SORTSWAP( l, r );
          }
         parted:;
          SORTSWAP( piv, l );
          piv = l;
        }

        // now we expand the center to include all the matching pivots
        SORTKEYSET( piv );
        mpl = ll;
        rpiv = piv;
        do
        {
          ++mpl;
          rpiv = SORTINDEXNEXT( rpiv );
        } while ( ( SORTINDEXSMALLER( rpiv, right ) ) && ( SORTKEYISEQUAL( piv, rpiv ) ) );

        lpiv = piv;
        goto skipfirstdec;
        do
        {
          --ll;
         skipfirstdec: 
          lpiv = SORTINDEXPREV( lpiv );
        } while ( ( SORTINDEXSMALLER( left, lpiv ) ) && ( SORTKEYISEQUAL( piv, lpiv ) ) );

        // is the small side on the left or the right?
        if ( SORTINDEXSMALLER( piv, mid ) )
        {
          // left partition is smaller

          // stick the larger partition on the stack (invert ptr to signify heapsort)
          if ( mpl < ( length >> 7 ) )
          {
            stk[ stk_ptr + stk_ptr ] = right;
            stk[ stk_ptr + stk_ptr + 1 ] = rpiv;
          }
          else
          {
            stk[ stk_ptr + stk_ptr ] = rpiv;
            stk[ stk_ptr + stk_ptr + 1 ] = right;
          }
          stk_len[ stk_ptr ] = length - mpl;
          ++stk_ptr;

          right = lpiv;
          length = ll;
        } 
        else
        {
          // right partition is smaller

          // stick the larger partition on the stack (invert ptr to signify heapsort)
          if ( ( length - mpl ) < ( length >> 7 ) )
          {
            stk[ stk_ptr + stk_ptr ] = lpiv;
            stk[ stk_ptr + stk_ptr + 1 ] = left;
          }
          else
          {
            stk[ stk_ptr + stk_ptr ] = left;
            stk[ stk_ptr + stk_ptr + 1 ] = lpiv;
          }
          stk_len[ stk_ptr ] = ll;
          ++stk_ptr;

          left = rpiv;
          length = length - mpl;
        }
      }
    }
      
    if ( stk_ptr == 0 )
      break;

    --stk_ptr;
    left = stk[ stk_ptr + stk_ptr ];
    right = stk[ stk_ptr + stk_ptr + 1 ];
    length = stk_len[ stk_ptr ];

    // pointers are inverted, which means to use heapsort
    if ( left > right )
#endif
    {
      SORTINDEX __i; SORTINDEX ind; SORTINDEX __v; SORTINDEX __n;
      unsigned int __s, __k;

      __s = length >> 1;
#ifndef SORTTINY
      __i = right;
      right = left;
      left = __i;
      __i = SORTINDEXADD( __i, __s );
#else
      __i = SORTINDEXADD( left, __s );
#endif

     s_loop:
      --__s;
      __i = SORTINDEXPREV( __i );
      ind = __i;
      __k = ( __s << 1 ) + 1;

     do_loop:
      __v = SORTINDEXADD( left, __k );
      __n = SORTINDEXNEXT( __v );
      SORTKEYSET( __v );
      if ( ( ( __n <= right ) ) && ( SORTKEYISBEFORE( __v, __n ) ) )
      {
        ++__k;
        __v = __n;
        SORTKEYSET( __v );
      }

      if ( SORTKEYISAFTER( __v, ind ) )
      {
        SORTSWAP( ind, __v );
        ind = __v;
        __k = ( __k << 1 ) + 1;

        if ( __k < length )
          goto do_loop;
      }

      if ( __s ) goto s_loop;

       --length;
      SORTSWAP( left, right );
      right = SORTINDEXPREV( right );
      ind = left;
      __k = 1;
      if ( length > 1 )
        goto do_loop;
    }  
#ifndef SORTTINY
  }
#endif
  no_sort:;
}


#undef SORTINDEX
#undef SORTSWAP
#undef SORTSWAP3
#undef SORTFIRSTINDEX

#ifdef SORTLENGTH
#undef SORTLENGTH
#endif

#ifdef SORTLASTINDEX
#undef SORTLASTINDEX
#endif

#ifdef SORTKEYDECLARE
#undef SORTKEYDECLARE
#endif

#ifdef SORTKEYSET
#undef SORTKEYSET
#endif

#undef SORTKEYISBEFORE
#undef SORTKEYISEQUAL
#undef SORTKEYISAFTER
#undef SORTINDEXADD
#undef SORTINDEXDIFF
#undef SORTINDEXNEXT
#undef SORTINDEXPREV
#undef SORTINDEXSMALLER
#undef SORTRESTRICT
#undef SORTSWAPGENERIC
#undef SORTSWAP3GENERIC
#undef SORTSWAPSPECIFIC
#undef SORTSWAP3SPECIFIC
#undef SORTSWAPDEFAULT
#undef SORTSWAPLENGTH
#undef SORTSWAP3DEFAULT
#undef SORTSWAP3LENGTH
#undef SORTTINY
#undef SORTPACKED
#undef SORTALIGN
#undef SORTU64

#if _MSC_VER >= 1400
  #pragma warning( pop )
#endif
