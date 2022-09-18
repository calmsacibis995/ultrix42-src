#ifndef lint
static char *sccsid = "@(#)fb.c	4.3      (ULTRIX)  10/16/90";
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
 * Modification History
 *
 * November, 1989	Jim Gettys
 *
 *		Complete rewrite for multiscreen support.  Boy how kludges
 *		accumulate over time....
 *
 */

#include "../data/fb_data.c"
#include "../io/tc/tc.h"

int	fbprobe(), fbattach(), fbkint(), fbint();

u_short	fbstd[] = { 0 };

struct	uba_driver fbdriver = 
        { fbprobe, 0, fbattach, 0, fbstd, "fb", fbinfo };

extern int cpu;			/* have to do a minor amount of stuff */
extern  int     ws_display_type;
extern  int     ws_display_units;

/*
 * Probe to see if the graphic device will interrupt.
 */
fbprobe(nxv, ui)
char *nxv;
register struct uba_device *ui;
{
	register int i;
        /* 
	 * the initialization of the first screen is done through fb_cons_init,
	 * so if we have gotten this far we are alive so return a 1
	 */
#ifdef notdef
	for (i = 0; i < nfb_types; i++) {
		if ( strncmp(fb_type[i].type, nxv->????) == 0) return (1);
		return 1;
	}
	return(0);
#endif
	return(1);
}

fbtype(address)
	caddr_t address;
{
#define IS_MONO   (*((short *) PM_CSR_ADDR) & PM_CSR_MONO)
	register int i;
	char *module = "";
	if(cpu == DS_3100)
	    if(IS_MONO)
		module = "PMAX-MFB";  
	    else
	        module = "PMAX-CFB";
	else {
	    for (i = 0; i < TC_OPTION_SLOTS; i++) {
		if((caddr_t)PHYS_TO_K1(tc_slot[i].physaddr) == address) {
		    module =  tc_slot[i].modulename;
		    break;
		}
	    }
	}
	if(*module == NULL)
	    cprintf("fb - couldn't identify tc module\n");
	for (i = 0; i < nfb_types; i++)
	    if (strncmp(fb_type[i].screen.moduleID, module, TC_ROMNAMLEN) == 0) 
		    return i;
	return -1;
#undef IS_MONO
}

/*
 * Routine to attach to the graphic device.
 */

fbattach(ui)
	register struct uba_device *ui;
{
        register struct fb_info *fbp = &fb_softc[ui->ui_unit];
	int ret;
	ret = fb_attach(ui->ui_addr, ui->ui_unit, ui->ui_flags);
	if (fbp->attach) return (*fbp->attach)(ui);
	return ret;
}

/*
 * the routine that does the real work.  This is so the console can get
 * initialized before normal attach goes on.
 */

fb_attach(address, unit, flags)
	register caddr_t address;
	int unit;
	int flags;
{
	static caddr_t console_address = NULL;
        register struct fb_info *fbp = &fb_softc[unit];
	register int dev_type, m_type = flags;
	register int i;

	if ((dev_type = fbtype(address)) == -1) return 0;
	/* deal with screen first, but only init screen structure once! */
	if(console_address != address) fbp->screen = fb_type[dev_type].screen;
	fbp->screen.screen = unit;

	/* default to monitor type in screen structure unless flags set */
	if (flags != 0) {
		if ((m_type < 0) || (m_type >= nmon_types)) m_type = 0;
		fbp->screen.monitor_type = monitor_type[m_type];
	}

	/*
	 * if we are the console, then we've already done general 
	 * initialization, so we shouldn't attempt to define another screen
	 * given that it's already been defined.  Note that we've dealt
	 * with screen type above.
	 */
	if (console_address == address) return 0;

	/* then with the depths and visual types */
	for (i = 0; i < fbp->screen.allowed_depths; i++) {
		fbp->depth[i]  = fb_type[dev_type].depth[i];
		fbp->depth[i].which_depth = i;
	}

	for (i = 0; i < fbp->screen.nvisuals; i++) {
		fbp->visual[i] = fb_type[dev_type].visual[i];
		fbp->visual[i].which_visual = i;
	}
	/* get the screen, colormap, cursor and interrupt functions over */
	fbp->sf   		= fb_type[dev_type].sf;
	fbp->cmf  		= fb_type[dev_type].cmf;
	fbp->cf   		= fb_type[dev_type].cf;
	fbp->attach		= fb_type[dev_type].attach;
	fbp->interrupt 		= fb_type[dev_type].interrupt;

	fbp->sf.sc = (*(fbp->sf.init_closure))
		(fbp->sf.sc, address, unit, fb_type[dev_type].screen_type);
	fbp->cf.cc = (*(fbp->cf.init_closure))
		(fbp->cf.cc, address, unit, fb_type[dev_type].cursor_type);
	fbp->cmf.cmc = (*(fbp->cmf.init_closure))
		(fbp->cmf.cmc, address, unit,
			 fb_type[dev_type].color_map_type);
	i = ws_define_screen(&fbp->screen, fbp->visual, fbp->depth, 
			 &fbp->sf, &fbp->cmf, &fbp->cf);
	if (console_address == NULL) console_address = address;
	if (i == -1) {
	  printf("fb_driver: could not define screen\n");
	  return(0);
	}
	ws_display_type = WS_DTYPE;
        ws_display_units = 1;
	return 1;
}


caddr_t fb_init_closure (closure, address, unit, type)
	caddr_t closure;
	caddr_t address;
	int unit;
{
	struct fb_info  *fbp = (struct fb_info *) closure;
	register int i;
	fbp = fbp + unit;
	for (i = 0; i < fbp->screen.allowed_depths; i++) {
		fbp->depth[i].physaddr = 
			address + (int)fbp->depth[i].physaddr;
		if (fbp->depth[i].plane_mask_phys)
			fbp->depth[i].plane_mask_phys = 
				address + (int)fbp->depth[i].plane_mask_phys;
	}
	return (caddr_t) &fbp->screen;
}

/*
 * Graphic device ioctl routine.
 */
/*ARGSUSED*/
fbioctl(dev, cmd, data, flag)
	dev_t dev;
	register caddr_t data;
{
	register struct tty *tp;
	register int unit = minor(dev);
	register unsigned char *cp;
	return (0);
}

/*
 * Graphics device end-of-frame interrupt service routine.
 * Cursor and/or colormap loading at end of frame interrupt gets done
 * by hardware specific interrupt routine.  For example, the 3MAX
 * color frame buffer provides a routine that implements this.
 */
fbint(unit)
	int unit;
{
	register struct fb_info *fp = &fb_softc[unit];
	register struct uba_device *ui = fbinfo[unit];
	if (fp->interrupt) {
	    (*fp->interrupt)(ui, fp);
	    return;
	}
}


/*
 * scroll screen one line; should work on both mono and color screens
 */

fb_scroll_screen(closure, screen)
	caddr_t closure;
	ws_screen_descriptor *screen;
{
	register struct fb_info *fbp = (struct fb_info *)closure;
	register ws_screen_descriptor *sp = &fbp->screen;
 	register ws_depth_descriptor *dp = &fbp->depth[sp->root_depth];
	register int *dest, *src, *spp, *dpp;
	register int temp0,temp1,temp2,temp3;
	register int i, j, wpl, wpfbl;
	int n;
	/* compute # bits,  unroll to 4 words at a time */

	wpl = (sp->f_width * sp->max_col * dp->bits_per_pixel) / (8 * 16);
	wpfbl = (dp->fb_width * dp->bits_per_pixel) / 32;
	n = sp->max_row * sp->f_height;
	spp = src   =  (int *)(dp->physaddr + 
	    (dp->fb_width * sp->f_height * dp->bits_per_pixel / 8));
	dpp = dest = (int *)(dp->physaddr);
	for (j = 0; j < n; j++) {
		src = spp;
		dest = dpp;
		i = 0;
		do {
			temp0 = src[0]; temp1 = src[1]; 
			temp2 = src[2]; temp3 = src[3];
			dest[0] = temp0; dest[1] = temp1;
			dest[2] = temp2; dest[3] = temp3;
			dest += 4; src += 4;
			i += 1;
		} while(i < wpl);
		spp += wpfbl;
		dpp += wpfbl;
	};
	/* Now zero out the last line */
	n = sp->f_width * sp->max_col * dp->bits_per_pixel / 8;
	spp = (int *)(dp->physaddr +
	    (dp->fb_width * sp->f_height * sp->max_row * dp->bits_per_pixel / 8));
	for (j = 0; j < sp->f_height; j++) {
		bzero (spp, n);
		spp += wpfbl;
	}
}

/*
 * Clear the bitmap.  Should work for most screens.
 */
fb_clear_screen(closure, screen)
	caddr_t closure;
	ws_screen_descriptor *screen;
{
	register struct fb_info *fbp = (struct fb_info *)closure;
	register ws_screen_descriptor *sp = &fbp->screen;
 	register ws_depth_descriptor *dp = &fbp->depth[sp->root_depth];
	register char *lp;
	register int i, stride, nbytes;
	lp = dp->physaddr;
	stride = dp->fb_width * dp->bits_per_pixel / 8;
	nbytes = sp->width * dp->bits_per_pixel / 8;

	for (i = 0; i < sp->height; i++, lp += stride)
		bzero (lp, nbytes);
}
static unsigned int fontmask_bits[16] = {
	0x00000000,
	0x00000001,
	0x00000100,
	0x00000101,
	0x00010000,
	0x00010001,
	0x00010100,
	0x00010101,
	0x01000000,
	0x01000001,
	0x01000100,
	0x01000101,
	0x01010000,
	0x01010001,
	0x01010100,
	0x01010101
};
/*
 * Keyboard translation and font tables
 */
extern  char *q_special[],q_font[];
extern  u_short q_key[],q_shift_key[];
/*
 * put a character on the screen.  This routine is both depth and 
 * font dependent.
 */
fb_blitc(closure, screen, row, col, c) 
	caddr_t closure;
	ws_screen_descriptor *screen;
	int row, col;
	u_char c;
{
	register struct fb_info *fbp = (struct fb_info *)closure;
	register ws_screen_descriptor *sp = &fbp->screen;
 	register ws_depth_descriptor *dp = &fbp->depth[sp->root_depth];
	unsigned int *pInt;
	register int i, j;
	register char *b_row, *f_row;
	register int ote;
	/*
	 * xA1 to XFD are the printable characters added with 8-bit
	 * support.
	 */
	if(( c >= ' ' && c <= '~' ) || ( c >= 0xA1 && c <= 0xFD))   {
		b_row = dp->physaddr + (row * dp->fb_width * sp->f_height)
			    + (col * sp->f_width);
		i = c - ' ';
		if( i < 0 || i > 221 ) i = 0;
		else 	{
			/* These are to skip the (32) 8-bit  control chars, 
			 * as well as DEL and 0xA0 which aren't printable 
			 */
			if (c > '~') i -= 34; 
		    	i *= 15;
		}
		f_row = (char *)((int)q_font + i);
		if (dp->bits_per_pixel == 8) {
			ote = dp->fb_width * dp->bits_per_pixel / 32;
			pInt = (unsigned int *) b_row;
			/*
			 * fontmask_bits converts a nibble (4 bytes) to a long
			 * word containing 4 pixels corresponding to each bit
			 * in the nibble.
			 * Thus we write two longwords for each byte in font.
			 * 
			 * Remember the font is 8 bits wide and 15 bits high.
			 *
			 * We add 256 to the pointer to point to the pixel on
			 * the next scan line directly below the current pixel.
			 */
			for( j = 0; j < 15; j++) {
				*pInt =  fontmask_bits[(*f_row)&0xf];
				*(pInt+1)= fontmask_bits[((*f_row)>>4)&0xf];
				f_row++; 
				pInt += ote;
			}
		}
		else if(dp->bits_per_pixel == 1) {
			ote = dp->fb_width * dp->bits_per_pixel / 8;
			b_row = dp->physaddr + (row * sp->f_height & 0x3ff)
						* ote + col;
			*b_row = *f_row++; b_row += ote;
			*b_row = *f_row++; b_row += ote;
			*b_row = *f_row++; b_row += ote;
			*b_row = *f_row++; b_row += ote;
			*b_row = *f_row++; b_row += ote;
			*b_row = *f_row++; b_row += ote;
			*b_row = *f_row++; b_row += ote;
			*b_row = *f_row++; b_row += ote;
			*b_row = *f_row++; b_row += ote;
			*b_row = *f_row++; b_row += ote;
			*b_row = *f_row++; b_row += ote;
			*b_row = *f_row++; b_row += ote;
			*b_row = *f_row++; b_row += ote;
			*b_row = *f_row++; b_row += ote;
			*b_row = *f_row++; b_row += ote;
		}
	}
	return(0);
}

fb_map_unmap_screen(closure, depths, screen, mp)
	caddr_t closure;
	ws_depth_descriptor *depths;
	ws_screen_descriptor *screen;
	ws_map_control *mp;
{
	register struct fb_info *fbp = (struct fb_info *)closure;
	int nbytes;
	register ws_depth_descriptor *dp = depths + mp->which_depth;
	/* unmap not yet (if ever) implemented) */
	if (mp->map_unmap == UNMAP_SCREEN) return (EINVAL);
	nbytes = ((dp->fb_width * dp->fb_height) * dp->bits_per_pixel) >> 3;
	if ((dp->pixmap = ws_map_region(dp->physaddr, nbytes, 0600)) == NULL)
		return(ENOMEM);
	if(dp->plane_mask_phys) 
	    if ((dp->plane_mask = ws_map_region(dp->plane_mask_phys, 4, 0600))
			 == NULL)  return(ENOMEM);
/* XXX is this returning right error code???? */
	return (0);

}	

fb_cons_init()
{
	register caddr_t address;
	int ret;

	if(cpu == DS_3100) 
	    address = (caddr_t)BITMAP_ADDR ;
	else {
	    address = (caddr_t)tc_where_option("fb");
	    /* address == 0 if there is no cfb board in system */
	    if (address == 0) return 0;	
	    address = (caddr_t)PHYS_TO_K1(address);
	}
	fb_attach(address, 0, 0);
	ret = ws_cons_init();
    
/* rpbfix: scroll so that console rom output is not munged */
/*	   use printf until rom get fixed		   */
/*	cfbscroll();					   */
	cprintf("\n");
	return (ret);
}

fb_init_screen() {}
