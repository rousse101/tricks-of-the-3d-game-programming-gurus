// T3DLIB4.H - Header file for T3DLIB4.CPP game engine library

// watch for multiple inclusions
#ifndef T3DLIB4
#define T3DLIB4

// DEFINES & CONSTANTS /////////////////////////////////////

// defines for small numbers
#define EPSILON_E3 (float)(1E-3) 
#define EPSILON_E4 (float)(1E-4) 
#define EPSILON_E5 (float)(1E-5)
#define EPSILON_E6 (float)(1E-6)

// defines for parametric line intersections
#define PARM_LINE_NO_INTERSECT          0
#define PARM_LINE_INTERSECT_IN_SEGMENT  1
#define PARM_LINE_INTERSECT_OUT_SEGMENT 2
#define PARM_LINE_INTERSECT_EVERYWHERE  3

// TYPES //////////////////////////////////////////////////

// 2x2 matrix /////////////////////////////////////////////
typedef struct MATRIX2X2_TYP
{
    union
    {
        float M[2][2]; // array indexed data storage

        // storage in row major form with explicit names
        struct
        {
            float M00, M01;
            float M10, M11;
        }; // end explicit names

    }; // end union
} MATRIX2X2, *MATRIX2X2_PTR;

// 1x4 matrix /////////////////////////////////////////////
typedef struct MATRIX1X4_TYP
{
    union
    {
        float M[4]; // array indexed data storage

        // storage in row major form with explicit names
        struct
        {
            float M00, M01, M02, M03;
        }; // end explicit names

    }; // end union
} MATRIX1X4, *MATRIX1X4_PTR;

// 4x4 matrix /////////////////////////////////////////////
typedef struct MATRIX4X4_TYP
{
    union
    {
        float M[4][4]; // array indexed data storage

        // storage in row major form with explicit names
        struct
        {
            float M00, M01, M02, M03;
            float M10, M11, M12, M13;
            float M20, M21, M22, M23;
            float M30, M31, M32, M33;
        }; // end explicit names

    }; // end union

} MATRIX4X4, *MATRIX4X4_PTR;

// 4x3 matrix /////////////////////////////////////////////
typedef struct MATRIX4X3_TYP
{
    union
    {
        float M[4][3]; // array indexed data storage

        // storage in row major form with explicit names
        struct
        {
            float M00, M01, M02;
            float M10, M11, M12;
            float M20, M21, M22;
            float M30, M31, M32;
        }; // end explicit names

    }; // end union

} MATRIX4X3, *MATRIX4X3_PTR;

// vector types ///////////////////////////////////////////

// 2D vector, point without the w ////////////////////////
typedef struct VECTOR2D_TYP
{
    union
    {
        float M[2]; // array indexed storage

        // explicit names
        struct
        {
            float x,y;
        }; // end struct

    }; // end union

} VECTOR2D, POINT2D, *VECTOR2D_PTR, *POINT2D_PTR;

// 3D vector, point without the w ////////////////////////
typedef struct VECTOR3D_TYP
{
    union
    {
        float M[3]; // array indexed storage

        // explicit names
        struct
        {
            float x,y,z;
        }; // end struct

    }; // end union

} VECTOR3D, POINT3D, *VECTOR3D_PTR, *POINT3D_PTR;

// 4D homogenous vector, point with w ////////////////////
typedef struct VECTOR4D_TYP
{
    union
    {
        float M[4]; // array indexed storage

        // explicit names
        struct
        {
            float x,y,z,w;
        }; // end struct
    }; // end union

} VECTOR4D, POINT4D, *VECTOR4D_PTR, *POINT4D_PTR;

// 2D parametric line /////////////////////////////////////////
typedef struct PARMLINE2D_TYP
{
    POINT2D  p0; // start point of parametric line
    POINT2D  p1; // end point of parametric line
    VECTOR2D v;  // direction vector of line segment
    // |v|=|p0->p1|
} PARMLINE2D, *PARMLINE2D_PTR;

// 3D parametric line /////////////////////////////////////////
typedef struct PARMLINE3D_TYP
{
    POINT3D  p0; // start point of parametric line
    POINT3D  p1; // end point of parametric line
    VECTOR3D v;  // direction vector of line segment
    // |v|=|p0->p1|
} PARMLINE3D, *PARMLINE3D_PTR;

// 3D plane ///////////////////////////////////////////////////
typedef struct PLANE3D_TYP
{
    POINT3D p0; // point on the plane
    VECTOR3D n; // normal to the plane (not necessarily a unit vector)
} PLANE3D, *PLANE3D_PTR;

// 2D polar coordinates ///////////////////////////////////////
typedef struct POLAR2D_TYP
{
    float r;     // the radi of the point
    float theta; // the angle in rads
} POLAR2D, *POLAR2D_PTR;

// 3D cylindrical coordinates ////////////////////////////////
typedef struct CYLINDRICAL3D_TYP
{
    float r;     // the radi of the point
    float theta; // the angle in degrees about the z axis
    float z;     // the z-height of the point
} CYLINDRICAL3D, *CYLINDRICAL3D_PTR;

// 3D spherical coordinates //////////////////////////////////
typedef struct SPHERICAL3D_TYP
{
    float p;      // rho, the distance to the point from the origin
    float theta;  // the angle from the z-axis and the line segment o->p
    float phi;    // the angle from the projection if o->p onto the x-y 
    // plane and the x-axis
} SPHERICAL3D, *SPHERICAL3D_PTR;

// 4d quaternion ////////////////////////////////////////////
// note the union gives us a number of ways to work with
// the components of the quaternion
typedef struct QUAT_TYP
{
    union
    {
        float M[4]; // array indexed storage w,x,y,z order

        // vector part, real part format
        struct 
        {
            float    q0;  // the real part
            VECTOR3D qv;  // the imaginary part xi+yj+zk
        };
        struct
        {
            float w,x,y,z;
        }; 
    }; // end union

} QUAT, *QUAT_PTR;

// fixed point types //////////////////////////////////////////

typedef int FIXP16;
typedef int *FIXP16_PTR;

///////////////////////////////////////////////////////////////

// FORWARD REF DEFINES & CONSTANTS ////////////////////////////

// identity matrices

// 4x4 identity matrix
const MATRIX4X4 IMAT_4X4 = {1,0,0,0, 
0,1,0,0, 
0,0,1,0, 
0,0,0,1};

// 4x3 identity matrix (note this is not correct mathematically)
// but later we may use 4x3 matrices with the assumption that 
// the last column is always [0 0 0 1]t
const MATRIX4X3 IMAT_4X3 = {1,0,0, 
0,1,0, 
0,0,1, 
0,0,0,};


// 3x3 identity matrix
const MATRIX3X3 IMAT_3X3 = {1,0,0, 
0,1,0, 
0,0,1};

// 2x2 identity matrix
const MATRIX2X2 IMAT_2X2 = {1,0, 
0,1};

// MACROS, SMALL INLINE FUNCS /////////////////////////////////

// matrix macros

// macros to clear out matrices
#define MAT_ZERO_2X2(m) {memset((void *)(m), 0, sizeof(MATRIX2X2));}
#define MAT_ZERO_3X3(m) {memset((void *)(m), 0, sizeof(MATRIX3X3));}
#define MAT_ZERO_4X4(m) {memset((void *)(m), 0, sizeof(MATRIX4X4));}
#define MAT_ZERO_4X3(m) {memset((void *)(m), 0, sizeof(MATRIX4X3));}

// macros to set the identity matrix
#define MAT_IDENTITY_2X2(m) {memcpy((void *)(m), (void *)&IMAT_2X2, sizeof(MATRIX2X2));}
#define MAT_IDENTITY_3X3(m) {memcpy((void *)(m), (void *)&IMAT_3X3, sizeof(MATRIX3X3));}
#define MAT_IDENTITY_4X4(m) {memcpy((void *)(m), (void *)&IMAT_4X4, sizeof(MATRIX4X4));}
#define MAT_IDENTITY_4X3(m) {memcpy((void *)(m), (void *)&IMAT_4X3, sizeof(MATRIX4X3));}

// matrix copying macros
#define MAT_COPY_2X2(src_mat, dest_mat) {memcpy((void *)(dest_mat), (void *)(src_mat), sizeof(MATRIX2X2) ); }
#define MAT_COPY_3X3(src_mat, dest_mat) {memcpy((void *)(dest_mat), (void *)(src_mat), sizeof(MATRIX3X3) ); }
#define MAT_COPY_4X4(src_mat, dest_mat) {memcpy((void *)(dest_mat), (void *)(src_mat), sizeof(MATRIX4X4) ); }
#define MAT_COPY_4X3(src_mat, dest_mat) {memcpy((void *)(dest_mat), (void *)(src_mat), sizeof(MATRIX4X3) ); }

// matrix transposing macros
inline void MAT_TRANSPOSE_3X3(MATRIX3X3_PTR m) 
{ MATRIX3X3 mt;
mt.M00 = m->M00; mt.M01 = m->M10; mt.M02 = m->M20;
mt.M10 = m->M01; mt.M11 = m->M11; mt.M12 = m->M21;
mt.M20 = m->M02; mt.M21 = m->M12; mt.M22 = m->M22;
memcpy((void *)m,(void *)&mt, sizeof(MATRIX3X3)); }

inline void MAT_TRANSPOSE_4X4(MATRIX4X4_PTR m) 
{ MATRIX4X4 mt;
mt.M00 = m->M00; mt.M01 = m->M10; mt.M02 = m->M20; mt.M03 = m->M30; 
mt.M10 = m->M01; mt.M11 = m->M11; mt.M12 = m->M21; mt.M13 = m->M31; 
mt.M20 = m->M02; mt.M21 = m->M12; mt.M22 = m->M22; mt.M23 = m->M32; 
mt.M30 = m->M03; mt.M31 = m->M13; mt.M32 = m->M22; mt.M33 = m->M33; 
memcpy((void *)m,(void *)&mt, sizeof(MATRIX4X4)); }

inline void MAT_TRANSPOSE_3X3(MATRIX3X3_PTR m, MATRIX3X3_PTR mt) 
{ mt->M00 = m->M00; mt->M01 = m->M10; mt->M02 = m->M20;
mt->M10 = m->M01; mt->M11 = m->M11; mt->M12 = m->M21;
mt->M20 = m->M02; mt->M21 = m->M12; mt->M22 = m->M22; }

inline void MAT_TRANSPOSE_4X4(MATRIX4X4_PTR m, MATRIX4X4_PTR mt) 
{ mt->M00 = m->M00; mt->M01 = m->M10; mt->M02 = m->M20; mt->M03 = m->M30; 
mt->M10 = m->M01; mt->M11 = m->M11; mt->M12 = m->M21; mt->M13 = m->M31; 
mt->M20 = m->M02; mt->M21 = m->M12; mt->M22 = m->M22; mt->M23 = m->M32; 
mt->M30 = m->M03; mt->M31 = m->M13; mt->M32 = m->M22; mt->M33 = m->M33; }

// small inline functions that could be re-written as macros, but would
// be less robust

// matrix and vector column swaping macros
inline void MAT_COLUMN_SWAP_2X2(MATRIX2X2_PTR m, int c, MATRIX1X2_PTR v) 
{ m->M[0][c]=v->M[0]; m->M[1][c]=v->M[1]; } 

inline void MAT_COLUMN_SWAP_3X3(MATRIX3X3_PTR m, int c, MATRIX1X3_PTR v) 
{ m->M[0][c]=v->M[0]; m->M[1][c]=v->M[1]; m->M[2][c]=v->M[2]; } 

inline void MAT_COLUMN_SWAP_4X4(MATRIX4X4_PTR m, int c, MATRIX1X4_PTR v) 
{m->M[0][c]=v->M[0]; m->M[1][c]=v->M[1]; m->M[2][c]=v->M[2]; m->M[3][c]=v->M[3]; } 

inline void MAT_COLUMN_SWAP_4X3(MATRIX4X3_PTR m, int c, MATRIX1X4_PTR v) 
{m->M[0][c]=v->M[0]; m->M[1][c]=v->M[1]; m->M[2][c]=v->M[2]; m->M[3][c]=v->M[3]; } 

// vector macros, note the 4D vector sets w=1
// vector zeroing macros
inline void VECTOR2D_ZERO(VECTOR2D_PTR v) 
{(v)->x = (v)->y = 0.0;}

inline void VECTOR3D_ZERO(VECTOR3D_PTR v) 
{(v)->x = (v)->y = (v)->z = 0.0;}

inline void VECTOR4D_ZERO(VECTOR4D_PTR v) 
{(v)->x = (v)->y = (v)->z = 0.0; (v)->w = 1.0;}

// macros to initialize vectors with explicit components
inline void VECTOR2D_INITXY(VECTOR2D_PTR v, float x, float y) 
{(v)->x = (x); (v)->y = (y);}

inline void VECTOR3D_INITXYZ(VECTOR3D_PTR v, float x, float y, float z) 
{(v)->x = (x); (v)->y = (y); (v)->z = (z);}

inline void VECTOR4D_INITXYZ(VECTOR4D_PTR v, float x,float y,float z) 
{(v)->x = (x); (v)->y = (y); (v)->z = (z); (v)->w = 1.0;}

// used to convert from 4D homogenous to 4D non-homogenous
inline void VECTOR4D_DIV_BY_W(VECTOR4D_PTR v) 
{(v)->x/=(v)->w; (v)->y/=(v)->w; (v)->z/=(v)->w;  }

inline void VECTOR4D_DIV_BY_W_VECTOR3D(VECTOR4D_PTR v4, VECTOR3D_PTR v3) 
{(v3)->x = (v4)->x/(v4)->w; (v3)->y = (v4)->y/(v4)->w; (v3)->z = (v4)->z/(v4)->w;  }

// vector intialization macros to initialize with other vectors
inline void VECTOR2D_INIT(VECTOR2D_PTR vdst, VECTOR2D_PTR vsrc) 
{(vdst)->x = (vsrc)->x; (vdst)->y = (vsrc)->y;  }

inline void VECTOR3D_INIT(VECTOR3D_PTR vdst, VECTOR3D_PTR vsrc) 
{(vdst)->x = (vsrc)->x; (vdst)->y = (vsrc)->y;  (vdst)->z = (vsrc)->z; }

inline void VECTOR4D_INIT(VECTOR4D_PTR vdst, VECTOR4D_PTR vsrc) 
{(vdst)->x = (vsrc)->x; (vdst)->y = (vsrc)->y;  
(vdst)->z = (vsrc)->z; (vdst)->w = (vsrc)->w;  }


// vector copying macros
inline void VECTOR2D_COPY(VECTOR2D_PTR vdst, VECTOR2D_PTR vsrc) 
{(vdst)->x = (vsrc)->x; (vdst)->y = (vsrc)->y;  }


inline void VECTOR3D_COPY(VECTOR3D_PTR vdst, VECTOR3D_PTR vsrc) 
{(vdst)->x = (vsrc)->x; (vdst)->y = (vsrc)->y;  (vdst)->z = (vsrc)->z; }

inline void VECTOR4D_COPY(VECTOR4D_PTR vdst, VECTOR4D_PTR vsrc) 
{(vdst)->x = (vsrc)->x; (vdst)->y = (vsrc)->y;  
(vdst)->z = (vsrc)->z; (vdst)->w = (vsrc)->w;  }


// point initialization macros
inline void POINT2D_INIT(POINT2D_PTR vdst, POINT2D_PTR vsrc) 
{(vdst)->x = (vsrc)->x; (vdst)->y = (vsrc)->y;  }

inline void POINT3D_INIT(POINT3D_PTR vdst, POINT3D_PTR vsrc) 
{(vdst)->x = (vsrc)->x; (vdst)->y = (vsrc)->y;  (vdst)->z = (vsrc)->z; }

inline void POINT4D_INIT(POINT4D_PTR vdst, POINT4D_PTR vsrc) 
{(vdst)->x = (vsrc)->x; (vdst)->y = (vsrc)->y;  
(vdst)->z = (vsrc)->z; (vdst)->w = (vsrc)->w;  }

// point copying macros
inline void POINT2D_COPY(POINT2D_PTR vdst, POINT2D_PTR vsrc) 
{(vdst)->x = (vsrc)->x; (vdst)->y = (vsrc)->y;  }

inline void POINT3D_COPY(POINT3D_PTR vdst, POINT3D_PTR vsrc) 
{(vdst)->x = (vsrc)->x; (vdst)->y = (vsrc)->y;  (vdst)->z = (vsrc)->z; }

inline void POINT4D_COPY(POINT4D_PTR vdst, POINT4D_PTR vsrc) 
{(vdst)->x = (vsrc)->x; (vdst)->y = (vsrc)->y;  
(vdst)->z = (vsrc)->z; (vdst)->w = (vsrc)->w;  }

// quaternion macros
inline void QUAT_ZERO(QUAT_PTR q) 
{(q)->x = (q)->y = (q)->z = (q)->w = 0.0;}

inline void QUAT_INITWXYZ(QUAT_PTR q, float w, float x,float y,float z) 
{ (q)->w = (w); (q)->x = (x); (q)->y = (y); (q)->z = (z); }

inline void QUAT_INIT_VECTOR3D(QUAT_PTR q, VECTOR3D_PTR v) 
{ (q)->w = 0; (q)->x = (v->x); (q)->y = (v->y); (q)->z = (v->z); }

inline void QUAT_INIT(QUAT_PTR qdst, QUAT_PTR qsrc) 
{(qdst)->w = (qsrc)->w;  (qdst)->x = (qsrc)->x;  
(qdst)->y = (qsrc)->y;  (qdst)->z = (qsrc)->z;  }

inline void QUAT_COPY(QUAT_PTR qdst, QUAT_PTR qsrc) 
{(qdst)->x = (qsrc)->x; (qdst)->y = (qsrc)->y;  
(qdst)->z = (qsrc)->z; (qdst)->w = (qsrc)->w;  }

// convert integer and float to fixed point 16.16
#define INT_TO_FIXP16(i) ((i) <<  FIXP16_SHIFT)
#define FLOAT_TO_FIXP16(f) (((float)(f) * (float)FIXP16_MAG+0.5))

// convert fixed point to float
#define FIXP16_TO_FLOAT(fp) ( ((float)fp)/FIXP16_MAG)

// extract the whole part and decimal part from a fixed point 16.16
#define FIXP16_WP(fp) ((fp) >> FIXP16_SHIFT)
#define FIXP16_DP(fp) ((fp) && FIXP16_DP_MASK)

// PROTOTYPES /////////////////////////////////////////////

// trig functions
float Fast_Sin(float theta);
float Fast_Cos(float theta);

// polar, cylindrical, spherical functions
void POLAR2D_To_POINT2D(POLAR2D_PTR polar, POINT2D_PTR rect);
void POLAR2D_To_RectXY(POLAR2D_PTR polar, float *x, float *y);
void POINT2D_To_POLAR2D(POINT2D_PTR rect, POLAR2D_PTR polar);
void POINT2D_To_PolarRTh(POINT2D_PTR rect, float *r, float *theta);
void CYLINDRICAL3D_To_POINT3D(CYLINDRICAL3D_PTR cyl, POINT3D_PTR rect);
void CYLINDRICAL3D_To_RectXYZ(CYLINDRICAL3D_PTR cyl, float *x, float *y, float *z);
void POINT3D_To_CYLINDRICAL3D(POINT3D_PTR rect, CYLINDRICAL3D_PTR cyl);
void POINT3D_To_CylindricalRThZ(POINT3D_PTR rect, float *r, float *theta, float *z);
void SPHERICAL3D_To_POINT3D(SPHERICAL3D_PTR sph, POINT3D_PTR rect);
void SPHERICAL3D_To_RectXYZ(SPHERICAL3D_PTR sph, float *x, float *y, float *z);
void POINT3D_To_SPHERICAL3D(POINT3D_PTR rect, SPHERICAL3D_PTR sph);
void POINT3D_To_SphericalPThPh(POINT3D_PTR rect, float *p, float *theta, float *phi);

// 2d vector functions
void VECTOR2D_Add(VECTOR2D_PTR va, VECTOR2D_PTR vb, VECTOR2D_PTR vsum);
VECTOR2D VECTOR2D_Add(VECTOR2D_PTR va, VECTOR2D_PTR vb);
void VECTOR2D_Sub(VECTOR2D_PTR va, VECTOR2D_PTR vb, VECTOR2D_PTR vdiff);
VECTOR2D VECTOR2D_Sub(VECTOR2D_PTR va, VECTOR2D_PTR vb);
void VECTOR2D_Scale(float k, VECTOR2D_PTR va);
void VECTOR2D_Scale(float k, VECTOR2D_PTR va, VECTOR2D_PTR vscaled);
float VECTOR2D_Dot(VECTOR2D_PTR va, VECTOR2D_PTR vb);
float VECTOR2D_Length(VECTOR2D_PTR va);
float VECTOR2D_Length_Fast(VECTOR2D_PTR va);
void VECTOR2D_Normalize(VECTOR2D_PTR va);
void VECTOR2D_Normalize(VECTOR2D_PTR va, VECTOR2D_PTR vn);
void VECTOR2D_Build(VECTOR2D_PTR init, VECTOR2D_PTR term, VECTOR2D_PTR result);
float VECTOR2D_CosTh(VECTOR2D_PTR va, VECTOR2D_PTR vb);
void VECTOR2D_Print(VECTOR2D_PTR va, char *name);

// 3d vector functions
void VECTOR3D_Add(VECTOR3D_PTR va, VECTOR3D_PTR vb, VECTOR3D_PTR vsum);
VECTOR3D VECTOR3D_Add(VECTOR3D_PTR va, VECTOR3D_PTR vb);
void VECTOR3D_Sub(VECTOR3D_PTR va, VECTOR3D_PTR vb, VECTOR3D_PTR vdiff);
VECTOR3D VECTOR3D_Sub(VECTOR3D_PTR va, VECTOR3D_PTR vb);
void VECTOR3D_Scale(float k, VECTOR3D_PTR va);
void VECTOR3D_Scale(float k, VECTOR3D_PTR va, VECTOR3D_PTR vscaled);
float VECTOR3D_Dot(VECTOR3D_PTR va, VECTOR3D_PTR vb);
void VECTOR3D_Cross(VECTOR3D_PTR va,VECTOR3D_PTR vb,VECTOR3D_PTR vn);
VECTOR3D VECTOR3D_Cross(VECTOR3D_PTR va, VECTOR3D_PTR vb);
float VECTOR3D_Length(VECTOR3D_PTR va);
float VECTOR3D_Length_Fast(VECTOR3D_PTR va);
void VECTOR3D_Normalize(VECTOR3D_PTR va);
void VECTOR3D_Normalize(VECTOR3D_PTR va, VECTOR3D_PTR vn);
void VECTOR3D_Build(VECTOR3D_PTR init, VECTOR3D_PTR term, VECTOR3D_PTR result);
float VECTOR3D_CosTh(VECTOR3D_PTR va, VECTOR3D_PTR vb);
void VECTOR3D_Print(VECTOR3D_PTR va, char *name);

// 4d vector functions
void VECTOR4D_Add(VECTOR4D_PTR va, VECTOR4D_PTR vb, VECTOR4D_PTR vsum);
VECTOR4D VECTOR4D_Add(VECTOR4D_PTR va, VECTOR4D_PTR vb);
void VECTOR4D_Sub(VECTOR4D_PTR va, VECTOR4D_PTR vb, VECTOR4D_PTR vdiff);
VECTOR4D VECTOR4D_Sub(VECTOR4D_PTR va, VECTOR4D_PTR vb);
void VECTOR4D_Scale(float k, VECTOR4D_PTR va);
void VECTOR4D_Scale(float k, VECTOR4D_PTR va, VECTOR4D_PTR vscaled);
float VECTOR4D_Dot(VECTOR4D_PTR va, VECTOR4D_PTR vb);
void VECTOR4D_Cross(VECTOR4D_PTR va,VECTOR4D_PTR vb,VECTOR4D_PTR vn);
VECTOR4D VECTOR4D_Cross(VECTOR4D_PTR va, VECTOR4D_PTR vb);
float VECTOR4D_Length(VECTOR4D_PTR va);
float VECTOR4D_Length_Fast(VECTOR4D_PTR va);
void VECTOR4D_Normalize(VECTOR4D_PTR va);
void VECTOR4D_Normalize(VECTOR4D_PTR va, VECTOR4D_PTR vn);
void VECTOR4D_Build(VECTOR4D_PTR init, VECTOR4D_PTR term, VECTOR4D_PTR result);
float VECTOR4D_CosTh(VECTOR4D_PTR va, VECTOR4D_PTR vb);
void VECTOR4D_Print(VECTOR4D_PTR va, char *name);


// 2x2 matrix functions  (note there others in T3DLIB1.CPP|H)
void Mat_Init_2X2(MATRIX2X2_PTR ma, float m00, float m01, float m10, float m11);
void Print_Mat_2X2(MATRIX2X2_PTR ma, char *name);
float Mat_Det_2X2(MATRIX2X2_PTR m);
void Mat_Add_2X2(MATRIX2X2_PTR ma, MATRIX2X2_PTR mb, MATRIX2X2_PTR msum);
void Mat_Mul_2X2(MATRIX2X2_PTR ma, MATRIX2X2_PTR mb, MATRIX2X2_PTR mprod);
int Mat_Inverse_2X2(MATRIX2X2_PTR m, MATRIX2X2_PTR mi);
int Solve_2X2_System(MATRIX2X2_PTR A, MATRIX1X2_PTR X, MATRIX1X2_PTR B);

// 3x3 matrix functions (note there others in T3DLIB1.CPP|H)
void Mat_Add_3X3(MATRIX3X3_PTR ma, MATRIX3X3_PTR mb, MATRIX3X3_PTR msum);
void Mat_Mul_VECTOR3D_3X3(VECTOR3D_PTR  va, MATRIX3X3_PTR mb,VECTOR3D_PTR  vprod);
int Mat_Inverse_3X3(MATRIX3X3_PTR m, MATRIX3X3_PTR mi);
void Mat_Init_3X3(MATRIX3X3_PTR ma, 
                  float m00, float m01, float m02,
                  float m10, float m11, float m12,
                  float m20, float m21, float m22);
void Print_Mat_3X3(MATRIX3X3_PTR ma, char *name);
float Mat_Det_3X3(MATRIX3X3_PTR m);
int Solve_3X3_System(MATRIX3X3_PTR A, MATRIX1X3_PTR X, MATRIX1X3_PTR B);

// 4x4 matrix functions
void Mat_Add_4X4(MATRIX4X4_PTR ma, MATRIX4X4_PTR mb, MATRIX4X4_PTR msum);
void Mat_Mul_4X4(MATRIX4X4_PTR ma, MATRIX4X4_PTR mb, MATRIX4X4_PTR mprod);
void Mat_Mul_1X4_4X4(MATRIX1X4_PTR ma, MATRIX4X4_PTR mb, MATRIX1X4_PTR mprod);
void Mat_Mul_VECTOR3D_4X4(VECTOR3D_PTR  va, MATRIX4X4_PTR mb, VECTOR3D_PTR  vprod);
void Mat_Mul_VECTOR3D_4X3(VECTOR3D_PTR  va, MATRIX4X3_PTR mb, VECTOR3D_PTR  vprod);
void Mat_Mul_VECTOR4D_4X4(VECTOR4D_PTR  va, MATRIX4X4_PTR mb, VECTOR4D_PTR  vprod);
void Mat_Mul_VECTOR4D_4X3(VECTOR4D_PTR  va, MATRIX4X4_PTR mb, VECTOR4D_PTR  vprod);
int Mat_Inverse_4X4(MATRIX4X4_PTR m, MATRIX4X4_PTR mi);
void Mat_Init_4X4(MATRIX4X4_PTR ma, 
                  float m00, float m01, float m02, float m03,
                  float m10, float m11, float m12, float m13,
                  float m20, float m21, float m22, float m23,
                  float m30, float m31, float m32, float m33);
void Print_Mat_4X4(MATRIX4X4_PTR ma, char *name);

// quaternion functions
void QUAT_Add(QUAT_PTR q1, QUAT_PTR q2, QUAT_PTR qsum);
void QUAT_Sub(QUAT_PTR q1, QUAT_PTR q2, QUAT_PTR qdiff);
void QUAT_Conjugate(QUAT_PTR q, QUAT_PTR qconj);
void QUAT_Scale(QUAT_PTR q, float scale, QUAT_PTR qs);
void QUAT_Scale(QUAT_PTR q, float scale);
float QUAT_Norm(QUAT_PTR q);
float QUAT_Norm2(QUAT_PTR q);
void QUAT_Normalize(QUAT_PTR q, QUAT_PTR qn);
void QUAT_Normalize(QUAT_PTR q);
void QUAT_Unit_Inverse(QUAT_PTR q, QUAT_PTR qi);
void QUAT_Unit_Inverse(QUAT_PTR q);
void QUAT_Inverse(QUAT_PTR q, QUAT_PTR qi);
void QUAT_Inverse(QUAT_PTR q);
void QUAT_Mul(QUAT_PTR q1, QUAT_PTR q2, QUAT_PTR qprod);
void QUAT_Triple_Product(QUAT_PTR q1, QUAT_PTR q2, QUAT_PTR q3, QUAT_PTR qprod);
void VECTOR3D_Theta_To_QUAT(QUAT_PTR q, VECTOR3D_PTR v, float theta);
void VECTOR4D_Theta_To_QUAT(QUAT_PTR q, VECTOR4D_PTR v, float theta);
void EulerZYX_To_QUAT(QUAT_PTR q, float theta_z, float theta_y, float theta_x);
void QUAT_To_VECTOR3D_Theta(QUAT_PTR q, VECTOR3D_PTR v, float *theta);
void QUAT_Print(QUAT_PTR q, char *name);

// 2d parametric line functions
void Init_Parm_Line2D(POINT2D_PTR p_init, POINT2D_PTR p_term, PARMLINE2D_PTR p);
void Compute_Parm_Line2D(PARMLINE2D_PTR p, float t, POINT2D_PTR pt);
int Intersect_Parm_Lines2D(PARMLINE2D_PTR p1, PARMLINE2D_PTR p2, float *t1, float *t2);
int Intersect_Parm_Lines2D(PARMLINE2D_PTR p1, PARMLINE2D_PTR p2, POINT2D_PTR pt);

// 3d parametric line functions
void Init_Parm_Line3D(POINT3D_PTR p_init, POINT3D_PTR p_term, PARMLINE3D_PTR p);
void Compute_Parm_Line3D(PARMLINE3D_PTR p, float t, POINT3D_PTR pt);

// 3d plane functions
void PLANE3D_Init(PLANE3D_PTR plane, POINT3D_PTR p0, 
                  VECTOR3D_PTR normal, int normalize);
float Compute_Point_In_Plane3D(POINT3D_PTR pt, PLANE3D_PTR plane);
int Intersect_Parm_Line3D_Plane3D(PARMLINE3D_PTR pline, PLANE3D_PTR plane, 
                                  float *t, POINT3D_PTR pt);

// fixed point functions
FIXP16 FIXP16_MUL(FIXP16 fp1, FIXP16 fp2);
FIXP16 FIXP16_DIV(FIXP16 fp1, FIXP16 fp2);
void FIXP16_Print(FIXP16 fp);


// GLOBALS ////////////////////////////////////////////////


// EXTERNALS //////////////////////////////////////////////

#endif
