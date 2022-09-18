# 	@(#)Makelocal.mk	4.1	(ULTRIX)	7/17/90
include $(GMAKEVARS)

HDRS = ../../hdr/*.h

HDRDIR = ../../hdr
#	Directory containing SCCS header files
LIBDIR = ../../lib/_$(MACHINE).b
#	SCCS common object library directory

FECFLAGS = -DUIDUSER
CFLAGS = -O 
CINCLUDES = -I$(HDRDIR)
LDFLAGS =  -n
LIBES = ../../libPW/_$(MACHINE).b/libPW.a

all:	admin bdiff comb  delta get help prs rmchg \
	sccs unget val vc what 


admin:	admin.o $(LIBDIR)/comobj.a $(LIBES) 
	$(LDCMD) admin.o $(LIBDIR)/comobj.a $(LIBES)

admin.o:	../admin.c $(HDRS)
	$(CCCMD) ../admin.c

bdiff:	bdiff.o $(LIBDIR)/comobj.a $(LIBES)
	$(LDCMD) bdiff.o $(LIBDIR)/comobj.a $(LIBES)

bdiff.o:	../bdiff.c $(HDRS)
	$(CCCMD) ../bdiff.c

comb:	comb.o $(LIBDIR)/comobj.a $(LIBES)
	$(LDCMD) comb.o $(LIBDIR)/comobj.a $(LIBES)

comb.o:		../comb.c $(HDRS)
	$(CCCMD) ../comb.c

delta:	delta.o $(LIBDIR)/comobj.a $(LIBES)
	$(LDCMD) delta.o $(LIBDIR)/comobj.a $(LIBES)

delta.o:	../delta.c $(HDRS)
	$(CCCMD) ../delta.c

get:	get.o $(LIBDIR)/comobj.a $(LIBES)
	$(LDCMD) get.o $(LIBDIR)/comobj.a $(LIBES)

get.o:		../get.c $(HDRS)                             
	$(CCCMD) ../get.c

help:	help.o $(LIBDIR)/comobj.a $(LIBES)
	$(LDCMD) help.o $(LIBDIR)/comobj.a $(LIBES)

help.o:		../help.c $(HDRS)
	$(CCCMD) ../help.c

prs:	prs.o $(LIBDIR)/comobj.a $(LIBES)
	$(LDCMD) prs.o $(LIBDIR)/comobj.a $(LIBES)

prs.o:		../prs.c $(HDRS)
	$(CCCMD) ../prs.c

rmchg:	rmchg.o $(LIBDIR)/comobj.a $(LIBES)
	$(LDCMD) rmchg.o $(LIBDIR)/comobj.a $(LIBES)

rmchg.o:	../rmchg.c $(HDRS)
	$(CCCMD) ../rmchg.c

CCONLY=cc
sccs:	sccs.o
	$(CCONLY) -o sccs sccs.o

sccs.o:
	$(CCCMD) $(FECFLAGS) ../sccs.c


unget:	unget.o $(LIBDIR)/comobj.a $(LIBES)
	$(LDCMD) unget.o $(LIBDIR)/comobj.a $(LIBES)

unget.o:	../unget.c $(HDRS)
	$(CCCMD) ../unget.c

val:	val.o $(LIBDIR)/comobj.a $(LIBES)
	$(LDCMD) val.o $(LIBDIR)/comobj.a $(LIBES)

val.o:	../val.c $(HDRS)                                 
	$(CCCMD) ../val.c

vc:	vc.o $(LIBDIR)/comobj.a
	$(LDCMD) vc.o $(LIBDIR)/comobj.a $(LIBES)

vc.o:	../vc.c $(HDRS)                                 
	$(CCCMD) ../vc.c

what:	what.o $(LIBDIR)/comobj.a $(LIBES)
	$(LDCMD) what.o $(LIBDIR)/comobj.a  $(LIBES)

what.o:		../what.c $(HDRS)                               
	$(CCCMD) ../what.c

$(LIBDIR)/comobj.a:
	cd ../../lib ; $(MAKE)

$(LIBES):
	cd ../../libPW ; $(MAKE)

pretools tools1 install:

include $(GMAKERULES)
