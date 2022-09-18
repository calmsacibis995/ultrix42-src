/*
 * @(#)lk201_data.c	4.1	(ULTRIX)	8/13/90
 */
/************************************************************************
 *									*
 *			Copyright (c) 1990 by				*
 *		Digital Equipment Corporation, Maynard, MA		*
 *			All rights reserved.				*
 *									*
 *   This software is furnished under a license and may be used and	*
 *   copied  only  in accordance with the terms of such license and	*
 *   with the  inclusion  of  the  above  copyright  notice.   This	*
 *   software  or  any	other copies thereof may not be provided or	*
 *   otherwise made available to any other person.  No title to and	*
 *   ownership of the software is hereby transferred.			*
 *									*
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or	reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/

#include "../h/types.h"
#include "../h/workstation.h"
#include "../h/inputdevice.h"
#include "../h/wsdevice.h"
#include "../h/lk201.h"


#define KEYSYMDEF(code, keysym) (((code) << 24) | (keysym))
/*
 * Note that if a keycode appears additional times, it defines further
 * symbols on the the same keycode.  DDX translates this to the appropriate
 * data structure.  All this is to save bytes in the kernel.
 * WARNING: keycodes and keysym tables must be EXACTLY in sync!
 */
unsigned char lk201_keycodes[] = {
    KEY_F1,
    KEY_F2,
    KEY_F3,
    KEY_F4,
    KEY_F5,
    KEY_F6,
    KEY_F7,
    KEY_F8,
    KEY_F9,
    KEY_F10,
    KEY_ESC,		/* escape is primary, due to previous stupidity...*/
    KEY_F11,
    KEY_F12,
    KEY_F13,
    KEY_F14,

    KEY_HELP,
    KEY_MENU,

    KEY_F17,
    KEY_F18,
    KEY_F19,
    KEY_F20,

    KEY_FIND,
    KEY_INSERT_HERE,
    KEY_REMOVE,
    KEY_SELECT,
    KEY_PREV_SCREEN,
    KEY_NEXT_SCREEN,

    KEY_KP_0,
    KEY_KP_PERIOD,
    KEY_KP_ENTER,
    KEY_KP_1,
    KEY_KP_2,
    KEY_KP_3,
    KEY_KP_4,
    KEY_KP_5,
    KEY_KP_6,
    KEY_KP_COMMA,
    KEY_KP_7,
    KEY_KP_8,
    KEY_KP_9,
    KEY_KP_HYPHEN,
    KEY_KP_PF1,
    KEY_KP_PF2,
    KEY_KP_PF3,
    KEY_KP_PF4,

    KEY_LEFT,
    KEY_RIGHT,
    KEY_DOWN,
    KEY_UP,

    KEY_SHIFT,
    KEY_SHIFT_R,
    KEY_CTRL,
    KEY_LOCK,
    KEY_COMPOSE,
    KEY_COMPOSE,
    KEY_COMPOSE_R,
    KEY_COMPOSE_R,
    KEY_ALT_L,
    KEY_ALT_R,
    KEY_DELETE,
    KEY_RETURN,
    KEY_TAB,

    KEY_TILDE,
    KEY_TILDE,

    KEY_TR_1,
    KEY_TR_1,
    KEY_Q,
    KEY_A,
    KEY_Z,

    KEY_TR_2,
    KEY_TR_2,

    KEY_W,
    KEY_S,
    KEY_X,

    KEY_LANGLE_RANGLE,
    KEY_LANGLE_RANGLE,

    KEY_TR_3,
    KEY_TR_3,

    KEY_E,
    KEY_D,
    KEY_C,

    KEY_TR_4,
    KEY_TR_4,

    KEY_R,
    KEY_F,
    KEY_V,
    KEY_SPACE,

    KEY_TR_5,
    KEY_TR_5,

    KEY_T,
    KEY_G,
    KEY_B,

    KEY_TR_6,
    KEY_TR_6,

    KEY_Y,
    KEY_H,
    KEY_N,

    KEY_TR_7,
    KEY_TR_7,

    KEY_U,
    KEY_J,
    KEY_M,

    KEY_TR_8,
    KEY_TR_8,

    KEY_I,
    KEY_K,

    KEY_COMMA,
    KEY_COMMA,

    KEY_TR_9,
    KEY_TR_9,

    KEY_O,
    KEY_L,

    KEY_PERIOD,
    KEY_PERIOD,

    KEY_TR_0,
    KEY_TR_0,

    KEY_P,

    KEY_SEMICOLON,
    KEY_SEMICOLON,

    KEY_QMARK,
    KEY_QMARK,

    KEY_PLUS,
    KEY_PLUS,

    KEY_RBRACE,
    KEY_RBRACE,

    KEY_VBAR,
    KEY_VBAR,

    KEY_UBAR,
    KEY_UBAR,

    KEY_LBRACE,
    KEY_LBRACE,

    KEY_QUOTE,
    KEY_QUOTE,

};

unsigned int lk201_keysyms[] = {
    XK_F1,
    XK_F2,
    XK_F3,
    XK_F4,
    XK_F5,
    XK_F6,
    XK_F7,
    XK_F8,
    XK_F9,
    XK_F10,
    XK_Escape,
    XK_F11,
    XK_F12,
    XK_F13,
    XK_F14,

    XK_Help,
    XK_Menu,

    XK_F17,
    XK_F18,
    XK_F19,
    XK_F20,

    XK_Find,
    XK_Insert,
    DXK_Remove,
    XK_Select,
    XK_Prior,
    XK_Next,

    XK_KP_0,
    XK_KP_Decimal,
    XK_KP_Enter,
    XK_KP_1,
    XK_KP_2,
    XK_KP_3,
    XK_KP_4,
    XK_KP_5,
    XK_KP_6,
    XK_KP_Separator,
    XK_KP_7,
    XK_KP_8,
    XK_KP_9,
    XK_KP_Subtract,
    XK_KP_F1,
    XK_KP_F2,
    XK_KP_F3,
    XK_KP_F4,

    XK_Left,
    XK_Right,
    XK_Down,
    XK_Up,

    XK_Shift_L,
    XK_Shift_R,
    XK_Control_L,
    XK_Caps_Lock,
    XK_Multi_key,
    XK_Meta_L,
    XK_Multi_key,
    XK_Meta_R,
    XK_Alt_L,
    XK_Alt_R,
    XK_Delete,
    XK_Return,
    XK_Tab,

    XK_quoteleft,
    XK_asciitilde,

    XK_1,
    XK_exclam,
    XK_Q,
    XK_A,
    XK_Z,

    XK_2,
    XK_at,

    XK_W,
    XK_S,
    XK_X,

    XK_less,
    XK_greater,

    XK_3,
    XK_numbersign,

    XK_E,
    XK_D,
    XK_C,

    XK_4,
    XK_dollar,

    XK_R,
    XK_F,
    XK_V,
    XK_space,

    XK_5,
    XK_percent,

    XK_T,
    XK_G,
    XK_B,

    XK_6,
    XK_asciicircum,

    XK_Y,
    XK_H,
    XK_N,

    XK_7,
    XK_ampersand,

    XK_U,
    XK_J,
    XK_M,

    XK_8,
    XK_asterisk,

    XK_I,
    XK_K,

    XK_comma,
    XK_less,

    XK_9,
    XK_parenleft,

    XK_O,
    XK_L,

    XK_period,
    XK_greater,

    XK_0,
    XK_parenright,

    XK_P,

    XK_semicolon,
    XK_colon,

    XK_slash,
    XK_question,

    XK_equal,
    XK_plus,

    XK_bracketright,
    XK_braceright,

    XK_backslash,
    XK_bar,

    XK_minus,
    XK_underscore,

    XK_bracketleft,
    XK_braceleft,

    XK_quoteright,
    XK_quotedbl,

};

/* this table defines which keycodes have modifier bits associated with them */

/* Key masks. Used as modifiers to GrabButton and GrabKey, results of QueryPointer,
   state in various key-, mouse-, and button-related events. */

#ifndef ShiftMask
#define ShiftMask		(1<<0)
#define LockMask		(1<<1)
#define ControlMask		(1<<2)
#define Mod1Mask		(1<<3)
#define Mod2Mask		(1<<4)
#define Mod3Mask		(1<<5)
#define Mod4Mask		(1<<6)
#define Mod5Mask		(1<<7)
#endif

ws_keycode_modifiers lk201_modifiers[] = {
    { KEY_LOCK, LockMask},
    { KEY_SHIFT, ShiftMask},
    { KEY_SHIFT_R, ShiftMask},
    { KEY_CTRL, ControlMask},
    { KEY_COMPOSE, Mod1Mask},
    { KEY_COMPOSE_R, Mod1Mask},
    { KEY_ALT_L, Mod2Mask},
    { KEY_ALT_R, Mod2Mask},
};

ws_keyboard_definition lk201_definition = {
	0,
	LK201_GLYPHS_PER_KEY,		/* beware of this constant!!! */
	sizeof (lk201_keysyms) / sizeof (unsigned int),
	sizeof (lk201_modifiers) / sizeof (ws_keycode_modifiers),
	3
};
