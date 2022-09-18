/*	@(#)scsi_dcode.h	4.1	(ULTRIX)	7/17/90	*/

/************************************************************************
 *									*
 *			Copyright (c) 1989 by				*
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
 *   This software is  derived  from  software  received  from  the	*
 *   University    of   California,   Berkeley,   and   from   Bell	*
 *   Laboratories.  Use, duplication, or disclosure is  subject  to	*
 *   restrictions  under  license  agreements  with  University  of	*
 *   California and with AT&T.						*
 *									*
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or  reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/

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
#ifdef FORMAT
	{ SZ_FORMAT,     "DISK: FORMAT UNIT Command"                },
	{ SZ_REASSIGN,   "DISK: REASSIGN BLOCK Command"             },
	{ SZ_VFY_DATA,   "DISK: VERIFY DATA Command"                },
	{ SZ_RDD,        "DISK: READ DEFECT DATA Command"           },
#ifdef vax
	{ SZ_READL,      "DISK: READ LONG Command"                  },
	{ SZ_WRITEL,     "DISK: WRITE LONG Command"                 },
#endif vax
	{ RZSPECIAL,     "DISK: set for special disk commands"      },
#endif FORMAT
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
	{ RZ22,          "RZ22  40 MB winchester disk"              },
	{ RZ23,          "RZ23 100 MB winchester disk"              },
        { RZ55,          "RZ55 300+ MB winchester disk"             },
#ifdef vax
	{ RZ56,          "RZ56 600+ MB winchester disk"             },
#endif vax
	{ RX23,          "RX23 3.5 1.4MB SCSI floppy disk"          },
	{ RX33,          "RX33 5.25 1.2MB SCSI floppy disk"         },
	{ RZxx,          "RZxx non-DEC disk (may[not] work)"        },
	{ RRD40,         "RRD40 CDROM (optical disk)"               },
#ifdef vax
	{ CDxx,          "CDxx non-DEC CDROM (may[not] work)"       },
#endif vax
        { NULL,          NULL                                       }
};

