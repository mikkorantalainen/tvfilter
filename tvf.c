/* 	
 *	File:			
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

#include <signal.h>

#define GETOPT_LONG_SUPPORTED

#include "tvf.h"
#include "ui.h"

/*************************************
 *  G L O B A L   V A R I A B L E S  *
 *************************************/
t_status	status;
/*************************************
 *    I M P L E M E N T A T I O N    *
 *************************************/

/*
	signal_handler(...)
	
	IN:	signal number
	OUT:	none
	DESC:	System signal handling - we should exit gracefully when we get
		SIGINT or SIGTERM and try to save our file as FILENAME_AUTOSAVE
		(or something like that) if(/when) we get SIGSEGV.
	
		I'm not sure what we should do if we get SIGFPE because I
		think we probably get it only from rendering so it should not
		be fatal. I think that we should use the same procedure as
		with SIGINT/SIGTERM, though ingnoring could be one option.
		Warning message would be good in that case.
*/
void signal_handler(int sig)
{
	if	(sig == SIGINT || sig == SIGTERM)
	{		
		status.showvideo = 0;
		status.shutdown  = 1;
		
	}
	else if	(sig == SIGFPE)
	{
		printf("\nFloating point error\n");
		abort();
	}
	else if	(sig == SIGSEGV)
	{
		printf("\nSegmentation Fault\n");
		abort();
	}
	else
	{
		/* we should NEVER ever be here - but in case we are, lets tell it to user */
		printf("error: signal_handler(sig=%d): unknown signal\n",sig);
	}
}

/*
	int parse_options(...)
	
	IN:	the same parameters as passed to main()
	OUT:	none
	DESC:	parses command line parameters (given in argc and argv)
		and setups global variables accordingly.
*/
void parse_options(int argc, char** argv)
{
	const char* help_text =
		"usage: " PROGNAME_STRING " [option(s)]\n"
		"\n"
		FULLNAME_STRING " " VERSION_STRING "\n"
		"\n"
		"Copyright (c) 2001-2003 Mikko Rantalainen <mira@st.jyu.fi>\n"
		"\n"
		"Options:\n"
		"\n"
		"-f\n"
		"--fullscreen\n"
		"	Start in fullscreen mode instead of default windows mode.\n"
		"-W WIDTH\n"
		"--width WIDTH\n"
		"	Screen width (window or fullscreen)\n"
		"-H HEIGHT\n"
		"--height HEIGHT\n"
		"	Screen height (window or fullscreen)\n"
		"-h\n"
		"--help\n"
		"	Print this message to stdout and exit successfully. Will not\n"
		"	parse trailing options.\n"
		"-V\n"
		"--version\n"
		"	Print version to stdout and exit successfully. Will not\n"
		"	parse trailing options.\n"
		"-d\n"
		"--deinterlace\n"
		"	Force deinterlacing instead of heuristics to decide whether it's\n"
		"	needed or not. Lowers CPU requirements a bit but potentially\n"
		"	decreases image quality.\n"
		"\n";

#ifdef GETOPT_LONG_SUPPORTED
       	const struct option long_options[] =
       	{
		{"fullscreen",		0, 0, 'f'},
		{"deinterlace",		0, 0, 'd'},
		{"width",		1, 0, 'W'},
		{"height",		1, 0, 'H'},
		{"help",		0, 0, 'h'},
		{"version",		0, 0, 'V'},
		{"brightness",		1, 0, 'b'},
		{"contrast",		1, 0, 'c'},
		{"colour",		1, 0, 'C'},
		{"whiteness",		1, 0, 'w'},
		{"channel",		1, 0, 's'},
		{0, 0, 0, 0}
	};
       	int option_index = 0;
#endif
	int c;
	char* opts;
	opts = "fdH:W:hVb:c:C:w:s:"; /* all possible options */

	/*********************************
		setup default config
		        and
		setup global values
	**********************************/

	status.pixelratio	= 1.0;
	status.shutdown		= 0;
	status.showvideo	= 1;
	status.width		= 768;
	status.height		= 576;
	status.fullscreen	= 0;
	status.deinterlace	= 0;
	status.autodeinterlace	= 1;
	status.debug		= 0;
	status.brightness	= -1.0; /* don't mess around */
	status.contrast		= -1.0;
	status.colour		= -1.0;
	status.whiteness	= -1.0;
	status.channel		= -1; /* don't mess around */

	/*********************************/

        while (1)
        {
 	#ifdef GETOPT_LONG_SUPPORTED
             	c = getopt_long(argc, argv, opts,long_options, &option_index);
  	#else
             	c = getopt(argc, argv, opts);
   	#endif
             	if (c == -1) break; /* we have read all options */
             	
             	switch (c)
             	{
             		case 0:
				/*
				 * Following commented out section has code that allows
				 * long-options without corresponding short option,
				 * but because long-options are not supported in all UNIXes
				 * I do not want to use these.
				 * This has been left here to make it easier to use long-options
				 * in the future if portability is not needed.
				 * This section currently prints only error message because
				 * we should not be here - it happens if we have defined long
				 * option without defined short option.
				 */
             			/* 
				 * printf("option %s", long_options[option_index].name);
             			 * if (optarg)
             			 * 	printf("with arg %s", optarg);
             			 * printf("\n");
             			 */
             			printf("void parse_options(int argc, char** argv): internal error\n");
             			break;
             		case 'f':
             			status.fullscreen = 1;
             			break;
             		case 'd':
             			status.deinterlace = 1;
             			break;
             		case 'W':
				status.width = (int)atof(optarg);
             			break;
             		case 'H':
				status.height = (int)atof(optarg);
             			break;

             		case 'b':
				status.brightness = atof(optarg);
             			break;
             		case 'c':
				status.contrast = atof(optarg);
             			break;
             		case 'C':
				status.colour = atof(optarg);
             			break;
             		case 'w':
				status.whiteness = atof(optarg);
             			break;
             		case 's':
				status.channel = (int)atof(optarg)-1;
             			break;

             		case 'V':
             			printf(FULLNAME_STRING " " VERSION_STRING "\n");
             			exit(0);
             			break;
             		case 'h':
				printf("%s", help_text);
				exit(0);
             			break;
             		default:
             			break;
             			
             	}
        }
 	
	if (optind < argc)
	{
		while (optind < argc)
		{
			printf("Parameter %s discarded.\n",argv[optind]);
			optind++;
		}
	}                                                                    	
}

/*
	main()
	
	IN:	the usual
	OUT:	0 if all ok - 1 if error
	DESC:	main function - opens a window and
		checks for OpenGL and executes main (gtk) loop
*/
int main (int argc, char** argv)
{
	/* set signal handlers */
	signal(SIGINT ,signal_handler);
	signal(SIGTERM,signal_handler);
	signal(SIGFPE ,signal_handler);
	signal(SIGSEGV,signal_handler);

	/* initialize gtk */
	parse_options(argc, argv);

	if (!ui_main())
	{
		fprintf(stderr,"Oops.\n");
		exit(1);
	}
	return 0;
}

