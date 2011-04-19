// DEMOII3_4.CPP - DirectInput keyboard example
// based on demo from Volume I
// system based on 
// conservation of momentum and kinetic energy
// to compile make sure to include DDRAW.LIB, DSOUND.LIB,
// DINPUT.LIB, DINPUT8.LIB, WINMM.LIB, and of course the T3DLIB files

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

// default screen size
#define SCREEN_WIDTH    640  // size of screen
#define SCREEN_HEIGHT   480
#define SCREEN_BPP      8    // bits per pixel

#define MAX_COLORS_PALETTE   256

// skelaton directions
#define SKELATON_EAST         0
#define SKELATON_NEAST        1  
#define SKELATON_NORTH        2
#define SKELATON_NWEST        3
#define SKELATON_WEST         4
#define SKELATON_SWEST        5
#define SKELATON_SOUTH        6
#define SKELATON_SEAST        7

#define WALL_ANIMATION_COLOR  29

// PROTOTYPES /////////////////////////////////////////////

// game console
int Game_Init(void *parms=NULL);
int Game_Shutdown(void *parms=NULL);
int Game_Main(void *parms=NULL);

// GLOBALS ////////////////////////////////////////////////

HWND main_window_handle           = NULL; // save the window handle
HINSTANCE main_instance           = NULL; // save the instance
char buffer[256];                          // used to print text


// demo globals
BOB          skelaton;     // the player skelaton

// animation sequences for bob
int skelaton_anims[8][4] = { {0,1,0,2},
                             {0+4,1+4,0+4,2+4},
                             {0+8,1+8,0+8,2+8},
                             {0+12,1+12,0+12,2+12},
                             {0+16,1+16,0+16,2+16},
                             {0+20,1+20,0+20,2+20},
                             {0+24,1+24,0+24,2+24},
                             {0+28,1+28,0+28,2+28}, };

BOB          plasma;       // players weapon

BITMAP_IMAGE reactor;      // the background   

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

// T3D II GAME PROGRAMMING CONSOLE FUNCTIONS ////////////////

int Game_Init(void *parms)
{
// this function is where you do all the initialization 
// for your game

char filename[80]; // used to read files

// start up DirectDraw (replace the parms as you desire)
DDraw_Init(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_BPP, WINDOWED_APP);

// initialize directinput
DInput_Init();

// acquire the keyboard 
DInput_Init_Keyboard();

// add calls to acquire other directinput devices here...


// initialize directsound and directmusic
DSound_Init();
DMusic_Init();

// hide the mouse
ShowCursor(FALSE);

// seed random number generator
srand(Start_Clock());

// all your initialization code goes here...

///////////////////////////////////////////////////////////

// load the background
Load_Bitmap_File(&bitmap8bit, "REACTOR.BMP");

// set the palette to background image palette
Set_Palette(bitmap8bit.palette);

// create and load the reactor bitmap image
Create_Bitmap(&reactor, 0,0, 640, 480);
Load_Image_Bitmap(&reactor,&bitmap8bit,0,0,BITMAP_EXTRACT_MODE_ABS);
Unload_Bitmap_File(&bitmap8bit);

// now let's load in all the frames for the skelaton!!!

// create skelaton bob
if (!Create_BOB(&skelaton,0,0,56,72,32,
           BOB_ATTR_VISIBLE | BOB_ATTR_MULTI_ANIM,DDSCAPS_SYSTEMMEMORY))
   return(0);

// load the frames in 8 directions, 4 frames each
// each set of frames has a walk and a fire, frame sets
// are loaded in counter clockwise order looking down
// from a birds eys view or the x-z plane
for (int direction = 0; direction < 8; direction++)
    { 
    // build up file name
    sprintf(filename,"SKELSP%d.BMP",direction);

    // load in new bitmap file
    Load_Bitmap_File(&bitmap8bit,filename);
 
    Load_Frame_BOB(&skelaton,&bitmap8bit,0+direction*4,0,0,BITMAP_EXTRACT_MODE_CELL);  
    Load_Frame_BOB(&skelaton,&bitmap8bit,1+direction*4,1,0,BITMAP_EXTRACT_MODE_CELL);  
    Load_Frame_BOB(&skelaton,&bitmap8bit,2+direction*4,2,0,BITMAP_EXTRACT_MODE_CELL);  
    Load_Frame_BOB(&skelaton,&bitmap8bit,3+direction*4,0,1,BITMAP_EXTRACT_MODE_CELL);  

    // unload the bitmap file
    Unload_Bitmap_File(&bitmap8bit);

    // set the animation sequences for skelaton
    Load_Animation_BOB(&skelaton,direction,4,skelaton_anims[direction]);

    } // end for direction

// set up stating state of skelaton
Set_Animation_BOB(&skelaton, 0);
Set_Anim_Speed_BOB(&skelaton, 4);
Set_Vel_BOB(&skelaton, 0,0);
Set_Pos_BOB(&skelaton, 0, 128);

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

// kill the reactor
Destroy_Bitmap(&reactor);

// kill skelaton
Destroy_BOB(&skelaton);

// now directsound
DSound_Stop_All_Sounds();
DSound_Delete_All_Sounds();
DSound_Shutdown();

// directmusic
DMusic_Delete_All_MIDI();
DMusic_Shutdown();

// shut down directinput
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
 
static int   player_moving = 0; // tracks player motion
static PALETTEENTRY glow = {0,0,0,PC_NOCOLLAPSE};  // used to animation red border
static int glow_count = 0, glow_dx = 5;

// start the timing clock
Start_Clock();

// clear the drawing surface
DDraw_Fill_Surface(lpddsback, 0);

// lock the back buffer
DDraw_Lock_Back_Surface();

// draw the background reactor image
Draw_Bitmap(&reactor, back_buffer, back_lpitch, 0);

// unlock the back buffer
DDraw_Unlock_Back_Surface();

// read keyboard and other devices here
DInput_Read_Keyboard();

// reset motion flag
player_moving = 0;

// test direction of motion, this is a good example of testing the keyboard
// although the code could be optimized this is more educational

if (keyboard_state[DIK_RIGHT] && keyboard_state[DIK_UP]) 
   {
   // move skelaton
   skelaton.x+=2;
   skelaton.y-=2;
   dx=2; dy=-2;

   // set motion flag
   player_moving = 1;

   // check animation needs to change
   if (skelaton.curr_animation != SKELATON_NEAST)
      Set_Animation_BOB(&skelaton,SKELATON_NEAST);

   } // end if
else
if (keyboard_state[DIK_LEFT] && keyboard_state[DIK_UP]) 
   {
   // move skelaton
   skelaton.x-=2;
   skelaton.y-=2;
   dx=-2; dy=-2;

   // set motion flag
   player_moving = 1;

   // check animation needs to change
   if (skelaton.curr_animation != SKELATON_NWEST)
      Set_Animation_BOB(&skelaton,SKELATON_NWEST);

   } // end if
else
if (keyboard_state[DIK_LEFT] && keyboard_state[DIK_DOWN]) 
   {
   // move skelaton
   skelaton.x-=2;
   skelaton.y+=2;
   dx=-2; dy=2;

   // set motion flag
   player_moving = 1;

   // check animation needs to change
   if (skelaton.curr_animation != SKELATON_SWEST)
      Set_Animation_BOB(&skelaton,SKELATON_SWEST);

   } // end if
else
if (keyboard_state[DIK_RIGHT] && keyboard_state[DIK_DOWN]) 
   {
   // move skelaton
   skelaton.x+=2;
   skelaton.y+=2;
   dx=2; dy=2;

   // set motion flag
   player_moving = 1;

   // check animation needs to change
   if (skelaton.curr_animation != SKELATON_SEAST)
      Set_Animation_BOB(&skelaton,SKELATON_SEAST);

   } // end if
else
if (keyboard_state[DIK_RIGHT]) 
   {
   // move skelaton
   skelaton.x+=2;
   dx=2; dy=0;

   // set motion flag
   player_moving = 1;

   // check animation needs to change
   if (skelaton.curr_animation != SKELATON_EAST)
      Set_Animation_BOB(&skelaton,SKELATON_EAST);

   } // end if
else
if (keyboard_state[DIK_LEFT])  
   {
   // move skelaton
   skelaton.x-=2;
   dx=-2; dy=0; 
   
   // set motion flag
   player_moving = 1;

   // check animation needs to change
   if (skelaton.curr_animation != SKELATON_WEST)
      Set_Animation_BOB(&skelaton,SKELATON_WEST);

   } // end if
else
if (keyboard_state[DIK_UP])    
   {
   // move skelaton
   skelaton.y-=2;
   dx=0; dy=-2;
   
   // set motion flag
   player_moving = 1;

   // check animation needs to change
   if (skelaton.curr_animation != SKELATON_NORTH)
      Set_Animation_BOB(&skelaton,SKELATON_NORTH);

   } // end if
else
if (keyboard_state[DIK_DOWN])  
   {
   // move skelaton
   skelaton.y+=2;
   dx=0; dy=+2;

   // set motion flag
   player_moving = 1;

   // check animation needs to change
   if (skelaton.curr_animation != SKELATON_SOUTH)
      Set_Animation_BOB(&skelaton,SKELATON_SOUTH);

   } // end if

// only animate if player is moving
if (player_moving)
   {
   // animate skelaton
   Animate_BOB(&skelaton);


   // see if skelaton hit a wall
   
   // lock surface, so we can scan it
   DDraw_Lock_Back_Surface();
   
   // call the color scanner with WALL_ANIMATION_COLOR, the color of the glowing wall
   // try to center the scan in the center of the object to make it 
   // more realistic
   if (Color_Scan(skelaton.x+16, skelaton.y+16,
                  skelaton.x+skelaton.width-16, skelaton.y+skelaton.height-16,                                    
                  WALL_ANIMATION_COLOR, WALL_ANIMATION_COLOR, back_buffer,back_lpitch))
      {
      // back the skelaton up along its last trajectory
      skelaton.x-=dx;
      skelaton.y-=dy;
      } // end if
   
   // done, so unlock
   DDraw_Unlock_Back_Surface();

   // check if skelaton is off screen
   if (skelaton.x < 0 || skelaton.x > (screen_width - skelaton.width))
      skelaton.x-=dx;

   if (skelaton.y < 0 || skelaton.y > (screen_height - skelaton.height))
      skelaton.y-=dy;

   } // end if

// draw the skelaton
Draw_BOB(&skelaton, lpddsback);

// animate color
glow.peGreen+=glow_dx;

// test boundary
if (glow.peGreen == 0 || glow.peGreen == 255)
   glow_dx = -glow_dx;

Set_Palette_Entry(WALL_ANIMATION_COLOR, &glow);

// draw some text
Draw_Text_GDI("I STILL HAVE A BONE TO PICK!",0,screen_height - 32,WALL_ANIMATION_COLOR,lpddsback);

Draw_Text_GDI("USE ARROW KEYS TO MOVE, <ESC> TO EXIT.",0,0,RGB(32,32,32),lpddsback);


// flip the surfaces
DDraw_Flip();

// sync to 30ish fps
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