/*	@(#)mac.h	4.1 (Ultrix) 7/2/90	*/
/*	Orig ID: mac.h	4.2	82/12/24	*/

#
/*
 *	UNIX shell
 *
 *	S. R. Bourne
 *	Bell Telephone Laboratories
 *
 */

#define LOCAL	static
#define PROC	extern
#define TYPE	typedef
#define STRUCT	TYPE struct
#define UNION	TYPE union
#define REG	register

#define IF	if(
#define THEN	){
#define ELSE	} else {
#define ELIF	} else if (
#define FI	;}

#define BEGIN	{
#define END	}
#define SWITCH	switch(
#define IN	){
#define ENDSW	}
#define FOR	for(
#define WHILE	while(
#define DO	){
#define OD	;}
#define REP	do{
#define PER	}while(
#undef DONE
#define DONE	);
#define LOOP	for(;;){
#define POOL	}


#define SKIP	;
#define DIV	/
#define REM	%
#define NEQ	^
#define ANDF	&&
#define ORF	||

#define TRUE	(-1)
#define FALSE	0
#define LOBYTE	0377
#define STRIP	0177
#define QUOTE	0200

#define EOF	0
#define NL	'\n'
#define SP	' '
#define LQ	'`'
#define RQ	'\''
#define MINUS	'-'
#define COLON	':'
