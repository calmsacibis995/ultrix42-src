# @(#)Makelocal.mk	4.1 (ULTRIX) 7/17/90
# Makefile for extract, strextract, strmerge, & trans, the I18N string
# extraction tools.
#
#	15th Feb 88 - Created Martin Hills, EUEG.
include $(GMAKEVARS)

CINCLUDES  =  -I. -I.. -I$(SRCROOT)/usr/include
CDEFINES  =  -DULTRIX
 
OBJS	= duplicate.o extract.o gen.o getline.o re.o strextract.o \
	  strmerge.o trans.o
 
all: 		extract strextract strmerge trans 
 
trans:		trans.o getline.o
		$(LDCMD) trans.o getline.o -lcursesX
 
strextract:	strextract.o re.o gen.o duplicate.o
		$(LDCMD) strextract.o re.o gen.o duplicate.o
 
strmerge: 	strmerge.o re.o gen.o duplicate.o
		$(LDCMD) strmerge.o re.o gen.o duplicate.o
 
extract: 	extract.o re.o gen.o duplicate.o
		$(LDCMD) extract.o re.o gen.o duplicate.o -lcursesX
 
$(OBJS):	defs.h
 
duplicate.o:	duplicate.c
extract.o:	extract.c
gen.o:		gen.c
getline.o:	getline.c
re.o:		re.c
strextract.o:	strextract.c
strmerge.o:	strmerge.c
trans.o: 	trans.c

install : all
	install -c -m 555 -o bin -g system -s extract ${DESTROOT}/usr/bin
	install -c -m 555 -o bin -g system -s strextract ${DESTROOT}/usr/bin
	install -c -m 555 -o bin -g system -s strmerge ${DESTROOT}/usr/bin
	install -c -m 555 -o bin -g system -s trans ${DESTROOT}/usr/bin

include $(GMAKERULES)
