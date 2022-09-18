!
! MASSBUS DISK COMMAND PROCEDURE
!
! THE MASSBUS TR LEVEL MUST BE DEPOSITED IN R1 AND THE
! UNIT NUMBER MUST BE DEPOSITED IN R3 BEFORE EXECUTING THIS PROCEDURE
!
!
SET SNAP ON		! Enable ERROR_HALT snapshots
SET FBOX OFF		! ULTRIX will turn on Fbox
INIT			! SRM processor init
UNJAM			! UNJAM SBIAs, Enable Master SBI interrupts
INIT/PAMM		! 
DEPOSIT CSWP 8		! Turn off the cache (ULTRIX turns the cache on)
!
DEPOSIT R0 0		! Device Type is Massbus Disk
!R1 MUST BE PRESET TO THE DESIRED MBA TR
DEPOSIT R2 0		! Controller number
!R3 MUST BE PRESET TO THE DESIRED UNIT NUMBER
DEPOSIT R4 0		! Logical block number to boot from if R5 bit 3 is set
DEPOSIT R5 1000B	! BOOT ULTRIX TO SINGLE USER AND PROMPT FOR IMAGENAME
DEPOSIT SP 200		! Set the stack pointer
LOAD/START:200 VMB	! Load VMB 200 bytes above the start of the good block
START 200 		! Start VMB at the load address
