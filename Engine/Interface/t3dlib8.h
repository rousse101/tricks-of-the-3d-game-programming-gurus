// T3DLIB8.H - header file for T3DLIB8.H

// watch for multiple inclusions
#ifndef T3DLIB8
#define T3DLIB8

// DEFINES //////////////////////////////////////////////////////////////////

// general clipping flags for polygons
#define CLIP_POLY_X_PLANE           0x0001 // cull on the x clipping planes
#define CLIP_POLY_Y_PLANE           0x0002 // cull on the y clipping planes
#define CLIP_POLY_Z_PLANE           0x0004 // cull on the z clipping planes
#define CLIP_OBJECT_XYZ_PLANES      (CULL_OBJECT_X_PLANE | CULL_OBJECT_Y_PLANE | CULL_OBJECT_Z_PLANE)

// defines for light types version 2.0
#define LIGHTV2_ATTR_AMBIENT      0x0001    // basic ambient light
#define LIGHTV2_ATTR_INFINITE     0x0002    // infinite light source
#define LIGHTV2_ATTR_DIRECTIONAL  0x0002    // infinite light source (alias)
#define LIGHTV2_ATTR_POINT        0x0004    // point light source
#define LIGHTV2_ATTR_SPOTLIGHT1   0x0008    // spotlight type 1 (simple)
#define LIGHTV2_ATTR_SPOTLIGHT2   0x0010    // spotlight type 2 (complex)

#define LIGHTV2_STATE_ON          1         // light on
#define LIGHTV2_STATE_OFF         0         // light off


// transformation control flags
#define TRANSFORM_COPY_LOCAL_TO_TRANS   3 // copy data as is, no transform

// TYPES ///////////////////////////////////////////////////////////////////

// version 2.0 of light structure
typedef struct LIGHTV2_TYP
{
    int state; // state of light
    int id;    // id of light
    int attr;  // type of light, and extra qualifiers

    RGBAV1 c_ambient;   // ambient light intensity
    RGBAV1 c_diffuse;   // diffuse light intensity
    RGBAV1 c_specular;  // specular light intensity

    POINT4D  pos;       // position of light world/transformed
    POINT4D  tpos;
    VECTOR4D dir;       // direction of light world/transformed
    VECTOR4D tdir;
    float kc, kl, kq;   // attenuation factors
    float spot_inner;   // inner angle for spot light
    float spot_outer;   // outer angle for spot light
    float pf;           // power factor/falloff for spot lights

    int   iaux1, iaux2; // auxiliary vars for future expansion
    float faux1, faux2;
    void *ptr;

} LIGHTV2, *LIGHTV2_PTR;


// CLASSES /////////////////////////////////////////////////////////////////


// MACROS ///////////////////////////////////////////////////////////////////



// TYPES ///////////////////////////////////////////////////////////////////

// EXTERNALS ///////////////////////////////////////////////////////////////

extern LIGHTV2 lights2[MAX_LIGHTS];  // lights in system

// PROTOTYPES //////////////////////////////////////////////////////////////

void Clip_Polys_RENDERLIST4DV2(RENDERLIST4DV2_PTR rend_list, CAM4DV1_PTR cam, int clip_flags);

int Generate_Terrain_OBJECT4DV2(OBJECT4DV2_PTR obj,     // pointer to object
                                float twidth,            // width in world coords on x-axis
                                float theight,           // height (length) in world coords on z-axis
                                float vscale,           // vertical scale of terrain
                                char *height_map_file,  // filename of height bitmap encoded in 256 colors
                                char *texture_map_file, // filename of texture map
                                int rgbcolor,           // color of terrain if no texture        
                                VECTOR4D_PTR pos,       // initial position
                                VECTOR4D_PTR rot,       // initial rotations
                                int poly_attr);         // the shading attributes we would like


int Light_OBJECT4DV2_World2_16(OBJECT4DV2_PTR obj,  // object to process
                               CAM4DV1_PTR cam,     // camera position
                               LIGHTV2_PTR lights,  // light list (might have more than one)
                               int max_lights);     // maximum lights in list


int Light_OBJECT4DV2_World2(OBJECT4DV2_PTR obj,  // object to process
                            CAM4DV1_PTR cam,     // camera position
                            LIGHTV2_PTR lights,  // light list (might have more than one)
                            int max_lights);      // maximum lights in list

int Light_RENDERLIST4DV2_World2(RENDERLIST4DV2_PTR rend_list,  // list to process
                                CAM4DV1_PTR cam,     // camera position
                                LIGHTV2_PTR lights,  // light list (might have more than one)
                                int max_lights);     // maximum lights in list


int Light_RENDERLIST4DV2_World2_16(RENDERLIST4DV2_PTR rend_list,  // list to process
                                   CAM4DV1_PTR cam,     // camera position
                                   LIGHTV2_PTR lights,  // light list (might have more than one)
                                   int max_lights);     // maximum lights in list


// lighting system
int Init_Light_LIGHTV2(LIGHTV2_PTR lights,       // light array to work with (new)
                       int           index,      // index of light to create (0..MAX_LIGHTS-1)
                       int          _state,      // state of light
                       int          _attr,       // type of light, and extra qualifiers
                       RGBAV1       _c_ambient,  // ambient light intensity
                       RGBAV1       _c_diffuse,  // diffuse light intensity
                       RGBAV1       _c_specular, // specular light intensity
                       POINT4D_PTR  _pos,        // position of light
                       VECTOR4D_PTR _dir,        // direction of light
                       float        _kc,         // attenuation factors
                       float        _kl, 
                       float        _kq, 
                       float        _spot_inner, // inner angle for spot light
                       float        _spot_outer, // outer angle for spot light
                       float        _pf);        // power factor/falloff for spot lights

int Reset_Lights_LIGHTV2(LIGHTV2_PTR lights,       // light array to work with (new)
                         int max_lights);          // number of lights in system


void Transform_LIGHTSV2(LIGHTV2_PTR lights,  // array of lights to transform
                        int num_lights,      // number of lights to transform
                        MATRIX4X4_PTR mt,    // transformation matrix
                        int coord_select);   // selects coords to transform

#endif


