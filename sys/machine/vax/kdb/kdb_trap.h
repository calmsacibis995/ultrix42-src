/*
 * @(#)kdb_trap.h	4.1	ULTRIX	7/2/90
 */
#define include_kdb() 						 \
 								 \
	case T_KDB_ENTRY:	/* enter kdb */			 	 \
	{							 \
		extern enter_kdb_now;				 \
		if (enter_kdb_now != 1) 			 \
			goto kdb_goto_label;/* arith fault in kernel */  \
		else enter_kdb_now = 0; 			 \
		/* fall through */ 				 \
	}							 \
	case T_BPTFLT:		/* bpt instruction fault */ 	 \
	case T_TRCTRAP:		/* trace trap */ 		 \
	{							 \
		extern int kdb_saved_ipl;			 \
		extern int in_kdb;				 \
		extern char *kdb_stack_ptr;			 \
		extern int kdb_bkpt_type;			 \
		extern char *saved_stack_ptr;			 \
		extern int *kdb_pc_ptr, *kdb_psl_ptr;		 \
		/* 						 \
		 * This lets us into kdb three times, more than  \
		 * that and we are probably in an infinite loop: \
		 * This at least lets us get the dump. 		 \
		 */ 						 \
		locr0[PS] &= ~PSL_T; 				 \
		i = SIGTRAP; 					 \
		kdb_bkpt_type = type; 				 \
		if (in_kdb++ > 2) 				 \
			goto kdb_goto_label;			 \
		kdb_pc_ptr = &pc;				 \
		kdb_psl_ptr = &psl;				 \
		if (in_kdb == 1)				\
			kdb_saved_ipl = splhigh();		 \
		/* 						 \
		 * this is a hack.  We know that kdb_local_regptr is  \
		 * r11, and that it is not being used at this point  \
		 * Use movl instead of mfpr since sp is just r14  \
		 */ 						 \
 								 \
		asm("movl sp,r11"); 				 \
		saved_stack_ptr = (char *)REG_11; 		 \
		REG_11 = (int *)kdb_stack_ptr; 			 \
		asm("movl r11,sp"); 				 \
 								 \
		force_call(type); 				 \
		kdb_local_regptr = (int *)saved_stack_ptr; 	 \
		asm("movl r11,sp"); 				 \
		in_kdb--; 					 \
		if (in_kdb == 0)				\
			splx(kdb_saved_ipl);			\
		return; 					 \
	}							 \

