#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90
include $(GMAKEVARS)
LIBDIR=	/usr/lib
CFLAGS=	-DTRACE -O
SRCS=	2648.h bit.h \
	agoto.c aminmax.c aon.c areaclear.c beep.c bitcopy.c cleara.c \
	clearg.c curon.c dispmsg.c draw.c drawbox.c dumpmat.c \
	emptyrow.c error.c escseq.c gdefault.c gon.c kon.c line.c mat.c \
	message.c minmax.c motion.c move.c movecurs.c newmat.c outchar.c \
	outstr.c printg.c rawchar.c rbon.c rdchar.c readline.c set.c \
	setmat.c sync.c texton.c ttyinit.c update.c video.c zermat.c \
	zoomn.c zoomon.c zoomout.c
OBJS=	agoto.o aminmax.o aon.o areaclear.o beep.o bitcopy.o cleara.o \
	clearg.o curon.o dispmsg.o draw.o drawbox.o dumpmat.o \
	emptyrow.o error.o escseq.o gdefault.o gon.o kon.o line.o mat.o \
	message.o minmax.o motion.o move.o movecurs.o newmat.o outchar.o \
	outstr.o printg.o rawchar.o rbon.o rdchar.o readline.o set.o \
	setmat.o sync.o texton.o ttyinit.o update.o video.o zermat.o \
	zoomn.o zoomon.o zoomout.o

all: lib2648.a
lib2648.a:	${OBJS}
	ar cr lib2648.a ${OBJS}
	ranlib lib2648.a

tools2:	lib2648.a
tools2 install:
	install -c lib2648.a ${DESTROOT}${LIBDIR}/lib2648.a
	ranlib ${DESTROOT}${LIBDIR}/lib2648.a

include $(GMAKERULES)

agoto.o:	agoto.c
aminmax.o:	aminmax.c
aon.o:	aon.c
areaclear.o:	areaclear.c
beep.o:	beep.c
bitcopy.o:	bitcopy.c
cleara.o:	cleara.c
clearg.o:	clearg.c
curon.o:	curon.c
dispmsg.o:	dispmsg.c
draw.o:	draw.c
drawbox.o:	drawbox.c
dumpmat.o:	dumpmat.c
emptyrow.o:	emptyrow.c
error.o:	error.c
escseq.o:	escseq.c
gdefault.o:	gdefault.c
gon.o:	gon.c
kon.o:	kon.c
line.o:	line.c
mat.o:	mat.c
message.o:	message.c
minmax.o:	minmax.c
motion.o:	motion.c
move.o:	move.c
movecurs.o:	movecurs.c
newmat.o:	newmat.c
outchar.o:	outchar.c
outstr.o:	outstr.c
printg.o:	printg.c
rawchar.o:	rawchar.c
rbon.o:	rbon.c
rdchar.o:	rdchar.c
readline.o:	readline.c
set.o:	set.c
setmat.o:	setmat.c
sync.o:	sync.c
texton.o:	texton.c
ttyinit.o:	ttyinit.c
update.o:	update.c
video.o:	video.c
zermat.o:	zermat.c
zoomn.o:	zoomn.c
zoomon.o:	zoomon.c
zoomout.o:	zoomout.c

