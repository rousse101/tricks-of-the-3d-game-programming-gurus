// DEMOII8_9.CPP - .COB file loading demo with the works!
// READ THIS!
// To compile make sure to include DDRAW.LIB, DSOUND.LIB,
// DINPUT.LIB, DINPUT8.LIB, WINMM.LIB in the project link list, and of course 
// the C++ source modules T3DLIB1-6.CPP and the headers T3DLIB1-6.H
// be in the working directory of the compiler
// also this program uses a menu, thus you need to include the resource
// template DEMOII8_8.RC to complile

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

#include "demoii8_9res.h" // include the resource header

// DEFINES ////////////////////////////////////////////////

// defines for windows interface
#define WINDOW_CLASS_NAME "WIN3DCLASS"  // class name
#define WINDOW_TITLE      "T3D Graphics Console Ver 2.0"
#define WINDOW_WIDTH      800 // size of window
#define WINDOW_HEIGHT     600

#define WINDOW_BPP        16    // bitdepth of window (8,16,24 etc.)
                                // note: if windowed and not
                                // fullscreen then bitdepth must
                                // be same as system bitdepth
                                // also if 8-bit the a pallete
                                // is created and attached

#define WINDOWED_APP      1     // 0 not windowed, 1 windowed


// defines for the game universe
#define UNIVERSE_RADIUS   4000

#define POINT_SIZE        100
#define NUM_POINTS_X      (2*UNIVERSE_RADIUS/POINT_SIZE)
#define NUM_POINTS_Z      (2*UNIVERSE_RADIUS/POINT_SIZE)
#define NUM_POINTS        (NUM_POINTS_X*NUM_POINTS_Z)

// defines for objects
#define NUM_TOWERS        64 // 96
#define NUM_TANKS         32 // 24
#define TANK_SPEED        15

// create some constants for ease of access
#define AMBIENT_LIGHT_INDEX   0 // ambient light index
#define INFINITE_LIGHT_INDEX  1 // infinite light index
#define POINT_LIGHT_INDEX     2 // point light index
#define SPOT_LIGHT_INDEX      3 // spot light index

// PROTOTYPES /////////////////////////////////////////////

// game console
int Game_Init(void *parms=NULL);
int Game_Shutdown(void *parms=NULL);
int Game_Main(void *parms=NULL);

BOOL CALLBACK DialogProc(HWND hwnddlg,  // handle to dialog box
                         UINT umsg,     // message
                         WPARAM wparam, // first message parameter
                         LPARAM lparam);  // second message parameter

// GLOBALS ////////////////////////////////////////////////

HWND main_window_handle           = NULL; // save the window handle
HINSTANCE main_instance           = NULL; // save the instance
char buffer[2048];                        // used to print text

// initialize camera position and direction
POINT4D  cam_pos    = {0,0,-250,1};
POINT4D  cam_target = {0,0,0,1};
VECTOR4D cam_dir    = {0,0,0,1};

// all your initialization code goes here...
VECTOR4D vscale={1.0,1.0,1.0,1}, 
         vpos = {0,0,0,1}, 
         vrot = {0,0,0,1};

CAM4DV1        cam;       // the single camera

OBJECT4DV1     obj_tower,    // used to hold the master tower
               obj_tank,     // used to hold the master tank
               obj_marker,   // the ground marker
               obj_player;   // the player object             

POINT4D        towers[NUM_TOWERS],
               tanks[NUM_TANKS];
               
RENDERLIST4DV1 rend_list; // the render list

// state variables for different rendering modes and help
int wireframe_mode = -1;
int backface_mode  = 1;
int lighting_mode  = 1;
int help_mode      = 1;
int zsort_mode     = 1;

// controlled during load dialog
int swapyz   = 0,
    iwinding = 0,
    dolocal   = 1,
    doworld   = 1;

char ascfilename[256]; // holds file name when loader loads

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

     case WM_COMMAND:
         {
          switch(LOWORD(wparam))
                {
                // handle the FILE menu
                case ID_FILE_EXIT:
                     {
                     // terminate window
                     PostQuitMessage(0);
                     } break;

                case ID_FILE_LOADASCFILE:
                     {
                     // call line input dialog
   	                 if (DialogBox (main_instance, MAKEINTRESOURCE(IDD_DIALOG1), hwnd, DialogProc ) == IDOK )
 					   {

                       // load in default object
                       VECTOR4D_INITXYZ(&vscale,5.00,5.00,5.00);
 
                       Load_OBJECT4DV1_COB(&obj_player,ascfilename,  
                                              &vscale, &vpos, &vrot, 
                                              (swapyz*VERTEX_FLAGS_SWAP_YZ) | 
                                              (iwinding*VERTEX_FLAGS_INVERT_WINDING_ORDER) | 
                                              (dolocal*VERTEX_FLAGS_TRANSFORM_LOCAL) | 
                                              (doworld*VERTEX_FLAGS_TRANSFORM_LOCAL_WORLD) );

                       } // end if

                     } break;

                 case ID_LIGHTS_TOGGLEAMBIENTLIGHT:
                     {
                     // toggle ambient light
                     if (lights[AMBIENT_LIGHT_INDEX].state == LIGHTV1_STATE_ON)
                        lights[AMBIENT_LIGHT_INDEX].state = LIGHTV1_STATE_OFF;
                     else
                        lights[AMBIENT_LIGHT_INDEX].state = LIGHTV1_STATE_ON;

                     } break;

                 case ID_LIGHTS_TOGGLEPOINTLIGHT:
                     {
                     // toggle point light
                     if (lights[POINT_LIGHT_INDEX].state == LIGHTV1_STATE_ON)
                        lights[POINT_LIGHT_INDEX].state = LIGHTV1_STATE_OFF;
                     else
                        lights[POINT_LIGHT_INDEX].state = LIGHTV1_STATE_ON;

                     } break;

                 case ID_LIGHTS_TOGGLESPOTLIGHT:
                     {
                     // toggle spot light
                     if (lights[SPOT_LIGHT_INDEX].state == LIGHTV1_STATE_ON)
                        lights[SPOT_LIGHT_INDEX].state = LIGHTV1_STATE_OFF;
                     else
                        lights[SPOT_LIGHT_INDEX].state = LIGHTV1_STATE_ON;

                     } break;

                 case ID_LIGHTS_TOGGLEINFINITELIGHT:
                     {
                     // toggle spot light
                     if (lights[INFINITE_LIGHT_INDEX].state == LIGHTV1_STATE_ON)
                        lights[INFINITE_LIGHT_INDEX].state = LIGHTV1_STATE_OFF;
                     else
                        lights[INFINITE_LIGHT_INDEX].state = LIGHTV1_STATE_ON;

                     } break;

                 case ID_ZSORTING_ENABLE:
                      {
                      zsort_mode = 1;
                      } break;

                 case ID_ZSORTING_DISABLE:
                      {
                      zsort_mode = -1;
                      } break;

                 case ID_BACKFACEREMOVAL_ENABLE:
                      {
                      backface_mode = 1;
                      } break;

                 case ID_BACKFACEREMOVAL_DISABLE:
                      {
                      backface_mode = -1;
                      } break;


                 case ID_RENDERINGMODE_SOLID:
                      {
                      // toggle wireframe mode
                      wireframe_mode = -1;
                      } break;

                 case ID_RENDERINGMODE_WIREFRAME:
                      {
                      // toggle wireframe mode
                      wireframe_mode = 1;
                      } break;


                // handle the HELP menu
                case ID_HELP_ABOUT:                 
                     {
                     sprintf(buffer, "Polys: %d, Vertices: %d", obj_player.num_polys, obj_player.num_vertices);
                     //  pop up a message box
                     MessageBox(hwnd, buffer, 
                               ".COB File Loader Demo",
                                MB_OK | MB_ICONEXCLAMATION);
                     } break;

                default: break;

             } // end switch wparam

          } break; // end WM_COMMAND


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

///////////////////////////////////////////////////////////

BOOL CALLBACK DialogProc(HWND hwnddlg,  // handle to dialog box
                         UINT umsg,     // message
                         WPARAM wparam, // first message parameter
                         LPARAM lparam)  // second message parameter
			
{
// dialog handler for the line input

// get the handle to the text edit box
HWND htextedit = GetDlgItem (hwnddlg, IDC_EDIT1);
HWND hswapyz   = GetDlgItem(hwnddlg, IDC_SWAPYZ);
HWND hiwinding = GetDlgItem(hwnddlg, IDC_IWINDING);
HWND hlocal    = GetDlgItem(hwnddlg, IDC_LOCAL);
HWND hworld    = GetDlgItem(hwnddlg, IDC_WORLD);


switch (umsg)
    {
	case WM_INITDIALOG:
		{
		SetFocus ( htextedit );
		} break;

	case WM_COMMAND:
		 {
		 switch (LOWORD(wparam))
                {
                case IDOK:
				  	 {
					 int linelength = SendMessage ( htextedit, EM_LINELENGTH, 0, 0 );
 					 ascfilename[0] = (unsigned char )255;
					 SendMessage (htextedit, EM_GETLINE, 0, (LPARAM) (LPCSTR) ascfilename );
					 ascfilename[linelength] = 0;

                     // set swap and winding order globals
                     swapyz   = (int) SendMessage(hswapyz, BM_GETCHECK, 0,0);
                     iwinding = (int) SendMessage(hiwinding, BM_GETCHECK, 0,0);
                     dolocal  = (int) SendMessage(hlocal, BM_GETCHECK, 0,0);
                     doworld  = (int) SendMessage(hworld, BM_GETCHECK, 0,0);
 
					 EndDialog (hwnddlg, IDOK );
		   		     return TRUE;
					 } break;

				case IDCANCEL:
					{
					EndDialog ( hwnddlg, IDCANCEL );
					return TRUE;
  				    } break;

     			default: break;
				} // end switch 
   		   
          } break;
	
} // end switch

// return normal
return 0;

} // end DialogProc

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
winclass.lpszMenuName	= MAKEINTRESOURCE(IDR_ASCMENU);
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

// load in default object
VECTOR4D_INITXYZ(&vscale,5.00,5.00,5.00);

Load_OBJECT4DV1_COB(&obj_player,"sphere01.cob",  
                       &vscale, &vpos, &vrot, 
                       VERTEX_FLAGS_SWAP_YZ | 
                       VERTEX_FLAGS_TRANSFORM_LOCAL | VERTEX_FLAGS_TRANSFORM_LOCAL_WORLD);

// set up lights
Reset_Lights_LIGHTV1();

// create some working colors
RGBAV1 white, gray, black, red, green, blue;

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

VECTOR4D slight_pos = {0,200,0,0};
VECTOR4D slight_dir = {-1,0,-1,0};

// spot light
Init_Light_LIGHTV1(SPOT_LIGHT_INDEX,
                   LIGHTV1_STATE_ON,         // turn the light on
                   LIGHTV1_ATTR_SPOTLIGHT2,  // spot light type 2
                   black, red, black,      // color for diffuse term only
                   &slight_pos, &slight_dir, // need pos only
                   0,.001,0,                 // linear attenuation only
                   0,0,1);    


// create lookup for lighting engine
RGB_16_8_IndexedRGB_Table_Builder(DD_PIXEL_FORMAT565,  // format we want to build table for
                                  palette,             // source palette
                                  rgblookup);          // lookup table

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
char work_string[256]; // temp string
int index; // looping var

// start the timing clock
Start_Clock();

// clear the drawing surface 
DDraw_Fill_Surface(lpddsback, 0);

// read keyboard and other devices here
DInput_Read_Keyboard();

// game logic here...

// reset the render list
Reset_RENDERLIST4DV1(&rend_list);

// allow user to move camera

// forward/backward
if (keyboard_state[DIK_UP])
   {
   // move forward
   cam.pos.x += 5*Fast_Sin(cam.dir.y);
   cam.pos.z += 5*Fast_Cos(cam.dir.y);
   } // end if

if (keyboard_state[DIK_DOWN])
   {
   // move backward
   cam.pos.x -= 5*Fast_Sin(cam.dir.y);
   cam.pos.z -= 5*Fast_Cos(cam.dir.y);
   } // end if

// rotate object
if (keyboard_state[DIK_RIGHT])
   {
   obj_player.dir.y+=2;
   } // end if

if (keyboard_state[DIK_LEFT])
   {
   obj_player.dir.y-=6;
   } // end if

if (obj_player.dir.y > 360)
    obj_player.dir.y-=360;

if (obj_player.dir.y < 0)
    obj_player.dir.y+=360;

// ambient rotation
obj_player.dir.y+=2;


// scale object
if (keyboard_state[DIK_PGUP])
   {
VECTOR4D_INITXYZ(&vscale,1.1,1.1,1.1);  
   Scale_OBJECT4DV1(&obj_player, &vscale);
   } // end if

if (keyboard_state[DIK_PGDN])
   {
   VECTOR4D_INITXYZ(&vscale,.9,.9,.9);
   Scale_OBJECT4DV1(&obj_player, &vscale);
   } // end if

// modes and lights

// wireframe mode
if (keyboard_state[DIK_W])
   {
   // toggle wireframe mode
   wireframe_mode = -wireframe_mode;
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
   if (lights[SPOT_LIGHT_INDEX].state == LIGHTV1_STATE_ON)
      lights[SPOT_LIGHT_INDEX].state = LIGHTV1_STATE_OFF;
   else
      lights[SPOT_LIGHT_INDEX].state = LIGHTV1_STATE_ON;

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
lights[POINT_LIGHT_INDEX].pos.x = 300*Fast_Cos(plight_ang);
lights[POINT_LIGHT_INDEX].pos.y = 200;
lights[POINT_LIGHT_INDEX].pos.z = 300*Fast_Sin(plight_ang);

if ((plight_ang+=3) > 360)
    plight_ang = 0;

// move spot light source in ellipse around game world
lights[SPOT_LIGHT_INDEX].pos.x = 200*Fast_Cos(slight_ang);
lights[SPOT_LIGHT_INDEX].pos.y = 200;
lights[SPOT_LIGHT_INDEX].pos.z = 200*Fast_Sin(slight_ang);

if ((slight_ang-=5) < 0)
    slight_ang = 360;


/////////////////////////////////////////

// generate camera matrix
Build_CAM4DV1_Matrix_Euler(&cam, CAM_ROT_SEQ_ZYX);

// insert the player into the world
// reset the object (this only matters for backface and object removal)
Reset_OBJECT4DV1(&obj_player);

// generate rotation matrix around y axis
Build_XYZ_Rotation_MATRIX4X4(0, obj_player.dir.y, 0, &mrot);

// rotate the local coords of the object
Transform_OBJECT4DV1(&obj_player, &mrot, TRANSFORM_LOCAL_TO_TRANS,1);

// perform world transform
Model_To_World_OBJECT4DV1(&obj_player, TRANSFORM_TRANS_ONLY);


           // perform lighting
           if (lighting_mode==1)
              Light_OBJECT4DV1_World16(&obj_player, &cam, lights, 4);   

           // insert the object into render list
           Insert_OBJECT4DV1_RENDERLIST4DV12(&rend_list, &obj_player,0,lighting_mode);

// insert the object into render list
//Insert_OBJECT4DV1_RENDERLIST4DV12(&rend_list, &obj_player,0, 0);

//////////////////////////////////////

// remove backfaces
if (backface_mode==1)
   Remove_Backfaces_RENDERLIST4DV1(&rend_list, &cam);

// light scene all at once 
//if (lighting_mode==1)
//   Light_RENDERLIST4DV1_World16(&rend_list, &cam, lights, 4);

// apply world to camera transform
World_To_Camera_RENDERLIST4DV1(&rend_list, &cam);

// sort the polygon list (hurry up!)
if (zsort_mode == 1)
   Sort_RENDERLIST4DV1(&rend_list,  SORT_POLYLIST_AVGZ);

// apply camera to perspective transformation
Camera_To_Perspective_RENDERLIST4DV1(&rend_list, &cam);

// apply screen transform
Perspective_To_Screen_RENDERLIST4DV1(&rend_list, &cam);

sprintf(work_string,"pos:[%f, %f, %f] heading:[%f] elev:[%f], polys[%d]", 
        cam.pos.x, cam.pos.y, cam.pos.z, cam.dir.y, cam.dir.x, debug_polys_rendered_per_frame); 

Draw_Text_GDI(work_string, 0, WINDOW_HEIGHT-20, RGB(0,255,0), lpddsback);

sprintf(work_string,"Lighting [%s]: Ambient=%d, Infinite=%d, Point=%d, Spot=%d | Zsort [%s], BckFceRM [%s]", 
                                                                                 ((lighting_mode == 1) ? "ON" : "OFF"),
                                                                                 lights[AMBIENT_LIGHT_INDEX].state,
                                                                                 lights[INFINITE_LIGHT_INDEX].state, 
                                                                                 lights[POINT_LIGHT_INDEX].state,
                                                                                 lights[SPOT_LIGHT_INDEX].state,
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
    Draw_Text_GDI("<RIGHT ARROW>....Rotate object right.", 0, text_y+=12, RGB(255,255,255), lpddsback);
    Draw_Text_GDI("<LEFT ARROW>.....Rotate object left.", 0, text_y+=12, RGB(255,255,255), lpddsback);
    Draw_Text_GDI("<UP ARROW>.......Move camera forward.", 0, text_y+=12, RGB(255,255,255), lpddsback);
    Draw_Text_GDI("<DOWN ARROW>.....Move camera backward.", 0, text_y+=12, RGB(255,255,255), lpddsback);
    Draw_Text_GDI("<PG UP>..........Scale object down", 0, text_y+=12, RGB(255,255,255), lpddsback);
    Draw_Text_GDI("<PG DWN>.........Scale object down", 0, text_y+=12, RGB(255,255,255), lpddsback);
    Draw_Text_GDI("<SPACE BAR>......Turbo.", 0, text_y+=12, RGB(255,255,255), lpddsback);
    Draw_Text_GDI("<H>..............Toggle Help.", 0, text_y+=12, RGB(255,255,255), lpddsback);
    Draw_Text_GDI("<ESC>............Exit demo.", 0, text_y+=12, RGB(255,255,255), lpddsback);

    } // end help

// lock the back buffer
DDraw_Lock_Back_Surface();

// reset number of polys rendered
debug_polys_rendered_per_frame = 0;

// render the object
if (wireframe_mode  == 1)
   Draw_RENDERLIST4DV1_Wire16(&rend_list, back_buffer, back_lpitch);
else
   Draw_RENDERLIST4DV1_Solid16(&rend_list, back_buffer, back_lpitch);

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