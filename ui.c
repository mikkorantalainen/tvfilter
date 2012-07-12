/* 	
 *	File:			interface.c
 *	Description:		User Interface related functions
 *	Program:		
 *	License:		GPL Version 2
 *	Modifications:		See CVS tree
 *	File Description:	
 *	Program Description:	
 *	Author:			Mikko Rantalainen <mira@st.jyu.fi>
 *
 *	Copyright (c) 2001-2003 Mikko Rantalainen
 */

#include "tvf.h"
#include "ui.h"
#include "grabber.h"
#include "SDL_syswm.h" /* for window resize & position -- X11 support only! */
#include <unistd.h> /* execlp() */
#include <stdio.h> /* perror() */

/* oversizing with fullscreen (per side), set 0 if there's any problems */
#define OVERSIZEX (16) /* horizontal overscan in pixels */
#define OVERSIZEY (0) /* vertical overscan in pixels */
#define OVERSIZEXBALANCE (14) /* set to zero for even sized borders */
#define OVERSIZEYBALANCE (0)  /* and negative to move towards right */

/* frequencey and input */
#define NO_CHANNELS 11
unsigned long int channels[NO_CHANNELS][2] =
{
	{196250000,0}, /* YLE1 */
	{210250000,0}, /* YLE2 */
	{182250000,0}, /* MTV3 */
	{189250000,0}, /* Nelonen */
	{266250000,0}, /* MTV */
	{259250000,0}, /* TVTV */
	{245250000,0}, /* Eurosport */
	{273250000,0}, /* MoonTV */
	{0,1}, /* Composite1 */
	{0,3}, /* Composite3 */
	{0,2}, /* S-Video */
};
static int channelindex = -1;


/* add SDL_OPENGL to flags if OpenGL is to be used! */
#if 1
Uint32 sdl_screen_flags = SDL_HWSURFACE|SDL_DOUBLEBUF|SDL_ANYFORMAT|SDL_RESIZABLE|SDL_HWACCEL;
#else
Uint32 sdl_screen_flags = SDL_HWSURFACE|SDL_DOUBLEBUF|SDL_ANYFORMAT|SDL_RESIZABLE;
#endif

void window_to_fullscreen(SDL_Surface *screen)
{
#ifdef unix
	SDL_SysWMinfo info;

	SDL_VERSION(&info.version);
	if ( SDL_GetWMInfo(&info) > 0 )
	{
		int x, y;
		int w, h;
		if ( info.subsystem == SDL_SYSWM_X11 )
		{
			info.info.x11.lock_func();
			w = DisplayWidth(info.info.x11.display,
					 DefaultScreen(info.info.x11.display));
			h = DisplayHeight(info.info.x11.display,
					 DefaultScreen(info.info.x11.display));
			x = (w - screen->w)/2;
			y = (h - screen->h)/2;

			/* make window fill the screen */
			/* XMoveResizeWindow(info.info.x11.display, info.info.x11.wmwindow, 0-OVERSIZE,0-OVERSIZE, w+OVERSIZE,h+OVERSIZE); */
			XMoveResizeWindow(info.info.x11.display, info.info.x11.wmwindow, -OVERSIZEX+OVERSIZEXBALANCE,-OVERSIZEY+OVERSIZEYBALANCE, w+OVERSIZEX*2,h+OVERSIZEY*2);

			info.info.x11.unlock_func();
		}
		else
		{
			printf("Only X11 subsystem window positioning is supported.\n");
		}
	}
#else
#warning Need to implement this function for system != X11
#endif /* unix */
	SDL_ShowCursor(SDL_DISABLE); /* hide mouse cursor */
}

void fullscreen_to_window(SDL_Surface *screen)
{
#ifdef unix
	SDL_SysWMinfo info;


	SDL_VERSION(&info.version);
	if ( SDL_GetWMInfo(&info) > 0 )
	{
		int x, y;
		int w, h;
		if ( info.subsystem == SDL_SYSWM_X11 )
		{
			status.width = GR_HEIGHT*4/3 /*GR_WIDTH*/;
			status.height = GR_HEIGHT;

			info.info.x11.lock_func();
			w = DisplayWidth(info.info.x11.display,
					 DefaultScreen(info.info.x11.display));
			h = DisplayHeight(info.info.x11.display,
					 DefaultScreen(info.info.x11.display));
			x = (w - status.width)/2;
			y = (h - status.height)/2;
#if 1
			/* center window */
			XMoveResizeWindow(info.info.x11.display, info.info.x11.wmwindow, x, y, status.width, status.height);
#else
			/* make window fill the screen */
			XMoveResizeWindow(info.info.x11.display, info.info.x11.wmwindow, 0,0, w,h);
#endif
			info.info.x11.unlock_func();
		}
		else
		{
			printf("Only X11 subsystem window positioning is supported.\n");
		}
	}
#else
#warning Need to implement this function for system != X11
#endif /* unix */
	SDL_ShowCursor(SDL_ENABLE); /* hide mouse cursor */
}


SDL_Surface* screen_resize(int w,int h,int fullscreen)
{
	SDL_Surface* screen;

	status.width = w;
	status.height = h;

	if (fullscreen)
		sdl_screen_flags |= SDL_FULLSCREEN;
	else
		sdl_screen_flags &= (~SDL_FULLSCREEN);

	screen = SDL_SetVideoMode(status.width, status.height, 16, sdl_screen_flags);
	return screen;
}

void handle_key(SDL_Surface* screen,SDL_Overlay* overlay,t_videosource* vsrc, SDLKey key)
{
	if ( key == SDLK_ESCAPE )
	{
		status.shutdown = 1;
	}
	if ( key == SDLK_f )
	{
#if 0
		/* set fullscreen mode */
		SDL_Delay(100);
		screen = screen_resize(status.width,status.height,1);
		SDL_Delay(100);
#else
/*
		printf("%s:%d: FIXME! Full screen doesn't work during capture!\n",__FILE__,__LINE__);
*/
		window_to_fullscreen(screen);
#endif
	}
	if ( key == SDLK_w )
	{
#if 0
		/* set windowed mode */
		screen = screen_resize(status.width,status.height,0);
#else
		fullscreen_to_window(screen);
#endif
	}
	if ( key == SDLK_a )
	{
		/* show deinterlaced frames */
		status.autodeinterlace = 1;
		status.debug = 0;
	}
	if ( key == SDLK_d )
	{
		/* show deinterlaced frames */
		status.autodeinterlace = 0;
		status.deinterlace = 1;
		status.debug = 0;
	}
	if ( key == SDLK_t )
	{
		/* show deinterlaced frames */
		status.autodeinterlace = 0;
		status.deinterlace = 1;
		status.debug = 1;
	}
	if ( key == SDLK_i )
	{
		/* show interlaced frames */
		status.autodeinterlace = 0;
		status.deinterlace = 0;
		status.debug = 0;
	}
	if ( key == SDLK_c )
	{
		/* toggle capture */
		status.showvideo = status.showvideo ? 0 : 1;
	}
	if ( key == SDLK_m )
	{
		/* start teletext (alevt) */
		if (fork() == 0)
		{
			execlp("alevt","alevt","-geometry","41x25+0+0",NULL);
			perror("fork() and execlp('alevt') failed");
			exit(2);
		}
	}
	if ( key == SDLK_q )
	{
		status.shutdown = 1;
	}
	if ( key == SDLK_UP )
	{
		status.showvideo = 1;
	}
	if ( key == SDLK_DOWN )
	{
		status.showvideo = 0;
	}
	if ( key == SDLK_LEFT )
	{
		status.showvideo = 0;
	}
	if ( key == SDLK_RIGHT )
	{
		status.showvideo = 1;
	}
	if ( key == SDLK_KP_PLUS || key == SDLK_PLUS )
	{
		channelindex++;
		if (channelindex >= NO_CHANNELS)
			channelindex = 0;
		printf("kp+ %d\n",channelindex);
		gr_set_input_channel(vsrc,channels[channelindex][1]);
		gr_set_frequency(vsrc,channels[channelindex][0]);
		gr_set_volume(vsrc,100); /* turn volume to 100% */
	}
	if ( key == SDLK_KP_MINUS || key == SDLK_MINUS )
	{
		channelindex--;
		if (channelindex < 0)
			channelindex = NO_CHANNELS-1;
		printf("kp- %d\n",channelindex);
		gr_set_input_channel(vsrc,channels[channelindex][1]);
		gr_set_frequency(vsrc,channels[channelindex][0]);
		gr_set_volume(vsrc,100); /* turn volume to 100% */
	}
	/*if ( SDL_GetModState() & KMOD_SHIFT )*/
	
}

void ui_schedule(SDL_Surface* screen,SDL_Overlay* overlay,t_videosource* vsrc)
{
	SDL_Event event;
	while ( SDL_PollEvent(&event) )
	{
		switch(event.type)
		{
			case SDL_VIDEORESIZE: /* window size has been changed */
				screen = screen_resize(event.resize.w, event.resize.h,screen->flags  & SDL_FULLSCREEN ? 1 : 0);
			break;
			case SDL_KEYDOWN:
				printf("key %d down\n",event.key.keysym.sym);
				handle_key(screen,overlay,vsrc,event.key.keysym.sym);
			break;
			case SDL_KEYUP:
				printf("key %d up\n",event.key.keysym.sym);
			break;
			case SDL_MOUSEMOTION:
				/* printf ("mouse to (%i, %i)\n", event.motion.x, event.motion.y); */
			break;
			case SDL_MOUSEBUTTONDOWN:
				printf("mouse button %d down\n",event.button.button);
			break;
			case SDL_MOUSEBUTTONUP:
				printf("mouse button %d up\n",event.button.button);
			break;
			case SDL_QUIT: /* window has been closed */
				status.shutdown = 1;
			break;
		}
	}
}

/*
	allocate space for SDL_Overlay of format YV12
	returns NULL if something goes wrong
*/
SDL_Overlay* new_yv12_buffer(void)
{
	int sizeO;
	int sizeY;
	int sizeV;
	int sizeU;
	int sizePX;
	int sizePI;
	
	SDL_Overlay* o;
	
	sizeO = sizeof(SDL_Overlay); /* single overlay */
	sizeY = sizeof(unsigned char)*GR_WIDTH*GR_HEIGHT;
	sizeV = sizeU = sizeof(unsigned char)*(GR_WIDTH/2)*(GR_HEIGHT/2);
	sizePX = sizeof(unsigned char*)*3;
	sizePI = sizeof(unsigned short)*3;

	/* ok - get all memory required... */
	
	o = (SDL_Overlay*)malloc(sizeO);
	if (o == NULL)
		return NULL;
	o->pixels = (unsigned char**)malloc(sizePX);
	o->pitches = (unsigned short*)malloc(sizePI);
	if (o->pixels == NULL || o->pitches == NULL)
		return NULL;

	o->w = GR_WIDTH;
	o->h = GR_HEIGHT;
	o->pixels[0] = (unsigned char*)malloc(sizeY);
	o->pixels[1] = (unsigned char*)malloc(sizeV);
	o->pixels[2] = (unsigned char*)malloc(sizeU);
	o->pitches[0] = GR_WIDTH;
	o->pitches[1] = GR_WIDTH/2;
	o->pitches[2] = GR_WIDTH/2;
	o->planes = 3;
	o->format = SDL_YV12_OVERLAY;
	if (o->pixels[0] == NULL || o->pixels[1] == NULL || o->pixels[2] == NULL)
		return NULL;
	D printf("allocated buffer o=%p\n",o);

	return o;
}

int delete_yv12_buffer(SDL_Overlay* buffer)
{
	SDL_Overlay* o = buffer;
	/* free allocated memory */
	D printf("freeing buffer o=%p\n",o);
	free(o->pixels[0]);
	free(o->pixels[1]);
	free(o->pixels[2]);
	free(o->pixels);
	free(o->pitches);
	free(o);
	return TRUE;
}

int clear_yv12_buffer(SDL_Overlay* buffer)
{
	SDL_Overlay* o = buffer;
	int x,y;

	/* Y */
	for (y = 0; y < o->h; y++)
		for (x = 0; x < o->w; x++)
			*(o->pixels[0] + o->pitches[0]*y + x) = 0;
	/* V */
	for (y = 0; y < o->h/2; y++)
		for (x = 0; x < o->w/2; x++)
			*(o->pixels[1] + o->pitches[1]*y + x) = 128;
	/* U */
	for (y = 0; y < o->h/2; y++)
		for (x = 0; x < o->w/2; x++)
			*(o->pixels[2] + o->pitches[2]*y + x) = 128;
	return TRUE;
}

int clearborder_yv12_buffer(SDL_Overlay* buffer,unsigned int sizex,unsigned int sizey)
{
	SDL_Overlay* o = buffer;
	int x,y;

	/* Y top */
	for (y = 0; y < sizey; y++)
		for (x = 0; x < o->w; x++)
			*(o->pixels[0] + o->pitches[0]*y + x) = 0;
	/* Y right */
	for (y = sizey; y < o->h; y++)
		for (x = o->w - sizex; x < o->w; x++)
			*(o->pixels[0] + o->pitches[0]*y + x) = 0;
	/* Y bottom */
	for (y = o->h - sizey; y < o->h; y++)
		for (x = 0; x < o->w - sizex; x++)
			*(o->pixels[0] + o->pitches[0]*y + x) = 0;
	/* Y left */
	for (y = sizey; y < o->h - sizey; y++)
		for (x = 0; x < sizex; x++)
			*(o->pixels[0] + o->pitches[0]*y + x) = 0;



	/* V top */
	for (y = 0/2; y < sizey/2; y++)
		for (x = 0/2; x < o->w/2; x++)
			*(o->pixels[1] + o->pitches[1]*y + x) = 128;
	/* V right */
	for (y = sizey/2; y < o->h/2; y++)
		for (x = (o->w - sizex)/2; x < o->w/2; x++)
			*(o->pixels[1] + o->pitches[1]*y + x) = 128;
	/* V bottom */
	for (y = (o->h - sizey)/2; y < o->h/2; y++)
		for (x = 0/2; x < (o->w - sizex)/2; x++)
			*(o->pixels[1] + o->pitches[1]*y + x) = 128;
	/* V left */
	for (y = sizey/2; y < (o->h - sizey)/2; y++)
		for (x = 0/2; x < sizex/2; x++)
			*(o->pixels[1] + o->pitches[1]*y + x) = 128;



	/* U top */
	for (y = 0/2; y < sizey/2; y++)
		for (x = 0/2; x < o->w/2; x++)
			*(o->pixels[2] + o->pitches[2]*y + x) = 128;
	/* U right */
	for (y = sizey/2; y < o->h/2; y++)
		for (x = (o->w - sizex)/2; x < o->w/2; x++)
			*(o->pixels[2] + o->pitches[2]*y + x) = 128;
	/* U bottom */
	for (y = (o->h - sizey)/2; y < o->h/2; y++)
		for (x = 0/2; x < (o->w - sizex)/2; x++)
			*(o->pixels[2] + o->pitches[2]*y + x) = 128;
	/* U left */
	for (y = sizey/2; y < (o->h - sizey)/2; y++)
		for (x = 0/2; x < sizex/2; x++)
			*(o->pixels[2] + o->pitches[2]*y + x) = 128;



	return TRUE;
}



int ui_main(void)
{
	SDL_Surface* screen;
	SDL_Overlay* overlay;	/* yuv overlay */
	SDL_Overlay* overlay2;	/* yuv overlay number 2 */
	SDL_Overlay* buffer;	/* yuv buffer for deinterlacing */
	Uint32 time;
	int interlaced;
	int progressive_frames = 0;
	int interlaced_frames = 0;
	int i;

	t_videosource vsrc; /* video source */
	srand(42); /* randomize */
	time = 0;

	buffer = new_yv12_buffer();
	if ( buffer == NULL )
	{
		fprintf(stderr, "Failed to allocate space for filtering.\n");
		exit(4);
	}

	SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER); /* SDL_INIT_AUDIO */

#if 0
	if (status.fullscreen)
		sdl_screen_flags |= SDL_FULLSCREEN;
#endif		
	screen = SDL_SetVideoMode(status.width, status.height, 16, sdl_screen_flags);

	if ( ! screen )
	{
		fprintf(stderr, "Couldn't set %dx%d video mode: %s\n", status.width, status.height, SDL_GetError());
		SDL_Quit();
		exit(2);
	}
	
	if (status.fullscreen)
		window_to_fullscreen(screen);
	
	SDL_WM_SetCaption(FULLNAME_STRING " " VERSION_STRING, FULLNAME_STRING " " VERSION_STRING);

/*
	YV12 

	This is the format of choice for many software MPEG codecs.
	It comprises an NxM Y plane followed by
	(N/2)x(M/2) V and U planes. 

	NOTE!
	
	I used this format because all the other formats seem to be software
	modes, no matter that overlay.hw_overlay is true... at least /w G400.

	In the end this isn't that bad but we lose some color detail.
*/
	/* setup overlay: */
	overlay = SDL_CreateYUVOverlay(GR_WIDTH, GR_HEIGHT,SDL_YV12_OVERLAY, screen);
	if (! overlay || overlay->format != SDL_YV12_OVERLAY)
	{
		fprintf(stderr, "Couldn't get %dx%d YV12 overlay (1 of 2): %s\n", GR_WIDTH, GR_HEIGHT, SDL_GetError());
		exit(3);
	}
	overlay2 = SDL_CreateYUVOverlay(GR_WIDTH, GR_HEIGHT,SDL_YV12_OVERLAY, screen);
	if (! overlay2 || overlay2->format != SDL_YV12_OVERLAY)
	{
		fprintf(stderr, "Couldn't get %dx%d YV12 overlay (2 of 2): %s\n", GR_WIDTH, GR_HEIGHT, SDL_GetError());
		exit(4);
	}
	
#if 1
	/* print info */
	if (overlay->planes == 3 && overlay2->planes == 3) /* sanity check */
	{
		printf("Overlay info:\n");
		printf("Format: 0x%08X\n",overlay->format);
		printf("Size: %d x %d\n",overlay->w, overlay->h);
		printf("Planes: %d\n",overlay->planes);

		printf("+ Y-plane: %p\n",overlay->pixels[0]);
		printf("+ Y-pitch: %d\n",overlay->pitches[0]);
		printf("+ V-plane: %p\n",overlay->pixels[1]);
		printf("+ V-pitch: %d\n",overlay->pitches[1]);
		printf("+ U-plane: %p\n",overlay->pixels[2]);
		printf("+ U-pitch: %d\n",overlay->pitches[2]);

		printf("Hardware: %s\n", overlay->hw_overlay ? "yes" : "no");
	}
	else
	{
		fprintf(stderr,"Warning: number of planes %d instead of expected 3.\n",overlay->planes);
	}
#endif

	clear_yv12_buffer(overlay);
	clear_yv12_buffer(overlay2);

	status.shutdown = 0;
	status.showvideo = 1;
	
	#ifdef CHECK
	fprintf(stdout,"\nTrying to open video display to check configuration.\n");
	#endif

	/* setup device */
	vsrc.dev = "/dev/video";
	if (! gr_open(&vsrc))
	{
		fprintf(stderr,"Failed to open device.\n");
		status.shutdown = 1;
		return FALSE;
	}

	#ifndef CHECK
	/* print device info */
	gr_get_info(&vsrc);
	#endif

	/* setup image source */
	/*
	gr_set_input_channel(&vsrc,0);
	gr_set_frequency(&vsrc,266250000);
	*/

	if (status.channel >= 0 && status.channel < NO_CHANNELS)
	{
		gr_set_input_channel(&vsrc,channels[status.channel][1]);
		gr_set_frequency(&vsrc,channels[status.channel][0]);
		channelindex = status.channel;
	}

	/* setup picture */
	
	gr_set_picture(&vsrc,
			status.brightness,
			status.contrast,
			status.colour,
			-1, /* hue */
			status.whiteness);
	
	gr_set_volume(&vsrc,100);

	/* start capturing */
	if (! gr_begin_capture(&vsrc))
	{
		fprintf(stderr,"Failed to start video capture.\n");
		status.shutdown = 1;
		return FALSE;
	}

	while ( ! status.shutdown )
	{
		#ifdef CHECK
		status.shutdown = 1;
		#endif

		ui_schedule(screen,overlay,&vsrc); /* handle ui events */
		if (status.showvideo)
		{

			/* I get slightly more performace if frame
			syncing is done before Overlay locking. Your
			mileage may vary. */
			gr_sync_next(&vsrc);

			if (status.deinterlace)
			{
				SDL_Rect drect;
				drect.x=0;
				drect.y=0;
				drect.w=status.width;
				drect.h=status.height;
				SDL_DisplayYUVOverlay(overlay2, &drect);
			}

			if (status.autodeinterlace)
			{
				/* check random line */
				i = 20 + (float)(GR_HEIGHT-40)*rand()/(float)(RAND_MAX+1.0);
				interlaced = gr_detect_interlacing_in_yv12(&vsrc,overlay,buffer,i);
				if (interlaced)
				{
					progressive_frames = 0;
					interlaced_frames++;
				}
				else
				{
					progressive_frames++;
					interlaced_frames = 0;
				}


				if (progressive_frames > 25 && status.deinterlace)
				{
					printf("deinterlacing: off (auto) (frame=%d)\n",vsrc.frame_count);
					status.deinterlace = FALSE;
				}
				if (interlaced_frames > 2 && !status.deinterlace)
				{
					printf("deinterlacing: on (auto) (frame=%d)\n",vsrc.frame_count);
					status.deinterlace = TRUE;
				}
				D printf("deinterlace=%d progressive_frames=%d interlaced_frames=%d\n",status.deinterlace,progressive_frames,interlaced_frames);
			}

			SDL_LockYUVOverlay(overlay2);
			SDL_LockYUVOverlay(overlay);
			/* *convert* YUV422 frame to YV12 directly to overlay */
			if (status.deinterlace)
			{
				D printf("deinterlacing\n");
				gr_fetch_deinterlaced_frame_as_yv12(&vsrc,overlay,overlay2,buffer,status.debug);
			}
			else
			{
				D printf("no deinterlacing\n");
				gr_fetch_frame_as_yv12(&vsrc,overlay,buffer,status.debug);
			}


			/* there's often garbage in the edges of the screen -- it seems that even tv stations don't have good enough monitors! */

			/* NOTE! I've seen incorrect results if I try
			to black out 1-4 pixels from both sides. Result
			looks pretty much the same as looking the
			resulting image if PCI latency is set too low.
			Only that in this case errors don't show in the
			whole picture but in small vertical area.
			Blacking out only at top and bottom seems to
			work fine. */

			clearborder_yv12_buffer(overlay,0,2);
			clearborder_yv12_buffer(overlay2,0,2);
			SDL_UnlockYUVOverlay(overlay);
			SDL_UnlockYUVOverlay(overlay2);

			//time = SDL_GetTicks();

			/* show overlay - this *should* be practically
			no-op with hw support. However at least with my
			G400 and SDL 1.1.7 this takes like 15ms. I
			could apply a noise filter in that time... */

#if 1
			{
				SDL_Rect drect;
				drect.x=0;
				drect.y=0;
				drect.w=status.width;
				drect.h=status.height;
				SDL_DisplayYUVOverlay(overlay, &drect);
			}
#endif			
			//printf("SDL_DisplayYUVOverlay() took %d ms.\n",SDL_GetTicks() - time);
		}
		else
		{
			/* we should stop capture, but because I
			   don't see other use for not showing the
			   video but to debug this should be enough */
			SDL_Delay(35);
#if 0
			SDL_LockYUVOverlay(overlay);
			SDL_UnlockYUVOverlay(overlay);
			{
				SDL_Rect drect;
				drect.x=0;
				drect.y=0;
				drect.w=status.width;
				drect.h=status.height;
				SDL_DisplayYUVOverlay(overlay, &drect);
			}
#endif
		}
	} /* while not shutdown */

	if ( ! gr_end_capture(&vsrc) )
	{
		fprintf(stderr,"Failed to stop video capture.\n");
		return FALSE;
	}
	if ( ! gr_close(&vsrc) )
	{
		fprintf(stderr,"Failed to close device.\n");
		return FALSE;
	}
	
	SDL_Quit();
	delete_yv12_buffer(buffer);

	#ifdef CHECK
	fprintf(stdout,"Everything looks OK.\n");
	#endif
	return TRUE;
}
#if 0
	/*vsrc.frequency = 259250000;*/ /* TVTV Hz */
	vsrc.frequency = 266250000; /* MTV Hz */
#endif
