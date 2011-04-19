// DEMOII9_4_8b.CPP - 8-bit Tank demo with gouraud shading and texture mapping
// READ THIS!
// To compile make sure to include DDRAW.LIB, DSOUND.LIB,
// DINPUT.LIB, DINPUT8.LIB, WINMM.LIB in the project link list, and of course 
// the C++ source modules T3DLIB1-7.CPP and the headers T3DLIB1-7.H
// be in the working directory of the compiler

// INCLUDES ///////////////////////////////////////////////

#define DEBUG_ON

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
#include "T3DLIB4.h"
#include "T3DLIB5.h"
#include "T3DLIB6.h"
#include "T3DLIB7.h"

// DEFINES ////////////////////////////////////////////////

// defines for windows interface
#define WINDOW_CLASS_NAME "WIN3DCLASS"  // class name
#define WINDOW_TITLE      "T3D Graphics Console Ver 2.0"
#define WINDOW_WIDTH      800   // size of window
#define WINDOW_HEIGHT     600

#define WINDOW_BPP        8    // bitdepth of window (8,16,24 etc.)
                                // note: if windowed and not
                                // fullscreen then bitdepth must
                                // be same as system bitdepth
                                // also if 8-bit the a pallete
                                // is created and attached

#define WINDOWED_APP      0  // 0 not windowed, 1 windowed


// defines for the game universe
#define UNIVERSE_RADIUS   4000

#define POINT_SIZE        100
#define NUM_POINTS_X      (2*UNIVERSE_RADIUS/POINT_SIZE)
#define NUM_POINTS_Z      (2*UNIVERSE_RADIUS/POINT_SIZE)
#define NUM_POINTS        (NUM_POINTS_X*NUM_POINTS_Z)

// defines for objects
#define NUM_TOWERS        96
#define NUM_TANKS         32 
#define TANK_SPEED        15

// create some constants for ease of access
#define AMBIENT_LIGHT_INDEX   0 // ambient light index
#define INFINITE_LIGHT_INDEX  1 // infinite light index
#define POINT_LIGHT_INDEX     2 // point light index
#define SPOT_LIGHT1_INDEX     4 // point light index
#define SPOT_LIGHT2_INDEX     3 // spot light index

// PROTOTYPES /////////////////////////////////////////////

// game console
int Game_Init(void *parms=NULL);
int Game_Shutdown(void *parms=NULL);
int Game_Main(void *parms=NULL);

// GLOBALS ////////////////////////////////////////////////

HWND main_window_handle           = NULL; // save the window handle
HINSTANCE main_instance           = NULL; // save the instance
char buffer[2048];                        // used to print text

// initialize camera position and direction
POINT4D  cam_pos    = {0,40,0,1};
POINT4D  cam_target = {0,0,0,1};
VECTOR4D cam_dir    = {0,0,0,1};

// all your initialization code goes here...
VECTOR4D vscale={1.0,1.0,1.0,1}, 
         vpos = {0,0,0,1}, 
         vrot = {0,0,0,1};

CAM4DV1        cam;       // the single camera

OBJECT4DV2     obj_tower,    // used to hold the master tower
               obj_tank,     // used to hold the master tank
               obj_marker,   // the ground marker
               obj_player;   // the player object             

POINT4D        towers[NUM_TOWERS],
               tanks[NUM_TANKS];
               
RENDERLIST4DV2 rend_list; // the render list
BITMAP_IMAGE textures[12]; // holds our texture library 
RGBAV1 white, gray, black, red, green, blue;

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

int index; // looping var

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
if (!WINDOWED_APP)
    ShowCursor(FALSE);

// seed random number generator
srand(Start_Clock());

Open_Error_File("ERROR.TXT");

// initialize math engine
Build_Sin_Cos_Tables();

// initialize the camera with 90 FOV, normalized coordinates
Init_CAM4DV1(&cam,      // the camera object
             CAM_MODEL_EULER, // the euler model
             &cam_pos,  // initial camera position
             &cam_dir,  // initial camera angles
             &cam_target,      // no target
             200.0,      // near and far clipping planes
             12000.0,
             120.0,      // field of view in degrees
             WINDOW_WIDTH,   // size of final screen viewport
             WINDOW_HEIGHT);

// load the master tank object
VECTOR4D_INITXYZ(&vscale,0.75,0.75,0.75);
Load_OBJECT4DV2_PLG(&obj_tank, "tank3_8b.plg",&vscale, &vpos, &vrot);

// load player object for 3rd person view
VECTOR4D_INITXYZ(&vscale,30.75,30.75,30.75);

Load_OBJECT4DV2_COB(&obj_player,"tie02_256.cob",  
                       &vscale, &vpos, &vrot, VERTEX_FLAGS_INVERT_TEXTURE_V | 
                                              VERTEX_FLAGS_SWAP_YZ | 
                                              VERTEX_FLAGS_TRANSFORM_LOCAL_WORLD );

// load the master tower object
VECTOR4D_INITXYZ(&vscale,1.0, 2.0, 1.0);
Load_OBJECT4DV2_PLG(&obj_tower, "towerg1_8b.plg",&vscale, &vpos, &vrot);

// load the master ground marker
VECTOR4D_INITXYZ(&vscale,3.0,3.0,3.0);
Load_OBJECT4DV2_PLG(&obj_marker, "marker1_8b.plg",&vscale, &vpos, &vrot);

// position the tanks
for (index = 0; index < NUM_TANKS; index++)
    {
    // randomly position the tanks
    tanks[index].x = RAND_RANGE(-UNIVERSE_RADIUS, UNIVERSE_RADIUS);
    tanks[index].y = 0; // obj_tank.max_radius;
    tanks[index].z = RAND_RANGE(-UNIVERSE_RADIUS, UNIVERSE_RADIUS);
    tanks[index].w = RAND_RANGE(0,360);
    } // end for

// position the towers
for (index = 0; index < NUM_TOWERS; index++)
    {
    // randomly position the tower
    towers[index].x = RAND_RANGE(-UNIVERSE_RADIUS, UNIVERSE_RADIUS);
    towers[index].y = 0; // obj_tower.max_radius;
    towers[index].z = RAND_RANGE(-UNIVERSE_RADIUS, UNIVERSE_RADIUS);
    } // end for

// set up lights
Reset_Lights_LIGHTV1();

// create some working colors

white.rgba = _RGBA32BIT(255,255,255,0);
gray.rgba  = _RGBA32BIT(100,100,100,0);
black.rgba = _RGBA32BIT(0,0,0,0);
red.rgba   = _RGBA32BIT(255,0,0,0);
green.rgba = _RGBA32BIT(0,255,0,0);
blue.rgba  = _RGBA32BIT(0,0,255,0);


// ambient light
Init_Light_LIGHTV1(AMBIENT_LIGHT_INDEX,   
                   LIGHTV1_STATE_ON,      // turn the light on
                   LIGHTV1_ATTR_AMBIENT,  // ambient light type
                   gray, black, black,    // color for ambient term only
                   NULL, NULL,            // no need for pos or dir
                   0,0,0,                 // no need for attenuation
                   0,0,0);                // spotlight info NA

VECTOR4D dlight_dir = {-1,0,-1,0};

// directional light
Init_Light_LIGHTV1(INFINITE_LIGHT_INDEX,  
                   LIGHTV1_STATE_ON,      // turn the light on
                   LIGHTV1_ATTR_INFINITE, // infinite light type
                   black, gray, black,    // color for diffuse term only
                   NULL, &dlight_dir,     // need direction only
                   0,0,0,                 // no need for attenuation
                   0,0,0);                // spotlight info NA


VECTOR4D plight_pos = {0,200,0,0};

// point light
Init_Light_LIGHTV1(POINT_LIGHT_INDEX,
                   LIGHTV1_STATE_ON,      // turn the light on
                   LIGHTV1_ATTR_POINT,    // pointlight type
                   black, green, black,   // color for diffuse term only
                   &plight_pos, NULL,     // need pos only
                   0,.001,0,              // linear attenuation only
                   0,0,1);                // spotlight info NA

VECTOR4D slight2_pos = {0,200,0,0};
VECTOR4D slight2_dir = {-1,0,-1,0};

// spot light2
Init_Light_LIGHTV1(SPOT_LIGHT2_INDEX,
                   LIGHTV1_STATE_ON,           // turn the light on
                   LIGHTV1_ATTR_SPOTLIGHT2,    // spot light type 2
                   black, red, black,          // color for diffuse term only
                   &slight2_pos, &slight2_dir, // need pos only
                   0,.001,0,                   // linear attenuation only
                   0,0,1);    

// create lookup for lighting engine
RGB_16_8_IndexedRGB_Table_Builder(DD_PIXEL_FORMAT565,  // format we want to build table for
                                  palette,             // source palette
                                  rgblookup);          // lookup table

// create 4.4.4 to 8 bit index color lookup for texture mapping
RGB_12_8_Lighting_Table_Builder(palette,         // source palette
                                rgblightlookup);  // lookup table

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

// now directsound
DSound_Stop_All_Sounds();
DSound_Delete_All_Sounds();
DSound_Shutdown();

// directmusic
DMusic_Delete_All_MIDI();
DMusic_Shutdown();

// shut down directinput
DInput_Release_Keyboard();

// shutdown directinput
DInput_Shutdown();

// shutdown directdraw last
DDraw_Shutdown();

Close_Error_File();

// return success
return(1);
} // end Game_Shutdown

//////////////////////////////////////////////////////////

int Game_Main(void *parms)
{
// this is the workhorse of your game it will be called
// continuously in real-time this is like main() in C
// all the calls for you game go here!

static MATRIX4X4 mrot;   // general rotation matrix

// these are used to create a circling camera
static float view_angle = 0; 
static float camera_distance = 6000;
static VECTOR4D pos = {0,0,0,0};
static float tank_speed;
static float turning = 0;
// state variables for different rendering modes and help
static int wireframe_mode = 1;
static int backface_mode  = 1;
static int lighting_mode  = 1;
static int help_mode      = 1;
static int zsort_mode     = 1;

char work_string[256]; // temp string

int index; // looping var

// start the timing clock
Start_Clock();

// clear the drawing surface 
//DDraw_Fill_Surface(lpddsback, 0);

#if 1
// draw the sky
//Draw_Rectangle(0,0, WINDOW_WIDTH-1, WINDOW_HEIGHT/2, 166, lpddsback);
//Draw_Rectangle(0,WINDOW_HEIGHT/2, WINDOW_WIDTH-1, WINDOW_HEIGHT-1, rgblookup[RGB16Bit565(115,42,16)], lpddsback);
Draw_Rectangle(0,0, WINDOW_WIDTH, WINDOW_HEIGHT/2, rgblookup[RGB16Bit565(0,35,50)], lpddsback);
//Draw_Rectangle(0,0, WINDOW_WIDTH-1, WINDOW_HEIGHT/2, RGB16Bit(0,140,192), lpddsback);

// draw the ground
//Draw_Rectangle(0,WINDOW_HEIGHT/2, WINDOW_WIDTH-1, WINDOW_HEIGHT-1, 28, lpddsback);
Draw_Rectangle(0,WINDOW_HEIGHT/2-1, WINDOW_WIDTH, WINDOW_HEIGHT, rgblookup[RGB16Bit565(20,12,0)], lpddsback);
//Draw_Rectangle(0,WINDOW_HEIGHT/2, WINDOW_WIDTH-1, WINDOW_HEIGHT-1, RGB16Bit(103,62,3), lpddsback);


// read keyboard and other devices here
DInput_Read_Keyboard();

// game logic here...

// reset the render list
Reset_RENDERLIST4DV2(&rend_list);

// allow user to move camera

// turbo
if (keyboard_state[DIK_SPACE])
    tank_speed = 5*TANK_SPEED;
else
    tank_speed = TANK_SPEED;

// forward/backward
if (keyboard_state[DIK_UP])
   {
   // move forward
   cam.pos.x += tank_speed*Fast_Sin(cam.dir.y);
   cam.pos.z += tank_speed*Fast_Cos(cam.dir.y);
   } // end if

if (keyboard_state[DIK_DOWN])
   {
   // move backward
   cam.pos.x -= tank_speed*Fast_Sin(cam.dir.y);
   cam.pos.z -= tank_speed*Fast_Cos(cam.dir.y);
   } // end if

// rotate
if (keyboard_state[DIK_RIGHT])
   {
   cam.dir.y+=3;
   
   // add a little turn to object
   if ((turning+=2) > 15)
      turning=15;
   } // end if

if (keyboard_state[DIK_LEFT])
   {
   cam.dir.y-=3;

   // add a little turn to object
   if ((turning-=2) < -15)
      turning=-15;

   } // end if
else // center heading again
   {
   if (turning > 0)
       turning-=1;
   else
   if (turning < 0)
       turning+=1;

   } // end else

// modes and lights

// wireframe mode
if (keyboard_state[DIK_W])
   {
   // toggle wireframe mode
   if (++wireframe_mode > 1)
       wireframe_mode=0;
   Wait_Clock(100); // wait, so keyboard doesn't bounce
   } // end if

// backface removal
if (keyboard_state[DIK_B])
   {
   // toggle backface removal
   backface_mode = -backface_mode;
   Wait_Clock(100); // wait, so keyboard doesn't bounce
   } // end if

// lighting
if (keyboard_state[DIK_L])
   {
   // toggle lighting engine completely
   lighting_mode = -lighting_mode;
   Wait_Clock(100); // wait, so keyboard doesn't bounce
   } // end if

// toggle ambient light
if (keyboard_state[DIK_A])
   {
   // toggle ambient light
   if (lights[AMBIENT_LIGHT_INDEX].state == LIGHTV1_STATE_ON)
      lights[AMBIENT_LIGHT_INDEX].state = LIGHTV1_STATE_OFF;
   else
      lights[AMBIENT_LIGHT_INDEX].state = LIGHTV1_STATE_ON;

   Wait_Clock(100); // wait, so keyboard doesn't bounce
   } // end if

// toggle infinite light
if (keyboard_state[DIK_I])
   {
   // toggle ambient light
   if (lights[INFINITE_LIGHT_INDEX].state == LIGHTV1_STATE_ON)
      lights[INFINITE_LIGHT_INDEX].state = LIGHTV1_STATE_OFF;
   else
      lights[INFINITE_LIGHT_INDEX].state = LIGHTV1_STATE_ON;

   Wait_Clock(100); // wait, so keyboard doesn't bounce
   } // end if

// toggle point light
if (keyboard_state[DIK_P])
   {
   // toggle point light
   if (lights[POINT_LIGHT_INDEX].state == LIGHTV1_STATE_ON)
      lights[POINT_LIGHT_INDEX].state = LIGHTV1_STATE_OFF;
   else
      lights[POINT_LIGHT_INDEX].state = LIGHTV1_STATE_ON;

   Wait_Clock(100); // wait, so keyboard doesn't bounce
   } // end if


// toggle spot light
if (keyboard_state[DIK_S])
   {
   // toggle spot light
   if (lights[SPOT_LIGHT2_INDEX].state == LIGHTV1_STATE_ON)
      lights[SPOT_LIGHT2_INDEX].state = LIGHTV1_STATE_OFF;
   else
      lights[SPOT_LIGHT2_INDEX].state = LIGHTV1_STATE_ON;

   Wait_Clock(100); // wait, so keyboard doesn't bounce
   } // end if


// help menu
if (keyboard_state[DIK_H])
   {
   // toggle help menu 
   help_mode = -help_mode;
   Wait_Clock(100); // wait, so keyboard doesn't bounce
   } // end if

// z-sorting
if (keyboard_state[DIK_Z])
   {
   // toggle z sorting
   zsort_mode = -zsort_mode;
   Wait_Clock(100); // wait, so keyboard doesn't bounce
   } // end if



static float plight_ang = 0, slight_ang = 0; // angles for light motion

// move point light source in ellipse around game world
lights[POINT_LIGHT_INDEX].pos.x = 4000*Fast_Cos(plight_ang);
lights[POINT_LIGHT_INDEX].pos.y = 200;
lights[POINT_LIGHT_INDEX].pos.z = 4000*Fast_Sin(plight_ang);

if ((plight_ang+=3) > 360)
    plight_ang = 0;

// move spot light source in ellipse around game world
lights[SPOT_LIGHT2_INDEX].pos.x = 2000*Fast_Cos(slight_ang);
lights[SPOT_LIGHT2_INDEX].pos.y = 200;
lights[SPOT_LIGHT2_INDEX].pos.z = 2000*Fast_Sin(slight_ang);

if ((slight_ang-=5) < 0)
    slight_ang = 360;

// generate camera matrix
Build_CAM4DV1_Matrix_Euler(&cam, CAM_ROT_SEQ_ZYX);

// insert the player into the world
// reset the object (this only matters for backface and object removal)
Reset_OBJECT4DV2(&obj_player);

// set position of tank

obj_player.world_pos.x = cam.pos.x+300*Fast_Sin(cam.dir.y);
obj_player.world_pos.y = cam.pos.y-70;
obj_player.world_pos.z = cam.pos.z+300*Fast_Cos(cam.dir.y);


// generate rotation matrix around y axis
static int turn=0; 
Build_XYZ_Rotation_MATRIX4X4(0, cam.dir.y+turning, 0, &mrot);

// rotate the local coords of the object
Transform_OBJECT4DV2(&obj_player, &mrot, TRANSFORM_LOCAL_TO_TRANS,1);

// perform world transform
Model_To_World_OBJECT4DV2(&obj_player, TRANSFORM_TRANS_ONLY);

//Light_OBJECT4DV2_World(&obj_player, &cam, lights, 4);

// insert the object into render list
Insert_OBJECT4DV2_RENDERLIST4DV2(&rend_list, &obj_player,0);

#if 1

//////////////////////////////////////////////////////////

// insert the tanks in the world
for (index = 0; index < NUM_TANKS; index++)
    {
    // reset the object (this only matters for backface and object removal)
    Reset_OBJECT4DV2(&obj_tank);

    // generate rotation matrix around y axis
    Build_XYZ_Rotation_MATRIX4X4(0, tanks[index].w, 0, &mrot);

    // rotate the local coords of the object
    Transform_OBJECT4DV2(&obj_tank, &mrot, TRANSFORM_LOCAL_TO_TRANS,1);

    // set position of tank
    obj_tank.world_pos.x = tanks[index].x;
    obj_tank.world_pos.y = tanks[index].y;
    obj_tank.world_pos.z = tanks[index].z;

    // attempt to cull object   
    if (!Cull_OBJECT4DV2(&obj_tank, &cam, CULL_OBJECT_XYZ_PLANES))
       {
       // if we get here then the object is visible at this world position
       // so we can insert it into the rendering list
       // perform local/model to world transform
       Model_To_World_OBJECT4DV2(&obj_tank, TRANSFORM_TRANS_ONLY);

       //Light_OBJECT4DV2_World(&obj_tank, &cam, lights, 4);

       // insert the object into render list
       Insert_OBJECT4DV2_RENDERLIST4DV2(&rend_list, &obj_tank,0);
       } // end if
 
    } // end for

////////////////////////////////////////////////////////


// insert the towers in the world
for (index = 0; index < NUM_TOWERS; index++)
    {
    // reset the object (this only matters for backface and object removal)
    Reset_OBJECT4DV2(&obj_tower);

    // set position of tower
    obj_tower.world_pos.x = towers[index].x;
    obj_tower.world_pos.y = towers[index].y;
    obj_tower.world_pos.z = towers[index].z;

    // attempt to cull object   
    if (!Cull_OBJECT4DV2(&obj_tower, &cam, CULL_OBJECT_XYZ_PLANES))
       {
       // if we get here then the object is visible at this world position
       // so we can insert it into the rendering list
       // perform local/model to world transform
       Model_To_World_OBJECT4DV2(&obj_tower);

       //Light_OBJECT4DV2_World(&obj_tower, &cam, lights, 4);

       // insert the object into render list
       Insert_OBJECT4DV2_RENDERLIST4DV2(&rend_list, &obj_tower,0);

       } // end if
 
    } // end for

///////////////////////////////////////////////////////////////

// seed number generator so that modulation of markers is always the same
srand(13);

static int mcount = 0, mdir = 2;

mcount+=mdir;
if (mcount > 200 || mcount < -200) { mdir=-mdir; mcount+=mdir; }

// insert the ground markers into the world
for (int index_x = 0; index_x < NUM_POINTS_X; index_x++)
    for (int index_z = 0; index_z < NUM_POINTS_Z; index_z++)
        {
        // reset the object (this only matters for backface and object removal)
        Reset_OBJECT4DV2(&obj_marker);

        // set position of tower
        obj_marker.world_pos.x = RAND_RANGE(-100,100)-UNIVERSE_RADIUS+index_x*POINT_SIZE;
        obj_marker.world_pos.y = obj_marker.max_radius[0] + 50*Fast_Sin(index_x*10+Fast_Sin(index_z)+mcount);
        obj_marker.world_pos.z = RAND_RANGE(-100,100)-UNIVERSE_RADIUS+index_z*POINT_SIZE;

        // attempt to cull object   
        if (!Cull_OBJECT4DV2(&obj_marker, &cam, CULL_OBJECT_XYZ_PLANES))
           {
           // if we get here then the object is visible at this world position
           // so we can insert it into the rendering list
           // perform local/model to world transform
           Model_To_World_OBJECT4DV2(&obj_marker);

           //Light_OBJECT4DV2_World(&obj_marker, &cam, lights, 4);

           // insert the object into render list
           Insert_OBJECT4DV2_RENDERLIST4DV2(&rend_list, &obj_marker,0);

           } // end if

        } // end for

////////////////////////////////////////////////////////////////////////

#endif

// remove backfaces
if (backface_mode==1)
   Remove_Backfaces_RENDERLIST4DV2(&rend_list, &cam);

// light scene all at once 
if (lighting_mode==1)
   Light_RENDERLIST4DV2_World(&rend_list, &cam, lights, 4);

// apply world to camera transform
World_To_Camera_RENDERLIST4DV2(&rend_list, &cam);

// sort the polygon list (hurry up!)
if (zsort_mode == 1)
   Sort_RENDERLIST4DV2(&rend_list,  SORT_POLYLIST_AVGZ);

// apply camera to perspective transformation
Camera_To_Perspective_RENDERLIST4DV2(&rend_list, &cam);

// apply screen transform
Perspective_To_Screen_RENDERLIST4DV2(&rend_list, &cam);

sprintf(work_string,"pos:[%f, %f, %f] heading:[%f] elev:[%f], polys[%d]", 
        cam.pos.x, cam.pos.y, cam.pos.z, cam.dir.y, cam.dir.x, debug_polys_rendered_per_frame); 

Draw_Text_GDI(work_string, 0, WINDOW_HEIGHT-20, RGB(0,255,0), lpddsback);

sprintf(work_string,"Lighting [%s]: Ambient=%d, Infinite=%d, Point=%d, Spot=%d | Zsort [%s], BckFceRM [%s]", 
                                                                                 ((lighting_mode == 1) ? "ON" : "OFF"),
                                                                                 lights[AMBIENT_LIGHT_INDEX].state,
                                                                                 lights[INFINITE_LIGHT_INDEX].state, 
                                                                                 lights[POINT_LIGHT_INDEX].state,
                                                                                 lights[SPOT_LIGHT2_INDEX].state,
                                                                                 ((zsort_mode == 1) ? "ON" : "OFF"),
                                                                                 ((backface_mode == 1) ? "ON" : "OFF"));

Draw_Text_GDI(work_string, 0, WINDOW_HEIGHT-34, RGB(0,255,0), lpddsback);

// draw instructions
Draw_Text_GDI("Press ESC to exit. Press <H> for Help.", 0, 0, RGB(0,255,0), lpddsback);

// should we display help
int text_y = 16;
if (help_mode==1)
    {
    // draw help menu
    Draw_Text_GDI("<A>..............Toggle ambient light source.", 0, text_y+=12, RGB(255,255,255), lpddsback);
    Draw_Text_GDI("<I>..............Toggle infinite light source.", 0, text_y+=12, RGB(255,255,255), lpddsback);
    Draw_Text_GDI("<P>..............Toggle point light source.", 0, text_y+=12, RGB(255,255,255), lpddsback);
    Draw_Text_GDI("<S>..............Toggle spot light source.", 0, text_y+=12, RGB(255,255,255), lpddsback);
    Draw_Text_GDI("<W>..............Toggle wire frame/solid mode.", 0, text_y+=12, RGB(255,255,255), lpddsback);
    Draw_Text_GDI("<B>..............Toggle backface removal.", 0, text_y+=12, RGB(255,255,255), lpddsback);
    Draw_Text_GDI("<RIGHT ARROW>....Rotate player right.", 0, text_y+=12, RGB(255,255,255), lpddsback);
    Draw_Text_GDI("<LEFT ARROW>.....Rotate player left.", 0, text_y+=12, RGB(255,255,255), lpddsback);
    Draw_Text_GDI("<UP ARROW>.......Move player forward.", 0, text_y+=12, RGB(255,255,255), lpddsback);
    Draw_Text_GDI("<DOWN ARROW>.....Move player backward.", 0, text_y+=12, RGB(255,255,255), lpddsback);
    Draw_Text_GDI("<SPACE BAR>......Turbo.", 0, text_y+=12, RGB(255,255,255), lpddsback);
    Draw_Text_GDI("<H>..............Toggle Help.", 0, text_y+=12, RGB(255,255,255), lpddsback);
    Draw_Text_GDI("<ESC>............Exit demo.", 0, text_y+=12, RGB(255,255,255), lpddsback);

    } // end help

// lock the back buffer
DDraw_Lock_Back_Surface();

// reset number of polys rendered
debug_polys_rendered_per_frame = 0;

// render the object

if (wireframe_mode  == 0)
   Draw_RENDERLIST4DV2_Wire(&rend_list, back_buffer, back_lpitch);
else
if (wireframe_mode  == 1)
   Draw_RENDERLIST4DV2_Solid(&rend_list, back_buffer, back_lpitch);

#endif

// unlock the back buffer
DDraw_Unlock_Back_Surface();

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