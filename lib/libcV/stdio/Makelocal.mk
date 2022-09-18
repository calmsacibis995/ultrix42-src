#  @(#)Makelocal.mk	4.1  ULTRIX  7/3/90

include $(GMAKEVARS)

COMMON=	clrerr.c ctermid.c cuserid.c doscan.c doopen.c\
	fclose.c fdopen.c fgetc.c fgets.c filbuf.c findiop.c \
	flsbuf.c fopen.c fprintf.c fputc.c fputs.c fread.c \
	freopen.c fseek.c ftell.c fwrite.c\
	getchar.c gets.c getw.c printf.c putchar.c puts.c \
	putw.c rew.c scanf.c setbuf.c setbuffer.c setvbuf.c \
	sprintf.c strout.c tempnam.c tmpfile.c tmpnam.c \
	ungetc.c vfprintf.c vprintf.c vsprintf.c

OBJS=	clrerr.o ctermid.o cuserid.o doscan.o doopen.o\
	fclose.o fdopen.o fgetc.o fgets.o filbuf.o findiop.o \
	flsbuf.o fopen.o fprintf.o fputc.o fputs.o fread.o \
	freopen.o fseek.o ftell.o fwrite.o\
	getchar.o gets.o getw.o printf.o putchar.o puts.o \
	putw.o rew.o scanf.o setbuf.o setbuffer.o setvbuf.o \
	sprintf.o strout.o tempnam.o tmpfile.o tmpnam.o \
	ungetc.o vfprintf.o vprintf.o vsprintf.o

include ../Makelocal_$(MACHINE).mk

clrerr.o:		clrerr.c
ctermid.o:		ctermid.c
cuserid.o:		cuserid.c
doscan.o:		doscan.c
doopen.o:		doopen.c
fclose.o:		fclose.c
fdopen.o:		fdopen.c
fgetc.o:		fgetc.c
fgets.o:		fgets.c
filbuf.o:		filbuf.c
findiop.o:		findiop.c
flsbuf.o:		flsbuf.c
fopen.o:		fopen.c
fprintf.o:		fprintf.c
fputc.o:		fputc.c
fputs.o:		fputs.c
fread.o:		fread.c
freopen.o:		freopen.c
fseek.o:		fseek.c
ftell.o:		ftell.c
fwrite.o:		fwrite.c
getchar.o:		getchar.c
gets.o:			gets.c
getw.o:			getw.c
printf.o:		printf.c
putchar.o:		putchar.c
puts.o:			puts.c
putw.o:			putw.c
rew.o:			rew.c
scanf.o:		scanf.c
setbuf.o:		setbuf.c
setbuffer.o:		setbuffer.c
setvbuf.o:		setvbuf.c
sprintf.o:		sprintf.c
strout.o:		strout.c
tempnam.o:		tempnam.c
tmpfile.o:		tmpfile.c
tmpnam.o:		tmpnam.c
ungetc.o:		ungetc.c
vfprintf.o:		vfprintf.c
vprintf.o:		vprintf.c
vsprintf.o:		vsprintf.c

$(COMMON):
	$(RM) $@
	$(LN) -s ../../../libc/stdio/$@ $@

include $(GMAKERULES)
