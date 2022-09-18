/*#@(#)runtime.c	4.1	Ultrix	7/17/90*/
/* $Header: runtime.c,v 1.5 84/05/19 11:40:08 powell Exp $ */
#include <stdio.h>
#include <signal.h>

/* This file contains routines used by the Modula-2 runtime */
/*  and the standard library math and unix modules */

int runtime__init_complete = 0;
int parameters_argc;
char **parameters_argv, **parameters_envp;
int unix_argc;
char **unix_argv, **unix_envp;
FILE *unix_stdin, *unix_stdout, *unix_stderr;

runtime__init(argc,argv,envp) int argc; char *argv[], *envp[]; {

    if (runtime__init_complete) return;
    runtime__init_complete = 1;

    parameters_argc = argc;
    parameters_argv = argv;
    parameters_envp = envp;
    unix_argc = argc;
    unix_argv = argv;
    unix_envp = envp;
    unix_stdin = stdin;
    unix_stdout = stdout;
    unix_stderr = stderr;
}
uexit(n) int n; {
    exit(n);
}
/* the following returns the program's cputime in milliseconds */
#include <time.h>
#include <sys/resource.h>
SYSTEM_cputime(){
    struct rusage ru;
    getrusage(0,&ru);
    return(ru.ru_utime.tv_sec*1000+ru.ru_utime.tv_usec/1000);
}

/* initialization for the SYSTEM module */

SYSTEM__init() {}

/* initialization for the bitoperations module */

BITOPERATIONS__init() {}

/* initialization for the IO module */

FILE *IO_INPUT, *IO_OUTPUT, *IO_TERMINAL;
int IO__initflag = 0;
IO__init(){
    if (IO__initflag) return;
    IO_INPUT = stdin;
    IO_OUTPUT = stdout;
    IO_TERMINAL = stderr;
}

/* math module */

#include <math.h>
math__init(){}
/* use int parameters to make C use one word (floats are passed as doubles) */
float math_sin(x) int x; { return(sin(x,0)); }
float math_cos(x) int x; { return(cos(x,0)); }
float math_atan(x) int x; { return(atan(x,0)); }
float math_atan2(x,y) int x, y; { return(atan2(x,0,y,0)); }
float math_exp(x) int x; { return(exp(x,0)); }
float math_sqrt(x) int x; { return(sqrt(x,0)); }
float math_log(x) int x; { return(log(x,0)); }
float math_ldexp(x,exp) int x; int exp; { return(ldexp(x,0,exp)); }

double math_longsin(x) double x; { return(sin(x)); }
double math_longcos(x) double x; { return(cos(x)); }
double math_longatan(x) double x; { return(atan(x)); }
double math_longatan2(x,y) double x, y; { return(atan2(x,y)); }
double math_longexp(x) double x; { return(exp(x)); }
double math_longsqrt(x) double x; { return(sqrt(x)); }
double math_longlog(x) double x; { return(log(x)); }
double math_longldexp(x,exp) double x; int exp; { return(ldexp(x,exp)); }

/* Error routines */

runtime__errorrange(val,min,max) int val, min, max; {
    fprintf(stderr,"Value %d is not in subrange [%d..%d]\n",val,min,max);
    abort();
}
runtime__errorsubscript(val,min,max) int val, min, max; {
    fprintf(stderr,"Subscript %d is out of range [%d..%d]\n",val,min,max);
    abort();
}
runtime__erroraddr(){
    fprintf(stderr,"Invalid pointer was dereferenced\n");
    abort();
}
runtime__errornoreturn(){
    fprintf(stderr,"No return statement at end of function\n");
    abort();
}
runtime__errorcase(){
    fprintf(stderr,"Case selector value does not match a label\n");
    abort();
}
runtime__errorvariant(t) int t; {
    fprintf(stderr,"Improper variant tag (%d) for field\n",t);
    abort();
}
runtime__errorassert(s) char *s; {
    fprintf(stderr,"Assertion failed: %s\n",s);
    abort();
}
