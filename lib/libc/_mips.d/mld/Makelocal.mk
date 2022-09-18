#  @(#)Makelocal.mk	4.1  ULTRIX  7/3/90

include $(GMAKEVARS)

CDEFINES=-DPORTAR -signed -DBSD

OBJS=	ldaclose.o ldaopen.o ldclose.o ldahread.o ldlseek.o \
	ldnlseek.o ldnrseek.o ldnsseek.o ldnshread.o ldopen.o \
	ldrseek.o ldlread.o ldsseek.o ldshread.o ldfhread.o \
	ldtbseek.o ldohseek.o ldtbread.o ldtbindex.o ldgetname.o \
	ldgetpd.o allocldptr.o freeldptr.o vldldptr.o sgetl.o sputl.o \
	ldgetaux.o ranhash.o ldgetrfd.o ldfsymorder.o ldfscnorder.o \
	stprint.o stfd.o stcu.o stfe.o stio.o sterror.o staux.o sex.o \
	swap.o disassembler.o stwarning.o stinternal.o nlist.o \
	ldfgptorder.o ldflitorder.o opnames.o neednop.o

ldaclose.o:		ldaclose.c
ldaopen.o:		ldaopen.c
ldclose.o:		ldclose.c
ldahread.o:		ldahread.c
ldlseek.o:		ldlseek.c
ldnlseek.o:		ldnlseek.c
ldnrseek.o:		ldnrseek.c
ldnsseek.o:		ldnsseek.c
ldnshread.o:		ldnshread.c
ldopen.o:		ldopen.c
ldrseek.o:		ldrseek.c
ldlread.o:		ldlread.c
ldsseek.o:		ldsseek.c
ldshread.o:		ldshread.c
ldfhread.o:		ldfhread.c
ldtbseek.o:		ldtbseek.c
ldohseek.o:		ldohseek.c
ldtbread.o:		ldtbread.c
ldtbindex.o:		ldtbindex.c
ldgetname.o:		ldgetname.c
ldgetpd.o:		ldgetpd.c
allocldptr.o:		allocldptr.c
freeldptr.o:		freeldptr.c
vldldptr.o:		vldldptr.c
sgetl.o:		sgetl.c
sputl.o:		sputl.c
ldgetaux.o:		ldgetaux.c
ranhash.o:		ranhash.c
ldgetrfd.o:		ldgetrfd.c
ldfsymorder.o:		ldfsymorder.c
ldfscnorder.o:		ldfscnorder.c
stprint.o:		stprint.c
stfd.o:			stfd.c
stcu.o:			stcu.c
stfe.o:			stfe.c
stio.o:			stio.c
sterror.o:		sterror.c
staux.o:		staux.c
sex.o:			sex.c
swap.o:			swap.c
disassembler.o:		disassembler.c
stwarning.o:		stwarning.c
stinternal.o:		stinternal.c
nlist.o:		nlist.c
ldfgptorder.o:		ldfgptorder.c
ldflitorder.o:		ldflitorder.c
opnames.o:		opnames.c
neednop.o:		neednop.c

$(OBJS): stext.h

stext.h: stio.c staux.c stprint.c stfd.c stfe.c stcu.c
	sed -f ../ext.sed ../stio.c ../staux.c ../stprint.c ../stfd.c \
		../stfe.c ../stcu.c > stext.h

$(OBJS):
	$(CCCMD) -G 0 ../$<
	$(MV) $*.o G0/$*.o
	$(CCCMD) ../$<

clean: cleanG0

cleanG0:
	-$(RM) G0/*

include $(GMAKERULES)
