#ifndef lint
static char *sccsid = "@(#)pmvdac.c	4.2      (ULTRIX)  10/16/90";
#endif lint
/************************************************************************
 *									*
 *			Copyright (c) 1990 by				*
 *		Digital Equipment Corporation, Maynard, MA		*
 *			All rights reserved.				*
 *									*
 *   This software is furnished under a license and may be used and	*
 *   copied  only  in accordance with the terms of such license and	*
 *   with the  inclusion  of  the  above  copyright  notice.   This	*
 *   software  or  any  other copies thereof may not be provided or	*
 *   otherwise made available to any other person.  No title to and	*
 *   ownership of the software is hereby transferred.			*
 *									*
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or  reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/

/*
 * support for PMAX 
 * written by Joel Gringorten, from pm.c
 */

#include "../h/types.h"
#include "../h/workstation.h"
#include "../io/ws/pmvdac.h"
#include "../h/param.h"
#include "../h/buf.h"
#include "../io/uba/ubavar.h"

extern struct pmvdacinfo pmvdac_type[];

#define IS_MONO   (*((short *) PM_CSR_ADDR) & PM_CSR_MONO)

volatile u_short cur_reg = 0; /* Register to keep track of pcc-cmdr bits*/ 

/* XXX this doesn't belong in this file */

caddr_t pmax_init_closure(closure, address, unit, type)
	caddr_t closure;
	caddr_t address;
	int unit;
	int type;
{
	volatile struct pmvdacinfo *bp = (struct pmvdacinfo *)closure;
	bp = bp + unit;			/* set to correct unit */
	*bp = pmvdac_type[type];		/* set to initial values */
	bp->pmvdac_addr = VDAC_ADDR;
	bp->pccregs_addr = PCC_ADDR;
	return (caddr_t)bp;
}


/* ARGSUSED */
pmax_set_cursor_position(closure, sp, x, y)
	caddr_t closure;
	ws_screen_descriptor *sp;
	register int x, y;
{
	register struct pmvdacinfo *pmi = (struct pmvdacinfo *)closure;
   	register volatile struct pcc_regs *pccregs = pmi->pccregs_addr;
	register unsigned short xt =  x + pmi->fb_xoffset - pmi->x_hot;
	register unsigned short yt =  y + pmi->fb_yoffset - pmi->y_hot;
        pccregs->xpos = xt;
        pccregs->ypos = yt;
	return(0);
}


int pmax_load_cursor(closure, screen, cursor)
	caddr_t closure;
	ws_screen_descriptor *screen;
	ws_cursor_data *cursor;
{
	register struct pmvdacinfo *pmi = (struct pmvdacinfo *)closure;
	pmax_reformat_cursor(pmi, cursor);
	pmi->x_hot = cursor->x_hot;
	pmi->y_hot = cursor->y_hot;
	pmax_load_formatted_cursor(pmi);
	return(0);
}

static pmax_reformat_cursor(pmi, cursor)
	register ws_cursor_data *cursor;
	register struct pmvdacinfo *pmi;
{
	int i, lastRow = ((cursor->height < 16) ? cursor->height : 16);
	register unsigned short widthmask = (1<<cursor->width)-1;
	register unsigned short *a = pmi->bits;
	register unsigned short *b = pmi->bits + 16;
	register unsigned int *src  = (unsigned int *)cursor->cursor;
	register unsigned int *mask = (unsigned int *)cursor->mask;
	bzero((char *)a, 32*sizeof(short));
	for (i = 0; i < lastRow; i++, a++, b++, src++, mask++) {
	    *a = *src;
	    *b = *mask;
	    *a &= widthmask;
	    *b &= widthmask;
	}
}

pmax_load_formatted_cursor(pmi)
	register struct pmvdacinfo *pmi;
{
	register volatile struct pcc_regs *pccregs = pmi->pccregs_addr;		
	register unsigned short *cur = pmi->bits;
	register int i;
	cur_reg |= LODSA;
	pccregs->cmdr = cur_reg;
	for (i = 0; i < 32; i++ ){
	    pccregs->memory = *cur++;
	    wbflush();
	}
	cur_reg &= ~LODSA;
	pccregs->cmdr = cur_reg;
}


pmax_recolor_cursor (closure, screen, fg, bg)
	caddr_t closure;
	ws_screen_descriptor *screen;
	ws_color_cell *fg, *bg;
{
	register struct pmvdacinfo *pmi = (struct pmvdacinfo *)closure;
        register vdac_regs *vdac = pmi->pmvdac_addr;    
	pmi->cursor_fg = *fg;
	pmi->cursor_bg = *bg;
	pmax_restore_cursor_color(closure);
	return 0;
}


pmax_init_color_map(closure)
	caddr_t closure;
{
	register int i;
	register struct pmvdacinfo *pmi = (struct pmvdacinfo *)closure;
	register vdac_regs *vdac = pmi->pmvdac_addr;    
    
    pmax_init(closure);
    pmax_init_cursor(closure);
    
/*    *planemask = 0xff; */

    if (IS_MONO)
    {
        for (i = 0; i < 256; i++)
        {
    	    vdac->map_wr = i; wbflush();
    	    vdac->map_ram = (i < 128)? 0x00: 0xff; wbflush();
    	    vdac->map_ram = (i < 128)? 0x00: 0xff; wbflush();
    	    vdac->map_ram = (i < 128)? 0x00: 0xff; wbflush();
        }
    }
    else
    {
        vdac->map_wr = 0; wbflush();
        vdac->map_ram = 0; wbflush();
        vdac->map_ram = 0; wbflush();
        vdac->map_ram = 0; wbflush();
    
        for(i = 1; i <256; i++) 
        {
            vdac->map_wr = i; wbflush();
            vdac->map_ram = 0xff; wbflush();
            vdac->map_ram = 0xff; wbflush();
            vdac->map_ram = 0xff; wbflush();
        }
    }

    pmi->cursor_fg.red = pmi->cursor_fg.green = pmi->cursor_fg.blue = 0xffff;
    pmi->cursor_bg.red = pmi->cursor_bg.green = pmi->cursor_bg.blue = 0x0000;
    pmax_restore_cursor_color(closure);
}


pmax_restore_cursor_color(closure)
	caddr_t closure;
{
    register struct pmvdacinfo *pmi = (struct pmvdacinfo *)closure;
    register vdac_regs *vdac = pmi->pmvdac_addr;    
    vdac->over_wr = 0x04;
    wbflush();
    vdac->over_regs = (u_char)(pmi->cursor_bg.red >> 8);   
    wbflush();
    vdac->over_regs = (u_char)(pmi->cursor_bg.green >> 8);   
    wbflush();
    vdac->over_regs = (u_char)(pmi->cursor_bg.blue >> 8);   
    wbflush();
    vdac->over_wr = 0x08;
    wbflush();
    vdac->over_regs = 0x00;
    wbflush();
    vdac->over_regs = 0x00;
    wbflush();
    vdac->over_regs = 0x7f;
    wbflush();
    vdac->over_wr = 0x0c;
    wbflush();
    vdac->over_regs = (u_char)(pmi->cursor_fg.red >> 8);   
    wbflush();
    vdac->over_regs = (u_char)(pmi->cursor_fg.green >> 8);   
    wbflush();
    vdac->over_regs = (u_char)(pmi->cursor_fg.blue >> 8);   
    wbflush();
}

vdacInit(vdac)
    register vdac_regs *vdac;
{
    /*
     * Initialize the VDAC
     */
    vdac->over_wr = 0x04;
    wbflush();
    vdac->over_regs = 0x00;
    wbflush();
    vdac->over_regs = 0x00;
    wbflush();
    vdac->over_regs = 0x00;
    wbflush();
    vdac->over_wr = 0x08;
    wbflush();
    vdac->over_regs = 0x00;
    wbflush();
    vdac->over_regs = 0x00;
    wbflush();
    vdac->over_regs = 0x7f;
    wbflush();
    vdac->over_wr = 0x0c;
    wbflush();
    vdac->over_regs = 0xff;
    wbflush();
    vdac->over_regs = 0xff;
    wbflush();
    vdac->over_regs = 0xff;
    wbflush();
    vdac->mask = 0xff; 
    wbflush();
}

pmax_init_cursor(closure)
	caddr_t closure;
{

	register struct pmvdacinfo *pmi = (struct pmvdacinfo *)closure;
	register volatile struct pcc_regs *pccregs = pmi->pccregs_addr;		
	int i;
	cur_reg |= LODSA;
	pccregs->cmdr = cur_reg;
	for (i = 0; i < 32; i++ ){
	    pccregs->memory = 0;
	    wbflush();
	}
	cur_reg &= ~LODSA;
	pccregs->cmdr = cur_reg;
}


pmax_init(closure)
	caddr_t closure;
{
	register struct pmvdacinfo *pmi = (struct pmvdacinfo *)closure;
	register volatile struct pcc_regs *pccregs = pmi->pccregs_addr;		
        register vdac_regs *vdac = pmi->pmvdac_addr;    
	int	i;
	i = FOPB | VBHI;
	pccregs->cmdr = i;
	cur_reg |= ( ENPA | ENPB);
	pccregs->cmdr = cur_reg;
	vdacInit(vdac);
}

/* returns 0 if succeeded, -1 if it couldn't (index too big) */
/* ARGSUSED */
int pmax_load_color_map_entry(closure, map, entry)
	caddr_t closure;
	int map;		/* not used; only single map in this device */
	register ws_color_cell *entry;
{
	register struct pmvdacinfo *pmi = (struct pmvdacinfo *)closure;
	register vdac_regs *vdac = pmi->pmvdac_addr;    
	if(entry->index > 256) 
		return -1;
	vdac->map_wr = entry->index; wbflush();
	vdac->map_ram = entry->red >> 8; wbflush();
	vdac->map_ram = entry->green >> 8; wbflush();
	vdac->map_ram = entry->blue >> 8; wbflush();
	return 0;
}

pmax_video_off(closure)
	caddr_t closure;
{
	register struct pmvdacinfo *pmi = (struct pmvdacinfo *)closure;
	register volatile struct pcc_regs *pccregs = pmi->pccregs_addr;		
	register  vdac_regs *vdac = pmi->pmvdac_addr;    

	if (!IS_MONO)
	    vdacInit(vdac);
	cur_reg |= FOPB;
	cur_reg |= FOPA; 
	cur_reg &= ~(ENPA);
	cur_reg &= ~(ENPB); 
	pccregs->cmdr = cur_reg; wbflush();
	vdac->over_wr = 12; wbflush();
	vdac->over_regs = 0; wbflush();
	vdac->over_regs = 0; wbflush();
	vdac->over_regs = 0; wbflush();
	return(0);
}

pmax_video_on(closure)
	caddr_t closure;
{
	register struct pmvdacinfo *pmi = (struct pmvdacinfo *)closure;
	register volatile struct pcc_regs *pccregs = pmi->pccregs_addr;		
	register  vdac_regs *vdac = pmi->pmvdac_addr;    
        pmax_restore_cursor_color(closure);
	cur_reg |= (ENPA);
	cur_reg |= (ENPB);
	cur_reg &= ~(FOPA); 
	cur_reg &= ~(FOPB);
	pccregs->cmdr = cur_reg; wbflush();
}

pmax_cursor_on_off(closure, onoff)
	caddr_t closure;
{
    return 0;
}

