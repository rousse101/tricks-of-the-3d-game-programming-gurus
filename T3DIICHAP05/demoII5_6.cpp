// DEMOII5_6.CPP - Linear System Equation solver

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
MATRIX2X2 mA;
MATRIX1X2 mB;
MATRIX1X2 mX; 

// open the error system, so we can see output
// notice the last parameter "stdout" this tell the error
// system that we don't want to write errors to a text file
// but straight out to the screen!
Open_Error_File("", stdout);

while(1) {

printf("\n2D Linear System Solver\n");

// get matrix A
printf("\nEnter the values for the matrix A (2x2)");
printf("\nin row major form m00, m01, m10, m11?");
scanf("%f, %f, %f, %f", &mA.M00, &mA.M01, &mA.M10, &mA.M11);

printf("\nEnter the values for matrix B (2x1)");
printf("\nin column major form m00, m10?");
scanf("%f, %f", &mB.M00, &mB.M01);

// techically you can't multiply a 2x2 * 1x2, 
// but you can multiply is a 2x2 * (1x2)t
// that is the transpose of a 1x2 is a 2x1, 
// bottom line is the B matrix is just 2 numbers!

// now solve the system
if (Solve_2X2_System(&mA, &mX, &mB))
   {
   // now print results
   VECTOR2D_Print((VECTOR2D_PTR)&mX, "Solution matrix mX");
   } // end if
else
    printf("\nNo Solution!");

} // end while

// close the error system
Close_Error_File();


} // end main
