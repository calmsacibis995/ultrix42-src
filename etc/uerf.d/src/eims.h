/*  sccsid  =  @(#)eims.h	4.2   (ULTRIX)   9/11/90  */
/*
*	.TITLE	- EIMS.H literals
*	.IDENT	/1-001/
*
*++
*
* FACILITY:		[ EIMS - EIMS Record Management System ]
*
* ABSTRACT:
*
*	This module defines all global compile time literals used by
*	EIMS and modules that call EIMS. These include literals
*       that are passed as a constant paramater, error and information
*	return values, and other compile time constants.
*	
* ENVIRONMENT:	VAX/VMS C,  ULTRIX-32 C
*
* AUTHOR:  Don Zaremba,  CREATION DATE:  29-May-86
*
* MODIFIED BY:
*
*   NOTE: The values associated with each literal are subject to 
*	  change. No program should be written assuming it has 
*	  knowledge of these values.
*
*/
#ifndef EIMS_H
#define EIMS_H

/* ERMS facility code */
#define ES$ 1

/*MSGBEGIN*/
/* Error and Information messages */
#define	 ES$SUCC 1001	/* Success */
#define  ES$FAIL 1002	/* Failure */
#define	 ES$NODB 1010	/* Data Base not found */
#define  ES$STRM 1011	/* Un#defined stream value */
#define  ES$FNOF 1012	/* File not found */
#define  ES$EXIS 1013	/* File exists */
#define  ES$EOF  1014    /* End of file */
#define	 ES$BDEX 1015	/* Bad expression */
#define  ES$ARGS 1016	/* Bad argument count */
#define  ES$NOFLD 1017	/* Invalid EIMS field number */
#define  ES$TPER 1018	/* Type error */
#define  ES$NOADS 1019  /* No more ads segments */
#define  ES$OPENMODE 1020 /* Bad open mode value */
#define  ES$LOGICFILE 1021 /* Bad logical file */
#define  ES$NOREC 1022 /* Record not found on a read statement */
#define  ES$DUP 1023 /* Duplicate record */
#define ES$NOMEM 1024 /* Insufficient memory to allocate space */
#define ES$BADFMT 1025 /* Bad format */
#define ES$BADOPCODE 1026 /* Bad opcode */
#define ES$TBLERR 1027 /* Table lookup error */
/* DDTBL return codes */
#define DD$SUCC 1028 /* Success table lookup */
#define DD$FAIL 1029 /* Failed table lookup */
/*MSGEND*/


/* Paramter Constants */
#define	 ES$REMEMBER 1
#define  ES$FORGET 2
#define  ES$CONFIG 3   /* Configuration file */
#define  ES$EVENT  4   /* Event file */
#define  ES$SUMMARY 5  /* Summary file */
#define  ES$VIEW  6
#define  ES$MODIFY 7
#define  ES$CREATE 8
#define  ES$APPEND 9
#define  ES$NEXT 10
#define  ES$FIRST 11
#define  ES$RESTART 12
#define  ES$GLOBAL 13
#define  ES$LOCAL 14
#define  ES$MAILBOX 15
#define  ES$ON 16
#define  ES$OFF 17

#define  ES$AND 1
#define  ES$OR 2
#define  ES$NOT 3
#define  ES$EQUAL 4
#define  ES$RANGE 5
#define  ES$LT 6
#define  ES$LE 7
#define  ES$GT 8
#define  ES$GE 9

#define  ES$EIMS 1
#define  ES$USER 2

/* Standard record sizes */
#define	 ES$ADSMAX 10	/* Maximum number of ADS's */
#define  ES$EISIZE 800
#define  ES$DISIZE 800
#define  ES$SDSIZE 800
#define  ES$CDSIZE 4500
#define  ES$ADSIZE 4500
#define  ES$SISIZE 2000
#define  ES$CISIZE 1000

#define ES$EISVBA 4
#define ES$DISVBA 4
#define ES$SDSVBA 4
#define ES$CDSVBA 4
#define ES$ADSVBA 4
#define ES$SISVBA 4
#define ES$CISVBA 4


/* Segment types as used in segment_header */
#define ES$EIS 1    
#define ES$DIS 2    
#define ES$SDS 3
#define ES$CDS 4
#define ES$ADS 5
#define ES$SIS 6
#define ES$CIS 7

/* EIMS data types */
#define EM$SHORT 1
#define EM$LONG 2
#define EM$DATE 3
#define EM$STRING 4
#define EM$SHORTINDEX 5
#define EM$LONGINDEX 6


/*
*--
*/
#endif EIMS_H
