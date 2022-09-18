# @(#)Makelocal.mk	4.1	ULTRIX	7/3/90

include $(GMAKEVARS)

COMMON=	doopen.c doscan.c \
	fclose.c fdopen.c fopen.c fprintf.c fread.c freopen.c fwrite.c \
	printf.c scanf.c sprintf.c ungetc.c

OBJS=	doopen.o doscan.o \
	fclose.o fdopen.o fopen.o fprintf.o fread.o freopen.o fwrite.o \
	printf.o scanf.o sprintf.o ungetc.o

include ../Makelocal_$(MACHINE).mk

doopen.o:		doopen.c
doscan.o:		doscan.c
fclose.o:		fclose.c
fdopen.o:		fdopen.c
fopen.o:		fopen.c
fprintf.o:		fprintf.c
fread.o:		fread.c
freopen.o:		freopen.c
fwrite.o:		fwrite.c
printf.o:		printf.c
scanf.o:		scanf.c
sprintf.o:		sprintf.c
ungetc.o:		ungetc.c

$(COMMON):
	$(RM) $@
	$(LN) -s ../../../libc/stdio/$@ $@

include $(GMAKERULES)
