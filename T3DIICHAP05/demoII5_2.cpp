// DEMOII5_2.CPP - 3D plane-point solver

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
VECTOR3D plane_n; // plane normal
POINT3D  plane_p; // point on plane
PLANE3D plane;    // the plane

printf("\n3D Plane Half-Space Tester\n");

// get the normal to the plane
printf("\nEnter normal to plane: nx, ny, nz?");
scanf("%f, %f, %f", &plane_n.x, &plane_n.y, &plane_n.z);

// get a point on the plane
printf("\nEnter a point on the plane: x,y,z?");
scanf("%f, %f, %f", &plane_p.x, &plane_p.y, &plane_p.z);

// create the plane from the point and normal
PLANE3D_Init(&plane, &plane_p, &plane_n, TRUE);

// test point
POINT3D p_test;

while(1) {
printf("\nEnter a point to test for which half-space: x,y,z?");
scanf("%f, %f, %f", &p_test.x, &p_test.y, &p_test.z);

// test the point
float hs = Compute_Point_In_Plane3D(&p_test, &plane);

if (hs > 0.0)
   printf("\nhalf space = %f, thus, point is on positive half-space",hs);
else
if (hs < 0.0)
   printf("\nhalf space = %f, thus, point is on negative half-space",hs);
else
if (hs==0.0)
   printf("\nhalf space = %f, thus, point is in plane",hs);

} // end while


} // end main
