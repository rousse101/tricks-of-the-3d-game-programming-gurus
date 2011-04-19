// DEMOII5_4.CPP - Quaternion lab

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
QUAT     q1={0,0,0,0}, 
         q1tmp={0,0,0,}, 
         q2={0,0,0,0}, 
         q2tmp={0,0,0,}, 
         qtmp={0,0,0,},  
         qv={0,0,0,}; // our working quaterions
VECTOR3D v={0,0,0}, vtmp;   // working vectors
POINT3D  p={0,0,0}, ptmp;   // working points
float theta;                // general angle

// open the error system, so we can see output
// notice the last parameter "stdout" this tell the error
// system that we don't want to write errors to a text file
// but straight out to the screen!
Open_Error_File("", stdout);


printf("\nQuaternion Lab");
printf("\nNote: the lab uses a number of variables to perform various operations");
printf("\nThey are quaterions q1,q2, and qtmp, along with the vector v.");
printf("\nMany operations will be stored in the temporary quaternion qtmp\n");

int sel;    // user selection
int done=0; // exit flag


// main loop
while(!done)
{
// draw menu
printf("\nQuaternion Lab Menu - (c) Alien Programmers Inc. - We're ready to invade you!");
printf("\n(1) Enter q1 in general format:  w1,x1,y1,z1?");
printf("\n(2) Enter in rotation quaterion and store in q1");
printf("\n(3) Enter q2 in general format:  w1,x1,y1,z1?");
printf("\n(4) Enter vector v in format:  x,y,z?");
printf("\n(5) Print q1");
printf("\n(6) Print q2");
printf("\n(7) Print qtmp");
printf("\n(8) Print v");
printf("\n(9) Add q1+q2, note result is stored in qtmp");
printf("\n(10) Subtract q1-q2, note result is stored in qtmp");
printf("\n(11) Normalize q1 (note alters q1)");
printf("\n(12) Normalize q2 (note alters q1)");
printf("\n(13) Compute conjugate of q1, note result is stored in qtmp");
printf("\n(14) Compute conjugate of q2, note result is stored in qtmp");
printf("\n(15) Compute inverse of q1, note result is stored in qtmp");
printf("\n(16) Compute inverse of q2, note result is stored in qtmp");
printf("\n(17) Multiply q1*q2, note result is stored in qtmp");
printf("\n(18) Multiply q2*q1, note result is stored in qtmp");
printf("\n(19) Compute triple product q1*vector*q1(-1) and store in qtmp, ");
printf("\nthis will rotate v assuming q1 is a unit rotation quaterion");
printf("\n(20) Clear All");
printf("\n(21) Exit");
printf("\nMake Selection and Press Enter?");
scanf("%d",&sel);


// what to do?
switch(sel)
      {
        case 1: // printf("\n(1) Enter q1 in general format:  w1,x1,y1,z1?");
        {
        printf("\nEnter q1 in general format:  w1,x1,y1,z1?");
        scanf("%f, %f, %f, %f", &q1.w, &q1.x, &q1.y, &q1.z);
    
        } break;

        case 2: // printf("\n(2) Enter in rotation quaterion and store in q1");
        {
        printf("\nEnter in direction vector v you wish to rotate about in form: x,y,z");
        printf("\n(system will normalize, so it need not have unit length)?");
        scanf("%f, %f, %f", &vtmp.x, &vtmp.y, &vtmp.z);

        printf("\nEnter in angle of rotation in degrees (0-360)?");
        scanf("%f",&theta);

        printf("\nGenerating rotation quaterion %f degree around vector (%f, %f, %f)...", 
                theta, vtmp.x, vtmp.y, vtmp.z);

        // normalize v
        VECTOR3D_Normalize(&vtmp);
        
        // compute quaterion
        VECTOR3D_Theta_To_QUAT(&q1, &vtmp, DEG_TO_RAD(theta));         

        QUAT_Print(&q1,"q1 in rotation format");

        } break;

        case 3: // printf("\n(3) Enter q2 in general format:  w1,x1,y1,z1?");
        {
        printf("\nEnter q2 in general format:  w1,x1,y1,z1?");
        scanf("%f, %f, %f, %f", &q2.w, &q2.x, &q2.y, &q2.z);
        } break;

        case 4: // printf("\n(4) Enter vector v in format:  x,y,z?");
        {
        printf("\nEnter vector v in format:  x,y,z?");
        scanf("%f, %f, %f", &v.x, &v.y, &v.z);
        } break;

        case 5: // printf("\n(5) Print q1");
        {
        QUAT_Print(&q1,"q1");
        } break;

        case 6: // printf("\n(6) Print q2");
        {
        QUAT_Print(&q2,"q2");
        } break;

        case 7: // printf("\n(7) Print qtmp");
        {
        QUAT_Print(&qtmp,"qtmp");
        } break;

        case 8: // printf("\n(8) Print v");
        {
        VECTOR3D_Print(&v,"v");
        } break;

        case 9: // printf("\n(9) Add q1+q2, note result is stored in qtmp");
        {
        QUAT_Add(&q1, &q2, &qtmp);
        QUAT_Print(&qtmp,"qtmp=q1+q2");
        } break;

        case 10: // printf("\n(10) Subtract q1-q2, note result is stored in qtmp");
        {
        QUAT_Sub(&q1, &q2, &qtmp);
        QUAT_Print(&qtmp,"qtmp=q1-q2");
        } break;

        case 11: // printf("\n(11) Normalize q1 (note alters q1)");
        {
        QUAT_Normalize(&q1);
        QUAT_Print(&q1,"q1");
        } break;

        case 12: // printf("\n(12) Normalize q2 (note alters q1)");
        {
        QUAT_Normalize(&q1);
        QUAT_Print(&q1,"q1");
        } break;

        case 13: // printf("\n(13) Compute conjugate of q1, note result is stored in qtmp");
        {
        QUAT_Conjugate(&q1, &qtmp);
        QUAT_Print(&qtmp,"qtmp = q1*");
        } break;

        case 14: // printf("\n(14) Compute conjugate of q2, note result is stored in qtmp");
        {
        QUAT_Conjugate(&q2, &qtmp);
        QUAT_Print(&qtmp,"qtmp = q2*");
        } break;

        case 15: // printf("\n(15) Compute inverse of q1, note result is stored in qtmp");
        {
        QUAT_Inverse(&q1, &qtmp);
        QUAT_Print(&qtmp,"qtmp = q1(-1)");
        } break;

        case 16: // printf("\n(16) Compute inverse of q2, note result is stored in qtmp");
        {
        QUAT_Inverse(&q2, &qtmp);
        QUAT_Print(&qtmp,"qtmp = q2(-1)");
        } break;

        case 17: // printf("\n(17) Multiply q1*q2, note result is stored in qtmp");
        {
        QUAT_Mul(&q1, &q2, &qtmp);
        QUAT_Print(&qtmp,"qtmp=q1 * q2");
        } break;

        case 18: // printf("\n(18) Multiply q2*q1, note result is stored in qtmp");
        {
        QUAT_Mul(&q2, &q1, &qtmp);
        QUAT_Print(&qtmp,"qtmp=q2 * q1");
        } break;

        case 19: // printf("\n(19) Compute triple product q1*vector*q1(-1) and store in qtmp, 
                 // this will rotate v assuming q1 is a unit rotation quaterion");
        {
        printf("\nComputing triple product...note q1 will be normalized if length <> 1");
        QUAT_Normalize(&q1);
        printf("\nAssuming q1 represents rotation quaternion, value is:");
        QUAT_Print(&q1,"q1");
        
        printf("\nVector v represent point or vector to be rotated, value is:");
        VECTOR3D_Print(&v,"v");
        printf("\nv after conversion to quaternion format is:");
 
        // vector must be converted to a quaternion
        QUAT_INIT_VECTOR3D(&qv, &v);
        QUAT_Print(&qv,"qv"); 

        // now perform triple product, but conjugate = inverse for unit quaterions
        // so all we need for inverse is to compute conjugate of q1
        QUAT_Conjugate(&q1, &q1tmp);

        // now perform triple multiply q1 * v(in q format) * q1(-1)
        // this will rotate v
        QUAT_Triple_Product(&q1, &qv, &q1tmp, &qtmp);
  
        printf("\nResulting quaternion after multiplication is v rotated which equals:");
        QUAT_Print(&qtmp,"v rotated in quaternion format");

        } break;
        case 20: // printf("\n(20) Clear All");
        {
        // simply clear all values
        VECTOR3D_ZERO(&v);
        QUAT_ZERO(&q1);
        QUAT_ZERO(&q2);
        QUAT_ZERO(&qtmp);
        } break;
        case 21: // printf("\n(21) Exit");
        {
        // set exit flag
        done=1;
        } break;

      default: break;

      } // end switch

printf("\nPress Spacebar to continue...");
getch();

} // end while

// close error system
Close_Error_File();

} // end main
