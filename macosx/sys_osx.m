//___________________________________________________________________________________________________________nFO
// "sys_osx.m" - MacOS X system functions.
//
// Written by:	Axel "awe" Wefers		[mailto:awe@fruitz-of-dojo.de].
//		(C)2001-2002 Fruitz Of Dojo 	[http://www.fruitz-of-dojo.de].
//
// Quakeª is copyrighted by id software	[http://www.idsoftware.com].
//
// Version History:
//
// TenebraeQuake:
// v1.0:   Initial release.
//
// Standard Quake branch [pre-TenebraeQuake]:
// v1.0.8: Extended the GLQuake and GLQuakeWorld startup dialog with "FSAA Samples" option.
// v1.0.7: From now on you will only be asked to locate the "id1" folder, if the application is not
//	   installed inside the same folder which holds the "id1" folder.
// v1.0.6: Fixed cursor vsisibility on system error.
//         Fixed loss of mouse control after CMD-TABing with the software renderer.
//         Fixed a glitch with the Quake/QuakeWorld options menu [see "menu.c"].
//         Reworked keypad code [is now hardware dependent] to avoid "sticky" keys.
//	   Moved fixing of linebreaks to "cmd.c".
// v1.0.5: Added support for numeric keypad.
//         Added support for paste [CMD-V and "Edit/Paste" menu].
//	   Added support for up to 5 mouse buttons.
//         Added "Connect To Server" service.
//	   Added support for CMD-M, CMD-H and CMD-Q.
//         Added web link to the help menu.
//	   Fixed keyboard repeat after application quit.
// v1.0.4: Changed all vsprintf () calls to vsnprintf (). vsnprintf () is cleaner.
// v1.0.3: Fixed the broken "drag MOD onto Quake icon" method [blame me].
//	   Enabled Num. Lock Key.
// v1.0.2: Reworked settings dialog.
//	   Reenabled keyboard repeat and mousescaling.
//         Some internal changes.
// v1.0.1: Added support for GLQuake.
//	   Obscure characters within the base pathname [like 'Ä'...] are now allowed.
//	   Better support for case sensitive file systems.
//	   FIX: "mkdir" commmand [for QuakeWorld].
//	   FIX: The "id1" folder had to be lower case. Can now be upper or lower case.
// v1.0:   Initial release.
//______________________________________________________________________________________________________iNCLUDES

#pragma mark =Includes=

#import <AppKit/AppKit.h>
#import <Cocoa/Cocoa.h>
#import <Foundation/Foundation.h>

#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <limits.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <string.h>
#include <ctype.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/param.h>
#include <errno.h>
#import <drivers/event_status_driver.h>

#include "quakedef.h"

#pragma mark -

//_______________________________________________________________________________________________________dEFINES

#pragma mark =Defines=

#define BETA_VERSION			// undef to remove warning dialog!

#define FRUITZ_OF_DOJO_URL		@"http://www.fruitz-of-dojo.de/"

#define	DEFAULT_BASE_PATH		@"Quake ID1 Path"
#define DEFAULT_GL_DISPLAY		@"TenebraeQuake Display"
#define	DEFAULT_GL_DISPLAY_MODE		@"TenebraeQuake Display Mode"
#define DEFAULT_GL_COLORS		@"TenebraeQuake Display Depth"
#define DEFAULT_GL_SAMPLES		@"TenebraeQuake Samples"
#define DEFAULT_GL_FADE_ALL		@"TenebraeQuake Fade All Displays"
#define DEFAULT_GL_FULLSCREEN		@"TenebraeQuake Fullscreen"

#define INITIAL_BASE_PATH		@"id1"
#define INITIAL_GL_DISPLAY		@"0"
#define	INITIAL_GL_DISPLAY_MODE		@"640x480 67Hz"
#define INITIAL_GL_COLORS		@"0"
#define INITIAL_GL_SAMPLES		@"0"
#define INITIAL_GL_FADE_ALL		@"YES"
#define	INITIAL_GL_FULLSCREEN		@"YES"

#define	ID1_VALIDATION_FILE		@"/PAK0.PAK"
#define TENEBRAE_VALIDATION_FILE	@"../tenebrae/PAK0.PAK"
#define	GAME_COMMAND			"-game"

#define	QUAKE_BASE_PATH			"."
#define QWSV_BASE_PATH			"."

#define	MOUSE_BUTTONS			5	// MacOS X supports up to 32 buttons[change in in_osx.m, too].

#define	SYS_STRING_SIZE			1024	// string size for vsnprintf ().

#define MAX_DISPLAYS 			100

#pragma mark -

//________________________________________________________________________________________________________mACROS

#pragma mark =Macros=

#define CHECK_MALLOC(MEM_PTR)	if ((MEM_PTR) == NULL)						   \
                                {								   \
                                    Sys_Error ("Out of memory!");				   \
                                }

#define CHECK_MOUSE_ENABLED()	if ((gDisplayFullscreen == NO && _windowed_mouse.value == 0.0f) || \
                                    gMouseEnabled == NO || gIsMinimized == YES || gIsDeactivated)  \
                                {								   \
                                    return;							   \
                                }

#pragma mark -

//__________________________________________________________________________________________________________vARS

#pragma mark =Variables=

extern  UInt			gDisplay;
extern	SInt                    gARBMultiSamples;
extern  CGDirectDisplayID  	gDisplayList[MAX_DISPLAYS];
extern  CGDisplayCount		gDisplayCount;
extern  NSDictionary		*gDisplayMode;
extern  Boolean			gFadeAllDisplays;
extern  cvar_t			_windowed_mouse;
extern  NSWindow		*gWindow;
extern  Boolean			gIsMinimized;
extern  qboolean		gDisplayFullscreen,
                                gMouseEnabled;

Boolean                         gIsHidden = NO,
                                gIsDeactivated = NO;
qboolean			isDedicated;

static	char			gRequestedServer[128];
static 	NSDate			*gDistantPast;
static 	SInt			gNoStdOut = 0,
                                gArgCount;
static 	char 			*gModDir = NULL,
                                **gArgValues = NULL;
static 	Boolean			gDenyDrag = NO,
                                gWasDragged = NO,
                                gHostInitialized = NO;
                                
static 	UInt8	gSpecialKey[] = {
                                    K_UPARROW, K_DOWNARROW, K_LEFTARROW, K_RIGHTARROW,
                                         K_F1,        K_F2,        K_F3,         K_F4,
                                         K_F5,        K_F6,        K_F7,         K_F8,
                                         K_F9,       K_F10,       K_F11,        K_F12,
                                        K_F13,       K_F14,       K_F15,            0,
                                            0,   	 0, 	      0, 	    0,
                                            0, 		 0, 	      0, 	    0,
                                            0, 		 0, 	      0, 	    0,
                                            0, 		 0, 	      0, 	    0,
                                            0, 	 	 0, 	      0,        K_INS,
                                        K_DEL, 	    K_HOME, 	      0, 	K_END,
                                       K_PGUP,      K_PGDN,	      0,	    0,
                                      K_PAUSE,		 0,	      0,	    0,
                                            0,		 0,	      0,	    0,
                                            0, 	 K_NUMLOCK, 	      0, 	    0,                                                                        					    0, 		 0, 	      0, 	    0,
                                            0, 		 0, 	      0, 	    0,
                                            0, 		 0, 	  K_INS, 	    0
                                };

static 	UInt8	gNumPadKey[] = 	{	
                                            0,	          0,	       0, 	       0,
                                            0,	          0,	       0, 	       0,
                                            0,	          0,	       0, 	       0,
                                            0,	          0,	       0, 	       0,
                                            0,	          0,	       0, 	       0,
                                            0,	          0,	       0, 	       0,
                                            0,	          0,	       0, 	       0,
                                            0,	          0,	       0, 	       0,
                                            0,	          0,	       0, 	       0,
                                            0,	          0,	       0, 	       0,
                                            0,	          0,	       0, 	       0,
                                            0,	          0,	       0, 	       0,
                                            0,	          0,	       0, 	       0,
                                            0,	          0,	       0, 	       0,
                                            0,	          0,	       0, 	       0,
                                            0,	          0,	       0, 	       0,
                                            0, K_PERIOD_PAD,	       0, K_ASTERISK_PAD,
                                            0,   K_PLUS_PAD,	       0,	       0,
                                            0,		  0,	       0,	       0,
                                  K_ENTER_PAD,  K_SLASH_PAD, K_MINUS_PAD,	       0,
                                            0,  K_EQUAL_PAD,     K_0_PAD, 	 K_1_PAD,
                                      K_2_PAD,      K_3_PAD,     K_4_PAD, 	 K_5_PAD,
                                      K_6_PAD,      K_7_PAD,           0,        K_8_PAD,
                                      K_9_PAD,	   	  0,	       0,              0
                                };

#pragma mark -

//____________________________________________________________________________________________________iNTERFACES

#pragma mark =ObjC Interfaces=

@interface Quake : NSObject
{
    IBOutlet NSPopUpButton	*displayPopUp;
    IBOutlet NSPopUpButton	*modePopUp;
    IBOutlet NSPopUpButton	*colorsPopUp;
    IBOutlet NSPopUpButton	*samplesPopUp;
    IBOutlet NSButton		*fullscreenCheckBox;
    IBOutlet NSButton		*fadeAllCheckBox;
    IBOutlet NSWindow		*settingsWindow;
    
    NSMutableArray		*gModeList;
}

+ (void) initialize;
- (BOOL) application: (NSApplication *) theSender openFile: (NSString *) theFilePath;
- (void) applicationDidFinishLaunching: (NSNotification *) theNote;
- (NSApplicationTerminateReply) applicationShouldTerminate: (NSApplication *) theSender;
- (void) applicationDidResignActive: (NSNotification *) theNote;
- (void) applicationDidBecomeActive: (NSNotification *) theNote;
- (void) applicationWillHide: (NSNotification *) theNote;
- (void) applicationWillUnhide: (NSNotification *) theNote;

- (void) buildDisplayList;
- (IBAction) buildResolutionList: (id) theSender;
- (void) newGame: (id) theSender;
- (void) setupDialog;
- (Boolean) isEqualTo: (NSString *) theString;
- (NSString *) displayModeToString: (NSDictionary *) theDisplayMode;

- (void) startQuake;
- (void) performOpenURL: (NSString *) theURL;

- (IBAction) pasteString: (id) theSender;
- (IBAction) visitFOD: (id) theSender;

- (void) connectToServer: (NSPasteboard *) thePasteboard userData:(NSString *)theData
                   error: (NSString **) theError;
@end

#pragma mark -
                        
//___________________________________________________________________________________________fUNCTION_pROTOTYPES

#pragma mark =Function Prototypes=

extern Boolean	GL_CheckARBMultisampleExtension (CGDirectDisplayID theDisplay);

extern void	IN_ReceiveMouseMove (SInt, SInt);
extern void	IN_ShowCursor (Boolean);

extern void 	M_Menu_Quit_f (void);

char *		Sys_GetClipboardData (void);
void	 	Sys_DoEvents (NSEvent *, NSEventType);
void		Sys_SetKeyboardRepeatEnabled (Boolean);
void		Sys_SwitchCase (char *, UInt16);
void		Sys_FixFileNameCase (char *);
void		Sys_CheckForIDDirectory (void);

SInt 		main (SInt, const char **);

#pragma mark -

//______________________________________________________________________________________________Sys_SwitchCase()

void	Sys_SwitchCase (char *theString, UInt16 thePosition)
{
    while (theString[thePosition] != 0x00)
    {
        if (theString[thePosition] >= 'a' && theString[thePosition] <= 'z')
        {
            theString[thePosition] += ('A' - 'a');
        }
        else
        {
            if (theString[thePosition] >= 'A' && theString[thePosition] <= 'Z')
                theString[thePosition] += ('a' - 'A');
        }
        thePosition++;
    }
}

//______________________________________________________________________________________Sys_FixForFileNameCase()

void	Sys_FixFileNameCase (char *thePath)
{
    FILE	*myFile = NULL;
   
    if (!(myFile = fopen (thePath, "rb")))
    {
        UInt16		myPathEnd = 0, i;

        for (i = 0; i < strlen (thePath); i++)
            if (thePath[i] == '/') myPathEnd = i;
        if (myPathEnd)
        {
            char		myBaseDir[MAXPATHLEN];
            UInt16		myPathStart;

            getcwd (myBaseDir, MAXPATHLEN);
            i = 0;
            while (thePath[i] != '/') i++;
            myPathStart = i++;
            while (i <= myPathEnd)
            {
                if (thePath[i] == '/')
                {
                    thePath[i] = 0x00;
                    if (chdir (thePath))
                    {
                        Sys_SwitchCase (thePath, myPathStart);
                        chdir (myBaseDir);
                        if(chdir (thePath))
                        {
                            Sys_SwitchCase (thePath, myPathStart);
                            thePath[i] = '/';
                            chdir (myBaseDir);
                            return;
                        }
                    }
                    myPathStart = i + 1;
                    thePath[i] = '/';
                    chdir (myBaseDir);
                }
                i++;
            }
            chdir (myBaseDir);
        }
        if (!(myFile = fopen (thePath, "rb")))
        {
            if(!(i = strlen (thePath))) return;
            i--;
            while (i)
            {
                if(thePath[i - 1] == '/') break;
                i--;
            }
            Sys_SwitchCase (thePath, i);
            if (!(myFile = fopen (thePath, "rb")))
                Sys_SwitchCase (thePath, i);
        }
    }
    if (myFile != NULL)
        fclose (myFile);
}

//____________________________________________________________________________________________Sys_FileOpenRead()

SInt	Sys_FileOpenRead (char *thePath, SInt *theHandle)
{
    SInt		myHandle;
    struct stat		myFileInfo;

    Sys_FixFileNameCase (thePath);

    myHandle = open (thePath, O_RDONLY, 0666);
    *theHandle = myHandle;
    
    if (myHandle == -1)
        return -1;
        
    if (fstat (myHandle, &myFileInfo) == -1)
        Sys_Error ("Can\'t open file \"%s\", reason: \"%s\".", thePath, strerror (errno));
        
    return (myFileInfo.st_size);
}

//___________________________________________________________________________________________Sys_FileOpenWrite()

SInt	Sys_FileOpenWrite (char *thePath)
{
    SInt     myHandle;

    umask (0);
    myHandle = open (thePath, O_RDWR | O_CREAT | O_TRUNC, 0666);
    
    if (myHandle == -1)
        Sys_Error ("Can\'t open file \"%s\" for writing, reason: \"%s\".", thePath, strerror(errno));
        
    return (myHandle);
}

//_______________________________________________________________________________________________Sys_FileClose()

void	Sys_FileClose (SInt theHandle)
{
    close (theHandle);
}

//________________________________________________________________________________________________Sys_FileSeek()

void	Sys_FileSeek (SInt theHandle, SInt thePosition)
{
    lseek (theHandle, thePosition, SEEK_SET);
}

//________________________________________________________________________________________________Sys_FileRead()

SInt	Sys_FileRead (SInt theHandle, void *theDest, SInt theCount)
{
    return (read (theHandle, theDest, theCount));
}

//_______________________________________________________________________________________________Sys_FileWrite()

SInt	Sys_FileWrite (SInt theHandle, void *theData, SInt theCount)
{
    return (write (theHandle, theData, theCount));
}

//________________________________________________________________________________________________Sys_FileTime()

int	Sys_FileTime (char *thePath)
{
    struct stat		myBuffer;
	
    if (stat (thePath, &myBuffer) == -1)
        return (-1);
        
    return (myBuffer.st_mtime);
}

//___________________________________________________________________________________________________Sys_mkdir()

void	Sys_mkdir (char *thePath)
{
    if (mkdir (thePath, 0777) != -1) return;
    
    if (errno != EEXIST)
        Sys_Error ("\"mkdir %s\" failed, reason: \"%s\".", thePath, strerror(errno));
}
//___________________________________________________________________________________________________Sys_Strtime()

void	Sys_Strtime (char *theTime)
{
	// FIX-ME : get system time as int Hour,int Min, int Sec
	//sprintf(theTime,"%02d:%02d:%02d",0,0,0);
	theTime[0]='\0';
}

//_______________________________________________________________________________________Sys_MakeCodeWriteable()

void	Sys_MakeCodeWriteable (UInt32 theStartAddress, UInt32 theLength)
{
    unsigned long 	myAddress;
    SInt 		myPageSize = getpagesize();

    myAddress = (theStartAddress & ~(myPageSize - 1)) - myPageSize;

    if (mprotect ((char*)myAddress, theLength + theStartAddress - myAddress + myPageSize, 7) < 0)
        Sys_Error ("Memory protection change failed!\n");
}

//___________________________________________________________________________________________________Sys_Error()

void	Sys_Error (char *theError, ...)
{
    va_list     myArgPtr;
    char        myString[SYS_STRING_SIZE];

    fcntl (0, F_SETFL, fcntl (0, F_GETFL, 0) & ~FNDELAY);
    
    va_start (myArgPtr, theError);
    vsnprintf (myString, SYS_STRING_SIZE, theError, myArgPtr);
    va_end (myArgPtr);

    Host_Shutdown();
    IN_ShowCursor (YES);
    Sys_SetKeyboardRepeatEnabled (YES);
    
    NSRunCriticalAlertPanel (@"An error has occured:", [NSString stringWithCString: myString],
                             NULL, NULL, NULL);
    NSLog (@"An error has occured: %@\n", [NSString stringWithCString: myString]);
    
    exit (1);
}

//____________________________________________________________________________________________________Sys_Warn()

void	Sys_Warn (char *theWarning, ...)
{
#if defined(DEBUG_BUILD)

    va_list     myArgPtr;
    char        myString[SYS_STRING_SIZE];
    
    va_start (myArgPtr, theWarning);
    vsnprintf (myString, SYS_STRING_SIZE, theWarning, myArgPtr);
    va_end (myArgPtr);
    
    fprintf (stderr, "Warning: %s", myString);

#endif /* DEBUG_BUILD */
} 

//_________________________________________________________________________________________________Sys_sprintf()

char *	Sys_sprintf (char *theFormat, ...)
{
    va_list     	myArgPtr;
    static char		myString[SYS_STRING_SIZE];
    
    va_start (myArgPtr, theFormat);
    vsnprintf (myString, SYS_STRING_SIZE, theFormat, myArgPtr);
    va_end (myArgPtr);
    
    return(myString);
}

//__________________________________________________________________________________________________Sys_Printf()

void	Sys_Printf (char *theFormat, ...)
{
    return;
}

//____________________________________________________________________________________________________Sys_Quit()

void	Sys_Quit (void)
{
    extern cvar_t	gl_fsaa;
    NSUserDefaults	*myDefaults = [NSUserDefaults standardUserDefaults];
    NSString		*myString;
    SInt		mySamples = gl_fsaa.value;
    
    // shutdown host:
    Host_Shutdown ();
    fcntl (0, F_SETFL, fcntl (0, F_GETFL, 0) & ~FNDELAY);
    fflush (stdout);
    
    // reenable keyboard repeat:
    Sys_SetKeyboardRepeatEnabled (YES);

    // save the FSAA setting again [for Radeon users]:
    switch (mySamples)
    {
        case 2:
        case 4:
            mySamples <<= 1;
            break;
        default:
            mySamples = 0;
            break;
    }
    myString = [NSString stringWithFormat: @"%d", mySamples];
    if ([myString isEqualToString: INITIAL_GL_SAMPLES])
    {
        [myDefaults removeObjectForKey: DEFAULT_GL_SAMPLES];
    }
    else
    {
        [myDefaults setObject: myString forKey: DEFAULT_GL_SAMPLES];
    }
    [myDefaults synchronize];

    exit (0);
}

//_______________________________________________________________________________________________Sys_FloatTime()

double	Sys_FloatTime (void)
{
    struct timeval 	myTimeValue;
    struct timezone 	myTimeZone; 
    static SInt      	myStartSeconds; 
    
    // return deltaT since the first call:
    gettimeofday (&myTimeValue, &myTimeZone);

    if (!myStartSeconds)
    {
        myStartSeconds = myTimeValue.tv_sec;
        return (myTimeValue.tv_usec / 1000000.0);
    }
    
    return ((myTimeValue.tv_sec - myStartSeconds) + (myTimeValue.tv_usec / 1000000.0));
}

//____________________________________________________________________________________________Sys_ConsoleInput()

char *	Sys_ConsoleInput (void)
{
    return (NULL);
}

//___________________________________________________________________________________________________Sys_Sleep()

void	Sys_Sleep (void)
{
    NSEvent 		*myEvent;
    NSAutoreleasePool 	*myPool = NULL;
   
    sleep (1);
   
    while (1)
    {
        myPool = [[NSAutoreleasePool alloc] init];
        if (myPool == NULL)
        {
            return;
        }
        
        // get the current event:
        myEvent = [NSApp nextEventMatchingMask: NSAnyEventMask
                                     untilDate: gDistantPast
                                        inMode: NSDefaultRunLoopMode
                                    dequeue: YES];
    
        // break out if all events flushed!
        if (myEvent == NULL)
        {
            [myPool release];
            break;
        }

        // process the event:
        [NSApp sendEvent: myEvent];
        [myPool release];
    }
}

//___________________________________________________________________________________________Sys_SendKeyEvents()

void	Sys_SendKeyEvents (void)
{
    NSEvent			*myEvent;
    NSAutoreleasePool 		*myPool;
    
    // required for [y]es/[n]o dialogs within Quake:
    myPool = [[NSAutoreleasePool alloc] init];
    myEvent = [NSApp nextEventMatchingMask:NSAnyEventMask untilDate:gDistantPast
                                        inMode:NSDefaultRunLoopMode dequeue:YES];
                                        
    Sys_DoEvents (myEvent, [myEvent type]);
    
    [myPool release];
}

//_________________________________________________________________________________________Sys_HighFPPrecision()

void	Sys_HighFPPrecision (void)
{
}

//__________________________________________________________________________________________Sys_LowFPPrecision()

void	Sys_LowFPPrecision (void)
{
}

//______________________________________________________________________________________________Sys_DoubleTime()

double	Sys_DoubleTime (void)
{
    return (Sys_FloatTime ());
}

//________________________________________________________________________________________________Sys_DebugLog()

void	Sys_DebugLog (char *theFile, char *theFormat, ...)
{
    va_list 		myArgPtr; 
    static char 	myText[SYS_STRING_SIZE];
    SInt 		myFileDescriptor;

    // get the format:
    va_start (myArgPtr, theFormat);
    vsnprintf (myText, SYS_STRING_SIZE, theFormat, myArgPtr);
    va_end (myArgPtr);

    // append the message to the debug logfile:
    myFileDescriptor = open (theFile, O_WRONLY | O_CREAT | O_APPEND, 0666);
    write (myFileDescriptor, myText, strlen (myText));
    close (myFileDescriptor);
}

//________________________________________________________________________________Sys_SetKeyboardRepeatEnabled()

void	Sys_SetKeyboardRepeatEnabled (Boolean theState)
{
    static Boolean	myKeyboardRepeatEnabled = YES;
    static double	myOriginalKeyboardRepeatInterval;
    static double	myOriginalKeyboardRepeatThreshold;
    NXEventHandle	myEventStatus;
    
    if (theState == myKeyboardRepeatEnabled)
        return;
    if (!(myEventStatus = NXOpenEventStatus ()))
        return;
        
    if (theState)
    {
        NXSetKeyRepeatInterval (myEventStatus, myOriginalKeyboardRepeatInterval);
        NXSetKeyRepeatThreshold (myEventStatus, myOriginalKeyboardRepeatThreshold);
        NXResetKeyboard (myEventStatus);
    }
    else
    {
        myOriginalKeyboardRepeatInterval = NXKeyRepeatInterval (myEventStatus);
        myOriginalKeyboardRepeatThreshold = NXKeyRepeatThreshold (myEventStatus);
        NXSetKeyRepeatInterval (myEventStatus, 3456000.0f);
        NXSetKeyRepeatThreshold (myEventStatus, 3456000.0f);
    }
    
    NXCloseEventStatus (myEventStatus);
    myKeyboardRepeatEnabled = theState;
}

//_______________________________________________________________________________________Sys_GetClipboardData ()

char *	Sys_GetClipboardData (void)
{
    NSPasteboard	*myPasteboard = NULL;
    NSArray 		*myPasteboardTypes = NULL;

    myPasteboard = [NSPasteboard generalPasteboard];
    myPasteboardTypes = [myPasteboard types];
    if ([myPasteboardTypes containsObject: NSStringPboardType])
    {
        NSString	*myClipboardString;

        myClipboardString = [myPasteboard stringForType: NSStringPboardType];
        if (myClipboardString != NULL && [myClipboardString length] > 0)
        {
            return (strdup ([myClipboardString cString]));
        }
    }
    return (NULL);
}

//____________________________________________________________________________________Sys_CheckForIDDirectory ()

void	Sys_CheckForIDDirectory (void)
{
    char		*myBaseDir = NULL;
    SInt		myResult,
                        myPathLength;
    BOOL		myFirstRun = YES,
                        myDefaultPath = YES,
                        myIDPath = NO,
                        myTenebraePath = NO,
                        myPathChanged = NO;
    NSOpenPanel		*myOpenPanel;
    NSUserDefaults 	*myDefaults;
    NSArray		*myFolder;
    NSString		*myValidatePath = nil,
                        *myBasePath = nil;

    // get the user defaults:
    myDefaults = [NSUserDefaults standardUserDefaults];

    // prepare the open panel for requesting the "id1" folder:
    myOpenPanel = [NSOpenPanel openPanel];
    [myOpenPanel setAllowsMultipleSelection: NO];
    [myOpenPanel setCanChooseFiles: NO];
    [myOpenPanel setCanChooseDirectories: YES];
    [myOpenPanel setTitle: @"Please locate the \"id1\" folder:"];
    
    // get the "id1" path from the prefs:
    myBasePath = [myDefaults stringForKey: DEFAULT_BASE_PATH];
    
    while (1)
    {
        if (myBasePath)
        {
            // check if the path exists:
            myValidatePath = [myBasePath stringByAppendingPathComponent: ID1_VALIDATION_FILE];
            myIDPath = [[NSFileManager defaultManager] fileExistsAtPath: myValidatePath];
            if (myIDPath == YES)
            {
                // check for the "tenebrae" folder:
                myValidatePath = [myBasePath stringByAppendingPathComponent: TENEBRAE_VALIDATION_FILE];
                myTenebraePath = [[NSFileManager defaultManager] fileExistsAtPath: myValidatePath];
                if (myTenebraePath == YES)
                {
                    // get a POSIX version of the path:
                    myBaseDir = (char *) [myBasePath fileSystemRepresentation];
                    myPathLength = strlen (myBaseDir);
                    
                    // check if the last component was "id1":
                    if (myPathLength >= 3)
                    {
                        if ((myBaseDir[myPathLength - 3] == 'i' || myBaseDir[myPathLength - 3] == 'I') &&
                            (myBaseDir[myPathLength - 2] == 'd' || myBaseDir[myPathLength - 2] == 'D') &&
                            myBaseDir[myPathLength - 1] == '1')
                        {
                            // remove "id1":
                            myBaseDir[myPathLength - 3] = 0x00;
                            
                            // change working directory to the selected path:
                            if (!chdir (myBaseDir))
                            {
                                if (myPathChanged)
                                {
                                    [myDefaults setObject: myBasePath forKey: DEFAULT_BASE_PATH];
                                    [myDefaults synchronize];
                                }
                                break;
                            }
                            else
                            {
                                NSRunCriticalAlertPanel (@"Can\'t change to the selected path!",
                                                        @"The selection was: \"%@\"", NULL, NULL, NULL,
                                                        myBasePath);
                            }
                        }
                    }
                }
            }
        }
        
        // if the path from the user defaults is bad, look if the id1 folder is located at the same
        // folder as our Quake application:
        if (myDefaultPath == YES)
        {
            myBasePath = [[[[NSBundle mainBundle] bundlePath] stringByDeletingLastPathComponent]
                                              stringByAppendingPathComponent: INITIAL_BASE_PATH];
            myPathChanged = YES;
            myDefaultPath = NO;
            continue;
        }
        else
        {
            // if we run for the first time or the location of the "id1" folder changed, show a dialog:
            if (myFirstRun == YES)
            {
                NSRunInformationalAlertPanel (@"You will now be asked to locate the \"id1\" folder.",
                                              @"This folder is part of the standard installation of "
                                              @"Quake. You will only be asked for it again, if you "
                                              @"change the location of this folder.",
                                              NULL, NULL, NULL);
                myFirstRun = NO;
            }
            else
            {
                if (myIDPath == NO)
                {
                    NSRunInformationalAlertPanel (@"You have not selected the \"id1\" folder.",
                                                  @"The \"id1\" folder comes with the shareware or retail "
                                                  @"version of Quake and has to contain at least the two "
                                                  @"files \"PAK0.PAK\" and \"PAK1.PAK\".",
                                                  NULL, NULL, NULL);
                }
                else
                {
                    if (myTenebraePath == NO)
                    {
                        NSRunInformationalAlertPanel (@"Your Quake folder, which holds the \"id1\" folder, "
                                                      @"does not hold the \"tenebrae\" folder.",
                                                      @"The \"tenebrae\" folder comes on the same disc image "
                                                      @"as this application. Please copy this folder to your "
                                                      @"Quake folder and try agin.",
                                                      NULL, NULL, NULL);
                    }
                }
            }
        }
        
        // request the "id1" folder:
        myResult = [myOpenPanel runModalForDirectory:nil file: nil types: nil];
        
        // if the user selected "Cancel", quit Quake:
        if (myResult == NSCancelButton)
        {
            [NSApp terminate: nil];
        }
        
        // get the selected path:
        myFolder = [myOpenPanel filenames];
        if (![myFolder count])
        {
            continue;
        }
        myBasePath = [myFolder objectAtIndex: 0];
        myPathChanged = YES;
    }
    
    // just check if the mod is located at the same folder as the id1 folder:
    if (gWasDragged && strcmp (gModDir, myBaseDir) != 0)
    {
        Sys_Error ("The modification has to be located within the same folder as the \"id1\" folder.");
    }
}

//________________________________________________________________________________________Sys_CheckSpecialKeys()

int	Sys_CheckSpecialKeys (int theKey)
{
    extern qboolean	keydown[];
    int			myKey;

    // do a fast evaluation:
    if (keydown[K_COMMAND] == false)
    {
        return (0);
    }
    
    myKey = toupper (theKey);
    
    // check the keys:
    switch (myKey)
    {
        case K_TAB:
        case 'H':
            if (gDisplayFullscreen == NO)
            {
                // hide application if windowed [CMD-TAB handled by system]:
                if (myKey == 'H')
                {
                    [NSApp hide: NULL];

                    return (1);
                }
            }
            break;
        case 'M':
            // minimize window [CMD-M]:
            if (gDisplayFullscreen == NO && gIsMinimized == NO && gWindow != NULL)
            {
                [gWindow miniaturize: NULL];

                return (1);
            }
            break;
        case 'Q':
            // application quit [CMD-Q]:
            M_Menu_Quit_f ();

            return (1);
    }

    // paste [CMD-V] already checked inside "keys.c" and "menu.c"!
    return (0);
}

//________________________________________________________________________________________________Sys_DoEvents()

void	Sys_DoEvents (NSEvent *myEvent, NSEventType myType)
{
    static NSString		*myKeyboardBuffer;
    static CGMouseDelta		myMouseDeltaX, myMouseDeltaY, myMouseWheel;
    static unichar		myCharacter;
    static UInt8		i;
    static UInt16		myKeyPad;
    static UInt32	 	myFilteredMouseButtons, myMouseButtons, myLastMouseButtons = 0, 
                                myKeyboardBufferSize, myFilteredFlags, myFlags, myLastFlags = 0;

    // we check here for events:
    switch (myType)
    {
        // mouse buttons [max. number of supported buttons is defined via MOUSE_BUTTONS]:
        case NSSystemDefined:
            CHECK_MOUSE_ENABLED ();
            
            if ([myEvent subtype] == 7)
            {
                myMouseButtons = [myEvent data2];
                myFilteredMouseButtons = myLastMouseButtons ^ myMouseButtons;
                
                for (i = 0; i < MOUSE_BUTTONS; i++)
                {
                    if(myFilteredMouseButtons & (1 << i))
                        Key_Event (K_MOUSE1 + i, (myMouseButtons & (1 << i)) ? 1 : 0);
                }
                
                myLastMouseButtons = myMouseButtons;
            }
            break;
            
        // scroll wheel:
        case NSScrollWheel:
            CHECK_MOUSE_ENABLED ();
            
            myMouseWheel = [myEvent deltaY];

            if(myMouseWheel > 0)
            {
                Key_Event (K_MWHEELUP, true);
                Key_Event (K_MWHEELUP, false);
            }
            else
            {
                Key_Event (K_MWHEELDOWN, true);
                Key_Event (K_MWHEELDOWN, false);
            }
            break;
           
        // mouse movement:
        case NSMouseMoved:
        case NSLeftMouseDragged:
        case NSRightMouseDragged:
        case 27:
            CHECK_MOUSE_ENABLED ();
            
            CGGetLastMouseDelta (&myMouseDeltaX, &myMouseDeltaY);
            IN_ReceiveMouseMove (myMouseDeltaX, myMouseDeltaY);
            break;
            
        // key up and down:
        case NSKeyDown:
        case NSKeyUp:
            myKeyboardBuffer = [myEvent charactersIgnoringModifiers];
            myKeyboardBufferSize = [myKeyboardBuffer length];
            
            for (i = 0; i < myKeyboardBufferSize; i++)
            {
                myCharacter = [myKeyboardBuffer characterAtIndex: i];
                
                if ((myCharacter & 0xFF00) ==  0xF700)
                {
                    myCharacter -= 0xF700;
                    if (myCharacter < 0x47)
                    {
                        Key_Event (gSpecialKey[myCharacter], (myType == NSKeyDown));
                    }
                }
                else
                {
                    myFlags = [myEvent modifierFlags];
                    
                    // v1.0.6: we will now check hardware codes for the keypad,
                    // otherwise we can not distinguish between between keypad numbers
                    // and normal number keys. We will only check for the keypad, if
                    // the flag for the keypad is set!
                    if (myFlags & NSNumericPadKeyMask)
                    {
                        myKeyPad = [myEvent keyCode];
            
                        if (myKeyPad < 0x5D && gNumPadKey[myKeyPad] != 0x00)
                        {
                            Key_Event (gNumPadKey[myKeyPad], (myType == NSKeyDown));
                            break;
                        }
                    }
                    if (myCharacter < 0x80)
                    {
                        if (myCharacter >= 'A' && myCharacter <= 'Z')
                            myCharacter += 'a' - 'A';
                        Key_Event (myCharacter, (myType == NSKeyDown));
                    }
                }
            }
            
            break;
        
        // special keys:
        case NSFlagsChanged:
            myFlags = [myEvent modifierFlags];
            myFilteredFlags = myFlags ^ myLastFlags;
            myLastFlags = myFlags;
            
            if (myFilteredFlags & NSAlphaShiftKeyMask)
                Key_Event (K_CAPSLOCK, (myFlags & NSAlphaShiftKeyMask) ? 1:0);
                
            if (myFilteredFlags & NSShiftKeyMask)
                Key_Event (K_SHIFT, (myFlags & NSShiftKeyMask) ? 1:0);
                
            if (myFilteredFlags & NSControlKeyMask)
                Key_Event (K_CTRL, (myFlags & NSControlKeyMask) ? 1:0);
                
            if (myFilteredFlags & NSAlternateKeyMask)
                Key_Event (K_ALT, (myFlags & NSAlternateKeyMask) ? 1:0);
                
            if (myFilteredFlags & NSCommandKeyMask)
                Key_Event (K_COMMAND, (myFlags & NSCommandKeyMask) ? 1:0);
                
            if (myFilteredFlags & NSNumericPadKeyMask)
                Key_Event (K_NUMLOCK, (myFlags & NSNumericPadKeyMask) ? 1:0);
            
            break;
        
        // ignore anything else, if not windowed:
        default:
            if (gDisplayFullscreen == NO)
            {
                [NSApp sendEvent: myEvent];
            }
            break;
    }
}

#pragma mark -

//__________________________________________________________________________________________iMPLEMENTATION_Quake

@implementation Quake : NSObject

//____________________________________________________________________________________________________initialize

+ (void) initialize
{
    NSUserDefaults	*myDefaults = [NSUserDefaults standardUserDefaults];
    NSString		*myDefaultPath = [[[[NSBundle mainBundle]
                                            bundlePath]
                                           stringByDeletingLastPathComponent]
                                          stringByAppendingPathComponent: INITIAL_BASE_PATH];

    // required for event handling:
    gDistantPast = [[NSDate distantPast] retain];
    
    // register application defaults:
    [myDefaults registerDefaults: [NSDictionary dictionaryWithObjects:
                                    [NSArray arrayWithObjects: myDefaultPath,
                                                               INITIAL_GL_DISPLAY,
                                                               INITIAL_GL_DISPLAY_MODE,
                                                               INITIAL_GL_COLORS,
                                                               INITIAL_GL_SAMPLES,
                                                               INITIAL_GL_FADE_ALL,
                                                               INITIAL_GL_FULLSCREEN,
                                                               nil]
                                    forKeys: 
                                    [NSArray arrayWithObjects: DEFAULT_BASE_PATH,
                                                               DEFAULT_GL_DISPLAY,
                                                               DEFAULT_GL_DISPLAY_MODE,
                                                               DEFAULT_GL_COLORS,
                                                               DEFAULT_GL_SAMPLES,
                                                               DEFAULT_GL_FADE_ALL,
                                                               DEFAULT_GL_FULLSCREEN,
                                                               nil]
                                  ]];
}

//_______________________________________________________________________________________________________dealloc

- (void) dealloc
{
    [gDistantPast release];
    [super dealloc];
}

//_________________________________________________________________________________________application:openFile:

- (BOOL) application: (NSApplication *) theSender openFile: (NSString *) theFilePath
{
    // allow only dragging one time:
    if (gDenyDrag == YES)
    {
        return (NO);
    }
    gDenyDrag = YES;
    
    if (gArgCount > 2)
    {
        return (NO);
    }    
    
    // we have received a filepath:
    if (theFilePath != NULL)
    {
    
        char 		*myMod  = (char *) [[theFilePath lastPathComponent] fileSystemRepresentation];
        char 		*myPath = (char *) [theFilePath fileSystemRepresentation];
        char		**myNewArgValues;
        Boolean		myDirectory;
        SInt32		i;
        
        // is the filepath a folder?
        if (![[NSFileManager defaultManager] fileExistsAtPath:theFilePath isDirectory:&myDirectory])
        {
            Sys_Error ("The dragged item is not a valid file!");
        }
        if (myDirectory == NO)
        {
            Sys_Error ("The dragged item is not a folder!");
        }
        
        // prepare the new command line options:
        myNewArgValues = malloc (sizeof(char *) * 3);
        CHECK_MALLOC (myNewArgValues);
        gArgCount = 3;
        myNewArgValues[0] = gArgValues[0];
        gArgValues = myNewArgValues;
        gArgValues[1] = GAME_COMMAND;
        gArgValues[2] = malloc (strlen (myPath) + 1);
        CHECK_MALLOC (gArgValues[2]);
        strcpy (gArgValues[2], myMod);
        
        // get the path of the mod [compare it with the id1 path later]:
        gModDir = malloc (strlen (myPath) + 1);
        CHECK_MALLOC (gModDir);
        strcpy (gModDir, myPath);
                
        // dispose the foldername of the mod:
        i = strlen (gModDir) - 1;
        while (i > 1)
        {
            if (gModDir[i - 1] == '/')
            {
                gModDir[i] = 0x00;
                break;
            }
            i--;
        }

        gWasDragged = YES;
        
        return (YES);
    }
    return (NO);
}

//________________________________________________________________________________applicationDidFinishLaunching:

- (void) applicationDidFinishLaunching: (NSNotification *) theNote
{
    // don't accept any drags from this point on!
    gDenyDrag = YES;

    // enable services:
    [NSApp setServicesProvider: self];

    // examine the "id1" folder and check if the MOD has the right location:
    Sys_CheckForIDDirectory ();

    NS_DURING
    {
        // show the settings dialog:
        [self setupDialog];
    }
    NS_HANDLER
    {
        NSString	*myException = [localException reason];
        
        if (myException == NULL)
        {
            myException = @"Unknown exception!";
        }
        NSLog (@"An error has occured: %@\n", myException);
        NSRunCriticalAlertPanel (@"An error has occured:", myException, NULL, NULL, NULL);
    }
    NS_ENDHANDLER;
}


//___________________________________________________________________________________applicationShouldTerminate:

- (NSApplicationTerminateReply) applicationShouldTerminate: (NSApplication *) theSender
{
    if (gHostInitialized == YES)
    {
        M_Menu_Quit_f ();
        return (NSTerminateCancel);
    }
    
    return (NSTerminateNow);
}

//___________________________________________________________________________________applicationDidResignActive:

- (void) applicationDidResignActive: (NSNotification *) theNote
{
    if (gHostInitialized == NO)
    {
        return;
    }
    
    IN_ShowCursor (YES);
    Sys_SetKeyboardRepeatEnabled (YES);
    gIsDeactivated = YES;
}

//___________________________________________________________________________________applicationDidBecomeActive:

- (void) applicationDidBecomeActive: (NSNotification *) theNote
{
    if (gHostInitialized == NO)
    {
        return;
    }
    
    if (gDisplayFullscreen == YES || (gMouseEnabled == YES && _windowed_mouse.value != 0.0f))
    {
        IN_ShowCursor (NO);
    }
    
    Sys_SetKeyboardRepeatEnabled (NO);
    gIsDeactivated = NO;
}

//__________________________________________________________________________________________applicationWillHide:

- (void) applicationWillHide: (NSNotification *) theNote
{
    if (gHostInitialized == NO)
    {
        return;
    }
    
    IN_ShowCursor (YES);
    Sys_SetKeyboardRepeatEnabled (YES);
    gIsHidden = YES;
}

//________________________________________________________________________________________applicationWillUnhide:

- (void) applicationWillUnhide: (NSNotification *) theNote
{
    if (gHostInitialized == NO)
    {
        return;
    }
    
    if (gDisplayFullscreen == YES || (gMouseEnabled == YES && _windowed_mouse.value != 0.0f))
    {
        IN_ShowCursor (NO);
    }
    
    Sys_SetKeyboardRepeatEnabled (NO);
    gIsHidden = NO;
    gIsDeactivated = NO;
}

//_____________________________________________________________________________________________buildDisplayList:

- (void) buildDisplayList
{
    UInt32	i;
    
    // retrieve displays list
    if (CGGetActiveDisplayList (MAX_DISPLAYS, gDisplayList, &gDisplayCount) != CGDisplayNoErr)
    {
        Sys_Error ("Can\'t build display list!");
    }
    
    // add the displays to the popup:
    [displayPopUp removeAllItems];
    for (i = 0; i < gDisplayCount; i++)
    {
        [displayPopUp addItemWithTitle: [NSString stringWithFormat: @"%d", i]];
    }

    [displayPopUp selectItemAtIndex: 0];
}

//__________________________________________________________________________________________buildResolutionList:

- (IBAction) buildResolutionList: (id) theSender
{
    CGDisplayCount	myDisplayIndex;
    CGDirectDisplayID	mySelectedDisplay;
    NSArray		*myDisplayModes;
    UInt16		myResolutionIndex = 0, myModeCount, i;
    UInt32		myColors, myDepth;
    NSDictionary 	*myCurMode;
    NSString 		*myDescription, *myOldResolution = NULL;

    // get the old resolution:
    if (gModeList != NULL)
    {
        // modelist can have a count of zero...
        if([gModeList count] > 0)
        {
            myOldResolution = [self displayModeToString: [gModeList objectAtIndex:
                                                                    [modePopUp indexOfSelectedItem]]];
        }
        [gModeList release];
        gModeList = NULL;
    }
    
    // figure out which display was selected by the user:
    myDisplayIndex = [displayPopUp indexOfSelectedItem];
    if (myDisplayIndex >= gDisplayCount)
    {
        myDisplayIndex = 0;
        [displayPopUp selectItemAtIndex: myDisplayIndex];
    }
    mySelectedDisplay = gDisplayList[myDisplayIndex];
    
    // get the list of display modes:
    myDisplayModes = [(NSArray *) CGDisplayAvailableModes (mySelectedDisplay) retain];
    CHECK_MALLOC (myDisplayModes);
    gModeList = [[NSMutableArray alloc] init];
    CHECK_MALLOC (gModeList);
    
    // filter modes:
    myModeCount = [myDisplayModes count];
    myColors = ([colorsPopUp indexOfSelectedItem] == 0) ? 16 : 32;
    for (i = 0; i < myModeCount; i++)
    {
        myCurMode = [myDisplayModes objectAtIndex: i];
        myDepth = [[myCurMode objectForKey: (NSString *) kCGDisplayBitsPerPixel] intValue];
        if (myColors == myDepth)
        {
            UInt16	j = 0;
            
            // I got double entries while testing [OSX bug?], so we have to check:
            while (j < [gModeList count])
            {
                if ([[self displayModeToString: [gModeList objectAtIndex: j]] isEqualToString:
                    [self displayModeToString: myCurMode]])
                {
                    break;
                }
                j++;
            }
            if (j == [gModeList count])
            {
                [gModeList addObject: myCurMode];
            }
        }
    }

    // Fill the popup with the resulting modes:
    [modePopUp removeAllItems];
    myModeCount = [gModeList count];
    if (myModeCount == 0)
    {
        [modePopUp addItemWithTitle: @"not available!"];
    }
    else
    {
        for (i = 0; i < myModeCount; i++)
        {
            myDescription = [self displayModeToString: [gModeList objectAtIndex: i]];
            if(myOldResolution != nil)
            {
                if([myDescription isEqualToString: myOldResolution]) myResolutionIndex = i;
            }
            [modePopUp addItemWithTitle: myDescription];
        }
    }
    [modePopUp selectItemAtIndex: myResolutionIndex];

    // last not least check for multisample buffers:
    if (GL_CheckARBMultisampleExtension (mySelectedDisplay) == YES)
    {
        [samplesPopUp setEnabled: YES];
    }
    else
    {
        [samplesPopUp setEnabled: NO];
        [samplesPopUp selectItemAtIndex: 0];
    }

    // clean up:
    if (myDisplayModes != NULL)
    {
        [myDisplayModes release];
    }
}

//__________________________________________________________________________________________________setupDialog:

- (void) setupDialog
{
    NSUserDefaults 	*myDefaults = NULL;
    NSString 		*myDescriptionStr = NULL,
                        *myDefaultModeStr = NULL;
    UInt		i = 0,
                        myDefaultDisplay,
                        myDefaultColors,
                        myDefaultSamples;

    // build the display list:
    [self buildDisplayList];

    myDefaults = [NSUserDefaults standardUserDefaults];
   
    // set up the displays popup:
    myDefaultDisplay = [[myDefaults stringForKey: DEFAULT_GL_DISPLAY] intValue];
    if (myDefaultDisplay > gDisplayCount)
    {
        myDefaultDisplay = 0;
    }
    [displayPopUp selectItemAtIndex: myDefaultDisplay];
    
    // set up the colors popup:
    myDefaultColors = [[myDefaults stringForKey: DEFAULT_GL_COLORS] intValue];
    if (myDefaultColors > 1)
    {
        myDefaultDisplay = 1;
    }
    [colorsPopUp selectItemAtIndex: myDefaultColors];

    // build the resolution list:
    [self buildResolutionList: nil];

    // setup the modes popup:
    myDefaultModeStr = [myDefaults stringForKey: DEFAULT_GL_DISPLAY_MODE];
    while (i < [gModeList count])
    {
        myDescriptionStr = [self displayModeToString: [gModeList objectAtIndex: i]];
        if ([myDefaultModeStr isEqualToString: myDescriptionStr])
        {
            [modePopUp selectItemAtIndex: i];
            break;
        }
        i++;
    }
    
    // setup the samples popup:
    myDefaultSamples = ([[myDefaults stringForKey: DEFAULT_GL_SAMPLES] intValue]) >> 2;
    if (myDefaultSamples > 2)
        myDefaultSamples = 2;
    if ([samplesPopUp isEnabled] == NO)
        myDefaultSamples = 0;
    [samplesPopUp selectItemAtIndex: myDefaultSamples];
    
    // setup checkboxes:
    [fadeAllCheckBox setState: [myDefaults boolForKey: DEFAULT_GL_FADE_ALL]];
    [fullscreenCheckBox setState: [myDefaults boolForKey: DEFAULT_GL_FULLSCREEN]];
    
    // setup the window:
    [settingsWindow center];
    [settingsWindow makeKeyAndOrderFront: nil];

#ifdef BETA_VERSION

    NSBeginInformationalAlertSheet (@"This is a public beta version of TenebraeQuake. "
                                    @"Since we are only able to test the game with an ATI Radeon/PCI, "
                                    @"we need your feedback on compatibility with NVidia boards.",
                                    NULL, NULL, NULL, settingsWindow, NULL, NULL, NULL, NULL,
                                    @"Please send bug reports to \"awe@fruitz-of-dojo.de\"!");

#endif /* BETA_VERSION */
}

//_________________________________________________________________________________saveCheckBox:initial:default:

- (void) saveCheckBox: (NSButton *) theButton initial: (NSString *) theInitial
              default: (NSString *) theDefault userDefaults: (NSUserDefaults *) theUserDefaults
{
    // has our checkbox the initial value? if, delete from defaults::
    if ([theButton state] == [self isEqualTo: theInitial])
    {
        [theUserDefaults removeObjectForKey: theDefault];
    }
    else
    {
        // write to defaults:
        if ([theButton state] == YES)
        {
            [theUserDefaults setObject: @"YES" forKey: theDefault];
        }
        else
        {
            [theUserDefaults setObject: @"NO" forKey: theDefault];
        }
    }
}

//___________________________________________________________________________________saveString:initial:default:

- (void) saveString: (NSString *) theString initial: (NSString *) theInitial
            default: (NSString *) theDefault userDefaults: (NSUserDefaults *) theUserDefaults
{
    // has our popup menu the initial value? if, delete from defaults:
    if ([theString isEqualToString: theInitial])
    {
        [theUserDefaults removeObjectForKey: theDefault];
    }
    else
    {
        // write to defaults:
        [theUserDefaults setObject: theString forKey: theDefault];
    }
}

//______________________________________________________________________________________________________newGame:

- (void) newGame: (id) theSender
{
    NSUserDefaults	*myDefaults = [NSUserDefaults standardUserDefaults];
    NSString		*myModeStr;
    UInt		myMode, myColors;

    // check if display modes are available:
    if ([gModeList count] == 0)
    {
        NSBeginAlertSheet (@"No display modes available!", NULL, NULL, NULL, settingsWindow, NULL,
                           NULL, NULL, NULL, @"Please try other displays and/or color settings.");
        return;
    }

    // save the display:
    gDisplay = [displayPopUp indexOfSelectedItem];
    [self saveString: [NSString stringWithFormat: @"%d", gDisplay] initial: INITIAL_GL_DISPLAY
                                                                   default: DEFAULT_GL_DISPLAY
                                                              userDefaults: myDefaults];

    // save the display mode:
    myMode = [modePopUp indexOfSelectedItem];
    myModeStr = [self displayModeToString: [gModeList objectAtIndex: myMode]];
    [self saveString: myModeStr initial: INITIAL_GL_DISPLAY_MODE default: DEFAULT_GL_DISPLAY_MODE
                                                            userDefaults: myDefaults];
    
    // save the colors:
    myColors = [colorsPopUp indexOfSelectedItem];
    [self saveString: [NSString stringWithFormat: @"%d", myColors] initial: INITIAL_GL_COLORS
                                                                   default: DEFAULT_GL_COLORS
                                                              userDefaults: myDefaults];
    
    // save the samples:
    if ([samplesPopUp isEnabled] == YES)
        gARBMultiSamples = [samplesPopUp indexOfSelectedItem] << 2;
    else
        gARBMultiSamples = 0;
    [self saveString: [NSString stringWithFormat: @"%d", gARBMultiSamples] initial: INITIAL_GL_SAMPLES
                                                                           default: DEFAULT_GL_SAMPLES
                                                                      userDefaults: myDefaults];
    
    // save the state of the "fade all" checkbox:
    [self saveCheckBox: fadeAllCheckBox initial: INITIAL_GL_FADE_ALL default: DEFAULT_GL_FADE_ALL
                                                                userDefaults: myDefaults];
    
    // save the state of the "fullscreen" checkbox:
    [self saveCheckBox: fullscreenCheckBox initial: INITIAL_GL_FULLSCREEN
                                           default: DEFAULT_GL_FULLSCREEN
                                      userDefaults: myDefaults];
    
    // synchronize prefs and start Quake:
    [myDefaults synchronize];
    [settingsWindow close];
    gDisplayMode = [gModeList objectAtIndex: myMode];
    gDisplayFullscreen = [fullscreenCheckBox state];
    gFadeAllDisplays = [fadeAllCheckBox state];

    [self startQuake];
}

//____________________________________________________________________________________________________isEqualTo:

- (Boolean) isEqualTo: (NSString *) theString
{
    // just some boolean str compare:
    if([theString isEqualToString:@"YES"]) return(YES);
        return(NO);
}

//__________________________________________________________________________________________displayModeToString:

- (NSString *) displayModeToString: (NSDictionary *) theDisplayMode
{
    // generate a display mode string with the format: "(width)x(height) (frequency)Hz":
    return ([NSString stringWithFormat: @"%dx%d %dHz",
                      [[theDisplayMode objectForKey: (NSString *)kCGDisplayWidth] intValue],
                      [[theDisplayMode objectForKey: (NSString *)kCGDisplayHeight] intValue],
                      [[theDisplayMode objectForKey: (NSString *)kCGDisplayRefreshRate] intValue]]);
}

//_______________________________________________________________________________connectToServer:userData:error:

- (void) connectToServer: (NSPasteboard *) thePasteboard userData:(NSString *)theData
                   error: (NSString **) theError
{
    NSArray 	*myPasteboardTypes;

    myPasteboardTypes = [thePasteboard types];

    if ([myPasteboardTypes containsObject: NSStringPboardType])
    {
        NSString 	*myRequestedServer;

        myRequestedServer = [thePasteboard stringForType: NSStringPboardType];
        if (myRequestedServer != NULL)
        {
            snprintf (gRequestedServer, 128, "connect %s\n", [myRequestedServer cString]);
            
            // required because of the GLQuakeWorld options dialog.
            // we have to wait with the command until host_init() is finished!
            if (gHostInitialized == YES)
            {
                Cbuf_AddText (gRequestedServer);
            }
            return;
        }
    }
    *theError = @"Unable to connect to a server: could not find a string on the pasteboard!";
}

//__________________________________________________________________________________________________pasteString:

- (IBAction) pasteString: (id) theSender
{
    extern qboolean	keydown[];
    qboolean		myOldCommand,
                        myOldVKey;

    // get the old state of the paste keys:
    myOldCommand = keydown[K_COMMAND];
    myOldVKey = keydown['v'];

    // send the keys required for paste:
    keydown[K_COMMAND] = true;
    Key_Event ('v', true);

    // set the old state of the paste keys:
    Key_Event ('v', false);
    keydown[K_COMMAND] = myOldCommand;
}

//_____________________________________________________________________________________________________visitFOD:

- (IBAction) visitFOD: (id) theSender
{
    [self performOpenURL: FRUITZ_OF_DOJO_URL];
}

//_______________________________________________________________________________________________performOpenURL:

- (void) performOpenURL: (NSString *) theURL
{
    NSTask		*openURL;

    if((openURL = [[NSTask alloc] init]))
    {
        [openURL setLaunchPath: @"/usr/bin/osascript"];
        [openURL setCurrentDirectoryPath: [@"~/" stringByExpandingTildeInPath]];
        [openURL setArguments: [NSArray arrayWithObjects:@"-e",
                                        [NSString stringWithFormat:@"open location \"%@\"", theURL],
                                        NULL]];
        [openURL launch];
    }
    else
    {
        NSRunAlertPanel ([NSString stringWithFormat:@"Can\'t launch the URL:\n \"%@\".", theURL],
                         @"Reason: Task initialization failed.", NULL, NULL, NULL);
    }
    
    [openURL release];
}

//___________________________________________________________________________________________________startQuake:

- (void) startQuake
{
    extern SInt32		vcrFile;
    extern SInt32		recording;

    quakeparms_t    		myParameters;
    SInt32			j;
    NSEvent 			*myEvent;
    NSAutoreleasePool 		*myPool = NULL;
    double			myTime, myOldtime, myNewtime;

    // prepare host init:
    signal (SIGFPE, SIG_IGN);
    memset (&myParameters, 0x00, sizeof (myParameters));

    COM_InitArgv (gArgCount, gArgValues);

    myParameters.argc = com_argc;
    myParameters.argv = com_argv;

    myParameters.memsize = 32*1024*1024;
    j = COM_CheckParm ("-mem");
    if (j)
    {
        myParameters.memsize = (int) (Q_atof (com_argv[j + 1]) * 1024 * 1024);
    }
    
    myParameters.membase = malloc (myParameters.memsize);
    myParameters.basedir = QUAKE_BASE_PATH;
    
    fcntl (0, F_SETFL, fcntl (0, F_GETFL, 0) | FNDELAY);
    
    Host_Init (&myParameters);
    gHostInitialized = YES;

    if(COM_CheckParm ("-nostdout"))
    {
        gNoStdOut = 1;
    }

    // disable keyboard repeat:
    Sys_SetKeyboardRepeatEnabled (NO);

    // some nice console output for credits:
    Con_Printf ("\n€‚\n");
    Con_Printf (" TenebraeQuake for MacOS X - v%0.2f\n", MACOSX_VERSION);
    Con_Printf ("   Ported by: awe^fruitz of dojo\n");
    Con_Printf ("Visit: http://www.fruitz-of-dojo.de\n");
    Con_Printf ("\n   tiger style kung fu is strong\n");
    Con_Printf ("  but our style is more effective!\n");
    Con_Printf ("€‚\n\n");

    myOldtime = Sys_FloatTime ();

    if (gRequestedServer[0] != 0x0)
    {
        Cbuf_AddText (gRequestedServer);
    }

    // our main loop:
    while (1)
    {
        if (gIsHidden == YES)
        {
            S_StopAllSounds (YES);
            while (gIsHidden == YES)
            {
                Sys_Sleep ();
            }
            Key_Event (K_COMMAND, 0);
        }
    
        myPool = [[NSAutoreleasePool alloc] init];
        myEvent = [NSApp nextEventMatchingMask: NSAnyEventMask
                                     untilDate: gDistantPast
                                        inMode: NSDefaultRunLoopMode
                                       dequeue: YES];

        // do a frame if we didn't receive an event:
        if (!myEvent)
        {
            [myPool release];

            myNewtime = Sys_FloatTime();
            myTime = myNewtime - myOldtime;
            
            if (cls.state == ca_dedicated)
            {
                if (myTime < sys_ticrate.value && (vcrFile == -1 || recording))
                {
                    usleep(1);
                    continue;
                }
                myTime = sys_ticrate.value;
            }
            
            if (myTime > sys_ticrate.value * 2)
            {
                myOldtime = myNewtime;
            }
            else
            {
                myOldtime += myTime;
            }
            
            // finally do the frame:
            Host_Frame (myTime);
        }
        else
        {
            // do events:
            Sys_DoEvents (myEvent, [myEvent type]);

            [myPool release];
        }
    }
}

@end

#pragma mark -

//________________________________________________________________________________________________________main()

SInt	main (SInt theArgCount, const char **theArgValues)
{
    // just startup the application [if we have no commandline app]:
    gArgCount = theArgCount;
    gArgValues = (char **) theArgValues;
    
    return (NSApplicationMain (theArgCount, theArgValues));
}

//___________________________________________________________________________________________________________eOF
