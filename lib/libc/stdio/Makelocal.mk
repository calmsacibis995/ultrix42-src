#  @(#)Makelocal.mk	4.1  ULTRIX  7/3/90

include $(GMAKEVARS)

OBJS=	atexit.o cleanup.o clrerr.o ctermid.o cuserid.o data.o doscan.o \
	doopen.o exit.o \
	fclose.o fdopen.o feof.o ferror.o fgetc.o fgetpos.o fgets.o \
	filbuf.o fileno.o findiop.o flsbuf.o fopen.o \
	fprintf.o fputc.o fputs.o fread.o \
	freopen.o fseek.o fsetpos.o ftell.o fwrite.o \
	getc.o getchar.o gets.o getstdiobuf.o getw.o \
	printf.o putc.o putchar.o puts.o putw.o rew.o scanf.o \
	setbuf.o setbuffer.o setlinebuf.o setvbuf.o sibuf.o sobuf.o \
	sprintf.o strout.o tempnam.o tmpfile.o tmpnam.o \
	ungetc.o vfprintf.o vprintf.o vsprintf.o

atexit.o:		atexit.c
cleanup.o:		cleanup.c
clrerr.o:		clrerr.c
ctermid.o:		ctermid.c
cuserid.o:		cuserid.c
data.o:			data.c
doscan.o:		doscan.c
doopen.o:		doopen.c
exit.o:			exit.c
fclose.o:		fclose.c
fdopen.o:		fdopen.c
feof.o:			feof.c
ferror.o:		ferror.c
fgetc.o:		fgetc.c
fgetpos.o:		fgetpos.c
fgets.o:		fgets.c
filbuf.o:		filbuf.c
fileno.o:		fileno.c
findiop.o:		findiop.c
flsbuf.o:		flsbuf.c
fopen.o:		fopen.c
fprintf.o:		fprintf.c
fputc.o:		fputc.c
fputs.o:		fputs.c
fread.o:		fread.c
freopen.o:		freopen.c
fseek.o:		fseek.c
fsetpos.o:		fsetpos.c
ftell.o:		ftell.c
fwrite.o:		fwrite.c
getc.o:			getc.c
getchar.o:		getchar.c
gets.o:			gets.c
getstdiobuf.o:		getstdiobuf.c
getw.o:			getw.c
printf.o:		printf.c
putc.o:			putc.c
putchar.o:		putchar.c
puts.o:			puts.c
putw.o:			putw.c
rew.o:			rew.c
scanf.o:		scanf.c
setbuf.o:		setbuf.c
setbuffer.o:		setbuffer.c
setlinebuf.o:		setlinebuf.c
setvbuf.o:		setvbuf.c
sibuf.o:		sibuf.c
sobuf.o:		sobuf.c
sprintf.o:		sprintf.c
strout.o:		strout.c
tempnam.o:		tempnam.c
tmpfile.o:		tmpfile.c
tmpnam.o:		tmpnam.c
ungetc.o:		ungetc.c
vfprintf.o:		vfprintf.c
vprintf.o:		vprintf.c
vsprintf.o:		vsprintf.c

include ../Makelocal_$(MACHINE).mk

include $(GMAKERULES)
