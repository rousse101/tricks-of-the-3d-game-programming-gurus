// DEMOII13_3.CPP - BSP Demo with frustrum culling 
// READ THIS!
// To compile make sure to include DDRAW.LIB, DSOUND.LIB,
// DINPUT.LIB, DINPUT8.LIB, WINMM.LIB in the project link list, and of course 
// the C++ source modules T3DLIB1-11.CPP and the headers T3DLIB1-11.H
// be in the working directory of the compiler
// also this program uses a menu, thus you need to include the resource
// template DEMOII13_1.RC to complile

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

#include <ddraw.h>    // directX includes
#include <dsound.h> 
#include <dmksctrl.h>
#include <dmusici.h>
#include <dmusicc.h>
#include <dmusicf.h>
#include <dinput.h>

#include "T3DLIB1.h"  // game library includes
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

#include "demoii13_1res.h" // include the resource header

// DEFINES ////////////////////////////////////////////////

// defines for windows interface
#define WINDOW_CLASS_NAME "WIN3DCLASS"  // class name
#define WINDOW_TITLE      "T3D Graphics Console Ver 2.0"
#define WINDOW_WIDTH      800           // size of window
#define WINDOW_HEIGHT     600

#define WINDOW_BPP        16    // bitdepth of window (8,16,24 etc.)
                                // note: if windowed and not
                                // fullscreen then bitdepth must
                                // be same as system bitdepth
                                // also if 8-bit the a pallete
                                // is created and attached

#define WINDOWED_APP      1     // 0 not windowed, 1 windowed

// types of gadgets
#define GADGET_TYPE_TOGGLE             0  // toggles from off/on, on/off
#define GADGET_TYPE_MOMENTARY          1  // lights up while you press it
#define GADGET_TYPE_STATIC             2  // doesn't do anything visual

// gadget ids
#define GADGET_ID_SEGMENT_MODE_ID      0  // drawing a single segment
#define GADGET_ID_POLYLINE_MODE_ID     2  // drawing a polyline
#define GADGET_ID_DELETE_MODE_ID       4  // delete lines 
#define GADGET_ID_CLEAR_ALL_ID         6  // clear all lines
#define GADGET_ID_FLOOR_MODE_ID        8  // drawing floor tiles

#define GADGET_ID_WALL_TEXTURE_DEC     20 // previous texture
#define GADGET_ID_WALL_TEXTURE_INC     21 // next texture

#define GADGET_ID_WALL_HEIGHT_DEC      22 // decrease wall height
#define GADGET_ID_WALL_HEIGHT_INC      23 // increase wall height

#define GADGET_ID_FLOOR_TEXTURE_DEC    30 // previous floor texture
#define GADGET_ID_FLOOR_TEXTURE_INC    31 // next floor texture

#define GADGET_OFF                     0  // the gadget is off
#define GADGET_ON                      1  // the gadget is on

#define NUM_GADGETS                    11 // number of active gadget      

// texture defines
#define NUM_TEXTURES                   23  // number of available textures 
#define TEXTURE_SIZE                   128 // size of texture m x m

#define MAX_LINES                      256 // maximum number of lines in the demo
                                           // these lines will later become 3-D walls

#define LINE_INVALID                   0   // constants that define the state of a line
#define LINE_VALID                     1

// interface action states
#define ACTION_STARTING_LINE           0  // used to determine if user is starting a new line
#define ACTION_ENDING_LINE             1  // or ending one
#define ACTION_STARTING_POLYLINE       2  // starting a polyline
#define ACTION_ADDING_POLYLINE         3  // adding polyline segment
#define ACTION_ENDING_POLYLINE         4  // done with polyline
#define ACTION_DELETE_LINE             5  // deleting a line
#define ACTION_FLOOR_MODE              6  // adding floor tiles

// control areas of GUI
#define BSP_MIN_X                  0      // the bounding rectangle of the
#define BSP_MAX_X                  648    // BSP editor area
#define BSP_MIN_Y                  0
#define BSP_MAX_Y                  576

#define BSP_GRID_SIZE              24     // the width/height of each cell
#define BSP_CELLS_X                28     // number of horizontal tick marks
#define BSP_CELLS_Y                25     // number of vertical tick marks

#define SCREEN_TO_WORLD_X    (-BSP_MAX_X/2)   // the translation factors to move the origin
#define SCREEN_TO_WORLD_Z    (-BSP_MAX_Y/2)   // to the center of the 2D universe

#define WORLD_SCALE_X              2      // scaling factors from screen space to world
#define WORLD_SCALE_Y              2      // space (note the inversion of the Z)
#define WORLD_SCALE_Z             -2


// bounds of gui components
#define INTERFACE_MIN_X            656    // the bounding rectangle of the control
#define INTERFACE_MAX_X            800    // button area to the right of editor
#define INTERFACE_MIN_Y            0
#define INTERFACE_MAX_Y            600

// editor states
#define EDITOR_STATE_INIT          0      // the editor is initializing
#define EDITOR_STATE_EDIT          1      // the editor is in edit mode
#define EDITOR_STATE_VIEW          2      // the editor is in viewing mode

// simple movement model defines
#define CAM_DECEL                  (.25)  // deceleration/friction
#define MAX_SPEED                  15     // maximum speed of camera

// create some constants for ease of access
#define AMBIENT_LIGHT_INDEX   0 // ambient light index
#define INFINITE_LIGHT_INDEX  1 // infinite light index
#define POINT_LIGHT_INDEX     2 // point light index
#define POINT_LIGHT2_INDEX    3 // point light index

// hacking stuff...
#define NUM_SCENE_OBJECTS          500    // number of scenery objects 
#define UNIVERSE_RADIUS            2000   // size of universe

// M A C R O S ///////////////////////////////////////////////////////////////

// TYPES //////////////////////////////////////////////////

// to help a little with the buttons and controls
typedef struct GADGET_TYP
        {
        int type;          // type of gadget
        int state;         // state of gadget
        int id;            // id number of gadget
        int x0, y0;        // screen position of gadget
        int width, height; // size of gadget
        BOB_PTR bitmap;    // bitmaps of gadget
        int count;
 
       } GADGET, *GADGET_PTR;

// the bsp line type that represents single 2d lines
typedef struct BSP2D_LINE_TYP 
        {
        int id;            // id of line
        int color;         // color of line (polygon)
        int attr;          // line (polygon) attributes (shading modes etc.)
        int texture_id;    // texture index 
        POINT2D p0, p1;    // endpoints of line

        int elev, height;  // height of wall

        } BSP2D_LINE, *BSP2D_LINE_PTR; 


// PROTOTYPES /////////////////////////////////////////////

// game console
int Game_Init(void *parms=NULL);
int Game_Shutdown(void *parms=NULL);
int Game_Main(void *parms=NULL);

// windows gui functions
BOOL CALLBACK DialogProc(HWND hwnddlg,    // handle to dialog box
                         UINT umsg,       // message
                         WPARAM waram,    // first message parameter
                         LPARAM lparam);  // second message parameter

// gui interface functions
int Process_Gadgets(int mx, int my, UCHAR mbuttons[]);

void Draw_Gadgets(LPDIRECTDRAWSURFACE7 lpdds);

void Load_Gadgets(void);

void Draw_Grid(UCHAR *video_buffer, 
               int mempitch,
               int rgbcolor, 
               int x0, int y0, 
               int xpitch, int ypitch,
               int xcells, int ycells);

int Delete_Line(int x,int y,int delete_line);

void Draw_Lines(UCHAR *video_buffer, int mempitch);

void Draw_Floors(LPDIRECTDRAWSURFACE7 lpdds);

void Reset_Editor(void);

// bsp loading and saving
int Load_BSP_File_LEV(char *filename);

int Save_BSP_File_LEV(char *filename, int flags);


void Convert_Lines_To_Walls(void);

int Generate_Floors_OBJECT4DV2(OBJECT4DV2_PTR obj,   // pointer to object
                                int rgbcolor,       // color of floor if no texture        
                                VECTOR4D_PTR pos,   // initial position
                                VECTOR4D_PTR rot,   // initial rotations
                                int poly_attr);     // the shading attributes we would like

// GLOBALS ////////////////////////////////////////////////

HWND main_window_handle           = NULL; // save the window handle
HINSTANCE main_instance           = NULL; // save the instance
char buffer[2048];                        // used to print text

char ascfilename[256]; // holds file name when loader loads

BOB background;        // the background image
BOB button_img;        // holds the button images

BITMAP_IMAGE background_bmp;   // holds the background

BOB textures;          // holds the textures for display in editor
BITMAP_IMAGE texturemaps[NUM_TEXTURES]; // holds the textures for the 3D display
                                        // copies basically...

BSP2D_LINE lines[MAX_LINES];   // this holds the lines in the system, we could
                               // use a linked list, but let's not make this harder
                               // than it already is!

BSPNODEV1_PTR wall_list = NULL,  // global pointer to the linked list of walls
              bsp_root  = NULL;  // global pointer to root of BSP tree

int floors[BSP_CELLS_Y-1][BSP_CELLS_X-1]; // hold the texture id's for the floor
                                          // which we will turn into a mesh later

// sound ids
int beep0_sound_id = 0;      // button beep sound

int total_lines     = 0;     // number of lines that have been defined
int starting_line   = 0;     // used to close contours
int snap_to_grid    = 1;     // flag to snap to grid        
int view_grid       = 1;     // flag to show grid
int view_walls      = 1;     // flag to show walls
int view_floors     = 1;     // flag to show floors


int mouse_debounce[3]   = {0,0,0}; // used to debounce left, middle, right buttons

int nearest_line        = -1;      // index of nearest line to mouse pointer
int texture_index_wall  = 11;      // index of wall texture
int texture_index_floor = 13;      // index of floor texture
int wall_height         = 64;      // initial wall height
int num_floor_tiles     = 0;       // current number of floor tiles
int poly_attr           = 0;       // general polygon attribute
int poly_color          = 0;       // general polygon color
int ceiling_height      = 0;       // height of ceiling 

int editor_state        = EDITOR_STATE_INIT; // starting state of editor

// define all the gadgets, their type, state, bitmap index, position, etc.
GADGET buttons[NUM_GADGETS] = 
       { 
       {GADGET_TYPE_TOGGLE,    GADGET_ON, GADGET_ID_SEGMENT_MODE_ID,    670, 50,          116,32, NULL,0}, 
       {GADGET_TYPE_TOGGLE,    GADGET_OFF, GADGET_ID_POLYLINE_MODE_ID,  670, 50+(50-12),  116,32, NULL,0}, 
       {GADGET_TYPE_TOGGLE,    GADGET_OFF, GADGET_ID_DELETE_MODE_ID,    670, 50+(100-24), 116,32, NULL,0}, 
       {GADGET_TYPE_MOMENTARY, GADGET_OFF, GADGET_ID_CLEAR_ALL_ID,      670, 348,         116,32, NULL,0}, 
       {GADGET_TYPE_TOGGLE,    GADGET_OFF, GADGET_ID_FLOOR_MODE_ID,     670, 556,         116,32, NULL,0}, 

       {GADGET_TYPE_STATIC,    GADGET_OFF, GADGET_ID_WALL_TEXTURE_DEC,  680, 316,         16,16, NULL,0}, 
       {GADGET_TYPE_STATIC,    GADGET_OFF, GADGET_ID_WALL_TEXTURE_INC,  700, 316,         16,16, NULL,0}, 

       {GADGET_TYPE_STATIC,    GADGET_OFF, GADGET_ID_WALL_HEIGHT_DEC,   680, 208,         16,16, NULL,0}, 
       {GADGET_TYPE_STATIC,    GADGET_OFF, GADGET_ID_WALL_HEIGHT_INC,   700, 208,         16,16, NULL,0}, 

       {GADGET_TYPE_STATIC,    GADGET_OFF, GADGET_ID_FLOOR_TEXTURE_DEC, 680, 522,         16,16, NULL,0}, 
       {GADGET_TYPE_STATIC,    GADGET_OFF, GADGET_ID_FLOOR_TEXTURE_INC, 700, 522,         16,16, NULL,0}, 
       };

// set initial action state
int action = ACTION_STARTING_LINE;

// 3D viewing stuff

// initialize camera position and direction
POINT4D  cam_pos    = {0,0,0,1};
POINT4D  cam_target = {0,0,0,1};
VECTOR4D cam_dir    = {0,0,0,1};

VECTOR4D vz    = {0,0,1,1};
VECTOR4D vdir  = {0,0,0,1};

// all your initialization code goes here...
VECTOR4D vscale={1.0,1.0,1.0,1},  
         vpos = {0,0,150,1}, 
         vrot = {0,0,0,1};

CAM4DV1         cam;           // the single camera
OBJECT4DV2      obj_scene;     // pointer to active working object
RENDERLIST4DV2  rend_list;     // the render list
ZBUFFERV1       zbuffer;       // out little z buffer!
RENDERCONTEXTV1 rc;            // the rendering context

RGBAV1          white,          // general colors 
                gray, 
                black, 
                red, 
                green, 
                blue,
                yellow,
                orange; 

// physical model defines
float cam_speed  = 0;       // speed of the camera/jeep

// hacking stuff
POINT4D scene_objects[NUM_SCENE_OBJECTS]; // positions of scenery objects


// MACROS ////////////////////////////////////////////////

// this macro assumes the lookup has 360 elements, where
// each element is in .5 degree increments roughly
// x must be in the range [-1,1]
#define FAST_INV_COS(x) (dp_inverse_cos[(int)(((float)x+1)*(float)180)])

// FUNCTIONS //////////////////////////////////////////////

LRESULT CALLBACK WindowProc(HWND hwnd, 
						    UINT msg, 
                            WPARAM wparam, 
                            LPARAM lparam)
{
// this is the main message handler of the system
PAINTSTRUCT	ps;		   // used in WM_PAINT
HDC			hdc;	   // handle to a device context
HMENU       hmenu;     // menu handle sending commands
  
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

     // pass mouse events out to globals
     case WM_MOUSEMOVE:
          {
          int fwkeys     = wparam;          // key flags 
          mouse_state.lX = LOWORD(lparam);  // horizontal position of cursor 
          mouse_state.lY = HIWORD(lparam);  // vertical position of cursor 
          } break;
     
     case WM_LBUTTONDOWN:
          {
          int fwkeys     = wparam;          // key flags 
          mouse_state.lX = LOWORD(lparam);  // horizontal position of cursor 
          mouse_state.lY = HIWORD(lparam);  // vertical position of cursor 
          mouse_state.rgbButtons[0] = 1;
          } break;

     case WM_MBUTTONDOWN:
          {
          int fwkeys     = wparam;          // key flags 
          mouse_state.lX = LOWORD(lparam);  // horizontal position of cursor 
          mouse_state.lY = HIWORD(lparam);  // vertical position of cursor 
          mouse_state.rgbButtons[1] = 1;
          } break;

     case WM_RBUTTONDOWN:
          {
          int fwkeys     = wparam;          // key flags 
          mouse_state.lX = LOWORD(lparam);  // horizontal position of cursor 
          mouse_state.lY = HIWORD(lparam);  // vertical position of cursor 
          mouse_state.rgbButtons[2] = 1;
          } break;

     case WM_COMMAND:
         {
         // get the menu that has generated this command
         hmenu = GetMenu(hwnd);
         
          // what is the id of the command
          switch(LOWORD(wparam))
                {
                // handle the FILE menu
                case ID_FILE_EXIT:
                     {
                     // terminate window
                     PostQuitMessage(0);
                     } break;

                case ID_FILE_LOAD_LEV:
                     {
                     // call load dialog and load .lev file
   	                 if (DialogBox (main_instance, MAKEINTRESOURCE(IDD_DIALOG1), hwnd, DialogProc ) == IDOK )
 					   {
                       Load_BSP_File_LEV(ascfilename);
                       } // end if

                     } break;

                case ID_FILE_SAVE_LEV:
                     {
                     // call save dialog and load .lev file
   	                 if (DialogBox (main_instance, MAKEINTRESOURCE(IDD_DIALOG2), hwnd, DialogProc ) == IDOK )
 					    {
                        Save_BSP_File_LEV(ascfilename,0);
                        } // end if

                     } break;

                case IDC_VIEWGRID:
                     {
                     // toggle viewing state
                     view_grid = -view_grid;
 
                     // toggle checkmark
                     if (view_grid==1)
                        CheckMenuItem(hmenu, IDC_VIEWGRID, MF_CHECKED);
                     else
                        CheckMenuItem(hmenu, IDC_VIEWGRID, MF_UNCHECKED);

                     } break;

                case IDC_VIEWWALLS:
                     {
                     // toggle viewing state
                     view_walls = -view_walls;
 
                     // toggle checkmark
                     if (view_walls==1)
                        CheckMenuItem(hmenu, IDC_VIEWWALLS, MF_CHECKED);
                     else
                        CheckMenuItem(hmenu, IDC_VIEWWALLS, MF_UNCHECKED);

                     } break;

                case IDC_VIEWFLOORS:
                     {
                     // toggle viewing state
                     view_floors = -view_floors;
 
                     // toggle checkmark
                     if (view_floors==1)
                        CheckMenuItem(hmenu, IDC_VIEWFLOORS, MF_CHECKED);
                     else
                        CheckMenuItem(hmenu, IDC_VIEWFLOORS, MF_UNCHECKED);

                     } break;

                case ID_BUILD_TO_FILE:
                     {
                     // not fully implemented yet....

                     // first delete any previous tree
                     Bsp_Delete(bsp_root);

                     // convert 2-d lines to 3-d walls
                     Convert_Lines_To_Walls();

                     // build the 3-D bsp tree
                     Bsp_Build_Tree(wall_list);

                     // alias the bsp tree to the wall list first node
                     // which has now become the root of the tree
                     bsp_root = wall_list;
   
                     // print out to error file
                     Bsp_Print(bsp_root);

                     // save .bsp to file

                     } break;

                case ID_COMPILE_VIEW:
                     {
                     // compile bsp

                     // first delete any previous tree
                     Bsp_Delete(bsp_root);

                     // reset ceiling height
                     // conversion will find heighest
                     ceiling_height  = 0;
             
                     // convert 2-d lines to 3-d walls
                     Convert_Lines_To_Walls();

                     // build the 3-D bsp tree
                     Bsp_Build_Tree(wall_list);

                     // alias the bsp tree to the wall list first node
                     // which has now become the root of the tree
                     bsp_root = wall_list;
   
                     // print out to error file
                     Bsp_Print(bsp_root);

                     // position camera at (0,0,0) and wall height
                     cam.pos.x = 0;
                     cam.pos.y = wall_height; 
                     cam.pos.z = 0;

                     // set heading and speed
                     cam.dir.y = 0;
                     cam_speed = 0;

                     // set position of floor to (0,0,0)
                     vpos.x = 0;
                     vpos.y = 0;
                     vpos.z = 0;

                     // generate the floor mesh object
                     Generate_Floors_OBJECT4DV2(&obj_scene,   // pointer to object
                              RGB16Bit(255,255,255),         // color of floor if no texture        
                              &vpos,                         // initial position
                              NULL,                          // initial rotations
                              POLY4DV2_ATTR_DISABLE_MATERIAL 
                              | POLY4DV2_ATTR_2SIDED 
                              | POLY4DV2_ATTR_RGB16 
                              | POLY4DV2_ATTR_SHADE_MODE_TEXTURE
                              | POLY4DV2_ATTR_SHADE_MODE_FLAT); // the shading attributes we would like

                     // switch states to view state
                     editor_state = EDITOR_STATE_VIEW;
                     } break;

                // handle the HELP menu
                case ID_HELP_ABOUT:                 
                     {
                     //  pop up a message box
                     MessageBox(hwnd, "Version 1.0", 
                               "BSP Level Generator",
                                MB_OK);
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

// get the handles to controls...
HWND htextedit = GetDlgItem(hwnddlg, IDC_EDIT1);

// options (not implemented yet...)
HWND hoption1 = GetDlgItem(hwnddlg, IDC_OPTION1);
HWND hoption2 = GetDlgItem(hwnddlg, IDC_OPTION2);
HWND hoption3 = GetDlgItem(hwnddlg, IDC_OPTION3);
HWND hoption4 = GetDlgItem(hwnddlg, IDC_OPTION4);

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
                     // text has been entered, update gloabal string with filename
					 int linelength = SendMessage ( htextedit, EM_LINELENGTH, 0, 0 );
 					 ascfilename[0] = (unsigned char )255;
					 SendMessage (htextedit, EM_GETLINE, 0, (LPARAM) (LPCSTR) ascfilename );
					 ascfilename[linelength] = 0;

                     // get checks 
                     SendMessage(hoption1, BM_GETCHECK, 0,0);
                     SendMessage(hoption2, BM_GETCHECK, 0,0);
                     SendMessage(hoption3, BM_GETCHECK, 0,0);
                     SendMessage(hoption4, BM_GETCHECK, 0,0);
 
                     // terminate dialog
					 EndDialog (hwnddlg, IDOK );
		   		     return TRUE;
					 } break;

				case IDCANCEL:
					{
                    // terminate dialog
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
DDraw_Init2(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_BPP, WINDOWED_APP);

// initialize directinput
DInput_Init();

// acquire the keyboard 
DInput_Init_Keyboard();
//DInput_Init_Mouse();

// add calls to acquire other directinput devices here...

// initialize directsound and directmusic
DSound_Init();
DMusic_Init();

// hide the mouse
if (!WINDOWED_APP)
    ShowCursor(FALSE);

// seed random number generator 
srand(Start_Clock());

Open_Error_File("BSPERROR.TXT");

// initialize math engine
Build_Sin_Cos_Tables();

// build the inverse cosine lookup
Build_Inverse_Cos_Table(dp_inverse_cos, 360);

// load in the background
Create_BOB(&background, 0,0,800,600,1, BOB_ATTR_VISIBLE | BOB_ATTR_SINGLE_FRAME, DDSCAPS_SYSTEMMEMORY, 0, 16); 
Load_Bitmap_File(&bitmap16bit, "bspgui02.bmp");
Load_Frame_BOB16(&background, &bitmap16bit,0,0,0,BITMAP_EXTRACT_MODE_ABS);
Unload_Bitmap_File(&bitmap16bit);

// load all the gadget imagery
Load_Gadgets();

// load the sounds
beep0_sound_id = DSound_Load_WAV("menuitem.wav");

// load the textures
// create the bob image to hold all the textures
Create_BOB(&textures, 0,0,TEXTURE_SIZE,TEXTURE_SIZE,NUM_TEXTURES, BOB_ATTR_VISIBLE | BOB_ATTR_SINGLE_FRAME, DDSCAPS_SYSTEMMEMORY, 0, 16); 

// set visibility
textures.attr |= BOB_ATTR_VISIBLE;

// interate and extract both the off and on bitmaps for each animated button
for (index = 0; index < NUM_TEXTURES; index++)
    {
    // load the bitmap to extract imagery
    sprintf(buffer, "bspdemotext128_%d.bmp", index);
    Load_Bitmap_File(&bitmap16bit, buffer);

    // load the frame for the bob
    Load_Frame_BOB16(&textures, &bitmap16bit,index,0,0,BITMAP_EXTRACT_MODE_ABS);

    // load in the frame for the bitmap
    Create_Bitmap(&texturemaps[index],0,0, TEXTURE_SIZE, TEXTURE_SIZE, 16);
    Load_Image_Bitmap16(&texturemaps[index], &bitmap16bit,0,0,BITMAP_EXTRACT_MODE_ABS);

    // unload the bitmap
    Unload_Bitmap_File(&bitmap16bit);
    } // end for index

// initialize the camera with 90 FOV, normalized coordinates
Init_CAM4DV1(&cam,            // the camera object
             CAM_MODEL_EULER, // the euler model
             &cam_pos,        // initial camera position
             &cam_dir,        // initial camera angles
             &cam_target,     // no target
             5.0,            // near and far clipping planes
             12000.0,
             120.0,           // field of view in degrees
             WINDOW_WIDTH,    // size of final screen viewport
             WINDOW_HEIGHT);
 
// set a scaling vector
VECTOR4D_INITXYZ(&vscale,20.00,20.00,20.00); 

// load the object in
Load_OBJECT4DV2_COB2(&obj_scene, "cube_flat_textured_01.cob",  
                     &vscale, &vpos, &vrot, VERTEX_FLAGS_SWAP_YZ  |
                                            VERTEX_FLAGS_TRANSFORM_LOCAL 
                                           /* VERTEX_FLAGS_TRANSFORM_LOCAL_WORLD*/,0 );


// position the scenery objects randomly
for (index = 0; index < NUM_SCENE_OBJECTS; index++)
    {
    // randomly position object
    scene_objects[index].x = RAND_RANGE(-UNIVERSE_RADIUS, UNIVERSE_RADIUS);
    scene_objects[index].y = RAND_RANGE(-200, 200);
    scene_objects[index].z = RAND_RANGE(-UNIVERSE_RADIUS, UNIVERSE_RADIUS);
    } // end for

// set up lights
Reset_Lights_LIGHTV2(lights2, MAX_LIGHTS);

// create some working colors
white.rgba = _RGBA32BIT(255,255,255,0);
gray.rgba  = _RGBA32BIT(128,128,128,0);
black.rgba = _RGBA32BIT(0,0,0,0);
red.rgba   = _RGBA32BIT(255,0,0,0);
green.rgba = _RGBA32BIT(0,255,0,0);
blue.rgba  = _RGBA32BIT(0,0,255,0);
orange.rgba = _RGBA32BIT(255,128,0,0);
yellow.rgba  = _RGBA32BIT(255,255,0,0);

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
Init_Light_LIGHTV2(lights2,
                   POINT_LIGHT_INDEX,
                   LIGHTV2_STATE_ON,      // turn the light on
                   LIGHTV2_ATTR_POINT,    // pointlight type
                   black, gray, black,  // color for diffuse term only
                   &plight_pos, NULL,     // need pos only
                   0,.001,0,              // linear attenuation only
                   0,0,1);                // spotlight info NA


// point light
Init_Light_LIGHTV2(lights2,
                   POINT_LIGHT2_INDEX,
                   LIGHTV2_STATE_ON,    // turn the light on
                   LIGHTV2_ATTR_POINT,  // pointlight type
                   black, white, black,   // color for diffuse term only
                   &plight_pos, NULL,   // need pos only
                   0,.002,0,            // linear attenuation only
                   0,0,1);              // spotlight info NA

// create the z buffer
Create_Zbuffer(&zbuffer,
               WINDOW_WIDTH,
               WINDOW_HEIGHT,
               ZBUFFER_ATTR_32BIT);

// reset the editor
Reset_Editor();

// load background image that scrolls 
Load_Bitmap_File(&bitmap16bit, "red01a.BMP");
Create_Bitmap(&background_bmp,0,0,800,600,16);
Load_Image_Bitmap16(&background_bmp, &bitmap16bit,0,0,BITMAP_EXTRACT_MODE_ABS);
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

// release the z buffer
Delete_Zbuffer(&zbuffer);

// close the error file
Close_Error_File();

// return success
return(1);
} // end Game_Shutdown

///////////////////////////////////////////////////////////////////////////////

int Game_Main(void *parms)
{
// this is the workhorse of your game it will be called
// continuously in real-time this is like main() in C
// all the calls for you game go here!
static int id = -1;


static MATRIX4X4 mrot;   // general rotation matrix

static float plight_ang = 0, 
             slight_ang = 0; // angles for light motion

// use these to rotate objects
static float x_ang = 0, y_ang = 0, z_ang = 0;

// state variables for different rendering modes and help
static int wireframe_mode = 1;
static int backface_mode  = -1;
static int lighting_mode  = 1;
static int help_mode      = 1;
static int zsort_mode     = -1;
static int x_clip_mode    = 1;
static int y_clip_mode    = 1;
static int z_clip_mode    = 1;
static int z_buffer_mode  = 1;
static int display_mode   = 1;
static int bsp_backface_mode   = 1;

char work_string[256]; // temp string

int index; // looping var

// is user in editor mode or viewing mode?
switch(editor_state)
      {
      case EDITOR_STATE_INIT: /////////////////////////////////////////////////
      {
       
      // now transition to edit mode
      editor_state = EDITOR_STATE_EDIT;
      } break;

      case EDITOR_STATE_EDIT: /////////////////////////////////////////////////
      {
    // start the timing clock
    Start_Clock();

    // clear the drawing surface 
    //DDraw_Fill_Surface(lpddsback, 0);
    lpddsback->Blt(NULL, background.images[0], NULL, DDBLT_WAIT, NULL);

    // read keyboard and other devices here
    DInput_Read_Keyboard();
    //DInput_Read_Mouse();

    // game logic here...

    // find nearest line 
    nearest_line = Delete_Line(mouse_state.lX, mouse_state.lY,0);

    // is user trying to press left button?
    if (mouse_state.rgbButtons[0] || mouse_state.rgbButtons[2])// && mouse_debounce[0] == 0)
       {
       // determine area click is occuring in
       if (mouse_state.lX > INTERFACE_MIN_X && mouse_state.lX < INTERFACE_MAX_X && 
           mouse_state.lY > INTERFACE_MIN_Y && mouse_state.lY < INTERFACE_MAX_Y)
       { 
       // check for gadget
       if ((id = Process_Gadgets(mouse_state.lX, mouse_state.lY, mouse_state.rgbButtons)) >= 0)
          {
          // play a sound first
          //DSound_Play(beep0_sound_id);

          // process clicks
          switch(id)
                {
                case GADGET_ID_SEGMENT_MODE_ID:
                {
                action = ACTION_STARTING_LINE;

                } break;

                case GADGET_ID_POLYLINE_MODE_ID:
                {
                action = ACTION_STARTING_POLYLINE; 
                } break;

                case GADGET_ID_DELETE_MODE_ID: 
                {
                action = ACTION_DELETE_LINE;
                } break;

                case GADGET_ID_CLEAR_ALL_ID:
                {
                // reset everything
                Reset_Editor();

                } break;

                case GADGET_ID_FLOOR_MODE_ID:
                {
                action = ACTION_FLOOR_MODE;
                } break;

                case GADGET_ID_WALL_TEXTURE_DEC:
                {
                if (--texture_index_wall < -1)
                   texture_index_wall = -1;

                } break;

                case GADGET_ID_WALL_TEXTURE_INC:
                {
                if (++texture_index_wall >= NUM_TEXTURES)
                   texture_index_wall = NUM_TEXTURES-1;
            
                } break;

                case GADGET_ID_WALL_HEIGHT_DEC:
                {
                if ((wall_height-=8) < 0)
                   wall_height = 0;

                } break;

                case GADGET_ID_WALL_HEIGHT_INC:
                {
                wall_height+=8;
            
                } break;

                case GADGET_ID_FLOOR_TEXTURE_DEC:
                {
                if (--texture_index_floor < -1)
                   texture_index_floor = -1;

                } break;

                case GADGET_ID_FLOOR_TEXTURE_INC:
                {

                if (++texture_index_floor >= NUM_TEXTURES)
                   texture_index_floor = NUM_TEXTURES-1;

                } break;

                default: break;

                } // end switch

          } // end if
       else
          {

          } // end else

       } // end if interface

       // test for bsp area
       if (mouse_state.lX > BSP_MIN_X && mouse_state.lX < BSP_MAX_X && 
           mouse_state.lY > BSP_MIN_Y && mouse_state.lY < BSP_MAX_Y)
         {
               // process bsp editor area click

               // test if this is starting point or endpoint
               if (action == ACTION_STARTING_LINE)
                  {
                  // set starting field, note the offsets to center the
                  // starting point in middle of cross hairs

                  if (!snap_to_grid)
                     {
                     lines[total_lines].p0.x = mouse_state.lX;
                     lines[total_lines].p0.y = mouse_state.lY;
                     } // end if
                  else
                     {
                     lines[total_lines].p0.x = BSP_GRID_SIZE * (int)(0.5+(float)mouse_state.lX / (float)BSP_GRID_SIZE);
                     lines[total_lines].p0.y = BSP_GRID_SIZE * (int)(0.5+(float)mouse_state.lY / (float)BSP_GRID_SIZE);
                     } // end if


                  // remember starting line to close contour
                  starting_line = total_lines;
              
                  // set point type to ending point
                  action = ACTION_ENDING_LINE;

                  // wait for mouse button release
                  // Wait_For_Mouse_Up();

                  } // end if start of wall
               else
               if (action == ACTION_ENDING_LINE)
                  {
                  // check for normal left mouse click
                  if (mouse_state.rgbButtons[0])
                     {
                     // must be the end of a wall or ending point
                  if (!snap_to_grid)
                     {
                     lines[total_lines].p1.x = mouse_state.lX;
                     lines[total_lines].p1.y = mouse_state.lY;
                     } // end if
                  else
                     {
                     lines[total_lines].p1.x = BSP_GRID_SIZE * (int)(0.5+(float)mouse_state.lX / (float)BSP_GRID_SIZE);
                     lines[total_lines].p1.y = BSP_GRID_SIZE * (int)(0.5+(float)mouse_state.lY / (float)BSP_GRID_SIZE);
                     } // end if

                     // set texture id
                     lines[total_lines].texture_id = texture_index_wall;

                     // set height
                     lines[total_lines].height = wall_height;

                     // set attributes
                     lines[total_lines].attr   = poly_attr;

                     // set color
                     lines[total_lines].color  = poly_color;
            
                     // set action back to starting point
                     action = ACTION_STARTING_LINE;

                     // advance number of lines
                     if (++total_lines >= MAX_LINES)
                        total_lines = MAX_LINES - 1;

                     // wait for mouse button release
                     // Wait_For_Mouse_Up();
                     } // end if
                  else
                  // check for right mouse click trying to close contour
                  if (mouse_state.rgbButtons[2])
                     {
                     // reset, user wants to restart segment
                     action = ACTION_STARTING_LINE;
                     } // end if

                  } // end else if
               else
               // test if this is starting point or endpoint
               if (action == ACTION_STARTING_POLYLINE)
                  {
                  // set starting field, note the offsets to center the
                  // starting point in middle of cross hairs

                  if (!snap_to_grid)
                     {
                     lines[total_lines].p0.x = mouse_state.lX;
                     lines[total_lines].p0.y = mouse_state.lY;
                     } // end if
                  else
                     {
                     lines[total_lines].p0.x = BSP_GRID_SIZE * (int)(0.5+(float)mouse_state.lX / (float)BSP_GRID_SIZE);
                     lines[total_lines].p0.y = BSP_GRID_SIZE * (int)(0.5+(float)mouse_state.lY / (float)BSP_GRID_SIZE);
                     } // end if

                  // remember starting line to close contour
                  starting_line = total_lines;
              
                  // set point type to ending point
                  action = ACTION_ADDING_POLYLINE;

                  // wait for mouse button release
                  // Wait_For_Mouse_Up();

                  } // end if start of wall
               else
               if (action == ACTION_ADDING_POLYLINE)
                  {
                  // check for normal left mouse click
                  if (mouse_state.rgbButtons[0])
                     {
                     // must be the end of a wall or ending point
                  if (!snap_to_grid)
                     {
                     lines[total_lines].p1.x = mouse_state.lX;
                     lines[total_lines].p1.y = mouse_state.lY;
                     } // end if
                  else
                     {
                     lines[total_lines].p1.x = BSP_GRID_SIZE * (int)(0.5+(float)mouse_state.lX / (float)BSP_GRID_SIZE);
                     lines[total_lines].p1.y = BSP_GRID_SIZE * (int)(0.5+(float)mouse_state.lY / (float)BSP_GRID_SIZE);
                     } // end if

                     // set texture id
                     lines[total_lines].texture_id = texture_index_wall;

                     // set height
                     lines[total_lines].height = wall_height;

                     // set attributes
                     lines[total_lines].attr   = poly_attr;

                     // set color
                     lines[total_lines].color  = poly_color;

                     // set action back to starting point
                     action = ACTION_ADDING_POLYLINE;

                     // advance number of lines
                     if (++total_lines >= MAX_LINES)
                        total_lines = MAX_LINES - 1;

                     // set start point as last endpoint
                     lines[total_lines].p0.x = lines[total_lines-1].p1.x;
                     lines[total_lines].p0.y = lines[total_lines-1].p1.y;

                     // wait for mouse button release
                     // Wait_For_Mouse_Up();
                     } // end if
                  else
                  // check for right mouse click trying to close contour
                  if (mouse_state.rgbButtons[2])
                     {
                     // end the polyline
                     action = ACTION_STARTING_POLYLINE;
                     // Wait_For_Mouse_Up();
                     } // end if

                  } // end else if
               else
               if (action==ACTION_DELETE_LINE)
                  {
                  // try and delete the line nearest selected point
                  Delete_Line(mouse_state.lX, mouse_state.lY,1);

                  // wait for mouse release
                  // Wait_For_Mouse_Up();

                  } // end if
               else
               if (action==ACTION_FLOOR_MODE)
                  {
                  // compute cell that mouse is on
                  int cell_x = (mouse_state.lX - BSP_MIN_X) / BSP_GRID_SIZE;
                  int cell_y = (mouse_state.lY - BSP_MIN_Y) / BSP_GRID_SIZE;

                  // drawing or erasing
                  if (mouse_state.rgbButtons[0])
                     {
                     // set texture id
                     floors[cell_y][cell_x] = texture_index_floor;
                     } // end if
                  else
                  if (mouse_state.rgbButtons[2])
                     {
                     // set texture id
                     floors[cell_y][cell_x] = -1;
                     } // end if
        
                  } // end if

       } // end if in bsp area

       // force button debounce
       if (mouse_state.rgbButtons[0])
           mouse_state.rgbButtons[0] = 0;

       if (mouse_state.rgbButtons[2])
           mouse_state.rgbButtons[2] = 0;

       //mouse_debounce[0] = 1;

       } // end if

    // debounce mouse
    //if (!mouse_state.rgbButtons[0] && mouse_debounce[0] == 1)
    //   mouse_debounce[0] = 0;

    // draw the floors
    if (view_floors == 1)
        Draw_Floors(lpddsback);

    // lock the back buffer
    DDraw_Lock_Back_Surface();

    // draw the lines
    if (view_walls == 1)
       Draw_Lines(back_buffer, back_lpitch);

    // draw the grid
    if (view_grid==1)
       Draw_Grid(back_buffer, back_lpitch, RGB16Bit(255,255,255), BSP_MIN_X, BSP_MIN_Y,
                                                               BSP_GRID_SIZE, BSP_GRID_SIZE,
                                                               BSP_CELLS_X,BSP_CELLS_Y);

    // unlock the back buffer
    DDraw_Unlock_Back_Surface();

    // draw all the buttons
    Draw_Gadgets(lpddsback);

    // draw the textures previews
    if (texture_index_wall >= 0)
       {
       // selected texture
       textures.x = 665;
       textures.y = 302-55;
       textures.curr_frame = texture_index_wall;
       Draw_Scaled_BOB16(&textures, 64,64,lpddsback);

       // get the line that mouse is closest to
       nearest_line = Delete_Line(mouse_state.lX, mouse_state.lY,0);

       // draw texture that is applied to line in secondary preview window
       if (nearest_line >= 0)
          {
          textures.x = 736;
          textures.y = 247;
          textures.curr_frame = lines[nearest_line].texture_id;
          Draw_Scaled_BOB16(&textures, 32,32,lpddsback);

          // now draw height of line
          sprintf(buffer,"%d",lines[nearest_line].height);
          Draw_Text_GDI(buffer, 750,188,RGB(0,255,0),lpddsback);      

          } // end if

       } // end if 

    // draw the floor texture preview
    if (texture_index_floor >= 0)
       {
       textures.x = 665;
       textures.y = 453;
       textures.curr_frame = texture_index_floor;
       Draw_Scaled_BOB16(&textures, 64,64,lpddsback);
       } // end if

    // draw the wall index
    if (texture_index_wall >= 0)
       {
       sprintf(buffer,"%d",texture_index_wall);
       Draw_Text_GDI(buffer, 738,336-56,RGB(0,255,0),lpddsback);
       } // end if
    else
       {
       Draw_Text_GDI("None", 738,336,RGB(0,255,0),lpddsback);
       } // end 

    // wall height
    sprintf(buffer,"%d",wall_height);
    Draw_Text_GDI(buffer, 688, 188,RGB(0,255,0),lpddsback);

    // texture index for floors
    if (texture_index_floor >= 0)
       {
       sprintf(buffer,"%d",texture_index_floor);
       Draw_Text_GDI(buffer, 738,488,RGB(0,255,0),lpddsback);
       } // end if
    else
       {
       Draw_Text_GDI("None", 738,488,RGB(0,255,0),lpddsback);
       } // end 

    // display stats
    sprintf(buffer,"WorldPos=(%d,%d). Num Walls=%d. Num Floor Tiles=%d. Total Polys=%d.",
            mouse_state.lX, mouse_state.lY, total_lines+1, num_floor_tiles, 2*(total_lines+1+num_floor_tiles));

    Draw_Text_GDI(buffer, 8,WINDOW_HEIGHT - 18,RGB(0,255,0),lpddsback);

    // flip the surfaces
    DDraw_Flip2();

    // sync to 30ish fps
    Sleep(10);

    #if 0
    // check of user is trying to exit
    if (KEY_DOWN(VK_ESCAPE) || keyboard_state[DIK_ESCAPE])
        {
        PostMessage(main_window_handle, WM_DESTROY,0,0);
        } // end if
    #endif

      } break;

      case EDITOR_STATE_VIEW: ///////////////////////////////////////////////
      {
    // start the timing clock
    Start_Clock();

    // clear the drawing surface 
    DDraw_Fill_Surface(lpddsback, 0);

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

    // BSP backface removal
    if (keyboard_state[DIK_C])
       {
       // toggle backface removal
       bsp_backface_mode = -bsp_backface_mode;
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

    // forward/backward
    if (keyboard_state[DIK_UP])
       {
       // move forward
       if ( (cam_speed+=2) > MAX_SPEED) cam_speed = MAX_SPEED;
       } // end if
    else
    if (keyboard_state[DIK_DOWN])
       {
       // move backward
       if ((cam_speed-=2) < -MAX_SPEED) cam_speed = -MAX_SPEED;
       } // end if

    // rotate around y axis or yaw
    if (keyboard_state[DIK_RIGHT])
       {
       cam.dir.y+=10;

       // scroll the background
       Scroll_Bitmap(&background_bmp, -10);
       } // end if

    if (keyboard_state[DIK_LEFT])
       {
       cam.dir.y-=10;

       // scroll the background
       Scroll_Bitmap(&background_bmp, 10);
       } // end if

      // ambient scrolling 
      static int scroll_counter = 0;
      if (++scroll_counter > 5)
         {
         // scroll the background
         Scroll_Bitmap(&background_bmp, 1);

         // reset scroll counter
         scroll_counter = 0;
         } // end if

    // move point light source in ellipse around game world
    lights2[POINT_LIGHT_INDEX].pos.x = 1000*Fast_Cos(plight_ang);
    lights2[POINT_LIGHT_INDEX].pos.y = 100;
    lights2[POINT_LIGHT_INDEX].pos.z = 1000*Fast_Sin(plight_ang);

    // move point light source in ellipse around game world
    lights2[POINT_LIGHT2_INDEX].pos.x = 500*Fast_Cos(-2*plight_ang);
    lights2[POINT_LIGHT2_INDEX].pos.y = 200;
    lights2[POINT_LIGHT2_INDEX].pos.z = 1000*Fast_Sin(-2*plight_ang);

    if ((plight_ang+=3) > 360)
        plight_ang = 0;

    // decelerate camera
    if (cam_speed > (CAM_DECEL) ) cam_speed-=CAM_DECEL;
    else
    if (cam_speed < (-CAM_DECEL) ) cam_speed+=CAM_DECEL;
    else
       cam_speed = 0;

    // move camera
    cam.pos.x += cam_speed*Fast_Sin(cam.dir.y);
    cam.pos.z += cam_speed*Fast_Cos(cam.dir.y);
    
    // generate camera matrix
    Build_CAM4DV1_Matrix_Euler(&cam, CAM_ROT_SEQ_ZYX);

    // build the camera vector rotation matrix ////////////////////////////////
    Build_XYZ_Rotation_MATRIX4X4(cam.dir.x, 
                                 cam.dir.y, 
                                 cam.dir.z,
                                 &mrot);

    // transform the (0,0,1) vector and store it in the cam.n vector, this is now the 
    // view direction vector
    Mat_Mul_VECTOR4D_4X4(&vz, &mrot, &cam.n);

    // normalize the viewing vector
    VECTOR4D_Normalize(&cam.n); ///////////////////////////////////////////////

    // render the floors into polylist first

    // reset the object (this only matters for backface and object removal)
    Reset_OBJECT4DV2(&obj_scene);

    // set position of floor
    obj_scene.world_pos.x = 0;
    obj_scene.world_pos.y = 0;
    obj_scene.world_pos.z = 0;

    // attempt to cull object   
    if (!Cull_OBJECT4DV2(&obj_scene, &cam, CULL_OBJECT_XYZ_PLANES))
       {
       // create an identity to work with
       MAT_IDENTITY_4X4(&mrot);
 
       // rotate the local coords of the object
       Transform_OBJECT4DV2(&obj_scene, &mrot, TRANSFORM_LOCAL_TO_TRANS,1);

       // perform world transform
       Model_To_World_OBJECT4DV2(&obj_scene, TRANSFORM_TRANS_ONLY);

       // insert the object into render list
       Insert_OBJECT4DV2_RENDERLIST4DV2(&rend_list, &obj_scene,0);
       } // end if

    // reset the object (this only matters for backface and object removal)
    Reset_OBJECT4DV2(&obj_scene);

    // set position of ceiling
    obj_scene.world_pos.x = 0;
    obj_scene.world_pos.y = ceiling_height;
    obj_scene.world_pos.z = 0;

    // attempt to cull object   
    if (!Cull_OBJECT4DV2(&obj_scene, &cam, CULL_OBJECT_XYZ_PLANES))
       {
       // create an identity to work with
       MAT_IDENTITY_4X4(&mrot);
 
       // rotate the local coords of the object
       Transform_OBJECT4DV2(&obj_scene, &mrot, TRANSFORM_LOCAL_TO_TRANS,1);

       // perform world transform
       Model_To_World_OBJECT4DV2(&obj_scene, TRANSFORM_TRANS_ONLY);

       // insert the object into render list
       Insert_OBJECT4DV2_RENDERLIST4DV2(&rend_list, &obj_scene,0);
       } // end if

#if 0
       // generate rotation matrix around y axis
       Build_XYZ_Rotation_MATRIX4X4(0, 1, 0, &mrot);

       //MAT_IDENTITY_4X4(&mrot);

       // transform bsp tree
       Bsp_Transform(bsp_root, &mrot, TRANSFORM_LOCAL_ONLY); 
#endif


    int polycount = rend_list.num_polys;

    if (bsp_backface_mode == 1)
       {
       // insert the bsp into rendering list
       Bsp_Insertion_Traversal_FrustrumCull_RENDERLIST4DV2(&rend_list, bsp_root, &cam, 1);
       }
   else
       {
       // insert the bsp into rendering list without culling
       Bsp_Insertion_Traversal_RENDERLIST4DV2(&rend_list, bsp_root, &cam, 1);
       } 

    // compute total number of polys inserted into list
    polycount = rend_list.num_polys - polycount;
 
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

    // draw background
    Draw_Bitmap16(&background_bmp, back_buffer, back_lpitch,0);

    // reset number of polys rendered
    debug_polys_rendered_per_frame = 0;

    // render the renderinglist
    if (wireframe_mode  == 0)
       Draw_RENDERLIST4DV2_Wire16(&rend_list, back_buffer, back_lpitch);
    else
    if (wireframe_mode  == 1)
       {

       // is z buffering enabled
       if (z_buffer_mode == 1)
       {
       // set up rendering context for 1/z buffer
       rc.attr         = RENDER_ATTR_INVZBUFFER  
                         //| RENDER_ATTR_ALPHA  
                         //| RENDER_ATTR_MIPMAP  
                         //| RENDER_ATTR_BILERP
                         | RENDER_ATTR_TEXTURE_PERSPECTIVE_AFFINE;


       } // end if
       else
       {    
       // set up rendering context for no zbuffer
       rc.attr         = RENDER_ATTR_NOBUFFER  
                         //| RENDER_ATTR_ALPHA  
                         //| RENDER_ATTR_MIPMAP  
                         //| RENDER_ATTR_BILERP
                         | RENDER_ATTR_TEXTURE_PERSPECTIVE_AFFINE;
        } // end else

      // if z abuffer or 1/z buffer is being used then clear z buffer    
      if (rc.attr & RENDER_ATTR_ZBUFFER)
         {
         // clear the z buffer
         Clear_Zbuffer(&zbuffer, (32000 << FIXP16_SHIFT));
         } // end if
      else
      if (rc.attr & RENDER_ATTR_INVZBUFFER)
         {
         // clear the 1/z buffer
         Clear_Zbuffer(&zbuffer, (0 << FIXP16_SHIFT));
         } // end if
   
       rc.video_buffer   = back_buffer;
       rc.lpitch         = back_lpitch;
       rc.mip_dist       = 0;
       rc.zbuffer        = (UCHAR *)zbuffer.zbuffer;
       rc.zpitch         = WINDOW_WIDTH*4;
       rc.rend_list      = &rend_list;
       rc.texture_dist   = 0;
       rc.alpha_override = -1;

       // render scene
       Draw_RENDERLIST4DV2_RENDERCONTEXTV1_16(&rc);
       } // end if

    // unlock the back buffer
    DDraw_Unlock_Back_Surface();

    sprintf(work_string,"Lighting [%s]: Ambient=%d, Infinite=%d, BckFceRM [%s], BSP Frustrum Cull [%s], Zsort [%s], Zbuffer [%s]", 
                                                                                     ((lighting_mode == 1) ? "ON" : "OFF"),
                                                                                     lights[AMBIENT_LIGHT_INDEX].state,
                                                                                     lights[INFINITE_LIGHT_INDEX].state, 
                                                                                     ((backface_mode == 1) ? "ON" : "OFF"),
                                                                                     ((bsp_backface_mode == 1) ? "ON" : "OFF"),
                                                                                     ((zsort_mode == 1) ? "ON" : "OFF"),
                                                                                     ((z_buffer_mode == 1) ? "ON" : "OFF"));

    Draw_Text_GDI(work_string, 0, WINDOW_HEIGHT-34-16, RGB(0,255,0), lpddsback);

    // draw instructions
    Draw_Text_GDI("Press ESC to return to editor mode. Press <H> for Help.", 0, 0, RGB(0,255,0), lpddsback);

    // should we display help
    int text_y = 16;
    if (help_mode==1)
        {
        // draw help menu
        Draw_Text_GDI("<A>..............Toggle ambient light source.", 0, text_y+=12, RGB(255,255,255), lpddsback);
        Draw_Text_GDI("<I>..............Toggle infinite light source.", 0, text_y+=12, RGB(255,255,255), lpddsback);
        Draw_Text_GDI("<P>..............Toggle point lights.", 0, text_y+=12, RGB(255,255,255), lpddsback);
        Draw_Text_GDI("<B>..............Toggle backface removal.", 0, text_y+=12, RGB(255,255,255), lpddsback);
        Draw_Text_GDI("<S>..............Toggle Z sorting.", 0, text_y+=12, RGB(255,255,255), lpddsback);
        Draw_Text_GDI("<C>..............Toggle BSP Backface Culling.", 0, text_y+=12, RGB(255,255,255), lpddsback);
        Draw_Text_GDI("<Z>..............Toggle Z buffering.", 0, text_y+=12, RGB(255,255,255), lpddsback);
        Draw_Text_GDI("<Z>..............Toggle Wireframe mode.", 0, text_y+=12, RGB(255,255,255), lpddsback);
        Draw_Text_GDI("<H>..............Toggle Help.", 0, text_y+=12, RGB(255,255,255), lpddsback);
        Draw_Text_GDI("<ESC>............Exit demo.", 0, text_y+=12, RGB(255,255,255), lpddsback);

        } // end help

    sprintf(work_string,"Polys Rendered: %d, Polys lit: %d, Polys Inserted from BSP: %d", debug_polys_rendered_per_frame, debug_polys_lit_per_frame, polycount);
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
       // switch game state back to editor more
       editor_state = EDITOR_STATE_EDIT;
       } // end if

      } break;     

      default: break;

      } // end switch


// return success
return(1);
 
} // end Game_Main

/////////////////////////////////////////////////////////////////////////////

void Reset_Editor(void)
{
// this function resets all the runtime variables of the editor and zeros things
// out

// reset all variables
total_lines         = 0;     // number of lines that have been defined
starting_line       = 0;     // used to close contours
snap_to_grid        = 1;     // flag to snap to grid        
view_grid           = 1;     // flag to show grid
view_walls          = 1;
view_floors         = 1;
nearest_line        = -1;
texture_index_wall  = 11;
texture_index_floor = 13;
wall_height         = 64;
ceiling_height      = 0;   
num_floor_tiles     = 0; 
poly_attr           = (POLY4DV2_ATTR_DISABLE_MATERIAL | POLY4DV2_ATTR_2SIDED | POLY4DV2_ATTR_RGB16 | POLY4DV2_ATTR_SHADE_MODE_FLAT );
poly_color          = RGB16Bit(255,255,255);

// initialize the floors
for (int cell_y = 0; cell_y < BSP_CELLS_Y-1; cell_y++)
    for (int cell_x = 0; cell_x < BSP_CELLS_X-1; cell_x++)
         floors[cell_y][cell_x] = -1;

// initialize the wall lines
memset(lines, 0, sizeof BSP2D_LINE*MAX_LINES);

} // end Reset_Editor

//////////////////////////////////////////////////////////////////////////////

void Draw_Floors(LPDIRECTDRAWSURFACE7 lpdds)
{
// this function draws all the floors by rendering the textures that are 
// mapped onto the floor scaled down (it very cute)

// reset number of floor tiles
num_floor_tiles = 0;

for (int cell_y = 0; cell_y < BSP_CELLS_Y-1; cell_y++)
    {
    for (int cell_x = 0; cell_x < BSP_CELLS_X-1; cell_x++)
        {
        // extract texture id
        int texture_id = floors[cell_y][cell_x];

        // is there a texture here?
        if (texture_id > -1)
           {
           // set position of texture on level editor space
           textures.x = BSP_MIN_X + (cell_x * BSP_GRID_SIZE);
           textures.y = BSP_MIN_Y + (cell_y * BSP_GRID_SIZE);

           // set texture id
           textures.curr_frame = texture_id;
           Draw_Scaled_BOB16(&textures, BSP_GRID_SIZE,BSP_GRID_SIZE,lpdds);
      
           // increment number of floor tiles
           num_floor_tiles++;

           } // end if 

        } // end for cell_y

   } // end for cell_y

} // end Draw_Floors

//////////////////////////////////////////////////////////////////////////////

void Draw_Lines(UCHAR *video_buffer, 
                int mempitch)
{
// this function draws all the lines for the BSP
for (int index = 0; index < total_lines; index++)
    {
    // draw each line, textured lines red, blank lines blue

    if (lines[index].texture_id >= 0)
       {
       Draw_Line16( lines[index].p0.x, lines[index].p0.y,
                    lines[index].p1.x, lines[index].p1.y,
                    RGB16Bit(255,0,0), video_buffer, mempitch);
       } // end if
    else
       {
       Draw_Line16( lines[index].p0.x, lines[index].p0.y,
                    lines[index].p1.x, lines[index].p1.y,
                    RGB16Bit(128,128,128), video_buffer, mempitch);
       } // end if
 
    // mark endpoints with X
    Draw_Line16( lines[index].p0.x-4, lines[index].p0.y-4,
                 lines[index].p0.x+4, lines[index].p0.y+4,
                 RGB16Bit(0,255,0), video_buffer, mempitch);
      
    Draw_Line16( lines[index].p0.x+4, lines[index].p0.y-4,
                 lines[index].p0.x-4, lines[index].p0.y+4,
                 RGB16Bit(0,255,0), video_buffer, mempitch);

    Draw_Line16( lines[index].p1.x-4, lines[index].p1.y-4,
                 lines[index].p1.x+4, lines[index].p1.y+4,
                 RGB16Bit(0,255,0), video_buffer, mempitch);
      
    Draw_Line16( lines[index].p1.x+4, lines[index].p1.y-4,
                 lines[index].p1.x-4, lines[index].p1.y+4,
                 RGB16Bit(0,255,0), video_buffer, mempitch);

    } // end for index

// now draw special lines
if (action == ACTION_ENDING_LINE &&
   mouse_state.lX > BSP_MIN_X && mouse_state.lX < BSP_MAX_X && 
   mouse_state.lY > BSP_MIN_Y && mouse_state.lY < BSP_MAX_Y)
   {
   Draw_Line16( lines[total_lines].p0.x, lines[total_lines].p0.y,
                mouse_state.lX, mouse_state.lY,
                RGB16Bit(255,255,255), video_buffer, mempitch);
 
   } // end if

// hightlight path to end point
if (action == ACTION_ADDING_POLYLINE &&
   mouse_state.lX > BSP_MIN_X && mouse_state.lX < BSP_MAX_X && 
   mouse_state.lY > BSP_MIN_Y && mouse_state.lY < BSP_MAX_Y)
   {
   Draw_Line16( lines[total_lines].p0.x, lines[total_lines].p0.y,
                mouse_state.lX, mouse_state.lY,
                RGB16Bit(255,255,255), video_buffer, mempitch);
 
   } // end if

static int red_counter = 0; // used to flicker delete candidate 
if ((red_counter+=32) > 255) red_counter = 0;

// highlight line mouse is hovering over
if (mouse_state.lX > BSP_MIN_X && mouse_state.lX < BSP_MAX_X && 
    mouse_state.lY > BSP_MIN_Y && mouse_state.lY < BSP_MAX_Y)
   {
   if (nearest_line >= 0)
      {
      if (action == ACTION_DELETE_LINE)
         {
          Draw_Line16( lines[nearest_line].p0.x, lines[nearest_line].p0.y,
                   lines[nearest_line].p1.x, lines[nearest_line].p1.y,
                      RGB16Bit(red_counter,red_counter,red_counter), video_buffer, mempitch);
         } // end if
      else
         {
          Draw_Line16( lines[nearest_line].p0.x, lines[nearest_line].p0.y,
                   lines[nearest_line].p1.x, lines[nearest_line].p1.y,
                      RGB16Bit(255,255,255), video_buffer, mempitch);
         } // end if

      } // end if
 
   } // end if         

} // end Draw_Lines

//////////////////////////////////////////////////////////////////////////////

int Delete_Line(int x, int y, int delete_line)
{
// this function hunts thru the lines and deletes the one closest to
// the sent position (which is the center of the cross hairs

int curr_line,          // current line being processed
    best_line=-1;       // the best match so far in process 

float sx,sy,            // starting coordinates of test line
      ex,ey,            // ending coordinates of test line
      length_line,      // total length of line being tested
      length_1,         // length of lines from endpoints of test line to target area
      length_2,
      min_x,max_x,      // bounding box of test line
      min_y,max_y;
      
float best_error=10000, // start error off really large
      test_error;       // current error being processed

// process each line and find best fit
for (curr_line=0; curr_line < total_lines; curr_line++)
    {
    // extract line parameters
    sx = lines[curr_line].p0.x;
    sy = lines[curr_line].p0.y;

    ex = lines[curr_line].p1.x;
    ey = lines[curr_line].p1.y;

    // first compute length of line
    length_line = sqrt( (ex-sx) * (ex-sx) + (ey-sy) * (ey-sy) );

    // compute length of first endpoint to selected position
    length_1    = sqrt( (x-sx) * (x-sx) + (y-sy) * (y-sy) );

    // compute length of second endpoint to selected position
    length_2    = sqrt( (ex-x) * (ex-x) + (ey-y) * (ey-y) );

    // compute the bounding box of line
    min_x = MIN(sx,ex)-2;
    min_y = MIN(sy,ey)-2;
    max_x = MAX(sx,ex)+2;
    max_y = MAX(sy,ey)+2;

    // if the selection position is within bounding box then compute distance
    // errors and save this line as a possibility

    if (x >= min_x && x <= max_x && y >= min_y && y <= max_y)
       {
       // compute percent error of total length to length of lines
       // from endpoint to selected position
       test_error = (float)(100 * fabs( (length_1 + length_2) - length_line) ) / (float)length_line;

       // test if this line is a better selection than the last
       if (test_error < best_error)
          {
          // make this line the "best selection" so far
          best_error = test_error;
          best_line  = curr_line;
          } // end if

       } // end if in bounding box

    } // end for curr_line

// did we get a line to delete?
if (best_line != -1)
   {
   // delete the line from the line array by copying another line into
   // the position

   // test for deletion enable or just viewing
   if (delete_line)
      {
      // test for special cases
      if (total_lines == 1) // a single line
         total_lines = 0;
      else
      if (best_line == (total_lines-1))  // the line to delete is the last in array
         total_lines--;
      else
         {
         // the line to delete must be in the 0th to total_lines-1 position
         // so copy the last line into the deleted one and decrement the
         // number of lines in system
         lines[best_line] = lines[--total_lines];
         } // end else
      } // end if

   // return the line number that was deleted
   return(best_line);
   } // end if
else
    return(-1);

} // end Delete_Line

///////////////////////////////////////////////////////////////////////////////

void Load_Gadgets(void)
{
// this function simply loads all the gadget images into the global image 
// array, additionally it sets up the pointers the images in the buttons
// array

// create the bob image to hold all the button images
Create_BOB(&button_img, 0,0,128,48,10, BOB_ATTR_VISIBLE | BOB_ATTR_SINGLE_FRAME, DDSCAPS_SYSTEMMEMORY, 0, 16); 

// set visibility
button_img.attr |= BOB_ATTR_VISIBLE;

// load the bitmap to extract imagery
Load_Bitmap_File(&bitmap16bit, "bspguicon01.bmp");

// interate and extract both the off and on bitmaps for each animated button
for (int index = 0; index < 10; index++)
    {
    // load the frame left to right, row by row
    Load_Frame_BOB16(&button_img, &bitmap16bit,index,index%2,index/2,BITMAP_EXTRACT_MODE_CELL);

    } // end for index

    // fix pointers in gadget array
    buttons[GADGET_ID_SEGMENT_MODE_ID].bitmap  = &button_img;
    buttons[GADGET_ID_POLYLINE_MODE_ID].bitmap = &button_img;
    buttons[GADGET_ID_DELETE_MODE_ID].bitmap   = &button_img;
    buttons[GADGET_ID_CLEAR_ALL_ID].bitmap     = &button_img;
    buttons[GADGET_ID_FLOOR_MODE_ID].bitmap    = &button_img;

// unload the bitmap
Unload_Bitmap_File(&bitmap16bit);

} // end Load_Gadgets

//////////////////////////////////////////////////////////////////////////////

void Draw_Gadgets(LPDIRECTDRAWSURFACE7 lpdds)
{
// draws all the gadgets, note that buffer must be unlocked

// interate and draw all the buttons 
for (int index = 0; index < NUM_GADGETS; index++)
    {
    // get the current button
    GADGET_PTR curr_button = &buttons[index];

    // what type of button
    switch(curr_button->type)
          {
          case GADGET_TYPE_TOGGLE:    // toggles from off/on, on/off
               {
               // set the frame of the button
               button_img.curr_frame = curr_button->id + curr_button->state;
     
               // set position of the button
               button_img.x = curr_button->x0;
               button_img.y = curr_button->y0;
  
               // draw the button               
               Draw_BOB16(&button_img, lpdds);

               } break;

          case GADGET_TYPE_MOMENTARY: // lights up while you press it
               {
               // set the frame of the button
               button_img.curr_frame = curr_button->id + curr_button->state;
     
               // set position of the button
               button_img.x = curr_button->x0;
               button_img.y = curr_button->y0;
  
               // draw the button               
               Draw_BOB16(&button_img, lpdds);

               // increment counter
               if (curr_button->state == GADGET_ON)
                  {
                  if (++curr_button->count > 2)
                     {
                     curr_button->state = GADGET_OFF;
                     curr_button->count = 0;
                     } // end if
                  } // end if
                  
               } break;

          case GADGET_TYPE_STATIC:    // doesn't do anything visual    
               {
               // do nothing

               } break;

          default:break;
          
          } // end switch

    } // end for index

} // end Draw_Gadgets

//////////////////////////////////////////////////////////////////////////////

int Process_Gadgets(int mx, int my, UCHAR mbuttons[])
{
// this function processes the gadgets messages and changes their states
// based on clicks from the mouse

// iterate thru all the gadgets and determine if there has been a click in
// any of the clickable areas 

for (int index = 0; index < NUM_GADGETS; index++)
    {
    // get the current button
    GADGET_PTR curr_button = &buttons[index];

    // test for a button press
    if (mbuttons[0] | mbuttons[1] | mbuttons[2])
    {
    // determine if there has been a collision
    if ( (mx > curr_button->x0) && (mx < curr_button->x0+curr_button->width) && 
         (my > curr_button->y0) && (my < curr_button->y0+curr_button->height) )
    {
    // what type of button
    switch(curr_button->type)
          {
          case GADGET_TYPE_TOGGLE:    // toggles from off/on, on/off
               {
               // set the frame of the button
               if (curr_button->state == GADGET_OFF)
                  curr_button->state = GADGET_ON;

               // reset all other mutual exclusive toggles
               for (int index2 = 0; index2 < NUM_GADGETS; index2++)
                   if (index2!=index && buttons[index2].type == GADGET_TYPE_TOGGLE)
                       buttons[index2].state = GADGET_OFF;

               // return the id of the button pressed
               return(curr_button->id);

               } break;

          case GADGET_TYPE_MOMENTARY: // lights up while you press it
               {
               // set the frame of the button to onn
               curr_button->state = GADGET_ON;

               // return the id of the button pressed
               return(curr_button->id);

               } break;

          case GADGET_TYPE_STATIC:    // doesn't do anything visual    
               {
               // do nothing

               // return the id of the button pressed
               return(curr_button->id);

               } break;

          default:break;
          
          } // end switch

    } // end if collision detected

    } // end if button pressed
    else
    {
    // reset buttons, etc. do housekeeping

    } // end else   


    } // end for index

// nothing was pressed
return(-1);

} // end Process_Gadgets

//////////////////////////////////////////////////////////////////////////////

void Draw_Grid(UCHAR *video_buffer, 
               int mempitch,
               int rgbcolor, 
               int x0, int y0, 
               int xpitch, int ypitch,
               int xcells, int ycells)
{
// this function draws the grid
for (int xc=0; xc < xcells; xc++)
    {
    for (int yc=0; yc < ycells; yc++)
        {
        Draw_Pixel16(x0 + xc*xpitch, y0 + yc*ypitch, rgbcolor, video_buffer, mempitch);
        } // end for y
    } // end for x

} // end Draw_Grid

//////////////////////////////////////////////////////////////////////////////

/*
BSP .LEV file format version 1.0

d integer, f floating point value

Version: f    <-- the version, 1.0 in this case

NumSections: d  <-- number of sections, always 2 for now

Section: walls
NumWalls: n    <-- number of walls 

x0.f y0.f x1.f y1.f evel.f height.f text_id.d color.d attr.d <-- endpoints, elevation, and height of wall, texture id, color, attributes
.
.
.
x0.f y0.f x1.f y1.f elev.f height.f text_id.d color.d attr.d <-- endpoints, elevation, and height of wall, texture id, color, attributes

EndSection

Section: floors
NumFloorsX: d              < -- number of floor cells in X directions, or columns
NumFloorsY: d              < -- number of floor cells in Y direction, or rows

id0.d id1.d id2.d ..... id(NumFloorsX - 1).d    < -- a total of NumFloorsY rows, each idi is the texture id
.                                               < -- -1 means no floor tile there
.
.
.
id0.d id1.d id2.d ..... id(NumFloorsX - 1).d

EndSection

example*********


Version: 1.0
NumSections: 2

Section: walls
NumWalls: 4 
10 10 20 10 0 10 0
20 10 20 20 0 10 0
20 20 10 20 0 10 0
10 20 10 10 0 10 0
Ends

Section: floors
NumFloorsX: 6
NumFloorsY: 4
-1 0 0 -1 -1 -1
-1 0 0 -1 -1 -1
-1 -1 -1 -1 -1 -1
-1 -1 -1 -1 -1 -1
ends
*/


int Load_BSP_File_LEV(char *filename)
{
// this function loads a bsp .LEV file, it uses very simple fscanf functions
// rather than our parser 
float version;     // holds the version number

int num_sections,  // number of sections
    num_walls,     // number of walls
    bsp_cells_x,   // number of cells on x,y axis
    bsp_cells_y;

FILE *fp; // file pointer

char token1[80], token2[80], token3[80]; // general string tokens

// try and open file for writing
if ((fp = fopen(filename, "r")) == NULL)
   {
   sprintf(buffer, "Error - File %s not found.",filename);
   MessageBox(main_window_handle, buffer, "BSP Level Generator",MB_OK);   
   return(0);
   } // end if

// read version
fscanf(fp, "%s %f", token1, &version);

if (version!=1.0f)
   {
   sprintf(buffer, "Error - Wrong Version.",filename);
   MessageBox(main_window_handle, buffer, "BSP Level Generator",MB_OK);   
   return(0);
   } // end if

// read number of sections
fscanf(fp, "%s %d", token1, &num_sections);

if (num_sections!=2)
   {
   sprintf(buffer, "Error - Wrong Number of Sections.",filename);
   MessageBox(main_window_handle, buffer, "BSP Level Generator",MB_OK);   
   return(0);
   } // end if

// read wall section
fscanf(fp,"%s %s", token1, token2);

// read number of walls
fscanf(fp,"%s %d", token1, &num_walls);

// read each wall
for (int w_index = 0; w_index < num_walls; w_index++)
    {
    // x0.f y0.f x1.f y1.f elev.f height.f text_id.d color.id attr.d
    fscanf(fp, "%f %f %f %f %d %d %d %d %d", &lines[w_index].p0.x, 
                                             &lines[w_index].p0.y,
                                             &lines[w_index].p1.x, 
                                             &lines[w_index].p1.y,
                                             &lines[w_index].elev, 
                                             &lines[w_index].height,
                                             &lines[w_index].texture_id,
                                             &lines[w_index].color,
                                             &lines[w_index].attr );

    } // end for w_index

// read the end section 
fscanf(fp,"%s", token1);

// read floor section
fscanf(fp,"%s %s", token1, token2);

// read number of x floors
fscanf(fp,"%s %d", token1, &bsp_cells_x);

// read number of y floors
fscanf(fp,"%s %d", token1, &bsp_cells_y);

// read in actual texture indices of floor tiles
for (int y_index = 0; y_index < BSP_CELLS_Y-1; y_index++)
    {
    // read in next row of indices
    for (int x_index = 0; x_index < BSP_CELLS_X-1; x_index++)
        {
        fscanf(fp, "%d", &floors[y_index][x_index]);
        } // end for x_index       

    } // end for

// read the end section 
fscanf(fp,"%s", token1);

// set globals
total_lines = num_walls;
// bsp_cells_x
// bsp_cells_y

// close the file
fclose(fp);

// return success
return(1);

} // end Load_BSP_File_LEV

//////////////////////////////////////////////////////////////////////////////

int Save_BSP_File_LEV(char *filename, int flags)
{
// this function loads a bsp .LEV file using fprint for simplicity
FILE *fp; // file pointer

// try and open file for writing
if ((fp = fopen(filename, "w")) == NULL)
   return(0);

// write version
fprintf(fp, "\nVersion: 1.0");

// print out a newline
fprintf(fp, "\n");

// write number of sections
fprintf(fp, "\nNumSections: 2");

// print out a newline
fprintf(fp, "\n");

// write wall section
fprintf(fp,"\nSection: walls");

// print out a newline
fprintf(fp, "\n");

// write number of walls
fprintf(fp,"\nNumWalls: %d", total_lines);

// print out a newline
fprintf(fp, "\n");

// write each wall
for (int w_index = 0; w_index < total_lines; w_index++)
    {
    // x0.f y0.f x1.f y1.f elev.f height.f text_id.d
    fprintf(fp, "\n%d %d %d %d %d %d %d %d %d ", (int)lines[w_index].p0.x, 
                                                 (int)lines[w_index].p0.y,
                                                 (int)lines[w_index].p1.x, 
                                                 (int)lines[w_index].p1.y,
                                                 lines[w_index].elev, 
                                                 lines[w_index].height,
                                                 lines[w_index].texture_id,
                                                 lines[w_index].color,
                                                 lines[w_index].attr);
    } // end for w_index

// print out a newline
fprintf(fp, "\n");

// write the end section 
fprintf(fp,"\nEndSection");

// print out a newline
fprintf(fp, "\n");


// write floor section
fprintf(fp,"\nSection: floors");

// print out a newline
fprintf(fp, "\n");

// write number of x floors
fprintf(fp,"\nNumFloorsX: %d", BSP_CELLS_X-1);

// write number of y floors
fprintf(fp,"\nNumFloorsY: %d", BSP_CELLS_Y-1);

// print out a newline
fprintf(fp, "\n");

// write out actual texture indices of floor tiles
for (int y_index = 0; y_index < BSP_CELLS_Y-1; y_index++)
    {
    // print out a newline
    fprintf(fp, "\n");

    // print out next row of indices
    for (int x_index = 0; x_index < BSP_CELLS_X-1; x_index++)
        {
        fprintf(fp, "%d ", floors[y_index][x_index]);
        } // end for x_index       

    } // end for

// print out a newline
fprintf(fp, "\n");

// write the end section 
fprintf(fp,"\nEndSection");

// close the file
fclose(fp);

// return success
return(1);

} // end Save_BSP_File_LEV

//////////////////////////////////////////////////////////////////////////////

void Convert_Lines_To_Walls(void)
{
// this function converts the list of 2-D lines into a linked list of 3-D
// walls, computes the normal of each wall and sets the pointers up
// also the function labels the lines on the screen so the user can see them

BSPNODEV1_PTR last_wall, // used to track the last wall processed
           temp_wall;    // used as a temporary to build a wall up

VECTOR4D u,v;            // working vectors

char buffer[256];        // text buffer

// process each 2-d line and convert it into a 3-d wall
for (int index=0; index < total_lines; index++)
    {
    // allocate the memory for the wall
    temp_wall = (BSPNODEV1_PTR)malloc(sizeof(BSPNODEV1));
    memset(temp_wall, 0, sizeof(BSPNODEV1));

    // set up links
    temp_wall->link  = NULL;
    temp_wall->front = NULL;
    temp_wall->back  = NULL;

    // assign points, note how y and z are transposed and the y's of the
    // walls are fixed, this is because we ae looking down on the universe
    // from an aerial view and the wall height is arbitrary; however, with
    // the constants we have selected the walls are WALL_CEILING units tall centered
    // about the x-z plane

    // vertex 0
    temp_wall->wall.vlist[0].x = WORLD_SCALE_X * (SCREEN_TO_WORLD_X + lines[index].p0.x);
    temp_wall->wall.vlist[0].y = lines[index].elev + lines[index].height;
    temp_wall->wall.vlist[0].z = WORLD_SCALE_Z * (SCREEN_TO_WORLD_Z + lines[index].p0.y);
    temp_wall->wall.vlist[0].w = 1;

    // vertex 1
    temp_wall->wall.vlist[1].x = WORLD_SCALE_X * (SCREEN_TO_WORLD_X + lines[index].p1.x);
    temp_wall->wall.vlist[1].y = lines[index].elev + lines[index].height;
    temp_wall->wall.vlist[1].z = WORLD_SCALE_Z * (SCREEN_TO_WORLD_Z + lines[index].p1.y);
    temp_wall->wall.vlist[1].w = 1;

    // vertex 2
    temp_wall->wall.vlist[2].x = WORLD_SCALE_X * (SCREEN_TO_WORLD_X + lines[index].p1.x);
    temp_wall->wall.vlist[2].y = lines[index].elev;
    temp_wall->wall.vlist[2].z = WORLD_SCALE_Z * (SCREEN_TO_WORLD_Z + lines[index].p1.y);
    temp_wall->wall.vlist[2].w = 1;

    // vertex 3
    temp_wall->wall.vlist[3].x = WORLD_SCALE_X * (SCREEN_TO_WORLD_X + lines[index].p0.x);
    temp_wall->wall.vlist[3].y = lines[index].elev;
    temp_wall->wall.vlist[3].z = WORLD_SCALE_Z * (SCREEN_TO_WORLD_Z + lines[index].p0.y);
    temp_wall->wall.vlist[3].w = 1;

    // compute normal to wall

    // find two vectors co-planer in the wall
    VECTOR4D_Build(&temp_wall->wall.vlist[0].v, &temp_wall->wall.vlist[1].v,&u);
    VECTOR4D_Build(&temp_wall->wall.vlist[0].v, &temp_wall->wall.vlist[3].v, &v);

    // use cross product to compute normal
    VECTOR4D_Cross(&u, &v, &temp_wall->wall.normal);

    // normalize the normal vector
    float length = VECTOR4D_Length(&temp_wall->wall.normal);

    temp_wall->wall.normal.x /= length;
    temp_wall->wall.normal.y /= length;
    temp_wall->wall.normal.z /= length; 
    temp_wall->wall.normal.w  = 1.0;
    temp_wall->wall.nlength   = length;

    // set id number for debugging
    temp_wall->id = index;

    // set attributes of polygon
    temp_wall->wall.attr = lines[index].attr;

    // set color of polygon, if texture then white
    // else whatever the color is
    temp_wall->wall.color = lines[index].color;

    // now based on attributes set vertex flags

    // every vertex has a point at least, set that in the flags attribute
    SET_BIT(temp_wall->wall.vlist[0].attr, VERTEX4DTV1_ATTR_POINT); 
    SET_BIT(temp_wall->wall.vlist[1].attr, VERTEX4DTV1_ATTR_POINT);
    SET_BIT(temp_wall->wall.vlist[2].attr, VERTEX4DTV1_ATTR_POINT);
    SET_BIT(temp_wall->wall.vlist[3].attr, VERTEX4DTV1_ATTR_POINT);

    // check for gouraud of phong shading, if so need normals
    if ( (temp_wall->wall.attr & POLY4DV2_ATTR_SHADE_MODE_GOURAUD) ||  
         (temp_wall->wall.attr & POLY4DV2_ATTR_SHADE_MODE_PHONG) )
       {
       // the vertices from this polygon all need normals, set that in the flags attribute
       SET_BIT(temp_wall->wall.vlist[0].attr, VERTEX4DTV1_ATTR_NORMAL); 
       SET_BIT(temp_wall->wall.vlist[1].attr, VERTEX4DTV1_ATTR_NORMAL);
       SET_BIT(temp_wall->wall.vlist[2].attr, VERTEX4DTV1_ATTR_NORMAL);
       SET_BIT(temp_wall->wall.vlist[3].attr, VERTEX4DTV1_ATTR_NORMAL);
       // ??? make sure to create vertex normal....
       } // end if
     
     // if texture in enabled the enable texture coordinates
     if (lines[index].texture_id >= 0)
        {
        // apply texture to this polygon
        SET_BIT(temp_wall->wall.attr, POLY4DV2_ATTR_SHADE_MODE_TEXTURE);
        temp_wall->wall.texture = &texturemaps[lines[index].texture_id];

        // assign the texture coordinates
        temp_wall->wall.vlist[0].u0 = 0;
        temp_wall->wall.vlist[0].v0 = 0;

        temp_wall->wall.vlist[1].u0 = TEXTURE_SIZE-1;
        temp_wall->wall.vlist[1].v0 = 0;

        temp_wall->wall.vlist[2].u0 = TEXTURE_SIZE-1;
        temp_wall->wall.vlist[2].v0 = TEXTURE_SIZE-1;

        temp_wall->wall.vlist[3].u0 = 0;
        temp_wall->wall.vlist[3].v0 = TEXTURE_SIZE-1;
        
        // set texture coordinate attributes
        SET_BIT(temp_wall->wall.vlist[0].attr, VERTEX4DTV1_ATTR_TEXTURE); 
        SET_BIT(temp_wall->wall.vlist[1].attr, VERTEX4DTV1_ATTR_TEXTURE); 
        SET_BIT(temp_wall->wall.vlist[2].attr, VERTEX4DTV1_ATTR_TEXTURE); 
        SET_BIT(temp_wall->wall.vlist[3].attr, VERTEX4DTV1_ATTR_TEXTURE); 
        } // end if

#if 0
    // check for alpha enable 
    if (temp_wall->wall.attr & POLY4DV2_ATTR_TRANSPARENT)
       {
          int ialpha = (int)( (float)alpha/255 * (float)(NUM_ALPHA_LEVELS-1) + (float)0.5);

          // set the alpha color in upper 8 bits of color
          temp_wall->wall.color += (ialpha << 24);

       } // end if 
#endif

    // finally set the polygon to active
    temp_wall->wall.state = POLY4DV2_STATE_ACTIVE;    
    
    // update ceiling height to max wall height
    if ((lines[index].elev + lines[index].height) > ceiling_height)
       ceiling_height = lines[index].elev + lines[index].height;

    Write_Error("\n\nConverting wall %d, v0=[%f,%f,%f]\n v1=[%f,%f,%f]\n v2=[%f,%f,%f]\n v3=[%f,%f,%f] \nnl=%f", index,
                 temp_wall->wall.vlist[0].x, temp_wall->wall.vlist[0].y, temp_wall->wall.vlist[0].z, 
                 temp_wall->wall.vlist[1].x, temp_wall->wall.vlist[1].y, temp_wall->wall.vlist[1].z, 
                 temp_wall->wall.vlist[2].x, temp_wall->wall.vlist[2].y, temp_wall->wall.vlist[2].z, 
                 temp_wall->wall.vlist[3].x, temp_wall->wall.vlist[3].y, temp_wall->wall.vlist[3].z, 
                 temp_wall->wall.nlength );

    // test if this is first wall
    if (index==0)
       {
       // set head of wall list pointer and last wall pointer
       wall_list = temp_wall;
       last_wall = temp_wall;
       } // end if first wall
    else
       {
       // the first wall has been taken care of

       // link the last wall to the next wall
       last_wall->link = temp_wall;

       // move the last wall to the next wall
       last_wall = temp_wall;
       } // end else

    } // end for index

} // end Convert_Lines_To_Walls

//////////////////////////////////////////////////////////////////////////////

int Generate_Floors_OBJECT4DV2(OBJECT4DV2_PTR obj, // pointer to object
                              int rgbcolor,        // color of floor if no texture        
                              VECTOR4D_PTR pos,    // initial position
                              VECTOR4D_PTR rot,    // initial rotations
                              int poly_attr)       // the shading attributes we would like
{
// this function uses the global floor and texture arrays to generate the
// floor mesh under the player, basically for each floor tile that is not -1
// there should be a floor tile at that position, hence the function uses
// this fact to create an object that is composed of triangles that makes
// up the floor mesh, also the texture pointers are assigned to the global
// texture bitmaps, the function only creates a single floor mesh, the 
// same mesh is used for the ceiling, but only raised during rendering...

char buffer[256];  // working buffer
BITMAP_FILE height_bitmap; // holds the height bitmap

// Step 1: clear out the object and initialize it a bit
memset(obj, 0, sizeof(OBJECT4DV2));

// set state of object to active and visible
obj->state = OBJECT4DV2_STATE_ACTIVE | OBJECT4DV2_STATE_VISIBLE;

// set position of object
obj->world_pos.x = pos->x;
obj->world_pos.y = pos->y;
obj->world_pos.z = pos->z;
obj->world_pos.w = pos->w;

// set number of frames
obj->num_frames = 1;
obj->curr_frame = 0;
obj->attr       = OBJECT4DV2_ATTR_SINGLE_FRAME;

// store any outputs here..
obj->ivar1 = 0;
obj->ivar2 = 0;
obj->fvar1 = 0;
obj->fvar2 = 0;

int floor_tiles = 0;

// set number of vertices and polygons to zero
obj->num_vertices = 0;
obj->num_polys    = 0;

// count up floor tiles
for (int y_index = 0; y_index < BSP_CELLS_Y-1; y_index++)
    {
    // print out next row of indices
    for (int x_index = 0; x_index < BSP_CELLS_X-1; x_index++)
        {
        // is there an active tile here?
        if (floors[y_index][x_index] >=0 )
           {
           // 2 triangles per tile
           obj->num_polys+=2;     

           // 4 vertices per tile
           obj->num_vertices+=4;        
           } // end if 

        } // end for x_index       

    } // end for

// allocate the memory for the vertices and number of polys
// the call parameters are redundant in this case, but who cares
if (!Init_OBJECT4DV2(obj,   // object to allocate
                     obj->num_vertices, 
                     obj->num_polys, 
                     obj->num_frames))
    {
    Write_Error("\nTerrain generator error (can't allocate memory).");
    } // end if

    // flag object as having textures
    SET_BIT(obj->attr, OBJECT4DV2_ATTR_TEXTURES);

// now generate the vertex list and polygon list at the same time
// 2 triangles per floor tile, 4 vertices per floor tile
// generate the data row by row

// initialize indices, we generate on the fly when we find an active tile
int curr_vertex = 0;
int curr_poly   = 0;

for (y_index = 0; y_index < BSP_CELLS_Y-1; y_index++)
    {
    for (int x_index = 0; x_index < BSP_CELLS_X-1; x_index++)
        {
        int texture_id = -1;

        // does this tile have a texture    
        if ((texture_id = floors[y_index][x_index]) >=0 )
        { 
        // compute the vertices

        // vertex 0
        obj->vlist_local[curr_vertex].x = WORLD_SCALE_X * (SCREEN_TO_WORLD_X + (x_index+0)*BSP_GRID_SIZE);
        obj->vlist_local[curr_vertex].y = 0;
        obj->vlist_local[curr_vertex].z = WORLD_SCALE_Z * (SCREEN_TO_WORLD_Z + (y_index+1)*BSP_GRID_SIZE);
        obj->vlist_local[curr_vertex].w = 1;  

        // we need texture coordinates
        obj->tlist[curr_vertex].x = 0;
        obj->tlist[curr_vertex].y = 0;

        // every vertex has a point at least, set that in the flags attribute
        SET_BIT(obj->vlist_local[curr_vertex].attr, VERTEX4DTV1_ATTR_POINT);

        // increment vertex index
        curr_vertex++;

        // vertex 1
        obj->vlist_local[curr_vertex].x = WORLD_SCALE_X * (SCREEN_TO_WORLD_X + (x_index+0)*BSP_GRID_SIZE);
        obj->vlist_local[curr_vertex].y = 0;
        obj->vlist_local[curr_vertex].z = WORLD_SCALE_Z * (SCREEN_TO_WORLD_Z + (y_index+0)*BSP_GRID_SIZE);
        obj->vlist_local[curr_vertex].w = 1;  

        // we need texture coordinates
        obj->tlist[curr_vertex].x = 0;
        obj->tlist[curr_vertex].y = TEXTURE_SIZE-1;

        // every vertex has a point at least, set that in the flags attribute
        SET_BIT(obj->vlist_local[curr_vertex].attr, VERTEX4DTV1_ATTR_POINT);

        // increment vertex index
        curr_vertex++;

        // vertex 2
        obj->vlist_local[curr_vertex].x = WORLD_SCALE_X * (SCREEN_TO_WORLD_X + (x_index+1)*BSP_GRID_SIZE);
        obj->vlist_local[curr_vertex].y = 0;
        obj->vlist_local[curr_vertex].z = WORLD_SCALE_Z * (SCREEN_TO_WORLD_Z + (y_index+0)*BSP_GRID_SIZE);
        obj->vlist_local[curr_vertex].w = 1;  

        // we need texture coordinates
        obj->tlist[curr_vertex].x = TEXTURE_SIZE-1;
        obj->tlist[curr_vertex].y = TEXTURE_SIZE-1;

        // every vertex has a point at least, set that in the flags attribute
        SET_BIT(obj->vlist_local[curr_vertex].attr, VERTEX4DTV1_ATTR_POINT);

        // increment vertex index
        curr_vertex++;

        // vertex 3
        obj->vlist_local[curr_vertex].x = WORLD_SCALE_X * (SCREEN_TO_WORLD_X + (x_index+1)*BSP_GRID_SIZE);
        obj->vlist_local[curr_vertex].y = 0;
        obj->vlist_local[curr_vertex].z = WORLD_SCALE_Z * (SCREEN_TO_WORLD_Z + (y_index+1)*BSP_GRID_SIZE);
        obj->vlist_local[curr_vertex].w = 1;  

        // we need texture coordinates
        obj->tlist[curr_vertex].x = TEXTURE_SIZE-1;
        obj->tlist[curr_vertex].y = 0;

        // every vertex has a point at least, set that in the flags attribute
        SET_BIT(obj->vlist_local[curr_vertex].attr, VERTEX4DTV1_ATTR_POINT);

        // increment vertex index
        curr_vertex++;

        // upper left poly vertex index
        obj->plist[curr_poly].vert[0] = curr_vertex-4; // 0
        obj->plist[curr_poly].vert[1] = curr_vertex-3; // 1
        obj->plist[curr_poly].vert[2] = curr_vertex-2; // 2

        // lower right poly vertex index
        obj->plist[curr_poly+1].vert[0] = curr_vertex-4; // 0
        obj->plist[curr_poly+1].vert[1] = curr_vertex-2; // 2
        obj->plist[curr_poly+1].vert[2] = curr_vertex-1; // 3

        // point polygon vertex list to object's vertex list
        // note that this is redundant since the polylist is contained
        // within the object in this case and its up to the user to select
        // whether the local or transformed vertex list is used when building up
        // polygon geometry, might be a better idea to set to NULL in the context
        // of polygons that are part of an object
        obj->plist[curr_poly].vlist   = obj->vlist_local; 
        obj->plist[curr_poly+1].vlist = obj->vlist_local; 

        // set attributes of polygon with sent attributes
        obj->plist[curr_poly].attr    = poly_attr;
        obj->plist[curr_poly+1].attr  = poly_attr;

        // set color of polygon 
        obj->plist[curr_poly].color   = rgbcolor;
        obj->plist[curr_poly+1].color = rgbcolor;

        // apply texture to this polygon
        obj->plist[curr_poly].texture   = &texturemaps[ floors[y_index][x_index] ];
        obj->plist[curr_poly+1].texture = &texturemaps[ floors[y_index][x_index] ];

        // assign the texture coordinate index
        // upper left poly
        obj->plist[curr_poly].text[0] = curr_vertex-4; // 0
        obj->plist[curr_poly].text[1] = curr_vertex-3; // 1
        obj->plist[curr_poly].text[2] = curr_vertex-2; // 2

        // lower right poly
        obj->plist[curr_poly+1].text[0] = curr_vertex-4; // 0
        obj->plist[curr_poly+1].text[1] = curr_vertex-2; // 2
        obj->plist[curr_poly+1].text[2] = curr_vertex-1; // 3
 
        // set texture coordinate attributes
        SET_BIT(obj->vlist_local[ obj->plist[curr_poly].vert[0] ].attr, VERTEX4DTV1_ATTR_TEXTURE); 
        SET_BIT(obj->vlist_local[ obj->plist[curr_poly].vert[1] ].attr, VERTEX4DTV1_ATTR_TEXTURE); 
        SET_BIT(obj->vlist_local[ obj->plist[curr_poly].vert[2] ].attr, VERTEX4DTV1_ATTR_TEXTURE); 

        SET_BIT(obj->vlist_local[ obj->plist[curr_poly+1].vert[0] ].attr, VERTEX4DTV1_ATTR_TEXTURE); 
        SET_BIT(obj->vlist_local[ obj->plist[curr_poly+1].vert[1] ].attr, VERTEX4DTV1_ATTR_TEXTURE); 
        SET_BIT(obj->vlist_local[ obj->plist[curr_poly+1].vert[2] ].attr, VERTEX4DTV1_ATTR_TEXTURE); 

        // set the material mode to ver. 1.0 emulation
        SET_BIT(obj->plist[curr_poly].attr, POLY4DV2_ATTR_DISABLE_MATERIAL);
        SET_BIT(obj->plist[curr_poly+1].attr, POLY4DV2_ATTR_DISABLE_MATERIAL);

        // finally set the polygon to active
        obj->plist[curr_poly].state   = POLY4DV2_STATE_ACTIVE;    
        obj->plist[curr_poly+1].state = POLY4DV2_STATE_ACTIVE;  

        // point polygon vertex list to object's vertex list
        // note that this is redundant since the polylist is contained
        // within the object in this case and its up to the user to select
        // whether the local or transformed vertex list is used when building up
        // polygon geometry, might be a better idea to set to NULL in the context
        // of polygons that are part of an object
        obj->plist[curr_poly].vlist   = obj->vlist_local; 
        obj->plist[curr_poly+1].vlist = obj->vlist_local; 

        // set texture coordinate list, this is needed
        obj->plist[curr_poly].tlist   = obj->tlist;
        obj->plist[curr_poly+1].tlist = obj->tlist;
       
        // increase polygon count
        curr_poly+=2;
 
        } // end if there is a textured poly here
        
        } // end for x_index

     } // end for y_index

// compute average and max radius
Compute_OBJECT4DV2_Radius(obj);

// compute the polygon normal lengths
Compute_OBJECT4DV2_Poly_Normals(obj);

// compute vertex normals for any gouraud shaded polys
Compute_OBJECT4DV2_Vertex_Normals(obj);

// return success
return(1);

} // end Generate_Floors_OBJECT4DV2
