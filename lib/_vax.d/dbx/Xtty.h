/*
 * sccsid: "@(#)Xtty.h	4.1 Ultrix	7/3/90"
 *
 * Derived from: 
 * $Header: Xtty.h,v 10.3 86/02/01 15:42:44 tony Rel $
 */
/*
 * Note.mjg: xdb doesn't need the window ID, so by commenting it out
 * we eliminate the need to include Xlib.h.
 */
typedef struct _TTYWindow {
/*** 	Window w;		 * The window id */
	int pid;		/* The pid of the subprocess xterm */
	short file;		/* The file id of the tty to read and write 
	    	    	    	   characters to/from */
} TTYWindow;

TTYWindow *CreateTTYWindow();
