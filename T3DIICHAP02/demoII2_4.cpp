// DEMOII2_4.CPP - A complete windows program with a slighty
// different main event loop

// INCLUDES ///////////////////////////////////////////////
#define WIN32_LEAN_AND_MEAN  // just say no to MFC

#include <windows.h>   // include all the windows headers
#include <windowsx.h>  // include useful macros
#include <stdio.h>     
#include <math.h>

// DEFINES ////////////////////////////////////////////////

// defines for windows 
#define WINDOW_CLASS_NAME "WINCLASS1"

// GLOBALS ////////////////////////////////////////////////


// FUNCTIONS //////////////////////////////////////////////
LRESULT CALLBACK WindowProc(HWND hwnd, 
						    UINT msg, 
                            WPARAM wparam, 
                            LPARAM lparam)
{
// this is the main message handler of the system
PAINTSTRUCT		ps;		// used in WM_PAINT
HDC				hdc;	// handle to a device context

// what is the message 
switch(msg)
	{	
	case WM_CREATE: 
        {
		// do initialization stuff here

        // return success
		return(0);
		} break;

	case WM_PAINT: 
		{
		// simply validate the window
		hdc = BeginPaint(hwnd,&ps);	 
		// you would do all your painting here
        EndPaint(hwnd,&ps);

        // return success
		return(0);
   		} break;

	case WM_DESTROY: 
		{
		// kill the application, this sends a WM_QUIT message 
		PostQuitMessage(0);

        // return success
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

WNDCLASSEX winclass; // this will hold the class we create
HWND	   hwnd;	 // generic window handle
MSG		   msg;		 // generic message

// first fill in the window class stucture
winclass.cbSize         = sizeof(WNDCLASSEX);
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
winclass.hIconSm        = LoadIcon(NULL, IDI_APPLICATION);

// register the window class
if (!RegisterClassEx(&winclass))
	return(0);

// create the window
if (!(hwnd = CreateWindowEx(NULL,                  // extended style
                            WINDOW_CLASS_NAME,     // class
						    "Your Basic Window++", // title
						    WS_OVERLAPPEDWINDOW | WS_VISIBLE,
					 	    0,0,	    // initial x,y
						    400,400,  // initial width, height
						    NULL,	    // handle to parent 
						    NULL,	    // handle to menu
						    hinstance,// instance of this application
						    NULL)))	// extra creation parms
return(0);

// enter main event loop, but this time we use PeekMessage()
// instead of GetMessage() to retrieve messages
while(TRUE)
	{
    // test if there is a message in queue, if so get it
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
	// Game_Main(); // or whatever your loop is called
	} // end while

// return to Windows like this
return(msg.wParam);

} // end WinMain

///////////////////////////////////////////////////////////

