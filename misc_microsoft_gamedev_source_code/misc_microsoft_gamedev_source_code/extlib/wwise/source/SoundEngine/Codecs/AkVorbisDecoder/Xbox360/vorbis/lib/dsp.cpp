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

 function: PCM data vector blocking, windowing and dis/reassembly

 ********************************************************************/

#include <stdlib.h> 
#include "ogg/ogg.h"
#include "mdct.h"
#include "vorbis/ivorbiscodec.h"
#include "vorbis/codec_internal.h"
#include "misc.h"
#include "window_lookup.h"

int vorbis_dsp_restart(vorbis_dsp_state *v)
{
	v->state.out_end		= -1;
	v->state.out_begin		= -1;
	v->state.granulepos		= -1;
	v->state.sequence		= -1;
	v->state.sample_count	= -1;

	return 0;
}

int vorbis_dsp_init(vorbis_dsp_state* v, CAkVorbisAllocator& VorbisAllocator )
{
	int i;

	vorbis_info* vi = &(v->vi);

	codec_setup_info *ci=(codec_setup_info *)&vi->csi;

	// allocate the pointers
	v->work			= (ogg_int32_t **)VorbisAllocator.Alloc(vi->channels*sizeof(*v->work));
	v->mdctright	= (ogg_int32_t **)VorbisAllocator.Alloc(vi->channels*sizeof(*v->mdctright));
	v->vi.AddAllocatedMemory(vi->channels*sizeof(*v->work),"v->work");
	v->vi.AddAllocatedMemory(vi->channels*sizeof(*v->mdctright),"v->mdctright");

	// otherwise vorbis_dsp_clear will take it for some valid address
	v->work[0]		= NULL;

	// allocate the arrays
	int WorkSize	= v->GetWorkSize();
	int DctSize		= v->GetDctSize();
	int AllocSize	= WorkSize + DctSize;

	// Do not count in UVM, do not alloc in UVM memory
	char* pWork		= (char*)_ogg_malloc(AllocSize);

	// out of memory
	if(pWork == NULL)
	{
		return -1;
	}

	char* pDct		= pWork + WorkSize;

	memset(pWork,0,WorkSize);
	memset(pDct,0,DctSize);

	// we need the size per channel now
	WorkSize /= vi->channels;
	DctSize /= vi->channels;
	for(i = 0 ; i < vi->channels ; ++i)
	{
		v->work[i]		= (ogg_int32_t *)pWork;
		v->mdctright[i]	= (ogg_int32_t *)pDct;

		pWork += WorkSize;
		pDct += DctSize;
	}

	v->state.lW=0; /* previous window size */
	v->state.W=0;  /* current window size */

	return vorbis_dsp_restart(v);
}

void vorbis_dsp_clear(vorbis_dsp_state *v, CAkVorbisAllocator& VorbisAllocator)
{
	vorbis_info*	vi = &(v->vi);

	if(v->work)
	{
		if(v->work[0])
		{
			// Do not count in UVM, do not alloc in UVM memory
			_ogg_free(v->work[0]);
		}
		v->work = NULL;
	}

	v->mdctright = NULL;
}

int ilog(ogg_uint32_t v)
{
	int ret=0;
	if(v)
	{
		--v;
	}
	while(v)
	{
		ret++;
		v>>=1;
	}
	return(ret);
}

static float *_vorbis_window(int left){
  switch(left){
/*
  case 32:
    return vwin64;
  case 64:
    return vwin128;
*/
  case 128:
    return vwin256;
  case 256:
    return vwin512;
  case 512:
    return vwin1024;
  case 1024:
    return vwin2048;
  case 2048:
    return vwin4096;
/*
#ifndef LIMIT_TO_64kHz
  case 4096:
    return vwin8192;
#endif
*/
  default:
    return(0);
  }
}

/* pcm==0 indicates we just want the pending samples, no more */
int vorbis_dsp_pcmout(vorbis_dsp_state *v,ogg_int16_t *pcm,int samples)
{
	vorbis_info* vi = &(v->vi);
	codec_setup_info *ci = (codec_setup_info *)&vi->csi;
	if((v->state.out_begin > -1) && (v->state.out_begin < v->state.out_end))
	{
		int n = v->state.out_end - v->state.out_begin;
		if(pcm)
		{
			int i;
			if(n > samples)
			{
				n = samples;
			}
			float *Window0 = _vorbis_window(ci->blocksizes[0]>>1);
			float*Window1 = _vorbis_window(ci->blocksizes[1]>>1);
			for(i = 0 ; i < vi->channels ; i++)
			{
				mdct_unroll_lap(ci->blocksizes[0],
								ci->blocksizes[1],
								v->state.lW,
								v->state.W,
								(float *) v->work[i],
								(float *) v->mdctright[i],
								Window0,
								Window1,
								pcm + i,
								vi->channels,
								v->state.out_begin,
								v->state.out_begin + n);
			}
		}
		return(n);
	}
	return(0);
}

int vorbis_dsp_read(vorbis_dsp_state *v,int s)
{
	if(s && v->state.out_begin+s>v->state.out_end)
	{
		return(OV_EINVAL);
	}
	v->state.out_begin += s;
	return(0);
}

// enable for SPU only version
int vorbis_dsp_synthesis(vorbis_dsp_state *vd,ogg_packet *op)
{
	vorbis_info*		vi = &(vd->vi);
	codec_setup_info*	ci = (codec_setup_info *)&vi->csi;
	int mode,i;

	oggpack_readinit(&vd->opb,&(op->buffer));

	/* Check the packet type */
	if(oggpack_read(&vd->opb,1)!=0)
	{
		/* Oops.  This is not an audio data packet */
#ifdef _DEBUG
		PRINT("This is not an audio data packet\n");
#endif
		return OV_ENOTAUDIO ;
	}

	/* read our mode and pre/post windowsize */
	mode=oggpack_read(&vd->opb,ilog(ci->modes));
	if(mode==-1 || mode>=ci->modes)
	{
#ifdef _DEBUG
		PRINT("mode %d vi->csi.modes %d\n",mode,vi->csi.modes);
#endif
		return OV_EBADPACKET;
	}

	/* shift information we still need from last window */
	vd->state.lW=vd->state.W;
	vd->state.W=ci->mode_param[mode].blockflag;

	for(i=0;i<vi->channels;i++)
	{
		mdct_shift_right(ci->blocksizes[vd->state.lW],vd->work[i],vd->mdctright[i]);
	}

	if(vd->state.W)
	{
		int temp;
		oggpack_read(&vd->opb,1);
		temp=oggpack_read(&vd->opb,1);
		if(temp==-1)
		{
#ifdef _DEBUG
			PRINT("temp==-1\n");
#endif
			return OV_EBADPACKET;
		}
	}

	/* packet decode and portions of synthesis that rely on only this block */
	mapping_inverse(vd,ci->map_param+ci->mode_param[mode].mapping);

	if(vd->state.out_begin==-1)
	{
		vd->state.out_begin=0;
		vd->state.out_end=0;
	}
	else
	{
		vd->state.out_begin=0;
		vd->state.out_end=ci->blocksizes[vd->state.lW]/4+ci->blocksizes[vd->state.W]/4;
	}

	/* track the frame number... This is for convenience, but also
	making sure our last packet doesn't end with added padding.

	This is not foolproof!  It will be confused if we begin
	decoding at the last page after a seek or hole.  In that case,
	we don't have a starting point to judge where the last frame
	is.  For this reason, vorbisfile will always try to make sure
	it reads the last two marked pages in proper sequence */

	/* if we're out of sequence, dump granpos tracking until we sync back up */
	if(vd->state.sequence==-1 || vd->state.sequence+1 != op->packetno-3)
	{
		/* out of sequence; lose count */
		vd->state.granulepos=-1;
		vd->state.sample_count=-1;
	}

	vd->state.sequence=op->packetno;
	vd->state.sequence=vd->state.sequence-3;

	if(vd->state.sample_count==-1)
	{
		vd->state.sample_count=0;
	}
	else
	{
		vd->state.sample_count += ci->blocksizes[vd->state.lW]/4+ci->blocksizes[vd->state.W]/4;
	}

	if(vd->state.granulepos==-1)
	{
		if(op->granulepos!=-1)
		{ /* only set if we have a
			position to set to */

			vd->state.granulepos=op->granulepos;

			/* is this a short page? */
			if(vd->state.sample_count>vd->state.granulepos)
			{
				/* corner case; if this is both the first and last audio page,
				then spec says the end is cut, not beginning */
				if(op->e_o_s)
				{
					/* trim the end */
					/* no preceeding granulepos; assume we started at zero (we'd
					have to in a short single-page stream) */
					/* granulepos could be -1 due to a seek, but that would result
					in a long coun t, not short count */

					vd->state.out_end-=vd->state.sample_count-vd->state.granulepos;
				}
				else
				{
					/* trim the beginning */
					vd->state.out_begin+=vd->state.sample_count-vd->state.granulepos;
					if(vd->state.out_begin>vd->state.out_end)
					{
						vd->state.out_begin=vd->state.out_end;
					}
				}
			}
		}
	}
	else
	{
		vd->state.granulepos += ci->blocksizes[vd->state.lW]/4+ci->blocksizes[vd->state.W]/4;
		if(op->granulepos!=-1 && vd->state.granulepos!=op->granulepos)
		{
			if(vd->state.granulepos>op->granulepos)
			{
				long extra=vd->state.granulepos-op->granulepos;

				if(extra)
				{
					if(op->e_o_s)
					{
						/* partial last frame.  Strip the extra samples off */
						vd->state.out_end-=extra;
					} /* else {Shouldn't happen *unless* the bitstream is out of
					  spec.  Either way, believe the bitstream } */
				}
			} /* else {Shouldn't happen *unless* the bitstream is out of
			  spec.  Either way, believe the bitstream } */
			vd->state.granulepos=op->granulepos;
		}
	}

	return(0);
}
