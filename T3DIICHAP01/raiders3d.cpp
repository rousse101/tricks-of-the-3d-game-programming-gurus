// Raiders3D - RAIDERS3D.CPP -- our first 3D game

// READ THIS!
// To compile make sure to include DDRAW.LIB, DSOUND.LIB,
// DINPUT.LIB, WINMM.LIB in the project link list, and of course 
// the C++ source modules T3DLIB1.CPP,T3DLIB2.CPP, and T3DLIB3.CPP
// in the project!!! And the T3DLIB1.H,T3DLIB2.H, and T3DLIB3.H
// header files in the working directory, so the compiler
// can find them

// to run the game make sure that your desktop is in 16bit
// color mode with a resolution of 640x480 or higher

// INCLUDES ///////////////////////////////////////////////

#define INITGUID       // make sure all the COM interfaces are available
// instead of this you can include the .LIB file
// DXGUID.LIB

#define WIN32_LEAN_AND_MEAN  

#include "CommonHeader.h"

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

#define WINDOW_BPP        16   // bitdepth of window (8,16,24 etc.)
// note: if windowed and not
// fullscreen then bitdepth must
// be same as system bitdepth
// also if 8-bit the a pallete
// is created and attached

#define WINDOWED_APP      1     // 0 not windowed, 1 windowed

////////////////////////////////////////////////////////////

#define NUM_STARS    512  // number of stars in sim
#define NUM_TIES     32   // number of tie fighters in sim

// 3D engine constants
#define NEAR_Z          10   // the near clipping plane
#define FAR_Z           2000 // the far clipping plane    
#define VIEW_DISTANCE   320  // viewing distance from viewpoint 
// this gives a field of view of 90 degrees
// when projected on a window of 640 wide
// player constants
#define CROSS_VEL       8  // speed that the cross hair moves
#define PLAYER_Z_VEL    8  // virtual z velocity that player is moving
// to simulate motion without moving

// tie fighter model constants
#define NUM_TIE_VERTS     10
#define NUM_TIE_EDGES     8

// explosiond
#define NUM_EXPLOSIONS    (NUM_TIES) // total number of explosions

// game states
#define GAME_RUNNING      1
#define GAME_OVER         0

// TYPES ///////////////////////////////////////////////////

// this a 3D point
typedef struct POINT3D_TYP
{
    USHORT color;     // color of point 16-bit
    float x,y,z;      // coordinates of point in 3D
} POINT3D, *POINT3D_PTR;

// this is a 3D line, nothing more than two indices into a vertex list
typedef struct LINE3D_TYP
{
    USHORT color;  // color of line 16-bit
    int v1,v2;     // indices to endpoints of line in vertex list

} LINE3D, *LINE3D_PTR;

// a tie fighter
typedef struct TIE_TYP
{
    int state;      // state of the tie, 0=dead, 1=alive
    float x, y, z;  // position of the tie
    float xv,yv,zv; // velocity of the tie
} TIE, *TIE_PTR;

// a basic 3D vector used for velocity
typedef struct VEC3D_TYP
{
    float x,y,z; // coords of vector
} VEC3D, *VEC3D_PTR;

// a wireframe explosion
typedef struct EXPL_TYP
{
    int state;       // state of explosion
    int counter;     // counter for explosion 
    USHORT color;    // color of explosion

    // an explosion is a collection of edges/lines
    // based on the 3D model of the tie that is exploding
    POINT3D p1[NUM_TIE_EDGES];  // start point of edge n
    POINT3D p2[NUM_TIE_EDGES];  // end point of edge n

    VEC3D   vel[NUM_TIE_EDGES]; // velocity of shrapnel

} EXPL, *EXPL_PTR;

// PROTOTYPES /////////////////////////////////////////////

// game console
int Game_Init(void *parms=NULL);
int Game_Shutdown(void *parms=NULL);
int Game_Main(void *parms=NULL);

// game functions
void Init_Tie(int index);

// GLOBALS ////////////////////////////////////////////////

HWND main_window_handle           = NULL; // save the window handle
HINSTANCE main_instance           = NULL; // save the instance
char buffer[256];                         // used to print text

// the tie fighter is a collection of vertices connected by
// lines that make up the shape. only one tie fighter see if 
// you can replicate it?

POINT3D tie_vlist[NUM_TIE_VERTS]; // vertex list for the tie fighter model
LINE3D  tie_shape[NUM_TIE_EDGES]; // edge list for the tie fighter model
TIE     ties[NUM_TIES];           // tie fighters

POINT3D stars[NUM_STARS]; // the starfield

// some colors, note we can't build them until we know the bit
// depth format 5.5.5 or 5.6.5, so we wait a minute and do it in the
// Game_Init() function
USHORT rgb_green, rgb_white, rgb_red, rgb_blue;

// player vars
float cross_x = 0, // cross hairs
cross_y = 0; 

int cross_x_screen  = WINDOW_WIDTH/2,   // used for cross hair
cross_y_screen  = WINDOW_HEIGHT/2, 
target_x_screen = WINDOW_WIDTH/2,   // used for targeter
target_y_screen = WINDOW_HEIGHT/2;

int player_z_vel = 4; // virtual speed of viewpoint/ship
int cannon_state = 0; // state of laser cannon
int cannon_count = 0; // laster cannon counter

EXPL explosions[NUM_EXPLOSIONS]; // the explosiions

int misses = 0; // tracks number of missed ships
int hits   = 0; // tracks number of hits
int score  = 0; // take a guess :)

// music and sound stuff
int main_track_id = -1, // main music track id
laser_id      = -1, // sound of laser pulse
explosion_id  = -1, // sound of explosion
flyby_id      = -1; // sound of tie fighter flying by

int game_state = GAME_RUNNING; // state of game

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

    int index = 0; // used for looping

    Open_Error_File("error.txt");

    // start up DirectDraw (replace the parms as you desire)
    DDraw_Init(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_BPP, WINDOWED_APP);

    // initialize directinput
    DInput_Init();

    // acquire the keyboard 
    DInput_Init_Keyboard();

    // initialize directsound
    DSound_Init();

    // load in sound fx
    explosion_id = DSound_Load_WAV("exp1.wav");
    laser_id     = DSound_Load_WAV("shocker.wav");

    // initialize directmusic
    DMusic_Init();

    // load and start main music track
    main_track_id = DMusic_Load_MIDI("midifile2.mid");
    DMusic_Play(main_track_id);

    // add calls to acquire other directinput devices here...

    // hide the mouse
    if (!WINDOWED_APP)
        ShowCursor(FALSE);

    // seed random number generator
    srand(Start_Clock());

    // all your initialization code goes here...

    // create system colors
    rgb_green = RGB16Bit(0,255,0);
    rgb_white = RGB16Bit(255,255,255);
    rgb_blue  = RGB16Bit(0,0,255);
    rgb_red   = RGB16Bit(255,0,0);

    // create the starfield
    for (index=0; index < NUM_STARS; index++)
    {
        // randomly position stars in an elongated cylinder stretching from
        // the viewpoint 0,0,-d to the yon clipping plane 0,0,far_z
        stars[index].x = -WINDOW_WIDTH/2  + rand()%WINDOW_WIDTH;
        stars[index].y = -WINDOW_HEIGHT/2 + rand()%WINDOW_HEIGHT;
        stars[index].z = NEAR_Z + rand()%(FAR_Z - NEAR_Z);

        // set color of stars
        stars[index].color = rgb_white;
    } // end for index

    // create the tie fighter model

    // the vertex list for the tie fighter
    POINT3D temp_tie_vlist[NUM_TIE_VERTS] = 
        // color, x,y,z
    { {rgb_white,-40,40,0},    // p0
    {rgb_white,-40,0,0},    // p1
    {rgb_white,-40,-40,0},   // p2
    {rgb_white,-10,0,0},    // p3
    {rgb_white,0,20,0},     // p4
    {rgb_white,10,0,0},     // p5
    {rgb_white,0,-20,0},    // p6
    {rgb_white,40,40,0},     // p7
    {rgb_white,40,0,0},     // p8
    {rgb_white,40,-40,0}};   // p9

    // copy the model into the real global arrays
    for (index=0; index<NUM_TIE_VERTS; index++)
        tie_vlist[index] = temp_tie_vlist[index];

    // the edge list for the tie fighter
    LINE3D temp_tie_shape[NUM_TIE_EDGES] = 
        // color, vertex 1, vertex 2
    { {rgb_green,0,2      },    // l0
    {rgb_green,1,3      },    // l1
    {rgb_green,3,4      },    // l2
    {rgb_green,4,5      },    // l3
    {rgb_green,5,6      },    // l4
    {rgb_green,6,3      },    // l5
    {rgb_green,5,8      },    // l6
    {rgb_green,7,9      } };  // l7

    // copy the model into the real global arrays
    for (index=0; index<NUM_TIE_EDGES; index++)
        tie_shape[index] = temp_tie_shape[index];

    // initialize the position of each tie fighter and it's velocity
    for (index=0; index<NUM_TIES; index++)
    {
        // initialize this tie fighter
        Init_Tie(index);

    } // end for index

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

void Start_Explosion(int tie)
{
    // this starts an explosion based on the sent tie fighter

    // first hunt and see if an explosion is free
    for (int index=0; index < NUM_EXPLOSIONS; index++)
    {
        if (explosions[index].state==0)
        {
            // start this explosion up using the properties
            // if the tie figther index sent

            explosions[index].state   = 1; // enable state of explosion
            explosions[index].counter = 0; // reset counter for explosion 

            // set color of explosion
            explosions[index].color = rgb_green;

            // make copy of of edge list, so we can blow it up
            for (int edge=0; edge < NUM_TIE_EDGES; edge++)
            {
                // start point of edge
                explosions[index].p1[edge].x = ties[tie].x+tie_vlist[tie_shape[edge].v1].x;
                explosions[index].p1[edge].y = ties[tie].y+tie_vlist[tie_shape[edge].v1].y;
                explosions[index].p1[edge].z = ties[tie].z+tie_vlist[tie_shape[edge].v1].z;

                // end point of edge
                explosions[index].p2[edge].x = ties[tie].x+tie_vlist[tie_shape[edge].v2].x;
                explosions[index].p2[edge].y = ties[tie].y+tie_vlist[tie_shape[edge].v2].y;
                explosions[index].p2[edge].z = ties[tie].z+tie_vlist[tie_shape[edge].v2].z;

                // compute trajectory vector for edges
                explosions[index].vel[edge].x = ties[tie].xv - 8+rand()%16;
                explosions[index].vel[edge].y = ties[tie].yv - 8+rand()%16;
                explosions[index].vel[edge].z = -3+rand()%4;

            } // end for edge

            // done, so return
            return;
        } // end if found

    } // end for index

} // end Start_Explosion

///////////////////////////////////////////////////////////

void Process_Explosions(void)
{
    // this processes all the explosions

    // loop thro all the explosions and render them
    for (int index=0; index<NUM_EXPLOSIONS; index++)
    {
        // test if this explosion is active?
        if (explosions[index].state==0)
            continue;

        for (int edge=0; edge<NUM_TIE_EDGES; edge++)
        {
            // must be exploding, update edges (shrapel)
            explosions[index].p1[edge].x+=explosions[index].vel[edge].x;
            explosions[index].p1[edge].y+=explosions[index].vel[edge].y;
            explosions[index].p1[edge].z+=explosions[index].vel[edge].z;

            explosions[index].p2[edge].x+=explosions[index].vel[edge].x;
            explosions[index].p2[edge].y+=explosions[index].vel[edge].y;
            explosions[index].p2[edge].z+=explosions[index].vel[edge].z;
        } // end for edge

        // test for terminatation of explosion?
        if (++explosions[index].counter > 100)
            explosions[index].state = explosions[index].counter = 0;            

    } // end for index

} // end Process_Explosions

///////////////////////////////////////////////////////////

void Draw_Explosions(void)
{
    // this draws all the explosions

    // loop thro all the explosions and render them
    for (int index=0; index<NUM_EXPLOSIONS; index++)
    {
        // test if this explosion is active?
        if (explosions[index].state==0)
            continue;

        // render this explosion
        // each explosion is made of a number of edges
        for (int edge=0; edge < NUM_TIE_EDGES; edge++)
        {
            POINT3D p1_per, p2_per; // used to hold perspective endpoints

            // test if edge if beyond near clipping plane
            if (explosions[index].p1[edge].z < NEAR_Z && 
                explosions[index].p2[edge].z < NEAR_Z)
                continue;

            // step 1: perspective transform each end point
            p1_per.x = VIEW_DISTANCE*explosions[index].p1[edge].x/explosions[index].p1[edge].z;
            p1_per.y = VIEW_DISTANCE*explosions[index].p1[edge].y/explosions[index].p1[edge].z;
            p2_per.x = VIEW_DISTANCE*explosions[index].p2[edge].x/explosions[index].p2[edge].z;
            p2_per.y = VIEW_DISTANCE*explosions[index].p2[edge].y/explosions[index].p2[edge].z;

            // step 2: compute screen coords
            int p1_screen_x = WINDOW_WIDTH/2  + p1_per.x;
            int p1_screen_y = WINDOW_HEIGHT/2 - p1_per.y;
            int p2_screen_x = WINDOW_WIDTH/2  + p2_per.x;
            int p2_screen_y = WINDOW_HEIGHT/2 - p2_per.y;

            // step 3: draw the edge
            Draw_Clip_Line16(p1_screen_x, p1_screen_y, p2_screen_x, p2_screen_y,
                explosions[index].color,back_buffer, back_lpitch);    

        } // end for edge

    } // end for index

} // end Draw_Explosions

//////////////////////////////////////////////////////////

void Move_Starfield(void)
{
    // move the stars

    int index; // looping var

    // the stars are technically stationary,but we are going
    // to move them to simulate motion of the viewpoint
    for (index=0; index<NUM_STARS; index++)
    {
        // move the next star
        stars[index].z-=player_z_vel;

        // test for past near clipping plane
        if (stars[index].z <= NEAR_Z)
            stars[index].z = FAR_Z;

    } // end for index

} // end Move_Starfield

/////////////////////////////////////////////////////////

void Draw_Starfield(void)
{
    // draw the stars in 3D using perspective transform

    int index; // looping var

    for (index=0; index<NUM_STARS; index++)
    {
        // draw the next star
        // step 1: perspective transform
        float x_per = VIEW_DISTANCE*stars[index].x/stars[index].z;
        float y_per = VIEW_DISTANCE*stars[index].y/stars[index].z;

        // step 2: compute screen coords
        int x_screen = WINDOW_WIDTH/2  + x_per;
        int y_screen = WINDOW_HEIGHT/2 - y_per;

        // clip to screen coords
        if (x_screen>=WINDOW_WIDTH || x_screen < 0 || 
            y_screen >= WINDOW_HEIGHT || y_screen < 0)
        {
            // continue to next star
            continue; 
        } // end if
        else
        {
            // else render to buffer
            ((USHORT *)back_buffer)[x_screen + y_screen*(back_lpitch >> 1)] 
            = stars[index].color;
        } // end else

    } // end for index

}  // Draw_Starfield

/////////////////////////////////////////////////////////

void Init_Tie(int index)
{
    // this function starts a tie fighter up at the far end
    // of the universe and sends it our way!

    // position each tie in the viewing volume
    ties[index].x = -WINDOW_WIDTH  + rand()%(2*WINDOW_WIDTH);
    ties[index].y = -WINDOW_HEIGHT + rand()%(2*WINDOW_HEIGHT);
    ties[index].z =  4*FAR_Z;

    // initialize velocity of tie fighter
    ties[index].xv = -4+rand()%8;
    ties[index].yv = -4+rand()%8;
    ties[index].zv = -4-rand()%64;

    // turn the tie fighter on
    ties[index].state = 1;
} // end Init_Tie

/////////////////////////////////////////////////////////

void Process_Ties(void)
{
    // process the tie fighters and do AI (what there is of it!)
    int index; // looping var

    // move each tie fighter toward the viewpoint
    for (index=0; index<NUM_TIES; index++)
    {
        // is this one dead?
        if (ties[index].state==0)
            continue;

        // move the next star
        ties[index].z+=ties[index].zv;
        ties[index].x+=ties[index].xv;
        ties[index].y+=ties[index].yv;

        // test for past near clipping plane
        if (ties[index].z <= NEAR_Z)
        {
            // reset this tie
            Init_Tie(index);

            // another got away
            misses++;

        } // reset tie

    } // end for index

}  // Process_Ties

//////////////////////////////////////////////////////////

void Draw_Ties(void)
{
    // draw the tie fighters in 3D wireframe with perspective

    int index; // looping var

    // used to compute the bounding box of tie fighter
    // for collision detection
    int bmin_x, bmin_y, bmax_x, bmax_y; 

    // draw each tie fighter
    for (index=0; index < NUM_TIES; index++)
    {
        // draw the next tie fighter

        // is this one dead?
        if (ties[index].state==0)
            continue;

        // reset the bounding box to impossible values
        bmin_x =  100000;
        bmax_x = -100000;
        bmin_y =  100000;
        bmax_y = -100000;

        // based on z-distance shade tie fighter
        // normalize the distance from 0 to max_z then
        // scale it to 255, so the closer the brighter
        USHORT rgb_tie_color = RGB16Bit(0,(255-255*(ties[index].z/(4*FAR_Z))),0);

        // each tie fighter is made of a number of edges
        for (int edge=0; edge < NUM_TIE_EDGES; edge++)
        {
            POINT3D p1_per, p2_per; // used to hold perspective endpoints

            // step 1: perspective transform each end point
            // note the translation of each point to the position of the tie fighter
            // that is the model is relative to the position of each tie fighter -- IMPORTANT
            p1_per.x = 
                VIEW_DISTANCE*(ties[index].x+tie_vlist[tie_shape[edge].v1].x)/
                (tie_vlist[tie_shape[edge].v1].z+ties[index].z);

            p1_per.y = VIEW_DISTANCE*(ties[index].y+tie_vlist[tie_shape[edge].v1].y)/
                (tie_vlist[tie_shape[edge].v1].z+ties[index].z);

            p2_per.x = VIEW_DISTANCE*(ties[index].x+tie_vlist[tie_shape[edge].v2].x)/
                (tie_vlist[tie_shape[edge].v2].z+ties[index].z);

            p2_per.y = VIEW_DISTANCE*(ties[index].y+tie_vlist[tie_shape[edge].v2].y)/
                (tie_vlist[tie_shape[edge].v2].z+ties[index].z);

            // step 2: compute screen coords
            int p1_screen_x = WINDOW_WIDTH/2  + p1_per.x;
            int p1_screen_y = WINDOW_HEIGHT/2 - p1_per.y;
            int p2_screen_x = WINDOW_WIDTH/2  + p2_per.x;
            int p2_screen_y = WINDOW_HEIGHT/2 - p2_per.y;

            // step 3: draw the edge
            Draw_Clip_Line16(p1_screen_x, p1_screen_y, p2_screen_x, p2_screen_y,
                rgb_tie_color,back_buffer, back_lpitch);

            // update bounding box with next edge
            int min_x = min(p1_screen_x, p2_screen_x);
            int max_x = max(p1_screen_x, p2_screen_x);

            int min_y = min(p1_screen_y, p2_screen_y);
            int max_y = max(p1_screen_y, p2_screen_y);

            bmin_x = min(bmin_x, min_x);
            bmin_y = min(bmin_y, min_y);

            bmax_x = max(bmax_x, max_x);
            bmax_y = max(bmax_y, max_y);

        } // end for edge

        // test if this guy has been hit by lasers???
        if (cannon_state==1)
        {
            // simple test of screen coords of bounding box contain laser target
            if (target_x_screen > bmin_x && target_x_screen < bmax_x &&
                target_y_screen > bmin_y && target_y_screen < bmax_y)
            {
                // this tie is dead meat!
                Start_Explosion(index);

                // start sound
                DSound_Play(explosion_id );

                // increase score
                score+=ties[index].z;

                // add one more hit
                hits++;

                // finally reset this tie figher
                Init_Tie(index);

            } // end if

        } // end if

    } // end for index

} // end Draw_Ties

//////////////////////////////////////////////////////////

int Game_Main(void *parms)
{
    // this is the workhorse of your game it will be called
    // continuously in real-time this is like main() in C
    // all the calls for your game go here!

    int        index = 0;       // looping var

    // start the timing clock
    Start_Clock();

    // clear the drawing surface 
    DDraw_Fill_Surface(lpddsback, 0);

    // read keyboard and other devices here
    DInput_Read_Keyboard();

    // game logic here...

    if (game_state==GAME_RUNNING)
    {
        // move players crosshair
        if (keyboard_state[DIK_RIGHT])
        {
            // move cross hair to right
            cross_x+=CROSS_VEL;

            // test for wraparound
            if (cross_x > WINDOW_WIDTH/2)
                cross_x = -WINDOW_WIDTH/2;   

        } // end if
        if (keyboard_state[DIK_LEFT])
        {
            // move cross hair to left
            cross_x-=CROSS_VEL;

            // test for wraparound
            if (cross_x < -WINDOW_WIDTH/2)
                cross_x = WINDOW_WIDTH/2;   
        } // end if
        if (keyboard_state[DIK_DOWN])
        {
            // move cross hair up
            cross_y-=CROSS_VEL;

            // test for wraparound
            if (cross_y < -WINDOW_HEIGHT/2)
                cross_y = WINDOW_HEIGHT/2;   
        } // end if
        if (keyboard_state[DIK_UP])
        {
            // move cross hair up
            cross_y+=CROSS_VEL;

            // test for wraparound
            if (cross_y > WINDOW_HEIGHT/2)
                cross_y = -WINDOW_HEIGHT/2;   
        } // end if

        // speed of ship controls
        if (keyboard_state[DIK_A])
            player_z_vel++;
        else
            if (keyboard_state[DIK_S])
                player_z_vel--;

        // test if player is firing laser cannon
        if (keyboard_state[DIK_SPACE] && cannon_state==0)
        {
            // fire the cannon
            cannon_state = 1;
            cannon_count = 0;

            // save last position of targeter
            target_x_screen = cross_x_screen;
            target_y_screen = cross_y_screen;

            // make sound
            DSound_Play(laser_id);

        } // end if   

    } // end if game running

    // process cannon, simple FSM ready->firing->cool

    // firing phase
    if (cannon_state == 1)
        if (++cannon_count > 15)
            cannon_state = 2;

    // cool down phase
    if (cannon_state == 2)
        if (++cannon_count > 20)
            cannon_state = 0;

    // move the starfield
    Move_Starfield();

    // move and perform ai for ties
    Process_Ties();

    // Process the explosions
    Process_Explosions();

    // lock the back buffer and obtain pointer and width
    DDraw_Lock_Back_Surface();

    // draw the starfield
    Draw_Starfield();

    // draw the tie fighters
    Draw_Ties();

    // draw the explosions
    Draw_Explosions();

    // draw the crosshairs 

    // first compute screen coords of crosshair
    // note inversion of y-axis
    cross_x_screen = WINDOW_WIDTH/2  + cross_x;
    cross_y_screen = WINDOW_HEIGHT/2 - cross_y;

    // draw the crosshair in screen coords
    Draw_Clip_Line16(cross_x_screen-16,cross_y_screen,
        cross_x_screen+16,cross_y_screen,
        rgb_red,back_buffer,back_lpitch);

    Draw_Clip_Line16(cross_x_screen,cross_y_screen-16,
        cross_x_screen,cross_y_screen+16,
        rgb_red,back_buffer,back_lpitch);

    Draw_Clip_Line16(cross_x_screen-16,cross_y_screen-4,
        cross_x_screen-16,cross_y_screen+4,
        rgb_red,back_buffer,back_lpitch);

    Draw_Clip_Line16(cross_x_screen+16,cross_y_screen-4,
        cross_x_screen+16,cross_y_screen+4,
        rgb_red,back_buffer,back_lpitch);

    // draw the laser beams
    if (cannon_state == 1)
    {
        if ((rand()%2 == 1))
        { 
            // right beam
            Draw_Clip_Line16(WINDOW_WIDTH-1, WINDOW_HEIGHT-1,
                -4+rand()%8+target_x_screen,-4+rand()%8+target_y_screen,
                RGB16Bit(0,0,rand()),back_buffer,back_lpitch);
        } // end if    
        else
        {
            // left beam
            Draw_Clip_Line16(0, WINDOW_HEIGHT-1,
                -4+rand()%8+target_x_screen,-4+rand()%8+target_y_screen,
                RGB16Bit(0,0,rand()),back_buffer,back_lpitch);
        } // end if

    } // end if

    // done rendering, unlock back buffer surface
    DDraw_Unlock_Back_Surface();

    // draw the informtion
    sprintf(buffer, "Score %d     Kills %d      Escaped %d", score, hits, misses);
    Draw_Text_GDI(buffer, 0,0,RGB(0,255,0), lpddsback);

    if (game_state==GAME_OVER)
        Draw_Text_GDI("G A M E  O V E R", WINDOW_WIDTH/2-8*10,WINDOW_HEIGHT/2,RGB(255,255,255), lpddsback);

    // check if the music has finished, if so restart
    if (DMusic_Status_MIDI(main_track_id)==MIDI_STOPPED)
        DMusic_Play(main_track_id);

    // flip the surfaces
    DDraw_Flip();

    // sync to 30ish fps
    Wait_Clock(30);

    // check for game state switch
    if (misses > 4*NUM_TIES)
        game_state = GAME_OVER;

    // check of user is trying to exit
    if (KEY_DOWN(VK_ESCAPE) || keyboard_state[DIK_ESCAPE])
    {
        PostMessage(main_window_handle, WM_DESTROY,0,0);

    } // end if

    // return success
    return(1);

} // end Game_Main

//////////////////////////////////////////////////////////
