//___________________________________________________________________________________________________________nFO
// "in_osx.m" - MacOS X mouse driver
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
// v1.0.6: Mouse sensitivity works now as expected.
// v1.0.5: Reworked whole mouse handling code [required for windowed mouse].
// v1.0.0: Initial release.
//______________________________________________________________________________________________________iNCLUDES

#pragma mark =Includes=

#include "quakedef.h"

#include <AppKit/AppKit.h>
#import <IOKit/IOKitLib.h>
#import <IOKit/hidsystem/IOHIDLib.h>
#import <IOKit/hidsystem/IOHIDParameter.h>

#pragma mark -

//______________________________________________________________________________________________________tYPEDEFS

#pragma mark =TypeDefs=

typedef struct	{
                        SInt		X;
                        SInt		Y;
                        SInt		OldX;
                        SInt		OldY;
                }	gMousePositionStruct;

#pragma mark -

//_______________________________________________________________________________________________________sTATICS

#pragma mark =Variables=

extern Boolean			gIsMinimized,
                                gIsHidden,
                                gIsDeactivated;
extern qboolean			gDisplayFullscreen;
extern cvar_t			_windowed_mouse;

cvar_t				aux_look = {"auxlook","1", true};
cvar_t				m_filter = {"m_filter","1"};

qboolean			gMouseEnabled;

static qboolean			gMouseMoved;
static gMousePositionStruct	gMousePosition, gMouseNewPosition, gMouseOldPosition;

#pragma mark -

//___________________________________________________________________________________________fUNCTION_pROTOTYPES

#pragma mark =Function Prototypes=

void			IN_CenterCursor (void);
void			IN_ShowCursor (Boolean);

void 			IN_InitMouse (void);
void 			IN_ReceiveMouseMove (CGMouseDelta, CGMouseDelta);
void 			IN_MouseMove (usercmd_t *);

static io_connect_t	IN_GetIOHandle (void);
static void 		IN_SetMouseScalingEnabled (Boolean theState);

#pragma mark -

//____________________________________________________________________________________________Toggle_AuxLook_f()

void	Toggle_AuxLook_f (void)
{
    if (aux_look.value)
    {
        Cvar_Set ("auxlook","0");
    }
    else
    {
        Cvar_Set ("auxlook","1");
    }
}

//__________________________________________________________________________________________Force_CenterView_f()

void	Force_CenterView_f (void)
{
    cl.viewangles[PITCH] = 0;
}

//______________________________________________________________________________________________IN_GetIOHandle()

io_connect_t IN_GetIOHandle (void)
{
    io_connect_t 	myHandle = MACH_PORT_NULL;
    kern_return_t	myStatus;
    io_service_t	myService = MACH_PORT_NULL;
    mach_port_t		myMasterPort;

    myStatus = IOMasterPort (MACH_PORT_NULL, &myMasterPort );
    if (myStatus != KERN_SUCCESS)
    {
        return (NULL);
    }

    myService = IORegistryEntryFromPath (myMasterPort, kIOServicePlane ":/IOResources/IOHIDSystem");
    if (myService == NULL)
    {
        return (NULL);
    }

    myStatus = IOServiceOpen (myService, mach_task_self (), kIOHIDParamConnectType, &myHandle);
    IOObjectRelease (myService);

    return (myHandle);
}

//___________________________________________________________________________________IN_SetMouseScalingEnabled()

void	IN_SetMouseScalingEnabled (Boolean theState)
{
    static BOOL		myMouseScalingEnabled = YES;
    static double	myOldAcceleration = 0.0;
    io_connect_t	myIOHandle = NULL;

    // Do we have a state change?
    if (theState == myMouseScalingEnabled)
    {
        return;
    }
    
    // Get the IOKit handle:
    myIOHandle = IN_GetIOHandle ();
    if (myIOHandle == NULL)
    {
        return;
    }

    // Set the mouse acceleration according to the current state:
    if (theState == YES)
    {
        IOHIDSetAccelerationWithKey (myIOHandle,
                                     CFSTR (kIOHIDMouseAccelerationType),
                                     myOldAcceleration);
    }
    else
    {
        kern_return_t	myStatus;

        myStatus = IOHIDGetAccelerationWithKey (myIOHandle,
                                                CFSTR (kIOHIDMouseAccelerationType),
                                                &myOldAcceleration);

        // change only the settings, if we were successfull!
        if (myStatus != kIOReturnSuccess || myOldAcceleration == 0.0)
        {
            theState = YES;
        }
         
        // finally change the acceleration:
        if (theState == NO)
        {
            IOHIDSetAccelerationWithKey (myIOHandle,  CFSTR (kIOHIDMouseAccelerationType), -1.0);
        }
    }
    
    myMouseScalingEnabled = theState;
    IOServiceClose (myIOHandle);
}

//_______________________________________________________________________________________________IN_ShowCursor()

void	IN_ShowCursor (Boolean theState)
{
    static Boolean	myCursorIsVisible = YES;

    // change only if we got a state change:
    if (theState != myCursorIsVisible)
    {
        if (theState == YES)
        {
            CGAssociateMouseAndMouseCursorPosition (YES);
            IN_SetMouseScalingEnabled (YES);
            IN_CenterCursor ();
            CGDisplayShowCursor (kCGDirectMainDisplay);
        }
        else
        {
            [NSApp activateIgnoringOtherApps: YES];
            CGDisplayHideCursor (kCGDirectMainDisplay);
            CGAssociateMouseAndMouseCursorPosition (NO);
            IN_CenterCursor ();
            IN_SetMouseScalingEnabled (NO);
        }
        myCursorIsVisible = theState;
    }
}

//_____________________________________________________________________________________________IN_CenterCursor()

void	IN_CenterCursor (void)
{
    CGPoint		myCenter;

    if (gDisplayFullscreen == NO)
    {
        extern float	gWindowPosX, gWindowPosY;
        float		myCenterX = gWindowPosX, myCenterY = -gWindowPosY;

        // calculate the window center:
        myCenterX += (float) (vid.width >> 1);
        myCenterY += (float) CGDisplayPixelsHigh (kCGDirectMainDisplay) - (float) (vid.height >> 1);
        
        myCenter = CGPointMake (myCenterX, myCenterY);
    }
    else
    {
        // just center at the middle of the screen:
        myCenter = CGPointMake ((float) (vid.width >> 1), (float) (vid.height >> 1));
    }

    // and go:
    CGDisplayMoveCursorToPoint (kCGDirectMainDisplay, myCenter);
}

//________________________________________________________________________________________________IN_InitMouse()

void	IN_InitMouse (void)
{
    // check for command line:
    if (COM_CheckParm ("-nomouse"))
    {
        gMouseEnabled = NO;
        return;
    }
    else
    {
        gMouseEnabled = YES;
    }
    
    gMouseMoved = NO;
}

//_____________________________________________________________________________________________________IN_Init()

void 	IN_Init (void)
{
    // register variables:
    Cvar_RegisterVariable (&m_filter);
    Cvar_RegisterVariable (&aux_look);
    
    // register console commands:
    Cmd_AddCommand ("toggle_auxlook", Toggle_AuxLook_f);
    Cmd_AddCommand ("force_centerview", Force_CenterView_f);
    
    // init the mouse:
    IN_InitMouse ();
    
    IN_SetMouseScalingEnabled (NO);
}

//_________________________________________________________________________________________________IN_Shutdown()

void 	IN_Shutdown (void)
{
    IN_SetMouseScalingEnabled (YES);
}

//_________________________________________________________________________________________________IN_Commands()

void 	IN_Commands (void)
{
    // avoid popping of the app back to the front:
    if (gIsHidden == YES)
    {
        return;
    }
    
    // set the cursor visibility by respecting the display mode:
    if (gDisplayFullscreen == 1.0f)
    {
        IN_ShowCursor (NO);
    }
    else
    {
        // is the mouse in windowed mode?
        if (gMouseEnabled == YES && gIsDeactivated == NO &&
            gIsMinimized == NO && _windowed_mouse.value != 0.0f)
        {
            IN_ShowCursor (NO);
        }
        else
        {
            IN_ShowCursor (YES);
        }
    }
}

//_________________________________________________________________________________________IN_ReceiveMouseMove()

void	IN_ReceiveMouseMove (CGMouseDelta theDeltaX, CGMouseDelta theDeltaY)
{
    gMouseNewPosition.X = theDeltaX;
    gMouseNewPosition.Y = theDeltaY;
}

//________________________________________________________________________________________________IN_MouseMove()

void	IN_MouseMove (usercmd_t *cmd)
{
    CGMouseDelta	myMouseX = gMouseNewPosition.X,
                        myMouseY = gMouseNewPosition.Y;

    if ((gDisplayFullscreen == NO && _windowed_mouse.value == 0.0f) ||
        gMouseEnabled == NO || gIsMinimized == YES || gIsDeactivated == YES)
    {
        return;
    }

    gMouseNewPosition.X = 0;
    gMouseNewPosition.Y = 0;

    if (m_filter.value != 0.0f)
    {
        gMousePosition.X = (myMouseX + gMouseOldPosition.X) >> 1;
        gMousePosition.Y = (myMouseY + gMouseOldPosition.Y) >> 1;
    }
    else
    {
        gMousePosition.X = myMouseX;
        gMousePosition.Y = myMouseY;
    }

    gMouseOldPosition.X = myMouseX;
    gMouseOldPosition.Y = myMouseY;

    gMousePosition.X *= sensitivity.value;
    gMousePosition.Y *= sensitivity.value;

    // lookstrafe or view?
    if ((in_strafe.state & 1) || (lookstrafe.value && (in_mlook.state & 1)))
    {
        cmd->sidemove += m_side.value * gMousePosition.X;
    }
    else
    {
        cl.viewangles[YAW] -= m_yaw.value * gMousePosition.X;
    }
                
    if (in_mlook.state & 1)
    {
        V_StopPitchDrift ();
    }
            
    if ((in_mlook.state & 1) && !(in_strafe.state & 1))
    {
        cl.viewangles[PITCH] += m_pitch.value * gMousePosition.Y;
        
        if (cl.viewangles[PITCH] > 80)
        {
            cl.viewangles[PITCH] = 80;
        }
        
        if (cl.viewangles[PITCH] < -70)
        {
            cl.viewangles[PITCH] = -70;
        }
    }
    else
    {
        if ((in_strafe.state & 1) && noclip_anglehack)
        {
            cmd->upmove -= m_forward.value * gMousePosition.Y;
        }
        else
        {
            cmd->forwardmove -= m_forward.value * gMousePosition.Y;
        }
    }

    // force the mouse to the center, so there's room to move:
    if (myMouseX != 0 || myMouseY != 0)
    {
        IN_CenterCursor ();
    }
}

//_____________________________________________________________________________________________________IN_Move()

void IN_Move (usercmd_t *cmd)
{
    IN_MouseMove (cmd);
}

//___________________________________________________________________________________________________________eOF
