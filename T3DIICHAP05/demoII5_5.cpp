// DEMOII5_5.CPP - Fixed point example

// INCLUDES ///////////////////////////////////////////////////

#define WIN32_LEAN_AND_MEAN  

#ifndef INITGUID
#define INITGUID       // you need this or DXGUID.LIB
#endif

#include <windows.h>   // include important windows stuff
#include <windowsx.h> 
#include <mmsystem.h>
#include <objbase.h>
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
#include <direct.h>
#include <wchar.h>

#include <ddraw.h>      // needed for defs in T3DLIB1.H 
#include "T3DLIB1.H"    // T3DLIB4 is based on some defs in this 
#include "T3DLIB4.H"

// DEFINES ////////////////////////////////////////////////////



// TYPES //////////////////////////////////////////////////////


// CLASSES ////////////////////////////////////////////////////

// GLOBALS ////////////////////////////////////////////////////

// define vars expected by graphics engine to compile
HWND main_window_handle;

// FUNCTIONS //////////////////////////////////////////////////
void main()
{
FIXP16 fp1=0, fp2=0, fp3=0; // working vars
float f;              // used to input floating point values
int done=0; // exit flag
int sel;    // user input 

// open the error system, so we can see output
// notice the last parameter "stdout" this tell the error
// system that we don't want to write errors to a text file
// but straight out to the screen!
Open_Error_File("", stdout);


// print titles
printf("\nFixed Point Lab\n");
printf("\nAllows you to enter in two fixed point numbers: \nfp1, fp2 and work with them\n");

while(!done) 
{
printf("\nFixed Point Menu\n");
printf("\n(1) Enter in floating point number and store in fp1.");
printf("\n(2) Enter in floating point number and store in fp2.");
printf("\n(3) Print fp1.");
printf("\n(4) Print fp2.");
printf("\n(5) Print fp3.");
printf("\n(6) Add fp1+fp2, store in fp3.");
printf("\n(7) Subtract fp1-fp2, store in fp3.");
printf("\n(8) Multiply fp1*fp2, store in fp3.");
printf("\n(9) Divide fp1/fp2, store in fp3.");
printf("\n(10) Exit.");
printf("\nSelect a function and press enter?");
scanf("%d", &sel);

// what's the selection?
switch(sel)
    {
    case 1: // printf("\n(1) Enter in floating point number and store in fp1.");
    {
    printf("\nEnter in floating point value and it will be converted to fixed point and stored in fp1?");
    scanf("%f", &f);
    fp1 = FLOAT_TO_FIXP16(f);

    } break;

    case 2: // printf("\n(2) Enter in floating point number and store in fp2.");
    {
    printf("\nEnter in floating point value and it will be converted to fixed point and stored in fp2?");
    scanf("%f", &f);
    fp2 = FLOAT_TO_FIXP16(f);
    } break;    

    case 3: // printf("\n(3) Print fp1.");
    {
    FIXP16_Print(fp1);
    } break;

    case 4: // printf("\n(4) Print fp2.");
    {
    FIXP16_Print(fp2);
    } break;

    case 5: // printf("\n(5) Print fp3.");
    {
    FIXP16_Print(fp3);
    } break;

    case 6: // printf("\n(6) Add fp1+fp2, store in fp3.");
    {
    // just add them
    fp3=fp1+fp2;
    printf("\nfp1+fp2=");
    FIXP16_Print(fp3);
    } break;

    case 7: // printf("\n(7) Subtract fp1-fp2, store in fp3.");
    {
    // just sutract them
    fp3=fp1-fp2;
    printf("\nfp1-fp2=");
    FIXP16_Print(fp3);
    } break;

    case 8: // printf("\n(8) Multiply fp1*fp2, store in fp3.");
    {
    // call asm function to multiply
    fp3 = FIXP16_MUL(fp1, fp2);
    printf("\nfp1*fp2=");
    FIXP16_Print(fp3);
    } break;

    case 9: // printf("\n(9) Divide fp1/fp2, store in fp3.");
    {
    // call asm function to divide
    fp3 = FIXP16_DIV(fp1, fp2);
    printf("\nfp1/fp2=");
    FIXP16_Print(fp3);
    } break;

    case 10: // printf("\n(10) Exit.");
    {
    done=1;
    } break;

    default: break;

    } // end switch

printf("\nPress spacebar to continue");
getch();

} // end while

// close error
Close_Error_File();

} // end main
