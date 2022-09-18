# 	@(#)Makelocal.mk	4.1	(ULTRIX)	7/17/90
include $(GMAKEVARS)

LNAME = libPW.a
CFLAGS = -O
CINCLUDES=-I.. -I../../hdr

OBJ =	anystr.o bal.o curdir.o fdfopen.o giveup.o strspn.o strpbrk.o strtok.o \
	imatch.o index.o lockit.o logname.o move.o patoi.o \
 	patol.o regcmp.o regex.o rename.o repeat.o repl.o satoi.o \
 	setsig.o sname.o strend.o substr.o trnslat.o userdir.o \
 	username.o verify.o any.o xalloc.o xcreat.o xlink.o \
 	xopen.o xpipe.o xunlink.o xwrite.o xmsg.o \
 	cat.o dname.o fatal.o clean.o userexit.o zero.o zeropad.o

$(OBJ):
	$(CCCMD) ../$(@:.o=.c)

all:	$(LNAME)

$(LNAME): $(OBJ)
	-rm -f $(LNAME)
	ar r $(LNAME) $(OBJ)
	ranlib $(LNAME)

anystr.o dname.o fatal.o fdfopen.o lockit.o satoi.o setsig.o xcreat.o \
	xmsg.o: ../../hdr/macros.h


fatal.o rename.o : ../../hdr/fatal.h 

pretools tools1 install:

anystr.o:	../anystr.c
bal.o:	../bal.c
curdir.o:	../curdir.c
fdfopen.o:	../fdfopen.c
giveup.o:	../giveup.c
strspn.o:	../strspn.c
strpbrk.o:	../strpbrk.c
strtok.o:	../strtok.c
imatch.o:	../imatch.c
index.o:	../index.c
lockit.o:	../lockit.c
logname.o:	../logname.c
move.o:	../move.c
patoi.o:	../patoi.c
patol.o:	../patol.c
regcmp.o:	../regcmp.c
regex.o:	../regex.c
rename.o:	../rename.c
repeat.o:	../repeat.c
repl.o:	../repl.c
satoi.o:	../satoi.c
setsig.o:	../setsig.c
sname.o:	../sname.c
strend.o:	../strend.c
substr.o:	../substr.c
trnslat.o:	../trnslat.c
userdir.o:	../userdir.c
username.o:	../username.c
verify.o:	../verify.c
any.o:	../any.c
xalloc.o:	../xalloc.c
xcreat.o:	../xcreat.c
xlink.o:	../xlink.c
xopen.o:	../xopen.c
xpipe.o:	../xpipe.c
xunlink.o:	../xunlink.c
xwrite.o:	../xwrite.c
xmsg.o:	../xmsg.c
cat.o:	../cat.c
dname.o:	../dname.c
fatal.o:	../fatal.c
clean.o:	../clean.c
userexit.o:	../userexit.c
zero.o:	../zero.c
zeropad.o:	../zeropad.c

include $(GMAKERULES)
