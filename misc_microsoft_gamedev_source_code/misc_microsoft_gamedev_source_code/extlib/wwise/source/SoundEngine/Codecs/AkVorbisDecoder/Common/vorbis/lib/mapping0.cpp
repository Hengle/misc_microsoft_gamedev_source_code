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

 function: channel mapping 0 implementation

 ********************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "ogg/ogg.h"
#include "os.h"
#include "vorbis/ivorbiscodec.h"
#include "mdct.h"
#include "vorbis/codec_internal.h"
#include "codebook.h"
#include "misc.h"

#if !defined(AK_PS3) || defined(SPU_CODEBOOK)

static int ilog(unsigned int v){
  int ret=0;
  if(v)--v;
  while(v){
    ret++;
    v>>=1;
  }
  return(ret);
}

/* also responsible for range checking */
int mapping_info_unpack(vorbis_info_mapping*	info,
						vorbis_info*			vi,
						oggpack_buffer*			opb,
						CAkVorbisAllocator&		VorbisAllocator)
{
	int i;
	int AllocSize;
	codec_setup_info     *ci=(codec_setup_info *)&vi->csi;
	memset(info,0,sizeof(*info));

  if(oggpack_read(opb,1))
    info->submaps=oggpack_read(opb,4)+1;
  else
    info->submaps=1;

	if(oggpack_read(opb,1))
	{
		info->coupling_steps=oggpack_read(opb,8)+1;
		int AllocSize = info->coupling_steps*sizeof(*info->coupling);
		info->coupling = (coupling_step*)VorbisAllocator.Alloc(AllocSize);
		vi->AddAllocatedMemory(AllocSize,"info->coupling");

		for(i=0;i<info->coupling_steps;i++)
		{
			int testM=info->coupling[i].mag=oggpack_read(opb,ilog(vi->channels));
			int testA=info->coupling[i].ang=oggpack_read(opb,ilog(vi->channels));

			if(	testM<0 || 
				testA<0 || 
				testM==testA || 
				testM>=vi->channels ||
				testA>=vi->channels)
			{
				goto err_out;
			}
		}
	}

	if(oggpack_read(opb,2)>0)
	{
		goto err_out; /* 2,3:reserved */
	}

	if(info->submaps>1)
	{
		int AllocSize = sizeof(*info->chmuxlist)*vi->channels;
		info->chmuxlist = (unsigned char*)VorbisAllocator.Alloc(AllocSize);
		vi->AddAllocatedMemory(AllocSize,"info->chmuxlist");

		for(i=0;i<vi->channels;i++)
		{
			info->chmuxlist[i]=oggpack_read(opb,4);
			if(info->chmuxlist[i]>=info->submaps)
			{
				goto err_out;
			}
		}
	}

	AllocSize = sizeof(*info->submaplist)*info->submaps;
	info->submaplist = (submap*)VorbisAllocator.Alloc(AllocSize);
	vi->AddAllocatedMemory(AllocSize,"info->submaplist");

	for(i=0;i<info->submaps;i++)
	{
		oggpack_read(opb,8);
		info->submaplist[i].floor=oggpack_read(opb,8);
		if(info->submaplist[i].floor>=ci->floors)
		{
			goto err_out;
		}
		info->submaplist[i].residue=oggpack_read(opb,8);
		if(info->submaplist[i].residue>=ci->residues)
		{
			goto err_out;
		}
  }

  return 0;

 err_out:
  return -1;
}
#endif

#if !defined(AK_PS3) || defined(SPU_DECODER)

int mapping_inverse(vorbis_dsp_state *vd,vorbis_info_mapping *info)
{
	vorbis_info*		vi = &(vd->vi);
	codec_setup_info     *ci=(codec_setup_info *)&vi->csi;

	int                   i,j;
	long                  n=ci->blocksizes[vd->state.W];

	ogg_int32_t **pcmbundle = (ogg_int32_t**)alloca(sizeof(*pcmbundle)*vi->channels);
	int          *zerobundle = (int*)alloca(sizeof(*zerobundle)*vi->channels);
	int          *nonzero	= (int*)alloca(sizeof(*nonzero)*vi->channels);
	ogg_int32_t **floormemo = (ogg_int32_t**)alloca(sizeof(*floormemo)*vi->channels);

#ifdef _CHECKSTACKALLOC
	int Size = vi->channels*(sizeof(*pcmbundle)+sizeof(*zerobundle)+sizeof(*nonzero)+sizeof(*floormemo));
#endif
	/* recover the spectral envelope; store it in the PCM vector for now */
	for(i=0;i<vi->channels;i++)
	{
		int submap=0;
		int floorno;

		if(info->submaps>1)
		{
			submap=info->chmuxlist[i];
		}
		floorno=info->submaplist[submap].floor;

		/* floor 1 */
		floormemo[i] = (ogg_int32_t*)alloca(sizeof(*floormemo[i])*floor1_memosize(&ci->floor_param[floorno]));

#ifdef _CHECKSTACKALLOC
		Size += sizeof(*floormemo[i])*floor1_memosize(&ci->floor_param[floorno]);
#endif

		floormemo[i]=floor1_inverse1(vd,&ci->floor_param[floorno],floormemo[i]);

		if(floormemo[i])
		{
			nonzero[i]=1;
		}
		else
		{
			nonzero[i]=0;
		}
		memset(vd->work[i],0,sizeof(*vd->work[i])*n/2);
	}

#ifdef _CHECKSTACKALLOC
	Stack.Alloc(Size);
#endif
	/* channel coupling can 'dirty' the nonzero listing */
	for(i=0;i<info->coupling_steps;i++)
	{
		if(nonzero[info->coupling[i].mag] ||
			nonzero[info->coupling[i].ang])
		{
				nonzero[info->coupling[i].mag]=1; 
				nonzero[info->coupling[i].ang]=1; 
		}
	}

	/* recover the residue into our working vectors */
	for(i=0;i<info->submaps;i++)
	{
		int ch_in_bundle	= 0;

		for(j = 0 ; j < vi->channels ; j++)
		{
			if(!info->chmuxlist || info->chmuxlist[j]==i)
			{
				if(nonzero[j])
				{
					zerobundle[ch_in_bundle]=1;
				}
				else
				{
					zerobundle[ch_in_bundle]=0;
				}
				pcmbundle[ch_in_bundle++]=vd->work[j];
			}
		}

		res_inverse(vd,ci->residue_param+info->submaplist[i].residue,pcmbundle,zerobundle,ch_in_bundle);
	}

	//for(j=0;j<vi->channels;j++)
	//_analysis_output("coupled",seq+j,vb->pcm[j],-8,n/2,0,0);

	/* channel coupling */
	for(i=info->coupling_steps-1;i>=0;i--)
	{
		ogg_int32_t *pcmM=vd->work[info->coupling[i].mag];
		ogg_int32_t *pcmA=vd->work[info->coupling[i].ang];

		for(j=0;j<n/2;j++)
		{
			ogg_int32_t mag=pcmM[j];
			ogg_int32_t ang=pcmA[j];

			if(mag>0)
			{
				if(ang>0)
				{
					pcmM[j]=mag;
					pcmA[j]=mag-ang;
				}
				else
				{
					pcmA[j]=mag;
					pcmM[j]=mag+ang;
				}
			}
			else
			{
				if(ang>0)
				{
					pcmM[j]=mag;
					pcmA[j]=mag+ang;
				}
				else
				{
					pcmA[j]=mag;
					pcmM[j]=mag-ang;
				}
			}
		}
	}

	//for(j=0;j<vi->channels;j++)
	//_analysis_output("residue",seq+j,vb->pcm[j],-8,n/2,0,0);

	/* compute and apply spectral envelope */
	for(i=0;i<vi->channels;i++)
	{
		ogg_int32_t *pcm = vd->work[i];
		int submap=0;
		int floorno;

		if(info->submaps>1)
		{
			submap=info->chmuxlist[i];
		}
		floorno=info->submaplist[submap].floor;

		/* floor 1 */
		floor1_inverse2(vd,&ci->floor_param[floorno],floormemo[i],pcm);
	}

	//for(j=0;j<vi->channels;j++)
	//_analysis_output("mdct",seq+j,vb->pcm[j],-24,n/2,0,1);

	/* transform the PCM data; takes PCM vector, vb; modifies PCM vector */
	/* only MDCT right now.... */
#if defined( WIN32 )
	// Note: Special case Win32, use SSE2 if available.
	if(vd->bSSE2)
	{
		for(i=0;i<vi->channels;i++)
		{
			mdct_backward_SSE2(n,vd->work[i]);
		}
	}
	else
	{
		for(i=0;i<vi->channels;i++)
		{
			mdct_backward(n,vd->work[i]);
		}
	}
#else
	for(i=0;i<vi->channels;i++)
	{
		mdct_backward(n,vd->work[i]);
	}
#endif // defined( WIN32 )

	//for(j=0;j<vi->channels;j++)
	//_analysis_output("imdct",seq+j,vb->pcm[j],-24,n,0,0);

	/* all done! */

#ifdef _CHECKSTACKALLOC
	Stack.Free(Size);
#endif
	return(0);
}
#endif
