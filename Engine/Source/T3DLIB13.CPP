// T3DLIB13.CPP - Quake md2 model software, motion and collision

// I N C L U D E S ///////////////////////////////////////////////////////////

#define DEBUG_ON

#define WIN32_LEAN_AND_MEAN  

#include "CommonHeader.h"

#include <ddraw.h>      // needed for defs in T3DLIB1.H 
#include "T3DLIB1.H"
#include "T3DLIB4.H"
#include "T3DLIB5.H"
#include "T3DLIB6.H"
#include "T3DLIB7.H"
#include "T3DLIB8.H"
#include "T3DLIB9.H"
#include "T3DLIB10.H"
#include "T3DLIB11.H"
#include "T3DLIB12.H"
#include "T3DLIB13.H"

// DEFINES //////////////////////////////////////////////////////////////////

// EXTERNALS /////////////////////////////////////////////

extern HWND main_window_handle; // save the window handle
extern HINSTANCE main_instance; // save the instance

// GLOBALS //////////////////////////////////////////////////////////////////

// high speed timing functions
INT64  lpFrequency;
INT64  lpPerformanceCount;
INT64  ticks_per_ms;

// the animations, the frames, and timing look good on most
// models, but tweak them to suit your needs OR create a table
// for every character in your game to fine tune your animation
MD2_ANIMATION md2_animations[NUM_MD2_ANIMATIONS]  = 
{
    // format: start frame (0..197), end frame (0..197), 
    // interpolation rate (0..1, 1 for no interpolation), 
    // speed (0..10, 0 fastest, 1 fast, 2 medium, 3 slow...)

    {0,39,0.5,1},    // MD2_ANIM_STATE_STANDING_IDLE       0
    {40,45,0.5,2},   // MD2_ANIM_STATE_RUN                 1
    {46,53,0.5,1},   // MD2_ANIM_STATE_ATTACK              2 
    {54,57,0.5,1},   // MD2_ANIM_STATE_PAIN_1              3
    {58,61,0.5,1},   // MD2_ANIM_STATE_PAIN_2              4
    {62,65,0.5,1},   // MD2_ANIM_STATE_PAIN_3              5
    {66,71,0.5,1},   // MD2_ANIM_STATE_JUMP                6
    {72,83,0.5,1},   // MD2_ANIM_STATE_FLIP                7
    {84,94,0.5,1},   // MD2_ANIM_STATE_SALUTE              8
    {95,111,0.5,1},  // MD2_ANIM_STATE_TAUNT               9
    {112,122,0.5,1}, // MD2_ANIM_STATE_WAVE                10
    {123,134,0.5,1}, // MD2_ANIM_STATE_POINT               11 
    {135,153,0.5,1}, // MD2_ANIM_STATE_CROUCH_STAND        12 
    {154,159,0.5,1}, // MD2_ANIM_STATE_CROUCH_WALK         13
    {160,168,0.5,1}, // MD2_ANIM_STATE_CROUCH_ATTACK       14
    {169,172,0.5,1}, // MD2_ANIM_STATE_CROUCH_PAIN         15
    {173,177,0.25,0}, // MD2_ANIM_STATE_CROUCH_DEATH        16  
    {178,183,0.25,0}, // MD2_ANIM_STATE_DEATH_BACK          17
    {184,189,0.25,0}, // MD2_ANIM_STATE_DEATH_FORWARD       18
    {190,197,0.25,0}, // MD2_ANIM_STATE_DEATH_SLOW          19
};

// ASCII names for debugging
char *md2_anim_strings[] = 
{
    "MD2_ANIM_STATE_STANDING_IDLE ",
    "MD2_ANIM_STATE_RUN",
    "MD2_ANIM_STATE_ATTACK", 
    "MD2_ANIM_STATE_PAIN_1",
    "MD2_ANIM_STATE_PAIN_2",
    "MD2_ANIM_STATE_PAIN_3",
    "MD2_ANIM_STATE_JUMP",
    "MD2_ANIM_STATE_FLIP",
    "MD2_ANIM_STATE_SALUTE",
    "MD2_ANIM_STATE_TAUNT",
    "MD2_ANIM_STATE_WAVE",
    "MD2_ANIM_STATE_POINT",       
    "MD2_ANIM_STATE_CROUCH_STAND", 
    "MD2_ANIM_STATE_CROUCH_WALK ",
    "MD2_ANIM_STATE_CROUCH_ATTACK",
    "MD2_ANIM_STATE_CROUCH_PAIN",
    "MD2_ANIM_STATE_CROUCH_DEATH",  
    "MD2_ANIM_STATE_DEATH_BACK",
    "MD2_ANIM_STATE_DEATH_FORWARD",
    "MD2_ANIM_STATE_DEATH_SLOW",
};


// FUNCTIONS ////////////////////////////////////////////////////////////////

int Set_Animation_MD2(MD2_CONTAINER_PTR md2_obj,     // md2 object 
                      int anim_state,                // which animation to play
                      int anim_mode = MD2_ANIM_LOOP) // mode of animation single/loop
{
    // this function initializes an animation for play back
    md2_obj->anim_state   = anim_state; 
    md2_obj->anim_counter = 0;    
    md2_obj->anim_speed   = md2_animations[anim_state].anim_speed;
    md2_obj->anim_mode    = anim_mode; 

    // set initial frame 
    md2_obj->curr_frame   = md2_animations[anim_state].start_frame;

    // set animation complete flag
    md2_obj->anim_complete = 0;

    // return success
    return(1);

} // end Set_Animation_MD2

///////////////////////////////////////////////////////////////////////////////

int Animate_MD2(MD2_CONTAINER_PTR md2_obj) // md2 object
{
    // animate the mesh to next frame based on state and interpolation values...

    // update animation counter
    if (++md2_obj->anim_counter >= md2_obj->anim_speed)
    {
        // reset counter
        md2_obj->anim_counter = 0;

        // animate mesh with interpolation, algorithm is straightforward interpolate
        // from current frame in animation to next and blend vertex positions based  
        // on interpolant ivalue in the md2_container class, couple tricky parts
        // are to watch out for the endpost values of the animation, etc.
        // if the interpolation rate irate=1.0 then there is no interpolation
        // for all intent purposes...

        // add interpolation rate to interpolation value, test for next frame
        md2_obj->curr_frame+=md2_animations[md2_obj->anim_state].irate;

        // test if sequence is complete?
        if (md2_obj->curr_frame > md2_animations[md2_obj->anim_state].end_frame)
        {
            // test for one shot, if so then reset to last frame, if loop, the loop
            if (md2_obj->anim_mode == MD2_ANIM_LOOP)
            {
                // loop animation back to starting frame
                md2_obj->curr_frame = md2_animations[md2_obj->anim_state].start_frame;
            } // end if
            else
            {
                // MD2_ANIM_SINGLE_SHOT
                md2_obj->curr_frame = md2_animations[md2_obj->anim_state].end_frame; 

                // set complete flag incase outside wants to take action
                md2_obj->anim_complete = 1;
            } // end else

        } // end if sequence complete

    } // end if time to animate

    // return success
    return(1);

} // end Animate_MD2

///////////////////////////////////////////////////////////////////////////////

int Extract_MD2_Frame(OBJECT4DV2_PTR obj,        // pointer to destination object
                      MD2_CONTAINER_PTR obj_md2) // md2 object to extract frame from
{
    // this function extracts a single frame of animation from the md2 container and stores
    // in into the object4dv2 container, this is so we can leverage the library to transform,
    // light, etc. the mesh rather than writing new functions, granted the process of extraction
    // is an unneeded overhead, but since we will only have a few MD2 object running around 
    // the rendering swamps the time to extract by many orders of magnitude, so the extraction
    // is negligable
    // the function also interpolates frames; if the curr_frame value is non-intergral then
    // the function will blend frames together based on the decimal fraction 

    // test if frame number is greater than max frames allowable, some models are malformed
    int frame_0,   // starting frame to interpolate
        frame_1;   // ending frame to interpolate


    // step 1: decide if this is an interpolated frame?

    float ivalue = obj_md2->curr_frame - (int)obj_md2->curr_frame;

    // test for integer?
    if (ivalue == 0.0)
    {
        // single frame, no interpolation
        frame_0  = obj_md2->curr_frame;

        // check for overflow?
        if (frame_0 >= obj_md2->num_frames)
            frame_0 = obj_md2->num_frames-1;

        // copy vertex list for selected frame, vertex list begins at
        // base index (obj_md2->num_verts * obj_md2->curr_frame)
        int base_index = obj_md2->num_verts * frame_0;

        // copy vertices now from base
        for (int vindex = 0; vindex < obj_md2->num_verts; vindex++)
        {
            // copy the vertex
            obj->vlist_local[vindex].x = obj_md2->vlist[vindex + base_index].x;
            obj->vlist_local[vindex].y = obj_md2->vlist[vindex + base_index].y;
            obj->vlist_local[vindex].z = obj_md2->vlist[vindex + base_index].z;
            obj->vlist_local[vindex].w = 1;  

            // every vertex has a point and texture attached, set that in the flags attribute
            SET_BIT(obj->vlist_local[vindex].attr, VERTEX4DTV1_ATTR_POINT);
            SET_BIT(obj->vlist_local[vindex].attr, VERTEX4DTV1_ATTR_TEXTURE);

        } // end for vindex

    } // end if
    else
    {
        // interpolate between curr_frame and curr_frame+1 based
        // on ivalue
        frame_0  = obj_md2->curr_frame;
        frame_1  = obj_md2->curr_frame+1;

        // check for overflow?
        if (frame_0 >= obj_md2->num_frames)
            frame_0 = obj_md2->num_frames-1;

        // check for overflow?
        if (frame_1 >= obj_md2->num_frames)
            frame_1 = obj_md2->num_frames-1;

        // interpolate vertex lists for selected frame(s), vertex list(s) begin at
        // base index (obj_md2->num_verts * obj_md2->curr_frame)
        int base_index_0 = obj_md2->num_verts * frame_0;
        int base_index_1 = obj_md2->num_verts * frame_1;

        // interpolate vertices now from base frame 0,1
        for (int vindex = 0; vindex < obj_md2->num_verts; vindex++)
        {
            // interpolate the vertices
            obj->vlist_local[vindex].x = ((1-ivalue)*obj_md2->vlist[vindex + base_index_0].x + ivalue*obj_md2->vlist[vindex + base_index_1].x);
            obj->vlist_local[vindex].y = ((1-ivalue)*obj_md2->vlist[vindex + base_index_0].y + ivalue*obj_md2->vlist[vindex + base_index_1].y);
            obj->vlist_local[vindex].z = ((1-ivalue)*obj_md2->vlist[vindex + base_index_0].z + ivalue*obj_md2->vlist[vindex + base_index_1].z);
            obj->vlist_local[vindex].w = 1;  

            // every vertex has a point and texture attached, set that in the flags attribute
            SET_BIT(obj->vlist_local[vindex].attr, VERTEX4DTV1_ATTR_POINT);
            SET_BIT(obj->vlist_local[vindex].attr, VERTEX4DTV1_ATTR_TEXTURE);
        } // end for vindex

    } // end if

    // return success
    return(1);

} // end Extract_MD2_Frame

///////////////////////////////////////////////////////////////////////////////

int Load_Object_MD2(MD2_CONTAINER_PTR obj_md2, // the loaded md2 file placed in container
                    char *modelfile,    // the filename of the .MD2 model
                    VECTOR4D_PTR scale, // initial scaling factors
                    VECTOR4D_PTR pos,   // initial position
                    VECTOR4D_PTR rot,   // initial rotations (not implemented)
                    char *texturefile,  // the texture filename for the model
                    int attr,           // the lighting/model attributes for the model
                    int color,          // base color if no texturing
                    int vertex_flags)   // control ordering etc.
{
    // this function loads in an md2 file, extracts all the data and stores it in the 
    // container class which will be used later to load frames into a the standard object
    // type for rendering on the fly

    FILE *fp      = NULL;  // file pointer to model
    int   flength = -1;    // general file length
    UCHAR *buffer = NULL;  // used to buffer md2 file data

    MD2_HEADER_PTR md2_header;   // pointer to the md2 header

    // begin by loading in the .md2 model file
    if ((fp = fopen(modelfile, "rb"))==NULL)
    {
        Write_Error("\nLoad_Object_MD2 - couldn't find file %s", modelfile);
        return(0);
    } // end if

    // find the length of the model file
    // seek to end of file
    fseek(fp, 0, SEEK_END);

    // where is the file pointer?
    flength = ftell(fp);

    // now read the md2 file into a buffer to analyze it

    // re-position file pointer to beginning of file
    fseek(fp, 0, SEEK_SET);

    // allocate memory to hold file
    buffer = (UCHAR *)malloc(flength+1);

    // load data into buffer
    int bytes_read = fread(buffer, sizeof(UCHAR), flength, fp);

    // the header is the first item in the buffer, so alias a pointer
    // to it, so we can start analyzing it and creating the model
    md2_header = (MD2_HEADER_PTR)buffer;

    Write_Error("\nint identifier        = %d", md2_header->identifier);
    Write_Error("\nint version           = %d", md2_header->version);
    Write_Error("\nint skin_width        = %d", md2_header->skin_width);
    Write_Error("\nint skin_height       = %d", md2_header->skin_height);
    Write_Error("\nint framesize         = %d", md2_header->framesize);
    Write_Error("\nint num_skins         = %d", md2_header->num_skins);
    Write_Error("\nint num_verts         = %d", md2_header->num_verts);
    Write_Error("\nint num_textcoords    = %d", md2_header->num_textcoords);
    Write_Error("\nint num_polys         = %d", md2_header->num_polys);
    Write_Error("\nint num_openGLcmds    = %d", md2_header->num_openGLcmds);
    Write_Error("\nint num_frames        = %d", md2_header->num_frames);
    Write_Error("\nint offset_skins      = %d", md2_header->offset_skins);
    Write_Error("\nint offset_textcoords = %d", md2_header->offset_textcoords);
    Write_Error("\nint offset_polys      = %d", md2_header->offset_polys);
    Write_Error("\nint offset_frames     = %d", md2_header->offset_frames);
    Write_Error("\nint offset_openGLcmds = %d", md2_header->offset_openGLcmds);
    Write_Error("\nint offset_end        = %d", md2_header->offset_end);

    // test for valid file
    if (md2_header->identifier != MD2_MAGIC_NUM || md2_header->version != MD2_VERSION)
    {
        fclose(fp);
        return(0);
    } // end if

    // assign fields to container class
    obj_md2->state          = 0;                          // state of the model
    obj_md2->attr           = attr;                       // attributes of the model
    obj_md2->color          = color;                      // 
    obj_md2->num_frames     = md2_header->num_frames;     // number of frames in the model
    obj_md2->num_polys      = md2_header->num_polys;      // number of polygons
    obj_md2->num_verts      = md2_header->num_verts;      // number of vertices 
    obj_md2->num_textcoords = md2_header->num_textcoords; // number of texture coordinates
    obj_md2->curr_frame     = 0;                          // current frame in animation
    obj_md2->skin           = NULL;                       // pointer to texture skin for model
    obj_md2->world_pos      = *pos;                       // position object in world

    // allocate memory for mesh data
    obj_md2->polys = (MD2_POLY_PTR)malloc(md2_header->num_polys      * sizeof(MD2_POLY)); // pointer to polygon list
    obj_md2->vlist = (VECTOR3D_PTR)malloc(md2_header->num_frames     * md2_header->num_verts * sizeof(VECTOR3D)); // pointer to vertex coordinate list
    obj_md2->tlist = (VECTOR2D_PTR)malloc(md2_header->num_textcoords * sizeof(VECTOR2D)); // pointer to texture coordinate list

#if (MD2_DEBUG==1)
    Write_Error("\nTexture Coordinates:");
#endif

    for (int tindex = 0; tindex < md2_header->num_textcoords; tindex++)
    {
#if (MD2_DEBUG==1)
        Write_Error("\ntextcoord[%d] = (%d, %d)", tindex,
            ((MD2_TEXTCOORD_PTR)(buffer+md2_header->offset_textcoords))[tindex].u, 
            ((MD2_TEXTCOORD_PTR)(buffer+md2_header->offset_textcoords))[tindex].v);
#endif    
        // insert texture coordinate into storage container
        obj_md2->tlist[tindex].x = ((MD2_TEXTCOORD_PTR)(buffer+md2_header->offset_textcoords))[tindex].u;
        obj_md2->tlist[tindex].y = ((MD2_TEXTCOORD_PTR)(buffer+md2_header->offset_textcoords))[tindex].v;
    } // end for vindex

#if (MD2_DEBUG==1)
    Write_Error("\nVertex List:");
#endif

    for (int findex = 0; findex < md2_header->num_frames; findex++)
    {
#if (MD2_DEBUG==1)
        Write_Error("\n\n******************************************************************************");
        Write_Error("\n\nF R A M E # %d", findex);
        Write_Error("\n\n******************************************************************************\n");
#endif

        MD2_FRAME_PTR frame_ptr = (MD2_FRAME_PTR)(buffer + md2_header->offset_frames + md2_header->framesize * findex);

        // extract md2 scale and translate, additionally use sent scale and translate
        float sx = frame_ptr->scale[0],
            sy = frame_ptr->scale[1],
            sz = frame_ptr->scale[2],
            tx = frame_ptr->translate[0],  
            ty = frame_ptr->translate[1],  
            tz = frame_ptr->translate[2];

#if (MD2_DEBUG==1)    
        Write_Error("\nScale: (%f, %f, %f)\nTranslate: (%f, %f, %f)", sx, sy, sz, tx, ty, tz);  
#endif

        for (int vindex = 0; vindex < md2_header->num_verts; vindex++)
        {
            VECTOR3D v;

            // scale and translate compressed vertex
            v.x = (float)frame_ptr->vlist[vindex].v[0] * sx + tx;
            v.y = (float)frame_ptr->vlist[vindex].v[1] * sy + ty;
            v.z = (float)frame_ptr->vlist[vindex].v[2] * sz + tz;

            // scale final point based on sent data
            v.x = scale->x * v.x;
            v.y = scale->y * v.y;
            v.z = scale->z * v.z;

            float temp; // used for swaping

            // test for vertex modifications to winding order etc.
            if (vertex_flags & VERTEX_FLAGS_INVERT_X)
                v.x = -v.x;

            if (vertex_flags & VERTEX_FLAGS_INVERT_Y)
                v.y = -v.y;

            if (vertex_flags & VERTEX_FLAGS_INVERT_Z)
                v.z = -v.z;

            if (vertex_flags & VERTEX_FLAGS_SWAP_YZ)
                SWAP(v.y, v.z, temp);

            if (vertex_flags & VERTEX_FLAGS_SWAP_XZ)
                SWAP(v.x, v.z, temp);

            if (vertex_flags & VERTEX_FLAGS_SWAP_XY)
                SWAP(v.x, v.y, temp);

#if (MD2_DEBUG==1)
            Write_Error("\nVertex #%d = (%f, %f, %f)", vindex, v.x, v.y, v.z);
#endif
            // insert vertex into vertex list which is laid out frame 0, frame 1,..., frame n 
            // frame i: vertex 0, vertex 1,....vertex j
            obj_md2->vlist[vindex + (findex * obj_md2->num_verts)] = v;
        } // end vindex

    } // end findex

#if (MD2_DEBUG==1)
    Write_Error("\nPolygon List:");
#endif

    MD2_POLY_PTR poly_ptr = (MD2_POLY_PTR)(buffer + md2_header->offset_polys);

    for (int pindex = 0; pindex < md2_header->num_polys; pindex++)
    {
        // insert polygon into polygon list in container
        if (vertex_flags & VERTEX_FLAGS_INVERT_WINDING_ORDER)
        {
            // inverted winding order

            // vertices
            obj_md2->polys[pindex].vindex[0] = poly_ptr[pindex].vindex[2];
            obj_md2->polys[pindex].vindex[1] = poly_ptr[pindex].vindex[1];
            obj_md2->polys[pindex].vindex[2] = poly_ptr[pindex].vindex[0];

            // texture coordinates
            obj_md2->polys[pindex].tindex[0] = poly_ptr[pindex].tindex[2];
            obj_md2->polys[pindex].tindex[1] = poly_ptr[pindex].tindex[1];
            obj_md2->polys[pindex].tindex[2] = poly_ptr[pindex].tindex[0];
        } // end if
        else
        {
            // normal winding order

            // vertices
            obj_md2->polys[pindex].vindex[0] = poly_ptr[pindex].vindex[0];
            obj_md2->polys[pindex].vindex[1] = poly_ptr[pindex].vindex[1];
            obj_md2->polys[pindex].vindex[2] = poly_ptr[pindex].vindex[2];

            // texture coordinates
            obj_md2->polys[pindex].tindex[0] = poly_ptr[pindex].tindex[0];
            obj_md2->polys[pindex].tindex[1] = poly_ptr[pindex].tindex[1];
            obj_md2->polys[pindex].tindex[2] = poly_ptr[pindex].tindex[2];

        } // end if

#if (MD2_DEBUG==1)
        Write_Error("\npoly %d: v(%d, %d, %d), t(%d, %d, %d)", pindex,
            obj_md2->polys[pindex].vindex[0], obj_md2->polys[pindex].vindex[1], obj_md2->polys[pindex].vindex[2],
            obj_md2->polys[pindex].tindex[0], obj_md2->polys[pindex].tindex[1], obj_md2->polys[pindex].tindex[2]);
#endif
    } // end for vindex

    // close the file
    fclose(fp);

    //////////////////////////////////////////////////////////////////////////////
    // load the texture from disk
    Load_Bitmap_File(&bitmap16bit, texturefile);

    // create a proper size and bitdepth bitmap
    obj_md2->skin = (BITMAP_IMAGE_PTR)malloc(sizeof(BITMAP_IMAGE));

    // initialize bitmap
    Create_Bitmap(obj_md2->skin,0,0,
        bitmap16bit.bitmapinfoheader.biWidth,
        bitmap16bit.bitmapinfoheader.biHeight,
        bitmap16bit.bitmapinfoheader.biBitCount);

    // load the bitmap image
    Load_Image_Bitmap16(obj_md2->skin, &bitmap16bit,0,0,BITMAP_EXTRACT_MODE_ABS);

    // done, so unload the bitmap
    Unload_Bitmap_File(&bitmap16bit);


    // finally release the memory for the temporary buffer
    if (buffer)
        free(buffer);

    // return success
    return(1);

} // end Load_Object_MD2

///////////////////////////////////////////////////////////////////////////////

int Prepare_OBJECT4DV2_For_MD2(OBJECT4DV2_PTR obj,        // pointer to destination object
                               MD2_CONTAINER_PTR obj_md2) // md2 object to extract frame from
{
    // this function prepares the OBJECT4DV2 to be used as a vessel to hold
    // frames from the md2 container, it allocated the memory needed, set fields
    // and pre-computes as much as possible since each new frame will change only
    // the vertex list

    Write_Error("\nPreparing MD2_CONTAINER_PTR %x for OBJECT4DV2_PTR %x", obj_md2, obj);

    ///////////////////////////////////////////////////////////////////////////////
    // clear out the object and initialize it a bit
    memset(obj, 0, sizeof(OBJECT4DV2));

    // set state of object to active and visible
    obj->state = OBJECT4DV2_STATE_ACTIVE | OBJECT4DV2_STATE_VISIBLE;

    // set some information in object
    obj->num_frames   = 1;   // always set to 1
    obj->curr_frame   = 0;
    obj->attr         = OBJECT4DV2_ATTR_SINGLE_FRAME | OBJECT4DV2_ATTR_TEXTURES; 

    obj->num_vertices = obj_md2->num_verts;
    obj->num_polys    = obj_md2->num_polys;
    obj->texture      = obj_md2->skin;

    // set position of object
    obj->world_pos = obj_md2->world_pos;

    // allocate the memory for the vertices and number of polys
    // the call parameters are redundant in this case, but who cares
    if (!Init_OBJECT4DV2(obj,   
        obj->num_vertices, 
        obj->num_polys, 
        obj->num_frames))
    {
        Write_Error("\n(can't allocate memory).");
    } // end if

    ////////////////////////////////////////////////////////////////////////////////
    // compute average and max radius using the vertices from frame 0, this isn't
    // totally accurate, but the volume of the object hopefully does vary wildly 
    // during animation

    // reset incase there's any residue
    obj->avg_radius[0] = 0;
    obj->max_radius[0] = 0;

    // loop thru and compute radius
    for (int vindex = 0; vindex < obj_md2->num_verts; vindex++)
    {
        // update the average and maximum radius (use frame 0)
        float dist_to_vertex = 
            sqrt(obj_md2->vlist[vindex].x * obj_md2->vlist[vindex].x +
            obj_md2->vlist[vindex].y * obj_md2->vlist[vindex].y +
            obj_md2->vlist[vindex].z * obj_md2->vlist[vindex].z );

        // accumulate total radius
        obj->avg_radius[0]+=dist_to_vertex;

        // update maximum radius   
        if (dist_to_vertex > obj->max_radius[0])
            obj->max_radius[0] = dist_to_vertex; 

    } // end for vertex

    // finallize average radius computation
    obj->avg_radius[0]/=obj->num_vertices;

    Write_Error("\nMax radius=%f, Avg. Radius=%f",obj->max_radius[0], obj->avg_radius[0]);
    Write_Error("\nWriting texture coordinates...");

    ///////////////////////////////////////////////////////////////////////////////
    // copy texture coordinate list always the same
    for (int tindex = 0; tindex < obj_md2->num_textcoords; tindex++)
    {
        // now texture coordinates
        obj->tlist[tindex].x = obj_md2->tlist[tindex].x;
        obj->tlist[tindex].y = obj_md2->tlist[tindex].y;
    } // end for tindex

    Write_Error("\nWriting polygons...");

    // generate the polygon index list, always the same
    for (int pindex=0; pindex < obj_md2->num_polys; pindex++)
    {
        // set polygon indices
        obj->plist[pindex].vert[0] = obj_md2->polys[pindex].vindex[0];
        obj->plist[pindex].vert[1] = obj_md2->polys[pindex].vindex[1];
        obj->plist[pindex].vert[2] = obj_md2->polys[pindex].vindex[2];

        // point polygon vertex list to object's vertex list
        // note that this is redundant since the polylist is contained
        // within the object in this case and its up to the user to select
        // whether the local or transformed vertex list is used when building up
        // polygon geometry, might be a better idea to set to NULL in the context
        // of polygons that are part of an object
        obj->plist[pindex].vlist   = obj->vlist_local; 

        // set attributes of polygon with sent attributes
        obj->plist[pindex].attr    = obj_md2->attr;

        // set color of polygon
        obj->plist[pindex].color   = obj_md2->color;

        // apply texture to this polygon
        obj->plist[pindex].texture = obj_md2->skin;

        // assign the texture coordinates
        obj->plist[pindex].text[0] = obj_md2->polys[pindex].tindex[0];
        obj->plist[pindex].text[1] = obj_md2->polys[pindex].tindex[1];
        obj->plist[pindex].text[2] = obj_md2->polys[pindex].tindex[2];

        // set texture coordinate attributes
        SET_BIT(obj->vlist_local[ obj->plist[pindex].vert[0] ].attr, VERTEX4DTV1_ATTR_TEXTURE); 
        SET_BIT(obj->vlist_local[ obj->plist[pindex].vert[1] ].attr, VERTEX4DTV1_ATTR_TEXTURE); 
        SET_BIT(obj->vlist_local[ obj->plist[pindex].vert[2] ].attr, VERTEX4DTV1_ATTR_TEXTURE); 

        // set the material mode to ver. 1.0 emulation
        SET_BIT(obj->plist[pindex].attr, POLY4DV2_ATTR_DISABLE_MATERIAL);

        // finally set the polygon to active
        obj->plist[pindex].state = POLY4DV2_STATE_ACTIVE;    

        // point polygon vertex list to object's vertex list
        // note that this is redundant since the polylist is contained
        // within the object in this case and its up to the user to select
        // whether the local or transformed vertex list is used when building up
        // polygon geometry, might be a better idea to set to NULL in the context
        // of polygons that are part of an object
        obj->plist[pindex].vlist = obj->vlist_local; 

        // set texture coordinate list, this is needed
        obj->plist[pindex].tlist = obj->tlist;

        // extract vertex indices
        int vindex_0 = obj_md2->polys[pindex].vindex[0];
        int vindex_1 = obj_md2->polys[pindex].vindex[1];
        int vindex_2 = obj_md2->polys[pindex].vindex[2];

        // we need to compute the normal of this polygon face, and recall
        // that the vertices are in cw order, u=p0->p1, v=p0->p2, n=uxv
        VECTOR4D u, v, n;

        // build u, v
        //VECTOR4D_Build(&obj->vlist_local[ vindex_0 ].v, &obj->vlist_local[ vindex_1 ].v, &u);
        //VECTOR4D_Build(&obj->vlist_local[ vindex_0 ].v, &obj->vlist_local[ vindex_2 ].v, &v);

        u.x = obj_md2->vlist[vindex_1].x - obj_md2->vlist[vindex_0].x;
        u.y = obj_md2->vlist[vindex_1].y - obj_md2->vlist[vindex_0].y;
        u.z = obj_md2->vlist[vindex_1].z - obj_md2->vlist[vindex_0].z;
        u.w = 1;

        v.x = obj_md2->vlist[vindex_2].x - obj_md2->vlist[vindex_0].x;
        v.y = obj_md2->vlist[vindex_2].y - obj_md2->vlist[vindex_0].y;
        v.z = obj_md2->vlist[vindex_2].z - obj_md2->vlist[vindex_0].z;
        v.w = 1;

        // compute cross product
        VECTOR4D_Cross(&u, &v, &n);

        // compute length of normal accurately and store in poly nlength
        // +- epsilon later to fix over/underflows
        obj->plist[pindex].nlength = VECTOR4D_Length(&n); 

    } // end for poly

    // return success
    return(1);

} // end Prepare_OBJECT4DV2_For_MD2

/////////////////////////////////////////////////////////////////////////////

void Start_Fast_Timer(void)
{
    // call this function to start the timer
    static int first_time = 1;

    if (first_time)
    {
        QueryPerformanceFrequency((LARGE_INTEGER *)&lpFrequency);
        ticks_per_ms = lpFrequency/1000;
        first_time = 0;
    }

    // get the current time and save it globally
    QueryPerformanceCounter((LARGE_INTEGER *)&lpPerformanceCount);
} // end Start_Fast_Timer

//////////////////////////////////////////////////////////////////////////////

void Wait_Fast_Timer(int delay)
{
    // call this function to delay a specific number of milliseconds
    INT64 lpCurrPerformanceCount;

    while(1)
    {
        // get current time
        QueryPerformanceCounter((LARGE_INTEGER *)&lpCurrPerformanceCount);

        // wait until delay
        if ( (lpCurrPerformanceCount - lpPerformanceCount) > (delay*ticks_per_ms) ) 
            break;

    } // end if

} // end Wait_Fast_Timer

//////////////////////////////////////////////////////////////////////////////