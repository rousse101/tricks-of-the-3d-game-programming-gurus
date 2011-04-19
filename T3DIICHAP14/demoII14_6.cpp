// DEMOII14_6.CPP - shadow demo with lightmapping and fan model
// READ THIS!
// To compile make sure to include DDRAW.LIB, DSOUND.LIB,
// DINPUT.LIB, DINPUT8.LIB, WINMM.LIB in the project link list, and of course 
// the C++ source modules T3DLIB1-12.CPP and the headers T3DLIB1-12.H
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
#include <iostream.h>  // include important C/C++ stuff
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

#include <ddraw.h>     // directX includes 
#include <dsound.h> 
#include <dmksctrl.h>
#include <dmusici.h>
#include <dmusicc.h>
#include <dmusicf.h>
#include <dinput.h>
#include "T3DLIB1.h"   // game library includes
#include "T3DLIB2.h"
#include "T3DLIB3.h"
#include "T3DLIB4.h"
#include "T3DLIB5.h"
#include "T3DLIB6.h"
#include "T3DLIB7.h"
#include "T3DLIB8.h"
#include "T3DLIB9.h"
#include "T3DLIB10.h"
#include "T3DLIB11.h"
#include "T3DLIB12.h"
                                 
// DEFINES ////////////////////////////////////////////////

// defines for windows interface
#define WINDOW_CLASS_NAME "WIN3DCLASS"  // class name
#define WINDOW_TITLE      "T3D Graphics Console Ver 2.0"
#define WINDOW_WIDTH      800  // size of window
#define WINDOW_HEIGHT     600

#define WINDOW_BPP        16    // bitdepth of window (8,16,24 etc.)
                                // note: if windowed and not
                                // fullscreen then bitdepth must
                                // be same as system bitdepth
                                // also if 8-bit the a pallete
                                // is created and attached
   
#define WINDOWED_APP      0 // 0 not windowed, 1 windowed

// create some constants for ease of access
#define AMBIENT_LIGHT_INDEX   0 // ambient light index 
#define INFINITE_LIGHT_INDEX  1 // infinite light index
#define POINT_LIGHT_INDEX     2 // point light index
#define POINT_LIGHT2_INDEX    3 // point light index

// terrain defines
#define TERRAIN_WIDTH         1000
#define TERRAIN_HEIGHT        1000
#define TERRAIN_SCALE         700
#define MAX_SPEED             20

#define PITCH_RETURN_RATE (.33) // how fast the pitch straightens out
#define PITCH_CHANGE_RATE (.02) // the rate that pitch changes relative to height 
#define CAM_HEIGHT_SCALER (.3)  // percentage cam height changes relative to height
#define VELOCITY_SCALER (.025)  // rate velocity changes with height
#define CAM_DECEL       (.25)   // deceleration/friction


#define NUM_OBJECTS          3 // number of objects per class

#define NUM_LIGHT_OBJECTS    5 // number of light models

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
POINT4D  cam_pos    = {0,500,-400,1};
POINT4D  cam_target = {0,0,0,1};
VECTOR4D cam_dir    = {0,0,0,1};

// all your initialization code goes here...
VECTOR4D vscale={1.0,1.0,1.0,1}, 
         vpos = {0,0,150,1}, 
         vrot = {0,0,0,1};

CAM4DV1         cam;            // the single camera
OBJECT4DV2      obj_terrain;    // the terrain object

OBJECT4DV2_PTR  obj_work;
OBJECT4DV2      obj_array[NUM_OBJECTS]; 

OBJECT4DV2_PTR  obj_light;
OBJECT4DV2      obj_light_array[NUM_LIGHT_OBJECTS];

OBJECT4DV2      shadow_obj; 

// filenames of objects to load
char *object_filenames[NUM_OBJECTS] = {
                                        "fan_01b.cob",
                                        "sphere_gouraud_textured_02.cob",  
                                        "fire_constant_cube01.cob",
                                      };

int curr_object  = 0;

#define INDEX_RED_LIGHT_INDEX       0
#define INDEX_GREEN_LIGHT_INDEX     1
#define INDEX_BLUE_LIGHT_INDEX      2
#define INDEX_YELLOW_LIGHT_INDEX    3
#define INDEX_WHITE_LIGHT_INDEX     4

// filenames of objects to load
char *object_light_filenames[NUM_LIGHT_OBJECTS] = {
                                            "cube_constant_red_01.cob",
                                            "cube_constant_green_01.cob",  
                                            "cube_constant_blue_01.cob",
                                            "cube_constant_yellow_01.cob",
                                            "cube_constant_white_01.cob",
                                            };
int curr_light_object  = 0;

RENDERLIST4DV2  rend_list;      // the render list

RGBAV1          white,          // general colors 
                gray, 
                black, 
                red, 
                green, 
                blue,
                yellow,
                orange; 

ZBUFFERV1 zbuffer;   // our little z buffer!
RENDERCONTEXTV1  rc; // the rendering context;

// physical model defines play with these to change the feel of the vehicle
float gravity       = -.40;    // general gravity
float vel_y         = 0;       // the y velocity of camera/jeep
float cam_speed     = 0;       // speed of the camera/jeep
float sea_level     = 50;      // sea level of the simulation
float gclearance    = 125;      // clearance from the camera to the ground
float neutral_pitch = 10;   // the neutral pitch of the camera

// sounds
int wind_sound_id = -1;

// game imagery assets
BOB cockpit;               // the cockpit image

#define NUM_LIGHTMAPS   9  // 

BITMAP_IMAGE lightmaps[NUM_LIGHTMAPS];   // the light maps
int curr_lightmap = 0;
BITMAP_IMAGE texture_copy;               // copy of the texture

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
           0,                                    // x position
           0,                                    // y position
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
DDraw_Init2(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_BPP, WINDOWED_APP,0);

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
Init_CAM4DV1(&cam,            // the camera object
             CAM_MODEL_EULER, // the euler model
             &cam_pos,        // initial camera position
             &cam_dir,        // initial camera angles
             &cam_target,     // no target
             10.0,            // near and far clipping planes
             12000.0,
             90.0,            // field of view in degrees
             WINDOW_WIDTH,    // size of final screen viewport
             WINDOW_HEIGHT);

VECTOR4D terrain_pos = {0,0,0,0}; 

Generate_Terrain_OBJECT4DV2(&obj_terrain,            // pointer to object
                            TERRAIN_WIDTH,           // width in world coords on x-axis
                            TERRAIN_HEIGHT,          // height (length) in world coords on z-axis
                            TERRAIN_SCALE,           // vertical scale of terrain
                            "heighlightmap_01.bmp",  // filename of height bitmap encoded in 256 colors
                            "sandstone256_256_01.bmp", // "grass256_256_01.bmp", //"checker2562562.bmp",   // filename of texture map
                             RGB16Bit(255,255,255),  // color of terrain if no texture        
                             &terrain_pos,           // initial position
                             NULL,                   // initial rotations
                             POLY4DV2_ATTR_RGB16  
                             //| POLY4DV2_ATTR_SHADE_MODE_FLAT 
                             | POLY4DV2_ATTR_SHADE_MODE_GOURAUD
                             | POLY4DV2_ATTR_SHADE_MODE_TEXTURE);



// now make a copy of the texture bitmap to use as texture source for the 
// lightmapping along with the light map
Create_Bitmap(&texture_copy, 0, 0, 256, 256, 16);
Copy_Bitmap(&texture_copy, 0,0, 
             obj_terrain.texture,0,0,
             256,256);

// set a scaling vector 
VECTOR4D_INITXYZ(&vscale, 20, 20, 20);  

// load all the objects in
for (int index_obj=0; index_obj < NUM_OBJECTS; index_obj++)
    {
    Load_OBJECT4DV2_COB2(&obj_array[index_obj], object_filenames[index_obj],  
                        &vscale, &vpos, &vrot, VERTEX_FLAGS_SWAP_YZ // | VERTEX_FLAGS_INVERT_WINDING_ORDER 
                                               | VERTEX_FLAGS_TRANSFORM_LOCAL 
                                               | VERTEX_FLAGS_TRANSFORM_LOCAL_WORLD
                                               ,0 );

    // angle for circular rotation
    obj_array[index_obj].ivar1 = 0;

    // set initial position
    obj_array[index_obj].world_pos.x = 0;
    obj_array[index_obj].world_pos.y = 200;
    obj_array[index_obj].world_pos.z = 0;
    
    } // end for index_obj

// set current object
curr_object = 0;
obj_work    = &obj_array[curr_object];

// set a scaling vector
VECTOR4D_INITXYZ(&vscale, 20, 20, 20); 

// load all the light objects in
for (index_obj=0; index_obj < NUM_LIGHT_OBJECTS; index_obj++)
    {
    Load_OBJECT4DV2_COB2(&obj_light_array[index_obj], object_light_filenames[index_obj],  
                        &vscale, &vpos, &vrot, VERTEX_FLAGS_INVERT_WINDING_ORDER 
                                               | VERTEX_FLAGS_TRANSFORM_LOCAL 
                                               | VERTEX_FLAGS_TRANSFORM_LOCAL_WORLD
                                              ,0 );
    } // end for index
   
// set current object
curr_light_object = 0;
obj_light    = &obj_light_array[curr_light_object];



// load the shadow object in
VECTOR4D_INITXYZ(&vscale, 1, 1, 1); 
Load_OBJECT4DV2_COB2(&shadow_obj,"shadow_poly_01.cob",  
                    &vscale, &vpos, &vrot, VERTEX_FLAGS_INVERT_WINDING_ORDER | VERTEX_FLAGS_SWAP_YZ 
                                          | VERTEX_FLAGS_TRANSFORM_LOCAL 
                                          | VERTEX_FLAGS_TRANSFORM_LOCAL_WORLD
                                          ,0 );
// set up lights
Reset_Lights_LIGHTV2(lights2, MAX_LIGHTS);

// create some working colors
white.rgba = _RGBA32BIT(255,255,255,0);
gray.rgba  = _RGBA32BIT(100,100,100,0);
black.rgba = _RGBA32BIT(0,0,0,0);
red.rgba   = _RGBA32BIT(255,0,0,0);
green.rgba = _RGBA32BIT(0,255,0,0);
blue.rgba  = _RGBA32BIT(0,0,255,0);
orange.rgba = _RGBA32BIT(255,128,0,0);
yellow.rgba  = _RGBA32BIT(255,255,0,0);

// ambient light
Init_Light_LIGHTV2(lights2,
                   AMBIENT_LIGHT_INDEX,   
                   LIGHTV2_STATE_ON,      // turn the light on
                   LIGHTV2_ATTR_AMBIENT,  // ambient light type
                   gray, black, black,    // color for ambient term only
                   NULL, NULL,            // no need for pos or dir
                   0,0,0,                 // no need for attenuation
                   0,0,0);                // spotlight info NA

VECTOR4D dlight_dir = {-1,1,-1,1};

// directional light
Init_Light_LIGHTV2(lights2,
                   INFINITE_LIGHT_INDEX,  
                   LIGHTV2_STATE_ON,      // turn the light on
                   LIGHTV2_ATTR_INFINITE, // infinite light type
                   black, gray, black,    // color for diffuse term only
                   NULL, &dlight_dir,     // need direction only
                   0,0,0,                 // no need for attenuation
                   0,0,0);                // spotlight info NA


VECTOR4D plight_pos = {0,500,0,1};

// point light
Init_Light_LIGHTV2(lights2,
                   POINT_LIGHT_INDEX,
                   LIGHTV2_STATE_ON,      // turn the light on
                   LIGHTV2_ATTR_POINT,    // pointlight type
                   black, green, black,   // color for diffuse term only
                   &plight_pos, NULL,     // need pos only
                   0,.001,0,              // linear attenuation only
                   0,0,1);                // spotlight info NA


// point light
Init_Light_LIGHTV2(lights2,
                   POINT_LIGHT2_INDEX,
                   LIGHTV2_STATE_ON,     // turn the light on
                   LIGHTV2_ATTR_POINT,   // pointlight type
                   black, red, black,  // color for diffuse term only
                   &plight_pos, NULL,    // need pos only
                   0,.002,0,             // linear attenuation only
                   0,0,1);               // spotlight info NA

VECTOR4D slight2_pos = {0,200,0,1};
VECTOR4D slight2_dir = {-1,1,-1,1};


// create lookup for lighting engine
RGB_16_8_IndexedRGB_Table_Builder(DD_PIXEL_FORMAT565,  // format we want to build table for
                                  palette,             // source palette
                                  rgblookup);          // lookup table

// create the z buffer
Create_Zbuffer(&zbuffer,
               WINDOW_WIDTH,
               WINDOW_HEIGHT,
               ZBUFFER_ATTR_32BIT);
 

// build alpha lookup table
RGB_Alpha_Table_Builder(NUM_ALPHA_LEVELS, rgb_alpha_table);

// load background sounds
wind_sound_id = DSound_Load_WAV("STATIONTHROB.WAV");

// start the sounds
DSound_Play(wind_sound_id, DSBPLAY_LOOPING);
DSound_Set_Volume(wind_sound_id, 100);

#if 0
// load in the cockpit image
Create_BOB(&cockpit, 0,0,800,600,2, BOB_ATTR_VISIBLE | BOB_ATTR_SINGLE_FRAME, DDSCAPS_SYSTEMMEMORY, 0, 16); 
Load_Bitmap_File(&bitmap16bit, "lego02.BMP");
Load_Frame_BOB16(&cockpit, &bitmap16bit,0,0,0,BITMAP_EXTRACT_MODE_ABS);
Unload_Bitmap_File(&bitmap16bit);

Load_Bitmap_File(&bitmap16bit, "lego02b.BMP");
Load_Frame_BOB16(&cockpit, &bitmap16bit,1,0,0,BITMAP_EXTRACT_MODE_ABS);
Unload_Bitmap_File(&bitmap16bit);
#endif


// load in the bitmaps for the lightmaps
for (index=0; index < NUM_LIGHTMAPS; index++)
    {
    sprintf(buffer,"fanlightmaps_0%d.bmp",index+1);

    Load_Bitmap_File(&bitmap16bit, buffer);
    Create_Bitmap(&lightmaps[index],0,0,256,256, 16);
    Load_Image_Bitmap16(&lightmaps[index], &bitmap16bit,0,0,BITMAP_EXTRACT_MODE_ABS);
    Unload_Bitmap_File(&bitmap16bit);                                    
    } // end for index


// set single precission
//_control87( _PC_24, MCW_PC );

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

static float plight_ang = 0, 
             slight_ang = 0; // angles for light motion

// use these to rotate objects
static float x_ang = 0, y_ang = 0, z_ang = 0;

// state variables for different rendering modes and help
static int wireframe_mode   = 1;
static int backface_mode    = 1;
static int lighting_mode    = 1;
static int help_mode        = 1;
static int zsort_mode       = 1;
static int x_clip_mode      = 1;
static int y_clip_mode      = 1;
static int z_clip_mode      = 1;

static float hl = 300, // artificial light height
             ks = 1.25; // generic scaling factor to make things look good

char work_string[256]; // temp string

int index; // looping var

// start the timing clock
Start_Clock();

// clear the drawing surface 
DDraw_Fill_Surface(lpddsback, 0);

// draw the sky
Draw_Rectangle(0,0, WINDOW_WIDTH-1, WINDOW_HEIGHT-1, RGB16Bit(50,50,200), lpddsback);

// draw the ground
//Draw_Rectangle(0,WINDOW_HEIGHT*.38, WINDOW_WIDTH, WINDOW_HEIGHT, RGB16Bit(25,50,110), lpddsback);

// read keyboard and other devices here
DInput_Read_Keyboard();

// game logic here...

// reset the render list
Reset_RENDERLIST4DV2(&rend_list);

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
   if (lights2[AMBIENT_LIGHT_INDEX].state == LIGHTV2_STATE_ON)
      lights2[AMBIENT_LIGHT_INDEX].state = LIGHTV2_STATE_OFF;
   else
      lights2[AMBIENT_LIGHT_INDEX].state = LIGHTV2_STATE_ON;

   Wait_Clock(100); // wait, so keyboard doesn't bounce
   } // end if

// toggle infinite light
if (keyboard_state[DIK_I])
   {
   // toggle ambient light
   if (lights2[INFINITE_LIGHT_INDEX].state == LIGHTV2_STATE_ON)
      lights2[INFINITE_LIGHT_INDEX].state = LIGHTV2_STATE_OFF;
   else
      lights2[INFINITE_LIGHT_INDEX].state = LIGHTV2_STATE_ON;

   Wait_Clock(100); // wait, so keyboard doesn't bounce
   } // end if

// toggle point light
if (keyboard_state[DIK_P])
   {
   // toggle point light
   if (lights2[POINT_LIGHT_INDEX].state == LIGHTV2_STATE_ON)
      lights2[POINT_LIGHT_INDEX].state = LIGHTV2_STATE_OFF;
   else
      lights2[POINT_LIGHT_INDEX].state = LIGHTV2_STATE_ON;

   // toggle point light
   if (lights2[POINT_LIGHT2_INDEX].state == LIGHTV2_STATE_ON)
      lights2[POINT_LIGHT2_INDEX].state = LIGHTV2_STATE_OFF;
   else
      lights2[POINT_LIGHT2_INDEX].state = LIGHTV2_STATE_ON;

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

// move to next object
if (keyboard_state[DIK_O])
   {
   VECTOR4D old_pos;
   old_pos = obj_work->world_pos;

   if (++curr_object >= NUM_OBJECTS)
      curr_object = 0;

   // update pointer
   obj_work = &obj_array[curr_object];
   obj_work->world_pos = old_pos;

   Wait_Clock(100); // wait, so keyboard doesn't bounce
   } // end if

// forward/backward
if (keyboard_state[DIK_UP])
   {
   // move forward
   if ( (cam_speed+=1) > MAX_SPEED) cam_speed = MAX_SPEED;
   } // end if
else
if (keyboard_state[DIK_DOWN])
   {
   // move backward
   if ((cam_speed-=1) < -MAX_SPEED) cam_speed = -MAX_SPEED;
   } // end if

// rotate around y axis or yaw
if (keyboard_state[DIK_RIGHT])
   {
   cam.dir.y+=5;
   } // end if

if (keyboard_state[DIK_LEFT])
   {
   cam.dir.y-=5;
   } // end if

// change height of point light source 1
if (keyboard_state[DIK_1])
   {
   lights2[POINT_LIGHT_INDEX].pos.y-=10;
   } // end if

if (keyboard_state[DIK_2])
   {
   lights2[POINT_LIGHT_INDEX].pos.y+=10;
   } // end if


// change height of point light source 2
if (keyboard_state[DIK_3])
   {
   lights2[POINT_LIGHT2_INDEX].pos.y-=10;
   } // end if

if (keyboard_state[DIK_4])
   {
   lights2[POINT_LIGHT2_INDEX].pos.y+=10;
   } // end if

// perform lightmapping here //////////////////////////////////////////////

// the idea is to take the source lightmap and use it to modulate the texture
// that is being mapped onto the target polygons and then write the results
// into the texture for rendering

///////////////////////////////////////////
// our little image processing algorithm :)

// Pixel_dest[x,y]rgb = pixel_source[x,y]rgb * light_map[x,y]rgb

USHORT *sbuffer = (USHORT *)texture_copy.buffer;
USHORT *lbuffer = (USHORT *)lightmaps[curr_lightmap].buffer;
USHORT *dbuffer = (USHORT *)obj_terrain.texture->buffer;


// perform RGB transformation on bitmap
for (int iy = 0; iy < texture_copy.height; iy++)
    for (int ix = 0; ix < texture_copy.width; ix++)
         {
         int rs,gs,bs;   // used to extract the source rgb values
         int rl, gl, bl; // light map rgb values
         int rf,gf,bf;   // the final rgb terms

         // extract pixel from source bitmap
         USHORT spixel = sbuffer[iy*texture_copy.width + ix];

         // extract RGB values
         _RGB565FROM16BIT(spixel, &rs,&gs,&bs); 

         // extract pixel from lightmap bitmap
         USHORT lpixel = lbuffer[iy*texture_copy.width + ix];

         // extract RGB values
         _RGB565FROM16BIT(lpixel, &rl,&gl,&bl);

         // compute modulation term
         //rf = ( (float)rs*(float)rl/(float)32 );
         //gf = ( (float)gs*(float)gl/(float)64 );
         //bf = ( (float)bs*(float)bl/(float)32 );

         rf = ( (rs*rl) >> 5 );
         gf = ( (gs*gl) >> 6 );
         bf = ( (bs*bl) >> 5 );

         // test for overflow
         //if (rf > 255) rf=255;
         //if (gf > 255) gf=255;
         //if (bf > 255) bf=255;

         // rebuild RGB and test for overflow
         // and write back to buffer
         dbuffer[iy*texture_copy.width + ix] = _RGB16BIT565(rf,gf,bf);
         
         } // end for ix     

if (++curr_lightmap >=9 )
   curr_lightmap = 0;


// motion section /////////////////////////////////////////////////////////

// terrain following, simply find the current cell we are over and then
// index into the vertex list and find the 4 vertices that make up the 
// quad cell we are hovering over and then average the values, and based
// on the current height and the height of the terrain push the player upward

// the terrain generates and stores some results to help with terrain following
//ivar1 = columns;
//ivar2 = rows;
//fvar1 = col_vstep;
//fvar2 = row_vstep;

int cell_x = (cam.pos.x  + TERRAIN_WIDTH/2) / obj_terrain.fvar1;
int cell_y = (cam.pos.z  + TERRAIN_HEIGHT/2) / obj_terrain.fvar1;

static float terrain_height, delta;

// test if we are on terrain
if ( (cell_x >=0) && (cell_x < obj_terrain.ivar1) && (cell_y >=0) && (cell_y < obj_terrain.ivar2) )
   {
   // compute vertex indices into vertex list of the current quad
   int v0 = cell_x + cell_y*obj_terrain.ivar2;
   int v1 = v0 + 1;
   int v2 = v1 + obj_terrain.ivar2;
   int v3 = v0 + obj_terrain.ivar2;   

   // now simply index into table 
   terrain_height = 0.25 * (obj_terrain.vlist_trans[v0].y + obj_terrain.vlist_trans[v1].y +
                            obj_terrain.vlist_trans[v2].y + obj_terrain.vlist_trans[v3].y);

   // compute height difference
   delta = terrain_height - (cam.pos.y - gclearance);

   // test for penetration
   if (delta > 0)
      {
      // apply force immediately to camera (this will give it a springy feel)
      vel_y+=(delta * (VELOCITY_SCALER));

      // test for pentration, if so move up immediately so we don't penetrate geometry
      cam.pos.y+=(delta*CAM_HEIGHT_SCALER);

      // now this is more of a hack than the physics model :) let move the front
      // up and down a bit based on the forward velocity and the gradient of the 
      // hill
      cam.dir.x -= (delta*PITCH_CHANGE_RATE);
 
      } // end if

   } // end if

// decelerate camera
if (cam_speed > (CAM_DECEL) ) cam_speed-=CAM_DECEL;
else
if (cam_speed < (-CAM_DECEL) ) cam_speed+=CAM_DECEL;
else
   cam_speed = 0;

// force camera to seek a stable orientation
if (cam.dir.x > (neutral_pitch+PITCH_RETURN_RATE)) cam.dir.x -= (PITCH_RETURN_RATE);
else
if (cam.dir.x < (neutral_pitch-PITCH_RETURN_RATE)) cam.dir.x += (PITCH_RETURN_RATE);
 else 
   cam.dir.x = neutral_pitch;

// apply gravity
vel_y+=gravity;

// test for absolute sea level and push upward..
if (cam.pos.y < sea_level)
   { 
   vel_y = 0; 
   cam.pos.y = sea_level;
   } // end if

// move camera
cam.pos.x += cam_speed*Fast_Sin(cam.dir.y);
cam.pos.z += cam_speed*Fast_Cos(cam.dir.y);
cam.pos.y += vel_y;

// move point light source in ellipse around game world
lights2[POINT_LIGHT_INDEX].pos.x = 500*Fast_Cos(plight_ang);
//lights2[POINT_LIGHT_INDEX].pos.y = 200;
lights2[POINT_LIGHT_INDEX].pos.z = 500*Fast_Sin(plight_ang);

// move point light source in ellipse around game world
lights2[POINT_LIGHT2_INDEX].pos.x = 200*Fast_Cos(-2*plight_ang);
//lights2[POINT_LIGHT2_INDEX].pos.y = 400;
lights2[POINT_LIGHT2_INDEX].pos.z = 200*Fast_Sin(-2*plight_ang);

if ((plight_ang+=1) > 360)
    plight_ang = 0;

// generate camera matrix
Build_CAM4DV1_Matrix_Euler(&cam, CAM_ROT_SEQ_ZYX);

//////////////////////////////////////////////////////////////////////////
// the terrain

// reset the object (this only matters for backface and object removal)
Reset_OBJECT4DV2(&obj_terrain);

// generate rotation matrix around y axis
//Build_XYZ_Rotation_MATRIX4X4(x_ang, y_ang, z_ang, &mrot);

MAT_IDENTITY_4X4(&mrot); 

// rotate the local coords of the object
Transform_OBJECT4DV2(&obj_terrain, &mrot, TRANSFORM_LOCAL_TO_TRANS,1);

// perform world transform
Model_To_World_OBJECT4DV2(&obj_terrain, TRANSFORM_TRANS_ONLY);

// insert the object into render list
Insert_OBJECT4DV2_RENDERLIST4DV2(&rend_list, &obj_terrain,0);

//////////////////////////////////////////////////////////////////////////

int v0, v1, v2, v3; // used to track vertices

VECTOR4D pl,  // position of the light
         po,  // position of the occluder object/vertex
         vlo, // vector from light to object
         ps;  // position of the shadow

float    rs,  // radius of shadow 
         t;   // parameter t

//////////////////////////////////////////////////////////////////////////
// render shaded object that projects shadow

// update rotation angle of object
obj_work->ivar1+=1.0;

if (obj_work->ivar1 >= 360)
    obj_work->ivar1 = 0;

// set position of object 
//obj_work->world_pos.x = 150*Fast_Cos(obj_work->ivar1);
//obj_work->world_pos.y = 200+75*Fast_Sin(obj_work->ivar1);
//obj_work->world_pos.z = 150*Fast_Sin(obj_work->ivar1);

// reset the object (this only matters for backface and object removal)
Reset_OBJECT4DV2(obj_work);

// generate rotation matrix around y axis
Build_XYZ_Rotation_MATRIX4X4(0, -10.0f, 0, &mrot);

Transform_OBJECT4DV2(obj_work, &mrot, TRANSFORM_LOCAL_ONLY,1);

// create identity matrix
MAT_IDENTITY_4X4(&mrot);

// rotate the local coords of the object
Transform_OBJECT4DV2(obj_work, &mrot, TRANSFORM_LOCAL_TO_TRANS,1);

// perform world transform
Model_To_World_OBJECT4DV2(obj_work, TRANSFORM_TRANS_ONLY);

// insert the object into render list
Insert_OBJECT4DV2_RENDERLIST4DV2(&rend_list, obj_work,0);

//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
// draw all the light objects to represent the position of light sources

// reset the object (this only matters for backface and object removal)
Reset_OBJECT4DV2(&obj_light_array[INDEX_GREEN_LIGHT_INDEX]);

// set position of object to light
obj_light_array[INDEX_GREEN_LIGHT_INDEX].world_pos = lights2[POINT_LIGHT_INDEX].pos;

// create identity matrix
MAT_IDENTITY_4X4(&mrot);

// transform the local coords of the object
Transform_OBJECT4DV2(&obj_light_array[INDEX_GREEN_LIGHT_INDEX], &mrot, TRANSFORM_LOCAL_TO_TRANS,1);

// perform world transform
Model_To_World_OBJECT4DV2(&obj_light_array[INDEX_GREEN_LIGHT_INDEX], TRANSFORM_TRANS_ONLY);

// insert the object into render list
Insert_OBJECT4DV2_RENDERLIST4DV2(&rend_list, &obj_light_array[INDEX_GREEN_LIGHT_INDEX],0);

// reset the object (this only matters for backface and object removal)
Reset_OBJECT4DV2(&obj_light_array[INDEX_RED_LIGHT_INDEX]);

// set position of object to light
obj_light_array[INDEX_RED_LIGHT_INDEX].world_pos = lights2[POINT_LIGHT2_INDEX].pos;

// create identity matrix
MAT_IDENTITY_4X4(&mrot);

// transform the local coords of the object
Transform_OBJECT4DV2(&obj_light_array[INDEX_RED_LIGHT_INDEX], &mrot, TRANSFORM_LOCAL_TO_TRANS,1);

// perform world transform
Model_To_World_OBJECT4DV2(&obj_light_array[INDEX_RED_LIGHT_INDEX], TRANSFORM_TRANS_ONLY);

// insert the object into render list
Insert_OBJECT4DV2_RENDERLIST4DV2(&rend_list, &obj_light_array[INDEX_RED_LIGHT_INDEX],0);

////////////////////////////////////////////////////////////////////////////////////

// reset number of polys rendered
debug_polys_rendered_per_frame = 0;
debug_polys_lit_per_frame = 0;

// perform rendering pass one

// remove backfaces
if (backface_mode==1)
   Remove_Backfaces_RENDERLIST4DV2(&rend_list, &cam);

// apply world to camera transform
World_To_Camera_RENDERLIST4DV2(&rend_list, &cam);

// clip the polygons themselves now
Clip_Polys_RENDERLIST4DV2(&rend_list, &cam, CLIP_POLY_X_PLANE | CLIP_POLY_Y_PLANE | CLIP_POLY_Z_PLANE );

// light scene all at once 
if (lighting_mode==1)
   {
   Transform_LIGHTSV2(lights2, 4, &cam.mcam, TRANSFORM_LOCAL_TO_TRANS);
   Light_RENDERLIST4DV2_World2_16(&rend_list, &cam, lights2, 4);
   } // end if

// sort the polygon list (hurry up!)
if (zsort_mode == 1)
   Sort_RENDERLIST4DV2(&rend_list,  SORT_POLYLIST_AVGZ);

// apply camera to perspective transformation
Camera_To_Perspective_RENDERLIST4DV2(&rend_list, &cam);

// apply screen transform
Perspective_To_Screen_RENDERLIST4DV2(&rend_list, &cam);

// lock the back buffer
DDraw_Lock_Back_Surface();

// reset number of polys rendered
debug_polys_rendered_per_frame = 0;

// render the object

if (wireframe_mode  == 0)
   Draw_RENDERLIST4DV2_Wire16(&rend_list, back_buffer, back_lpitch);
else
if (wireframe_mode  == 1)
   {
   // perspective mode affine texturing
      // set up rendering context
      rc.attr =    RENDER_ATTR_ZBUFFER  
              // | RENDER_ATTR_ALPHA  
              // | RENDER_ATTR_MIPMAP  
              // | RENDER_ATTR_BILERP
                 | RENDER_ATTR_TEXTURE_PERSPECTIVE_AFFINE;

   // initialize zbuffer to 0 fixed point
   Clear_Zbuffer(&zbuffer, (16000 << FIXP16_SHIFT));

   // set up remainder of rendering context
   rc.video_buffer   = back_buffer;
   rc.lpitch         = back_lpitch;
   rc.mip_dist       = 0;
   rc.zbuffer        = (UCHAR *)zbuffer.zbuffer;
   rc.zpitch         = WINDOW_WIDTH*4;
   rc.rend_list      = &rend_list;
   rc.texture_dist   = 0;
   rc.alpha_override = -1;

   // render scene
   Draw_RENDERLIST4DV2_RENDERCONTEXTV1_16_2(&rc);
   } // end if

// unlock the back buffer
DDraw_Unlock_Back_Surface();

// draw cockpit
//Draw_BOB16(&cockpit, lpddsback);

sprintf(work_string,"Lighting [%s]: Ambient=%d, Infinite=%d, Point=%d, BckFceRM [%s], Green Light y=%f, Red Light y=%f", 
                                                                                 ((lighting_mode == 1) ? "ON" : "OFF"),
                                                                                 lights2[AMBIENT_LIGHT_INDEX].state,
                                                                                 lights2[INFINITE_LIGHT_INDEX].state, 
                                                                                 lights2[POINT_LIGHT_INDEX].state,
                                                                                 ((backface_mode == 1) ? "ON" : "OFF"), 
                                                                                 lights2[POINT_LIGHT_INDEX].pos.y, lights2[POINT_LIGHT2_INDEX].pos.y);

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
    Draw_Text_GDI("<W>..............Toggle wire frame/solid mode.", 0, text_y+=12, RGB(255,255,255), lpddsback);
    Draw_Text_GDI("<B>..............Toggle backface removal.", 0, text_y+=12, RGB(255,255,255), lpddsback);
    Draw_Text_GDI("<O>..............Select different objects.", 0, text_y+=12, RGB(255,255,255), lpddsback);
    Draw_Text_GDI("<1>,<2>..........Change height of green point light.", 0, text_y+=12, RGB(255,255,255), lpddsback);
    Draw_Text_GDI("<3>,<4>..........Change height of red point light.", 0, text_y+=12, RGB(255,255,255), lpddsback);
    Draw_Text_GDI("<H>..............Toggle Help.", 0, text_y+=12, RGB(255,255,255), lpddsback);
    Draw_Text_GDI("<ESC>............Exit demo.", 0, text_y+=12, RGB(255,255,255), lpddsback);

    } // end help

sprintf(work_string,"Polys Rendered: %d, Polys lit: %d", debug_polys_rendered_per_frame, debug_polys_lit_per_frame);
Draw_Text_GDI(work_string, 0, WINDOW_HEIGHT-34-16-16, RGB(0,255,0), lpddsback);

sprintf(work_string,"CAM [%5.2f, %5.2f, %5.2f], CELL [%d, %d]",  cam.pos.x, cam.pos.y, cam.pos.z, cell_x, cell_y);

Draw_Text_GDI(work_string, 0, WINDOW_HEIGHT-34-16-16-16, RGB(0,255,0), lpddsback);

// flip the surfaces
DDraw_Flip2();

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