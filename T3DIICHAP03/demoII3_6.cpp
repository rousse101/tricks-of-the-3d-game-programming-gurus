// DEMOII3_6.CPP - Directinput joystick demo based on code
// from volume I

// READ THIS!
// To compile make sure to include DDRAW.LIB, DSOUND.LIB,
// DINPUT.LIB, DINPUT8.LIB, WINMM.LIB in the project link list, and of course 
// the C++ source modules T3DLIB1.CPP,T3DLIB2.CPP, and T3DLIB3.CPP
// and the headers T3DLIB1.H,T3DLIB2.H, and T3DLIB3.H must
// be in the working directory of the compiler

// INCLUDES ///////////////////////////////////////////////

#define INITGUID       // make sure al the COM interfaces are available
                       // instead of this you can include the .LIB file
                       // DXGUID.LIB

#define WIN32_LEAN_AND_MEAN  

#include <windows.h>   // include important windows stuff
#include <windowsx.h> 
#include <mmsystem.h>
#include <iostream.h> // include important C/C++ stuff
#include <conio.h>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h> 
#include <math.h>
#include <io.h>
#include <fcntl.h>

#include <ddraw.h>  // directX includes
#include <dsound.h>
#include <dmksctrl.h>
#include <dmusici.h>
#include <dmusicc.h>
#include <dmusicf.h>
#include <dinput.h>
#include "T3DLIB1.h" // game library includes
#include "T3DLIB2.h"
#include "T3DLIB3.h"

// DEFINES ////////////////////////////////////////////////

// defines for windows interface
#define WINDOW_CLASS_NAME "WIN3DCLASS"  // class name
#define WINDOW_TITLE      "T3D Graphics Console Ver 2.0"
#define WINDOW_WIDTH      640   // size of window
#define WINDOW_HEIGHT     480

#define WINDOW_BPP        8    // bitdepth of window (8,16,24 etc.)
                                // note: if windowed and not
                                // fullscreen then bitdepth must
                                // be same as system bitdepth
                                // also if 8-bit the a pallete
                                // is created and attached

#define WINDOWED_APP      0     // 0 not windowed, 1 windowed

// PROTOTYPES /////////////////////////////////////////////

// game console
int Game_Init(void *parms=NULL);
int Game_Shutdown(void *parms=NULL);
int Game_Main(void *parms=NULL);

int Start_Missile(void);
int Move_Missile(void);
int Draw_Missile(void);

// GLOBALS ////////////////////////////////////////////////

HWND main_window_handle           = NULL; // save the window handle
HINSTANCE main_instance           = NULL; // save the instance
char buffer[256];                          // used to print text


// demo globals

BITMAP_IMAGE playfield;       // used to hold playfield
BITMAP_IMAGE mushrooms[4];    // holds mushrooms
BOB          blaster;         // holds bug blaster

int blaster_anim[5] = {0,1,2,1,0};  // blinking animation

// lets use a line segment for the missle
int missile_x,              // position of missle
    missile_y,            
    missile_state;          // state of missle 0 off, 1 on


// FUNCTIONS //////////////////////////////////////////////

LRESULT CALLBACK WindowProc(HWND hwnd, 
						    UINT msg, 
                            WPARAM wparam, 
                            LPARAM lparam)
{
// this is the main message handler of the system
PAINTSTRUCT	ps;		   // used in WM_PAINT
HDC			hdc;	   // handle to a device context

// what is the message 
switch(msg)
	{	
	case WM_CREATE: 
        {
		// do initialization stuff here
		return(0);
		} break;

    case WM_PAINT:
         {
         // start painting
         hdc = BeginPaint(hwnd,&ps);

         // end painting
         EndPaint(hwnd,&ps);
         return(0);
        } break;

	case WM_DESTROY: 
		{
		// kill the application			
		PostQuitMessage(0);
		return(0);
		} break;

	default:break;

    } // end switch

// process any messages that we didn't take care of 
return (DefWindowProc(hwnd, msg, wparam, lparam));

} // end WinProc

// WINMAIN ////////////////////////////////////////////////

int WINAPI WinMain(	HINSTANCE hinstance,
					HINSTANCE hprevinstance,
					LPSTR lpcmdline,
					int ncmdshow)
{
// this is the winmain function

WNDCLASS winclass;	// this will hold the class we create
HWND	 hwnd;		// generic window handle
MSG		 msg;		// generic message
HDC      hdc;       // generic dc
PAINTSTRUCT ps;     // generic paintstruct

// first fill in the window class stucture
winclass.style			= CS_DBLCLKS | CS_OWNDC | 
                          CS_HREDRAW | CS_VREDRAW;
winclass.lpfnWndProc	= WindowProc;
winclass.cbClsExtra		= 0;
winclass.cbWndExtra		= 0;
winclass.hInstance		= hinstance;
winclass.hIcon			= LoadIcon(NULL, IDI_APPLICATION);
winclass.hCursor		= LoadCursor(NULL, IDC_ARROW);
winclass.hbrBackground	= (HBRUSH)GetStockObject(BLACK_BRUSH);
winclass.lpszMenuName	= NULL; 
winclass.lpszClassName	= WINDOW_CLASS_NAME;

// register the window class
if (!RegisterClass(&winclass))
	return(0);

// create the window, note the test to see if WINDOWED_APP is
// true to select the appropriate window flags
if (!(hwnd = CreateWindow(WINDOW_CLASS_NAME, // class
						  WINDOW_TITLE,	 // title
						  (WINDOWED_APP ? (WS_OVERLAPPED | WS_SYSMENU | WS_CAPTION) : (WS_POPUP | WS_VISIBLE)),
					 	  0,0,	   // x,y
						  WINDOW_WIDTH,  // width
                          WINDOW_HEIGHT, // height
						  NULL,	   // handle to parent 
						  NULL,	   // handle to menu
						  hinstance,// instance
						  NULL)))	// creation parms
return(0);

// save the window handle and instance in a global
main_window_handle = hwnd;
main_instance      = hinstance;

// resize the window so that client is really width x height
if (WINDOWED_APP)
{
// now resize the window, so the client area is the actual size requested
// since there may be borders and controls if this is going to be a windowed app
// if the app is not windowed then it won't matter
RECT window_rect = {0,0,WINDOW_WIDTH-1,WINDOW_HEIGHT-1};

// make the call to adjust window_rect
AdjustWindowRectEx(&window_rect,
     GetWindowStyle(main_window_handle),
     GetMenu(main_window_handle) != NULL, 
     GetWindowExStyle(main_window_handle));

// save the global client offsets, they are needed in DDraw_Flip()
window_client_x0 = -window_rect.left;
window_client_y0 = -window_rect.top;

// now resize the window with a call to MoveWindow()
MoveWindow(main_window_handle,
           0, // x position
           0, // y position
           window_rect.right - window_rect.left, // width
           window_rect.bottom - window_rect.top, // height
           FALSE);

// show the window, so there's no garbage on first render
ShowWindow(main_window_handle, SW_SHOW);
} // end if windowed

// perform all game console specific initialization
Game_Init();

// disable CTRL-ALT_DEL, ALT_TAB, comment this line out 
// if it causes your system to crash
SystemParametersInfo(SPI_SCREENSAVERRUNNING, TRUE, NULL, 0);

// enter main event loop
while(1)
	{
	if (PeekMessage(&msg,NULL,0,0,PM_REMOVE))
		{ 
		// test if this is a quit
        if (msg.message == WM_QUIT)
           break;
	
		// translate any accelerator keys
		TranslateMessage(&msg);

		// send the message to the window proc
		DispatchMessage(&msg);
		} // end if
    
    // main game processing goes here
    Game_Main();

	} // end while

// shutdown game and release all resources
Game_Shutdown();

// enable CTRL-ALT_DEL, ALT_TAB, comment this line out 
// if it causes your system to crash
SystemParametersInfo(SPI_SCREENSAVERRUNNING, FALSE, NULL, 0);

// return to Windows like this
return(msg.wParam);

} // end WinMain

/////////////////////////////////////////////////////////

int Start_Missile(void)
{
// this function starts the missle, if it is currently off

if (missile_state==0)
    {
    // enable missile
    missile_state = 1;

    // computer position of missile
    missile_x = blaster.x + 16;
    missile_y = blaster.y-4;

    // return success
    return(1);
    } // end if

// couldn't start missile
return(0);

} // end Start_Missile

///////////////////////////////////////////////////////////

int Move_Missile(void)
{
// this function moves the missle 

// test if missile is alive
if (missile_state==1)
   {
   // move the missile upward
   if ((missile_y-=10) < 0)
      {
      missile_state = 0;
      return(1);
      } // end if

   // lock secondary buffer
   DDraw_Lock_Back_Surface();

   // add missile collision here


   // unlock surface
   DDraw_Unlock_Back_Surface();

   // return success
   return(1);

   } // end if

// return failure
return(0);

} // end Move_Missle

///////////////////////////////////////////////////////////

int Draw_Missile(void)
{
// this function draws the missile 

// test if missile is alive
if (missile_state==1)
   {
   // lock secondary buffer
   DDraw_Lock_Back_Surface();

   // draw the missile in green
   Draw_Clip_Line(missile_x, missile_y, 
                  missile_x, missile_y+6,
                  250,back_buffer, back_lpitch);

   // unlock surface
   DDraw_Unlock_Back_Surface();

   // return success
   return(1);

   } // end if

// return failure
return(0);

} // end Draw_Missle

///////////////////////////////////////////////////////////

// T3D II GAME PROGRAMMING CONSOLE FUNCTIONS ////////////////

int Game_Init(void *parms)
{
// this function is where you do all the initialization 
// for your game

int index; // looping var

// start up DirectDraw (replace the parms as you desire)
DDraw_Init(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_BPP, WINDOWED_APP);

// initialize directinput
DInput_Init();

// acquire the keyboard 
DInput_Init_Keyboard();

// initialize the joystick
DInput_Init_Joystick(-24,24,-24,24);

// initialize directsound and directmusic
DSound_Init();
DMusic_Init();

// hide the mouse
ShowCursor(FALSE);

// seed random number generator
srand(Start_Clock());

// load the background
Load_Bitmap_File(&bitmap8bit, "MUSH.BMP");

// set the palette to background image palette
Set_Palette(bitmap8bit.palette);

// load in the four frames of the mushroom
for (index=0; index<4; index++)
    {
    // create mushroom bitmaps
    Create_Bitmap(&mushrooms[index],0,0,32,32);
    Load_Image_Bitmap(&mushrooms[index],&bitmap8bit,index,0,BITMAP_EXTRACT_MODE_CELL);  
    } // end for index

// now create the bug blaster bob
Create_BOB(&blaster,0,0,32,32,3,
           BOB_ATTR_VISIBLE | BOB_ATTR_MULTI_ANIM | BOB_ATTR_ANIM_ONE_SHOT,
           DDSCAPS_SYSTEMMEMORY);

// load in the four frames of the mushroom
for (index=0; index<3; index++)
     Load_Frame_BOB(&blaster,&bitmap8bit,index,index,1,BITMAP_EXTRACT_MODE_CELL);  

// unload the bitmap file
Unload_Bitmap_File(&bitmap8bit);

// set the animation sequences for bug blaster
Load_Animation_BOB(&blaster,0,5,blaster_anim);

// set up stating state of bug blaster
Set_Pos_BOB(&blaster,320, 400);
Set_Anim_Speed_BOB(&blaster,3);

// set clipping rectangle to screen extents so objects dont
// mess up at edges
RECT screen_rect = {0,0,screen_width,screen_height};
lpddclipper = DDraw_Attach_Clipper(lpddsback,1,&screen_rect);

// create mushroom playfield bitmap
Create_Bitmap(&playfield,0,0,SCREEN_WIDTH,SCREEN_HEIGHT);
playfield.attr |= BITMAP_ATTR_LOADED;

// fill in the background
Load_Bitmap_File(&bitmap8bit, "GRASS.BMP");

// load the grass bitmap image
Load_Image_Bitmap(&playfield,&bitmap8bit,0,0,BITMAP_EXTRACT_MODE_ABS);
Unload_Bitmap_File(&bitmap8bit);

// create the random mushroom patch
for (index=0; index<50; index++)
    {
    // select a mushroom
    int mush = rand()%4;

    // set mushroom to random position
    mushrooms[mush].x = rand()%(SCREEN_WIDTH-32);
    mushrooms[mush].y = rand()%(SCREEN_HEIGHT-128);

    // now draw the mushroom into playfield
    Draw_Bitmap(&mushrooms[mush], playfield.buffer, playfield.width,1);

    } // end for

// return success
return(1);

} // end Game_Init

///////////////////////////////////////////////////////////

int Game_Shutdown(void *parms)
{
// this function is where you shutdown your game and
// release all resources that you allocated

// shut everything down

// release all your resources created for the game here....
// kill the bug blaster
Destroy_BOB(&blaster);

// kill the mushroom maker
for (int index=0; index<4; index++)
    Destroy_Bitmap(&mushrooms[index]);

// kill the playfield bitmap
Destroy_Bitmap(&playfield);

// now directsound
DSound_Stop_All_Sounds();
DSound_Delete_All_Sounds();
DSound_Shutdown();

// directmusic
DMusic_Delete_All_MIDI();
DMusic_Shutdown();

// shut down directinput
DInput_Release_Keyboard();
DInput_Release_Joystick();

DInput_Shutdown();

// shutdown directdraw last
DDraw_Shutdown();

// return success
return(1);
} // end Game_Shutdown

//////////////////////////////////////////////////////////

int Game_Main(void *parms)
{
// this is the workhorse of your game it will be called
// continuously in real-time this is like main() in C
// all the calls for you game go here!

int          index;             // looping var
int          dx,dy;             // general deltas used in collision detection


// start the timing clock
Start_Clock();

// clear the drawing surface
DDraw_Fill_Surface(lpddsback, 0);

// get the joystick data
DInput_Read_Joystick();

// lock the back buffer
DDraw_Lock_Back_Surface();

// draw the background reactor image
Draw_Bitmap(&playfield, back_buffer, back_lpitch, 0);

// unlock the back buffer
DDraw_Unlock_Back_Surface();

// is the player moving?
blaster.x+=joy_state.lX;
blaster.y+=joy_state.lY;

// test bounds
if (blaster.x > SCREEN_WIDTH-32)
    blaster.x = SCREEN_WIDTH-32;
else
if (blaster.x < 0)
    blaster.x = 0;

if (blaster.y > SCREEN_HEIGHT-32)
    blaster.y = SCREEN_HEIGHT-32;
else
if (blaster.y < SCREEN_HEIGHT-128)
    blaster.y = SCREEN_HEIGHT-128;

// is player firing?
if (joy_state.rgbButtons[0])
   Start_Missile();

// move and draw missle
Move_Missile();
Draw_Missile();

// is it time to blink eyes
if ((rand()%100)==50)
   Set_Animation_BOB(&blaster,0);

// draw blaster
Animate_BOB(&blaster);
Draw_BOB(&blaster,lpddsback);

// draw some text
Draw_Text_GDI("Make My Centipede!",0,0,RGB(255,255,255),lpddsback);

// display joystick and buttons 0-7
sprintf(buffer,"Joystick Stats: X-Axis=%d, Y-Axis=%d, buttons(%d,%d,%d,%d,%d,%d,%d,%d)",
                                                                      joy_state.lX,joy_state.lY,
                                                                      joy_state.rgbButtons[0],
                                                                      joy_state.rgbButtons[1],
                                                                      joy_state.rgbButtons[2],
                                                                      joy_state.rgbButtons[3],
                                                                      joy_state.rgbButtons[4],
                                                                      joy_state.rgbButtons[5],
                                                                      joy_state.rgbButtons[6],
                                                                      joy_state.rgbButtons[7]);

Draw_Text_GDI(buffer,0,SCREEN_HEIGHT-20,RGB(255,255,50),lpddsback);

// print out name of joystick
sprintf(buffer, "Joystick Name & Vendor: %s",joyname);
Draw_Text_GDI(buffer,0,SCREEN_HEIGHT-40,RGB(255,255,50),lpddsback);


// flip the surfaces
DDraw_Flip();

// sync to 30 fps
Wait_Clock(30);


// check of user is trying to exit
if (KEY_DOWN(VK_ESCAPE) || keyboard_state[DIK_ESCAPE])
    {
    PostMessage(main_window_handle, WM_DESTROY,0,0);

    } // end if



// return success
return(1);

 
} // end Game_Main

//////////////////////////////////////////////////////////