//___________________________________________________________________________________________________________nFO
// "snd_osx.m" - MacOS X Sound driver.
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
// v1.0.0: Initial release.
//______________________________________________________________________________________________________iNCLUDES

#pragma mark =Includes=

#include "quakedef.h"

#include <CoreAudio/AudioHardware.h>

#pragma mark -

//_______________________________________________________________________________________________________dEFINES

#pragma mark =Defines=

#define OUTPUT_BUFFER_SIZE	(4 * 1024)
#define NO 			0
#define	YES			1

#pragma mark -

//______________________________________________________________________________________________________tYPEDEFS

#pragma mark =TypeDefs=

typedef int				SInt;
typedef unsigned int			UInt;

#pragma mark -

//_____________________________________________________________________________________________________vARIABLES

#pragma mark =Variables=

AudioDeviceID 				gSoundDeviceID;
Boolean					gCDIOProcIsInstalled = NO, gAudioIOProcIsInstalled = NO;
static unsigned char 			gSoundBuffer[64*1024];
static UInt32				gBufferPosition;
static AudioStreamBasicDescription	gSoundBasicDescription;

#pragma mark -

//___________________________________________________________________________________________fUNCTION_pROTOTYPES

#pragma mark =Function Prototypes=

extern OSStatus CD_AudioIOProc (AudioDeviceID, const AudioTimeStamp *, const AudioBufferList *,
                                const AudioTimeStamp *, AudioBufferList *, const AudioTimeStamp *,
                                void *);
static OSStatus SNDDMA_AudioIOProc (AudioDeviceID, const AudioTimeStamp *, const AudioBufferList *,
                                   const AudioTimeStamp *, AudioBufferList *, const AudioTimeStamp *,
                                   void *);

#pragma mark -

//__________________________________________________________________________________________SNDDMA_AudioIOProc()

static OSStatus SNDDMA_AudioIOProc (AudioDeviceID inDevice,
                                    const AudioTimeStamp *inNow,
                                    const AudioBufferList *inInputData,
                                    const AudioTimeStamp *inInputTime,
                                    AudioBufferList *outOutputData, 
                                    const AudioTimeStamp *inOutputTime,
                                    void *inClientData)
{
    UInt16	i;
    short	*myDMA = ((short *) gSoundBuffer) + gBufferPosition / (shm->samplebits >> 3);
    float	*myOutBuffer = (float *) outOutputData->mBuffers[0].mData;

    // convert the buffer to float, required by CoreAudio:
    for (i = 0; i < OUTPUT_BUFFER_SIZE; i++)
    {
        *myOutBuffer++ = (*myDMA++) * (1.0f / 32768.0f);
    }
    
    // increase the bufferposition:
    gBufferPosition += OUTPUT_BUFFER_SIZE * (shm->samplebits >> 3);
    if (gBufferPosition >= sizeof (gSoundBuffer))
    {
        gBufferPosition = 0;
    }
    
    // return 0 = no error:
    return (0);
}

//_________________________________________________________________________________________________SNDDMA_Init()

qboolean SNDDMA_Init (void)
{
    UInt32	myPropertySize, myBufferByteCount;

    Con_Printf ("Initializing CoreAudio...\n");
    myPropertySize = sizeof (gSoundDeviceID);
    
    // find a suitable audio device:
    if (AudioHardwareGetProperty (kAudioHardwarePropertyDefaultOutputDevice, &myPropertySize,
                                  &gSoundDeviceID))
    {
        Con_Printf ("Audio init fails: Can\t get audio device.");
        return (0);
    }
    
    // is the device valid?
    if (gSoundDeviceID == kAudioDeviceUnknown)
    {
        Con_Printf ("Audio init fails: Unsupported audio device.\n");
        return (0);
    }
    
    // set the buffersize for the audio device:
    myPropertySize = sizeof (myBufferByteCount);
    myBufferByteCount = OUTPUT_BUFFER_SIZE * sizeof (float);
    if (AudioDeviceSetProperty (gSoundDeviceID, NULL, 0, NO, kAudioDevicePropertyBufferSize,
                                myPropertySize, &myBufferByteCount))
    {
        Con_Printf ("Can't get audiobuffer.");
        return (0);
    }
    
    // get the audiostream format:
    myPropertySize = sizeof (gSoundBasicDescription);
    if (AudioDeviceGetProperty (gSoundDeviceID, 0, NO, kAudioDevicePropertyStreamFormat,
                                &myPropertySize, &gSoundBasicDescription))
    {
        Con_Printf ("Audio init fails.\n");
        return (0);
    }
    
    // is the format LinearPCM?
    if (gSoundBasicDescription.mFormatID != kAudioFormatLinearPCM)
    {
        Con_Printf ("Default Audio Device doesn't support Linear PCM!\n");
        return(0);
    }
    
    // is sound ouput suppressed?
    if (!COM_CheckParm ("-nosound"))
    {
        // add the sound FX IO:
        if (AudioDeviceAddIOProc (gSoundDeviceID, SNDDMA_AudioIOProc, NULL))
        {
            Con_Printf ("Audio init fails: Can\'t install IOProc.\n");
            return (0);
        }
        
        // start the sound FX:
        if (AudioDeviceStart (gSoundDeviceID, SNDDMA_AudioIOProc))
        {
            Con_Printf ("Audio init fails: Can\'t start audio.\n");
            return (0);
        }
        gAudioIOProcIsInstalled = YES;
    }
    else
    {
        gAudioIOProcIsInstalled = NO;
    }
    
    // add the CD Audio IO:
    if (AudioDeviceAddIOProc (gSoundDeviceID, CD_AudioIOProc, NULL))
    {
        Con_Printf ("CD init fails: Can\'t install IOProc.\n");
    }
    else
    {
        gCDIOProcIsInstalled = YES;
    }
    
    // setup Quake sound variables:
    shm = (void *) Hunk_AllocName (sizeof (*shm), "shm");
    shm->splitbuffer = 0;
    shm->samplebits = 16;
    shm->speed = gSoundBasicDescription.mSampleRate;
    shm->channels = gSoundBasicDescription.mChannelsPerFrame;
    shm->samples = sizeof (gSoundBuffer) / (shm->samplebits >> 3);
    shm->samplepos = 0;
    shm->soundalive = true;
    shm->gamealive = true;
    shm->submission_chunk = OUTPUT_BUFFER_SIZE;
    shm->buffer = gSoundBuffer;
    gBufferPosition = 0;
    
    // output a description of the sound format:
    if (!COM_CheckParm ("-nosound"))
    {
        Con_Printf ("Sound Channels: %d\n", shm->channels);
        Con_Printf ("Sound sample bits: %d\n", shm->samplebits);
    }
    
    return (1);
}

//_____________________________________________________________________________________________SNDDMA_Shutdown()

void	SNDDMA_Shutdown (void)
{
    // shut everything down:
    if (gAudioIOProcIsInstalled)
    {
        AudioDeviceStop (gSoundDeviceID, SNDDMA_AudioIOProc);
        AudioDeviceRemoveIOProc (gSoundDeviceID, SNDDMA_AudioIOProc);
    }
}

//_______________________________________________________________________________________________SNDDMA_Submit()

void	SNDDMA_Submit(void)
{
}

//____________________________________________________________________________________________SNDDMA_GetDMAPos()

SInt	SNDDMA_GetDMAPos(void)
{
    if (!gAudioIOProcIsInstalled)
    {
        return (0);
    }
    return (gBufferPosition / (shm->samplebits >> 3));
}

//___________________________________________________________________________________________________________eOF
