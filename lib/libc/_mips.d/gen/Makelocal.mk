#  @(#)Makelocal.mk	4.2  ULTRIX  10/12/90

include $(GMAKEVARS)

STDOBJS=_setjmp.o abort.o abs.o atod.o bcmp.o bcopy.o bzero.o div.o \
	dtoa.o dwmultu.o environ.o execute_br.o \
	fabs.o ffs.o fp_class.o fp_control.o \
	fp_instr.o frexp.o index.o insque.o isnan.o ldexp.o loadstore.o \
	modf.o \
	remque.o rindex.o setjmp.o strcat.o strncat.o strcmp.o  \
	strcpy.o tenscale.o atof.o ecvt.o emulate_br.o \
	setjmperr.o sigbus.o strlen.o strncpy.o alloca.o wbflush.o \
	swap_lw_bytes.o swap_word_bytes.o swap_words.o 

all: $(STDOBJS) fpi_sigfpe.o

fpi_sigfpe.o:	fpi_sigfpe.c
	$(CC) -c -DBSD -f $(CFLAGS) $(CINCLUDES) -G 0 ../fpi_sigfpe.c
	$(MV) fpi_sigfpe.o G0/fpi_sigfpe.o
	$(CC) -c -DBSD -f $(CFLAGS) $(CINCLUDES) ../fpi_sigfpe.c

_setjmp.o:			_setjmp.s
abort.o:			abort.s
abs.o:				abs.s
alloca.o:			alloca.c
atod.o:				atod.s
atof.o:				atof.c
bcmp.o:				bcmp.s
bcopy.o:			bcopy.s
bzero.o:			bzero.s
div.o:				div.s
dtoa.o:				dtoa.s
dwmultu.o:			dwmultu.s
ecvt.o:				ecvt.c
emulate_br.o:			emulate_br.c
environ.o:			environ.s
execute_br.o:			execute_br.s
fabs.o:				fabs.s
ffs.o:				ffs.s
fp_class.o:			fp_class.s
fp_control.o:			fp_control.s
fp_instr.o:			fp_instr.s
frexp.o:			frexp.s
index.o:			index.s
insque.o:			insque.s
isnan.o:			isnan.c
ldexp.o:			ldexp.s
loadstore.o:			loadstore.s
modf.o:				modf.s
remque.o:			remque.s
rindex.o:			rindex.s
setjmp.o:			setjmp.s
setjmperr.o:			setjmperr.c
sigbus.o:			sigbus.c
strcat.o:			strcat.s
strcmp.o:			strcmp.s
strcpy.o:			strcpy.s
strlen.o:			strlen.c
strncat.o:			strncat.c
strncpy.o:			strncpy.c
tenscale.o:			tenscale.s
swap_lw_bytes.o:		swap_lw_bytes.s
swap_word_bytes.o:		swap_word_bytes.s
swap_words.o:			swap_words.s
wbflush.o:			wbflush.c
xmalloc.o:			xmalloc.s

$(STDOBJS):
	$(CCCMD) -G 0 ../$<
	$(MV) $*.o G0/$*.o
	$(CCCMD) ../$<

clean: cleanG0

cleanG0:
	-$(RM) G0/*

include $(GMAKERULES)
