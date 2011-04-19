// DEMOII16_2.CPP - Hello world SIMD demo

// I N C L U D E S ///////////////////////////////////////////////////////////

#define WIN32_LEAN_AND_MEAN  
#include <windows.h>   // include important windows stuff
#include <windowsx.h> 
#include <stdio.h>
#include <math.h> 
#include <xmmintrin.h> // <-- this is needed for SIMD support (SSE I)

// MAIN //////////////////////////////////////////////////////////////////

void main()
{
// print out os/processor support 
if (IsProcessorFeaturePresent(PF_MMX_INSTRUCTIONS_AVAILABLE))
   printf("\nMMX Available.\n");
else
   printf("\nMMX NOT Available.\n");

if (IsProcessorFeaturePresent(PF_XMMI_INSTRUCTIONS_AVAILABLE))
   printf("\nXMMI Available.\n");
else
   {
   printf("\nXMMI NOT Available.\n");
   return;
   } // end if

// define 3 SIMD packed values, remember always must be 16byte aligned
__declspec(align(16)) static float x[4] = {1,2,3,4};
__declspec(align(16)) static float y[4] = {5,6,7,8};
__declspec(align(16)) static float z[4] = {0,0,0,0};

__m128 m0, m1, m2;

// lets do a little SIMD to add X and Y and store the results in Z
_asm
   {
   lea esi, x
   movaps xmm0, [esi]  // move the value of x into XMM0
   addps  xmm0, y  // add the value of y to XMM0
   movaps z, xmm0  // store the results from XMM0 into z   
   } // end asm

// print the results
printf("\n x[%f,%f,%f,%f]", x[0], x[1], x[2], x[3]);
printf("\n+y[%f,%f,%f,%f]", y[0], y[1], y[2], y[3]);
printf("\n_______________________________________");
printf("\n=z[%f,%f,%f,%f]\n\n",  z[0], z[1], z[2], z[3]);

} // end main

