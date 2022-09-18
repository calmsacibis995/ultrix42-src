!
! Version: 001.000
!
!	CI PORT BOOT COMMAND FILE - CIRA.COM
!
! Operating System Disk:	CI DEVICE
!
!	R1 gets SBIA #0; TR number of the CI780
!	R2 gets HSC port number (in HEX)
!	R3 gets Unit number to boot from (in HEX)
!
SET SNAP ON		! Enable ERROR_HALT snapshots
SET FBOX OFF		! ULTRIX will turn on Fbox
INIT			! SRM processor init
UNJAM			! UNJAM SBIAs, Enable Master SBI interrupts
INIT/PAMM		! 
DEPOSIT CSWP 8		! Turn off the cache (ULTRIX turns the cache on)
!
DEPOSIT R0 20		! Device Type is CI780
DEPOSIT R4 0		! Logical block number to boot from if R5 bit 3 is set
DEPOSIT R5 1000B	! BOOT ULTRIX TO SINGLE USER AND PROMPT FOR IMAGENAME
DEPOSIT SP 200		! Set the stack pointer
LOAD/START:200 VMB	! Load VMB 200 bytes above the start of the good block
START 200 		! Start VMB at the load address
