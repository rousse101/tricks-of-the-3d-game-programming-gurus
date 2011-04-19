// T3DLIB12.H - header file for T3DLIB12.H

// watch for multiple inclusions
#ifndef T3DLIB12
#define T3DLIB12

// DEFINES //////////////////////////////////////////////////////////////////


// TYPES ///////////////////////////////////////////////////////////////////


// CLASSES /////////////////////////////////////////////////////////////////


// M A C R O S ///////////////////////////////////////////////////////////////



// EXTERNALS ///////////////////////////////////////////////////////////////

extern HWND main_window_handle; // save the window handle
extern HINSTANCE main_instance; // save the instance


// MACROS ////////////////////////////////////////////////


// PROTOTYPES //////////////////////////////////////////////////////////////


// new rendering context function that calls the transparent rasterizers                             
void Draw_RENDERLIST4DV2_RENDERCONTEXTV1_16_3(RENDERCONTEXTV1_PTR rc);


// normal z-buffered, with write thru z and transparency support on textures
void Draw_Textured_TriangleGSWTZB2_16(POLYF4DV2_PTR face, // ptr to face
                                      UCHAR *_dest_buffer,     // pointer to video buffer
                                      int mem_pitch,           // bytes per line, 320, 640 etc.
                                      UCHAR *_zbuffer,         // pointer to z-buffer
                                      int zpitch);             // bytes per line of zbuffer

void Draw_Textured_TriangleFSWTZB3_16(POLYF4DV2_PTR face, // ptr to face
                                      UCHAR *_dest_buffer,     // pointer to video buffer
                                      int mem_pitch,           // bytes per line, 320, 640 etc.
                                      UCHAR *_zbuffer,         // pointer to z-buffer
                                      int zpitch);             // bytes per line of zbuffer


void Draw_Textured_TriangleWTZB3_16(POLYF4DV2_PTR face,  // ptr to face
                                    UCHAR *_dest_buffer,       // pointer to video buffer
                                    int mem_pitch,             // bytes per line, 320, 640 etc.
                                    UCHAR *_zbuffer,           // pointer to z-buffer
                                    int zpitch);               // bytes per line of zbuffer

// normal z-buffered, and transparency support on textures

void Draw_Textured_TriangleZB3_16(POLYF4DV2_PTR face,  // ptr to face
                                  UCHAR *_dest_buffer,  // pointer to video buffer
                                  int mem_pitch,        // bytes per line, 320, 640 etc.
                                  UCHAR *_zbuffer,       // pointer to z-buffer
                                  int zpitch);          // bytes per line of zbuffer


// normal z-buffered, with write thru z and transparency support on textures and alpha
void Draw_Textured_TriangleWTZB_Alpha16_2(POLYF4DV2_PTR face,  // ptr to face
                                          UCHAR *_dest_buffer,             // pointer to video buffer
                                          int mem_pitch,                   // bytes per line, 320, 640 etc.
                                          UCHAR *_zbuffer,                 // pointer to z-buffer
                                          int zpitch,                      // bytes per line of zbuffer
                                          int alpha);

void Draw_Textured_TriangleZB_Alpha16_2(POLYF4DV2_PTR face,  // ptr to face
                                        UCHAR *_dest_buffer,           // pointer to video buffer
                                        int mem_pitch,                 // bytes per line, 320, 640 etc.
                                        UCHAR *_zbuffer,               // pointer to z-buffer
                                        int zpitch,                    // bytes per line of zbuffer
                                        int alpha);

void Draw_Textured_Triangle_Alpha16_2(POLYF4DV2_PTR face,   // ptr to face
                                      UCHAR *_dest_buffer,        // pointer to video buffer
                                      int mem_pitch, int alpha);  // bytes per line, 320, 640 etc.



#endif
