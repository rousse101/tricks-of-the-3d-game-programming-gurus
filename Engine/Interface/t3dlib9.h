// T3DLIB9.H - header file for T3DLIB9.H

// watch for multiple inclusions
#ifndef T3DLIB9
#define T3DLIB9

// DEFINES //////////////////////////////////////////////////////////////////

// defines for zbuffer
#define ZBUFFER_ATTR_16BIT   16
#define ZBUFFER_ATTR_32BIT   32 

// TYPES ///////////////////////////////////////////////////////////////////

// structure for zbuffer
typedef struct ZBUFFERV1_TYP
{
    int attr;       // attributes of zbuffer
    UCHAR *zbuffer; // ptr to storage
    int width;      // width in zpixels
    int height;     // height in zpixels
    int sizeq;      // total size in QUADs
    // of zbuffer
} ZBUFFERV1, *ZBUFFERV1_PTR;

// CLASSES /////////////////////////////////////////////////////////////////


// MACROS ///////////////////////////////////////////////////////////////////



// TYPES ///////////////////////////////////////////////////////////////////

// EXTERNALS ///////////////////////////////////////////////////////////////


// PROTOTYPES //////////////////////////////////////////////////////////////

void Draw_Textured_TriangleZB16(POLYF4DV2_PTR face,  // ptr to face
                                UCHAR *_dest_buffer,  // pointer to video buffer
                                int mem_pitch,        // bytes per line, 320, 640 etc.
                                UCHAR *zbuffer,       // pointer to z-buffer
                                int zpitch);          // bytes per line of zbuffer

void Draw_Textured_TriangleFSZB16(POLYF4DV2_PTR face, // ptr to face
                                  UCHAR *_dest_buffer,  // pointer to video buffer
                                  int mem_pitch,        // bytes per line, 320, 640 etc.
                                  UCHAR *zbuffer,       // pointer to z-buffer
                                  int zpitch);          // bytes per line of zbuffer


void Draw_Triangle_2DZB_16(POLYF4DV2_PTR face, // ptr to face
                           UCHAR *_dest_buffer,  // pointer to video buffer
                           int mem_pitch,        // bytes per line, 320, 640 etc.
                           UCHAR *zbuffer,       // pointer to z-buffer
                           int zpitch);          // bytes per line of zbuffer


void Draw_Gouraud_TriangleZB16(POLYF4DV2_PTR face,   // ptr to face
                               UCHAR *_dest_buffer, // pointer to video buffer
                               int mem_pitch,      // bytes per line, 320, 640 etc.
                               UCHAR *zbuffer,    // pointer to z-buffer
                               int zpitch);       // bytes per line of zbuffer


void Draw_RENDERLIST4DV2_SolidZB16(RENDERLIST4DV2_PTR rend_list, 
                                   UCHAR *video_buffer, 
                                   int lpitch,
                                   UCHAR *zbuffer,
                                   int zpitch);

int Create_Zbuffer(ZBUFFERV1_PTR zb, // pointer to a zbuffer object
                   int width,      // width 
                   int height,     // height
                   int attr);       // attributes of zbuffer

int Delete_Zbuffer(ZBUFFERV1_PTR zb);

void Clear_Zbuffer(ZBUFFERV1_PTR zb, UINT data);

#endif


