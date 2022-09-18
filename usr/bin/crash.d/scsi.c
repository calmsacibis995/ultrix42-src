#ifndef	lint 
static	char	*sccsid = "@(#)scsi.c	4.4	(ULTRIX)	2/14/91";
#endif	lint

/************************************************************************
 *                                                                      *
 *                      Copyright (c) 1989 by                           *
 *              Digital Equipment Corporation, Maynard, MA              *
 *                      All rights reserved.                            *
 *                                                                      *
 *   This software is furnished under a license and may be used and     *
 *   copied  only  in accordance with the terms of such license and     *
 *   with the  inclusion  of  the  above  copyright  notice.   This     *
 *   software  or  any  other copies thereof may not be provided or     *
 *   otherwise made available to any other person.  No title to and     *
 *   ownership of the software is hereby transferred.                   *
 *                                                                      *
 *   The information in this software is subject to change  without     *
 *   notice  and should not be construed as a commitment by Digital     *
 *   Equipment Corporation.                                             *
 *                                                                      *
 *   Digital assumes no responsibility for the use  or  reliability     *
 *   of its software on equipment which is not supplied by Digital.     *
 *                                                                      *
 ************************************************************************
 *
 *
 *   Facility:	SCSI data structure formatter for the crash utility.
 *
 *   Abstract:	This module contains the routines used to display the
 *		contents of sz_softc data structure.
 *
 *   Author:	Janet L. Schank    9/28/89
 *
 *   History:
 *
 *   29-Jan-91   Robin Miller
 *	Added CD-ROM audio commands to scsi_command_opcodes[] table.
 *	This support is only being added to the MIPS side at this time.
 *
 *   07-Jan-91   Robin Miller
 *   o	Removed CD-ROM commands from scsi_command_opcodes[] table since
 *	CD-ROM audio support isn't being released in ULTRIX 4.Titanium.
 *   o	Added 10-byte READ/WRITE commands to scsi_command_opcodes[] table.
 *   o	Removed '#ifdef FORMAT' conditionalization, always included.
 *   o	Removed '#ifdef vax' conditionalization around READ/WRITE LONG
 *	commands since they also exist in the MIPS code.
 *
 *   30-Aug-90   Robin Miller
 *   o	Added devices RZ24, RZ57, RZ23L, RX26, RZ25, & RRD42 to the
 *	scsi_device_class[] table.
 *   o	Added new density codes to scsi_category_stat_devio[] table.
 *   o	Added CDROM commands to scsi_command_opcodes[] table.
 *
 *   02-Mar-90   Janet Schank
 *        Changed to use the new mips softc pointer.  Changed NSCSI to
 *        NSCSIBUS.
 *
 */

#include 	<sys/types.h>
#include 	<sys/buf.h>
#include 	<sys/devio.h>
#include 	<sys/mtio.h>
#include 	<sys/param.h>
#include	<fs/ufs/fs.h>
#include	<stdio.h>

#include	"crash.h"

#ifdef vax
#include        <io/scsi/vax/scsivar.h>
#include        <io/scsi/vax/scsireg.h>
#endif vax
#ifdef mips
#include        <io/scsi/mips/scsivar.h>
#include        <io/scsi/mips/scsireg.h>
#endif mips

#define EQUAL_MATCH 0
#define OR_MATCH 1

int
	scsi_read_softc();
char
	*scsi_decode();
void
	do_scsiprint_trans(),
	do_scsiprint_dctstats(),
	do_scsiprint_spinstats(),
	do_scsiprint_sii(),
	do_scsiprint_err(),
	do_scsiprint_bbr(),
	do_scsiprint_cmd(),
	do_scsiprint_devtab(),
	do_scsiprint_targ(),
	do_scsiprint_cntlr();


struct scsi_dcode {
	long code;
	char *desc;
};

struct scsi_dcode scsi_device_flags[] = {
	{ SCSI_REQSNS,    "SCSI_REQSNS"                             },
	{ SCSI_STARTUNIT, "SCSI_STARTUNIT"                          },
	{ SCSI_TRYSYNC,   "SCSI_TRYSYNC"                            },
	{ SCSI_TESTUNITREADY,"SCSI_TESTUNITREADY"                   },
	{ SCSI_READCAPACITY,"SCSI_READCAPACITY"                     },
	{ SCSI_NODIAG,    "SCSI_NODIAG"                             },
	{ SCSI_MODSEL_PF, "SCSI_MODSEL_PF"                          },
	{ SCSI_REMOVABLE_DISK,"SCSI_REMOVABLE_DISK"                 },
	{ SCSI_MODSEL_EXABYTE,"SCSI_MODSEL_EXABYTE"                 },
#ifdef vax
	{ SCSI_NODBBR,    "SCSI_NODBBR"                             },
#endif vax
        { NULL,           NULL                                      }
};

struct scsi_dcode scsi_mesg_protocols[] = {
	{ SZ_CMDCPT,     "Command Complete"                         },
        { SZ_EXTMSG,     "Extended message"                         },
        { SZ_SDP,        "Save Data Pointers"                       },
        { SZ_RDP,        "Restore Data Pointers (DISK: new)"        },
        { SZ_DISCON,     "Disconnect"                               },
#ifdef vax
        { SZ_IDE,        "Initiator Detected Error"                 },
#endif vax
        { SZ_ABT,        "Abort"                                    },
        { SZ_MSGREJ,     "Message Reject"                           },
        { SZ_NOP,        "No Operation"                             },
        { SZ_MSGPE,      "Message Parity Error"                     },
        { SZ_LNKCMP,     "Linked Command Complete"                  },
        { SZ_LNKCMPF,    "Linked Command Complete with Flag"        },
        { SZ_DEVRST,     "Bus Device Reset"                         },
        { SZ_ID_NODIS,   "IDENTIFY wo/ disconnect capability"       },
        { SZ_ID_DIS,     "IDENTIFY w/ disconnect capability"        },
        { NULL,          NULL                                       }
};

struct scsi_dcode scsi_phase_states[] = {
        { SZ_SP_ARB,     "Arbitrate for the Bus"                    },
	{ SZ_SP_SEL,     "Select the target device"                 },
        { SZ_CMD_PHA,    "Command Phase"                            },
        { SZ_DATAO_PHA,  "Data Out Phase"                           },
	{ SZ_DATAI_PHA,  "Data In Phase"                            },
	{ SZ_STATUS_PHA, "Status Phase"                             },
	{ SZ_MESSI_PHA,  "Message In Phase"                         },
	{ SZ_MESSO_PHA,  "Message Out Phase"                        },
	{ SZ_RELBUS,     "Release the Bus"                          },
        { NULL,          NULL                                       }
};

struct scsi_dcode scsi_status_byte[] = {
	{ SZ_GOOD,       "Good"                                     },
	{ SZ_CHKCND,     "Check Condition"                          },
        { SZ_BUSY,       "Device cannot accept a command (busy)"    },
	{ SZ_INTRM,      "Intermediate"                             },
	{ SZ_RESCNF,     "Reservation Conflict"                     },
	{ SZ_BAD,        "Fatal error command (couldn't do RQSNS)"  },
        { NULL,          NULL                                       },
};

struct scsi_dcode scsi_statpos_states[] = {
	{ SZ_NEXT,       "Process next request from queue"          },
	{ SZ_SP_START,   "Start a Status or Positioning Operation"  },
	{ SZ_SP_CONT,    "Continue SZ_SP_SETUP:"                    },
	{ SZ_RW_START,   "Setup a Read or Write Operation"          },
	{ SZ_RW_CONT,    "Start/Continue a Read or Write Operation" },
	{ SZ_R_STDMA,    "Start DMA READ"                           },
	{ SZ_W_STDMA,    "Start DMA WRITE"                          },
	{ SZ_R_DMA,      "Read DMA processing"                      },
        { SZ_W_DMA,      "Write DMA processing"                     },
	{ SZ_R_COPY,     "Copy READ data to memory"                 },
	{ SZ_ERR,        "DMA Error"                                },
        { NULL,          NULL                                       }
};

struct scsi_dcode scsi_stat_devio[] = {
	{ DEV_BOM,       "Beginning-of-medium (BOM)"                },
	{ DEV_EOM,       "End-of-medium (EOM)"                      },
	{ DEV_OFFLINE,   "Offline"                                  },
	{ DEV_WRTLCK,    "Write locked"                             },
	{ DEV_BLANK,     "Blank media"                              },
        { DEV_WRITTEN,   "Write on last operation"                  },
        { DEV_CSE,       "Cleared serious exception"                },
	{ DEV_SOFTERR,   "Device soft error"                        },
	{ DEV_HARDERR,   "Device hard error"                        },
	{ DEV_DONE,      "Operation complete"                       },
	{ DEV_RETRY,     "Retry"                                    },
	{ DEV_ERASED,    "Erased"                                   },
        { NULL,          NULL                                       }
};

struct scsi_dcode scsi_category_stat_devio[] = {
	{ DEV_TPMARK,    "Unexpected tape mark"                     },
	{ DEV_SHRTREC,   "Short record"                             },
	{ DEV_RDOPP,     "Read opposite"                            },
	{ DEV_RWDING,    "Rewinding"                                },
	{ DEV_800BPI,    "800 bpi tape density"                     },
	{ DEV_1600BPI,   "1600 bpi tape density"                    },
	{ DEV_6250BPI,   "6250 bpi tape density"                    },
	{ DEV_6666BPI,   "6666 bpi tape density"                    },
        { DEV_10240BPI,  "10240 bpi tape density"                   },
	{ DEV_38000BPI,  "38000 bpi tape density"		    },
	{ DEV_LOADER,    "Media loader present"			    },
	{ DEV_38000_CP,  "38000 bpi compacted density"		    },
	{ DEV_76000BPI,  "76000 bpi tape density"		    },
	{ DEV_76000_CP,  "76000 bpi compacted density"		    },
	{ DEV_8000_BPI,  "QIC-24 9 tracks"			    },
	{ DEV_10000_BPI, "QIC-120 15 tracks & QIC-150 18 tracks"    },
	{ DEV_16000_BPI, "QIC-320/525 26 tracks"		    },
        { NULL,          NULL                                       }
};

struct scsi_dcode scsi_command_opcodes[] = {
	{ SZ_TUR,        "TEST UNIT READY Command"                  },
	{ SZ_REWIND,     "REWIND Command"                           },
	{ SZ_RQSNS,      "REQUEST SENSE Command"                    },
	{ SZ_RBL,        "READ BLOCK LIMITS Command"                },
        { SZ_READ,       "READ Command"                             },
	{ SZ_WRITE,      "WRITE Command"                            },
	{ SZ_TRKSEL,     "TRACK SELECT Command"                     },
	{ SZ_RESUNIT,    "RESERVE UNIT Command"                     },
	{ SZ_WFM,        "WRITE FILEMARKS Command"                  },
        { SZ_SPACE,      "SPACE Command"                            },
	{ SZ_INQ,        "INQUIRY Command"                          },
	{ SZ_VFY,        "VERIFY Command"                           },
	{ SZ_RBD,        "RECOVER BUFFERED DATA Command"            },
	{ SZ_MODSEL,     "MODE SELECT Command"                      },
	{ SZ_RELUNIT,    "RELEASE UNIT Command"                     },
	{ SZ_ERASE,      "ERASE Command"                            },
	{ SZ_MODSNS,     "MODE SENSE Command"                       },
	{ SZ_SSLU,       "START/STOP or LOAD/UNLOAD Command"        },
	{ SZ_RECDIAG,    "RECEIVE DIAGNOSTIC RESULT Command"        },
	{ SZ_SNDDIAG,    "SEND DIAGNOSTIC Command"                  },
#ifdef mips
	{ SZ_MEDREMOVAL, "PREVENT/ALLOW MEDIUM REMOVAL Command"	    },
#endif /* mips */
	{ SZ_P_FSPACER,  "Psuedo opcode for space record"           },
	{ SZ_P_FSPACEF,  "Psuedo opcode for space file"             },
	{ SZ_RDCAP,      "DISK: READ CAPACITY"                      },
	{ SZ_P_BSPACER,  "Psuedo opcode for backspace record"       },
	{ SZ_P_BSPACEF,  "Pseudo opcode for backspace file"         },
	{ SZ_P_CACHE,    "Pseudo opcode for buffered mode"          },
	{ SZ_P_NOCACHE,  "Pseudo opcode for no buffered mode"       },
	{ SZ_P_LOAD,     "Pseudo opcode for load (not used)"        },
	{ SZ_P_UNLOAD,   "Pseudo opcode for unload"                 },
	{ SZ_P_SSUNIT,   "Pseudo opcode for start/stop unit"        },
	{ SZ_P_RETENSION,"Pseudo opcode for retension"		    },
#ifdef mips
	{ SZ_P_EJECT,	 "Psuedo opcode for eject caddy"	    },
#endif /* mips */
	{ SZ_FORMAT,     "DISK: FORMAT UNIT Command"                },
	{ SZ_REASSIGN,   "DISK: REASSIGN BLOCK Command"             },
	{ SZ_VFY_DATA,   "DISK: VERIFY DATA Command"                },
	{ SZ_RDD,        "DISK: READ DEFECT DATA Command"           },
	{ SZ_READL,      "DISK: READ LONG Command"                  },
	{ SZ_WRITEL,     "DISK: WRITE LONG Command"                 },
	{ SZ_READ_10,    "DISK: READ 10-byte Command"		    },
	{ SZ_WRITE_10,   "DISK: WRITE 10-byte Command"		    },
#ifdef mips
	{ SZ_SEEK_10,	 "DISK: SEEK 10-byte Command"		    },
#endif /* mips */
	{ RZSPECIAL,     "DISK: set for special disk commands"      },
#ifdef mips
	{ SZ_WRITE_BUFFER,      "CDROM: WRITE BUFFER Command"	    },
	{ SZ_READ_BUFFER,       "CDROM: READ BUFFER Command"	    },
	{ SZ_CHANGE_DEFINITION,	"CDROM: CHANGE DEFINITION Command"  },
	{ SZ_READ_SUBCHAN,	"CDROM: READ SUB-CHANNEL Command"   },
	{ SZ_READ_TOC,		"CDROM: READ TOC Command"	    },
	{ SZ_READ_HEADER,	"CDROM: READ HEADER Command"	    },
	{ SZ_PLAY_AUDIO,	"CDROM: PLAY AUDIO Command"	    },
	{ SZ_PLAY_AUDIO_MSF,	"CDROM: PLAY AUDIO MSF Command"	    },
	{ SZ_PLAY_AUDIO_TI,	"CDROM: PLAY AUDIO TRACK/INDEX"	    },
	{ SZ_PLAY_TRACK_REL,	"CDROM: PLAY TRACK RELATIVE Cmd"    },
	{ SZ_PAUSE_RESUME,	"CDROM: PAUSE/RESUME Command"	    },
	{ SZ_PLAY_AUDIO_12,	"CDROM: PLAY AUDIO Cmd (12 byte)"   },
	{ SZ_PLAY_TRACK_REL_12,	"CDROM: PLAY TRACK RELATIVE Cmd"    },
	{ SZ_SET_ADDRESS_FORMAT,"CDROM: SET ADDRESS FORMAT Cmd"	    },
	{ SZ_PLAYBACK_STATUS,	"CDROM: PLAYBACK STATUS Command"    },
	{ SZ_PLAY_TRACK,	"CDROM: PLAY TRACK Command"	    },
	{ SZ_PLAY_MSF,		"CDROM: PLAY MSF Command"	    },
	{ SZ_PLAY_VAUDIO,	"CDROM: PLAY AUDIO Command"	    },
	{ SZ_PLAYBACK_CONTROL,	"CDROM: PLAYBACK CONTROL Command"   },
#endif /* mips */
        { NULL,          NULL                                       }
};

struct scsi_dcode scsi_sc_selstat[] = {
	{ SZ_IDLE,       "The device is not selected (BUS Free)"    },
	{ SZ_SELECT,     "The device is selected"                   },
	{ SZ_DISCONN,    "The device has disconnected"              },
	{ SZ_RESELECT,   "The device is in the reselection process" },
	{ SZ_BBWAIT,     "Bus Busy Wait (wait for bus free)"        },
#ifdef vax
	{ SZ_SELTIMO,    "Waiting for select (250 MS timeout)"      },
#endif vax
        { NULL,          NULL                                       }
};

struct scsi_dcode scsi_sc_szflags[] = {
	{ SZ_NORMAL,     "Normal (no szflags set)"                  },
	{ SZ_NEED_SENSE, "Need to do a Request Sense command"       },
	{ SZ_REPOSITION, "Need to reposition the tape	 (NOT USED)"},
	{ SZ_ENCR_ERR,   "Encountered an error"                     },
	{ SZ_DID_DMA,    "A DMA operation has been done"            },
	{ SZ_WAS_DISCON, "Disconnect occured during this command"   },
	{ SZ_NODEVICE,   "No SCSI device present"                   },
        { SZ_BUSYBUS,    "Bus is busy, don't start next command"    },
	{ SZ_RSTIMEOUT,  "Disconnected command to a disk timed out" },
	{ SZ_TIMERON,    "Disconnect being timed by timeout() call" },
	{ SZ_DID_STATUS, "Status phase occurred"                    },
	{ SZ_DMA_DISCON, "DMA was interrupted by a disconnect"      },
	{ SZ_SELWAIT,    "Waiting for 250 MS select timeout"        },
	{ SZ_ASSERT_ATN, "Need to assert the ATN signal"            },
	{ SZ_REJECT_MSG, "Need to reject a message"                 },
	{ SZ_RESET_DEV,  "Need to reset a scsi device"              },
	{ SZ_ABORT_CMD,  "Need to abort the current command"        },
	{ SZ_RETRY_CMD,  "Need to retry the current command"        },
	{ SZ_BUSYTARG,   "Target is busy, retry command later"      },
	{ SZ_RECOVERED,  "A recovered error occured, used w/BBR"    },
        { NULL,          NULL                                       }
};

struct scsi_dcode scsi_request_sense_key[] = {
	{ SZ_NOSENSE,    "Successful cmd or EOT, ILI, FILEMARK"     },
	{ SZ_RECOVERR,   "Successful cmd with controller recovery"  },
	{ SZ_NOTREADY,   "Device present but not ready"             },
	{ SZ_MEDIUMERR,  "Cmd terminated with media flaw"           },
	{ SZ_HARDWARE,   "Controller or drive hardware error"       },
	{ SZ_ILLEGALREQ, "Illegal command or command parameters"    },
	{ SZ_UNITATTEN,  "Unit Attention"                           },
	{ SZ_DATAPROTECT,"Write attempted to write protected media" },
	{ SZ_BLANKCHK,   "Read zero length record (EOD)"            },
	{ SZ_VNDRUNIQUE, "SZ_VNDRUNIQUE"                            },
	{ SZ_COPYABORTD, "SZ_COPYABORTD"                            },
	{ SZ_ABORTEDCMD, "Cmd aborted that may retry successfully"  },
	{ SZ_EQUAL,      "SZ_EQUAL"                                 },
	{ SZ_VOLUMEOVFL, "Buffer data left over after EOM"          },
	{ SZ_MISCOMPARE, "Miscompare on Verify command"             },
	{ SZ_RESERVED,   "SZ_RESERVED"                              },
#ifdef vax
	{ SZ_ASC_RRETRY, "SZ_ASC_RRETRY"                            },
	{ SZ_ASC_RERROR, "SZ_ASC_RERROR"                            },
#endif vax
        { NULL,          NULL                                       }
};

struct scsi_dcode scsi_device_class[] = {
	{ SZ_TAPE,       "TAPE device"                              },
        { SZ_DISK,       "DISK device"                              },
	{ SZ_CDROM,      "CDROM device"                             },
	{ SZ_UNKNOWN,    "UNKNOWN/UNSUPPORTED device"               },
        { TZ30,          "TZ30 cartridge tape"                      },
	{ TZK50,         "TZK50 cartridge tape"                     },
        { TZxx,          "TZxx non-DEC tape (may[not] work)"        },
	{ TZ05,		 "CSS TZ05 tape"			    },
	{ TZ07,		 "CSS TZ07 tape"			    },
	{ TZ9TRK,	 "Generic non-DEC 9trk tape"		    },
	{ TLZ04,	 "TLZ04 (RDAT) tape"			    },
	{ TZRDAT,	 "Generic RDAT tape"			    },
	{ TZK10,	 "TZK10 (QIC) tape"			    },
	{ TZQIC,	 "Generic non-DEC QIC tape"                 },
	{ TZK08,	 "Exabyte TZK08 8mm tape"		    },
	{ TZ8MM,	 "Generic non-DEC 8mm tape"		    },
	{ RZ22,          "RZ22  40 MB winchester disk"              },
	{ RZ23,          "RZ23 100 MB winchester disk"              },
        { RZ55,          "RZ55 300+ MB winchester disk"             },
#ifdef vax
	{ RZ56,          "RZ56 600+ MB winchester disk"             },
#endif vax
	{ RX23,          "RX23 3.5 1.4MB SCSI floppy disk"          },
	{ RX33,          "RX33 5.25 1.2MB SCSI floppy disk"         },
	{ RZxx,          "RZxx non-DEC disk (may[not] work)"        },
	{ RZ24,          "RZ24 winchester disk"                     },
	{ RZ57,          "RZ57 winchester disk"                     },
	{ RZ23L,         "RZ23L 116Mb winchester disk"              },
	{ RX26,          "RX26 3.5 2.8MB SCSI floppy disk"          },
	{ RZ25,          "RZ25 winchester disk"                     },
	{ RRD40,         "RRD40 CDROM (optical disk)"               },
#ifdef vax
	{ CDxx,          "CDxx non-DEC CDROM (may[not] work)"       },
#endif vax
	{ RRD42,         "RRD42 CDROM (optical disk)"               },
        { NULL,          NULL                                       }
};

char *token();
c_scsibus(c)
	char *c;
{
	char *arg;
	int index;
	unsigned int addr;
	
	arg = token();
	
	if (scsi_read_softc() < 0)
		return;
	
	/*
	 * If arg is NULL, print everything.
	 */
	if (arg == NULL) {
		printscsi(do_scsiprint_cntlr);
		return;
	}
	if ((strncmp(arg, "-ta", 3) == 0) ||
	    (strncmp(arg, "ta", 2) == 0)) {
		printscsi(do_scsiprint_targ);
		return;
	}
	if ((strncmp(arg, "-de", 3) == 0) ||
	    (strncmp(arg, "de", 2) == 0)) {
		printscsi(do_scsiprint_devtab);
		return;
	}
	if ((strncmp(arg, "-tr", 3) == 0) ||
	    (strncmp(arg, "tr", 2) == 0)) {
		printscsi(do_scsiprint_trans);
		return;
	}
	if ((strncmp(arg, "-cm", 3) == 0) ||
	    (strncmp(arg, "cm", 2) == 0)) {
		printscsi(do_scsiprint_cmd);
		return;
	}
	if ((strncmp(arg, "-bb", 3) == 0) ||
	    (strncmp(arg, "bb", 2) == 0)) {
		printscsi(do_scsiprint_bbr);
		return;
	}
	if ((strncmp(arg, "-er", 3) == 0) ||
	    (strncmp(arg, "er", 2) == 0)) {
		printscsi(do_scsiprint_err);
		return;
	}
	if ((strncmp(arg, "-si", 3) == 0) ||
	    (strncmp(arg, "si", 2) == 0)) {
		printscsi(do_scsiprint_sii);
		return;
	}
	if ((strncmp(arg, "-dc", 3) == 0) ||
	    (strncmp(arg, "dc", 2) == 0)) {
		printscsi(do_scsiprint_dctstats);
		return;
	}
	if ((strncmp(arg, "-sp", 3) == 0) ||
	    (strncmp(arg, "sp", 2) == 0)) {
		printscsi(do_scsiprint_spinstats);
		return;
	}
	if ((strncmp(arg, "-a", 2) == 0) ||
	    (strncmp(arg, "a", 1) == 0)) {
		printscsi(do_scsiprint_cntlr);
		printscsi(do_scsiprint_targ);
		printscsi(do_scsiprint_devtab);
		printscsi(do_scsiprint_trans);
		printscsi(do_scsiprint_cmd);
		printscsi(do_scsiprint_bbr);
		printscsi(do_scsiprint_err);
		printscsi(do_scsiprint_sii);
		printscsi(do_scsiprint_dctstats);
		printscsi(do_scsiprint_spinstats);
		
		return;
	}
	
	printf("SCSI help, try:\n");
	printf("\t-target\t");
	printf("\t-devtab\t");
	printf("\t-transfer\n");
	printf("\t-cmd\t");
	printf("\t-bbr\t");
	printf("\t-error\n");
	printf("\t-sii\t");
	printf("\t-dctstats\t");
	printf("-spinstats\n");
	printf("\t-all\n");
	
}

int nNSCSIBUS;
struct sz_softc *sz_softc;

/*
 * scsi_read_softc will read in the sz_softc structure for all
 * the scsi controllers.  
 */
int
scsi_read_softc()
{
	static int already_called = 0;
	int s_value;

	if (already_called)
		return(0);

	if (Nscsibus.s_value == 0) {
		printf("\tNULL nNSCSIBUS value\n");
		return(-1);
	}
	if (readmem((char *) &nNSCSIBUS, Nscsibus.s_value, sizeof(int)) !=
	    sizeof(int)) {
		printf("Could not read nNSCSIBUS at 0x%x.\n",Nscsibus.s_value);
		return(-1);
	}

	if (Sz_softc.s_value == 0) {
		printf("\tNULL sz_softc pointer\n");
		return(-1);
	}

	s_value = Sz_softc.s_value;

	sz_softc = (struct sz_softc *) malloc(sizeof(struct sz_softc)*nNSCSIBUS);

	if (readmem(sz_softc, s_value, sizeof(struct sz_softc) * nNSCSIBUS) !=
	    sizeof(struct sz_softc) * nNSCSIBUS) {
		printf("Could not read sz_softc at 0x%x.\n",s_value);
		return(-1);
	}

	already_called = 1;

	return(0);
}

/*
 * Call the passed function for each controller that exists
 */
printscsi(fn)
void (*fn)();
{
	int cntlr;

	for (cntlr=0; cntlr < nNSCSIBUS; cntlr++)
		fn(cntlr);
}

/*
 * Print SCSI controller related information.
 */
void		
do_scsiprint_cntlr(cntrl)
int cntrl;
{
	struct sz_softc *sc = &sz_softc[cntrl];

	printf("\nSCSI controller information:\n");
	printf("SCSI controller #%d:\n",cntrl);

	printf("\tsc_sysid:\t0x%x\n",sc->sc_sysid);
	printf("\tsc_cntlr_alive:\t%d\n",sc->sc_cntlr_alive);
	printf("\tsc_aipfts:\t%d\n",sc->sc_aipfts);
	printf("\tsc_lostarg:\t%d\n",sc->sc_lostarb);
	printf("\tsc_lastid:\t%d\n",sc->sc_lastid);
	printf("\tsc_sc_active:\t%d\n",sc->sc_active);
	printf("\tsc_prevpha:\t%s\n",
	       scsi_decode((long)sc->sc_prevpha, scsi_phase_states,
			   EQUAL_MATCH));
	printf("\tsc_fstate:\t%s\n",
	       scsi_decode((long)sc->sc_fstate, scsi_phase_states,
			   EQUAL_MATCH));
	printf("\tport_start:\t"); praddr(sc->port_start); printf("\n");
	printf("\tport_reset:\t"); praddr(sc->port_reset); printf("\n");
#ifdef vax
	printf("\tsc_swcount:\t%d\n",sc->sc_swcount);
	printf("\tsc_rip:\t\t%d\n",sc->sc_rip);
	printf("\tsc_scs_selena:\t0x%x\n",sc->sc_scs_selena);
#endif vax
	printf("\tsc_progress:\t0x%x\n",sc->sc_progress);
}

/*
 * Print SCSI target related information
 */
void
do_scsiprint_targ(cntrl)
int cntrl;
{
	struct sz_softc *sc = &sz_softc[cntrl];
	char temp[132];
	int i;

	for (i=0; i < NDPS; i++) {

		if (sc->sc_devtyp[i] == 0)
			continue;

		printf("\nSCSI target informaiton:\n");
		printf("Controller #%d Targid #%d:\n", cntrl, i);

		printf("\tsc_alive:\t%d\n",sc->sc_alive[i]);

		printf("\tdevice_comp:\t"); praddr(sc->device_comp[i]);
		printf("\n");

		printf("\tsc_flags:\t%s\n",
		       scsi_decode(sc->sc_flags[i], scsi_stat_devio,
				   OR_MATCH));

		printf("\tsc_cat_flags:\t%s\n",
		       scsi_decode(sc->sc_category_flags[i],
				   scsi_category_stat_devio, OR_MATCH));

		printf("\tsc_devtype:\t%s\n",
		       scsi_decode((long)sc->sc_devtyp[i],
				   scsi_device_class, OR_MATCH));

		printf("\tsc_unit:\t%d\n", sc->sc_unit[i]);
		bzero(temp, sizeof(temp));
		strncpy(temp, sc->sc_device[i], DEV_SIZE);
		printf("\tsc_device:\t%s\n", temp);

		printf("\tsc_szflags:\t%s\n",
		       scsi_decode(sc->sc_szflags[i],
				   scsi_sc_szflags, OR_MATCH));

		printf("\tsc_curcmd:\t%s\n",
		       scsi_decode((long)sc->sc_curcmd[i],
				   scsi_command_opcodes, EQUAL_MATCH));
		printf("\tsc_actcmd:\t%s\n",
		       scsi_decode((long)sc->sc_actcmd[i],
				   scsi_command_opcodes, EQUAL_MATCH));
		printf("\tsc_selstat:\t%s\n",
		       scsi_decode((long)sc->sc_selstat[i],
				   scsi_sc_selstat, EQUAL_MATCH));
		printf("\tsc_xstate:\t%s\n",
		       scsi_decode((long)sc->sc_xstate[i],
				   scsi_statpos_states, EQUAL_MATCH));
		printf("\tsc_xevent:\t%s\n",
		       scsi_decode((long)sc->sc_xevent[i],
				   scsi_statpos_states, EQUAL_MATCH));
		printf("\tsc_pxstate:\t%s\n",
		       scsi_decode((long)sc->sc_pxstate[i],
				   scsi_statpos_states, EQUAL_MATCH));
		printf("\tsc_c_status:\t%s\n",
		       scsi_decode((long)sc->sc_c_status[i],
				   scsi_status_byte, EQUAL_MATCH));
		printf("\tsc_c_snskey:\t%s\n",
		       scsi_decode((long)sc->sc_c_snskey[i],
				   scsi_request_sense_key, EQUAL_MATCH));
		printf("\tsc_status:\t%s\n",
		       scsi_decode((long)sc->sc_status[i],
				   scsi_status_byte, EQUAL_MATCH));
		printf("\tsc_message:\t%s\n",
		       scsi_decode((long)sc->sc_message[i],
				   scsi_mesg_protocols, EQUAL_MATCH));
		bzero(temp, sizeof(temp));
		strncpy(temp, sc->sc_devnam[i], SZ_DNSIZE);
		printf("\tsc_devnam:\t%s\n", temp);
		bzero(temp, sizeof(temp));
		strncpy(temp, sc->sc_revlvl[i], SZ_REV_LEN);
		printf("\tsc_revlvl:\t%s\n", temp);
	}
}

/*
 * Print SCSI devtab information
 */
void
do_scsiprint_devtab(cntlr)
int cntlr;
{
	struct sz_softc *sc = &sz_softc[cntlr];
	struct scsi_devtab devtab;
	char name[132];
	int i;

	for (i=0; i < NDPS; i++) {

		if (sc->sc_devtyp[i] == 0)
			continue;

		bzero(name, sizeof(name));
		bzero(&devtab, sizeof(devtab));

		printf("\nSCSI device tab information:\n");
		printf("Controller #%d Targid #%d:\n", cntlr, i);
		if (readmem((char *)&devtab, (int) sc->sc_devtab[i],
			sizeof(devtab)) != sizeof(devtab))
		{
			printf("Unable to read in the devtab.\n");
			continue;
		}
		if (readmem(name, (int) devtab.name, devtab.namelen) !=
		    devtab.namelen)
		{
			printf("Unable to read in devtab name.\n");
			continue;
		}

		printf("\tnamelen:\t%d\n",devtab.namelen);
		printf("\tname:\t\t%s\n",name);
		printf("\tdevtype:\t%s\n",
		       scsi_decode(devtab.devtype,
				   scsi_device_class, EQUAL_MATCH));
		printf("\tpart table:\t"); praddr(devtab.disksize);
		printf("\n");
		printf("\tprobedelay:\t0x%x\n",devtab.probedelay);
#ifdef vax
		printf("\tmspw:\t\t0x%x\n",devtab.mspw);
#endif vax

		if (devtab.flags == 0)
			printf("\tflags:\t\tNone\n");
		else
			printf("\tflags:\t\t%s\n",
			       scsi_decode(devtab.flags,
					   scsi_device_flags, OR_MATCH));
	}
}

/*
 * Print SCSI command/message information
 */
void
do_scsiprint_cmd(cntlr)
int cntlr;
{
	struct sz_softc *sc = &sz_softc[cntlr];
	int i, j;

	for (i=0; i < NDPS; i++) {

		if (sc->sc_devtyp[i] == 0)
			continue;

		printf("\nSCSI message and command information:\n");
		printf("Controller #%d Targid #%d:\n", cntlr, i);

		printf("\textmessg:\t0x%x 0x%x 0x%x 0x%x 0x%x\n",
		       sc->sc_extmessg[i][0],sc->sc_extmessg[i][1],
		       sc->sc_extmessg[i][2],sc->sc_extmessg[i][3],
		       sc->sc_extmessg[i][4]);

		printf("\tsc_cmdlog:\t");
		for (j=0; j < 12; j++)
			printf("0x%x ",sc->sc_cmdlog[i][j]);
		printf("\n");

		printf("\tExtended sense data:\n");
		printf("\t\terror code:\t\t0x%x\n",sc->sc_sns[i].errcode);
		printf("\t\terror class:\t\t0x%x\n",sc->sc_sns[i].errclass);
		printf("\t\tvalid:\t\t\t0x%x\n",sc->sc_sns[i].valid);
		printf("\t\tsegment number:\t\t0x%x\n",sc->sc_sns[i].segnum);
		printf("\t\tsense key:\t\t0x%x\n",sc->sc_sns[i].snskey);
		printf("\t\tillegal length indicator:0x%x\n",
		       sc->sc_sns[i].ili);
		printf("\t\tend of medium:\t\t0x%x\n",sc->sc_sns[i].eom);
		printf("\t\tfilemark:\t\t0x%x\n",sc->sc_sns[i].filmrk);
		printf("\t\tinformation byte (msb):\t0x%x\n",
		       sc->sc_sns[i].infobyte3);
		printf("\t\tinformation byte:\t0x%x\n",
		       sc->sc_sns[i].infobyte2);
		printf("\t\tinformation byte:\t0x%x\n",
		       sc->sc_sns[i].infobyte1);
		printf("\t\tinformation byte (lsb):\t0x%x\n",
		       sc->sc_sns[i].infobyte0);
		printf("\t\tadditional sense length:0x%x\n",
		       sc->sc_sns[i].asl);
		if ((sc->sc_devtyp[i] & SZ_TAPE) == SZ_TAPE) {
			printf("\t\tTAPE specific types:\n");
			printf("\t\t\tcont err code:\t0x%x\n",
			       sc->sc_sns[i].asb.tz_asb.ctlr);
			printf("\t\t\tdrive error #0:\t0x%x\n",
			       sc->sc_sns[i].asb.tz_asb.drv0);
			printf("\t\t\tdrive error #1:\t0x%x\n",
			       sc->sc_sns[i].asb.tz_asb.drv1);
		}
		if ((sc->sc_devtyp[i] & SZ_DISK) == SZ_DISK) {
			printf("\t\tDISK specific bytes:\n");
			printf("\t\t\taddit sense code:\t0x%x\n",
			       sc->sc_sns[i].asb.rz_asb.asc);
		}
		if ((sc->sc_devtyp[i] & SZ_CDROM) == SZ_CDROM) {
			printf("\t\tCDROM specific bytes:\n");
			printf("\t\t\taddit sense code:\t0x%x\n",
			       sc->sc_sns[i].asb.cd_asb.asc);
			printf("\t\t\tfrufld:\t\t\t0x%x\n",
			       sc->sc_sns[i].asb.cd_asb.frufld);
			printf("\t\t\tbit pointer:\t\t0x%x\n",
			       sc->sc_sns[i].asb.cd_asb.bitp);
			printf("\t\t\tbit pointer valid:\t0x%x\n",
			       sc->sc_sns[i].asb.cd_asb.bpv);
			printf("\t\t\tvendor unique:\t\t0x%x\n",
			       sc->sc_sns[i].asb.cd_asb.vu);
			printf("\t\t\tcommand/data:\t\t0x%x\n",
			       sc->sc_sns[i].asb.cd_asb.cd);
			printf("\t\t\tfield pointer valid:\t0x%x\n",
			       sc->sc_sns[i].asb.cd_asb.fpv);
			printf("\t\t\tfield pointer (msb):\t0x%x\n",
			       sc->sc_sns[i].asb.cd_asb.fpmsb);
			printf("\t\t\tfield pointer (lsb):\t0x%x\n",
			       sc->sc_sns[i].asb.cd_asb.fplsb);
		}

		printf("\tsc_cmdpkt:\n");
		printf("\t\tcmd:\t");
		for(j=0; j<6; j++)
			printf("0x%x ",sc->sc_cmdpkt[i].altcmd.cmd[j] & 0xff);
		printf("\n\t\tdata:\t");
		for(j=0; j<8; j++)
			printf("0x%x ",sc->sc_cmdpkt[i].altcmd.dat[j] & 0xff);
		printf("\n\t\t\t");
		for(j=8; j<16; j++)
			printf("0x%x ",sc->sc_cmdpkt[i].altcmd.dat[j] & 0xff);
		printf("\n");

		printf("\tsc_status:\t%s\n",
		       scsi_decode(sc->sc_status[i],
				   scsi_status_byte, EQUAL_MATCH));
	}
}

/*
 * Print Bad Block Replacement data
 */
void
do_scsiprint_bbr(cntlr)
int cntlr;
{
	struct sz_softc *sc = &sz_softc[cntlr];
	int i;

	for (i=0; i < NDPS; i++) {

		if (sc->sc_devtyp[i] == 0)
			continue;

		if ((sc->sc_devtyp[i] & SZ_DISK) != SZ_DISK)
			continue;

		printf("\nSCSI Bad Block Replacement information:\n");
		printf("Controller #%d Targid #%d:\n", cntlr, i);
		printf("\tsc_bbr_active:\t0x%x\n",sc->sc_bbr_active[i]);
		printf("\tsc_bbr_state:\t0x%x\n",sc->sc_bbr_state[i]);
		printf("\tsc_bbr_oper:\t0x%x\n",sc->sc_bbr_oper[i]);
		printf("\tsc_bbr_read:\t0x%x\n",sc->sc_bbr_read[i]);
		printf("\tsc_bbr_rawr:\t0x%x\n",sc->sc_bbr_rawr[i]);
		printf("\tsc_bbr_write:\t0x%x\n",sc->sc_bbr_write[i]);
		printf("\tsc_bbraddr:\t0x%x\n",sc->sc_bbraddr[i]);
		printf("\tsc_bbrparams:\t0x%x\n",sc->sc_bbrparams[i]);
	}
}

/*
 * Print SCSI error related information
 */
void
do_scsiprint_err(cntlr)
int cntlr;
{
	struct sz_softc *sc = &sz_softc[cntlr];
	int i;

	for (i=0; i < NDPS; i++) {

		if (sc->sc_devtyp[i] == 0)
			continue;

		printf("\nSCSI Error information:\n");
		printf("Controller #%d Targid #%d:\n", cntlr, i);


#ifdef vax
		printf("\tsc_statlog:\t0x%x\n",sc->sc_statlog[i]);
		printf("\tsc_selretry:\t0x%x\n",sc->sc_sel_retry[i]);
#endif vax
		printf("\tsc_softcnt:\t0x%x\n",sc->sc_softcnt[i]);
		printf("\tsc_hardcnt:\t0x%x\n",sc->sc_hardcnt[i]);
	}
}

/*
 * Print SII information
 */
void
do_scsiprint_sii(cntlr)
int cntlr;
{
	struct sz_softc *sc = &sz_softc[cntlr];
	int i;

	for (i=0; i < NDPS; i++) {

		if (sc->sc_devtyp[i] == 0)
			continue;

		printf("\nSCSI SII information:\n");
		printf("Controller #%d Targid #%d:\n", cntlr, i);

		printf("\tsc_siioddbyte:\t0x%x\n",sc->sc_siioddbyte[i]);
		printf("\tsc_siireqack:\t0x%x\n",sc->sc_siireqack[i]);
		printf("\tsc_siisentsync:\t0x%x\n",sc->sc_siisentsync[i]);
		printf("\tsc_siidmacount:\t0x%x\n",sc->sc_siidmacount[i]);
	}
}

/*
 * Print SPIN_STATS information
 */
void
do_scsiprint_spinstats(cntlr)
int cntlr;
{
#ifdef	SPIN_STATS
	struct sz_softc *sc = &sz_softc[cntlr];
	int i;

	for (i=0; i < NDPS; i++) {

		if (sc->sc_devtyp[i] == 0)
			continue;

		printf("\nSCSI Spin Stat information:\n");
		printf("Controller #%d Targid #%d:\n", cntlr, i);

		printf("\tsc_i_spin1:\t0x%x\n",sc->sc_i_spin1[i]);
		printf("\tsc_i_spcmd:\t0x%x\n",sc->sc_i_spcmd[i]);
		printf("\tsc_i_phase:\t0x%x\n",sc->sc_i_phase[i]);
		printf("\tsc_ss_spin1:\t0x%x\n",sc->sc_ss_spin1[i]);
		printf("\tsc_ss_spcmd:\t0x%x\n",sc->sc_ss_spcmd[i]);
		printf("\tsc_ss_phase:\t0x%x\n",sc->sc_ss_phase[i]);
	}

#else SPIN_STATS

	printf("\nController #%d:  SPIN_STATS not defined\n", cntlr);

#endif SPIN_STATS
}

/*
 * Print DCT_STATS information.
 */
void
do_scsiprint_dctstats(cntlr)
int cntlr;
{
#ifdef	DCT_STATS
	struct sz_softc *sc = &sz_softc[cntlr];
	int i;

	for (i=0; i < NDPS; i++) {

		if (sc->sc_devtyp[i] == 0)
			continue;

		printf("\nSCSI DCT Stat information:\n");
		printf("Controller #%d Targid #%d:\n", cntlr, i);

		printf("\tsc_dcstart:\t0x%x\n",sc->sc_dcstart[i]);
		printf("\tsc_dcend:\t0x%x\n",sc->sc_dcend[i]);
		printf("\tsc_dcdiff:\t0x%x\n",sc->sc_dcdiff[i]);
		printf("\tsc_dclongest:\t0x%x\n",sc->sc_dclongest[i]);

	}

#else DCT_STATS

	printf("\nController #%d:  DCT_STATS not defined\n", cntlr);

#endif DCT_STATS
}

/*
 * Print data transfer information
 */
void
do_scsiprint_trans(cntlr)
int cntlr;
{
	struct sz_softc *sc = &sz_softc[cntlr];
	int i;

	for (i=0; i < NDPS; i++) {

		if (sc->sc_devtyp[i] == 0)
			continue;

		printf("\nSCSI data transfer information:\n");
		printf("Controller #%d Targid #%d:\n", cntlr, i);

		printf("\tsc_b_bcount:\t0x%x\n",sc->sc_b_bcount[i]);
		printf("\tsc_bpcount:\t0x%x\n",sc->sc_bpcount[i]);
		printf("\tsc_segcnt:\t0x%x\n",sc->sc_segcnt[i]);
		printf("\tsc_xfercnt:\t0x%x\n",sc->sc_xfercnt[i]);
		printf("\tsc_resid:\t0x%x\n",sc->sc_resid[i]);
		printf("\tsc_savcnt:\t0x%x\n",sc->sc_savcnt[i]);
	}
}

/*
 * Return a description of the given code.
 */
char *
scsi_decode(code, dc, type)
long code;
struct scsi_dcode *dc;
int type;
{
	int i;
	char desc[256];

	bzero(desc, sizeof(desc));

	for (; dc->desc != NULL; *dc++) {
		if (((type == EQUAL_MATCH) && (dc->code == code)) ||
		    ((type == OR_MATCH) && ((dc->code | code) == code)))
		{
			if (strlen(desc) > 0)
				strcat(desc, ",");
			strcat(desc, dc->desc);

			if (type == EQUAL_MATCH)
				break;

			code -= dc->code;
		}
	}

	if (strlen(desc) == 0)
		sprintf(desc, "0x%x", code);

	return (desc);
}

	
