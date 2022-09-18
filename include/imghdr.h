/*	@(#)imghdr.h	1.1	Ultrix	7/2/86 */
/************************************************************************/
/*									*/
/*	imghdr.h - Ultrix32 image file format 				*/
/*									*/
/*	Description: This header file describes the image file format   */
/*	used in the ULTRIX system product. It was derived from the 	*/
/*	combination of a need for performance and the DEC standard image*/
/*	file format, DSIFF. 						*/
/*									*/
/*	The file is divided into three distinct sections.     		*/
/*		1. The file header containing image reference data      */
/*		2. Spectral map(if any)					*/
/*		3. Image spectral bands					*/
/*									*/
/*	The three sections are organized as follows:			*/
/*									*/
/*			   IMAGE FILETYPE FLAG				*/
/*			_______________________				*/
/*			   FILE HEADER					*/
/*			-----------------------				*/
/*			   SPECTRAL MAP				        */
/*			-----------------------				*/
/*			   SPECTRAL BAND GROUPS				*/
/*			-----------------------				*/
/*			   SPECTRAL BAND DATA				*/
/*			-----------------------				*/
/*			   SPECTRAL BAND DATA				*/
/*			-----------------------				*/
/*			   SPECTRAL BAND DATA				*/
/*			-----------------------				*/
/*				  EOF  					*/
/*									*/
/*	NOTE:   1. ALL SECTION LENGTHS ARE IN 512BYTE sector units	*/
/*		2. PADDING IS ZERO FILL					*/
/*		3. DIGITAL BYTE AND BIT ORDER				*/
/*									*/
/*	NOTE: the inform text item exactly fills out the header to one  */
/*		sector(512bytes). This should be maintained by keeping  */
/*		the parameter FLDUSED=to the nuber of used header fields*/
/*									*/
/************************************************************************/

/* format default image definitions */
/************************************************************************/
#define MAXFLDS 50	/* maximum number of header fields before info  */
#define INFSIZ  (512-(sizeof(int)*MAXFLDS))
#define FLDUSED 21	/* number of header fields inuse		*/
#define IMGFLG  1234567 /* image filetype flag				*/
/************************************************************************/
/*************     IMAGE TYPES	*****************************************/
#define	NONE	0	/* type unknown					*/
#define	BITONE	1	/* Bitonal					*/
#define	CONTONE 2	/* Continuous Tone (gray)			*/
#define RGB	3	/* RGB						*/
#define	YIQ	4	/* Luminance (TV Broadcasting Application)	*/
#define	HSV	5	/* Hue saturation value				*/
#define LANDSAT 6 	/* LANDSAT					*/
#define QDSS	RGB	/* multiple plane local format 			*/
#define	QVSS	BITONE	/* single plane local format			*/
#define LOCAL	QDSS	/* local image format for Ultrix 		*/
/************************************************************************/
/*	gridtype definitions */
/************************************************************************/
#define	SQR	0	/* square grid					*/
#define HEXE    1	/* hexangle with even line indentation		*/
#define HEXO	2	/* hexangle with odd line indentation		*/
/************************************************************************/
/*	image encoding schemes	*/
/************************************************************************/
#define	PCM	0	/* no compaction				*/
#define	CCITT_1 1	/* CCITT encoded uncompresed (T.4)		*/
#define CCITT_2 2	/* CCITT one dimensional Modified huffman	*/
#define CCITT_3 3	/* CCITT two dimensional Modified huffman	*/
#define MODMOD 	4	/* Modified Modified Read			*/
/* 5-10 reserved for bi-tonal variations				*/
/* 11-20 reserved for continuous tone variations			*/
/* 21-30 reserved for multi-spectral variations				*/
/* 31-40 reserved for time-varying					*/
/* 41-60 reserved for DEC						*/
/************************************************************************/
/*	default header length	*/
/************************************************************************/
#define	HEDLEN	1	/* default header length			*/
/************************************************************************/
/*	default color map parameters */
/************************************************************************/
#define CMPLEN	3	/* default color map length in sectors		*/
#define CMPENM	256	/* number of colormap entries			*/
#define	CMPSNM	3	/* three R,G,B sections to each color map entry */
#define	CMSCSZ 	16	/* bits/color map section			*/
/************************************************************************/
/* 	default spectral band parameters				*/
/*	for full screen workstation images 				*/
/* 	images that are not full screen require different parameters    */
/************************************************************************/
#define	SPBNUM	1	/* number of spectral bands			*/
#define	SPBGNM	1	/* number of spectral band groups		*/
#define	SPBYNM  864	/* number of scan lines/image			*/
#define	SPBXNM	1024	/* number of pixels/scanline			*/
#define SPBZNM  8	/* number of bits/pixel				*/
#define SPBLEN 	1728	/* length of spectral band data:512byte sectors	*/
/************************************************************************/
/*	original image orientation	*/
/*	with respect to pixel ordering  */
/************************************************************************/
#define	LFTORG	0	/* left to right	*/
#define BOTTOP  1	/* bottom to top	*/
#define RGTOLF  2	/* right to left	*/
#define TOPBOT  3	/* top to bottom	*/
/************************************************************************/
/*	pixel resolution in fractions of an inch	*/
/************************************************************************/
#define	IMXRES  78	/*X pixel resolution interpreted as 1/IMXRES    */
#define	IMYRES	78	/*Y pixel resolution interpreted as 1/IMYRES 	*/
/************************************************************************/
/*	Interscene time	(for multiple spectral groups (motion picture)) */
/*		interpreted as micro seconds				*/
/*		timgap = 1000000 => 1 sec duration between image groups	*/
/************************************************************************/
#define NOTIME	0	/* notime gap for single spectral group image	*/
/************************************************************************/

/******** image header structure	**********/
/* NOTE: MAXFLDS = FLDUSED+reserve fields        */
struct	imghdr	{
		int	imgflg;	/* image filetype flag= IMGFLG		*/
				/*DSIFF-ImageHeader			*/
		int	format;	/* image format (fax, workstation, RGB) */
				/*DSIFF-SpectralBandMappingScheme	*/
		int	grdtyp; /* grid type				*/
				/*DSIFF-PixelPathAngle and		*/
				/*	ScanLineProgressionAngle	*/
		int	coding; /* image encoding scheme 		*/
				/*DSIFF-EncodingSchemeType		*/
		int	hedlen; /* header length in sectors		*/
				/*DSIFF- not used			*/
		int	cmplen; /* colormap length in sectors		*/
				/*DSIFF-SpectralBandLookupTable data    */
		int	cmpenm; /* number of color map entries		*/
				/*DSIFF-SpectralBandLookupTable data    */
		int	cmpsnm; /* number of color map sections/entry   */
				/*DSIFF-SpectralBandLookupTable data    */
		int	cmscsz; /* size of each colormap section (bits)	*/	
				/*DSIFF-SpectralBandLookupTable data    */
		int	spbgnm; /* length of spectral band groups	*/
				/*DSIFF- not used			*/
		int	spblen; /* length of spectral band data		*/
				/*DSIFF- not used			*/
		int	spbnum; /* number spectral bands		*/
				/*DSIFF-SpectralSpaceDimension		*/
		int	spbynm; /* number of scan lines/band in pixels	*/
				/*DSIFF-ScanLinesPerImage		*/
		int	spbxnm; /* width of each band in pixels		*/
				/*DSIFF-PixelsPerScanLine		*/
		int	spbznm; /* number of bits/pixel			*/
				/*DSIFF-BitsPerPixel			*/
		int	orient;	/* image pixel ordering			*/
				/*DSIFF-PixelPathAngle			*/
		int	imxres; /* separation of adjacent pixels	*/
				/*DSIFF-IntraScanLinePixelCenterDistance*/
		int	imyres; /* separation of pixel scan lines	*/
				/*DSIFF-InterScanLinePixelCenterDistance*/
		int	imxpos; /* relative x position in larger grid	*/
				/*DSIFF- not used			*/
		int	imypos; /* relative y position in larger grid	*/
				/*DSIFF- not used			*/
		int	timgap; /* interscene time duration		*/
				/*DSIFF- INTERSCENE_TIME		*/
		int	reserve[MAXFLDS-FLDUSED]; /* reserve fields		*/
		char	inform[INFSIZ];/* image information string (ASCII)	*/
				/*DSIFF-AsciiDescriptor			*/
		};
/*	default color map 	*/
struct	colmap	{
		unsigned short	color[256][3];
		};
