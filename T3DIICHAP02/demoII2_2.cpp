// DEMOII2_2.CPP - a simple message box
#define WIN32_LEAN_AND_MEAN 

#include <windows.h>        // the main windows headers 
#include <windowsx.h>       // a lot of cool macros

// main entry point for all windows programs
int WINAPI WinMain(HINSTANCE hinstance,
			       HINSTANCE hprevinstance,
			       LPSTR lpcmdline,
			       int ncmdshow)
{
// call message box api with NULL for parent window handle
MessageBox(NULL, "THERE CAN BE ONLY ONE!!!",
                 "MY FIRST WINDOWS PROGRAM",
                  MB_OK | MB_ICONEXCLAMATION);

// exit program
return(0);

} // end WinMain
