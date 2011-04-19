// T3DLIB10.H - header file for T3DLIB10.H

// watch for multiple inclusions
#ifndef T3DLIB10
#define T3DLIB10

// DEFINES //////////////////////////////////////////////////////////////////

// alpha blending defines
#define NUM_ALPHA_LEVELS              8   // total number of alpha levels

// perspective correct/ 1/z buffer defines
#define FIXP28_SHIFT                  28  // used for 1/z buffering
#define FIXP22_SHIFT                  22  // used for u/z, v/z perspective texture mapping

// new attributes to support mip mapping
#define POLY4DV2_ATTR_MIPMAP          0x0400 // flags if polygon has a mipmap
#define OBJECT4DV2_ATTR_MIPMAP        0x0008 // flags if object has a mipmap

///////////////////////////////////////////////////////////////////////////////

// defines that control the rendering function state attributes
// note each class of control flags is contained within
// a 4-bit nibble where possible, this helps with future expansion

// no z buffering, polygons will be rendered as are in list
#define RENDER_ATTR_NOBUFFER                     0x00000001  

// use z buffering rasterization
#define RENDER_ATTR_ZBUFFER                      0x00000002  

// use 1/z buffering rasterization
#define RENDER_ATTR_INVZBUFFER                   0x00000004  

// use mipmapping
#define RENDER_ATTR_MIPMAP                       0x00000010  

// enable alpha blending and override
#define RENDER_ATTR_ALPHA                        0x00000020  

// enable bilinear filtering, but only supported for
// constant shaded/affine textures
#define RENDER_ATTR_BILERP                       0x00000040  

// use affine texturing for all polys
#define RENDER_ATTR_TEXTURE_PERSPECTIVE_AFFINE   0x00000100  

// use perfect perspective texturing
#define RENDER_ATTR_TEXTURE_PERSPECTIVE_CORRECT  0x00000200  

// use linear piecewise perspective texturing
#define RENDER_ATTR_TEXTURE_PERSPECTIVE_LINEAR   0x00000400  

// use a hybrid of affine and linear piecewise based on distance
#define RENDER_ATTR_TEXTURE_PERSPECTIVE_HYBRID1  0x00000800  

// not implemented yet
#define RENDER_ATTR_TEXTURE_PERSPECTIVE_HYBRID2  0x00001000  

// TYPES ///////////////////////////////////////////////////////////////////

// this is a new type to contain the "rendering context" RC, so
// we don't have to keep passing more and more variables to the
// rendering functions, we can fill this structure in with what we
// need and then pass it as a whole
typedef struct RENDERCONTEXTV1_TYP
{
    int     attr;                 // all the rendering attributes
    RENDERLIST4DV2_PTR rend_list; // ptr to rendering list to render
    UCHAR  *video_buffer;         // ptr to video buffer to render to
    int     lpitch;               // memory pitch in bytes of video buffer       

    UCHAR  *zbuffer;              // ptr to z buffer or 1/z buffer
    int     zpitch;               // memory pitch in bytes of z or 1/z buffer
    int     alpha_override;       // alpha value to override ALL polys with

    int     mip_dist;             // maximum distance to divide up into 
    // mip levels
    // 0 - (NUM_ALPHA_LEVELS - 1)
    int     texture_dist,         // the distance to enable affine texturing
        texture_dist2;        // when using hybrid perspective/affine mode

    // future expansion
    int     ival1, ivalu2;        // extra integers
    float   fval1, fval2;         // extra floats
    void    *vptr;                // extra pointer

} RENDERCONTEXTV1, *RENDERCONTEXTV1_PTR;

// CLASSES /////////////////////////////////////////////////////////////////


// MACROS ///////////////////////////////////////////////////////////////////



// EXTERNALS ///////////////////////////////////////////////////////////////

// this table contains each possible RGB value multiplied by some scaler
extern USHORT rgb_alpha_table[NUM_ALPHA_LEVELS][65536];  

extern HWND main_window_handle; // save the window handle
extern HINSTANCE main_instance; // save the instance


// PROTOTYPES //////////////////////////////////////////////////////////////

// z buffered versions
void Draw_Textured_TriangleZB2_16(POLYF4DV2_PTR face,  // ptr to face
                                  UCHAR *_dest_buffer,  // pointer to video buffer
                                  int mem_pitch,        // bytes per line, 320, 640 etc.
                                  UCHAR *zbuffer,       // pointer to z-buffer
                                  int zpitch);          // bytes per line of zbuffer

void Draw_Textured_Bilerp_TriangleZB_16(POLYF4DV2_PTR face,  // ptr to face
                                        UCHAR *_dest_buffer,  // pointer to video buffer
                                        int mem_pitch,        // bytes per line, 320, 640 etc.
                                        UCHAR *zbuffer,       // pointer to z-buffer
                                        int zpitch);          // bytes per line of zbuffer


void Draw_Textured_TriangleFSZB2_16(POLYF4DV2_PTR face, // ptr to face
                                    UCHAR *_dest_buffer,  // pointer to video buffer
                                    int mem_pitch,        // bytes per line, 320, 640 etc.
                                    UCHAR *zbuffer,       // pointer to z-buffer
                                    int zpitch);          // bytes per line of zbuffer



void Draw_Textured_TriangleGSZB_16(POLYF4DV2_PTR face,   // ptr to face
                                   UCHAR *_dest_buffer, // pointer to video buffer
                                   int mem_pitch,       // bytes per line, 320, 640 etc.
                                   UCHAR *_zbuffer,     // pointer to z-buffer
                                   int zpitch);         // bytes per line of zbuffer

void Draw_Triangle_2DZB2_16(POLYF4DV2_PTR face, // ptr to face
                            UCHAR *_dest_buffer, // pointer to video buffer
                            int mem_pitch,       // bytes per line, 320, 640 etc.
                            UCHAR *zbuffer,      // pointer to z-buffer
                            int zpitch);         // bytes per line of zbuffer


void Draw_Gouraud_TriangleZB2_16(POLYF4DV2_PTR face,   // ptr to face
                                 UCHAR *_dest_buffer,// pointer to video buffer
                                 int mem_pitch,      // bytes per line, 320, 640 etc.
                                 UCHAR *zbuffer,     // pointer to z-buffer
                                 int zpitch);        // bytes per line of zbuffer


void Draw_RENDERLIST4DV2_SolidZB2_16(RENDERLIST4DV2_PTR rend_list, 
                                     UCHAR *video_buffer, 
                                     int lpitch,
                                     UCHAR *zbuffer,
                                     int zpitch);


// 1/z versions
void Draw_Textured_TriangleINVZB_16(POLYF4DV2_PTR face,  // ptr to face
                                    UCHAR *_dest_buffer,  // pointer to video buffer
                                    int mem_pitch,        // bytes per line, 320, 640 etc.
                                    UCHAR *zbuffer,       // pointer to z-buffer
                                    int zpitch);          // bytes per line of zbuffer


void Draw_Textured_Bilerp_TriangleINVZB_16(POLYF4DV2_PTR face,  // ptr to face
                                           UCHAR *_dest_buffer,  // pointer to video buffer
                                           int mem_pitch,        // bytes per line, 320, 640 etc.
                                           UCHAR *zbuffer,       // pointer to z-buffer
                                           int zpitch);          // bytes per line of zbuffer


void Draw_Textured_TriangleFSINVZB_16(POLYF4DV2_PTR face, // ptr to face
                                      UCHAR *_dest_buffer,  // pointer to video buffer
                                      int mem_pitch,        // bytes per line, 320, 640 etc.
                                      UCHAR *zbuffer,       // pointer to z-buffer
                                      int zpitch);          // bytes per line of zbuffer


void Draw_Textured_TriangleGSINVZB_16(POLYF4DV2_PTR face,   // ptr to face
                                      UCHAR *_dest_buffer, // pointer to video buffer
                                      int mem_pitch,       // bytes per line, 320, 640 etc.
                                      UCHAR *_zbuffer,     // pointer to z-buffer
                                      int zpitch);         // bytes per line of zbuffer

void Draw_Triangle_2DINVZB_16(POLYF4DV2_PTR face, // ptr to face
                              UCHAR *_dest_buffer,  // pointer to video buffer
                              int mem_pitch,        // bytes per line, 320, 640 etc.
                              UCHAR *zbuffer,       // pointer to z-buffer
                              int zpitch);          // bytes per line of zbuffer


void Draw_Gouraud_TriangleINVZB_16(POLYF4DV2_PTR face,   // ptr to face
                                   UCHAR *_dest_buffer, // pointer to video buffer
                                   int mem_pitch,       // bytes per line, 320, 640 etc.
                                   UCHAR *zbuffer,      // pointer to z-buffer
                                   int zpitch);         // bytes per line of zbuffer


void Draw_Textured_Perspective_Triangle_INVZB_16(POLYF4DV2_PTR face,  // ptr to face
                                                 UCHAR *_dest_buffer, // pointer to video buffer
                                                 int mem_pitch,       // bytes per line, 320, 640 etc.
                                                 UCHAR *_zbuffer,     // pointer to z-buffer
                                                 int zpitch);         // bytes per line of zbuffer


void Draw_Textured_Perspective_Triangle_FSINVZB_16(POLYF4DV2_PTR face,// ptr to face
                                                   UCHAR *_dest_buffer,// pointer to video buffer
                                                   int mem_pitch,      // bytes per line, 320, 640 etc.
                                                   UCHAR *_zbuffer,    // pointer to z-buffer
                                                   int zpitch);        // bytes per line of zbuffer

void Draw_Textured_PerspectiveLP_Triangle_FSINVZB_16(POLYF4DV2_PTR face,// ptr to face
                                                     UCHAR *_dest_buffer, // pointer to video buffer
                                                     int mem_pitch,       // bytes per line, 320, 640 etc.
                                                     UCHAR *_zbuffer,     // pointer to z-buffer
                                                     int zpitch);         // bytes per line of zbuffer


void Draw_Textured_PerspectiveLP_Triangle_INVZB_16(POLYF4DV2_PTR face,  // ptr to face
                                                   UCHAR *_dest_buffer, // pointer to video buffer
                                                   int mem_pitch,       // bytes per line, 320, 640 etc.
                                                   UCHAR *_zbuffer,     // pointer to z-buffer
                                                   int zpitch);         // bytes per line of zbuffer

void Draw_RENDERLIST4DV2_SolidINVZB_16(RENDERLIST4DV2_PTR rend_list, 
                                       UCHAR *video_buffer, 
                                       int lpitch,
                                       UCHAR *zbuffer,
                                       int zpitch);


// testing function
void Draw_RENDERLIST4DV2_Hybrid_Textured_SolidINVZB_16(RENDERLIST4DV2_PTR rend_list, 
                                                       UCHAR *video_buffer, 
                                                       int lpitch,
                                                       UCHAR *zbuffer,
                                                       int zpitch,
                                                       float dist1, float dist2);

// z buffered and alpha
void Draw_Textured_TriangleZB_Alpha16(POLYF4DV2_PTR face,  // ptr to face
                                      UCHAR *_dest_buffer, // pointer to video buffer
                                      int mem_pitch,       // bytes per line, 320, 640 etc.
                                      UCHAR *zbuffer,      // pointer to z-buffer
                                      int zpitch,          // bytes per line of zbuffer
                                      int alpha);

void Draw_Textured_TriangleFSZB_Alpha16(POLYF4DV2_PTR face, // ptr to face
                                        UCHAR *_dest_buffer, // pointer to video buffer
                                        int mem_pitch,       // bytes per line, 320, 640 etc.
                                        UCHAR *zbuffer,      // pointer to z-buffer
                                        int zpitch,          // bytes per line of zbuffer
                                        int alpha);

void Draw_Textured_TriangleGSZB_Alpha16(POLYF4DV2_PTR face,   // ptr to face
                                        UCHAR *_dest_buffer, // pointer to video buffer
                                        int mem_pitch,       // bytes per line, 320, 640 etc.
                                        UCHAR *_zbuffer,     // pointer to z-buffer
                                        int zpitch,          // bytes per line of zbuffer
                                        int alpha);


void Draw_Triangle_2DZB_Alpha16(POLYF4DV2_PTR face, // ptr to face
                                UCHAR *_dest_buffer, // pointer to video buffer
                                int mem_pitch,       // bytes per line, 320, 640 etc.
                                UCHAR *zbuffer,      // pointer to z-buffer
                                int zpitch,          // bytes per line of zbuffer
                                int alpha);

void Draw_Gouraud_TriangleZB_Alpha16(POLYF4DV2_PTR face,   // ptr to face
                                     UCHAR *_dest_buffer, // pointer to video buffer
                                     int mem_pitch,       // bytes per line, 320, 640 etc.
                                     UCHAR *zbuffer,      // pointer to z-buffer
                                     int zpitch,          // bytes per line of zbuffer
                                     int alpha);


void Draw_RENDERLIST4DV2_SolidZB_Alpha16(RENDERLIST4DV2_PTR rend_list, 
                                         UCHAR *video_buffer, 
                                         int lpitch,
                                         UCHAR *zbuffer,
                                         int zpitch,
                                         int alpha_override);


// 1/z buffered and alpha
void Draw_Textured_TriangleINVZB_Alpha16(POLYF4DV2_PTR face,  // ptr to face
                                         UCHAR *_dest_buffer, // pointer to video buffer
                                         int mem_pitch,       // bytes per line, 320, 640 etc.
                                         UCHAR *zbuffer,      // pointer to z-buffer
                                         int zpitch,          // bytes per line of zbuffer
                                         int alpha);

void Draw_Textured_TriangleFSINVZB_Alpha16(POLYF4DV2_PTR face, // ptr to face
                                           UCHAR *_dest_buffer, // pointer to video buffer
                                           int mem_pitch,       // bytes per line, 320, 640 etc.
                                           UCHAR *zbuffer,      // pointer to z-buffer
                                           int zpitch,          // bytes per line of zbuffer
                                           int alpha);

void Draw_Textured_TriangleGSINVZB_Alpha16(POLYF4DV2_PTR face,   // ptr to face
                                           UCHAR *_dest_buffer, // pointer to video buffer
                                           int mem_pitch,       // bytes per line, 320, 640 etc.
                                           UCHAR *_zbuffer,     // pointer to z-buffer
                                           int zpitch,          // bytes per line of zbuffer
                                           int alpha);


void Draw_Triangle_2DINVZB_Alpha16(POLYF4DV2_PTR face, // ptr to face
                                   UCHAR *_dest_buffer, // pointer to video buffer
                                   int mem_pitch,       // bytes per line, 320, 640 etc.
                                   UCHAR *zbuffer,      // pointer to z-buffer
                                   int zpitch,          // bytes per line of zbuffer
                                   int alpha);

void Draw_Gouraud_TriangleINVZB_Alpha16(POLYF4DV2_PTR face,   // ptr to face
                                        UCHAR *_dest_buffer, // pointer to video buffer
                                        int mem_pitch,       // bytes per line, 320, 640 etc.
                                        UCHAR *zbuffer,      // pointer to z-buffer
                                        int zpitch,          // bytes per line of zbuffer
                                        int alpha);

void Draw_Textured_Perspective_Triangle_INVZB_Alpha16(POLYF4DV2_PTR face,  // ptr to face
                                                      UCHAR *_dest_buffer, // pointer to video buffer
                                                      int mem_pitch,       // bytes per line, 320, 640 etc.
                                                      UCHAR *_zbuffer,     // pointer to z-buffer
                                                      int zpitch,          // bytes per line of zbuffer
                                                      int alpha);


void Draw_Textured_PerspectiveLP_Triangle_INVZB_Alpha16(POLYF4DV2_PTR face,  // ptr to face
                                                        UCHAR *_dest_buffer, // pointer to video buffer
                                                        int mem_pitch,       // bytes per line, 320, 640 etc.
                                                        UCHAR *_zbuffer,     // pointer to z-buffer
                                                        int zpitch,          // bytes per line of zbuffer
                                                        int alpha);

void Draw_Textured_Perspective_Triangle_FSINVZB_Alpha16(POLYF4DV2_PTR face,  // ptr to face
                                                        UCHAR *_dest_buffer, // pointer to video buffer
                                                        int mem_pitch,       // bytes per line, 320, 640 etc.
                                                        UCHAR *_zbuffer,     // pointer to z-buffer
                                                        int zpitch,          // bytes per line of zbuffer
                                                        int alpha);

void Draw_Textured_PerspectiveLP_Triangle_FSINVZB_Alpha16(POLYF4DV2_PTR face,  // ptr to face
                                                          UCHAR *_dest_buffer, // pointer to video buffer
                                                          int mem_pitch,       // bytes per line, 320, 640 etc.
                                                          UCHAR *_zbuffer,     // pointer to z-buffer
                                                          int zpitch,          // bytes per line of zbuffer
                                                          int alpha);


void Draw_RENDERLIST4DV2_SolidINVZB_Alpha16(RENDERLIST4DV2_PTR rend_list, 
                                            UCHAR *video_buffer, 
                                            int lpitch,
                                            UCHAR *zbuffer,
                                            int zpitch,
                                            int alpha_override);


// non zbuffered versions
void Draw_Textured_Triangle2_16(POLYF4DV2_PTR face, // ptr to face
                                UCHAR *_dest_buffer,  // pointer to video buffer
                                int mem_pitch);       // bytes per line, 320, 640 etc.


void Draw_Textured_Bilerp_Triangle_16(POLYF4DV2_PTR face, // ptr to face
                                      UCHAR *_dest_buffer,  // pointer to video buffer
                                      int mem_pitch);       // bytes per line, 320, 640 etc.

void Draw_Textured_TriangleFS2_16(POLYF4DV2_PTR face, // ptr to face
                                  UCHAR *_dest_buffer, // pointer to video buffer
                                  int mem_pitch);      // bytes per line, 320, 640 etc.


void Draw_Triangle_2D3_16(POLYF4DV2_PTR face,  // ptr to face
                          UCHAR *_dest_buffer,// pointer to video buffer
                          int mem_pitch);     // bytes per line, 320, 640 etc.


void Draw_Gouraud_Triangle2_16(POLYF4DV2_PTR face, // ptr to face
                               UCHAR *_dest_buffer, // pointer to video buffer
                               int mem_pitch);      // bytes per line, 320, 640 etc.



void Draw_Textured_TriangleGS_16(POLYF4DV2_PTR face,  // ptr to face
                                 UCHAR *_dest_buffer, // pointer to video buffer
                                 int mem_pitch);      // bytes per line, 320, 640 etc.

void Draw_Textured_Perspective_Triangle_16(POLYF4DV2_PTR face,  // ptr to face
                                           UCHAR *_dest_buffer, // pointer to video buffer
                                           int mem_pitch);      // bytes per line, 320, 640 etc.


void Draw_Textured_PerspectiveLP_Triangle_16(POLYF4DV2_PTR face,  // ptr to face
                                             UCHAR *_dest_buffer, // pointer to video buffer
                                             int mem_pitch);      // bytes per line, 320, 640 etc.

void Draw_Textured_Perspective_Triangle_FS_16(POLYF4DV2_PTR face,  // ptr to face
                                              UCHAR *_dest_buffer, // pointer to video buffer
                                              int mem_pitch);      // bytes per line, 320, 640 etc.

void Draw_Textured_PerspectiveLP_Triangle_FS_16(POLYF4DV2_PTR face,  // ptr to face
                                                UCHAR *_dest_buffer, // pointer to video buffer
                                                int mem_pitch);      // bytes per line, 320, 640 etc.


void Draw_RENDERLIST4DV2_Solid2_16(RENDERLIST4DV2_PTR rend_list, 
                                   UCHAR *video_buffer, 
                                   int lpitch);


// alpha versions, non-z buffered
int RGB_Alpha_Table_Builder(int num_alpha_levels,   // number of levels to create
                            USHORT rgb_alpha_table[NUM_ALPHA_LEVELS][65536]); // lookup table


void Draw_Triangle_2D_Alpha16(POLYF4DV2_PTR face,  // ptr to face
                              UCHAR *_dest_buffer, // pointer to video buffer
                              int mem_pitch,       // bytes per line, 320, 640 etc.
                              int alpha);
void Draw_Textured_Triangle_Alpha16(POLYF4DV2_PTR face,  // ptr to face
                                    UCHAR *_dest_buffer,  // pointer to video buffer
                                    int mem_pitch,        // bytes per line, 320, 640 etc.
                                    int alpha);

void Draw_Textured_TriangleFS_Alpha16(POLYF4DV2_PTR face, // ptr to face
                                      UCHAR *_dest_buffer,  // pointer to video buffer
                                      int mem_pitch,        // bytes per line, 320, 640 etc.
                                      int alpha);


void Draw_Textured_TriangleGS_Alpha16(POLYF4DV2_PTR face,  // ptr to face
                                      UCHAR *_dest_buffer, // pointer to video buffer
                                      int mem_pitch,       // bytes per line, 320, 640 etc.
                                      int alpha);


void Draw_Gouraud_Triangle_Alpha16(POLYF4DV2_PTR face,   // ptr to face
                                   UCHAR *_dest_buffer, // pointer to video buffer
                                   int mem_pitch,       // bytes per line, 320, 640 etc.
                                   int alpha);


void Draw_RENDERLIST4DV2_Solid_Alpha16(RENDERLIST4DV2_PTR rend_list, 
                                       UCHAR *video_buffer, 
                                       int lpitch, 
                                       int alpha_override);


// new rendering function that works for everything!!! yaaa!!!!!
void Draw_RENDERLIST4DV2_RENDERCONTEXTV1_16(RENDERCONTEXTV1_PTR rc);

void Transform_RENDERLIST4DV2(RENDERLIST4DV2_PTR rend_list, // render list to transform
                              MATRIX4X4_PTR mt,   // transformation matrix
                              int coord_select);   // selects coords to transform

// new model functions 
int Load_OBJECT4DV2_COB2(OBJECT4DV2_PTR obj, // pointer to object
                         char *filename,     // filename of Caligari COB file
                         VECTOR4D_PTR scale, // initial scaling factors
                         VECTOR4D_PTR pos,   // initial position
                         VECTOR4D_PTR rot,   // initial rotations
                         int vertex_flags,   // flags to re-order vertices 
                         // and perform transforms
                         int mipmap=0);      // mipmap enable flag
// 0 means no mipmap, 1 means
// generate mip map

int Generate_Terrain2_OBJECT4DV2(OBJECT4DV2_PTR obj,   // pointer to object
                                 float twidth,          // width in world coords on x-axis
                                 float theight,         // height (length) in world coords on z-axis
                                 float vscale,          // vertical scale of terrain
                                 char *height_map_file, // filename of height bitmap encoded in 256 colors
                                 char *texture_map_file,// filename of texture map
                                 int rgbcolor,          // color of terrain if no texture        
                                 VECTOR4D_PTR pos,      // initial position
                                 VECTOR4D_PTR rot,      // initial rotations
                                 int poly_attr,         // the shading attributes we would like
                                 float sea_level=-1,    // height of sea level
                                 int alpha=-1);         // alpha level for sea polygons 

// new directdraw functions
int DDraw_Init2(int width, int height, int bpp, int windowed, int backbuffer_enable = 1);
int DDraw_Flip2(void);

// mip mapping functions
int Generate_Mipmaps(BITMAP_IMAGE_PTR source, BITMAP_IMAGE_PTR *mipmaps, float gamma = 1.01);
int Delete_Mipmaps(BITMAP_IMAGE_PTR *mipmaps, int leave_level_0);

#endif
