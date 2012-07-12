/*
    SDL Overlay performance test.

    Compiling:
    
    gcc `sdl-config --cflags --libs` -o test test.c
    
    
    Results for SDL_DisplayYUVOverlay() I have got:
    
    IYUV: 32 ms Hardware: no
    YV12: 17 ms Hardware: yes
    YUY2: 33 ms Hardware: yes
    UYVY: 33 ms Hardware: yes
    YVYU: 49 ms Hardware: no
    
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

Uint32 sdl_screen_flags = SDL_HWSURFACE|SDL_DOUBLEBUF|SDL_ANYFORMAT|SDL_HWACCEL|SDL_RESIZABLE;

int main(void)
{
    SDL_Surface* screen;
    SDL_Overlay* overlay;    /* yuv overlay */
    SDL_Rect drect;
    Uint32 stime,time;
    int i;

    printf("\nTesting SDL overlay locking and rendering speed...\n");

    SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER); /* SDL_INIT_AUDIO */

/*    screen = SDL_SetVideoMode(800, 600, 16, SDL_HWSURFACE); */
    screen = SDL_SetVideoMode(800, 600, 16, sdl_screen_flags);
    overlay = SDL_CreateYUVOverlay(768, 576,SDL_YV12_OVERLAY, screen);

    stime = SDL_GetTicks();
    for (i = 0; i < 100; i++)
    {
        time = SDL_GetTicks();
        SDL_LockYUVOverlay(overlay);

        /* real image rendering onto overlay pixels happens here */
	#if 0
	    /* simulation */
	    for (j=0; j<(768*576); j++)
	    	j++;
	#endif
	
        SDL_UnlockYUVOverlay(overlay);
	#ifdef DEBUG
        printf("SDL_Lock+UnlockYUVOverlay() took %d ms. ",SDL_GetTicks() - time);
	#endif

        time = SDL_GetTicks();

        if (1)
        {
            drect.x=0;
            drect.y=0;
            drect.w=800;
            drect.h=600;
            SDL_DisplayYUVOverlay(overlay, &drect);
            
	    
	    SDL_DisplayYUVOverlay(overlay, &drect);
        }
	
	#ifdef DEBUG
        printf("SDL_DisplayYUVOverlay() took %d ms.\n",SDL_GetTicks() - time);
	#endif
    }

    stime = SDL_GetTicks() - stime;
    printf("Average %f ms per frame.\n",(float)stime/100);
    printf("Hardware: %s\n", overlay->hw_overlay ? "yes" : "no");
    printf("overlay.pixels[0]=%p\n",overlay->pixels[0]);

    SDL_FreeYUVOverlay(overlay);
    SDL_Quit();

    return 0;
}
