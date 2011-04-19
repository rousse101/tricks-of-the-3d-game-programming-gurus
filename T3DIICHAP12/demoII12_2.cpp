// DEMOII12_2.CPP - Alpha Blending demo 
// READ THIS!
// To compile make sure to include DDRAW.LIB, DSOUND.LIB,
// DINPUT.LIB, DINPUT8.LIB, WINMM.LIB in the project link list, and of course 
// the C++ source modules T3DLIB1-10.CPP and the headers T3DLIB1-10.H
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
#include "T3DLIB8.h"
#include "T3DLIB9.h"
#include "T3DLIB10.h"

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
   
#define WINDOWED_APP      0    // 0 not windowed, 1 windowed 

// create some constants for ease of access
#define AMBIENT_LIGHT_INDEX   0 // ambient light index
#define INFINITE_LIGHT_INDEX  1 // infinite light index
#define POINT_LIGHT_INDEX     2 // point light index
#define SPOT_LIGHT1_INDEX     4 // point light index
#define SPOT_LIGHT2_INDEX     3 // spot light index

#define CAM_DECEL            (.25)  // deceleration/friction
#define MAX_SPEED             20
#define NUM_OBJECTS           1     // number of objects system loads
#define NUM_SCENE_OBJECTS     500    // number of scenery objects 
#define UNIVERSE_RADIUS       2000  // size of universe
#define MAX_VEL               2    // maxium velocity of objects

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
POINT4D  cam_pos    = {0,0,0,1};
POINT4D  cam_target = {0,0,0,1};
VECTOR4D cam_dir    = {0,0,0,1};

// all your initialization code goes here...
VECTOR4D vscale={1.0,1.0,1.0,1},  
         vpos = {0,0,150,1}, 
         vrot = {0,0,0,1};

CAM4DV1         cam;                    // the single camera
OBJECT4DV2_PTR  obj_work;               // pointer to active working object
OBJECT4DV2      obj_array[NUM_OBJECTS], // array of objects 
                obj_scene;              // general scenery object
               
// filenames of objects to load
char *object_filenames[NUM_OBJECTS] = {
                                        "fighter_01.cob",
                                      };

int curr_object = 0;                  // currently active object index

POINT4D         scene_objects[NUM_SCENE_OBJECTS];     // positions of scenery objects
VECTOR4D        scene_objects_vel[NUM_SCENE_OBJECTS]; // velocities of scenery objects

RENDERLIST4DV2  rend_list;      // the render list
RGBAV1 white, gray, black, red, green, blue; // general colors

// physical model defines
float cam_speed  = 0;       // speed of the camera/jeep

ZBUFFERV1 zbuffer;          // out little z buffer!

RENDERCONTEXTV1 rc;         // the rendering context

BOB background;             // the background image

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
Init_CAM4DV1(&cam,      // the camera object
             CAM_MODEL_EULER, // the euler model
             &cam_pos,  // initial camera position
             &cam_dir,  // initial camera angles
             &cam_target,      // no target
             10.0,        // near and far clipping planes
             12000.0,
             120.0,      // field of view in degrees
             WINDOW_WIDTH,   // size of final screen viewport
             WINDOW_HEIGHT);
 
// set a scaling vector
VECTOR4D_INITXYZ(&vscale,5.00,5.00,5.00); 

// load all the objects in
for (int index_obj=0; index_obj < NUM_OBJECTS; index_obj++)
    {
    Load_OBJECT4DV2_COB2(&obj_array[index_obj], object_filenames[index_obj],  
                        &vscale, &vpos, &vrot, VERTEX_FLAGS_SWAP_YZ  | VERTEX_FLAGS_INVERT_WINDING_ORDER |
                                               VERTEX_FLAGS_TRANSFORM_LOCAL 
                                               /* VERTEX_FLAGS_TRANSFORM_LOCAL_WORLD*/,0 );

    } // end for index_obj

// set current object
curr_object = 0;
obj_work = &obj_array[curr_object];


// set a scaling vector
VECTOR4D_INITXYZ(&vscale,20.00,20.00,20.00); 

// load in the scenery object that we will place all over the place
Load_OBJECT4DV2_COB2(&obj_scene, "borg_flat_01.cob",  
                        &vscale, &vpos, &vrot, VERTEX_FLAGS_SWAP_YZ  |
                                               VERTEX_FLAGS_TRANSFORM_LOCAL 
                                               /* VERTEX_FLAGS_TRANSFORM_LOCAL_WORLD*/, 1);

// position the scenery objects randomly
for (index = 0; index < NUM_SCENE_OBJECTS; index++)
    {
    // randomly position object
    scene_objects[index].x = RAND_RANGE(-UNIVERSE_RADIUS, UNIVERSE_RADIUS);
    scene_objects[index].y = RAND_RANGE(-(UNIVERSE_RADIUS/2), (UNIVERSE_RADIUS/2));
    scene_objects[index].z = RAND_RANGE(-UNIVERSE_RADIUS, UNIVERSE_RADIUS);
    } // end for


// select random velocities
for (index = 0; index < NUM_SCENE_OBJECTS; index++)
    {
    // randomly position object
    scene_objects_vel[index].x = RAND_RANGE(-MAX_VEL, MAX_VEL);
    scene_objects_vel[index].y = RAND_RANGE(-MAX_VEL, MAX_VEL);
    scene_objects_vel[index].z = RAND_RANGE(-MAX_VEL, MAX_VEL);
    } // end for

// set up lights
Reset_Lights_LIGHTV2(lights2, MAX_LIGHTS);

// create some working colors
white.rgba = _RGBA32BIT(255,255,255,0);
gray.rgba  = _RGBA32BIT(150,150,150,0);
black.rgba = _RGBA32BIT(0,0,0,0);
red.rgba   = _RGBA32BIT(255,0,0,0);
green.rgba = _RGBA32BIT(0,255,0,0);
blue.rgba  = _RGBA32BIT(0,0,255,0);

// ambient light
Init_Light_LIGHTV2(lights2,               // array of lights to work with
                   AMBIENT_LIGHT_INDEX,   
                   LIGHTV2_STATE_ON,      // turn the light on
                   LIGHTV2_ATTR_AMBIENT,  // ambient light type
                   gray, black, black,    // color for ambient term only
                   NULL, NULL,            // no need for pos or dir
                   0,0,0,                 // no need for attenuation
                   0,0,0);                // spotlight info NA

VECTOR4D dlight_dir = {-1,0,-1,1}; 

// directional light
Init_Light_LIGHTV2(lights2,               // array of lights to work with
                   INFINITE_LIGHT_INDEX,  
                   LIGHTV2_STATE_ON,      // turn the light on
                   LIGHTV2_ATTR_INFINITE, // infinite light type
                   black, gray, black,    // color for diffuse term only
                   NULL, &dlight_dir,     // need direction only
                   0,0,0,                 // no need for attenuation
                   0,0,0);                // spotlight info NA


VECTOR4D plight_pos = {0,200,0,1};

// point light
Init_Light_LIGHTV2(lights2,               // array of lights to work with
                   POINT_LIGHT_INDEX,
                   LIGHTV2_STATE_ON,      // turn the light on
                   LIGHTV2_ATTR_POINT,    // pointlight type
                   black, green, black,    // color for diffuse term only
                   &plight_pos, NULL,     // need pos only
                   0,.002,0,              // linear attenuation only
                   0,0,1);                // spotlight info NA


VECTOR4D slight2_pos = {0,1000,0,1};
VECTOR4D slight2_dir = {-1,0,-1,1};

// spot light2
Init_Light_LIGHTV2(lights2,                  // array of lights to work with
                   SPOT_LIGHT2_INDEX,
                   LIGHTV2_STATE_ON,         // turn the light on
                   LIGHTV2_ATTR_SPOTLIGHT2,  // spot light type 2
                   black, red, black,        // color for diffuse term only
                   &slight2_pos, &slight2_dir, // need pos only
                   0,.001,0,                 // linear attenuation only
                   0,0,1);    


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


// load in the background
Create_BOB(&background, 0,0,800,600,1, BOB_ATTR_VISIBLE | BOB_ATTR_SINGLE_FRAME, DDSCAPS_SYSTEMMEMORY, 0, 16); 
Load_Bitmap_File(&bitmap16bit, "nebred01.bmp");
Load_Frame_BOB16(&background, &bitmap16bit,0,0,0,BITMAP_EXTRACT_MODE_ABS);
Unload_Bitmap_File(&bitmap16bit);

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

Delete_Zbuffer(&zbuffer);

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

static float alpha_override = 0,
             alpha_inc = .25;

// use these to rotate objects
static float x_ang = 0, y_ang = 0, z_ang = 0;

// state variables for different rendering modes and help
static int wireframe_mode = 1;
static int backface_mode  = 1;
static int lighting_mode  = 1;
static int help_mode      = 1;
static int zsort_mode     = 1;
static int x_clip_mode    = 1;
static int y_clip_mode    = 1;
static int z_clip_mode    = 1;
static int z_buffer_mode  = 1;
static int display_mode   = 1;
static float turning      = 0;

char work_string[256]; // temp string

int index; // looping var

// start the timing clock
Start_Clock();

// clear the drawing surface 
//DDraw_Fill_Surface(lpddsback, 0);

// draw the sky
//Draw_Rectangle(0,0, WINDOW_WIDTH, WINDOW_HEIGHT, RGB16Bit(0,0,0), lpddsback);
lpddsback->Blt(NULL, background.images[0], NULL, DDBLT_WAIT, NULL); 

// draw the ground
//Draw_Rectangle(0,WINDOW_HEIGHT*.5, WINDOW_WIDTH, WINDOW_HEIGHT, RGB16Bit(190,190,230), lpddsback);

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

   Wait_Clock(100); // wait, so keyboard doesn't bounce
   } // end if


// toggle spot light
if (keyboard_state[DIK_S])
   {
   // toggle spot light
   if (lights2[SPOT_LIGHT2_INDEX].state == LIGHTV2_STATE_ON)
      lights2[SPOT_LIGHT2_INDEX].state = LIGHTV2_STATE_OFF;
   else
      lights2[SPOT_LIGHT2_INDEX].state = LIGHTV2_STATE_ON;

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
if (keyboard_state[DIK_S])
   {
   // toggle z sorting
   zsort_mode = -zsort_mode;
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

// rotate
if (keyboard_state[DIK_RIGHT])
   {
   cam.dir.y+=3;
   
   // add a little turn to object
   if ((turning+=2) > 25)
      turning=25;
   } // end if

if (keyboard_state[DIK_LEFT])
   {
   cam.dir.y-=3;

   // add a little turn to object
   if ((turning-=2) < -25)
      turning=-25;

   } // end if
else // center heading again
   {
   if (turning > 0)
       turning-=1;
   else
   if (turning < 0)
       turning+=1;

   } // end else

// move to next object
if (keyboard_state[DIK_N])
   {
   if (++curr_object >= NUM_OBJECTS)
      curr_object = 0;
 
   // update pointer
   obj_work = &obj_array[curr_object];
   Wait_Clock(100); // wait, so keyboard doesn't bounce
   } // end if

// decelerate camera
if (cam_speed > (CAM_DECEL) ) cam_speed-=CAM_DECEL;
else
if (cam_speed < (-CAM_DECEL) ) cam_speed+=CAM_DECEL;
else
   cam_speed = 0;

// move camera
cam.pos.x += cam_speed*Fast_Sin(cam.dir.y);
cam.pos.z += cam_speed*Fast_Cos(cam.dir.y);

// move point light source in ellipse around game world
lights2[POINT_LIGHT_INDEX].pos.x = 1000*Fast_Cos(plight_ang);
lights2[POINT_LIGHT_INDEX].pos.y = 100;
lights2[POINT_LIGHT_INDEX].pos.z = 1000*Fast_Sin(plight_ang);

if ((plight_ang+=3) > 360)
    plight_ang = 0;

// move spot light source in ellipse around game world
lights2[SPOT_LIGHT2_INDEX].pos.x = 1000*Fast_Cos(slight_ang);
lights2[SPOT_LIGHT2_INDEX].pos.y = 200;
lights2[SPOT_LIGHT2_INDEX].pos.z = 1000*Fast_Sin(slight_ang);

if ((slight_ang-=5) < 0)
    slight_ang = 360;

// position player

obj_work->world_pos.x = cam.pos.x + 75*Fast_Sin(cam.dir.y);
obj_work->world_pos.y = cam.pos.y + -20;
obj_work->world_pos.z = cam.pos.z + 75*Fast_Cos(cam.dir.y);

// generate camera matrix
Build_CAM4DV1_Matrix_Euler(&cam, CAM_ROT_SEQ_ZYX);

////////////////////////////////////////////////////////
// insert the scenery into universe
for (index = 0; index < NUM_SCENE_OBJECTS; index++)
    {
    // reset the object (this only matters for backface and object removal)
    Reset_OBJECT4DV2(&obj_scene);

    // set position of tower
    obj_scene.world_pos.x = scene_objects[index].x;
    obj_scene.world_pos.y = scene_objects[index].y;
    obj_scene.world_pos.z = scene_objects[index].z;

    // move objects
    scene_objects[index].x+=scene_objects_vel[index].x;
    scene_objects[index].y+=scene_objects_vel[index].y;
    scene_objects[index].z+=scene_objects_vel[index].z;

    // test for out of bounds
    if (scene_objects[index].x >= UNIVERSE_RADIUS || scene_objects[index].x <= -UNIVERSE_RADIUS)
       { 
       scene_objects_vel[index].x=-scene_objects_vel[index].x;
       scene_objects[index].x+=scene_objects_vel[index].x;
       } // end if

    if (scene_objects[index].y >= (UNIVERSE_RADIUS/2) || scene_objects[index].y <= -(UNIVERSE_RADIUS/2))
       { 
       scene_objects_vel[index].y=-scene_objects_vel[index].y;
       scene_objects[index].y+=scene_objects_vel[index].y;
       } // end if

    if (scene_objects[index].z >= UNIVERSE_RADIUS  || scene_objects[index].z <= -UNIVERSE_RADIUS)
       { 
       scene_objects_vel[index].z=-scene_objects_vel[index].z;
       scene_objects[index].z+=scene_objects_vel[index].z;
       } // end if

    // attempt to cull object   
    if (!Cull_OBJECT4DV2(&obj_scene, &cam, CULL_OBJECT_XYZ_PLANES))
       {
       MAT_IDENTITY_4X4(&mrot);
 
       // rotate the local coords of the object
       Transform_OBJECT4DV2(&obj_scene, &mrot, TRANSFORM_LOCAL_TO_TRANS,1);

       // perform world transform
       Model_To_World_OBJECT4DV2(&obj_scene, TRANSFORM_TRANS_ONLY);

       // insert the object into render list
       Insert_OBJECT4DV2_RENDERLIST4DV2(&rend_list, &obj_scene,0);

       } // end if
 
    } // end for

///////////////////////////////////////////////////////////////
// insert the player object into universe

// reset the object (this only matters for backface and object removal)
Reset_OBJECT4DV2(obj_work);

// generate rotation matrix around y axis
Build_XYZ_Rotation_MATRIX4X4(-15, cam.dir.y+turning, 0, &mrot);

//MAT_IDENTITY_4X4(&mrot);

// rotate the local coords of the object
Transform_OBJECT4DV2(obj_work, &mrot, TRANSFORM_LOCAL_TO_TRANS,1);

// perform world transform
Model_To_World_OBJECT4DV2(obj_work, TRANSFORM_TRANS_ONLY);

// insert the object into render list
Insert_OBJECT4DV2_RENDERLIST4DV2(&rend_list, obj_work,0);

// update rotation angles
if ((x_ang+=.2) > 360) x_ang = 0;
if ((y_ang+=.4) > 360) y_ang = 0;
if ((z_ang+=.8) > 360) z_ang = 0;

// update the alpha level
alpha_override+=alpha_inc;

if (alpha_override >= NUM_ALPHA_LEVELS || alpha_override < 0)
   {
   alpha_inc=-alpha_inc;
   alpha_override+=alpha_inc;
   } // end if


// reset number of polys rendered
debug_polys_rendered_per_frame = 0;
debug_polys_lit_per_frame = 0;

// remove backfaces
if (backface_mode==1)
   Remove_Backfaces_RENDERLIST4DV2(&rend_list, &cam);

// apply world to camera transform
World_To_Camera_RENDERLIST4DV2(&rend_list, &cam);

// clip the polygons themselves now
Clip_Polys_RENDERLIST4DV2(&rend_list, &cam, ((x_clip_mode == 1) ? CLIP_POLY_X_PLANE : 0) | 
                                            ((y_clip_mode == 1) ? CLIP_POLY_Y_PLANE : 0) | 
                                            ((z_clip_mode == 1) ? CLIP_POLY_Z_PLANE : 0) );

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

// render the renderinglist
if (wireframe_mode  == 0)
   Draw_RENDERLIST4DV2_Wire16(&rend_list, back_buffer, back_lpitch);
else
if (wireframe_mode  == 1)
   {
   // initialize zbuffer to 16000 fixed point
   Clear_Zbuffer(&zbuffer, (000 << FIXP16_SHIFT));


#if 0
RENDERCONTEXTV1 rc;

// no z buffering, polygons will be rendered as are in list
#define RENDER_ATTR_NOBUFFER                     0x00000001  

// use z buffering rasterization
#define RENDER_ATTR_ZBUFFER                      0x00000002  

// use 1/z buffering rasterization
#define RENDER_ATTR_INVZBUFFER                   0x00000004  

// use mipmapping
#define RENDER_ATTR_MIPMAP                       0x00000010  

// enable alpha blending and override
#define RENDER_ATTR_ALPHA                        0x00000020  

// use affine texturing for all polys
#define RENDER_ATTR_TEXTURE_PERSPECTIVE_AFFINE   0x00000100  

// use perfect perspective texturing
#define RENDER_ATTR_TEXTURE_PERSPECTIVE_CORRECT  0x00000200  

// use linear piecewise perspective texturing
#define RENDER_ATTR_TEXTURE_PERSPECTIVE_LINEAR   0x00000400  

// use a hybrid of affine and linear piecewise based on distance
#define RENDER_ATTR_TEXTURE_PERSPECTIVE_HYBRID1  0x00000800  
 
// not implemented yet
#define RENDER_ATTR_TEXTURE_PERSPECTIVE_HYBRID2  0x00001000  

#endif

     // set up rendering context
   rc.attr         = RENDER_ATTR_NOBUFFER  
                     | RENDER_ATTR_ALPHA  
                     | RENDER_ATTR_MIPMAP  
                     | RENDER_ATTR_BILERP
                     | RENDER_ATTR_TEXTURE_PERSPECTIVE_AFFINE;
   
   rc.video_buffer   = back_buffer;
   rc.lpitch         = back_lpitch;
   rc.mip_dist       = 3500;
   rc.zbuffer        = (UCHAR *)zbuffer.zbuffer;
   rc.zpitch         = WINDOW_WIDTH*4;
   rc.rend_list      = &rend_list;
   rc.texture_dist   = 0;
   rc.alpha_override = alpha_override;

   // render scene
   Draw_RENDERLIST4DV2_RENDERCONTEXTV1_16(&rc);
   }

// unlock the back buffer
DDraw_Unlock_Back_Surface();

sprintf(work_string,"Lighting [%s]: Ambient=%d, Infinite=%d, Point=%d, Spot=%d, BckFceRM [%s], Zsort [%s]", 
                                                                                 ((lighting_mode == 1) ? "ON" : "OFF"),
                                                                                 lights[AMBIENT_LIGHT_INDEX].state,
                                                                                 lights[INFINITE_LIGHT_INDEX].state, 
                                                                                 lights[POINT_LIGHT_INDEX].state,
                                                                                 lights[SPOT_LIGHT2_INDEX].state,
                                                                                 ((backface_mode == 1) ? "ON" : "OFF"),
                                                                                 ((zsort_mode == 1) ? "ON" : "OFF"));


Draw_Text_GDI(work_string, 0, WINDOW_HEIGHT-34-16, RGB(0,255,0), lpddsback);

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
    Draw_Text_GDI("<S>..............Toggle Z sorting.", 0, text_y+=12, RGB(255,255,255), lpddsback);
    Draw_Text_GDI("<H>..............Toggle Help.", 0, text_y+=12, RGB(255,255,255), lpddsback);
    Draw_Text_GDI("<ESC>............Exit demo.", 0, text_y+=12, RGB(255,255,255), lpddsback);

    } // end help

sprintf(work_string,"Polys Rendered: %d, Polys lit: %d", debug_polys_rendered_per_frame, debug_polys_lit_per_frame);
Draw_Text_GDI(work_string, 0, WINDOW_HEIGHT-34-16-16, RGB(0,255,0), lpddsback);

sprintf(work_string,"CAM [%5.2f, %5.2f, %5.2f]",  cam.pos.x, cam.pos.y, cam.pos.z);

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