// DEMOII5_1.CPP - 2D parametric line intersector

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
printf("\n2D Parametric Line Intersector\n");

POINT2D p1, p2, p3, p4;  // our endpoints
PARMLINE2D pl1, pl2; // our line

while(1) {

printf("\nEnter coords parametric line from p1 to p2 in form: x1,y1, x2,y2?");
scanf("%f, %f, %f, %f",&p1.x, &p1.y, &p2.x, &p2.y);

printf("\nEnter coords parametric line from p3 to p4 in form: x3,y3, x4,y4?");
scanf("%f, %f, %f, %f",&p3.x, &p3.y, &p4.x, &p4.y);

// create a parametric line from p1 to p2
Init_Parm_Line2D(&p1, &p2, &pl1);

// create a parametric line from p3 to p4
Init_Parm_Line2D(&p3, &p4, &pl2);

float t1=0, t2=0; // storage for parameters

// compute intersection
int intersection_type = Intersect_Parm_Lines2D(&pl1, &pl2, &t1, &t2);

// based on type of intersection display results
switch(intersection_type)
      {
      case PARM_LINE_NO_INTERSECT:
           {
           printf("\nThe lines do not intersect!");
           } break;

      case PARM_LINE_INTERSECT_IN_SEGMENT:
           {
           POINT2D pt;
           Compute_Parm_Line2D(&pl1, t1, &pt);
           printf("\nThe lines intersect when t1=%f, t2=%f at (%f, %f)",t1, t2, pt.x, pt.y);
           } break;

      case PARM_LINE_INTERSECT_OUT_SEGMENT:
           {
           POINT2D pt;
           Compute_Parm_Line2D(&pl1, t1, &pt);
           printf("\nThe lines intersect when t1=%f, t2=%f at (%f, %f)",t1, t2, pt.x, pt.y);
           printf("\nNote that the intersection occurs out of range of the line segment(s)");
           } break;

      default: break;
 
      } // end switch


} // end while


} // end main
