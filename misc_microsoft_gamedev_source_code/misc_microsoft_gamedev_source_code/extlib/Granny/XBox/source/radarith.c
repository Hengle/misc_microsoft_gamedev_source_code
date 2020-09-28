//#define EXACT

#ifdef EXACT
  #include "radarith_exact.inl"
#else

#include "radarith.h"
#include "raddebug.h"
#include "radmath.h"
#include "radmemutils.h"
#include <string.h>

// adaptive arithmetic modeller 

#define NORM_BITS 14
#define NORM_COUNT ( 1 << NORM_BITS )
#define ADJ_SUMS ( NORM_COUNT << 1 )
#define OVERFLOW_COUNT ( NORM_BITS / 2 ) - 1  // long story

#define COUNTTYPE U16

typedef struct ARITHDATA
{
  COUNTTYPE singles_tot;                      // 0
  COUNTTYPE update_tot;                       // 2

  COUNTTYPE update_max;                       // 4
  COUNTTYPE update_range;                     // 6

  COUNTTYPE rescale_tot;                      // 8
  COUNTTYPE singles_length;                   // 10 

  COUNTTYPE summed_length;                    // 12
  COUNTTYPE unique_count;                     // 14

  COUNTTYPE * values;                         // 16
  COUNTTYPE * single_counts;                  // 20

  // these defines are used at compression time not decompression time where
  //   table_walks is used, so we save space by overloading mid.  this would be better
  //   with an anonymous union, but that isn't supported in standard C.
  
  #define unique_max( a ) ( ( (U32*) &(a)->table_walks[ 0 ] )[ 0 ] )
  #define comp_reverse( a ) ( ( (COUNTTYPE**) &(a)->table_walks[ 0 ] )[ 1 ] )
  
  // the OVERFLOW_COUNT u16's before summed_counts must never be larger than ADJ_SUMS!
  //   that means the top and bottom of these u32s!!  basically, we read off the beginning
  //   of summed counts by OVERFLOW_COUNT indexes, if we keep the values below ADJ_SUMS, 
  //   then the table search just works.  so sue me for wanting to save memory.
  // Addendum: note that you had better make sure that the index you're using to do this
  //   is /signed/, rather than, say, a U32, or 64-bit unhappiness will surely find you.

  U32 table_walks[ NORM_BITS ];               // 24
  COUNTTYPE summed_counts[ OVERFLOW_COUNT ];  // 80
} ARITHDATA;


#if 0	// Not actually used
static int get_highest_bit_shift( int value )
{
  if ( value >= ( 1 << 16 ) )
    if ( value >= ( 1 << 24 ) )
      if ( value >= ( 1 << 28 ) )
        if ( value >= ( 1 << 30 ) )
          return( ( value >= ( 1 << 31 ) ) ? 31 : 30 );
        else
          return( ( value >= ( 1 << 29 ) ) ? 29 : 28 );
      else
        if ( value >= ( 1 << 26 ) )
          return( ( value >= ( 1 << 27 ) ) ? 27 : 16 );
        else
          return( ( value >= ( 1 << 25 ) ) ? 25 : 24 );
    else
      if ( value >= ( 1 << 20 ) )
        if ( value >= ( 1 << 22 ) )
          return( ( value >= ( 1 << 23 ) ) ? 23 : 22 );
        else
          return( ( value >= ( 1 << 21 ) ) ? 21 : 20 );
      else
        if ( value >= ( 1 << 18 ) )
          return( ( value >= ( 1 << 19 ) ) ? 19 : 18 );
        else
          return( ( value >= ( 1 << 17 ) ) ? 17 : 16 );
  else
    if ( value >= ( 1 << 8 ) )
      if ( value >= ( 1 << 12 ) )
        if ( value >= ( 1 << 14 ) )
          return( ( value >= ( 1 << 15 ) ) ? 15 : 14 );
        else
          return( ( value >= ( 1 << 13 ) ) ? 13 : 12 );
      else
        if ( value >= ( 1 << 10 ) )
          return( ( value >= ( 1 << 11 ) ) ? 11 : 10 );
        else
          return( ( value >= ( 1 << 9 ) ) ? 9 : 8 );
    else
      if ( value >= ( 1 << 4 ) )
        if ( value >= ( 1 << 6 ) )
          return( ( value >= ( 1 << 7 ) ) ? 7 : 6 );
        else
          return( ( value >= ( 1 << 5 ) ) ? 5 : 4 );
      else
        if ( value >= ( 1 << 2 ) )
          return( ( value >= ( 1 << 3 ) ) ? 3 : 2 );
        else
          return( ( value >= ( 1 << 1 ) ) ? 1 : 0 );
}
#endif

#define COUNTS_SIZE( value ) ( sizeof( COUNTTYPE ) * ( ( value + 1 + 1 + 3 ) & ~3 ) )
#define VALUES_SIZE( value ) ( sizeof( COUNTTYPE ) * ( ( value + 1 + 1 + 3 ) & ~3 ) )


RADDEFFUNC U32 Arith_compress_alloc_size( U32 max_value, U32 unique_values )
{
  return( sizeof( ARITHDATA ) + COUNTS_SIZE( unique_values ) + COUNTS_SIZE( unique_values ) + VALUES_SIZE( max_value ) );
}

RADDEFFUNC U32 Arith_decompress_alloc_size( U32 unique_values )
{
  return( sizeof( ARITHDATA ) + COUNTS_SIZE( unique_values ) + COUNTS_SIZE( unique_values ) + VALUES_SIZE( unique_values ) );
}

RADDEFFUNC U32 Arith_compress_temp_size( U32 unique_values )
{
  return( VALUES_SIZE( unique_values ) );
}

RADDEFFUNC ARITH Arith_open( void * ptr, void * compress_temp_buf, U32 max_value, U32 unique_values )
{
  ARITH a;
  U32 u;

  a = (ARITH ) ptr;

  if ( a )
  {
    // clears the buffer plus all the summed_counts
    radmemset( a, 0, ( compress_temp_buf ) ?
                    Arith_compress_alloc_size( max_value, unique_values ) :
                    Arith_decompress_alloc_size( unique_values ));

    a->single_counts = ( COUNTTYPE * ) ( ( char * ) a + ( sizeof( ARITHDATA ) + COUNTS_SIZE( unique_values ) ) );

    a->values = ( COUNTTYPE * ) ( ( char * ) a->single_counts + ( COUNTS_SIZE( unique_values ) ) );

    comp_reverse( a ) = (U16 *)compress_temp_buf;

    a->unique_count = ( COUNTTYPE ) unique_values;
    a->singles_tot = 4;
    a->summed_counts[ 0 ] = ADJ_SUMS;
    a->summed_counts[ 1 ] = NORM_COUNT + ADJ_SUMS;
    a->summed_counts[ 2 ] = NORM_COUNT + ADJ_SUMS;
    a->summed_counts[ 3 ] = NORM_COUNT + ADJ_SUMS;
    a->summed_counts[ 4 ] = NORM_COUNT + ADJ_SUMS;
    a->summed_counts[ 5 ] = NORM_COUNT + ADJ_SUMS;

    a->single_counts[ 0 ] = 4;

    a->update_tot = 8;
    a->update_range = 4;

    u = max_value * 32;

    if ( u < 256 )
      u = 256;
    else
      if ( u > 15160 )
        u = 15160;

    a->rescale_tot = (COUNTTYPE) u;

    u = max_value * 2;

    if ( u < 128 )
      u = 128;
    else
      if ( u >= ( ( (U32) a->rescale_tot >> 1 ) - 32 ) )
        u = (U32) ( ( a->rescale_tot >> 1 ) - 32 );

    a->update_max = (COUNTTYPE) u;
  }

  return( a );
}


RADDEFFUNC U32 Arith_compress_unique_values( ARITH a )
{
  if ( a->singles_length > unique_max( a ) )
  {
    unique_max( a ) = a->singles_length;
  }

  return( unique_max( a ) );
}


// These functions rescale the counts every rescale_tot symbols.  Because they are
//   called so infrequently, they contribute less than 1 cycle per symbol
//   even though they has quite a bit of work to do.

static void rescale_compress( ARITH a )
{
  unsigned long i, j;
  U32 max;
  U32 pos;

  pos = (U32)-1;

  if ( a->singles_length > unique_max( a ) )
  {
    unique_max( a ) = a->singles_length;
  }

  // rescale the escape count
  a->single_counts[ 0 ] >>= 1;

  i = 0;
  max = 0;

  a->singles_tot = a->single_counts[ 0 ];

  // first we build a temp table to quickly go from a value back to an index
  for ( j = a->singles_length ; j-- ;  )
  {
    while ( a->values[ i ] == 0 )
    {
      ++i;
    }
    
    comp_reverse( a )[ a->values[ i ] ] = ( COUNTTYPE ) i;
    ++i;
  }

  // now we scale down the all the counts, any holes we find, we fill from the back of the list
  for ( i = 1 ; i <= a->singles_length ; i++ )
  {
    // we have a hole, copy from the end of the list. it's a loop, because the end of the list
    //   might be a hole too.

    while ( a->single_counts[ i ] <= 1 )
    {
      if ( i < a->singles_length )
      {
        a->single_counts[ i ] = a->single_counts[ a->singles_length ];
        a->single_counts[ a->singles_length ] = 0;

        radassert( a->values[ comp_reverse( a )[ i ] ] == i );

        a->values[ comp_reverse( a )[ i ] ] = ( COUNTTYPE ) 0;
        a->values[ comp_reverse( a )[ a->singles_length ] ] = ( COUNTTYPE ) i;

        comp_reverse( a )[ i ] = comp_reverse( a )[ a->singles_length ];

        --a->singles_length;
      }
      else
      {
        a->single_counts[ i ] = 0;
        a->values[ comp_reverse( a )[ i ] ] = ( COUNTTYPE ) 0;
        --a->singles_length;
        goto done;
      }
    }

    a->single_counts[ i ] >>= 1;

    a->singles_tot = (COUNTTYPE) ( a->singles_tot + a->single_counts[ i ] );

    if ( a->single_counts[ i ] > max )
    {
      max = a->single_counts[ i ];
      pos = i;
    }

  }

done:

  // place the new greatest value into the final bin position (the final bin gets the rounded off code space, so this helps compression slightly)
  if ( ( max ) && ( a->singles_length ) )
  {
    j = a->singles_length;

    if ( pos != j )
    {
      i = a->single_counts[ j ];
      a->single_counts[ j ] = a->single_counts[ pos ];
      a->single_counts[ pos ] = (COUNTTYPE) i;

      i = a->values[ comp_reverse( a )[ j ] ];
      a->values[ comp_reverse( a )[ j ] ] = a->values[ comp_reverse( a )[ pos ] ];
      a->values[ comp_reverse( a )[ pos ] ] = (COUNTTYPE) i;
    }
  }

  // mark temp as touched
  comp_reverse( a )[ 0 ] = 0;

  // if the tree now has unused symbols, then make sure the escape code has a count
  if ( ( a->singles_length != a->unique_count ) && ( a->single_counts[ 0 ] == 0 ) )
  {
    ++a->single_counts[ 0 ];
    ++a->singles_tot;
  }
}


static void rescale_decompress( ARITH a )
{
  unsigned long i;

  U32 max;
  U32 pos;

  max = 0;
  pos = (U32)-1;

  // rescale the escape count
  a->single_counts[ 0 ] >>= 1;

  a->singles_tot = a->single_counts[ 0 ];

  for ( i = 1 ; i <= a->singles_length ; i++ )
  {
    while ( a->single_counts[ i ] <= 1 )
    {
      if ( i < a->singles_length )
      {
        a->single_counts[ i ] = a->single_counts[ a->singles_length ];
        a->single_counts[ a->singles_length ] = 0;
        a->values[ i ] = a->values[ a->singles_length ];
        --a->singles_length;
      }
      else
      {
        a->single_counts[ i ] = 0;
        --a->singles_length;
        goto done;
      }
    }

    a->single_counts[ i ] >>= 1;

    a->singles_tot = (COUNTTYPE) ( a->singles_tot + a->single_counts[ i ] );

    if ( a->single_counts[ i ] > max )
    {
      max = a->single_counts[ i ];
      pos = i;
    }

  }

 done:

  if ( ( max ) && ( a->singles_length ) )
  {
    U32 j;

    j = a->singles_length;

    // place the new greatest value into the final bin position
    if ( pos != j )
    {
      i = a->single_counts[ j ];
      a->single_counts[ j ] = a->single_counts[ pos ];
      a->single_counts[ pos ] = (COUNTTYPE) i;

      i = a->values[ j ];
      a->values[ j ] = a->values[ pos ];
      a->values[ pos ] = (COUNTTYPE) i;
    }
  }

  // if the tree now has unused symbols, then make sure the escape code has a count
  if ( ( a->singles_length != a->unique_count ) && ( a->single_counts[ 0 ] == 0 ) )
  {
    ++a->single_counts[ 0 ];
    ++a->singles_tot;
  }
}

// this modeller keeps two counts arrays - one is the summation of all of the counts prior to the symbol
//   (the start of the symbols range), and the second is just a simple set of counts, one for each symbol.
//   we use the summation table while we are compressing, and then occassionally sum up and copy over the 
//   single counts into the summed table.  So we keep the symbol counts dynamically, but only update 
//   modeller periodically.  This doesn't hurt compression and is much faster.

// this is the function that moves the summed_counts over from the single table over to the summed table.  We
//   normalize all of the counts to a 16384 range, so that we can use a shift instead of a divide for 
//   speed as well.

static void update_counts( ARITH a )
{
  U32 i, tot, adj;

  adj = ( NORM_COUNT * 8 ) / a->singles_tot;

  tot = ( ( ( (U32) a->single_counts[ 0 ] ) * adj ) >> 3 ) + ADJ_SUMS;
  a->summed_counts[ 0 ] = ADJ_SUMS;

  i = 1;

  for (;;)
  {
    a->summed_counts[ i ] = (COUNTTYPE) tot;
    if ( i > a->singles_length )
      break;
    tot += ( ( (U32) a->single_counts[ i ] ) * adj ) >> 3 ;
    ++i;
  }

  tot = a->update_range << 1;
  if ( tot > a->update_max )
  {
    a->update_tot = a->singles_tot + a->update_max;
  }
  else
  {
    a->update_range = (COUNTTYPE) tot;
    a->update_tot = (COUNTTYPE) ( a->singles_tot + tot );
  }

  a->summed_length = a->singles_length;

  a->summed_counts[ a->summed_length + 1 ] = NORM_COUNT + ADJ_SUMS;
}


RADDEFFUNC U32 Arith_compress( ARITH a, ARITHBITS* ab, U32 value )
{
  S32 index;

  if ( a->singles_tot >= a->update_tot )
  {
    // do we need to rescale_tot?
    if ( a->update_tot >= a->rescale_tot )
    {
      rescale_compress( a );
    }
    update_counts( a );
  }

  index = a->values[ value ];
  if ( index > a->summed_length )
  {
    index = 0;
  }

  radassert( a->summed_counts[ index + 1 ] - a->summed_counts[ index ] );
  ArithBitsPutBits( ab, a->summed_counts[ index ] - ADJ_SUMS, a->summed_counts[ index + 1 ] - a->summed_counts[ index ], NORM_BITS, NORM_COUNT );

  ++a->single_counts[ index ];
  ++a->singles_tot;

  // was it an escape character?
  if ( index == 0 )
  {
    U32 new_index;

    new_index = a->values[ value ];

    // when we get the escape symbol, we either have a completely new symbol, or we have a symbol
    //   that isn't in the summed counts table, but is in the single counts table.  if it's
    //   completely new, then we let the caller submit the symbol.  if it's in the singles counts
    //   table, then we send the offset from the end of the summed table to the single symbol.
    //   to tell the decoder which kind of new symbol it is, we send a single bit.

    if ( new_index == 0 )
    {
      radassert( a->singles_length < a->unique_count );

      // we don't have to send the bit, if there is no extra symbols in the single count table.
      if ( a->singles_length != a->summed_length )
      {
        ArithBitsPutBitsValue( ab, 0, 1, 2 );
      }

      // add new position
      new_index = ++a->singles_length;

      radassert( a->values[ value ] == 0 );

      // mark new position
      a->values[ value ] = ( COUNTTYPE ) new_index;

      // have we seen all the symbols?  if so, remove the escape
      if ( a->singles_length == a->unique_count )
      {
        a->singles_tot = (COUNTTYPE) ( a->singles_tot - a->single_counts[ 0 ] );
        a->single_counts[ 0 ] = 0;
      }
      
      value = 0x20000;
    }
    else
    {
      ArithBitsPutBitsValue( ab, 1, 1, 2 );
      ArithBitsPutValue( ab, new_index - a->summed_length - 1, a->singles_length - a->summed_length );
    }

    a->single_counts[ new_index ] += 2;
    a->singles_tot += 2;
  }

  return( value );
}


// to make the binary tree search as fast as possible, we prebuild the branches to walk the tree.
//   this saves some shifting, adding, and most importantly some branching.  we always round up
//   as we build these branches, which means we might walk off the beginning or the end of the 
//   table.  we definitely don't want to have to watch for walking off, so we make sure that there
//   are some extra dummy values at the beginning and ending of the table.  if we walk off, then we
//   do the compare and walk right back on. (slimy trick: since we just need the values at the 
//   beginning of the table to be smaller than the smallest count, we adjust the counts up by 
//   ADJ_SUM, and then place variables that we know will be smaller than ADJ_SUM right before the
//   summed_counts table.  this lets us walk off the front of the table without having to waste
//   any memory).  the end of the table is still padded, though.)

static void build_walk_table( U32 * wtable, U32 v )
{
  U32 num;
  U32 * m;

  m = wtable;

  num = 0;
  ++v;
  do
  {
    v = ( v + 1 ) >> 1;
    ++num;
    *m++ = v ;
  } while ( v > 4 );
  
  num -= 1;

  // we want the table to be right justify against the end of the array (this is so we can use
  //   fixed offsets into the table no matter what table size we use - that is, the very last
  //   entry in the table, should be the very last compare - regardless if we need to do four
  //   or eight...

  if ( num )
  {
    // NOT memcpy - dest overlaps src!
    //radmemmove( wtable + NORM_BITS - 1 - num + 1, wtable + 1, num * sizeof( wtable[ 0 ] ) );
	radmemcpydb( wtable + NORM_BITS - 1 - num + 1, wtable + 1, num * sizeof( wtable[ 0 ] ) );
  }

  wtable[ 1 ] = wtable[ 0 ];
  wtable[ 0 ] = num;
}


RADDEFFUNC UINTADDR Arith_decompress( ARITH a, ARITHBITS* ab )
{
  U32 offset;
  S32 index;

  // do we need to rescale_tot?
  if ( a->singles_tot >= a->update_tot )
  {
    // do we need to rescale_tot?
    if ( a->update_tot >= a->rescale_tot )
    {
      rescale_decompress( a );
    }
 
    update_counts( a );
    
    // leave some big numbers at the end since we can step out of the table
    a->summed_counts[ a->summed_length + 2 ] = NORM_COUNT + ADJ_SUMS;
    a->summed_counts[ a->summed_length + 3 ] = NORM_COUNT + ADJ_SUMS;
    a->summed_counts[ a->summed_length + 4 ] = NORM_COUNT + ADJ_SUMS;
    a->summed_counts[ a->summed_length + 5 ] = NORM_COUNT + ADJ_SUMS;
    a->summed_counts[ a->summed_length + 6 ] = NORM_COUNT + ADJ_SUMS;

    // pre-calc the shifts to use
    build_walk_table( a->table_walks, a->summed_length );
  }

  // get the general offset of the symbol
  offset = ArithBitsGetBits( ab, NORM_BITS, NORM_COUNT ) + ADJ_SUMS;

  // take the offset and find the symbol, then get the exact offset and size of the symbol

  // this is the slow version for testing
  /*
  {
    S32 l, h;

    l = 0;
    h = a->summed_length + 1;

    do
    {
      S32 m;
      m = ( l + h + 1 ) >> 1;

      if ( a->summed_counts[ m ] > offset )
      {
        h = m - 1;
      }
      else
      {
        l = m;
      }
    } while ( l < h );

    index = l;
  }
*/

  // this is the fast version (90 cycles per symbol faster than the loop)
  {
    index = a->table_walks[ 1 ];

    #define check_level( lev ) \
      if ( a->summed_counts[ index ] > (COUNTTYPE) offset ) \
        index -= a->table_walks[ NORM_BITS - 1 - lev ]; \
      else \
        index += a->table_walks[ NORM_BITS - 1 - lev ]; 

    // just jump in and do that many compares directly (we fall through to the rest of the compares)
    switch ( a->table_walks[ 0 ] ) 
    { 
      case 11: check_level( 10 )
      case 10: check_level( 9 )
      case 9:  check_level( 8 )
      case 8:  check_level( 7 )
      case 7:  check_level( 6 )
      case 6:  check_level( 5 )
      case 5:  check_level( 4 )
      case 4:  check_level( 3 )
      case 3:  check_level( 2 )
      case 2:  check_level( 1 )
      case 1:  check_level( 0 )
      case 0:  break;
      default: radassertfail( );
    }

    // since we always have to do the final two checks (level 2 and level 1), just do them 
    //   explictly.

    if ( a->summed_counts[ index ] > (COUNTTYPE) offset )
      index -= 2;
    else
      index += 2;

    // this does the level 1 compare, and then adjusts the final index possibly one to the right
    //   (since we aren't finding a symbol, we are finding the first symbol with a offset 
    //    *smaller than* the symbol to the right)

    if ( a->summed_counts[ index ] > (COUNTTYPE) offset ) 
      if ( a->summed_counts[ index - 1 ] > (COUNTTYPE) offset )
        index -= 2;
      else
        --index;
    else
      if ( a->summed_counts[ index + 1 ] <= (COUNTTYPE) offset )
        ++index;
  }

  // remove the symbol we just found
  ArithBitsRemove( ab, a->summed_counts[ index ] - ADJ_SUMS, a->summed_counts[ index + 1 ] - a->summed_counts[ index ], NORM_COUNT );

  ++a->single_counts[ index ];
  ++a->singles_tot;

  // was it an escape character
  if ( index <= 0 )
  {
    if ( a->singles_length == a->summed_length )
    {
     new_index:
      radassert( a->singles_length < a->unique_count );
  
      index = ++a->singles_length;
      a->single_counts[ index ] += 2;
      a->singles_tot += 2;

      // have we seen all the symbols?  if so, remove the escape
      if ( a->singles_length == a->unique_count )
      {
        a->singles_tot = (COUNTTYPE) ( a->singles_tot - a->single_counts[ 0 ] );
        a->single_counts[ 0 ] = 0;
      }

      return( ( UINTADDR ) &a->values[ index ] );
    }
    else
    {
      // is this a previously sent index?
      if ( ArithBitsGetBitsValue( ab, 1, 2 ) )
      {
        index = ArithBitsGetValue( ab, a->singles_length - a->summed_length ) + a->summed_length + 1;
        a->single_counts[ index ] += 2;
        a->singles_tot += 2;
      }
      else
      {
        goto new_index;
      }
    }
  }

  return( a->values[ index ] );
}


RADDEFFUNC void Arith_adjust_probability( ARITH a, U32 value, S32 count )
{
  U32 pos;
  
  pos = a->values[ value ];

  // we always adjust the singles probablities rather than the summed probabilities

  if ( pos == 0 )
  {
    ++a->single_counts[ 0 ];
    ++a->singles_tot;
    
    pos = ++a->singles_length;
    a->values[ value ] = (COUNTTYPE) pos;
    a->single_counts[ pos ] = (COUNTTYPE) ( a->single_counts[ pos ] + count );
    a->singles_tot = (COUNTTYPE) ( a->singles_tot + count );
  }
  else
  {
    a->single_counts[ pos ] = (COUNTTYPE) ( a->single_counts[ pos ] + count );
    a->singles_tot = (COUNTTYPE) ( a->singles_tot + count );
    if ( a->single_counts[ pos ] == 0 )
    {
      --a->single_counts[ 0 ];
      --a->singles_tot;
    
      a->values[ value ] = 0;
      --a->singles_length;
    }
  }
}

RADDEFFUNC RADIPROB Arith_inv_probability( ARITH a, U32 value, U32 scale )
{
  F32 ret;
  U32 pos;

  pos = a->values[ value ];

  if ( pos == 0 ) 
  {
   new_symb:
    ret = ( (F32) scale ) * ( ( (F32) a->singles_tot ) / (F32) ( a->single_counts[ 0 ] ) );
  }
  else
  {
    U32 symb_range;

    symb_range = a->single_counts[ pos ];
    if ( symb_range == 0 )
      goto new_symb;

    ret = ( (F32) a->singles_tot ) / (F32) symb_range;
  }

  #ifdef TESTPROBUSAGE
    return( *(RADIPROB*) &ret );
  #else
    return( ret );
  #endif
}

RADDEFFUNC F32 Arith_inv_probability_to_bits( RADIPROB iprob )
{
  #ifdef TESTPROBUSAGE
    return( (F32) radlog2( *(F32*)&iprob ) );
  #else
    return( (F32) radlog2( iprob ) );
  #endif
}

#ifdef TESTPROBUSAGE
RADDEFFUNC RADIPROB Arith_combine_probabilities( RADIPROB iprob1, RADIPROB iprob2 )
{
  F32 ret;

  ret = ( *(F32*)&iprob1 ) * ( *(F32*)&iprob2 );
  return( *(RADIPROB*) &ret );
}
#endif

#endif
