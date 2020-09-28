/* The general purpose RAD C container.

   Description:

   This container can represent most types of containers: lists, maps,
   doubly linked lists, dynamic arrays - sorted or unsorted. The way that
   you use it is that you first set some defines, and then include this
   file. It will be specialized to exactly the kind of container and data
   types that you requested with the macro defines.

   If the container is sorted, the underlying data structure is an AVL tree.
   This is a traditional binary tree that is always kept balanced - as you
   add or remove elements, we continually rebalance the tree so that you can
   never end up with the worst-case linear time of a normal binary tree.

   AVL trees are pretty cool - log2n or better at *everything*. Adding,
   removing and searching all take place in log2n with *no* linear or
   polynomial worst cases. It will sort as fast or faster than any trivial
   sort algorithm (such as QuickSort) on standard data. And, since it has no
   worst case, it can be much, much faster than QuickSort-style algorithms
   when each is presented with data that is _already_ somewhat sorted
   (QuickSort hates pre-sorted data - it works it at order N^2, baby!)

   If the container is unsorted, then we just use a doubly linked list.
   Adding and removing is instant, searching takes linear time (N).

   If you request that the list be unsorted and that a linked list should
   NOT be kept, then the container simply acts as an group memory allocator
   (only Add and Remove will be defined).

   This container uses *your* element structures!  That is, it doesn't wrap
   your element structure - it uses fields inside it!  If you are using a
   sorted list, then you need to place a "left" and "right" field inside it
   (their type can just be untyped void* pointers), for example. This is
   nice because you don't need to dereference anything to get to your data -
   when you search for something, you get back a pointer to your element
   structure - not an indirect pointer or iterator object.

   This container is written in standard C - no C++, no templates. It is
   completely specialized each time you include it. It can create header and
   source files (if you are going to use it in multiple modules), or it can
   just kick inline included files. You can customize the calling
   conventions and modifiers, so the function can be visible only inside
   your source file to do everyone. It uses no special types, so you don't
   need to include any "rad.h" or anything.

   The container can do the element memory allocation for you (growing
   dynamically as necessary), or you can do all of the memory allocation and
   the container will just manage the searching, pointer maintainance, etc.

   As I mentioned above, it is extremely fast - twice the speed when adding
   as an STL::map or ::list, four times the speed at removing. Everything is
   log2n or better (unless you are unsorted), so it will almost always be
   the best data structure to use (some AVL freaks out there don't believe
   any other data structure should *ever* be used, in fact). The only time
   I can imagine using something else now is maybe hashing when you know you
   have a good hash generator. If you don't *know* that your hash is close
   to optimal, AVL trees have be proven to beat them overall - log2n is
   really fast!

*/

/* The functions:

   Each of these functions is be prepended with the CONTAINER_NAME define,
   so if your container name is "map", then Initialize should be called as
   "mapInitialize". The item type returned by these function is control by
   CONTAINER_ITEM_TYPE define.

   To document these functions, let's assume that CONTAINER_NAME is
   "list", CONTAINER_ITEM_TYPE is "element", and element is declared like
    this:

typedef struct element
{
  char Name[ 256 ];
  int  id_number;

  // required by the container code
  void * left;
  void * right;
  void * prev;
  void * next;
} element;

   And we want our list to be sorted by Name. Since we have an int
   inside the element, you can regard this container as a map with
   Name as the key and id_number as the associated data. Obviously,
   since it's your structure, you can put whatever data you want in
   here as well (more than just the id_number), so it's more
   convenient than a STL map (where the data index would have to be an
   indirected structure). It's really nice to work with *your* own
   data structures!


void listInitialize( list * l, int HintItems );

   This function initialized the container structure. To use a container,
   first create a container structure, then have it initialized, and then
   pass it to the various container functions.

   The HintItems is the number of elements that will be allocated upon
   initialization (if we handle item allocations at all - you can have the
   list simply access your structure instances if you want).

   If we do handle allocations, then once HintItems elements have been
   consumed, then we will allocate CONTAINER_NUM_PER_ALLOCATION elements at
   a time whenever we run out from then on. Since we allocate multiple
   elements at once, it's important that your item structures are aligned.


element * listAdd( list * l, char * name, int id_number ); // if we are handling allocations
element * listAdd( list * l, element * item );             // if we aren't handling allocation

   This function adds a new element to the list. The add variables (name
   and id_number) are the list of parameters defined by
   CONTAINER_ADD_FIELDS macro. This can be multiple parameters (like for a
   map), or just a single parameter (like for a simple list). It calls the
   CONTAINER_ADD_ASSIGN macro inside this function to allow you to copy the
   name and id_number stack parameters into the new element structure.

   If we are not doing the memory allocations for you, then you pass in the
   actual address of a structure that we will use in place (don't free it
   until you call listRemove obviously).

   Adding is order Log2n when the list is sorted, instant when the list is
   unsorted.


element * listRemove( list * l, element * remove );

   This function removes an element from the list. You can get the element
   pointer from and of the movement functions (Find, Next, Prev, etc).
   Items at the beginning and ending of the list are removed a little faster
   (about 10%).

   Note that you can use the pointer returned from listRemove until the
   next call to Add or Remove. This is really nice, because you don't have
   to do any gyrations to move to the next element before you delete, etc.

   Removing is order Log2n when the list is sorted, instant when the list is
   unsorted.


element * listRemoveFirst( list * l );

   This function removes the first element from the list. You can get the
   Items at the beginning and ending of the list are removed a little faster
   (about 10%).

   Note that you can use the pointer returned from listRemoveFirst until the
   next call to Add or Remove. This is really nice, because you don't have
   to do any gyrations to move to the next element before you delete, etc.

   Removing is order Log2n when the list is sorted, instant when the list is
   unsorted.


element * listRemoveLast( list * l );

   This function removes the last element from the list. You can get the
   Items at the beginning and ending of the list are removed a little faster
   (about 10%).

   Note that you can use the pointer returned from listRemoveFirst until the
   next call to Add or Remove. This is really nice, because you don't have
   to do any gyrations to move to the next element before you delete, etc.

   Removing is order Log2n when the list is sorted, instant when the list is
   unsorted.


element * listNext( list * l, element * current );

   This function takes an element and returns the next element in order.
   When you hit the end of the list, 0 is returned. This call is instant
   (except if you don't use the CONTAINER_KEEP_LINKED_LIST define in which
   case it is Log2n).


element * listPrev( list * l, element * current );

   This function takes an element and returns the previous element in order.
   When you hit the end of the list, 0 is returned. This call is instant
   (except if you don't use the CONTAINER_KEEP_LINKED_LIST define in which
   case it is Log2n).


element * listFirst( list * l );

   This function returns the first value in the list. It is always instant.


element * listLast( list * l );

   This function returns the last value in the list. It is always instant.


element * listFind( list * l, char * name );

   This function will find the element containing the specified key. If the
   key could not be found, 0 is returned. The actual fields used for
   searching are defined by the CONTAINER_FIND_FIELDS macro.

   Finding is Log2n when the list is sorted, N when it isn't.


element * listFindGT( list * l, char * name );

   This function will find the first element in the list greater than the
   key. The key does not have to be present - if only 3 and 5 are in the
   list and you search for 4 or 3, then element 5 will be returned. If
   there is no element larger than the key, then 0 will be returned. The
   actual fields used for searching are defined by the CONTAINER_FIND_FIELDS
   macro.

   Finding is Log2n when the list is sorted, N when it isn't.


element * listFindLT( list * l, char * name );

   This function will find the first element in the list less than the key.
   The key does not have to be present - if only 3 and 5 are in the list and
   you search for 4 or 5, then element 3 will be returned. If there is no
   element less than the key, then 0 will be returned. The actual fields
   used for searching are defined by the CONTAINER_FIND_FIELDS macro.

   Finding is Log2n when the list is sorted, N when it isn't.


void listClear( list * l );

   This function empties the list completely, and puts all allocated memory
   back in the available pool (if we are handling allocations).  We don't
   free any dynamic memory here, though - to clear the list *and* free the
   memory, use listFreeMemory. After calling this function, the list is
   initialized and empty (so you can starting adding items again if
   necessary).


void listFreeMemory( list * l );

   If the container is handling allocations, this function frees any dynamic
   memory that was allocated as you added items. You usually call this
   function after you are done using the container. If we aren't handle
   memory allocations, then this function isn't necessary (or even
   generated). After calling this function, the list is initialized and
   empty (so you can starting adding items again if necessary).

*/


/* Container control defines.

   To use the container, you are expected to set the macros described next
   before including "contain.inl" to specialize the container for your
   situation. At minimum, you must set the macros marked as "(*REQUIRED*)".


(*REQUIRED*)  #define CONTAINER_NAME SortedList

   Name of the container itself (like list or sortedlist or map, etc). The
   type of the container will be a structure of type CONTAINER_NAME, so if
   you used "list", then the container structure will be "list" as well. The
   container name is also prepended to each of the container function names,
   so Create will be "listCreate", Add will be "listAdd", etc.


(*REQUIRED*)  #define CONTAINER_ITEM_TYPE Item

   Name of the structure that will be contained in the container (like item,
   or element, etc). The size of your element structure should be aligned if
   we are handling your allocations for best speed. The element structure
   should be declared before including this file. The item type should
   include the left, right, prev and next fields inside it (assuming the
   default container settings).


(*REQUIRED IF WE DO ALLOCATION*)  #define CONTAINER_ADD_FIELDS  S32 data, char * key

   This is the list of parameters that you pass to the Add function when we
   are doing the allocation. This define should be formatted as a list of
   parameters to a function (ie "int zip, char * name"). The container code
   doesn't use these variables directly, it just expects you to copy them
   into an item structure inside your CONTAINER_ADD_ASSIGN macro. If you
   don't have the container do the memory allocation, then you don't need to
   define this macro.


(*REQUIRED IF WE DO ALLOCATION*)  #define CONTAINER_ADD_ASSIGN( item )  strcpy( (item)->name, key); (item)->zip = data;

   This macro is invoked inside the Add function to let you assign the
   values passed to Add with the CONTAINER_ADD_FIELDS macro into the item
   structure that is passed to this macro.  This will usually be simple copy
   code like: "item->key = key; item->data = data;"  If you don't have the
   container to the memory allocation, then you don't need to define this
   macro.


(*REQUIRED IF SORTED*)  #define CONTAINER_COMPARE_ITEMS( item1, item2 ) stricmp( (item1)->name, (item2)->name )

   This is the macro to compare one item to another. It should return <0 if
   item 1 is less than item 2, =0 if item 1 equals item 2, >0 if item 1 is
   greater than item 2. The "zero" value for this compare is defined by the
   CONTAINER_COMPARE_EQUAL_VALUE macro. This macro only has to be defined if
   the list is sorted.


(*REQUIRED IF USING FIND*)  #define CONTAINER_FIND_FIELDS char * key

   This is the list of parameters passed to the find functions (Find,
   FindLT, FindGT).  This define should be formatted as a list of parameters
   to a function (ie "int zip, char * name"). The container code doesn't use
   these variables directly, it just expects you to compare them to an item
   structure inside your CONTAINER_COMPARE_FIND_FIELDS macro. If you don't
   use any of the find functions, then you don't need to define this macro.


(*REQUIRED IF USING FIND*)  #define CONTAINER_COMPARE_FIND_FIELDS( item ) stricmp( (item)->name, key )

   This macro is invoked inside the find functions (Find, FindLT, FindGT).
   It is expected to compare the fields specified by the
   CONTAINER_FIND_FIELDS macro to the item structure that is passed to it.
   This macro should return <0 if the fields are less than the item, =0 if
   the fields match the item, and >0 if the fields are larger than the item.
   The "zero" value for this compare is defined by the
   CONTAINER_COMPARE_EQUAL_VALUE macro. If you don't use any of the find
   functions, then you don't need to define this macro.


#define CONTAINER_COMPARE_EQUAL_VALUE 0

   This is the value to compare the result of CONTAINER_COMPARE_ITEMS to.
   This defaults to "0" (the integer, not the string), so you only have to
   change it if you compare returns some other type (like float or an enum).


#define CONTAINER_COMPARE_RESULT_TYPE int

   This is the type of the result returned from CONTAINER_COMPARE_ITEMS. It
   defaults to "int", so you only need to change it if you return a float or
   enum type.


#define CONTAINER_SORTED 1

   This define controls whether we use an AVL tree to sort the incoming
   items. If you don't sort the items, then you don't need to have "left"
   and "right" fields in your item structure. If you are sorted, then you
   need to have these two fields in there somewhere - they can simply be
   typed as "void *" pointers. If you are unsorted, adding and removing
   will be instant, but finding will be linear time (we have to search the
   whole list). By default, we sort the items.


#define CONTAINER_KEEP_LINKED_LIST 1

   This define controls whether we use the "prev" and "next" pointers in
   your element structure to maintain a linked list for fast iteration. If
   we use "prev" and "next", most of the operations are a little faster
   (around 2%), and iteration (using Next and Previous) goes from Log2n down
   to instant.  This linked list does costs an additional 8 bytes per item,
   though. By default, we do keep a linked list.


#define CONTAINER_UNUSED_ITEM_FIELD align

   This should be the name of a byte sized or larger unused field in your
   item structure. We will still work if you don't have this, but we're a
   little faster (about 4%) if you give us a temp variable to use per item.

   Note that since you usually need a dword or other dummy field to keep
   your structures aligned, you can usually use one of these alignment
   fields to avoid wasting any further space. The container will work,
   though, without this field being defined.


#define CONTAINER_FUNCTION_DECORATE( ret ) RADDEFFUNC ret RADLINK

   This is the macro used to decorate the external container functions. It
   is passed the return type of the function and it can insert whatever
   modifiers you need before or after the return type (__cdecl, __dllexport,
   static, etc). By default, this macro is defined to "static ret".


#define CONTAINER_DO_ALLOCATION 1

   If you define this value to 0, then we won't do any per-item memory
   allocations (which means no memory allocations anywhere inside the
   container code).

   When you use this option, you pass in item pointers to the Add function
   and we'll operate on those fields inside your items.  Make sure to remove
   an item from the container before releasing the memory, of course. By
   default, we do perform the item memory allocations for you.


#define CONTAINER_NUM_PER_ALLOCATION 32

   This define controls how many items are allocated each time we exhaust
   our supply when handling dynamic memory allocations for you. This is why
   it's important that you items are aligned - we allocate groups of them
   together. By default, we allocate 32 containers at a time.


#define CONTAINER_SUPPORT_DUPES 1

   This define controls whether the container will support duplicate items.
   Supporting duplicate items means a little more work needs to be done when
   adding or removing (unless the CONTAINER_NEED_EXACT_DUPE_REMOVAL flag is
   turned off).  By default, duplicates are supported.


#define CONTAINER_NEED_EXACT_DUPE_REMOVAL 1

   If you set this define to 0, then you agree to never remove a duplicated
   item (duplicated meaning identical keys) OR that you don't mind that when
   a duplicated key *is* removed, it may or may not be the exact item you
   passed to Remove (it will be the same key, but maybe not the same item).

   If you agree to this restriction, then adding and removing is about 2%
   faster (we don't compare memory locations as a key tie-breaker). Note
   that removing the first or last element of a list always works exactly,
   regardless of this setting. By default, we assume that you want
   duplicated keys to be removed exactly.


(*REQUIRED ONCE*)  #define CONTAINER_EMIT_CODE 0

   This define is used to tell this file to emit the code for the container.
   When this macro is undefined or 0, then we emit the header file for the
   container. By default, we only emit the header.

   This define is required to be set at least once in your project (to
   actually generate the code to link with).

   In my projects, I usually create a header file that contains my item
   structure, all my defines, and then includes "contain.inl". I then use this
   header file in any external source files that want to access the
   container.

   Then, I make a C file that defines the CONTAINER_EMIT_CODE macro and then
   simply includes my header file. This causes all of the same code
   generation macros to be setup *and* the code is emitted as well.


#define CONTAINER_LEFT_NAME  left
#define CONTAINER_RIGHT_NAME right
#define CONTAINER_PREV_NAME  prev
#define CONTAINER_NEXT_NAME  next

   These are the names of the left, right, prev, and next fields inside your
   item structure. These already default to "left", "right", "prev" and
   "next", so you only have to change them in unusual situations (like
   allowing an item to exist in two containers at once, for example).


#define CONTAINER_UNUSED_COUNT_NAME CONTAINER_LEFT_NAME
#define CONTAINER_UNUSED_NEXT_NAME  CONTAINER_RIGHT_NAME

   These defines are the names to use for the unused count and unused
   next variables. These variables are only used when an item is
   removed and placed into the unused list. If you never remove an
   item, or if we are handling the allocations, then you don't need to
   set these defines.  By default, we use the left and right fields if
   the list is sorted, and the prev and next fields if you are using a
   linked list.  If you aren't sorting and you aren't keeping a linked
   list (so we are just doing your allocations), then you must set
   these variables.


#define CONTAINER_MALLOC( bytes ) malloc( bytes )
#define CONTAINER_FREE( ptr )     free( ptr )

   When you are emitting code, these defines specify the dynamic memory
   functions to call.  If we aren't handling your item allocations, then you
   don't need to define these macros. By default, we use "malloc" and
   "free".


#define CONTAINER_ASSERT radassert

   When you are emitting code, this define specifies what assert function to
   call. By default, we use "assert". There are a bunch of asserts in the
   container source code, so debug mode runs a little slower.


#define CONTAINER_DEBUG_FORMAT_STRING( buf, item, skew ) sprintf( buf, "%0.0f,%i %c", (item)->key, (item)->data, skew );

   If you define this macro, we will display the tree in a background GDI
   window. This is useful for debugging. You are expected to fill the buf
   string varible with a string describing the item. The skew variable is a
   character describing the balance of the AVL tree (L=left, B=balanced,
   R=right).


#define CONTAINER_NEED_REMOVE      1
#define CONTAINER_NEED_REMOVEFIRST 1
#define CONTAINER_NEED_REMOVELAST  1
#define CONTAINER_NEED_PREVIOUS    1
#define CONTAINER_NEED_NEXT        1
#define CONTAINER_NEED_FIRST       1
#define CONTAINER_NEED_LAST        1
#define CONTAINER_NEED_FIND        1
#define CONTAINER_NEED_FINDLT      1
#define CONTAINER_NEED_FINDGT      1
#define CONTAINER_NEED_CLEAR       1

   These defines allow you to turn off generation of specific functions. If
   you use "static" for your function decoration, then you can let the
   linker drop the functions that you don't use and not worry about these
   defines. However, if you are exposing the container as an external API,
   you may want to remove the functions you don't need.

#define CONTAINER_STRUCT_ALREADY_DEFINED 0

   If you define CONTAINER_STRUCT_ALREADY_DEFINED, then contain.inl no longer generates
   a structure definition for the container itself.  Instead, it expects
   you to have defined it, and you then have the option of specifying the
   following defines for the various members thereof:

   #define CONTAINER_ROOT_MEMBER root
   #define CONTAINER_UNUSED_MEMBER unused
   #define CONTAINER_FIRST_MEMBER first
   #define CONTAINER_LAST_MEMBER last
   #define CONTAINER_REMOVE_PREV_MEMBER remove_prev
   #define CONTAINER_REMOVE_NEXT_MEMBER remove_next

*/


//==========================================================================
//  Now comes all the code (which is a little weird with all the macros
//==========================================================================

#if !CONTAINER_EMIT_CODE

#define TEST 0


#ifndef CONTAINER_SORTED
  #define CONTAINER_SORTED 1
#endif

#ifndef CONTAINER_KEEP_LINKED_LIST
  #define CONTAINER_KEEP_LINKED_LIST 1
#endif

#ifndef CONTAINER_DO_ALLOCATION
  #define CONTAINER_DO_ALLOCATION 1
#endif

#ifndef CONTAINER_SUPPORT_DUPES
  #define CONTAINER_SUPPORT_DUPES 1
#endif

#ifndef CONTAINER_NEED_EXACT_DUPE_REMOVAL
  #define CONTAINER_NEED_EXACT_DUPE_REMOVAL 1
#endif

#ifndef CONTAINER_COMPARE_EQUAL_VALUE
  #define CONTAINER_COMPARE_EQUAL_VALUE 0
#endif

#ifndef CONTAINER_COMPARE_RESULT_TYPE
  #define CONTAINER_COMPARE_RESULT_TYPE int
#endif

#ifndef CONTAINER_FUNCTION_DECORATE
  #define CONTAINER_FUNCTION_DECORATE( ret ) static ret
#endif

#ifndef CONTAINER_NUM_PER_ALLOCATION
  #define CONTAINER_NUM_PER_ALLOCATION 32
#endif

#ifndef CONTAINER_MALLOC
  #define CONTAINER_MALLOC( bytes ) malloc( bytes )
#endif

#ifndef CONTAINER_FREE
  #define CONTAINER_FREE( ptr ) free( ptr )
#endif

#ifndef CONTAINER_ASSERT
  #define CONTAINER_ASSERT assert
#endif

#ifndef CONTAINER_LEFT_NAME
  #define CONTAINER_LEFT_NAME left
#endif

#ifndef CONTAINER_RIGHT_NAME
  #define CONTAINER_RIGHT_NAME right
#endif

#ifndef CONTAINER_PREV_NAME
  #define CONTAINER_PREV_NAME prev
#endif

#ifndef CONTAINER_NEXT_NAME
  #define CONTAINER_NEXT_NAME next
#endif

#ifndef CONTAINER_UNUSED_COUNT_NAME
  #if CONTAINER_SORTED
    #define CONTAINER_UNUSED_COUNT_NAME CONTAINER_LEFT_NAME
  #else
    #define CONTAINER_UNUSED_COUNT_NAME CONTAINER_PREV_NAME
  #endif
#endif

#ifndef CONTAINER_UNUSED_NEXT_NAME
  #if CONTAINER_SORTED
    #define CONTAINER_UNUSED_NEXT_NAME  CONTAINER_RIGHT_NAME
  #else
    #define CONTAINER_UNUSED_NEXT_NAME  CONTAINER_NEXT_NAME
  #endif
#endif

#ifndef CONTAINER_NEED_REMOVE
#define CONTAINER_NEED_REMOVE      1
#endif

#ifndef CONTAINER_NEED_REMOVEFIRST
#define CONTAINER_NEED_REMOVEFIRST 1
#endif

#ifndef CONTAINER_NEED_REMOVELAST
#define CONTAINER_NEED_REMOVELAST  1
#endif

#ifndef CONTAINER_NEED_PREVIOUS
#define CONTAINER_NEED_PREVIOUS    1
#endif

#ifndef CONTAINER_NEED_NEXT
#define CONTAINER_NEED_NEXT        1
#endif

#ifndef CONTAINER_NEED_FIRST
#define CONTAINER_NEED_FIRST       1
#endif

#ifndef CONTAINER_NEED_LAST
#define CONTAINER_NEED_LAST        1
#endif

#ifndef CONTAINER_NEED_FIND
#define CONTAINER_NEED_FIND        1
#endif

#ifndef CONTAINER_NEED_FINDLT
#define CONTAINER_NEED_FINDLT      1
#endif

#ifndef CONTAINER_NEED_FINDGT
#define CONTAINER_NEED_FINDGT      1
#endif

#ifndef CONTAINER_NEED_CLEAR
#define CONTAINER_NEED_CLEAR       1
#endif

#ifndef CONTAINER_USE_OVERLOADING
#define CONTAINER_USE_OVERLOADING  0
#endif

#ifndef CONTAINER_NEED_UNUSED
#define CONTAINER_NEED_UNUSED (CONTAINER_NEED_REMOVE || CONTAINER_NEED_REMOVEFIRST || CONTAINER_NEED_REMOVELAST || CONTAINER_DO_ALLOCATION)
#endif
    
#if !CONTAINER_SUPPORT_DUPES
  #undef CONTAINER_NEED_EXACT_DUPE_REMOVAL
  #define CONTAINER_NEED_EXACT_DUPE_REMOVAL 0
#endif

#if !defined(CONTAINER_STRUCT_ALREADY_DEFINED)
#define CONTAINER_STRUCT_ALREADY_DEFINED 0
#endif

#if !defined(CONTAINER_UNUSED_MEMBER)
#define CONTAINER_UNUSED_MEMBER unused
#endif

#if !defined(CONTAINER_FIRST_MEMBER)
#define CONTAINER_FIRST_MEMBER first
#endif

#if !defined(CONTAINER_LAST_MEMBER)
#define CONTAINER_LAST_MEMBER last
#endif

#if !defined(CONTAINER_ROOT_MEMBER)
#define CONTAINER_ROOT_MEMBER root
#endif

#if !defined(CONTAINER_PREV_MEMBER)
#define CONTAINER_REMOVE_PREV_MEMBER remove_prev
#endif

#if !defined(CONTAINER_NEXT_MEMBER)
#define CONTAINER_REMOVE_NEXT_MEMBER remove_next
#endif

#if !CONTAINER_STRUCT_ALREADY_DEFINED
typedef struct CONTAINER_NAME
{
  CONTAINER_ITEM_TYPE * CONTAINER_UNUSED_MEMBER;

  #if CONTAINER_SORTED || CONTAINER_KEEP_LINKED_LIST
    CONTAINER_ITEM_TYPE * CONTAINER_FIRST_MEMBER;
    CONTAINER_ITEM_TYPE * CONTAINER_LAST_MEMBER;
  #endif

  #if CONTAINER_SORTED
    CONTAINER_ITEM_TYPE * CONTAINER_ROOT_MEMBER;
  #else
    #if CONTAINER_DO_ALLOCATION
      #if CONTAINER_KEEP_LINKED_LIST
        CONTAINER_ITEM_TYPE * CONTAINER_REMOVE_PREV_MEMBER;
        CONTAINER_ITEM_TYPE * CONTAINER_REMOVE_NEXT_MEMBER;
      #endif
    #endif
  #endif

  #if CONTAINER_DO_ALLOCATION
    void * CONTAINER_BUFS_MEMBER;
  #endif

} CONTAINER_NAME;
#endif


#else

  // include the header file
  #undef CONTAINER_EMIT_CODE
  #define CONTAINER_EMIT_CODE 0
  #undef CONTAINER_NO_UNDEFINE
  #define CONTAINER_NO_UNDEFINE 1
  #include "contain.inl"
  #undef CONTAINER_EMIT_CODE
  #define CONTAINER_EMIT_CODE 1
  #undef CONTAINER_NO_UNDEFINE

#endif

// goofy preprocessor gyrations to do concatinations on a defined name
#define cat_a(a,b) a ## b
#define cat_b(a,b) cat_a(a,b)
#define MakeNamePre( pre ) cat_b(pre,CONTAINER_NAME)
#define MakeNamePostAlways( post ) cat_b(CONTAINER_NAME,post)
#if (__cplusplus && CONTAINER_USE_OVERLOADING)
#define MakeNamePost( post ) post
#else
#define MakeNamePost( post ) MakeNamePostAlways(post)
#endif


#if CONTAINER_EMIT_CODE

// this should be an int the same size as a pointer
#if _MSC_VER >= 1300
  #if _WIN64
    #define CONTAINER_INTPTR __w64 __int64
  #else
    #define CONTAINER_INTPTR __w64 long
  #endif
#else
  #define CONTAINER_INTPTR int
#endif

// Check the size.  Should give neg subscript error if we get that wrong...
typedef char CONTAINER_INTPTR_SizeCheck[sizeof(CONTAINER_INTPTR) == sizeof(void*) ? 1 : -1];

#ifndef CONTAINER_UNUSED_ITEM_FIELD

  #if CONTAINER_FAVOR_RIGHT

    #define SET_PTR( overloaded, value ) (overloaded).bits = ( ( (overloaded).bits & 3 ) | ( (CONTAINER_INTPTR) (value) ) )
    #define GET_PTR( overloaded )        ( ( CONTAINER_ITEM_TYPE * ) ( (overloaded).bits & ~3 ) )

    #define SET_LEFT_SKEW( item )        ( (MakeNamePostAlways( OVERLOADED_PTR )*) &(item)->CONTAINER_LEFT_NAME )->bits = ( ( (MakeNamePostAlways( OVERLOADED_PTR )*) &(item)->CONTAINER_LEFT_NAME )->bits & ~3 )
    #define SET_NO_SKEW( item )          ( (MakeNamePostAlways( OVERLOADED_PTR )*) &(item)->CONTAINER_LEFT_NAME )->bits = ( ( ( (MakeNamePostAlways( OVERLOADED_PTR )*) &(item)->CONTAINER_LEFT_NAME )->bits & ~3 ) | 1 )
    #define SET_RIGHT_SKEW( item )       ( (MakeNamePostAlways( OVERLOADED_PTR )*) &(item)->CONTAINER_LEFT_NAME )->bits = ( ( ( (MakeNamePostAlways( OVERLOADED_PTR )*) &(item)->CONTAINER_LEFT_NAME )->bits & ~3 ) | 2 )
    #define SET_SKEW( item, value )      ( (MakeNamePostAlways( OVERLOADED_PTR )*) &(item)->CONTAINER_LEFT_NAME )->bits = ( ( ( (MakeNamePostAlways( OVERLOADED_PTR )*) &(item)->CONTAINER_LEFT_NAME )->bits & ~3 ) | ( value ) )
    #define GET_SKEW( item )             ( ( (MakeNamePostAlways( OVERLOADED_PTR )*) &(item)->CONTAINER_LEFT_NAME )->bits & 3 )

    #define IS_LEFT_SKEWED( item )       ( ( ( (MakeNamePostAlways( OVERLOADED_PTR )*) &(item)->CONTAINER_LEFT_NAME )->bits & 3 ) < 1 )
    #define IS_RIGHT_SKEWED( item )      ( ( ( (MakeNamePostAlways( OVERLOADED_PTR )*) &(item)->CONTAINER_LEFT_NAME )->bits & 3 ) > 1 )
    #define IS_NOT_SKEWED( item )        ( ( ( (MakeNamePostAlways( OVERLOADED_PTR )*) &(item)->CONTAINER_LEFT_NAME )->bits & 3 ) == 1 )

    #define SET_LEFT( item, value )      SET_PTR( * ( (MakeNamePostAlways( OVERLOADED_PTR )*) &(item)->CONTAINER_LEFT_NAME ), value )
    #define GET_LEFT( item )             GET_PTR( * ( (MakeNamePostAlways( OVERLOADED_PTR )*) &(item)->CONTAINER_LEFT_NAME ) )

    #define SET_RIGHT( item, value )     ( (MakeNamePostAlways( OVERLOADED_PTR )*) &(item)->CONTAINER_RIGHT_NAME )->ptr = (value)
    #define GET_RIGHT( item )            ( (MakeNamePostAlways( OVERLOADED_PTR )*) &(item)->CONTAINER_RIGHT_NAME )->ptr

  #else

    #define SET_PTR( overloaded, value ) (overloaded).bits = ( ( (overloaded).bits & 3 ) | ( (CONTAINER_INTPTR) (value) ) )
    #define GET_PTR( overloaded )        ( ( CONTAINER_ITEM_TYPE * ) ( (overloaded).bits & ~3 ) )

    #define SET_LEFT_SKEW( item )        ( (MakeNamePostAlways( OVERLOADED_PTR ) *) &(item)->CONTAINER_RIGHT_NAME )->bits = ( ( (MakeNamePostAlways( OVERLOADED_PTR )*) &(item)->CONTAINER_RIGHT_NAME )->bits & ~3 )
    #define SET_NO_SKEW( item )          ( (MakeNamePostAlways( OVERLOADED_PTR ) *) &(item)->CONTAINER_RIGHT_NAME )->bits = ( ( ( (MakeNamePostAlways( OVERLOADED_PTR )*) &(item)->CONTAINER_RIGHT_NAME )->bits & ~3 ) | 1 )
    #define SET_RIGHT_SKEW( item )       ( (MakeNamePostAlways( OVERLOADED_PTR ) *) &(item)->CONTAINER_RIGHT_NAME )->bits = ( ( ( (MakeNamePostAlways( OVERLOADED_PTR )*) &(item)->CONTAINER_RIGHT_NAME )->bits & ~3 ) | 2 )
    #define SET_SKEW( item, value )             ( (MakeNamePostAlways( OVERLOADED_PTR ) *) &(item)->CONTAINER_RIGHT_NAME )->bits = ( ( ( (MakeNamePostAlways( OVERLOADED_PTR )*) &(item)->CONTAINER_RIGHT_NAME )->bits & ~3 ) | ( value ) )
    #define GET_SKEW( item )             ( ( (MakeNamePostAlways( OVERLOADED_PTR ) *) &(item)->CONTAINER_RIGHT_NAME )->bits & 3 )

    #define IS_LEFT_SKEWED( item )       ( ( ( (MakeNamePostAlways( OVERLOADED_PTR ) *) &(item)->CONTAINER_RIGHT_NAME )->bits & 3 ) < 1 )
    #define IS_RIGHT_SKEWED( item )      ( ( ( (MakeNamePostAlways( OVERLOADED_PTR ) *) &(item)->CONTAINER_RIGHT_NAME )->bits & 3 ) > 1 )
    #define IS_NOT_SKEWED( item )        ( ( ( (MakeNamePostAlways( OVERLOADED_PTR ) *) &(item)->CONTAINER_RIGHT_NAME )->bits & 3 ) == 1 )

    #define SET_RIGHT( item, value )     SET_PTR( * ( (MakeNamePostAlways( OVERLOADED_PTR ) *) &(item)->CONTAINER_RIGHT_NAME ), value )
    #define GET_RIGHT( item )            GET_PTR( * ( (MakeNamePostAlways( OVERLOADED_PTR ) *) &(item)->CONTAINER_RIGHT_NAME ) )

    #define SET_LEFT( item, value )      ( (MakeNamePostAlways( OVERLOADED_PTR ) *) &(item)->CONTAINER_LEFT_NAME )->ptr = (value)
    #define GET_LEFT( item )             ( (MakeNamePostAlways( OVERLOADED_PTR ) *) &(item)->CONTAINER_LEFT_NAME )->ptr

  #endif

#else

  #define SET_PTR( overloaded, value )   (overloaded).ptr = (value)
  #define GET_PTR( overloaded )          (overloaded).ptr

  #define SET_LEFT_SKEW( item )          (item)->CONTAINER_UNUSED_ITEM_FIELD = 0
  #define SET_NO_SKEW( item )            (item)->CONTAINER_UNUSED_ITEM_FIELD = 1
  #define SET_RIGHT_SKEW( item )         (item)->CONTAINER_UNUSED_ITEM_FIELD = 2
  #define SET_SKEW( item, value )        (item)->CONTAINER_UNUSED_ITEM_FIELD = ( value )
  #define GET_SKEW( item )               (item)->CONTAINER_UNUSED_ITEM_FIELD

  #define IS_LEFT_SKEWED( item )         ( (item)->CONTAINER_UNUSED_ITEM_FIELD < 1 )
  #define IS_RIGHT_SKEWED( item )        ( (item)->CONTAINER_UNUSED_ITEM_FIELD > 1 )
  #define IS_NOT_SKEWED( item )          ( (item)->CONTAINER_UNUSED_ITEM_FIELD == 1 )

  #define SET_LEFT( item, value )        ( (MakeNamePostAlways( OVERLOADED_PTR ) *) &(item)->CONTAINER_LEFT_NAME )->ptr = (value)
  #define GET_LEFT( item )               ( (MakeNamePostAlways( OVERLOADED_PTR ) *) &(item)->CONTAINER_LEFT_NAME )->ptr

  #define SET_RIGHT( item, value )       ( (MakeNamePostAlways( OVERLOADED_PTR ) *) &(item)->CONTAINER_RIGHT_NAME )->ptr = (value)
  #define GET_RIGHT( item )              ( (MakeNamePostAlways( OVERLOADED_PTR ) *) &(item)->CONTAINER_RIGHT_NAME )->ptr

#endif

#if CONTAINER_DO_ALLOCATION
  #define SET_UNUSED_COUNT( item, value )  ( (MakeNamePostAlways( OVERLOADED_PTR ) *) &(item)->CONTAINER_UNUSED_COUNT_NAME )->bits = (value)
  #define GET_UNUSED_COUNT( item )         ( (MakeNamePostAlways( OVERLOADED_PTR ) *) &(item)->CONTAINER_UNUSED_COUNT_NAME )->bits
  #define SET_UNUSED_NEXT( item, value )   ( (MakeNamePostAlways( OVERLOADED_PTR ) *) &(item)->CONTAINER_UNUSED_NEXT_NAME )->ptr = (value)
  #define GET_UNUSED_NEXT( item )          ( (MakeNamePostAlways( OVERLOADED_PTR ) *) &(item)->CONTAINER_UNUSED_NEXT_NAME )->ptr
#endif

#define SET_NEXT( item, value )          ( (MakeNamePostAlways( OVERLOADED_PTR ) *) &(item)->CONTAINER_NEXT_NAME )->ptr = (value)
#define GET_NEXT( item )                 ( (MakeNamePostAlways( OVERLOADED_PTR ) *) &(item)->CONTAINER_NEXT_NAME )->ptr
#define SET_PREV( item, value )          ( (MakeNamePostAlways( OVERLOADED_PTR ) *) &(item)->CONTAINER_PREV_NAME )->ptr = (value)
#define GET_PREV( item )                 ( (MakeNamePostAlways( OVERLOADED_PTR ) *) &(item)->CONTAINER_PREV_NAME )->ptr

typedef union MakeNamePostAlways( OVERLOADED_PTR )
{
  struct CONTAINER_ITEM_TYPE * ptr;
  CONTAINER_INTPTR   bits;
} MakeNamePostAlways( OVERLOADED_PTR );


#if CONTAINER_DO_ALLOCATION
  typedef struct MakeNamePostAlways( ITEM_BUFFER )
  {
    struct MakeNamePostAlways( ITEM_BUFFER ) * next;
    CONTAINER_INTPTR num_items;
    CONTAINER_INTPTR align[ 2 ];

    CONTAINER_ITEM_TYPE items[ 1 ];
  } MakeNamePostAlways( ITEM_BUFFER );
#endif


#if defined( CONTAINER_DEBUG_FORMAT_STRING )
  #include "treedisp.inl"
#endif


// add a new buffer for the items

#if CONTAINER_DO_ALLOCATION

static void MakeNamePost( add_new_buffer ) ( CONTAINER_NAME * sl, int num_items )
{
  MakeNamePostAlways( ITEM_BUFFER ) * b;

  if ( num_items < CONTAINER_NUM_PER_ALLOCATION )
  {
    num_items = CONTAINER_NUM_PER_ALLOCATION;
  }

  b = (MakeNamePostAlways( ITEM_BUFFER )*) CONTAINER_MALLOC( sizeof( MakeNamePostAlways( ITEM_BUFFER ) ) + ( num_items - 1 ) * sizeof( CONTAINER_ITEM_TYPE ) );
  if (b)
  {
    SET_UNUSED_NEXT( b->items, 0 );
    SET_UNUSED_COUNT( b->items, num_items );

    b->num_items = num_items;
    b->next = (MakeNamePostAlways( ITEM_BUFFER ) *) sl->CONTAINER_BUFS_MEMBER;

    sl->CONTAINER_BUFS_MEMBER = b;
    sl->CONTAINER_UNUSED_MEMBER = b->items;
  }
}


// free the items buffer and return the next one

static MakeNamePostAlways( ITEM_BUFFER ) * MakeNamePost( free_buffer ) ( MakeNamePostAlways( ITEM_BUFFER ) * b )
{
  MakeNamePostAlways( ITEM_BUFFER ) * next;

  next = b->next;
  CONTAINER_FREE( b );

  return( next );
}


// grab an unused item pointer out of the unused list

static CONTAINER_ITEM_TYPE * MakeNamePost( get_unused_item ) ( CONTAINER_NAME * sl )
{
  CONTAINER_ITEM_TYPE * new_item;

  if ( sl->CONTAINER_UNUSED_MEMBER == 0 )
  {
    MakeNamePost( add_new_buffer ) ( sl, CONTAINER_NUM_PER_ALLOCATION );
    if (sl->CONTAINER_UNUSED_MEMBER == 0)
      return 0;
  }

  new_item = sl->CONTAINER_UNUSED_MEMBER;

  if ( GET_UNUSED_COUNT( new_item ) > 1 )
  {
    sl->CONTAINER_UNUSED_MEMBER = new_item + 1;
    SET_UNUSED_COUNT( sl->CONTAINER_UNUSED_MEMBER, GET_UNUSED_COUNT( new_item ) - 1 );
    SET_UNUSED_NEXT( sl->CONTAINER_UNUSED_MEMBER, GET_UNUSED_NEXT( new_item ) );
  }
  else
  {
    sl->CONTAINER_UNUSED_MEMBER = GET_UNUSED_NEXT( sl->CONTAINER_UNUSED_MEMBER );
  }

  return( new_item );
}

#endif


#if CONTAINER_NEED_UNUSED
// add node to the unused linked list

static void MakeNamePost( add_to_unused ) ( CONTAINER_NAME * sl, CONTAINER_ITEM_TYPE * remove )
{
  #if CONTAINER_DO_ALLOCATION
    SET_UNUSED_COUNT( remove, 1 );
    SET_UNUSED_NEXT( remove, sl->CONTAINER_UNUSED_MEMBER );

    #if !CONTAINER_SORTED && CONTAINER_KEEP_LINKED_LIST
      sl->CONTAINER_REMOVE_PREV_MEMBER = GET_PREV( remove );
      sl->CONTAINER_REMOVE_NEXT_MEMBER = GET_NEXT( remove );
    #endif

  #endif

  sl->CONTAINER_UNUSED_MEMBER = remove;
}
#endif

#if CONTAINER_SORTED

// rotate node around left

static void MakeNamePost( rotate_left ) ( CONTAINER_ITEM_TYPE * item, MakeNamePostAlways( OVERLOADED_PTR ) * node )
{
  CONTAINER_ITEM_TYPE * right;

  right = GET_RIGHT( item );

  SET_PTR( *node, right );
  SET_RIGHT( item, GET_LEFT( right ) );
  SET_LEFT( right, item );
}


// rotate node around right

static void MakeNamePost( rotate_right ) ( CONTAINER_ITEM_TYPE * item, MakeNamePostAlways( OVERLOADED_PTR ) * node )
{
  CONTAINER_ITEM_TYPE * left;

  left = GET_LEFT( item );

  SET_PTR( *node, left );
  SET_LEFT( item, GET_RIGHT( left ) );
  SET_RIGHT( left, item );
}


#if CONTAINER_NEED_REMOVE || CONTAINER_NEED_REMOVEFIRST || CONTAINER_NEED_REMOVELAST

// left side has shrunk so do the rebalance - returns 1 if need to rebalance higher up, 0 if done

static int MakeNamePost( left_has_shrunk ) ( MakeNamePostAlways( OVERLOADED_PTR ) * node )
{
  CONTAINER_ITEM_TYPE * item, * right;

  item = GET_PTR( * node );
  right = GET_RIGHT( item );

  if ( IS_LEFT_SKEWED( item ) )
  {
    SET_NO_SKEW( item );
    return( 1 );
  }
  else if ( IS_RIGHT_SKEWED( item ) )
  {
    if ( IS_RIGHT_SKEWED( right ) )
    {
      SET_NO_SKEW( item );
      SET_NO_SKEW( right );
      MakeNamePost( rotate_left ) ( item, node );
      return( 1 );
    }
    else if ( IS_NOT_SKEWED( right ) )
    {
      SET_RIGHT_SKEW( item );
      SET_LEFT_SKEW( right );
      MakeNamePost( rotate_left ) ( item, node );
      return( 0 );
    }
    else
    {
      CONTAINER_ITEM_TYPE * rights_left;

      rights_left = GET_LEFT( right );

      if ( IS_LEFT_SKEWED( rights_left ) )
      {
        SET_NO_SKEW( item );
        SET_RIGHT_SKEW( right );
      }
      else if ( IS_RIGHT_SKEWED( rights_left ) )
      {
        SET_LEFT_SKEW( item );
        SET_NO_SKEW( right );
      }
      else
      {
        SET_NO_SKEW( item );
        SET_NO_SKEW( right );
      }

      SET_NO_SKEW( rights_left );
      MakeNamePost( rotate_right ) ( right, (MakeNamePostAlways( OVERLOADED_PTR )*) &item->CONTAINER_RIGHT_NAME );
      MakeNamePost( rotate_left ) ( item, node );
      return( 1 );
    }
  }
  else
  {
    SET_RIGHT_SKEW( item );
    return( 0 );
  }
}


// right side has shrunk so do the rebalance - returns 1 if need to rebalance higher up, 0 if done

static int MakeNamePost( right_has_shrunk ) ( MakeNamePostAlways( OVERLOADED_PTR ) * node )
{
  CONTAINER_ITEM_TYPE * item, * left;

  item = GET_PTR( * node );
  left = GET_LEFT( item );

  if ( IS_RIGHT_SKEWED( item ) )
  {
    SET_NO_SKEW( item );
    return( 1 );
  }
  else if ( IS_LEFT_SKEWED( item ) )
  {
    if ( IS_LEFT_SKEWED( left ) )
    {
      SET_NO_SKEW( item );
      SET_NO_SKEW( left );
      MakeNamePost( rotate_right ) ( item, node );
      return( 1 );
    }
    else if ( IS_NOT_SKEWED( left ) )
    {
      SET_LEFT_SKEW( item );
      SET_RIGHT_SKEW( left );
      MakeNamePost( rotate_right ) ( item, node );
      return( 0 );
    }
    else
    {
      CONTAINER_ITEM_TYPE * lefts_right;

      lefts_right = GET_RIGHT( left );

      if ( IS_LEFT_SKEWED( lefts_right ) )
      {
        SET_RIGHT_SKEW( item );
        SET_NO_SKEW( left );
      }
      else if ( IS_RIGHT_SKEWED( lefts_right ) )
      {
        SET_NO_SKEW( item );
        SET_LEFT_SKEW( left );
      }
      else
      {
        SET_NO_SKEW( item );
        SET_NO_SKEW( left );
      }

      SET_NO_SKEW( lefts_right );
      MakeNamePost( rotate_left ) ( left, (MakeNamePostAlways( OVERLOADED_PTR ) *) &item->CONTAINER_LEFT_NAME );
      MakeNamePost( rotate_right ) ( item, node );
      return( 1 );
    }
  }
  else
  {
    SET_LEFT_SKEW( item );
    return( 0 );
  }
}

#endif


#if CONTAINER_NEED_REMOVE || CONTAINER_NEED_REMOVEFIRST

// finds the first item, and then removes it (and stores it in unused)

static void MakeNamePost( get_and_remove_top ) ( CONTAINER_NAME * sl )
{
  CONTAINER_ITEM_TYPE * i, * right;
  MakeNamePostAlways( OVERLOADED_PTR ) * stack[ 33 ]; // would store 8 billion items - probably safe
  int stack_ptr;
  MakeNamePostAlways( OVERLOADED_PTR ) * node;

  node = (MakeNamePostAlways( OVERLOADED_PTR ) *) &sl->CONTAINER_ROOT_MEMBER;
  i = sl->CONTAINER_ROOT_MEMBER;

  CONTAINER_ASSERT( i );

  // put the first right branch at the top of the stack (in case we're deleting the root)
  stack[ 0 ] = (MakeNamePostAlways( OVERLOADED_PTR ) *) &i->CONTAINER_RIGHT_NAME;
  stack_ptr = 1;

  // find the leftmost node, saving our path along the way
  while ( 1 )
  {
    CONTAINER_ITEM_TYPE * left;

    left = GET_LEFT( i );
    if ( left == 0 )
    {
      break;
    }

    stack[ stack_ptr++ ] = node;

    node = (MakeNamePostAlways( OVERLOADED_PTR ) *) &i->CONTAINER_LEFT_NAME;

    i = left;
  }

  right = GET_RIGHT( i );

  #if CONTAINER_KEEP_LINKED_LIST
    #if TEST
    {
      if ( right )
      {
        radassert( GET_NEXT( i ) == right );
      }
      else
      {
        radassert( GET_NEXT( i ) == GET_PTR( *stack[ stack_ptr - 1 ] ) );
      }
    }
    #endif

    {
      CONTAINER_ITEM_TYPE * next;

      // update next pointer
      next = GET_NEXT( i );
      if ( next )
      {
        SET_PREV( next, 0 );
      }
      sl->CONTAINER_FIRST_MEMBER = next;
    }

  #else
  {
    if ( right )
    {
      sl->CONTAINER_FIRST_MEMBER = right;
    }
    else
    {
      sl->CONTAINER_FIRST_MEMBER = GET_PTR( *stack[ stack_ptr - 1 ] );
      if ( sl->CONTAINER_FIRST_MEMBER == 0 )
      {
        sl->CONTAINER_LAST_MEMBER = 0;
      }
    }
  }
  #endif

  // in a balanced tree this should never happen
  CONTAINER_ASSERT( ( right == 0 ) || ( GET_LEFT( right ) == 0 ) );

  // remove the node
  SET_PTR( *node, right );

  // add it to the unused list
  MakeNamePost( add_to_unused ) ( sl, i );


  // now walk the path we took rotating if necessary along the way
  while ( stack_ptr > 1 )
  {
    --stack_ptr;
    if ( !MakeNamePost( left_has_shrunk ) ( stack[ stack_ptr ] ) )
      break;  // break out early if we're restored to balance
  }
}

#endif

#if CONTAINER_NEED_REMOVE || CONTAINER_NEED_REMOVELAST

// finds the last item, and then removes it (and stores it in unused)

static void MakeNamePost( get_and_remove_bottom ) ( CONTAINER_NAME * sl )
{
  CONTAINER_ITEM_TYPE * i, * left;
  MakeNamePostAlways( OVERLOADED_PTR ) * stack[ 32 ]; // would store 4 billion items - probably safe
  int stack_ptr;
  MakeNamePostAlways( OVERLOADED_PTR ) * node;

  node = (MakeNamePostAlways( OVERLOADED_PTR ) *) &sl->CONTAINER_ROOT_MEMBER;
  i = sl->CONTAINER_ROOT_MEMBER;

  CONTAINER_ASSERT( i );

  // put the first left branch at the top of the stack (in case we're deleting the root)
  stack[ 0 ] = (MakeNamePostAlways( OVERLOADED_PTR ) *) &i->CONTAINER_LEFT_NAME;
  stack_ptr = 1;

  // find the rightmost node, saving our path along the way
  for(;;)
  {
    CONTAINER_ITEM_TYPE * right;

    right = GET_RIGHT( i );
    if ( right == 0 )
    {
      break;
    }

    stack[ stack_ptr++ ] = node;

    node = (MakeNamePostAlways( OVERLOADED_PTR ) *) &i->CONTAINER_RIGHT_NAME;

    i = right;
  }

  left = GET_LEFT( i );

  #if CONTAINER_KEEP_LINKED_LIST
    #if TEST
    {
      if ( left )
      {
        radassert( GET_PREV( i ) == left );
      }
      else
      {
        radassert( GET_PREV( i ) == GET_PTR( *stack[ stack_ptr - 1 ] ) );
      }
    }
    #endif

    {
      CONTAINER_ITEM_TYPE * prev;

      // update prev pointer
      prev = GET_PREV( i );
      if ( prev )
      {
        SET_NEXT( prev, 0 );
      }
      sl->CONTAINER_LAST_MEMBER = prev;
    }
  #else
  {
    if ( left )
    {
      sl->CONTAINER_LAST_MEMBER = left;
    }
    else
    {
      sl->CONTAINER_LAST_MEMBER = GET_PTR( *stack[ stack_ptr - 1 ] );
      if ( sl->CONTAINER_LAST_MEMBER == 0 )
      {
        sl->CONTAINER_FIRST_MEMBER = 0;
      }
    }
  }
  #endif

  // in a balanced tree this should never happen
  CONTAINER_ASSERT( ( left == 0 ) || ( GET_RIGHT( left ) == 0 ) );

  // remove the node
  SET_PTR( *node, left );

  // add it to the unused list
  MakeNamePost( add_to_unused ) ( sl, i );


  // now walk back up the path we took rotating if necessary along the way to rebalance
  while ( stack_ptr > 1 )
  {
    --stack_ptr;
    if ( !MakeNamePost( right_has_shrunk ) ( stack[ stack_ptr ] ) )
      break;  // break out early if we're restored to balance
  }
}

#endif

#if CONTAINER_NEED_REMOVE

// finds and element, and then removes it (and stores it in unused)

static void MakeNamePost( get_and_remove ) ( CONTAINER_NAME * sl, CONTAINER_ITEM_TYPE * remove_item )
{
  CONTAINER_ITEM_TYPE * i, * left, * right;
  MakeNamePostAlways( OVERLOADED_PTR ) * stack[ 32 ]; // would store 4 billion items - probably safe
  char side_stack[ 32 ];
  int stack_ptr;
  MakeNamePostAlways( OVERLOADED_PTR ) * node;

  node = (MakeNamePostAlways( OVERLOADED_PTR ) *) &sl->CONTAINER_ROOT_MEMBER;
  i = sl->CONTAINER_ROOT_MEMBER;

  CONTAINER_ASSERT( i );

  stack_ptr = 0;

  // find the rightmost node, saving our path along the way
  while ( 1 )
  {
    CONTAINER_COMPARE_RESULT_TYPE comp;

    comp = CONTAINER_COMPARE_ITEMS( remove_item, i );

    if ( comp < CONTAINER_COMPARE_EQUAL_VALUE )
    {
      CONTAINER_ITEM_TYPE * left;

      #if CONTAINER_NEED_EXACT_DUPE_REMOVAL
        remove_left:
      #endif

      left = GET_LEFT( i );

      if ( left == 0 )
        return;  // remove_item not found

      stack[ stack_ptr ] = node;
      side_stack[ stack_ptr ++ ] = 0;

      node = (MakeNamePostAlways( OVERLOADED_PTR ) *) &i->CONTAINER_LEFT_NAME;
      i = left;
    }
    else if ( comp > CONTAINER_COMPARE_EQUAL_VALUE )
    {
      CONTAINER_ITEM_TYPE * right;

      #if CONTAINER_NEED_EXACT_DUPE_REMOVAL
        remove_right:
      #endif

      right = GET_RIGHT( i );

      if ( right == 0 )
        return;  // remove_item not found

      stack[ stack_ptr ] = node;
      side_stack[ stack_ptr ++ ] = 1;

      node = (MakeNamePostAlways( OVERLOADED_PTR ) *) &i->CONTAINER_RIGHT_NAME;
      i = right;
    }
    else
    {
      #if CONTAINER_NEED_EXACT_DUPE_REMOVAL
      if ( remove_item < i )
        goto remove_left;
      else if ( remove_item > i )
        goto remove_right;
      #endif
      break; // found it
    }
  }

  // remove the node
  #if CONTAINER_KEEP_LINKED_LIST
  {
    CONTAINER_ITEM_TYPE * prev, * next;

    // update prev and next pointer - no worries about first and last
    prev = GET_PREV( i );
    next = GET_NEXT( i );

    SET_PREV( next, prev );
    SET_NEXT( prev, next );
  }
  #endif

  left = GET_LEFT( i );
  right = GET_RIGHT( i );

  if ( left == 0 )
  {
    CONTAINER_ASSERT( ( right == 0 ) || ( ( GET_LEFT( right ) == 0 ) && ( GET_RIGHT( right ) == 0 ) ) );

    SET_PTR( *node, right );
  }
  else if ( right == 0 )
  {
    CONTAINER_ASSERT( ( GET_LEFT( left ) == 0 ) && ( GET_RIGHT( left ) == 0 ) );

    SET_PTR( *node, left );
  }
  else
  {
    int unsigned save_stack_ptr;
    CONTAINER_ITEM_TYPE * j, * r;

    // ok, we have two full sides - here's what we're going to do
    //   we going to find the left side's highest right value.
    //   we're going to switch our deleted item with this other value
    //   since we're taking a value off the bottom (to use up at the
    //   top), we keep running the stack down (as if we really deleted
    //   the highest right value).  then we stick the highest right
    //   back in the deleted item position. the only trick to do
    //   is to patch our stack where we walked through to include
    //   the highest right item instead of the deleted item.
    //   then we let the stack get rotated as usual!

    save_stack_ptr = stack_ptr;

    stack[ stack_ptr ] = node;
    side_stack[ stack_ptr ++ ] = 0;

    node = (MakeNamePostAlways( OVERLOADED_PTR ) *) &i->CONTAINER_LEFT_NAME;
    j = left;

    while ( 1 )
    {
      r = GET_RIGHT( j );

      if ( r == 0 )
        break;

      stack[ stack_ptr ] = node;
      side_stack[ stack_ptr ++ ] = 1;

      node = (MakeNamePostAlways( OVERLOADED_PTR ) *) &j->CONTAINER_RIGHT_NAME;
      j = r;
    }

    if ( left != j )
    {
      // copy up the left pointer to the right most item's old spot
      SET_PTR( *node, GET_LEFT( j ) );

      // copy the table data from the deleted item into the right most
      SET_LEFT( j, left );
    }

    SET_RIGHT( j, right );
    SET_SKEW( j, GET_SKEW( i ) );

    // point the parent of the deleted option to the right most item
    SET_PTR( *stack[ save_stack_ptr ], j );

    // patch the stack so that the deleted item is gone and the right most remains
    stack[ save_stack_ptr + 1 ] = (MakeNamePostAlways( OVERLOADED_PTR ) *) &j->CONTAINER_LEFT_NAME;
  }

  // now walk back up the path we took rotating if necessary along the way to rebalance
  while ( stack_ptr )
  {
    --stack_ptr;
    if ( side_stack[ stack_ptr ] == 0 )
    {
      if ( !MakeNamePost( left_has_shrunk ) ( stack[ stack_ptr ] ) )
        break;  // break out early if we're restored to balance
    }
    else
    {
      if ( !MakeNamePost( right_has_shrunk ) ( stack[ stack_ptr ] ) )
        break;  // break out early if we're restored to balance
    }
  }

  // add it to the unused list
  MakeNamePost( add_to_unused ) ( sl, i );
}

#endif


#if CONTAINER_SORTED

// balances left side - returns 1 when a higher rebalance may be necessary, 0 when done

static int MakeNamePost( left_has_grown ) ( MakeNamePostAlways( OVERLOADED_PTR ) * node )
{
  CONTAINER_ITEM_TYPE * item, * left;

  item = GET_PTR( *node );
  left = GET_LEFT( item );

  if ( IS_LEFT_SKEWED( item ) )
  {
    if ( IS_LEFT_SKEWED( left ) )
    {
      SET_NO_SKEW( item );
      SET_NO_SKEW( left );
      MakeNamePost( rotate_right ) ( item, node );
    }
    else
    {
      CONTAINER_ITEM_TYPE * lefts_right;

      lefts_right = GET_RIGHT( left );

      if ( IS_LEFT_SKEWED( lefts_right ) )
      {
        SET_RIGHT_SKEW( item );
        SET_NO_SKEW( left );
      }
      else if ( IS_RIGHT_SKEWED( lefts_right ) )
      {
        SET_NO_SKEW( item );
        SET_LEFT_SKEW( left );
      }
      else
      {
        SET_NO_SKEW( item );
        SET_NO_SKEW( left );
      }

      SET_NO_SKEW( lefts_right );

      MakeNamePost( rotate_left ) ( left, (MakeNamePostAlways( OVERLOADED_PTR )*) &item->CONTAINER_LEFT_NAME );
      MakeNamePost( rotate_right ) ( item, node );
    }
    return( 0 );
  }
  else if ( IS_RIGHT_SKEWED( item ) )
  {
    SET_NO_SKEW( item );
    return( 0 );
  }
  else
  {
    SET_LEFT_SKEW( item );
    return( 1 );
  }
}


// balances right side - returns 1 when a higher rebalance may be necessary, 0 when done

static int MakeNamePost( right_has_grown ) ( MakeNamePostAlways( OVERLOADED_PTR ) * node )
{
  CONTAINER_ITEM_TYPE * item, * right;

  item = GET_PTR( *node );
  right = GET_RIGHT( item );

  if ( IS_LEFT_SKEWED( item ) )
  {
    SET_NO_SKEW( item );
    return( 0 );
  }
  else if ( IS_RIGHT_SKEWED( item ) )
  {
    if ( IS_RIGHT_SKEWED( right ) )
    {
      SET_NO_SKEW( item );
      SET_NO_SKEW( right );
      MakeNamePost( rotate_left ) ( item, node );
    }
    else
    {
      CONTAINER_ITEM_TYPE * rights_left;

      rights_left = GET_LEFT( right );

      if ( IS_RIGHT_SKEWED( rights_left ) )
      {
        SET_LEFT_SKEW( item );
        SET_NO_SKEW( right );
      }
      else if ( IS_LEFT_SKEWED( rights_left ) )
      {
        SET_NO_SKEW( item );
        SET_RIGHT_SKEW( right );
      }
      else
      {
        SET_NO_SKEW( item );
        SET_NO_SKEW( right );
      }

      SET_NO_SKEW( rights_left );

      MakeNamePost( rotate_right ) ( right, (MakeNamePostAlways( OVERLOADED_PTR )*) &item->CONTAINER_RIGHT_NAME );
      MakeNamePost( rotate_left ) ( item, node );
    }
    return( 0 );
  }
  else
  {
    SET_RIGHT_SKEW( item );
    return( 1 );
  }
}


// returns zero when need to possibly balance, 1 if done

static void MakeNamePost( insert_into_tree ) ( CONTAINER_NAME * sl, CONTAINER_ITEM_TYPE * new_item )
{
  MakeNamePostAlways( OVERLOADED_PTR ) * stack[ 32 ]; // would store 2 billion items - probably safe (AVL trees can be unbalanced, so it isn't 4 billion)
  char side_stack[ 32 ];
  int stack_ptr;
  MakeNamePostAlways( OVERLOADED_PTR ) * node;
  CONTAINER_ITEM_TYPE * i;

  #if !CONTAINER_KEEP_LINKED_LIST
    int lefts, rights;

    lefts = 0;
    rights = 0;
  #endif

  if ( sl->CONTAINER_ROOT_MEMBER == 0 )
  {
    sl->CONTAINER_ROOT_MEMBER = new_item;
    sl->CONTAINER_FIRST_MEMBER = new_item;
    sl->CONTAINER_LAST_MEMBER = new_item;

    #if CONTAINER_KEEP_LINKED_LIST
      SET_PREV( new_item, 0 );
      SET_NEXT( new_item, 0 );
    #endif

    SET_LEFT( new_item, 0 );
    SET_RIGHT( new_item, 0 );
    SET_NO_SKEW( new_item );
    return;
  }

  node = (MakeNamePostAlways( OVERLOADED_PTR ) *) &sl->CONTAINER_ROOT_MEMBER;
  i = sl->CONTAINER_ROOT_MEMBER;

  stack_ptr = 0;

  for(;;)
  {
    CONTAINER_COMPARE_RESULT_TYPE comp;

    CONTAINER_ASSERT( new_item != i );

    comp = CONTAINER_COMPARE_ITEMS( new_item, i );

    if ( comp < CONTAINER_COMPARE_EQUAL_VALUE )
    {
      CONTAINER_ITEM_TYPE * left;

      #if CONTAINER_NEED_EXACT_DUPE_REMOVAL
        add_left:
      #endif

      #if !CONTAINER_KEEP_LINKED_LIST
      ++lefts;
      #endif

      left = GET_LEFT( i );

      stack[ stack_ptr ] = node;
      side_stack[ stack_ptr ++ ] = 0;

      if ( left == 0 )
      {
        // found the final position - insert into i's left!
        SET_LEFT( i, new_item );

        #if CONTAINER_KEEP_LINKED_LIST
        {
          CONTAINER_ITEM_TYPE * prev;

          prev = GET_PREV( i );
          if ( prev == 0 )
          {
            sl->CONTAINER_FIRST_MEMBER = new_item;
          }
          else
          {
            SET_NEXT( prev, new_item );
          }

          SET_NEXT( new_item, i );
          SET_PREV( new_item, prev );
          SET_PREV( i, new_item );
        }
        #else
        if ( rights == 0 )
        {
          sl->CONTAINER_FIRST_MEMBER = new_item;
        }
        #endif

        SET_LEFT( new_item, 0 );
        SET_RIGHT( new_item, 0 );
        SET_NO_SKEW( new_item );

        break;
      }

      node = (MakeNamePostAlways( OVERLOADED_PTR ) *) &i->CONTAINER_LEFT_NAME;
      i = left;
    }
    else
    #if CONTAINER_NEED_EXACT_DUPE_REMOVAL
    if ( comp > CONTAINER_COMPARE_EQUAL_VALUE )
    #endif
    {
      CONTAINER_ITEM_TYPE * right;

      #if CONTAINER_NEED_EXACT_DUPE_REMOVAL
        add_right:
      #endif

      #if !CONTAINER_KEEP_LINKED_LIST
      ++rights;
      #endif

      right = GET_RIGHT( i );

      stack[ stack_ptr ] = node;
      side_stack[ stack_ptr ++ ] = 1;

      if ( right == 0 )
      {
        // found the final position - insert into i's right!

        SET_RIGHT( i, new_item );

        #if CONTAINER_KEEP_LINKED_LIST
        {
          CONTAINER_ITEM_TYPE * next;

          next = GET_NEXT( i );
          if ( next == 0 )
          {
            sl->CONTAINER_LAST_MEMBER = new_item;
          }
          else
          {
            SET_PREV( next, new_item );
          }

          SET_PREV( new_item, i );
          SET_NEXT( new_item, next );
          SET_NEXT( i, new_item );
        }
        #else
        if ( lefts == 0 )
        {
          sl->CONTAINER_LAST_MEMBER = new_item;
        }
        #endif

        SET_LEFT( new_item, 0 );
        SET_RIGHT( new_item, 0 );
        SET_NO_SKEW( new_item );

        break;
      }

      node = (MakeNamePostAlways( OVERLOADED_PTR ) *) &i->CONTAINER_RIGHT_NAME;
      i = right;
    }
    #if CONTAINER_NEED_EXACT_DUPE_REMOVAL
    else
    {
      if ( new_item < i )
        goto add_left;
      else
        goto add_right;
    }
    #endif
  }

  // now walk back up the path we took rotating if necessary along the way to rebalance
  do
  {
    --stack_ptr;
    if ( side_stack[ stack_ptr ] == 0 )
    {
      if ( !MakeNamePost( left_has_grown ) ( stack[ stack_ptr ] ) )
        break;
    }
    else
    {
      if ( !MakeNamePost( right_has_grown ) ( stack[ stack_ptr ] ) )
        break;
    }
  } while ( stack_ptr );
}

#endif

#endif

#endif

//========================================================================
// public functions
//========================================================================


CONTAINER_FUNCTION_DECORATE( void ) MakeNamePost( Initialize ) ( CONTAINER_NAME * sl, int HintItems )
#if CONTAINER_EMIT_CODE
{
  #if CONTAINER_SORTED
    sl->CONTAINER_ROOT_MEMBER = 0;
  #else
    #if CONTAINER_DO_ALLOCATION
      #if CONTAINER_KEEP_LINKED_LIST
        sl->CONTAINER_REMOVE_PREV_MEMBER = 0;
        sl->CONTAINER_REMOVE_NEXT_MEMBER = 0;
      #endif
    #endif
  #endif

  #if CONTAINER_SORTED || CONTAINER_KEEP_LINKED_LIST
    sl->CONTAINER_FIRST_MEMBER = 0;
    sl->CONTAINER_LAST_MEMBER = 0;
  #endif

  #if CONTAINER_DO_ALLOCATION
    sl->CONTAINER_BUFS_MEMBER = 0;
    MakeNamePost( add_new_buffer ) ( sl, HintItems );
  #endif
}
#else
;
#endif

CONTAINER_FUNCTION_DECORATE( CONTAINER_ITEM_TYPE * ) MakeNamePost( Add )( CONTAINER_NAME * sl,
#if CONTAINER_DO_ALLOCATION
                                                         CONTAINER_ADD_FIELDS )
#else
                                                         CONTAINER_ITEM_TYPE * new_item )
#endif
#if CONTAINER_EMIT_CODE
{
/*
  VERY SPECIAL NOTICE FROM CASEY:
  I count on the ADD_ASSIGN happening before the insertion/next-prev-fixup
  stuff, because in the ADD_ASSIGN, I often will romp the whole structure
  from somewhere else and such, so the next/prev/left/right pointers
  WILL get destroyed.  So this code is great just as it is, please
  don't rearrange it!!  Thanks.
 */

  #if CONTAINER_DO_ALLOCATION
    CONTAINER_ITEM_TYPE * new_item;

    new_item = MakeNamePost( get_unused_item ) ( sl );
    if (!new_item)
      return 0;

    CONTAINER_ADD_ASSIGN( new_item );
  #endif

  #if CONTAINER_SORTED
    MakeNamePost( insert_into_tree ) ( sl, new_item );

    #if TEST
      #if CONTAINER_KEEP_LINKED_LIST
      {
        CONTAINER_ITEM_TYPE * i = sl->CONTAINER_ROOT_MEMBER;
        while ( GET_LEFT( i ) )
        {
          i = GET_LEFT( i );
        }
        CONTAINER_ASSERT( ( sl->CONTAINER_FIRST_MEMBER == i ) && ( GET_PREV( sl->CONTAINER_FIRST_MEMBER ) == 0 ) );

        if (GET_NEXT(new_item) )
        {
          CONTAINER_ASSERT( GET_PREV( GET_NEXT( new_item ) ) == new_item );
        }
      }
      #endif
    #endif

  #else

    #if CONTAINER_KEEP_LINKED_LIST
      SET_NEXT( new_item, 0 );
      SET_PREV( new_item, sl->CONTAINER_LAST_MEMBER );

      if ( sl->CONTAINER_LAST_MEMBER )
      {
        SET_NEXT( sl->CONTAINER_LAST_MEMBER, new_item );
      }
      if ( sl->CONTAINER_FIRST_MEMBER == 0 )
      {
        sl->CONTAINER_FIRST_MEMBER = new_item;
      }
      sl->CONTAINER_LAST_MEMBER = new_item;
    #endif

  #endif

  #ifdef CONTAINER_DEBUG_FORMAT_STRING
    MakeNamePost( draw_tree ) ( sl );
  #endif

  return( new_item );
}
#else
;
#endif


#if CONTAINER_NEED_REMOVE

CONTAINER_FUNCTION_DECORATE( CONTAINER_ITEM_TYPE * ) MakeNamePost( Remove )( CONTAINER_NAME * sl, CONTAINER_ITEM_TYPE * item )
#if CONTAINER_EMIT_CODE
{
  #if CONTAINER_SORTED
    if ( sl->CONTAINER_FIRST_MEMBER == item )
    {
      MakeNamePost( get_and_remove_top ) ( sl );

      #if TEST
        #if CONTAINER_KEEP_LINKED_LIST
          if ( sl->CONTAINER_ROOT_MEMBER )
          {
            CONTAINER_ITEM_TYPE * i = sl->CONTAINER_ROOT_MEMBER;
            while ( GET_LEFT( i ) )
            {
              i = GET_LEFT( i );
            }
            CONTAINER_ASSERT( ( sl->CONTAINER_FIRST_MEMBER == i ) && ( GET_PREV( sl->CONTAINER_FIRST_MEMBER ) == 0 ) );
          }
        #endif
      #endif
    }
    else if ( sl->CONTAINER_LAST_MEMBER == item )
    {
      MakeNamePost( get_and_remove_bottom ) ( sl );

      #if TEST
        #if CONTAINER_KEEP_LINKED_LIST
          if ( sl->CONTAINER_ROOT_MEMBER )
          {
            CONTAINER_ITEM_TYPE * i = sl->CONTAINER_ROOT_MEMBER;
            while ( GET_RIGHT( i ) )
            {
              i = GET_RIGHT( i );
            }
            CONTAINER_ASSERT( ( sl->CONTAINER_LAST_MEMBER == i ) && ( GET_NEXT( sl->CONTAINER_LAST_MEMBER ) == 0 ) );
          }
        #endif
      #endif
    }
    else
    {
      MakeNamePost( get_and_remove ) ( sl, item );
    }

  #else

    #if CONTAINER_KEEP_LINKED_LIST

    CONTAINER_ITEM_TYPE * prev, * next;

    prev = GET_PREV( item );
    next = GET_NEXT( item );

    if ( prev )
    {
      SET_NEXT( prev, next );
    }
    else
    {
      sl->CONTAINER_FIRST_MEMBER = next;
    }

    if ( next )
    {
      SET_PREV( next, prev );
    }
    else
    {
      sl->CONTAINER_LAST_MEMBER = prev;
    }

    #endif

    MakeNamePost( add_to_unused ) ( sl, item );

  #endif

  #ifdef CONTAINER_DEBUG_FORMAT_STRING
    MakeNamePost( draw_tree ) ( sl );
  #endif

  return( sl->CONTAINER_UNUSED_MEMBER );
}
#else
;
#endif

#endif


#if CONTAINER_NEED_REMOVEFIRST && ( CONTAINER_SORTED || CONTAINER_KEEP_LINKED_LIST )

CONTAINER_FUNCTION_DECORATE( CONTAINER_ITEM_TYPE * ) MakeNamePost( RemoveFirst )( CONTAINER_NAME * sl )
#if CONTAINER_EMIT_CODE
{
  if ( sl->CONTAINER_FIRST_MEMBER == 0 )
    return( 0 );

  #if CONTAINER_SORTED
    MakeNamePost( get_and_remove_top ) ( sl );

    #if TEST
      #if CONTAINER_KEEP_LINKED_LIST
        if ( sl->CONTAINER_ROOT_MEMBER )
        {
          CONTAINER_ITEM_TYPE * i = sl->CONTAINER_ROOT_MEMBER;
          while ( GET_LEFT( i ) )
          {
            i = GET_LEFT( i );
          }
          CONTAINER_ASSERT( ( sl->CONTAINER_FIRST_MEMBER == i ) && ( GET_PREV( sl->CONTAINER_FIRST_MEMBER ) == 0 ) );
        }
      #endif
    #endif
  #else
  {
    CONTAINER_ITEM_TYPE * next, * item;

    item = sl->CONTAINER_FIRST_MEMBER;

    next = GET_NEXT( item );

    sl->CONTAINER_FIRST_MEMBER = next;

    if ( next )
    {
      SET_PREV( next, 0 );
    }
    else
    {
      sl->CONTAINER_LAST_MEMBER = 0;
    }

    MakeNamePost( add_to_unused ) ( sl, item );
  }
  #endif

  #ifdef CONTAINER_DEBUG_FORMAT_STRING
    MakeNamePost( draw_tree ) ( sl );
  #endif

  return( sl->CONTAINER_UNUSED_MEMBER );
}
#else
;
#endif

#endif


#if CONTAINER_NEED_REMOVELAST && ( CONTAINER_SORTED || CONTAINER_KEEP_LINKED_LIST )

CONTAINER_FUNCTION_DECORATE( CONTAINER_ITEM_TYPE * ) MakeNamePost( RemoveLast )( CONTAINER_NAME * sl )
#if CONTAINER_EMIT_CODE
{
  if ( sl->CONTAINER_LAST_MEMBER == 0 )
    return( 0 );

  #if CONTAINER_SORTED
    MakeNamePost( get_and_remove_bottom ) ( sl );

    #if TEST
      #if CONTAINER_KEEP_LINKED_LIST
        if ( sl->CONTAINER_ROOT_MEMBER )
        {
          CONTAINER_ITEM_TYPE * i = sl->CONTAINER_ROOT_MEMBER;
          while ( GET_RIGHT( i ) )
          {
            i = GET_RIGHT( i );
          }
          CONTAINER_ASSERT( ( sl->CONTAINER_LAST_MEMBER == i ) && ( GET_NEXT( sl->CONTAINER_LAST_MEMBER ) == 0 ) );
        }
      #endif
    #endif

  #else
  {
    CONTAINER_ITEM_TYPE * prev, * item;

    item = sl->CONTAINER_LAST_MEMBER;

    prev = GET_PREV( item );

    if ( prev )
    {
      SET_NEXT( prev, 0 );
    }
    else
    {
      sl->CONTAINER_FIRST_MEMBER = 0;
    }

    sl->CONTAINER_LAST_MEMBER = prev;

    MakeNamePost( add_to_unused ) ( sl, item );
  }
  #endif

  #ifdef CONTAINER_DEBUG_FORMAT_STRING
    MakeNamePost( draw_tree ) ( sl );
  #endif

  return( sl->CONTAINER_UNUSED_MEMBER );
}
#else
;
#endif

#endif


#if CONTAINER_NEED_FIRST && ( CONTAINER_SORTED || CONTAINER_KEEP_LINKED_LIST )

CONTAINER_FUNCTION_DECORATE( CONTAINER_ITEM_TYPE * ) MakeNamePost( First )( CONTAINER_NAME * sl )
#if CONTAINER_EMIT_CODE
{
  return( sl->CONTAINER_FIRST_MEMBER );
}
#else
;
#endif

#endif


#if CONTAINER_NEED_NEXT && ( CONTAINER_SORTED || CONTAINER_KEEP_LINKED_LIST )

CONTAINER_FUNCTION_DECORATE( CONTAINER_ITEM_TYPE * ) MakeNamePost( Next )( CONTAINER_NAME * sl, CONTAINER_ITEM_TYPE * item )
#if CONTAINER_EMIT_CODE
{
  #if CONTAINER_KEEP_LINKED_LIST
    #if !CONTAINER_SORTED && CONTAINER_DO_ALLOCATION
      if ( item == sl->CONTAINER_UNUSED_MEMBER )
        return( sl->CONTAINER_REMOVE_NEXT_MEMBER );
      else
        return( GET_NEXT( item ) );
    #else
      return( GET_NEXT( item ) );
    #endif
  #else
  {
    CONTAINER_ITEM_TYPE * i;

    i = GET_RIGHT( item );

    if ( i )
    {
      // find the leftmost branch off the right
      while ( 1 )
      {
        CONTAINER_ITEM_TYPE * left;

        left = GET_LEFT( i );

        if ( left == 0 )
        {
          break;
        }

        i = left;
      }

      return( i );
    }
    else
    {

      CONTAINER_ITEM_TYPE * last_left;

      i = sl->CONTAINER_ROOT_MEMBER;

      last_left = sl->CONTAINER_LAST_MEMBER;

      while ( i )
      {
        CONTAINER_COMPARE_RESULT_TYPE comp;

        comp = CONTAINER_COMPARE_ITEMS( item, i );

        if ( comp < CONTAINER_COMPARE_EQUAL_VALUE )
        {
          CONTAINER_ITEM_TYPE * left;

          #if CONTAINER_NEED_EXACT_DUPE_REMOVAL
            do_left:
          #endif

          left = GET_LEFT( i );

          if ( left == 0 )
          {
            return( i );
          }

          last_left = i;
          i = left;
        }
        #if CONTAINER_NEED_EXACT_DUPE_REMOVAL
        else if ( comp > CONTAINER_COMPARE_EQUAL_VALUE )
        #else
        else
        #endif
        {
          CONTAINER_ITEM_TYPE * right;

          #if CONTAINER_NEED_EXACT_DUPE_REMOVAL
            do_right:
          #endif

          right = GET_RIGHT( i );

          if ( right == 0 )
          {
            return( ( last_left == item ) ? 0 : last_left  );
          }

          i = right;
        }
        #if CONTAINER_NEED_EXACT_DUPE_REMOVAL
        else
        {
          if ( item < i )
            goto do_left;
          else
            goto do_right;
        }
        #endif
      }

      // not found - return the end of the list
      return( ( sl->CONTAINER_LAST_MEMBER == item ) ? 0 : sl->CONTAINER_LAST_MEMBER );
    }
  }
  #endif
}
#else
;
#endif

#endif


#if CONTAINER_NEED_PREVIOUS && ( CONTAINER_SORTED || CONTAINER_KEEP_LINKED_LIST )

CONTAINER_FUNCTION_DECORATE( CONTAINER_ITEM_TYPE * ) MakeNamePost( Previous )( CONTAINER_NAME * sl, CONTAINER_ITEM_TYPE * item )
#if CONTAINER_EMIT_CODE
{
  #if CONTAINER_KEEP_LINKED_LIST
    #if !CONTAINER_SORTED && CONTAINER_DO_ALLOCATION
      if ( item == sl->CONTAINER_UNUSED_MEMBER )
        return( sl->CONTAINER_REMOVE_PREV_MEMBER );
      else
        return( GET_PREV( item ) );
    #else
      return( GET_PREV( item ) );
    #endif
  #else
  {
    CONTAINER_ITEM_TYPE * i;

    i = GET_LEFT( item );

    if ( i )
    {
      // find the rightmost branch off the left
      while ( 1 )
      {
        CONTAINER_ITEM_TYPE * right;

        right = GET_RIGHT( i );

        if ( right == 0 )
        {
          break;
        }

        i = right;
      }

      return( i );
    }
    else
    {

      CONTAINER_ITEM_TYPE * i, * last_right;

      i = sl->CONTAINER_ROOT_MEMBER;

      last_right = sl->CONTAINER_FIRST_MEMBER;

      while ( i )
      {
        CONTAINER_COMPARE_RESULT_TYPE comp;

        comp = CONTAINER_COMPARE_ITEMS( item, i );

        if ( comp > CONTAINER_COMPARE_EQUAL_VALUE )
        {
          CONTAINER_ITEM_TYPE * right;

          #if CONTAINER_NEED_EXACT_DUPE_REMOVAL
            do_right:
          #endif

          right = GET_RIGHT( i );

          if ( right == 0 )
          {
            return( i );
          }

          last_right = i;

          i = right;
        }
        #if CONTAINER_NEED_EXACT_DUPE_REMOVAL
        else if ( comp < CONTAINER_COMPARE_EQUAL_VALUE )
        #else
        else
        #endif
        {
          CONTAINER_ITEM_TYPE * left;

          #if CONTAINER_NEED_EXACT_DUPE_REMOVAL
            do_left:
          #endif

          left = GET_LEFT( i );

          if ( left == 0 )
          {
            return( ( last_right == item ) ? 0 : last_right );
          }

          i = left;
        }
        #if CONTAINER_NEED_EXACT_DUPE_REMOVAL
        else
        {
          if ( item > i )
            goto do_right;
          else
            goto do_left;
        }
        #endif
      }

      // not found
      return( ( sl->CONTAINER_FIRST_MEMBER == item ) ? 0 : sl->CONTAINER_FIRST_MEMBER );
    }
  }
  #endif
}
#else
;
#endif

#endif


#if CONTAINER_NEED_LAST && ( CONTAINER_SORTED || CONTAINER_KEEP_LINKED_LIST )

CONTAINER_FUNCTION_DECORATE ( CONTAINER_ITEM_TYPE * ) MakeNamePost( Last )( CONTAINER_NAME * sl )
#if CONTAINER_EMIT_CODE
{
  return( sl->CONTAINER_LAST_MEMBER );
}
#else
;
#endif

#endif


#if ( CONTAINER_NEED_FIND ) && defined( CONTAINER_FIND_FIELDS ) && defined( CONTAINER_COMPARE_FIND_FIELDS )  && ( CONTAINER_SORTED || CONTAINER_KEEP_LINKED_LIST )

CONTAINER_FUNCTION_DECORATE ( CONTAINER_ITEM_TYPE * ) MakeNamePost( Find )( CONTAINER_NAME * sl, CONTAINER_FIND_FIELDS )
#if CONTAINER_EMIT_CODE
{
  #if CONTAINER_SORTED
    CONTAINER_ITEM_TYPE * i;

    i = sl->CONTAINER_ROOT_MEMBER;

    while ( i )
    {
      CONTAINER_COMPARE_RESULT_TYPE comp;

      comp = CONTAINER_COMPARE_FIND_FIELDS( i );

      if ( comp < CONTAINER_COMPARE_EQUAL_VALUE )
      {
        i = GET_LEFT( i );
      }
      else if ( comp > CONTAINER_COMPARE_EQUAL_VALUE )
      {
        i = GET_RIGHT( i );
      }
      else
      {
        return( i );
      }
    }

    // not found
    return( 0 );
  #else
    CONTAINER_ITEM_TYPE * i;

    i = sl->CONTAINER_FIRST_MEMBER;

    while ( i )
    {
      if ( CONTAINER_COMPARE_FIND_FIELDS( i ) == CONTAINER_COMPARE_EQUAL_VALUE )
      {
        return( i );
      }

      i = GET_NEXT( i );
    }

    return( 0 );
  #endif
}
#else
;
#endif

#endif


#if ( CONTAINER_NEED_FINDGT ) && defined( CONTAINER_FIND_FIELDS ) && defined( CONTAINER_COMPARE_FIND_FIELDS )  && ( CONTAINER_SORTED || CONTAINER_KEEP_LINKED_LIST )

CONTAINER_FUNCTION_DECORATE ( CONTAINER_ITEM_TYPE * ) MakeNamePost( FindGT )( CONTAINER_NAME * sl, CONTAINER_FIND_FIELDS )
#if CONTAINER_EMIT_CODE
{
  #if CONTAINER_SORTED
    CONTAINER_ITEM_TYPE * i, * last_left;

    i = sl->CONTAINER_ROOT_MEMBER;

    last_left = 0;

    while ( i )
    {
      CONTAINER_COMPARE_RESULT_TYPE comp;

      comp = CONTAINER_COMPARE_FIND_FIELDS( i );

      if ( comp < CONTAINER_COMPARE_EQUAL_VALUE )
      {
        CONTAINER_ITEM_TYPE * left;

        left = GET_LEFT( i );

        if ( left == 0 )
        {
          return( i );
        }

        last_left = i;
        i = left;
      }
      else
      {
        CONTAINER_ITEM_TYPE * right;

        right = GET_RIGHT( i );

        if ( right == 0 )
        {
          return( last_left );
        }

        i = right;
      }
    }

    // not found
    return( 0 );
  #else
    CONTAINER_ITEM_TYPE * i, * best;

    i = sl->CONTAINER_FIRST_MEMBER;

    best = 0;

    while ( i )
    {
      if ( CONTAINER_COMPARE_FIND_FIELDS( i ) < CONTAINER_COMPARE_EQUAL_VALUE )
      {
        if ( best )
        {
          if ( CONTAINER_COMPARE_ITEMS( i, best ) < CONTAINER_COMPARE_EQUAL_VALUE )
          {
            best = i;
          }
        }
        else
        {
          best = i;
        }
      }

      i = GET_NEXT( i );
    }

    return( best );
  #endif
}
#else
;
#endif

#endif


#if ( CONTAINER_NEED_FINDLT ) && defined( CONTAINER_FIND_FIELDS ) && defined( CONTAINER_COMPARE_FIND_FIELDS ) && ( CONTAINER_SORTED || CONTAINER_KEEP_LINKED_LIST )

CONTAINER_FUNCTION_DECORATE ( CONTAINER_ITEM_TYPE * ) MakeNamePost( FindLT )( CONTAINER_NAME * sl, CONTAINER_FIND_FIELDS )
#if CONTAINER_EMIT_CODE
{
  #if CONTAINER_SORTED
    CONTAINER_ITEM_TYPE * i, * last_right;

    i = sl->CONTAINER_ROOT_MEMBER;

    last_right = 0;

    while ( i )
    {
      CONTAINER_COMPARE_RESULT_TYPE comp;

      comp = CONTAINER_COMPARE_FIND_FIELDS( i );

      if ( comp <= CONTAINER_COMPARE_EQUAL_VALUE )
      {
        CONTAINER_ITEM_TYPE * left;

        left = GET_LEFT( i );

        if ( left == 0 )
        {
          return( last_right );
        }

        i = left;
      }
      else if ( comp > CONTAINER_COMPARE_EQUAL_VALUE )
      {
        CONTAINER_ITEM_TYPE * right;

        right = GET_RIGHT( i );

        if ( right == 0 )
        {
          return( i );
        }

        last_right = i;

        i = right;
      }
    }

    // not found
    return( 0 );

  #else

    CONTAINER_ITEM_TYPE * i, * best;

    i = sl->CONTAINER_FIRST_MEMBER;

    best = 0;

    while ( i )
    {
      if ( CONTAINER_COMPARE_FIND_FIELDS( i ) > CONTAINER_COMPARE_EQUAL_VALUE )
      {
        if ( best )
        {
          if ( CONTAINER_COMPARE_ITEMS( i, best ) > CONTAINER_COMPARE_EQUAL_VALUE )
          {
            best = i;
          }
        }
        else
        {
          best = i;
        }
      }

      i = GET_NEXT( i );
    }

    return( best );
  #endif
}
#else
;
#endif

#endif


#if CONTAINER_NEED_CLEAR

CONTAINER_FUNCTION_DECORATE( void ) MakeNamePost( Clear )( CONTAINER_NAME * sl )
#if CONTAINER_EMIT_CODE
{
  #if CONTAINER_DO_ALLOCATION

    MakeNamePostAlways( ITEM_BUFFER ) * b;

    b = (MakeNamePostAlways( ITEM_BUFFER ) *) sl->CONTAINER_BUFS_MEMBER;

  #endif

  #if CONTAINER_NEED_UNUSED
  sl->CONTAINER_UNUSED_MEMBER = 0;
  #endif

  #if CONTAINER_DO_ALLOCATION

  while ( b )
  {
    SET_UNUSED_NEXT( b->items, sl->CONTAINER_UNUSED_MEMBER );
    SET_UNUSED_COUNT( b->items, b->num_items );
    sl->CONTAINER_UNUSED_MEMBER = b->items;
    b = b->next;
  }

  #endif

  #if CONTAINER_SORTED
    sl->CONTAINER_ROOT_MEMBER = 0;
  #else
    #if CONTAINER_DO_ALLOCATION
      #if CONTAINER_KEEP_LINKED_LIST
        sl->CONTAINER_REMOVE_PREV_MEMBER = 0;
        sl->CONTAINER_REMOVE_NEXT_MEMBER = 0;
      #endif
    #endif
  #endif

  #if CONTAINER_SORTED || CONTAINER_KEEP_LINKED_LIST
    sl->CONTAINER_FIRST_MEMBER = 0;
    sl->CONTAINER_LAST_MEMBER = 0;
  #endif
}
#else
;
#endif

#endif


#if CONTAINER_DO_ALLOCATION

CONTAINER_FUNCTION_DECORATE( void ) MakeNamePost( FreeMemory )( CONTAINER_NAME * sl )
#if CONTAINER_EMIT_CODE
{
  MakeNamePostAlways( ITEM_BUFFER ) * b;

  b = (MakeNamePostAlways( ITEM_BUFFER ) *) sl->CONTAINER_BUFS_MEMBER;

  while ( b )
  {
    b = MakeNamePost( free_buffer ) ( b );
  }

  #if CONTAINER_SORTED
    sl->CONTAINER_ROOT_MEMBER = 0;
  #else
    #if CONTAINER_DO_ALLOCATION
      #if CONTAINER_KEEP_LINKED_LIST
        sl->CONTAINER_REMOVE_PREV_MEMBER = 0;
        sl->CONTAINER_REMOVE_NEXT_MEMBER = 0;
      #endif
    #endif
  #endif

  #if CONTAINER_SORTED || CONTAINER_KEEP_LINKED_LIST
    sl->CONTAINER_FIRST_MEMBER = 0;
    sl->CONTAINER_LAST_MEMBER = 0;
  #endif

  sl->CONTAINER_BUFS_MEMBER = 0;
  sl->CONTAINER_UNUSED_MEMBER = 0;
}
#else
;
#endif

#endif


#if !CONTAINER_NO_UNDEFINE

#ifdef SET_PTR
  #undef SET_PTR
#endif

#ifdef GET_PTR
  #undef GET_PTR
#endif

#ifdef SET_LEFT_SKEW
  #undef SET_LEFT_SKEW
#endif

#ifdef SET_NO_SKEW
  #undef SET_NO_SKEW
#endif

#ifdef SET_RIGHT_SKEW
  #undef SET_RIGHT_SKEW
#endif

#ifdef SET_SKEW
  #undef SET_SKEW
#endif

#ifdef GET_SKEW
  #undef GET_SKEW
#endif

#ifdef IS_LEFT_SKEWED
  #undef IS_LEFT_SKEWED
#endif

#ifdef IS_RIGHT_SKEWED
  #undef IS_RIGHT_SKEWED
#endif

#ifdef IS_NOT_SKEWED
  #undef IS_NOT_SKEWED
#endif

#ifdef SET_LEFT
  #undef SET_LEFT
#endif

#ifdef GET_LEFT
  #undef GET_LEFT
#endif

#ifdef SET_RIGHT
  #undef SET_RIGHT
#endif

#ifdef GET_RIGHT
  #undef GET_RIGHT
#endif

#ifdef CONTAINER_NAME
  #undef CONTAINER_NAME
#endif

#ifdef CONTAINER_ITEM_TYPE
  #undef CONTAINER_ITEM_TYPE
#endif

#ifdef CONTAINER_ADD_FIELDS
  #undef CONTAINER_ADD_FIELDS
#endif

#ifdef CONTAINER_COMPARE_ITEMS
  #undef CONTAINER_COMPARE_ITEMS
#endif

#ifdef CONTAINER_ADD_ASSIGN
  #undef CONTAINER_ADD_ASSIGN
#endif

#ifdef CONTAINER_FIND_FIELDS
  #undef CONTAINER_FIND_FIELDS
#endif

#ifdef CONTAINER_COMPARE_FIND_FIELDS
  #undef CONTAINER_COMPARE_FIND_FIELDS
#endif

#ifdef CONTAINER_COMPARE_EQUAL_VALUE
  #undef CONTAINER_COMPARE_EQUAL_VALUE
#endif

#ifdef CONTAINER_COMPARE_RESULT_TYPE
  #undef CONTAINER_COMPARE_RESULT_TYPE
#endif

#ifdef CONTAINER_SORTED
  #undef CONTAINER_SORTED
#endif

#ifdef CONTAINER_KEEP_LINKED_LIST
  #undef CONTAINER_KEEP_LINKED_LIST
#endif

#ifdef CONTAINER_UNUSED_ITEM_FIELD
  #undef CONTAINER_UNUSED_ITEM_FIELD
#endif

#ifdef CONTAINER_FUNCTION_DECORATE
  #undef CONTAINER_FUNCTION_DECORATE
#endif

#ifdef CONTAINER_LEFT_NAME
  #undef CONTAINER_LEFT_NAME
#endif

#ifdef CONTAINER_RIGHT_NAME
  #undef CONTAINER_RIGHT_NAME
#endif

#ifdef CONTAINER_PREV_NAME
  #undef CONTAINER_PREV_NAME
#endif

#ifdef CONTAINER_NEXT_NAME
  #undef CONTAINER_NEXT_NAME
#endif

#ifdef CONTAINER_UNUSED_COUNT_NAME
  #undef CONTAINER_UNUSED_COUNT_NAME
#endif

#ifdef CONTAINER_UNUSED_NEXT_NAME
  #undef CONTAINER_UNUSED_NEXT_NAME
#endif

#ifdef CONTAINER_DO_ALLOCATION
  #undef CONTAINER_DO_ALLOCATION
#endif

#ifdef CONTAINER_NUM_PER_ALLOCATION
  #undef CONTAINER_NUM_PER_ALLOCATION
#endif

#ifdef CONTAINER_NEED_EXACT_DUPE_REMOVAL
  #undef CONTAINER_NEED_EXACT_DUPE_REMOVAL
#endif

#ifdef CONTAINER_EMIT_CODE
  #undef CONTAINER_EMIT_CODE
#endif

#ifdef CONTAINER_MALLOC
  #undef CONTAINER_MALLOC
#endif

#ifdef CONTAINER_FREE
  #undef CONTAINER_FREE
#endif

#ifdef CONTAINER_ASSERT
  #undef CONTAINER_ASSERT
#endif

#ifdef CONTAINER_DEBUG_FORMAT_STRING
  #undef CONTAINER_DEBUG_FORMAT_STRING
#endif

#ifdef CONTAINER_NEED_REMOVE
  #undef CONTAINER_NEED_REMOVE
#endif

#ifdef CONTAINER_NEED_REMOVEFIRST
  #undef CONTAINER_NEED_REMOVEFIRST
#endif

#ifdef CONTAINER_NEED_REMOVELAST
  #undef CONTAINER_NEED_REMOVELAST
#endif

#ifdef CONTAINER_NEED_PREVIOUS
  #undef CONTAINER_NEED_PREVIOUS
#endif

#ifdef CONTAINER_NEED_NEXT
  #undef CONTAINER_NEED_NEXT
#endif

#ifdef CONTAINER_NEED_FIRST
  #undef CONTAINER_NEED_FIRST
#endif

#ifdef CONTAINER_NEED_LAST
  #undef CONTAINER_NEED_LAST
#endif

#ifdef CONTAINER_NEED_FIND
  #undef CONTAINER_NEED_FIND
#endif

#ifdef CONTAINER_NEED_FINDLT
  #undef CONTAINER_NEED_FINDLT
#endif

#ifdef CONTAINER_NEED_FINDGT
  #undef CONTAINER_NEED_FINDGT
#endif

#ifdef CONTAINER_NEED_CLEAR
  #undef CONTAINER_NEED_CLEAR
#endif

#ifdef CONTAINER_NEED_UNUSED
  #undef CONTAINER_NEED_UNUSED
#endif

#ifdef CONTAINER_USE_OVERLOADING
  #undef CONTAINER_USE_OVERLOADING
#endif

#ifdef CONTAINER_UNUSED_ITEM_FIELD
  #undef CONTAINER_UNUSED_ITEM_FIELD
#endif

#ifdef CONTAINER_SUPPORT_DUPES
  #undef CONTAINER_SUPPORT_DUPES
#endif

#ifdef CONTAINER_NEED_EXACT_DUPE_REMOVAL
  #undef CONTAINER_NEED_EXACT_DUPE_REMOVAL
#endif

#ifdef CONTAINER_STRUCT_ALREADY_DEFINED
  #undef CONTAINER_STRUCT_ALREADY_DEFINED
#endif

#ifdef CONTAINER_ROOT_MEMBER
  #undef CONTAINER_ROOT_MEMBER
#endif

#ifdef CONTAINER_UNUSED_MEMBER
  #undef CONTAINER_UNUSED_MEMBER
#endif

#ifdef CONTAINER_FIRST_MEMBER
  #undef CONTAINER_FIRST_MEMBER
#endif

#ifdef CONTAINER_LAST_MEMBER
  #undef CONTAINER_LAST_MEMBER
#endif

#ifdef CONTAINER_REMOVE_PREV_MEMBER
  #undef CONTAINER_REMOVE_PREV_MEMBER
#endif

#ifdef CONTAINER_REMOVE_NEXT_MEMBER
  #undef CONTAINER_REMOVE_NEXT_MEMBER
#endif

#endif
