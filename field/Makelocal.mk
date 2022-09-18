#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

NFILES = Readmefirst syscript 8600README
FILES =  cmx dskx fsx fsxr lpx memx memxr mtx netx \
	shmx shmxb showsnap shoadrs tapex

AOUTS = cmx dskx fsx fsxr lpx memx memxr mtx netx shmx shmxb tapex
cmx:	cmx.o diaglib.o
dskx:	dskx.o diaglib.o
fsx:	fsx.o diaglib.o
fsxr:	fsxr.o diaglib.o
lpx:	lpx.o diaglib.o
memx:	memx.o diaglib.o
memxr:	memxr.o diaglib.o
mtx:	mtx.o diaglib.o
netx:	netx.o diaglib.o
shmx:	shmx.o diaglib.o
shmxb:	shmxb.o diaglib.o
tapex:	tapex.o 

OBJS = cmx.o dskx.o fsx.o fsxr.o lpx.o memx.o memxr.o mtx.o netx.o shmx.o \
	shmxb.o csl.o emm.o esc.o ipr.o isp.o mstr.o pamm.o sbi.o \
	uvsr.o shoadrs.o tapex.o

OBJSMISC = diaglib.o

diaglib.o:	diaglib.c
cmx.o:		cmx.c
dskx.o:		dskx.c
fsx.o:		fsx.c
fsxr.o:		fsxr.c
lpx.o:		lpx.c
memx.o:		memx.c
memxr.o:	memxr.c
mtx.o:		mtx.c
netx.o:		netx.c
shmx.o:		shmx.c
shmxb.o:	shmxb.c
csl.o:		csl.c
emm.o:		emm.c
esc.o:		esc.c
ipr.o:		ipr.c
isp.o:		isp.c
mstr.o:		mstr.c
pamm.o:		pamm.c
sbi.o:		sbi.c
uvsr.o:		uvsr.c
shoadrs.o:	shoadrs.c
tapex.o:	tapex.c

SHOWSNAP = csl.o emm.o esc.o ipr.o isp.o mstr.o pamm.o sbi.o uvsr.o

SHOADRS = shoadrs.o

ALL = showsnap shoadrs
LOADLIBES = -lerrlog

DESTLIST= ${DESTROOT}/usr/field

all: ${ALL}


showsnap: ${SHOWSNAP}
	cc -O -o showsnap ${SHOWSNAP}

shoadrs: ${SHOADRS}
	cc -O -o shoadrs ${SHOADRS}


install: 
	for i in ${FILES}; do \
		(install -c -s -m 744 $$i ${DESTROOT}/usr/field/$$i); done
	for i in ${NFILES}; do \
		(install -c -m 744 ../$$i ${DESTROOT}/usr/field/$$i); done
	for i in ${ALL}; do \
		(strip ${DESTROOT}/usr/field/$$i); done
	cp ../../root/.cshrc ${DESTROOT}/usr/field
	cp ../../root/.login ${DESTROOT}/usr/field
	install -c -m 440 ../makefile.rdt ${DESTROOT}/usr/field/Makefile


include $(GMAKERULES)
