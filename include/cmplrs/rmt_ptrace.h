#ifndef rmt_ptrace_h
#define rmt_ptrace_h

#define PT_DBREAD	14
#define PT_DBWRITE	15
#define PT_DHREAD	16
#define PT_DHWRITE	17
#define PT_QUERY	18

enum ptrace_mode_t {host_debug, sproc_debug, sable_debug, pixie_debug,
	prom_debug};


/* some of the prototypes are commented out because they refer to
   typedef names that are local to dbx */
/* the ptrace prototype below is not as type strong as it could be,
   because "address" is not always an address */

/* ptrace.c functions */
ptrace_mux ();
void set_ptrace_mode( enum ptrace_mode_t pmode );
enum ptrace_mode_t get_ptrace_mode( void );
void set_ptrace_bp_op( unsigned long bp_op );
unsigned get_ptrace_bp_op( void );
void set_debug_ptrace( int on_off );
int is_debug_ptrace( void );
void set_debug_comm( int on_off );
int is_debug_comm( void );
void set_debug_protocol( int on_off );
int is_debug_protocol( void );


/* dbgmon functions */
void prom_init( void );
prom_ptrace ();
int prom_RdRespone ( int chrequest, int pidrequest, int *data);
void prom_BootRemote ( char *progname, char *args[], unsigned long entry_point,
		       int auto_load, char *dbgmon_pathname, int qualify_name);
void prom_download ( char *progname, char **args,
		     int qualify_name, int just_load);
int prom_InitRemote ( char *pdbxport, int use_sockets, char *service_name,
		      int sync);
void prom_sigint_handler( void );


/* sable functions */
sable_ptrace ();
void sable_BootRemote ( char *progname, char *args[] );
int sable_InitRemote ( int use_sockets );
void sable_sigint_handler( void );


/* pixie functions */
void pixie_init( /* char *pixie_objname, int pixie_textbase,
		 int pixie_textoffset, FILE *pixie_objfile */ );
void set_pixie_base (  long base );
int pixie_map_addr( int addr);
int mipspc_from_pixiepc (int pid, int pcPixie);
pixie_ptrace();
#endif
