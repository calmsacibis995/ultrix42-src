!
!	CONSOLE BOOT COMMAND FILE - CNSL.COM
!
! Operating System Disk:	CONSOLE BLOCK STORAGE DEVICE
!
!
SET SNAP ON		! Enable ERROR_HALT snapshots
SET FBOX OFF		! ULTRIX will turn on Fbox
INIT			! SRM processor init
UNJAM			! UNJAM SBIAs, Enable Master SBI interrupts
DEPOSIT CSWP 8		! Turn off the cache (ULTRIX turns the cache on)
!
DEPOSIT R0 40		! Device Type is Console Block Storage Device
DEPOSIT R1 0		! Unused
DEPOSIT R2 0		! Unused
DEPOSIT R3 0		! Unused
DEPOSIT R4 0		! Unused
DEPOSIT R5 1000B	! BOOT ULTRIX TO SINGLE USER AND PROMPT FOR IMAGENAME
DEPOSIT SP 200		! Set the stack pointer
LOAD/START:200 VMB	! Load VMB 200 bytes above the start of the good block
START 200 		! Start VMB at the load address
