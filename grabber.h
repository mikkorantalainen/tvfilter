/* 	
 *	File:			tvf.h
 *	Description:		main header file
 *	Program:		TV Filter
 *	License:		GPL Version 2
 *	Modifications:		See CVS tree
 *	File Description:	
 *	Program Description:	
 *	Author:			Mikko Rantalainen <mira@st.jyu.fi>
 *
 *	Copyright (c) 2001-2003 Mikko Rantalainen
 */

#ifndef GRABBER_H
#define GRABBER_H

#include <stdio.h>
#include <SDL.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/videodev.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <math.h>

/***** config ***************************************************************/

/* set whether or not to filter UV to reduce noise */
#define GR_FILTER_UV 1

#define GR_FULL			1	/* grab and filter full resolution? set to 0 if dropping frames */
#define GR_RGB_GIVES_BGR	1	/* fix buggy drivers that return bytes in wrong order */

/* Filters: */
#define GR_NR			1	/* use CPU noise reduction ? */
#define GR_NR_HQ		1	/* set to 1 for high quality noise filtering (CPU!) */
#define GR_NR_THRESHOLD		16	/* noise reduction - increase if you see noise */

#define GR_DEINTERLACE		GR_FULL	/* there's no point trying to deinterlace if you dont grap full height */
#define GR_DEINTERLACE_HQ	1	/* high quality de-interlacing (CPU!) */
#define GR_DEINTERLACE_THRESHOLD 9	/* increase if you see noise */

#define GR_PREV_FRAME_HISTORY	0	/* use filtered frame as history instead of real source */

/***** /config **************************************************************/


/* be quiet during checking */

#ifdef CHECK
#define printf if (0) printf
#endif

/* debug */

#ifdef DEBUG
#define D	if (1)
#else
#define	D	if (0)
#endif


#define MIN(a,b)	((a)<(b)?(a):(b))
#define MAX(a,b)	((a)>(b)?(a):(b))
#define CLAMP(x,a,b)	MAX(MIN(x,(b)),(a))
#define ABS(a)		((a)<0?(-(a)):(a))
#define HERE		g_printerr("%s:%d: here.\n",__FILE__,__LINE__)

#if GR_RGB_GIVES_BGR
	#define GR_R			2
	#define GR_G			1
	#define GR_B			0
#else
	#define GR_R			0
	#define GR_G			1
	#define GR_B			2
#endif

/* one step in frequency presented in ints is 62500 hz */
#define int2hz(i)	((ulong)i*62500)
#define hz2int(i)	((ulong)i/62500)

#if GR_FULL
	#define GR_WIDTH	512
	#define GR_HEIGHT	576	/* must be dividable by 2 */
#else   /* grab half height */
	#define GR_WIDTH	768	/* 192,384,768 for PAL */
	#define GR_HEIGHT	288	/* 144,288,576 for PAL */
#endif

#if GR_HEIGHT < 576 && GR_DEINTERLACE
#warning No point to deinterlace without full frame (PAL)
#endif

#ifndef TRUE
	#define TRUE	1
#endif
#ifndef FALSE
	#define FALSE	0
#endif

typedef struct
{
	/****************************************************
	public fields:
	****************************************************/

	char* dev;
	ulong frequency; /* in Hz */
	Uint32	frame_count; /* read only */


	/****************************************************
	for internal use:
	****************************************************/

	Uint32	time_start;
	Uint32	time_last;
	Uint32	time_capture_end;

	int fd;				/* file descriptor for device */
	unsigned char* frame;		/* pointer to last captured frame (YUV422) */
	unsigned char* hwframebase;	/* pointer to mmap()ed data */
	int hwsize;			/* hardware buffer size */
	int hwframes;			/* hardware frame count */
	int hwoffsets[2];		/* hardware frame offsets */

	struct video_mmap capture;	/* frame size & format w/ mmap()ed capture */
	/* struct video_mmap
	{
		unsigned	int frame;		-- Frame (0 - n) for double buffer
		int		height,width;
		unsigned	int format;		-- should be VIDEO_PALETTE_*
	}; */
} t_videosource;


int gr_open(t_videosource* src);
int gr_set_volume(t_videosource* src,int value); /* 0 = mute, 100 = max*/
int gr_set_input_channel(t_videosource* src,int input); /* set input channel to input */
int gr_set_picture(t_videosource* src,float brightness, float contrast, float colour, float hue, float whiteness);
int gr_set_frequency(t_videosource* src,ulong freq); /* freq in Hz */
int gr_begin_capture(t_videosource* src);
int gr_sync_next(t_videosource* src);
int gr_detect_interlacing_in_yv12(t_videosource* src,SDL_Overlay* buffer,SDL_Overlay* history,int checky);
int gr_fetch_frame_as_yv12(t_videosource* src,SDL_Overlay* buffer,SDL_Overlay* history, int test);
int gr_fetch_deinterlaced_frame_as_yv12(t_videosource* src,SDL_Overlay* buffer,SDL_Overlay* buffer2,SDL_Overlay* history, int test);
int gr_end_capture(t_videosource* src);
int gr_close(t_videosource* src);
int gr_get_info(t_videosource* src);
int gr_set_tuner(t_videosource* src,int tuner);

#endif /* GRABBER_H */
