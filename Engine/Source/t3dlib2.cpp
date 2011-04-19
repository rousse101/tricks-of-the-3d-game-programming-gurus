// T3DLIB2.CPP - Game Engine Part II

// INCLUDES ///////////////////////////////////////////////

#define WIN32_LEAN_AND_MEAN  
// #define INITGUID

#include "CommonHeader.h"

#include <ddraw.h>  // directX includes
#include <dinput.h>

#include "T3DLIB1.H"
#include "T3DLIB2.H"

// DEFINES ////////////////////////////////////////////////

// TYPES //////////////////////////////////////////////////

// PROTOTYPES /////////////////////////////////////////////

// EXTERNALS /////////////////////////////////////////////

extern HWND main_window_handle;     // access to main window handle in main module
extern HINSTANCE main_instance; // save the instance

// GLOBALS ////////////////////////////////////////////////

// directinput globals
LPDIRECTINPUT8       lpdi      = NULL;    // dinput object
LPDIRECTINPUTDEVICE8 lpdikey   = NULL;    // dinput keyboard
LPDIRECTINPUTDEVICE8 lpdimouse = NULL;    // dinput mouse
LPDIRECTINPUTDEVICE8 lpdijoy   = NULL;    // dinput joystick
GUID                 joystickGUID;        // guid for main joystick
char                 joyname[80];         // name of joystick

// these contain the target records for all di input packets
UCHAR keyboard_state[256]; // contains keyboard state table
DIMOUSESTATE mouse_state;  // contains state of mouse
DIJOYSTATE joy_state;      // contains state of joystick
int joystick_found = 0;    // tracks if joystick was found and inited

// FUNCTIONS //////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////

BOOL CALLBACK DInput_Enum_Joysticks(LPCDIDEVICEINSTANCE lpddi,
                                    LPVOID guid_ptr) 
{
    // this function enumerates the joysticks, but
    // stops at the first one and returns the
    // instance guid of it, so we can create it

    *(GUID*)guid_ptr = lpddi->guidInstance; 

    // copy name into global
    strcpy(joyname, (char *)lpddi->tszProductName);

    // stop enumeration after one iteration
    return(DIENUM_STOP);

} // end DInput_Enum_Joysticks

//////////////////////////////////////////////////////////////////////////////

int DInput_Init(void)
{
    // this function initializes directinput

    if (FAILED(DirectInput8Create(main_instance, DIRECTINPUT_VERSION, IID_IDirectInput8, (void **)&lpdi,NULL)))
        return(0);

    // return success
    return(1);

} // end DInput_Init

///////////////////////////////////////////////////////////

void DInput_Shutdown(void)
{
    // this function shuts down directinput

    if (lpdi)
        lpdi->Release();

} // end DInput_Shutdown

///////////////////////////////////////////////////////////

int DInput_Init_Joystick(int min_x, int max_x, int min_y, int max_y, int dead_zone)
{
    // this function initializes the joystick, it allows you to set
    // the minimum and maximum x-y ranges 

    // first find the fucking GUID of your particular joystick
    lpdi->EnumDevices(DI8DEVCLASS_GAMECTRL, 
        DInput_Enum_Joysticks, 
        &joystickGUID, 
        DIEDFL_ATTACHEDONLY); 

    // create a temporary IDIRECTINPUTDEVICE (1.0) interface, so we query for 2
    LPDIRECTINPUTDEVICE lpdijoy_temp = NULL;

    if (lpdi->CreateDevice(joystickGUID, &lpdijoy, NULL)!=DI_OK)
        return(0);

    // set cooperation level
    if (lpdijoy->SetCooperativeLevel(main_window_handle, 
        DISCL_NONEXCLUSIVE | DISCL_BACKGROUND)!=DI_OK)
        return(0);

    // set data format
    if (lpdijoy->SetDataFormat(&c_dfDIJoystick)!=DI_OK)
        return(0);

    // set the range of the joystick
    DIPROPRANGE joy_axis_range;

    // first x axis
    joy_axis_range.lMin = min_x;
    joy_axis_range.lMax = max_x;

    joy_axis_range.diph.dwSize       = sizeof(DIPROPRANGE); 
    joy_axis_range.diph.dwHeaderSize = sizeof(DIPROPHEADER); 
    joy_axis_range.diph.dwObj        = DIJOFS_X;
    joy_axis_range.diph.dwHow        = DIPH_BYOFFSET;

    lpdijoy->SetProperty(DIPROP_RANGE,&joy_axis_range.diph);

    // now y-axis
    joy_axis_range.lMin = min_y;
    joy_axis_range.lMax = max_y;

    joy_axis_range.diph.dwSize       = sizeof(DIPROPRANGE); 
    joy_axis_range.diph.dwHeaderSize = sizeof(DIPROPHEADER); 
    joy_axis_range.diph.dwObj        = DIJOFS_Y;
    joy_axis_range.diph.dwHow        = DIPH_BYOFFSET;

    lpdijoy->SetProperty(DIPROP_RANGE,&joy_axis_range.diph);


    // and now the dead band
    DIPROPDWORD dead_band; // here's our property word

    // scale dead zone by 100
    dead_zone*=100;

    dead_band.diph.dwSize       = sizeof(dead_band);
    dead_band.diph.dwHeaderSize = sizeof(dead_band.diph);
    dead_band.diph.dwObj        = DIJOFS_X;
    dead_band.diph.dwHow        = DIPH_BYOFFSET;

    // deadband will be used on both sides of the range +/-
    dead_band.dwData            = dead_zone;

    // finally set the property
    lpdijoy->SetProperty(DIPROP_DEADZONE,&dead_band.diph);

    dead_band.diph.dwSize       = sizeof(dead_band);
    dead_band.diph.dwHeaderSize = sizeof(dead_band.diph);
    dead_band.diph.dwObj        = DIJOFS_Y;
    dead_band.diph.dwHow        = DIPH_BYOFFSET;

    // deadband will be used on both sides of the range +/-
    dead_band.dwData            = dead_zone;


    // finally set the property
    lpdijoy->SetProperty(DIPROP_DEADZONE,&dead_band.diph);

    // acquire the joystick
    if (lpdijoy->Acquire()!=DI_OK)
        return(0);

    // set found flag
    joystick_found = 1;

    // return success
    return(1);

} // end DInput_Init_Joystick

///////////////////////////////////////////////////////////

int DInput_Init_Mouse(void)
{
    // this function intializes the mouse

    // create a mouse device 
    if (lpdi->CreateDevice(GUID_SysMouse, &lpdimouse, NULL)!=DI_OK)
        return(0);

    // set cooperation level
    // change to EXCLUSIVE FORGROUND for better control
    if (lpdimouse->SetCooperativeLevel(main_window_handle, 
        DISCL_NONEXCLUSIVE | DISCL_BACKGROUND)!=DI_OK)
        return(0);

    // set data format
    if (lpdimouse->SetDataFormat(&c_dfDIMouse)!=DI_OK)
        return(0);

    // acquire the mouse
    if (lpdimouse->Acquire()!=DI_OK)
        return(0);

    // return success
    return(1);

} // end DInput_Init_Mouse

///////////////////////////////////////////////////////////

int DInput_Init_Keyboard(void)
{
    // this function initializes the keyboard device

    // create the keyboard device  
    if (lpdi->CreateDevice(GUID_SysKeyboard, &lpdikey, NULL)!=DI_OK)
        return(0);

    // set cooperation level
    if (lpdikey->SetCooperativeLevel(main_window_handle, 
        DISCL_NONEXCLUSIVE | DISCL_BACKGROUND)!=DI_OK)
        return(0);

    // set data format
    if (lpdikey->SetDataFormat(&c_dfDIKeyboard)!=DI_OK)
        return(0);

    // acquire the keyboard
    if (lpdikey->Acquire()!=DI_OK)
        return(0);

    // return success
    return(1);

} // end DInput_Init_Keyboard

///////////////////////////////////////////////////////////

int DInput_Read_Joystick(void)
{
    // this function reads the joystick state

    // make sure the joystick was initialized
    if (!joystick_found)
        return(0);

    if (lpdijoy)
    {
        // this is needed for joysticks only    
        if (lpdijoy->Poll()!=DI_OK)
            return(0);

        if (lpdijoy->GetDeviceState(sizeof(DIJOYSTATE), (LPVOID)&joy_state)!=DI_OK)
            return(0);
    }
    else
    {
        // joystick isn't plugged in, zero out state
        memset(&joy_state,0,sizeof(joy_state));

        // return error
        return(0);
    } // end else

    // return sucess
    return(1);

} // end DInput_Read_Joystick

///////////////////////////////////////////////////////////

int DInput_Read_Mouse(void)
{
    // this function reads  the mouse state

    if (lpdimouse)    
    {
        if (lpdimouse->GetDeviceState(sizeof(DIMOUSESTATE), (LPVOID)&mouse_state)!=DI_OK)
            return(0);
    }
    else
    {
        // mouse isn't plugged in, zero out state
        memset(&mouse_state,0,sizeof(mouse_state));

        // return error
        return(0);
    } // end else

    // return sucess
    return(1);

} // end DInput_Read_Mouse

///////////////////////////////////////////////////////////

int DInput_Read_Keyboard(void)
{
    // this function reads the state of the keyboard

    if (lpdikey)
    {
        if (lpdikey->GetDeviceState(256, (LPVOID)keyboard_state)!=DI_OK)
            return(0);
    }
    else
    {
        // keyboard isn't plugged in, zero out state
        memset(keyboard_state,0,sizeof(keyboard_state));

        // return error
        return(0);
    } // end else

    // return sucess
    return(1);

} // end DInput_Read_Keyboard

///////////////////////////////////////////////////////////

void DInput_Release_Joystick(void)
{
    // this function unacquires and releases the joystick

    if (lpdijoy)
    {    
        lpdijoy->Unacquire();
        lpdijoy->Release();
    } // end if

} // end DInput_Release_Joystick

///////////////////////////////////////////////////////////

void DInput_Release_Mouse(void)
{
    // this function unacquires and releases the mouse

    if (lpdimouse)
    {    
        lpdimouse->Unacquire();
        lpdimouse->Release();
    } // end if

} // end DInput_Release_Mouse

///////////////////////////////////////////////////////////

void DInput_Release_Keyboard(void)
{
    // this function unacquires and releases the keyboard

    if (lpdikey)
    {
        lpdikey->Unacquire();
        lpdikey->Release();
    } // end if

} // end DInput_Release_Keyboard

