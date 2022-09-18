#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/usr/etc $(DESTROOT)/etc

RTDEVNAM='"/dev/bootdev"'
OPSYSDEF= -DULTRIX32
CDEFINES=DRTDEVNAM=$(RTDEVNAM) $(OPSYSDEF)

AOUT=	arff

OBJS=	arcpad.o ardown.o arff.o   arnlen.o arpars.o arrad5.o \
	arvfyn.o arwcid.o rtcons.o rtctrd.o rtdate.o rtdele.o \
	rtdepm.o rtdire.o rtdsio.o rtextr.o rtinit.o rtmkfi.o \
	rtmkfn.o rtmvde.o rtopen.o rtpakn.o rtpio.o  rtprot.o \
	rtrepl.o rtscnd.o rtspds.o rtwmat.o

arcpad.o:	arcpad.c
ardown.o:	ardown.c arff.h
arff.o:		arff.c arff.h
arnlen.o:	arnlen.c
arpars.o:	arpars.c arff.h
arrad5.o:	arrad5.c arff.h
arvfyn.o:	arvfyn.c
arwcid.o:	arwcid.c
rtcons.o:	rtcons.c arff.h
rtctrd.o:	rtctrd.c arff.h
rtdate.o:	rtdate.c arff.h
rtdele.o:	rtdele.c arff.h
rtdepm.o:	rtdepm.c arff.h
rtdire.o:	rtdire.c arff.h
rtdsio.o:	rtdsio.c arff.h
rtextr.o:	rtextr.c arff.h
rtinit.o:	rtinit.c arff.h
rtmkfi.o:	rtmkfi.c arff.h
rtmkfn.o:	rtmkfn.c arff.h
rtmvde.o:	rtmvde.c arff.h
rtopen.o:	rtopen.c arff.h
rtpakn.o:	rtpakn.c
rtpio.o:	rtpio.c arff.h
rtprot.o:	rtprot.c arff.h
rtrepl.o:	rtrepl.c arff.h
rtscnd.o:	rtscnd.c arff.h
rtspds.o:	rtspds.c arff.h
rtwmat.o:	rtwmat.c

install:		
	$(INSTALL) -s -c arff $(DESTROOT)/usr/etc/arff
	$(RM) $(DESTROOT)/etc/arff
	$(LN) -s ../usr/etc/arff $(DESTROOT)/etc/arff

include $(GMAKERULES)
