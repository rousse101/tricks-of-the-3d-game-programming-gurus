
// DEMOII3_2.CPP - full screen 256 color demo
// based on 2D collision response demo from Volume I

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

#define WINDOW_BPP        8   // bitdepth of window (8,16,24 etc.)
                                // note: if windowed and not
                                // fullscreen then bitdepth must
                                // be same as system bitdepth
                                // also if 8-bit the a pallete
                                // is created and attached

#define WINDOWED_APP      0     // 0 not windowed, 1 windowed

// physics demo defines
#define NUM_BALLS       10   // number of pool balls
#define BALL_RADIUS     12   // radius of ball

// extents of table
#define TABLE_MIN_X     100
#define TABLE_MAX_X     500
#define TABLE_MIN_Y     50
#define TABLE_MAX_Y     450

// variable lookup indices
#define INDEX_X               0 
#define INDEX_Y               1  
#define INDEX_XV              2 
#define INDEX_YV              3  
#define INDEX_MASS            4

// MACROS ///////////////////////////////////////////////

#define RAND_RANGE(x,y) ( (x) + (rand()%((y)-(x)+1)))
#define DOT_PRODUCT(ux,uy,vx,vy) ((ux)*(vx) + (uy)*(vy))

// PROTOTYPES /////////////////////////////////////////////

// game console
int Game_Init(void *parms=NULL);
int Game_Shutdown(void *parms=NULL);
int Game_Main(void *parms=NULL);

// GLOBALS ////////////////////////////////////////////////

HWND main_window_handle           = NULL; // save the window handle
HINSTANCE main_instance           = NULL; // save the instance
char buffer[256];                          // used to print text

BITMAP_IMAGE background_bmp;      // holds the background
BOB          balls[NUM_BALLS];    // the balls

int ball_ids[8];                  // sound ids for balls

float cof_E  =    1.0;  // coefficient of restitution, < 1 makes them loose energy
                        // during the collision modeling friction, heat, deformation
                        // etc. > 1 is impossible, but makes them gain energy!

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

int index; // looping varsIable

char filename[80]; // used to build up filenames

// seed random number generate
srand(Start_Clock());

// start up DirectDraw (replace the parms as you desire)
DDraw_Init(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_BPP, WINDOWED_APP);

// load background image
Load_Bitmap_File(&bitmap8bit, "GREENGRID.BMP");
Create_Bitmap(&background_bmp,0,0,640,480);
Load_Image_Bitmap(&background_bmp, &bitmap8bit,0,0,BITMAP_EXTRACT_MODE_ABS);
Set_Palette(bitmap8bit.palette);
Unload_Bitmap_File(&bitmap8bit);

// load the bitmaps
Load_Bitmap_File(&bitmap8bit, "BALLS8.BMP");

// create master ball
Create_BOB(&balls[0],0,0,24,24,6,BOB_ATTR_MULTI_FRAME | BOB_ATTR_VISIBLE, DDSCAPS_SYSTEMMEMORY);

// load the imagery in
for (index=0; index < 6; index++)
    Load_Frame_BOB(&balls[0], &bitmap8bit, index, index,0,BITMAP_EXTRACT_MODE_CELL);

// create all the clones
for (index=1; index < NUM_BALLS; index++)
    Clone_BOB(&balls[0], &balls[index]);

// now set the initial conditions of all the balls
for (index=0; index < NUM_BALLS; index++)
    {
    // set position randomly
    balls[index].varsF[INDEX_X] = RAND_RANGE(TABLE_MIN_X+20,TABLE_MAX_X-20);
    balls[index].varsF[INDEX_Y] = RAND_RANGE(TABLE_MIN_Y+20,TABLE_MAX_Y-20);

    // set initial velocity
    balls[index].varsF[INDEX_XV] = RAND_RANGE(-100, 100)/15;
    balls[index].varsF[INDEX_YV] = RAND_RANGE(-100, 100)/15;

    // set mass of ball in virtual kgs :)
    balls[index].varsF[INDEX_MASS] = 1; // 1 for now

    // set ball color
    balls[index].curr_frame = rand()%6;

    } // end for index

// unload bitmap image
Unload_Bitmap_File(&bitmap8bit);

// hide the mouse
if (!WINDOWED_APP)
ShowCursor(FALSE);

// initialize directinput
DInput_Init();

// acquire the keyboard only
DInput_Init_Keyboard();

// initilize DirectSound
DSound_Init();

// load background sounds
ball_ids[0] = DSound_Load_WAV("PBALL.WAV");

// clone sounds
for (index=1; index<8; index++)
    ball_ids[index] = DSound_Replicate_Sound(ball_ids[0]);

// set clipping rectangle to screen extents so objects dont
// mess up at edges
RECT screen_rect = {0,0,screen_width,screen_height};
lpddclipper = DDraw_Attach_Clipper(lpddsback,1,&screen_rect);

// set clipping region
min_clip_x = TABLE_MIN_X;
min_clip_y = TABLE_MIN_Y;
max_clip_x = TABLE_MAX_X;
max_clip_y = TABLE_MAX_Y;


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

// kill all the bobs
for (int index=0; index<NUM_BALLS; index++)
    Destroy_BOB(&balls[index]);

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

void Ball_Sound(void)
{        
// this functions hunts for an open handle to play a collision sound

// start a hit sound
for (int sound_index=0; sound_index < 8; sound_index++)
    {
    // test if this sound is playing
    if (DSound_Status_Sound(ball_ids[sound_index])==0)
       {
       DSound_Play(ball_ids[sound_index]);
       break;
       } // end if

     } // end for 

} // end Ball_Sound

///////////////////////////////////////////////////////////

void Collision_Response(void)
{
// this function does all the "real" physics to determine if there has
// been a collision between any ball and any other ball, if there is a collision
// the function uses the mass of each ball along with the intial velocities to 
// compute the resulting velocities

// from the book we know that in general 
// va2 = (e+1)*mb*vb1+va1(ma - e*mb)/(ma+mb)
// vb2 = (e+1)*ma*va1+vb1(ma - e*mb)/(ma+mb)

// and the objects will have direction vectors co-linear to the normal
// of the point of collision, but since we are using spheres here as the objects
// we know that the normal to the point of collision is just the vector from the 
// center's of each object, thus the resulting velocity vector of each ball will
// be along this normal vector direction

// step 1: test each object against each other object and test for a collision
// there are better ways to do this other than a double nested loop, but since
// there are a small number of objects this is fine, also we want to somewhat model
// if two or more balls hit simulataneously

for (int ball_a = 0; ball_a < NUM_BALLS; ball_a++)
     {
     for (int ball_b = ball_a+1; ball_b < NUM_BALLS; ball_b++)
         {
         if (ball_a == ball_b) 
            continue;

         // compute the normal vector from a->b
         float nabx = (balls[ball_b].varsF[INDEX_X] - balls[ball_a].varsF[INDEX_X] );
         float naby = (balls[ball_b].varsF[INDEX_Y] - balls[ball_a].varsF[INDEX_Y] );
         float length = sqrt(nabx*nabx + naby*naby);

         // is there a collision?
         if (length <= 2.0*(BALL_RADIUS*.75))
            {
            // the balls have made contact, compute response

            // compute the response coordinate system axes
            // normalize normal vector
            nabx/=length;
            naby/=length;

            // compute the tangential vector perpendicular to normal, simply rotate vector 90
            float tabx =  -naby;
            float taby =  nabx;

            // draw collision
            DDraw_Lock_Primary_Surface();

            // blue is normal
            Draw_Clip_Line(balls[ball_a].varsF[INDEX_X]+0.5, 
               balls[ball_a].varsF[INDEX_Y]+0.5,
               balls[ball_a].varsF[INDEX_X]+20*nabx+0.5,
               balls[ball_a].varsF[INDEX_Y]+20*naby+0.5,
               252, primary_buffer, primary_lpitch); 

            // yellow is tangential
            Draw_Clip_Line(balls[ball_a].varsF[INDEX_X]+0.5, 
               balls[ball_a].varsF[INDEX_Y]+0.5,
               balls[ball_a].varsF[INDEX_X]+20*tabx+0.5,
               balls[ball_a].varsF[INDEX_Y]+20*taby+0.5,
               251, primary_buffer, primary_lpitch); 

             DDraw_Unlock_Primary_Surface();

            // tangential is also normalized since it's just a rotated normal vector
        
            // step 2: compute all the initial velocities
            // notation ball: (a,b) initial: i, final: f, n: normal direction, t: tangential direction

            float vait = DOT_PRODUCT(balls[ball_a].varsF[INDEX_XV], 
                                     balls[ball_a].varsF[INDEX_YV], 
                                     tabx, taby);

            float vain = DOT_PRODUCT(balls[ball_a].varsF[INDEX_XV], 
                                     balls[ball_a].varsF[INDEX_YV], 
                                     nabx, naby);

            float vbit = DOT_PRODUCT(balls[ball_b].varsF[INDEX_XV], 
                                     balls[ball_b].varsF[INDEX_YV], 
                                     tabx, taby);

            float vbin = DOT_PRODUCT(balls[ball_b].varsF[INDEX_XV], 
                                     balls[ball_b].varsF[INDEX_YV], 
                                     nabx, naby);


            // now we have all the initial velocities in terms of the n and t axes
            // step 3: compute final velocities after collision, from book we have
            // note: all this code can be optimized, but I want you to see what's happening :)

            float ma = balls[ball_a].varsF[INDEX_MASS];
            float mb = balls[ball_b].varsF[INDEX_MASS];
 
            float vafn = (mb*vbin*(cof_E+1) + vain*(ma - cof_E*mb)) / (ma + mb);
            float vbfn = (ma*vain*(cof_E+1) - vbin*(ma - cof_E*mb)) / (ma + mb);

            // now luckily the tangential components are the same before and after, so
            float vaft = vait;
            float vbft = vbit;

            // and that's that baby!
            // the velocity vectors are:
            // object a (vafn, vaft)
            // object b (vbfn, vbft)    
 
            // the only problem is that we are in the wrong coordinate system! we need to 
            // translate back to the original x,y coordinate system, basically we need to 
            // compute the sum of the x components relative to the n,t axes and the sum of
            // the y components relative to the n,t axis, since n,t may both have x,y
            // components in the original x,y coordinate system
            
            float xfa = vafn*nabx + vaft*tabx;
            float yfa = vafn*naby + vaft*taby;

            float xfb = vbfn*nabx + vbft*tabx;
            float yfb = vbfn*naby + vbft*taby;

            // store results
            balls[ball_a].varsF[INDEX_XV] = xfa;
            balls[ball_a].varsF[INDEX_YV] = yfa;

            balls[ball_b].varsF[INDEX_XV] = xfb;
            balls[ball_b].varsF[INDEX_YV] = yfb;

            // update position
            balls[ball_a].varsF[INDEX_X]+=balls[ball_a].varsF[INDEX_XV];
            balls[ball_a].varsF[INDEX_Y]+=balls[ball_a].varsF[INDEX_YV];

            balls[ball_b].varsF[INDEX_X]+=balls[ball_b].varsF[INDEX_XV];
            balls[ball_b].varsF[INDEX_Y]+=balls[ball_b].varsF[INDEX_YV];

            } // end if

         } // end for ball2

     } // end for ball1

} // end Collision_Response

//////////////////////////////////////////////////////////

int Game_Main(void *parms)
{
// this is the workhorse of your game it will be called
// continuously in real-time this is like main() in C
// all the calls for you game go here!

int index; // looping var

// start the timing clock
Start_Clock();


// lock back buffer and copy background into it
DDraw_Lock_Back_Surface();

// draw background
Draw_Bitmap(&background_bmp, back_buffer, back_lpitch,0);

// draw table
HLine(TABLE_MIN_X, TABLE_MAX_X, TABLE_MIN_Y, 250, back_buffer, back_lpitch);
HLine(TABLE_MIN_X, TABLE_MAX_X, TABLE_MAX_Y, 250, back_buffer, back_lpitch);
VLine(TABLE_MIN_Y, TABLE_MAX_Y, TABLE_MIN_X, 250, back_buffer, back_lpitch);
VLine(TABLE_MIN_Y, TABLE_MAX_Y, TABLE_MAX_X, 250, back_buffer, back_lpitch);


// unlock back surface
DDraw_Unlock_Back_Surface();

// read keyboard
DInput_Read_Keyboard();


// check for change of e
if (keyboard_state[DIK_RIGHT])
    cof_E+=.01;
else
if (keyboard_state[DIK_LEFT])
    cof_E-=.01;

float total_ke_x = 0, total_ke_y = 0;

// move all the balls and compute system momentum
for (index=0; index < NUM_BALLS; index++)
    {
    // move the ball
    balls[index].varsF[INDEX_X]+=balls[index].varsF[INDEX_XV];
    balls[index].varsF[INDEX_Y]+=balls[index].varsF[INDEX_YV];

    // add x,y contributions to kinetic energy
    total_ke_x+=(balls[index].varsF[INDEX_XV]*balls[index].varsF[INDEX_XV]*balls[index].varsF[INDEX_MASS]);
    total_ke_y+=(balls[index].varsF[INDEX_YV]*balls[index].varsF[INDEX_YV]*balls[index].varsF[INDEX_MASS]);

    } // end fof
    
// test for boundary collision with virtual table edge, no need for collision
// response here, I know what's going to happen :)
for (index=0; index < NUM_BALLS; index++)
    {
    if ((balls[index].varsF[INDEX_X] >= TABLE_MAX_X-BALL_RADIUS) || 
        (balls[index].varsF[INDEX_X] <= TABLE_MIN_X+BALL_RADIUS))
        {
        // invert velocity
        balls[index].varsF[INDEX_XV] = -balls[index].varsF[INDEX_XV];

        balls[index].varsF[INDEX_X]+=balls[index].varsF[INDEX_XV];
        balls[index].varsF[INDEX_Y]+=balls[index].varsF[INDEX_YV];

        // start a hit sound
        Ball_Sound();

        } // end if

    if ((balls[index].varsF[INDEX_Y] >= TABLE_MAX_Y-BALL_RADIUS) || 
        (balls[index].varsF[INDEX_Y] <= TABLE_MIN_Y+BALL_RADIUS))
        {
        // invert velocity
        balls[index].varsF[INDEX_YV] =-balls[index].varsF[INDEX_YV];

        balls[index].varsF[INDEX_X]+=balls[index].varsF[INDEX_XV];
        balls[index].varsF[INDEX_Y]+=balls[index].varsF[INDEX_YV];

        // play sound
        Ball_Sound();

        } // end if

    } // end for index

// draw the balls
for (index=0; index < NUM_BALLS; index++)
    {
    balls[index].x = balls[index].varsF[INDEX_X]+0.5-BALL_RADIUS;
    balls[index].y = balls[index].varsF[INDEX_Y]+0.5-BALL_RADIUS;
    
    Draw_BOB(&balls[index], lpddsback);
    } // end for

// draw the velocity vectors
DDraw_Lock_Back_Surface();
for (index=0; index < NUM_BALLS; index++)
    {
    Draw_Clip_Line(balls[index].varsF[INDEX_X]+0.5, 
              balls[index].varsF[INDEX_Y]+0.5,
              balls[index].varsF[INDEX_X]+2*balls[index].varsF[INDEX_XV]+0.5,
              balls[index].varsF[INDEX_Y]+2*balls[index].varsF[INDEX_YV]+0.5,
              246, back_buffer, back_lpitch); 
    } // end for
DDraw_Unlock_Back_Surface();

// draw the title
Draw_Text_GDI("ELASTIC Object-Object Collision Response DEMO, Press <ESC> to Exit.",10, 10,RGB(255,255,255), lpddsback);

// draw the title
sprintf(buffer,"Coefficient of Restitution e=%f, use <RIGHT>, <LEFT> arrow to change.", cof_E);
Draw_Text_GDI(buffer,10, 30,RGB(255,255,255), lpddsback);

sprintf(buffer,"Total System Kinetic Energy Sum(1/2MiVi^2)=%f ",0.5*sqrt(total_ke_x*total_ke_x+total_ke_y*total_ke_y));
Draw_Text_GDI(buffer,10, 465, RGB(255,255,255), lpddsback);


// flip the surfaces
DDraw_Flip();

// run collision response algorithm here
Collision_Response();

// sync to 30 fps = 1/30sec = 33 ms
Wait_Clock(33);

// check of user is trying to exit
if (KEY_DOWN(VK_ESCAPE) || keyboard_state[DIK_ESCAPE])
    {
    PostMessage(main_window_handle, WM_DESTROY,0,0);

    // stop all sounds
    DSound_Stop_All_Sounds();

    // do a screen transition
    // Screen_Transitions(SCREEN_GREENNESS,NULL,0);
    } // end if

// return success
return(1);

} // end Game_Main

//////////////////////////////////////////////////////////