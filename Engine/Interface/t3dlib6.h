// T3DLIB6.H - header file for T3DLIB6.H

// watch for multiple inclusions
#ifndef T3DLIB6
#define T3DLIB6

// DEFINES //////////////////////////////////////////////////////////////////

#define VERTEX_FLAGS_INVERT_X               0x0001   // inverts the Z-coordinates
#define VERTEX_FLAGS_INVERT_Y               0x0002   // inverts the Z-coordinates
#define VERTEX_FLAGS_INVERT_Z               0x0004   // inverts the Z-coordinates
#define VERTEX_FLAGS_SWAP_YZ                0x0008   // transforms a RHS model to a LHS model
#define VERTEX_FLAGS_SWAP_XZ                0x0010   
#define VERTEX_FLAGS_SWAP_XY                0x0020
#define VERTEX_FLAGS_INVERT_WINDING_ORDER   0x0040   // invert winding order from cw to ccw or ccw to cc


#define VERTEX_FLAGS_TRANSFORM_LOCAL        0x0200   // if file format has local transform then do it!
#define VERTEX_FLAGS_TRANSFORM_LOCAL_WORLD  0x0400  // if file format has local to world then do it!


// defines for materials, follow our polygon attributes as much as possible
#define MATV1_ATTR_2SIDED                 0x0001
#define MATV1_ATTR_TRANSPARENT            0x0002
#define MATV1_ATTR_8BITCOLOR              0x0004
#define MATV1_ATTR_RGB16                  0x0008
#define MATV1_ATTR_RGB24                  0x0010

#define MATV1_ATTR_SHADE_MODE_CONSTANT    0x0020
#define MATV1_ATTR_SHADE_MODE_EMMISIVE    0x0020 // alias
#define MATV1_ATTR_SHADE_MODE_FLAT        0x0040
#define MATV1_ATTR_SHADE_MODE_GOURAUD     0x0080
#define MATV1_ATTR_SHADE_MODE_FASTPHONG   0x0100
#define MATV1_ATTR_SHADE_MODE_TEXTURE     0x0200

// defines for material system
#define MAX_MATERIALS                     256

// states of materials
#define MATV1_STATE_ACTIVE                0x0001
#define MATV1_STATE_INACTIVE              0x0001

// defines for light types
#define LIGHTV1_ATTR_AMBIENT      0x0001    // basic ambient light
#define LIGHTV1_ATTR_INFINITE     0x0002    // infinite light source
#define LIGHTV1_ATTR_DIRECTIONAL  0x0002    // infinite light source (alias)
#define LIGHTV1_ATTR_POINT        0x0004    // point light source
#define LIGHTV1_ATTR_SPOTLIGHT1   0x0008    // spotlight type 1 (simple)
#define LIGHTV1_ATTR_SPOTLIGHT2   0x0010    // spotlight type 2 (complex)

#define LIGHTV1_STATE_ON          1         // light on
#define LIGHTV1_STATE_OFF         0         // light off

#define MAX_LIGHTS                8         // good luck with 1!

// polygon sorting, and painters algorithm defines

// flags for sorting algorithm
#define SORT_POLYLIST_AVGZ  0  // sorts on average of all vertices
#define SORT_POLYLIST_NEARZ 1  // sorts on closest z vertex of each poly
#define SORT_POLYLIST_FARZ  2  // sorts on farthest z vertex of each poly


#define PARSER_DEBUG_OFF // enables/disables conditional compilation 

#define PARSER_STRIP_EMPTY_LINES        1   // strips all blank lines
#define PARSER_LEAVE_EMPTY_LINES        2   // leaves empty lines
#define PARSER_STRIP_WS_ENDS            4   // strips ws space at ends of line
#define PARSER_LEAVE_WS_ENDS            8   // leaves it
#define PARSER_STRIP_COMMENTS           16  // strips comments out
#define PARSER_LEAVE_COMMENTS           32  // leaves comments in

#define PARSER_BUFFER_SIZE              256 // size of parser line buffer
#define PARSER_MAX_COMMENT              16  // maximum size of comment delimeter string

#define PARSER_DEFAULT_COMMENT          "#"  // default comment string for parser

// pattern language
#define PATTERN_TOKEN_FLOAT   'f'
#define PATTERN_TOKEN_INT     'i'
#define PATTERN_TOKEN_STRING  's'
#define PATTERN_TOKEN_LITERAL '\''

// state machine defines for pattern matching
#define PATTERN_STATE_INIT       0

#define PATTERN_STATE_RESTART    1
#define PATTERN_STATE_FLOAT      2
#define PATTERN_STATE_INT        3 
#define PATTERN_STATE_LITERAL    4
#define PATTERN_STATE_STRING     5
#define PATTERN_STATE_NEXT       6

#define PATTERN_STATE_MATCH      7
#define PATTERN_STATE_END        8

#define PATTERN_MAX_ARGS         16
#define PATTERN_BUFFER_SIZE      80

// TYPES ///////////////////////////////////////////////////////////////////

// RGB+alpha color
typedef struct RGBAV1_TYP
{
    union 
    {
        int rgba;                    // compressed format
        UCHAR rgba_M[4];             // array format
        struct {  UCHAR a,b,g,r;  }; // explict name format
    }; // end union

} RGBAV1, *RGBAV1_PTR;


// a first version of a "material"
typedef struct MATV1_TYP
{
    int state;           // state of material
    int id;              // id of this material, index into material array
    char name[64];       // name of material
    int  attr;           // attributes, the modes for shading, constant, flat, 
    // gouraud, fast phong, environment, textured etc.
    // and other special flags...

    RGBAV1 color;            // color of material
    float ka, kd, ks, power; // ambient, diffuse, specular, 
    // coefficients, note they are 
    // separate and scalars since many 
    // modelers use this format
    // along with specular power

    RGBAV1 ra, rd, rs;       // the reflectivities/colors pre-
    // multiplied, to more match our 
    // definitions, each is basically
    // computed by multiplying the 
    // color by the k's, eg:
    // rd = color*kd etc.

    char texture_file[80];   // file location of texture
    BITMAP_IMAGE texture;    // actual texture map (if any)

    int   iaux1, iaux2;      // auxiliary vars for future expansion
    float faux1, faux2;
    void *ptr;

} MATV1, *MATV1_PTR;


// first light structure
typedef struct LIGHTV1_TYP
{
    int state; // state of light
    int id;    // id of light
    int attr;  // type of light, and extra qualifiers

    RGBAV1 c_ambient;   // ambient light intensity
    RGBAV1 c_diffuse;   // diffuse light intensity
    RGBAV1 c_specular;  // specular light intensity

    POINT4D  pos;       // position of light
    VECTOR4D dir;       // direction of light
    float kc, kl, kq;   // attenuation factors
    float spot_inner;   // inner angle for spot light
    float spot_outer;   // outer angle for spot light
    float pf;           // power factor/falloff for spot lights

    int   iaux1, iaux2; // auxiliary vars for future expansion
    float faux1, faux2;
    void *ptr;

} LIGHTV1, *LIGHTV1_PTR;


// pcx file header
typedef struct PCX_HEADER_TYP 
{
    UCHAR  manufacturer;    // always 0x0A
    UCHAR  version;         // version 0x05 for version 3.0 and later
    UCHAR  encoding;        // always 1
    UCHAR  bits_per_pixel;  // bits per pixel 1,2,4,8
    USHORT xmin, ymin;      // coordinates of upper left corner
    USHORT xmax, ymax;      // coordinates of lower right corner
    USHORT hres;            // horizontal resolution of image in dpi 75/100 typ
    USHORT yres;            // vertical resolution of image in dpi 75/100
    UCHAR  EGAcolors[48];   // ega palette, not used for 256 color images
    UCHAR  reserved;        // reserved for future use, video mode?
    UCHAR  color_planes;    // number of color planes 3 for 24-bit imagery
    USHORT bytes_per_line;  // bytes per line, always even!
    USHORT palette_type;    // 1 for gray scale, 2 for color palette
    USHORT scrnw;           // width of screen image taken from
    USHORT scrnh;           // height of screen image taken from     
    UCHAR  filler[54];      // filler bytes
} PCX_HEADER, *PCX_HEADER_PTR;


// CLASSES /////////////////////////////////////////////////////////////////

// parser class ///////////////////////////////////////////////
class CPARSERV1
{
public:

    // constructor /////////////////////////////////////////////////
    CPARSERV1();

    // destructor ///////////////////////////////////////////////////
    ~CPARSERV1() ;

    // reset file system ////////////////////////////////////////////
    int Reset();

    // open file /////////////////////////////////////////////////////
    int Open(char *filename);

    // close file ////////////////////////////////////////////////////
    int Close();

    // get line //////////////////////////////////////////////////////
    char *Getline(int mode);

    // sets the comment string ///////////////////////////////////////
    int SetComment(char *string);

    // find pattern in line //////////////////////////////////////////
    int Pattern_Match(char *string, char *pattern, ...);

    // VARIABLE DECLARATIONS /////////////////////////////////////////

public: 

    FILE *fstream;                    // file pointer
    char buffer[PARSER_BUFFER_SIZE];  // line buffer
    int  length;                      // length of current line
    int  num_lines;                   // number of lines processed
    char comment[PARSER_MAX_COMMENT]; // single line comment string

    // pattern matching parameter storage, easier that variable arguments
    // anything matched will be stored here on exit from the call to pattern()
    char  pstrings[PATTERN_MAX_ARGS][PATTERN_BUFFER_SIZE]; // any strings
    int   num_pstrings;

    float pfloats[PATTERN_MAX_ARGS];                       // any floats
    int   num_pfloats;

    int   pints[PATTERN_MAX_ARGS];                         // any ints
    int   num_pints;

}; // end CLASS CPARSERV1 //////////////////////////////////////////////

typedef CPARSERV1 *CPARSERV1_PTR;

// MACROS ///////////////////////////////////////////////////////////////////

#define SIGN(x) ((x) > 0 ? (1) : (-1))

// this builds a 32 bit color value in 8.8.8.a format (8-bit alpha mode)
#define _RGBA32BIT(r,g,b,a) ((a) + ((b) << 8) + ((g) << 16) + ((r) << 24))


// this builds extract the RGB components of a 16 bit color value in 5.5.5 format (1-bit alpha mode)
#define _RGB555FROM16BIT(RGB, r,g,b) { *r = ( ((RGB) >> 10) & 0x1f); *g = (((RGB) >> 5) & 0x1f); *b = ( (RGB) & 0x1f); }

// this extracts the RGB components of a 16 bit color value in 5.6.5 format (green dominate mode)
#define _RGB565FROM16BIT(RGB, r,g,b) { *r = ( ((RGB) >> 11) & 0x1f); *g = (((RGB) >> 5) & 0x3f); *b = ((RGB) & 0x1f); }

// TYPES ///////////////////////////////////////////////////////////////////

// PROTOTYPES //////////////////////////////////////////////////////////////

void Draw_OBJECT4DV1_Solid(OBJECT4DV1_PTR obj, UCHAR *video_buffer, int lpitch);

void Draw_RENDERLIST4DV1_Solid(RENDERLIST4DV1_PTR rend_list, UCHAR *video_buffer, int lpitch);

void Draw_OBJECT4DV1_Solid16(OBJECT4DV1_PTR obj, UCHAR *video_buffer, int lpitch);

void Draw_RENDERLIST4DV1_Solid16(RENDERLIST4DV1_PTR rend_list, UCHAR *video_buffer, int lpitch);


int Convert_Bitmap_8_16(BITMAP_FILE_PTR bitmap);

int StripChars(char *string_in, char *string_out, char *strip_chars, int case_on=1);

int ReplaceChars(char *string_in, char *string_out, char *replace_chars, char rep_char, int case_on=1);

char *StringLtrim(char *string);

char *StringRtrim(char *string);

float IsFloat(char *fstring);

int   IsInt(char *istring);

int Load_OBJECT4DV1_3DSASC(OBJECT4DV1_PTR obj, char *filename,  
                           VECTOR4D_PTR scale, VECTOR4D_PTR pos, VECTOR4D_PTR rot, 
                           int vertex_flags=0);


int Load_OBJECT4DV1_COB(OBJECT4DV1_PTR obj,   // pointer to object
                        char *filename,       // filename of Caligari COB file
                        VECTOR4D_PTR scale,   // initial scaling factors
                        VECTOR4D_PTR pos,     // initial position
                        VECTOR4D_PTR rot,     // initial rotations
                        int vertex_flags=0);  // flags to re-order vertices 
// and perform transforms


int RGBto8BitIndex(UCHAR r, UCHAR g, UCHAR b, LPPALETTEENTRY palette, int flush_cache);

int RGB_16_8_Indexed_Intensity_Table_Builder(LPPALETTEENTRY src_palette,  // source palette
                                             UCHAR rgbilookup[256][256],  // lookup table
                                             int intensity_normalization=1);

int RGB_16_8_IndexedRGB_Table_Builder(int rgb_format,             // format we want to build table for
                                      LPPALETTEENTRY src_palette, // source palette
                                      UCHAR *rgblookup);          // lookup table

// lighting system
int Init_Light_LIGHTV1(int           index,      // index of light to create (0..MAX_LIGHTS-1)
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

int Reset_Lights_LIGHTV1(void);

// material system
int Reset_Materials_MATV1(void);

// inserts an object into renderlist with shaded color override             
int Insert_OBJECT4DV1_RENDERLIST4DV12(RENDERLIST4DV1_PTR rend_list, 
                                      OBJECT4DV1_PTR obj,
                                      int insert_local=0,
                                      int lighting_on=0);
// light an object
int Light_OBJECT4DV1_World16(OBJECT4DV1_PTR obj,  // object to process
                             CAM4DV1_PTR cam,     // camera position
                             LIGHTV1_PTR lights,  // light list (might have more than one)
                             int max_lights);     // maximum lights in list


int Light_OBJECT4DV1_World(OBJECT4DV1_PTR obj,  // object to process
                           CAM4DV1_PTR cam,     // camera position
                           LIGHTV1_PTR lights,  // light list (might have more than one)
                           int max_lights);      // maximum lights in list

// light the entire rendering list
int Light_RENDERLIST4DV1_World(RENDERLIST4DV1_PTR rend_list,  // list to process
                               CAM4DV1_PTR cam,     // camera position
                               LIGHTV1_PTR lights,  // light list (might have more than one)
                               int max_lights);     // maximum lights in list


// light the entire rendering list
int Light_RENDERLIST4DV1_World16(RENDERLIST4DV1_PTR rend_list,  // list to process
                                 CAM4DV1_PTR cam,     // camera position
                                 LIGHTV1_PTR lights,  // light list (might have more than one)
                                 int max_lights);     // maximum lights in list

// z-sort algorithm (simple painters algorithm)
void Sort_RENDERLIST4DV1(RENDERLIST4DV1_PTR rend_list, int sort_method=SORT_POLYLIST_AVGZ);

// avg z-compare
int Compare_AvgZ_POLYF4DV1(const void *arg1, const void *arg2);

// near z-compare
int Compare_NearZ_POLYF4DV1(const void *arg1, const void *arg2);

// far z-compare
int Compare_FarZ_POLYF4DV1(const void *arg1, const void *arg2);

// GLOBALS ///////////////////////////////////////////////////////////////////

extern MATV1 materials[MAX_MATERIALS]; // materials in system
extern int num_materials;              // current number of materials


extern LIGHTV1 lights[MAX_LIGHTS];  // lights in system
extern int num_lights;              // current number of lights

// these look up tables are used by the 8-bit lighting engine
// the first one holds a color translation table in the form of each
// row is a color 0..255, and each row consists of 256 shades of that color
// the data in each row is the color/intensity indices and the resulting value
// is an 8-bit index into the real color lookup that should be used as the color

// the second table works by each index being a compressed 16bit RGB value
// the data indexed by that RGB value IS the index 0..255 of the real
// color lookup that matches the desired color the closest

extern UCHAR rgbilookup[256][256];         // intensity RGB 8-bit lookup storage
extern UCHAR rgblookup[65536];             // RGB 8-bit color lookup

//////////////////////////////////////////////////////////////////////////////

#endif


