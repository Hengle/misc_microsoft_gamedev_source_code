/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggVorbis 'TREMOR' CODEC SOURCE CODE.   *
 *                                                                  *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE OggVorbis 'TREMOR' SOURCE CODE IS (C) COPYRIGHT 1994-2002    *
 * BY THE Xiph.Org FOUNDATION http://www.xiph.org/                  *
 *                                                                  *
 ********************************************************************

 function: libvorbis codec headers

 ********************************************************************/

#ifndef _vorbis_codec_h_
#define _vorbis_codec_h_

#include "ogg/ogg.h"

#ifndef ALIGN_SIZE_16
#define ALIGN_SIZE_16( _size_ ) _size_;
#endif

#ifdef _DEBUG
#ifdef _CHECKSTACKALLOC
struct StackAlloc
{
	void Reset()
	{
		StackAlloc = 0;
		MaxStackAlloc = 0;
	}

	void Alloc(int Size)
	{
		// round to number of quadwords
		StackAlloc += ALIGN_SIZE_16(Size)
		if(StackAlloc > MaxStackAlloc)
		{
			MaxStackAlloc = StackAlloc;
		}
	}

	void Free(int Size)
	{
		//StackAlloc -= (((Size + 15) / 16) * 16);
		StackAlloc -= Size;
		assert(StackAlloc>=0);
	}

	void PrintMaxStackAlloc()
	{
		printf("MaxStackAlloc %d\n",MaxStackAlloc);
	}

	int StackAlloc;
	int MaxStackAlloc;
};
extern StackAlloc Stack;

#endif
#endif

typedef struct
{
	char			class_dim;        /* 1 to 8 */
	char			class_subs;       /* 0,1,2,3 (bits: 1<<n poss) */
	unsigned char	class_book;       /* subs ^ dim entries */
	unsigned char	class_subbook[8]; /* [VIF_CLASS][subs] */
} floor1class;  

typedef struct
{
	floor1class  *Class;          /* [VIF_CLASS] */
	char         *partitionclass; /* [VIF_PARTS]; 0 to 15 */
	ogg_uint16_t *postlist;       /* [VIF_POSIT+2]; first two implicit */ 
	char         *forward_index;  /* [VIF_POSIT+2]; */
	char         *hineighbor;     /* [VIF_POSIT]; */
	char         *loneighbor;     /* [VIF_POSIT]; */

	int          partitions;    /* 0 to 31 */
	int          posts;
	int          mult;          /* 1 2 3 or 4 */ 

} vorbis_info_floor1;

typedef struct {
  unsigned char blockflag;
  unsigned char mapping;
} vorbis_info_mode;

struct vorbis_info_mapping;
struct vorbis_info_residue;
struct codebook;

typedef struct codec_setup_info
{
	/* Vorbis supports only short and long blocks, but allows the
	encoder to choose the sizes */

	long blocksizes[2];

	/* modes are the primary means of supporting on-the-fly different
	blocksizes, different channel mappings (LR or M/A),
	different residue backends, etc.  Each mode consists of a
	blocksize flag and a mapping (along with the mapping setup */

	int        modes;
	int        maps;
	int        floors;
	int        residues;
	int        books;

	// all these allocations are done in _vorbis_unpack_books
	vorbis_info_mode	*mode_param;		// vorbis_info_mode		mode_param[modes]
	vorbis_info_mapping	*map_param;			// vorbis_info_mapping	map_param[maps]
	vorbis_info_floor1	*floor_param;		// vorbis_info_floor*	floor_param[floors]
	vorbis_info_residue	*residue_param;		// vorbis_info_residue	residue_param[residues]
	codebook			*book_param;		// codebook				book_param[books]

} codec_setup_info;

typedef struct vorbis_info
{
#if defined(_TRACKMEMORY) && !defined(__SPU__)
	void AddAllocatedMemory(int Size,char* String)
	{
		AllocatedMemory += Size;
		printf("%s,%d\n",String,Size);
	}
#else
	inline void AddAllocatedMemory(int Size,char* String) {};
#endif
//	int version;
	int channels;
	long rate;

  /* The below bitrate declarations are *hints*.
     Combinations of the three values carry the following implications:
     
     all three set to the same value: 
       implies a fixed rate bitstream
     only nominal set: 
       implies a VBR stream that averages the nominal bitrate.  No hard 
       upper/lower limit
     upper and or lower set: 
       implies a VBR bitstream that obeys the bitrate limits. nominal 
       may also be set to give a nominal rate.
     none set:
       the coder does not care to speculate.
  */

//	long bitrate_upper;
//	long bitrate_nominal;
//	long bitrate_lower;
//	long bitrate_window;

	codec_setup_info csi;
#if defined(_TRACKMEMORY) && !defined(__SPU__)
	int AllocatedMemory;
#endif
} vorbis_info;

typedef struct vorbis_comment
{
	char **user_comments;
	int   *comment_lengths;
	int    comments;
	char  *vendor;
} vorbis_comment;


/* Vorbis PRIMITIVES: general ***************************************/
#ifndef __SPU__
extern void     vorbis_info_init(vorbis_info *vi);
extern int      vorbis_info_blocksize(vorbis_info *vi,int zo);
#endif
/* Vorbis ERRORS and return codes ***********************************/

#define OV_FALSE      -1  
#define OV_EOF        -2
#define OV_HOLE       -3

#define OV_EREAD      -128
#define OV_EFAULT     -129
#define OV_EIMPL      -130
#define OV_EINVAL     -131
#define OV_ENOTVORBIS -132
#define OV_EBADHEADER -133
#define OV_EVERSION   -134
#define OV_ENOTAUDIO  -135
#define OV_EBADPACKET -136
#define OV_EBADLINK   -137
#define OV_ENOSEEK    -138

#endif

