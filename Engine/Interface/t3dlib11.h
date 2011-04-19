// T3DLIB11.H - header file for T3DLIB11.H

// watch for multiple inclusions
#ifndef T3DLIB11
#define T3DLIB11

// DEFINES //////////////////////////////////////////////////////////////////

// new rendering context defines

// use a Z buffer, but with write thru, no compare
#define RENDER_ATTR_WRITETHRUZBUFFER   0x00000008  

#define WALL_SPLIT_ID                  4096   // used to tag walls that are split

// defines for BHV system
#define MAX_OBJECTS_PER_BHV_CELL    64   // maximum objects that will fit in one BHV cell
// note: this is DIFFERENT than a node
#define MIN_OBJECTS_PER_BHV_CELL    2    // recursion stops if a cell has less that this 
// number of objects

#define MAX_BHV_CELL_DIVISIONS      8    // maximum number of cells in one axis that
// space can be divided into 

#define MAX_OBJECTS_PER_BHV_NODE    512  // maximum objects a node can handle

#define MAX_BHV_PER_NODE            32   // maximum children per node


///////////////////////////////////////////////////////////////////////////////


// TYPES ///////////////////////////////////////////////////////////////////

// a self contained quad polygon used for the render list version 2 //////
// we need this to represent the walls since they are quads
typedef struct POLYF4DV2Q_TYP
{
    int      state;           // state information
    int      attr;            // physical attributes of polygon
    int      color;           // color of polygon
    int      lit_color[4];    // holds colors after lighting, 0 for flat shading
    // 0,1,2 for vertex colors after vertex lighting
    BITMAP_IMAGE_PTR texture; // pointer to the texture information for simple texture mapping

    int      mati;          // material index (-1) for no material

    float    nlength;       // length of the polygon normal if not normalized 
    VECTOR4D normal;        // the general polygon normal

    float    avg_z;         // average z of vertices, used for simple sorting

    VERTEX4DTV1 vlist[4];   // the vertices of this quad
    VERTEX4DTV1 tvlist[4];  // the vertices after transformation if needed 

    POLYF4DV2Q_TYP *next;   // pointer to next polygon in list??
    POLYF4DV2Q_TYP *prev;   // pointer to previous polygon in list??

} POLYF4DV2Q, *POLYF4DV2Q_PTR;

// a general bsp node, all bsp trees are built from this
// struct, and the root of a bsp is also this struct
typedef struct BSPNODEV1_TYP
{
    int        id;     // id tage for debugging
    POLYF4DV2Q  wall;  // the actual wall quad

    struct BSPNODEV1_TYP *link;  // pointer to next wall
    struct BSPNODEV1_TYP *front; // pointer to walls in front
    struct BSPNODEV1_TYP *back;  // pointer to walls behind

} BSPNODEV1, *BSPNODEV1_PTR;

// object container to hold any object
typedef struct OBJ_CONTAINERV1_TYP
{
    int state;          // state of container object
    int attr;           // attributes of container object
    POINT4D pos;        // position of container object
    VECTOR4D vel;       // velocity of object container
    VECTOR4D rot;       // rotational rate of object container
    int auxi[8];        // aux array of 8 ints
    int auxf[8];        // aux array of 8 floats
    void *aux_ptr;      // aux pointer
    void *obj;          // pointer to master object

} OBJ_CONTAINERV1, *OBJ_CONTAINERV1_PTR;

// a bounding heirarchical volume node
typedef struct BHV_NODEV1_TYP
{
    int state;         // state of this node
    int attr;          // attributes of this node
    POINT4D pos;       // center of node
    VECTOR4D radius;   // x,y,z radius of node
    int num_objects;   // number of objects contained in node
    OBJ_CONTAINERV1 *objects[MAX_OBJECTS_PER_BHV_NODE]; // objects

    int num_children;  // number of children nodes

    BHV_NODEV1_TYP *links[MAX_BHV_PER_NODE]; // links to children

} BHV_NODEV1, *BHV_NODEV1_PTR;

// internally used for the 3D array sorting algorithm
typedef struct BHV_CELL_TYP
{
    int num_objects;                                     // number of objects in this cell
    OBJ_CONTAINERV1 *obj_list[MAX_OBJECTS_PER_BHV_CELL]; // storage for object pointers
} BHV_CELL, *BHV_CELL_PTR;


// CLASSES /////////////////////////////////////////////////////////////////


// M A C R O S ///////////////////////////////////////////////////////////////

// tests if two 3-d points are equal
#define VECTOR4D_Equal(v1,v2) (FCMP((v1)->x,(v2)->x) && FCMP((v1)->y,(v2)->y) && FCMP((v1)->z,(v2)->z))
#define VECTOR3D_Equal(v1,v2) (FCMP((v1)->x,(v2)->x) && FCMP((v1)->y,(v2)->y) && FCMP((v1)->z,(v2)->z))

// this macro assumes the lookup has 360 elements, where
// each element is in .5 degree increments roughly
// x must be in the range [-1,1]
#define FAST_INV_COS(x) (dp_inverse_cos[(int)(((float)x+1)*(float)180)])

// EXTERNALS ///////////////////////////////////////////////////////////////

extern HWND main_window_handle; // save the window handle
extern HINSTANCE main_instance; // save the instance

// dot product look up table
extern float dp_inverse_cos[360+2]; // 0 to 180 in .5 degree steps
// the +2 for padding

extern int bhv_nodes_visited;       // used to track how many nodes where visited

// MACROS ////////////////////////////////////////////////



// PROTOTYPES //////////////////////////////////////////////////////////////

void Build_Inverse_Cos_Table(float *invcos,      // storage for table
                             int   range_scale); // range for table to span

void Draw_RENDERLIST4DV2_RENDERCONTEXTV1_16_2(RENDERCONTEXTV1_PTR rc);


void Draw_Gouraud_TriangleWTZB2_16(POLYF4DV2_PTR face,  // ptr to face
                                   UCHAR *_dest_buffer,      // pointer to video buffer
                                   int mem_pitch,            // bytes per line, 320, 640 etc.
                                   UCHAR *_zbuffer,          // pointer to z-buffer
                                   int zpitch);              // bytes per line of zbuffer

void Draw_Triangle_2DWTZB_16(POLYF4DV2_PTR face,        // ptr to face
                             UCHAR *_dest_buffer,         // pointer to video buffer
                             int mem_pitch,               // bytes per line, 320, 640 etc.
                             UCHAR *_zbuffer,             // pointer to z-buffer
                             int zpitch);                 // bytes per line of zbuffer


void Draw_Textured_TriangleGSWTZB_16(POLYF4DV2_PTR face, // ptr to face
                                     UCHAR *_dest_buffer,    // pointer to video buffer
                                     int mem_pitch,          // bytes per line, 320, 640 etc.
                                     UCHAR *_zbuffer,        // pointer to z-buffer
                                     int zpitch);            // bytes per line of zbuffer

void Draw_Textured_TriangleFSWTZB2_16(POLYF4DV2_PTR face, // ptr to face
                                      UCHAR *_dest_buffer,     // pointer to video buffer
                                      int mem_pitch,           // bytes per line, 320, 640 etc.
                                      UCHAR *_zbuffer,         // pointer to z-buffer
                                      int zpitch);             // bytes per line of zbuffer


void Draw_Textured_TriangleWTZB2_16(POLYF4DV2_PTR face,  // ptr to face
                                    UCHAR *_dest_buffer,       // pointer to video buffer
                                    int mem_pitch,             // bytes per line, 320, 640 etc.
                                    UCHAR *_zbuffer,           // pointer to z-buffer
                                    int zpitch);               // bytes per line of zbuffer

// BSP PROTOTYPES///////////////////////////////////////////////////////

void Bsp_Translate(BSPNODEV1_PTR root, VECTOR4D_PTR trans);

void Bsp_Delete(BSPNODEV1_PTR root);

void Bsp_Print(BSPNODEV1_PTR root);

void Bsp_Build_Tree(BSPNODEV1_PTR root);

void Bsp_Transform(BSPNODEV1_PTR root,  // root of bsp tree
                   MATRIX4X4_PTR mt,    // transformation matrix
                   int coord_select);   // selects coords to transform)

void Intersect_Lines(float x0,float y0,float x1,float y1,
                     float x2,float y2,float x3,float y3,
                     float *xi,float *yi);


void Bsp_Insertion_Traversal_RENDERLIST4DV2(RENDERLIST4DV2_PTR rend_list, 
                                            BSPNODEV1_PTR root,
                                            CAM4DV1_PTR cam,
                                            int insert_local);


void Bsp_Insertion_Traversal_RemoveBF_RENDERLIST4DV2(RENDERLIST4DV2_PTR rend_list, 
                                                     BSPNODEV1_PTR root,
                                                     CAM4DV1_PTR cam,
                                                     int insert_local);

void Bsp_Insertion_Traversal_FrustrumCull_RENDERLIST4DV2(RENDERLIST4DV2_PTR rend_list, 
                                                         BSPNODEV1_PTR root,
                                                         CAM4DV1_PTR cam,
                                                         int insert_local);



// BHV prototypes //////////////////////////////////////////////////////////

void BHV_Build_Tree(BHV_NODEV1_PTR bhv_tree,         // tree to build
                    OBJ_CONTAINERV1_PTR bhv_objects, // ptr to all objects in intial scene
                    int num_objects,                 // number of objects in initial scene
                    int level,                       // recursion level
                    int num_divisions,               // number of division per level
                    int universe_radius);            // initial size of universe to enclose

int BHV_FrustrumCull(BHV_NODEV1_PTR bhv_tree, // the root of the BHV  
                     CAM4DV1_PTR cam,         // camera to cull relative to
                     int cull_flags);         // clipping planes to consider

void BHV_Reset_Tree(BHV_NODEV1_PTR bhv_tree);



#endif
