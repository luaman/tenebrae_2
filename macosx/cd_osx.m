//___________________________________________________________________________________________________________nFO
// "cd_osx.m" - MacOS X audio CD driver.
//
// Written by:	Axel "awe" Wefers		[mailto:awe@fruitz-of-dojo.de].
//		(C)2001-2002 Fruitz Of Dojo 	[http://www.fruitz-of-dojo.de].
//
// Quakeª is copyrighted by id software	[http://www.idsoftware.com].
//
// Version History:
//
// TenebraeQuake:
// v1.0.0: Initial release.
//
// Standard Quake branch [pre-TenebraeQuake]:
// v1.0.1: Added "cdda" as extension for detection of audio-tracks [required by MacOS X v10.1 or later].
// v1.0.0: Initial release.
//______________________________________________________________________________________________________iNCLUDES

#pragma mark =Includes=

#include "quakedef.h"

#include <AppKit/AppKit.h>
#include <Foundation/Foundation.h>
#include <CoreAudio/AudioHardware.h>
#include <sys/mount.h>

#pragma mark -

//_______________________________________________________________________________________________________dEFINES

#pragma mark =Defines=

#define CD_SAMPLE_BUFFER (4 * 1024)

#pragma mark -

//______________________________________________________________________________________________________tYPEDEFS

#pragma mark =TypeDefs=

typedef int				SInt;
typedef unsigned int			UInt;

typedef struct	{
                    UInt		chunkID;
                    UInt		chunkSize;
                    UInt		fileType;
                } AIFFChunkHeader;

typedef struct	{
                    UInt		chunkID;
                    UInt		chunkSize;
                } AIFFGenericChunk;

typedef struct	{
                    UInt		offset;
                    UInt		blockSize;
                } AIFFSSNDData;

typedef struct	{
                    FILE 		*file;
                } AIFFInfo; 

#pragma mark -

//_____________________________________________________________________________________________________vARIABLES

#pragma mark =Variables=

extern cvar_t			bgmvolume;
extern AudioDeviceID 		gSoundDeviceID;
extern Boolean			gCDIOProcIsInstalled;
static Boolean			gAudioCDIsPaused, gLoopTrack; 
static UInt8			gPlayAudioCD;
static UInt16			gCDTrackCount, gCurCDTrack;
static SInt16			gCDSampleBuffer[CD_SAMPLE_BUFFER];
static NSMutableArray *		gCDTracks;
static AIFFInfo *         	gAIFFStream;
static float			gCDVolume;
static char 			gCDDevice[256];

#pragma mark -

//___________________________________________________________________________________________fUNCTION_pROTOTYPES

#pragma mark =Function Prototypes=

OSStatus CD_AudioIOProc (AudioDeviceID, const AudioTimeStamp *, const AudioBufferList *,
                         const AudioTimeStamp *, AudioBufferList *, const AudioTimeStamp *, void *);

static	AIFFInfo *	CD_OpenTrack(NSString *);
static	void		CD_CloseTrack(AIFFInfo *);
static	Boolean		CD_GetTrackList(void);
static 	void 		CD_f(void);

#pragma mark -

//________________________________________________________________________________________________CD_OpenTrack()

AIFFInfo *	CD_OpenTrack (NSString *thePath)
{
    const char 		*myPathStr;
    AIFFInfo		*myAIFFStream;
    FILE		*myFile;
    AIFFChunkHeader	myChunkHeader;
    AIFFGenericChunk	myChunk;
    AIFFSSNDData	mySsndData;
    
    // convert the path to POSIX compatible string and open the file:
    myPathStr = [thePath fileSystemRepresentation];
    myFile = fopen (myPathStr, "r");
    if (!myFile)
    {
        Con_Printf ("Can\'t open audio track!\n");
        return (NULL);
    }
    
    // initialize the new stream:
    myAIFFStream = malloc (sizeof(*myAIFFStream));
    if (myAIFFStream == NULL)
    {
        Con_Printf ("Can\'t open audio track!\n");
        return (NULL);
    }
    myAIFFStream->file = myFile;
    
    // get the header and check the format [must be AIFF]:
    fread (&myChunkHeader, 1, sizeof (myChunkHeader), myAIFFStream->file);
    if (myChunkHeader.chunkID != 'FORM')
    {
        Con_Printf ("Can\'t open audio track!\n");
        CD_CloseTrack (myAIFFStream);
        return (NULL);
    }
    if (myChunkHeader.fileType != 'AIFC')
    {
        Con_Printf ("Can\'t open audio track!\n");
        CD_CloseTrack (myAIFFStream);
        return (NULL);
    }
    
    // look for the "SSND" chunk:
    while (1)
    {
        fread (&myChunk, 1, sizeof (myChunk), myAIFFStream->file);
        if (myChunk.chunkID == 'SSND')
        {
            break;
        }
        fseek (myAIFFStream->file, myChunk.chunkSize, SEEK_CUR);
        if (ferror (myAIFFStream->file))
        {
            return (NULL);
        }
    }
    fread (&mySsndData, 1, sizeof (mySsndData), myAIFFStream->file);

    return (myAIFFStream);
}

//_______________________________________________________________________________________________CD_CloseTrack()

void	CD_CloseTrack (AIFFInfo *theAIFFStream)
{
    // close the file and free memory:
    if (theAIFFStream != NULL)
    {
        fclose (theAIFFStream->file);
        free (theAIFFStream);
    }
}

//______________________________________________________________________________________________CD_AudioIOProc()

OSStatus CD_AudioIOProc (AudioDeviceID 		inDevice,
                         const AudioTimeStamp 	*inNow,
                         const AudioBufferList 	*inInputData,
                         const AudioTimeStamp 	*inInputTime,
                         AudioBufferList 	*outOutputData,
                         const AudioTimeStamp 	*inOutputTime,
                         void 			*inClientData)
{
    float	*myOutBuffer;
    UInt	i = 0;

    // get the current CoreAudio buffer:
    myOutBuffer = (float *)outOutputData->mBuffers[0].mData;
    
    // is a stream open?
    if (gAIFFStream)
    {
        SInt16		*myInBuffer;
        UInt		mySampleCount;
        float		myScale = gCDVolume / 32768.0f; 
        
        // read the current AIFF sample data from CD:
        myInBuffer = gCDSampleBuffer;
        mySampleCount = fread (gCDSampleBuffer, sizeof (SInt16), CD_SAMPLE_BUFFER, gAIFFStream->file);
        
        // was the buffer filled?
        if (mySampleCount < CD_SAMPLE_BUFFER)
        {
            // no: probably end of file! so get the next track!
            if (feof (gAIFFStream->file))
            {
                if (gLoopTrack)
                {
                    // loop? restart the track.
                    fseek (gAIFFStream->file, 0L, SEEK_SET);
                    mySampleCount += fread (gCDSampleBuffer + mySampleCount, sizeof (SInt16),
                                            CD_SAMPLE_BUFFER - mySampleCount, gAIFFStream->file);
                }
                else
                {
                    // next track: so close the current track
                    CD_CloseTrack (gAIFFStream);
                    
                    // was it the last track?
                    if (gCurCDTrack == gCDTrackCount)
                    {
                        gCurCDTrack = 1;
                    }
                    else
                    {
                        gCurCDTrack++;
                    }
                    
                    // open the new track:
                    gAIFFStream = CD_OpenTrack ([gCDTracks objectAtIndex: gCurCDTrack - 1]);
                    
                    // fill the rest of the buffer:
                    mySampleCount += fread (gCDSampleBuffer + mySampleCount, sizeof (SInt16),
                                            CD_SAMPLE_BUFFER - mySampleCount, gAIFFStream->file);
                }
                 
            }
        }
        for (i = 0; i < mySampleCount; i++)
        {
            // convert the buffer to float:
            *myOutBuffer++ = ((SInt16) (((*myInBuffer & 0xff00) >> 8) | ((*myInBuffer & 0x00ff) << 8)))
                             * myScale;
            myInBuffer++;
        }
    }
    
    // fill the rest of the buffer with zeros:
    for (; i < CD_SAMPLE_BUFFER; i++)
    {
        *myOutBuffer++ = 0.0f;
    }
    
    return (0);
}

//_____________________________________________________________________________________________CD_GetTrackList()

Boolean	CD_GetTrackList (void)
{
    NSAutoreleasePool 		*myPool;
    NSDirectoryEnumerator	*myDirEnum;
    NSFileManager		*myFileManager;
    UInt			myMountCount;
    struct statfs  		*myMountList;
    NSString			*myMountPath;
    NSString			*myFilePath;
    UInt16			i, myStrLength;
    
    // release previously allocated memory:
    if (gCDTracks)
    {
        [gCDTracks release];
    }
    
    // get memory for the new tracklisting:
    gCDTracks = [[NSMutableArray alloc] init];
    myPool = [[NSAutoreleasePool alloc] init];
    gCDTrackCount = 0;
    
    // get number of mounted devices:
    myMountCount = getmntinfo (&myMountList, MNT_NOWAIT);
    
    // zero devices? return.
    if (myMountCount <= 0)
    {
        [gCDTracks release];
        gCDTracks = NULL;
        gCDTrackCount = 0;
        Con_Printf ("No Audio-CD found.\n");
        return (0);
    }
    myFileManager = [NSFileManager defaultManager];
    while (myMountCount--)
    {
        // is the device read only?
        if ((myMountList[myMountCount].f_flags & MNT_RDONLY) != MNT_RDONLY) continue;
        
        // is the device local?
        if ((myMountList[myMountCount].f_flags & MNT_LOCAL) != MNT_LOCAL) continue;
        
        // is the device "cdda"?
        if (strcmp (myMountList[myMountCount].f_fstypename, "cddafs")) continue;
        
        // is the device a directory?
        if (strrchr (myMountList[myMountCount].f_mntonname, '/') == NULL) continue;
        
        // we have found a Audio-CD!
        Con_Printf ("Found Audio-CD at mount entry: \"%s\".\n", myMountList[myMountCount].f_mntonname);
        
        // preserve the device name:
        myStrLength = strlen (myMountList[myMountCount].f_mntonname);
        if (myStrLength > 255) myStrLength = 255;
        for (i = 0; i < myStrLength; i++)
        {
            gCDDevice[i] = myMountList[myMountCount].f_mntonname[i];
        }
        gCDDevice[255] = 0x00;
        myMountPath = [NSString stringWithCString: myMountList[myMountCount].f_mntonname];
        myDirEnum = [myFileManager enumeratorAtPath: myMountPath];

        // get all audio tracks:
        while ((myFilePath = [myDirEnum nextObject]))
        {
            if ([[myFilePath pathExtension] isEqualToString: @"cdda"] ||	// MacOS X 10.1+
                [[myFilePath pathExtension] isEqualToString: @"aiff"])		// MacOS X 10.0.0 - 10.0.4
            {
                [gCDTracks addObject: [myMountPath stringByAppendingPathComponent: myFilePath]];
                gCDTrackCount++;
            }
        }
        
        break;
    }
    
    // release the pool:
    [myPool release];
    
    // just security:
    if (![gCDTracks count])
    {
        [gCDTracks release];
        gCDTracks = NULL;
        gCDTrackCount = 0;
        Con_Printf ("No audio tracks found.\n");
        return (0);
    }
    
    return (1);
}

//________________________________________________________________________________________________CDAudio_Play()

void	CDAudio_Play (UInt8 theTrack, qboolean theLooping)
{
    if (gCDTracks && gCDIOProcIsInstalled)
    {
        // check for mismatching CD no.:
        if (theTrack > gCDTrackCount)
        {
            theTrack = gCDTrackCount;
        }
        else
        {
            if (theTrack <= 0)
            {
                theTrack = 1;
            }
        }
        gCurCDTrack = theTrack;
        gAIFFStream = CD_OpenTrack ([gCDTracks objectAtIndex: theTrack - 1]);
        
        // start the Audio IO:
        if(AudioDeviceStart (gSoundDeviceID, CD_AudioIOProc))
        {
            Con_Printf ("Failed to play audio CD.\n");
        }
        else
        {
            // pause the Audio-CD if volume is zero:
            if (gCDVolume == 0.0)
            {
                CDAudio_Pause ();
            }
            gPlayAudioCD = 1;
            gAudioCDIsPaused = NO;
            gLoopTrack = theLooping;
        }
    }
}

//________________________________________________________________________________________________CDAudio_Stop()

void	CDAudio_Stop (void)
{
    // just stop the audio IO:
    if (gCDIOProcIsInstalled)
    {
        AudioDeviceStop (gSoundDeviceID, CD_AudioIOProc);
        CD_CloseTrack (gAIFFStream);
        gAIFFStream = NULL;
        gPlayAudioCD = 0;
    }
}

//_______________________________________________________________________________________________CDAudio_Pause()

void	CDAudio_Pause (void)
{
    // just stop the audio IO:
    if (gCDIOProcIsInstalled)
    {
        AudioDeviceStop (gSoundDeviceID, CD_AudioIOProc);
        gAudioCDIsPaused = YES;
        gPlayAudioCD = 0;
    }
}

//______________________________________________________________________________________________CDAudio_Resume()

void	CDAudio_Resume (void)
{
    // resume only, if CD audio was previously paused:
    if (gCDIOProcIsInstalled && gAudioCDIsPaused)
    {
        if (AudioDeviceStart (gSoundDeviceID, CD_AudioIOProc))
        {
            Con_Printf ("Can\'t resume CD auio. Use \"cd reset\" to fix the problem.\n");
        }
        else
        {
            gAudioCDIsPaused = NO;
        }
    }
}

//______________________________________________________________________________________________CDAudio_Update()

void	CDAudio_Update (void)
{
    // just update volume settings here:
    if (gCDVolume != bgmvolume.value)
    {
        if (gCDVolume)
        {
            gCDVolume = bgmvolume.value;
            if (!gCDVolume)
            {
                CDAudio_Pause ();
            }
        }
        else
        {
            gCDVolume = bgmvolume.value;
            if (!gCDVolume)
            {
                CDAudio_Resume ();
            }
        }
    }
}

//________________________________________________________________________________________________CDAudio_Init()

SInt	CDAudio_Init (void)
{
    // add "cd" console command:
    Cmd_AddCommand ("cd", CD_f);

    gCDVolume = bgmvolume.value;
    gPlayAudioCD = 0;
    
    // init SNDDMA, if "-nosound" was used:
    if (COM_CheckParm ("-nosound"))
    {
        if (SNDDMA_Init ())
        {
            // get the track listing:
            if (CD_GetTrackList ())
            {
                gAudioCDIsPaused = NO;
                Con_Print ("CoreAudio CD driver initialized...\n");

                return(1);
            }
        }
    }
    else
    {
         // get the track listing:
        if (CD_GetTrackList ())
        {
            gAudioCDIsPaused = NO;
            Con_Print ("CoreAudio CD driver initialized...\n");

            return (1);
        }
    }
    
    // failure. return 0.
    Con_Print ("CoreAudio CD driver failed. Reason: No CD found.\n");
    
    return (0);
}

//____________________________________________________________________________________________CDAudio_Shutdown()

void	CDAudio_Shutdown (void)
{
    // shutdown the audio IO:
    if (gCDIOProcIsInstalled)
    {
        CDAudio_Stop ();
        AudioDeviceRemoveIOProc (gSoundDeviceID, CD_AudioIOProc);
        if (gCDTracks != NULL)
        {
            [gCDTracks release];
            gCDTracks = NULL;
        }
    }
}

//________________________________________________________________________________________________________CD_f()

void	CD_f (void)
{
    char	*myCommandOption;

    // this command requires options!
    if (Cmd_Argc () < 2)
    {
        return;
    }

    // get the option:
    myCommandOption = Cmd_Argv (1);
    
    // turn CD playback on:
    if (Q_strcasecmp (myCommandOption, "on") == 0)
    {
        if (!gCDTrackCount)
        {
            CD_GetTrackList();
        }
        CDAudio_Play(1, 0);
        
	return;
    }
    
    // turn CD playback off:
    if (Q_strcasecmp (myCommandOption, "off") == 0)
    {
        CDAudio_Stop ();
        
	return;
    }

    // reset the current CD:
    if (Q_strcasecmp (myCommandOption, "reset") == 0)
    {
        CDAudio_Stop ();
        if (CD_GetTrackList ())
        {
            Con_Printf ("CD found. %d tracks [\"%s\"].\n", gCDTrackCount, gCDDevice);
            gAudioCDIsPaused = NO;
	}
        else
        {
            Con_Printf ("No CD found.\n");            
        }
        
	return;
    }
    
    // the following commands require a valid track array, so build it, if not present:
    if (!gCDTrackCount)
    {
        CD_GetTrackList ();
        if (!gCDTrackCount)
        {
            Con_Printf ("No CD in player.\n");
            
            return;
        }
    }
    
    // play the selected track:
    if (Q_strcasecmp (myCommandOption, "play") == 0)
    {
        CDAudio_Play (Q_atoi (Cmd_Argv (2)), 0);
        
	return;
    }
    
    // loop the selected track:
    if (Q_strcasecmp (myCommandOption, "loop") == 0)
    {
        CDAudio_Play (Q_atoi (Cmd_Argv (2)), 1);
        
	return;
    }
    
    // stop the current track:
    if (Q_strcasecmp (myCommandOption, "stop") == 0)
    {
        CDAudio_Stop ();
        
	return;
    }
    
    // pause the current track:
    if (Q_strcasecmp (myCommandOption, "pause") == 0)
    {
        CDAudio_Pause ();
        
	return;
    }
    
    // resume the current track:
    if (Q_strcasecmp (myCommandOption, "resume") == 0)
    {
        CDAudio_Resume ();
        
	return;
    }
    
    // eject the CD:
    if (Q_strcasecmp (myCommandOption, "eject") == 0)
    {
        // stop CD playback.
        CDAudio_Stop ();
        
        // get the current device:
        if (!gCDTracks)
        {
            CD_GetTrackList();
        }
        
        // eject the CD:
        if (gCDTracks)
        {
            if ([[NSWorkspace sharedWorkspace]
                 unmountAndEjectDeviceAtPath:[NSString stringWithCString:gCDDevice]])
            {
                [gCDTracks release];
                gCDTracks = NULL;
                gCDTrackCount = 0;
                gCurCDTrack = 0;
                gPlayAudioCD = 0;
            }
            else
            {
                Con_Printf ("Can't eject Audio-CD!\n");
            }
        }
        else
        {
            Con_Printf ("No Audio-CD in drive!\n");
        }
        
	return;
    }
    
    // output CD info:
    if (Q_strcasecmp(myCommandOption, "info") == 0)
    {
        Con_Printf ("Playing track %d of %d [\"%s\"].\n", gCurCDTrack, gCDTrackCount, gCDDevice);
        Con_Printf ("Volume is: %.2f.\n", gCDVolume);
        
	return;
    }
}

//___________________________________________________________________________________________________________eOF
