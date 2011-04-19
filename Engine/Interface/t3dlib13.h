// T3DLIB13.H - header file for T3DLIB13.H

// watch for multiple inclusions
#ifndef T3DLIB13
#define T3DLIB13

// DEFINES //////////////////////////////////////////////////////////////////

#define MD2_MAGIC_NUM       (('I') + ('D' << 8) + ('P' << 16) + ('2' << 24))
#define MD2_VERSION         8 

#define MD2_DEBUG           0  // 0 - minimal output when loading files
// 1 - the farm
// md2 animation defines
#define NUM_MD2_ANIMATIONS  20 // total number of MD2 animations
#define MD2_MAX_FRAMES     198 // total number of MD2 frames, but I have seen 199, and 200

// md2 animation states
#define MD2_ANIM_STATE_STANDING_IDLE      0  // model is standing and idling
#define MD2_ANIM_STATE_RUN                1  // model is running
#define MD2_ANIM_STATE_ATTACK             2  // model is firing weapon/attacking
#define MD2_ANIM_STATE_PAIN_1             3  // model is being hit version 1
#define MD2_ANIM_STATE_PAIN_2             4  // model is being hit version 2
#define MD2_ANIM_STATE_PAIN_3             5  // model is being hit version 3
#define MD2_ANIM_STATE_JUMP               6  // model is jumping
#define MD2_ANIM_STATE_FLIP               7  // model is using hand gestures :)
#define MD2_ANIM_STATE_SALUTE             8  // model is saluting
#define MD2_ANIM_STATE_TAUNT              9  // model is taunting
#define MD2_ANIM_STATE_WAVE               10 // model is waving at someone
#define MD2_ANIM_STATE_POINT              11 // model is pointing at someone
#define MD2_ANIM_STATE_CROUCH_STAND       12 // model is crouching and idling
#define MD2_ANIM_STATE_CROUCH_WALK        13 // model is walking while crouching
#define MD2_ANIM_STATE_CROUCH_ATTACK      14 // model is firing weapon/attacking with crouching
#define MD2_ANIM_STATE_CROUCH_PAIN        15 // model is being hit while crouching
#define MD2_ANIM_STATE_CROUCH_DEATH       16 // model is dying while crouching
#define MD2_ANIM_STATE_DEATH_BACK         17 // model is dying while falling backward
#define MD2_ANIM_STATE_DEATH_FORWARD      18 // model is dying while falling forward
#define MD2_ANIM_STATE_DEATH_SLOW         19 // model is dying slowly (any direction)

// modes to run animation
#define MD2_ANIM_LOOP                        0 // play animation over and over
#define MD2_ANIM_SINGLE_SHOT                 1 // single shot animation

// note: the idle animations are sometimes used for other things like salutes, hand motions, etc.



// TYPES ///////////////////////////////////////////////////////////////////

// this if the header structure for a Quake II .MD2 file by id Software,
// I have slightly annotated the field names to make them more readable 
// and less cryptic :)
typedef struct MD2_HEADER_TYP
{

    int identifier;     // identifies the file type, should be "IDP2"
    int version;        // version number, should be 8
    int skin_width;     // width of texture map used for skinning
    int skin_height;    // height of texture map using for skinning
    int framesize;      // number of bytes in a single frame of animation
    int num_skins;      // total number of skins
    // listed by ASCII filename and are available 
    // for loading if files are found in full path
    int num_verts;      // number of vertices in each model frame, the 
    // number of vertices in each frame is always the
    // same
    int num_textcoords; // total number of texture coordinates in entire file 
    // may be larger than the number of vertices
    int num_polys;      // number of polygons per model, or per frame of 
    // animation if you will
    int num_openGLcmds; // number of openGL commands which can help with
    // rendering optimization
    // however, we won't be using them

    int num_frames;     // total number of animation frames

    // memory byte offsets to actual data for each item

    int offset_skins;      // offset in bytes from beginning of file to the 
    // skin array that holds the file name for each skin, 
    // each file name record is 64 bytes
    int offset_textcoords; // offset in bytes from the beginning of file to the  
    // texture coordinate array

    int offset_polys;      // offset in bytes from beginning of file to the 
    // polygon mesh

    int offset_frames;     // offset in bytes from beginning of file to the 
    // vertex data for each frame

    int offset_openGLcmds; // offset in bytes from beginning of file to the 
    // openGL commands

    int offset_end;        // offset in bytes from beginning of file to end of file

} MD2_HEADER, *MD2_HEADER_PTR; 


// this is a single point for the md2 model, contains an 8-bit scaled x,y,z 
// and an index for the normal to the point that is encoded as an
// index into a normal table supplied by id software
typedef struct MD2_POINT_TYP
{

    unsigned char v[3];          // vertex x,y,z in compressed byte format
    unsigned char normal_index;  // index into normal table from id Software (unused)

} MD2_POINT, *MD2_POINT_PTR; 

// this is a single u,v texture
typedef struct MD2_TEXTCOORD_TYP
{
    short u,v; 
} MD2_TEXTCOORD, *MD2_TEXTCOORD_PTR;

// this is a single frame for the md2 format, it has a small header portion 
// which describes how to scale and translate the model vertices, but then
// has an array to the actual data points which is variable, so the definition
// uses a single unit array to allow the compiler to address up to n with
// array syntax
typedef struct MD2_FRAME_TYP
{

    float scale[3];          // x,y,z scaling factors for the frame vertices
    float translate[3];      // x,y,z translation factors for the frame vertices
    char name[16];           // ASCII name of the model, "evil death lord" etc. :)

    MD2_POINT vlist[1];      // beginning of vertex storage array

} MD2_FRAME, *MD2_FRAME_PTR;

// this is a data structure for a single md2 polygon (triangle)
// it's composed of 3 vertices and 3 texture coordinates, both
// indices
typedef struct MD2_POLY_TYP
{

    unsigned short vindex[3];   // vertex indices 
    unsigned short tindex[3];   // texture indices

} MD2_POLY, *MD2_POLY_PTR;

// finally, we will create a container class to hold the md2 model and help
// with animation etc. later, but this model will be converted on the fly
// to one of our object models each frame, once the md2 model is loaded,
// is is converted to the container format and the original md2 model is 
// discarded..
typedef struct MD2_CONTAINER_TYP
{
    int state;          // state of the model
    int attr;           // attributes of the model
    int color;          // base color if no texture
    int num_frames;     // number of frames in the model
    int num_polys;      // number of polygons
    int num_verts;      // number of vertices 
    int num_textcoords; // number of texture coordinates

    BITMAP_IMAGE_PTR  skin;   // pointer to texture skin for model

    MD2_POLY_PTR polys; // pointer to polygon list
    VECTOR3D_PTR vlist; // pointer to vertex coordinate list
    VECTOR2D_PTR tlist; // pointer to texture coordinate list

    VECTOR4D world_pos; // position of object
    VECTOR4D vel;       // velocity of object

    int   ivars[8];     // integer variables
    float fvars[8];     // floating point variables
    int counters[8];    // general counters      

    int anim_state;     // state of animation
    int anim_counter;   // general animation counter
    int anim_mode;      // single shot, loop, etc.
    int anim_speed;     // smaller number, faster animation
    int anim_complete;  // flags if single shot animation is done
    float curr_frame;     // current frame in animation 

} MD2_CONTAINER, *MD2_CONTAINER_PTR; 

///////////////////////////////////////////////////////////////////////////////

// holds a simple animation start-end plus interpolation rate
typedef struct MD2_ANIMATION_TYP
{
    int   start_frame;  // starting and ending frame
    int   end_frame;  
    float irate;        // interpolation rate
    int   anim_speed;   // animation rate 

} MD2_ANIMATION, *MD2_ANIMATION_PTR;


// CLASSES /////////////////////////////////////////////////////////////////


// EXTERNALS ///////////////////////////////////////////////////////////////

extern HWND main_window_handle; // save the window handle
extern HINSTANCE main_instance; // save the instance
extern MD2_ANIMATION md2_animations[NUM_MD2_ANIMATIONS];
extern char *md2_anim_strings[];

// MACROS //////////////////////////////////////////////////////////////////

// PROTOTYPES //////////////////////////////////////////////////////////////


int Load_Object_MD2(MD2_CONTAINER_PTR obj_md2, // the loaded md2 file placed in container
                    char *modelfile,    // the filename of the .MD2 model
                    VECTOR4D_PTR scale, // initial scaling factors
                    VECTOR4D_PTR pos,   // initial position
                    VECTOR4D_PTR rot,   // initial rotations
                    char *texturefile,  // the texture filename for the model
                    int attr,           // the lighting/model attributes for the model
                    int color,          // base color if no texturing
                    int vertex_flags);  // control ordering etc.



int Prepare_OBJECT4DV2_For_MD2(OBJECT4DV2_PTR obj,        // pointer to destination object
                               MD2_CONTAINER_PTR obj_md2); // md2 object to extract frame from


int Extract_MD2_Frame(OBJECT4DV2_PTR obj,        // pointer to destination object
                      MD2_CONTAINER_PTR obj_md2); // md2 object to extract frame from


int Set_Animation_MD2(MD2_CONTAINER_PTR md2_obj, 
                      int anim_state,
                      int anim_mode);

int Animate_MD2(MD2_CONTAINER_PTR md2_obj);



// timer functions
void Start_Fast_Timer(void);
void Wait_Fast_Timer(int delay);

#endif
