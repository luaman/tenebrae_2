//___________________________________________________________________________________________________________nFO
// "tiff-export.c" - tiff export [will be saved to an uncompressed tiff].
//
// NOTE: was hacked together after analyzing some tiff files with HexEdit and Tiffy.
//
// Written by:	Axel "awe" Wefers		[mailto:awe@fruitz-of-dojo.de].
//
//
// Part of the "Hagane Engine v2.0".
//
// Copyright (c) 2001-2002 Fruitz Of Dojo. All rights reserved.
//______________________________________________________________________________________________________iNCLUDES

#pragma mark =Includes=

#include "quakedef.h"

#pragma mark -

//_________________________________________________________________________________________________________eNUMS

#pragma mark =Enums=

enum {
         TIF_BYTE = 1,
         TIF_ASCII,
         TIF_SHORT,
         TIF_LONG,
         TIF_RATIONAL
     };

#pragma mark -

//_______________________________________________________________________________________________________dEFINES

#pragma mark =Defines=

#ifdef __BIG_ENDIAN__

#define TIF_ENDIANESS                	{ 0x4D, 0x4D, 0x00, 0x2A }

#else

#define TIF_ENDIANESS    		{ 0x49, 0x49, 0x2A, 0x00 }

#endif /* __BIG_ENDIAN__ */

#define TIF_IFD_OFFSET           	0x0000001E
#define TIF_NUM_TAGS                 	0x000E
#define TIF_SUBFILE_TYPE            	0x00000000
#define TIF_BITS_PER_SAMPLE          	{ 8, 8, 8}
#define TIF_SAMPLES_PER_PIXEL       	3
#define TIF_SAMPLE_OFFSET            	0x0018
#define TIF_COMPRESSION              	0x0001
#define TIF_PHOTOMETRIC              	0x0002
#define TIF_STRIP_OFFSETS            	0x000000CC
#define TIF_RESOLUTION_X_OFFSET      	8
#define TIF_RESOLUTION_Y_OFFSET      	16
#define TIF_RESOLUTION_X             	{ 72, 1 }
#define TIF_RESOLUTION_Y             	{ 72, 1 }
#define TIF_PLANAR_CONFIGURATION     	1
#define TIF_RESOLUTION_UNIT          	2
#define TIF_BLANK_TAG                	{   			     \
                                            0x00, 0x00, 0x00, 0x00,  \
                                            0x00, 0x00, 0x00, 0x00,  \
                                            0x00, 0x00, 0x00, 0x00   \
                                        } 

#define TIF_TAG_SUBFILE_TYPE         	0x00FE
#define TIF_TAG_IMAGE_WIDTH          	0x0100
#define TIF_TAG_IMAGE_HEIGHT         	0x0101
#define TIF_TAG_BITS_PER_SAMPLE      	0x0102
#define TIF_TAG_COMPRESSION          	0x0103
#define TIF_TAG_PHOTOMETRIC          	0x0106
#define TIF_TAG_STRIP_OFFSETS        	0x0111
#define TIF_TAG_SAMPLES_PER_PIXEL    	0x0115
#define TIF_TAG_ROWS_PER_STRIP       	0x0116
#define TIF_TAG_STRIP_BYTE_COUNTS    	0x0117
#define TIF_TAG_RESOLUTION_X         	0x011A
#define TIF_TAG_RESOLUTION_Y         	0x011B
#define TIF_TAG_PLANAR_CONFIGURATION 	0x011C
#define TIF_TAG_RESOLUTION_UNIT      	0x0128

#pragma mark -

//_____________________________________________________________________________________________________cONSTANTS

#pragma mark =Constants=

const unsigned char	cTifEndianHeader[4]  = TIF_ENDIANESS,
                        cTifBlankTag[12]     = TIF_BLANK_TAG;
const unsigned short	cTifBitsPerSample[3] = TIF_BITS_PER_SAMPLE,
                        cTifNumTags          = TIF_NUM_TAGS,
                        cTifCompression      = TIF_COMPRESSION,
                        cTifPhotometric      = TIF_PHOTOMETRIC,
                        cTifSamplesPerPixel  = TIF_SAMPLES_PER_PIXEL,
                        cTifPlanarConfig     = TIF_PLANAR_CONFIGURATION,
                        cTifResolutionUnit   = TIF_RESOLUTION_UNIT;
const unsigned int	cTifIfdOffset        = TIF_IFD_OFFSET,
                        cTifResolutionX[2]   = TIF_RESOLUTION_X,
                        cTifResolutionY[2]   = TIF_RESOLUTION_Y,
                        cTifSampleOffset     = TIF_SAMPLE_OFFSET,
                        cTifSubfileType      = TIF_SUBFILE_TYPE,
                        cTifStripOffsets     = TIF_STRIP_OFFSETS,
                        cTifResXOffset       = TIF_RESOLUTION_X_OFFSET,
                        cTifResYOffset       = TIF_RESOLUTION_Y_OFFSET;

#pragma mark -

//___________________________________________________________________________________________fUNCTION_pROTOTYPES

#pragma mark =Function Prototypes=

extern char *	Sys_sprintf (char *theFormat, ...);

void 		TIF_WriteFile (char *theFileName, unsigned int theWidth, unsigned int theHeight,
                               unsigned char *theData);

static void	TIF_WriteTag (int theFile, unsigned short theTagName, unsigned short theTagSize,
                              unsigned int theTagLength, const void (*theTagData));

#pragma mark -

//________________________________________________________________________________________________TIF_WriteTag()

void	TIF_WriteTag (int theFile, unsigned short theTagName, unsigned short theTagSize,
                      unsigned int theTagLength, const void (*theTagData))
{
    unsigned char	mySize = 0;

    Sys_FileWrite (theFile, &theTagName,   sizeof (short));
    Sys_FileWrite (theFile, &theTagSize,   sizeof (short));
    Sys_FileWrite (theFile, &theTagLength, sizeof (int));

    switch (theTagSize)
    {
        case TIF_BYTE:
        case TIF_ASCII:
	    mySize = sizeof (char);
            break;
        case TIF_SHORT:
	    mySize = sizeof (short);
	    break;
        case TIF_LONG:
	    mySize = sizeof (int);
            break;
        case TIF_RATIONAL:
	    mySize = sizeof (float);
	    break;
    }

    if (theTagName == TIF_TAG_BITS_PER_SAMPLE)
    {
        mySize = sizeof (int);
    }
    
    Sys_FileWrite (theFile, (void *) theTagData, mySize);
    Sys_FileWrite (theFile, (void *) cTifBlankTag, 12 - (sizeof (short) << 1) - (sizeof (int)) - mySize);
}

//_______________________________________________________________________________________________TIF_WriteFile()

void TIF_WriteFile (char *theFileName, unsigned int theWidth, unsigned int theHeight, unsigned char *theData)
{
    char		*myFileName = NULL;
    int			myFile;
    unsigned int        mySize = theWidth * theHeight * cTifSamplesPerPixel;

    // get the file to write to:
    myFileName = Sys_sprintf ("%s/%s", com_gamedir, theFileName);
    myFile = Sys_FileOpenWrite (myFileName);
    
    if (myFile == -1)
    {
        Sys_Printf ("TIF_WriteFile: failed on \"%s\"\n", myFileName);
        return;
    }
    
    Sys_Printf ("TIF_WriteFile: %s\n", myFileName);
    
    // write the header data:
    Sys_FileWrite (myFile, (void *) cTifEndianHeader, 	sizeof (char) << 2);
    Sys_FileWrite (myFile, (void *) &cTifIfdOffset, 	sizeof (int));
    Sys_FileWrite (myFile, (void *) cTifResolutionX, 	sizeof (int) << 1);
    Sys_FileWrite (myFile, (void *) cTifResolutionY, 	sizeof (int) << 1);
    Sys_FileWrite (myFile, (void *) cTifBitsPerSample, 	sizeof (short) * 3);
    Sys_FileWrite (myFile, (void *) &cTifNumTags, 	sizeof (short));
    
    TIF_WriteTag (myFile, TIF_TAG_SUBFILE_TYPE, 	TIF_LONG, 	1, &cTifSubfileType);
    TIF_WriteTag (myFile, TIF_TAG_IMAGE_WIDTH, 		TIF_LONG, 	1, &theWidth);
    TIF_WriteTag (myFile, TIF_TAG_IMAGE_HEIGHT, 	TIF_LONG, 	1, &theHeight);
    TIF_WriteTag (myFile, TIF_TAG_BITS_PER_SAMPLE, 	TIF_SHORT, 	3, &cTifSampleOffset);
    TIF_WriteTag (myFile, TIF_TAG_COMPRESSION, 		TIF_SHORT, 	1, &cTifCompression);
    TIF_WriteTag (myFile, TIF_TAG_PHOTOMETRIC, 		TIF_SHORT, 	1, &cTifPhotometric);
    TIF_WriteTag (myFile, TIF_TAG_STRIP_OFFSETS, 	TIF_LONG, 	1, &cTifStripOffsets);
    TIF_WriteTag (myFile, TIF_TAG_SAMPLES_PER_PIXEL, 	TIF_SHORT, 	1, &cTifSamplesPerPixel);
    TIF_WriteTag (myFile, TIF_TAG_ROWS_PER_STRIP, 	TIF_LONG, 	1, &theWidth);
    TIF_WriteTag (myFile, TIF_TAG_STRIP_BYTE_COUNTS, 	TIF_LONG, 	1, &mySize);
    TIF_WriteTag (myFile, TIF_TAG_RESOLUTION_X, 	TIF_RATIONAL,	1, &cTifResXOffset);
    TIF_WriteTag (myFile, TIF_TAG_RESOLUTION_Y, 	TIF_RATIONAL, 	1, &cTifResYOffset); 
    TIF_WriteTag (myFile, TIF_TAG_PLANAR_CONFIGURATION, TIF_SHORT, 	1, &cTifPlanarConfig);
    TIF_WriteTag (myFile, TIF_TAG_RESOLUTION_UNIT, 	TIF_SHORT, 	1, &cTifResolutionUnit);

    Sys_FileWrite (myFile, (void *) cTifBlankTag, sizeof (char) << 2);
    
    // write the bitmap data:
    Sys_FileWrite (myFile, theData, mySize);
    
    // good bye...
    Sys_FileClose (myFile);
}

//___________________________________________________________________________________________________________eOF
