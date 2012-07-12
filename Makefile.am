#AM_CFLAGS	= -Wall -O3 -funroll-all-loops -fno-volatile -march=i586 -mcpu=i586 $(SDL_CFLAGS)
#AM_LFLAGS	= $(SDL_LIBS)

bin_PROGRAMS	= tvf
#check_PROGRAMS  = tvftest
#TESTS		= tvftest
check_PROGRAMS  = test test2 tvftest
TESTS		= test test2 tvftest
EXTRA_DIST	= optimize.sh test.c test2.c

tvftest_SOURCES	= tvf.c tvf.h ui.c ui.h grabber.c grabber.h
tvftest_CFLAGS	= @SDL_CFLAGS@ -DCHECK
tvftest_LDADD	= $(SDL_LIBS)

tvf_SOURCES	= tvf.c tvf.h ui.c ui.h grabber.c grabber.h
tvf_CFLAGS	= @SDL_CFLAGS@
tvf_LDADD	= $(SDL_LIBS)

test_SOURCES	= test.c
test_CFLAGS	= @SDL_CFLAGS@
test_LDADD	= $(SDL_LIBS)

test2_SOURCES	= test2.c
test2_CFLAGS	= @SDL_CFLAGS@
test2_LDADD	= $(SDL_LIBS)
