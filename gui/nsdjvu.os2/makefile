#** Module   :MAKEFILE
#** Abstract :Makefile for the NPDJVU.DLL
#**
#** Copyright (C) Sergey I. Yevtushenko
#**
#** Log: Fri  05/12/2003 Created

COMMON = .
NAME = npdjvu

.SUFFIXES: .cpp .o .rc .res

.all: npdjvu.dll

CPP_FLAGS = -O2 -s -Zdll -I ../../libdjvu -I ../.. -DNO_DEBUG -DHAVE_CONFIG_H -DTHREADMODEL=NOTHREADS -Wall -mcpu=athlon -static-libgcc -Zmap
CC=g++ $(CPP_FLAGS)

BITMAPS= bitmap/bck01.bmp bitmap/first.bmp bitmap/fwd01.bmp bitmap/last.bmp

.cpp.o:
	$(CC) -c $<

.rc.res:
	rc.exe -i . -r $<

npdjvu.dll: npos2.o npdjvu.o npdjvu.res npdjvu.def
	@$(CC) -o npdjvu.dll npos2.o npdjvu.o ../../libdjvu/libdjvulibre.a npdjvu.def
	lxlite npdjvu.dll
	rc npdjvu.res $@

npdjvu.o: npdjvu.cpp npdjvu.h npapi.h

npos2.o: npos2.cpp npapi.h npupp.h

npdjvu.res: npdjvu.rc npdjvu.h npapi.h $(BITMAPS)

npdjvu.def: npdjvu.tpl npdjvu.h
	@mkver.cmd

clean:
	@-del *.o
	@-del *.res
	@-del *.dll
	@-del *.def
