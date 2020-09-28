#include "radbase.h"
#include "raddebug.h"
#include "radlz.h"
#include "radarith.h"
#include <string.h>

#define MAX_LENS 64

// split the offsets into three tables, so that the low bits (which are mostly
//   noise don't interfere with the compression of the other bits), and the middle
//   bits can be predicted from the top bits (whew)

#define LMAX ( 1 << LOW_BITS )
#define MMAX ( 1 << MED_BITS )
#define HMAX ( 1 << TOP_BITS )

#define GETL( val ) ( ( val ) & ( LMAX - 1 ) )
#define GETM( val ) ( ( ( val ) >> LOW_BITS ) & ( MMAX - 1 ) )
#define GETT( val ) ( ( val ) >> ( LOW_BITS + MED_BITS ) )

typedef struct QUICKFIND
{
  U8 const * addr;
  U32 run_count;
  struct QUICKFIND * next[ 3 ];
  struct QUICKFIND * prev[ 3 ];
} QUICKFIND;


#define ADDRESS_MASK 3

typedef struct LZCDATA
{
  ARITH bytes[ ADDRESS_MASK + 1 ], lens[ MAX_LENS + 1 ], offsl, offst, offsm[ 256 ];
  U32 max_bytes, uniq_bytes, max_offset, max_offs, max_offsL, max_offsM, max_offsT;
  QUICKFIND ** quick2;
  QUICKFIND ** quick3;
  QUICKFIND ** quick4;
  QUICKFIND * buffer;
  U32 buffer_pos;
  U32 bytes_compressed;
  U32 offst_used, offsm_used;
  U32 last_len;
  U32 run, last_run;
  U32 hash2_size, hash3_size, hash4_size;
  U8 const * save_pos;
  U32 save_len;
  U32 save_bit_pat;
  void const * save_match;
} LZCDATA;

static U32 long_lengths[ 4 ] = { MAX_LENS * 2, MAX_LENS * 3, MAX_LENS * 4, MAX_LENS * 8 };

#ifdef DISPLAY_HASH_STATS
  #include <stdio.h>
  static U32 max[3]={0,0};
  static F32 tot[3]={0,0};
  static U32 num[3]={0,0};
#endif


static U32 calc_run( U8 const* addr, U32 len )
{
  U32 j;

  if ( len <= 1 )
    return( 0 );

  --len;

  for ( j = 0 ; j < len ; j++ )
  {
    if ( addr[ j ] != addr[ j + 1 ] )
      break;
  }

  return( j );
}

static void scan_hashes( LZC lz, QUICKFIND * start, U32 which, U8 const* cur, U32 cur_len, U32 stop_len, U32* long_len, void const ** match, U32 * bit_pat, F32 * best_comp )
{
  F32 compb;
  RADIPROB comp;
  QUICKFIND * quick = start;
  
  #ifdef DISPLAY_HASH_STATS
    U32 c=0;
  #endif

  do
  {
    U32 len, b;
    U32 off;

    #ifdef DISPLAY_HASH_STATS
    c++;
    #endif

    // see we've hit the stop_len and we're further away that our current match
    //   if so, then we ain't going to find a closer match.  There are occassionally
    //   farther away matchs that compress better, but it's so rare that the speed
    //   savings this check gives us is worth it.
    if ( ( stop_len == *long_len ) && ( ( quick->addr + quick->run_count ) <= ( U8* ) *match ) )
      break;

    if ( quick->addr[ 0 ] == cur[ 0 ] )
    {
      U8 const * cmp1;
      U8 const * cmp2;
      U8 const * start;

      start = quick->addr;

      if ( quick->run_count >= lz->run )
      {
        cmp1 = start + quick->run_count + 1;
        cmp2 = cur + lz->run + 1;
        if ( cmp1 >= cmp2)
        {
          start = cur - 1;
          len = lz->run + 1;
        }
        else
        {
          len = lz->run + 1;
          start = cmp1 - ( lz->run + 1 );
          while ( *cmp1 == *cmp2 )
          {
            ++cmp1;
            ++cmp2;
            len = ( U32 ) ( cmp2 - cur );

            if ( len >= stop_len )
              break;
          }
        }
      }
      else
      {
        len = quick->run_count + 1;
      }

      if ( len >= *long_len )  // gets very slightly better compression if this check is taken out, but it slows down
      {
        // clip to end of buffer
        if ( len > cur_len )
          len = cur_len;

        // clip the lengths
        if ( len > ( MAX_LENS - 3 ) )
        {
          if ( len > long_lengths[ 0 ] )
          {
            if ( len >= long_lengths[ 1 ] )
            {
              if ( len >= long_lengths[ 2 ] )
              {
                if ( len >= long_lengths[ 3 ] )
                {
                  len = long_lengths[ 3 ];
                  b = MAX_LENS - 1;
                }
                else
                {
                  len = long_lengths[ 2 ];
                  b = MAX_LENS - 2;
                }
              }
              else
              {
                len = long_lengths[ 1 ];
                b = MAX_LENS - 3;
              }
            }
            else
            {
              len = long_lengths[ 0 ];
              b = MAX_LENS - 4;
            }
          }
          else
          {
            len = MAX_LENS - 3;
            b = ( MAX_LENS - 3 ) - 2;
          }
        }
        else
        {
          b = len - 2;
        }

        // cast for 64-bit
        off = (U32)(cur - start - 1);
        comp = Arith_inv_probability( lz->lens[ lz->last_len ], b + 1, MAX_LENS + 1 );
        comp = Arith_combine_probabilities( comp, Arith_inv_probability( lz->offsl, GETL( off ), lz->max_offsL ) );
        comp = Arith_combine_probabilities( comp, Arith_inv_probability( lz->offst, GETT( off ), lz->offst_used ) );
        comp = Arith_combine_probabilities( comp, Arith_inv_probability( lz->offsm[ GETT( off ) ], GETM( off ), lz->offsm_used ) );
        compb = Arith_inv_probability_to_bits( comp ) / ( ( F32 ) len );

        if ( compb < *best_comp)
        {
          *best_comp = compb;
          *long_len = len;
          *match = start;
          *bit_pat = b;
          if ( len >= stop_len )
            break;
        }
        else
        {
          if ( compb == *best_comp )
          {
            if ( start > ( U8* ) *match )
            {
              *long_len = len;
              *match = start;
              *bit_pat = b;
              if ( len >= stop_len )
                break;
            }
          }
        }
      }
    }

    quick = quick->next[ which ];

  } while ( quick );

#ifdef DISPLAY_HASH_STATS
  {
    if (max[which]<c )
      max[which]=c;
    tot[which]+=c;
    num[which]++;
  }
#endif
}


static U32 hashw( U32 val, U32 size )
{
  while ( val >= size )
  {
    val = ( val % size ) + ( val / size );
  }

  return( val );
}

#define hash2( addr, size ) ( hashw( ( ( U16* ) ( addr ) )[ 0 ], ( size ) ) )
#define hash3( addr, size ) ( hashw( ( ( U32* ) ( addr ) )[ 0 ] & 0xffffff, ( size ) ) )
#define hash4( addr, size ) ( hashw( ( ( U32* ) ( addr ) )[ 0 ], ( size ) ) )


// find the most compressible string match
static U32 find_match( LZC l, U8 const * cur, U32 cur_len, F32 best_comp, void const ** match, U32 * bit_pat )
{
  QUICKFIND * start;
  U32 best_len = 0;

  *match = 0;

  // scan each of the hash tables

  if ( cur_len >= 3 )
  {
    start = l->quick4[ hash4( cur, l->hash4_size ) ];
    if ( start )
    {
      scan_hashes( l, start, 2, cur, cur_len, ( cur_len <= long_lengths[ 3 ] ) ? cur_len : long_lengths[ 3 ], &best_len, match, bit_pat, &best_comp );
    }
  }

  if ( ( best_len <= 3 ) && ( cur_len >= 3 ) )
  {
    start = l->quick3[ hash3( cur, l->hash3_size ) ];
    if ( start )
    {
      scan_hashes( l, start, 1, cur, cur_len, ( cur_len <= 3 ) ? cur_len : 3, &best_len, match, bit_pat, &best_comp );
    }
  }

  if ( ( best_len <= 2 ) && ( cur_len >= 2 ) )
  {
    start = l->quick2[ hash2( cur, l->hash2_size ) ];
    if ( start )
    {
      scan_hashes( l, start, 0, cur, cur_len, ( cur_len <= 2 ) ? cur_len : 2, &best_len, match, bit_pat, &best_comp );
    }
  }

  return( best_len );
}


// removes dupicate strings from the 2 and 3 length hashes (since the 3 and 4
//   hash will always find any strings longer, we can just keep uniques in
//   2 and 3).
static void remove_matching_head( QUICKFIND* q, U32 mask, U32 which )
{
  U32 value = ( ( U32* ) q->addr )[ 0 ] & mask;
  q = q->next[ which ];
  while ( q )
  {
    QUICKFIND * n = q->next[ which ];
    if ( value == ( ( ( U32* ) q->addr )[ 0 ] & mask ) )
    {
      q->prev[ which ]->next[ which ] = n;
      if ( n )
        n->prev[ which ] = q->prev[ which ];

      q->prev[ which ] = 0;
      q->next[ which ] = 0;
    }
    q = n;
  }
}


// add a string to the hash lookups
static void add_to_quick( LZC l,
                          U8 const* cur,
                          U32 cur_len )
{
  U32 hash;
  QUICKFIND* buf, * buf_next;

  buf = l->buffer + l->buffer_pos;
  l->buffer_pos = ( l->buffer_pos + 1 ) % l->max_offs;
  buf_next = l->buffer + l->buffer_pos;

  // if this buffer position is full, replace it!
  if ( buf->addr )
  {
    if ( buf->run_count < 4 )
    {
      // counts of 3 and below don't need to be propogated forward
      if ( buf->prev[ 0 ] )
      {
        // this is a tail pointer - just remove it from the other list
        buf->prev[ 0 ]->next[ 0 ] = 0;
        buf->prev[ 0 ] = 0;
      }
      else
      {
        // this is a solidary head pointer - remove it from the quick list
        U32 old_hash = hash2( buf->addr, l->hash2_size );
        radassert( buf->next[ 0 ] == 0 );
        if ( l->quick2[ old_hash ] == buf )
          l->quick2[ old_hash ] = 0;
      }

      if ( buf->prev[ 1 ] )
      {
        // this is a tail pointer - just remove it from the other list
        buf->prev[ 1 ]->next[ 1 ] = 0;
        buf->prev[ 1 ] = 0;
      }
      else
      {
        // this is a solidary head pointer - remove it from the quick list
        U32 old_hash = hash3( buf->addr, l->hash3_size );
        radassert( buf->next[ 1 ] == 0 );
        if ( l->quick3[ old_hash ] == buf )
          l->quick3[ old_hash ] = 0;
      }
      if ( buf->prev[ 2 ] )
      {
        // this is a tail pointer - just remove it from the other list
        buf->prev[ 2 ]->next[ 2 ] = 0;
        buf->prev[ 2 ] = 0;
      }
      else
      {
        // this is a solidary head pointer - remove it from the quick list
        U32 old_hash = hash4( buf->addr, l->hash4_size );
        radassert( buf->next[ 2 ] == 0 );
        if ( l->quick4[ old_hash ] == buf )
          l->quick4[ old_hash ] = 0;
      }
    }
    else
    {
      radassert( buf_next->addr == 0 );

      // propogate this run forward into the next bin
      buf_next->addr = buf->addr + 1;
      buf_next->run_count = ( buf->run_count <= 2 ) ? 0 : ( buf->run_count - 1 );

      buf_next->next[ 0 ] = 0;
      buf_next->next[ 1 ] = 0;
      buf_next->next[ 2 ] = 0;

      if ( buf->prev[ 0 ] )
      {
        // this is a tail pointer - replace with next pointer
        buf->prev[ 0 ]->next[ 0 ] = buf_next;
        buf_next->prev[ 0 ] = buf->prev[ 0 ];
        buf->prev[ 0 ] = 0;
      }
      else
      {
        // this is a solidary head pointer - replace it in the quick list
        U32 old_hash = hash2( buf->addr, l->hash2_size );
        radassert( buf->next[ 0 ] == 0 );
        if ( l->quick2[ old_hash ] == buf )
          l->quick2[ old_hash ] = buf_next;
      }

      if ( buf->prev[ 1 ] )
      {
        // this is a tail pointer - replace with next pointer
        buf->prev[ 1 ]->next[ 1 ] = buf_next;
        buf_next->prev[ 1 ] = buf->prev[ 1 ];
        buf->prev[ 1 ] = 0;
      }
      else
      {
        // this is a solidary head pointer - replace it in the quick list
        U32 old_hash = hash3( buf->addr, l->hash3_size );
        radassert( buf->next[ 1 ] == 0 );
        if ( l->quick3[ old_hash ] == buf )
          l->quick3[ old_hash ] = 0;
      }

      if ( buf->prev[ 2 ] )
      {
        // this is a tail pointer - replace with next pointer
        buf->prev[ 2 ]->next[ 2 ] = buf_next;
        buf_next->prev[ 2 ] = buf->prev[ 2 ];
        buf->prev[ 2 ] = 0;
      }
      else
      {
        // this is a solidary head pointer - replace it in the quick list
        U32 old_hash = hash4( buf->addr, l->hash4_size );
        radassert( buf->next[ 2 ] == 0 );
        if ( l->quick4[ old_hash ] == buf )
          l->quick4[ old_hash ] = 0;
      }
    }

    buf->addr = 0;
  }

  //  do we need to set this position (we don't insert it, if the run is greater than 3)
  if ( l->last_run <= 2 )
  {
    buf->addr = cur;

    hash = hash2( cur, l->hash2_size );
    buf->next[0] = l->quick2[ hash ];
    l->quick2[ hash ] = buf;

    hash = hash3( cur, l->hash3_size );
    buf->next[1] = l->quick3[ hash ];
    l->quick3[ hash ] = buf;

    hash = hash4( cur, l->hash4_size );
    buf->next[2] = l->quick4[ hash ];
    l->quick4[ hash ] = buf;

    // set the previous pointer, if there is one to be set
    if ( buf->next[0] )
      buf->next[0]->prev[0] = buf;
    if ( buf->next[1] )
      buf->next[1]->prev[1] = buf;
    if ( buf->next[2] )
      buf->next[2]->prev[2] = buf;

    remove_matching_head( buf, 0xffff, 0 );
    remove_matching_head( buf, 0xfffffff, 1 );

    buf->run_count = l->run;

    l->last_run = l->run;
  }

  // decrement the runs
  if ( l->run )
  {
    if ( l->run >= cur_len )
      --l->run;
    else
    {
      if ( cur[ l->run + 1 ] != cur[ 0 ] )
        --l->run;
    }
  }

  if ( l->last_run )
    --l->last_run;

  if ( l->run == 0 )
    l->run = calc_run( cur+1, cur_len - 1 );
}

static void get_max_offsets( U32 max_offset, U32 * low_max, U32 * med_max, U32 * high_max )
{
  *low_max = ( max_offset >= LMAX ) ? LMAX : ( max_offset + 1 );
  *med_max = ( ( max_offset >> LOW_BITS ) >= MMAX ) ? MMAX : ( ( max_offset >> LOW_BITS ) + 1 );
  *high_max = GETT( max_offset ) + 1;
}

#define HASH2ADJ 16
#define HASH3ADJ 8
#define HASH4ADJ 4

RADDEFFUNC U32 LZ_compress_alloc_size( U32 max_byte_value,
                                       U32 uniq_byte_values,
                                       U32 max_offset )
{
  U32 size, h, low_max, med_max, high_max;

  size = sizeof( LZCDATA );

  h = max_offset / HASH2ADJ;
  h = ( h <= 14 ) ? 7 : ( h - 7 );
  size += h * sizeof(void*); // for quick2

  h = max_offset / HASH3ADJ;
  h = ( h <= 36 ) ? 7 : ( h - 29 );
  size += h * sizeof(void*); // for quick3

  h = max_offset / HASH4ADJ;
  h = ( h <= 48 ) ? 7 : ( h - 41 );
  size += h * sizeof(void*); // for quick4

  size += max_offset * sizeof( QUICKFIND ); // for buffer

  h = Arith_compress_alloc_size( max_byte_value, uniq_byte_values ); // for bytes
  size += ( h * ( ADDRESS_MASK + 1 ) );

  size += Arith_compress_alloc_size( MAX_LENS, MAX_LENS + 1 ) * ( MAX_LENS + 1 ); // for lengths

  get_max_offsets( max_offset, &low_max, &med_max, &high_max );

  size += Arith_compress_alloc_size( low_max - 1, low_max ); // for low offsets

  size += ( high_max * Arith_compress_alloc_size( med_max - 1, med_max ) ); // for med offsets

  size += Arith_compress_alloc_size( high_max - 1, high_max ); // for high offsets

  return( size );
}


RADDEFFUNC U32 LZ_compress_temp_size( U32 max_byte_value,
                                      U32 uniq_byte_values,
                                      U32 max_offset )
{
  U32 size, low_max, med_max, high_max;

  RAD_UNUSED_VARIABLE(max_byte_value);
  RAD_UNUSED_VARIABLE(uniq_byte_values);

  size = MAX_LENS + 1;
  if ( ( ( U32 ) max_byte_value + 1 ) > size )
    size = ( U32 ) max_byte_value + 1;

  get_max_offsets( max_offset, &low_max, &med_max, &high_max );

  if ( low_max > size )
    size = low_max;

  if ( med_max > size )
    size = med_max;

  if ( high_max > size )
    size = high_max;

  return( Arith_compress_temp_size( size ) );
}


typedef struct LZ_HEADER
{
  U32 max_offset_and_byte;
  U32 uniq_offset_and_byte;
  U32 uniq_lens;
} LZ_HEADER;


// Returns how much data area to save for the header.
RADDEFFUNC U32 LZ_compress_header_size( void )
{
  return( sizeof( LZ_HEADER ) );
}

#include "radmem.h"
#include "radmemutils.h"


RADDEFFUNC LZC LZ_compress_open( void * ptr,
                                 void * compress_temp_buf,
                                 U32 max_byte_value,
                                 U32 uniq_byte_values,
                                 U32 max_offset )
{
  U32 size, i;
  U8 * addr;
  LZC l = ( LZC ) ptr;

  l->max_bytes = max_byte_value + 1;
  l->max_offs = max_offset;

  get_max_offsets( max_offset, &l->max_offsL, &l->max_offsM, &l->max_offsT );

  l->uniq_bytes = uniq_byte_values;
  l->save_pos = ( U8* ) -1;
  l->buffer_pos = 0;
  l->bytes_compressed = 0;
  l->last_len = 0;
  l->run = 0;
  l->last_run = 0;

  l->hash2_size = l->max_offs / HASH2ADJ;
  l->hash2_size = ( l->hash2_size <= 14 ) ? 7 : ( l->hash2_size - 7 );
  l->hash3_size = l->max_offs / HASH3ADJ;
  l->hash3_size = ( l->hash3_size <= 36 ) ? 7 : ( l->hash3_size - 29 );
  l->hash4_size = l->max_offs / HASH4ADJ;
  l->hash4_size = ( l->hash4_size <= 48 ) ? 7 : ( l->hash4_size - 41 );

  l->quick2 = ( QUICKFIND** ) ( l + 1 );
  radmemset( l->quick2, 0, l->hash2_size * sizeof(void*) );

  l->quick3 = l->quick2 + l->hash2_size;
  radmemset( l->quick3, 0, l->hash3_size * sizeof(void*) );

  l->quick4 = l->quick3 + l->hash3_size;
  radmemset( l->quick4, 0, l->hash4_size * sizeof(void*) );

  l->buffer = ( QUICKFIND* ) ( l->quick4 + l->hash4_size );
  radmemset( l->buffer, 0, l->max_offs * sizeof( QUICKFIND ) );

  addr = (U8*) ( l->buffer + l->max_offs );
  size = Arith_compress_alloc_size( max_byte_value, uniq_byte_values );

  for ( i = 0 ; i <= ADDRESS_MASK ; i++ )
  {
    l->bytes[ i ] = Arith_open( addr,
                                compress_temp_buf,
                                max_byte_value,
                                uniq_byte_values );
    addr += size;
  }

  size = Arith_compress_alloc_size( MAX_LENS, MAX_LENS + 1 );

  for ( i = 0 ; i <= MAX_LENS ; i++ )
  {
    l->lens[ i ] = Arith_open( addr,
                               compress_temp_buf,
                               MAX_LENS,
                               MAX_LENS + 1 );

    addr += size;
  }

  size = Arith_compress_alloc_size( l->max_offsM - 1, l->max_offsM );

  for ( i = 0 ; i < l->max_offsT ; i++ )
  {
    l->offsm[ i ] = Arith_open( addr,
                                compress_temp_buf,
                                l->max_offsM - 1,
                                l->max_offsM );
    addr += size;
  }

  l->offsl = Arith_open( addr,
                         compress_temp_buf,
                         l->max_offsL - 1,
                         l->max_offsL );

  addr += Arith_compress_alloc_size( l->max_offsL - 1, l->max_offsL );

  l->offst = Arith_open( addr,
                         compress_temp_buf,
                         l->max_offsT - 1,
                         l->max_offsT );


  return( l );
}


// Returns the header for this compression.
RADDEFFUNC void LZ_compress_get_header( LZC l,
                                        void * header )
{
  U32 s1, s2;
  U32 j, i;
  LZ_HEADER * h = ( LZ_HEADER * ) header;

  h->max_offset_and_byte = ( l->max_offs << 9 ) + l->max_bytes;

  s2 = 0;
  for ( i = 0 ; i <= ADDRESS_MASK ; i++ )
  {
    s1 = Arith_compress_unique_values( l->bytes[ i ] ) + 1;
    if ( s1 > s2 )
      s2 = s1;
  }

  if ( s2 > l->uniq_bytes )
    s2 = l ->uniq_bytes;

  s1 = ( Arith_compress_unique_values( l->offst ) + 1 ) << ( LOW_BITS + MED_BITS );
  if ( s1 > l->max_offs )
    s1 = l->max_offs;

  h->uniq_offset_and_byte = ( s1 << 9 ) + s2;

  for ( j = 0 ; j < 4 ; j++ )
  {
    s1 = 0;
    for ( i = 0 ; i < ( MAX_LENS / 4 ) ; i++ )
    {
      U32 u = Arith_compress_unique_values( l->lens[ ( j * ( MAX_LENS / 4 ) ) + i ] ) + 1;
      if ( u > s1 )
        s1 = u;
    }
    if ( s1 > ( MAX_LENS + 1 ) )
      s1 = MAX_LENS + 1;
    s2 = ( s2 << 8 ) + s1;
  }

  for ( j = ( ( MAX_LENS / 4 ) * 4 ) ; j <= MAX_LENS ; j++ )
  {
    U32 u = Arith_compress_unique_values( l->lens[ j ] ) + 1;
    if ( u > s1 )
      s1 = u;
  }
  if ( s1 > ( MAX_LENS + 1 ) )
    s1 = MAX_LENS + 1;
  s2 = ( s2 & 0xffffff00 ) + s1;

  h->uniq_lens = s2;
}

/*
#include <stdio.h>
static void append_byte(char const * name,U32 b )
{
  FILE *f;

  f = fopen(name, "ab+");
  fwrite(&b,1,1,f);
  fclose(f);
}
*/

RADDEFFUNC U32 LZ_compress( LZC l, ARITHBITS* ab, U8 const * input, U32 input_len )
{
  if ( input_len < 1 )
  {
    if ( input_len == 0 )
    {
      return( 0 );
    }
    // just one byte left, so go dump it
    goto literal;
  }
  else
  {
    void const * match;
    U32 len, bit_pat;
    RADIPROB first_char_lit_size, lit_len_size;

    lit_len_size = Arith_inv_probability( l->lens[ l->last_len ], 0, MAX_LENS + 1 );
    first_char_lit_size = Arith_combine_probabilities( lit_len_size , Arith_inv_probability( l->bytes[ ( (UINTADDR) input ) & ADDRESS_MASK ], input[ 0 ], l->max_bytes ) );

    if ( input == l->save_pos )
    {
      len = l->save_len;
      match = l->save_match;
      bit_pat = l->save_bit_pat;
    }
    else
    {
      if ( l->bytes_compressed == 0 )
        l->run = calc_run( input, input_len );

      len = find_match( l,
                        input,
                        input_len,
                        Arith_inv_probability_to_bits( first_char_lit_size ),
                        &match,
                        &bit_pat );
    }

    // add the new string to the lookup
    add_to_quick( l, input, input_len );

    if ( len >= 2 )
    {
      U32 off, j;

      // cast for 64-bit
      off = (U32)(( input - ( U8* ) match ) - 1);

      // if this is a short length, check against literal size and better encoding
      if ( len <= 5 )
      {
        RADIPROB run_size, lits_size;
        U32 t;

        // step 1
        // make sure this wouldn't be beaten by a literal

        // calculate the run size
        run_size = Arith_inv_probability( l->lens[ l->last_len ], bit_pat + 1, MAX_LENS + 1 );

        run_size = Arith_combine_probabilities( run_size, Arith_inv_probability( l->offsl, GETL( off ), l->max_offsL ) );
        run_size = Arith_combine_probabilities( run_size, Arith_inv_probability( l->offst, GETT( off ), l->offst_used ) );
        run_size = Arith_combine_probabilities( run_size, Arith_inv_probability( l->offsm[ GETT( off ) ], GETM( off ), l->offsm_used ) );

        // calculate the literal size
        lits_size = first_char_lit_size;

        for ( j = 1 ; j < len ; j++ )
        {
          // if the literal going to be larger, go ahead and use the run
          if ( lits_size > run_size )
            break;

          Arith_adjust_probability( l->bytes[ ( (UINTADDR) &input[ j - 1 ] ) & ADDRESS_MASK ] , input[ j - 1 ], 1 );

          lits_size = Arith_combine_probabilities( lits_size, Arith_combine_probabilities( lit_len_size , Arith_inv_probability( l->bytes[ ( (UINTADDR) &input[ j ] ) & ADDRESS_MASK ], input[ j ], l->max_bytes ) ) );
        }

        // reset the probabilities
        for ( t = 1 ; t < j ; t++ )
        {
          Arith_adjust_probability( l->bytes[ ( (UINTADDR) &input[ t - 1 ] ) & ADDRESS_MASK ], input[ t - 1 ], -1 );
        }

        if ( lits_size < run_size )
        {
          // if we got here, then the literal is going to be smaller
          goto literal;
        }

        // step 2
        // check whether we'd get a better match if we first encoded a literal (lazy stab at flexible parsing)
        l->save_pos = input + 1;

        l->save_len = find_match( l,
                                  l->save_pos,
                                  input_len - 1,
                                  Arith_inv_probability_to_bits( Arith_combine_probabilities( lit_len_size, Arith_inv_probability( l->bytes[ ( (UINTADDR) l->save_pos ) & ADDRESS_MASK ], l->save_pos[ 0 ], l->max_bytes ) ) ),
                                  &l->save_match,
                                  &l->save_bit_pat );

        {
          RADIPROB bit1;

          bit1 = first_char_lit_size;
          if ( l->save_len >= 2 )
          {
            bit1 = Arith_combine_probabilities( bit1, Arith_inv_probability( l->lens[ 0 ], l->save_bit_pat + 1, MAX_LENS + 1 ) );
            // cast for 64-bit
            j = (U32)(( l->save_pos ) - ( U8* ) l->save_match - 1);

            bit1 = Arith_combine_probabilities( bit1, Arith_inv_probability( l->offsl, GETL( off ), l->max_offsL ) );
            bit1 = Arith_combine_probabilities( bit1, Arith_inv_probability( l->offst, GETT( off ), l->offst_used ) );
            bit1 = Arith_combine_probabilities( bit1, Arith_inv_probability( l->offsm[ GETT( off ) ], GETM( off ), l->offsm_used ) );

            if ( ( ( ( F32 ) ( l->save_len + 1 ) ) / Arith_inv_probability_to_bits( bit1 ) ) > ( ( ( F32 ) len ) / Arith_inv_probability_to_bits( run_size ) ) )
            {
              goto literal;
            }
          }
        }
      }

      //printf("%i: %i %i\n",l->bytes_compressed, len, off + 1 );

      //append_byte("thelens",bit_pat+1);

      // dump the length code
      if ( Arith_was_escaped( Arith_compress( l->lens[ l->last_len ], ab, bit_pat + 1 ) ) )
      {
        // dump the escaped value
        ArithBitsPutValue( ab, bit_pat + 1, MAX_LENS + 1 );
      }

      //append_byte("theloffs",off&L);
      
      // dump the offset
      if ( Arith_was_escaped( Arith_compress( l->offsl, ab, GETL( off ) ) ) )
      {
        // dump the escaped value
        ArithBitsPutValue( ab, GETL( off ), l->max_offsL );
      }

      //append_byte("thehoffs",GETH(off)>>8);
      //append_byte("thehoffs",GETH(off)&255);
      //append_byte("thehoffsl",GETH(off)&255);
      //append_byte("thehoffsh",GETH(off)>>8);

      // dump the offset
      if ( Arith_was_escaped( Arith_compress( l->offst, ab, GETT( off ) ) ) )
      {
        // dump the escaped value
        ArithBitsPutValue( ab, GETT( off ), l->offst_used );
      }

      if ( Arith_was_escaped( Arith_compress( l->offsm[ GETT( off ) ], ab, GETM( off ) ) ) )
      {
        // dump the escaped value
        ArithBitsPutValue( ab, GETM( off ), l->offsm_used );
      } 

      l->last_len = bit_pat + 1;

      l->bytes_compressed += len;

      j = l->max_offs;
      if ( j > l->bytes_compressed )
        j = l->bytes_compressed;

      l->offst_used = GETT( j ) + 1;
      if ( j >= ( MMAX << LOW_BITS ) )
        l->offsm_used = MMAX;
      else
        l->offsm_used = ( j >> LOW_BITS ) + 1;

      // add all of the lookups
      j = len;
      while ( --j )
      {
        ++input;
        --input_len;

        if ( input_len > 2 )
          add_to_quick( l, input, input_len );
      }

      return( len );
    }
    else
    {
      U32 j;

     literal:

      //printf("%i: lit: %c %i\n",l->bytes_compressed, *input, ( (U32) input ) & ADDRESS_MASK );
      ++l->bytes_compressed;

      j = l->max_offs;
      if ( j > l->bytes_compressed )
        j = l->bytes_compressed;

      l->offst_used = GETT( j ) + 1;
      if ( j >= ( MMAX << LOW_BITS ) )
        l->offsm_used = MMAX;
      else
        l->offsm_used = ( j >> LOW_BITS ) + 1;

      //append_byte("thelens",bit_pat+1);

      // dump the literal flag
      if ( Arith_was_escaped( Arith_compress( l->lens[ l->last_len ], ab, 0 ) ) )
      {
        // dump the escaped value
        ArithBitsPutValue( ab, 0, MAX_LENS + 1 );
      }

      l->last_len = 0;

      //append_byte("thebytes",*input);

      // dump the byte
      if ( Arith_was_escaped( Arith_compress( l->bytes[ ( (UINTADDR) input ) & ADDRESS_MASK ], ab, *input ) ) )
      {
        // dump the escaped value
        ArithBitsPutValue( ab, *input, l->max_bytes );
      }

      return( 1 );
    }
  }
}


typedef struct LZDDATA
{
  ARITH bytes[ ADDRESS_MASK + 1 ], lens[ MAX_LENS + 1 ], offsl, offst, offsm[ 256 ];
  U32 max_bytes, max_offs, max_offsL, max_offsM, max_offsT;
  U32 bytes_decompressed;
  U32 last_len;
} LZDDATA;


static void copy_bytes( void* d, void* s, U32 length)
{
  U8* dest, *src;

  src = ( U8* ) s;
  dest = ( U8* ) d;

  if ( length >= 4 )
  {
    if ( ( src - dest ) >= 4 )
    {
      do
      {
        length -= 4;
        ( (U32*) dest )[ 0 ] = ( (U32*) src )[ 0 ];
        dest += 4;
        src += 4;
      } while ( length > 4 );

      if ( length == 0 )
        return;
    }
  }
    
  do
  {
    length--;
    *dest++ = *src++;
  } while ( length );

}


RADDEFFUNC U32 LZ_decompress_alloc_size( U32 max_byte_value,
                                         U32 uniq_byte_values,
                                         U32 max_offset )
{
  U32 size, h, low_max, med_max, high_max;
  RAD_UNUSED_VARIABLE(max_byte_value);

  size = sizeof( LZDDATA );

  get_max_offsets( max_offset, &low_max, &med_max, &high_max );

  h = Arith_decompress_alloc_size( uniq_byte_values ); // for bytes
  size += ( h * ( ADDRESS_MASK + 1 ) );

  size += Arith_decompress_alloc_size( MAX_LENS + 1 ) * ( MAX_LENS + 1 ); // for lengths

  size += Arith_decompress_alloc_size( low_max ); // for low offsets

  size += ( high_max * Arith_decompress_alloc_size( med_max ) ); // for med offsets

  size += Arith_decompress_alloc_size( high_max ); // for high offsets

  return( size );
}


RADDEFFUNC LZD LZ_decompress_open( void * ptr,
                                   U32 max_byte_value,
                                   U32 uniq_byte_values,
                                   U32 max_offset )
{
  U32 i, size;
  U8 * addr;
  LZD l = ( LZD ) ptr;

  l->max_bytes = max_byte_value + 1;
  l->max_offs = max_offset;

  get_max_offsets( l->max_offs, &l->max_offsL, &l->max_offsM, &l->max_offsT );

  l->last_len = 0;
  l->bytes_decompressed = 0;

  addr = (U8*) ( l + 1 );
  size = Arith_decompress_alloc_size( uniq_byte_values );

  for ( i = 0 ; i <= ADDRESS_MASK ; i++ )
  {
    l->bytes[ i ] = Arith_open( addr,
                                0,
                                max_byte_value,
                                uniq_byte_values );
    addr += size;
  }

  size =  Arith_decompress_alloc_size( MAX_LENS + 1 );

  for ( i = 0 ; i <= MAX_LENS ; i++ )
  {
    l->lens[ i ] = Arith_open( addr,
                               0,
                               MAX_LENS,
                               MAX_LENS + 1 );

    addr += size;
  }

  size =  Arith_decompress_alloc_size( l->max_offsM );

  for ( i = 0 ; i < l->max_offsT ; i++ )
  {
    l->offsm[ i ] = Arith_open( addr,
                                0,
                                l->max_offsM - 1,
                                l->max_offsM );

    addr += size;
  }

  l->offsl = Arith_open( addr,
                         0,
                         l->max_offsL - 1,
                         l->max_offsL );

  l->offst = Arith_open( addr + Arith_decompress_alloc_size( l->max_offsL ),
                         0,
                         l->max_offsT - 1,
                         l->max_offsT );

  return( l );
}


RADDEFFUNC U32 LZ_decompress_alloc_size_from_header( void * header )
{
  U32 size, i, low_max, med_max, high_max;
  LZ_HEADER * h = ( LZ_HEADER * ) header;

  size = sizeof( LZDDATA );

  i = Arith_decompress_alloc_size( h->uniq_offset_and_byte & 511 ); // for bytes
  size += ( i * ( ADDRESS_MASK + 1 ) );

  get_max_offsets( h->max_offset_and_byte >> 9, &low_max, &med_max, &high_max );

  size += Arith_decompress_alloc_size( low_max ); // for low offsets

  size += ( high_max * Arith_decompress_alloc_size( med_max ) ); // for med offsets

  size += Arith_decompress_alloc_size( GETT( h->uniq_offset_and_byte >> 9 ) + 1 ); // for high offsets

  for ( i = 0 ; i < 4 ; i++ )
  {
    U32 s = Arith_decompress_alloc_size( ( h->uniq_lens >> ( ( 3 - i ) * 8 ) ) & 255 );
    size += ( s * ( MAX_LENS / 4 ) );
  }

  size += ( Arith_decompress_alloc_size( h->uniq_lens & 255 ) * ( MAX_LENS - ( ( MAX_LENS / 4 ) * 4 ) + 1 ) );

  return( size );
}


RADDEFFUNC LZD LZ_decompress_open_from_header( void * ptr,
                                               void * header )
{
  U32 i, j, size;
  U8 * addr;
  U32 uniq = 0;
  LZD l = ( LZD ) ptr;
  LZ_HEADER * h = ( LZ_HEADER * ) header;

  l->max_bytes = h->max_offset_and_byte & 511;
  l->max_offs = h->max_offset_and_byte >> 9;

  get_max_offsets( l->max_offs, &l->max_offsL, &l->max_offsM, &l->max_offsT );
  
  l->last_len = 0;
  l->bytes_decompressed = 0;

  j = h->uniq_offset_and_byte & 511;
  addr = (U8*) ( l + 1 );
  size = Arith_decompress_alloc_size( j );

  for ( i = 0 ; i <= ADDRESS_MASK ; i++ )
  {
    l->bytes[ i ] = Arith_open( addr,
                                0,
                                l->max_bytes - 1,
                                j );
    addr += size;
  }
  
  for ( j = 0 ; j < 4 ; j++ )
  {
    uniq = ( h->uniq_lens >> ( ( 3 - j ) * 8 ) ) & 255;
    size = Arith_decompress_alloc_size( uniq );
    for ( i = 0 ; i < ( MAX_LENS / 4 ) ; i++ )
    {
      l->lens[ ( j * ( MAX_LENS / 4 ) ) + i ] = Arith_open( addr, 0, MAX_LENS, uniq );
      addr += size;
    }
  }

  for ( i = ( ( MAX_LENS / 4 ) * 4 ) ; i <= MAX_LENS ; i++ )
  {
    // This use of uniq should indeed inherit the value of the last iteration of the previous loop
    l->lens[ i ] = Arith_open( addr, 0, MAX_LENS, uniq );
    addr += size;
  }

  size = Arith_decompress_alloc_size( l->max_offsM );

  for ( i = 0 ; i < l->max_offsT ; i++ )
  {
    l->offsm[ i ] = Arith_open( addr, 0, l->max_offsM - 1, l->max_offsM );
    addr += size;
  }

  l->offsl = Arith_open( addr,
                         0,
                         l->max_offsL - 1,
                         l->max_offsL );

  l->offst = Arith_open( addr + Arith_decompress_alloc_size( l->max_offsL ),
                         0,
                         l->max_offsT - 1,
                         GETT( h->uniq_offset_and_byte >> 9 ) + 1 );

  return( l );
}


RADDEFFUNC U32 LZ_decompress( LZD l, 
                              ARITHBITS* ab, 
                              U8 * output )
{
  UINTADDR v;
  U32 escaped;

  // get the length count/type bit

  v = Arith_decompress( l->lens[ l->last_len ], ab );
  if ( Arith_was_escaped( v ) )
  {
    // get the escaped value
    escaped = ArithBitsGetValue( ab, MAX_LENS + 1 );

    // and save it
    Arith_set_decompressed_symbol( v, escaped );

    v = escaped;
  }

  // always a valid cast
  l->last_len = (U32)v;

  // is this a length/offset pair?
  if ( v )
  {
    U32 len, off, max_ofs;
    UINTADDR m;

    max_ofs = l->max_offs;
    if ( max_ofs > l->bytes_decompressed )
      max_ofs = l->bytes_decompressed;

    len = ( v >= ( MAX_LENS - 3 ) ) ? long_lengths[ v - ( MAX_LENS - 3 ) ] : (U32)( v + 1 );

    v = Arith_decompress( l->offsl, ab );
    if ( Arith_was_escaped( v ) )
    {
      // get the escaped value
      escaped = ArithBitsGetValue( ab, l->max_offsL );

      // and save it
      Arith_set_decompressed_symbol( v, escaped );

      v = escaped;
    }

    off = (U32)(v + 1);

    v = Arith_decompress( l->offst, ab );
    if ( Arith_was_escaped( v ) )
    {
      // get the escaped value
      escaped = ArithBitsGetValue( ab, GETT( max_ofs ) + 1 );

      // and save it
      Arith_set_decompressed_symbol( v, escaped );

      v = escaped;
    }

    m = Arith_decompress( l->offsm[ v ], ab );
    if ( Arith_was_escaped( m ) )
    {
      U32 offsm_used;
  
      if ( max_ofs >= ( MMAX << LOW_BITS ) )
        offsm_used = MMAX;
      else
        offsm_used = ( max_ofs >> LOW_BITS ) + 1;

      // get the escaped value
      escaped = ArithBitsGetValue( ab, offsm_used );

      // and save it
      Arith_set_decompressed_symbol( m, escaped );

      m = escaped;
    };
    
    // cast for 64-bit
    off = (U32)(off + ( m << LOW_BITS ) + ( v << ( LOW_BITS + MED_BITS ) ));

    //printf("%i: %i %i\n",l->bytes_decompressed, len, off );

    l->bytes_decompressed += len;

    copy_bytes( output, output - off, len );

    return( len );
  }
  else
  {
    // nope, just decode a literal

    v = Arith_decompress( l->bytes[ ( (UINTADDR) output ) & ADDRESS_MASK ], ab );
    if ( Arith_was_escaped( v ) )
    {
      // get the escaped value
      escaped = ArithBitsGetValue( ab, l->max_bytes );

      // and save it
      Arith_set_decompressed_symbol( v, escaped );

      v = escaped;
    }

    *output = ( U8 ) v;

    //printf("%i: lit: %c %i\n",l->bytes_decompressed, v, ( (U32) output ) & ADDRESS_MASK);

    ++l->bytes_decompressed;

    return( 1 );
  }
}


RADDEFFUNC U32 LZ_decompress_sizeonly( LZD l, ARITHBITS* ab )
{
  UINTADDR v;
  U32 escaped;

  // get the length count/type bit
  v = Arith_decompress( l->lens[ l->last_len ], ab );
  if ( Arith_was_escaped( v ) )
  {
    // get the escaped value
    // and save it
    escaped = ArithBitsGetValue( ab, MAX_LENS + 1 );
    Arith_set_decompressed_symbol( v, escaped );

    v = escaped;
  }

  // always a valid cast
  l->last_len = (U32)v;

  // is this a length/offset pair?
  if ( v )
  {
    U32 len, max_ofs;
    UINTADDR m;

    max_ofs = l->max_offs;
    if ( max_ofs > l->bytes_decompressed )
      max_ofs = l->bytes_decompressed;

    len = ( v >= ( MAX_LENS - 3 ) ) ? long_lengths[ v - ( MAX_LENS - 3 ) ] : (U32)( v + 1 );

    v = Arith_decompress( l->offsl, ab );
    if ( Arith_was_escaped( v ) )
    {
      // get the escaped value
      // and save it
      escaped = ArithBitsGetValue( ab, l->max_offsL );
      Arith_set_decompressed_symbol( v, escaped );
    }

    v = Arith_decompress( l->offst, ab );
    if ( Arith_was_escaped( v ) )
    {
      // get the escaped value
      // and save it
      escaped = ArithBitsGetValue( ab, GETT( max_ofs ) + 1 );
      Arith_set_decompressed_symbol( v, escaped );

      v = escaped;
    }

    m = Arith_decompress( l->offsm[ v ], ab );
    if ( Arith_was_escaped( m ) )
    {
      U32 offsm_used;
      if ( max_ofs >= ( MMAX << LOW_BITS ) )
        offsm_used = MMAX;
      else
        offsm_used = ( max_ofs >> LOW_BITS ) + 1;

      // get the escaped value
      // and save it
      escaped = ArithBitsGetValue( ab, offsm_used );
      Arith_set_decompressed_symbol( m, escaped );
    }

    l->bytes_decompressed += len;
    return( len );
  }
  else
  {
    // nope, just decode a literal
    v = Arith_decompress( l->bytes[l->bytes_decompressed & ADDRESS_MASK], ab );
    if ( Arith_was_escaped( v ) )
    {
      // get the escaped value
      // and save it
      escaped = ArithBitsGetValue( ab, l->max_bytes );
      Arith_set_decompressed_symbol( v, escaped );
    }

    ++l->bytes_decompressed;

    return( 1 );
  }
}
