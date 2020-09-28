/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggVorbis 'TREMOR' CODEC SOURCE CODE.   *
 *                                                                  *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE OggVorbis 'TREMOR' SOURCE CODE IS (C) COPYRIGHT 1994-2003    *
 * BY THE Xiph.Org FOUNDATION http://www.xiph.org/                  *
 *                                                                  *
 ********************************************************************

 function: maintain the info structure, info <-> header packets

 ********************************************************************/

/* general handling of the header and the vorbis_info structure (and
   substructures) */

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "ogg/ogg.h"
#include "vorbis/ivorbiscodec.h"
#include "vorbis/codec_internal.h"
#include "codebook.h"
#include "os.h"

/* helpers */

#ifndef __SPU__

/* blocksize 0 is guaranteed to be short, 1 is guarantted to be long.
   They may be equal, but short will never ge greater than long */
int vorbis_info_blocksize(vorbis_info *vi,int zo)
{
	return vi->csi.blocksizes[zo];
}


/* used by synthesis, which has a full, alloced vi */
void vorbis_info_init(vorbis_info *vi)
{
	memset(vi,0,sizeof(*vi));
}


/* Header packing/unpacking ********************************************/
static int _vorbis_unpack_info(vorbis_info *vi,oggpack_buffer *opb, CAkVorbisAllocator& VorbisAllocator)
{
	codec_setup_info* ci = (codec_setup_info *)&vi->csi;

	// PhM : version is not used for decoding
	if(oggpack_read(opb,32) != 0)
	{
		return(OV_EVERSION);
	}

	vi->channels = oggpack_read(opb,8);
	vi->rate = oggpack_read(opb,32);

	// PhM : skip those not used for decoding
	oggpack_read(opb,32);	// vi->bitrate_upper
	oggpack_read(opb,32);	// bitrate_nominal
	oggpack_read(opb,32);	// bitrate_lower

	ci->blocksizes[0] = 1 << oggpack_read(opb,4);
	ci->blocksizes[1] = 1 << oggpack_read(opb,4);
  
#ifdef LIMIT_TO_64kHz
	if(vi->rate >= 64000 || ci->blocksizes[1] > 4096)
	{
		goto err_out;
	}
#else
	if(vi->rate < 64000 && ci->blocksizes[1] > 4096)
	{
		goto err_out;
	}
#endif

	if(vi->rate<1)
	{
	  goto err_out;
	}
	if(vi->channels<1)
	{
	  goto err_out;
	}
	if(ci->blocksizes[0]<64)
	{
	  goto err_out; 
	}
	if(ci->blocksizes[1]<ci->blocksizes[0])
	{
	  goto err_out;
	}
	if(ci->blocksizes[1]>8192)
	{
	  goto err_out;
	}

	if(oggpack_read(opb,1)!=1)
	{
	  goto err_out; /* EOP check */
	}

	return(0);

err_out:
	return(OV_EBADHEADER);
}

#endif

/* all of the real encoding details are here.  The modes, books,
   everything */
static int _vorbis_unpack_books(vorbis_info *vi,oggpack_buffer *opb, CAkVorbisAllocator& VorbisAllocator)
{
	codec_setup_info* ci = (codec_setup_info *)&vi->csi;
	int i;
//--------------------------------------------------------------------------------
// codebooks
//--------------------------------------------------------------------------------
	ci->books = oggpack_read(opb,8)+1;
	int AllocSize = sizeof(*ci->book_param)*ci->books;
	ci->book_param = (codebook *)VorbisAllocator.Calloc(AllocSize);
	vi->AddAllocatedMemory(AllocSize,"ci->book_param");

	for(i = 0 ; i < ci->books ; i++)
	{
		int AllocatedMem = vorbis_book_unpack(opb,ci->book_param + i,VorbisAllocator);
		if(AllocatedMem < 0)
		{
			goto err_out;
		}
		vi->AddAllocatedMemory(AllocatedMem,"vorbis_book_unpack");
	}
//--------------------------------------------------------------------------------
// time backend settings, not actually used
//--------------------------------------------------------------------------------
	i = oggpack_read(opb,6);
	for(; i >= 0 ; --i)
	{
		if(oggpack_read(opb,16)!=0)
		{
			goto err_out;
		}
	}
//--------------------------------------------------------------------------------
// floor backend settings
//--------------------------------------------------------------------------------
	ci->floors = oggpack_read(opb,6)+1;

	AllocSize = sizeof(vorbis_info_floor1) * ci->floors;

	ci->floor_param	= (vorbis_info_floor1*)VorbisAllocator.Calloc(AllocSize);
	vi->AddAllocatedMemory(AllocSize,"ci->floor_param");

	for(i = 0 ; i < ci->floors ; i++)
	{
		long FloorType = oggpack_read(opb,16);
		if(FloorType != 1)
		{
			goto err_out;
		}
		if(floor1_info_unpack(&ci->floor_param[i],vi,opb,VorbisAllocator) != 0)
		{
			goto err_out;
		}
	}
//--------------------------------------------------------------------------------
// residue backend settings
//--------------------------------------------------------------------------------
	ci->residues = oggpack_read(opb,6)+1;
	AllocSize = sizeof(*ci->residue_param)*ci->residues;
	ci->residue_param	= (vorbis_info_residue*)VorbisAllocator.Alloc(AllocSize);
	vi->AddAllocatedMemory(AllocSize,"ci->residue_param");

	for(i = 0 ; i < ci->residues ; ++i)
	{
		if(res_unpack(ci->residue_param + i,vi,opb,VorbisAllocator))
		{
			goto err_out;
		}
	}
//--------------------------------------------------------------------------------
// map backend settings
//--------------------------------------------------------------------------------
	ci->maps = oggpack_read(opb,6)+1;
	AllocSize = sizeof(*ci->map_param)*ci->maps;
	ci->map_param	= (vorbis_info_mapping*)VorbisAllocator.Alloc(AllocSize);
	vi->AddAllocatedMemory(AllocSize,"ci->map_param");

	for(i = 0 ; i < ci->maps ; ++i)
	{
		if(oggpack_read(opb,16)!=0)
		{
			goto err_out;
		}
		if(mapping_info_unpack(ci->map_param+i,vi,opb,VorbisAllocator))
		{
			goto err_out;
		}
	}
//--------------------------------------------------------------------------------
// mode settings
//--------------------------------------------------------------------------------
	ci->modes=oggpack_read(opb,6)+1;
	AllocSize = ci->modes*sizeof(*ci->mode_param);
	ci->mode_param	= (vorbis_info_mode *)VorbisAllocator.Alloc(AllocSize);
	vi->AddAllocatedMemory(AllocSize,"ci->mode_param");

	for(i = 0 ; i < ci->modes ; ++i)
	{
		ci->mode_param[i].blockflag = oggpack_read(opb,1);
		if(oggpack_read(opb,16))
		{
			goto err_out;
		}
		if(oggpack_read(opb,16))
		{
			goto err_out;
		}
		ci->mode_param[i].mapping = oggpack_read(opb,8);
		if(ci->mode_param[i].mapping>=ci->maps)
		{
			goto err_out;
		}
	}

	if(oggpack_read(opb,1)!=1)goto err_out; /* top level EOP check */

	return(0);

err_out:
	return(OV_EBADHEADER);
}

/* The Vorbis header is in three packets; the initial small packet in
   the first page that identifies basic parameters, a second packet
   with bitstream comments and a third packet that holds the
   codebook. */

int vorbis_dsp_headerin(vorbis_info *vi,ogg_packet *op, CAkVorbisAllocator& VorbisAllocator)
{
	oggpack_buffer opb;

	if(op)
	{
		oggpack_readinit(&opb,&(op->buffer));

		/* Which of the three types of header is this? */
		/* Also verify header-ness, vorbis */
		{
			int packtype = oggpack_read(&opb,8);

			oggpack_adv( &opb, 6 * 8 ); // skip over 'vorbis' string

			switch(packtype)
			{
#ifndef __SPU__
				case 0x01: /* least significant *bit* is read first */
				if(vi->rate!=0)
				{
				  /* previously initialized info header */
				  return(OV_EBADHEADER);
				}

				return(_vorbis_unpack_info(vi,&opb,VorbisAllocator));

				case 0x03: /* least significant *bit* is read first */
				if(vi->rate==0)
				{
					/* um... we didn't get the initial header */
					return(OV_EBADHEADER);
				}

				// PhM : we don't need the comments
//				return(_vorbis_unpack_comment(vc,&opb));
				return(0);
#endif
#if !defined(AK_PS3) || defined(SPU_CODEBOOK)
				case 0x05: /* least significant *bit* is read first */
				if(vi->rate==0)
				{
					/* um... we didn;t get the initial header or comments yet */
					return(OV_EBADHEADER);
				}

				return(_vorbis_unpack_books(vi,&opb,VorbisAllocator));
#endif
				default:
				/* Not a valid vorbis header type */
				return(OV_EBADHEADER);
				break;
			}
		}
	}
	return(OV_EBADHEADER);
}
