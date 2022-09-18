/*
 * @(#)lk201.h	4.2	(ULTRIX)	10/16/90
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

/* 
 * command keycodes for Digital LK200/LK400 series keyboards.
 */

#define LK_POWER_UP	0x01		/* initial byte of power up code */
#define LK_UPDOWN	0x86		/* bits for setting lk201 modes */
#define LK_AUTODOWN	0x82
#define LK_DOWN		0x80
#define LK_DEFAULTS	0xd3		/* reset (some) default settings*/
#define LK_AR_ENABLE	0xe3		/* global auto repeat enable	*/
#define LK_CL_ENABLE	0x1b		/* keyclick enable		*/
#define LK_CL_DISABLE	0x99		/* keyclick disable		*/
#define LK_CL_SOUND	0x9f		/* sound keyclick		*/
#define LK_KBD_ENABLE	0x8b		/* keyboard enable		*/
#define LK_BELL_ENABLE	0x23		/* the bell			*/
#define LK_BELL_DISABLE 0xa1		/* disable bell entirely	*/
#define LK_LED_ENABLE	0x13		/* light led			*/
#define LK_LED_DISABLE	0x11		/* turn off led			*/
#define LK_RING_BELL	0xa7		/* ring keyboard bell		*/
#define LED_1		0x81		/* led bits			*/
#define LED_2		0x82
#define LED_3		0x84
#define LED_4		0x88
#define LED_ALL		0x8f
#define LK_KDOWN_ERROR	0x3d		/* key down on powerup error	*/
#define LK_POWER_ERROR	0x3e		/* keyboard failure on pwrup tst*/
#define LK_OUTPUT_ERROR 0xb5		/* keystrokes lost during inhbt */
#define LK_INPUT_ERROR	0xb6		/* garbage command to keyboard	*/
#define LK_LOWEST	0x56		/* lowest significant keycode	*/
#define LK_DIV6_START	0xad		/* start of div 6		*/
#define LK_DIV5_END	0xb2		/* end of div 5			*/
#define LK_ENABLE_401	0xe9		/* turn on LK401 mode		*/
#define LK_MODE_CHANGE	0xba		/* mode change ack		*/


#define MIN_LK201_KEY            86
#define MAX_LK201_KEY           251
#define LK201_GLYPHS_PER_KEY      2

/* the keys themselves */

#define KEY_F1			 86
#define KEY_F2			 87
#define KEY_F3			 88
#define KEY_F4			 89
#define KEY_F5			 90
#define KEY_F6			100
#define KEY_F7			101
#define KEY_F8			102
#define KEY_F9			103
#define KEY_F10			104
#define KEY_F11			113
#define KEY_F12			114
#define KEY_F13			115
#define KEY_F14			116
#define KEY_HELP		124
#define KEY_MENU		125
#define KEY_F17			128
#define KEY_F18			129
#define KEY_F19			130
#define KEY_F20			131
#define KEY_FIND		138
#define KEY_INSERT_HERE		139
#define KEY_REMOVE		140
#define KEY_SELECT		141
#define KEY_PREV_SCREEN		142
#define KEY_NEXT_SCREEN		143
#define KEY_KP_0		146	/* key pad */
#define KEY_KP_PERIOD		148	/* key pad */
#define KEY_KP_ENTER		149	/* key pad */
#define KEY_KP_1		150	/* key pad */
#define KEY_KP_2		151	/* key pad */
#define KEY_KP_3		152	/* key pad */
#define KEY_KP_4		153	/* key pad */
#define KEY_KP_5		154	/* key pad */
#define KEY_KP_6		155	/* key pad */
#define KEY_KP_COMMA		156	/* key pad */
#define KEY_KP_7		157	/* key pad */
#define KEY_KP_8		158	/* key pad */
#define KEY_KP_9		159	/* key pad */
#define KEY_KP_HYPHEN		160
#define KEY_KP_PF1		161
#define KEY_KP_PF2		162
#define KEY_KP_PF3		163
#define KEY_KP_PF4		164
#define KEY_LEFT		167
#define KEY_RIGHT		168
#define KEY_DOWN		169
#define KEY_UP			170
#define KEY_SHIFT_R		171	/* LK401 */
#define KEY_ALT_L		172	/* LK401 */
#define KEY_COMPOSE_R		173	/* LK401 */
#define KEY_SHIFT		174
#define KEY_SHIFT_L		174
#define KEY_CTRL		175
#define KEY_LOCK		176
#define KEY_COMPOSE		177
#define KEY_APPLE		177
#define KEY_META		177
#define KEY_ALT_R		178	/* LK401 */
#define KEY_DELETE		188
#define KEY_RETURN		189
#define KEY_TAB			190
#define KEY_TILDE		191
#define KEY_TR_1		192	/* Top Row */
#define KEY_Q			193
#define KEY_A			194
#define KEY_Z			195
#define KEY_TR_2		197
#define KEY_W			198
#define KEY_S			199
#define KEY_X			200
#define KEY_LANGLE_RANGLE	201	/* xxx */
#define KEY_TR_3		203
#define KEY_E			204
#define KEY_D			205
#define KEY_C			206
#define KEY_TR_4		208
#define KEY_R			209
#define KEY_F			210
#define KEY_V			211
#define KEY_SPACE		212
#define KEY_TR_5		214
#define KEY_T			215
#define KEY_G			216
#define KEY_B			217
#define KEY_TR_6		219
#define KEY_Y			220
#define KEY_H			221
#define KEY_N			222
#define KEY_TR_7		224
#define KEY_U			225
#define KEY_J			226
#define KEY_M			227
#define KEY_TR_8		229
#define KEY_I			230
#define KEY_K			231
#define KEY_COMMA		232	/* xxx */
#define KEY_TR_9		234
#define KEY_O			235
#define KEY_L			236
#define KEY_PERIOD		237	/* xxx */
#define KEY_TR_0		239
#define KEY_P			240
#define KEY_SEMICOLON		242	/* xxx */
#define KEY_QMARK		243
#define KEY_PLUS		245	/* xxx */
#define KEY_RBRACE		246
#define KEY_VBAR		247	/* xxx */
#define KEY_UBAR		249	/* xxx */
#define KEY_LBRACE		250
#define KEY_QUOTE		251

/*
 * keysyms defined on default keyboard
 * This really should just be an include of /usr/include/X11/keysym.h
 * But this isn't in the base system (at least at the moment), so
 * we have to kludge around it.
 */

#define XK_F1			0xFFBE
#define XK_F2			0xFFBF
#define XK_F3			0xFFC0
#define XK_F4			0xFFC1
#define XK_F5			0xFFC2
#define XK_F6			0xFFC3
#define XK_F7			0xFFC4
#define XK_F8			0xFFC5
#define XK_F9			0xFFC6
#define XK_F10			0xFFC7
#define XK_F11			0xFFC8
#ifndef KEY_ESC
#define KEY_ESC KEY_F11
#endif
#define XK_L1			0xFFC8
#define XK_F12			0xFFC9
#define XK_L2			0xFFC9
#define XK_F13			0xFFCA
#define XK_L3			0xFFCA
#define XK_F14			0xFFCB
#define XK_L4			0xFFCB
#define XK_F15			0xFFCC
#define XK_L5			0xFFCC
#define XK_F16			0xFFCD
#define XK_L6			0xFFCD
#define XK_F17			0xFFCE
#define XK_L7			0xFFCE
#define XK_F18			0xFFCF
#define XK_L8			0xFFCF
#define XK_F19			0xFFD0
#define XK_L9			0xFFD0
#define XK_F20			0xFFD1
#define XK_L10			0xFFD1
#define XK_F21			0xFFD2
#define XK_R1			0xFFD2
#define XK_F22			0xFFD3
#define XK_R2			0xFFD3
#define XK_F23			0xFFD4
#define XK_R3			0xFFD4
#define XK_F24			0xFFD5
#define XK_R4			0xFFD5
#define XK_F25			0xFFD6
#define XK_R5			0xFFD6
#define XK_F26			0xFFD7
#define XK_R6			0xFFD7
#define XK_F27			0xFFD8
#define XK_R7			0xFFD8
#define XK_F28			0xFFD9
#define XK_R8			0xFFD9
#define XK_F29			0xFFDA
#define XK_R9			0xFFDA
#define XK_F30			0xFFDB
#define XK_R10			0xFFDB
#define XK_F31			0xFFDC
#define XK_R11			0xFFDC
#define XK_F32			0xFFDD
#define XK_R12			0xFFDD
#define XK_R13			0xFFDE
#define XK_F33			0xFFDE
#define XK_F34			0xFFDF
#define XK_R14			0xFFDF
#define XK_F35			0xFFE0
#define XK_R15			0xFFE0

#define XK_Help			0xFF6A	/* Help, ? */
#define XK_Menu			0xFF67
#define XK_Find			0xFF68	/* Find, search */
#define XK_Insert		0xFF63	/* Insert, insert here */
#define DXK_Remove		0x1000FF00
#define XK_Select		0xFF60	/* Select, mark */
#define XK_Prior		0xFF55	/* Prior, previous */
#define XK_Next			0xFF56	/* Next */


/* Keypad Functions, keypad numbers cleverly chosen to map to ascii */

#define XK_KP_Space		0xFF80	/* space */
#define XK_KP_Tab		0xFF89
#define XK_KP_Enter		0xFF8D	/* enter */
#define XK_KP_F1		0xFF91	/* PF1, KP_A, ... */
#define XK_KP_F2		0xFF92
#define XK_KP_F3		0xFF93
#define XK_KP_F4		0xFF94
#define XK_KP_Equal		0xFFBD	/* equals */
#define XK_KP_Multiply		0xFFAA
#define XK_KP_Add		0xFFAB
#define XK_KP_Separator		0xFFAC	/* separator, often comma */
#define XK_KP_Subtract		0xFFAD
#define XK_KP_Decimal		0xFFAE
#define XK_KP_Divide		0xFFAF

#define XK_KP_0			0xFFB0
#define XK_KP_1			0xFFB1
#define XK_KP_2			0xFFB2
#define XK_KP_3			0xFFB3
#define XK_KP_4			0xFFB4
#define XK_KP_5			0xFFB5
#define XK_KP_6			0xFFB6
#define XK_KP_7			0xFFB7
#define XK_KP_8			0xFFB8
#define XK_KP_9			0xFFB9

#define XK_Left			0xFF51	/* Move left, left arrow */
#define XK_Up			0xFF52	/* Move up, up arrow */
#define XK_Right		0xFF53	/* Move right, right arrow */
#define XK_Down			0xFF54	/* Move down, down arrow */

/* Modifiers */

#define XK_Shift_L		0xFFE1	/* Left shift */
#define XK_Shift_R		0xFFE2	/* Right shift */
#define XK_Control_L		0xFFE3	/* Left control */
#define XK_Control_R		0xFFE4	/* Right control */
#define XK_Caps_Lock		0xFFE5	/* Caps lock */
#define XK_Shift_Lock		0xFFE6	/* Shift lock */

#define XK_Meta_L		0xFFE7	/* Left meta */
#define XK_Meta_R		0xFFE8	/* Right meta */
#define XK_Alt_L		0xFFE9	/* Left alt */
#define XK_Alt_R		0xFFEA	/* Right alt */
#define XK_Super_L		0xFFEB	/* Left super */
#define XK_Super_R		0xFFEC	/* Right super */
#define XK_Hyper_L		0xFFED	/* Left hyper */
#define XK_Hyper_R		0xFFEE	/* Right hyper */

/* International & multi-key character composition */

#define XK_Multi_key		0xFF20  /* Multi-key character compose */
#define XK_Kanji		0xFF21	/* Kanji, Kanji convert */

/*
 *  Latin 1
 *  Byte 3 = 0
 */
#define XK_space               0x020
#define XK_exclam              0x021
#define XK_quotedbl            0x022
#define XK_numbersign          0x023
#define XK_dollar              0x024
#define XK_percent             0x025
#define XK_ampersand           0x026
#define XK_apostrophe          0x027
#define XK_quoteright          0x027	/* deprecated */
#define XK_parenleft           0x028
#define XK_parenright          0x029
#define XK_asterisk            0x02a
#define XK_plus                0x02b
#define XK_comma               0x02c
#define XK_minus               0x02d
#define XK_period              0x02e
#define XK_slash               0x02f
#define XK_0                   0x030
#define XK_1                   0x031
#define XK_2                   0x032
#define XK_3                   0x033
#define XK_4                   0x034
#define XK_5                   0x035
#define XK_6                   0x036
#define XK_7                   0x037
#define XK_8                   0x038
#define XK_9                   0x039
#define XK_colon               0x03a
#define XK_semicolon           0x03b
#define XK_less                0x03c
#define XK_equal               0x03d
#define XK_greater             0x03e
#define XK_question            0x03f
#define XK_at                  0x040
#define XK_A                   0x041
#define XK_B                   0x042
#define XK_C                   0x043
#define XK_D                   0x044
#define XK_E                   0x045
#define XK_F                   0x046
#define XK_G                   0x047
#define XK_H                   0x048
#define XK_I                   0x049
#define XK_J                   0x04a
#define XK_K                   0x04b
#define XK_L                   0x04c
#define XK_M                   0x04d
#define XK_N                   0x04e
#define XK_O                   0x04f
#define XK_P                   0x050
#define XK_Q                   0x051
#define XK_R                   0x052
#define XK_S                   0x053
#define XK_T                   0x054
#define XK_U                   0x055
#define XK_V                   0x056
#define XK_W                   0x057
#define XK_X                   0x058
#define XK_Y                   0x059
#define XK_Z                   0x05a
#define XK_bracketleft         0x05b
#define XK_backslash           0x05c
#define XK_bracketright        0x05d
#define XK_asciicircum         0x05e
#define XK_underscore          0x05f
#define XK_grave               0x060
#define XK_quoteleft           0x060	/* deprecated */
#define XK_a                   0x061
#define XK_b                   0x062
#define XK_c                   0x063
#define XK_d                   0x064
#define XK_e                   0x065
#define XK_f                   0x066
#define XK_g                   0x067
#define XK_h                   0x068
#define XK_i                   0x069
#define XK_j                   0x06a
#define XK_k                   0x06b
#define XK_l                   0x06c
#define XK_m                   0x06d
#define XK_n                   0x06e
#define XK_o                   0x06f
#define XK_p                   0x070
#define XK_q                   0x071
#define XK_r                   0x072
#define XK_s                   0x073
#define XK_t                   0x074
#define XK_u                   0x075
#define XK_v                   0x076
#define XK_w                   0x077
#define XK_x                   0x078
#define XK_y                   0x079
#define XK_z                   0x07a
#define XK_braceleft           0x07b
#define XK_bar                 0x07c
#define XK_braceright          0x07d
#define XK_asciitilde          0x07e

/*
 * TTY Functions, cleverly chosen to map to ascii, for convenience of
 * programming, but could have been arbitrary (at the cost of lookup
 * tables in client code.
 */

#define XK_BackSpace		0xFF08	/* back space, back char */
#define XK_Tab			0xFF09
#define XK_Linefeed		0xFF0A	/* Linefeed, LF */
#define XK_Clear		0xFF0B
#define XK_Return		0xFF0D	/* Return, enter */
#define XK_Pause		0xFF13	/* Pause, hold */
#define XK_Scroll_Lock		0xFF14
#define XK_Escape		0xFF1B
#define XK_Delete		0xFFFF	/* Delete, rubout */

/*
 * Keycodes for special keys and functions
 */

#define SHIFT	0xae
#define SHIFT_RIGHT 0xab
#define LOCK	0xb0
#define REPEAT	0xb4
#define CNTRL	0xaf
#define ALLUP	0xb3
#define HOLD	0x56

struct lk201info {
    int (*(lk_getc))();
    int (*(lk_putc))();
    short timeout;		/* autorepeat timeout */
    short interval;		/* autorepeat interval */
    ws_keyboard *kp;		/* keyboard associated with this device */
    short shift;
    short cntrl;
    short lock;
    short hold;
    short last;
    short inkbdreset;		/* are we resetting keyboard state? 	*/
    short up_down_mode;		/* is keyboard in up/down mode?		*/
    unsigned int keys[8];	/* current state of keys */
    ws_pointer *p;
    ws_event_queue *queue;
    short repeating;		/* do we have an autorepeat going?	*/
};

#ifdef KERNEL
int lk201_getc();
int lk201_putc();
caddr_t lk201_init_closure();
void lk201_up_down_mode();
void lk201_reset_keyboard();
void lk201_ring_bell();
int lk201_set_keyboard_control();
void lk201_autorepeat();
extern ws_keycode_modifiers lk201_modifiers[];
extern unsigned int lk201_keysyms[];
extern unsigned char lk201_keycodes[];
extern ws_keyboard_definition lk201_definition;
#endif
