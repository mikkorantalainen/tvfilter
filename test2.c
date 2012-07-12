/*
    SDL Overlay performance test.

    Compiling:
    
    gcc `sdl-config --cflags --libs` -O3 -fomit-frame-pointer -march=athlon -mcpu=athlon -o test2 test2.c

    I also tried with this:
    gcc `sdl-config --cflags --libs` -O3 -funroll-loops -fno-volatile -fomit-frame-pointer -fschedule-insns -fschedule-insns2 -march=athlon -mcpu=athlon -fno-force-addr -ffast-math -fexpensive-optimizations -fsched-spec-load -fsched-spec-load-dangerous -frerun-loop-opt -funroll-all-loops -fprefetch-loop-arrays -fmove-all-movables -frename-registers -o test2 test2.c
    but it didn't increase rendering speed.
    
    Results for rendering two overlays (= two fields):
    
    YV12: 42 ms Hardware: yes
    
    My environment:
    
    Linux 2.4.4
    SDL 1.2.0
    
    MSI K7T Pro
    AMD Duron 650
    Matrox G400 Single 16MB
    256MB
*/

#include "SDL.h"
#include <stdlib.h>
#define SIMULATE 1
#define BORDER 0
#define FRAMES 100
#define WIDTH 512
#define HEIGHT 576
static int SCREENWIDTH = 768;
static int SCREENHEIGHT = 576;

Uint32 sdl_screen_flags = SDL_HWSURFACE|SDL_DOUBLEBUF|SDL_ANYFORMAT|SDL_HWACCEL|SDL_RESIZABLE;

void ui_schedule(SDL_Surface* screen)
{
	SDL_Event event;
	while ( SDL_PollEvent(&event) )
	{
		switch(event.type)
		{
			case SDL_VIDEORESIZE: /* window size has been changed */
				SCREENWIDTH = event.resize.w;
				SCREENHEIGHT = event.resize.h;
				screen = SDL_SetVideoMode(SCREENWIDTH, SCREENHEIGHT, 16, sdl_screen_flags);
				printf("RESIZE\n");
			break;
		}
	}
}


int main(void)
{
    SDL_Surface* screen;
    SDL_Overlay* overlay;    /* yuv overlay */
    SDL_Overlay* overlay2;    /* yuv overlay */
    SDL_Rect drect;
    Uint32 stime,time;
    int i;
    unsigned long j;
    
    SDL_Overlay* o;
    int x,y;

    printf("\nTesting SDL_Overlay speed:\n");
    printf("If you get less than 50ms per frame you can display\n");
    printf("deinterlaced progressive PAL video.\n");
    printf("Rendering %d frames...\n",FRAMES);

    SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER); /* SDL_INIT_AUDIO */

/*    screen = SDL_SetVideoMode(800, 600, 16, SDL_HWSURFACE); */
    screen = SDL_SetVideoMode(SCREENWIDTH, SCREENHEIGHT, 16, sdl_screen_flags);
    overlay = SDL_CreateYUVOverlay(WIDTH, HEIGHT,SDL_YV12_OVERLAY, screen);
    overlay2 = SDL_CreateYUVOverlay(WIDTH, HEIGHT,SDL_YV12_OVERLAY, screen);

    stime = SDL_GetTicks();
    for (i = 0; i < FRAMES; i++)
    {
        time = SDL_GetTicks();

        SDL_LockYUVOverlay(overlay);
	
	ui_schedule(screen);
	
        if (1)
        {
            drect.x=0;
            drect.y=0;
            drect.w=SCREENWIDTH;
            drect.h=SCREENHEIGHT;
            SDL_DisplayYUVOverlay(overlay2, &drect);
        }

        /* real image rendering onto overlay pixels happens here */
	#if SIMULATE
		o = overlay;
		/* Y */
		for (y = BORDER; y < (o->h - BORDER); y++)
			for (x = BORDER; x < (o->w - BORDER); x++)
				*(o->pixels[0] + o->pitches[0]*y + x) = (y<<8)/(HEIGHT);

		/* U */
		for (y = BORDER/2; y < (o->h - BORDER)/2; y++)
			for (x = BORDER/2; x < (o->w - BORDER)/2; x++)
				*(o->pixels[1] + o->pitches[1]*y + x) = 255 - 255*x/WIDTH;
		/* V */
		for (y = BORDER/2; y < (o->h - BORDER)/2; y++)
			for (x = BORDER/2; x < (o->w - BORDER)/2; x++)
				*(o->pixels[2] + o->pitches[1]*y + x) = 255;
	#endif
        SDL_UnlockYUVOverlay(overlay);
	#ifdef DEBUG
        printf("Lock(overlay)+DisplayYUVOverlay(overlay2)+UnlockYUVOverlay(overlay)  took %d ms.\n",SDL_GetTicks() - time);
	#endif

        time = SDL_GetTicks();

        SDL_LockYUVOverlay(overlay2);
        if (1)
        {
            drect.x=0;
            drect.y=0;
            drect.w=SCREENWIDTH;
            drect.h=SCREENHEIGHT;
            SDL_DisplayYUVOverlay(overlay, &drect);
        }
        /* real image rendering onto overlay pixels happens here */
	#if SIMULATE
		o = overlay2;
		/* Y */
		for (y = BORDER; y < (o->h - BORDER); y++)
			for (x = BORDER; x < (o->w - BORDER); x++)
				*(o->pixels[0] + o->pitches[0]*y + x) = (x<<8)/(WIDTH);

		/* U */
		for (y = BORDER/2; y < (o->h - BORDER)/2; y++)
			for (x = BORDER/2; x < (o->w - BORDER)/2; x++)
				*(o->pixels[1] + o->pitches[1]*y + x) = ((x-i-y) & y ^ i & 1) * 255;
		/* V */
		for (y = BORDER/2; y < (o->h - BORDER)/2; y++)
			for (x = BORDER/2; x < (o->w - BORDER)/2; x++)
				*(o->pixels[2] + o->pitches[1]*y + x) = 255*y/HEIGHT;
	#endif
        SDL_UnlockYUVOverlay(overlay2);

	#ifdef DEBUG
        printf("Lock(overlay2)+DisplayYUVOverlay(overlay)+UnlockYUVOverlay(overlay2) took %d ms.\n",SDL_GetTicks() - time);
	#endif
    }

    stime = SDL_GetTicks() - stime;
    printf("Average %2.02f ms per frame (field pair).\n",(float)stime/(float)FRAMES);
    printf("Hardware: %s\n", overlay->hw_overlay ? "yes" : "no");
    /* printf("overlay.pixels[0]=%p\n",overlay->pixels[0]); */

    SDL_FreeYUVOverlay(overlay);
    SDL_Quit();

    return 0;
}
