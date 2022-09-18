/*
 *	@(#)ltferrs.h	4.1	(ULTRIX)	7/17/90	
 *
 */
/****************************************************************
 *								*
 *			Copyright (c) 1985 by			*
 *		Digital Equipment Corporation, Maynard, MA	*
 *			All rights reserved.			*
 *								*
 *   This software is furnished under a license and may be used *
 *   and copied  only  in accordance with the terms of such	*
 *   license and with the  inclusion  of  the  above  copyright *
 *   notice. This software  or  any  other copies thereof may	*
 *   not be provided or otherwise made available to any other	*
 *   person.  No title to and ownership of the software is	*
 *   hereby transferred.					*
 *								*
 *   The information in this software is subject to change	*
 *   without  notice  and should not be construed as a		*
 *   commitment by Digital  Equipment Corporation.		*
 *								*
 *   Digital assumes  no responsibility   for  the use  or	*
 *   reliability of its software on equipment which is not	*
 *   supplied by Digital.					*
 *								*
 ****************************************************************/
/**/
/*
 *
 *	File name:
 *
 *	    ltferrs.h
 *
 *	Source file description:
 *
 *		This file contains definitions of all
 *		Labeled Tape Facility (LTF) error messages
 *		and error messages macros.
 *
 *
 *	Functions:
 *
 *		n/a
 *
 *	Usage:
 *
 *		n/a
 *
 *	Compile:
 *
 *		#include "ltferrs.h"	Include local error message defs.
 *
 *	Modification history:
 *	~~~~~~~~~~~~~~~~~~~~
 *
 *	revision			comments
 *	--------	-----------------------------------------------
 *	 01.0		12-April-85	Ray Glaser
 *			Create orginal version.
 *	
 *	 01.1		4-Sep-85	Suzanne Logcher
 *			Put FATAL, Warning, or Info in errors
 *			Add some more
 */
/**/
/*
 * ->	General Print ERROR macro
 */

#define PERROR fprintf(stderr,
#define PROMPT fprintf(stderr,

/*
 * ->	Error messages.
 */
#ifdef MAINC	/* Only compile the actual messges when compiling the
		 * main line logic, else - define as externals below.
		 * (otherwise, all messages would be multiply defined
	         * at link time.
		 */
/*_A_*/
char *ALTERN	= "Alternative pathname (return to skip extract)?";
char *ANSIV	= "  Volume is:  ANSI Version";

/*_B_*/
char *BADCENT	= "Warning > Invalid century in creation date, 20th century default used";
char *BADCNT1	= "Warning > For file";
char *BADCNT2	= "File character count =";
char *BADCNT3	= "!= bytes read from volume =";
char *BADST	= "FATAL > Bad stat call on path name ->";
char BELL	= {007};
char *BFRCNE	= "FATAL > Begining & final FUF record counts not equal ->";
char *BYTE	= " byte";
char *BYTES	= " bytes";

/*_C_*/
char *CANTBUF	= "FATAL > Cannot read buffer offset field of size ->";
char *CANTCGET	= "Warning > Cannot get status of device ->";
char *CANTCLS	= "FATAL > Cannot close device ->";
char *CANTCHD	= "FATAL > Cannot change directory to ->";
char *CANTCHW	= "Warning > Cannot change directory to ->";
char *CANTCRE	= "FATAL > Cannot create file ->";
char *CANTDEVIO	= "Warning > Cannot perform DEVIOCGET ioctl call on device ->";
char *CANTFPW	= "Warning > Cannot find user name in password file, UID = ";
char *CANTFSF	= "FATAL > Cannot forwardspace (file) ->";
char *CANTL1	= "Warning > Cannot find link file for ->";
char *CANTLF	= "Warning > Cannot link";
char *CANTMKD	= "FATAL > Cannot make directory ->";
char *CANTOD	= "FATAL > Cannot open directory ->";
char *CANTOPEN	= "FATAL > Cannot open ->";
char *CANTOPW	= "Warning > Cannot open ->";
char *CANTPER	= "Warning > Can only use p key with x option";
char *CANTRL	= "FATAL > Cannot read label ->";
char *CANTRD	= "FATAL > Cannot read from ->";
char *CANTREW	= "FATAL > Cannot rewind. Busy or not online ->";
char *CANTRSL	= "Warning > Cannot read symbolic link ->";
char *CANTSTS	= "Warning > Cannot execute file stat call for symbolic link file of ->";
char *CANTSTW	= "Warning > Cannot execute file stat call for file ->";
char *CANTWEOF = "FATAL > Cannot write EOF on ->";
char *CANTWVL	= "FATAL > Cannot write VOL1 on ->";
char *CONFF	= "FATAL > Options c, t, and x are mutually exclusive";

/*_D_*/
char *DIRCRE	= " (directory created)";

/*_E_*/
char *ENFNAM	= "Enter FILE name, or return to quit or end input > ";
char *EINVLD	= "FATAL > -- END INVALID LABEL DATA --";
char *EOFINM	= "FATAL > EOF encountered in middle of file ->";
char *ERRDEV	= "FATAL > Error on device";
char *ERREOT	= "FATAL > End of Tape (EOT) encountered on ->";
char *ERRUNIT	= "device unit #";
char *ERRWRF	= "FATAL > Error writing file ->";
char *EXISTS	= "Warning > File already exists ->";

/*_F_*/
char *FILENNG	= "Warning > Original file name cannot be reproduced on non-Ultrix systems";
char *FNTL	= "Warning > File name too long for ANSI label set ->";
char *FSTCB	= "Warning > First control byte in FUF is ->";
char *FUFTL	= "FATAL > Fortran Unformatted File record too long";

/*_G_*/
char *GETWDF	= "FATAL > Get working directory call (getwd) failure ->";

/*_H_*/
#ifndef U11	/* JSD: ULTRIX-11 has it's own help command */
char *HELP1	= "One of the following options enclosed in {} is required\n";
char *HELP2	= "c = create a new volume, previous content is overwritten";
char *HELP3	= "H = help mode, print this summary";
char *HELP4	= "t = table the contents of the volume";
char *HELP5	= "x = extract files from the volume\n";
char *HELP6	= "Items enclosed in second [] are optional keys\n";
char *HELP7	= "a = output ANSI Version 3 format to volume";
char *HELP8	= "h = output file pointed to by a symbolic link instead of symbolic link file";
char *HELP9	= "o = omit outputting directory blocks to volume";
char *HELP10	= "O = omit the usage of headers 3 to 9";
char *HELP11	= "p = change permissions and owner of extracted files to original values,";
char *HELP12	= "    must be super user, used only in extraction";
char *HELP13	= "v = verbose mode, provide additional information about files/operation";
char *HELP14	= "V = big verbose mode, include directory information in table of contents";
char *HELP15	= "w = warn if a file name is truncated on creation or may be overwritten";
char *HELP16	= "    on extract";
char *HELP17	= "0..31 = select the unit number for the named tape device\n";
char *HELP18	= "Press RETURN to continue ...";
char *HELP19	= "Items enclosed in third [] are optional keys & require respective";
char *HELP20	= "arguments\n";
char *HELP21	= "B = set blocksize, max = 20480 bytes, default = 2048 bytes,";
char *HELP22	= "    min = 18 bytes, used only in creation";
char *HELP23	= "f = set device name, default = /dev/rmt0h";
char *HELP24	= "I = set input method, either by stdin or by providing a file name";
char *HELP25	= "L = set volume label, maximum six characters";
char *HELP26	= "P = set position number in form of #,# with # > 0, not used in creation";
#endif NOT U11	/* JSD */

char *HLINKTO	= "Info > Hard link to ->";
char *HOSTF	= "FATAL > Call to gethostname (HOSTNM) failed";

/*_I_*/
char *IMPIDC	= "Warning > Implementation ID changed to ->";
char *IMPIDM	= " Implementation ID is: ";
char *INTERCH	= "Interchange Name =";
char *INVBS	= "FATAL > Invalid block size. Min = 18 bytes, Max =";
char *INVOWN	= "FATAL > Non  'a'  characters in  OWNER ID  ->";
char *INVLD	= "FATAL > -- INVALID LABEL DATA FOLLOWS --";
char *INVLF	= "FATAL > Invalid label format in ->";
char *INVLFS	= "Warning > Skipping Unsupported Tape Label -> ";
char *INVLFW	= "Warning > Reading Tape Volume Label -> VOL";
char *INVLNO	= " (invalid label number)";
char *INVNF	= "FATAL > Invalid numeric format in ->";
char *INVPN	= "FATAL > Invalid position sequence number -> ";
char *INVPNUSE	= "Warning > P key only used with t or x option ";
char *INVPS	= "FATAL > Invalid position section number -> ";
#ifndef U11
char *INVUNIT	= "FATAL > Invalid unit number.  Min = 0, Max = 31";
#else
char *INVRS	= "FATAL > Invalid record length. Min = 1 byte, Max =";
#endif
char *INVVID1	= "FATAL > Invalid characters in L key (see LTF(5))";
char *INVVID2	= "Warning > 'Z' Indicates invalid character(s) ->";

/*_J_*/ /*_K_*/

/*_L_*/
/*_M_*/
char *MHL	= "Head link file not extracted ?\n";
char *MS1	= "The  -u  key has precedence over the  -d  key";
char *MS2	= "The  -u  key does not apply when extracting binary files";
char *MS3	= "The  -d  key does not apply when extracting binary files";
char *MULTIV1	= "FATAL > EOV label encountered. Last input file not complete";

/*_N_*/
char *NOARGS	= "FATAL > No file names specified for c option";
char *NOBLK	= "FATAL > No blocksize specified with B key";
char *NOFIL	= "FATAL > No device specified with f key";
char *NOFUNC	= "FATAL > No option specified";
char *NOINP	= "FATAL > No input specified with I key";
char *NOMEM	= "FATAL > No free memory, exiting ...";
char *NOMDIR	= "Warning > No free memory for directory list";
char *NONAFN	= "Warning > Non 'A' characters in file name ->";
char *NOPOS	= "FATAL > No posnmbr specified with P key";
#ifdef U11
char *NOREC	= "FATAL > No reclen specified with R key";
#endif
char *NOTEX	= "FATAL > File not extracted";
char *NOTONP	= "Warning > File not found on volume after position number ->";
char *NOTONV	= "Warning > File not found on volume ->";
char *NOTSU	= "Warning > Not super user, cannot use p key with uid ->";
char *NOVALFI	= "FATAL > No valid files in argument list";
char *NOVOL	= "FATAL > No volumeid specified with L key";

/*_O_*/
char *OFFL1	= ", Place";
char *OFFL2	= "ONLINE";
char *OWNRID	= " Owner  ID is: ";
char *OVRWRT	= "Overwrite (y/n return=no) ?";

/*_P_*/ /*_Q_*/

/*_R_*/
#ifdef U11
char *RECLTS	= "FATAL > Record length too short";
#endif

/*_S_*/
char *SCNDCB	= "Warning > Second control byte in FUF is ->";
char *SLINKTO	= "Info > Symbolic link to ->";
char *SPCLDF	= "Warning > Cannot dump special device file ->";
char *STOPCRIN	= "Press y to quit OR return to skip unknown file when volume is created >";

/*_T_*/
#if 0
char *TAPEB	= " Tape block";
char *TAPEBS	= " Tape blocks";
#endif
char *TMA	= "FATAL > Too many arguments, out of memory";
#ifndef U11
char *TRYHELP	= "Info > Type ltf H for explanation of the usage of the switches ";
#else
char *TRYHELP	= "Use 'help ltf' for an explanation of the switches.";
#endif
char *TRYNH3	= "Warning > Try reading the volume with the O key (Noheader3)";

/*_U_*/
char *UNQ	= "FATAL > Unknown key ->";
char *USEDF	= "Warning > Ltf file type determination being overwritten for file ->";
#ifndef U11
char *USE1	= "usage: ltf [-]{cHtx}[ahoOpvVw0..31][BfILP] [blocksize] [devicefilename]";
char *USE2	= "       [inputfile] [volumelabel] [positionnumber] [files...]\n";
#else
char *USE1	= "usage: ltf [-]{ctx} [aghknoOpvVw0..9] [BfILPR] [blocksize] [devicefilename]";
char *USE2	= "       [inputfile] [volumelabel] [positionnumber] [recordlength] [files...]\n";
#endif
char *UNSAV	= " (unsupported ANSI version)";

/*_V_*/
char *VOLCRE	= " Volume  created   on: ";
char *VOLIDTL	= "FATAL > Maximum volume id length is 6 a-characters";
char *VOLIS	= " Volume ID is: ";

/*_W_*/
char *WRLINM	= "FATAL > Wrong record length in middle of file ->";
char *WRTLCK	= ", WRITE ENABLE ->";

/*_X_*/ /*_Y_*/ /*_Z_*/

#else	/* When compiling sub-modules, define error messages as
	 * externals to avoid multiply defined errors from the
	 * linkage editor.
	 */
/*_A_e_*/
extern char *ALTERN, *ANSIV;

/*_B_e_*/
extern char *BADCENT, *BADCNT1, *BADCNT2, *BADCNT3;
extern char *BADST;
extern char BELL;
extern char *BFRCNE, *BYTE, *BYTES;

/*_C_e_*/
extern char *CANTBUF, *CANTCGET, *CANTCLS, *CANTCHD;
extern char *CANTCHW, *CANTCRE, *CANTDEVIO, *CANTFPW, *CANTFSF;
extern char *CANTL1, *CANTLF, *CANTMKD, *CANTOD, *CANTOPEN, *CANTOPW;
extern char *CANTPER, *CANTRL, *CANTRD, *CANTREW, *CANTRSL; 
extern char *CANTSTS, *CANTSTW, *CANTWEOF, *CANTWVL, *CONFF;

/*_D_e_*/
extern char *DIRCRE;

/*_E_e_*/
extern char *ENFNAM, *EINVLD, *EOFINM, *ERREOT, *ERRWRF, *EXISTS;
extern char *ERRDEV, *ERRUNIT;

/*_F_e_*/
extern char *FILENNG, *FNTL, *FSTCB, *FUFTL;

/*_G_e_*/
extern char *GETWDF;

/*_H_e_*/
#ifndef U11
extern char *HELP1, *HELP2, *HELP2, *HELP3, *HELP4, *HELP5, *HELP6;
extern char *HELP7, *HELP8, *HELP9, *HELP10, *HELP11, *HELP12;
extern char *HELP13, *HELP14, *HELP15, *HELP16, *HELP17, *HELP18;
extern char *HELP19, *HELP20, *HELP21, *HELP22, *HELP23, *HELP24;
extern char *HELP25, *HELP26;
#endif NOT U11
extern char *HLINKTO, *HOSTF;

/*_I_e_*/
extern char *IMPIDC, *IMPIDM, *INTERCH;
extern char *INVBS, *INVLD, *INVLF, *INVLFS, *INVLFW, *INVLNO, *INVNF;
extern char *INVOWN, *INVPN, *INVPNUSE, *INVPS;
#ifndef U11
extern char *INVUNIT;
#else
extern char *INVRS;
#endif
extern char *INVVID1, *INVVID2;

/*_J_e_*/ /*_K_e_*/

/*_L_e_*/
/*_M_e_*/
extern char *MHL, *MS1, *MS2, *MS3, *MULTIV1;

/*_N_e_*/
extern char *NOFUNC, *NOFIL, *NOMEM, *NOMDIR, *NONAFN, *NOTEX, *NOTONP; 
extern char *NOTONV, *NOTSU, *NOVALFI, *NOVOL, *NOARGS, *NOBLK, *NOPOS;
#ifdef U11
extern char *NOREC;
#endif
extern char *NOINP;
 
/*_O_e_*/
extern char *OFFL1, *OFFL2, *OWNRID, *OVRWRT;

/*_P_e_*/ /*_Q_e_*/

/*_R_e_*/
#ifdef U11
extern char *RECLTS;
#endif

/*_S_e_*/
extern char *SCNDCB, *SLINKTO, *SPCLDF, *STOPCRIN;


/*_T_e_*/
#if 0
extern char *TAPEB, *TAPEBS;
#endif
extern char *TMA, *TRYHELP, *TRYNH3;

/*_U_e_*/
extern char *UNQ, *USEDF, *USE1, *USE2, *UNSAV;

/*_V_e_*/
extern char *VOLCRE, *VOLIDTL, *VOLIS;

/*_W_e_*/
extern char *WRLINM, *WRTLCK;

/*_X_e_*/ /*_Y_e_*/ /*_Z_e_*/

#endif

/**\\**\\**\\**\\**\\**  EOM  ltferrs.h  **\\**\\**\\**\\**\\*/
/**\\**\\**\\**\\**\\**  EOM  ltferrs.h  **\\**\\**\\**\\**\\*/
