//___________________________________________________________________________________________________________nFO
// "gl_vidosx.m" - MacOS X OpenGL Video driver
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
// v1.0.8: Fixed an issue with the console aspect, if apspect ratio was not 4:3.
//	   Added support for gl_arb_multisample.
// v1.0.7: Brings back the video options menu.
//	   "vid_wait" now availavle via video options.
//	   ATI Radeon only:
//	   Added support for FSAA, via variable "gl_fsaa" or video options.
//	   Added support for Truform, via variable "gl_truform" or video options.
//	   Added support for anisotropic texture filtering, via variable "gl_anisotropic" or options.
// v1.0.5: Added "minimized in Dock" mode.
//         Displays are now catured manually due to a bug with CGReleaseAllDisplays().
//	   Reduced the fade duration to 1.0s.
// v1.0.4: Fixed continuous console output, if gamma setting fails.
//	   Fixed a multi-monitor issue.
// v1.0.3: Enables setting the gamma via the brightness slider at the options dialog.
//	   Enable/Disable VBL syncing via "vid_wait".
// v1.0.2: GLQuake/GLQuakeWorld:
//	   Fixed a performance issue [see "gl_rsurf.c"].
//         Default value of "gl_keeptjunctions" is now "1" [see "gl_rmain.c"].
//	   Added "DrawSprocket" style gamma fading at game start/end.
//         Some internal changes.
//         GLQuakeWorld:
//         Fixed console width/height bug with resolutions other than 640x480 [was always 640x480].
// v1.0.1: Initial release.
//______________________________________________________________________________________________________iNCLUDES

#pragma mark =Includes=

#include	"quakedef.h"

#import		<AppKit/AppKit.h>
#import 	<IOKit/graphics/IOGraphicsTypes.h>
#import 	<mach-o/dyld.h>
#import 	<OpenGL/OpenGL.h>
#import 	<OpenGL/gl.h>
#import 	<OpenGL/glu.h>
#import 	<OpenGL/glext.h>

#pragma mark -

//_______________________________________________________________________________________________________dEFINES

#pragma mark =Defines=

#define HWA_OPENGL_ONLY				// allow only systems with HWA OpenGL.
#define	USE_BUFFERED_WINDOW			// the appearance of buffered windows is nicer.
#define  ALLOW_MULTITEXTURE_EXTENSION		// doesn't seem to work with current MacOS X [10.1].

#define FADE_DURATION		1.0f

#define	MAX_DISPLAYS		100		// max displays for gamma fading.

#define WARP_WIDTH              320
#define WARP_HEIGHT             200

#define CONSOLE_MIN_WIDTH	320
#define CONSOLE_MIN_HEIGHT	200

#define ATI_FSAA_LEVEL		510		// required for CGLSetParamter () to enable FSAA.

#define FONT_WIDTH		8
#define	FONT_HEIGHT		8
#define FIRST_MENU_LINE		40

#pragma mark -

//________________________________________________________________________________________________________mACROS

#pragma mark =Macros=

#define	SQUARE(A)		((A) * (A))

#pragma mark -

//_________________________________________________________________________________________________________eNUMS

#pragma mark =Enums=

enum {
        VIDEO_WAIT,
        VIDEO_FSAA,
        VIDEO_ANISOTROPIC,
        VIDEO_TRUFORM
     };

#pragma mark -

//__________________________________________________________________________________________________________mISC

#pragma mark =TypeDefs=

typedef unsigned int 	   	UInt;
typedef signed int         	SInt;

typedef struct 		{
                                CGDirectDisplayID 	displayID;
                                CGGammaValue		component[9];
                        }	osxCGGammaStruct;
                
typedef struct		{
                                UInt16		Width;
                                UInt16		Height;
                                float		Refresh;
                                char		Desc[32];
                        }	osxCGModeListStruct;

#pragma mark -

//____________________________________________________________________________________________________iNTERFACES

#pragma mark =ObjC Interfaces=

@interface NSOpenGLContext (CGLContextAccess)
- (CGLContextObj) cglContext;
@end

@interface QuakeView : NSView
@end

#pragma mark -

//_____________________________________________________________________________________________________vARIABLES

#pragma mark =Variables=

const char			*gl_renderer,
                                *gl_vendor,
                                *gl_version,
                                *gl_extensions;

extern qboolean			gMouseEnabled;

cvar_t				vid_mode = { "vid_mode", "0", 0 };
cvar_t				vid_redrawfull = { "vid_redrawfull", "0", 0 };
cvar_t				vid_wait = { "vid_wait", "1", 1 };
cvar_t				_vid_default_mode = { "_vid_default_mode", "0", 1 };
cvar_t				_vid_default_blit_mode = { "_vid_default_blit_mode", "0", 1 };
cvar_t				_windowed_mouse = { "_windowed_mouse","0", 0 };
cvar_t				gl_anisotropic = { "gl_anisotropic", "0", 1 };
cvar_t				gl_fsaa = { "gl_fsaa", "1", 0 };
cvar_t				gl_truform = { "gl_truform", "-1", 1 };
cvar_t				gl_ztrick = { "gl_ztrick", "1" };

unsigned			d_8to24table[256];
unsigned char			d_15to8table[65536];
unsigned char 			d_8to8graytable[256];

NSDictionary			*gDisplayMode;
CGDirectDisplayID  		gDisplayList[MAX_DISPLAYS];
CGDisplayCount			gDisplayCount;
NSWindow			*gWindow;
qboolean			gDisplayFullscreen,
                                isPermedia = NO,
                                gl_palettedtex = NO,
                                gl_nvcombiner = NO,
                                gl_geforce3 = NO,
                                gl_radeon = NO,
                                gl_var = NO,
                                gl_fsaaavailable = NO,
                                gl_mtexable = NO,
                               	gl_pntriangles = NO,
                                gl_texturefilteranisotropic = NO;
Boolean				gFadeAllDisplays,
                                gWindowedMouse = NO,
                                gIsMinimized = NO;
UInt				gDisplay;
SInt				texture_extension_number = 1,
				texture_mode = GL_LINEAR,
                                gARBMultiSamples = 0;
float				gWindowPosX,
                                gWindowPosY;
GLfloat				gldepthmin,
                                gldepthmax,
                                gl_texureanisotropylevel = 1.0f;
void *				AGP_Buffer = NULL;

static const float 		gGLTruformAmbient[4] = { 1.0f, 1.0f, 1.0f, 1.0f };

static CGDirectDisplayID	gCapturedDisplayList[MAX_DISPLAYS];
static CGDisplayCount           gCapturedDisplayCount = 0;
static NSDictionary		*gOriginalMode;
static NSOpenGLContext		*gGLContext;
static osxCGGammaStruct		*gOriginalGamma = NULL;
static QuakeView		*gWindowView = NULL;
static NSImage			*gMiniWindow = NULL,
                                *gOldIcon = NULL;
static Boolean			gDisplayIs8Bit,
                                gGLAnisotropic = NO;
static UInt			gDisplayWidth, 
                                gDisplayHeight,
                                gDisplayDepth;
static UInt32			*gMiniBuffer = NULL;
static float 			vid_gamma = 1.0f,
                                gVidWait = 0.0f,
                                gFSAALevel = 1.0f,
                                gPNTriangleLevel = -1.0f;
static SInt8	    		gMenuMaxLine,
                                gMenuLine = FIRST_MENU_LINE,
                                gMenuItem = VIDEO_WAIT;

#pragma mark -

//___________________________________________________________________________________________fUNCTION_pROTOTYPES

#pragma mark =Function ProtoTypes =

extern 	void 		M_Menu_Options_f (void);
extern 	void 		M_Print (int cx, int cy, char *str);
extern 	void 		M_DrawCharacter (int cx, int line, int num);
extern 	void 		M_DrawPic (int x, int y, qpic_t *pic);
extern	void		IN_ShowCursor (Boolean);
extern	double		Sys_FloatTime (void);
extern	void		GL_CreateShadersRadeon (void);
extern	qboolean	GL_LookupRadeonSymbols (void);

static	void		VID_CheckGamma (unsigned char *);
static	Boolean		VID_CaptureDisplays (Boolean);
static	Boolean		VID_ReleaseDisplays (Boolean);
static  Boolean		VID_FadeGammaInit (Boolean);
static	void		VID_FadeGammaOut (Boolean, float);
static	void		VID_FadeGammaIn (Boolean, float);
static	void		VID_SetWait (UInt32);
static	Boolean		VID_SetDisplayMode (void);
static	void		VID_RestoreOldIcon (void);
static	void 		VID_MenuDraw (void);
static	void		VID_MenuKey (int theKey);

Boolean 		GL_CheckARBMultisampleExtension (CGDirectDisplayID theDisplay);
void *			GL_GetProcAddress (const char *theName, qboolean theSafeMode);

static	void		GL_CheckPalettedTexture (void);
static	void		GL_CheckMultiTextureExtensions (void);
static	void		GL_CheckDiffuseBumpMappingExtensions (void);
static	void		GL_CheckSpecularBumpMappingExtensions (void);
static	void		GL_CheckGeforce3Extensions (void);
static	void		GL_CheckRadeonExtensions (void);
static	void		GL_CheckVertexArrayRange (void);
static	void		GL_CheckPNTrianglesExtensions (void);
static	void		GL_CheckSwitchFSAAOnTheFly (void);
static	void		GL_CheckTextureFilterAnisotropic (void);
static	void		GL_Init (void);
static	void		GL_SetFSAA (UInt32 theFSAALevel);
static	void		GL_RenderInsideDock (void);

#pragma mark -

//______________________________________________________________________________________________VID_LockBuffer()

#ifdef QUAKE_WORLD

void 	VID_LockBuffer (void)
{
}

#endif /* QUAKE_WORLD */

//____________________________________________________________________________________________VID_UnlockBuffer()

#ifdef QUAKE_WORLD

void	VID_UnlockBuffer (void)
{
}

#endif /* QUAKE_WORLD */

//__________________________________________________________________________________________________VID_Is8bit()

qboolean VID_Is8bit (void)
{
    return (gDisplayIs8Bit);
}

//______________________________________________________________________________________________VID_CheckGamma()

void	VID_CheckGamma (unsigned char *thePalette)
{
    float		myNewValue;
    unsigned char	myPalette[768];
    SInt		i;

    if ((i = COM_CheckParm ("-gamma")) == 0)
    {
        if ((gl_renderer && strstr (gl_renderer, "Voodoo")) ||
           (gl_vendor && strstr (gl_vendor, "3Dfx")))
        {
            vid_gamma = 1.0f;
        }
	else
        {
            vid_gamma = 0.6f;
        }
    }
    else
    {
	vid_gamma = Q_atof (com_argv[i+1]);
    }
  
    for (i = 0 ; i < 768 ; i++)
    {
        myNewValue = pow ((thePalette[i] + 1) / 256.0f, vid_gamma) * 255 + 0.5f;
        
	if (myNewValue < 0.0f)
            myNewValue = 0.0f;
            
        if (myNewValue > 255.0f)
            myNewValue = 255.0f;
            
	myPalette[i] = (unsigned char) myNewValue;
    }
    
    memcpy (thePalette, myPalette, sizeof (myPalette));
}

//______________________________________________________________________________________________VID_SetPalette()

void	VID_SetPalette (UInt8 *thePalette)
{
    UInt		myRedComp, myGreenComp, myBlueComp,
                        myColorValue, myBestValue,
                        *myColorTable;
    SInt		myNewRedComp, myNewGreenComp, myNewBlueComp,
                        myCurDistance, myBestDistance;
    UInt16		i;
    UInt8		*myPalette,
                        *myShadeTable;

    myPalette	 = thePalette;
    myShadeTable = d_8to8graytable;
    myColorTable = d_8to24table;
    
    for (i = 0; i < 256; i++)
    {
        myRedComp       = myPalette[0];
        myGreenComp     = myPalette[1];
	myBlueComp      = myPalette[2];

	myColorValue    = (myRedComp << 24) + (myGreenComp << 16) + (myBlueComp << 8) + (0xFF << 0);

	*myColorTable++ = myColorValue;
        *myShadeTable++ = (myRedComp + myGreenComp + myBlueComp) / 3;
	myPalette       += 3;
    }
    
    d_8to24table[255] &= 0xffffff00;
    
    for (i = 0; i < (1 << 15); i++)
    {
        myRedComp   = ((i & 0x001F) << 3) + 4;
	myGreenComp = ((i & 0x03E0) >> 2) + 4;
	myBlueComp  = ((i & 0x7C00) >> 7) + 4;
        
	myPalette   = (UInt8 *) d_8to24table;
        
	for (myColorValue = 0 , myBestValue = 0, myBestDistance = SQUARE(10000);
             myColorValue < 256;
             myColorValue++, myPalette += 4)
        {
            myNewRedComp   = (SInt) myRedComp   - (SInt) myPalette[0];
            myNewGreenComp = (SInt) myGreenComp - (SInt) myPalette[1];
            myNewBlueComp  = (SInt) myBlueComp  - (SInt) myPalette[2];
            
            myCurDistance  = SQUARE(myNewRedComp) + SQUARE(myNewGreenComp) + SQUARE(myNewBlueComp);
            
            if (myCurDistance < myBestDistance)
            {
                myBestValue = myColorValue;
		myBestDistance = myCurDistance;
            }
	}
	d_15to8table[i] = myBestValue;
    }
}

//____________________________________________________________________________________________VID_ShiftPalette()

void	VID_ShiftPalette (UInt8 *thePalette)
{
    // nothing to do here...
}

//_________________________________________________________________________________________________VID_SetMode()

SInt 	VID_SetMode (SInt modenum, UInt8 *thePalette)
{
    return (1);
}

//_________________________________________________________________________________________VID_CaptureDisplays()

Boolean	VID_CaptureDisplays (Boolean theCaptureAllDisplays)
{
    CGDisplayErr	myError;
    SInt16		i;
    
    if (theCaptureAllDisplays == NO)
    {
        if (CGDisplayIsCaptured (gDisplayList[gDisplay]) == NO)
        {
            myError = CGDisplayCapture (gDisplayList[gDisplay]);
            if (myError != CGDisplayNoErr)
            {
                return (NO);
            }
        }
        return (YES);
    }

    // are the displays already captured?
    if (gCapturedDisplayCount != 0)
    {
        return (YES);
    }

    // we have to loop manually thru each display, since there is a bug with CGReleaseAllDisplays().
    // [only active displays will be released and not all captured!]

    // get the active display list:
    myError = CGGetActiveDisplayList (MAX_DISPLAYS, gCapturedDisplayList, &gCapturedDisplayCount);
    if (myError != CGDisplayNoErr || gCapturedDisplayCount == 0)
    {
        gCapturedDisplayCount = 0;

        return (NO);
    }
    
    // capture each active display:
    for (i = 0; i < gCapturedDisplayCount; i++)
    {
        myError = CGDisplayCapture (gCapturedDisplayList[i]);
        
        if (myError != CGDisplayNoErr)
        {
            for (; i >= 0; i--)
            {
                CGDisplayRelease (gCapturedDisplayList[i]);
            }

            gCapturedDisplayCount = 0;

            return (NO);
        }
    }
    
    return (YES);
}

//_________________________________________________________________________________________VID_ReleaseDisplays()

Boolean	VID_ReleaseDisplays (Boolean theCaptureAllDisplays)
{
    CGDisplayErr	myError;
    SInt16		i;
    
    if (theCaptureAllDisplays == NO)
    {
        if (CGDisplayIsCaptured (gDisplayList[gDisplay]) == YES)
        {
            myError = CGDisplayRelease (gDisplayList[gDisplay]);
            if (myError != CGDisplayNoErr)
            {
                return (NO);
            }
        }
        return (YES);
    }

    // are the displays already released?
    if (gCapturedDisplayCount == 0)
    {
        return (YES);
    }
    
    // release each captured display:
    for (i = 0; i < gCapturedDisplayCount; i++)
    {
        CGDisplayRelease (gCapturedDisplayList[i]);
    }
    
    gCapturedDisplayCount = 0;
    return (YES);
}

//___________________________________________________________________________________________VID_FadeGammaInit()

Boolean	VID_FadeGammaInit (Boolean theFadeOnAllDisplays)
{
    static Boolean	myFadeOnAllDisplays;
    CGDisplayErr	myError;
    UInt32		i;

    // if init fails, no gamma fading will be used!    
    if (gOriginalGamma != NULL)
    {
        // initialized, but did we change the number of displays?
        if (theFadeOnAllDisplays == myFadeOnAllDisplays)
        {
            return (YES);
        }
        free (gOriginalGamma);
        gOriginalGamma = NULL;
    }

    // get the list of displays, in case something bad is going on:
    if (gDisplayList == NULL || gDisplayCount == 0)
    {
        return (NO);
    }
    
    if (gDisplay > gDisplayCount)
    {
        gDisplay = 0;
    }
    
    if (theFadeOnAllDisplays == NO)
    {
        gDisplayList[0] = gDisplayList[gDisplay];
        gDisplayCount = 1;
        gDisplay = 0;
    }
    
    // get memory for our original gamma table(s):
    gOriginalGamma = malloc (sizeof (osxCGGammaStruct) * gDisplayCount);
    if (gOriginalGamma == NULL)
    {
        return (NO);
    }
    
    // store the original gamma values within this table(s):
    for (i = 0; i < gDisplayCount; i++)
    {
        if (gDisplayCount == 1)
        {
            gOriginalGamma[i].displayID = gDisplayList[gDisplay];
        }
        else
        {
            gOriginalGamma[i].displayID = gDisplayList[i];
        }
        
        myError = CGGetDisplayTransferByFormula (gOriginalGamma[i].displayID,
                                                 &gOriginalGamma[i].component[0],
                                                 &gOriginalGamma[i].component[1],
                                                 &gOriginalGamma[i].component[2],
                                                 &gOriginalGamma[i].component[3],
                                                 &gOriginalGamma[i].component[4],
                                                 &gOriginalGamma[i].component[5],
                                                 &gOriginalGamma[i].component[6],
                                                 &gOriginalGamma[i].component[7],
                                                 &gOriginalGamma[i].component[8]);
        if (myError != CGDisplayNoErr)
        {
            free (gOriginalGamma);
            gOriginalGamma = NULL;
            return (NO);
        }
    }

    myFadeOnAllDisplays = theFadeOnAllDisplays;

    return (YES);
}

//____________________________________________________________________________________________VID_FadeGammaOut()

void	VID_FadeGammaOut (Boolean theFadeOnAllDisplays, float theDuration)
{
    osxCGGammaStruct	myCurGamma;
    float		myStartTime = 0.0f, myCurScale = 0.0f;
    UInt32		i, j;

    // check if initialized:
    if (VID_FadeGammaInit (theFadeOnAllDisplays) == NO)
    {
        return;
    }
    
    // get the time of the fade start:
    myStartTime = Sys_FloatTime ();
    
    // fade for the choosen duration:
    while (1)
    {
        // calculate the current scale and clamp it:
        if (theDuration > 0.0f)
        {
            myCurScale = 1.0f - (Sys_FloatTime () - myStartTime) / theDuration;
            if (myCurScale < 0.0f)
            {
                myCurScale = 0.0f;
            }
        }

        // fade the gamma for each display:        
        for (i = 0; i < gDisplayCount; i++)
        {
            // calculate the current intensity for each color component:
            for (j = 1; j < 9; j += 3)
            {
                myCurGamma.component[j] = myCurScale * gOriginalGamma[i].component[j];
            }

            // set the current gamma:
            CGSetDisplayTransferByFormula (gOriginalGamma[i].displayID,
                                           gOriginalGamma[i].component[0],
                                           myCurGamma.component[1],
                                           gOriginalGamma[i].component[2],
                                           gOriginalGamma[i].component[3],
                                           myCurGamma.component[4],
                                           gOriginalGamma[i].component[5],
                                           gOriginalGamma[i].component[6],
                                           myCurGamma.component[7],
                                           gOriginalGamma[i].component[8]);
        }
        
        // are we finished?
        if(myCurScale <= 0.0f)
        {
            break;
        } 
    }
}

//_____________________________________________________________________________________________VID_FadeGammaIn()

void	VID_FadeGammaIn (Boolean theFadeOnAllDisplays, float theDuration)
{
    osxCGGammaStruct	myCurGamma;
    float		myStartTime = 0.0f, myCurScale = 1.0f;
    UInt32		i, j;

    // check if initialized:
    if (gOriginalGamma == NULL)
    {
        return;
    }
    
    // get the time of the fade start:
    myStartTime = Sys_FloatTime ();
    
    // fade for the choosen duration:
    while (1)
    {
        // calculate the current scale and clamp it:
        if (theDuration > 0.0f)
        {
            myCurScale = (Sys_FloatTime () - myStartTime) / theDuration;
            if (myCurScale > 1.0f)
            {
                myCurScale = 1.0f;
            }
        }

        // fade the gamma for each display:
        for (i = 0; i < gDisplayCount; i++)
        {
            // calculate the current intensity for each gamma component:
            for (j = 1; j < 9; j += 3)
            {
                myCurGamma.component[j] = myCurScale * gOriginalGamma[i].component[j];
            }

            // set the current gamma:
            CGSetDisplayTransferByFormula (gOriginalGamma[i].displayID,
                                           gOriginalGamma[i].component[0],
                                           myCurGamma.component[1],
                                           gOriginalGamma[i].component[2],
                                           gOriginalGamma[i].component[3],
                                           myCurGamma.component[4],
                                           gOriginalGamma[i].component[5],
                                           gOriginalGamma[i].component[6],
                                           myCurGamma.component[7],
                                           gOriginalGamma[i].component[8]);
        }
        
        // are we finished?
        if(myCurScale >= 1.0f)
        {
            break;
        } 
    }
}

//_____________________________________________________________________________________VID_CreateGLPixelFormat()

NSOpenGLPixelFormat *	VID_CreateGLPixelFormat (void)
{
    NSOpenGLPixelFormat			*myPixelFormat;
    NSOpenGLPixelFormatAttribute	myAttributeList[32];
    UInt				i = 0;

    if (gARBMultiSamples > 0)
    {
        if (gARBMultiSamples > 8)
            gARBMultiSamples = 8;
        myAttributeList[i++] = NSOpenGLPFASampleBuffers;
        myAttributeList[i++] = 1;
        myAttributeList[i++] = NSOpenGLPFASamples;
        myAttributeList[i++] = gARBMultiSamples;
    }

    myAttributeList[i++] = NSOpenGLPFANoRecovery;
    myAttributeList[i++] = NSOpenGLPFAClosestPolicy;
    myAttributeList[i++] = NSOpenGLPFAAccelerated;
    myAttributeList[i++] = NSOpenGLPFADoubleBuffer;

    myAttributeList[i++] = NSOpenGLPFAColorSize;
    myAttributeList[i++] = gDisplayDepth;

    myAttributeList[i++] = NSOpenGLPFADepthSize;
    myAttributeList[i++] = 24;

    myAttributeList[i++] = NSOpenGLPFAAlphaSize;
    myAttributeList[i++] = 8;
    
    myAttributeList[i++] = NSOpenGLPFAStencilSize;
    myAttributeList[i++] = 8;

    myAttributeList[i++] = NSOpenGLPFAAccumSize;
    myAttributeList[i++] = 0;
                
    if (gDisplayFullscreen == YES)
    {
        myAttributeList[i++] = NSOpenGLPFAFullScreen;
        myAttributeList[i++] = NSOpenGLPFAScreenMask;
        myAttributeList[i++] = CGDisplayIDToOpenGLDisplayMask (gDisplayList[gDisplay]);
    }
    else
    {
        myAttributeList[i++] = NSOpenGLPFAWindow;
    }
    
    myAttributeList[i++] = 0;
  
    myPixelFormat = [[NSOpenGLPixelFormat alloc] initWithAttributes: myAttributeList];

    return (myPixelFormat);
}

//_________________________________________________________________________________________________VID_SetWait()

void VID_SetWait (UInt32 theState)
{
    // set theState to 1 to enable, to 0 to disable VBL syncing.
    [gGLContext makeCurrentContext];
    if(CGLSetParameter (CGLGetCurrentContext (), kCGLCPSwapInterval, &theState) == CGDisplayNoErr)
    {
        gVidWait = vid_wait.value;
        if (theState == 0)
        {
            Con_Printf ("video wait successfully disabled!\n");
        }
        else
        {
            Con_Printf ("video wait successfully enabled!\n");
        }
    }
    else
    {
        vid_wait.value = gVidWait;
        Con_Printf ("Error while trying to change video wait!\n");
    }    
}

//_________________________________________________________________________________________________VID_SetMode()

Boolean	VID_SetDisplayMode (void)
{
    NSOpenGLPixelFormat		*myPixelFormat;

    // just for security:
    if (gDisplayList == NULL || gDisplayCount == 0)
    {
        Sys_Error ("Invalid list of active displays!");
    }

    // save the old display mode:
    gOriginalMode = (NSDictionary *) CGDisplayCurrentMode (gDisplayList[gDisplay]);
    
    if (gDisplayFullscreen)
    {
        // hide cursor:
        IN_ShowCursor (NO);

        // fade the display(s) to black:
        VID_FadeGammaOut (gFadeAllDisplays, FADE_DURATION);
        
        // capture display(s);
        if (VID_CaptureDisplays (gFadeAllDisplays) == NO)
            Sys_Error ("Unable to capture display(s)!");
        
        // switch the display mode:
        if (CGDisplaySwitchToMode (gDisplayList[gDisplay], (CFDictionaryRef) gDisplayMode)
            != CGDisplayNoErr)
        {
            Sys_Error ("Unable to switch the displaymode!");
        }

        gDisplayDepth = [[gDisplayMode objectForKey: (id)kCGDisplayBitsPerPixel] intValue];
    }
    else
    {
        gDisplayDepth = [[gOriginalMode objectForKey: (id)kCGDisplayBitsPerPixel] intValue];
    }
    
    // 8 bit display depth is possible with windowed modes:
    if (gDisplayDepth == 8) gDisplayIs8Bit = YES;
        else gDisplayIs8Bit = NO;

    // get the pixel format:
    if ((myPixelFormat = VID_CreateGLPixelFormat ()) == NULL)
    {
        if (gARBMultiSamples > 0)
            Sys_Error ("Unable to find a matching pixelformat. Set FSAA samples to zero and try again.");
        else
            Sys_Error ("Unable to find a matching pixelformat. Probably your graphics board is not supported.");
    }

    // initialize the OpenGL context:
    if (!(gGLContext = [[NSOpenGLContext alloc] initWithFormat: myPixelFormat shareContext: nil]))
    {
        if (gARBMultiSamples > 0)
            Sys_Error ("Unable to create an OpenGL context. Set FSAA samples to zero and try again.");
        else
            Sys_Error ("Unable to create an OpenGL context. Probably your graphics board is not supported.");
    }

    // get rid of the pixel format:
    [myPixelFormat release];

    if (gDisplayFullscreen)
    {
        // attach the OpenGL context to fullscreen:
        if (CGLSetFullScreen ([gGLContext cglContext]) != CGDisplayNoErr)
            Sys_Error ("Unable to use the selected displaymode for fullscreen OpenGL.");
        
        // fade the gamma back:
        VID_FadeGammaIn (gFadeAllDisplays, 0.0f);
    }
    else
    {
        NSRect 		myContentRect;

        // setup the window according to our settings:
        myContentRect = NSMakeRect (0, 0, gDisplayWidth, gDisplayHeight);
        gWindow = [[NSWindow alloc] initWithContentRect: myContentRect
                                              styleMask: NSTitledWindowMask | NSMiniaturizableWindowMask
#ifdef USE_BUFFERED_WINDOW
                                                backing: NSBackingStoreBuffered
#else
                                                backing: NSBackingStoreRetained
#endif /* USE_BUFFERED_WINDOW */
                                                  defer: NO];
        [gWindow setTitle: @"TenebraeQuake"];

        gWindowView = [[QuakeView alloc] initWithFrame: myContentRect];
        if (gWindowView == NULL)
        {
            Sys_Error ("Unable to create content view!\n");
        }

        // setup the view for tracking the window location:
        [gWindow setContentView: gWindowView];
        [gWindow makeFirstResponder: gWindowView];
        [gWindow setDelegate: gWindowView];

        // attach the OpenGL context to the window:
        [gGLContext setView: [gWindow contentView]];
        
        // finally show the window:
        [gWindow center];
        [gWindow setAcceptsMouseMovedEvents: YES];
        [gWindow makeKeyAndOrderFront: nil];
        [gWindow makeMainWindow];
        [gWindow flushWindow];

        // setup the miniwindow [if one alloc fails, the miniwindow will not be drawn]:
        gMiniWindow = [[NSImage alloc] initWithSize: NSMakeSize (128, 128)];
        gOldIcon = [NSImage imageNamed: @"NSApplicationIcon"];
        gMiniBuffer = malloc (gDisplayWidth * gDisplayHeight * 4);
    }
    
    // Lock the OpenGL context to the refresh rate of the display [for clean rendering], if desired:
    VID_SetWait((UInt32) vid_wait.value);
    
    return (YES);
}

//____________________________________________________________________________________________________VID_Init()

void	VID_Init (unsigned char *thePalette)
{
    char		myGLDir[MAX_OSPATH];
    UInt		i;

    // register miscelanous vars:
    Cvar_RegisterVariable (&vid_mode);
    Cvar_RegisterVariable (&_vid_default_mode);
    Cvar_RegisterVariable (&_vid_default_blit_mode);
    Cvar_RegisterVariable (&vid_wait);
    Cvar_RegisterVariable (&vid_redrawfull);
    Cvar_RegisterVariable (&_windowed_mouse);
    Cvar_RegisterVariable (&gl_anisotropic);
    Cvar_RegisterVariable (&gl_fsaa);
    Cvar_RegisterVariable (&gl_truform);
    Cvar_RegisterVariable (&gl_ztrick);
        
    // setup basic width/height:
    vid.maxwarpwidth = WARP_WIDTH;
    vid.maxwarpheight = WARP_HEIGHT;
    vid.colormap = host_colormap;
    vid.fullbright = 256 - LittleLong (*((SInt *)vid.colormap + 2048));

    gDisplayWidth  = [[gDisplayMode objectForKey: (id)kCGDisplayWidth] intValue];
    gDisplayHeight = [[gDisplayMode objectForKey: (id)kCGDisplayHeight] intValue];

    // get width from command line parameters [only for windowed mode]:
    if ((i = COM_CheckParm("-width")))
        gDisplayWidth = atoi (com_argv[i+1]);
    vid.width = gDisplayWidth;

    // get height from command line parameters [only for windowed mode]:
    if ((i = COM_CheckParm("-height")))
        gDisplayHeight = atoi (com_argv[i+1]);
    vid.height = gDisplayHeight;
    
    vid.aspect = ((float) vid.height / (float) vid.width) * (320.0f / 240.0f);
    vid.numpages = 2;

    // switch the video mode:
    if (!VID_SetDisplayMode())
        Sys_Error ("Can\'t initialize video!");

    // setup console width according to display width:
    if ((i = COM_CheckParm("-conwidth")))
        vid.conwidth = Q_atoi (com_argv[i+1]);
    else
        vid.conwidth = vid.width;
    vid.conwidth &= 0xfff8;

    // setup console height according to display height:
    if ((i = COM_CheckParm ("-conheight")))
        vid.conheight = Q_atoi (com_argv[i+1]);
    else
        vid.conheight = vid.height;

    // check the console size:
    if (vid.conwidth > gDisplayWidth)
        vid.conwidth = gDisplayWidth;
    if (vid.conheight > gDisplayHeight)
        vid.conheight = gDisplayHeight;

    if (vid.conwidth < CONSOLE_MIN_WIDTH)
        vid.conwidth = CONSOLE_MIN_WIDTH;
    if (vid.conheight < CONSOLE_MIN_HEIGHT)
        vid.conheight = CONSOLE_MIN_HEIGHT;

    // setup OpenGL:
    GL_Init ();

    // setup the "glquake" folder within the "id1" folder:
    sprintf (myGLDir, "%s/glquake", com_gamedir);
    Sys_mkdir (myGLDir);

    // enable the video options menu:
    vid_menudrawfn = VID_MenuDraw;
    vid_menukeyfn = VID_MenuKey;
    
    // finish up initialization:
    VID_CheckGamma (thePalette);
    VID_SetPalette (thePalette);
    Con_SafePrintf ("Video mode %dx%d initialized.\n", gDisplayWidth, gDisplayHeight);
    vid.recalc_refdef = 1;
}

//________________________________________________________________________________________________VID_Shutdown()

void	VID_Shutdown (void)
{
    // restore old application icon, if changed:
    VID_RestoreOldIcon ();
    
    // release the buffer of the mini window:
    if (gMiniBuffer != NULL)
    {
        free (gMiniBuffer);
        gMiniBuffer = NULL;
    }
            
    // release the miniwindow:
    if (gMiniWindow != NULL)
    {
        [gMiniWindow release];
        gMiniWindow = NULL;
    }

    // release the old icon:
    if (gOldIcon != NULL)
    {
        [gOldIcon release];
        gOldIcon = NULL;
    }

    // close the old window:
    if (gWindow != NULL)
    {
        [gWindow close];
        [gWindow release];
        gWindow = NULL;
    }

    // close the content view:
    if (gWindowView != NULL)
    {
        [gWindowView release];
        gWindowView = NULL;
    }
    
    // fade gamma out, to avoid splash:
    if (gDisplayFullscreen == YES && gOriginalMode != NULL)
    {
        VID_FadeGammaOut (gFadeAllDisplays, 0.0f);
    }

    // clean up the OpenGL context:
    if (gGLContext != NULL)
    {
        [NSOpenGLContext clearCurrentContext];
        [gGLContext clearDrawable];
        [gGLContext release];
        gGLContext = NULL;
    }
    
    // free the vertexarray buffer:
    if (gl_var == YES && AGP_Buffer != NULL)
    {
        free (AGP_Buffer);
        AGP_Buffer = NULL;
    }
    
    // restore the old display mode:
    if (gDisplayFullscreen == YES)
    {
        if (gOriginalMode != NULL)
        {
            CGDisplaySwitchToMode (gDisplayList[gDisplay], (CFDictionaryRef) gOriginalMode);
        
            VID_ReleaseDisplays (gFadeAllDisplays);

            VID_FadeGammaIn (gFadeAllDisplays, FADE_DURATION);
            
            // clean up the gamma fading:
            if (gOriginalGamma != NULL)
            {
                free (gOriginalGamma);
                gOriginalGamma = NULL;
            }
        }
    }
    else
    {
        if (gWindow != NULL)
        {
            [gWindow release];
        }
    }
}

//__________________________________________________________________________________________VID_RestoreOldIcon()

void	VID_RestoreOldIcon (void)
{
    if (gIsMinimized == YES)
    {
        if (gOldIcon != NULL)
        {
            [NSApp setApplicationIconImage: gOldIcon];
        }
        gIsMinimized = NO;
    }
}

//________________________________________________________________________________________________VID_MenuDraw()

void	VID_MenuDraw (void)
{
    qpic_t	*myPicture;
    UInt8	myRow = FIRST_MENU_LINE, myString[16];

    myPicture = Draw_CachePic ("gfx/vidmodes.lmp");
    M_DrawPic ((320 - myPicture->width) / 2, 4, myPicture);
    
    // draw vid_wait option:
    M_Print (FONT_WIDTH, myRow, "Video Sync:");
    if (vid_wait.value) M_Print ((39 - 2) * FONT_WIDTH, myRow, "On");
        else M_Print ((39 - 3) * FONT_WIDTH, myRow, "Off");
    if (gMenuLine == myRow) gMenuItem = VIDEO_WAIT;
    
    // draw FSAA option:
    if (gl_fsaaavailable == YES)
    {
        myRow += FONT_HEIGHT;
        M_Print (FONT_WIDTH, myRow, "FSAA:");
        if (gl_fsaa.value == 1.0f)
        {
             M_Print ((39 - 3) * FONT_WIDTH, myRow, "Off");
        }
        else
        {
            snprintf (myString, 16, "%dx", (int) gl_fsaa.value);
            M_Print ((39 - strlen (myString)) * FONT_WIDTH, myRow, myString);
        }
        if (gMenuLine == myRow) gMenuItem = VIDEO_FSAA;
    }
    
    // draw anisotropic option:
    if (gl_texturefilteranisotropic == YES)
    {
        myRow += FONT_HEIGHT;
        M_Print (FONT_WIDTH, myRow, "Anisotropic Texture Filtering:");
        if (gl_anisotropic.value) M_Print ((39 - 2) * FONT_WIDTH, myRow, "On");
            else M_Print ((39 - 3) * FONT_WIDTH, myRow, "Off");
        if (gMenuLine == myRow) gMenuItem = VIDEO_ANISOTROPIC;
    }
    
    // draw truform option:
    if (gl_pntriangles == YES)
    {
        myRow += FONT_HEIGHT;
        M_Print (FONT_WIDTH, myRow, "ATI Truform Tesselation Level:");
        if (gl_truform.value < 0)
        {
             M_Print ((39 - 3) * FONT_WIDTH, myRow, "Off");
        }
        else
        {
            snprintf (myString, 16, "%dx", (int) gl_truform.value);
            M_Print ((39 - strlen (myString)) * FONT_WIDTH, myRow, myString);
        }
        if (gMenuLine == myRow) gMenuItem = VIDEO_TRUFORM;
    }

    M_Print (4 * FONT_WIDTH + 4, 36 + 23 * FONT_HEIGHT, "Video modes must be set at the");
    M_Print (11 * FONT_WIDTH + 4, 36 + 24 * FONT_HEIGHT, "startup dialog!");
    
    M_DrawCharacter (0, gMenuLine, 12 + ((int)(realtime * 4) & 1));
    gMenuMaxLine = myRow;
}

//_________________________________________________________________________________________________VID_MenuKey()

void	VID_MenuKey (int theKey)
{
    switch (theKey)
    {
        case K_ESCAPE:
            S_LocalSound ("misc/menu1.wav");
            M_Menu_Options_f ();
            break;
	case K_UPARROW:
            S_LocalSound ("misc/menu1.wav");
            gMenuLine -= FONT_HEIGHT;
            if (gMenuLine < FIRST_MENU_LINE)
                gMenuLine = gMenuMaxLine;
            break;
	case K_DOWNARROW:
            S_LocalSound ("misc/menu1.wav");
            gMenuLine += FONT_HEIGHT;
            if (gMenuLine > gMenuMaxLine)
                gMenuLine = FIRST_MENU_LINE;
            break;
        case K_LEFTARROW:
            S_LocalSound ("misc/menu1.wav");
            switch (gMenuItem)
            {
                case VIDEO_WAIT:
                    Cvar_SetValue ("vid_wait", (vid_wait.value == 0.0f) ? 1.0f : 0.0f);
                    break;
                case VIDEO_FSAA:
                    Cvar_SetValue ("gl_fsaa", (gl_fsaa.value <= 1.0f) ? 4.0f : gl_fsaa.value / 2.0f);
                    break;
                case VIDEO_ANISOTROPIC:
                    Cvar_SetValue ("gl_anisotropic", (gl_anisotropic.value == 0.0f) ? 1.0f : 0.0f);
                    break;
                case VIDEO_TRUFORM:
                    Cvar_SetValue ("gl_truform", (gl_truform.value <= -1.0f) ? 7.0f : gl_truform.value - 1.0f);
                    break;
            }
            break;
        case K_RIGHTARROW:
	case K_ENTER:
            S_LocalSound ("misc/menu1.wav");
            switch (gMenuItem)
            {
                case VIDEO_WAIT:
                    Cvar_SetValue ("vid_wait", (vid_wait.value == 0.0f) ? 1.0f : 0.0f);
                    break;
                case VIDEO_FSAA:
                    Cvar_SetValue ("gl_fsaa", (gl_fsaa.value >= 4.0f) ? 1.0f : gl_fsaa.value * 2.0f);
                    break;
                case VIDEO_ANISOTROPIC:
                    Cvar_SetValue ("gl_anisotropic", (gl_anisotropic.value == 0.0f) ? 1.0f : 0.0f);
                    break;
                case VIDEO_TRUFORM:
                    Cvar_SetValue ("gl_truform", (gl_truform.value >= 7.0f) ? -1.0f : gl_truform.value + 1.0f);
                    break;
            }
            break;
        default:
            break;
    }
}

//___________________________________________________________________________________________GL_GetProcAddress()

void *	GL_GetProcAddress (const char *theName, qboolean theSafeMode)
{
    NSSymbol	mySymbol = NULL;
    char *	mySymbolName = malloc (strlen (theName) + 2);

    if (mySymbolName != NULL)
    {
        strcpy (mySymbolName + 1, theName);
        mySymbolName[0] = '_';

        mySymbol = NULL;
        if (NSIsSymbolNameDefined (mySymbolName))
            mySymbol = NSLookupAndBindSymbol (mySymbolName);

        free (mySymbolName);
    }
    
    if (theSafeMode == YES && mySymbol == NULL)
    {
        Sys_Error ("Failed to import a required OpenGL function!\n");
    }
    
    return ((mySymbol != NULL) ? NSAddressOfSymbol (mySymbol) : NULL);
}

//_____________________________________________________________________________________________GL_CheckVersion()

Boolean	GL_CheckVersion (UInt8 theMajor, UInt8 theMinor)
{
    UInt8	myMajor, myMinor;
    
    // just security:
    if (gl_version == NULL)
    {
        return (NO);
    }
    
    // get the major version number:
    myMajor = atoi (gl_version);
    if (myMajor < theMajor)
    {
        return (NO);
    }

    // compare minor version numbers only if the majors are equal:
    if (myMajor == theMajor)
    {
        const char *	myGLVersion = gl_version;
        
        while (*myGLVersion != 0x00 && *myGLVersion++ != '.');
        myMinor = atoi (myGLVersion);
        
        if (myMinor < theMinor)
        {
            return (NO);
        }        
    }

    return (YES);
}

//_____________________________________________________________________________________GL_CheckPalettedTexture()

void	GL_CheckPalettedTexture (void)
{
    if (strstr (gl_extensions, "GL_EXT_paletted_texture"))
    {
        gl_palettedtex = YES;
        Con_Printf ("Found GL_EXT_paletted_texture...\n");
    }
}

//______________________________________________________________________________GL_CheckMultiTextureExtensions()

void	GL_CheckMultiTextureExtensions (void) 
{
    // look for the extension:
    if (!strstr (gl_extensions, "GL_ARB_multitexture "))
    {
        Sys_Error ("\"GL_ARB_multitexture\" not found.\nYour 3D board is not supported by TenebraeQuake.\n");
    }

    qglActiveTextureARB = GL_GetProcAddress ("glActiveTextureARB", YES);
    qglClientActiveTextureARB = GL_GetProcAddress ("glClientActiveTextureARB", YES);
    qglMultiTexCoord1fARB = GL_GetProcAddress ("glMultiTexCoord1fARB", YES);
    qglMultiTexCoord2fARB = GL_GetProcAddress ("glMultiTexCoord2fARB", YES);
    qglMultiTexCoord2fvARB = GL_GetProcAddress ("glMultiTexCoord2fvARB", YES);
    qglMultiTexCoord3fARB = GL_GetProcAddress ("glMultiTexCoord3fARB", YES);
    qglMultiTexCoord3fvARB = GL_GetProcAddress ("glMultiTexCoord3fvARB", YES);

    Con_Printf ("Found GL_ARB_multitexture...\n");
    gl_mtexable = YES;
}

//________________________________________________________________________GL_CheckDiffuseBumpMappingExtensions()

void	GL_CheckDiffuseBumpMappingExtensions (void) 
{
    if (!strstr(gl_extensions, "GL_ARB_texture_env_combine"))
    {
        Sys_Error ("\"GL_ARB_texture_env_combine\" not found.\n"
                   "Your 3D board is not supported by TenebraeQuake.\n");
    }
    Con_Printf ("Found GL_ARB_texture_env_combine...\n");

    if (!strstr(gl_extensions, "GL_ARB_texture_env_dot3"))
    {
        Sys_Error ("\"GL_ARB_texture_env_dot3\" not found.\n"
                   "Your 3D board is not supported by TenebraeQuake.\n");
    }
    Con_Printf ("Found GL_ARB_texture_env_dot3...\n");
    
    if (!strstr(gl_extensions, "GL_ARB_texture_cube_map"))
    {
        Sys_Error ("\"GL_ARB_texture_cube_map\" not found.\n"
                   "Your 3D board is not supported by TenebraeQuake.\n");
    }
    Con_Printf ("Found GL_ARB_texture_cube_map...\n");
    
    if (GL_CheckVersion (1, 2) == NO &&
        !strstr(gl_extensions, "GL_SGI_texture_edge_clamp") && 
	!strstr(gl_extensions, "GL_EXT_texture_edge_clamp"))
    {
        Con_Printf("Warning no edge_clamp extension found...");
    }    
}

//_______________________________________________________________________GL_CheckSpecularBumpMappingExtensions()

void	GL_CheckSpecularBumpMappingExtensions (void) 
{
    gl_nvcombiner = NO;
        
    if ((strstr(gl_extensions, "GL_NV_register_combiners")) && (!COM_CheckParm ("-forcenonv")))
    {
        qglCombinerParameterfvNV = GL_GetProcAddress ("glCombinerParameterfvNV", NO);
        qglCombinerParameterivNV = GL_GetProcAddress ("glCombinerParameterivNV", NO);
        qglCombinerParameterfNV = GL_GetProcAddress ("glCombinerParameterfNV", NO);
	qglCombinerParameteriNV = GL_GetProcAddress ("glCombinerParameteriNV", NO);
	qglCombinerInputNV = GL_GetProcAddress ("glCombinerInputNV", NO);
	qglCombinerOutputNV = GL_GetProcAddress ("glCombinerOutputNV", NO);
	qglFinalCombinerInputNV = GL_GetProcAddress ("glFinalCombinerInputNV", NO);
	qglGetCombinerInputParameterfvNV = GL_GetProcAddress ("glGetCombinerInputParameterfvNV", NO);
	qglGetCombinerInputParameterivNV = GL_GetProcAddress ("glGetCombinerInputParameterivNV", NO);
	qglGetCombinerOutputParameterfvNV = GL_GetProcAddress ("glGetCombinerOutputParameterfvNV", NO);
	qglGetCombinerOutputParameterivNV = GL_GetProcAddress ("glGetCombinerOutputParameterivNV", NO);
	qglGetFinalCombinerInputParameterfvNV = GL_GetProcAddress ("glGetFinalCombinerInputParameterfvNV", NO);
	qglGetFinalCombinerInputParameterivNV = GL_GetProcAddress ("glGetFinalCombinerInputParameterivNV", NO);

        if (qglCombinerParameterfvNV != NULL &&
            qglCombinerParameterivNV != NULL &&
            qglCombinerParameterfNV != NULL &&
            qglCombinerParameteriNV != NULL &&
            qglCombinerInputNV != NULL &&
            qglCombinerOutputNV != NULL &&
            qglFinalCombinerInputNV != NULL &&
            qglGetCombinerInputParameterfvNV != NULL &&
            qglGetCombinerInputParameterivNV != NULL &&
            qglGetCombinerOutputParameterfvNV != NULL &&
            qglGetCombinerOutputParameterivNV != NULL &&
            qglGetFinalCombinerInputParameterfvNV != NULL &&
            qglGetFinalCombinerInputParameterivNV != NULL)
        {
            gl_nvcombiner = YES;
            Con_Printf ("Found GL_NV_register_combiners...\n");
        }
    }
}

//__________________________________________________________________________________GL_CheckGeforce3Extensions()

// TODO: Replace GL_NV_vertex_program with GL_ARB_vertex_program since it will probably never be supported under
//	 MacOS X.

void	GL_CheckGeforce3Extensions (void) 
{
    GLint	mySupportedTextureUnits;
    
    glGetIntegerv (GL_MAX_TEXTURE_UNITS_ARB, &mySupportedTextureUnits); 
    Con_Printf ("%d texture units\n", mySupportedTextureUnits);
    gl_geforce3 = NO;

    if (strstr (gl_extensions, "GL_NV_vertex_program") && (mySupportedTextureUnits >= 4)  &&
        (!COM_CheckParm ("-forcegf2")) && (gl_nvcombiner == YES))
    {
        if (GL_CheckVersion (1, 2) == YES)
        {
            qglTexImage3DEXT = GL_GetProcAddress ("glTexImage3D", NO);
            Con_Printf ("Found OpenGL v1.2 or later. Using \"glTexImage3D ()\"\n");
        }
        else
        {
            if (strstr (gl_extensions, "GL_EXT_texture3D"))
            {
                qglTexImage3DEXT = GL_GetProcAddress ("glTexImage3DEXT", NO);
                Con_Printf ("Found GL_EXT_texture3D...\n");
            }
        }
            
        if (qglTexImage3DEXT != NULL)
        {
	    qglAreProgramsResidentNV = GL_GetProcAddress ("glAreProgramsResidentNV", NO);
            qglBindProgramNV = GL_GetProcAddress ("glBindProgramNV", NO);
            qglDeleteProgramsNV = GL_GetProcAddress ("glDeleteProgramsNV", NO);
            qglExecuteProgramNV = GL_GetProcAddress ("glExecuteProgramNV", NO);
            qglGenProgramsNV = GL_GetProcAddress ("glGenProgramsNV", NO);
            qglGetProgramParameterdvNV = GL_GetProcAddress ("glGetProgramParameterdvNV", NO);
            qglGetProgramParameterfvNV = GL_GetProcAddress ("glGetProgramParameterfvNV", NO);
            qglGetProgramivNV = GL_GetProcAddress ("glGetProgramivNV", NO);
            qglGetProgramStringNV = GL_GetProcAddress ("glGetProgramStringNV", NO);
            qglGetTrackMatrixivNV = GL_GetProcAddress ("glGetTrackMatrixivNV", NO);
            qglGetVertexAttribdvNV = GL_GetProcAddress ("glGetVertexAttribdvNV", NO);
            qglGetVertexAttribfvNV = GL_GetProcAddress ("glGetVertexAttribfvNV", NO);
            qglGetVertexAttribivNV = GL_GetProcAddress ("glGetVertexAttribivNV", NO);
            qglGetVertexAttribPointervNV = GL_GetProcAddress ("glGetVertexAttribPointervNV", NO);
            qglIsProgramNV = GL_GetProcAddress ("glIsProgramNV", NO);
            qglLoadProgramNV = GL_GetProcAddress ("glLoadProgramNV", NO);
            qglProgramParameter4dNV = GL_GetProcAddress ("glProgramParameter4dNV", NO);
            qglProgramParameter4dvNV = GL_GetProcAddress ("glProgramParameter4dvNV", NO);
            qglProgramParameter4fNV = GL_GetProcAddress ("glProgramParameter4fNV", NO);
            qglProgramParameter4fvNV = GL_GetProcAddress ("glProgramParameter4fvNV", NO);
            qglProgramParameters4dvNV = GL_GetProcAddress ("glProgramParameters4dvNV", NO);
            qglProgramParameters4fvNV = GL_GetProcAddress ("glProgramParameters4fvNV", NO);
            qglRequestResidentProgramsNV = GL_GetProcAddress ("glRequestResidentProgramsNV", NO);
            qglTrackMatrixNV = GL_GetProcAddress ("glTrackMatrixNV", NO);
        
/*            qglBindProgramARB = GL_GetProcAddress ("glBindProgramARB", NO);
            qglDeleteProgramsARB = GL_GetProcAddress ("glDeleteProgramsNV", NO);
            qglGenProgramsARB = GL_GetProcAddress ("glGenProgramsNV", NO);
            qglGetProgramivARB = GL_GetProcAddress ("glGetProgramivNV", NO);
            qglGetProgramStringARB = GL_GetProcAddress ("glGetProgramStringNV", NO);
            qglGetVertexAttribdvARB = GL_GetProcAddress ("glGetVertexAttribdvNV", NO);
            qglGetVertexAttribfvARB = GL_GetProcAddress ("glGetVertexAttribfvNV", NO);
            qglGetVertexAttribivARB = GL_GetProcAddress ("glGetVertexAttribivNV", NO);
            qglGetVertexAttribPointervARB = GL_GetProcAddress ("glGetVertexAttribPointervNV", NO);
            qglIsProgramARB = GL_GetProcAddress ("glIsProgramNV", NO);
            qglProgramStringARB = GL_GetProcAddress ("glLoadProgramNV", NO);

            if (qglBindProgramARB != NULL &&
                qglDeleteProgramsARB != NULL &&
                qglGenProgramsARB != NULL &&
                qglGetProgramivARB != NULL &&
                qglGetVertexAttribdvARB != NULL &&
                qglGetVertexAttribfvARB != NULL &&
                qglGetVertexAttribivARB != NULL &&
                qglGetVertexAttribPointervARB != NULL &&
                qglIsProgramARB != NULL &&
                qglProgramStringARB != NULL &&
                glTrackMatrixNV != NULL)*/

            if (qglAreProgramsResidentNV != NULL &&
                qglBindProgramNV != NULL &&
                qglDeleteProgramsNV != NULL &&
                qglExecuteProgramNV != NULL &&
                qglGenProgramsNV != NULL &&
                qglGetProgramParameterdvNV != NULL &&
                qglGetProgramParameterfvNV != NULL &&
                qglGetProgramivNV != NULL &&
                qglGetProgramStringNV != NULL &&
                qglGetTrackMatrixivNV != NULL &&
                qglGetVertexAttribdvNV != NULL &&
                qglGetVertexAttribfvNV != NULL &&
                qglGetVertexAttribivNV != NULL &&
                qglGetVertexAttribPointervNV != NULL &&
                qglIsProgramNV != NULL &&
                qglLoadProgramNV != NULL &&
                qglProgramParameter4dNV != NULL &&
                qglProgramParameter4dvNV != NULL &&
                qglProgramParameter4fNV != NULL &&
                qglProgramParameter4fvNV != NULL &&
                qglProgramParameters4dvNV != NULL &&
                qglProgramParameters4fvNV != NULL &&
                qglRequestResidentProgramsNV != NULL &&
                qglTrackMatrixNV != NULL)
            {
                Con_Printf ("Using NVidia Geforce 3 or 4Ti path.\n");
                gl_geforce3 = YES;
                gl_filter_min = GL_LINEAR_MIPMAP_LINEAR;
                gl_filter_max = GL_LINEAR;
            }
        }
    }
}

//____________________________________________________________________________________GL_CheckRadeonExtensions()

void GL_CheckRadeonExtensions (void) 
{
    GLint	mySupportedTextureUnits;
    Boolean	myOpenGL12 = NO,
                myExtTexImage3D = NO;
    
    glGetIntegerv (GL_MAX_TEXTURE_UNITS_ARB, &mySupportedTextureUnits); 
    gl_radeon = NO;
    myOpenGL12 = GL_CheckVersion (1, 2);
    myExtTexImage3D = strstr (gl_extensions, "GL_EXT_vertex_shader") ? YES : NO;

    if ((myOpenGL12 == YES || myExtTexImage3D == YES)
	&& (mySupportedTextureUnits >= 4) 
        && (!COM_CheckParm ("-forcegeneric"))
	&& strstr (gl_extensions, "GL_ATI_fragment_shader")
	&& strstr (gl_extensions, "GL_EXT_vertex_shader"))
    {
        if (myOpenGL12 == YES)
        {
            qglTexImage3DEXT = GL_GetProcAddress ("glTexImage3D", NO);
            Con_Printf ("Found OpenGL v1.2 or later. Using \"glTexImage3D ()\"\n");
        }
        else
        {
            if (myExtTexImage3D = YES)
            {
                qglTexImage3DEXT = GL_GetProcAddress ("glTexImage3DEXT", NO);
                Con_Printf ("Found GL_EXT_texture3D...\n");
            }
        }
        
        if (qglTexImage3DEXT != NULL)
        {
            if (GL_LookupRadeonSymbols () == YES)
            {
                gl_filter_min = GL_LINEAR_MIPMAP_LINEAR;
                gl_filter_max = GL_LINEAR;
                GL_CreateShadersRadeon();
                gl_radeon = YES;
                Con_Printf("Using ATI Radeon 8500 path.\n");
            }
        }
    }
    
    if (gl_radeon == NO)
    {
        if (gl_geforce3 == YES)
        {
            Con_Printf("Using Geforce3 or Geforce4Ti path\n");
        }
        else
        {
            if (gl_nvcombiner == YES)
            {
                Con_Printf("Using Geforce2MX or Geforce4MX path\n");
            }
            else
            {
                Con_Printf("Using generic path.\n");
            }
        }
    }
}

//____________________________________________________________________________________GL_CheckVertexArrayRange()

void	GL_CheckVertexArrayRange (void) 
{
    gl_var = NO;

/*   
    // Don't use it yet. There seems to be something up.
    
    if (strstr(gl_extensions, "GL_APPLE_vertex_array_range"))
    {
        qglFlushVertexArrayRangeAPPLE = GL_GetProcAddress ("glFlushVertexArrayRangeAPPLE", NO);
        qglVertexArrayRangeAPPLE = GL_GetProcAddress ("glVertexArrayRangeAPPLE", NO);
        //qglVertexArrayParameteriAPPLE = GL_GetProcAddress ("glVertexArrayParameteriAPPLE", NO);

        if (qglFlushVertexArrayRangeAPPLE != NULL &&
            qglVertexArrayRangeAPPLE != NULL)
        {
            AGP_Buffer = malloc (AGP_BUFFER_SIZE);
            if (AGP_Buffer != NULL)
            {
                gl_var = YES;
                Con_Printf ("Found GL_APPLE_vertex_array_range...\nAllocated %dkb.\n", AGP_BUFFER_SIZE / 1024);
            }
        }
    }
*/
}

//_______________________________________________________________________________GL_CheckPNTrianglesExtensions()

void	GL_CheckPNTrianglesExtensions (void)
{
    if (strstr (gl_extensions, "GL_ATIX_pn_triangles"))
    {
        // symbols present?
        if (glPNTrianglesiATIX != NULL)
        {
            Con_Printf ("Found GL_ATIX_pn_triangles...\n");       
            gl_pntriangles = YES;
        }
        else
        {
            gl_pntriangles = NO;
        }
    }
    else
    {
	gl_pntriangles = NO;    
    }
}

//_____________________________________________________________________________GL_CheckARBMultisampleExtension()

Boolean GL_CheckARBMultisampleExtension (CGDirectDisplayID theDisplay)
{
    CGLRendererInfoObj			myRendererInfo;
    CGLError				myError;
    UInt64				myDisplayMask;
    long				myCount,
                                        myIndex,
                                        mySampleBuffers,
                                        mySamples,
                                        myMaxSampleBuffers = 0,
                                        myMaxSamples = 0;

    // retrieve the renderer info for the selected display:
    myDisplayMask = CGDisplayIDToOpenGLDisplayMask (theDisplay);
    myError = CGLQueryRendererInfo (myDisplayMask, &myRendererInfo, &myCount);
    
    if (myError == kCGErrorSuccess)
    {
        // loop through all renderers:
        for (myIndex = 0; myIndex < myCount; myIndex++)
        {
            // check if the current renderer supports sample buffers:
            myError = CGLDescribeRenderer (myRendererInfo, myIndex, kCGLRPMaxSampleBuffers, &mySampleBuffers);
            if (myError == kCGErrorSuccess && mySampleBuffers > 0)
            {
                // retrieve the number of samples supported by the current renderer:
                myError = CGLDescribeRenderer (myRendererInfo, myIndex, kCGLRPMaxSamples, &mySamples);
                if (myError == kCGErrorSuccess && mySamples > myMaxSamples)
                {
                    myMaxSampleBuffers = mySampleBuffers;
                    myMaxSamples = mySamples;
                }
            }
        }
        
        // get rid of the renderer info:
        CGLDestroyRendererInfo (myRendererInfo);
    }
    
    // NOTE: we could return the max number of samples at this point, but unfortunately there is a bug
    //       with the ATI Radeon/PCI drivers: We would return 4 instead of 8. So we assume that the
    //       max samples are always 8 if we have sample buffers and max samples is greater than 1.
    
    if (myMaxSampleBuffers > 0 && myMaxSamples > 1)
        return (YES);
    return (NO);
}

//__________________________________________________________________________________GL_CheckSwitchFSAAOnTheFly()

void	GL_CheckSwitchFSAAOnTheFly (void)
{
    // Changing the FSAA samples is only available for Radeon boards
    // [a sample buffer is not required at the pixelformat].
    //
    // We don't want to support FSAA under 10.1.x because the current driver will crash the WindowServer.
    // Thus we check for the Radeon string AND the GL_ARB_multisample extension, which is only available for
    // Radeon boards under 10.2 or later.
    
    if (strstr (gl_renderer, "ATI Radeon") && strstr (gl_extensions, "GL_ARB_multisample"))
    {
        Con_Printf ("Graphics board is capable to switch FSAA samples on the fly...\n");
        gl_fsaaavailable = YES;
        switch (gARBMultiSamples)
        {
            case 4:
                gFSAALevel = 2.0f;
                break;
            case 8:
                gFSAALevel = 4.0f;
                break;
            case 0:
            default:
                gFSAALevel = 1.0f;
                break;
        }
        gl_fsaa.value = gFSAALevel;
        return;
    }
    gl_fsaaavailable = NO;
}

//____________________________________________________________________________GL_CheckTextureFilterAnisotropic()

void	GL_CheckTextureFilterAnisotropic (void)
{
    if (strstr (gl_extensions, "GL_EXT_texture_filter_anisotropic"))
    {
        Con_Printf ("Found GL_EXT_texture_filter_anisotropic...\n");
        gl_texturefilteranisotropic = YES;
    }
    else
    {
        gl_texturefilteranisotropic = NO; 
    }
}

//_____________________________________________________________________________________________________GL_Init()

void	GL_Init (void)
{
    // show OpenGL stats at the console:
    gl_vendor = glGetString (GL_VENDOR);
    Con_Printf ("GL_VENDOR: %s\n", gl_vendor);
    
    gl_renderer = glGetString (GL_RENDERER);
    Con_Printf ("GL_RENDERER: %s\n", gl_renderer);
    
    gl_version = glGetString (GL_VERSION);
    Con_Printf ("GL_VERSION: %s\n", gl_version);
    
    gl_extensions = glGetString (GL_EXTENSIONS);
    Con_Printf ("GL_EXTENSIONS: %s\n", gl_extensions);
    
    // not required for MacOS X, but nevertheless:
    if (!strncasecmp ((char*) gl_renderer, "Permedia", 8))
        isPermedia = YES;

    // check for paletted texture extension (GL_Upload8_EXT ()):
    GL_CheckPalettedTexture ();

    // check for multitexture extensions:
    GL_CheckMultiTextureExtensions ();

    // check for extensions required for bump-mapping:
    GL_CheckDiffuseBumpMappingExtensions ();
    GL_CheckSpecularBumpMappingExtensions ();
    GL_CheckGeforce3Extensions ();
    GL_CheckRadeonExtensions ();
    GL_CheckVertexArrayRange ();
    
    // check for pn_triangles extension:
    GL_CheckPNTrianglesExtensions ();

    // check if FSAA is available:
    GL_CheckSwitchFSAAOnTheFly ();

    // check for texture filter anisotropic extension:
    GL_CheckTextureFilterAnisotropic ();

    // setup OpenGL:    
    if (gARBMultiSamples > 0)
        glEnable (GL_MULTISAMPLE_ARB);

    glClearColor (0,0,0,0);
    glCullFace (GL_FRONT);
    glEnable (GL_TEXTURE_2D);

    glEnable (GL_ALPHA_TEST);    
    glAlphaFunc (GL_GREATER, 0.666f);

    glPolygonMode (GL_FRONT_AND_BACK, GL_FILL);
    glShadeModel (GL_FLAT);
    
    glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    
    glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
}

//________________________________________________________________________________________GL_RenderInnsideDock()

void	GL_RenderInsideDock (void)
{
    if ([gWindow isMiniaturized] == YES)
    {
        UInt8		*myPlanes[5];
        UInt32		myStore,
                        *myTop,
                        *myBottom,
                        myRow = vid.width,
                        mySize = myRow * gDisplayHeight,
                        j;
        
        if (gMiniBuffer == NULL || gMiniWindow == NULL)
        {
            return;
        }
        
        // get the current OpenGL rendering:
        glReadPixels (0, 0, gDisplayWidth, gDisplayHeight, GL_RGBA, GL_UNSIGNED_BYTE, gMiniBuffer);

        // mirror the buffer [only vertical]:
        myTop = gMiniBuffer;
        myBottom = myTop + mySize - myRow;
        while (myTop < myBottom)
        {
            for (j = 0; j < myRow; j++)
            {
                myStore = myTop[j];
                myTop[j] = myBottom[j];
                myBottom[j] = myStore;
            }
            myTop += myRow;
            myBottom -= myRow;
        }
            
        myPlanes[0] = (UInt8 *) gMiniBuffer;
        
        [gMiniWindow lockFocus];
        NSDrawBitmap (NSMakeRect (0, 0, 128, 128), gDisplayWidth, gDisplayHeight, 8, 3, 32,
                      gDisplayWidth * 4, NO, NO, @"NSDeviceRGBColorSpace",
                      (const UInt8 * const *) myPlanes);
        [gMiniWindow unlockFocus];
    
        // doesn't seem to work:
        //[gWindow setMiniwindowImage: gMiniWindow];
        
        // so use the app icon instead:
        [NSApp setApplicationIconImage: gMiniWindow];
        gIsMinimized = YES;
    }
    else
    {
        VID_RestoreOldIcon ();
    }
}

//__________________________________________________________________________________________________GL_SetFSAA()

void	GL_SetFSAA (UInt32 theFSAALevel)
{
    // check the level value:
    if (theFSAALevel != 1 && theFSAALevel != 2 && theFSAALevel != 4)
    {
        Cvar_SetValue (gl_fsaa.name, gFSAALevel);
        Con_Printf ("Invalid FSAA level, accepted values are 1, 2 or 4!\n");
        return;
    }
    
    // check if FSAA is available:
    if (gl_fsaaavailable == NO)
    {
        gFSAALevel = gl_fsaa.value;
        if (theFSAALevel != 1)
        {
            Con_Printf ("FSAA not supported with the current graphics board!\n");
        }
        return;
    }
    
    // set the level:
    [gGLContext makeCurrentContext];
    if(CGLSetParameter (CGLGetCurrentContext (), ATI_FSAA_LEVEL, &theFSAALevel) == CGDisplayNoErr)
    {
        gFSAALevel = theFSAALevel;
        Con_Printf ("FSAA level set to: %d!\n", theFSAALevel);

    }
    else
    {
        Con_Printf ("Error while trying to set the new FSAA Level!\n");
        Cvar_SetValue (gl_fsaa.name, gFSAALevel);
    }
}

//___________________________________________________________________________________________GL_SetPNTriangles()

void	GL_SetPNTriangles (SInt32 thePNTriangleLevel)
{
    // check if the pntriangles extension is available:
    if (gl_pntriangles == NO)
    {
        if (thePNTriangleLevel != -1)
        {
            Con_Printf ("pntriangles not supported with the current graphics board!\n");
        }
        gPNTriangleLevel = (gl_truform.value > 7.0f) ? 7.0f : gl_truform.value;
        if (gPNTriangleLevel < 0.0f) gPNTriangleLevel = -1.0f;
        Cvar_SetValue (gl_truform.name, gPNTriangleLevel);
        return;
    }

    if (thePNTriangleLevel >= 0)
    {
        if (thePNTriangleLevel > 7)
        {
            thePNTriangleLevel = 7;
            Con_Printf ("Clamping to max. pntriangle level 7!\n");
        }
        
        // enable pn_triangles. lightning required due to a bug of OpenGL!
        glEnable (GL_PN_TRIANGLES_ATIX);
        glEnable (GL_LIGHTING);
        glLightModelfv (GL_LIGHT_MODEL_AMBIENT, gGLTruformAmbient);
        glEnable (GL_COLOR_MATERIAL);

        // point mode:
        glPNTrianglesiATIX (GL_PN_TRIANGLES_POINT_MODE_ATIX, GL_PN_TRIANGLES_POINT_MODE_LINEAR_ATIX);
            
        // normal mode (no normals used at all by Quake):
        glPNTrianglesiATIX (GL_PN_TRIANGLES_NORMAL_MODE_ATIX, GL_PN_TRIANGLES_NORMAL_MODE_LINEAR_ATIX);

        // tesselation level:
        glPNTrianglesiATIX (GL_PN_TRIANGLES_TESSELATION_LEVEL_ATIX, thePNTriangleLevel);

        Con_Printf ("Truform enabled, current tesselation level: %d!\n", thePNTriangleLevel);
    }
    else
    {
        thePNTriangleLevel = -1;
        glDisable (GL_PN_TRIANGLES_ATIX);
        glDisable (GL_LIGHTING);
        Con_Printf ("Truform disabled!\n");
    }
    gPNTriangleLevel = thePNTriangleLevel;
    Cvar_SetValue (gl_truform.name, gPNTriangleLevel);
}

//______________________________________________________________________________GL_SetTextureFilterAnisotropic()

void	GL_SetTextureFilterAnisotropic (UInt32 theState)
{
    // clamp the value to 1 [= enabled]:
    gGLAnisotropic = theState ? YES : NO;
    Cvar_SetValue (gl_anisotropic.name, gGLAnisotropic);
    
    // check if anisotropic filtering is available:
    if (gl_texturefilteranisotropic == NO)
    {
        gl_texureanisotropylevel = 1.0f;
        if (theState != 0)
        {
            Con_Printf ("Anisotropic tetxure filtering not supported with the current graphics board!\n");
        }
        return;
    }
    
    // enable/disable anisotropic filtering:
    if (theState == 0)
    {
        gl_texureanisotropylevel = 1.0f;
    }
    else
    {
        glGetFloatv (GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &gl_texureanisotropylevel);
    }
}

//___________________________________________________________________________________________GL_BeginRendering()

void	GL_BeginRendering (SInt *x, SInt *y, SInt *width, SInt *height)
{
    *x = *y = 0;
    *width = gDisplayWidth;
    *height = gDisplayHeight;
}

//_____________________________________________________________________________________________GL_EndRendering()

void	GL_EndRendering (void)
{
    // flush OpenGL buffers:
    glFlush ();
    CGLFlushDrawable ([gGLContext cglContext]);//CGLGetCurrentContext ());

    // check if video_wait changed:
    if(vid_wait.value != gVidWait)
    {
        VID_SetWait ((UInt32) vid_wait.value);
    }

    // check if anisotropic texture filtering changed:
    if (gl_anisotropic.value != gGLAnisotropic)
    {
        GL_SetTextureFilterAnisotropic ((UInt32) gl_anisotropic.value);
    }

    // check if vid_fsaa changed:
    if (gl_fsaa.value != gFSAALevel)
    {
        GL_SetFSAA ((UInt32) gl_fsaa.value);
    }

    // check if truform changed:
    if (gl_truform.value != gPNTriangleLevel)
    {
        GL_SetPNTriangles ((SInt32) gl_truform.value);
    }

    // specials for windowed/fullscreen mode:
    if (gDisplayFullscreen == YES)
    {
        static float	myOldGamma = 0.0f;

        // setting the gamma requires fullscreen mode...
        if (myOldGamma != v_gamma.value)
        {
            if (gOriginalGamma != NULL)
            {
                // set the current gamma:
                myOldGamma = 1.0f - v_gamma.value;
                if (myOldGamma < 0.0f)
                {
                    myOldGamma = 0.0f;
                }
                else
                {
                    if (myOldGamma >= 1.0f)
                    {
                        myOldGamma = 0.999f;
                    }
                }
                CGSetDisplayTransferByFormula (gOriginalGamma[gDisplay].displayID,
                                               myOldGamma,
                                               1.0f,
                                               gOriginalGamma[gDisplay].component[2],
                                               myOldGamma,
                                               1.0f,
                                               gOriginalGamma[gDisplay].component[5],
                                               myOldGamma,
                                               1.0f,
                                               gOriginalGamma[gDisplay].component[8]);
            }
            else
            {
                Con_Printf ("Can\'t set the requested gamma value!\n");
            }
            myOldGamma = v_gamma.value;
        }
    }
    else
    {
        // if minimized, render inside the Dock!
        GL_RenderInsideDock ();
    }
}

#pragma mark -

//________________________________________________________________________________iMPLEMENTATION_NSOpenGLContext 

@implementation NSOpenGLContext (CGLContextAccess)

//___________________________________________________________________________________________________cglContext:

- (CGLContextObj) cglContext;
{
    return (_contextAuxiliary);
}

@end

//______________________________________________________________________________________iMPLEMENTATION_QuakeView

@implementation QuakeView

//_________________________________________________________________________________________acceptsFirstResponder

-(BOOL) acceptsFirstResponder
{
    return YES;
}

//________________________________________________________________________________________________windowDidMove:

- (void)windowDidMove: (NSNotification *)note
{
    NSRect	myRect;

    myRect = [gWindow frame];
    gWindowPosX = myRect.origin.x + 1.0f;
    gWindowPosY = myRect.origin.y + 1.0f;
}

@end

//___________________________________________________________________________________________________________eOF 
