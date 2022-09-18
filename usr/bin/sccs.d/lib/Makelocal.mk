# 	@(#)Makelocal.mk	4.1	(ULTRIX)	7/17/90
include $(GMAKEVARS)

CFLAGS = -O
CINCLUDES=-I.. -I../../hdr
#LD?? = -ld
#LDFLAGS = -x

all:	comobj.a

OBJ  = chkid.o chksid.o del_ba.o  date_ba.o dodelt.o \
       del_ab.o dofile.o dohist.o doie.o dolist.o eqsid.o flushto.o newstats.o \
       permiss.o logname.o pf_ab.o date_ab.o rdmod.o setup.o \
       sid_ab.o sid_ba.o sidtoser.o sinit.o stats_ab.o \
       fmterr.o getline.o putline.o auxf.o cmrcheck.o error.o filehand.o \
       fmalloc.o gf.o xcreat.o xmsg.o deltack.o

$(OBJ):
	$(CCCMD) ../$(@:.o=.c)

comobj.a:	$(OBJ) 
	-rm -f comobj.a
	ar r comobj.a $(OBJ)
	ranlib comobj.a

chkid.o:	../chkid.c
chksid.o:	../chksid.c
del_ba.o:	../del_ba.c
date_ba.o:	../date_ba.c
dodelt.o:	../dodelt.c
del_ab.o:	../del_ab.c
dofile.o:	../dofile.c
dohist.o:	../dohist.c
doie.o:		../doie.c
dolist.o:	../dolist.c
eqsid.o:	../eqsid.c
flushto.o:	../flushto.c
newstats.o:	../newstats.c
permiss.o:	../permiss.c
logname.o:	../logname.c
pf_ab.o:	../pf_ab.c
date_ab.o:	../date_ab.c
rdmod.o:	../rdmod.c
setup.o:	../setup.c
sid_ab.o:	../sid_ab.c
sid_ba.o:	../sid_ba.c
sidtoser.o:	../sidtoser.c
sinit.o:	../sinit.c
stats_ab.o:	../stats_ab.c
fmterr.o:	../fmterr.c
getline.o:	../getline.c
putline.o:	../putline.c
auxf.o:		../auxf.c
cmrcheck.o:	../cmrcheck.c
error.o:	../error.c
filehand.o:	../filehand.c
fmalloc.o:	../fmalloc.c
gf.o:		../gf.c
xcreat.o:	../xcreat.c
xmsg.o:		../xmsg.c
deltack.o:	../deltack.c

pretools tools1 install:

include $(GMAKERULES)
