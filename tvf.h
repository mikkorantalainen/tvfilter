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
 *	Copyright (c) 2001 Mikko Rantalainen
 */

#ifndef TVF_H
#define TVF_H

#ifdef USE_DMALLOC
	#define MALLOC malloc
	#define FREE free
	#include <dmalloc.h>
#endif

#include <stdlib.h>

#ifdef GETOPT_LONG_SUPPORTED
	#include <getopt.h>
#else
	#include <unistd.h>
#endif


/***********************************************
 *  G L O B A L   S E T T I N G S   S T A R T  *
 ***********************************************/

#define PROGNAME_STRING		"tvf"
#ifdef PACKAGE
	#define FULLNAME_STRING		PACKAGE
#else
	#define FULLNAME_STRING		"TV Filter"
#endif
#ifdef VERSION
	#define VERSION_STRING		VERSION
#else
	#define VERSION_STRING		"0.1"
#endif
#define GETOPT_LONG_SUPPORTED	/* comment this if environment (e.g. SGI) cannot support getopt_long(..) in <getopt.h> */
#define MAX_OPEN_WINDOWS	10 /* max number of windows - too lazy to make dynamic list... */	
/***********************************************
 *  G L O B A L   S E T T I N G S   END  *
 ***********************************************/

#ifndef TRUE
	#define TRUE	1
#endif
#ifndef FALSE
	#define FALSE	0
#endif


typedef struct
{
	int beginx;	/* x-coord where mouse was pressed down */
	int beginy;	/* y-coord ... */
} t_tvview_config;

typedef struct
{
	int width;
	int height;
	t_tvview_config	tvview_config;
}
t_tvwindow;

typedef struct
{
	int		shutdown;			/* we have got some reason to shut down */
							/* check this in main loop */
	volatile int	showvideo;			/* if not true we should clear the screen */
	float		pixelratio;			/* pixel width/height ratio */
	int		fullscreen;			/* 1 or 0 */
	int		width;				/* screen width */
	int		height;				/* screen height */
	int		deinterlace;			/* 1 or 0 */
	int		autodeinterlace;		/* 1 or 0 */
	int		debug;				/* 1 or 0 */
}
t_status;

/*************************************
 *        P R O T O T Y P E S        *
 *************************************/

/*************************************
 *  G L O B A L   V A R I A B L E S  *
 *************************************/

extern		int 		total_frame_count;
extern		t_status	status;

#endif /*TVF_H */
