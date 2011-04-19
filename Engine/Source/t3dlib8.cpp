// T3DLIB8.CPP - clipping, terrain, and new lighting

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


// DEFINES //////////////////////////////////////////////////////////////////

// GLOBALS //////////////////////////////////////////////////////////////////

LIGHTV2 lights2[MAX_LIGHTS];  // lights in system

// FUNCTIONS ////////////////////////////////////////////////////////////////


void Clip_Polys_RENDERLIST4DV2(RENDERLIST4DV2_PTR rend_list, CAM4DV1_PTR cam, int clip_flags)
{
    // this function clips the polygons in the list against the requested clipping planes
    // and sets the clipped flag on the poly, so it's not rendered
    // note the function ONLY performs clipping on the near and far clipping plane
    // but will perform trivial tests on the top/bottom, left/right clipping planes
    // if a polygon is completely out of the viewing frustrum in these cases, it will
    // be culled, however, this test isn't as effective on games based on objects since
    // in most cases objects that are visible have polygons that are visible, but in the
    // case where the polygon list is based on a large object that ALWAYS has some portion
    // visible, testing for individual polys is worthwhile..
    // the function assumes the polygons have been transformed into camera space

    // internal clipping codes
#define CLIP_CODE_GZ   0x0001    // z > z_max
#define CLIP_CODE_LZ   0x0002    // z < z_min
#define CLIP_CODE_IZ   0x0004    // z_min < z < z_max

#define CLIP_CODE_GX   0x0001    // x > x_max
#define CLIP_CODE_LX   0x0002    // x < x_min
#define CLIP_CODE_IX   0x0004    // x_min < x < x_max

#define CLIP_CODE_GY   0x0001    // y > y_max
#define CLIP_CODE_LY   0x0002    // y < y_min
#define CLIP_CODE_IY   0x0004    // y_min < y < y_max

#define CLIP_CODE_NULL 0x0000

    int vertex_ccodes[3]; // used to store clipping flags
    int num_verts_in;     // number of vertices inside
    int v0, v1, v2;       // vertex indices

    float z_factor,       // used in clipping computations
        z_test;         // used in clipping computations

    float xi, yi, x01i, y01i, x02i, y02i, // vertex intersection points
        t1, t2,                         // parametric t values
        ui, vi, u01i, v01i, u02i, v02i; // texture intersection points

    int last_poly_index,            // last valid polygon in polylist
        insert_poly_index;          // the current position new polygons are inserted at

    VECTOR4D u,v,n;                 // used in vector calculations

    POLYF4DV2 temp_poly;            // used when we need to split a poly into 2 polys

    // set last, current insert index to end of polygon list
    // we don't want to clip poly's two times
    insert_poly_index = last_poly_index = rend_list->num_polys;

    // traverse polygon list and clip/cull polygons
    for (int poly = 0; poly < last_poly_index; poly++)
    {
        // acquire current polygon
        POLYF4DV2_PTR curr_poly = rend_list->poly_ptrs[poly];

        // is this polygon valid?
        // test this polygon if and only if it's not clipped, not culled,
        // active, and visible and not 2 sided. Note we test for backface in the event that
        // a previous call might have already determined this, so why work
        // harder!
        if ((curr_poly==NULL) || !(curr_poly->state & POLY4DV2_STATE_ACTIVE) ||
            (curr_poly->state & POLY4DV2_STATE_CLIPPED ) || 
            (curr_poly->state & POLY4DV2_STATE_BACKFACE) )
            continue; // move onto next poly

        // clip/cull to x-planes       
        if (clip_flags & CLIP_POLY_X_PLANE)
        {
            // clip/cull only based on x clipping planes
            // for each vertice determine if it's in the clipping region or beyond it and
            // set the appropriate clipping code
            // we do NOT clip the final triangles, we are only trying to trivally reject them 
            // we are going to clip polygons in the rasterizer to the screen rectangle
            // but we do want to clip/cull polys that are totally outside the viewfrustrum

            // since we are clipping to the right/left x-planes we need to use the FOV or
            // the plane equations to find the z value that at the current x position would
            // be outside the plane
            z_factor = (0.5)*cam->viewplane_width/cam->view_dist;  

            // vertex 0

            z_test = z_factor*curr_poly->tvlist[0].z;

            if (curr_poly->tvlist[0].x > z_test)
                vertex_ccodes[0] = CLIP_CODE_GX;
            else
                if (curr_poly->tvlist[0].x < -z_test)
                    vertex_ccodes[0] = CLIP_CODE_LX;
                else
                    vertex_ccodes[0] = CLIP_CODE_IX;

            // vertex 1

            z_test = z_factor*curr_poly->tvlist[1].z;         

            if (curr_poly->tvlist[1].x > z_test)
                vertex_ccodes[1] = CLIP_CODE_GX;
            else
                if (curr_poly->tvlist[1].x < -z_test)
                    vertex_ccodes[1] = CLIP_CODE_LX;
                else
                    vertex_ccodes[1] = CLIP_CODE_IX;

            // vertex 2

            z_test = z_factor*curr_poly->tvlist[2].z;              

            if (curr_poly->tvlist[2].x > z_test)
                vertex_ccodes[2] = CLIP_CODE_GX;
            else
                if (curr_poly->tvlist[2].x < -z_test)
                    vertex_ccodes[2] = CLIP_CODE_LX;
                else
                    vertex_ccodes[2] = CLIP_CODE_IX;

            // test for trivial rejections, polygon completely beyond right or left
            // clipping planes
            if ( ((vertex_ccodes[0] == CLIP_CODE_GX) && 
                (vertex_ccodes[1] == CLIP_CODE_GX) && 
                (vertex_ccodes[2] == CLIP_CODE_GX) ) ||

                ((vertex_ccodes[0] == CLIP_CODE_LX) && 
                (vertex_ccodes[1] == CLIP_CODE_LX) && 
                (vertex_ccodes[2] == CLIP_CODE_LX) ) )

            {
                // clip the poly completely out of frustrum
                SET_BIT(curr_poly->state, POLY4DV2_STATE_CLIPPED);

                // move on to next polygon
                continue;
            } // end if

        } // end if x planes

        // clip/cull to y-planes       
        if (clip_flags & CLIP_POLY_Y_PLANE)
        {
            // clip/cull only based on y clipping planes
            // for each vertice determine if it's in the clipping region or beyond it and
            // set the appropriate clipping code
            // we do NOT clip the final triangles, we are only trying to trivally reject them 
            // we are going to clip polygons in the rasterizer to the screen rectangle
            // but we do want to clip/cull polys that are totally outside the viewfrustrum

            // since we are clipping to the top/bottom y-planes we need to use the FOV or
            // the plane equations to find the z value that at the current y position would
            // be outside the plane
            z_factor = (0.5)*cam->viewplane_width/cam->view_dist;  

            // vertex 0
            z_test = z_factor*curr_poly->tvlist[0].z;

            if (curr_poly->tvlist[0].y > z_test)
                vertex_ccodes[0] = CLIP_CODE_GY;
            else
                if (curr_poly->tvlist[0].y < -z_test)
                    vertex_ccodes[0] = CLIP_CODE_LY;
                else
                    vertex_ccodes[0] = CLIP_CODE_IY;

            // vertex 1
            z_test = z_factor*curr_poly->tvlist[1].z;         

            if (curr_poly->tvlist[1].y > z_test)
                vertex_ccodes[1] = CLIP_CODE_GY;
            else
                if (curr_poly->tvlist[1].y < -z_test)
                    vertex_ccodes[1] = CLIP_CODE_LY;
                else
                    vertex_ccodes[1] = CLIP_CODE_IY;

            // vertex 2
            z_test = z_factor*curr_poly->tvlist[2].z;              

            if (curr_poly->tvlist[2].y > z_test)
                vertex_ccodes[2] = CLIP_CODE_GY;
            else
                if (curr_poly->tvlist[2].x < -z_test)
                    vertex_ccodes[2] = CLIP_CODE_LY;
                else
                    vertex_ccodes[2] = CLIP_CODE_IY;

            // test for trivial rejections, polygon completely beyond top or bottom
            // clipping planes
            if ( ((vertex_ccodes[0] == CLIP_CODE_GY) && 
                (vertex_ccodes[1] == CLIP_CODE_GY) && 
                (vertex_ccodes[2] == CLIP_CODE_GY) ) ||

                ((vertex_ccodes[0] == CLIP_CODE_LY) && 
                (vertex_ccodes[1] == CLIP_CODE_LY) && 
                (vertex_ccodes[2] == CLIP_CODE_LY) ) )

            {
                // clip the poly completely out of frustrum
                SET_BIT(curr_poly->state, POLY4DV2_STATE_CLIPPED);

                // move on to next polygon
                continue;
            } // end if

        } // end if y planes

        // clip/cull to z planes
        if (clip_flags & CLIP_POLY_Z_PLANE)
        {
            // clip/cull only based on z clipping planes
            // for each vertice determine if it's in the clipping region or beyond it and
            // set the appropriate clipping code
            // then actually clip all polygons to the near clipping plane, this will result
            // in at most 1 additional triangle

            // reset vertex counters, these help in classification
            // of the final triangle 
            num_verts_in = 0;

            // vertex 0
            if (curr_poly->tvlist[0].z > cam->far_clip_z)
            {
                vertex_ccodes[0] = CLIP_CODE_GZ;
            } 
            else
                if (curr_poly->tvlist[0].z < cam->near_clip_z)
                {
                    vertex_ccodes[0] = CLIP_CODE_LZ;
                }
                else
                {
                    vertex_ccodes[0] = CLIP_CODE_IZ;
                    num_verts_in++;
                } 

                // vertex 1
                if (curr_poly->tvlist[1].z > cam->far_clip_z)
                {
                    vertex_ccodes[1] = CLIP_CODE_GZ;
                } 
                else
                    if (curr_poly->tvlist[1].z < cam->near_clip_z)
                    {
                        vertex_ccodes[1] = CLIP_CODE_LZ;
                    }
                    else
                    {
                        vertex_ccodes[1] = CLIP_CODE_IZ;
                        num_verts_in++;
                    }     

                    // vertex 2
                    if (curr_poly->tvlist[2].z > cam->far_clip_z)
                    {
                        vertex_ccodes[2] = CLIP_CODE_GZ;
                    } 
                    else
                        if (curr_poly->tvlist[2].z < cam->near_clip_z)
                        {
                            vertex_ccodes[2] = CLIP_CODE_LZ;
                        }
                        else
                        {
                            vertex_ccodes[2] = CLIP_CODE_IZ;
                            num_verts_in++;
                        } 

                        // test for trivial rejections, polygon completely beyond far or near
                        // z clipping planes
                        if ( ((vertex_ccodes[0] == CLIP_CODE_GZ) && 
                            (vertex_ccodes[1] == CLIP_CODE_GZ) && 
                            (vertex_ccodes[2] == CLIP_CODE_GZ) ) ||

                            ((vertex_ccodes[0] == CLIP_CODE_LZ) && 
                            (vertex_ccodes[1] == CLIP_CODE_LZ) && 
                            (vertex_ccodes[2] == CLIP_CODE_LZ) ) )

                        {
                            // clip the poly completely out of frustrum
                            SET_BIT(curr_poly->state, POLY4DV2_STATE_CLIPPED);

                            // move on to next polygon
                            continue;
                        } // end if

                        // test if any vertex has protruded beyond near clipping plane?
                        if ( ( (vertex_ccodes[0] | vertex_ccodes[1] | vertex_ccodes[2]) & CLIP_CODE_LZ) )
                        {
                            // at this point we are ready to clip the polygon to the near 
                            // clipping plane no need to clip to the far plane since it can't 
                            // possible cause problems. We have two cases: case 1: the triangle 
                            // has 1 vertex interior to the near clipping plane and 2 vertices 
                            // exterior, OR case 2: the triangle has two vertices interior of 
                            // the near clipping plane and 1 exterior

                            // step 1: classify the triangle type based on number of vertices
                            // inside/outside
                            // case 1: easy case :)
                            if (num_verts_in == 1)
                            {
                                // we need to clip the triangle against the near clipping plane
                                // the clipping procedure is done to each edge leading away from
                                // the interior vertex, to clip we need to compute the intersection
                                // with the near z plane, this is done with a parametric equation of 
                                // the edge, once the intersection is computed the old vertex position
                                // is overwritten along with re-computing the texture coordinates, if
                                // there are any, what's nice about this case, is clipping doesn't 
                                // introduce any added vertices, so we can overwrite the old poly
                                // the other case below results in 2 polys, so at very least one has
                                // to be added to the end of the rendering list -- bummer

                                // step 1: find vertex index for interior vertex
                                if ( vertex_ccodes[0] == CLIP_CODE_IZ)
                                { v0 = 0; v1 = 1; v2 = 2; }
                                else 
                                    if (vertex_ccodes[1] == CLIP_CODE_IZ)
                                    { v0 = 1; v1 = 2; v2 = 0; }
                                    else
                                    { v0 = 2; v1 = 0; v2 = 1; }

                                    // step 2: clip each edge
                                    // basically we are going to generate the parametric line p = v0 + v01*t
                                    // then solve for t when the z component is equal to near z, then plug that
                                    // back into to solve for x,y of the 3D line, we could do this with high
                                    // level code and parametric lines, but to save time, lets do it manually

                                    // clip edge v0->v1
                                    VECTOR4D_Build(&curr_poly->tvlist[v0].v, &curr_poly->tvlist[v1].v, &v);                          

                                    // the intersection occurs when z = near z, so t = 
                                    t1 = ( (cam->near_clip_z - curr_poly->tvlist[v0].z) / v.z );

                                    // now plug t back in and find x,y intersection with the plane
                                    xi = curr_poly->tvlist[v0].x + v.x * t1;
                                    yi = curr_poly->tvlist[v0].y + v.y * t1;

                                    // now overwrite vertex with new vertex
                                    curr_poly->tvlist[v1].x = xi;
                                    curr_poly->tvlist[v1].y = yi;
                                    curr_poly->tvlist[v1].z = cam->near_clip_z; 

                                    // clip edge v0->v2
                                    VECTOR4D_Build(&curr_poly->tvlist[v0].v, &curr_poly->tvlist[v2].v, &v);                          

                                    // the intersection occurs when z = near z, so t = 
                                    t2 = ( (cam->near_clip_z - curr_poly->tvlist[v0].z) / v.z );

                                    // now plug t back in and find x,y intersection with the plane
                                    xi = curr_poly->tvlist[v0].x + v.x * t2;
                                    yi = curr_poly->tvlist[v0].y + v.y * t2;

                                    // now overwrite vertex with new vertex
                                    curr_poly->tvlist[v2].x = xi;
                                    curr_poly->tvlist[v2].y = yi;
                                    curr_poly->tvlist[v2].z = cam->near_clip_z; 

                                    // now that we have both t1, t2, check if the poly is textured, if so clip
                                    // texture coordinates
                                    if (curr_poly->attr & POLY4DV2_ATTR_SHADE_MODE_TEXTURE)
                                    {
                                        ui = curr_poly->tvlist[v0].u0 + (curr_poly->tvlist[v1].u0 - curr_poly->tvlist[v0].u0)*t1;
                                        vi = curr_poly->tvlist[v0].v0 + (curr_poly->tvlist[v1].v0 - curr_poly->tvlist[v0].v0)*t1;
                                        curr_poly->tvlist[v1].u0 = ui;
                                        curr_poly->tvlist[v1].v0 = vi;

                                        ui = curr_poly->tvlist[v0].u0 + (curr_poly->tvlist[v2].u0 - curr_poly->tvlist[v0].u0)*t2;
                                        vi = curr_poly->tvlist[v0].v0 + (curr_poly->tvlist[v2].v0 - curr_poly->tvlist[v0].v0)*t2;
                                        curr_poly->tvlist[v2].u0 = ui;
                                        curr_poly->tvlist[v2].v0 = vi;
                                    } // end if textured

                                    // finally, we have obliterated our pre-computed normal length
                                    // it needs to be recomputed!!!!

                                    // build u, v
                                    VECTOR4D_Build(&curr_poly->tvlist[v0].v, &curr_poly->tvlist[v1].v, &u);
                                    VECTOR4D_Build(&curr_poly->tvlist[v0].v, &curr_poly->tvlist[v2].v, &v);

                                    // compute cross product
                                    VECTOR4D_Cross(&u, &v, &n);

                                    // compute length of normal accurately and store in poly nlength
                                    // +- epsilon later to fix over/underflows
                                    curr_poly->nlength = VECTOR4D_Length_Fast(&n); 

                            } // end if
                            else
                                if (num_verts_in == 2)
                                { // num_verts = 2

                                    // must be the case with num_verts_in = 2 
                                    // we need to clip the triangle against the near clipping plane
                                    // the clipping procedure is done to each edge leading away from
                                    // the interior vertex, to clip we need to compute the intersection
                                    // with the near z plane, this is done with a parametric equation of 
                                    // the edge, however unlike case 1 above, the triangle will be split
                                    // into two triangles, thus during the first clip, we will store the 
                                    // results into a new triangle at the end of the rendering list, and 
                                    // then on the last clip we will overwrite the triangle being clipped

                                    // step 0: copy the polygon
                                    memcpy(&temp_poly, curr_poly, sizeof(POLYF4DV2) );

                                    // step 1: find vertex index for exterior vertex
                                    if ( vertex_ccodes[0] == CLIP_CODE_LZ)
                                    { v0 = 0; v1 = 1; v2 = 2; }
                                    else 
                                        if (vertex_ccodes[1] == CLIP_CODE_LZ)
                                        { v0 = 1; v1 = 2; v2 = 0; }
                                        else
                                        { v0 = 2; v1 = 0; v2 = 1; }

                                        // step 2: clip each edge
                                        // basically we are going to generate the parametric line p = v0 + v01*t
                                        // then solve for t when the z component is equal to near z, then plug that
                                        // back into to solve for x,y of the 3D line, we could do this with high
                                        // level code and parametric lines, but to save time, lets do it manually

                                        // clip edge v0->v1
                                        VECTOR4D_Build(&curr_poly->tvlist[v0].v, &curr_poly->tvlist[v1].v, &v);                          

                                        // the intersection occurs when z = near z, so t = 
                                        t1 = ( (cam->near_clip_z - curr_poly->tvlist[v0].z) / v.z );

                                        // now plug t back in and find x,y intersection with the plane
                                        x01i = curr_poly->tvlist[v0].x + v.x * t1;
                                        y01i = curr_poly->tvlist[v0].y + v.y * t1;

                                        // clip edge v0->v2
                                        VECTOR4D_Build(&curr_poly->tvlist[v0].v, &curr_poly->tvlist[v2].v, &v);                          

                                        // the intersection occurs when z = near z, so t = 
                                        t2 = ( (cam->near_clip_z - curr_poly->tvlist[v0].z) / v.z );

                                        // now plug t back in and find x,y intersection with the plane
                                        x02i = curr_poly->tvlist[v0].x + v.x * t2;
                                        y02i = curr_poly->tvlist[v0].y + v.y * t2; 

                                        // now we have both intersection points, we must overwrite the inplace
                                        // polygon's vertex 0 with the intersection point, this poly 1 of 2 from
                                        // the split

                                        // now overwrite vertex with new vertex
                                        curr_poly->tvlist[v0].x = x01i;
                                        curr_poly->tvlist[v0].y = y01i;
                                        curr_poly->tvlist[v0].z = cam->near_clip_z; 

                                        // now comes the hard part, we have to carefully create a new polygon
                                        // from the 2 intersection points and v2, this polygon will be inserted
                                        // at the end of the rendering list, but for now, we are building it up
                                        // in  temp_poly

                                        // so leave v2 alone, but overwrite v1 with v01, and overwrite v0 with v02
                                        temp_poly.tvlist[v1].x = x01i;
                                        temp_poly.tvlist[v1].y = y01i;
                                        temp_poly.tvlist[v1].z = cam->near_clip_z;              

                                        temp_poly.tvlist[v0].x = x02i;
                                        temp_poly.tvlist[v0].y = y02i;
                                        temp_poly.tvlist[v0].z = cam->near_clip_z;    

                                        // now that we have both t1, t2, check if the poly is textured, if so clip
                                        // texture coordinates
                                        if (curr_poly->attr & POLY4DV2_ATTR_SHADE_MODE_TEXTURE)
                                        {
                                            // compute poly 1 new texture coordinates from split
                                            u01i = curr_poly->tvlist[v0].u0 + (curr_poly->tvlist[v1].u0 - curr_poly->tvlist[v0].u0)*t1;
                                            v01i = curr_poly->tvlist[v0].v0 + (curr_poly->tvlist[v1].v0 - curr_poly->tvlist[v0].v0)*t1;

                                            // compute poly 2 new texture coordinates from split
                                            u02i = curr_poly->tvlist[v0].u0 + (curr_poly->tvlist[v2].u0 - curr_poly->tvlist[v0].u0)*t2;
                                            v02i = curr_poly->tvlist[v0].v0 + (curr_poly->tvlist[v2].v0 - curr_poly->tvlist[v0].v0)*t2;

                                            // write them all at the same time         
                                            // poly 1
                                            curr_poly->tvlist[v0].u0 = u01i;
                                            curr_poly->tvlist[v0].v0 = v01i;

                                            // poly 2
                                            temp_poly.tvlist[v0].u0 = u02i;
                                            temp_poly.tvlist[v0].v0 = v02i;
                                            temp_poly.tvlist[v1].u0 = u01i;
                                            temp_poly.tvlist[v1].v0 = v01i;

                                        } // end if textured


                                        // finally, we have obliterated our pre-computed normal lengths
                                        // they need to be recomputed!!!!

                                        // poly 1 first, in place

                                        // build u, v
                                        VECTOR4D_Build(&curr_poly->tvlist[v0].v, &curr_poly->tvlist[v1].v, &u);
                                        VECTOR4D_Build(&curr_poly->tvlist[v0].v, &curr_poly->tvlist[v2].v, &v);

                                        // compute cross product
                                        VECTOR4D_Cross(&u, &v, &n);

                                        // compute length of normal accurately and store in poly nlength
                                        // +- epsilon later to fix over/underflows
                                        curr_poly->nlength = VECTOR4D_Length_Fast(&n); 

                                        // now poly 2, temp_poly
                                        // build u, v
                                        VECTOR4D_Build(&temp_poly.tvlist[v0].v, &temp_poly.tvlist[v1].v, &u);
                                        VECTOR4D_Build(&temp_poly.tvlist[v0].v, &temp_poly.tvlist[v2].v, &v);

                                        // compute cross product
                                        VECTOR4D_Cross(&u, &v, &n);

                                        // compute length of normal accurately and store in poly nlength
                                        // +- epsilon later to fix over/underflows
                                        temp_poly.nlength = VECTOR4D_Length_Fast(&n); 

                                        // now we are good to go, insert the polygon into list
                                        // if the poly won't fit, it won't matter, the function will
                                        // just return 0
                                        Insert_POLYF4DV2_RENDERLIST4DV2(rend_list, &temp_poly);

                                } // end else

                        } // end if near_z clipping has occured

        } // end if z planes

    } // end for poly

} // end Clip_Polys_RENDERLIST4DV2

////////////////////////////////////////////////////////////

int Generate_Terrain_OBJECT4DV2(OBJECT4DV2_PTR obj,     // pointer to object
                                float twidth,            // width in world coords on x-axis
                                float theight,           // height (length) in world coords on z-axis
                                float vscale,           // vertical scale of terrain
                                char *height_map_file,  // filename of height bitmap encoded in 256 colors
                                char *texture_map_file, // filename of texture map
                                int rgbcolor,           // color of terrain if no texture        
                                VECTOR4D_PTR pos,       // initial position
                                VECTOR4D_PTR rot,       // initial rotations
                                int poly_attr)          // the shading attributes we would like
{
    // this function generates a terrain of width x height in the x-z plane
    // the terrain is defined by a height field encoded as color values 
    // of a 256 color texture, 0 being ground level 255 being 1.0, this value
    // is scaled by vscale for the height of each point in the height field
    // the height field generated will be composed of triangles where each vertex
    // in the height field is derived from the height map, thus if the height map
    // is 256 x 256 points then the final mesh will be (256-1) x (256-1) polygons 
    // with a absolute world size of width x height (in the x-z plane)
    // also if there is a texture map file then it will be mapped onto the terrain
    // and texture coordinates will be generated

    char buffer[256];  // working buffer

    float col_tstep, row_tstep;
    float col_vstep, row_vstep;
    int columns, rows;

    int rgbwhite;

    BITMAP_FILE height_bitmap; // holds the height bitmap

    // Step 1: clear out the object and initialize it a bit
    memset(obj, 0, sizeof(OBJECT4DV2));

    // set state of object to active and visible
    obj->state = OBJECT4DV2_STATE_ACTIVE | OBJECT4DV2_STATE_VISIBLE;

    // set position of object
    obj->world_pos.x = pos->x;
    obj->world_pos.y = pos->y;
    obj->world_pos.z = pos->z;
    obj->world_pos.w = pos->w;

    // create proper color word based on selected bit depth of terrain
    // rgbcolor is always in rgb5.6.5 format, so only need to downconvert for
    // 8-bit mesh
    if (poly_attr & POLY4DV1_ATTR_8BITCOLOR)
    { 
        rgbcolor = rgblookup[rgbcolor];
        rgbwhite = rgblookup[RGB16Bit(255,255,255)];
    } // end if
    else
    {
        rgbwhite = RGB16Bit(255,255,255);
    } // end else

    // set number of frames
    obj->num_frames = 1;
    obj->curr_frame = 0;
    obj->attr = OBJECT4DV2_ATTR_SINGLE_FRAME;

    // clear the bitmaps out
    memset(&height_bitmap, 0, sizeof(BITMAP_FILE));
    memset(&bitmap16bit, 0, sizeof(BITMAP_FILE));

    // Step 2: load in the height field
    Load_Bitmap_File(&height_bitmap, height_map_file);

    // compute basic information
    columns = height_bitmap.bitmapinfoheader.biWidth;
    rows    = height_bitmap.bitmapinfoheader.biHeight;

    col_vstep = twidth / (float)(columns - 1);
    row_vstep = theight / (float)(rows - 1);

    sprintf(obj->name ,"Terrain:%s%s", height_map_file, texture_map_file);
    obj->num_vertices = columns * rows;
    obj->num_polys    = ((columns - 1) * (rows - 1) ) * 2;

    // store some results to help with terrain following
    // use the auxialiary variables in the object -- might as well!
    obj->ivar1 = columns;
    obj->ivar2 = rows;
    obj->fvar1 = col_vstep;
    obj->fvar2 = row_vstep;

    // allocate the memory for the vertices and number of polys
    // the call parameters are redundant in this case, but who cares
    if (!Init_OBJECT4DV2(obj,   // object to allocate
        obj->num_vertices, 
        obj->num_polys, 
        obj->num_frames))
    {
        Write_Error("\nTerrain generator error (can't allocate memory).");
    } // end if



    // load texture map if there is one
    if ( (poly_attr & POLY4DV2_ATTR_SHADE_MODE_TEXTURE) && texture_map_file)
    {
        // load the texture from disk
        Load_Bitmap_File(&bitmap16bit, texture_map_file);

        // create a proper size and bitdepth bitmap
        obj->texture = (BITMAP_IMAGE_PTR)malloc(sizeof(BITMAP_IMAGE));
        Create_Bitmap(obj->texture,0,0,
            bitmap16bit.bitmapinfoheader.biWidth,
            bitmap16bit.bitmapinfoheader.biHeight,
            bitmap16bit.bitmapinfoheader.biBitCount);

        // load the bitmap image (later make this 8/16 bit)
        if (obj->texture->bpp == 16)
            Load_Image_Bitmap16(obj->texture, &bitmap16bit,0,0,BITMAP_EXTRACT_MODE_ABS);
        else
        {
            Load_Image_Bitmap(obj->texture, &bitmap16bit,0,0,BITMAP_EXTRACT_MODE_ABS);
        } // end else 8 bit


        // compute stepping factors in texture map for texture coordinate computation
        col_tstep = (float)(bitmap16bit.bitmapinfoheader.biWidth-1)/(float)(columns - 1);
        row_tstep = (float)(bitmap16bit.bitmapinfoheader.biHeight-1)/(float)(rows - 1);

        // flag object as having textures
        SET_BIT(obj->attr, OBJECT4DV2_ATTR_TEXTURES);

        // done, so unload the bitmap
        Unload_Bitmap_File(&bitmap16bit);
    } // end if

    Write_Error("\ncolumns = %d, rows = %d", columns, rows);
    Write_Error("\ncol_vstep = %f, row_vstep = %f", col_vstep, row_vstep);
    Write_Error("\ncol_tstep=%f, row_tstep=%f", col_tstep, row_tstep);
    Write_Error("\nnum_vertices = %d, num_polys = %d", obj->num_vertices, obj->num_polys);

    // Step 4: generate the vertex list, and texture coordinate list in row major form
    for (int curr_row = 0; curr_row < rows; curr_row++)
    {
        for (int curr_col = 0; curr_col < columns; curr_col++)
        {
            int vertex = (curr_row * columns) + curr_col;
            // compute the vertex
            obj->vlist_local[vertex].x = curr_col * col_vstep - (twidth/2);
            obj->vlist_local[vertex].y = vscale*((float)height_bitmap.buffer[curr_col + (curr_row * columns) ]) / 255;
            obj->vlist_local[vertex].z = curr_row * row_vstep - (theight/2);

            obj->vlist_local[vertex].w = 1;  

            // every vertex has a point at least, set that in the flags attribute
            SET_BIT(obj->vlist_local[vertex].attr, VERTEX4DTV1_ATTR_POINT);

            // need texture coord?
            if ( (poly_attr & POLY4DV2_ATTR_SHADE_MODE_TEXTURE) && texture_map_file)
            {
                // now texture coordinates
                obj->tlist[vertex].x = curr_col * col_tstep;
                obj->tlist[vertex].y = curr_row * row_tstep;

            } // end if

            Write_Error("\nVertex %d: V[%f, %f, %f], T[%f, %f]", vertex, obj->vlist_local[vertex].x,
                obj->vlist_local[vertex].y,
                obj->vlist_local[vertex].z,
                obj->tlist[vertex].x,
                obj->tlist[vertex].y);


        } // end for curr_col

    } // end curr_row

    // perform rotation transformation?

    // compute average and max radius
    Compute_OBJECT4DV2_Radius(obj);

    Write_Error("\nObject average radius = %f, max radius = %f", 
        obj->avg_radius[0], obj->max_radius[0]);

    // Step 5: generate the polygon list
    for (int poly=0; poly < obj->num_polys/2; poly++)
    {
        // polygons follow a regular pattern of 2 per square, row
        // major form, finding the correct indices is a pain, but 
        // the bottom line is we have an array of vertices mxn and we need
        // a list of polygons that is (m-1) x (n-1), with 2 triangles per
        // square with a consistent winding order... this is one one to arrive
        // at the indices, another way would be to use two loops, etc., 

        int base_poly_index = (poly % (columns-1)) + (columns * (poly / (columns - 1)) );

        // upper left poly
        obj->plist[poly*2].vert[0] = base_poly_index;
        obj->plist[poly*2].vert[1] = base_poly_index+columns;
        obj->plist[poly*2].vert[2] = base_poly_index+columns+1;

        // lower right poly
        obj->plist[poly*2+1].vert[0] = base_poly_index;
        obj->plist[poly*2+1].vert[1] = base_poly_index+columns+1;
        obj->plist[poly*2+1].vert[2] = base_poly_index+1;

        // point polygon vertex list to object's vertex list
        // note that this is redundant since the polylist is contained
        // within the object in this case and its up to the user to select
        // whether the local or transformed vertex list is used when building up
        // polygon geometry, might be a better idea to set to NULL in the context
        // of polygons that are part of an object
        obj->plist[poly*2].vlist = obj->vlist_local; 
        obj->plist[poly*2+1].vlist = obj->vlist_local; 

        // set attributes of polygon with sent attributes
        obj->plist[poly*2].attr = poly_attr;
        obj->plist[poly*2+1].attr = poly_attr;

        // now perform some test to make sure any secondary data elements are
        // set properly

        // set color of polygon
        obj->plist[poly*2].color = rgbcolor;
        obj->plist[poly*2+1].color = rgbcolor;

        // check for gouraud of phong shading, if so need normals
        if ( (obj->plist[poly*2].attr & POLY4DV2_ATTR_SHADE_MODE_GOURAUD) ||  
            (obj->plist[poly*2].attr & POLY4DV2_ATTR_SHADE_MODE_PHONG) )
        {
            // the vertices from this polygon all need normals, set that in the flags attribute
            SET_BIT(obj->vlist_local[ obj->plist[poly*2].vert[0] ].attr, VERTEX4DTV1_ATTR_NORMAL); 
            SET_BIT(obj->vlist_local[ obj->plist[poly*2].vert[1] ].attr, VERTEX4DTV1_ATTR_NORMAL); 
            SET_BIT(obj->vlist_local[ obj->plist[poly*2].vert[2] ].attr, VERTEX4DTV1_ATTR_NORMAL); 

            SET_BIT(obj->vlist_local[ obj->plist[poly*2+1].vert[0] ].attr, VERTEX4DTV1_ATTR_NORMAL); 
            SET_BIT(obj->vlist_local[ obj->plist[poly*2+1].vert[1] ].attr, VERTEX4DTV1_ATTR_NORMAL); 
            SET_BIT(obj->vlist_local[ obj->plist[poly*2+1].vert[2] ].attr, VERTEX4DTV1_ATTR_NORMAL); 

        } // end if

        // if texture in enabled the enable texture coordinates
        if (poly_attr & POLY4DV2_ATTR_SHADE_MODE_TEXTURE)
        {
            // apply texture to this polygon
            obj->plist[poly*2].texture = obj->texture;
            obj->plist[poly*2+1].texture = obj->texture;

            // assign the texture coordinates
            // upper left poly
            obj->plist[poly*2].text[0] = base_poly_index;
            obj->plist[poly*2].text[1] = base_poly_index+columns;
            obj->plist[poly*2].text[2] = base_poly_index+columns+1;

            // lower right poly
            obj->plist[poly*2+1].text[0] = base_poly_index;
            obj->plist[poly*2+1].text[1] = base_poly_index+columns+1;
            obj->plist[poly*2+1].text[2] = base_poly_index+1;

            // override base color to make poly more reflective
            obj->plist[poly*2].color = rgbwhite;
            obj->plist[poly*2+1].color = rgbwhite;

            // set texture coordinate attributes
            SET_BIT(obj->vlist_local[ obj->plist[poly*2].vert[0] ].attr, VERTEX4DTV1_ATTR_TEXTURE); 
            SET_BIT(obj->vlist_local[ obj->plist[poly*2].vert[1] ].attr, VERTEX4DTV1_ATTR_TEXTURE); 
            SET_BIT(obj->vlist_local[ obj->plist[poly*2].vert[2] ].attr, VERTEX4DTV1_ATTR_TEXTURE); 

            SET_BIT(obj->vlist_local[ obj->plist[poly*2+1].vert[0] ].attr, VERTEX4DTV1_ATTR_TEXTURE); 
            SET_BIT(obj->vlist_local[ obj->plist[poly*2+1].vert[1] ].attr, VERTEX4DTV1_ATTR_TEXTURE); 
            SET_BIT(obj->vlist_local[ obj->plist[poly*2+1].vert[2] ].attr, VERTEX4DTV1_ATTR_TEXTURE); 

        } // end if

        // set the material mode to ver. 1.0 emulation
        SET_BIT(obj->plist[poly*2].attr, POLY4DV2_ATTR_DISABLE_MATERIAL);
        SET_BIT(obj->plist[poly*2+1].attr, POLY4DV2_ATTR_DISABLE_MATERIAL);

        // finally set the polygon to active
        obj->plist[poly*2].state = POLY4DV2_STATE_ACTIVE;    
        obj->plist[poly*2+1].state = POLY4DV2_STATE_ACTIVE;  

        // point polygon vertex list to object's vertex list
        // note that this is redundant since the polylist is contained
        // within the object in this case and its up to the user to select
        // whether the local or transformed vertex list is used when building up
        // polygon geometry, might be a better idea to set to NULL in the context
        // of polygons that are part of an object
        obj->plist[poly*2].vlist = obj->vlist_local; 
        obj->plist[poly*2+1].vlist = obj->vlist_local; 

        // set texture coordinate list, this is needed
        obj->plist[poly*2].tlist = obj->tlist;
        obj->plist[poly*2+1].tlist = obj->tlist;

    } // end for poly
#if 0
    for (poly=0; poly < obj->num_polys; poly++)
    {
        Write_Error("\nPoly %d: Vi[%d, %d, %d], Ti[%d, %d, %d]",poly,
            obj->plist[poly].vert[0],
            obj->plist[poly].vert[1],
            obj->plist[poly].vert[2],
            obj->plist[poly].text[0],
            obj->plist[poly].text[1],
            obj->plist[poly].text[2]);

    } // end 
#endif

    // compute the polygon normal lengths
    Compute_OBJECT4DV2_Poly_Normals(obj);

    // compute vertex normals for any gouraud shaded polys
    Compute_OBJECT4DV2_Vertex_Normals(obj);

    // return success
    return(1);

} // end Generate_Terrain_OBJECT4DV2

//////////////////////////////////////////////////////////////////////////////

int Init_Light_LIGHTV2(LIGHTV2_PTR   lights,     // light array to work with (new)
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
                       float        _pf)         // power factor/falloff for spot lights
{
    // this function initializes a light based on the flags sent in _attr, values that
    // aren't needed are set to 0 by caller, nearly identical to version 1.0, however has
    // new support for transformed light coordinates 

    // make sure light is in range 
    if (index < 0 || index >= MAX_LIGHTS)
        return(0);

    // all good, initialize the light (many fields may be dead)
    lights[index].state       = _state;      // state of light
    lights[index].id          = index;       // id of light
    lights[index].attr        = _attr;       // type of light, and extra qualifiers

    lights[index].c_ambient   = _c_ambient;  // ambient light intensity
    lights[index].c_diffuse   = _c_diffuse;  // diffuse light intensity
    lights[index].c_specular  = _c_specular; // specular light intensity

    lights[index].kc          = _kc;         // constant, linear, and quadratic attenuation factors
    lights[index].kl          = _kl;   
    lights[index].kq          = _kq;   

    if (_pos)
    {
        VECTOR4D_COPY(&lights[index].pos, _pos);  // position of light
        VECTOR4D_COPY(&lights[index].tpos, _pos);
    } // end if

    if (_dir)
    {
        VECTOR4D_COPY(&lights[index].dir, _dir);  // direction of light

        // normalize it
        VECTOR4D_Normalize(&lights[index].dir);
        VECTOR4D_COPY(&lights[index].tdir, &lights[index].dir); 
    } // end if

    lights[index].spot_inner  = _spot_inner; // inner angle for spot light
    lights[index].spot_outer  = _spot_outer; // outer angle for spot light
    lights[index].pf          = _pf;         // power factor/falloff for spot lights

    // return light index as success
    return(index);

} // end Create_Light_LIGHTV2

//////////////////////////////////////////////////////////////////////////////

int Reset_Lights_LIGHTV2(LIGHTV2_PTR lights,       // light array to work with (new)
                         int max_lights)           // number of lights in system
{
    // this function simply resets all lights in the system
    static int first_time = 1;

    memset(lights, 0, max_lights*sizeof(LIGHTV2));

    // reset number of lights global
    num_lights = 0;

    // reset first time
    first_time = 0;

    // return success
    return(1);

} // end Reset_Lights_LIGHTV2

//////////////////////////////////////////////////////////////////////////////


int Light_OBJECT4DV2_World2_16(OBJECT4DV2_PTR obj,  // object to process
                               CAM4DV1_PTR cam,     // camera position
                               LIGHTV2_PTR lights,  // light list (might have more than one)
                               int max_lights)      // maximum lights in list
{
    // {andre work in progress }

    // 16-bit version of function
    // function lights an object based on the sent lights and camera. the function supports
    // constant/pure shading (emmisive), flat shading with ambient, infinite, point lights, and spot lights
    // note that this lighting function is rather brute force and simply follows the math, however
    // there are some clever integer operations that are used in scale 256 rather than going to floating
    // point, but why? floating point and ints are the same speed, HOWEVER, the conversion to and from floating
    // point can be cycle intensive, so if you can keep your calcs in ints then you can gain some speed
    // also note, type 1 spot lights are simply point lights with direction, the "cone" is more of a function
    // of the falloff due to attenuation, but they still look like spot lights
    // type 2 spot lights are implemented with the intensity having a dot product relationship with the
    // angle from the surface point to the light direction just like in the optimized model, but the pf term
    // that is used for a concentration control must be 1,2,3,.... integral and non-fractional


    unsigned int r_base, g_base,   b_base,  // base color being lit
        r_sum,  g_sum,    b_sum,   // sum of lighting process over all lights
        r_sum0,  g_sum0,  b_sum0,
        r_sum1,  g_sum1,  b_sum1,
        r_sum2,  g_sum2,  b_sum2,
        ri,gi,bi,
        shaded_color;            // final color

    float dp,     // dot product 
        dist,   // distance from light to surface
        dists, 
        i,      // general intensities
        nl,     // length of normal
        atten;  // attenuation computations

    VECTOR4D u, v, n, l, d, s; // used for cross product and light vector calculations

    //Write_Error("\nEntering lighting function");


    // test if the object is culled
    if (!(obj->state & OBJECT4DV2_STATE_ACTIVE) ||
        (obj->state & OBJECT4DV2_STATE_CULLED) ||
        !(obj->state & OBJECT4DV2_STATE_VISIBLE))
        return(0); 

    // for each valid poly, light it...
    for (int poly=0; poly < obj->num_polys; poly++)
    {
        // acquire polygon
        POLY4DV2_PTR curr_poly = &obj->plist[poly];

        // light this polygon if and only if it's not clipped, not culled,
        // active, and visible
        if (!(curr_poly->state & POLY4DV2_STATE_ACTIVE) ||
            (curr_poly->state & POLY4DV2_STATE_CLIPPED ) ||
            (curr_poly->state & POLY4DV2_STATE_BACKFACE) )
            continue; // move onto next poly

        // set state of polygon to lit, so we don't light again in renderlist
        // lighting system if it happens to get called
        SET_BIT(curr_poly->state, POLY4DV2_STATE_LIT);

        // extract vertex indices into master list, rember the polygons are 
        // NOT self contained, but based on the vertex list stored in the object
        // itself
        int vindex_0 = curr_poly->vert[0];
        int vindex_1 = curr_poly->vert[1];
        int vindex_2 = curr_poly->vert[2];

        // we will use the transformed polygon vertex list since the backface removal
        // only makes sense at the world coord stage further of the pipeline 

        //Write_Error("\npoly %d",poly);

        // we will use the transformed polygon vertex list since the backface removal
        // only makes sense at the world coord stage further of the pipeline 

        // test the lighting mode of the polygon (use flat for flat, gouraud))
        if (curr_poly->attr & POLY4DV2_ATTR_SHADE_MODE_FLAT)
        {
            //Write_Error("\nEntering Flat Shader");

            // step 1: extract the base color out in RGB mode, assume RGB 565
            _RGB565FROM16BIT(curr_poly->color, &r_base, &g_base, &b_base);

            // scale to 8 bit 
            r_base <<= 3;
            g_base <<= 2;
            b_base <<= 3;

            //Write_Error("\nBase color=%d,%d,%d", r_base, g_base, b_base);

            // initialize color sum
            r_sum  = 0;
            g_sum  = 0;
            b_sum  = 0;

            //Write_Error("\nsum color=%d,%d,%d", r_sum, g_sum, b_sum);

            // new optimization:
            // when there are multiple lights in the system we will end up performing numerous
            // redundant calculations to minimize this my strategy is to set key variables to 
            // to MAX values on each loop, then during the lighting calcs to test the vars for
            // the max value, if they are the max value then the first light that needs the math
            // will do it, and then save the information into the variable (causing it to change state
            // from an invalid number) then any other lights that need the math can use the previously
            // computed value

            // set surface normal.z to FLT_MAX to flag it as non-computed
            n.z = FLT_MAX;

            // loop thru lights
            for (int curr_light = 0; curr_light < max_lights; curr_light++)
            {
                // is this light active
                if (lights[curr_light].state==LIGHTV2_STATE_OFF)
                    continue;

                //Write_Error("\nprocessing light %d",curr_light);

                // what kind of light are we dealing with
                if (lights[curr_light].attr & LIGHTV2_ATTR_AMBIENT)
                {
                    //Write_Error("\nEntering ambient light...");

                    // simply multiply each channel against the color of the 
                    // polygon then divide by 256 to scale back to 0..255
                    // use a shift in real life!!! >> 8
                    r_sum+= ((lights[curr_light].c_ambient.r * r_base) / 256);
                    g_sum+= ((lights[curr_light].c_ambient.g * g_base) / 256);
                    b_sum+= ((lights[curr_light].c_ambient.b * b_base) / 256);

                    //Write_Error("\nambient sum=%d,%d,%d", r_sum, g_sum, b_sum);

                    // there better only be one ambient light!

                } // end if
                else
                    if (lights[curr_light].attr & LIGHTV2_ATTR_INFINITE) ///////////////////////////////////////////
                    {
                        //Write_Error("\nEntering infinite light...");

                        // infinite lighting, we need the surface normal, and the direction
                        // of the light source

                        // test if we already computed poly normal in previous calculation
                        if (n.z==FLT_MAX)       
                        {
                            // we need to compute the normal of this polygon face, and recall
                            // that the vertices are in cw order, u=p0->p1, v=p0->p2, n=uxv

                            // build u, v
                            VECTOR4D_Build(&obj->vlist_trans[ vindex_0].v, &obj->vlist_trans[ vindex_1].v, &u);
                            VECTOR4D_Build(&obj->vlist_trans[ vindex_0].v, &obj->vlist_trans[ vindex_2].v, &v);

                            // compute cross product
                            VECTOR4D_Cross(&u, &v, &n);
                        } // end if

                        // at this point, we are almost ready, but we have to normalize the normal vector!
                        // this is a key optimization we can make later, we can pre-compute the length of all polygon
                        // normals, so this step can be optimized
                        // compute length of normal
                        //nl = VECTOR4D_Length_Fast2(&n);
                        nl = curr_poly->nlength;  

                        // ok, recalling the lighting model for infinite lights
                        // I(d)dir = I0dir * Cldir
                        // and for the diffuse model
                        // Itotald =   Rsdiffuse*Idiffuse * (n . l)
                        // so we basically need to multiple it all together
                        // notice the scaling by 128, I want to avoid floating point calculations, not because they 
                        // are slower, but the conversion to and from cost cycles

                        dp = VECTOR4D_Dot(&n, &lights[curr_light].tdir);

                        // only add light if dp > 0
                        if (dp > 0)
                        { 
                            i = 128*dp/nl; 
                            r_sum+= (lights[curr_light].c_diffuse.r * r_base * i) / (256*128);
                            g_sum+= (lights[curr_light].c_diffuse.g * g_base * i) / (256*128);
                            b_sum+= (lights[curr_light].c_diffuse.b * b_base * i) / (256*128);
                        } // end if

                        //Write_Error("\ninfinite sum=%d,%d,%d", r_sum, g_sum, b_sum);

                    } // end if infinite light
                    else
                        if (lights[curr_light].attr & LIGHTV2_ATTR_POINT) ///////////////////////////////////////
                        {
                            //Write_Error("\nEntering point light...");

                            // perform point light computations
                            // light model for point light is once again:
                            //              I0point * Clpoint
                            //  I(d)point = ___________________
                            //              kc +  kl*d + kq*d2              
                            //
                            //  Where d = |p - s|
                            // thus it's almost identical to the infinite light, but attenuates as a function
                            // of distance from the point source to the surface point being lit

                            // test if we already computed poly normal in previous calculation
                            if (n.z==FLT_MAX)       
                            {
                                // we need to compute the normal of this polygon face, and recall
                                // that the vertices are in cw order, u=p0->p1, v=p0->p2, n=uxv

                                // build u, v
                                VECTOR4D_Build(&obj->vlist_trans[ vindex_0].v, &obj->vlist_trans[ vindex_1].v, &u);
                                VECTOR4D_Build(&obj->vlist_trans[ vindex_0].v, &obj->vlist_trans[ vindex_2].v, &v);

                                // compute cross product
                                VECTOR4D_Cross(&u, &v, &n);
                            } // end if

                            // at this point, we are almost ready, but we have to normalize the normal vector!
                            // this is a key optimization we can make later, we can pre-compute the length of all polygon
                            // normals, so this step can be optimized
                            // compute length of normal
                            //nl = VECTOR4D_Length_Fast2(&n);
                            nl = curr_poly->nlength;  

                            // compute vector from surface to light
                            VECTOR4D_Build(&obj->vlist_trans[ vindex_0].v, &lights[curr_light].tpos, &l);

                            // compute distance and attenuation
                            dist = VECTOR4D_Length_Fast2(&l);  

                            // and for the diffuse model
                            // Itotald =   Rsdiffuse*Idiffuse * (n . l)
                            // so we basically need to multiple it all together
                            // notice the scaling by 128, I want to avoid floating point calculations, not because they 
                            // are slower, but the conversion to and from cost cycles
                            dp = VECTOR4D_Dot(&n, &l);

                            // only add light if dp > 0
                            if (dp > 0)
                            { 
                                atten =  (lights[curr_light].kc + lights[curr_light].kl*dist + lights[curr_light].kq*dist*dist);    

                                i = 128*dp / (nl * dist * atten ); 

                                r_sum += (lights[curr_light].c_diffuse.r * r_base * i) / (256*128);
                                g_sum += (lights[curr_light].c_diffuse.g * g_base * i) / (256*128);
                                b_sum += (lights[curr_light].c_diffuse.b * b_base * i) / (256*128);
                            } // end if


                            //Write_Error("\npoint sum=%d,%d,%d",r_sum,g_sum,b_sum);

                        } // end if point
                        else
                            if (lights[curr_light].attr & LIGHTV2_ATTR_SPOTLIGHT1) ////////////////////////////////////
                            {
                                //Write_Error("\nentering spot light1...");

                                // perform spotlight/point computations simplified model that uses
                                // point light WITH a direction to simulate a spotlight
                                // light model for point light is once again:
                                //              I0point * Clpoint
                                //  I(d)point = ___________________
                                //              kc +  kl*d + kq*d2              
                                //
                                //  Where d = |p - s|
                                // thus it's almost identical to the infinite light, but attenuates as a function
                                // of distance from the point source to the surface point being lit

                                // test if we already computed poly normal in previous calculation
                                if (n.z==FLT_MAX)       
                                {
                                    // we need to compute the normal of this polygon face, and recall
                                    // that the vertices are in cw order, u=p0->p1, v=p0->p2, n=uxv

                                    // build u, v
                                    VECTOR4D_Build(&obj->vlist_trans[ vindex_0].v, &obj->vlist_trans[ vindex_1].v, &u);
                                    VECTOR4D_Build(&obj->vlist_trans[ vindex_0].v, &obj->vlist_trans[ vindex_2].v, &v);

                                    // compute cross product
                                    VECTOR4D_Cross(&u, &v, &n);
                                } // end if

                                // at this point, we are almost ready, but we have to normalize the normal vector!
                                // this is a key optimization we can make later, we can pre-compute the length of all polygon
                                // normals, so this step can be optimized
                                // compute length of normal
                                //nl = VECTOR4D_Length_Fast2(&n);
                                nl = curr_poly->nlength;  

                                // compute vector from surface to light
                                VECTOR4D_Build(&obj->vlist_trans[ vindex_0].v, &lights[curr_light].tpos, &l);

                                // compute distance and attenuation
                                dist = VECTOR4D_Length_Fast2(&l);  

                                // and for the diffuse model
                                // Itotald =   Rsdiffuse*Idiffuse * (n . l)
                                // so we basically need to multiple it all together
                                // notice the scaling by 128, I want to avoid floating point calculations, not because they 
                                // are slower, but the conversion to and from cost cycles

                                // note that I use the direction of the light here rather than a the vector to the light
                                // thus we are taking orientation into account which is similar to the spotlight model
                                dp = VECTOR4D_Dot(&n, &lights[curr_light].tdir);

                                // only add light if dp > 0
                                if (dp > 0)
                                { 
                                    atten =  (lights[curr_light].kc + lights[curr_light].kl*dist + lights[curr_light].kq*dist*dist);    

                                    i = 128*dp / (nl * atten ); 

                                    r_sum += (lights[curr_light].c_diffuse.r * r_base * i) / (256*128);
                                    g_sum += (lights[curr_light].c_diffuse.g * g_base * i) / (256*128);
                                    b_sum += (lights[curr_light].c_diffuse.b * b_base * i) / (256*128);
                                } // end if

                                //Write_Error("\nspotlight sum=%d,%d,%d",r_sum, g_sum, b_sum);

                            } // end if spotlight1
                            else
                                if (lights[curr_light].attr & LIGHTV2_ATTR_SPOTLIGHT2) // simple version ////////////////////
                                {
                                    //Write_Error("\nEntering spotlight2 ...");

                                    // perform spot light computations
                                    // light model for spot light simple version is once again:
                                    //         	     I0spotlight * Clspotlight * MAX( (l . s), 0)^pf                     
                                    // I(d)spotlight = __________________________________________      
                                    //               		 kc + kl*d + kq*d2        
                                    // Where d = |p - s|, and pf = power factor

                                    // thus it's almost identical to the point, but has the extra term in the numerator
                                    // relating the angle between the light source and the point on the surface

                                    // test if we already computed poly normal in previous calculation
                                    if (n.z==FLT_MAX)       
                                    {
                                        // we need to compute the normal of this polygon face, and recall
                                        // that the vertices are in cw order, u=p0->p1, v=p0->p2, n=uxv

                                        // build u, v
                                        VECTOR4D_Build(&obj->vlist_trans[ vindex_0].v, &obj->vlist_trans[ vindex_1].v, &u);
                                        VECTOR4D_Build(&obj->vlist_trans[ vindex_0].v, &obj->vlist_trans[ vindex_2].v, &v);

                                        // compute cross product
                                        VECTOR4D_Cross(&u, &v, &n);
                                    } // end if

                                    // at this point, we are almost ready, but we have to normalize the normal vector!
                                    // this is a key optimization we can make later, we can pre-compute the length of all polygon
                                    // normals, so this step can be optimized
                                    // compute length of normal
                                    //nl = VECTOR4D_Length_Fast2(&n);
                                    nl = curr_poly->nlength;  

                                    // and for the diffuse model
                                    // Itotald =   Rsdiffuse*Idiffuse * (n . l)
                                    // so we basically need to multiple it all together
                                    // notice the scaling by 128, I want to avoid floating point calculations, not because they 
                                    // are slower, but the conversion to and from cost cycles
                                    dp = VECTOR4D_Dot(&n, &lights[curr_light].tdir);

                                    // only add light if dp > 0
                                    if (dp > 0)
                                    { 
                                        // compute vector from light to surface (different from l which IS the light dir)
                                        VECTOR4D_Build( &lights[curr_light].tpos, &obj->vlist_trans[ vindex_0].v, &s);

                                        // compute length of s (distance to light source) to normalize s for lighting calc
                                        dists = VECTOR4D_Length_Fast2(&s);  

                                        // compute spot light term (s . l)
                                        float dpsl = VECTOR4D_Dot(&s, &lights[curr_light].tdir)/dists;

                                        // proceed only if term is positive
                                        if (dpsl > 0) 
                                        {
                                            // compute attenuation
                                            atten = (lights[curr_light].kc + lights[curr_light].kl*dists + lights[curr_light].kq*dists*dists);    

                                            // for speed reasons, pf exponents that are less that 1.0 are out of the question, and exponents
                                            // must be integral
                                            float dpsl_exp = dpsl;

                                            // exponentiate for positive integral powers
                                            for (int e_index = 1; e_index < (int)lights[curr_light].pf; e_index++)
                                                dpsl_exp*=dpsl;

                                            // now dpsl_exp holds (dpsl)^pf power which is of course (s . l)^pf 

                                            i = 128*dp * dpsl_exp / (nl * atten ); 

                                            r_sum += (lights[curr_light].c_diffuse.r * r_base * i) / (256*128);
                                            g_sum += (lights[curr_light].c_diffuse.g * g_base * i) / (256*128);
                                            b_sum += (lights[curr_light].c_diffuse.b * b_base * i) / (256*128);

                                        } // end if

                                    } // end if

                                    //Write_Error("\nSpotlight sum=%d,%d,%d",r_sum, g_sum, b_sum);

                                } // end if spot light

            } // end for light

            // make sure colors aren't out of range
            if (r_sum  > 255) r_sum = 255;
            if (g_sum  > 255) g_sum = 255;
            if (b_sum  > 255) b_sum = 255;

            //Write_Error("\nWriting final values to polygon %d = %d,%d,%d", poly, r_sum, g_sum, b_sum);

            // write the color over current color
            curr_poly->lit_color[0] = RGB16Bit(r_sum, g_sum, b_sum);

        } // end if
        else
            if (curr_poly->attr & POLY4DV2_ATTR_SHADE_MODE_GOURAUD) /////////////////////////////////
            {
                // gouraud shade, unfortunetly at this point in the pipeline, we have lost the original
                // mesh, and only have triangles, thus, many triangles will share the same vertices and
                // they will get lit 2x since we don't have any way to tell this, alas, performing lighting
                // at the object level is a better idea when gouraud shading is performed since the 
                // commonality of vertices is still intact, in any case, lighting here is similar to polygon
                // flat shaded, but we do it 3 times, once for each vertex, additionally there are lots
                // of opportunities for optimization, but I am going to lay off them for now, so the code
                // is intelligible, later we will optimize

                //Write_Error("\nEntering gouraud shader...");

                // step 1: extract the base color out in RGB mode
                // assume 565 format
                _RGB565FROM16BIT(curr_poly->color, &r_base, &g_base, &b_base);

                // scale to 8 bit 
                r_base <<= 3;
                g_base <<= 2;
                b_base <<= 3;

                //Write_Error("\nBase color=%d, %d, %d", r_base, g_base, b_base);

                // initialize color sum(s) for vertices
                r_sum0  = 0;
                g_sum0  = 0;
                b_sum0  = 0;

                r_sum1  = 0;
                g_sum1  = 0;
                b_sum1  = 0;

                r_sum2  = 0;
                g_sum2  = 0;
                b_sum2  = 0;

                //Write_Error("\nColor sums rgb0[%d, %d, %d], rgb1[%d,%d,%d], rgb2[%d,%d,%d]", r_sum0, g_sum0, b_sum0,   r_sum1, g_sum1, b_sum1, r_sum2, g_sum2, b_sum2);

                // new optimization:
                // when there are multiple lights in the system we will end up performing numerous
                // redundant calculations to minimize this my strategy is to set key variables to 
                // to MAX values on each loop, then during the lighting calcs to test the vars for
                // the max value, if they are the max value then the first light that needs the math
                // will do it, and then save the information into the variable (causing it to change state
                // from an invalid number) then any other lights that need the math can use the previously
                // computed value

                // loop thru lights
                for (int curr_light = 0; curr_light < max_lights; curr_light++)
                {
                    // is this light active
                    if (lights[curr_light].state==LIGHTV2_STATE_OFF)
                        continue;

                    //Write_Error("\nprocessing light %d", curr_light);

                    // what kind of light are we dealing with
                    if (lights[curr_light].attr & LIGHTV2_ATTR_AMBIENT) ///////////////////////////////
                    {
                        //Write_Error("\nEntering ambient light....");

                        // simply multiply each channel against the color of the 
                        // polygon then divide by 256 to scale back to 0..255
                        // use a shift in real life!!! >> 8
                        ri = ((lights[curr_light].c_ambient.r * r_base) / 256);
                        gi = ((lights[curr_light].c_ambient.g * g_base) / 256);
                        bi = ((lights[curr_light].c_ambient.b * b_base) / 256);

                        // ambient light has the same affect on each vertex
                        r_sum0+=ri;
                        g_sum0+=gi;
                        b_sum0+=bi;

                        r_sum1+=ri;
                        g_sum1+=gi;
                        b_sum1+=bi;

                        r_sum2+=ri;
                        g_sum2+=gi;
                        b_sum2+=bi;

                        // there better only be one ambient light!
                        //Write_Error("\nexiting ambient ,sums rgb0[%d, %d, %d], rgb1[%d,%d,%d], rgb2[%d,%d,%d]", r_sum0, g_sum0, b_sum0,  r_sum1, g_sum1, b_sum1,  r_sum2, g_sum2, b_sum2);

                    } // end if
                    else
                        if (lights[curr_light].attr & LIGHTV2_ATTR_INFINITE) /////////////////////////////////
                        {
                            //Write_Error("\nentering infinite light...");

                            // infinite lighting, we need the surface normal, and the direction
                            // of the light source

                            // no longer need to compute normal or length, we already have the vertex normal
                            // and it's length is 1.0  
                            // ....

                            // ok, recalling the lighting model for infinite lights
                            // I(d)dir = I0dir * Cldir
                            // and for the diffuse model
                            // Itotald =   Rsdiffuse*Idiffuse * (n . l)
                            // so we basically need to multiple it all together
                            // notice the scaling by 128, I want to avoid floating point calculations, not because they 
                            // are slower, but the conversion to and from cost cycles

                            // need to perform lighting for each vertex (lots of redundant math, optimize later!)

                            //Write_Error("\nv0=[%f, %f, %f]=%f, v1=[%f, %f, %f]=%f, v2=[%f, %f, %f]=%f",
                            // curr_poly->tvlist[0].n.x, curr_poly->tvlist[0].n.y,curr_poly->tvlist[0].n.z, VECTOR4D_Length(&curr_poly->tvlist[0].n),
                            // curr_poly->tvlist[1].n.x, curr_poly->tvlist[1].n.y,curr_poly->tvlist[1].n.z, VECTOR4D_Length(&curr_poly->tvlist[1].n),
                            // curr_poly->tvlist[2].n.x, curr_poly->tvlist[2].n.y,curr_poly->tvlist[2].n.z, VECTOR4D_Length(&curr_poly->tvlist[2].n) );

                            // vertex 0
                            dp = VECTOR4D_Dot(&obj->vlist_trans[ vindex_0].n, &lights[curr_light].tdir); 

                            // only add light if dp > 0
                            if (dp > 0)
                            { 
                                i = 128*dp; 
                                r_sum0+= (lights[curr_light].c_diffuse.r * r_base * i) / (256*128);
                                g_sum0+= (lights[curr_light].c_diffuse.g * g_base * i) / (256*128);
                                b_sum0+= (lights[curr_light].c_diffuse.b * b_base * i) / (256*128);
                            } // end if

                            // vertex 1
                            dp = VECTOR4D_Dot(&obj->vlist_trans[ vindex_1].n, &lights[curr_light].tdir);

                            // only add light if dp > 0
                            if (dp > 0)
                            { 
                                i = 128*dp; 
                                r_sum1+= (lights[curr_light].c_diffuse.r * r_base * i) / (256*128);
                                g_sum1+= (lights[curr_light].c_diffuse.g * g_base * i) / (256*128);
                                b_sum1+= (lights[curr_light].c_diffuse.b * b_base * i) / (256*128);
                            } // end if

                            // vertex 2
                            dp = VECTOR4D_Dot(&obj->vlist_trans[ vindex_2].n, &lights[curr_light].tdir);

                            // only add light if dp > 0
                            if (dp > 0)
                            { 
                                i = 128*dp; 
                                r_sum2+= (lights[curr_light].c_diffuse.r * r_base * i) / (256*128);
                                g_sum2+= (lights[curr_light].c_diffuse.g * g_base * i) / (256*128);
                                b_sum2+= (lights[curr_light].c_diffuse.b * b_base * i) / (256*128);
                            } // end if

                            //Write_Error("\nexiting infinite, color sums rgb0[%d, %d, %d], rgb1[%d,%d,%d], rgb2[%d,%d,%d]", r_sum0, g_sum0, b_sum0,  r_sum1, g_sum1, b_sum1,  r_sum2, g_sum2, b_sum2);

                        } // end if infinite light
                        else
                            if (lights[curr_light].attr & LIGHTV2_ATTR_POINT) //////////////////////////////////////
                            {
                                // perform point light computations
                                // light model for point light is once again:
                                //              I0point * Clpoint
                                //  I(d)point = ___________________
                                //              kc +  kl*d + kq*d2              
                                //
                                //  Where d = |p - s|
                                // thus it's almost identical to the infinite light, but attenuates as a function
                                // of distance from the point source to the surface point being lit

                                // .. normal already in vertex

                                //Write_Error("\nEntering point light....");

                                // compute vector from surface to light
                                VECTOR4D_Build(&obj->vlist_trans[ vindex_0].v, &lights[curr_light].tpos, &l);

                                // compute distance and attenuation
                                dist = VECTOR4D_Length_Fast2(&l);  

                                // and for the diffuse model
                                // Itotald =   Rsdiffuse*Idiffuse * (n . l)
                                // so we basically need to multiple it all together
                                // notice the scaling by 128, I want to avoid floating point calculations, not because they 
                                // are slower, but the conversion to and from cost cycles

                                // perform the calculation for all 3 vertices

                                // vertex 0
                                dp = VECTOR4D_Dot(&obj->vlist_trans[ vindex_0].n, &l);

                                // only add light if dp > 0
                                if (dp > 0)
                                { 
                                    atten =  (lights[curr_light].kc + lights[curr_light].kl*dist + lights[curr_light].kq*dist*dist);    

                                    i = 128*dp / (dist * atten ); 

                                    r_sum0 += (lights[curr_light].c_diffuse.r * r_base * i) / (256*128);
                                    g_sum0 += (lights[curr_light].c_diffuse.g * g_base * i) / (256*128);
                                    b_sum0 += (lights[curr_light].c_diffuse.b * b_base * i) / (256*128);
                                } // end if


                                // vertex 1
                                dp = VECTOR4D_Dot(&obj->vlist_trans[ vindex_1].n, &l);

                                // only add light if dp > 0
                                if (dp > 0)
                                { 
                                    atten =  (lights[curr_light].kc + lights[curr_light].kl*dist + lights[curr_light].kq*dist*dist);    

                                    i = 128*dp / (dist * atten ); 

                                    r_sum1 += (lights[curr_light].c_diffuse.r * r_base * i) / (256*128);
                                    g_sum1 += (lights[curr_light].c_diffuse.g * g_base * i) / (256*128);
                                    b_sum1 += (lights[curr_light].c_diffuse.b * b_base * i) / (256*128);
                                } // end if

                                // vertex 2
                                dp = VECTOR4D_Dot(&obj->vlist_trans[ vindex_2].n, &l);

                                // only add light if dp > 0
                                if (dp > 0)
                                { 
                                    atten =  (lights[curr_light].kc + lights[curr_light].kl*dist + lights[curr_light].kq*dist*dist);    

                                    i = 128*dp / (dist * atten ); 

                                    r_sum2 += (lights[curr_light].c_diffuse.r * r_base * i) / (256*128);
                                    g_sum2 += (lights[curr_light].c_diffuse.g * g_base * i) / (256*128);
                                    b_sum2 += (lights[curr_light].c_diffuse.b * b_base * i) / (256*128);
                                } // end if

                                //Write_Error("\nexiting point light, rgb0[%d, %d, %d], rgb1[%d,%d,%d], rgb2[%d,%d,%d]", r_sum0, g_sum0, b_sum0,  r_sum1, g_sum1, b_sum1, r_sum2, g_sum2, b_sum2);

                            } // end if point
                            else
                                if (lights[curr_light].attr & LIGHTV2_ATTR_SPOTLIGHT1) ///////////////////////////////////////
                                {
                                    // perform spotlight/point computations simplified model that uses
                                    // point light WITH a direction to simulate a spotlight
                                    // light model for point light is once again:
                                    //              I0point * Clpoint
                                    //  I(d)point = ___________________
                                    //              kc +  kl*d + kq*d2              
                                    //
                                    //  Where d = |p - s|
                                    // thus it's almost identical to the infinite light, but attenuates as a function
                                    // of distance from the point source to the surface point being lit

                                    //Write_Error("\nentering spotlight1....");

                                    // .. normal is already computed

                                    // compute vector from surface to light
                                    VECTOR4D_Build(&obj->vlist_trans[ vindex_0].v, &lights[curr_light].tpos, &l);

                                    // compute distance and attenuation
                                    dist = VECTOR4D_Length_Fast2(&l);  

                                    // and for the diffuse model
                                    // Itotald =   Rsdiffuse*Idiffuse * (n . l)
                                    // so we basically need to multiple it all together
                                    // notice the scaling by 128, I want to avoid floating point calculations, not because they 
                                    // are slower, but the conversion to and from cost cycles

                                    // note that I use the direction of the light here rather than a the vector to the light
                                    // thus we are taking orientation into account which is similar to the spotlight model

                                    // vertex 0
                                    dp = VECTOR4D_Dot(&obj->vlist_trans[ vindex_0].n, &lights[curr_light].tdir);

                                    // only add light if dp > 0
                                    if (dp > 0)
                                    { 
                                        atten =  (lights[curr_light].kc + lights[curr_light].kl*dist + lights[curr_light].kq*dist*dist);    

                                        i = 128*dp / ( atten ); 

                                        r_sum0 += (lights[curr_light].c_diffuse.r * r_base * i) / (256*128);
                                        g_sum0 += (lights[curr_light].c_diffuse.g * g_base * i) / (256*128);
                                        b_sum0 += (lights[curr_light].c_diffuse.b * b_base * i) / (256*128);
                                    } // end if

                                    // vertex 1
                                    dp = VECTOR4D_Dot(&obj->vlist_trans[ vindex_1].n, &lights[curr_light].tdir);

                                    // only add light if dp > 0
                                    if (dp > 0)
                                    { 
                                        atten =  (lights[curr_light].kc + lights[curr_light].kl*dist + lights[curr_light].kq*dist*dist);    

                                        i = 128*dp / ( atten ); 

                                        r_sum1 += (lights[curr_light].c_diffuse.r * r_base * i) / (256*128);
                                        g_sum1 += (lights[curr_light].c_diffuse.g * g_base * i) / (256*128);
                                        b_sum1 += (lights[curr_light].c_diffuse.b * b_base * i) / (256*128);
                                    } // end i

                                    // vertex 2
                                    dp = VECTOR4D_Dot(&obj->vlist_trans[ vindex_2].n, &lights[curr_light].tdir);

                                    // only add light if dp > 0
                                    if (dp > 0)
                                    { 
                                        atten =  (lights[curr_light].kc + lights[curr_light].kl*dist + lights[curr_light].kq*dist*dist);    

                                        i = 128*dp / ( atten ); 

                                        r_sum2 += (lights[curr_light].c_diffuse.r * r_base * i) / (256*128);
                                        g_sum2 += (lights[curr_light].c_diffuse.g * g_base * i) / (256*128);
                                        b_sum2 += (lights[curr_light].c_diffuse.b * b_base * i) / (256*128);
                                    } // end i

                                    //Write_Error("\nexiting spotlight1, sums rgb0[%d, %d, %d], rgb1[%d,%d,%d], rgb2[%d,%d,%d]", r_sum0, g_sum0, b_sum0,  r_sum1, g_sum1, b_sum1,  r_sum2, g_sum2, b_sum2);

                                } // end if spotlight1
                                else
                                    if (lights[curr_light].attr & LIGHTV2_ATTR_SPOTLIGHT2) // simple version //////////////////////////
                                    {
                                        // perform spot light computations
                                        // light model for spot light simple version is once again:
                                        //         	     I0spotlight * Clspotlight * MAX( (l . s), 0)^pf                     
                                        // I(d)spotlight = __________________________________________      
                                        //               		 kc + kl*d + kq*d2        
                                        // Where d = |p - s|, and pf = power factor

                                        // thus it's almost identical to the point, but has the extra term in the numerator
                                        // relating the angle between the light source and the point on the surface

                                        // .. already have normals and length are 1.0

                                        // and for the diffuse model
                                        // Itotald =   Rsdiffuse*Idiffuse * (n . l)
                                        // so we basically need to multiple it all together
                                        // notice the scaling by 128, I want to avoid floating point calculations, not because they 
                                        // are slower, but the conversion to and from cost cycles

                                        //Write_Error("\nEntering spotlight2...");

                                        // tons of redundant math here! lots to optimize later!

                                        // vertex 0
                                        dp = VECTOR4D_Dot(&obj->vlist_trans[ vindex_0].n, &lights[curr_light].tdir);

                                        // only add light if dp > 0
                                        if (dp > 0)
                                        { 
                                            // compute vector from light to surface (different from l which IS the light dir)
                                            VECTOR4D_Build( &lights[curr_light].tpos, &obj->vlist_trans[ vindex_0].v, &s);

                                            // compute length of s (distance to light source) to normalize s for lighting calc
                                            dists = VECTOR4D_Length_Fast2(&s);  

                                            // compute spot light term (s . l)
                                            float dpsl = VECTOR4D_Dot(&s, &lights[curr_light].tdir)/dists;

                                            // proceed only if term is positive
                                            if (dpsl > 0) 
                                            {
                                                // compute attenuation
                                                atten = (lights[curr_light].kc + lights[curr_light].kl*dists + lights[curr_light].kq*dists*dists);    

                                                // for speed reasons, pf exponents that are less that 1.0 are out of the question, and exponents
                                                // must be integral
                                                float dpsl_exp = dpsl;

                                                // exponentiate for positive integral powers
                                                for (int e_index = 1; e_index < (int)lights[curr_light].pf; e_index++)
                                                    dpsl_exp*=dpsl;

                                                // now dpsl_exp holds (dpsl)^pf power which is of course (s . l)^pf 

                                                i = 128*dp * dpsl_exp / ( atten ); 

                                                r_sum0 += (lights[curr_light].c_diffuse.r * r_base * i) / (256*128);
                                                g_sum0 += (lights[curr_light].c_diffuse.g * g_base * i) / (256*128);
                                                b_sum0 += (lights[curr_light].c_diffuse.b * b_base * i) / (256*128);

                                            } // end if

                                        } // end if

                                        // vertex 1
                                        dp = VECTOR4D_Dot(&obj->vlist_trans[ vindex_1].n, &lights[curr_light].tdir);

                                        // only add light if dp > 0
                                        if (dp > 0)
                                        { 
                                            // compute vector from light to surface (different from l which IS the light dir)
                                            VECTOR4D_Build( &lights[curr_light].tpos, &obj->vlist_trans[ vindex_1].v, &s);

                                            // compute length of s (distance to light source) to normalize s for lighting calc
                                            dists = VECTOR4D_Length_Fast2(&s);  

                                            // compute spot light term (s . l)
                                            float dpsl = VECTOR4D_Dot(&s, &lights[curr_light].tdir)/dists;

                                            // proceed only if term is positive
                                            if (dpsl > 0) 
                                            {
                                                // compute attenuation
                                                atten = (lights[curr_light].kc + lights[curr_light].kl*dists + lights[curr_light].kq*dists*dists);    

                                                // for speed reasons, pf exponents that are less that 1.0 are out of the question, and exponents
                                                // must be integral
                                                float dpsl_exp = dpsl;

                                                // exponentiate for positive integral powers
                                                for (int e_index = 1; e_index < (int)lights[curr_light].pf; e_index++)
                                                    dpsl_exp*=dpsl;

                                                // now dpsl_exp holds (dpsl)^pf power which is of course (s . l)^pf 

                                                i = 128*dp * dpsl_exp / ( atten ); 

                                                r_sum1 += (lights[curr_light].c_diffuse.r * r_base * i) / (256*128);
                                                g_sum1 += (lights[curr_light].c_diffuse.g * g_base * i) / (256*128);
                                                b_sum1 += (lights[curr_light].c_diffuse.b * b_base * i) / (256*128);

                                            } // end if

                                        } // end if

                                        // vertex 2
                                        dp = VECTOR4D_Dot(&obj->vlist_trans[ vindex_2].n, &lights[curr_light].tdir);

                                        // only add light if dp > 0
                                        if (dp > 0)
                                        { 
                                            // compute vector from light to surface (different from l which IS the light dir)
                                            VECTOR4D_Build( &lights[curr_light].tpos, &obj->vlist_trans[ vindex_2].v, &s);

                                            // compute length of s (distance to light source) to normalize s for lighting calc
                                            dists = VECTOR4D_Length_Fast2(&s);  

                                            // compute spot light term (s . l)
                                            float dpsl = VECTOR4D_Dot(&s, &lights[curr_light].tdir)/dists;

                                            // proceed only if term is positive
                                            if (dpsl > 0) 
                                            {
                                                // compute attenuation
                                                atten = (lights[curr_light].kc + lights[curr_light].kl*dists + lights[curr_light].kq*dists*dists);    

                                                // for speed reasons, pf exponents that are less that 1.0 are out of the question, and exponents
                                                // must be integral
                                                float dpsl_exp = dpsl;

                                                // exponentiate for positive integral powers
                                                for (int e_index = 1; e_index < (int)lights[curr_light].pf; e_index++)
                                                    dpsl_exp*=dpsl;

                                                // now dpsl_exp holds (dpsl)^pf power which is of course (s . l)^pf 

                                                i = 128*dp * dpsl_exp / ( atten ); 

                                                r_sum2 += (lights[curr_light].c_diffuse.r * r_base * i) / (256*128);
                                                g_sum2 += (lights[curr_light].c_diffuse.g * g_base * i) / (256*128);
                                                b_sum2 += (lights[curr_light].c_diffuse.b * b_base * i) / (256*128);
                                            } // end if

                                        } // end if

                                        //Write_Error("\nexiting spotlight2, sums rgb0[%d, %d, %d], rgb1[%d,%d,%d], rgb2[%d,%d,%d]", r_sum0, g_sum0, b_sum0,   r_sum1, g_sum1, b_sum1,  r_sum2, g_sum2, b_sum2);

                                    } // end if spot light

                } // end for light

                // make sure colors aren't out of range
                if (r_sum0  > 255) r_sum0 = 255;
                if (g_sum0  > 255) g_sum0 = 255;
                if (b_sum0  > 255) b_sum0 = 255;

                if (r_sum1  > 255) r_sum1 = 255;
                if (g_sum1  > 255) g_sum1 = 255;
                if (b_sum1  > 255) b_sum1 = 255;

                if (r_sum2  > 255) r_sum2 = 255;
                if (g_sum2  > 255) g_sum2 = 255;
                if (b_sum2  > 255) b_sum2 = 255;

                //Write_Error("\nwriting color for poly %d", poly);

                //Write_Error("\n******** final sums rgb0[%d, %d, %d], rgb1[%d,%d,%d], rgb2[%d,%d,%d]", r_sum0, g_sum0, b_sum0,  r_sum1, g_sum1, b_sum1, r_sum2, g_sum2, b_sum2);

                // write the colors
                curr_poly->lit_color[0] = RGB16Bit(r_sum0, g_sum0, b_sum0);
                curr_poly->lit_color[1] = RGB16Bit(r_sum1, g_sum1, b_sum1);
                curr_poly->lit_color[2] = RGB16Bit(r_sum2, g_sum2, b_sum2);

            } // end if
            else // assume POLY4DV2_ATTR_SHADE_MODE_CONSTANT
            {
                // emmisive shading only, do nothing
                // ...
                curr_poly->lit_color[0] = curr_poly->color;

                //Write_Error("\nentering constant shader, and exiting...");

            } // end if

    } // end for poly

    // return success
    return(1);

} // end Light_OBJECT4DV2_World2_16

///////////////////////////////////////////////////////////////////////////////

int Light_OBJECT4DV2_World2(OBJECT4DV2_PTR obj,  // object to process
                            CAM4DV1_PTR cam,     // camera position
                            LIGHTV2_PTR lights,  // light list (might have more than one)
                            int max_lights)      // maximum lights in list
{
    // {andre work in progress }

    // 8 bit version
    // function lights an object based on the sent lights and camera. the function supports
    // constant/pure shading (emmisive), flat shading with ambient, infinite, point lights, and spot lights
    // note that this lighting function is rather brute force and simply follows the math, however
    // there are some clever integer operations that are used in scale 256 rather than going to floating
    // point, but why? floating point and ints are the same speed, HOWEVER, the conversion to and from floating
    // point can be cycle intensive, so if you can keep your calcs in ints then you can gain some speed
    // also note, type 1 spot lights are simply point lights with direction, the "cone" is more of a function
    // of the falloff due to attenuation, but they still look like spot lights
    // type 2 spot lights are implemented with the intensity having a dot product relationship with the
    // angle from the surface point to the light direction just like in the optimized model, but the pf term
    // that is used for a concentration control must be 1,2,3,.... integral and non-fractional

    // the function works in 8-bit color space, and uses the rgblookup[] to look colors up from RGB values
    // basically, the function converts the 8-bit color index into an RGB value performs the lighting
    // operations and then back into an 8-bit color index with the table, so we loose a little bit during the incoming and 
    // outgoing transformations, however, the next step for optimization will be to make a purely monochromatic
    // 8-bit system that assumes ALL lights are white, but this function works with color for now

    unsigned int r_base, g_base,   b_base,  // base color being lit
        r_sum,  g_sum,    b_sum,   // sum of lighting process over all lights
        r_sum0,  g_sum0,  b_sum0,
        r_sum1,  g_sum1,  b_sum1,
        r_sum2,  g_sum2,  b_sum2,
        ri,gi,bi,
        shaded_color;            // final color

    float dp,     // dot product 
        dist,   // distance from light to surface
        dists, 
        i,      // general intensities
        nl,     // length of normal
        atten;  // attenuation computations

    VECTOR4D u, v, n, l, d, s; // used for cross product and light vector calculations

    //Write_Error("\nEntering lighting function");


    // test if the object is culled
    if (!(obj->state & OBJECT4DV2_STATE_ACTIVE) ||
        (obj->state & OBJECT4DV2_STATE_CULLED) ||
        !(obj->state & OBJECT4DV2_STATE_VISIBLE))
        return(0); 

    // for each valid poly, light it...
    for (int poly=0; poly < obj->num_polys; poly++)
    {
        // acquire polygon
        POLY4DV2_PTR curr_poly = &obj->plist[poly];

        // light this polygon if and only if it's not clipped, not culled,
        // active, and visible
        if (!(curr_poly->state & POLY4DV2_STATE_ACTIVE) ||
            (curr_poly->state & POLY4DV2_STATE_CLIPPED ) ||
            (curr_poly->state & POLY4DV2_STATE_BACKFACE) )
            continue; // move onto next poly

        // set state of polygon to lit, so we don't light again in renderlist
        // lighting system if it happens to get called
        SET_BIT(curr_poly->state, POLY4DV2_STATE_LIT);

        // extract vertex indices into master list, rember the polygons are 
        // NOT self contained, but based on the vertex list stored in the object
        // itself
        int vindex_0 = curr_poly->vert[0];
        int vindex_1 = curr_poly->vert[1];
        int vindex_2 = curr_poly->vert[2];

        // we will use the transformed polygon vertex list since the backface removal
        // only makes sense at the world coord stage further of the pipeline 

        //Write_Error("\npoly %d",poly);

        // we will use the transformed polygon vertex list since the backface removal
        // only makes sense at the world coord stage further of the pipeline 

        // test the lighting mode of the polygon (use flat for flat, gouraud))
        if (curr_poly->attr & POLY4DV2_ATTR_SHADE_MODE_FLAT)
        {
            //Write_Error("\nEntering Flat Shader");

            // step 1: extract the base color out in RGB mode (it's already in 8 bits per channel)
            r_base = palette[curr_poly->color].peRed;
            g_base = palette[curr_poly->color].peGreen;
            b_base = palette[curr_poly->color].peBlue;

            //Write_Error("\nBase color=%d,%d,%d", r_base, g_base, b_base);

            // initialize color sum
            r_sum  = 0;
            g_sum  = 0;
            b_sum  = 0;

            //Write_Error("\nsum color=%d,%d,%d", r_sum, g_sum, b_sum);

            // new optimization:
            // when there are multiple lights in the system we will end up performing numerous
            // redundant calculations to minimize this my strategy is to set key variables to 
            // to MAX values on each loop, then during the lighting calcs to test the vars for
            // the max value, if they are the max value then the first light that needs the math
            // will do it, and then save the information into the variable (causing it to change state
            // from an invalid number) then any other lights that need the math can use the previously
            // computed value

            // set surface normal.z to FLT_MAX to flag it as non-computed
            n.z = FLT_MAX;

            // loop thru lights
            for (int curr_light = 0; curr_light < max_lights; curr_light++)
            {
                // is this light active
                if (lights[curr_light].state==LIGHTV2_STATE_OFF)
                    continue;

                //Write_Error("\nprocessing light %d",curr_light);

                // what kind of light are we dealing with
                if (lights[curr_light].attr & LIGHTV2_ATTR_AMBIENT)
                {
                    //Write_Error("\nEntering ambient light...");

                    // simply multiply each channel against the color of the 
                    // polygon then divide by 256 to scale back to 0..255
                    // use a shift in real life!!! >> 8
                    r_sum+= ((lights[curr_light].c_ambient.r * r_base) / 256);
                    g_sum+= ((lights[curr_light].c_ambient.g * g_base) / 256);
                    b_sum+= ((lights[curr_light].c_ambient.b * b_base) / 256);

                    //Write_Error("\nambient sum=%d,%d,%d", r_sum, g_sum, b_sum);

                    // there better only be one ambient light!

                } // end if
                else
                    if (lights[curr_light].attr & LIGHTV2_ATTR_INFINITE) ///////////////////////////////////////////
                    {
                        //Write_Error("\nEntering infinite light...");

                        // infinite lighting, we need the surface normal, and the direction
                        // of the light source

                        // test if we already computed poly normal in previous calculation
                        if (n.z==FLT_MAX)       
                        {
                            // we need to compute the normal of this polygon face, and recall
                            // that the vertices are in cw order, u=p0->p1, v=p0->p2, n=uxv

                            // build u, v
                            VECTOR4D_Build(&obj->vlist_trans[ vindex_0].v, &obj->vlist_trans[ vindex_1].v, &u);
                            VECTOR4D_Build(&obj->vlist_trans[ vindex_0].v, &obj->vlist_trans[ vindex_2].v, &v);

                            // compute cross product
                            VECTOR4D_Cross(&u, &v, &n);
                        } // end if

                        // at this point, we are almost ready, but we have to normalize the normal vector!
                        // this is a key optimization we can make later, we can pre-compute the length of all polygon
                        // normals, so this step can be optimized
                        // compute length of normal
                        //nl = VECTOR4D_Length_Fast2(&n);
                        nl = curr_poly->nlength;  

                        // ok, recalling the lighting model for infinite lights
                        // I(d)dir = I0dir * Cldir
                        // and for the diffuse model
                        // Itotald =   Rsdiffuse*Idiffuse * (n . l)
                        // so we basically need to multiple it all together
                        // notice the scaling by 128, I want to avoid floating point calculations, not because they 
                        // are slower, but the conversion to and from cost cycles

                        dp = VECTOR4D_Dot(&n, &lights[curr_light].tdir);

                        // only add light if dp > 0
                        if (dp > 0)
                        { 
                            i = 128*dp/nl; 
                            r_sum+= (lights[curr_light].c_diffuse.r * r_base * i) / (256*128);
                            g_sum+= (lights[curr_light].c_diffuse.g * g_base * i) / (256*128);
                            b_sum+= (lights[curr_light].c_diffuse.b * b_base * i) / (256*128);
                        } // end if

                        //Write_Error("\ninfinite sum=%d,%d,%d", r_sum, g_sum, b_sum);

                    } // end if infinite light
                    else
                        if (lights[curr_light].attr & LIGHTV2_ATTR_POINT) ///////////////////////////////////////
                        {
                            //Write_Error("\nEntering point light...");

                            // perform point light computations
                            // light model for point light is once again:
                            //              I0point * Clpoint
                            //  I(d)point = ___________________
                            //              kc +  kl*d + kq*d2              
                            //
                            //  Where d = |p - s|
                            // thus it's almost identical to the infinite light, but attenuates as a function
                            // of distance from the point source to the surface point being lit

                            // test if we already computed poly normal in previous calculation
                            if (n.z==FLT_MAX)       
                            {
                                // we need to compute the normal of this polygon face, and recall
                                // that the vertices are in cw order, u=p0->p1, v=p0->p2, n=uxv

                                // build u, v
                                VECTOR4D_Build(&obj->vlist_trans[ vindex_0].v, &obj->vlist_trans[ vindex_1].v, &u);
                                VECTOR4D_Build(&obj->vlist_trans[ vindex_0].v, &obj->vlist_trans[ vindex_2].v, &v);

                                // compute cross product
                                VECTOR4D_Cross(&u, &v, &n);
                            } // end if

                            // at this point, we are almost ready, but we have to normalize the normal vector!
                            // this is a key optimization we can make later, we can pre-compute the length of all polygon
                            // normals, so this step can be optimized
                            // compute length of normal
                            //nl = VECTOR4D_Length_Fast2(&n);
                            nl = curr_poly->nlength;  

                            // compute vector from surface to light
                            VECTOR4D_Build(&obj->vlist_trans[ vindex_0].v, &lights[curr_light].tpos, &l);

                            // compute distance and attenuation
                            dist = VECTOR4D_Length_Fast2(&l);  

                            // and for the diffuse model
                            // Itotald =   Rsdiffuse*Idiffuse * (n . l)
                            // so we basically need to multiple it all together
                            // notice the scaling by 128, I want to avoid floating point calculations, not because they 
                            // are slower, but the conversion to and from cost cycles
                            dp = VECTOR4D_Dot(&n, &l);

                            // only add light if dp > 0
                            if (dp > 0)
                            { 
                                atten =  (lights[curr_light].kc + lights[curr_light].kl*dist + lights[curr_light].kq*dist*dist);    

                                i = 128*dp / (nl * dist * atten ); 

                                r_sum += (lights[curr_light].c_diffuse.r * r_base * i) / (256*128);
                                g_sum += (lights[curr_light].c_diffuse.g * g_base * i) / (256*128);
                                b_sum += (lights[curr_light].c_diffuse.b * b_base * i) / (256*128);
                            } // end if


                            //Write_Error("\npoint sum=%d,%d,%d",r_sum,g_sum,b_sum);

                        } // end if point
                        else
                            if (lights[curr_light].attr & LIGHTV2_ATTR_SPOTLIGHT1) ////////////////////////////////////
                            {
                                //Write_Error("\nentering spot light1...");

                                // perform spotlight/point computations simplified model that uses
                                // point light WITH a direction to simulate a spotlight
                                // light model for point light is once again:
                                //              I0point * Clpoint
                                //  I(d)point = ___________________
                                //              kc +  kl*d + kq*d2              
                                //
                                //  Where d = |p - s|
                                // thus it's almost identical to the infinite light, but attenuates as a function
                                // of distance from the point source to the surface point being lit

                                // test if we already computed poly normal in previous calculation
                                if (n.z==FLT_MAX)       
                                {
                                    // we need to compute the normal of this polygon face, and recall
                                    // that the vertices are in cw order, u=p0->p1, v=p0->p2, n=uxv

                                    // build u, v
                                    VECTOR4D_Build(&obj->vlist_trans[ vindex_0].v, &obj->vlist_trans[ vindex_1].v, &u);
                                    VECTOR4D_Build(&obj->vlist_trans[ vindex_0].v, &obj->vlist_trans[ vindex_2].v, &v);

                                    // compute cross product
                                    VECTOR4D_Cross(&u, &v, &n);
                                } // end if

                                // at this point, we are almost ready, but we have to normalize the normal vector!
                                // this is a key optimization we can make later, we can pre-compute the length of all polygon
                                // normals, so this step can be optimized
                                // compute length of normal
                                //nl = VECTOR4D_Length_Fast2(&n);
                                nl = curr_poly->nlength;  

                                // compute vector from surface to light
                                VECTOR4D_Build(&obj->vlist_trans[ vindex_0].v, &lights[curr_light].tpos, &l);

                                // compute distance and attenuation
                                dist = VECTOR4D_Length_Fast2(&l);  

                                // and for the diffuse model
                                // Itotald =   Rsdiffuse*Idiffuse * (n . l)
                                // so we basically need to multiple it all together
                                // notice the scaling by 128, I want to avoid floating point calculations, not because they 
                                // are slower, but the conversion to and from cost cycles

                                // note that I use the direction of the light here rather than a the vector to the light
                                // thus we are taking orientation into account which is similar to the spotlight model
                                dp = VECTOR4D_Dot(&n, &lights[curr_light].tdir);

                                // only add light if dp > 0
                                if (dp > 0)
                                { 
                                    atten =  (lights[curr_light].kc + lights[curr_light].kl*dist + lights[curr_light].kq*dist*dist);    

                                    i = 128*dp / (nl * atten ); 

                                    r_sum += (lights[curr_light].c_diffuse.r * r_base * i) / (256*128);
                                    g_sum += (lights[curr_light].c_diffuse.g * g_base * i) / (256*128);
                                    b_sum += (lights[curr_light].c_diffuse.b * b_base * i) / (256*128);
                                } // end if

                                //Write_Error("\nspotlight sum=%d,%d,%d",r_sum, g_sum, b_sum);

                            } // end if spotlight1
                            else
                                if (lights[curr_light].attr & LIGHTV2_ATTR_SPOTLIGHT2) // simple version ////////////////////
                                {
                                    //Write_Error("\nEntering spotlight2 ...");

                                    // perform spot light computations
                                    // light model for spot light simple version is once again:
                                    //         	     I0spotlight * Clspotlight * MAX( (l . s), 0)^pf                     
                                    // I(d)spotlight = __________________________________________      
                                    //               		 kc + kl*d + kq*d2        
                                    // Where d = |p - s|, and pf = power factor

                                    // thus it's almost identical to the point, but has the extra term in the numerator
                                    // relating the angle between the light source and the point on the surface

                                    // test if we already computed poly normal in previous calculation
                                    if (n.z==FLT_MAX)       
                                    {
                                        // we need to compute the normal of this polygon face, and recall
                                        // that the vertices are in cw order, u=p0->p1, v=p0->p2, n=uxv

                                        // build u, v
                                        VECTOR4D_Build(&obj->vlist_trans[ vindex_0].v, &obj->vlist_trans[ vindex_1].v, &u);
                                        VECTOR4D_Build(&obj->vlist_trans[ vindex_0].v, &obj->vlist_trans[ vindex_2].v, &v);

                                        // compute cross product
                                        VECTOR4D_Cross(&u, &v, &n);
                                    } // end if

                                    // at this point, we are almost ready, but we have to normalize the normal vector!
                                    // this is a key optimization we can make later, we can pre-compute the length of all polygon
                                    // normals, so this step can be optimized
                                    // compute length of normal
                                    //nl = VECTOR4D_Length_Fast2(&n);
                                    nl = curr_poly->nlength;  

                                    // and for the diffuse model
                                    // Itotald =   Rsdiffuse*Idiffuse * (n . l)
                                    // so we basically need to multiple it all together
                                    // notice the scaling by 128, I want to avoid floating point calculations, not because they 
                                    // are slower, but the conversion to and from cost cycles
                                    dp = VECTOR4D_Dot(&n, &lights[curr_light].tdir);

                                    // only add light if dp > 0
                                    if (dp > 0)
                                    { 
                                        // compute vector from light to surface (different from l which IS the light dir)
                                        VECTOR4D_Build( &lights[curr_light].tpos, &obj->vlist_trans[ vindex_0].v, &s);

                                        // compute length of s (distance to light source) to normalize s for lighting calc
                                        dists = VECTOR4D_Length_Fast2(&s);  

                                        // compute spot light term (s . l)
                                        float dpsl = VECTOR4D_Dot(&s, &lights[curr_light].tdir)/dists;

                                        // proceed only if term is positive
                                        if (dpsl > 0) 
                                        {
                                            // compute attenuation
                                            atten = (lights[curr_light].kc + lights[curr_light].kl*dists + lights[curr_light].kq*dists*dists);    

                                            // for speed reasons, pf exponents that are less that 1.0 are out of the question, and exponents
                                            // must be integral
                                            float dpsl_exp = dpsl;

                                            // exponentiate for positive integral powers
                                            for (int e_index = 1; e_index < (int)lights[curr_light].pf; e_index++)
                                                dpsl_exp*=dpsl;

                                            // now dpsl_exp holds (dpsl)^pf power which is of course (s . l)^pf 

                                            i = 128*dp * dpsl_exp / (nl * atten ); 

                                            r_sum += (lights[curr_light].c_diffuse.r * r_base * i) / (256*128);
                                            g_sum += (lights[curr_light].c_diffuse.g * g_base * i) / (256*128);
                                            b_sum += (lights[curr_light].c_diffuse.b * b_base * i) / (256*128);

                                        } // end if

                                    } // end if

                                    //Write_Error("\nSpotlight sum=%d,%d,%d",r_sum, g_sum, b_sum);

                                } // end if spot light

            } // end for light

            // make sure colors aren't out of range
            if (r_sum  > 255) r_sum = 255;
            if (g_sum  > 255) g_sum = 255;
            if (b_sum  > 255) b_sum = 255;

            //Write_Error("\nWriting final values to polygon %d = %d,%d,%d", poly, r_sum, g_sum, b_sum);

            // write the color over current color
            curr_poly->lit_color[0] = rgblookup[RGB16Bit565(r_sum, g_sum, b_sum)];

        } // end if
        else
            if (curr_poly->attr & POLY4DV2_ATTR_SHADE_MODE_GOURAUD) /////////////////////////////////
            {
                // gouraud shade, unfortunetly at this point in the pipeline, we have lost the original
                // mesh, and only have triangles, thus, many triangles will share the same vertices and
                // they will get lit 2x since we don't have any way to tell this, alas, performing lighting
                // at the object level is a better idea when gouraud shading is performed since the 
                // commonality of vertices is still intact, in any case, lighting here is similar to polygon
                // flat shaded, but we do it 3 times, once for each vertex, additionally there are lots
                // of opportunities for optimization, but I am going to lay off them for now, so the code
                // is intelligible, later we will optimize

                //Write_Error("\nEntering gouraud shader...");

                // step 1: extract the base color out in RGB mode (it's already in 8 bits per channel)
                r_base = palette[curr_poly->color].peRed;
                g_base = palette[curr_poly->color].peGreen;
                b_base = palette[curr_poly->color].peBlue;

                //Write_Error("\nBase color=%d, %d, %d", r_base, g_base, b_base);

                // initialize color sum(s) for vertices
                r_sum0  = 0;
                g_sum0  = 0;
                b_sum0  = 0;

                r_sum1  = 0;
                g_sum1  = 0;
                b_sum1  = 0;

                r_sum2  = 0;
                g_sum2  = 0;
                b_sum2  = 0;

                //Write_Error("\nColor sums rgb0[%d, %d, %d], rgb1[%d,%d,%d], rgb2[%d,%d,%d]", r_sum0, g_sum0, b_sum0,   r_sum1, g_sum1, b_sum1, r_sum2, g_sum2, b_sum2);

                // new optimization:
                // when there are multiple lights in the system we will end up performing numerous
                // redundant calculations to minimize this my strategy is to set key variables to 
                // to MAX values on each loop, then during the lighting calcs to test the vars for
                // the max value, if they are the max value then the first light that needs the math
                // will do it, and then save the information into the variable (causing it to change state
                // from an invalid number) then any other lights that need the math can use the previously
                // computed value

                // loop thru lights
                for (int curr_light = 0; curr_light < max_lights; curr_light++)
                {
                    // is this light active
                    if (lights[curr_light].state==LIGHTV2_STATE_OFF)
                        continue;

                    //Write_Error("\nprocessing light %d", curr_light);

                    // what kind of light are we dealing with
                    if (lights[curr_light].attr & LIGHTV2_ATTR_AMBIENT) ///////////////////////////////
                    {
                        //Write_Error("\nEntering ambient light....");

                        // simply multiply each channel against the color of the 
                        // polygon then divide by 256 to scale back to 0..255
                        // use a shift in real life!!! >> 8
                        ri = ((lights[curr_light].c_ambient.r * r_base) / 256);
                        gi = ((lights[curr_light].c_ambient.g * g_base) / 256);
                        bi = ((lights[curr_light].c_ambient.b * b_base) / 256);

                        // ambient light has the same affect on each vertex
                        r_sum0+=ri;
                        g_sum0+=gi;
                        b_sum0+=bi;

                        r_sum1+=ri;
                        g_sum1+=gi;
                        b_sum1+=bi;

                        r_sum2+=ri;
                        g_sum2+=gi;
                        b_sum2+=bi;

                        // there better only be one ambient light!
                        //Write_Error("\nexiting ambient ,sums rgb0[%d, %d, %d], rgb1[%d,%d,%d], rgb2[%d,%d,%d]", r_sum0, g_sum0, b_sum0,  r_sum1, g_sum1, b_sum1,  r_sum2, g_sum2, b_sum2);

                    } // end if
                    else
                        if (lights[curr_light].attr & LIGHTV2_ATTR_INFINITE) /////////////////////////////////
                        {
                            //Write_Error("\nentering infinite light...");

                            // infinite lighting, we need the surface normal, and the direction
                            // of the light source

                            // no longer need to compute normal or length, we already have the vertex normal
                            // and it's length is 1.0  
                            // ....

                            // ok, recalling the lighting model for infinite lights
                            // I(d)dir = I0dir * Cldir
                            // and for the diffuse model
                            // Itotald =   Rsdiffuse*Idiffuse * (n . l)
                            // so we basically need to multiple it all together
                            // notice the scaling by 128, I want to avoid floating point calculations, not because they 
                            // are slower, but the conversion to and from cost cycles

                            // need to perform lighting for each vertex (lots of redundant math, optimize later!)

                            //Write_Error("\nv0=[%f, %f, %f]=%f, v1=[%f, %f, %f]=%f, v2=[%f, %f, %f]=%f",
                            // curr_poly->tvlist[0].n.x, curr_poly->tvlist[0].n.y,curr_poly->tvlist[0].n.z, VECTOR4D_Length(&curr_poly->tvlist[0].n),
                            // curr_poly->tvlist[1].n.x, curr_poly->tvlist[1].n.y,curr_poly->tvlist[1].n.z, VECTOR4D_Length(&curr_poly->tvlist[1].n),
                            // curr_poly->tvlist[2].n.x, curr_poly->tvlist[2].n.y,curr_poly->tvlist[2].n.z, VECTOR4D_Length(&curr_poly->tvlist[2].n) );

                            // vertex 0
                            dp = VECTOR4D_Dot(&obj->vlist_trans[ vindex_0].n, &lights[curr_light].tdir); 

                            // only add light if dp > 0
                            if (dp > 0)
                            { 
                                i = 128*dp; 
                                r_sum0+= (lights[curr_light].c_diffuse.r * r_base * i) / (256*128);
                                g_sum0+= (lights[curr_light].c_diffuse.g * g_base * i) / (256*128);
                                b_sum0+= (lights[curr_light].c_diffuse.b * b_base * i) / (256*128);
                            } // end if

                            // vertex 1
                            dp = VECTOR4D_Dot(&obj->vlist_trans[ vindex_1].n, &lights[curr_light].tdir);

                            // only add light if dp > 0
                            if (dp > 0)
                            { 
                                i = 128*dp; 
                                r_sum1+= (lights[curr_light].c_diffuse.r * r_base * i) / (256*128);
                                g_sum1+= (lights[curr_light].c_diffuse.g * g_base * i) / (256*128);
                                b_sum1+= (lights[curr_light].c_diffuse.b * b_base * i) / (256*128);
                            } // end if

                            // vertex 2
                            dp = VECTOR4D_Dot(&obj->vlist_trans[ vindex_2].n, &lights[curr_light].tdir);

                            // only add light if dp > 0
                            if (dp > 0)
                            { 
                                i = 128*dp; 
                                r_sum2+= (lights[curr_light].c_diffuse.r * r_base * i) / (256*128);
                                g_sum2+= (lights[curr_light].c_diffuse.g * g_base * i) / (256*128);
                                b_sum2+= (lights[curr_light].c_diffuse.b * b_base * i) / (256*128);
                            } // end if

                            //Write_Error("\nexiting infinite, color sums rgb0[%d, %d, %d], rgb1[%d,%d,%d], rgb2[%d,%d,%d]", r_sum0, g_sum0, b_sum0,  r_sum1, g_sum1, b_sum1,  r_sum2, g_sum2, b_sum2);

                        } // end if infinite light
                        else
                            if (lights[curr_light].attr & LIGHTV2_ATTR_POINT) //////////////////////////////////////
                            {
                                // perform point light computations
                                // light model for point light is once again:
                                //              I0point * Clpoint
                                //  I(d)point = ___________________
                                //              kc +  kl*d + kq*d2              
                                //
                                //  Where d = |p - s|
                                // thus it's almost identical to the infinite light, but attenuates as a function
                                // of distance from the point source to the surface point being lit

                                // .. normal already in vertex

                                //Write_Error("\nEntering point light....");

                                // compute vector from surface to light
                                VECTOR4D_Build(&obj->vlist_trans[ vindex_0].v, &lights[curr_light].tpos, &l);

                                // compute distance and attenuation
                                dist = VECTOR4D_Length_Fast2(&l);  

                                // and for the diffuse model
                                // Itotald =   Rsdiffuse*Idiffuse * (n . l)
                                // so we basically need to multiple it all together
                                // notice the scaling by 128, I want to avoid floating point calculations, not because they 
                                // are slower, but the conversion to and from cost cycles

                                // perform the calculation for all 3 vertices

                                // vertex 0
                                dp = VECTOR4D_Dot(&obj->vlist_trans[ vindex_0].n, &l);

                                // only add light if dp > 0
                                if (dp > 0)
                                { 
                                    atten =  (lights[curr_light].kc + lights[curr_light].kl*dist + lights[curr_light].kq*dist*dist);    

                                    i = 128*dp / (dist * atten ); 

                                    r_sum0 += (lights[curr_light].c_diffuse.r * r_base * i) / (256*128);
                                    g_sum0 += (lights[curr_light].c_diffuse.g * g_base * i) / (256*128);
                                    b_sum0 += (lights[curr_light].c_diffuse.b * b_base * i) / (256*128);
                                } // end if


                                // vertex 1
                                dp = VECTOR4D_Dot(&obj->vlist_trans[ vindex_1].n, &l);

                                // only add light if dp > 0
                                if (dp > 0)
                                { 
                                    atten =  (lights[curr_light].kc + lights[curr_light].kl*dist + lights[curr_light].kq*dist*dist);    

                                    i = 128*dp / (dist * atten ); 

                                    r_sum1 += (lights[curr_light].c_diffuse.r * r_base * i) / (256*128);
                                    g_sum1 += (lights[curr_light].c_diffuse.g * g_base * i) / (256*128);
                                    b_sum1 += (lights[curr_light].c_diffuse.b * b_base * i) / (256*128);
                                } // end if

                                // vertex 2
                                dp = VECTOR4D_Dot(&obj->vlist_trans[ vindex_2].n, &l);

                                // only add light if dp > 0
                                if (dp > 0)
                                { 
                                    atten =  (lights[curr_light].kc + lights[curr_light].kl*dist + lights[curr_light].kq*dist*dist);    

                                    i = 128*dp / (dist * atten ); 

                                    r_sum2 += (lights[curr_light].c_diffuse.r * r_base * i) / (256*128);
                                    g_sum2 += (lights[curr_light].c_diffuse.g * g_base * i) / (256*128);
                                    b_sum2 += (lights[curr_light].c_diffuse.b * b_base * i) / (256*128);
                                } // end if

                                //Write_Error("\nexiting point light, rgb0[%d, %d, %d], rgb1[%d,%d,%d], rgb2[%d,%d,%d]", r_sum0, g_sum0, b_sum0,  r_sum1, g_sum1, b_sum1, r_sum2, g_sum2, b_sum2);

                            } // end if point
                            else
                                if (lights[curr_light].attr & LIGHTV2_ATTR_SPOTLIGHT1) ///////////////////////////////////////
                                {
                                    // perform spotlight/point computations simplified model that uses
                                    // point light WITH a direction to simulate a spotlight
                                    // light model for point light is once again:
                                    //              I0point * Clpoint
                                    //  I(d)point = ___________________
                                    //              kc +  kl*d + kq*d2              
                                    //
                                    //  Where d = |p - s|
                                    // thus it's almost identical to the infinite light, but attenuates as a function
                                    // of distance from the point source to the surface point being lit

                                    //Write_Error("\nentering spotlight1....");

                                    // .. normal is already computed

                                    // compute vector from surface to light
                                    VECTOR4D_Build(&obj->vlist_trans[ vindex_0].v, &lights[curr_light].tpos, &l);

                                    // compute distance and attenuation
                                    dist = VECTOR4D_Length_Fast2(&l);  

                                    // and for the diffuse model
                                    // Itotald =   Rsdiffuse*Idiffuse * (n . l)
                                    // so we basically need to multiple it all together
                                    // notice the scaling by 128, I want to avoid floating point calculations, not because they 
                                    // are slower, but the conversion to and from cost cycles

                                    // note that I use the direction of the light here rather than a the vector to the light
                                    // thus we are taking orientation into account which is similar to the spotlight model

                                    // vertex 0
                                    dp = VECTOR4D_Dot(&obj->vlist_trans[ vindex_0].n, &lights[curr_light].tdir);

                                    // only add light if dp > 0
                                    if (dp > 0)
                                    { 
                                        atten =  (lights[curr_light].kc + lights[curr_light].kl*dist + lights[curr_light].kq*dist*dist);    

                                        i = 128*dp / ( atten ); 

                                        r_sum0 += (lights[curr_light].c_diffuse.r * r_base * i) / (256*128);
                                        g_sum0 += (lights[curr_light].c_diffuse.g * g_base * i) / (256*128);
                                        b_sum0 += (lights[curr_light].c_diffuse.b * b_base * i) / (256*128);
                                    } // end if

                                    // vertex 1
                                    dp = VECTOR4D_Dot(&obj->vlist_trans[ vindex_1].n, &lights[curr_light].tdir);

                                    // only add light if dp > 0
                                    if (dp > 0)
                                    { 
                                        atten =  (lights[curr_light].kc + lights[curr_light].kl*dist + lights[curr_light].kq*dist*dist);    

                                        i = 128*dp / ( atten ); 

                                        r_sum1 += (lights[curr_light].c_diffuse.r * r_base * i) / (256*128);
                                        g_sum1 += (lights[curr_light].c_diffuse.g * g_base * i) / (256*128);
                                        b_sum1 += (lights[curr_light].c_diffuse.b * b_base * i) / (256*128);
                                    } // end i

                                    // vertex 2
                                    dp = VECTOR4D_Dot(&obj->vlist_trans[ vindex_2].n, &lights[curr_light].tdir);

                                    // only add light if dp > 0
                                    if (dp > 0)
                                    { 
                                        atten =  (lights[curr_light].kc + lights[curr_light].kl*dist + lights[curr_light].kq*dist*dist);    

                                        i = 128*dp / ( atten ); 

                                        r_sum2 += (lights[curr_light].c_diffuse.r * r_base * i) / (256*128);
                                        g_sum2 += (lights[curr_light].c_diffuse.g * g_base * i) / (256*128);
                                        b_sum2 += (lights[curr_light].c_diffuse.b * b_base * i) / (256*128);
                                    } // end i

                                    //Write_Error("\nexiting spotlight1, sums rgb0[%d, %d, %d], rgb1[%d,%d,%d], rgb2[%d,%d,%d]", r_sum0, g_sum0, b_sum0,  r_sum1, g_sum1, b_sum1,  r_sum2, g_sum2, b_sum2);

                                } // end if spotlight1
                                else
                                    if (lights[curr_light].attr & LIGHTV2_ATTR_SPOTLIGHT2) // simple version //////////////////////////
                                    {
                                        // perform spot light computations
                                        // light model for spot light simple version is once again:
                                        //         	     I0spotlight * Clspotlight * MAX( (l . s), 0)^pf                     
                                        // I(d)spotlight = __________________________________________      
                                        //               		 kc + kl*d + kq*d2        
                                        // Where d = |p - s|, and pf = power factor

                                        // thus it's almost identical to the point, but has the extra term in the numerator
                                        // relating the angle between the light source and the point on the surface

                                        // .. already have normals and length are 1.0

                                        // and for the diffuse model
                                        // Itotald =   Rsdiffuse*Idiffuse * (n . l)
                                        // so we basically need to multiple it all together
                                        // notice the scaling by 128, I want to avoid floating point calculations, not because they 
                                        // are slower, but the conversion to and from cost cycles

                                        //Write_Error("\nEntering spotlight2...");

                                        // tons of redundant math here! lots to optimize later!

                                        // vertex 0
                                        dp = VECTOR4D_Dot(&obj->vlist_trans[ vindex_0].n, &lights[curr_light].tdir);

                                        // only add light if dp > 0
                                        if (dp > 0)
                                        { 
                                            // compute vector from light to surface (different from l which IS the light dir)
                                            VECTOR4D_Build( &lights[curr_light].tpos, &obj->vlist_trans[ vindex_0].v, &s);

                                            // compute length of s (distance to light source) to normalize s for lighting calc
                                            dists = VECTOR4D_Length_Fast2(&s);  

                                            // compute spot light term (s . l)
                                            float dpsl = VECTOR4D_Dot(&s, &lights[curr_light].tdir)/dists;

                                            // proceed only if term is positive
                                            if (dpsl > 0) 
                                            {
                                                // compute attenuation
                                                atten = (lights[curr_light].kc + lights[curr_light].kl*dists + lights[curr_light].kq*dists*dists);    

                                                // for speed reasons, pf exponents that are less that 1.0 are out of the question, and exponents
                                                // must be integral
                                                float dpsl_exp = dpsl;

                                                // exponentiate for positive integral powers
                                                for (int e_index = 1; e_index < (int)lights[curr_light].pf; e_index++)
                                                    dpsl_exp*=dpsl;

                                                // now dpsl_exp holds (dpsl)^pf power which is of course (s . l)^pf 

                                                i = 128*dp * dpsl_exp / ( atten ); 

                                                r_sum0 += (lights[curr_light].c_diffuse.r * r_base * i) / (256*128);
                                                g_sum0 += (lights[curr_light].c_diffuse.g * g_base * i) / (256*128);
                                                b_sum0 += (lights[curr_light].c_diffuse.b * b_base * i) / (256*128);

                                            } // end if

                                        } // end if

                                        // vertex 1
                                        dp = VECTOR4D_Dot(&obj->vlist_trans[ vindex_1].n, &lights[curr_light].tdir);

                                        // only add light if dp > 0
                                        if (dp > 0)
                                        { 
                                            // compute vector from light to surface (different from l which IS the light dir)
                                            VECTOR4D_Build( &lights[curr_light].tpos, &obj->vlist_trans[ vindex_1].v, &s);

                                            // compute length of s (distance to light source) to normalize s for lighting calc
                                            dists = VECTOR4D_Length_Fast2(&s);  

                                            // compute spot light term (s . l)
                                            float dpsl = VECTOR4D_Dot(&s, &lights[curr_light].tdir)/dists;

                                            // proceed only if term is positive
                                            if (dpsl > 0) 
                                            {
                                                // compute attenuation
                                                atten = (lights[curr_light].kc + lights[curr_light].kl*dists + lights[curr_light].kq*dists*dists);    

                                                // for speed reasons, pf exponents that are less that 1.0 are out of the question, and exponents
                                                // must be integral
                                                float dpsl_exp = dpsl;

                                                // exponentiate for positive integral powers
                                                for (int e_index = 1; e_index < (int)lights[curr_light].pf; e_index++)
                                                    dpsl_exp*=dpsl;

                                                // now dpsl_exp holds (dpsl)^pf power which is of course (s . l)^pf 

                                                i = 128*dp * dpsl_exp / ( atten ); 

                                                r_sum1 += (lights[curr_light].c_diffuse.r * r_base * i) / (256*128);
                                                g_sum1 += (lights[curr_light].c_diffuse.g * g_base * i) / (256*128);
                                                b_sum1 += (lights[curr_light].c_diffuse.b * b_base * i) / (256*128);

                                            } // end if

                                        } // end if

                                        // vertex 2
                                        dp = VECTOR4D_Dot(&obj->vlist_trans[ vindex_2].n, &lights[curr_light].tdir);

                                        // only add light if dp > 0
                                        if (dp > 0)
                                        { 
                                            // compute vector from light to surface (different from l which IS the light dir)
                                            VECTOR4D_Build( &lights[curr_light].tpos, &obj->vlist_trans[ vindex_2].v, &s);

                                            // compute length of s (distance to light source) to normalize s for lighting calc
                                            dists = VECTOR4D_Length_Fast2(&s);  

                                            // compute spot light term (s . l)
                                            float dpsl = VECTOR4D_Dot(&s, &lights[curr_light].tdir)/dists;

                                            // proceed only if term is positive
                                            if (dpsl > 0) 
                                            {
                                                // compute attenuation
                                                atten = (lights[curr_light].kc + lights[curr_light].kl*dists + lights[curr_light].kq*dists*dists);    

                                                // for speed reasons, pf exponents that are less that 1.0 are out of the question, and exponents
                                                // must be integral
                                                float dpsl_exp = dpsl;

                                                // exponentiate for positive integral powers
                                                for (int e_index = 1; e_index < (int)lights[curr_light].pf; e_index++)
                                                    dpsl_exp*=dpsl;

                                                // now dpsl_exp holds (dpsl)^pf power which is of course (s . l)^pf 

                                                i = 128*dp * dpsl_exp / ( atten ); 

                                                r_sum2 += (lights[curr_light].c_diffuse.r * r_base * i) / (256*128);
                                                g_sum2 += (lights[curr_light].c_diffuse.g * g_base * i) / (256*128);
                                                b_sum2 += (lights[curr_light].c_diffuse.b * b_base * i) / (256*128);
                                            } // end if

                                        } // end if

                                        //Write_Error("\nexiting spotlight2, sums rgb0[%d, %d, %d], rgb1[%d,%d,%d], rgb2[%d,%d,%d]", r_sum0, g_sum0, b_sum0,   r_sum1, g_sum1, b_sum1,  r_sum2, g_sum2, b_sum2);

                                    } // end if spot light

                } // end for light

                // make sure colors aren't out of range
                if (r_sum0  > 255) r_sum0 = 255;
                if (g_sum0  > 255) g_sum0 = 255;
                if (b_sum0  > 255) b_sum0 = 255;

                if (r_sum1  > 255) r_sum1 = 255;
                if (g_sum1  > 255) g_sum1 = 255;
                if (b_sum1  > 255) b_sum1 = 255;

                if (r_sum2  > 255) r_sum2 = 255;
                if (g_sum2  > 255) g_sum2 = 255;
                if (b_sum2  > 255) b_sum2 = 255;

                //Write_Error("\nwriting color for poly %d", poly);

                //Write_Error("\n******** final sums rgb0[%d, %d, %d], rgb1[%d,%d,%d], rgb2[%d,%d,%d]", r_sum0, g_sum0, b_sum0,  r_sum1, g_sum1, b_sum1, r_sum2, g_sum2, b_sum2);

                // write the colors, leave in 5.6.5 format, so we have more color range
                // since the rasterizer will scale down to 8-bit
                curr_poly->lit_color[0] = RGB16Bit(r_sum0, g_sum0, b_sum0);
                curr_poly->lit_color[1] = RGB16Bit(r_sum1, g_sum1, b_sum1);
                curr_poly->lit_color[2] = RGB16Bit(r_sum2, g_sum2, b_sum2);

            } // end if
            else // assume POLY4DV2_ATTR_SHADE_MODE_CONSTANT
            {
                // emmisive shading only, do nothing
                // ...
                curr_poly->lit_color[0] = curr_poly->color;

                //Write_Error("\nentering constant shader, and exiting...");

            } // end if

    } // end for poly


    // return success
    return(1);

} // end Light_OBJECT4DV2_World2

//////////////////////////////////////////////////////////////////////////////

int Light_RENDERLIST4DV2_World2_16(RENDERLIST4DV2_PTR rend_list,  // list to process
                                   CAM4DV1_PTR cam,     // camera position
                                   LIGHTV2_PTR lights,  // light list (might have more than one)
                                   int max_lights)      // maximum lights in list
{

    // 16-bit version of function
    // function lights the entire rendering list based on the sent lights and camera. the function supports
    // constant/pure shading (emmisive), flat shading with ambient, infinite, point lights, and spot lights
    // note that this lighting function is rather brute force and simply follows the math, however
    // there are some clever integer operations that are used in scale 256 rather than going to floating
    // point, but why? floating point and ints are the same speed, HOWEVER, the conversion to and from floating
    // point can be cycle intensive, so if you can keep your calcs in ints then you can gain some speed
    // also note, type 1 spot lights are simply point lights with direction, the "cone" is more of a function
    // of the falloff due to attenuation, but they still look like spot lights
    // type 2 spot lights are implemented with the intensity having a dot product relationship with the
    // angle from the surface point to the light direction just like in the optimized model, but the pf term
    // that is used for a concentration control must be 1,2,3,.... integral and non-fractional
    // this function now performs emissive, flat, and gouraud lighting, results are stored in the 
    // lit_color[] array of each polygon

    unsigned int r_base, g_base,   b_base,  // base color being lit
        r_sum,  g_sum,    b_sum,   // sum of lighting process over all lights
        r_sum0,  g_sum0,  b_sum0,
        r_sum1,  g_sum1,  b_sum1,
        r_sum2,  g_sum2,  b_sum2,
        ri,gi,bi,
        shaded_color;            // final color

    float dp,     // dot product 
        dist,   // distance from light to surface
        dists, 
        i,      // general intensities
        nl,     // length of normal
        atten;  // attenuation computations

    VECTOR4D u, v, n, l, d, s; // used for cross product and light vector calculations

    //Write_Error("\nEntering lighting function");

    // for each valid poly, light it...
    for (int poly=0; poly < rend_list->num_polys; poly++)
    {
        // acquire polygon
        POLYF4DV2_PTR curr_poly = rend_list->poly_ptrs[poly];

        // light this polygon if and only if it's not clipped, not culled,
        // active, and visible
        if (!(curr_poly->state & POLY4DV2_STATE_ACTIVE) ||
            (curr_poly->state & POLY4DV2_STATE_CLIPPED ) ||
            (curr_poly->state & POLY4DV2_STATE_BACKFACE) ||
            (curr_poly->state & POLY4DV2_STATE_LIT) )
            continue; // move onto next poly

        //Write_Error("\npoly %d",poly);

#ifdef DEBUG_ON
        // track rendering stats
        debug_polys_lit_per_frame++;
#endif


        // set state of polygon to lit
        SET_BIT(curr_poly->state, POLY4DV2_STATE_LIT);

        // we will use the transformed polygon vertex list since the backface removal
        // only makes sense at the world coord stage further of the pipeline 

        // test the lighting mode of the polygon (use flat for flat, gouraud))
        if (curr_poly->attr & POLY4DV2_ATTR_SHADE_MODE_FLAT)
        {
            //Write_Error("\nEntering Flat Shader");

            // step 1: extract the base color out in RGB mode
            // assume 565 format
            _RGB565FROM16BIT(curr_poly->color, &r_base, &g_base, &b_base);

            // scale to 8 bit 
            r_base <<= 3;
            g_base <<= 2;
            b_base <<= 3;

            //Write_Error("\nBase color=%d,%d,%d", r_base, g_base, b_base);

            // initialize color sum
            r_sum  = 0;
            g_sum  = 0;
            b_sum  = 0;

            //Write_Error("\nsum color=%d,%d,%d", r_sum, g_sum, b_sum);

            // new optimization:
            // when there are multiple lights in the system we will end up performing numerous
            // redundant calculations to minimize this my strategy is to set key variables to 
            // to MAX values on each loop, then during the lighting calcs to test the vars for
            // the max value, if they are the max value then the first light that needs the math
            // will do it, and then save the information into the variable (causing it to change state
            // from an invalid number) then any other lights that need the math can use the previously
            // computed value

            // set surface normal.z to FLT_MAX to flag it as non-computed
            n.z = FLT_MAX;

            // loop thru lights
            for (int curr_light = 0; curr_light < max_lights; curr_light++)
            {
                // is this light active
                if (lights[curr_light].state==LIGHTV2_STATE_OFF)
                    continue;

                //Write_Error("\nprocessing light %d",curr_light);

                // what kind of light are we dealing with
                if (lights[curr_light].attr & LIGHTV2_ATTR_AMBIENT)
                {
                    //Write_Error("\nEntering ambient light...");

                    // simply multiply each channel against the color of the 
                    // polygon then divide by 256 to scale back to 0..255
                    // use a shift in real life!!! >> 8
                    r_sum+= ((lights[curr_light].c_ambient.r * r_base) / 256);
                    g_sum+= ((lights[curr_light].c_ambient.g * g_base) / 256);
                    b_sum+= ((lights[curr_light].c_ambient.b * b_base) / 256);

                    //Write_Error("\nambient sum=%d,%d,%d", r_sum, g_sum, b_sum);

                    // there better only be one ambient light!

                } // end if
                else
                    if (lights[curr_light].attr & LIGHTV2_ATTR_INFINITE) ///////////////////////////////////////////
                    {
                        //Write_Error("\nEntering infinite light...");

                        // infinite lighting, we need the surface normal, and the direction
                        // of the light source

                        // test if we already computed poly normal in previous calculation
                        if (n.z==FLT_MAX)       
                        {
                            // we need to compute the normal of this polygon face, and recall
                            // that the vertices are in cw order, u=p0->p1, v=p0->p2, n=uxv

                            // build u, v
                            VECTOR4D_Build(&curr_poly->tvlist[0].v, &curr_poly->tvlist[1].v, &u);
                            VECTOR4D_Build(&curr_poly->tvlist[0].v, &curr_poly->tvlist[2].v, &v);

                            // compute cross product
                            VECTOR4D_Cross(&u, &v, &n);
                        } // end if

                        // at this point, we are almost ready, but we have to normalize the normal vector!
                        // this is a key optimization we can make later, we can pre-compute the length of all polygon
                        // normals, so this step can be optimized
                        // compute length of normal
                        //nl = VECTOR4D_Length_Fast2(&n);
                        nl = curr_poly->nlength;  

                        // ok, recalling the lighting model for infinite lights
                        // I(d)dir = I0dir * Cldir
                        // and for the diffuse model
                        // Itotald =   Rsdiffuse*Idiffuse * (n . l)
                        // so we basically need to multiple it all together
                        // notice the scaling by 128, I want to avoid floating point calculations, not because they 
                        // are slower, but the conversion to and from cost cycles

                        dp = VECTOR4D_Dot(&n, &lights[curr_light].tdir);

                        // only add light if dp > 0
                        if (dp > 0)
                        { 
                            i = 128*dp/nl; 
                            r_sum+= (lights[curr_light].c_diffuse.r * r_base * i) / (256*128);
                            g_sum+= (lights[curr_light].c_diffuse.g * g_base * i) / (256*128);
                            b_sum+= (lights[curr_light].c_diffuse.b * b_base * i) / (256*128);
                        } // end if

                        //Write_Error("\ninfinite sum=%d,%d,%d", r_sum, g_sum, b_sum);

                    } // end if infinite light
                    else
                        if (lights[curr_light].attr & LIGHTV2_ATTR_POINT) ///////////////////////////////////////
                        {
                            //Write_Error("\nEntering point light...");

                            // perform point light computations
                            // light model for point light is once again:
                            //              I0point * Clpoint
                            //  I(d)point = ___________________
                            //              kc +  kl*d + kq*d2              
                            //
                            //  Where d = |p - s|
                            // thus it's almost identical to the infinite light, but attenuates as a function
                            // of distance from the point source to the surface point being lit

                            // test if we already computed poly normal in previous calculation
                            if (n.z==FLT_MAX)       
                            {
                                // we need to compute the normal of this polygon face, and recall
                                // that the vertices are in cw order, u=p0->p1, v=p0->p2, n=uxv

                                // build u, v
                                VECTOR4D_Build(&curr_poly->tvlist[0].v, &curr_poly->tvlist[1].v, &u);
                                VECTOR4D_Build(&curr_poly->tvlist[0].v, &curr_poly->tvlist[2].v, &v);

                                // compute cross product
                                VECTOR4D_Cross(&u, &v, &n);
                            } // end if

                            // at this point, we are almost ready, but we have to normalize the normal vector!
                            // this is a key optimization we can make later, we can pre-compute the length of all polygon
                            // normals, so this step can be optimized
                            // compute length of normal
                            //nl = VECTOR4D_Length_Fast2(&n);
                            nl = curr_poly->nlength;  

                            // compute vector from surface to light
                            VECTOR4D_Build(&curr_poly->tvlist[0].v, &lights[curr_light].tpos, &l);

                            // compute distance and attenuation
                            dist = VECTOR4D_Length_Fast2(&l);  

                            // and for the diffuse model
                            // Itotald =   Rsdiffuse*Idiffuse * (n . l)
                            // so we basically need to multiple it all together
                            // notice the scaling by 128, I want to avoid floating point calculations, not because they 
                            // are slower, but the conversion to and from cost cycles
                            dp = VECTOR4D_Dot(&n, &l);

                            // only add light if dp > 0
                            if (dp > 0)
                            { 
                                atten =  (lights[curr_light].kc + lights[curr_light].kl*dist + lights[curr_light].kq*dist*dist);    

                                i = 128*dp / (nl * dist * atten ); 

                                r_sum += (lights[curr_light].c_diffuse.r * r_base * i) / (256*128);
                                g_sum += (lights[curr_light].c_diffuse.g * g_base * i) / (256*128);
                                b_sum += (lights[curr_light].c_diffuse.b * b_base * i) / (256*128);
                            } // end if


                            //Write_Error("\npoint sum=%d,%d,%d",r_sum,g_sum,b_sum);

                        } // end if point
                        else
                            if (lights[curr_light].attr & LIGHTV2_ATTR_SPOTLIGHT1) ////////////////////////////////////
                            {
                                //Write_Error("\nentering spot light1...");

                                // perform spotlight/point computations simplified model that uses
                                // point light WITH a direction to simulate a spotlight
                                // light model for point light is once again:
                                //              I0point * Clpoint
                                //  I(d)point = ___________________
                                //              kc +  kl*d + kq*d2              
                                //
                                //  Where d = |p - s|
                                // thus it's almost identical to the infinite light, but attenuates as a function
                                // of distance from the point source to the surface point being lit

                                // test if we already computed poly normal in previous calculation
                                if (n.z==FLT_MAX)       
                                {
                                    // we need to compute the normal of this polygon face, and recall
                                    // that the vertices are in cw order, u=p0->p1, v=p0->p2, n=uxv

                                    // build u, v
                                    VECTOR4D_Build(&curr_poly->tvlist[0].v, &curr_poly->tvlist[1].v, &u);
                                    VECTOR4D_Build(&curr_poly->tvlist[0].v, &curr_poly->tvlist[2].v, &v);

                                    // compute cross product
                                    VECTOR4D_Cross(&u, &v, &n);
                                } // end if

                                // at this point, we are almost ready, but we have to normalize the normal vector!
                                // this is a key optimization we can make later, we can pre-compute the length of all polygon
                                // normals, so this step can be optimized
                                // compute length of normal
                                //nl = VECTOR4D_Length_Fast2(&n);
                                nl = curr_poly->nlength;  

                                // compute vector from surface to light
                                VECTOR4D_Build(&curr_poly->tvlist[0].v, &lights[curr_light].tpos, &l);

                                // compute distance and attenuation
                                dist = VECTOR4D_Length_Fast2(&l);  

                                // and for the diffuse model
                                // Itotald =   Rsdiffuse*Idiffuse * (n . l)
                                // so we basically need to multiple it all together
                                // notice the scaling by 128, I want to avoid floating point calculations, not because they 
                                // are slower, but the conversion to and from cost cycles

                                // note that I use the direction of the light here rather than a the vector to the light
                                // thus we are taking orientation into account which is similar to the spotlight model
                                dp = VECTOR4D_Dot(&n, &lights[curr_light].tdir);

                                // only add light if dp > 0
                                if (dp > 0)
                                { 
                                    atten =  (lights[curr_light].kc + lights[curr_light].kl*dist + lights[curr_light].kq*dist*dist);    

                                    i = 128*dp / (nl * atten ); 

                                    r_sum += (lights[curr_light].c_diffuse.r * r_base * i) / (256*128);
                                    g_sum += (lights[curr_light].c_diffuse.g * g_base * i) / (256*128);
                                    b_sum += (lights[curr_light].c_diffuse.b * b_base * i) / (256*128);
                                } // end if

                                //Write_Error("\nspotlight sum=%d,%d,%d",r_sum, g_sum, b_sum);

                            } // end if spotlight1
                            else
                                if (lights[curr_light].attr & LIGHTV2_ATTR_SPOTLIGHT2) // simple version ////////////////////
                                {
                                    //Write_Error("\nEntering spotlight2 ...");

                                    // perform spot light computations
                                    // light model for spot light simple version is once again:
                                    //         	     I0spotlight * Clspotlight * MAX( (l . s), 0)^pf                     
                                    // I(d)spotlight = __________________________________________      
                                    //               		 kc + kl*d + kq*d2        
                                    // Where d = |p - s|, and pf = power factor

                                    // thus it's almost identical to the point, but has the extra term in the numerator
                                    // relating the angle between the light source and the point on the surface

                                    // test if we already computed poly normal in previous calculation
                                    if (n.z==FLT_MAX)       
                                    {
                                        // we need to compute the normal of this polygon face, and recall
                                        // that the vertices are in cw order, u=p0->p1, v=p0->p2, n=uxv

                                        // build u, v
                                        VECTOR4D_Build(&curr_poly->tvlist[0].v, &curr_poly->tvlist[1].v, &u);
                                        VECTOR4D_Build(&curr_poly->tvlist[0].v, &curr_poly->tvlist[2].v, &v);

                                        // compute cross product
                                        VECTOR4D_Cross(&u, &v, &n);
                                    } // end if

                                    // at this point, we are almost ready, but we have to normalize the normal vector!
                                    // this is a key optimization we can make later, we can pre-compute the length of all polygon
                                    // normals, so this step can be optimized
                                    // compute length of normal
                                    //nl = VECTOR4D_Length_Fast2(&n);
                                    nl = curr_poly->nlength;  

                                    // and for the diffuse model
                                    // Itotald =   Rsdiffuse*Idiffuse * (n . l)
                                    // so we basically need to multiple it all together
                                    // notice the scaling by 128, I want to avoid floating point calculations, not because they 
                                    // are slower, but the conversion to and from cost cycles
                                    dp = VECTOR4D_Dot(&n, &lights[curr_light].tdir);

                                    // only add light if dp > 0
                                    if (dp > 0)
                                    { 
                                        // compute vector from light to surface (different from l which IS the light dir)
                                        VECTOR4D_Build( &lights[curr_light].tpos, &curr_poly->tvlist[0].v, &s);

                                        // compute length of s (distance to light source) to normalize s for lighting calc
                                        dists = VECTOR4D_Length_Fast2(&s);  

                                        // compute spot light term (s . l)
                                        float dpsl = VECTOR4D_Dot(&s, &lights[curr_light].tdir)/dists;

                                        // proceed only if term is positive
                                        if (dpsl > 0) 
                                        {
                                            // compute attenuation
                                            atten = (lights[curr_light].kc + lights[curr_light].kl*dists + lights[curr_light].kq*dists*dists);    

                                            // for speed reasons, pf exponents that are less that 1.0 are out of the question, and exponents
                                            // must be integral
                                            float dpsl_exp = dpsl;

                                            // exponentiate for positive integral powers
                                            for (int e_index = 1; e_index < (int)lights[curr_light].pf; e_index++)
                                                dpsl_exp*=dpsl;

                                            // now dpsl_exp holds (dpsl)^pf power which is of course (s . l)^pf 

                                            i = 128*dp * dpsl_exp / (nl * atten ); 

                                            r_sum += (lights[curr_light].c_diffuse.r * r_base * i) / (256*128);
                                            g_sum += (lights[curr_light].c_diffuse.g * g_base * i) / (256*128);
                                            b_sum += (lights[curr_light].c_diffuse.b * b_base * i) / (256*128);

                                        } // end if

                                    } // end if

                                    //Write_Error("\nSpotlight sum=%d,%d,%d",r_sum, g_sum, b_sum);

                                } // end if spot light

            } // end for light

            // make sure colors aren't out of range
            if (r_sum  > 255) r_sum = 255;
            if (g_sum  > 255) g_sum = 255;
            if (b_sum  > 255) b_sum = 255;

            //Write_Error("\nWriting final values to polygon %d = %d,%d,%d", poly, r_sum, g_sum, b_sum);

            // write the color over current color
            curr_poly->lit_color[0] = RGB16Bit(r_sum, g_sum, b_sum);

        } // end if
        else
            if (curr_poly->attr & POLY4DV2_ATTR_SHADE_MODE_GOURAUD) /////////////////////////////////
            {
                // gouraud shade, unfortunetly at this point in the pipeline, we have lost the original
                // mesh, and only have triangles, thus, many triangles will share the same vertices and
                // they will get lit 2x since we don't have any way to tell this, alas, performing lighting
                // at the object level is a better idea when gouraud shading is performed since the 
                // commonality of vertices is still intact, in any case, lighting here is similar to polygon
                // flat shaded, but we do it 3 times, once for each vertex, additionally there are lots
                // of opportunities for optimization, but I am going to lay off them for now, so the code
                // is intelligible, later we will optimize

                //Write_Error("\nEntering gouraud shader...");

                // step 1: extract the base color out in RGB mode
                // assume 565 format
                _RGB565FROM16BIT(curr_poly->color, &r_base, &g_base, &b_base);

                // scale to 8 bit 
                r_base <<= 3;
                g_base <<= 2;
                b_base <<= 3;

                //Write_Error("\nBase color=%d, %d, %d", r_base, g_base, b_base);

                // initialize color sum(s) for vertices
                r_sum0  = 0;
                g_sum0  = 0;
                b_sum0  = 0;

                r_sum1  = 0;
                g_sum1  = 0;
                b_sum1  = 0;

                r_sum2  = 0;
                g_sum2  = 0;
                b_sum2  = 0;

                //Write_Error("\nColor sums rgb0[%d, %d, %d], rgb1[%d,%d,%d], rgb2[%d,%d,%d]", r_sum0, g_sum0, b_sum0,   r_sum1, g_sum1, b_sum1, r_sum2, g_sum2, b_sum2);

                // new optimization:
                // when there are multiple lights in the system we will end up performing numerous
                // redundant calculations to minimize this my strategy is to set key variables to 
                // to MAX values on each loop, then during the lighting calcs to test the vars for
                // the max value, if they are the max value then the first light that needs the math
                // will do it, and then save the information into the variable (causing it to change state
                // from an invalid number) then any other lights that need the math can use the previously
                // computed value, however, since we already have the normals, not much here to cache on
                // a large scale, but small scale stuff is there, however, we will optimize those later

                // loop thru lights
                for (int curr_light = 0; curr_light < max_lights; curr_light++)
                {
                    // is this light active
                    if (lights[curr_light].state==LIGHTV2_STATE_OFF)
                        continue;

                    //Write_Error("\nprocessing light %d", curr_light);

                    // what kind of light are we dealing with
                    if (lights[curr_light].attr & LIGHTV2_ATTR_AMBIENT) ///////////////////////////////
                    {
                        //Write_Error("\nEntering ambient light....");

                        // simply multiply each channel against the color of the 
                        // polygon then divide by 256 to scale back to 0..255
                        // use a shift in real life!!! >> 8
                        ri = ((lights[curr_light].c_ambient.r * r_base) / 256);
                        gi = ((lights[curr_light].c_ambient.g * g_base) / 256);
                        bi = ((lights[curr_light].c_ambient.b * b_base) / 256);

                        // ambient light has the same affect on each vertex
                        r_sum0+=ri;
                        g_sum0+=gi;
                        b_sum0+=bi;

                        r_sum1+=ri;
                        g_sum1+=gi;
                        b_sum1+=bi;

                        r_sum2+=ri;
                        g_sum2+=gi;
                        b_sum2+=bi;

                        // there better only be one ambient light!
                        //Write_Error("\nexiting ambient ,sums rgb0[%d, %d, %d], rgb1[%d,%d,%d], rgb2[%d,%d,%d]", r_sum0, g_sum0, b_sum0,  r_sum1, g_sum1, b_sum1,  r_sum2, g_sum2, b_sum2);

                    } // end if
                    else
                        if (lights[curr_light].attr & LIGHTV2_ATTR_INFINITE) /////////////////////////////////
                        {
                            //Write_Error("\nentering infinite light...");

                            // infinite lighting, we need the surface normal, and the direction
                            // of the light source

                            // no longer need to compute normal or length, we already have the vertex normal
                            // and it's length is 1.0  
                            // ....

                            // ok, recalling the lighting model for infinite lights
                            // I(d)dir = I0dir * Cldir
                            // and for the diffuse model
                            // Itotald =   Rsdiffuse*Idiffuse * (n . l)
                            // so we basically need to multiple it all together
                            // notice the scaling by 128, I want to avoid floating point calculations, not because they 
                            // are slower, but the conversion to and from cost cycles

                            // need to perform lighting for each vertex (lots of redundant math, optimize later!)

                            //Write_Error("\nv0=[%f, %f, %f]=%f, v1=[%f, %f, %f]=%f, v2=[%f, %f, %f]=%f",
                            // curr_poly->tvlist[0].n.x, curr_poly->tvlist[0].n.y,curr_poly->tvlist[0].n.z, VECTOR4D_Length(&curr_poly->tvlist[0].n),
                            // curr_poly->tvlist[1].n.x, curr_poly->tvlist[1].n.y,curr_poly->tvlist[1].n.z, VECTOR4D_Length(&curr_poly->tvlist[1].n),
                            // curr_poly->tvlist[2].n.x, curr_poly->tvlist[2].n.y,curr_poly->tvlist[2].n.z, VECTOR4D_Length(&curr_poly->tvlist[2].n) );

                            // vertex 0
                            dp = VECTOR4D_Dot(&curr_poly->tvlist[0].n, &lights[curr_light].tdir); 

                            // only add light if dp > 0
                            if (dp > 0)
                            { 
                                i = 128*dp; 
                                r_sum0+= (lights[curr_light].c_diffuse.r * r_base * i) / (256*128);
                                g_sum0+= (lights[curr_light].c_diffuse.g * g_base * i) / (256*128);
                                b_sum0+= (lights[curr_light].c_diffuse.b * b_base * i) / (256*128);
                            } // end if

                            // vertex 1
                            dp = VECTOR4D_Dot(&curr_poly->tvlist[1].n, &lights[curr_light].tdir);

                            // only add light if dp > 0
                            if (dp > 0)
                            { 
                                i = 128*dp; 
                                r_sum1+= (lights[curr_light].c_diffuse.r * r_base * i) / (256*128);
                                g_sum1+= (lights[curr_light].c_diffuse.g * g_base * i) / (256*128);
                                b_sum1+= (lights[curr_light].c_diffuse.b * b_base * i) / (256*128);
                            } // end if

                            // vertex 2
                            dp = VECTOR4D_Dot(&curr_poly->tvlist[2].n, &lights[curr_light].tdir);

                            // only add light if dp > 0
                            if (dp > 0)
                            { 
                                i = 128*dp; 
                                r_sum2+= (lights[curr_light].c_diffuse.r * r_base * i) / (256*128);
                                g_sum2+= (lights[curr_light].c_diffuse.g * g_base * i) / (256*128);
                                b_sum2+= (lights[curr_light].c_diffuse.b * b_base * i) / (256*128);
                            } // end if

                            //Write_Error("\nexiting infinite, color sums rgb0[%d, %d, %d], rgb1[%d,%d,%d], rgb2[%d,%d,%d]", r_sum0, g_sum0, b_sum0,  r_sum1, g_sum1, b_sum1,  r_sum2, g_sum2, b_sum2);

                        } // end if infinite light
                        else
                            if (lights[curr_light].attr & LIGHTV2_ATTR_POINT) //////////////////////////////////////
                            {
                                // perform point light computations
                                // light model for point light is once again:
                                //              I0point * Clpoint
                                //  I(d)point = ___________________
                                //              kc +  kl*d + kq*d2              
                                //
                                //  Where d = |p - s|
                                // thus it's almost identical to the infinite light, but attenuates as a function
                                // of distance from the point source to the surface point being lit

                                // .. normal already in vertex

                                //Write_Error("\nEntering point light....");

                                // compute vector from surface to light
                                VECTOR4D_Build(&curr_poly->tvlist[0].v, &lights[curr_light].tpos, &l);

                                // compute distance and attenuation
                                dist = VECTOR4D_Length_Fast2(&l);  

                                // and for the diffuse model
                                // Itotald =   Rsdiffuse*Idiffuse * (n . l)
                                // so we basically need to multiple it all together
                                // notice the scaling by 128, I want to avoid floating point calculations, not because they 
                                // are slower, but the conversion to and from cost cycles

                                // perform the calculation for all 3 vertices

                                // vertex 0
                                dp = VECTOR4D_Dot(&curr_poly->tvlist[0].n, &l);

                                // only add light if dp > 0
                                if (dp > 0)
                                { 
                                    atten =  (lights[curr_light].kc + lights[curr_light].kl*dist + lights[curr_light].kq*dist*dist);    

                                    i = 128*dp / (dist * atten ); 

                                    r_sum0 += (lights[curr_light].c_diffuse.r * r_base * i) / (256*128);
                                    g_sum0 += (lights[curr_light].c_diffuse.g * g_base * i) / (256*128);
                                    b_sum0 += (lights[curr_light].c_diffuse.b * b_base * i) / (256*128);
                                } // end if


                                // vertex 1
                                dp = VECTOR4D_Dot(&curr_poly->tvlist[1].n, &l);

                                // only add light if dp > 0
                                if (dp > 0)
                                { 
                                    atten =  (lights[curr_light].kc + lights[curr_light].kl*dist + lights[curr_light].kq*dist*dist);    

                                    i = 128*dp / (dist * atten ); 

                                    r_sum1 += (lights[curr_light].c_diffuse.r * r_base * i) / (256*128);
                                    g_sum1 += (lights[curr_light].c_diffuse.g * g_base * i) / (256*128);
                                    b_sum1 += (lights[curr_light].c_diffuse.b * b_base * i) / (256*128);
                                } // end if

                                // vertex 2
                                dp = VECTOR4D_Dot(&curr_poly->tvlist[2].n, &l);

                                // only add light if dp > 0
                                if (dp > 0)
                                { 
                                    atten =  (lights[curr_light].kc + lights[curr_light].kl*dist + lights[curr_light].kq*dist*dist);    

                                    i = 128*dp / (dist * atten ); 

                                    r_sum2 += (lights[curr_light].c_diffuse.r * r_base * i) / (256*128);
                                    g_sum2 += (lights[curr_light].c_diffuse.g * g_base * i) / (256*128);
                                    b_sum2 += (lights[curr_light].c_diffuse.b * b_base * i) / (256*128);
                                } // end if

                                //Write_Error("\nexiting point light, rgb0[%d, %d, %d], rgb1[%d,%d,%d], rgb2[%d,%d,%d]", r_sum0, g_sum0, b_sum0,  r_sum1, g_sum1, b_sum1, r_sum2, g_sum2, b_sum2);

                            } // end if point
                            else
                                if (lights[curr_light].attr & LIGHTV2_ATTR_SPOTLIGHT1) ///////////////////////////////////////
                                {
                                    // perform spotlight/point computations simplified model that uses
                                    // point light WITH a direction to simulate a spotlight
                                    // light model for point light is once again:
                                    //              I0point * Clpoint
                                    //  I(d)point = ___________________
                                    //              kc +  kl*d + kq*d2              
                                    //
                                    //  Where d = |p - s|
                                    // thus it's almost identical to the infinite light, but attenuates as a function
                                    // of distance from the point source to the surface point being lit

                                    //Write_Error("\nentering spotlight1....");

                                    // .. normal is already computed

                                    // compute vector from surface to light
                                    VECTOR4D_Build(&curr_poly->tvlist[0].v, &lights[curr_light].tpos, &l);

                                    // compute distance and attenuation
                                    dist = VECTOR4D_Length_Fast2(&l);  

                                    // and for the diffuse model
                                    // Itotald =   Rsdiffuse*Idiffuse * (n . l)
                                    // so we basically need to multiple it all together
                                    // notice the scaling by 128, I want to avoid floating point calculations, not because they 
                                    // are slower, but the conversion to and from cost cycles

                                    // note that I use the direction of the light here rather than a the vector to the light
                                    // thus we are taking orientation into account which is similar to the spotlight model

                                    // vertex 0
                                    dp = VECTOR4D_Dot(&curr_poly->tvlist[0].n, &lights[curr_light].tdir);

                                    // only add light if dp > 0
                                    if (dp > 0)
                                    { 
                                        atten =  (lights[curr_light].kc + lights[curr_light].kl*dist + lights[curr_light].kq*dist*dist);    

                                        i = 128*dp / ( atten ); 

                                        r_sum0 += (lights[curr_light].c_diffuse.r * r_base * i) / (256*128);
                                        g_sum0 += (lights[curr_light].c_diffuse.g * g_base * i) / (256*128);
                                        b_sum0 += (lights[curr_light].c_diffuse.b * b_base * i) / (256*128);
                                    } // end if

                                    // vertex 1
                                    dp = VECTOR4D_Dot(&curr_poly->tvlist[1].n, &lights[curr_light].tdir);

                                    // only add light if dp > 0
                                    if (dp > 0)
                                    { 
                                        atten =  (lights[curr_light].kc + lights[curr_light].kl*dist + lights[curr_light].kq*dist*dist);    

                                        i = 128*dp / ( atten ); 

                                        r_sum1 += (lights[curr_light].c_diffuse.r * r_base * i) / (256*128);
                                        g_sum1 += (lights[curr_light].c_diffuse.g * g_base * i) / (256*128);
                                        b_sum1 += (lights[curr_light].c_diffuse.b * b_base * i) / (256*128);
                                    } // end i

                                    // vertex 2
                                    dp = VECTOR4D_Dot(&curr_poly->tvlist[2].n, &lights[curr_light].tdir);

                                    // only add light if dp > 0
                                    if (dp > 0)
                                    { 
                                        atten =  (lights[curr_light].kc + lights[curr_light].kl*dist + lights[curr_light].kq*dist*dist);    

                                        i = 128*dp / ( atten ); 

                                        r_sum2 += (lights[curr_light].c_diffuse.r * r_base * i) / (256*128);
                                        g_sum2 += (lights[curr_light].c_diffuse.g * g_base * i) / (256*128);
                                        b_sum2 += (lights[curr_light].c_diffuse.b * b_base * i) / (256*128);
                                    } // end i

                                    //Write_Error("\nexiting spotlight1, sums rgb0[%d, %d, %d], rgb1[%d,%d,%d], rgb2[%d,%d,%d]", r_sum0, g_sum0, b_sum0,  r_sum1, g_sum1, b_sum1,  r_sum2, g_sum2, b_sum2);

                                } // end if spotlight1
                                else
                                    if (lights[curr_light].attr & LIGHTV2_ATTR_SPOTLIGHT2) // simple version //////////////////////////
                                    {
                                        // perform spot light computations
                                        // light model for spot light simple version is once again:
                                        //         	     I0spotlight * Clspotlight * MAX( (l . s), 0)^pf                     
                                        // I(d)spotlight = __________________________________________      
                                        //               		 kc + kl*d + kq*d2        
                                        // Where d = |p - s|, and pf = power factor

                                        // thus it's almost identical to the point, but has the extra term in the numerator
                                        // relating the angle between the light source and the point on the surface

                                        // .. already have normals and length are 1.0

                                        // and for the diffuse model
                                        // Itotald =   Rsdiffuse*Idiffuse * (n . l)
                                        // so we basically need to multiple it all together
                                        // notice the scaling by 128, I want to avoid floating point calculations, not because they 
                                        // are slower, but the conversion to and from cost cycles

                                        //Write_Error("\nEntering spotlight2...");

                                        // tons of redundant math here! lots to optimize later!

                                        // vertex 0
                                        dp = VECTOR4D_Dot(&curr_poly->tvlist[0].n, &lights[curr_light].tdir);

                                        // only add light if dp > 0
                                        if (dp > 0)
                                        { 
                                            // compute vector from light to surface (different from l which IS the light dir)
                                            VECTOR4D_Build( &lights[curr_light].tpos, &curr_poly->tvlist[0].v, &s);

                                            // compute length of s (distance to light source) to normalize s for lighting calc
                                            dists = VECTOR4D_Length_Fast2(&s);  

                                            // compute spot light term (s . l)
                                            float dpsl = VECTOR4D_Dot(&s, &lights[curr_light].tdir)/dists;

                                            // proceed only if term is positive
                                            if (dpsl > 0) 
                                            {
                                                // compute attenuation
                                                atten = (lights[curr_light].kc + lights[curr_light].kl*dists + lights[curr_light].kq*dists*dists);    

                                                // for speed reasons, pf exponents that are less that 1.0 are out of the question, and exponents
                                                // must be integral
                                                float dpsl_exp = dpsl;

                                                // exponentiate for positive integral powers
                                                for (int e_index = 1; e_index < (int)lights[curr_light].pf; e_index++)
                                                    dpsl_exp*=dpsl;

                                                // now dpsl_exp holds (dpsl)^pf power which is of course (s . l)^pf 

                                                i = 128*dp * dpsl_exp / ( atten ); 

                                                r_sum0 += (lights[curr_light].c_diffuse.r * r_base * i) / (256*128);
                                                g_sum0 += (lights[curr_light].c_diffuse.g * g_base * i) / (256*128);
                                                b_sum0 += (lights[curr_light].c_diffuse.b * b_base * i) / (256*128);

                                            } // end if

                                        } // end if

                                        // vertex 1
                                        dp = VECTOR4D_Dot(&curr_poly->tvlist[1].n, &lights[curr_light].tdir);

                                        // only add light if dp > 0
                                        if (dp > 0)
                                        { 
                                            // compute vector from light to surface (different from l which IS the light dir)
                                            VECTOR4D_Build( &lights[curr_light].tpos, &curr_poly->tvlist[1].v, &s);

                                            // compute length of s (distance to light source) to normalize s for lighting calc
                                            dists = VECTOR4D_Length_Fast2(&s);  

                                            // compute spot light term (s . l)
                                            float dpsl = VECTOR4D_Dot(&s, &lights[curr_light].tdir)/dists;

                                            // proceed only if term is positive
                                            if (dpsl > 0) 
                                            {
                                                // compute attenuation
                                                atten = (lights[curr_light].kc + lights[curr_light].kl*dists + lights[curr_light].kq*dists*dists);    

                                                // for speed reasons, pf exponents that are less that 1.0 are out of the question, and exponents
                                                // must be integral
                                                float dpsl_exp = dpsl;

                                                // exponentiate for positive integral powers
                                                for (int e_index = 1; e_index < (int)lights[curr_light].pf; e_index++)
                                                    dpsl_exp*=dpsl;

                                                // now dpsl_exp holds (dpsl)^pf power which is of course (s . l)^pf 

                                                i = 128*dp * dpsl_exp / ( atten ); 

                                                r_sum1 += (lights[curr_light].c_diffuse.r * r_base * i) / (256*128);
                                                g_sum1 += (lights[curr_light].c_diffuse.g * g_base * i) / (256*128);
                                                b_sum1 += (lights[curr_light].c_diffuse.b * b_base * i) / (256*128);

                                            } // end if

                                        } // end if

                                        // vertex 2
                                        dp = VECTOR4D_Dot(&curr_poly->tvlist[2].n, &lights[curr_light].tdir);

                                        // only add light if dp > 0
                                        if (dp > 0)
                                        { 
                                            // compute vector from light to surface (different from l which IS the light dir)
                                            VECTOR4D_Build( &lights[curr_light].tpos, &curr_poly->tvlist[2].v, &s);

                                            // compute length of s (distance to light source) to normalize s for lighting calc
                                            dists = VECTOR4D_Length_Fast2(&s);  

                                            // compute spot light term (s . l)
                                            float dpsl = VECTOR4D_Dot(&s, &lights[curr_light].tdir)/dists;

                                            // proceed only if term is positive
                                            if (dpsl > 0) 
                                            {
                                                // compute attenuation
                                                atten = (lights[curr_light].kc + lights[curr_light].kl*dists + lights[curr_light].kq*dists*dists);    

                                                // for speed reasons, pf exponents that are less that 1.0 are out of the question, and exponents
                                                // must be integral
                                                float dpsl_exp = dpsl;

                                                // exponentiate for positive integral powers
                                                for (int e_index = 1; e_index < (int)lights[curr_light].pf; e_index++)
                                                    dpsl_exp*=dpsl;

                                                // now dpsl_exp holds (dpsl)^pf power which is of course (s . l)^pf 

                                                i = 128*dp * dpsl_exp / ( atten ); 

                                                r_sum2 += (lights[curr_light].c_diffuse.r * r_base * i) / (256*128);
                                                g_sum2 += (lights[curr_light].c_diffuse.g * g_base * i) / (256*128);
                                                b_sum2 += (lights[curr_light].c_diffuse.b * b_base * i) / (256*128);
                                            } // end if

                                        } // end if

                                        //Write_Error("\nexiting spotlight2, sums rgb0[%d, %d, %d], rgb1[%d,%d,%d], rgb2[%d,%d,%d]", r_sum0, g_sum0, b_sum0,   r_sum1, g_sum1, b_sum1,  r_sum2, g_sum2, b_sum2);

                                    } // end if spot light

                } // end for light

                // make sure colors aren't out of range
                if (r_sum0  > 255) r_sum0 = 255;
                if (g_sum0  > 255) g_sum0 = 255;
                if (b_sum0  > 255) b_sum0 = 255;

                if (r_sum1  > 255) r_sum1 = 255;
                if (g_sum1  > 255) g_sum1 = 255;
                if (b_sum1  > 255) b_sum1 = 255;

                if (r_sum2  > 255) r_sum2 = 255;
                if (g_sum2  > 255) g_sum2 = 255;
                if (b_sum2  > 255) b_sum2 = 255;

                //Write_Error("\nwriting color for poly %d", poly);

                //Write_Error("\n******** final sums rgb0[%d, %d, %d], rgb1[%d,%d,%d], rgb2[%d,%d,%d]", r_sum0, g_sum0, b_sum0,  r_sum1, g_sum1, b_sum1, r_sum2, g_sum2, b_sum2);

                // write the colors
                curr_poly->lit_color[0] = RGB16Bit(r_sum0, g_sum0, b_sum0);
                curr_poly->lit_color[1] = RGB16Bit(r_sum1, g_sum1, b_sum1);
                curr_poly->lit_color[2] = RGB16Bit(r_sum2, g_sum2, b_sum2);

            } // end if
            else // assume POLY4DV2_ATTR_SHADE_MODE_CONSTANT
            {
                // emmisive shading only, do nothing
                // ...
                curr_poly->lit_color[0] = curr_poly->color;

                //Write_Error("\nentering constant shader, and exiting...");

            } // end if

    } // end for poly

    // return success
    return(1);

} // end Light_RENDERLIST4DV2_World2_16

//////////////////////////////////////////////////////////////////////////////

int Light_RENDERLIST4DV2_World2(RENDERLIST4DV2_PTR rend_list,  // list to process
                                CAM4DV1_PTR cam,     // camera position
                                LIGHTV2_PTR lights,  // light list (might have more than one)
                                int max_lights)      // maximum lights in list
{
    // {andre work in progress }

    // 8-bit version of function
    // function lights the entire rendering list based on the sent lights and camera. the function supports
    // constant/pure shading (emmisive), flat shading with ambient, infinite, point lights, and spot lights
    // note that this lighting function is rather brute force and simply follows the math, however
    // there are some clever integer operations that are used in scale 256 rather than going to floating
    // point, but why? floating point and ints are the same speed, HOWEVER, the conversion to and from floating
    // point can be cycle intensive, so if you can keep your calcs in ints then you can gain some speed
    // also note, type 1 spot lights are simply point lights with direction, the "cone" is more of a function
    // of the falloff due to attenuation, but they still look like spot lights
    // type 2 spot lights are implemented with the intensity having a dot product relationship with the
    // angle from the surface point to the light direction just like in the optimized model, but the pf term
    // that is used for a concentration control must be 1,2,3,.... integral and non-fractional

    // also note since we are dealing with a rendering list and not object, the final lit color is
    // immediately written over the real color

    unsigned int r_base, g_base,   b_base,  // base color being lit
        r_sum,  g_sum,    b_sum,   // sum of lighting process over all lights
        r_sum0,  g_sum0,  b_sum0,
        r_sum1,  g_sum1,  b_sum1,
        r_sum2,  g_sum2,  b_sum2,
        ri,gi,bi,
        shaded_color;            // final color

    float dp,     // dot product 
        dist,   // distance from light to surface
        dists, 
        i,      // general intensities
        nl,     // length of normal
        atten;  // attenuation computations

    VECTOR4D u, v, n, l, d, s; // used for cross product and light vector calculations

    //Write_Error("\nEntering lighting function");

    // for each valid poly, light it...
    for (int poly=0; poly < rend_list->num_polys; poly++)
    {
        // acquire polygon
        POLYF4DV2_PTR curr_poly = rend_list->poly_ptrs[poly];

        // light this polygon if and only if it's not clipped, not culled,
        // active, and visible
        if (!(curr_poly->state & POLY4DV2_STATE_ACTIVE) ||
            (curr_poly->state & POLY4DV2_STATE_CLIPPED ) ||
            (curr_poly->state & POLY4DV2_STATE_BACKFACE) ||
            (curr_poly->state & POLY4DV2_STATE_LIT) )
            continue; // move onto next poly

        //Write_Error("\npoly %d",poly);

#ifdef DEBUG_ON
        // track rendering stats
        debug_polys_lit_per_frame++;
#endif

        // set state of polygon to lit
        SET_BIT(curr_poly->state, POLY4DV2_STATE_LIT);

        // we will use the transformed polygon vertex list since the backface removal
        // only makes sense at the world coord stage further of the pipeline 

        // test the lighting mode of the polygon (use flat for flat, gouraud))
        if (curr_poly->attr & POLY4DV2_ATTR_SHADE_MODE_FLAT)
        {
            //Write_Error("\nEntering Flat Shader");

            r_base = palette[curr_poly->color].peRed;
            g_base = palette[curr_poly->color].peGreen;
            b_base = palette[curr_poly->color].peBlue;

            //Write_Error("\nBase color=%d,%d,%d", r_base, g_base, b_base);

            // initialize color sum
            r_sum  = 0;
            g_sum  = 0;
            b_sum  = 0;

            //Write_Error("\nsum color=%d,%d,%d", r_sum, g_sum, b_sum);

            // new optimization:
            // when there are multiple lights in the system we will end up performing numerous
            // redundant calculations to minimize this my strategy is to set key variables to 
            // to MAX values on each loop, then during the lighting calcs to test the vars for
            // the max value, if they are the max value then the first light that needs the math
            // will do it, and then save the information into the variable (causing it to change state
            // from an invalid number) then any other lights that need the math can use the previously
            // computed value

            // set surface normal.z to FLT_MAX to flag it as non-computed
            n.z = FLT_MAX;

            // loop thru lights
            for (int curr_light = 0; curr_light < max_lights; curr_light++)
            {
                // is this light active
                if (lights[curr_light].state==LIGHTV2_STATE_OFF)
                    continue;

                //Write_Error("\nprocessing light %d",curr_light);

                // what kind of light are we dealing with
                if (lights[curr_light].attr & LIGHTV2_ATTR_AMBIENT)
                {
                    //Write_Error("\nEntering ambient light...");

                    // simply multiply each channel against the color of the 
                    // polygon then divide by 256 to scale back to 0..255
                    // use a shift in real life!!! >> 8
                    r_sum+= ((lights[curr_light].c_ambient.r * r_base) / 256);
                    g_sum+= ((lights[curr_light].c_ambient.g * g_base) / 256);
                    b_sum+= ((lights[curr_light].c_ambient.b * b_base) / 256);

                    //Write_Error("\nambient sum=%d,%d,%d", r_sum, g_sum, b_sum);

                    // there better only be one ambient light!

                } // end if
                else
                    if (lights[curr_light].attr & LIGHTV2_ATTR_INFINITE) ///////////////////////////////////////////
                    {
                        //Write_Error("\nEntering infinite light...");

                        // infinite lighting, we need the surface normal, and the direction
                        // of the light source

                        // test if we already computed poly normal in previous calculation
                        if (n.z==FLT_MAX)       
                        {
                            // we need to compute the normal of this polygon face, and recall
                            // that the vertices are in cw order, u=p0->p1, v=p0->p2, n=uxv

                            // build u, v
                            VECTOR4D_Build(&curr_poly->tvlist[0].v, &curr_poly->tvlist[1].v, &u);
                            VECTOR4D_Build(&curr_poly->tvlist[0].v, &curr_poly->tvlist[2].v, &v);

                            // compute cross product
                            VECTOR4D_Cross(&u, &v, &n);
                        } // end if

                        // at this point, we are almost ready, but we have to normalize the normal vector!
                        // this is a key optimization we can make later, we can pre-compute the length of all polygon
                        // normals, so this step can be optimized
                        // compute length of normal
                        //nl = VECTOR4D_Length_Fast2(&n);
                        nl = curr_poly->nlength;  

                        // ok, recalling the lighting model for infinite lights
                        // I(d)dir = I0dir * Cldir
                        // and for the diffuse model
                        // Itotald =   Rsdiffuse*Idiffuse * (n . l)
                        // so we basically need to multiple it all together
                        // notice the scaling by 128, I want to avoid floating point calculations, not because they 
                        // are slower, but the conversion to and from cost cycles

                        dp = VECTOR4D_Dot(&n, &lights[curr_light].tdir);

                        // only add light if dp > 0
                        if (dp > 0)
                        { 
                            i = 128*dp/nl; 
                            r_sum+= (lights[curr_light].c_diffuse.r * r_base * i) / (256*128);
                            g_sum+= (lights[curr_light].c_diffuse.g * g_base * i) / (256*128);
                            b_sum+= (lights[curr_light].c_diffuse.b * b_base * i) / (256*128);
                        } // end if

                        //Write_Error("\ninfinite sum=%d,%d,%d", r_sum, g_sum, b_sum);

                    } // end if infinite light
                    else
                        if (lights[curr_light].attr & LIGHTV2_ATTR_POINT) ///////////////////////////////////////
                        {
                            //Write_Error("\nEntering point light...");

                            // perform point light computations
                            // light model for point light is once again:
                            //              I0point * Clpoint
                            //  I(d)point = ___________________
                            //              kc +  kl*d + kq*d2              
                            //
                            //  Where d = |p - s|
                            // thus it's almost identical to the infinite light, but attenuates as a function
                            // of distance from the point source to the surface point being lit

                            // test if we already computed poly normal in previous calculation
                            if (n.z==FLT_MAX)       
                            {
                                // we need to compute the normal of this polygon face, and recall
                                // that the vertices are in cw order, u=p0->p1, v=p0->p2, n=uxv

                                // build u, v
                                VECTOR4D_Build(&curr_poly->tvlist[0].v, &curr_poly->tvlist[1].v, &u);
                                VECTOR4D_Build(&curr_poly->tvlist[0].v, &curr_poly->tvlist[2].v, &v);

                                // compute cross product
                                VECTOR4D_Cross(&u, &v, &n);
                            } // end if

                            // at this point, we are almost ready, but we have to normalize the normal vector!
                            // this is a key optimization we can make later, we can pre-compute the length of all polygon
                            // normals, so this step can be optimized
                            // compute length of normal
                            //nl = VECTOR4D_Length_Fast2(&n);
                            nl = curr_poly->nlength;  

                            // compute vector from surface to light
                            VECTOR4D_Build(&curr_poly->tvlist[0].v, &lights[curr_light].tpos, &l);

                            // compute distance and attenuation
                            dist = VECTOR4D_Length_Fast2(&l);  

                            // and for the diffuse model
                            // Itotald =   Rsdiffuse*Idiffuse * (n . l)
                            // so we basically need to multiple it all together
                            // notice the scaling by 128, I want to avoid floating point calculations, not because they 
                            // are slower, but the conversion to and from cost cycles
                            dp = VECTOR4D_Dot(&n, &l);

                            // only add light if dp > 0
                            if (dp > 0)
                            { 
                                atten =  (lights[curr_light].kc + lights[curr_light].kl*dist + lights[curr_light].kq*dist*dist);    

                                i = 128*dp / (nl * dist * atten ); 

                                r_sum += (lights[curr_light].c_diffuse.r * r_base * i) / (256*128);
                                g_sum += (lights[curr_light].c_diffuse.g * g_base * i) / (256*128);
                                b_sum += (lights[curr_light].c_diffuse.b * b_base * i) / (256*128);
                            } // end if


                            //Write_Error("\npoint sum=%d,%d,%d",r_sum,g_sum,b_sum);

                        } // end if point
                        else
                            if (lights[curr_light].attr & LIGHTV2_ATTR_SPOTLIGHT1) ////////////////////////////////////
                            {
                                //Write_Error("\nentering spot light1...");

                                // perform spotlight/point computations simplified model that uses
                                // point light WITH a direction to simulate a spotlight
                                // light model for point light is once again:
                                //              I0point * Clpoint
                                //  I(d)point = ___________________
                                //              kc +  kl*d + kq*d2              
                                //
                                //  Where d = |p - s|
                                // thus it's almost identical to the infinite light, but attenuates as a function
                                // of distance from the point source to the surface point being lit

                                // test if we already computed poly normal in previous calculation
                                if (n.z==FLT_MAX)       
                                {
                                    // we need to compute the normal of this polygon face, and recall
                                    // that the vertices are in cw order, u=p0->p1, v=p0->p2, n=uxv

                                    // build u, v
                                    VECTOR4D_Build(&curr_poly->tvlist[0].v, &curr_poly->tvlist[1].v, &u);
                                    VECTOR4D_Build(&curr_poly->tvlist[0].v, &curr_poly->tvlist[2].v, &v);

                                    // compute cross product
                                    VECTOR4D_Cross(&u, &v, &n);
                                } // end if

                                // at this point, we are almost ready, but we have to normalize the normal vector!
                                // this is a key optimization we can make later, we can pre-compute the length of all polygon
                                // normals, so this step can be optimized
                                // compute length of normal
                                //nl = VECTOR4D_Length_Fast2(&n);
                                nl = curr_poly->nlength;  

                                // compute vector from surface to light
                                VECTOR4D_Build(&curr_poly->tvlist[0].v, &lights[curr_light].tpos, &l);

                                // compute distance and attenuation
                                dist = VECTOR4D_Length_Fast2(&l);  

                                // and for the diffuse model
                                // Itotald =   Rsdiffuse*Idiffuse * (n . l)
                                // so we basically need to multiple it all together
                                // notice the scaling by 128, I want to avoid floating point calculations, not because they 
                                // are slower, but the conversion to and from cost cycles

                                // note that I use the direction of the light here rather than a the vector to the light
                                // thus we are taking orientation into account which is similar to the spotlight model
                                dp = VECTOR4D_Dot(&n, &lights[curr_light].tdir);

                                // only add light if dp > 0
                                if (dp > 0)
                                { 
                                    atten =  (lights[curr_light].kc + lights[curr_light].kl*dist + lights[curr_light].kq*dist*dist);    

                                    i = 128*dp / (nl * atten ); 

                                    r_sum += (lights[curr_light].c_diffuse.r * r_base * i) / (256*128);
                                    g_sum += (lights[curr_light].c_diffuse.g * g_base * i) / (256*128);
                                    b_sum += (lights[curr_light].c_diffuse.b * b_base * i) / (256*128);
                                } // end if

                                //Write_Error("\nspotlight sum=%d,%d,%d",r_sum, g_sum, b_sum);

                            } // end if spotlight1
                            else
                                if (lights[curr_light].attr & LIGHTV2_ATTR_SPOTLIGHT2) // simple version ////////////////////
                                {
                                    //Write_Error("\nEntering spotlight2 ...");

                                    // perform spot light computations
                                    // light model for spot light simple version is once again:
                                    //         	     I0spotlight * Clspotlight * MAX( (l . s), 0)^pf                     
                                    // I(d)spotlight = __________________________________________      
                                    //               		 kc + kl*d + kq*d2        
                                    // Where d = |p - s|, and pf = power factor

                                    // thus it's almost identical to the point, but has the extra term in the numerator
                                    // relating the angle between the light source and the point on the surface

                                    // test if we already computed poly normal in previous calculation
                                    if (n.z==FLT_MAX)       
                                    {
                                        // we need to compute the normal of this polygon face, and recall
                                        // that the vertices are in cw order, u=p0->p1, v=p0->p2, n=uxv

                                        // build u, v
                                        VECTOR4D_Build(&curr_poly->tvlist[0].v, &curr_poly->tvlist[1].v, &u);
                                        VECTOR4D_Build(&curr_poly->tvlist[0].v, &curr_poly->tvlist[2].v, &v);

                                        // compute cross product
                                        VECTOR4D_Cross(&u, &v, &n);
                                    } // end if

                                    // at this point, we are almost ready, but we have to normalize the normal vector!
                                    // this is a key optimization we can make later, we can pre-compute the length of all polygon
                                    // normals, so this step can be optimized
                                    // compute length of normal
                                    //nl = VECTOR4D_Length_Fast2(&n);
                                    nl = curr_poly->nlength;  

                                    // and for the diffuse model
                                    // Itotald =   Rsdiffuse*Idiffuse * (n . l)
                                    // so we basically need to multiple it all together
                                    // notice the scaling by 128, I want to avoid floating point calculations, not because they 
                                    // are slower, but the conversion to and from cost cycles
                                    dp = VECTOR4D_Dot(&n, &lights[curr_light].tdir);

                                    // only add light if dp > 0
                                    if (dp > 0)
                                    { 
                                        // compute vector from light to surface (different from l which IS the light dir)
                                        VECTOR4D_Build( &lights[curr_light].tpos, &curr_poly->tvlist[0].v, &s);

                                        // compute length of s (distance to light source) to normalize s for lighting calc
                                        dists = VECTOR4D_Length_Fast2(&s);  

                                        // compute spot light term (s . l)
                                        float dpsl = VECTOR4D_Dot(&s, &lights[curr_light].tdir)/dists;

                                        // proceed only if term is positive
                                        if (dpsl > 0) 
                                        {
                                            // compute attenuation
                                            atten = (lights[curr_light].kc + lights[curr_light].kl*dists + lights[curr_light].kq*dists*dists);    

                                            // for speed reasons, pf exponents that are less that 1.0 are out of the question, and exponents
                                            // must be integral
                                            float dpsl_exp = dpsl;

                                            // exponentiate for positive integral powers
                                            for (int e_index = 1; e_index < (int)lights[curr_light].pf; e_index++)
                                                dpsl_exp*=dpsl;

                                            // now dpsl_exp holds (dpsl)^pf power which is of course (s . l)^pf 

                                            i = 128*dp * dpsl_exp / (nl * atten ); 

                                            r_sum += (lights[curr_light].c_diffuse.r * r_base * i) / (256*128);
                                            g_sum += (lights[curr_light].c_diffuse.g * g_base * i) / (256*128);
                                            b_sum += (lights[curr_light].c_diffuse.b * b_base * i) / (256*128);

                                        } // end if

                                    } // end if

                                    //Write_Error("\nSpotlight sum=%d,%d,%d",r_sum, g_sum, b_sum);

                                } // end if spot light

            } // end for light

            // make sure colors aren't out of range
            if (r_sum  > 255) r_sum = 255;
            if (g_sum  > 255) g_sum = 255;
            if (b_sum  > 255) b_sum = 255;

            //Write_Error("\nWriting final values to polygon %d = %d,%d,%d", poly, r_sum, g_sum, b_sum);

            // write the color over current color       
            curr_poly->lit_color[0] = rgblookup[RGB16Bit565(r_sum, g_sum, b_sum)];

        } // end if
        else
            if (curr_poly->attr & POLY4DV2_ATTR_SHADE_MODE_GOURAUD) /////////////////////////////////
            {
                // gouraud shade, unfortunetly at this point in the pipeline, we have lost the original
                // mesh, and only have triangles, thus, many triangles will share the same vertices and
                // they will get lit 2x since we don't have any way to tell this, alas, performing lighting
                // at the object level is a better idea when gouraud shading is performed since the 
                // commonality of vertices is still intact, in any case, lighting here is similar to polygon
                // flat shaded, but we do it 3 times, once for each vertex, additionally there are lots
                // of opportunities for optimization, but I am going to lay off them for now, so the code
                // is intelligible, later we will optimize

                //Write_Error("\nEntering gouraud shader...");

                // step 1: extract the base color out in RGB mode
                r_base = palette[curr_poly->color].peRed;
                g_base = palette[curr_poly->color].peGreen;
                b_base = palette[curr_poly->color].peBlue;

                //Write_Error("\nBase color=%d, %d, %d", r_base, g_base, b_base);

                // initialize color sum(s) for vertices
                r_sum0  = 0;
                g_sum0  = 0;
                b_sum0  = 0;

                r_sum1  = 0;
                g_sum1  = 0;
                b_sum1  = 0;

                r_sum2  = 0;
                g_sum2  = 0;
                b_sum2  = 0;

                //Write_Error("\nColor sums rgb0[%d, %d, %d], rgb1[%d,%d,%d], rgb2[%d,%d,%d]", r_sum0, g_sum0, b_sum0,   r_sum1, g_sum1, b_sum1, r_sum2, g_sum2, b_sum2);

                // new optimization:
                // when there are multiple lights in the system we will end up performing numerous
                // redundant calculations to minimize this my strategy is to set key variables to 
                // to MAX values on each loop, then during the lighting calcs to test the vars for
                // the max value, if they are the max value then the first light that needs the math
                // will do it, and then save the information into the variable (causing it to change state
                // from an invalid number) then any other lights that need the math can use the previously
                // computed value       

                // loop thru lights
                for (int curr_light = 0; curr_light < max_lights; curr_light++)
                {
                    // is this light active
                    if (lights[curr_light].state==LIGHTV2_STATE_OFF)
                        continue;

                    //Write_Error("\nprocessing light %d", curr_light);

                    // what kind of light are we dealing with
                    if (lights[curr_light].attr & LIGHTV2_ATTR_AMBIENT) ///////////////////////////////
                    {
                        //Write_Error("\nEntering ambient light....");

                        // simply multiply each channel against the color of the 
                        // polygon then divide by 256 to scale back to 0..255
                        // use a shift in real life!!! >> 8
                        ri = ((lights[curr_light].c_ambient.r * r_base) / 256);
                        gi = ((lights[curr_light].c_ambient.g * g_base) / 256);
                        bi = ((lights[curr_light].c_ambient.b * b_base) / 256);

                        // ambient light has the same affect on each vertex
                        r_sum0+=ri;
                        g_sum0+=gi;
                        b_sum0+=bi;

                        r_sum1+=ri;
                        g_sum1+=gi;
                        b_sum1+=bi;

                        r_sum2+=ri;
                        g_sum2+=gi;
                        b_sum2+=bi;

                        // there better only be one ambient light!
                        //Write_Error("\nexiting ambient ,sums rgb0[%d, %d, %d], rgb1[%d,%d,%d], rgb2[%d,%d,%d]", r_sum0, g_sum0, b_sum0,  r_sum1, g_sum1, b_sum1,  r_sum2, g_sum2, b_sum2);

                    } // end if
                    else
                        if (lights[curr_light].attr & LIGHTV2_ATTR_INFINITE) /////////////////////////////////
                        {
                            //Write_Error("\nentering infinite light...");

                            // infinite lighting, we need the surface normal, and the direction
                            // of the light source

                            // no longer need to compute normal or length, we already have the vertex normal
                            // and it's length is 1.0  
                            // ....

                            // ok, recalling the lighting model for infinite lights
                            // I(d)dir = I0dir * Cldir
                            // and for the diffuse model
                            // Itotald =   Rsdiffuse*Idiffuse * (n . l)
                            // so we basically need to multiple it all together
                            // notice the scaling by 128, I want to avoid floating point calculations, not because they 
                            // are slower, but the conversion to and from cost cycles

                            // need to perform lighting for each vertex (lots of redundant math, optimize later!)

                            //Write_Error("\nv0=[%f, %f, %f]=%f, v1=[%f, %f, %f]=%f, v2=[%f, %f, %f]=%f",
                            // curr_poly->tvlist[0].n.x, curr_poly->tvlist[0].n.y,curr_poly->tvlist[0].n.z, VECTOR4D_Length(&curr_poly->tvlist[0].n),
                            // curr_poly->tvlist[1].n.x, curr_poly->tvlist[1].n.y,curr_poly->tvlist[1].n.z, VECTOR4D_Length(&curr_poly->tvlist[1].n),
                            // curr_poly->tvlist[2].n.x, curr_poly->tvlist[2].n.y,curr_poly->tvlist[2].n.z, VECTOR4D_Length(&curr_poly->tvlist[2].n) );

                            // vertex 0
                            dp = VECTOR4D_Dot(&curr_poly->tvlist[0].n, &lights[curr_light].tdir); 

                            // only add light if dp > 0
                            if (dp > 0)
                            { 
                                i = 128*dp; 
                                r_sum0+= (lights[curr_light].c_diffuse.r * r_base * i) / (256*128);
                                g_sum0+= (lights[curr_light].c_diffuse.g * g_base * i) / (256*128);
                                b_sum0+= (lights[curr_light].c_diffuse.b * b_base * i) / (256*128);
                            } // end if

                            // vertex 1
                            dp = VECTOR4D_Dot(&curr_poly->tvlist[1].n, &lights[curr_light].tdir);

                            // only add light if dp > 0
                            if (dp > 0)
                            { 
                                i = 128*dp; 
                                r_sum1+= (lights[curr_light].c_diffuse.r * r_base * i) / (256*128);
                                g_sum1+= (lights[curr_light].c_diffuse.g * g_base * i) / (256*128);
                                b_sum1+= (lights[curr_light].c_diffuse.b * b_base * i) / (256*128);
                            } // end if

                            // vertex 2
                            dp = VECTOR4D_Dot(&curr_poly->tvlist[2].n, &lights[curr_light].tdir);

                            // only add light if dp > 0
                            if (dp > 0)
                            { 
                                i = 128*dp; 
                                r_sum2+= (lights[curr_light].c_diffuse.r * r_base * i) / (256*128);
                                g_sum2+= (lights[curr_light].c_diffuse.g * g_base * i) / (256*128);
                                b_sum2+= (lights[curr_light].c_diffuse.b * b_base * i) / (256*128);
                            } // end if

                            //Write_Error("\nexiting infinite, color sums rgb0[%d, %d, %d], rgb1[%d,%d,%d], rgb2[%d,%d,%d]", r_sum0, g_sum0, b_sum0,  r_sum1, g_sum1, b_sum1,  r_sum2, g_sum2, b_sum2);

                        } // end if infinite light
                        else
                            if (lights[curr_light].attr & LIGHTV2_ATTR_POINT) //////////////////////////////////////
                            {
                                // perform point light computations
                                // light model for point light is once again:
                                //              I0point * Clpoint
                                //  I(d)point = ___________________
                                //              kc +  kl*d + kq*d2              
                                //
                                //  Where d = |p - s|
                                // thus it's almost identical to the infinite light, but attenuates as a function
                                // of distance from the point source to the surface point being lit

                                // .. normal already in vertex

                                //Write_Error("\nEntering point light....");

                                // compute vector from surface to light
                                VECTOR4D_Build(&curr_poly->tvlist[0].v, &lights[curr_light].tpos, &l);

                                // compute distance and attenuation
                                dist = VECTOR4D_Length_Fast2(&l);  

                                // and for the diffuse model
                                // Itotald =   Rsdiffuse*Idiffuse * (n . l)
                                // so we basically need to multiple it all together
                                // notice the scaling by 128, I want to avoid floating point calculations, not because they 
                                // are slower, but the conversion to and from cost cycles

                                // perform the calculation for all 3 vertices

                                // vertex 0
                                dp = VECTOR4D_Dot(&curr_poly->tvlist[0].n, &l);

                                // only add light if dp > 0
                                if (dp > 0)
                                { 
                                    atten =  (lights[curr_light].kc + lights[curr_light].kl*dist + lights[curr_light].kq*dist*dist);    

                                    i = 128*dp / (dist * atten ); 

                                    r_sum0 += (lights[curr_light].c_diffuse.r * r_base * i) / (256*128);
                                    g_sum0 += (lights[curr_light].c_diffuse.g * g_base * i) / (256*128);
                                    b_sum0 += (lights[curr_light].c_diffuse.b * b_base * i) / (256*128);
                                } // end if


                                // vertex 1
                                dp = VECTOR4D_Dot(&curr_poly->tvlist[1].n, &l);

                                // only add light if dp > 0
                                if (dp > 0)
                                { 
                                    atten =  (lights[curr_light].kc + lights[curr_light].kl*dist + lights[curr_light].kq*dist*dist);    

                                    i = 128*dp / (dist * atten ); 

                                    r_sum1 += (lights[curr_light].c_diffuse.r * r_base * i) / (256*128);
                                    g_sum1 += (lights[curr_light].c_diffuse.g * g_base * i) / (256*128);
                                    b_sum1 += (lights[curr_light].c_diffuse.b * b_base * i) / (256*128);
                                } // end if

                                // vertex 2
                                dp = VECTOR4D_Dot(&curr_poly->tvlist[2].n, &l);

                                // only add light if dp > 0
                                if (dp > 0)
                                { 
                                    atten =  (lights[curr_light].kc + lights[curr_light].kl*dist + lights[curr_light].kq*dist*dist);    

                                    i = 128*dp / (dist * atten ); 

                                    r_sum2 += (lights[curr_light].c_diffuse.r * r_base * i) / (256*128);
                                    g_sum2 += (lights[curr_light].c_diffuse.g * g_base * i) / (256*128);
                                    b_sum2 += (lights[curr_light].c_diffuse.b * b_base * i) / (256*128);
                                } // end if

                                //Write_Error("\nexiting point light, rgb0[%d, %d, %d], rgb1[%d,%d,%d], rgb2[%d,%d,%d]", r_sum0, g_sum0, b_sum0,  r_sum1, g_sum1, b_sum1, r_sum2, g_sum2, b_sum2);

                            } // end if point
                            else
                                if (lights[curr_light].attr & LIGHTV2_ATTR_SPOTLIGHT1) ///////////////////////////////////////
                                {
                                    // perform spotlight/point computations simplified model that uses
                                    // point light WITH a direction to simulate a spotlight
                                    // light model for point light is once again:
                                    //              I0point * Clpoint
                                    //  I(d)point = ___________________
                                    //              kc +  kl*d + kq*d2              
                                    //
                                    //  Where d = |p - s|
                                    // thus it's almost identical to the infinite light, but attenuates as a function
                                    // of distance from the point source to the surface point being lit

                                    //Write_Error("\nentering spotlight1....");

                                    // .. normal is already computed

                                    // compute vector from surface to light
                                    VECTOR4D_Build(&curr_poly->tvlist[0].v, &lights[curr_light].tpos, &l);

                                    // compute distance and attenuation
                                    dist = VECTOR4D_Length_Fast2(&l);  

                                    // and for the diffuse model
                                    // Itotald =   Rsdiffuse*Idiffuse * (n . l)
                                    // so we basically need to multiple it all together
                                    // notice the scaling by 128, I want to avoid floating point calculations, not because they 
                                    // are slower, but the conversion to and from cost cycles

                                    // note that I use the direction of the light here rather than a the vector to the light
                                    // thus we are taking orientation into account which is similar to the spotlight model

                                    // vertex 0
                                    dp = VECTOR4D_Dot(&curr_poly->tvlist[0].n, &lights[curr_light].tdir);

                                    // only add light if dp > 0
                                    if (dp > 0)
                                    { 
                                        atten =  (lights[curr_light].kc + lights[curr_light].kl*dist + lights[curr_light].kq*dist*dist);    

                                        i = 128*dp / ( atten ); 

                                        r_sum0 += (lights[curr_light].c_diffuse.r * r_base * i) / (256*128);
                                        g_sum0 += (lights[curr_light].c_diffuse.g * g_base * i) / (256*128);
                                        b_sum0 += (lights[curr_light].c_diffuse.b * b_base * i) / (256*128);
                                    } // end if

                                    // vertex 1
                                    dp = VECTOR4D_Dot(&curr_poly->tvlist[1].n, &lights[curr_light].tdir);

                                    // only add light if dp > 0
                                    if (dp > 0)
                                    { 
                                        atten =  (lights[curr_light].kc + lights[curr_light].kl*dist + lights[curr_light].kq*dist*dist);    

                                        i = 128*dp / ( atten ); 

                                        r_sum1 += (lights[curr_light].c_diffuse.r * r_base * i) / (256*128);
                                        g_sum1 += (lights[curr_light].c_diffuse.g * g_base * i) / (256*128);
                                        b_sum1 += (lights[curr_light].c_diffuse.b * b_base * i) / (256*128);
                                    } // end i

                                    // vertex 2
                                    dp = VECTOR4D_Dot(&curr_poly->tvlist[2].n, &lights[curr_light].tdir);

                                    // only add light if dp > 0
                                    if (dp > 0)
                                    { 
                                        atten =  (lights[curr_light].kc + lights[curr_light].kl*dist + lights[curr_light].kq*dist*dist);    

                                        i = 128*dp / ( atten ); 

                                        r_sum2 += (lights[curr_light].c_diffuse.r * r_base * i) / (256*128);
                                        g_sum2 += (lights[curr_light].c_diffuse.g * g_base * i) / (256*128);
                                        b_sum2 += (lights[curr_light].c_diffuse.b * b_base * i) / (256*128);
                                    } // end i

                                    //Write_Error("\nexiting spotlight1, sums rgb0[%d, %d, %d], rgb1[%d,%d,%d], rgb2[%d,%d,%d]", r_sum0, g_sum0, b_sum0,  r_sum1, g_sum1, b_sum1,  r_sum2, g_sum2, b_sum2);

                                } // end if spotlight1
                                else
                                    if (lights[curr_light].attr & LIGHTV2_ATTR_SPOTLIGHT2) // simple version //////////////////////////
                                    {
                                        // perform spot light computations
                                        // light model for spot light simple version is once again:
                                        //         	     I0spotlight * Clspotlight * MAX( (l . s), 0)^pf                     
                                        // I(d)spotlight = __________________________________________      
                                        //               		 kc + kl*d + kq*d2        
                                        // Where d = |p - s|, and pf = power factor

                                        // thus it's almost identical to the point, but has the extra term in the numerator
                                        // relating the angle between the light source and the point on the surface

                                        // .. already have normals and length are 1.0

                                        // and for the diffuse model
                                        // Itotald =   Rsdiffuse*Idiffuse * (n . l)
                                        // so we basically need to multiple it all together
                                        // notice the scaling by 128, I want to avoid floating point calculations, not because they 
                                        // are slower, but the conversion to and from cost cycles

                                        //Write_Error("\nEntering spotlight2...");

                                        // tons of redundant math here! lots to optimize later!

                                        // vertex 0
                                        dp = VECTOR4D_Dot(&curr_poly->tvlist[0].n, &lights[curr_light].tdir);

                                        // only add light if dp > 0
                                        if (dp > 0)
                                        { 
                                            // compute vector from light to surface (different from l which IS the light dir)
                                            VECTOR4D_Build( &lights[curr_light].tpos, &curr_poly->tvlist[0].v, &s);

                                            // compute length of s (distance to light source) to normalize s for lighting calc
                                            dists = VECTOR4D_Length_Fast2(&s);  

                                            // compute spot light term (s . l)
                                            float dpsl = VECTOR4D_Dot(&s, &lights[curr_light].tdir)/dists;

                                            // proceed only if term is positive
                                            if (dpsl > 0) 
                                            {
                                                // compute attenuation
                                                atten = (lights[curr_light].kc + lights[curr_light].kl*dists + lights[curr_light].kq*dists*dists);    

                                                // for speed reasons, pf exponents that are less that 1.0 are out of the question, and exponents
                                                // must be integral
                                                float dpsl_exp = dpsl;

                                                // exponentiate for positive integral powers
                                                for (int e_index = 1; e_index < (int)lights[curr_light].pf; e_index++)
                                                    dpsl_exp*=dpsl;

                                                // now dpsl_exp holds (dpsl)^pf power which is of course (s . l)^pf 

                                                i = 128*dp * dpsl_exp / ( atten ); 

                                                r_sum0 += (lights[curr_light].c_diffuse.r * r_base * i) / (256*128);
                                                g_sum0 += (lights[curr_light].c_diffuse.g * g_base * i) / (256*128);
                                                b_sum0 += (lights[curr_light].c_diffuse.b * b_base * i) / (256*128);

                                            } // end if

                                        } // end if

                                        // vertex 1
                                        dp = VECTOR4D_Dot(&curr_poly->tvlist[1].n, &lights[curr_light].tdir);

                                        // only add light if dp > 0
                                        if (dp > 0)
                                        { 
                                            // compute vector from light to surface (different from l which IS the light dir)
                                            VECTOR4D_Build( &lights[curr_light].tpos, &curr_poly->tvlist[1].v, &s);

                                            // compute length of s (distance to light source) to normalize s for lighting calc
                                            dists = VECTOR4D_Length_Fast2(&s);  

                                            // compute spot light term (s . l)
                                            float dpsl = VECTOR4D_Dot(&s, &lights[curr_light].tdir)/dists;

                                            // proceed only if term is positive
                                            if (dpsl > 0) 
                                            {
                                                // compute attenuation
                                                atten = (lights[curr_light].kc + lights[curr_light].kl*dists + lights[curr_light].kq*dists*dists);    

                                                // for speed reasons, pf exponents that are less that 1.0 are out of the question, and exponents
                                                // must be integral
                                                float dpsl_exp = dpsl;

                                                // exponentiate for positive integral powers
                                                for (int e_index = 1; e_index < (int)lights[curr_light].pf; e_index++)
                                                    dpsl_exp*=dpsl;

                                                // now dpsl_exp holds (dpsl)^pf power which is of course (s . l)^pf 

                                                i = 128*dp * dpsl_exp / ( atten ); 

                                                r_sum1 += (lights[curr_light].c_diffuse.r * r_base * i) / (256*128);
                                                g_sum1 += (lights[curr_light].c_diffuse.g * g_base * i) / (256*128);
                                                b_sum1 += (lights[curr_light].c_diffuse.b * b_base * i) / (256*128);

                                            } // end if

                                        } // end if

                                        // vertex 2
                                        dp = VECTOR4D_Dot(&curr_poly->tvlist[2].n, &lights[curr_light].tdir);

                                        // only add light if dp > 0
                                        if (dp > 0)
                                        { 
                                            // compute vector from light to surface (different from l which IS the light dir)
                                            VECTOR4D_Build( &lights[curr_light].tpos, &curr_poly->tvlist[2].v, &s);

                                            // compute length of s (distance to light source) to normalize s for lighting calc
                                            dists = VECTOR4D_Length_Fast2(&s);  

                                            // compute spot light term (s . l)
                                            float dpsl = VECTOR4D_Dot(&s, &lights[curr_light].tdir)/dists;

                                            // proceed only if term is positive
                                            if (dpsl > 0) 
                                            {
                                                // compute attenuation
                                                atten = (lights[curr_light].kc + lights[curr_light].kl*dists + lights[curr_light].kq*dists*dists);    

                                                // for speed reasons, pf exponents that are less that 1.0 are out of the question, and exponents
                                                // must be integral
                                                float dpsl_exp = dpsl;

                                                // exponentiate for positive integral powers
                                                for (int e_index = 1; e_index < (int)lights[curr_light].pf; e_index++)
                                                    dpsl_exp*=dpsl;

                                                // now dpsl_exp holds (dpsl)^pf power which is of course (s . l)^pf 

                                                i = 128*dp * dpsl_exp / ( atten ); 

                                                r_sum2 += (lights[curr_light].c_diffuse.r * r_base * i) / (256*128);
                                                g_sum2 += (lights[curr_light].c_diffuse.g * g_base * i) / (256*128);
                                                b_sum2 += (lights[curr_light].c_diffuse.b * b_base * i) / (256*128);
                                            } // end if

                                        } // end if

                                        //Write_Error("\nexiting spotlight2, sums rgb0[%d, %d, %d], rgb1[%d,%d,%d], rgb2[%d,%d,%d]", r_sum0, g_sum0, b_sum0,   r_sum1, g_sum1, b_sum1,  r_sum2, g_sum2, b_sum2);

                                    } // end if spot light

                } // end for light

                // make sure colors aren't out of range
                if (r_sum0  > 255) r_sum0 = 255;
                if (g_sum0  > 255) g_sum0 = 255;
                if (b_sum0  > 255) b_sum0 = 255;

                if (r_sum1  > 255) r_sum1 = 255;
                if (g_sum1  > 255) g_sum1 = 255;
                if (b_sum1  > 255) b_sum1 = 255;

                if (r_sum2  > 255) r_sum2 = 255;
                if (g_sum2  > 255) g_sum2 = 255;
                if (b_sum2  > 255) b_sum2 = 255;

                //Write_Error("\nwriting color for poly %d", poly);

                //Write_Error("\n******** final sums rgb0[%d, %d, %d], rgb1[%d,%d,%d], rgb2[%d,%d,%d]", r_sum0, g_sum0, b_sum0,  r_sum1, g_sum1, b_sum1, r_sum2, g_sum2, b_sum2);

                // write all colors in RGB form since we need to interpolate at the rasterizer
                curr_poly->lit_color[0] = RGB16Bit(r_sum0, g_sum0, b_sum0);
                curr_poly->lit_color[1] = RGB16Bit(r_sum1, g_sum1, b_sum1);
                curr_poly->lit_color[2] = RGB16Bit(r_sum2, g_sum2, b_sum2);

            } // end if
            else // assume POLY4DV2_ATTR_SHADE_MODE_CONSTANT
            {
                // emmisive shading only, do nothing
                // ...
                curr_poly->lit_color[0] = curr_poly->color;

                //Write_Error("\nentering constant shader, and exiting...");

            } // end if

    } // end for poly

    // return success
    return(1);

} // end Light_RENDERLIST4DV2_World2

//////////////////////////////////////////////////////////////////////////////////////

void Transform_LIGHTSV2(LIGHTV2_PTR lights,  // array of lights to transform
                        int num_lights,      // number of lights to transform
                        MATRIX4X4_PTR mt,    // transformation matrix
                        int coord_select)    // selects coords to transform

{
    // this function simply transforms all of the lights by the transformation matrix
    // this function is used to place the lights in camera space for example, so that
    // lighting calculations are correct if the lighting function is called AFTER
    // the polygons have already been trasformed to camera coordinates
    // also, later we may decided to optimize this a little by determining
    // the light type and only rotating what is needed, however there are thousands
    // of vertices in a scene and not rotating 10 more points isn't going to make 
    // a difference
    // NOTE: This function MUST be called even if a transform to the lights 
    // is not desired, that is, you are lighting in world coords, in this case
    // the local light positions and orientations MUST still be copied into the 
    // working variables for the lighting engine to use pos->tpos, dir->tdir
    // hence, call this function with TRANSFORM_COPY_LOCAL_TO_TRANS 
    // with a matrix of NULL

    int curr_light; // current light in loop
    MATRIX4X4 mr;   // used to build up rotation aspect of matrix only

    // we need to rotate the direction vectors of the lights also, but
    // the translation factors must be zeroed out in the matrix otherwise
    // the results will be incorrect, thus make a copy of the matrix and zero
    // the translation factors

    if (mt!=NULL)
    {
        MAT_COPY_4X4(mt, &mr);
        // zero translation factors
        mr.M30 = mr.M31 = mr.M32 = 0;
    } // end if

    // what coordinates should be transformed?
    switch(coord_select)
    {
    case TRANSFORM_COPY_LOCAL_TO_TRANS:
        {
            // loop thru all the lights
            for (curr_light = 0; curr_light < num_lights; curr_light++)
            {  
                lights[curr_light].tpos = lights[curr_light].pos;
                lights[curr_light].tdir = lights[curr_light].dir;
            } // end for

        } break;

    case TRANSFORM_LOCAL_ONLY:
        {
            // loop thru all the lights
            for (curr_light = 0; curr_light < num_lights; curr_light++)
            {   
                // transform the local/world light coordinates in place
                POINT4D presult; // hold result of each transformation

                // transform point position of each light
                Mat_Mul_VECTOR4D_4X4(&lights[curr_light].pos, mt, &presult);

                // store result back
                VECTOR4D_COPY(&lights[curr_light].pos, &presult); 

                // transform direction vector 
                Mat_Mul_VECTOR4D_4X4(&lights[curr_light].dir, &mr, &presult);

                // store result back
                VECTOR4D_COPY(&lights[curr_light].dir, &presult); 
            } // end for

        } break;

    case TRANSFORM_TRANS_ONLY:
        {
            // loop thru all the lights
            for (curr_light = 0; curr_light < num_lights; curr_light++)
            { 
                // transform each "transformed" light
                POINT4D presult; // hold result of each transformation

                // transform point position of each light
                Mat_Mul_VECTOR4D_4X4(&lights[curr_light].tpos, mt, &presult);

                // store result back
                VECTOR4D_COPY(&lights[curr_light].tpos, &presult); 

                // transform direction vector 
                Mat_Mul_VECTOR4D_4X4(&lights[curr_light].tdir, &mr, &presult);

                // store result back
                VECTOR4D_COPY(&lights[curr_light].tdir, &presult); 
            } // end for

        } break;

    case TRANSFORM_LOCAL_TO_TRANS:
        {
            // loop thru all the lights
            for (curr_light = 0; curr_light < num_lights; curr_light++)
            { 
                // transform each local/world light and place the results into the transformed
                // storage, this is the usual way the function will be called
                POINT4D presult; // hold result of each transformation

                // transform point position of each light
                Mat_Mul_VECTOR4D_4X4(&lights[curr_light].pos, mt, &lights[curr_light].tpos);

                // transform direction vector 
                Mat_Mul_VECTOR4D_4X4(&lights[curr_light].dir, &mr, &lights[curr_light].tdir);
            } // end for

        } break;

    default: break;

    } // end switch

} // end Transform_LIGHTSV2


