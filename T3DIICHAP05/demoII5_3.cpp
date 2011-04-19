// DEMOII5_3.CPP - 3D plane-parametric line intersector

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

printf("\n3D Plane-Parametric Line Intersector\n");

// get the normal to the plane
printf("\nEnter normal to plane: nx, ny, nz?");
scanf("%f, %f, %f", &plane_n.x, &plane_n.y, &plane_n.z);

// get a point on the plane
printf("\nEnter a point on the plane: x,y,z?");
scanf("%f, %f, %f", &plane_p.x, &plane_p.y, &plane_p.z);

// create the plane from the point and normal
PLANE3D_Init(&plane, &plane_p, &plane_n, TRUE);

POINT3D p1, p2;  // our endpoints
PARMLINE3D pl1; // our line

while(1) {

printf("\nEnter coords parametric line from p1 to p2 in form: x1,y1,z1, x2,y2,z2?");
scanf("%f, %f, %f, %f, %f, %f",&p1.x, &p1.y, &p1.z, &p2.x, &p2.y, &p2.z);

// create a parametric line from p1 to p2
Init_Parm_Line3D(&p1, &p2, &pl1);

float t;    // intersection parameter
POINT3D pt; // intersection point

// compute intersection
int intersection_type = Intersect_Parm_Line3D_Plane3D(&pl1, &plane, &t, &pt);

// based on type of intersection display results
switch(intersection_type)
      {
      case PARM_LINE_NO_INTERSECT:
           {
           printf("\nThe plane and line does not intersect!");
           } break;

      case PARM_LINE_INTERSECT_IN_SEGMENT:
           {
           POINT3D pt;
           Compute_Parm_Line3D(&pl1, t, &pt);
           printf("\nThe line and plane intersects when t=%f at (%f, %f, %f)",t,pt.x, pt.y, pt.z);
           } break;

      case PARM_LINE_INTERSECT_OUT_SEGMENT:
           {
           POINT3D pt;
           Compute_Parm_Line3D(&pl1, t, &pt);
           printf("\nThe line and plane intersects when t=%f at (%f, %f, %f)",t,pt.x, pt.y, pt.z);
           printf("\nNote that the intersection occurs out of range of the line segment");
           } break;

      default: break;
 
      } // end switch





} // end while


} // end main
