#!/bin/sh

CPU=athlon
#CPU=i686
#CPU=pentium4
#CPU=athlon-xp

#CFLAGS="-Wall -O3 -funroll-loops -fno-volatile -fomit-frame-pointer -fschedule-insns -fschedule-insns2 -march=$CPU -mcpu=$CPU -fno-force-addr -ffast-math -fexpensive-optimizations -fsched-spec-load -fsched-spec-load-dangerous -frerun-loop-opt -funroll-all-loops -fprefetch-loop-arrays -fmove-all-movables -frename-registers"
#CFLAGS="-O3 -fomit-frame-pointer -mcpu=$CPU -march=$CPU"
#CFLAGS="-O3 -fomit-frame-pointer -march=$CPU -mmmx -msse2 -mfpmath=sse,387"
#CFLAGS="-O3 -fomit-frame-pointer -march=$CPU -mmmx -m3dnow -fpeel-loops -funswitch-loops"
CFLAGS="-O3 -fomit-frame-pointer -march=$CPU -mmmx -m3dnow -fmove-all-movables -freduce-all-givs"

echo ""
echo "COMPILE TARGET CPU: $CPU"
echo ""
echo "This script will optimize the TV Filter for your system."
echo "The optimization is done by first compiling the filter"
echo "with profiling, running it as long as you want, and"
echo "compiling the filter again according to the gathered info."
echo ""
echo "After the program starts the first time, you should run it for"
echo "at least a couple of minutes watching the channels you'd"
echo "normally watch so that the generated statistics are correct."
echo "Pressing ESC or Q anytime will exit the TV Filter."
echo ""
echo "You need to run this script again if any source is modified."
echo ""
echo "Press enter to continue or CTRL+C to quit"
read enter

echo ""
echo "Compiling with profiling..."
echo "==========================="
echo ""
#CFLAGS="$CFLAGS -fprofile-arcs" ./configure && make clean all
make CFLAGS="$CFLAGS -g -fprofile-arcs" clean all
time -v ./tvf
echo ""
echo "Compiling fully optimized version..."
echo "===================================="
echo ""
rm -f ui.da
#CFLAGS="$CFLAGS -fbranch-probabilities" ./configure && make clean all && rm -f grabber.da tvf.da ui.da
#make CFLAGS="$CFLAGS -fbranch-probabilities -freorder-functions" clean all && rm -f grabber.da tvf.da ui.da
make CFLAGS="$CFLAGS -fbranch-probabilities" clean all && rm -i grabber.da tvf.da ui.da
time -v ./tvf

## ASM output:
##gcc -S `sdl-config --cflags` grabber.c -masm=intel -fverbose-asm
