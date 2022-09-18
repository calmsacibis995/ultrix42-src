#ifndef	CDROM_INCLUDE
#define	CDROM_INCLUDE	1
/*
 * @(#)cdrom.h	4.1	(ULTRIX)	2/14/91
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
 *   This software is  derived	from  software	received  from	the	*
 *   University    of	California,   Berkeley,   and	from   Bell	*
 *   Laboratories.  Use, duplication, or disclosure is	subject  to	*
 *   restrictions  under  license  agreements  with  University  of	*
 *   California and with AT&T.						*
 *									*
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or	reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/

/************************************************************************
 *
 * File:	cdrom.h
 * Date:	November 20, 1990
 * Author:	Robin Miller
 *
 * Description:
 *	CDROM I/O Control Definitions.
 *
 * Modification History:
 *
 */
#include <sys/ioctl.h>
#include <sys/types.h>

/*
 * Define CD-ROM Track Address structure.
 */
union cd_address {
	struct {
		u_char		: 8;
		u_char	m_units;
		u_char	s_units;
		u_char	f_units;
	} msf;				/* Minutes/Seconds/Frame format	*/
	    struct {
		u_char	addr3;
		u_char	addr2;
		u_char	addr1;
		u_char	addr0;
	} lba;				/* Logical Block Address format	*/
};

/*
 * CD-ROM Address Format Definitions.
 */
#define	CDROM_LBA_FORMAT	0	/* Logical Block Address format. */
#define	CDROM_MSF_FORMAT	1	/* Minute Second Frame format.	 */

/*
 * Structure for Play Audio Command.
 */
struct cd_play_audio {
	u_long	pa_lba;			/* Logical block address.	*/
	u_long	pa_length;		/* Transfer length in blocks.	*/
};

/*
 * Structure for Play Audio MSF Command.
 */
struct cd_play_audio_msf {
	u_char msf_starting_M_unit;	/* Starting M-unit.		*/
	u_char msf_starting_S_unit;	/* Starting S-unit.		*/
	u_char msf_starting_F_unit;	/* Starting F-unit.		*/
	u_char msf_ending_M_unit;	/* Ending M-unit.		*/
	u_char msf_ending_S_unit;	/* Ending S-unit.		*/
	u_char msf_ending_F_unit;	/* Ending F-unit.		*/
};

/*
 * Define Minimum and Maximum Values for Track & Index.
 */
#define CDROM_MIN_TRACK		1	/* Minimum track number.	*/
#define CDROM_MAX_TRACK		99	/* Maximum track number.	*/
#define CDROM_MIN_INDEX		1	/* Minimum index value.		*/
#define CDROM_MAX_INDEX		99	/* Maximum index value.		*/

/*
 * Structure for Play Audio Track/Index Command.
 */
struct cd_play_audio_ti {
	u_char	ti_starting_track;	/* Starting track number.	*/
	u_char	ti_starting_index;	/* Starting index value.	*/
	u_char	ti_ending_track;	/* Ending track number.		*/
	u_char	ti_ending_index;	/* Ending index value.		*/
};

/*
 * Structure for Play Audio Track Relative Command.
 */
struct cd_play_audio_tr {
	u_long	tr_lba;			/* Track relative LBA.		*/
	u_char	tr_starting_track;	/* Starting track number.	*/
	u_short	tr_xfer_length;		/* Transfer length.		*/
};

/*
 * Structure for Volume Control Command.
 */
struct cd_volume_control {
	u_char	vc_channel_0;		/* Channel 0 volume level.	*/
	u_char	vc_channel_1;		/* Channel 1 volume level.	*/
	u_char	vc_channel_2;		/* Channel 2 volume level.	*/
	u_char	vc_channel_3;		/* Channel 3 volume level.	*/
};

/*
 * Structure for Play Track Command.
 */
struct cd_play_track {
	u_char	pt_starting_track;	/* Starting track number.	*/
	u_char	pt_starting_index;	/* Starting index value.	*/
	u_char	pt_number_indexes;	/* Number of indexes.		*/
};

/*
 * Definitions for Playback Control/Playback Status Output Selection Codes.
 */
#define CDROM_MIN_VOLUME	0x0	/* Minimum volume level.	*/
#define CDROM_MAX_VOLUME	0xFF	/* Maximum volume level.	*/
#define CDROM_PORT_MUTED	0x0	/* Output port is muted.	*/
#define CDROM_CHANNEL_0		0x1	/* Channel 0 to output port.	*/
#define CDROM_CHANNEL_1		0x2	/* Channel 1 to output port.	*/
#define CDROM_CHANNEL_0_1	0x3	/* Channel 0 & 1 to output port	*/

/*
 * Structure for Playback Control/Status Commands.
 */
struct cd_playback {
	u_short	pb_alloc_length;	/* Allocation length.		*/
	caddr_t	pb_buffer;		/* Pointer to playback buffer.	*/
};

/*
 * Data Structure sent with Playback Control Command.
 */
struct cd_playback_control {
	u_char	pc_reserved[10];	/* Reserved.			*/
	u_char	pc_chan0_select	: 4,	/* Channel 0 selection code.	*/
				: 4;	/* Reserved.			*/
	u_char	pc_chan0_volume;	/* Channel 0 volume level.	*/
	u_char	pc_chan1_select	: 4,	/* Channel 1 selection code.	*/
				: 4;	/* Reserved.			*/
	u_char	pc_chan1_volume;	/* Channel 1 volume level.	*/
	u_char	pc_chan2_select	: 4,	/* Channel 2 selection code.	*/
				: 4;	/* Reserved.			*/
	u_char	pc_chan2_volume;	/* Channel 2 volume level.	*/
	u_char	pc_chan3_select	: 4,	/* Channel 3 selection code.	*/
				: 4;	/* Reserved.			*/
	u_char	pc_chan3_volume;	/* Channel 3 volume level.	*/
};

/*
 * Audio status return by Playback Status Command.
 */
#define PS_PLAY_IN_PROGRESS	0x00	/* Audio Play Oper In Progess.	*/
#define PS_PLAY_PAUSED		0x01	/* Audio Pause Oper In Progress	*/
#define PS_MUTING_ON		0x02	/* Audio Muting On.		*/
#define PS_PLAY_COMPLETED	0x03	/* Audio Play Oper Completed.	*/
#define PS_PLAY_ERROR		0x04	/* Error Occurred During Play.	*/
#define PS_PLAY_NOT_REQUESTED	0x05	/* Audio Play Oper Not Requested*/

/*
 * Data structure returned by Playback Status Command.
 */
struct cd_playback_status {
	u_char			: 8;	/* Reserved.			*/
	u_char	ps_lbamsf	: 1,	/* Address format 0/1 = LBA/MSF	*/
				: 7;	/* Reserved.			*/
	u_char	ps_data_len1;		/* Audio data length MSB.	*/
	u_char	ps_data_len0;		/* Audio data length LSB.	*/
	u_char	ps_audio_status;	/* Audio status.		*/
	u_char	ps_control	: 4,	/* Control field (attributes).	*/
				: 4;	/* Reserved.			*/
	union cd_address ps_absaddr;	/* Absolute CD-ROM address.	*/
	u_char	ps_chan0_select	: 4,	/* Channel 0 selection code.	*/
				: 4;	/* Reserved.			*/
	u_char	ps_chan0_volume;	/* Channel 0 volume level.	*/
	u_char	ps_chan1_select	: 4,	/* Channel 1 selection code.	*/
				: 4;	/* Reserved.			*/
	u_char	ps_chan1_volume;	/* Channel 1 volume level.	*/
	u_char	ps_chan2_select	: 4,	/* Channel 2 selection code.	*/
				: 4;	/* Reserved.			*/
	u_char	ps_chan2_volume;	/* Channel 2 volume level.	*/
	u_char	ps_chan3_select	: 4,	/* Channel 3 selection code.	*/
				: 4;	/* Reserved.			*/
	u_char	ps_chan3_volume;	/* Channel 3 volume level.	*/
};

#define ps_msf ps_absaddr.msf
#define ps_lba ps_absaddr.lba

/*
 * CD-ROM Sub-Channel Q Address Field Definitions.
 */
#define CDROM_NO_INFO_SUPPLIED	0x0	/* Information not supplied.	*/
#define CDROM_CURRENT_POS_DATA	0x1	/* Encodes current position data*/
#define CDROM_MEDIA_CATALOG_NUM	0x2	/* Encodes media catalog number.*/
#define CDROM_ENCODES_ISRC	0x3	/* Encodes ISRC.		*/
					/* ISRC=International-Standard-	*/
					/*		Recording-Code. */
/* Codes 0x4 through 0x7 are Reserved. */

/*
 * CD-ROM Data Track Definitions.
 */
#define CDROM_AUDIO_PREMPH	0x01	/* 0/1 = Without/With Pre-emphasis */
#define CDROM_COPY_PERMITTED	0x02	/* 0/1 = Copy Prohobited/Allowed   */
#define	CDROM_DATA_TRACK	0x04	/* 0 = Audio, 1 = Data track.	   */
#define CDROM_FOUR_CHAN_AUDIO	0x10	/* 0 = 2 Channel, 1 = 4 Channel.   */

/*
 * Structure for Table of Contents Entrys Command.
 */
struct cd_toc {
	u_char	toc_address_format;	/* Address format to return.	*/
	u_char	toc_starting_track;	/* Starting track number.	*/
	u_short	toc_alloc_length;	/* Allocation length.		*/
	caddr_t	toc_buffer;		/* Pointer to TOC buffer.	*/
};

/*
 * Define the Table of Contents (TOC) Header.
 */
struct cd_toc_header {
	u_char	th_data_len1;		/* TOC data length MSB.		*/
	u_char	th_data_len0;		/* TOC data length LSB.		*/
	u_char	th_starting_track;	/* Starting track number.	*/
	u_char	th_ending_track;	/* Ending track number.		*/
};

/*
 * Define Format of Entrys in Table of Contents.
 */
struct cd_toc_entry {
	u_char			: 8;	/* Reserved.			*/
	u_char	te_control	: 4;	/* Control field (attributes).	*/
	u_char	te_addr_type	: 4;	/* Address type information.	*/
	u_char	te_track_number;	/* The track number.		*/
	u_char			: 8;	/* Reserved.			*/
	union cd_address te_absaddr;	/* Absolute CD-ROM Address.	*/
};

#define te_msf te_absaddr.msf
#define te_lba te_absaddr.lba

/*
 * Sub-Channel Data Format Codes.
 */
#define CDROM_SUBQ_DATA		0x00	/* Sub-Channel data information.*/
#define CDROM_CURRENT_POSITION	0x01	/* Current position information.*/
#define CDROM_MEDIA_CATALOG	0x02	/* Media catalog number.	*/
#define CDROM_ISRC		0x03	/* ISRC information.		*/
					/* ISRC=International-Standard-	*/
					/*		Recording-Code. */
/* Codes 0x4 through 0xEF are Reserved. */
/* Codes 0xF0 through 0xFF are Vendor Specific. */

/*
 * Audio Status Definitions returned by Read Sub-Channel Data Command.
 */
#define	AS_AUDIO_INVALID	0x00	/* Audio status not supported.	*/
#define	AS_PLAY_IN_PROGRESS	0x11	/* Audio play operation in prog	*/
#define	AS_PLAY_PAUSED		0x12	/* Audio play operation paused.	*/
#define	AS_PLAY_COMPLETED	0x13	/* Audio play completed.	*/
#define	AS_PLAY_ERROR		0x14	/* Audio play stopped by error.	*/
#define	AS_NO_STATUS		0x15	/* No current audio status.	*/

/*
 * Structure for Read Sub-Channel Command.
 */
struct cd_sub_channel {
	u_char	sch_address_format;	/* Address format to return.	*/
	u_char	sch_data_format;	/* Sub-channel data format code	*/
	u_char	sch_track_number;	/* Track number.		*/
	u_short	sch_alloc_length;	/* Allocation length.		*/
	caddr_t	sch_buffer;		/* Pointer to SUBCHAN buffer.	*/
};

/*
 * Define Current Position Data Format.
 */
struct cd_subc_position {
	u_char	scp_data_format;	/* Data Format code.		*/
	u_char	scp_control	: 4;	/* Control field (attributes).	*/
	u_char	scp_addr_type	: 4;	/* Address type information.	*/
	u_char	scp_track_number;	/* The track number.		*/
	u_char	scp_index_number;	/* The index number.		*/
	union cd_address scp_absaddr;	/* Absolute CD-ROM Address.	*/
	union cd_address scp_reladdr;	/* Relative CD-ROM Address.	*/
};

#define scp_absmsf scp_absaddr.msf
#define scp_abslba scp_absaddr.lba
#define scp_relmsf scp_reladdr.msf
#define scp_rellba scp_reladdr.lba

/*
 * Define Media Catalog Number Data Format.
 */
struct cd_subc_media_catalog {
	u_char	smc_data_format;	/* Data Format code.		*/
	u_char			: 8;	/* Reserved.			*/
	u_char			: 8;	/* Reserved.			*/
	u_char			: 8;	/* Reserved.			*/
	u_char			: 7,	/* Reserved.			*/
		smc_mc_valid	: 1;	/* Media catalog valid 1 = True	*/
	u_char	smc_mc_number[15];	/* Media catalog number ASCII.	*/
};

/*
 * Define Track International Standard Recording Code Data Format.
 */
struct cd_subc_isrc_data {
	u_char	sid_data_format;	/* Data Format code.		*/
	u_char			: 8;	/* Reserved.			*/
	u_char	sid_track_number;	/* The track number.		*/
	u_char			: 8;	/* Reserved.			*/
	u_char			: 7,	/* Reserved.			*/
		sid_tc_valid	: 1;	/* Track code valid, 1 = True.	*/
	u_char	sid_tc_number[15];	/* International-Standard-	*/
					/*	Recording-Code (ASCII).	*/
};

/*
 * Define the Sub-Channel Data Header.
 */
struct cd_subc_header {
	u_char			: 8;	/* Reserved.			*/
	u_char	sh_audio_status;	/* Audio status.		*/
	u_char	sh_data_len1;		/* Sub-Channel Data length MSB.	*/
	u_char	sh_data_len0;		/* Sub-Channel Data length LSB.	*/
};

/*
 * Define Structure To Return All Sub-Channel Data.
 */
struct cd_subc_channel_data {
	struct cd_subc_header scd_header;
	struct cd_subc_position scd_position_data;
	struct cd_subc_media_catalog scd_media_catalog;
	struct cd_subc_isrc_data scd_isrc_data;
};

/*
 * Define General Sub-Channel Information Structure.
 */
struct cd_subc_information {
	struct cd_subc_header sci_header;
	union {
		struct cd_subc_channel_data sci_channel_data;
		struct cd_subc_position sci_position_data;
		struct cd_subc_media_catalog sci_media_catalog;
		struct cd_subc_isrc_data sci_isrc_data;
	} sci_data;
};

#define sci_scd		sci_data.sci_channel_data
#define sci_scp		sci_data.sci_position_data
#define sci_smc		sci_data.sci_media_catalog
#define sci_sid		sci_data.sci_isrc_data

/*
 * CD-ROM Data Mode Definitions.
 */
#define CDROM_DATA_MODE_ZERO	0	/* All bytes zero.		*/
#define CDROM_DATA_MODE_ONE	1	/* Data mode one format.	*/
#define CDROM_DATA_MODE_TWO	2	/* Data mode two format.	*/
/* Modes 0x03-0xFF are reserved. */

/*
 * Structure for Get/Set Data Mode Commands.
 */
struct cd_data_mode {
	u_char	dm_data_mode;		/* Data mode format.		*/
	u_char			: 8;	/* Reserved.			*/
	u_char			: 8;	/* Reserved.			*/
	u_char			: 8;	/* Reserved.			*/
	union cd_address dm_absaddr;	/* Absolute CD-ROM address.	*/
};

#define dm_msf dm_absaddr.msf
#define dm_lba dm_absaddr.lba

/*
 * Structure for Read Header Command.
 */
struct cd_read_header {
	u_char	rh_address_format;	/* Address format to return.	*/
	u_long	rh_lba;			/* Logical block address.	*/
	u_short	rh_alloc_length;	/* Allocation length.		*/
	caddr_t	rh_buffer;		/* Pointer to header buffer.	*/
};

/*
 * Data Structure returned by Read Header Command.
 */
struct cd_read_header_data {
	u_char	rhd_data_mode;		/* CD-ROM data mode (see above)	*/
	u_char			: 8;	/* Reserved.			*/
	u_char			: 8;	/* Reserved.			*/
	u_char			: 8;	/* Reserved.			*/
	union cd_address rhd_absaddr;	/* Absolute CD-ROM address.	*/
};

#define rhd_msf rhd_absaddr.msf
#define rhd_lba rhd_absaddr.lba

/*
 * CD-ROM Diagnostic Parameter Definitions.
 */
#define CDROM_NO_DIAGNOSTICS	0	/* Don't perform diagnostic.	*/
#define CDROM_CONTROLLER	1	/* Do controller diagnostic.	*/
#define CDROM_DRIVE_CONTROL	2	/* Do drive control diagnostic.	*/

#define CDROM_DIAG_SUCCESS	0	/* Diagnostic was successful.	*/

/*
 * Data Structure for Send/Receive Diagnostics Commands.
 */
struct cd_diagnostic_data {
	u_char			: 8;	/* Reserved.			*/
	u_char	dd_param_length;	/* Parameter length.		*/
	u_char	dd_rom_diag;		/* ROM diagnostic.		*/
	u_char	dd_ram_diag;		/* RAM diagnostic.		*/
	u_char	dd_data_buffer_diag;	/* Data buffer diagnostic.	*/
	u_char	dd_interface_diag;	/* Interface diagnostic.	*/
	u_char	dd_reserved[2];		/* Reserved.			*/
};

/*
 * CD-ROM I/O Control Commands.
 */
#define	CDROM_PAUSE_PLAY	_IO('p', 25) /* Pause audio operation.	*/
#define	CDROM_RESUME_PLAY	_IO('p', 26) /* Resume audio operation.	*/
#define CDROM_PLAY_AUDIO	\
	_IOW('p', 27, struct cd_play_audio)	/* Play audio LBA format.*/
#define	CDROM_PLAY_AUDIO_MSF	\
	_IOW('p', 28, struct cd_play_audio_msf)	/* Play audio MSF format.*/
#define	CDROM_PLAY_AUDIO_TI	\
	_IOW('p', 29, struct cd_play_audio_ti)	/* Play audio track/index.*/
#define	CDROM_PLAY_AUDIO_TR	\
	_IOW('p', 30, struct cd_play_audio_tr)	/* Play track relative. */
#define	CDROM_TOC_HEADER	\
	_IOR('p', 31, struct cd_toc_header) /* Read TOC header.		*/
#define	CDROM_TOC_ENTRYS	\
	_IOWR('p', 32, struct cd_toc)	/* Read TOC entrys.		*/
#define	CDROM_EJECT_CADDY	_IO('p', 33) /* Ejects the CDROM caddy.	*/
#define	CDROM_READ_SUBCHANNEL	\
	_IOWR('p', 36, struct cd_sub_channel) /* Read sub-channel data.	*/
#ifdef notdef
#define CDROM_GET_DATAMODE	\
	_IOR('p', 38, struct cd_data_mode)    /* Get current data mode.	*/
#define CDROM_SET_DATAMODE	\
	_IOW('p', 39, struct cd_data_mode)    /* Set the data mode.	*/
#endif notdef
#define CDROM_READ_HEADER	\
	_IOWR('p', 40, struct cd_read_header)  /* Read track header.	*/

/*
 * Vendor Unique Commands.
 */
#define CDROM_PLAY_VAUDIO	\
	_IOW('p', 50, struct cd_play_audio)	/* Play audio LBA format.*/
#define CDROM_PLAY_MSF		\
	_IOW('p', 51, struct cd_play_audio_msf)	/* Play audio MSF format.*/
#define CDROM_PLAY_TRACK	\
	_IOW('p', 52, struct cd_play_track)	/* Play audio track.	*/
#define CDROM_PLAYBACK_CONTROL	\
	_IOW('p', 53, struct cd_playback)	/* Playback control.	*/
#define CDROM_PLAYBACK_STATUS	\
	_IOWR('p', 54, struct cd_playback)	/* Playback status.	*/
#define CDROM_SET_ADDRESS_FORMAT \
	_IOW('p', 55, int)			/* Address Format.	*/

#endif /*	CDROM_INCLUDE */
