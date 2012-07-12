/*	File:			
 *	Description:		main file
 *	Program:		TV Filter
 *	License:		GPL Version 2
 *	Modifications:		See CVS tree
 *	File Description:	main program
 *	Program Description:	
 *	Author:			Mikko Rantalainen <mira@st.jyu.fi>
 *
 *	Copyright (c) 2001 Mikko Rantalainen
 */

#include "grabber.h"


int gr_open(t_videosource* src)
{
	struct video_mbuf mbuf;

	/* set some values to clearly undefined ones to help debugging */
	src->hwframebase = NULL;
	src->hwframes = src->hwoffsets[0] = src->hwoffsets[1] = 0;
	src->fd = -1;

	/* open device */
	if ( (src->fd = open(src->dev,O_RDWR)) == -1 )
	{
		printf("(R)Failed to open device %s: %s\n",src->dev,strerror(errno));
		return FALSE;
	}

	if ( ioctl(src->fd,VIDIOCGMBUF,&mbuf) != 0 )
	{
		printf("(R)Failed to query memory buffer of device %s: %s\n",src->dev,strerror(errno));
		return FALSE;
	}
	if ( mbuf.frames < 2 )
	{
		printf("(R)Device %s supports only %d frames in hardware. Would need 2.\n",src->dev,mbuf.frames);
		return FALSE;
	}
	
	src->hwsize = mbuf.size;
	src->hwframes = mbuf.frames;
	src->hwoffsets[0] = mbuf.offsets[0];
	src->hwoffsets[1] = mbuf.offsets[1];

	printf("(R)Hardware: buffersize=%d offset[0]=%d offset[1]=%d (max. %d frames)\n",src->hwsize,src->hwoffsets[0],src->hwoffsets[1],src->hwframes);

	/* map */
	src->hwframebase = mmap(0,src->hwsize,PROT_READ|PROT_WRITE,MAP_SHARED,src->fd,0);
	if (src->hwframebase == MAP_FAILED)
	{
		printf("(R)Failed to mmap() device %s: %s\n",src->dev,strerror(errno));
		return FALSE;
	}
	printf("(R)mmap()ed device %s to %p.\n",src->dev,src->hwframebase);

	return TRUE;
}

int gr_set_volume(t_videosource* src,int value) /* 0 = mute, 100 = max*/
{
	struct video_audio audio;

	/* unmute audio - just get default audio config and unmute it */
	if ( ioctl(src->fd,VIDIOCGAUDIO,&audio) != 0 )
	{
		printf("(R)Failed to query audio info of device %s: %s\n",src->dev,strerror(errno));
		return FALSE;
	}
	if (value >= 50)
		audio.flags = audio.flags & (~ VIDEO_AUDIO_MUTE);	/* unmute */
	else
		audio.flags = audio.flags | VIDEO_AUDIO_MUTE;		/* mute */

	audio.mode = audio.mode | VIDEO_SOUND_STEREO;			/* stereo sound */
	if ( ioctl(src->fd,VIDIOCSAUDIO,&audio) != 0 )
	{
		printf("(R)Failed to unmute audio of device %s: %s\n",src->dev,strerror(errno));
		return FALSE;
	}
	return TRUE;
}

int gr_set_input_channel(t_videosource* src,int input) /* set input channel to input */
{
	struct video_channel vidchan;

	/* select tuner to query */
	vidchan.channel = input;
	if ( ioctl(src->fd,VIDIOCGCHAN,&vidchan) != 0 )
	{
		printf("(R)Failed to query input channel of device %s: %s\n",src->dev,strerror(errno));
		return FALSE;
	}

	/* select just queried channel as active */
	if ( ioctl(src->fd,VIDIOCSCHAN,&vidchan) != 0 )
	{
		printf("(R)Failed to set input channel of device %s: %s\n",src->dev,strerror(errno));
		return FALSE;
	}
	return TRUE;
}

int gr_set_tuner(t_videosource* src,int tuner)
{
	struct video_tuner tvtuner;
	
	/* select tuner to query */
	tvtuner.tuner = tuner;
	if ( ioctl(src->fd,VIDIOCGTUNER,&tvtuner) != 0 )
	{
		printf("(R)Failed to query tuner info of device %s: %s\n",src->dev,strerror(errno));
		return FALSE;
	}

	/* select just queried tuner as active */
	if ( ioctl(src->fd,VIDIOCSTUNER,&tvtuner) != 0 )
	{
		printf("(R)Failed to set tuner of device %s: %s\n",src->dev,strerror(errno));
		return FALSE;
	}
	return TRUE;
}

int gr_set_frequency(t_videosource* src,ulong freq) /* freq in Hz */
{
	ulong ifreq;
	ifreq = hz2int(freq);
	
	if (freq == 0)
	{
		printf("(R)Frequency is zero. Tuning will be skipped.\n");
		return TRUE;
	}
	
	/* setup tuner first -- shouldn't be needed */
	/* gr_set_tuner(src,0); */
	
	printf("(R)Setting frequency to %lu Hz\n",freq);
	/* setup frequency */
	if ( ioctl(src->fd,VIDIOCSFREQ,&ifreq) != 0 )
	{
		printf("(R)Failed to query frequency info of device %s: %s\n",src->dev,strerror(errno));
		return FALSE;
	}
	src->frequency = freq;
	return TRUE;
}

int gr_begin_capture(t_videosource* src)
{
	/* ok - we have mapped the memory. Start capturing... */
	
	src->capture.frame = 0; /* capture frame 0 - we'll capture frame 1 later... */
#if 1
	src->capture.width = GR_WIDTH;
	src->capture.height = GR_HEIGHT;
#else
	src->capture.width = 768;
	src->capture.height = 576;
#endif
	src->capture.format = VIDEO_PALETTE_YUV422;
	if ( ioctl(src->fd,VIDIOCMCAPTURE,&(src->capture)) != 0 )
	{
		printf("(R)Failed to capture frame from device %s: %s\n",src->dev,strerror(errno));
		return FALSE;
	}

	printf("(R)Started capture\n");

	src->frame_count = 0;
	src->time_start = src->time_last = src->time_capture_end = SDL_GetTicks();
	return TRUE;
}

int gr_fetch_frame_as_yv12(t_videosource* src,SDL_Overlay* buffer,SDL_Overlay* history, int test)
{
	unsigned char* captured_frame = src->frame;
	int x,y;
	unsigned char* s; /* source pointer */
	unsigned char* s2; /* source pointer number 2 (for U/V-filtering)*/
	unsigned char* t; /* target pointer */
	unsigned char* p; /* history (previous) pointer */
	
	
	if (buffer->format != SDL_YV12_OVERLAY)
	{
		fprintf(stderr,"%s:%d: only SDL_YV12_OVERLAY supported.\n",__FILE__,__LINE__);
		return FALSE;
	}
	/*if (buffer->planes != 3 || buffer->pitches[1] != buffer->pitches[0] / 2 || buffer->pitches[1] != buffer->pitches[2])*/
	if (buffer->planes != 3 || buffer->pitches[1] != buffer->pitches[2])
	{
		fprintf(stderr,"%s:%d: planes or pitch doesn't match format YV12.\n",__FILE__,__LINE__);
		fprintf(stderr,"%s:%d: pl=%d p1=%d p0/2=%d p2=%d\n",__FILE__,__LINE__,buffer->planes,buffer->pitches[1],buffer->pitches[0] / 2,buffer->pitches[2]);
		return FALSE;
	}

	/* convert YUV422 to YV12: */
	/* Y */
	for (y = 0; y < GR_HEIGHT; y++)
	{
		s = captured_frame + (GR_WIDTH*y)*2; /* source to start of line, 2 bytes per pixel */
		t = buffer->pixels[0] + (buffer->pitches[0]*y); /* target to start of line */
		p = history->pixels[0] + (history->pitches[0]*y); /* target to start of line */
		for (x = 0; x < GR_WIDTH; x++)
		{
			*p = *t = *s;
			s+=2; /*each source Y-pixel is 2 bytes */
			t+=1; /*each target pixel is 1 bytes */
			p+=1;
		}
	}
	/* V */
	for (y = 0; y < GR_HEIGHT/2; y++)
	{
		s  = captured_frame + (GR_WIDTH*(y*2))*2 + 3;     /* source to start of line, 2 bytes per pixel, V-component in 2nd pixel */
		s2 = captured_frame + (GR_WIDTH*((y+1)*2))*2 + 3; /* source to start of line+1, 2 bytes per pixel, V-component in 2nd pixel */
		t = buffer->pixels[1] + (buffer->pitches[1]*y); /* target to start of line */
		for (x = 0; x < GR_WIDTH/2; x++)
		{
			*t = (*s + *s2 + 1) >> 1; /* avg of 2 V-values*/
			s+=4;	/*each source V-pixel is 4 bytes */
			s2+=4;	/*each source V-pixel is 4 bytes */
			t+=1;	/*each target pixel is 1 bytes */
		}
	}
	/* U */
	for (y = 0; y < GR_HEIGHT/2; y++)
	{
		s  = captured_frame + (GR_WIDTH*(y*2))*2 + 1;     /* source to start of line, 2 bytes per pixel, U-component in first pixel */
		s2 = captured_frame + (GR_WIDTH*((y+1)*2))*2 + 1; /* source to start of line+1, 2 bytes per pixel, U-component in 2nd pixel */
		t = buffer->pixels[2] + (buffer->pitches[2]*y); /* target to start of line */
		for (x = 0; x < GR_WIDTH/2; x++)
		{
			*t = (*s + *s2 + 1) >> 1; /* avg of 2 V-values*/
			s+=4;   /*each source U-pixel is 4 bytes */
			s2+=4;	/*each source U-pixel is 4 bytes */
			t+=1;   /*each target pixel is 1 bytes */
		}
	}
	return TRUE;
}

/* returns TRUE if image is interlaced, FALSE otherwise. Doesn't change anything in frame. Parameter 'y' tells which line to check. */
int gr_detect_interlacing_in_yv12(t_videosource* src,SDL_Overlay* buffer,SDL_Overlay* history,int checky)
{
	unsigned char* captured_frame = src->frame;
	int x,y;

	int diff0;
	int diff1;
	int diff;
	
	int pixelcount = 0;
	int interlacecount = 0;
	int interlaced = 0;

/* for Y-plane filtering/copy */
	unsigned char* sp0; /* source plus zero */
	unsigned char* sp1; /* source plus one */
	unsigned char* sp2; /* source plus two -- for deinterlacing */

	unsigned char* pp0; /* prev plus zero */
	unsigned char* pp1; /* prev plus one */

	/* optimizations because compiler doesn't do good enough results we have to buffer values read from pointers ourselves */
	unsigned char sp0b;
	unsigned char sp1b;
	unsigned char sp2b;
	/* no use to buffer targets because we have to write result anyway sometimes! */
	unsigned char pp0b;
	unsigned char pp1b;
	
	if (buffer->format != SDL_YV12_OVERLAY)
	{
		fprintf(stderr,"%s:%d: only SDL_YV12_OVERLAY supported.\n",__FILE__,__LINE__);
		return FALSE;
	}
	/*if (buffer->planes != 3 || buffer->pitches[1] != buffer->pitches[0] / 2 || buffer->pitches[1] != buffer->pitches[2])*/
	if (buffer->planes != 3 || buffer->pitches[1] != buffer->pitches[2])
	{
		fprintf(stderr,"%s:%d: planes or pitch doesn't match format YV12.\n",__FILE__,__LINE__);
		fprintf(stderr,"%s:%d: pl=%d p1=%d p0/2=%d p2=%d\n",__FILE__,__LINE__,buffer->planes,buffer->pitches[1],buffer->pitches[0] / 2,buffer->pitches[2]);
		return FALSE;
	}

/* -------------------------------------------------
convert YUV422 to YV12:
------------------------------------------------- */

	/* Y -- deinterlace during copy... */

	/* run two lines at once - deinterlace odd lines */
	for (y = checky; y < checky+4; y+=2) /* we have to ignore last line because there's no line below it to blend with when deinterlacing. */
	{
		/* pointers to starts of lines in {source, target and previous} frame */
		sp0 = captured_frame + (GR_WIDTH*y)*2;		
		sp1 = captured_frame + (GR_WIDTH*(y+1))*2;
		sp2 = captured_frame + (GR_WIDTH*(y+2))*2;

		pp0 = history->pixels[0] + (history->pitches[0]*y);
		pp1 = history->pixels[0] + (history->pitches[0]*(y+1));

		/* NORMAL VERSION */
		for (x = 0; x < GR_WIDTH; x++)
		{
/* set whether or not to filter Y-plane with cost of accuracy */
#define GR_FILTER_Y 0
			pp0b = *pp0;
			pp1b = *pp1;
			sp0b = *sp0;
			sp1b = *sp1;
			sp2b = *sp2;

			/* compute odd line pixel */

			diff  = sp0b - sp1b; /* difference between odd and even lines */
			diff0 = pp1b - sp1b;
			diff1 = pp0b - sp0b;
			diff  = diff*diff;
			diff0 = diff0*diff0;
			diff1 = diff1*diff1;


			if (diff0 <= GR_DEINTERLACE_THRESHOLD*GR_DEINTERLACE_THRESHOLD &&
			    diff1 <= GR_DEINTERLACE_THRESHOLD*GR_DEINTERLACE_THRESHOLD )
			{
				/* no motion - do nothing */
			}
			else
			{
				if ( diff > GR_DEINTERLACE_THRESHOLD*GR_DEINTERLACE_THRESHOLD )
				{
					interlacecount++; /* motion within interlaced image */
					//if (SDL_GetTicks() > 2000) *sp1 = 255;
				}
				pixelcount++;
				
			}

			/* update pointers */
			/* each pixel is 2 bytes in source, 1 byte elsewhere */
			sp0+=2;
			sp1+=2;
			sp2+=2;

			pp0+=1;
			pp1+=1;
		} /* end of NORMAL VERSION */
	}


	interlaced = 0;
	if (pixelcount > 20)
	{
		if (interlacecount*3 > pixelcount)
			interlaced = 1;
		if (pixelcount < 200 && interlacecount > 20)
			interlaced = 1;
	}
	else
	{
		if (interlacecount > 10)
			interlaced = 1;
	}
		

	D printf("pixelcount=%-5d interlacecount=%-5d interlaced=%s\n",pixelcount,interlacecount,interlaced ? "yes" : "no");

	return interlaced;
}



int gr_fetch_deinterlaced_frame_as_yv12(t_videosource* src,SDL_Overlay* buffer,SDL_Overlay* history,int test)
{
/* set whether or not to filter Y-plane with cost of accuracy */
#define GR_FILTER_Y 0

	unsigned char* captured_frame = src->frame;
	int x,y;

	register int diff0;
	register int diff1;
	register int diff2;

/* for Y-plane filtering/copy */
	register unsigned char* sp0; /* source plus zero */
	register unsigned char* sp1; /* source plus one */
	register unsigned char* sp2; /* source plus two -- for deinterlacing */

	register unsigned char* tp0; /* target plus zero */
	register unsigned char* tp1; /* target plus one */

	register unsigned char* pp0; /* previous plus zero */
	register unsigned char* pp1; /* previous plus one */
	
	/* optimizations because compiler doesn't do good enough results we have to buffer values read from pointers ourselves */
	register unsigned char sp0b;
	register unsigned char sp1b;
	register unsigned char sp2b;
	/* no use to buffer targets because we have to write result anyway sometimes! */
	register unsigned char pp0b;
	register unsigned char pp1b;
	
	/* filter Y-plane */
#if GR_FILTER_Y
	register unsigned char prev0 = 0;
	register unsigned char prev1 = 0;
#endif /* GR_FILTER_Y */
	
/* for U/V-plane filtering/copy */
#if 1
	register unsigned char* s; /* source pointer */
	register unsigned char* s2; /* source pointer number 2 (for U/V-filtering) -- points to pixel below current one */
	register unsigned char* t; /* source pointer */
#endif
	if (buffer->format != SDL_YV12_OVERLAY)
	{
		fprintf(stderr,"%s:%d: only SDL_YV12_OVERLAY supported.\n",__FILE__,__LINE__);
		return FALSE;
	}
	/*if (buffer->planes != 3 || buffer->pitches[1] != buffer->pitches[0] / 2 || buffer->pitches[1] != buffer->pitches[2])*/
	if (buffer->planes != 3 || buffer->pitches[1] != buffer->pitches[2])
	{
		fprintf(stderr,"%s:%d: planes or pitch doesn't match format YV12.\n",__FILE__,__LINE__);
		fprintf(stderr,"%s:%d: pl=%d p1=%d p0/2=%d p2=%d\n",__FILE__,__LINE__,buffer->planes,buffer->pitches[1],buffer->pitches[0] / 2,buffer->pitches[2]);
		return FALSE;
	}

/* -------------------------------------------------
convert YUV422 to YV12:
------------------------------------------------- */

	/* Y -- deinterlace during copy... */

	/* run two lines at once - deinterlace odd lines */
	for (y = 0; y < GR_HEIGHT-2; y+=2) /* we have to ignore last line because there's no line below it to blend with when deinterlacing. */
	{
		/* pointers to starts of lines in {source, target and previous} frame */
		sp0 = captured_frame + (GR_WIDTH*y)*2;		
		sp1 = captured_frame + (GR_WIDTH*(y+1))*2;
		sp2 = captured_frame + (GR_WIDTH*(y+2))*2;

		tp0 = buffer->pixels[0] + (buffer->pitches[0]*y);
		tp1 = buffer->pixels[0] + (buffer->pitches[0]*(y+1));

		pp0 = history->pixels[0] + (history->pitches[0]*y);
		pp1 = history->pixels[0] + (history->pitches[0]*(y+1));

		/* NORMAL VERSION */
		for (x = 0; x < GR_WIDTH; x++)
		{
			pp0b = *pp0;
			pp1b = *pp1;
			sp0b = *sp0;
			sp1b = *sp1;
			sp2b = *sp2;

			/* update even line pixel to target and history */
#if GR_FILTER_Y
			/* average between value of source and previous pixel */
			prev0 = *pp0 = *tp0 = (sp0b + prev0 + 1) >> 1;
#else
			*pp0 = *tp0 = sp0b;
#endif /* GR_FILTER_Y */

			/* compute odd line pixel */

			diff0 = pp1b - sp1b;
			diff1 = pp0b - sp0b;
			diff2 = sp0b - sp1b; /* difference between odd and even lines in new frame */
			diff0 = diff0*diff0;
			diff1 = diff1*diff1;
			diff2 = diff2*diff2;

			if (diff2 < GR_DEINTERLACE_THRESHOLD*GR_DEINTERLACE_THRESHOLD ||
			    (diff0 <= GR_DEINTERLACE_THRESHOLD*GR_DEINTERLACE_THRESHOLD &&
			    diff1 <= GR_DEINTERLACE_THRESHOLD*GR_DEINTERLACE_THRESHOLD))
			{
				/* no motion - copy pixel from source */
#if GR_FILTER_Y
				prev1 = *tp1 = (sp1b + prev1 + 1) >> 1;
#else
				*tp1 = sp1b;
#endif /* GR_FILTER_Y */
				/* save real source to history */
#if GR_PREV_FRAME_HISTORY
				*pp1 = *tp1;
#else
				*pp1 = sp1b;
#endif
			}
			else
			{
				/* motion - deinterlace */
				/* interpolate */
				/* update history also */
#if GR_FILTER_Y
				prev1 = *tp1 = (prev1 + prev1 + sp0b + sp2b + 3) >> 2;
#else
				*tp1 = (sp0b + sp2b + 1) >> 1;
#endif /* GR_FILTER_Y */
				/* save real source to history */
#if GR_PREV_FRAME_HISTORY
				*pp1 = *tp1;
#else
				*pp1 = sp1b;
#endif

#if 1
				if (test)
					*tp1 = 255;
#endif
			}

			/* update pointers */
			/* each pixel is 2 bytes in source, 1 byte otherwhere */
			sp0+=2;
			sp1+=2;
			sp2+=2;

			tp0+=1;
			tp1+=1;

			pp0+=1;
			pp1+=1;
		} /* end of NORMAL VERSION */
	}
/* ---------------------------------------------- */
	/* V */
	for (y = 0; y < GR_HEIGHT/2; y++)
	{
		s  = captured_frame + (GR_WIDTH*(y*2))*2 + 3;     /* source to start of line, 2 bytes per pixel, V-component in 2nd pixel */
		s2 = captured_frame + (GR_WIDTH*((y+1)*2))*2 + 3; /* source to start of line+1, 2 bytes per pixel, V-component in 2nd pixel */
		t = buffer->pixels[1] + (buffer->pitches[1]*y); /* target to start of line */
		for (x = 0; x < GR_WIDTH/2; x++)
		{
#if GR_FILTER_UV
			*t = (*s + *s2 + 1) >> 1; /* avg of 2 V-values*/
#else
			*t = *s; /* avg of 2 V-values*/
#endif
			s+=4;	/*each source V-pixel is 4 bytes */
			s2+=4;	/*each source V-pixel is 4 bytes */
			t+=1;	/*each target pixel is 1 bytes */
		}
	}
	/* U */
	for (y = 0; y < GR_HEIGHT/2; y++)
	{
		s  = captured_frame + (GR_WIDTH*(y*2))*2 + 1;     /* source to start of line, 2 bytes per pixel, U-component in first pixel */
		s2 = captured_frame + (GR_WIDTH*((y+1)*2))*2 + 1; /* source to start of line+1, 2 bytes per pixel, U-component in 2nd pixel */
		t = buffer->pixels[2] + (buffer->pitches[2]*y); /* target to start of line */
		for (x = 0; x < GR_WIDTH/2; x++)
		{
#if GR_FILTER_UV
			*t = (*s + *s2 + 1) >> 1; /* avg of 2 U-values*/
#else
			*t = *s; /* avg of 2 U-values*/
#endif
			s+=4;   /*each source U-pixel is 4 bytes */
			s2+=4;	/*each source U-pixel is 4 bytes */
			t+=1;   /*each target pixel is 1 bytes */
		}
	}
/* ---------------------------------------------- */
	return TRUE;
}

int gr_sync_next(t_videosource* src)
{
	Uint32 delay;
	Uint32 capturedelay;
	Uint32 avgdelay = 0;
	Uint32 processingdelay;
	Uint32 fps;
	Uint32 avgfps;
	Uint32 start_of_capture;
	int frame;

	D start_of_capture = SDL_GetTicks();
	D processingdelay = SDL_GetTicks() - src->time_capture_end;

	/* we need to tell driver to start capturing next frame before reading current one */

	/* save current frame number */
	frame = src->capture.frame;

	/* start capturing of next frame */
	src->capture.frame = src->capture.frame == 0 ? 1 : 0; /* from 0 to 1 and from 1 to 0 */
	if ( ioctl(src->fd,VIDIOCMCAPTURE,&(src->capture)) != 0 )
	{
		printf("(R)Failed to capture frame from device %s: %s\n",src->dev,strerror(errno));
		return FALSE;
	}
	/* wait for current frame to complete */
	if ( ioctl(src->fd,VIDIOCSYNC,&frame) )
	{
		printf("(R)Failed to sync frame from device %s: %s\n",src->dev,strerror(errno));
		return FALSE;
	}
	
	/* compute latest (hardware) frame address */
	src->frame = (src->hwframebase + src->hwoffsets[frame]);

	/* This is time we had extra (if we didn't dropped frames!) */
	D capturedelay = SDL_GetTicks() - start_of_capture;

	/* stats */
	src->frame_count++;
	delay = SDL_GetTicks() - src->time_last;
	D avgdelay =  (SDL_GetTicks() - src->time_start) / src->frame_count;
	if (avgdelay > 0 && delay > 0)
	{
		fps = 1000 / delay;
		avgfps = 1000 / avgdelay;
	}
	else
	{
		fps = 0;
		avgfps = 0;
	}
	if (delay > 50)
		printf("(GR)Dropped frame(s)? (Delay between frames: %d ms)\n",delay);
	D printf("(GR)Sync: %d ms AvgRate: %d fps User: %d ms\n",capturedelay,avgfps,processingdelay);

	src->time_last = src->time_capture_end = SDL_GetTicks();
	return TRUE;
}

int gr_end_capture(t_videosource* src)
{
	/* capture frame in progress or we get fatal errors... */
	if ( ioctl(src->fd,VIDIOCSYNC,&(src->capture.frame)) )
	{
		printf("(R)Failed to sync frame from device %s: %s\n",src->dev,strerror(errno));
		return FALSE;
	}

	printf("(R)Stopped capture\n");

	return TRUE;
}

int gr_close(t_videosource* src)
{
	/* try to make it always safe to call gr_close without fear of coredump */
	if (src->fd == -1)
	{
		fprintf(stderr,"grabber.c: gr_close(): error: device allready closed.\n");
		return FALSE;
	}


	gr_set_volume(src,0); /* turn audio off -- SHOULD WE DO IT OR LEAVE IT TO PROGRAM? */

	if (src->hwframebase != NULL)
		munmap(src->hwframebase,src->hwsize);
	close(src->fd); /* close device*/

	src->fd = -1;
	return TRUE;
}

int gr_get_info(t_videosource* src)
{
	int i;
	struct video_capability vidcap;
	struct video_tuner vidtun;
	struct video_channel vidchan;
	struct video_audio audio;
	ulong freq;
	
	/* query capabilities */
	if ( ioctl(src->fd,VIDIOCGCAP,&vidcap) != 0 )
	{
		printf("(R)Failed to query capabilities of device %s: %s\n",src->dev,strerror(errno));
		return FALSE;
	}

	printf("\nDevice information:\n-------------------\n");	
	printf("Device              %s\n",src->dev);
	printf("Name                %s\n",vidcap.name);
	printf("Type                0x%08X\n",vidcap.type);
	printf("Input Channels      %d\n",vidcap.channels);
	printf("Audio Devices       %d\n",vidcap.audios);
	printf("Max Width  (px)     %d\n",vidcap.maxwidth);
	printf("Max Height (px)     %d\n",vidcap.maxheight);

	if ( vidcap.type & (VIDEO_TUNER_PAL | VIDEO_TUNER_NTSC) )
	{
		printf("\nTuner Information:\n");
		vidtun.tuner = 0; /* select tuner - I think most cards have only one */
		if ( ioctl(src->fd,VIDIOCGTUNER,&vidtun) != 0 )
		{
			printf("(R)Failed to query tuner info of device %s: %s\n",src->dev,strerror(errno));
			return FALSE;
		}
		
		printf("Tuner ID            %d\n",vidtun.tuner);
		printf("Name                %s\n",vidtun.name);
		printf("Lowest Freg  (Hz)   %ld\n",int2hz(vidtun.rangelow));
		printf("Highest Freq (Hz)   %ld\n",int2hz(vidtun.rangehigh));
		printf("Flags               0x%08X\n",vidtun.flags);
		printf("Mode                0x%08X\n",vidtun.mode);
	}

	printf("\nInput Channel Information:\n");
	for (i=0; i<vidcap.channels; i++)
	{
		vidchan.channel = i; /* select tuner */
		if ( ioctl(src->fd,VIDIOCGCHAN,&vidchan) != 0 )
		{
			printf("(R)Failed to query tuner info of device %s: %s\n",src->dev,strerror(errno));
			return FALSE;
		}
		printf("[%d] Channel ID      %d\n",i,vidchan.channel);
		printf("[%d] Name            %s\n",i,vidchan.name);
	}		


	/* audio: */
	if ( ioctl(src->fd,VIDIOCGAUDIO,&audio) != 0 )
	{
		printf("(R)Failed to query audio info of device %s: %s\n",src->dev,strerror(errno));
		return FALSE;
	}

	printf("\nAudio information:\n");	
	printf("Device ID           %d\n",audio.audio);
	printf("Name                %s\n",audio.name);
	printf("Volume              %d\n",audio.volume);
	printf("Bass                %d\n",audio.bass);
	printf("Treble              %d\n",audio.treble);
	printf("Balance             %d\n",audio.balance);
	printf("Flags               0x%08X\n",audio.flags);
	printf("Mode                0x%08X\n",audio.mode);
	printf("Step                %d\n",audio.step);
	printf("\n");
	printf("Audio is%s mute (%s).\n",(audio.flags & VIDEO_AUDIO_MUTE) ? "" : " not", (audio.flags & VIDEO_SOUND_STEREO) ? "stereo" : "mono");
	if ( ioctl(src->fd,VIDIOCGFREQ,&freq) != 0 )
	{
		printf("(R)Failed to query frequency info of device %s: %s\n",src->dev,strerror(errno));
		return FALSE;
	}
	printf("Currently Selected Frequency is %lu Hz.\n",int2hz(freq));

	
	return TRUE;
}

