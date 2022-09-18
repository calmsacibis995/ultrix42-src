#  @(#)Makelocal.mk	4.1  ULTRIX  7/17/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/usr/lib $(DESTROOT)/usr/bin

# DICT is the full pathname of the file dict.d, the
#	dictionary file used by DICTION
DICT= -DDICT=\"/usr/lib/dict.d\"
CDEFINES=$(DICT)
LFLAGS=$(CFLAGS)

all: diction style1 style2 style3

diction: dprog
dprog: diction.c
	$(CC) $(LFLAGS) $(CDEFINES) ../diction.c -o dprog

style1: wdslex.o
	$(CC) wdslex.o -ll  -o style1
wdslex.o: wdslex.c nhash.c dict.c ydict.c names.h abbrev.c
	$(CC) -c $(CINCLUDES) wdslex.c
wdslex.c: nwords.l
	$(LEX) ../nwords.l
	$(MV) lex.yy.c wdslex.c

style2: endlex.o
	$(CC) endlex.o -ll  -o style2
endlex.o: names.h endlex.c ehash.c edict.c
	$(CC) -c $(CINCLUDES) endlex.c
endlex.c: end.l
	$(LEX) ../end.l
	$(MV) lex.yy.c endlex.c

style3: prtlex.o pscan.o outp.o
	$(CC) prtlex.o pscan.o outp.o -ll  -o style3
prtlex.o: names.h prtlex.c conp.h style.h names.h
	$(CC) -c $(CFLAGS) $(CINCLUDES) prtlex.c
prtlex.c: part.l
	$(LEX) ../part.l
	$(MV) lex.yy.c prtlex.c
pscan.o: names.h conp.h pscan.c
	$(CC) -c $(CFLAGS) $(CINCLUDES) ../pscan.c
outp.o: names.h conp.h style.h outp.c
	$(CC) -c $(CFLAGS) $(CINCLUDES) ../outp.c

install:
	install -c -s style1 ${DESTROOT}/usr/lib/style1
	install -c -s style2 ${DESTROOT}/usr/lib/style2
	install -c -s style3 ${DESTROOT}/usr/lib/style3
	install -c -s dprog ${DESTROOT}/usr/lib/dprog
	install -c ../style.sh ${DESTROOT}/usr/bin/style
	install -c ../diction.sh ${DESTROOT}/usr/bin/diction
	install -c ../explain.sh ${DESTROOT}/usr/bin/explain
	install -c ../dict.d ${DESTROOT}/usr/lib/dict.d
	install -c ../explain.d ${DESTROOT}/usr/lib/explain.d

include $(GMAKERULES)
