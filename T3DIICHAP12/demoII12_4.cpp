// DEMOII12_4.CPP - Water simulation 
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
#define WINDOW_WIDTH      640     // size of window
#define WINDOW_HEIGHT     480
#define WINDOW_BPP        16      // bitdepth of window (8,16,24 etc.)
                                  // note: if windowed and not
                                  // fullscreen then bitdepth must
                                  // be same as system bitdepth
                                  // also if 8-bit the a pallete
                                  // is created and attached
   
#define WINDOWED_APP      0       // 0 not windowed, 1 windowed

// create some constants for ease of access
#define AMBIENT_LIGHT_INDEX   0   // ambient light index
#define INFINITE_LIGHT_INDEX  1   // infinite light index
#define POINT_LIGHT_INDEX     2   // point light index

// terrain defines
#define TERRAIN_WIDTH         5000 
#define TERRAIN_HEIGHT        5000
#define TERRAIN_SCALE         700
#define MAX_SPEED             20

#define PITCH_RETURN_RATE (.50)   // how fast the pitch straightens out
#define PITCH_CHANGE_RATE (.045)  // the rate that pitch changes relative to height
#define CAM_HEIGHT_SCALER (.3)    // percentage cam height changes relative to height
#define VELOCITY_SCALER   (.03)   // rate velocity changes with height
#define CAM_DECEL         (.25)   // deceleration/friction
#define WATER_LEVEL         40    // level where wave will have effect
#define WAVE_HEIGHT         25    // amplitude of modulation
#define WAVE_RATE        (0.1f)   // how fast the wave propagates

#define MAX_JETSKI_TURN           25    // maximum turn angle
#define JETSKI_TURN_RETURN_RATE   .5    // rate of return for turn
#define JETSKI_TURN_RATE           2    // turn rate
#define JETSKI_YAW_RATE           .1    // yaw rates, return, and max
#define MAX_JETSKI_YAW            10 
#define JETSKI_YAW_RETURN_RATE    .02 

// game states
#define GAME_STATE_INIT            0
#define GAME_STATE_RESTART         1
#define GAME_STATE_RUN             2
#define GAME_STATE_INTRO           3
#define GAME_STATE_PLAY            4
#define GAME_STATE_EXIT            5

// delays for introductions
#define INTRO_STATE_COUNT1 (30*7)  // 7,9,14
#define INTRO_STATE_COUNT2 (30*9)
#define INTRO_STATE_COUNT3 (30*14)

// player/jetski states
#define JETSKI_OFF                 0  
#define JETSKI_START               1 
#define JETSKI_IDLE                2
#define JETSKI_ACCEL               3 
#define JETSKI_FAST                4

// PROTOTYPES /////////////////////////////////////////////

// game console
int Game_Init(void *parms=NULL);
int Game_Shutdown(void *parms=NULL);
int Game_Main(void *parms=NULL);

// GLOBALS ////////////////////////////////////////////////

HWND main_window_handle = NULL; // save the window handle
HINSTANCE main_instance = NULL; // save the instance
char buffer[2048];              // used to print text

// initialize camera position and direction
POINT4D  cam_pos    = {0,500,-200,1};
POINT4D  cam_target = {0,0,0,1};
VECTOR4D cam_dir    = {0,0,0,1};

// all your initialization code goes here...
VECTOR4D vscale={1.0,1.0,1.0,1}, 
         vpos = {0,0,150,1}, 
         vrot = {0,0,0,1};

CAM4DV1         cam;           // the single camera
OBJECT4DV2      obj_terrain,   // the terrain object
                obj_terrain2,
                obj_player;    // the player object
RENDERLIST4DV2  rend_list;     // the render list
RGBAV1 white, gray, lightgray, black, red, green, blue; // general colors

// physical model defines play with these to change the feel of the vehicle
float gravity       = -.30;    // general gravity
float vel_y         = 0;       // the y velocity of camera/jetski
float cam_speed     = 0;       // speed of the camera/jetski
float sea_level     = -30;      // sea level of the simulation
float gclearance    = 75;      // clearance from the camera to the ground/water
float neutral_pitch = 1;       // the neutral pitch of the camera
float turn_ang      = 0; 
float jetski_yaw    = 0;

float wave_count    = 0;       // used to track wave propagation
int scroll_count    = 0;

// ambient and in game sounds
int wind_sound_id          = -1, 
    water_light_sound_id   = -1,
    water_heavy_sound_id   = -1,
    water_splash_sound_id  = -1,
    zbuffer_intro_sound_id = -1,
    zbuffer_instr_sound_id = -1, 
    jetski_start_sound_id  = -1,
    jetski_idle_sound_id   = -1,
    jetski_fast_sound_id   = -1,
    jetski_accel_sound_id  = -1,
    jetski_splash_sound_id = -1;

// game imagery assets
BOB intro_image, 
    ready_image, 
    nice_one_image;

BITMAP_IMAGE background_bmp;   // holds the background
 
ZBUFFERV1 zbuffer;             // out little z buffer

int game_state         = GAME_STATE_INIT;
int game_state_count1  = 0;

int player_state       = JETSKI_OFF;
int player_state_count1 = 0;
int enable_model_view   = 0;

RENDERCONTEXTV1 rc; // the rendering context

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
             30.0,        // near and far clipping planes
             12000.0,
             90.0,      // field of view in degrees
             WINDOW_WIDTH,   // size of final screen viewport
             WINDOW_HEIGHT); 

// position the terrain at 0,0,0
VECTOR4D terrain_pos = {0,0,0,0};

#if 1
// generate land terrain
Generate_Terrain2_OBJECT4DV2(&obj_terrain,            // pointer to object
                            TERRAIN_WIDTH,           // width in world coords on x-axis
                            TERRAIN_HEIGHT,          // height (length) in world coords on z-axis
                            TERRAIN_SCALE,           // vertical scale of terrain
                            "water_track_height_04.bmp",  // filename of height bitmap encoded in 256 colors
                            "water_track_color_03.bmp",   // filename of texture map
                             RGB16Bit(255,255,255),  // color of terrain if no texture        
                             &terrain_pos,           // initial position
                             NULL,                   // initial rotations
                             POLY4DV2_ATTR_RGB16  
                             //| POLY4DV2_ATTR_SHADE_MODE_CONSTANT
                             // | POLY4DV2_ATTR_SHADE_MODE_FLAT 
                              | POLY4DV2_ATTR_SHADE_MODE_GOURAUD
                             | POLY4DV2_ATTR_SHADE_MODE_TEXTURE,
                             50, 150);

#endif

#if 1
// generate seabottom terrain
Generate_Terrain2_OBJECT4DV2(&obj_terrain2,            // pointer to object
                            TERRAIN_WIDTH,           // width in world coords on x-axis
                            TERRAIN_HEIGHT,          // height (length) in world coords on z-axis
                            1*TERRAIN_SCALE,           // vertical scale of terrain
                            "heightmap02.bmp",  // filename of height bitmap encoded in 256 colors
                            "seabottom_01.bmp",   // filename of texture map
                             RGB16Bit(255,255,255),  // color of terrain if no texture        
                             &terrain_pos,           // initial position
                             NULL,                   // initial rotations
                             POLY4DV2_ATTR_RGB16  
                             // | POLY4DV2_ATTR_SHADE_MODE_CONSTANT
                             | POLY4DV2_ATTR_SHADE_MODE_FLAT 
                             //| POLY4DV2_ATTR_SHADE_MODE_GOURAUD
                             | POLY4DV2_ATTR_SHADE_MODE_TEXTURE,
                             -1, -1);

#endif



// we are getting divide by zero errors due to degenerate triangles
// after projection, this is a problem with the rasterizer that we will
// fix later, but if we add a random bias to all y components of each vertice
// is fixes the problem, its a hack, but hey no one is perfect :)
for (int v = 0; v < obj_terrain.num_vertices; v++)
    obj_terrain.vlist_local[v].y+= ((float)RAND_RANGE(-5,5))/10;
 

// load the jetski model for player
VECTOR4D_INITXYZ(&vscale,17.5,17.50,17.50);  

Load_OBJECT4DV2_COB2(&obj_player, "jetski06.cob",  
                   &vscale, &vpos, &vrot, VERTEX_FLAGS_SWAP_YZ  |
                                          VERTEX_FLAGS_TRANSFORM_LOCAL  | VERTEX_FLAGS_INVERT_TEXTURE_V
                                               /* VERTEX_FLAGS_TRANSFORM_LOCAL_WORLD*/ );
 

// set up lights
Reset_Lights_LIGHTV2(lights2, MAX_LIGHTS);

// create some working colors
white.rgba     = _RGBA32BIT(255,255,255,0);
lightgray.rgba = _RGBA32BIT(200,200,200,0); 
gray.rgba      = _RGBA32BIT(100,100,100,0);
black.rgba     = _RGBA32BIT(0,0,0,0);
red.rgba       = _RGBA32BIT(255,0,0,0);
green.rgba     = _RGBA32BIT(0,255,0,0);
blue.rgba      = _RGBA32BIT(0,0,255,0);

// ambient light
Init_Light_LIGHTV2(lights2,               // array of lights to work with
                   AMBIENT_LIGHT_INDEX,   
                   LIGHTV2_STATE_ON,      // turn the light on
                   LIGHTV2_ATTR_AMBIENT,  // ambient light type
                   gray, black, black,    // color for ambient term only
                   NULL, NULL,            // no need for pos or dir
                   0,0,0,                 // no need for attenuation
                   0,0,0);                // spotlight info NA

VECTOR4D dlight_dir = {-1,1,0,1}; 

// directional light
Init_Light_LIGHTV2(lights2,               // array of lights to work with
                   INFINITE_LIGHT_INDEX,  
                   LIGHTV2_STATE_ON,      // turn the light on
                   LIGHTV2_ATTR_INFINITE, // infinite light type
                   black, lightgray, black,    // color for diffuse term only
                   NULL, &dlight_dir,     // need direction only
                   0,0,0,                 // no need for attenuation
                   0,0,0);                // spotlight info NA


VECTOR4D plight_pos = {0,200,0,1};

// point light
Init_Light_LIGHTV2(lights2,               // array of lights to work with
                   POINT_LIGHT_INDEX,
                   LIGHTV2_STATE_ON,      // turn the light on
                   LIGHTV2_ATTR_POINT,    // pointlight type
                   black, gray, black,    // color for diffuse term only
                   &plight_pos, NULL,     // need pos only
                   0,.002,0,              // linear attenuation only
                   0,0,1);                // spotlight info NA



// create lookup for lighting engine
RGB_16_8_IndexedRGB_Table_Builder(DD_PIXEL_FORMAT565,  // format we want to build table for
                                  palette,             // source palette
                                  rgblookup);          // lookup table


// load background sounds
wind_sound_id           = DSound_Load_WAV("WIND.WAV");
water_light_sound_id    = DSound_Load_WAV("WATERLIGHT01.WAV");
water_heavy_sound_id    = DSound_Load_WAV("WATERHEAVY01.WAV");
water_splash_sound_id   = DSound_Load_WAV("SPLASH01.WAV");
zbuffer_intro_sound_id  = DSound_Load_WAV("ZBUFFER02.WAV");
zbuffer_instr_sound_id  = DSound_Load_WAV("ZINSTRUCTIONS02.WAV");
jetski_start_sound_id   = DSound_Load_WAV("jetskistart04.WAV");
jetski_idle_sound_id    = DSound_Load_WAV("jetskiidle01.WAV");
jetski_fast_sound_id    = DSound_Load_WAV("jetskifast01.WAV");
jetski_accel_sound_id   = DSound_Load_WAV("jetskiaccel01.WAV");
jetski_splash_sound_id  = DSound_Load_WAV("splash01.WAV");

// load background image
Load_Bitmap_File(&bitmap16bit, "cloud03_640.BMP");
Create_Bitmap(&background_bmp,0,0,640,480,16);
Load_Image_Bitmap16(&background_bmp, &bitmap16bit,0,0,BITMAP_EXTRACT_MODE_ABS);
Unload_Bitmap_File(&bitmap16bit);


// load in all the text images

// intro
Load_Bitmap_File(&bitmap16bit, "zbufferwr_intro01.BMP");
Create_BOB(&intro_image, WINDOW_WIDTH/2 - 560/2,40,560,160,1, BOB_ATTR_VISIBLE | BOB_ATTR_SINGLE_FRAME, DDSCAPS_SYSTEMMEMORY, 0, 16); 
Load_Frame_BOB16(&intro_image, &bitmap16bit,0,0,0,BITMAP_EXTRACT_MODE_ABS);
Unload_Bitmap_File(&bitmap16bit);

// get ready
Load_Bitmap_File(&bitmap16bit, "zbufferwr_ready01.BMP");
Create_BOB(&ready_image, WINDOW_WIDTH/2 - 596/2,40,596,244,1, BOB_ATTR_VISIBLE | BOB_ATTR_SINGLE_FRAME, DDSCAPS_SYSTEMMEMORY, 0, 16); 
Load_Frame_BOB16(&ready_image, &bitmap16bit,0,0,0,BITMAP_EXTRACT_MODE_ABS);
Unload_Bitmap_File(&bitmap16bit);

// nice one
Load_Bitmap_File(&bitmap16bit, "zbufferwr_nice01.BMP");
Create_BOB(&nice_one_image, WINDOW_WIDTH/2 - 588/2,40,588,92,1, BOB_ATTR_VISIBLE | BOB_ATTR_SINGLE_FRAME, DDSCAPS_SYSTEMMEMORY, 0, 16); 
Load_Frame_BOB16(&nice_one_image, &bitmap16bit,0,0,0,BITMAP_EXTRACT_MODE_ABS);
Unload_Bitmap_File(&bitmap16bit);

// set single precission
//_control87( _PC_24, MCW_PC );

// allocate memory for zbuffer
Create_Zbuffer(&zbuffer,
               WINDOW_WIDTH,
               WINDOW_HEIGHT,
               ZBUFFER_ATTR_32BIT);

RGB_Alpha_Table_Builder(NUM_ALPHA_LEVELS, rgb_alpha_table);

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

// use these to rotate objects
static float x_ang = 0, y_ang = 0, z_ang = 0;

// state variables for different rendering modes and help
static int wireframe_mode = 1;
static int backface_mode  = 1;
static int lighting_mode  = 1;
static int help_mode      = -1;
static int zsort_mode     = 1;
static int x_clip_mode    = 1;
static int y_clip_mode    = 1;
static int z_clip_mode    = 1;
static int z_buffer_mode  = 1;
static int display_mode   = 1;
static int seafloor_mode  = 1;

char work_string[256]; // temp string

static int nice_one_on = 0; // used to display the nice one text
static int nice_count1 = 0;

static int alpha = 0;

int index; // looping var

// what state is the game in?
switch(game_state)
{

case GAME_STATE_INIT:
     {
     // perform any important initializations
 
     // transition to restart
     game_state = GAME_STATE_RESTART;

     } break;

case GAME_STATE_RESTART:
     {
     // reset all variables
     game_state_count1   = 0;
     player_state        = JETSKI_OFF;
     player_state_count1 = 0;
     gravity             = -.30;    
     vel_y               = 0;       
     cam_speed           = 0;       
     sea_level           = 30;      
     gclearance          = 75;      
     neutral_pitch       = 10;   
     turn_ang            = 0; 
     jetski_yaw          = 0;
     wave_count          = 0;
     scroll_count        = 0;
     enable_model_view   = 0;

     // set camera high atop mount aleph one
     cam.pos.x = 1550;
     cam.pos.y = 800;
     cam.pos.z = 1950;
     cam.dir.y = -140;      

     // turn on water sounds
     // start the sounds
     DSound_Play(wind_sound_id, DSBPLAY_LOOPING);
     DSound_Set_Volume(wind_sound_id, 75);

     DSound_Play(water_light_sound_id, DSBPLAY_LOOPING);
     DSound_Set_Volume(water_light_sound_id, 50);

     DSound_Play(water_heavy_sound_id, DSBPLAY_LOOPING);
     DSound_Set_Volume(water_heavy_sound_id, 30);

     DSound_Play(zbuffer_intro_sound_id);
     DSound_Set_Volume(zbuffer_intro_sound_id, 100);

     // transition to introduction sub-state of run
     game_state = GAME_STATE_INTRO;

     } break;

// in any of these cases enter into the main simulation loop
case GAME_STATE_RUN:   
case GAME_STATE_INTRO: 
case GAME_STATE_PLAY:
{  
    
// perform sub-state transition logic here
if (game_state == GAME_STATE_INTRO)
   {
   // update timer
   ++game_state_count1;

   // test for first part of intro
   if (game_state_count1 == INTRO_STATE_COUNT1)
      {
      // change amplitude of water
      DSound_Set_Volume(water_light_sound_id, 50);
      DSound_Set_Volume(water_heavy_sound_id, 100);

      // reposition camera in water
      cam.pos.x = 444; // 560;
      cam.pos.y = 200;
      cam.pos.z = -534; // -580;
      cam.dir.y = 45;// (-100);  

      // enable model
      enable_model_view = 1;
      } // end if
   else
   if (game_state_count1 == INTRO_STATE_COUNT2)
      {
      // change amplitude of water
      DSound_Set_Volume(water_light_sound_id, 20);
      DSound_Set_Volume(water_heavy_sound_id, 50);
 
      // play the instructions
      DSound_Play(zbuffer_instr_sound_id);
      DSound_Set_Volume(zbuffer_instr_sound_id, 100);

      } // end if
    else
    if (game_state_count1 == INTRO_STATE_COUNT3)
      {
      // reset counter for other use
      game_state_count1 = 0;

      // change amplitude of water
      DSound_Set_Volume(water_light_sound_id, 30);
      DSound_Set_Volume(water_heavy_sound_id, 70);

      // transition to play state
      game_state = GAME_STATE_PLAY;  
 
      } // end if

   } // end if

// start the timing clock
Start_Clock();

// clear the drawing surface 
//DDraw_Fill_Surface(lpddsback, 0);

// draw the sky
//Draw_Rectangle(0,0, WINDOW_WIDTH, WINDOW_HEIGHT*.38, RGB16Bit(50,100,255), lpddsback);

// draw the ground
//Draw_Rectangle(0,WINDOW_HEIGHT*.38, WINDOW_WIDTH, WINDOW_HEIGHT, RGB16Bit(25,50,110), lpddsback);

//Draw_BOB16(&background, lpddsback);

// read keyboard and other devices here
DInput_Read_Keyboard();

// game logic here...

// reset the render list
Reset_RENDERLIST4DV2(&rend_list);

if (keyboard_state[DIK_O]) 
   {
   if (++alpha >= NUM_ALPHA_LEVELS)
       alpha = 0;
   
   Wait_Clock(100); // wait, so keyboard doesn't bounce
   } // end if

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

// z buffer
if (keyboard_state[DIK_Z])
   {
   // toggle z buffer
   z_buffer_mode = -z_buffer_mode;
   Wait_Clock(100); // wait, so keyboard doesn't bounce
   } // end if

// display mode
if (keyboard_state[DIK_D])
   {
   // toggle display mode
   display_mode = -display_mode;
   Wait_Clock(100); // wait, so keyboard doesn't bounce
   } // end if


// seafloor mode
if (keyboard_state[DIK_T])
   {
   // toggle the seafloor mode
   seafloor_mode = -seafloor_mode;
   Wait_Clock(100); // wait, so keyboard doesn't bounce
   } // end if

// PLAYER CONTROL AREA ////////////////////////////////////////////////////////////

// filter player control if not in PLAY state
if (game_state==GAME_STATE_PLAY)
{
// forward/backward
if (keyboard_state[DIK_UP] && player_state >= JETSKI_START)
   {
   // test for acceleration 
   if (cam_speed == 0)
      {
      DSound_Play(jetski_accel_sound_id);
      DSound_Set_Volume(jetski_accel_sound_id, 100);

      } // end if
      
   // move forward
   if ( (cam_speed+=1) > MAX_SPEED) 
        cam_speed = MAX_SPEED;

   } // end if

/*
else
if (keyboard_state[DIK_DOWN])
   {
   // move backward
   if ((cam_speed-=1) < -MAX_SPEED) cam_speed = -MAX_SPEED;
   } // end if
*/

// rotate around y axis or yaw
if (keyboard_state[DIK_RIGHT])
   {
   cam.dir.y+=5;

   if ((turn_ang+=JETSKI_TURN_RATE) > MAX_JETSKI_TURN)
      turn_ang = MAX_JETSKI_TURN;

   if ((jetski_yaw-=(JETSKI_YAW_RATE*cam_speed)) < -MAX_JETSKI_YAW)
      jetski_yaw = -MAX_JETSKI_YAW;

   // scroll the background
   Scroll_Bitmap(&background_bmp, -10);

   } // end if

if (keyboard_state[DIK_LEFT])
   {
   cam.dir.y-=5;

   if ((turn_ang-=JETSKI_TURN_RATE) < -MAX_JETSKI_TURN)
      turn_ang = -MAX_JETSKI_TURN;

   if ((jetski_yaw+=(JETSKI_YAW_RATE*cam_speed)) > MAX_JETSKI_YAW)
      jetski_yaw = MAX_JETSKI_YAW;

   // scroll the background
   Scroll_Bitmap(&background_bmp, 10);

   } // end if

// is player trying to start jetski?
if ( (player_state == JETSKI_OFF) && keyboard_state[DIK_RETURN])
   {
   // start jetski
   player_state = JETSKI_START;

   // play start sound
   DSound_Play(jetski_start_sound_id);
   DSound_Set_Volume(jetski_start_sound_id, 100);

   // play idle in loop at 100%
   DSound_Play(jetski_idle_sound_id,DSBPLAY_LOOPING);
   DSound_Set_Volume(jetski_idle_sound_id, 100);

   // play fast sound in loop at 0%
   DSound_Play(jetski_fast_sound_id,DSBPLAY_LOOPING);
   DSound_Set_Volume(jetski_fast_sound_id, 0);

   } // end if 

} // end if controls enabled

// turn JETSKI straight
if (turn_ang > JETSKI_TURN_RETURN_RATE) turn_ang-=JETSKI_TURN_RETURN_RATE;
else
if (turn_ang < -JETSKI_TURN_RETURN_RATE) turn_ang+=JETSKI_TURN_RETURN_RATE;
else
   turn_ang = 0;

// turn JETSKI straight
if (jetski_yaw > JETSKI_YAW_RETURN_RATE) jetski_yaw-=JETSKI_YAW_RETURN_RATE;
else
if (jetski_yaw < -JETSKI_YAW_RETURN_RATE) jetski_yaw+=JETSKI_YAW_RETURN_RATE;
else
   jetski_yaw = 0;

// reset the object (this only matters for backface and object removal)
Reset_OBJECT4DV2(&obj_terrain);

Reset_OBJECT4DV2(&obj_terrain2);

// perform world transform to terrain now, so we can destroy the transformed
// coordinates with the wave function
Model_To_World_OBJECT4DV2(&obj_terrain, TRANSFORM_LOCAL_TO_TRANS);

obj_terrain2.world_pos.y=-220;
Model_To_World_OBJECT4DV2(&obj_terrain2, TRANSFORM_LOCAL_TO_TRANS);


// apply wind effect ////////////////////////////////////////////////////

// scroll the background
if (++scroll_count > 5) 
   {
   Scroll_Bitmap(&background_bmp, -1);
   scroll_count = 0;
   } // end if

// wave generator ////////////////////////////////////////////////////////

// for each vertex in the mesh apply wave modulation if height < water level
for (int v = 0; v < obj_terrain.num_vertices; v++)
    {
    // wave modulate below water level only
    if (obj_terrain.vlist_trans[v].y < WATER_LEVEL)
        obj_terrain.vlist_trans[v].y+=WAVE_HEIGHT*sin(wave_count+(float)v/(2*(float)obj_terrain.ivar2+0));
    } // end for v

// increase the wave position in time
wave_count+=WAVE_RATE;

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

// position point light in front of player
lights2[POINT_LIGHT_INDEX].pos.x = cam.pos.x + 170*Fast_Sin(cam.dir.y);
lights2[POINT_LIGHT_INDEX].pos.y = cam.pos.y + 50;
lights2[POINT_LIGHT_INDEX].pos.z = cam.pos.z + 170*Fast_Cos(cam.dir.y);

// position the player object slighty in front of camera and in water
obj_player.world_pos.x = cam.pos.x + (130+cam_speed*1.75)*Fast_Sin(cam.dir.y);
obj_player.world_pos.y = cam.pos.y - 20 + 7.5*sin(wave_count);
obj_player.world_pos.z = cam.pos.z + (130+cam_speed*1.75)*Fast_Cos(cam.dir.y);

// sound effect section ////////////////////////////////////////////////////////////

// make engine sound if player is pressing gas
DSound_Set_Freq(jetski_fast_sound_id,11000+fabs(cam_speed)*100+delta*8);
DSound_Set_Volume(jetski_fast_sound_id, fabs(cam_speed)*5);

// make splash sound if altitude changed enough
if ( (fabs(delta) > 30) && (cam_speed >= (.75*MAX_SPEED)) && ((rand()%20)==1) ) 
   {
   // play sound
   DSound_Play(jetski_splash_sound_id);
   DSound_Set_Freq(jetski_splash_sound_id,12000+fabs(cam_speed)*5+delta*2);
   DSound_Set_Volume(jetski_splash_sound_id, fabs(cam_speed)*5);

   // display nice one!
   nice_one_on = 1; 
   nice_count1 = 60;
   } // end if 


// begin 3D rendering section ///////////////////////////////////////////////////////

// generate camera matrix
Build_CAM4DV1_Matrix_Euler(&cam, CAM_ROT_SEQ_ZYX);

// generate rotation matrix around y axis
//Build_XYZ_Rotation_MATRIX4X4(x_ang, y_ang, z_ang, &mrot);

// rotate the local coords of the object
//Transform_OBJECT4DV2(&obj_terrain, &mrot, TRANSFORM_LOCAL_TO_TRANS,1);

// insert terrain objects into render list, check if sea floor is disabled
if (seafloor_mode == 1)
   Insert_OBJECT4DV2_RENDERLIST4DV2(&rend_list, &obj_terrain2,0);

// normal terrain
Insert_OBJECT4DV2_RENDERLIST4DV2(&rend_list, &obj_terrain,0);


#if 1
//MAT_IDENTITY_4X4(&mrot);

// generate rotation matrix around y axis
Build_XYZ_Rotation_MATRIX4X4(-cam.dir.x*1.5, cam.dir.y+turn_ang, jetski_yaw, &mrot);

// rotate the local coords of the object
Transform_OBJECT4DV2(&obj_player, &mrot, TRANSFORM_LOCAL_TO_TRANS,1);

// perform world transform
Model_To_World_OBJECT4DV2(&obj_player, TRANSFORM_TRANS_ONLY);

// don't show model unless in play state
if (enable_model_view)
   {
   // insert the object into render list
   Insert_OBJECT4DV2_RENDERLIST4DV2(&rend_list, &obj_player,0);
   } // end if
 
#endif

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
   Transform_LIGHTSV2(lights2, 3, &cam.mcam, TRANSFORM_LOCAL_TO_TRANS);
   Light_RENDERLIST4DV2_World2_16(&rend_list, &cam, lights2, 3);
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

// draw background
Draw_Bitmap16(&background_bmp, back_buffer, back_lpitch,0);

// render the object

if (wireframe_mode  == 0)
   {
   Draw_RENDERLIST4DV2_Wire16(&rend_list, back_buffer, back_lpitch);
   }
else
if (wireframe_mode  == 1)
   {
   if (z_buffer_mode == -1)
      {
      // draw without a z buffer

       // set up rendering context
       rc.attr         =   RENDER_ATTR_NOBUFFER  
                         | RENDER_ATTR_ALPHA  
                         // | RENDER_ATTR_MIPMAP  
                         | RENDER_ATTR_TEXTURE_PERSPECTIVE_AFFINE;

       rc.video_buffer = back_buffer;
       rc.lpitch       = back_lpitch;
       rc.mip_dist     = 2000;
       rc.zbuffer      = (UCHAR *)zbuffer.zbuffer;
       rc.zpitch       = WINDOW_WIDTH*4;
       rc.rend_list    = &rend_list;
       rc.texture_dist = 500;
       rc.alpha_override = -1;

       // render scene
       Draw_RENDERLIST4DV2_RENDERCONTEXTV1_16(&rc);
      }
   else 
      { 
       // initialize zbuffer to 32000 fixed point
       Clear_Zbuffer(&zbuffer, (0 << FIXP16_SHIFT));

       // set up rendering context
       rc.attr         =   RENDER_ATTR_INVZBUFFER  
                         | RENDER_ATTR_ALPHA  
                         // | RENDER_ATTR_MIPMAP  
                         | RENDER_ATTR_TEXTURE_PERSPECTIVE_AFFINE;

       rc.video_buffer = back_buffer;
       rc.lpitch       = back_lpitch;
       rc.mip_dist     = 2000;
       rc.zbuffer      = (UCHAR *)zbuffer.zbuffer;
       rc.zpitch       = WINDOW_WIDTH*4;
       rc.rend_list    = &rend_list;
       rc.texture_dist = 500;
       rc.alpha_override = -1;

       // render scene
       Draw_RENDERLIST4DV2_RENDERCONTEXTV1_16(&rc);
       }  // end else

   } // end if 

// dislay z buffer graphically (sorta)
if (display_mode==-1)
{
// use z buffer visualization mode
// copy each line of the z buffer into the back buffer and translate each z pixel
// into a color
USHORT *screen_ptr = (USHORT *)back_buffer;
UINT   *zb_ptr    =  (UINT *)zbuffer.zbuffer;

for (int y = 0; y < WINDOW_HEIGHT; y++)
    {
    for (int x = 0; x < WINDOW_WIDTH; x++)
        {
        // z buffer is 32 bit, so be carefull
        UINT zpixel = zb_ptr[x + y*WINDOW_WIDTH];
        zpixel = (zpixel/4096 & 0xffff);
        screen_ptr[x + y* (back_lpitch >> 1)] = (USHORT)zpixel;
         } // end for
    } // end for y

} // end if

// unlock the back buffer
DDraw_Unlock_Back_Surface();

// draw overlay text
if (game_state == GAME_STATE_INTRO &&  game_state_count1 < INTRO_STATE_COUNT1)
   {
   Draw_BOB16(&intro_image, lpddsback);
   } // end if
else
if (game_state == GAME_STATE_INTRO &&  game_state_count1 > INTRO_STATE_COUNT2)
   {
   Draw_BOB16(&ready_image, lpddsback);
   } // end if

// check for nice one display
if (nice_one_on == 1 &&  game_state == GAME_STATE_PLAY)
   {
   // are we done displaying?
   if (--nice_count1 <=0)
      nice_one_on = 0;

   Draw_BOB16(&nice_one_image, lpddsback);
   } // end if draw nice one

// draw statistics 
sprintf(work_string,"Lighting [%s]: Ambient=%d, Infinite=%d, Point=%d, BckFceRM [%s], Zsort [%s], Zbuffer [%s]", 
                                                                                 ((lighting_mode == 1) ? "ON" : "OFF"),
                                                                                 lights[AMBIENT_LIGHT_INDEX].state,
                                                                                 lights[INFINITE_LIGHT_INDEX].state, 
                                                                                 lights[POINT_LIGHT_INDEX].state,
                                                                                 ((backface_mode == 1) ? "ON" : "OFF"),
                                                                                 ((zsort_mode == 1) ? "ON" : "OFF"),
                                                                                 ((z_buffer_mode == 1) ? "ON" : "OFF"));

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
    Draw_Text_GDI("<N>..............Next object.", 0, text_y+=12, RGB(255,255,255), lpddsback);
    Draw_Text_GDI("<W>..............Toggle wire frame/solid mode.", 0, text_y+=12, RGB(255,255,255), lpddsback);
    Draw_Text_GDI("<B>..............Toggle backface removal.", 0, text_y+=12, RGB(255,255,255), lpddsback);
    Draw_Text_GDI("<S>..............Toggle Z sorting.", 0, text_y+=12, RGB(255,255,255), lpddsback);
    Draw_Text_GDI("<Z>..............Toggle Z buffering.", 0, text_y+=12, RGB(255,255,255), lpddsback);
    Draw_Text_GDI("<D>..............Toggle Normal 3D display / Z buffer visualization mode.", 0, text_y+=12, RGB(255,255,255), lpddsback);
    Draw_Text_GDI("<T>..............Toggle Sea floor.", 0, text_y+=12, RGB(255,255,255), lpddsback);    
    Draw_Text_GDI("<H>..............Toggle Help.", 0, text_y+=12, RGB(255,255,255), lpddsback);
    Draw_Text_GDI("<ESC>............Exit demo.", 0, text_y+=12, RGB(255,255,255), lpddsback);

    } // end help

sprintf(work_string,"Polys Rendered: %d, Polys lit: %d", debug_polys_rendered_per_frame, debug_polys_lit_per_frame);
Draw_Text_GDI(work_string, 0, WINDOW_HEIGHT-34-16-16, RGB(0,255,0), lpddsback);

sprintf(work_string,"CAM [%5.2f, %5.2f, %5.2f @ %5.2f]",  cam.pos.x, cam.pos.y, cam.pos.z, cam.dir.y);
Draw_Text_GDI(work_string, 0, WINDOW_HEIGHT-34-16-16-16, RGB(0,255,0), lpddsback);

// flip the surfaces
DDraw_Flip2();

// sync to 30ish fps
//Wait_Clock(30);

// check of user is trying to exit
if (KEY_DOWN(VK_ESCAPE) || keyboard_state[DIK_ESCAPE])
    {
    // player is requesting an exit, fine!
    game_state = GAME_STATE_EXIT;
    PostMessage(main_window_handle, WM_DESTROY,0,0);
    } // end if

// end main simulation loop
} break;

case GAME_STATE_EXIT:
     {
     // do any cleanup here, and exit

     } break;

default: break;

} // end switch game_state

// return success
return(1);
 
} // end Game_Main

//////////////////////////////////////////////////////////