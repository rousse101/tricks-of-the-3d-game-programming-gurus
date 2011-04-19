// DEMOII16_3.CPP - SIMD demo, contains class and demo code

// I N C L U D E S ///////////////////////////////////////////////////////////

#define INITGUID

#define WIN32_LEAN_AND_MEAN  

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
#include <limits.h>
#include <float.h>
#include <xmmintrin.h>
#include <emmintrin.h>
#include <fvec.h>

// DEFINES ////////////////////////////////////////////////////////////////////

// below is a simple macro to help build up the shuffle control word used by the 
// shuffle class instructions shufps*
// basically there are 4 bit fields each field encodes the source double
// word to move from source to destination, and destination to destination
// it's all very confusing, but in essence when the shufps instruction is used:
//
// shufps xmm_dest, xmm_src, control_flags
//
// xmm_dest is the destination SIMD register and xmm_src is the source SIMD
// register. The tricky part is how the instructions works (refer to intel
// manual for more detail), basically the instruction takes 2 of the 4 words
// from the source register and places them in the destination register hi words
// and also takes 2 of the 4 words from the destination register and places
// them in the low words of the destination register! So the destination is used
// as a "source" and a destination -- but just think of the source and destination
// registers as both being sources and the final result just happens to be STORED
// in one of the source registers at the END of the process
// in any case, the bit encoding to select a particular 32-bit word from either the 
// source or destination are:
// 00 - selects word 0
// 01 - selects word 1
// 10 - selects word 2
// 11 - selects word 3  

// the the control_flags has the following encoding:
// (high bit) A1A0 B1B0 C1C0 D1D0 (low bit)
// 
//  bits  operation
//
//  D1D0  Selects the 32-word from the DESTination operand and places it in the (low) word 0 of the result
//  C1C0  Selects the 32-word from the DESTination operand and places it in the word 1 of the result    
//  B1B0  Selects the 32-word from the SOURce operand and places it in the word 2 of the result    
//  A1A0  Selects the 32-word from the SOURce operand and places it in the (hi) word 3 of the result    
// and the parameters for the macro map these as follows:
// 
// srch - A1A0
// srcl - B1B0

// desth - C1C0
// destl - D1D0
// this is all in the intel "IA-32 Intel Architecture Software Developer Manual: Vol II"
#define SIMD_SHUFFLE(srch,srcl,desth,destl) (((srch) << 6) | ((srcl) << 4) \
                                            | ((desth) << 2) | ((destl)))

// enable the final output type by setting one of these to 1 (the rest MUST be 0!)
#define SIMD_INTRISIC      0
#define SIMD_ASM           1

// CLASESS ////////////////////////////////////////////////////////////////////

// this is the new vector class that supports SIMD sse, note the numerous ways we 
// can access the data members, this allows transparent support of assignments,
// data access, and intrinsic library use without lots of casts
class C_VECTOR4D
{
public:

union
    {
    __declspec(align(16)) __m128 v;   // SIMD data type access
    float M[4];                       // array indexed storage
    // explicit names
    struct
         {
         float x,y,z,w;
         }; // end struct
    }; // end union

// note: the declspec is redundant since in the type __m128 forces
// the compiler to align in 16-byte boundaries, so as long as __m128 is 
// part of the union declspec is NOT needed :) But, it can't 
// hurt and when you are defining locals and globals, always put 
// declspec(align(16)) to KNOW data is on 16-byte boundaries

// CONSTRUCTORS //////////////////////////////////////////////////////////////

C_VECTOR4D() 
{
// void constructor
// initialize vector to 0.0.0.1
x=y=z=0; w=1.0;
} // end C_VECTOR4D

//////////////////////////////////////////////////////////////////////////////

C_VECTOR4D(float _x, float _y, float _z, float _w = 1.0) 
{
// initialize vector to sent values
x = _x;
y = _y;
z = _z;
w = _w;
} // end C_VECTOR4D

// FUNCTIONS ////////////////////////////////////////////////////////////////

void init(float _x, float _y, float _z, float _w = 1.0) 
{
// initialize vector to sent values
x = _x;
y = _y;
z = _z;
w = _w;
} // end init

//////////////////////////////////////////////////////////////////////////////

void zero(void)
{
// initialize vector to 0.0.0.1
x=y=z=0; w=1.0;

} // end zero

//////////////////////////////////////////////////////////////////////////////

float length(void)
{
// computes the length of the vector
C_VECTOR4D vr = *this;

// set w=0
vr.w = 0;

// compile pure asm version?
#if (SIMD_ASM==1)

// begin inline asm version of SIMD dot product since we need its 
// results for the length since length = sqrt(v*v)
_asm
   {
   // first we need dot product of this*this
   movaps xmm0, vr.v   // move left operand into xmm0
   mulps xmm0, xmm0    // multiply operands vertically
   
   // at this point, xmm0  = 
   // [ (v1.x * v2.x), (v1.y * v2.y), (v1.z * v2.z), (1*1) ]
   // or more simply: let xmm0 = [x,y,z,1] = 
   // [ (v1.x * v2.x), (v1.y * v2.y), (v1.z * v2.z), (1*1) ]
   // we need to sum the x,y,z components into a single scalar
   // to compute the final dot product of:
   // dp = x + y + z == x1*x2 + y1*y2 + z1*z2

   // begin 
   // xmm0: = [x,y,z,1] (note: all regs in low to hight order)
   // xmm1: = [?,?,?,?]
   movaps xmm1, xmm0 // copy result into xmm1
   // xmm0: = [x,y,z,1]
   // xmm1: = [x,y,z,1]

   shufps xmm1, xmm0, SIMD_SHUFFLE(0x01,0x00,0x03,0x02) 
   // xmm0: = [x,y,z,1]
   // xmm1: = [z,1,x,y]

   addps xmm1, xmm0
   // xmm0: = [x  ,y  ,z  ,1]
   // xmm1: = [x+z,y+1,x+z,y+1]

   shufps xmm0, xmm1, SIMD_SHUFFLE(0x02,0x03,0x00,0x01) 
   // xmm0: = [y  ,x  ,y+1,x+z]
   // xmm1: = [x+z,y+1,x+z,y+1]

   // finally we can add!
   addps xmm0, xmm1
   // xmm0: = [x+y+z,x+y+1,x+y+z+1,x+y+z+1]
   // xmm1: = [x+z  ,y+1  ,x+z    ,y+1]
   // xmm0.x contains the dot product
   // xmm0.z, xmm0.w contains the dot+1

   // now low double word contains dot product, let's take squaroot
   sqrtss xmm0, xmm0
 
   movaps vr, xmm0 // save results

   } // end asm

#endif // end use inline asm version

// compile intrinsic version?
#if (SIMD_INTRISIC==1)

#endif // end use intrinsic library version

// return result
return(vr.x);

} // end length

// OVERLOADED OPERATORS //////////////////////////////////////////////////////

float& operator[](int index)
{
// return the ith element from the array
return(M[index]);
} // end operator[]

//////////////////////////////////////////////////////////////////////////////

C_VECTOR4D operator+(C_VECTOR4D &v)
{
// adds the "this" vector and the sent vector

__declspec(align(16)) C_VECTOR4D vr; // used to hold result, aligned on 16 bytes

// compile pure asm version?
#if (SIMD_ASM==1)

// begin inline asm version of SIMD add
_asm
   {
   mov esi, this       // "this" contains a point to the left operand
   mov edi, v          // v points to the right operand

   movaps xmm0, [esi]  // esi points to first vector, move into xmm0
   addps  xmm0, [edi]  // edi points to second vector, add it to xmm0
    
   movaps vr, xmm0     // move result into output vector

   } // end asm

#endif // end use inline asm version

// compile intrinsic version?
#if (SIMD_INTRISIC==1)

vr.v = _mm_add_ps(this->v, v.v);

#endif // end use intrinsic library version

// always set w=1
vr.w = 1.0;

// return result
return(vr);

} // end operator+

//////////////////////////////////////////////////////////////////////////////

C_VECTOR4D operator-(C_VECTOR4D &v)
{
// subtracts the "this" vector and the sent vector

__declspec(align(16)) C_VECTOR4D vr; // used to hold result, aligned on 16 bytes

// compile pure asm version?
#if (SIMD_ASM==1)

// begin inline asm version of SIMD add
_asm
   {
   mov esi, this       // "this" contains a point to the left operand
   mov edi, v          // v points to the right operand

   movaps xmm0, [esi]  // esi points to first vector, move into xmm0
   subps  xmm0, [edi]  // edi points to second vector, subtract it from xmm0
    
   movaps vr, xmm0     // move result into output vector

   } // end asm

#endif // end use inline asm version

// compile intrinsic version?
#if (SIMD_INTRISIC==1)

vr.v = _mm_sub_ps(this->v, v.v);

#endif // end use intrinsic library version

// always set w=1
vr.w = 1.0;

// return result
return(vr);

} // end operator-

//////////////////////////////////////////////////////////////////////////////

float operator*(C_VECTOR4D &v)
{
// the dot product will be * since dot product is a more common operation
// computes the dot between between the "this" vector and the sent vector

__declspec(align(16)) C_VECTOR4D vr; // used to hold result, aligned on 16 bytes

// compile pure asm version?
#if (SIMD_ASM==1)

// begin inline asm version of SIMD dot product
_asm
   {
   mov esi, this       // "this" contains a point to the left operand
   mov edi, v          // v points to the right operand

   movaps xmm0, [esi]  // move left operand into xmm0
   mulps xmm0,  [edi]  // multiply operands vertically
   
   // at this point, xmm0  = 
   // [ (v1.x * v2.x), (v1.y * v2.y), (v1.z * v2.z), (1*1) ]
   // or more simply: let xmm0 = [x,y,z,1] = 
   // [ (v1.x * v2.x), (v1.y * v2.y), (v1.z * v2.z), (1*1) ]
   // we need to sum the x,y,z components into a single scalar
   // to compute the final dot product of:
   // dp = x + y + z where x = x1*x2, y = y1*y2, z = z1*z2

   // begin 
   // xmm0: = [x,y,z,1] (note: all regs in low to hight order)
   // xmm1: = [?,?,?,?]
   movaps xmm1, xmm0 // copy result into xmm1
   // xmm0: = [x,y,z,1]
   // xmm1: = [x,y,z,1]

   shufps xmm1, xmm0, SIMD_SHUFFLE(0x01,0x00,0x03,0x02) 
   // xmm0: = [x,y,z,1]
   // xmm1: = [z,1,x,y]

   addps xmm1, xmm0
   // xmm0: = [x  ,y  ,z  ,1]
   // xmm1: = [x+z,y+1,x+z,y+1]

   shufps xmm0, xmm1, SIMD_SHUFFLE(0x02,0x03,0x00,0x01) 
   // xmm0: = [y  ,x  ,y+1,x+z]
   // xmm1: = [x+z,y+1,x+z,y+1]

   // finally we can add!
   addps xmm0, xmm1
   // xmm0: = [x+y+z,x+y+1,x+y+z+1,x+y+z+1]
   // xmm1: = [x+z  ,y+1  ,x+z    ,y+1]
   // xmm0.x contains the dot product
   // xmm0.z, xmm0.w contains the dot+1
   movaps vr, xmm0
   } // end asm

#endif // end use inline asm version

// compile intrinsic version?
#if (SIMD_INTRISIC==1)
vr.v = _mm_mul_ps(this->v, v.v);
return(vr.x + vr.y + vr.z);

#endif // end use intrinsic library version

// return result
return(vr.x);

} // end operator*

//////////////////////////////////////////////////////////////////////////////

void print(void)
{
// this member function prints out the vector
printf("\nv = [%f, %f, %f, %f]", this->x, this->y, this->z, this->w);
} // end print

//////////////////////////////////////////////////////////////////////////////

};  // end class C_VECTOR4D

// DEFINES /////////////////////////////////////////////////////////////////


// TYPES ///////////////////////////////////////////////////////////////////

// MACROS //////////////////////////////////////////////////////////////////


// GLOBALS //////////////////////////////////////////////////////////////////

HWND main_window_handle           = NULL; // save the window handle
HINSTANCE main_instance           = NULL; // save the instance
 
// MAIN //////////////////////////////////////////////////////////////////

void main()
{
// create some vectors
C_VECTOR4D v1, v2, v3; // create some vectors

// what kind of SIMD support do we have
if (IsProcessorFeaturePresent(PF_MMX_INSTRUCTIONS_AVAILABLE))
   printf("\nMMX Available.");
else
   printf("\nMMX NOT Available.");

if (IsProcessorFeaturePresent(PF_XMMI_INSTRUCTIONS_AVAILABLE))
   printf("\nXMMI Available.");
else
   printf("\nXMMI NOT Available.");

int sel = 0;

// enter main loop
while(1)
     {
     printf("\n\nSIMD Demo Main Menu\n");
     printf("\n1 - Print out all vectors v1, v2, v3");
     printf("\n2 - Enter values for v1");
     printf("\n3 - Enter values for v2");
     printf("\n4 - Enter values for v3");
     printf("\n5 - Compute v3=v1+v2");
     printf("\n6 - Compute v3=v1-v2");
     printf("\n7 - Compute v1.v2");
     printf("\n8 - Compute Length of v1, v2, v3");
     printf("\n0 - Exit this menu");
     printf("\n\nSelect one?");

     scanf("%d", &sel);

     // get input
     switch(sel)
          {
          case 0: // printf("\n0 - Exit this menu.");
                {
                return;
                } break;
          case 1: // printf("\n1 - Print out all vectors v1, v2, v3.");
                {
                printf("\nv1:");
                v1.print();

                printf("\nv2:");
                v2.print();

                printf("\nv3:");
                v3.print();

                } break;
          case 2: // printf("\n2 - Enter values for v1.");
                {
                printf("\nEnter x,y,z values for v1 seperated by spaces?");
                scanf("%f %f %f", &v1.M[0], &v1.M[1], &v1.M[2]);
                v1.M[3] = 1;

                } break;
          case 3: // printf("\n3 - Enter values for v2.");
                {
                printf("\nEnter x,y,z values for v2 seperated by spaces?");
                scanf("%f %f %f", &v2.M[0], &v2.M[1], &v2.M[2]);
                v2.M[3] = 1;

                } break;
          case 4: // printf("\n4 - Enter values for v3.");
                {
                printf("\nEnter x,y,z values for v3 seperated by spaces?");
                scanf("%f %f %f", &v3.M[0], &v3.M[1], &v3.M[2]);
                v3.M[3] = 1;

                } break;
          case 5: // printf("\n5 - Compute v3=v1+v2");
                {
                v3=v1+v2;
                } break;
          case 6: // printf("\n6 - Compute v3=v1-v2");
                {
                v3=v1-v2;
                } break;
          case 7: // printf("\n7 - Compute v1.v2");
                {
                float dp = v1*v2; 
                printf("\nv1.v2 = %f",dp);
                } break;
          case 8: // printf("\n8 - Compute Length of v1, v2, v3.");
                {
                float l1 = v1.length();
                float l2 = v2.length();
                float l3 = v3.length();
       
                printf("\nlength(v1) = %f", l1);
                printf("\nlength(v2) = %f", l2);
                printf("\nlength(v3) = %f", l3);

                } break;

          default: break;
          } // end switch

     } // end while

} // end main

