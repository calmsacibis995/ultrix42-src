/*
 * star.h
 *
 * Name:	starship
 * Purpose:	defines & data structure definitions
 * Environment:	Ultrix-32, with shared memory.
 * Compile:	see Makefile
 * Date:	April 19 1985
 * Author:	Alan Delorey
 * Remarks:

    These are the voyages of the independent star ships.

Whose lifetime mission: To explore strange new galaxies,
		        To seek out and destroy other star ships,
		        To boldly go where no other star ship dares!

*/

/* all arrays start at index 0 */

#define A 65		/* ASCII value of the letter A */
#define true 1
#define false 0
#define RETURN '\012'	/* ASCII value of LINE FEED (UNIX char for return) */
#define ASCII_ESC  27

#define PASSWDLEN  5	/* length of password for enterprise */
#define READSIZE  68	/* # of chars to scan for command input */

#define TORP  '.'
#define STAR  '*'
#define ION  '-'
#define BASE  '+'

#define VERT '|'	/* vertical lines on short & long range scans */
#define HORIZ '-'	/* horizontal lines on short & long range scans */
#define RCROSSHAIR HORIZ /* crosshair showing quad edge on short range scan */
#define CCROSSHAIR VERT	/* crosshair showing quad edge on short range scan */

#define FIRSTR 0	/* # of top row on the screen */
#define FIRSTC 0	/* # of leftmost col on the screen */

#define SRSCANR  1	/* start of srscan field */
#define SRSCANC  2
#define SRSCANROWS 11	/* size of srscan in rows & cols */
#define SRSCANCOLS 15	
#define SHDISPR 5	/* position w/ in SRSCAN where ship is displayed */
#define SHDISPC 7	
#define LSRSCANR (SRSCANROWS + SRSCANR - 1)  /* last row & col of srscan disp */
#define LSRSCANC (SRSCANCOLS + SRSCANC - 1)

#define LRSCANR  3	/* start of LRSCAN field */
#define LRSCANC  25
#define LRSCANSECTS  3	/* # of sects in LRSCAN */
#define LRSCANSIZE  5	/* width of LRSCAN sects */
#define LRLABELR  10
#define LRLABELC  24

#define DIRDISPR  2	/* directional indicator */
#define DIRDISPC  47
#define MAPDISPR  1	/* galaxy map display */
#define MAPDISPC  45
#define MAP_TIME  5

#define FLEETR  FIRSTR
#define FLEETC  59
#define FLEETMAXR  (BEARINGR - 1)

#define PHSRANGE 5	/* # of sectors that phasers will reach */
#define NUMQUADS 10	/* # of rows & cols of quadrants in the universe */
#define NUMSECTORS 10	/* # of rows & cols of sectors in each quadrant */
			/* NOTE: a sector is one char position */

	  /* status display fields */

#define BEARINGR  13	/* screen loc where VALUE (not label) is written */
#define BEARINGC  12
#define WARPSPEEDR  14
#define WARPSPEEDC  12
#define WARPSETR  15
#define WARPSETC  12
#define SECTORR  16
#define SECTORC  12
#define QUADR  17
#define QUADC  12

#define ENERGYR  13
#define ENERGYC  29
#define SHIELDSR 14
#define SHIELDSC 29
#define TORPR  15
#define TORPC  30
#define PHASERR  16
#define PHASERC  30
#define WARPDRIVER  17
#define WARPDRIVEC  30

#define IMPULSER  13
#define IMPULSEC  53
#define LIFESUPR  14
#define LIFESUPC  52
#define SRCONDR  15
#define SRCONDC  53
#define LRCONDR  16
#define LRCONDC  53
#define RADIOR   17
#define RADIOC   53

#define SENSORR  13
#define SENSORC  75
#define CLOAKINGR  14
#define CLOAKINGC  75

#define CMDHELPR  18		/* row to start command help at */
#define CMDHELPC  FIRSTC	/* col to start command help at */

#define MESSAGER  21	/* message line */
#define MESSAGEC  9

#define COMMANDR  22	/* location for command input */
#define COMMANDC  9

	  /* number of stars and bases */

#define NAMESIZE  10	/* # of chars in the ship names */
#define NUMSTARS  200	/* originally 300, decreased for now */
#define NUMBASES  5	/* # of star bases in the galaxy */
#define NUMCLOUDS 7	/* # of ion clouds in the galaxy */

#define ENERGY_MAX  50000
#define TORP_MAX  20	/* number of torpedos a star ship carries */
#define TORP_TRAVEL  10	/* distance (in sectors) a torp will travel */
#define TORP_VELOCITY  2 /* warp speed a torp will travel at */
#define WARP_MAX  6	/* max speed of the star ships */

#define FLEETMAX  9	/* # of ships that can be displayed in fleet display */

#define SPEED_CHAMT  0.5	/* amount of warp speed to change at once */

#define BASE_COST  1000		/* power lost when coliding w/ a base */
#define SHIP_COST  1000		/* power lost when coliding w/ a ship */
#define TORP_COST  7500		/* power lost when hit by a torpedo */
#define STAR_COST  10000	/* energy lost going thru a star */
#define ION_COST   500		/* power lost when passing thru an ion */
#define CHSPEED_COST 100	/* energy used changing speed (per SPEED_CHAMT) */
#define LOCATE_COST 7500	/* energy used to locate another ship */
#define CLOAK_COST 250		/* energy used ea sec cloaking is on */
#define PHASER_COST 10000	/* energy used in phaser blasts */

#define CONSTSP_SAVE  50	/* energy built up by keeping  speed */

#define MSG_TIME  3	/* # of seconds to leave message on screen */
#define DAMAGE_CONST  1	/* # of seconds dev out, per 100 energy units of hurt */
#define NUMDRONES 2	/* # of drones that can be in the game at once */


typedef char boolean;	/* make a boolean type to be one char */

typedef struct sector 	/* 10,000 sectors in universe */
	{
	char image;	/* the image in that sector */
	};

typedef struct quadrant
	{
	struct sector sectors[NUMSECTORS][NUMSECTORS]; /* one quadrant */
	int nships;		/* no of items in the quadrant */
	int nbases;
	int nstars;
	};

typedef struct torpedo_record
	{
	int torp_dir;		/* 1-9 (-1 means no torp) */
	int torp_dist;		/* how many sectors its gone */
	int torp_speed;		/* usu TORP_VELOCITY; if in dir of ship,
				   ship speed + TORP_VELOCITY */
	int tquadr;		/* current torpedo location */
	int tquadc;
	int tsectr;
	int tsectc;
	};


    /* star ship info record */

enum life_supports {RED, YELLOW, GREEN}; /* condition RED, YELLOW, GREEN */

typedef struct ship
	{
	char ship_ch;	/* char to rep. ship */
	char name[NAMESIZE + 1]; 	/* ship name */
	int energy;	/* energy level */
	int shields;	/* shield strength */
	int shield_time; /* 0 == OPR; n = time to repair */
	int torpedos;	/* # of torpedos remaining */
	int torp_time; /* 0 == OPR; n = time to repair */
	int phasers;	/* 0 == OPR; n = time to repair */
	float warp_set;	/* warp speed setting, 0.0 to 6.0 */
	float warp_speed;	/* current warp speed, 0.0 to 6.0 */
	int warp_drive; /* 0 == OPR; n = time to repair */
	int q_row, q_col;	/* ship location */
	int s_row, s_col;
	int bearing;	/* 1-9 for direction */
	int impulse;	/* 0 == OPR; n = time to repair */
	int sub_light; /* sublight speed counter: 2-4 secs/sector */
	enum life_supports life_support; /* condition RED, YELLOW, GREEN */
	int life_supp_time; /* 0 == OPR; n = time to repair */
	int sr_scan;	/* 0 == OPR; n = time to repair */
	int lr_scan;	/* 0 == OPR; n = time to repair */
	int radio;	/* 0 == OPR; n = time to repair */
	int sensors;	/* 0 == OPR; n = time to repair */
	int cloaking;	/* 0 == OPR; n = time to repair */
	boolean dock;	/* set true when dock command given */
	boolean docked;	/* kept true while ship still docked */
	boolean ship_invis; /* set true when ship going thru a star */
	boolean cloak;	/* set true when cloaking device is on */
	char msg_buf [READSIZE]; /* radio & other messages */
	int msg_timer;	/* # of seconds message has been on screen */
	int map_timer;	/* # of seconds galaxy map has been on screen */
	int kills;	/* how many ships killed */
	float avekills;	/* ave number of kills for this user */
	int glow;	/* set true if ship hit by phaser or torpedo */

	struct torpedo_record torp_info [TORP_MAX];	/* torp info for this ship */
	boolean torp_active; /* TRUE if any torpedos active for this ship */
	int bases[NUMBASES][2];	/* quadrant coordinates of bases "seen" */
	boolean allbases;	/* set true when all bases found */
	int locater, locatec;	/* row & col of `located' ship */
	int radiorow, radiocol;	/* row & col of ship in last radio msg */
	int starcounter;	/* counts seconds you spend in a star */

	boolean dis_energy;	/* Display the corresponding field when set to true */
	boolean dis_shields;
	boolean dis_torpedos;
	boolean dis_warp_set;
	boolean dis_warp_speed;
	boolean dis_quad;
	boolean dis_sector;
	boolean dis_bearing;
	boolean dis_fleet;
	boolean dis_msg;
	boolean dis_map;
	boolean dis_dir;

#ifdef OLDSR
	char old_sr [SRSCANROWS] [SRSCANCOLS];	/* old image of srscan */
#endif
	char old_lr1 [LRSCANSECTS] [LRSCANSECTS] [4];
	char *old_lr [LRSCANSECTS] [LRSCANSECTS]; /* old long range scan */
	int ioff;			/* old location of crosshair */
	int joff1;			/* old location of crosshair */
	int joff2;			/* old location of crosshair */
	};  /* ship record */

typedef struct drone
	{
	char d_char;		/* char to rep drone ship */
	char d_after;		/* char of ship which drone is after */
	char d_name[NAMESIZE + 1]; /* name of drone ship */
	boolean d_busy;		/* true if drone busy */
	int d_level;		/* level of difficulty */
	int d_delay;		/* level of difficulty */
	};

typedef struct univ		/* whole univers */
	{
	int Num_users;		/* MUST be 1st, to avoid race conditions */
	int Wait;		/* set true whiel 1st user inits DB */
	struct quadrant quadrants[NUMQUADS][NUMQUADS];
	struct ship Ships[26];
	struct drone Drones[NUMDRONES];
	};
