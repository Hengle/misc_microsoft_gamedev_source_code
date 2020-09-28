#include "radarith.h"
#include <raddebug.h>
#include <radmath.h>

// adaptive arithmetic modeller (exact counts kept with a binary tree)

#define COUNTTYPE U16
#define SCOUNTTYPE S16

typedef struct ARITHDATA* ARITH;

typedef struct ARITHDATA
{
  U32 tree_tot_count;               // 0
  U32 rescale;                      // 4
  U32 tree_length;                  // 8
  U32 tree_top;                     // 12
  U32 tree_depth;                   // 16
  U32 max_unique;                   // 20
  U32 unique_symb;                  // 24

  COUNTTYPE * values;               // 28
  COUNTTYPE * comp_reverse;         // 32
  
  COUNTTYPE counts[ 2 ];            // 36
} ARITHDATA;

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

static int get_binary_table_size( int values )
{
  int l, n;

  l = 1 << get_highest_bit_shift( values );
  n = l >> 1;

  while ( n ) 
  {
    l += n;
    if ( values < l )
      break;
    n >>= 1;
  } 

  return( l - 1 );
}
                                                           
#define COUNTS_SIZE( value ) ( sizeof( COUNTTYPE ) * ( ( get_binary_table_size ( value + 1 ) + 3 ) & ~3 ) )
#define VALUES_SIZE( value ) ( sizeof( COUNTTYPE ) * ( ( value + 1 + 1 + 3 ) & ~3 ) )


RADDEFFUNC U32 Arith_compress_alloc_size( U32 max_value, U32 unique_values )
{
  return( sizeof( ARITHDATA ) + COUNTS_SIZE( unique_values ) + VALUES_SIZE( max_value ) );
}

RADDEFFUNC U32 Arith_decompress_alloc_size( U32 unique_values )
{
  return( sizeof( ARITHDATA ) + COUNTS_SIZE( unique_values ) + VALUES_SIZE( unique_values ) );
}

RADDEFFUNC U32 Arith_compress_temp_size( U32 unique_values )
{
  return( VALUES_SIZE( unique_values ) );
}

RADDEFFUNC ARITH Arith_open( void * ptr, void * compress_temp_buf, U32 max_value, U32 unique_values )
{
  ARITH a;

  a = (ARITH ) ptr;

  if ( a )
  {
    // clears the buffer plus all the counts
    memset( a, 0, ( compress_temp_buf ) ?
                    Arith_compress_alloc_size( max_value, unique_values ) :
                    Arith_decompress_alloc_size( unique_values ));

    a->values = ( COUNTTYPE * ) ( ( char * ) a + ( sizeof( ARITHDATA ) + COUNTS_SIZE( unique_values ) ) );

    a->comp_reverse = compress_temp_buf;

    a->unique_symb = ( COUNTTYPE ) unique_values;
    a->tree_tot_count = 3;

    a->rescale = max_value * 32;
    if ( a->rescale < 128 )
      a->rescale = 128;
    else
      if ( sizeof( COUNTTYPE ) == 2 )
        if ( a->rescale > ( 65536 - 128 ) )
          a->rescale = ( 65536 - 128 );
  }

  return( a );
}


RADDEFFUNC U32 Arith_compress_unique_values( ARITH a )
{
  if ( a->tree_length > a->max_unique )
  {
    a->max_unique = a->tree_length;
  }

  return( a->max_unique );
}


static void increment_tree_counts ( ARITH a, U32 value, S32 amount )
{
  U32 current = a->tree_top;
  U32 highest_bit = a->tree_top + 1;

  a->tree_tot_count += amount;

  do
  {
    highest_bit >>= 1;
    if ( value < current )
    {
      a->counts[ current ] += (SCOUNTTYPE) amount;
      current -= highest_bit;
    }
    else
    {
      current += highest_bit;
    }
  } while ( highest_bit );
}

static U32 tree_to_linear( COUNTTYPE * current, COUNTTYPE * end, U32 level, U32 size )
{
  U32 ret;

  if ( level > 1 )
  {
    ret = tree_to_linear( current - level, end, level >> 1, current[ 0 ] );
    if ( current < end )
      current[ 0 ] = (COUNTTYPE) tree_to_linear( current + level, end, level >> 1, size - current[ 0 ] );
    else
      current[ 0 ] = 0;
  }
  else
  {
    if ( level < 1 )
    {
      current[ 0 ] = (COUNTTYPE) size;
    }
    else
    {
      U32 temp;

      ret = current[ -1 ];
      current[ -1 ] = current[ 0 ] - current[ -1 ];
      temp = size - current[ 0 ] - current[ 1 ];
      current[ 0 ] = current[ 1 ];
      current[ 1 ] = (COUNTTYPE) temp;
    }
  }
                            
  return( ret );            
}

static U32 linear_to_tree( COUNTTYPE * current, COUNTTYPE * end, U32 level, U32 count )
{
  U32 ret;

  if ( level > 1 )
  {
    if ( current < end )
      ret = linear_to_tree( current + level, end, level >> 1, current[ 0 ] );
    else
      ret = 0;
    current[ 0 ] = (COUNTTYPE) linear_to_tree( current - level, end, level >> 1, count );
    return( current[ 0 ] + ret );
  }
  else
  {
    if ( level < 1 )
    {
      ret = current[ 0 ];
      current[ 0 ] = 0;
    }
    else
    {
      ret = current[ -1 ] + current[ 0 ] + current[ 1 ] + count;
      current[ 1 ] = current[ 0 ];
      current[ 0 ] = (COUNTTYPE) ( current[ -1 ] + count );
      current[ -1 ] = (COUNTTYPE) count;
    }
  }

  return( ret );
}

// These functions rescale the counts every a->rescale symbols.  Because they are
//   called so infrequently, they contribute less than 1 cycle per symbol
//   even though they has quite a bit of work to do.

static void rescale_compress( ARITH a )
{
  unsigned long i, j;
  U32 max;
  U32 pos;

  tree_to_linear( a->counts + a->tree_top, a->counts + a->unique_symb + 1, ( a->tree_top + 1 ) >> 1, a->tree_tot_count );

  if ( a->tree_length > a->max_unique )
  {
    a->max_unique = a->tree_length;
  }

  // rescale the escape count
  a->counts[ 0 ] >>= 1;

  i = 0;
  max = 0;

  for ( j = a->tree_length ; j-- ;  )
  {
    while ( a->values[ i ] == 0 )
    {
      ++i;
    }
    
    a->comp_reverse[ a->values[ i ] ] = ( COUNTTYPE ) i;
    ++i;
  }

  for ( i = 1 ; i <= a->tree_length ; i++ )
  {
    while ( a->counts[ i ] <= 1 )
    {
      if ( i < a->tree_length )
      {
        a->counts[ i ] = a->counts[ a->tree_length ];
        a->counts[ a->tree_length ] = 0;

        radassert( a->values[ a->comp_reverse[ i ] ] == i );

        a->values[ a->comp_reverse[ i ] ] = ( COUNTTYPE ) 0;
        a->values[ a->comp_reverse[ a->tree_length ] ] = ( COUNTTYPE ) i;

        a->comp_reverse[ i ] = a->comp_reverse[ a->tree_length ];

        --a->tree_length;
      }
      else
      {
        a->counts[ i ] = 0;
        a->values[ a->comp_reverse[ i ] ] = ( COUNTTYPE ) 0;
        --a->tree_length;
        goto done;
      }
    }

    a->counts[ i ] >>= 1;

    if ( a->counts[ i ] > max )
    {
      max = a->counts[ i ];
      pos = i;
    }

  }

done:

  a->tree_depth = get_highest_bit_shift( a->tree_length + 1 );
  a->tree_top = ( 1 << a->tree_depth ) - 1;

  // place the new greatest value into the final bin position (minimize total increments)
  if ( ( max ) && ( a->tree_top > 1 ) )
  {
    j = a->tree_top - 1;
    if ( j == 0 )
      ++j;

    if ( pos != j )
    {
      i = a->counts[ j ];
      a->counts[ j ] = a->counts[ pos ];
      a->counts[ pos ] = (COUNTTYPE) i;

      i = a->values[ a->comp_reverse[ j ] ];
      a->values[ a->comp_reverse[ j ] ] = a->values[ a->comp_reverse[ pos ] ];
      a->values[ a->comp_reverse[ pos ] ] = (COUNTTYPE) i;
    }
  }

  // mark temp as touched
  a->comp_reverse[ 0 ] = 0;

  // if the tree now has unused symbols, then make sure the escape code has a count
  if ( ( a->tree_length != a->unique_symb ) && ( a->counts[ 0 ] == 0 ) )
  {
    a->counts[ 0 ] += 2;
  }

  a->tree_tot_count = linear_to_tree( a->counts + a->tree_top, a->counts + a->unique_symb + 1, ( a->tree_top + 1 ) >> 1, 0 );
}


static void rescale_decompress( ARITH a )
{
  unsigned long i;

  U32 max;
  U32 pos;

  tree_to_linear( a->counts + a->tree_top, a->counts + a->unique_symb + 1, ( a->tree_top + 1 ) >> 1, a->tree_tot_count );

  max = 0;

  // rescale the escape count
  a->counts[ 0 ] >>= 1;

  for ( i = 1 ; i <= a->tree_length ; i++ )
  {
    while ( a->counts[ i ] <= 1 )
    {
      if ( i < a->tree_length )
      {
        a->counts[ i ] = a->counts[ a->tree_length ];
        a->counts[ a->tree_length ] = 0;
        a->values[ i ] = a->values[ a->tree_length ];
        --a->tree_length;
      }
      else
      {
        a->counts[ i ] = 0;
        --a->tree_length;
        goto done;
      }
    }

    a->counts[ i ] >>= 1;

    if ( a->counts[ i ] > max )
    {
      max = a->counts[ i ];
      pos = i;
    }

  }

 done:

  a->tree_depth = get_highest_bit_shift( a->tree_length + 1 );
  a->tree_top = ( 1 << a->tree_depth ) - 1;

  if ( ( max ) && ( a->tree_top > 1 ) )
  {
    U32 j;

    j = a->tree_top - 1;
    if ( j == 0 )
      ++j;

    // place the new greatest value into the final bin position
    if ( pos != j )
    {
      i = a->counts[ j ];
      a->counts[ j ] = a->counts[ pos ];
      a->counts[ pos ] = (COUNTTYPE) i;

      i = a->values[ j ];
      a->values[ j ] = a->values[ pos ];
      a->values[ pos ] = (COUNTTYPE) i;
    }
  }

  // if the tree now has unused symbols, then make sure the escape code has a count
  if ( ( a->tree_length != a->unique_symb ) && ( a->counts[ 0 ] == 0 ) )
  {
    a->counts[ 0 ] += 2;
  }

  a->tree_tot_count = linear_to_tree( a->counts + a->tree_top, a->counts + a->unique_symb + 1, ( a->tree_top + 1 ) >> 1, 0 );
}


#define do_unrolled_depths \
{ \
  switch ( a->tree_depth ) \
  { \
    case 16: \
      check_level( 32768 ); \
      check_level( 16384 ); check_level( 8192 ); check_level( 4096 ); check_level( 2048 ); \
      check_level( 1024 ); check_level( 512 ); check_level( 256 ); check_level( 128 ); \
      check_level( 64 ); check_level( 32 ); check_level( 16 ); check_level( 8 ); \
      check_level( 4 ); check_level( 2 ); check_level( 1 ); check_level( 0 ); \
      break; \
    case 15: \
      check_level( 16384 ); check_level( 8192 ); check_level( 4096 ); check_level( 2048 ); \
      check_level( 1024 ); check_level( 512 ); check_level( 256 ); check_level( 128 ); \
      check_level( 64 ); check_level( 32 ); check_level( 16 ); check_level( 8 ); \
      check_level( 4 ); check_level( 2 ); check_level( 1 ); check_level( 0 ); \
      break; \
    case 14: \
      check_level( 8192 ); check_level( 4096 ); check_level( 2048 ); \
      check_level( 1024 ); check_level( 512 ); check_level( 256 ); check_level( 128 ); \
      check_level( 64 ); check_level( 32 ); check_level( 16 ); check_level( 8 ); \
      check_level( 4 ); check_level( 2 ); check_level( 1 ); check_level( 0 ); \
      break; \
    case 13: \
      check_level( 4096 ); check_level( 2048 ); \
      check_level( 1024 ); check_level( 512 ); check_level( 256 ); check_level( 128 ); \
      check_level( 64 ); check_level( 32 ); check_level( 16 ); check_level( 8 ); \
      check_level( 4 ); check_level( 2 ); check_level( 1 ); check_level( 0 ); \
      break; \
    case 12: \
      check_level( 2048 ); \
      check_level( 1024 ); check_level( 512 ); check_level( 256 ); check_level( 128 ); \
      check_level( 64 ); check_level( 32 ); check_level( 16 ); check_level( 8 ); \
      check_level( 4 ); check_level( 2 ); check_level( 1 ); check_level( 0 ); \
      break; \
    case 11: \
      check_level( 1024 ); check_level( 512 ); check_level( 256 ); check_level( 128 ); \
      check_level( 64 ); check_level( 32 ); check_level( 16 ); check_level( 8 ); \
      check_level( 4 ); check_level( 2 ); check_level( 1 ); check_level( 0 ); \
      break; \
    case 10: \
      check_level( 512 ); check_level( 256 ); check_level( 128 ); \
      check_level( 64 ); check_level( 32 ); check_level( 16 ); check_level( 8 ); \
      check_level( 4 ); check_level( 2 ); check_level( 1 ); check_level( 0 ); \
      break; \
    case 9: \
      check_level( 256 ); check_level( 128 ); \
      check_level( 64 ); check_level( 32 ); check_level( 16 ); check_level( 8 ); \
      check_level( 4 ); check_level( 2 ); check_level( 1 ); check_level( 0 ); \
      break; \
    case 8: \
      check_level( 128 ); \
      check_level( 64 ); check_level( 32 ); check_level( 16 ); check_level( 8 ); \
      check_level( 4 ); check_level( 2 ); check_level( 1 ); check_level( 0 ); \
      break; \
    case 7: \
      check_level( 64 ); check_level( 32 ); check_level( 16 ); check_level( 8 ); \
      check_level( 4 ); check_level( 2 ); check_level( 1 ); check_level( 0 ); \
      break; \
    case 6: \
      check_level( 32 ); check_level( 16 ); check_level( 8 ); \
      check_level( 4 ); check_level( 2 ); check_level( 1 ); check_level( 0 ); \
      break; \
    case 5: \
      check_level( 16 ); check_level( 8 ); \
      check_level( 4 ); check_level( 2 ); check_level( 1 ); check_level( 0 ); \
      break; \
    case 4: \
      check_level( 8 ); \
      check_level( 4 ); check_level( 2 ); check_level( 1 ); check_level( 0 ); \
      break; \
    case 3: \
      check_level( 4 ); check_level( 2 ); check_level( 1 ); check_level( 0 ); \
      break; \
    case 2: \
      check_level( 2 ); check_level( 1 ); check_level( 0 ); \
      break; \
    case 1: \
      check_level( 1 ); check_level( 0 ); \
      break; \
    case 0: \
      check_level( 0 ); \
      break; \
    default: \
      radassert( a->tree_depth <=16 ); \
      break; \
  } \
}

RADDEFFUNC U32 Arith_compress( ARITH a, ARITHBITS* ab, U32 value )
{
  U32 index;
  U32 symb_offset, symb_range;

  // do we need to rescale?
  if ( a->tree_tot_count >= a->rescale )
  {
    rescale_compress( a );
  }

  index = a->values[ value ];

  // take index and figure out the start position and the range
  {
    U32 current = a->tree_top;
    U32 cur_count;
    symb_range = a->tree_tot_count;
    symb_offset = 0;

    #define check_level( level ) \
    { \
      cur_count = a->counts[ current ]; \
      if ( index < current ) \
      { \
        symb_range = cur_count; \
        a->counts[ current ] = (COUNTTYPE) ( cur_count + 1 ); \
        current -= level; \
      } \
      else \
      { \
        symb_range -= cur_count; \
        symb_offset += cur_count; \
        current += level; \
      } \
    }

    do_unrolled_depths
    
    #undef check_level
  
  }

  radassert( ( symb_range > 0 ) && ( ( symb_offset + symb_range ) <= a->tree_tot_count ) ) ;

  ArithBitsPut( ab, symb_offset, symb_range, a->tree_tot_count );

  ++a->tree_tot_count;

  // was it an escape character?
  if ( index == 0 )
  {
    U32 next_top;

    radassert( a->tree_length < a->unique_symb );

    // add new position
    ++a->tree_length;

    radassert( a->values[ value ] == 0 );

    // mark new position
    a->values[ value ] = ( COUNTTYPE ) a->tree_length;

    next_top = a->tree_top + a->tree_top + 1;

    if ( a->tree_length >= next_top )
    {
      ++a->tree_depth;
      a->counts[ next_top ] = (COUNTTYPE) a->tree_tot_count;
      a->tree_top = next_top;
    }
    
    // increment counts
    increment_tree_counts( a, a->tree_length, 2 );

    // have we seen all the symbols?  if so, remove the escape
    if ( a->tree_length == a->unique_symb )
    {
      increment_tree_counts( a, 0, -(S32)(U32)a->counts[ 1 ] );
    }

    return( 0x20000 );
  }

  return( value );
}


RADDEFFUNC U32 Arith_decompress( ARITH a, ARITHBITS* ab )
{
  U32 offset, tree_index, symbol_contains;

  // do we need to rescale?
  if ( a->tree_tot_count >= a->rescale )
  {
    rescale_decompress( a );
  }

  // get the general offset of the symbol
  offset = ArithBitsGet( ab, a->tree_tot_count );

  // take the offset and find the symbol, then get the exact offset and size of the symbol
  {
    COUNTTYPE * current;
    S32 amt_remaining;
    S32 cur_count;

    current = a->counts + a->tree_top;
    amt_remaining = offset;
    symbol_contains = a->tree_tot_count;

    #define check_level( level ) \
    { \
      cur_count = *current; \
      if ( amt_remaining < cur_count ) \
      { \
        symbol_contains = cur_count; \
        *current = (COUNTTYPE) ( cur_count + 1 ); \
        current -= level; \
      } \
      else \
      { \
        tree_index = (U32) current; \
        symbol_contains -= cur_count; \
        amt_remaining -= cur_count; \
        current += level; \
      } \
    }

    do_unrolled_depths

    #undef check_levels

    offset -= amt_remaining;
  }

  tree_index = ( tree_index - (U32) a->counts ) >> ( sizeof( COUNTTYPE ) / 2 );

  // remove the symbol we just found
  ArithBitsRemove( ab, offset, symbol_contains, a->tree_tot_count );

  ++a->tree_tot_count;

  // was it an escape character
  if ( tree_index == 0 )
  {
    U32 next_top;

    radassert( a->tree_length < a->unique_symb );

    // add new position
    ++a->tree_length;

    next_top = a->tree_top + a->tree_top + 1;

    if ( a->tree_length >= next_top )
    {
      ++a->tree_depth;
      a->counts[ next_top ] = (COUNTTYPE) a->tree_tot_count;
      a->tree_top = next_top;
    }
    
    // increment counts
    increment_tree_counts( a, a->tree_length, 2 );

    // have we seen all the symbols?  if so, remove the escape
    if ( a->tree_length == a->unique_symb )
    {
      increment_tree_counts( a, 0, -(S32)(U32)a->counts[ 1 ] );
    }

    return( ( U32 ) &a->values[ a->tree_length ] );
  }

  return( a->values[ tree_index ] );
}


RADDEFFUNC void Arith_adjust_probability( ARITH a, U32 value, S32 count )
{
  U32 pos;
  
  pos = a->values[ value ];

  increment_tree_counts( a, pos, count );
}

static U32 get_symb_range( ARITH a, U32 index )
{
  U32 current = a->tree_top;
  U32 highest_bit = a->tree_top + 1;
  U32 symb_range;

  symb_range = a->tree_tot_count;

  do
  {
    U32 cur_count;

    cur_count = a->counts[ current ];
    highest_bit >>= 1;

    if ( index < current )
    {
      symb_range = cur_count;
      current -= highest_bit;
    }
    else
    {
      symb_range -= cur_count;
      current += highest_bit;
    }
  } while ( highest_bit );

  return( symb_range );
}

RADDEFFUNC RADIPROB Arith_inv_probability( ARITH a, U32 value, U32 scale )
{
  F32 ret;
  U32 pos;

  pos = a->values[ value ];

  if ( pos == 0 ) 
  {
   new_symb:
    ret = ( (F32) scale ) * ( ( (F32) a->tree_tot_count ) / (F32) get_symb_range( a, 0 ) );
  }
  else
  {
    U32 symb_range;

    symb_range = get_symb_range( a, pos );
    if ( pos == 0 )
      goto new_symb;

    ret = ( (F32) a->tree_tot_count ) / (F32) symb_range;
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

