// T3DLIB12.CPP - rasterizers that support textures with transparent pixels

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



// DEFINES //////////////////////////////////////////////////////////////////



// GLOBALS //////////////////////////////////////////////////////////////////



// FUNCTIONS ////////////////////////////////////////////////////////////////

void Draw_RENDERLIST4DV2_RENDERCONTEXTV1_16_3(RENDERCONTEXTV1_PTR rc)
{
    // this function renders the rendering list, it's based on the new
    // rendering context data structure which is container for everything
    // we need to consider when rendering, z, 1/z buffering, alpha, mipmapping,
    // perspective, bilerp, etc. the function is basically a merge of all the functions
    // we have written thus far, so its rather long, but better than having 
    // 20-30 rendering functions for all possible permutations!
    // this new version _3 supports the new RENDER_ATTR_WRITETHRUZBUFFER 
    // functionality in a very limited manner, it supports mip mapping in general
    // and only supports the following types:
    // constant shaded, flat shaded, gouraud shaded 
    // affine only: constant textured, flat textured, and gouraud textured
    // when not using the RENDER_ATTR_WRITETHRUZBUFFER support, the 
    // function is identical to the previous version
    // additionally, this function supports alpha blended constant textures
    // and any texture that is constant shaded with alpha with have support for 
    // transparent pixels, that is any pixel that has RGB value (0,0,0) in the
    // texture will not be rendered, the transparent rasterizers with alpha support
    // are for the constant shader, and textured shader, with nozbuffer, zbuffer, and 
    // zbuffer writethru

    POLYF4DV2 face; // temp face used to render polygon
    int alpha;      // alpha of the face

    // we need to try and separate as much conditional logic as possible
    // at the beginning of the function, so we can minimize it inline during
    // the traversal of the polygon list, let's start by subclassing which
    // kind of rendering we are doing none, z buffered, or 1/z buffered

    if (rc->attr & RENDER_ATTR_NOBUFFER) ////////////////////////////////////
    {
        // no buffering at all

        // at this point, all we have is a list of polygons and it's time
        // to draw them
        for (int poly=0; poly < rc->rend_list->num_polys; poly++)
        {
            // render this polygon if and only if it's not clipped, not culled,
            // active, and visible, note however the concecpt of "backface" is 
            // irrelevant in a wire frame engine though
            if (!(rc->rend_list->poly_ptrs[poly]->state & POLY4DV2_STATE_ACTIVE) ||
                (rc->rend_list->poly_ptrs[poly]->state & POLY4DV2_STATE_CLIPPED ) ||
                (rc->rend_list->poly_ptrs[poly]->state & POLY4DV2_STATE_BACKFACE) )
                continue; // move onto next poly

            // test for alpha override
            if (rc->alpha_override>= 0)
            {
                // set alpha to override value
                alpha = rc->alpha_override;
            }  // end if 
            else
            {
                // extract alpha (even if there isn't any)
                alpha = ((rc->rend_list->poly_ptrs[poly]->color & 0xff000000) >> 24);
            } // end else


            // need to test for textured first, since a textured poly can either
            // be emissive, or flat shaded, hence we need to call different
            // rasterizers    
            if (rc->rend_list->poly_ptrs[poly]->attr & POLY4DV2_ATTR_SHADE_MODE_TEXTURE)
            {
                // set the vertices
                face.tvlist[0].x = (float)rc->rend_list->poly_ptrs[poly]->tvlist[0].x;
                face.tvlist[0].y = (float)rc->rend_list->poly_ptrs[poly]->tvlist[0].y;
                face.tvlist[0].z  = (float)rc->rend_list->poly_ptrs[poly]->tvlist[0].z;
                face.tvlist[0].u0 = (float)rc->rend_list->poly_ptrs[poly]->tvlist[0].u0;
                face.tvlist[0].v0 = (float)rc->rend_list->poly_ptrs[poly]->tvlist[0].v0;

                face.tvlist[1].x = (float)rc->rend_list->poly_ptrs[poly]->tvlist[1].x;
                face.tvlist[1].y = (float)rc->rend_list->poly_ptrs[poly]->tvlist[1].y;
                face.tvlist[1].z  = (float)rc->rend_list->poly_ptrs[poly]->tvlist[1].z;
                face.tvlist[1].u0 = (float)rc->rend_list->poly_ptrs[poly]->tvlist[1].u0;
                face.tvlist[1].v0 = (float)rc->rend_list->poly_ptrs[poly]->tvlist[1].v0;

                face.tvlist[2].x = (float)rc->rend_list->poly_ptrs[poly]->tvlist[2].x;
                face.tvlist[2].y = (float)rc->rend_list->poly_ptrs[poly]->tvlist[2].y;
                face.tvlist[2].z  = (float)rc->rend_list->poly_ptrs[poly]->tvlist[2].z;
                face.tvlist[2].u0 = (float)rc->rend_list->poly_ptrs[poly]->tvlist[2].u0;
                face.tvlist[2].v0 = (float)rc->rend_list->poly_ptrs[poly]->tvlist[2].v0;

                // test if this is a mipmapped polygon?
                if (rc->rend_list->poly_ptrs[poly]->attr & POLY4DV2_ATTR_MIPMAP)
                {
                    // determine if mipmapping is desired at all globally
                    if (rc->attr & RENDER_ATTR_MIPMAP)
                    {
                        // determine mip level for this polygon

                        // first determine how many miplevels there are in mipchain for this polygon
                        int tmiplevels = logbase2ofx[((BITMAP_IMAGE_PTR *)(rc->rend_list->poly_ptrs[poly]->texture))[0]->width];

                        // now based on the requested linear miplevel fall off distance, cut
                        // the viewdistance into segments, determine what segment polygon is
                        // in and select mip level -- simple! later you might want something more
                        // robust, also note I only use a single vertex, you might want to find the average
                        // since for long walls perpendicular to view direction this might causing mip
                        // popping mid surface
                        int miplevel = (tmiplevels * rc->rend_list->poly_ptrs[poly]->tvlist[0].z / rc->mip_dist);

                        // clamp miplevel
                        if (miplevel > tmiplevels) miplevel = tmiplevels;

                        // based on miplevel select proper texture
                        face.texture = ((BITMAP_IMAGE_PTR *)(rc->rend_list->poly_ptrs[poly]->texture))[miplevel];

                        // now we must divide each texture coordinate by 2 per miplevel
                        for (int ts = 0; ts < miplevel; ts++)
                        {
                            face.tvlist[0].u0*=.5;
                            face.tvlist[0].v0*=.5;

                            face.tvlist[1].u0*=.5;
                            face.tvlist[1].v0*=.5;

                            face.tvlist[2].u0*=.5;
                            face.tvlist[2].v0*=.5;
                        } // end for

                    } // end if mipmmaping enabled globally
                    else // mipmapping not selected globally
                    {
                        // in this case the polygon IS mipmapped, but the caller has requested NO
                        // mipmapping, so we will support this by selecting mip level 0 since the
                        // texture pointer is pointing to a mip chain regardless
                        face.texture = ((BITMAP_IMAGE_PTR *)(rc->rend_list->poly_ptrs[poly]->texture))[0];

                        // note: texture coordinate manipulation is unneeded

                    } // end else

                } // end if
                else
                {
                    // assign the texture without change
                    face.texture = rc->rend_list->poly_ptrs[poly]->texture;
                } // end if

                // is this a plain emissive texture?
                if (rc->rend_list->poly_ptrs[poly]->attr & POLY4DV2_ATTR_SHADE_MODE_CONSTANT)
                {
                    // draw the textured triangle as emissive

                    if ((rc->attr & RENDER_ATTR_ALPHA) &&
                        ((rc->rend_list->poly_ptrs[poly]->attr & POLY4DV2_ATTR_TRANSPARENT) || rc->alpha_override>=0) )
                    {
                        // alpha version

                        // which texture mapper?
                        if (rc->attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_AFFINE)
                        {
                            // redo with transparency
                            Draw_Textured_Triangle_Alpha16_2(&face, rc->video_buffer, rc->lpitch, alpha);
                        } // end if
                        else
                            if (rc->attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_CORRECT)
                            {
                                // not supported yet!
                                // redo with transparency
                                Draw_Textured_Triangle_Alpha16_2(&face, rc->video_buffer, rc->lpitch, alpha);
                            } // end if
                            else
                                if (rc->attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_LINEAR)
                                {
                                    // not supported yet
                                    // redo with transparency
                                    Draw_Textured_Triangle_Alpha16_2(&face, rc->video_buffer, rc->lpitch, alpha);
                                } // end if
                                else
                                    if (rc->attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_HYBRID1)
                                    {
                                        // test z distance again perspective transition gate
                                        if (rc->rend_list->poly_ptrs[poly]->tvlist[0].z > rc-> texture_dist)
                                        {
                                            // default back to affine
                                            // redo with transparency
                                            Draw_Textured_Triangle_Alpha16_2(&face, rc->video_buffer, rc->lpitch, alpha);
                                        } // end if
                                        else
                                        {
                                            // use perspective linear
                                            // not supported yet
                                            // redo with transparency
                                            Draw_Textured_Triangle_Alpha16_2(&face, rc->video_buffer, rc->lpitch, alpha);
                                        } // end if

                                    } // end if

                    } // end if
                    else
                    {
                        // non alpha
                        // which texture mapper?
                        if (rc->attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_AFFINE)
                        {
                            // use bilerp?
                            if (rc->attr & RENDER_ATTR_BILERP)
                                Draw_Textured_Bilerp_Triangle_16(&face, rc->video_buffer, rc->lpitch);               
                            else
                                Draw_Textured_Triangle2_16(&face, rc->video_buffer, rc->lpitch);
                        } // end if
                        else
                            if (rc->attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_CORRECT)
                            {
                                // not supported yet
                                Draw_Textured_Triangle2_16(&face, rc->video_buffer, rc->lpitch);
                            } // end if
                            else
                                if (rc->attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_LINEAR)
                                {
                                    // not supported yet
                                    Draw_Textured_Triangle2_16(&face, rc->video_buffer, rc->lpitch);
                                } // end if
                                else
                                    if (rc->attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_HYBRID1)
                                    {
                                        // test z distance again perspective transition gate
                                        if (rc->rend_list->poly_ptrs[poly]->tvlist[0].z > rc-> texture_dist)
                                        {
                                            // default back to affine
                                            Draw_Textured_Triangle2_16(&face, rc->video_buffer, rc->lpitch);
                                        } // end if
                                        else
                                        {
                                            // use perspective linear
                                            // not supported yet
                                            Draw_Textured_Triangle2_16(&face, rc->video_buffer, rc->lpitch);
                                        } // end if

                                    } // end if

                    } // end if

                } // end if
                else
                    if (rc->rend_list->poly_ptrs[poly]->attr & POLY4DV2_ATTR_SHADE_MODE_FLAT)
                    {
                        // draw as flat shaded
                        face.lit_color[0] = rc->rend_list->poly_ptrs[poly]->lit_color[0];

                        if ((rc->attr & RENDER_ATTR_ALPHA) &&
                            ((rc->rend_list->poly_ptrs[poly]->attr & POLY4DV2_ATTR_TRANSPARENT) || rc->alpha_override>=0) )
                        {
                            // alpha version

                            // which texture mapper?
                            if (rc->attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_AFFINE)
                            {
                                Draw_Textured_TriangleFS_Alpha16(&face, rc->video_buffer, rc->lpitch, alpha);
                            } // end if
                            else
                                if (rc->attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_CORRECT)
                                {
                                    // not supported yet!
                                    Draw_Textured_TriangleFS_Alpha16(&face, rc->video_buffer, rc->lpitch, alpha);
                                } // end if
                                else
                                    if (rc->attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_LINEAR)
                                    {
                                        // not supported yet
                                        Draw_Textured_TriangleFS_Alpha16(&face, rc->video_buffer, rc->lpitch, alpha);
                                    } // end if
                                    else
                                        if (rc->attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_HYBRID1)
                                        {
                                            // test z distance again perspective transition gate
                                            if (rc->rend_list->poly_ptrs[poly]->tvlist[0].z > rc-> texture_dist)
                                            {
                                                // default back to affine
                                                Draw_Textured_TriangleFS_Alpha16(&face, rc->video_buffer, rc->lpitch, alpha);
                                            } // end if
                                            else
                                            {
                                                // use perspective linear
                                                // not supported yet
                                                Draw_Textured_TriangleFS_Alpha16(&face, rc->video_buffer, rc->lpitch, alpha);
                                            } // end if

                                        } // end if

                        } // end if
                        else
                        {
                            // non alpha
                            // which texture mapper?
                            if (rc->attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_AFFINE)
                            {
                                Draw_Textured_TriangleFS2_16(&face, rc->video_buffer, rc->lpitch);
                            } // end if
                            else
                                if (rc->attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_CORRECT)
                                {
                                    // not supported yet
                                    Draw_Textured_TriangleFS2_16(&face, rc->video_buffer, rc->lpitch);
                                } // end if
                                else
                                    if (rc->attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_LINEAR)
                                    {
                                        // not supported yet
                                        Draw_Textured_TriangleFS2_16(&face, rc->video_buffer, rc->lpitch);
                                    } // end if
                                    else
                                        if (rc->attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_HYBRID1)
                                        {
                                            // test z distance again perspective transition gate
                                            if (rc->rend_list->poly_ptrs[poly]->tvlist[0].z > rc-> texture_dist)
                                            {
                                                // default back to affine
                                                Draw_Textured_TriangleFS2_16(&face, rc->video_buffer, rc->lpitch);
                                            } // end if
                                            else
                                            {
                                                // use perspective linear
                                                // not supported yet
                                                Draw_Textured_TriangleFS2_16(&face, rc->video_buffer, rc->lpitch);
                                            } // end if

                                        } // end if

                        } // end if

                    } // end else
                    else
                    {
                        // must be gouraud POLY4DV2_ATTR_SHADE_MODE_GOURAUD
                        face.lit_color[0] = rc->rend_list->poly_ptrs[poly]->lit_color[0];
                        face.lit_color[1] = rc->rend_list->poly_ptrs[poly]->lit_color[1];
                        face.lit_color[2] = rc->rend_list->poly_ptrs[poly]->lit_color[2];

                        if ((rc->attr & RENDER_ATTR_ALPHA) &&
                            ((rc->rend_list->poly_ptrs[poly]->attr & POLY4DV2_ATTR_TRANSPARENT) || rc->alpha_override>=0) )
                        {
                            // alpha version

                            // which texture mapper?
                            if (rc->attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_AFFINE)
                            {
                                Draw_Textured_TriangleGS_Alpha16(&face, rc->video_buffer, rc->lpitch, alpha);
                            } // end if
                            else
                                if (rc->attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_CORRECT)
                                {
                                    // not supported yet!
                                    Draw_Textured_TriangleGS_Alpha16(&face, rc->video_buffer, rc->lpitch, alpha);
                                } // end if
                                else
                                    if (rc->attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_LINEAR)
                                    {
                                        // not supported yet
                                        Draw_Textured_TriangleGS_Alpha16(&face, rc->video_buffer, rc->lpitch, alpha);
                                    } // end if
                                    else
                                        if (rc->attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_HYBRID1)
                                        {
                                            // test z distance again perspective transition gate
                                            if (rc->rend_list->poly_ptrs[poly]->tvlist[0].z > rc-> texture_dist)
                                            {
                                                // default back to affine
                                                Draw_Textured_TriangleGS_Alpha16(&face, rc->video_buffer, rc->lpitch, alpha);
                                            } // end if
                                            else
                                            {
                                                // use perspective linear
                                                // not supported yet
                                                Draw_Textured_TriangleGS_Alpha16(&face, rc->video_buffer, rc->lpitch, alpha);
                                            } // end if

                                        } // end if

                        } // end if
                        else
                        {
                            // non alpha
                            // which texture mapper?
                            if (rc->attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_AFFINE)
                            {
                                Draw_Textured_TriangleGS_16(&face, rc->video_buffer, rc->lpitch);
                            } // end if
                            else
                                if (rc->attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_CORRECT)
                                {
                                    // not supported yet
                                    Draw_Textured_TriangleGS_16(&face, rc->video_buffer, rc->lpitch);
                                } // end if
                                else
                                    if (rc->attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_LINEAR)
                                    {
                                        // not supported yet
                                        Draw_Textured_TriangleGS_16(&face, rc->video_buffer, rc->lpitch);
                                    } // end if
                                    else
                                        if (rc->attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_HYBRID1)
                                        {
                                            // test z distance again perspective transition gate
                                            if (rc->rend_list->poly_ptrs[poly]->tvlist[0].z > rc-> texture_dist)
                                            {
                                                // default back to affine
                                                Draw_Textured_TriangleGS_16(&face, rc->video_buffer, rc->lpitch);
                                            } // end if
                                            else
                                            {
                                                // use perspective linear
                                                // not supported yet
                                                Draw_Textured_TriangleGS_16(&face, rc->video_buffer, rc->lpitch);
                                            } // end if

                                        } // end if

                        } // end if

                    } // end else

            } // end if      
            else
                if ((rc->rend_list->poly_ptrs[poly]->attr & POLY4DV2_ATTR_SHADE_MODE_FLAT) || 
                    (rc->rend_list->poly_ptrs[poly]->attr & POLY4DV2_ATTR_SHADE_MODE_CONSTANT) )
                {
                    // draw as constant shaded
                    face.lit_color[0] = rc->rend_list->poly_ptrs[poly]->lit_color[0];

                    // set the vertices
                    face.tvlist[0].x = (float)rc->rend_list->poly_ptrs[poly]->tvlist[0].x;
                    face.tvlist[0].y = (float)rc->rend_list->poly_ptrs[poly]->tvlist[0].y;
                    face.tvlist[0].z  = (float)rc->rend_list->poly_ptrs[poly]->tvlist[0].z;

                    face.tvlist[1].x = (float)rc->rend_list->poly_ptrs[poly]->tvlist[1].x;
                    face.tvlist[1].y = (float)rc->rend_list->poly_ptrs[poly]->tvlist[1].y;
                    face.tvlist[1].z  = (float)rc->rend_list->poly_ptrs[poly]->tvlist[1].z;

                    face.tvlist[2].x = (float)rc->rend_list->poly_ptrs[poly]->tvlist[2].x;
                    face.tvlist[2].y = (float)rc->rend_list->poly_ptrs[poly]->tvlist[2].y;
                    face.tvlist[2].z  = (float)rc->rend_list->poly_ptrs[poly]->tvlist[2].z;

                    // draw the triangle with basic flat rasterizer

                    // test for transparent
                    if ((rc->attr & RENDER_ATTR_ALPHA) &&
                        ((rc->rend_list->poly_ptrs[poly]->attr & POLY4DV2_ATTR_TRANSPARENT) || rc->alpha_override>=0) )
                    {
                        // redo with transparency
                        Draw_Triangle_2D_Alpha16(&face, rc->video_buffer, rc->lpitch,alpha);
                    } // end if
                    else
                    {
                        Draw_Triangle_2D3_16(&face, rc->video_buffer, rc->lpitch);
                    } // end if

                } // end if                    
                else
                    if (rc->rend_list->poly_ptrs[poly]->attr & POLY4DV2_ATTR_SHADE_MODE_GOURAUD)
                    {
                        // {andre take advantage of the data structures later..}
                        // set the vertices
                        face.tvlist[0].x  = (float)rc->rend_list->poly_ptrs[poly]->tvlist[0].x;
                        face.tvlist[0].y  = (float)rc->rend_list->poly_ptrs[poly]->tvlist[0].y;
                        face.tvlist[0].z  = (float)rc->rend_list->poly_ptrs[poly]->tvlist[0].z;
                        face.lit_color[0] = rc->rend_list->poly_ptrs[poly]->lit_color[0];

                        face.tvlist[1].x  = (float)rc->rend_list->poly_ptrs[poly]->tvlist[1].x;
                        face.tvlist[1].y  = (float)rc->rend_list->poly_ptrs[poly]->tvlist[1].y;
                        face.tvlist[1].z  = (float)rc->rend_list->poly_ptrs[poly]->tvlist[1].z;
                        face.lit_color[1] = rc->rend_list->poly_ptrs[poly]->lit_color[1];

                        face.tvlist[2].x  = (float)rc->rend_list->poly_ptrs[poly]->tvlist[2].x;
                        face.tvlist[2].y  = (float)rc->rend_list->poly_ptrs[poly]->tvlist[2].y;
                        face.tvlist[2].z  = (float)rc->rend_list->poly_ptrs[poly]->tvlist[2].z;
                        face.lit_color[2] = rc->rend_list->poly_ptrs[poly]->lit_color[2];

                        // draw the gouraud shaded triangle
                        // test for transparent
                        if ((rc->attr & RENDER_ATTR_ALPHA) &&
                            ((rc->rend_list->poly_ptrs[poly]->attr & POLY4DV2_ATTR_TRANSPARENT) || rc->alpha_override>=0) )
                        {
                            Draw_Gouraud_Triangle_Alpha16(&face, rc->video_buffer, rc->lpitch,alpha);
                        } // end if
                        else
                        {
                            Draw_Gouraud_Triangle2_16(&face, rc->video_buffer, rc->lpitch);
                        } // end if

                    } // end if gouraud

        } // end for poly

    } // end if RENDER_ATTR_NOBUFFER

    else
        if (rc->attr & RENDER_ATTR_ZBUFFER) ////////////////////////////////////
        {
            // use the z buffer

            // we have is a list of polygons and it's time draw them
            for (int poly=0; poly < rc->rend_list->num_polys; poly++)
            {
                // render this polygon if and only if it's not clipped, not culled,
                // active, and visible, note however the concecpt of "backface" is 
                // irrelevant in a wire frame engine though
                if (!(rc->rend_list->poly_ptrs[poly]->state & POLY4DV2_STATE_ACTIVE) ||
                    (rc->rend_list->poly_ptrs[poly]->state & POLY4DV2_STATE_CLIPPED ) ||
                    (rc->rend_list->poly_ptrs[poly]->state & POLY4DV2_STATE_BACKFACE) )
                    continue; // move onto next poly

                // test for alpha override
                if (rc->alpha_override>= 0)
                {
                    // set alpha to override value
                    alpha = rc->alpha_override;
                }  // end if 
                else
                {
                    // extract alpha (even if there isn't any)
                    alpha = ((rc->rend_list->poly_ptrs[poly]->color & 0xff000000) >> 24);
                } // end else

                // need to test for textured first, since a textured poly can either
                // be emissive, or flat shaded, hence we need to call different
                // rasterizers    
                if (rc->rend_list->poly_ptrs[poly]->attr & POLY4DV2_ATTR_SHADE_MODE_TEXTURE)
                {
                    // set the vertices
                    face.tvlist[0].x = (float)rc->rend_list->poly_ptrs[poly]->tvlist[0].x;
                    face.tvlist[0].y = (float)rc->rend_list->poly_ptrs[poly]->tvlist[0].y;
                    face.tvlist[0].z  = (float)rc->rend_list->poly_ptrs[poly]->tvlist[0].z;
                    face.tvlist[0].u0 = (float)rc->rend_list->poly_ptrs[poly]->tvlist[0].u0;
                    face.tvlist[0].v0 = (float)rc->rend_list->poly_ptrs[poly]->tvlist[0].v0;

                    face.tvlist[1].x = (float)rc->rend_list->poly_ptrs[poly]->tvlist[1].x;
                    face.tvlist[1].y = (float)rc->rend_list->poly_ptrs[poly]->tvlist[1].y;
                    face.tvlist[1].z  = (float)rc->rend_list->poly_ptrs[poly]->tvlist[1].z;
                    face.tvlist[1].u0 = (float)rc->rend_list->poly_ptrs[poly]->tvlist[1].u0;
                    face.tvlist[1].v0 = (float)rc->rend_list->poly_ptrs[poly]->tvlist[1].v0;

                    face.tvlist[2].x = (float)rc->rend_list->poly_ptrs[poly]->tvlist[2].x;
                    face.tvlist[2].y = (float)rc->rend_list->poly_ptrs[poly]->tvlist[2].y;
                    face.tvlist[2].z  = (float)rc->rend_list->poly_ptrs[poly]->tvlist[2].z;
                    face.tvlist[2].u0 = (float)rc->rend_list->poly_ptrs[poly]->tvlist[2].u0;
                    face.tvlist[2].v0 = (float)rc->rend_list->poly_ptrs[poly]->tvlist[2].v0;

                    // test if this is a mipmapped polygon?
                    if (rc->rend_list->poly_ptrs[poly]->attr & POLY4DV2_ATTR_MIPMAP)
                    {
                        // determine if mipmapping is desired at all globally
                        if (rc->attr & RENDER_ATTR_MIPMAP)
                        {
                            // determine mip level for this polygon

                            // first determine how many miplevels there are in mipchain for this polygon
                            int tmiplevels = logbase2ofx[((BITMAP_IMAGE_PTR *)(rc->rend_list->poly_ptrs[poly]->texture))[0]->width];

                            // now based on the requested linear miplevel fall off distance, cut
                            // the viewdistance into segments, determine what segment polygon is
                            // in and select mip level -- simple! later you might want something more
                            // robust, also note I only use a single vertex, you might want to find the average
                            // since for long walls perpendicular to view direction this might causing mip
                            // popping mid surface
                            int miplevel = (tmiplevels * rc->rend_list->poly_ptrs[poly]->tvlist[0].z / rc->mip_dist);

                            // clamp miplevel
                            if (miplevel > tmiplevels) miplevel = tmiplevels;

                            // based on miplevel select proper texture
                            face.texture = ((BITMAP_IMAGE_PTR *)(rc->rend_list->poly_ptrs[poly]->texture))[miplevel];

                            // now we must divide each texture coordinate by 2 per miplevel
                            for (int ts = 0; ts < miplevel; ts++)
                            {
                                face.tvlist[0].u0*=.5;
                                face.tvlist[0].v0*=.5;

                                face.tvlist[1].u0*=.5;
                                face.tvlist[1].v0*=.5;

                                face.tvlist[2].u0*=.5;
                                face.tvlist[2].v0*=.5;
                            } // end for

                        } // end if mipmmaping enabled globally
                        else // mipmapping not selected globally
                        {
                            // in this case the polygon IS mipmapped, but the caller has requested NO
                            // mipmapping, so we will support this by selecting mip level 0 since the
                            // texture pointer is pointing to a mip chain regardless
                            face.texture = ((BITMAP_IMAGE_PTR *)(rc->rend_list->poly_ptrs[poly]->texture))[0];

                            // note: texture coordinate manipulation is unneeded

                        } // end else

                    } // end if
                    else
                    {
                        // assign the texture without change
                        face.texture = rc->rend_list->poly_ptrs[poly]->texture;
                    } // end if

                    // is this a plain emissive texture?
                    if (rc->rend_list->poly_ptrs[poly]->attr & POLY4DV2_ATTR_SHADE_MODE_CONSTANT)
                    {
                        // draw the textured triangle as emissive
                        if ((rc->attr & RENDER_ATTR_ALPHA) &&
                            ((rc->rend_list->poly_ptrs[poly]->attr & POLY4DV2_ATTR_TRANSPARENT) || rc->alpha_override>=0) )
                        {
                            // alpha version

                            // which texture mapper?
                            if (rc->attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_AFFINE)
                            {
                                // redo with transparency
                                Draw_Textured_TriangleZB_Alpha16_2(&face, rc->video_buffer, rc->lpitch,rc->zbuffer,rc->zpitch, alpha);
                            } // end if
                            else
                                if (rc->attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_CORRECT)
                                {
                                    // not supported yet!
                                    // redo with transparency
                                    Draw_Textured_TriangleZB_Alpha16_2(&face, rc->video_buffer, rc->lpitch,rc->zbuffer,rc->zpitch, alpha);
                                } // end if
                                else
                                    if (rc->attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_LINEAR)
                                    {
                                        // not supported yet
                                        // redo with transparency
                                        Draw_Textured_TriangleZB_Alpha16_2(&face, rc->video_buffer, rc->lpitch,rc->zbuffer,rc->zpitch, alpha);
                                    } // end if
                                    else
                                        if (rc->attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_HYBRID1)
                                        {
                                            // test z distance again perspective transition gate
                                            if (rc->rend_list->poly_ptrs[poly]->tvlist[0].z > rc-> texture_dist)
                                            {
                                                // default back to affine
                                                // redo with transparency
                                                Draw_Textured_TriangleZB_Alpha16_2(&face, rc->video_buffer, rc->lpitch,rc->zbuffer,rc->zpitch, alpha);
                                            } // end if
                                            else
                                            {
                                                // use perspective linear
                                                // not supported yet
                                                // redo with transparency
                                                Draw_Textured_TriangleZB_Alpha16_2(&face, rc->video_buffer, rc->lpitch,rc->zbuffer,rc->zpitch, alpha);
                                            } // end if

                                        } // end if

                        } // end if
                        else
                        {
                            // non alpha
                            // which texture mapper?
                            if (rc->attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_AFFINE)
                            {
                                // use bilerp?
                                if (rc->attr & RENDER_ATTR_BILERP)
                                    Draw_Textured_Bilerp_TriangleZB_16(&face, rc->video_buffer, rc->lpitch,rc->zbuffer,rc->zpitch);               
                                else
                                    Draw_Textured_TriangleZB3_16(&face, rc->video_buffer, rc->lpitch,rc->zbuffer,rc->zpitch);

                            } // end if
                            else
                                if (rc->attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_CORRECT)
                                {
                                    // not supported yet
                                    Draw_Textured_TriangleZB3_16(&face, rc->video_buffer, rc->lpitch,rc->zbuffer,rc->zpitch);
                                } // end if
                                else
                                    if (rc->attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_LINEAR)
                                    {
                                        // not supported yet
                                        Draw_Textured_TriangleZB3_16(&face, rc->video_buffer, rc->lpitch,rc->zbuffer,rc->zpitch);
                                    } // end if
                                    else
                                        if (rc->attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_HYBRID1)
                                        {
                                            // test z distance again perspective transition gate
                                            if (rc->rend_list->poly_ptrs[poly]->tvlist[0].z > rc-> texture_dist)
                                            {
                                                // default back to affine
                                                Draw_Textured_TriangleZB3_16(&face, rc->video_buffer, rc->lpitch,rc->zbuffer,rc->zpitch);
                                            } // end if
                                            else
                                            {
                                                // use perspective linear
                                                // not supported yet
                                                Draw_Textured_TriangleZB3_16(&face, rc->video_buffer, rc->lpitch,rc->zbuffer,rc->zpitch);
                                            } // end if

                                        } // end if

                        } // end if

                    } // end if
                    else
                        if (rc->rend_list->poly_ptrs[poly]->attr & POLY4DV2_ATTR_SHADE_MODE_FLAT)
                        {
                            // draw as flat shaded
                            face.lit_color[0] = rc->rend_list->poly_ptrs[poly]->lit_color[0];

                            // test for transparency
                            if ((rc->attr & RENDER_ATTR_ALPHA) &&
                                ((rc->rend_list->poly_ptrs[poly]->attr & POLY4DV2_ATTR_TRANSPARENT) || rc->alpha_override>=0) )
                            {
                                // alpha version

                                // which texture mapper?
                                if (rc->attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_AFFINE)
                                {
                                    Draw_Textured_TriangleFSZB_Alpha16(&face, rc->video_buffer, rc->lpitch,rc->zbuffer,rc->zpitch, alpha);
                                } // end if
                                else
                                    if (rc->attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_CORRECT)
                                    {
                                        // not supported yet
                                        Draw_Textured_TriangleFSZB_Alpha16(&face, rc->video_buffer, rc->lpitch,rc->zbuffer,rc->zpitch, alpha);
                                    } // end if
                                    else
                                        if (rc->attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_LINEAR)
                                        {
                                            // not supported yet
                                            Draw_Textured_TriangleFSZB_Alpha16(&face, rc->video_buffer, rc->lpitch,rc->zbuffer,rc->zpitch, alpha);
                                        } // end if
                                        else
                                            if (rc->attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_HYBRID1)
                                            {
                                                // test z distance again perspective transition gate
                                                if (rc->rend_list->poly_ptrs[poly]->tvlist[0].z > rc-> texture_dist)
                                                {
                                                    // default back to affine
                                                    Draw_Textured_TriangleFSZB_Alpha16(&face, rc->video_buffer, rc->lpitch,rc->zbuffer,rc->zpitch, alpha);
                                                } // end if
                                                else
                                                {
                                                    // use perspective linear
                                                    // not supported yet
                                                    Draw_Textured_TriangleFSZB_Alpha16(&face, rc->video_buffer, rc->lpitch,rc->zbuffer,rc->zpitch, alpha);
                                                } // end if

                                            } // end if

                            } // end if
                            else
                            {
                                // non alpha
                                // which texture mapper?
                                if (rc->attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_AFFINE)
                                {
                                    Draw_Textured_TriangleFSZB2_16(&face, rc->video_buffer, rc->lpitch,rc->zbuffer,rc->zpitch);
                                } // end if
                                else
                                    if (rc->attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_CORRECT)
                                    {
                                        // not supported yet
                                        Draw_Textured_TriangleFSZB2_16(&face, rc->video_buffer, rc->lpitch,rc->zbuffer,rc->zpitch);
                                    } // end if
                                    else
                                        if (rc->attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_LINEAR)
                                        {
                                            // not supported yet
                                            Draw_Textured_TriangleFSZB2_16(&face, rc->video_buffer, rc->lpitch,rc->zbuffer,rc->zpitch);
                                        } // end if
                                        else
                                            if (rc->attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_HYBRID1)
                                            {
                                                // test z distance again perspective transition gate
                                                if (rc->rend_list->poly_ptrs[poly]->tvlist[0].z > rc-> texture_dist)
                                                {
                                                    // default back to affine
                                                    Draw_Textured_TriangleFSZB2_16(&face, rc->video_buffer, rc->lpitch,rc->zbuffer,rc->zpitch);
                                                } // end if
                                                else
                                                {
                                                    // use perspective linear
                                                    // not supported yet
                                                    Draw_Textured_TriangleFSZB2_16(&face, rc->video_buffer, rc->lpitch,rc->zbuffer,rc->zpitch);
                                                } // end if

                                            } // end if

                            } // end if

                        } // end else if
                        else
                        { // POLY4DV2_ATTR_SHADE_MODE_GOURAUD

                            // must be gouraud POLY4DV2_ATTR_SHADE_MODE_GOURAUD
                            face.lit_color[0] = rc->rend_list->poly_ptrs[poly]->lit_color[0];
                            face.lit_color[1] = rc->rend_list->poly_ptrs[poly]->lit_color[1];
                            face.lit_color[2] = rc->rend_list->poly_ptrs[poly]->lit_color[2];

                            // test for transparency
                            if ((rc->attr & RENDER_ATTR_ALPHA) &&
                                ((rc->rend_list->poly_ptrs[poly]->attr & POLY4DV2_ATTR_TRANSPARENT) || rc->alpha_override>=0) )
                            {
                                // alpha version

                                // which texture mapper?
                                if (rc->attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_AFFINE)
                                {
                                    Draw_Textured_TriangleGSZB_Alpha16(&face, rc->video_buffer, rc->lpitch,rc->zbuffer,rc->zpitch, alpha);
                                } // end if
                                else
                                    if (rc->attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_CORRECT)
                                    {
                                        // not supported yet :)
                                        Draw_Textured_TriangleGSZB_Alpha16(&face, rc->video_buffer, rc->lpitch,rc->zbuffer,rc->zpitch, alpha);
                                    } // end if
                                    else
                                        if (rc->attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_LINEAR)
                                        {
                                            // not supported yet :)
                                            Draw_Textured_TriangleGSZB_Alpha16(&face, rc->video_buffer, rc->lpitch,rc->zbuffer,rc->zpitch, alpha);
                                        } // end if
                                        else
                                            if (rc->attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_HYBRID1)
                                            {
                                                // test z distance again perspective transition gate
                                                if (rc->rend_list->poly_ptrs[poly]->tvlist[0].z > rc-> texture_dist)
                                                {
                                                    // default back to affine
                                                    Draw_Textured_TriangleGSZB_Alpha16(&face, rc->video_buffer, rc->lpitch,rc->zbuffer,rc->zpitch, alpha);
                                                } // end if
                                                else
                                                {
                                                    // use perspective linear
                                                    // not supported yet :)
                                                    Draw_Textured_TriangleGSZB_Alpha16(&face, rc->video_buffer, rc->lpitch,rc->zbuffer,rc->zpitch, alpha);
                                                } // end if

                                            } // end if

                            } // end if
                            else
                            {
                                // non alpha
                                // which texture mapper?
                                if (rc->attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_AFFINE)
                                {
                                    Draw_Textured_TriangleGSZB_16(&face, rc->video_buffer, rc->lpitch,rc->zbuffer,rc->zpitch);
                                } // end if
                                else
                                    if (rc->attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_CORRECT)
                                    {
                                        // not supported yet :)
                                        Draw_Textured_TriangleGSZB_16(&face, rc->video_buffer, rc->lpitch,rc->zbuffer,rc->zpitch);
                                    } // end if
                                    else
                                        if (rc->attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_LINEAR)
                                        {
                                            // not supported yet :)
                                            Draw_Textured_TriangleGSZB_16(&face, rc->video_buffer, rc->lpitch,rc->zbuffer,rc->zpitch);
                                        } // end if
                                        else
                                            if (rc->attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_HYBRID1)
                                            {
                                                // test z distance again perspective transition gate
                                                if (rc->rend_list->poly_ptrs[poly]->tvlist[0].z > rc-> texture_dist)
                                                {
                                                    // default back to affine
                                                    Draw_Textured_TriangleGSZB_16(&face, rc->video_buffer, rc->lpitch,rc->zbuffer,rc->zpitch);
                                                } // end if
                                                else
                                                {
                                                    // use perspective linear
                                                    // not supported yet :)
                                                    Draw_Textured_TriangleGSZB_16(&face, rc->video_buffer, rc->lpitch,rc->zbuffer,rc->zpitch);
                                                } // end if

                                            } // end if

                            } // end if

                        } // end else

                } // end if      
                else
                    if ((rc->rend_list->poly_ptrs[poly]->attr & POLY4DV2_ATTR_SHADE_MODE_FLAT) || 
                        (rc->rend_list->poly_ptrs[poly]->attr & POLY4DV2_ATTR_SHADE_MODE_CONSTANT) )
                    {
                        // draw as constant shaded
                        face.lit_color[0] = rc->rend_list->poly_ptrs[poly]->lit_color[0];

                        // set the vertices
                        face.tvlist[0].x = (float)rc->rend_list->poly_ptrs[poly]->tvlist[0].x;
                        face.tvlist[0].y = (float)rc->rend_list->poly_ptrs[poly]->tvlist[0].y;
                        face.tvlist[0].z = (float)rc->rend_list->poly_ptrs[poly]->tvlist[0].z;

                        face.tvlist[1].x = (float)rc->rend_list->poly_ptrs[poly]->tvlist[1].x;
                        face.tvlist[1].y = (float)rc->rend_list->poly_ptrs[poly]->tvlist[1].y;
                        face.tvlist[1].z = (float)rc->rend_list->poly_ptrs[poly]->tvlist[1].z;

                        face.tvlist[2].x = (float)rc->rend_list->poly_ptrs[poly]->tvlist[2].x;
                        face.tvlist[2].y = (float)rc->rend_list->poly_ptrs[poly]->tvlist[2].y;
                        face.tvlist[2].z = (float)rc->rend_list->poly_ptrs[poly]->tvlist[2].z;

                        // draw the triangle with basic flat rasterizer
                        // test for transparency
                        if ((rc->attr & RENDER_ATTR_ALPHA) &&
                            ((rc->rend_list->poly_ptrs[poly]->attr & POLY4DV2_ATTR_TRANSPARENT) || rc->alpha_override>=0) )
                        {
                            // alpha version
                            // redo with transparency
                            Draw_Triangle_2DZB_Alpha16(&face, rc->video_buffer, rc->lpitch,rc->zbuffer,rc->zpitch,alpha);
                        } // end if
                        else
                        {
                            // non alpha version
                            Draw_Triangle_2DZB_16(&face, rc->video_buffer, rc->lpitch,rc->zbuffer,rc->zpitch);
                        }  // end if

                    } // end if
                    else
                        if (rc->rend_list->poly_ptrs[poly]->attr & POLY4DV2_ATTR_SHADE_MODE_GOURAUD)
                        {
                            // {andre take advantage of the data structures later..}
                            // set the vertices
                            face.tvlist[0].x  = (float)rc->rend_list->poly_ptrs[poly]->tvlist[0].x;
                            face.tvlist[0].y  = (float)rc->rend_list->poly_ptrs[poly]->tvlist[0].y;
                            face.tvlist[0].z  = (float)rc->rend_list->poly_ptrs[poly]->tvlist[0].z;
                            face.lit_color[0] = rc->rend_list->poly_ptrs[poly]->lit_color[0];

                            face.tvlist[1].x  = (float)rc->rend_list->poly_ptrs[poly]->tvlist[1].x;
                            face.tvlist[1].y  = (float)rc->rend_list->poly_ptrs[poly]->tvlist[1].y;
                            face.tvlist[1].z  = (float)rc->rend_list->poly_ptrs[poly]->tvlist[1].z;
                            face.lit_color[1] = rc->rend_list->poly_ptrs[poly]->lit_color[1];

                            face.tvlist[2].x  = (float)rc->rend_list->poly_ptrs[poly]->tvlist[2].x;
                            face.tvlist[2].y  = (float)rc->rend_list->poly_ptrs[poly]->tvlist[2].y;
                            face.tvlist[2].z  = (float)rc->rend_list->poly_ptrs[poly]->tvlist[2].z;
                            face.lit_color[2] = rc->rend_list->poly_ptrs[poly]->lit_color[2];

                            // draw the gouraud shaded triangle
                            // test for transparency
                            if ((rc->attr & RENDER_ATTR_ALPHA) &&
                                ((rc->rend_list->poly_ptrs[poly]->attr & POLY4DV2_ATTR_TRANSPARENT) || rc->alpha_override>=0) )
                            {
                                // alpha version
                                Draw_Gouraud_TriangleZB_Alpha16(&face, rc->video_buffer, rc->lpitch,rc->zbuffer,rc->zpitch,alpha);
                            } // end if
                            else
                            { 
                                // non alpha
                                Draw_Gouraud_TriangleZB2_16(&face, rc->video_buffer, rc->lpitch,rc->zbuffer,rc->zpitch);
                            } // end if

                        } // end if gouraud

            } // end for poly

        } // end if RENDER_ATTR_ZBUFFER
        else
            if (rc->attr & RENDER_ATTR_INVZBUFFER) ////////////////////////////////////
            {
                // use the inverse z buffer

                // we have is a list of polygons and it's time draw them
                for (int poly=0; poly < rc->rend_list->num_polys; poly++)
                {
                    // render this polygon if and only if it's not clipped, not culled,
                    // active, and visible, note however the concecpt of "backface" is 
                    // irrelevant in a wire frame engine though
                    if (!(rc->rend_list->poly_ptrs[poly]->state & POLY4DV2_STATE_ACTIVE) ||
                        (rc->rend_list->poly_ptrs[poly]->state & POLY4DV2_STATE_CLIPPED ) ||
                        (rc->rend_list->poly_ptrs[poly]->state & POLY4DV2_STATE_BACKFACE) )
                        continue; // move onto next poly

                    // test for alpha override
                    if (rc->alpha_override>= 0)
                    {
                        // set alpha to override value
                        alpha = rc->alpha_override;
                    }  // end if 
                    else
                    {
                        // extract alpha (even if there isn't any)
                        alpha = ((rc->rend_list->poly_ptrs[poly]->color & 0xff000000) >> 24);
                    } // end else

                    // need to test for textured first, since a textured poly can either
                    // be emissive, or flat shaded, hence we need to call different
                    // rasterizers    
                    if (rc->rend_list->poly_ptrs[poly]->attr & POLY4DV2_ATTR_SHADE_MODE_TEXTURE)
                    {
                        // set the vertices
                        face.tvlist[0].x = (float)rc->rend_list->poly_ptrs[poly]->tvlist[0].x;
                        face.tvlist[0].y = (float)rc->rend_list->poly_ptrs[poly]->tvlist[0].y;
                        face.tvlist[0].z  = (float)rc->rend_list->poly_ptrs[poly]->tvlist[0].z;
                        face.tvlist[0].u0 = (float)rc->rend_list->poly_ptrs[poly]->tvlist[0].u0;
                        face.tvlist[0].v0 = (float)rc->rend_list->poly_ptrs[poly]->tvlist[0].v0;

                        face.tvlist[1].x = (float)rc->rend_list->poly_ptrs[poly]->tvlist[1].x;
                        face.tvlist[1].y = (float)rc->rend_list->poly_ptrs[poly]->tvlist[1].y;
                        face.tvlist[1].z  = (float)rc->rend_list->poly_ptrs[poly]->tvlist[1].z;
                        face.tvlist[1].u0 = (float)rc->rend_list->poly_ptrs[poly]->tvlist[1].u0;
                        face.tvlist[1].v0 = (float)rc->rend_list->poly_ptrs[poly]->tvlist[1].v0;

                        face.tvlist[2].x = (float)rc->rend_list->poly_ptrs[poly]->tvlist[2].x;
                        face.tvlist[2].y = (float)rc->rend_list->poly_ptrs[poly]->tvlist[2].y;
                        face.tvlist[2].z  = (float)rc->rend_list->poly_ptrs[poly]->tvlist[2].z;
                        face.tvlist[2].u0 = (float)rc->rend_list->poly_ptrs[poly]->tvlist[2].u0;
                        face.tvlist[2].v0 = (float)rc->rend_list->poly_ptrs[poly]->tvlist[2].v0;


                        // test if this is a mipmapped polygon?
                        if (rc->rend_list->poly_ptrs[poly]->attr & POLY4DV2_ATTR_MIPMAP)
                        {
                            // determine if mipmapping is desired at all globally
                            if (rc->attr & RENDER_ATTR_MIPMAP)
                            {
                                // determine mip level for this polygon

                                // first determine how many miplevels there are in mipchain for this polygon
                                int tmiplevels = logbase2ofx[((BITMAP_IMAGE_PTR *)(rc->rend_list->poly_ptrs[poly]->texture))[0]->width];

                                // now based on the requested linear miplevel fall off distance, cut
                                // the viewdistance into segments, determine what segment polygon is
                                // in and select mip level -- simple! later you might want something more
                                // robust, also note I only use a single vertex, you might want to find the average
                                // since for long walls perpendicular to view direction this might causing mip
                                // popping mid surface
                                int miplevel = (tmiplevels * rc->rend_list->poly_ptrs[poly]->tvlist[0].z / rc->mip_dist);

                                // clamp miplevel
                                if (miplevel > tmiplevels) miplevel = tmiplevels;

                                // based on miplevel select proper texture
                                face.texture = ((BITMAP_IMAGE_PTR *)(rc->rend_list->poly_ptrs[poly]->texture))[miplevel];

                                // now we must divide each texture coordinate by 2 per miplevel
                                for (int ts = 0; ts < miplevel; ts++)
                                {
                                    face.tvlist[0].u0*=.5;
                                    face.tvlist[0].v0*=.5;

                                    face.tvlist[1].u0*=.5;
                                    face.tvlist[1].v0*=.5;

                                    face.tvlist[2].u0*=.5;
                                    face.tvlist[2].v0*=.5;
                                } // end for

                            } // end if mipmmaping enabled globally
                            else // mipmapping not selected globally
                            {
                                // in this case the polygon IS mipmapped, but the caller has requested NO
                                // mipmapping, so we will support this by selecting mip level 0 since the
                                // texture pointer is pointing to a mip chain regardless
                                face.texture = ((BITMAP_IMAGE_PTR *)(rc->rend_list->poly_ptrs[poly]->texture))[0];

                                // note: texture coordinate manipulation is unneeded

                            } // end else

                        } // end if
                        else
                        {
                            // assign the texture without change
                            face.texture = rc->rend_list->poly_ptrs[poly]->texture;
                        } // end if

                        // is this a plain emissive texture?
                        if (rc->rend_list->poly_ptrs[poly]->attr & POLY4DV2_ATTR_SHADE_MODE_CONSTANT)
                        {
                            // draw the textured triangle as emissive
                            if ((rc->attr & RENDER_ATTR_ALPHA) &&
                                ((rc->rend_list->poly_ptrs[poly]->attr & POLY4DV2_ATTR_TRANSPARENT) || rc->alpha_override>=0) )
                            {
                                // alpha version

                                // which texture mapper?
                                if (rc->attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_AFFINE)
                                {
                                    Draw_Textured_TriangleINVZB_Alpha16(&face, rc->video_buffer, rc->lpitch,rc->zbuffer,rc->zpitch, alpha);
                                } // end if
                                else
                                    if (rc->attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_CORRECT)
                                    {
                                        Draw_Textured_Perspective_Triangle_INVZB_Alpha16(&face, rc->video_buffer, rc->lpitch,rc->zbuffer,rc->zpitch, alpha);
                                    } // end if
                                    else
                                        if (rc->attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_LINEAR)
                                        {
                                            Draw_Textured_PerspectiveLP_Triangle_INVZB_Alpha16(&face, rc->video_buffer, rc->lpitch,rc->zbuffer,rc->zpitch, alpha);
                                        } // end if
                                        else
                                            if (rc->attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_HYBRID1)
                                            {
                                                // test z distance again perspective transition gate
                                                if (rc->rend_list->poly_ptrs[poly]->tvlist[0].z > rc-> texture_dist)
                                                {
                                                    // default back to affine
                                                    Draw_Textured_TriangleINVZB_Alpha16(&face, rc->video_buffer, rc->lpitch,rc->zbuffer,rc->zpitch, alpha);
                                                } // end if
                                                else
                                                {
                                                    // use perspective linear
                                                    Draw_Textured_PerspectiveLP_Triangle_INVZB_Alpha16(&face, rc->video_buffer, rc->lpitch,rc->zbuffer,rc->zpitch, alpha);
                                                } // end if

                                            } // end if

                            } // end if
                            else
                            {
                                // non alpha
                                // which texture mapper?
                                if (rc->attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_AFFINE)
                                {
                                    // use bilerp?
                                    if (rc->attr & RENDER_ATTR_BILERP)
                                        Draw_Textured_Bilerp_TriangleINVZB_16(&face, rc->video_buffer, rc->lpitch,rc->zbuffer,rc->zpitch);             
                                    else
                                        Draw_Textured_TriangleINVZB_16(&face, rc->video_buffer, rc->lpitch,rc->zbuffer,rc->zpitch);

                                } // end if
                                else
                                    if (rc->attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_CORRECT)
                                    {
                                        Draw_Textured_Perspective_Triangle_INVZB_16(&face, rc->video_buffer, rc->lpitch,rc->zbuffer,rc->zpitch);
                                    } // end if
                                    else
                                        if (rc->attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_LINEAR)
                                        {
                                            Draw_Textured_PerspectiveLP_Triangle_INVZB_16(&face, rc->video_buffer, rc->lpitch,rc->zbuffer,rc->zpitch);
                                        } // end if
                                        else
                                            if (rc->attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_HYBRID1)
                                            {
                                                // test z distance again perspective transition gate
                                                if (rc->rend_list->poly_ptrs[poly]->tvlist[0].z > rc-> texture_dist)
                                                {
                                                    // default back to affine
                                                    Draw_Textured_TriangleINVZB_16(&face, rc->video_buffer, rc->lpitch,rc->zbuffer,rc->zpitch);
                                                } // end if
                                                else
                                                {
                                                    // use perspective linear
                                                    Draw_Textured_PerspectiveLP_Triangle_INVZB_16(&face, rc->video_buffer, rc->lpitch,rc->zbuffer,rc->zpitch);
                                                } // end if

                                            } // end if

                            } // end if

                        } // end if
                        else
                            if (rc->rend_list->poly_ptrs[poly]->attr & POLY4DV2_ATTR_SHADE_MODE_FLAT)
                            {
                                // draw as flat shaded
                                face.lit_color[0] = rc->rend_list->poly_ptrs[poly]->lit_color[0];

                                // test for transparency
                                if ((rc->attr & RENDER_ATTR_ALPHA) &&
                                    ((rc->rend_list->poly_ptrs[poly]->attr & POLY4DV2_ATTR_TRANSPARENT) || rc->alpha_override>=0) )
                                {
                                    // alpha version

                                    // which texture mapper?
                                    if (rc->attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_AFFINE)
                                    {
                                        Draw_Textured_TriangleFSINVZB_Alpha16(&face, rc->video_buffer, rc->lpitch,rc->zbuffer,rc->zpitch, alpha);
                                    } // end if
                                    else
                                        if (rc->attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_CORRECT)
                                        {
                                            Draw_Textured_Perspective_Triangle_FSINVZB_Alpha16(&face, rc->video_buffer, rc->lpitch,rc->zbuffer,rc->zpitch, alpha);
                                        } // end if
                                        else
                                            if (rc->attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_LINEAR)
                                            {
                                                Draw_Textured_PerspectiveLP_Triangle_FSINVZB_Alpha16(&face, rc->video_buffer, rc->lpitch,rc->zbuffer,rc->zpitch, alpha);
                                            } // end if
                                            else
                                                if (rc->attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_HYBRID1)
                                                {
                                                    // test z distance again perspective transition gate
                                                    if (rc->rend_list->poly_ptrs[poly]->tvlist[0].z > rc-> texture_dist)
                                                    {
                                                        // default back to affine
                                                        Draw_Textured_TriangleFSINVZB_Alpha16(&face, rc->video_buffer, rc->lpitch,rc->zbuffer,rc->zpitch, alpha);
                                                    } // end if
                                                    else
                                                    {
                                                        // use perspective linear
                                                        Draw_Textured_PerspectiveLP_Triangle_FSINVZB_Alpha16(&face, rc->video_buffer, rc->lpitch,rc->zbuffer,rc->zpitch, alpha);
                                                    } // end if

                                                } // end if

                                } // end if
                                else
                                {
                                    // non alpha
                                    // which texture mapper?
                                    if (rc->attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_AFFINE)
                                    {
                                        Draw_Textured_TriangleFSINVZB_16(&face, rc->video_buffer, rc->lpitch,rc->zbuffer,rc->zpitch);
                                    } // end if
                                    else
                                        if (rc->attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_CORRECT)
                                        {
                                            Draw_Textured_Perspective_Triangle_FSINVZB_16(&face, rc->video_buffer, rc->lpitch,rc->zbuffer,rc->zpitch);
                                        } // end if
                                        else
                                            if (rc->attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_LINEAR)
                                            {
                                                Draw_Textured_PerspectiveLP_Triangle_FSINVZB_16(&face, rc->video_buffer, rc->lpitch,rc->zbuffer,rc->zpitch);
                                            } // end if
                                            else
                                                if (rc->attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_HYBRID1)
                                                {
                                                    // test z distance again perspective transition gate
                                                    if (rc->rend_list->poly_ptrs[poly]->tvlist[0].z > rc-> texture_dist)
                                                    {
                                                        // default back to affine
                                                        Draw_Textured_TriangleFSINVZB_16(&face, rc->video_buffer, rc->lpitch,rc->zbuffer,rc->zpitch);
                                                    } // end if
                                                    else
                                                    {
                                                        // use perspective linear
                                                        Draw_Textured_PerspectiveLP_Triangle_FSINVZB_16(&face, rc->video_buffer, rc->lpitch,rc->zbuffer,rc->zpitch);
                                                    } // end if

                                                } // end if

                                } // end if

                            } // end else if
                            else
                            { // POLY4DV2_ATTR_SHADE_MODE_GOURAUD

                                // must be gouraud POLY4DV2_ATTR_SHADE_MODE_GOURAUD
                                face.lit_color[0] = rc->rend_list->poly_ptrs[poly]->lit_color[0];
                                face.lit_color[1] = rc->rend_list->poly_ptrs[poly]->lit_color[1];
                                face.lit_color[2] = rc->rend_list->poly_ptrs[poly]->lit_color[2];

                                // test for transparency
                                if ((rc->attr & RENDER_ATTR_ALPHA) &&
                                    ((rc->rend_list->poly_ptrs[poly]->attr & POLY4DV2_ATTR_TRANSPARENT) || rc->alpha_override>=0) )
                                {
                                    // alpha version

                                    // which texture mapper?
                                    if (rc->attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_AFFINE)
                                    {
                                        Draw_Textured_TriangleGSINVZB_Alpha16(&face, rc->video_buffer, rc->lpitch,rc->zbuffer,rc->zpitch, alpha);
                                    } // end if
                                    else
                                        if (rc->attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_CORRECT)
                                        {
                                            // not supported yet :)
                                            Draw_Textured_TriangleGSINVZB_Alpha16(&face, rc->video_buffer, rc->lpitch,rc->zbuffer,rc->zpitch, alpha);
                                        } // end if
                                        else
                                            if (rc->attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_LINEAR)
                                            {
                                                // not supported yet :)
                                                Draw_Textured_TriangleGSINVZB_Alpha16(&face, rc->video_buffer, rc->lpitch,rc->zbuffer,rc->zpitch, alpha);
                                            } // end if
                                            else
                                                if (rc->attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_HYBRID1)
                                                {
                                                    // test z distance again perspective transition gate
                                                    if (rc->rend_list->poly_ptrs[poly]->tvlist[0].z > rc-> texture_dist)
                                                    {
                                                        // default back to affine
                                                        Draw_Textured_TriangleGSINVZB_Alpha16(&face, rc->video_buffer, rc->lpitch,rc->zbuffer,rc->zpitch, alpha);
                                                    } // end if
                                                    else
                                                    {
                                                        // use perspective linear
                                                        // not supported yet :)
                                                        Draw_Textured_TriangleGSINVZB_Alpha16(&face, rc->video_buffer, rc->lpitch,rc->zbuffer,rc->zpitch, alpha);
                                                    } // end if

                                                } // end if

                                } // end if
                                else
                                {
                                    // non alpha
                                    // which texture mapper?
                                    if (rc->attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_AFFINE)
                                    {
                                        Draw_Textured_TriangleGSINVZB_16(&face, rc->video_buffer, rc->lpitch,rc->zbuffer,rc->zpitch);
                                    } // end if
                                    else
                                        if (rc->attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_CORRECT)
                                        {
                                            // not supported yet :)
                                            Draw_Textured_TriangleGSINVZB_16(&face, rc->video_buffer, rc->lpitch,rc->zbuffer,rc->zpitch);
                                        } // end if
                                        else
                                            if (rc->attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_LINEAR)
                                            {
                                                // not supported yet :)
                                                Draw_Textured_TriangleGSINVZB_16(&face, rc->video_buffer, rc->lpitch,rc->zbuffer,rc->zpitch);
                                            } // end if
                                            else
                                                if (rc->attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_HYBRID1)
                                                {
                                                    // test z distance again perspective transition gate
                                                    if (rc->rend_list->poly_ptrs[poly]->tvlist[0].z > rc-> texture_dist)
                                                    {
                                                        // default back to affine
                                                        Draw_Textured_TriangleFSINVZB_16(&face, rc->video_buffer, rc->lpitch,rc->zbuffer,rc->zpitch);
                                                    } // end if
                                                    else
                                                    {
                                                        // use perspective linear
                                                        // not supported yet :)
                                                        Draw_Textured_TriangleGSINVZB_16(&face, rc->video_buffer, rc->lpitch,rc->zbuffer,rc->zpitch);
                                                    } // end if

                                                } // end if

                                } // end if

                            } // end else

                    } // end if      
                    else
                        if ((rc->rend_list->poly_ptrs[poly]->attr & POLY4DV2_ATTR_SHADE_MODE_FLAT) || 
                            (rc->rend_list->poly_ptrs[poly]->attr & POLY4DV2_ATTR_SHADE_MODE_CONSTANT) )
                        {
                            // draw as constant shaded
                            face.lit_color[0] = rc->rend_list->poly_ptrs[poly]->lit_color[0];

                            // set the vertices
                            face.tvlist[0].x = (float)rc->rend_list->poly_ptrs[poly]->tvlist[0].x;
                            face.tvlist[0].y = (float)rc->rend_list->poly_ptrs[poly]->tvlist[0].y;
                            face.tvlist[0].z = (float)rc->rend_list->poly_ptrs[poly]->tvlist[0].z;

                            face.tvlist[1].x = (float)rc->rend_list->poly_ptrs[poly]->tvlist[1].x;
                            face.tvlist[1].y = (float)rc->rend_list->poly_ptrs[poly]->tvlist[1].y;
                            face.tvlist[1].z = (float)rc->rend_list->poly_ptrs[poly]->tvlist[1].z;

                            face.tvlist[2].x = (float)rc->rend_list->poly_ptrs[poly]->tvlist[2].x;
                            face.tvlist[2].y = (float)rc->rend_list->poly_ptrs[poly]->tvlist[2].y;
                            face.tvlist[2].z = (float)rc->rend_list->poly_ptrs[poly]->tvlist[2].z;

                            // draw the triangle with basic flat rasterizer
                            // test for transparency
                            if ((rc->attr & RENDER_ATTR_ALPHA) &&
                                ((rc->rend_list->poly_ptrs[poly]->attr & POLY4DV2_ATTR_TRANSPARENT) || rc->alpha_override>=0) )
                            {
                                // alpha version
                                Draw_Triangle_2DINVZB_Alpha16(&face, rc->video_buffer, rc->lpitch,rc->zbuffer,rc->zpitch,alpha);
                            } // end if
                            else
                            {
                                // non alpha version
                                Draw_Triangle_2DINVZB_16(&face, rc->video_buffer, rc->lpitch,rc->zbuffer,rc->zpitch);
                            }  // end if

                        } // end if
                        else
                            if (rc->rend_list->poly_ptrs[poly]->attr & POLY4DV2_ATTR_SHADE_MODE_GOURAUD)
                            {
                                // {andre take advantage of the data structures later..}
                                // set the vertices
                                face.tvlist[0].x  = (float)rc->rend_list->poly_ptrs[poly]->tvlist[0].x;
                                face.tvlist[0].y  = (float)rc->rend_list->poly_ptrs[poly]->tvlist[0].y;
                                face.tvlist[0].z  = (float)rc->rend_list->poly_ptrs[poly]->tvlist[0].z;
                                face.lit_color[0] = rc->rend_list->poly_ptrs[poly]->lit_color[0];

                                face.tvlist[1].x  = (float)rc->rend_list->poly_ptrs[poly]->tvlist[1].x;
                                face.tvlist[1].y  = (float)rc->rend_list->poly_ptrs[poly]->tvlist[1].y;
                                face.tvlist[1].z  = (float)rc->rend_list->poly_ptrs[poly]->tvlist[1].z;
                                face.lit_color[1] = rc->rend_list->poly_ptrs[poly]->lit_color[1];

                                face.tvlist[2].x  = (float)rc->rend_list->poly_ptrs[poly]->tvlist[2].x;
                                face.tvlist[2].y  = (float)rc->rend_list->poly_ptrs[poly]->tvlist[2].y;
                                face.tvlist[2].z  = (float)rc->rend_list->poly_ptrs[poly]->tvlist[2].z;
                                face.lit_color[2] = rc->rend_list->poly_ptrs[poly]->lit_color[2];

                                // draw the gouraud shaded triangle
                                // test for transparency
                                if ((rc->attr & RENDER_ATTR_ALPHA) &&
                                    ((rc->rend_list->poly_ptrs[poly]->attr & POLY4DV2_ATTR_TRANSPARENT) || rc->alpha_override>=0) )
                                {
                                    // alpha version
                                    Draw_Gouraud_TriangleINVZB_Alpha16(&face, rc->video_buffer, rc->lpitch,rc->zbuffer,rc->zpitch,alpha);
                                } // end if
                                else
                                { 
                                    // non alpha
                                    Draw_Gouraud_TriangleINVZB_16(&face, rc->video_buffer, rc->lpitch,rc->zbuffer,rc->zpitch);
                                } // end if

                            } // end if gouraud

                } // end for poly

            } // end if RENDER_ATTR_INVZBUFFER
            else
                if (rc->attr & RENDER_ATTR_WRITETHRUZBUFFER) ////////////////////////////////////
                {
                    // use the write thru z buffer

                    // we have is a list of polygons and it's time draw them
                    for (int poly=0; poly < rc->rend_list->num_polys; poly++)
                    {
                        // render this polygon if and only if it's not clipped, not culled,
                        // active, and visible, note however the concecpt of "backface" is 
                        // irrelevant in a wire frame engine though
                        if (!(rc->rend_list->poly_ptrs[poly]->state & POLY4DV2_STATE_ACTIVE) ||
                            (rc->rend_list->poly_ptrs[poly]->state & POLY4DV2_STATE_CLIPPED ) ||
                            (rc->rend_list->poly_ptrs[poly]->state & POLY4DV2_STATE_BACKFACE) )
                            continue; // move onto next poly

                        // test for alpha override
                        if (rc->alpha_override>= 0)
                        {
                            // set alpha to override value
                            alpha = rc->alpha_override;
                        }  // end if 
                        else
                        {
                            // extract alpha (even if there isn't any)
                            alpha = ((rc->rend_list->poly_ptrs[poly]->color & 0xff000000) >> 24);
                        } // end else

                        // need to test for textured first, since a textured poly can either
                        // be emissive, or flat shaded, hence we need to call different
                        // rasterizers    
                        if (rc->rend_list->poly_ptrs[poly]->attr & POLY4DV2_ATTR_SHADE_MODE_TEXTURE)
                        {
                            // set the vertices
                            face.tvlist[0].x = (float)rc->rend_list->poly_ptrs[poly]->tvlist[0].x;
                            face.tvlist[0].y = (float)rc->rend_list->poly_ptrs[poly]->tvlist[0].y;
                            face.tvlist[0].z  = (float)rc->rend_list->poly_ptrs[poly]->tvlist[0].z;
                            face.tvlist[0].u0 = (float)rc->rend_list->poly_ptrs[poly]->tvlist[0].u0;
                            face.tvlist[0].v0 = (float)rc->rend_list->poly_ptrs[poly]->tvlist[0].v0;

                            face.tvlist[1].x = (float)rc->rend_list->poly_ptrs[poly]->tvlist[1].x;
                            face.tvlist[1].y = (float)rc->rend_list->poly_ptrs[poly]->tvlist[1].y;
                            face.tvlist[1].z  = (float)rc->rend_list->poly_ptrs[poly]->tvlist[1].z;
                            face.tvlist[1].u0 = (float)rc->rend_list->poly_ptrs[poly]->tvlist[1].u0;
                            face.tvlist[1].v0 = (float)rc->rend_list->poly_ptrs[poly]->tvlist[1].v0;

                            face.tvlist[2].x = (float)rc->rend_list->poly_ptrs[poly]->tvlist[2].x;
                            face.tvlist[2].y = (float)rc->rend_list->poly_ptrs[poly]->tvlist[2].y;
                            face.tvlist[2].z  = (float)rc->rend_list->poly_ptrs[poly]->tvlist[2].z;
                            face.tvlist[2].u0 = (float)rc->rend_list->poly_ptrs[poly]->tvlist[2].u0;
                            face.tvlist[2].v0 = (float)rc->rend_list->poly_ptrs[poly]->tvlist[2].v0;

                            // test if this is a mipmapped polygon?
                            if (rc->rend_list->poly_ptrs[poly]->attr & POLY4DV2_ATTR_MIPMAP)
                            {
                                // determine if mipmapping is desired at all globally
                                if (rc->attr & RENDER_ATTR_MIPMAP)
                                {
                                    // determine mip level for this polygon

                                    // first determine how many miplevels there are in mipchain for this polygon
                                    int tmiplevels = logbase2ofx[((BITMAP_IMAGE_PTR *)(rc->rend_list->poly_ptrs[poly]->texture))[0]->width];

                                    // now based on the requested linear miplevel fall off distance, cut
                                    // the viewdistance into segments, determine what segment polygon is
                                    // in and select mip level -- simple! later you might want something more
                                    // robust, also note I only use a single vertex, you might want to find the average
                                    // since for long walls perpendicular to view direction this might causing mip
                                    // popping mid surface
                                    int miplevel = (tmiplevels * rc->rend_list->poly_ptrs[poly]->tvlist[0].z / rc->mip_dist);

                                    // clamp miplevel
                                    if (miplevel > tmiplevels) miplevel = tmiplevels;

                                    // based on miplevel select proper texture
                                    face.texture = ((BITMAP_IMAGE_PTR *)(rc->rend_list->poly_ptrs[poly]->texture))[miplevel];

                                    // now we must divide each texture coordinate by 2 per miplevel
                                    for (int ts = 0; ts < miplevel; ts++)
                                    {
                                        face.tvlist[0].u0*=.5;
                                        face.tvlist[0].v0*=.5;

                                        face.tvlist[1].u0*=.5;
                                        face.tvlist[1].v0*=.5;

                                        face.tvlist[2].u0*=.5;
                                        face.tvlist[2].v0*=.5;
                                    } // end for

                                } // end if mipmmaping enabled globally
                                else // mipmapping not selected globally
                                {
                                    // in this case the polygon IS mipmapped, but the caller has requested NO
                                    // mipmapping, so we will support this by selecting mip level 0 since the
                                    // texture pointer is pointing to a mip chain regardless
                                    face.texture = ((BITMAP_IMAGE_PTR *)(rc->rend_list->poly_ptrs[poly]->texture))[0];

                                    // note: texture coordinate manipulation is unneeded

                                } // end else

                            } // end if
                            else
                            {
                                // assign the texture without change
                                face.texture = rc->rend_list->poly_ptrs[poly]->texture;
                            } // end if

                            // is this a plain emissive texture?
                            if (rc->rend_list->poly_ptrs[poly]->attr & POLY4DV2_ATTR_SHADE_MODE_CONSTANT)
                            {
                                // draw the textured triangle as emissive
                                if ((rc->attr & RENDER_ATTR_ALPHA) &&
                                    ((rc->rend_list->poly_ptrs[poly]->attr & POLY4DV2_ATTR_TRANSPARENT) || rc->alpha_override>=0) )
                                {
                                    // alpha version

                                    // which texture mapper?
                                    if (rc->attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_AFFINE)
                                    {
                                        // redo with transparency
                                        Draw_Textured_TriangleZB_Alpha16_2(&face, rc->video_buffer, rc->lpitch,rc->zbuffer,rc->zpitch, alpha);
                                    } // end if
                                    else
                                        if (rc->attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_CORRECT)
                                        {
                                            // not supported yet!
                                            // redo with transparency
                                            Draw_Textured_TriangleZB_Alpha16_2(&face, rc->video_buffer, rc->lpitch,rc->zbuffer,rc->zpitch, alpha);
                                        } // end if
                                        else
                                            if (rc->attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_LINEAR)
                                            {
                                                // not supported yet
                                                // redo with transparency
                                                Draw_Textured_TriangleZB_Alpha16_2(&face, rc->video_buffer, rc->lpitch,rc->zbuffer,rc->zpitch, alpha);
                                            } // end if
                                            else
                                                if (rc->attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_HYBRID1)
                                                {
                                                    // test z distance again perspective transition gate
                                                    if (rc->rend_list->poly_ptrs[poly]->tvlist[0].z > rc-> texture_dist)
                                                    {
                                                        // default back to affine
                                                        // redo with transparency
                                                        Draw_Textured_TriangleZB_Alpha16_2(&face, rc->video_buffer, rc->lpitch,rc->zbuffer,rc->zpitch, alpha);
                                                    } // end if
                                                    else
                                                    {
                                                        // use perspective linear
                                                        // not supported yet
                                                        // redo with transparency
                                                        Draw_Textured_TriangleZB_Alpha16_2(&face, rc->video_buffer, rc->lpitch,rc->zbuffer,rc->zpitch, alpha);
                                                    } // end if

                                                } // end if

                                } // end if
                                else
                                {
                                    // non alpha
                                    // which texture mapper?
                                    if (rc->attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_AFFINE)
                                    {
                                        // use bilerp?
                                        if (rc->attr & RENDER_ATTR_BILERP)
                                            Draw_Textured_Bilerp_TriangleZB_16(&face, rc->video_buffer, rc->lpitch,rc->zbuffer,rc->zpitch);               
                                        else
                                            Draw_Textured_TriangleWTZB3_16(&face, rc->video_buffer, rc->lpitch,rc->zbuffer,rc->zpitch);

                                    } // end if
                                    else
                                        if (rc->attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_CORRECT)
                                        {
                                            // not supported yet
                                            Draw_Textured_TriangleZB2_16(&face, rc->video_buffer, rc->lpitch,rc->zbuffer,rc->zpitch);
                                        } // end if
                                        else
                                            if (rc->attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_LINEAR)
                                            {
                                                // not supported yet
                                                Draw_Textured_TriangleZB2_16(&face, rc->video_buffer, rc->lpitch,rc->zbuffer,rc->zpitch);
                                            } // end if
                                            else
                                                if (rc->attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_HYBRID1)
                                                {
                                                    // test z distance again perspective transition gate
                                                    if (rc->rend_list->poly_ptrs[poly]->tvlist[0].z > rc-> texture_dist)
                                                    {
                                                        // default back to affine
                                                        Draw_Textured_TriangleZB2_16(&face, rc->video_buffer, rc->lpitch,rc->zbuffer,rc->zpitch);
                                                    } // end if
                                                    else
                                                    {
                                                        // use perspective linear
                                                        // not supported yet
                                                        Draw_Textured_TriangleZB2_16(&face, rc->video_buffer, rc->lpitch,rc->zbuffer,rc->zpitch);
                                                    } // end if

                                                } // end if

                                } // end if

                            } // end if
                            else
                                if (rc->rend_list->poly_ptrs[poly]->attr & POLY4DV2_ATTR_SHADE_MODE_FLAT)
                                {
                                    // draw as flat shaded
                                    face.lit_color[0] = rc->rend_list->poly_ptrs[poly]->lit_color[0];

                                    // test for transparency
                                    if ((rc->attr & RENDER_ATTR_ALPHA) &&
                                        ((rc->rend_list->poly_ptrs[poly]->attr & POLY4DV2_ATTR_TRANSPARENT) || rc->alpha_override>=0) )
                                    {
                                        // alpha version

                                        // which texture mapper?
                                        if (rc->attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_AFFINE)
                                        {
                                            Draw_Textured_TriangleFSZB_Alpha16(&face, rc->video_buffer, rc->lpitch,rc->zbuffer,rc->zpitch, alpha);
                                        } // end if
                                        else
                                            if (rc->attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_CORRECT)
                                            {
                                                // not supported yet
                                                Draw_Textured_TriangleFSZB_Alpha16(&face, rc->video_buffer, rc->lpitch,rc->zbuffer,rc->zpitch, alpha);
                                            } // end if
                                            else
                                                if (rc->attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_LINEAR)
                                                {
                                                    // not supported yet
                                                    Draw_Textured_TriangleFSZB_Alpha16(&face, rc->video_buffer, rc->lpitch,rc->zbuffer,rc->zpitch, alpha);
                                                } // end if
                                                else
                                                    if (rc->attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_HYBRID1)
                                                    {
                                                        // test z distance again perspective transition gate
                                                        if (rc->rend_list->poly_ptrs[poly]->tvlist[0].z > rc-> texture_dist)
                                                        {
                                                            // default back to affine
                                                            Draw_Textured_TriangleFSZB_Alpha16(&face, rc->video_buffer, rc->lpitch,rc->zbuffer,rc->zpitch, alpha);
                                                        } // end if
                                                        else
                                                        {
                                                            // use perspective linear
                                                            // not supported yet
                                                            Draw_Textured_TriangleFSZB_Alpha16(&face, rc->video_buffer, rc->lpitch,rc->zbuffer,rc->zpitch, alpha);
                                                        } // end if

                                                    } // end if

                                    } // end if
                                    else
                                    {
                                        // non alpha
                                        // which texture mapper?
                                        if (rc->attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_AFFINE)
                                        {
                                            Draw_Textured_TriangleFSWTZB3_16(&face, rc->video_buffer, rc->lpitch,rc->zbuffer,rc->zpitch);
                                        } // end if
                                        else
                                            if (rc->attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_CORRECT)
                                            {
                                                // not supported yet
                                                Draw_Textured_TriangleFSZB2_16(&face, rc->video_buffer, rc->lpitch,rc->zbuffer,rc->zpitch);
                                            } // end if
                                            else
                                                if (rc->attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_LINEAR)
                                                {
                                                    // not supported yet
                                                    Draw_Textured_TriangleFSZB2_16(&face, rc->video_buffer, rc->lpitch,rc->zbuffer,rc->zpitch);
                                                } // end if
                                                else
                                                    if (rc->attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_HYBRID1)
                                                    {
                                                        // test z distance again perspective transition gate
                                                        if (rc->rend_list->poly_ptrs[poly]->tvlist[0].z > rc-> texture_dist)
                                                        {
                                                            // default back to affine
                                                            Draw_Textured_TriangleFSZB2_16(&face, rc->video_buffer, rc->lpitch,rc->zbuffer,rc->zpitch);
                                                        } // end if
                                                        else
                                                        {
                                                            // use perspective linear
                                                            // not supported yet
                                                            Draw_Textured_TriangleFSZB2_16(&face, rc->video_buffer, rc->lpitch,rc->zbuffer,rc->zpitch);
                                                        } // end if

                                                    } // end if

                                    } // end if

                                } // end else if
                                else
                                { // POLY4DV2_ATTR_SHADE_MODE_GOURAUD

                                    // must be gouraud POLY4DV2_ATTR_SHADE_MODE_GOURAUD
                                    face.lit_color[0] = rc->rend_list->poly_ptrs[poly]->lit_color[0];
                                    face.lit_color[1] = rc->rend_list->poly_ptrs[poly]->lit_color[1];
                                    face.lit_color[2] = rc->rend_list->poly_ptrs[poly]->lit_color[2];

                                    // test for transparency
                                    if ((rc->attr & RENDER_ATTR_ALPHA) &&
                                        ((rc->rend_list->poly_ptrs[poly]->attr & POLY4DV2_ATTR_TRANSPARENT) || rc->alpha_override>=0) )
                                    {
                                        // alpha version

                                        // which texture mapper?
                                        if (rc->attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_AFFINE)
                                        {
                                            Draw_Textured_TriangleGSZB_Alpha16(&face, rc->video_buffer, rc->lpitch,rc->zbuffer,rc->zpitch, alpha);
                                        } // end if
                                        else
                                            if (rc->attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_CORRECT)
                                            {
                                                // not supported yet :)
                                                Draw_Textured_TriangleGSZB_Alpha16(&face, rc->video_buffer, rc->lpitch,rc->zbuffer,rc->zpitch, alpha);
                                            } // end if
                                            else
                                                if (rc->attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_LINEAR)
                                                {
                                                    // not supported yet :)
                                                    Draw_Textured_TriangleGSZB_Alpha16(&face, rc->video_buffer, rc->lpitch,rc->zbuffer,rc->zpitch, alpha);
                                                } // end if
                                                else
                                                    if (rc->attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_HYBRID1)
                                                    {
                                                        // test z distance again perspective transition gate
                                                        if (rc->rend_list->poly_ptrs[poly]->tvlist[0].z > rc-> texture_dist)
                                                        {
                                                            // default back to affine
                                                            Draw_Textured_TriangleGSZB_Alpha16(&face, rc->video_buffer, rc->lpitch,rc->zbuffer,rc->zpitch, alpha);
                                                        } // end if
                                                        else
                                                        {
                                                            // use perspective linear
                                                            // not supported yet :)
                                                            Draw_Textured_TriangleGSZB_Alpha16(&face, rc->video_buffer, rc->lpitch,rc->zbuffer,rc->zpitch, alpha);
                                                        } // end if

                                                    } // end if

                                    } // end if
                                    else
                                    {
                                        // non alpha
                                        // which texture mapper?
                                        if (rc->attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_AFFINE)
                                        {
                                            Draw_Textured_TriangleGSZB_16(&face, rc->video_buffer, rc->lpitch,rc->zbuffer,rc->zpitch);
                                        } // end if
                                        else
                                            if (rc->attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_CORRECT)
                                            {
                                                // not supported yet :)
                                                Draw_Textured_TriangleGSZB_16(&face, rc->video_buffer, rc->lpitch,rc->zbuffer,rc->zpitch);
                                            } // end if
                                            else
                                                if (rc->attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_LINEAR)
                                                {
                                                    // not supported yet :)
                                                    Draw_Textured_TriangleGSZB_16(&face, rc->video_buffer, rc->lpitch,rc->zbuffer,rc->zpitch);
                                                } // end if
                                                else
                                                    if (rc->attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_HYBRID1)
                                                    {
                                                        // test z distance again perspective transition gate
                                                        if (rc->rend_list->poly_ptrs[poly]->tvlist[0].z > rc-> texture_dist)
                                                        {
                                                            // default back to affine
                                                            Draw_Textured_TriangleGSZB_16(&face, rc->video_buffer, rc->lpitch,rc->zbuffer,rc->zpitch);
                                                        } // end if
                                                        else
                                                        {
                                                            // use perspective linear
                                                            // not supported yet :)
                                                            Draw_Textured_TriangleGSZB_16(&face, rc->video_buffer, rc->lpitch,rc->zbuffer,rc->zpitch);
                                                        } // end if

                                                    } // end if

                                    } // end if

                                } // end else

                        } // end if      
                        else
                            if ((rc->rend_list->poly_ptrs[poly]->attr & POLY4DV2_ATTR_SHADE_MODE_FLAT) || 
                                (rc->rend_list->poly_ptrs[poly]->attr & POLY4DV2_ATTR_SHADE_MODE_CONSTANT) )
                            {
                                // draw as constant shaded
                                face.lit_color[0] = rc->rend_list->poly_ptrs[poly]->lit_color[0];

                                // set the vertices
                                face.tvlist[0].x = (float)rc->rend_list->poly_ptrs[poly]->tvlist[0].x;
                                face.tvlist[0].y = (float)rc->rend_list->poly_ptrs[poly]->tvlist[0].y;
                                face.tvlist[0].z = (float)rc->rend_list->poly_ptrs[poly]->tvlist[0].z;

                                face.tvlist[1].x = (float)rc->rend_list->poly_ptrs[poly]->tvlist[1].x;
                                face.tvlist[1].y = (float)rc->rend_list->poly_ptrs[poly]->tvlist[1].y;
                                face.tvlist[1].z = (float)rc->rend_list->poly_ptrs[poly]->tvlist[1].z;

                                face.tvlist[2].x = (float)rc->rend_list->poly_ptrs[poly]->tvlist[2].x;
                                face.tvlist[2].y = (float)rc->rend_list->poly_ptrs[poly]->tvlist[2].y;
                                face.tvlist[2].z = (float)rc->rend_list->poly_ptrs[poly]->tvlist[2].z;

                                // draw the triangle with basic flat rasterizer
                                // test for transparency
                                if ((rc->attr & RENDER_ATTR_ALPHA) &&
                                    ((rc->rend_list->poly_ptrs[poly]->attr & POLY4DV2_ATTR_TRANSPARENT) || rc->alpha_override>=0) )
                                {
                                    // alpha version
                                    // redo with transparency
                                    Draw_Triangle_2DZB_Alpha16(&face, rc->video_buffer, rc->lpitch,rc->zbuffer,rc->zpitch,alpha);
                                } // end if
                                else
                                {
                                    // non alpha version
                                    Draw_Triangle_2DZB_16(&face, rc->video_buffer, rc->lpitch,rc->zbuffer,rc->zpitch);
                                }  // end if

                            } // end if
                            else
                                if (rc->rend_list->poly_ptrs[poly]->attr & POLY4DV2_ATTR_SHADE_MODE_GOURAUD)
                                {
                                    // {andre take advantage of the data structures later..}
                                    // set the vertices
                                    face.tvlist[0].x  = (float)rc->rend_list->poly_ptrs[poly]->tvlist[0].x;
                                    face.tvlist[0].y  = (float)rc->rend_list->poly_ptrs[poly]->tvlist[0].y;
                                    face.tvlist[0].z  = (float)rc->rend_list->poly_ptrs[poly]->tvlist[0].z;
                                    face.lit_color[0] = rc->rend_list->poly_ptrs[poly]->lit_color[0];

                                    face.tvlist[1].x  = (float)rc->rend_list->poly_ptrs[poly]->tvlist[1].x;
                                    face.tvlist[1].y  = (float)rc->rend_list->poly_ptrs[poly]->tvlist[1].y;
                                    face.tvlist[1].z  = (float)rc->rend_list->poly_ptrs[poly]->tvlist[1].z;
                                    face.lit_color[1] = rc->rend_list->poly_ptrs[poly]->lit_color[1];

                                    face.tvlist[2].x  = (float)rc->rend_list->poly_ptrs[poly]->tvlist[2].x;
                                    face.tvlist[2].y  = (float)rc->rend_list->poly_ptrs[poly]->tvlist[2].y;
                                    face.tvlist[2].z  = (float)rc->rend_list->poly_ptrs[poly]->tvlist[2].z;
                                    face.lit_color[2] = rc->rend_list->poly_ptrs[poly]->lit_color[2];

                                    // draw the gouraud shaded triangle
                                    // test for transparency
                                    if ((rc->attr & RENDER_ATTR_ALPHA) &&
                                        ((rc->rend_list->poly_ptrs[poly]->attr & POLY4DV2_ATTR_TRANSPARENT) || rc->alpha_override>=0) )
                                    {
                                        // alpha version
                                        // redo with transparency
                                        Draw_Gouraud_TriangleZB_Alpha16(&face, rc->video_buffer, rc->lpitch,rc->zbuffer,rc->zpitch,alpha);
                                    } // end if
                                    else
                                    { 
                                        // non alpha
                                        Draw_Gouraud_TriangleWTZB2_16(&face, rc->video_buffer, rc->lpitch,rc->zbuffer,rc->zpitch);
                                    } // end if

                                } // end if gouraud

                    } // end for poly

                } // end if RENDER_ATTR_ZBUFFER

} // end Draw_RENDERLIST4DV2_RENDERCONTEXTV1_16_3

////////////////////////////////////////////////////////////////////////////////////////

void Draw_Textured_TriangleWTZB3_16(POLYF4DV2_PTR face,  // ptr to face
                                    UCHAR *_dest_buffer,       // pointer to video buffer
                                    int mem_pitch,             // bytes per line, 320, 640 etc.
                                    UCHAR *_zbuffer,           // pointer to z-buffer
                                    int zpitch)                // bytes per line of zbuffer
{
    // this function draws a textured triangle in 16-bit mode

    int v0=0,
        v1=1,
        v2=2,
        temp=0,
        tri_type = TRI_TYPE_NONE,
        irestart = INTERP_LHS;

    int dx,dy,dyl,dyr,      // general deltas
        u,v,z,
        du,dv,dz,
        xi,yi,              // the current interpolated x,y
        ui,vi,zi,           // the current interpolated u,v,z
        index_x,index_y,    // looping vars
        x,y,                // hold general x,y
        xstart,
        xend,
        ystart,
        yrestart,
        yend,
        xl,                 
        dxdyl,              
        xr,
        dxdyr,             
        dudyl,    
        ul,
        dvdyl,   
        vl,
        dzdyl,   
        zl,
        dudyr,
        ur,
        dvdyr,
        vr,
        dzdyr,
        zr;

    int x0,y0,tu0,tv0,tz0,    // cached vertices
        x1,y1,tu1,tv1,tz1,
        x2,y2,tu2,tv2,tz2;

    USHORT *screen_ptr  = NULL,
        *screen_line = NULL,
        *textmap     = NULL,
        *dest_buffer = (USHORT *)_dest_buffer,
        textel;

    UINT  *z_ptr = NULL,
        *zbuffer = (UINT *)_zbuffer;

#ifdef DEBUG_ON
    // track rendering stats
    debug_polys_rendered_per_frame++;
#endif

    // extract texture map
    textmap = (USHORT *)face->texture->buffer;

    // extract base 2 of texture width
    int texture_shift2 = logbase2ofx[face->texture->width];

    // adjust memory pitch to words, divide by 2
    mem_pitch >>=1;

    // adjust zbuffer pitch for 32 bit alignment
    zpitch >>= 2;

    // apply fill convention to coordinates
    face->tvlist[0].x = (int)(face->tvlist[0].x+0.5);
    face->tvlist[0].y = (int)(face->tvlist[0].y+0.5);

    face->tvlist[1].x = (int)(face->tvlist[1].x+0.5);
    face->tvlist[1].y = (int)(face->tvlist[1].y+0.5);

    face->tvlist[2].x = (int)(face->tvlist[2].x+0.5);
    face->tvlist[2].y = (int)(face->tvlist[2].y+0.5);

    // first trivial clipping rejection tests 
    if (((face->tvlist[0].y < min_clip_y)  && 
        (face->tvlist[1].y < min_clip_y)  &&
        (face->tvlist[2].y < min_clip_y)) ||

        ((face->tvlist[0].y > max_clip_y)  && 
        (face->tvlist[1].y > max_clip_y)  &&
        (face->tvlist[2].y > max_clip_y)) ||

        ((face->tvlist[0].x < min_clip_x)  && 
        (face->tvlist[1].x < min_clip_x)  &&
        (face->tvlist[2].x < min_clip_x)) ||

        ((face->tvlist[0].x > max_clip_x)  && 
        (face->tvlist[1].x > max_clip_x)  &&
        (face->tvlist[2].x > max_clip_x)))
        return;

    // sort vertices
    if (face->tvlist[v1].y < face->tvlist[v0].y) 
    {SWAP(v0,v1,temp);} 

    if (face->tvlist[v2].y < face->tvlist[v0].y) 
    {SWAP(v0,v2,temp);}

    if (face->tvlist[v2].y < face->tvlist[v1].y) 
    {SWAP(v1,v2,temp);}

    // now test for trivial flat sided cases
    if (FCMP(face->tvlist[v0].y, face->tvlist[v1].y) )
    { 
        // set triangle type
        tri_type = TRI_TYPE_FLAT_TOP;

        // sort vertices left to right
        if (face->tvlist[v1].x < face->tvlist[v0].x) 
        {SWAP(v0,v1,temp);}

    } // end if
    else
        // now test for trivial flat sided cases
        if (FCMP(face->tvlist[v1].y ,face->tvlist[v2].y))
        { 
            // set triangle type
            tri_type = TRI_TYPE_FLAT_BOTTOM;

            // sort vertices left to right
            if (face->tvlist[v2].x < face->tvlist[v1].x) 
            {SWAP(v1,v2,temp);}

        } // end if
        else
        {
            // must be a general triangle
            tri_type = TRI_TYPE_GENERAL;

        } // end else

        // extract vertices for processing, now that we have order
        x0  = (int)(face->tvlist[v0].x+0.0);
        y0  = (int)(face->tvlist[v0].y+0.0);
        tu0 = (int)(face->tvlist[v0].u0);
        tv0 = (int)(face->tvlist[v0].v0);
        tz0 = (int)(face->tvlist[v0].z+0.5);

        x1  = (int)(face->tvlist[v1].x+0.0);
        y1  = (int)(face->tvlist[v1].y+0.0);
        tu1 = (int)(face->tvlist[v1].u0);
        tv1 = (int)(face->tvlist[v1].v0);
        tz1 = (int)(face->tvlist[v1].z+0.5);

        x2  = (int)(face->tvlist[v2].x+0.0);
        y2  = (int)(face->tvlist[v2].y+0.0);
        tu2 = (int)(face->tvlist[v2].u0);
        tv2 = (int)(face->tvlist[v2].v0);
        tz2 = (int)(face->tvlist[v2].z+0.5);


        // degenerate triangle
        if ( ((x0 == x1) && (x1 == x2)) || ((y0 ==  y1) && (y1 == y2)))
            return;

        // set interpolation restart value
        yrestart = y1;

        // what kind of triangle
        if (tri_type & TRI_TYPE_FLAT_MASK)
        {

            if (tri_type == TRI_TYPE_FLAT_TOP)
            {
                // compute all deltas
                dy = (y2 - y0);

                dxdyl = ((x2 - x0)   << FIXP16_SHIFT)/dy;
                dudyl = ((tu2 - tu0) << FIXP16_SHIFT)/dy;  
                dvdyl = ((tv2 - tv0) << FIXP16_SHIFT)/dy;    
                dzdyl = ((tz2 - tz0) << FIXP16_SHIFT)/dy;    

                dxdyr = ((x2 - x1)   << FIXP16_SHIFT)/dy;
                dudyr = ((tu2 - tu1) << FIXP16_SHIFT)/dy;  
                dvdyr = ((tv2 - tv1) << FIXP16_SHIFT)/dy;   
                dzdyr = ((tz2 - tz1) << FIXP16_SHIFT)/dy;  

                // test for y clipping
                if (y0 < min_clip_y)
                {
                    // compute overclip
                    dy = (min_clip_y - y0);

                    // computer new LHS starting values
                    xl = dxdyl*dy + (x0  << FIXP16_SHIFT);
                    ul = dudyl*dy + (tu0 << FIXP16_SHIFT);
                    vl = dvdyl*dy + (tv0 << FIXP16_SHIFT);
                    zl = dzdyl*dy + (tz0 << FIXP16_SHIFT);

                    // compute new RHS starting values
                    xr = dxdyr*dy + (x1  << FIXP16_SHIFT);
                    ur = dudyr*dy + (tu1 << FIXP16_SHIFT);
                    vr = dvdyr*dy + (tv1 << FIXP16_SHIFT);
                    zr = dzdyr*dy + (tz1 << FIXP16_SHIFT);

                    // compute new starting y
                    ystart = min_clip_y;

                } // end if
                else
                {
                    // no clipping

                    // set starting values
                    xl = (x0 << FIXP16_SHIFT);
                    xr = (x1 << FIXP16_SHIFT);

                    ul = (tu0 << FIXP16_SHIFT);
                    vl = (tv0 << FIXP16_SHIFT);
                    zl = (tz0 << FIXP16_SHIFT);

                    ur = (tu1 << FIXP16_SHIFT);
                    vr = (tv1 << FIXP16_SHIFT);
                    zr = (tz1 << FIXP16_SHIFT);

                    // set starting y
                    ystart = y0;

                } // end else

            } // end if flat top
            else
            {
                // must be flat bottom

                // compute all deltas
                dy = (y1 - y0);

                dxdyl = ((x1 - x0)   << FIXP16_SHIFT)/dy;
                dudyl = ((tu1 - tu0) << FIXP16_SHIFT)/dy;  
                dvdyl = ((tv1 - tv0) << FIXP16_SHIFT)/dy;    
                dzdyl = ((tz1 - tz0) << FIXP16_SHIFT)/dy;   

                dxdyr = ((x2 - x0)   << FIXP16_SHIFT)/dy;
                dudyr = ((tu2 - tu0) << FIXP16_SHIFT)/dy;  
                dvdyr = ((tv2 - tv0) << FIXP16_SHIFT)/dy;   
                dzdyr = ((tz2 - tz0) << FIXP16_SHIFT)/dy;   

                // test for y clipping
                if (y0 < min_clip_y)
                {
                    // compute overclip
                    dy = (min_clip_y - y0);

                    // computer new LHS starting values
                    xl = dxdyl*dy + (x0  << FIXP16_SHIFT);
                    ul = dudyl*dy + (tu0 << FIXP16_SHIFT);
                    vl = dvdyl*dy + (tv0 << FIXP16_SHIFT);
                    zl = dzdyl*dy + (tz0 << FIXP16_SHIFT);

                    // compute new RHS starting values
                    xr = dxdyr*dy + (x0  << FIXP16_SHIFT);
                    ur = dudyr*dy + (tu0 << FIXP16_SHIFT);
                    vr = dvdyr*dy + (tv0 << FIXP16_SHIFT);
                    zr = dzdyr*dy + (tz0 << FIXP16_SHIFT);

                    // compute new starting y
                    ystart = min_clip_y;

                } // end if
                else
                {
                    // no clipping

                    // set starting values
                    xl = (x0 << FIXP16_SHIFT);
                    xr = (x0 << FIXP16_SHIFT);

                    ul = (tu0 << FIXP16_SHIFT);
                    vl = (tv0 << FIXP16_SHIFT);
                    zl = (tz0 << FIXP16_SHIFT);

                    ur = (tu0 << FIXP16_SHIFT);
                    vr = (tv0 << FIXP16_SHIFT);
                    zr = (tz0 << FIXP16_SHIFT);

                    // set starting y
                    ystart = y0;

                } // end else	

            } // end else flat bottom

            // test for bottom clip, always
            if ((yend = y2) > max_clip_y)
                yend = max_clip_y;

            // test for horizontal clipping
            if ((x0 < min_clip_x) || (x0 > max_clip_x) ||
                (x1 < min_clip_x) || (x1 > max_clip_x) ||
                (x2 < min_clip_x) || (x2 > max_clip_x))
            {
                // clip version

                // point screen ptr to starting line
                screen_ptr = dest_buffer + (ystart * mem_pitch);

                // point zbuffer to starting line
                z_ptr = zbuffer + (ystart * zpitch);

                for (yi = ystart; yi < yend; yi++)
                {
                    // compute span endpoints
                    xstart = ((xl + FIXP16_ROUND_UP) >> FIXP16_SHIFT);
                    xend   = ((xr + FIXP16_ROUND_UP) >> FIXP16_SHIFT);

                    // compute starting points for u,v interpolants
                    ui = ul + FIXP16_ROUND_UP;
                    vi = vl + FIXP16_ROUND_UP;
                    zi = zl + FIXP16_ROUND_UP;

                    // compute u,v interpolants
                    if ((dx = (xend - xstart))>0)
                    {
                        du = (ur - ul)/dx;
                        dv = (vr - vl)/dx;
                        dz = (zr - zl)/dx;
                    } // end if
                    else
                    {
                        du = (ur - ul);
                        dv = (vr - vl);
                        dz = (zr - zl);
                    } // end else

                    ///////////////////////////////////////////////////////////////////////

                    // test for x clipping, LHS
                    if (xstart < min_clip_x)
                    {
                        // compute x overlap
                        dx = min_clip_x - xstart;

                        // slide interpolants over
                        ui+=dx*du;
                        vi+=dx*dv;
                        zi+=dx*dz;

                        // reset vars
                        xstart = min_clip_x;

                    } // end if

                    // test for x clipping RHS
                    if (xend > max_clip_x)
                        xend = max_clip_x;

                    ///////////////////////////////////////////////////////////////////////

                    // draw span
                    for (xi=xstart; xi < xend; xi++)
                    {
                        // write thru the z buffer always
                        // write textel

                        // test for transparent pixel
                        textel = textmap[(ui >> FIXP16_SHIFT) + ((vi >> FIXP16_SHIFT) << texture_shift2)];

                        if (textel)
                        {
                            // update z-buffer
                            z_ptr[xi] = zi;           
                            // write pixel
                            screen_ptr[xi] = textel;
                        } // end if


                        // interpolate u,v,z
                        ui+=du;
                        vi+=dv;
                        zi+=dz;
                    } // end for xi

                    // interpolate u,v,x along right and left edge
                    xl+=dxdyl;
                    ul+=dudyl;
                    vl+=dvdyl;
                    zl+=dzdyl;

                    xr+=dxdyr;
                    ur+=dudyr;
                    vr+=dvdyr;
                    zr+=dzdyr;

                    // advance screen ptr
                    screen_ptr+=mem_pitch;

                    // advance zbuffer ptr
                    z_ptr+=zpitch;

                } // end for y

            } // end if clip
            else
            {
                // non-clip version

                // point screen ptr to starting line
                screen_ptr = dest_buffer + (ystart * mem_pitch);

                // point zbuffer to starting line
                z_ptr = zbuffer + (ystart * zpitch);

                for (yi = ystart; yi < yend; yi++)
                {
                    // compute span endpoints
                    xstart = ((xl + FIXP16_ROUND_UP) >> FIXP16_SHIFT);
                    xend   = ((xr + FIXP16_ROUND_UP) >> FIXP16_SHIFT);

                    // compute starting points for u,v interpolants
                    ui = ul + FIXP16_ROUND_UP;
                    vi = vl + FIXP16_ROUND_UP;
                    zi = zl + FIXP16_ROUND_UP;

                    // compute u,v interpolants
                    if ((dx = (xend - xstart))>0)
                    {
                        du = (ur - ul)/dx;
                        dv = (vr - vl)/dx;
                        dz = (zr - zl)/dx;
                    } // end if
                    else
                    {
                        du = (ur - ul);
                        dv = (vr - vl);
                        dz = (zr - zl);
                    } // end else

                    // draw span
                    for (xi=xstart; xi < xend; xi++)
                    {
                        // write thru the z buffer always
                        // write textel
                        // test for transparent pixel
                        textel = textmap[(ui >> FIXP16_SHIFT) + ((vi >> FIXP16_SHIFT) << texture_shift2)];

                        if (textel)
                        {
                            // update z-buffer
                            z_ptr[xi] = zi;           
                            // write pixel
                            screen_ptr[xi] = textel;
                        } // end if

                        // interpolate u,v,z
                        ui+=du;
                        vi+=dv;
                        zi+=dz;
                    } // end for xi

                    // interpolate u,v,x along right and left edge
                    xl+=dxdyl;
                    ul+=dudyl;
                    vl+=dvdyl;
                    zl+=dzdyl;

                    xr+=dxdyr;
                    ur+=dudyr;
                    vr+=dvdyr;
                    zr+=dzdyr;

                    // advance screen ptr
                    screen_ptr+=mem_pitch;

                    // advance zbuffer ptr
                    z_ptr+=zpitch;

                } // end for y

            } // end if non-clipped

        } // end if
        else
            if (tri_type==TRI_TYPE_GENERAL)
            {

                // first test for bottom clip, always
                if ((yend = y2) > max_clip_y)
                    yend = max_clip_y;

                // pre-test y clipping status
                if (y1 < min_clip_y)
                {
                    // compute all deltas
                    // LHS
                    dyl = (y2 - y1);

                    dxdyl = ((x2  - x1)  << FIXP16_SHIFT)/dyl;
                    dudyl = ((tu2 - tu1) << FIXP16_SHIFT)/dyl;  
                    dvdyl = ((tv2 - tv1) << FIXP16_SHIFT)/dyl;    
                    dzdyl = ((tz2 - tz1) << FIXP16_SHIFT)/dyl;  

                    // RHS
                    dyr = (y2 - y0);	

                    dxdyr = ((x2  - x0)  << FIXP16_SHIFT)/dyr;
                    dudyr = ((tu2 - tu0) << FIXP16_SHIFT)/dyr;  
                    dvdyr = ((tv2 - tv0) << FIXP16_SHIFT)/dyr;   
                    dzdyr = ((tz2 - tz0) << FIXP16_SHIFT)/dyr;   

                    // compute overclip
                    dyr = (min_clip_y - y0);
                    dyl = (min_clip_y - y1);

                    // computer new LHS starting values
                    xl = dxdyl*dyl + (x1  << FIXP16_SHIFT);
                    ul = dudyl*dyl + (tu1 << FIXP16_SHIFT);
                    vl = dvdyl*dyl + (tv1 << FIXP16_SHIFT);
                    zl = dzdyl*dyl + (tz1 << FIXP16_SHIFT);

                    // compute new RHS starting values
                    xr = dxdyr*dyr + (x0  << FIXP16_SHIFT);
                    ur = dudyr*dyr + (tu0 << FIXP16_SHIFT);
                    vr = dvdyr*dyr + (tv0 << FIXP16_SHIFT);
                    zr = dzdyr*dyr + (tz0 << FIXP16_SHIFT);

                    // compute new starting y
                    ystart = min_clip_y;

                    // test if we need swap to keep rendering left to right
                    if (dxdyr > dxdyl)
                    {
                        SWAP(dxdyl,dxdyr,temp);
                        SWAP(dudyl,dudyr,temp);
                        SWAP(dvdyl,dvdyr,temp);
                        SWAP(dzdyl,dzdyr,temp);
                        SWAP(xl,xr,temp);
                        SWAP(ul,ur,temp);
                        SWAP(vl,vr,temp);
                        SWAP(zl,zr,temp);
                        SWAP(x1,x2,temp);
                        SWAP(y1,y2,temp);
                        SWAP(tu1,tu2,temp);
                        SWAP(tv1,tv2,temp);
                        SWAP(tz1,tz2,temp);

                        // set interpolation restart
                        irestart = INTERP_RHS;

                    } // end if

                } // end if
                else
                    if (y0 < min_clip_y)
                    {
                        // compute all deltas
                        // LHS
                        dyl = (y1 - y0);

                        dxdyl = ((x1  - x0)  << FIXP16_SHIFT)/dyl;
                        dudyl = ((tu1 - tu0) << FIXP16_SHIFT)/dyl;  
                        dvdyl = ((tv1 - tv0) << FIXP16_SHIFT)/dyl;    
                        dzdyl = ((tz1 - tz0) << FIXP16_SHIFT)/dyl;  

                        // RHS
                        dyr = (y2 - y0);	

                        dxdyr = ((x2  - x0)  << FIXP16_SHIFT)/dyr;
                        dudyr = ((tu2 - tu0) << FIXP16_SHIFT)/dyr;  
                        dvdyr = ((tv2 - tv0) << FIXP16_SHIFT)/dyr;   
                        dzdyr = ((tz2 - tz0) << FIXP16_SHIFT)/dyr;   

                        // compute overclip
                        dy = (min_clip_y - y0);

                        // computer new LHS starting values
                        xl = dxdyl*dy + (x0  << FIXP16_SHIFT);
                        ul = dudyl*dy + (tu0 << FIXP16_SHIFT);
                        vl = dvdyl*dy + (tv0 << FIXP16_SHIFT);
                        zl = dzdyl*dy + (tz0 << FIXP16_SHIFT);

                        // compute new RHS starting values
                        xr = dxdyr*dy + (x0  << FIXP16_SHIFT);
                        ur = dudyr*dy + (tu0 << FIXP16_SHIFT);
                        vr = dvdyr*dy + (tv0 << FIXP16_SHIFT);
                        zr = dzdyr*dy + (tz0 << FIXP16_SHIFT);

                        // compute new starting y
                        ystart = min_clip_y;

                        // test if we need swap to keep rendering left to right
                        if (dxdyr < dxdyl)
                        {
                            SWAP(dxdyl,dxdyr,temp);
                            SWAP(dudyl,dudyr,temp);
                            SWAP(dvdyl,dvdyr,temp);
                            SWAP(dzdyl,dzdyr,temp);
                            SWAP(xl,xr,temp);
                            SWAP(ul,ur,temp);
                            SWAP(vl,vr,temp);
                            SWAP(zl,zr,temp);
                            SWAP(x1,x2,temp);
                            SWAP(y1,y2,temp);
                            SWAP(tu1,tu2,temp);
                            SWAP(tv1,tv2,temp);
                            SWAP(tz1,tz2,temp);

                            // set interpolation restart
                            irestart = INTERP_RHS;

                        } // end if

                    } // end if
                    else
                    {
                        // no initial y clipping

                        // compute all deltas
                        // LHS
                        dyl = (y1 - y0);

                        dxdyl = ((x1  - x0)  << FIXP16_SHIFT)/dyl;
                        dudyl = ((tu1 - tu0) << FIXP16_SHIFT)/dyl;  
                        dvdyl = ((tv1 - tv0) << FIXP16_SHIFT)/dyl;    
                        dzdyl = ((tz1 - tz0) << FIXP16_SHIFT)/dyl;   

                        // RHS
                        dyr = (y2 - y0);	

                        dxdyr = ((x2 - x0)   << FIXP16_SHIFT)/dyr;
                        dudyr = ((tu2 - tu0) << FIXP16_SHIFT)/dyr;  
                        dvdyr = ((tv2 - tv0) << FIXP16_SHIFT)/dyr;   		
                        dzdyr = ((tz2 - tz0) << FIXP16_SHIFT)/dyr;  

                        // no clipping y

                        // set starting values
                        xl = (x0 << FIXP16_SHIFT);
                        xr = (x0 << FIXP16_SHIFT);

                        ul = (tu0 << FIXP16_SHIFT);
                        vl = (tv0 << FIXP16_SHIFT);
                        zl = (tz0 << FIXP16_SHIFT);

                        ur = (tu0 << FIXP16_SHIFT);
                        vr = (tv0 << FIXP16_SHIFT);
                        zr = (tz0 << FIXP16_SHIFT);

                        // set starting y
                        ystart = y0;

                        // test if we need swap to keep rendering left to right
                        if (dxdyr < dxdyl)
                        {
                            SWAP(dxdyl,dxdyr,temp);
                            SWAP(dudyl,dudyr,temp);
                            SWAP(dvdyl,dvdyr,temp);
                            SWAP(dzdyl,dzdyr,temp);
                            SWAP(xl,xr,temp);
                            SWAP(ul,ur,temp);
                            SWAP(vl,vr,temp);
                            SWAP(zl,zr,temp);
                            SWAP(x1,x2,temp);
                            SWAP(y1,y2,temp);
                            SWAP(tu1,tu2,temp);
                            SWAP(tv1,tv2,temp);
                            SWAP(tz1,tz2,temp);

                            // set interpolation restart
                            irestart = INTERP_RHS;

                        } // end if

                    } // end else

                    // test for horizontal clipping
                    if ((x0 < min_clip_x) || (x0 > max_clip_x) ||
                        (x1 < min_clip_x) || (x1 > max_clip_x) ||
                        (x2 < min_clip_x) || (x2 > max_clip_x))
                    {
                        // clip version
                        // x clipping	

                        // point screen ptr to starting line
                        screen_ptr = dest_buffer + (ystart * mem_pitch);

                        // point zbuffer to starting line
                        z_ptr = zbuffer + (ystart * zpitch);

                        for (yi = ystart; yi < yend; yi++)
                        {
                            // compute span endpoints
                            xstart = ((xl + FIXP16_ROUND_UP) >> FIXP16_SHIFT);
                            xend   = ((xr + FIXP16_ROUND_UP) >> FIXP16_SHIFT);

                            // compute starting points for u,v interpolants
                            ui = ul + FIXP16_ROUND_UP;
                            vi = vl + FIXP16_ROUND_UP;
                            zi = zl + FIXP16_ROUND_UP;

                            // compute u,v interpolants
                            if ((dx = (xend - xstart))>0)
                            {
                                du = (ur - ul)/dx;
                                dv = (vr - vl)/dx;
                                dz = (zr - zl)/dx;
                            } // end if
                            else
                            {
                                du = (ur - ul);
                                dv = (vr - vl);
                                dz = (zr - zl);
                            } // end else

                            ///////////////////////////////////////////////////////////////////////

                            // test for x clipping, LHS
                            if (xstart < min_clip_x)
                            {
                                // compute x overlap
                                dx = min_clip_x - xstart;

                                // slide interpolants over
                                ui+=dx*du;
                                vi+=dx*dv;
                                zi+=dx*dz;

                                // set x to left clip edge
                                xstart = min_clip_x;

                            } // end if

                            // test for x clipping RHS
                            if (xend > max_clip_x)
                                xend = max_clip_x;

                            ///////////////////////////////////////////////////////////////////////

                            // draw span
                            for (xi=xstart; xi < xend; xi++)
                            {
                                // write thru the z buffer always
                                // write textel
                                // test for transparent pixel
                                textel = textmap[(ui >> FIXP16_SHIFT) + ((vi >> FIXP16_SHIFT) << texture_shift2)];

                                if (textel)
                                {
                                    // update z-buffer
                                    z_ptr[xi] = zi;           
                                    // write pixel
                                    screen_ptr[xi] = textel;
                                } // end if

                                // interpolate u,v,z
                                ui+=du;
                                vi+=dv;
                                zi+=dz;
                            } // end for xi

                            // interpolate u,v,x along right and left edge
                            xl+=dxdyl;
                            ul+=dudyl;
                            vl+=dvdyl;
                            zl+=dzdyl;

                            xr+=dxdyr;
                            ur+=dudyr;
                            vr+=dvdyr;
                            zr+=dzdyr;

                            // advance screen ptr
                            screen_ptr+=mem_pitch;

                            // advance zbuffer ptr
                            z_ptr+=zpitch;

                            // test for yi hitting second region, if so change interpolant
                            if (yi==yrestart)
                            {
                                // test interpolation side change flag

                                if (irestart == INTERP_LHS)
                                {
                                    // LHS
                                    dyl = (y2 - y1);	

                                    dxdyl = ((x2 - x1)   << FIXP16_SHIFT)/dyl;
                                    dudyl = ((tu2 - tu1) << FIXP16_SHIFT)/dyl;  
                                    dvdyl = ((tv2 - tv1) << FIXP16_SHIFT)/dyl;   		
                                    dzdyl = ((tz2 - tz1) << FIXP16_SHIFT)/dyl;   

                                    // set starting values
                                    xl = (x1  << FIXP16_SHIFT);
                                    ul = (tu1 << FIXP16_SHIFT);
                                    vl = (tv1 << FIXP16_SHIFT);
                                    zl = (tz1 << FIXP16_SHIFT);

                                    // interpolate down on LHS to even up
                                    xl+=dxdyl;
                                    ul+=dudyl;
                                    vl+=dvdyl;
                                    zl+=dzdyl;
                                } // end if
                                else
                                {
                                    // RHS
                                    dyr = (y1 - y2);	

                                    dxdyr = ((x1 - x2)   << FIXP16_SHIFT)/dyr;
                                    dudyr = ((tu1 - tu2) << FIXP16_SHIFT)/dyr;  
                                    dvdyr = ((tv1 - tv2) << FIXP16_SHIFT)/dyr;   		
                                    dzdyr = ((tz1 - tz2) << FIXP16_SHIFT)/dyr;  

                                    // set starting values
                                    xr = (x2  << FIXP16_SHIFT);
                                    ur = (tu2 << FIXP16_SHIFT);
                                    vr = (tv2 << FIXP16_SHIFT);
                                    zr = (tz2 << FIXP16_SHIFT);

                                    // interpolate down on RHS to even up
                                    xr+=dxdyr;
                                    ur+=dudyr;
                                    vr+=dvdyr;
                                    zr+=dzdyr;

                                } // end else

                            } // end if

                        } // end for y

                    } // end if
                    else
                    {
                        // no x clipping
                        // point screen ptr to starting line
                        screen_ptr = dest_buffer + (ystart * mem_pitch);

                        // point zbuffer to starting line
                        z_ptr = zbuffer + (ystart * zpitch);

                        for (yi = ystart; yi < yend; yi++)
                        {
                            // compute span endpoints
                            xstart = ((xl + FIXP16_ROUND_UP) >> FIXP16_SHIFT);
                            xend   = ((xr + FIXP16_ROUND_UP) >> FIXP16_SHIFT);

                            // compute starting points for u,v interpolants
                            ui = ul + FIXP16_ROUND_UP;
                            vi = vl + FIXP16_ROUND_UP;
                            zi = zl + FIXP16_ROUND_UP;

                            // compute u,v interpolants
                            if ((dx = (xend - xstart))>0)
                            {
                                du = (ur - ul)/dx;
                                dv = (vr - vl)/dx;
                                dz = (zr - zl)/dx;
                            } // end if
                            else
                            {
                                du = (ur - ul);
                                dv = (vr - vl);
                                dz = (zr - zl);
                            } // end else

                            // draw span
                            for (xi=xstart; xi < xend; xi++)
                            {
                                // write thru the z buffer always
                                // write textel
                                // test for transparent pixel
                                textel = textmap[(ui >> FIXP16_SHIFT) + ((vi >> FIXP16_SHIFT) << texture_shift2)];

                                if (textel)
                                {
                                    // update z-buffer
                                    z_ptr[xi] = zi;           
                                    // write pixel
                                    screen_ptr[xi] = textel;
                                } // end if

                                // interpolate u,v
                                ui+=du;
                                vi+=dv;
                                zi+=dz;
                            } // end for xi

                            // interpolate u,v,x along right and left edge
                            xl+=dxdyl;
                            ul+=dudyl;
                            vl+=dvdyl;
                            zl+=dzdyl;

                            xr+=dxdyr;
                            ur+=dudyr;
                            vr+=dvdyr;
                            zr+=dzdyr;

                            // advance screen ptr
                            screen_ptr+=mem_pitch;

                            // advance zbuffer ptr
                            z_ptr+=zpitch;

                            // test for yi hitting second region, if so change interpolant
                            if (yi==yrestart)
                            {
                                // test interpolation side change flag

                                if (irestart == INTERP_LHS)
                                {
                                    // LHS
                                    dyl = (y2 - y1);	

                                    dxdyl = ((x2 - x1)   << FIXP16_SHIFT)/dyl;
                                    dudyl = ((tu2 - tu1) << FIXP16_SHIFT)/dyl;  
                                    dvdyl = ((tv2 - tv1) << FIXP16_SHIFT)/dyl;   		
                                    dzdyl = ((tz2 - tz1) << FIXP16_SHIFT)/dyl;   

                                    // set starting values
                                    xl = (x1  << FIXP16_SHIFT);
                                    ul = (tu1 << FIXP16_SHIFT);
                                    vl = (tv1 << FIXP16_SHIFT);
                                    zl = (tz1 << FIXP16_SHIFT);

                                    // interpolate down on LHS to even up
                                    xl+=dxdyl;
                                    ul+=dudyl;
                                    vl+=dvdyl;
                                    zl+=dzdyl;
                                } // end if
                                else
                                {
                                    // RHS
                                    dyr = (y1 - y2);	

                                    dxdyr = ((x1 - x2)   << FIXP16_SHIFT)/dyr;
                                    dudyr = ((tu1 - tu2) << FIXP16_SHIFT)/dyr;  
                                    dvdyr = ((tv1 - tv2) << FIXP16_SHIFT)/dyr;   		
                                    dzdyr = ((tz1 - tz2) << FIXP16_SHIFT)/dyr; 

                                    // set starting values
                                    xr = (x2  << FIXP16_SHIFT);
                                    ur = (tu2 << FIXP16_SHIFT);
                                    vr = (tv2 << FIXP16_SHIFT);
                                    zr = (tz2 << FIXP16_SHIFT);

                                    // interpolate down on RHS to even up
                                    xr+=dxdyr;
                                    ur+=dudyr;
                                    vr+=dvdyr;
                                    zr+=dzdyr;

                                } // end else

                            } // end if

                        } // end for y

                    } // end else	

            } // end if

} // end Draw_Textured_TriangleWTZB3_16

///////////////////////////////////////////////////////////////////////////////

void Draw_Textured_TriangleFSWTZB3_16(POLYF4DV2_PTR face, // ptr to face
                                      UCHAR *_dest_buffer,  // pointer to video buffer
                                      int mem_pitch,        // bytes per line, 320, 640 etc.
                                      UCHAR *_zbuffer,       // pointer to z-buffer
                                      int zpitch)          // bytes per line of zbuffer
{
    // this function draws a textured triangle in 16-bit mode with flat shading

    int v0=0,
        v1=1,
        v2=2,
        temp=0,
        tri_type = TRI_TYPE_NONE,
        irestart = INTERP_LHS;

    int dx,dy,dyl,dyr,      // general deltas
        u,v,z,
        du,dv,dz,
        xi,yi,              // the current interpolated x,y
        ui,vi,zi,            // the current interpolated u,v,z
        index_x,index_y,    // looping vars
        x,y,                // hold general x,y
        xstart,
        xend,
        ystart,
        yrestart,
        yend,
        xl,                 
        dxdyl,              
        xr,
        dxdyr,             
        dudyl,    
        ul,
        dzdyl,    
        zl,
        dvdyl,   
        vl,
        dudyr,
        ur,
        dvdyr,
        vr,
        dzdyr,
        zr;

    USHORT r_base, g_base, b_base,
        r_textel, g_textel, b_textel, textel;

    int x0,y0,tu0,tv0,tz0,    // cached vertices
        x1,y1,tu1,tv1,tz1,
        x2,y2,tu2,tv2,tz2;

    USHORT *screen_ptr  = NULL,
        *screen_line = NULL,
        *textmap     = NULL,
        *dest_buffer = (USHORT *)_dest_buffer;

    UINT  *z_ptr = NULL,
        *zbuffer = (UINT *)_zbuffer;


#ifdef DEBUG_ON
    // track rendering stats
    debug_polys_rendered_per_frame++;
#endif

    // extract texture map
    textmap = (USHORT *)face->texture->buffer;

    // extract base 2 of texture width
    int texture_shift2 = logbase2ofx[face->texture->width];

    // adjust memory pitch to words, divide by 2
    mem_pitch >>=1;

    // adjust zbuffer pitch for 32 bit alignment
    zpitch >>= 2;

    // apply fill convention to coordinates
    face->tvlist[0].x = (int)(face->tvlist[0].x+0.5);
    face->tvlist[0].y = (int)(face->tvlist[0].y+0.5);

    face->tvlist[1].x = (int)(face->tvlist[1].x+0.5);
    face->tvlist[1].y = (int)(face->tvlist[1].y+0.5);

    face->tvlist[2].x = (int)(face->tvlist[2].x+0.5);
    face->tvlist[2].y = (int)(face->tvlist[2].y+0.5);

    // first trivial clipping rejection tests 
    if (((face->tvlist[0].y < min_clip_y)  && 
        (face->tvlist[1].y < min_clip_y)  &&
        (face->tvlist[2].y < min_clip_y)) ||

        ((face->tvlist[0].y > max_clip_y)  && 
        (face->tvlist[1].y > max_clip_y)  &&
        (face->tvlist[2].y > max_clip_y)) ||

        ((face->tvlist[0].x < min_clip_x)  && 
        (face->tvlist[1].x < min_clip_x)  &&
        (face->tvlist[2].x < min_clip_x)) ||

        ((face->tvlist[0].x > max_clip_x)  && 
        (face->tvlist[1].x > max_clip_x)  &&
        (face->tvlist[2].x > max_clip_x)))
        return;


    // sort vertices
    if (face->tvlist[v1].y < face->tvlist[v0].y) 
    {SWAP(v0,v1,temp);} 

    if (face->tvlist[v2].y < face->tvlist[v0].y) 
    {SWAP(v0,v2,temp);}

    if (face->tvlist[v2].y < face->tvlist[v1].y) 
    {SWAP(v1,v2,temp);}

    // now test for trivial flat sided cases
    if (FCMP(face->tvlist[v0].y, face->tvlist[v1].y) )
    { 
        // set triangle type
        tri_type = TRI_TYPE_FLAT_TOP;

        // sort vertices left to right
        if (face->tvlist[v1].x < face->tvlist[v0].x) 
        {SWAP(v0,v1,temp);}

    } // end if
    else
        // now test for trivial flat sided cases
        if (FCMP( face->tvlist[v1].y, face->tvlist[v2].y) )
        { 
            // set triangle type
            tri_type = TRI_TYPE_FLAT_BOTTOM;

            // sort vertices left to right
            if (face->tvlist[v2].x < face->tvlist[v1].x) 
            {SWAP(v1,v2,temp);}

        } // end if
        else
        {
            // must be a general triangle
            tri_type = TRI_TYPE_GENERAL;

        } // end else

        // extract base color of lit poly, so we can modulate texture a bit
        // for lighting
        _RGB565FROM16BIT(face->lit_color[0], &r_base, &g_base, &b_base);

        // extract vertices for processing, now that we have order
        x0  = (int)(face->tvlist[v0].x+0.0);
        y0  = (int)(face->tvlist[v0].y+0.0);
        tu0 = (int)(face->tvlist[v0].u0);
        tv0 = (int)(face->tvlist[v0].v0);
        tz0 = (int)(face->tvlist[v0].z+0.5);

        x1  = (int)(face->tvlist[v1].x+0.0);
        y1  = (int)(face->tvlist[v1].y+0.0);
        tu1 = (int)(face->tvlist[v1].u0);
        tv1 = (int)(face->tvlist[v1].v0);
        tz1 = (int)(face->tvlist[v1].z+0.5);

        x2  = (int)(face->tvlist[v2].x+0.0);
        y2  = (int)(face->tvlist[v2].y+0.0);
        tu2 = (int)(face->tvlist[v2].u0);
        tv2 = (int)(face->tvlist[v2].v0);
        tz2 = (int)(face->tvlist[v2].z+0.5);

        // degenerate triangle
        if ( ((x0 == x1) && (x1 == x2)) || ((y0 ==  y1) && (y1 == y2)))
            return;

        // set interpolation restart value
        yrestart = y1;

        // what kind of triangle
        if (tri_type & TRI_TYPE_FLAT_MASK)
        {

            if (tri_type == TRI_TYPE_FLAT_TOP)
            {
                // compute all deltas
                dy = (y2 - y0);

                dxdyl = ((x2 - x0)   << FIXP16_SHIFT)/dy;
                dudyl = ((tu2 - tu0) << FIXP16_SHIFT)/dy;  
                dvdyl = ((tv2 - tv0) << FIXP16_SHIFT)/dy;    
                dzdyl = ((tz2 - tz0) << FIXP16_SHIFT)/dy;    

                dxdyr = ((x2 - x1)   << FIXP16_SHIFT)/dy;
                dudyr = ((tu2 - tu1) << FIXP16_SHIFT)/dy;  
                dvdyr = ((tv2 - tv1) << FIXP16_SHIFT)/dy;   
                dzdyr = ((tz2 - tz1) << FIXP16_SHIFT)/dy;   

                // test for y clipping
                if (y0 < min_clip_y)
                {
                    // compute overclip
                    dy = (min_clip_y - y0);

                    // computer new LHS starting values
                    xl = dxdyl*dy + (x0  << FIXP16_SHIFT);
                    ul = dudyl*dy + (tu0 << FIXP16_SHIFT);
                    vl = dvdyl*dy + (tv0 << FIXP16_SHIFT);
                    zl = dzdyl*dy + (tz0 << FIXP16_SHIFT);

                    // compute new RHS starting values
                    xr = dxdyr*dy + (x1  << FIXP16_SHIFT);
                    ur = dudyr*dy + (tu1 << FIXP16_SHIFT);
                    vr = dvdyr*dy + (tv1 << FIXP16_SHIFT);
                    zr = dzdyr*dy + (tz1 << FIXP16_SHIFT);

                    // compute new starting y
                    ystart = min_clip_y;

                } // end if
                else
                {
                    // no clipping

                    // set starting values
                    xl = (x0 << FIXP16_SHIFT);
                    xr = (x1 << FIXP16_SHIFT);

                    ul = (tu0 << FIXP16_SHIFT);
                    vl = (tv0 << FIXP16_SHIFT);
                    zl = (tz0 << FIXP16_SHIFT);

                    ur = (tu1 << FIXP16_SHIFT);
                    vr = (tv1 << FIXP16_SHIFT);
                    zr = (tz1 << FIXP16_SHIFT);

                    // set starting y
                    ystart = y0;

                } // end else

            } // end if flat top
            else
            {
                // must be flat bottom

                // compute all deltas
                dy = (y1 - y0);

                dxdyl = ((x1 - x0)   << FIXP16_SHIFT)/dy;
                dudyl = ((tu1 - tu0) << FIXP16_SHIFT)/dy;  
                dvdyl = ((tv1 - tv0) << FIXP16_SHIFT)/dy;    
                dzdyl = ((tz1 - tz0) << FIXP16_SHIFT)/dy; 

                dxdyr = ((x2 - x0)   << FIXP16_SHIFT)/dy;
                dudyr = ((tu2 - tu0) << FIXP16_SHIFT)/dy;  
                dvdyr = ((tv2 - tv0) << FIXP16_SHIFT)/dy;   
                dzdyr = ((tz2 - tz0) << FIXP16_SHIFT)/dy;   

                // test for y clipping
                if (y0 < min_clip_y)
                {
                    // compute overclip
                    dy = (min_clip_y - y0);

                    // computer new LHS starting values
                    xl = dxdyl*dy + (x0  << FIXP16_SHIFT);
                    ul = dudyl*dy + (tu0 << FIXP16_SHIFT);
                    vl = dvdyl*dy + (tv0 << FIXP16_SHIFT);
                    zl = dzdyl*dy + (tz0 << FIXP16_SHIFT);

                    // compute new RHS starting values
                    xr = dxdyr*dy + (x0  << FIXP16_SHIFT);
                    ur = dudyr*dy + (tu0 << FIXP16_SHIFT);
                    vr = dvdyr*dy + (tv0 << FIXP16_SHIFT);
                    zr = dzdyr*dy + (tz0 << FIXP16_SHIFT);

                    // compute new starting y
                    ystart = min_clip_y;

                } // end if
                else
                {
                    // no clipping

                    // set starting values
                    xl = (x0 << FIXP16_SHIFT);
                    xr = (x0 << FIXP16_SHIFT);

                    ul = (tu0 << FIXP16_SHIFT);
                    vl = (tv0 << FIXP16_SHIFT);
                    zl = (tz0 << FIXP16_SHIFT);

                    ur = (tu0 << FIXP16_SHIFT);
                    vr = (tv0 << FIXP16_SHIFT);
                    zr = (tz0 << FIXP16_SHIFT);

                    // set starting y
                    ystart = y0;

                } // end else	

            } // end else flat bottom

            // test for bottom clip, always
            if ((yend = y2) > max_clip_y)
                yend = max_clip_y;

            // test for horizontal clipping
            if ((x0 < min_clip_x) || (x0 > max_clip_x) ||
                (x1 < min_clip_x) || (x1 > max_clip_x) ||
                (x2 < min_clip_x) || (x2 > max_clip_x))
            {
                // clip version

                // point screen ptr to starting line
                screen_ptr = dest_buffer + (ystart * mem_pitch);

                // point zbuffer to starting line
                z_ptr = zbuffer + (ystart * zpitch);

                for (yi = ystart; yi < yend; yi++)
                {
                    // compute span endpoints
                    xstart = ((xl + FIXP16_ROUND_UP) >> FIXP16_SHIFT);
                    xend   = ((xr + FIXP16_ROUND_UP) >> FIXP16_SHIFT);

                    // compute starting points for u,v interpolants
                    ui = ul + FIXP16_ROUND_UP;
                    vi = vl + FIXP16_ROUND_UP;
                    zi = zl + FIXP16_ROUND_UP;

                    // compute u,v interpolants
                    if ((dx = (xend - xstart))>0)
                    {
                        du = (ur - ul)/dx;
                        dv = (vr - vl)/dx;
                        dz = (zr - zl)/dx;
                    } // end if
                    else
                    {
                        du = (ur - ul);
                        dv = (vr - vl);
                        dz = (zr - zl);
                    } // end else

                    ///////////////////////////////////////////////////////////////////////

                    // test for x clipping, LHS
                    if (xstart < min_clip_x)
                    {
                        // compute x overlap
                        dx = min_clip_x - xstart;

                        // slide interpolants over
                        ui+=dx*du;
                        vi+=dx*dv;
                        zi+=dx*dz;

                        // reset vars
                        xstart = min_clip_x;

                    } // end if

                    // test for x clipping RHS
                    if (xend > max_clip_x)
                        xend = max_clip_x;

                    ///////////////////////////////////////////////////////////////////////

                    // draw span
                    for (xi=xstart; xi < xend; xi++)
                    {
                        // write thru the z buffer always
                        // write textel

                        // test for transparent pixel
                        textel = textmap[(ui >> FIXP16_SHIFT) + ((vi >> FIXP16_SHIFT) << texture_shift2)];

                        if (textel)
                        {
                            // extract rgb components
                            r_textel  = ((textel >> 11)       ); 
                            g_textel  = ((textel >> 5)  & 0x3f); 
                            b_textel =   (textel        & 0x1f);

                            // modulate textel with lit background color
                            r_textel*=r_base; 
                            g_textel*=g_base;
                            b_textel*=b_base;

                            // finally write pixel, note that we did the math such that the results are r*32, g*64, b*32
                            // hence we need to divide the results by 32,64,32 respetively, BUT since we need to shift
                            // the results to fit into the destination 5.6.5 word, we can take advantage of the shifts
                            // and they all cancel out for the most part, but we will need logical anding, we will do
                            // it later when we optimize more...
                            screen_ptr[xi] = ((b_textel >> 5) + ((g_textel >> 6) << 5) + ((r_textel >> 5) << 11));

                            // update z-buffer
                            z_ptr[xi] = zi;                     

                        } // end if


                        // interpolate u,v,z
                        ui+=du;
                        vi+=dv;
                        zi+=dz;
                    } // end for xi

                    // interpolate u,v,x along right and left edge
                    xl+=dxdyl;
                    ul+=dudyl;
                    vl+=dvdyl;
                    zl+=dzdyl;

                    xr+=dxdyr;
                    ur+=dudyr;
                    vr+=dvdyr;
                    zr+=dzdyr;

                    // advance screen ptr
                    screen_ptr+=mem_pitch;

                    // advance zbuffer ptr
                    z_ptr+=zpitch;

                } // end for y

            } // end if clip
            else
            {
                // non-clip version

                // point screen ptr to starting line
                screen_ptr = dest_buffer + (ystart * mem_pitch);

                // point zbuffer to starting line
                z_ptr = zbuffer + (ystart * zpitch);

                for (yi = ystart; yi < yend; yi++)
                {
                    // compute span endpoints
                    xstart = ((xl + FIXP16_ROUND_UP) >> FIXP16_SHIFT);
                    xend   = ((xr + FIXP16_ROUND_UP) >> FIXP16_SHIFT);

                    // compute starting points for u,v interpolants
                    ui = ul + FIXP16_ROUND_UP;
                    vi = vl + FIXP16_ROUND_UP;
                    zi = zl + FIXP16_ROUND_UP;

                    // compute u,v interpolants
                    if ((dx = (xend - xstart))>0)
                    {
                        du = (ur - ul)/dx;
                        dv = (vr - vl)/dx;
                        dz = (zr - zl)/dx;
                    } // end if
                    else
                    {
                        du = (ur - ul);
                        dv = (vr - vl);
                        dz = (zr - zl);
                    } // end else

                    // draw span
                    for (xi=xstart; xi < xend; xi++)
                    {
                        // write thru the z buffer always
                        // write textel
                        // test for transparent pixel
                        textel = textmap[(ui >> FIXP16_SHIFT) + ((vi >> FIXP16_SHIFT) << texture_shift2)];

                        if (textel)
                        {
                            // extract rgb components
                            r_textel  = ((textel >> 11)       ); 
                            g_textel  = ((textel >> 5)  & 0x3f); 
                            b_textel =   (textel        & 0x1f);

                            // modulate textel with lit background color
                            r_textel*=r_base; 
                            g_textel*=g_base;
                            b_textel*=b_base;

                            // finally write pixel, note that we did the math such that the results are r*32, g*64, b*32
                            // hence we need to divide the results by 32,64,32 respetively, BUT since we need to shift
                            // the results to fit into the destination 5.6.5 word, we can take advantage of the shifts
                            // and they all cancel out for the most part, but we will need logical anding, we will do
                            // it later when we optimize more...
                            screen_ptr[xi] = ((b_textel >> 5) + ((g_textel >> 6) << 5) + ((r_textel >> 5) << 11));

                            // update z-buffer
                            z_ptr[xi] = zi;                     
                        } // end if


                        // interpolate u,v,z
                        ui+=du;
                        vi+=dv;
                        zi+=dz;
                    } // end for xi

                    // interpolate u,v,x along right and left edge
                    xl+=dxdyl;
                    ul+=dudyl;
                    vl+=dvdyl;
                    zl+=dzdyl;

                    xr+=dxdyr;
                    ur+=dudyr;
                    vr+=dvdyr;
                    zr+=dzdyr;

                    // advance screen ptr
                    screen_ptr+=mem_pitch;

                    // advance zbuffer ptr
                    z_ptr+=zpitch;

                } // end for y

            } // end if non-clipped

        } // end if
        else
            if (tri_type==TRI_TYPE_GENERAL)
            {

                // first test for bottom clip, always
                if ((yend = y2) > max_clip_y)
                    yend = max_clip_y;

                // pre-test y clipping status
                if (y1 < min_clip_y)
                {
                    // compute all deltas
                    // LHS
                    dyl = (y2 - y1);

                    dxdyl = ((x2  - x1)  << FIXP16_SHIFT)/dyl;
                    dudyl = ((tu2 - tu1) << FIXP16_SHIFT)/dyl;  
                    dvdyl = ((tv2 - tv1) << FIXP16_SHIFT)/dyl;    
                    dzdyl = ((tz2 - tz1) << FIXP16_SHIFT)/dyl;    

                    // RHS
                    dyr = (y2 - y0);	

                    dxdyr = ((x2  - x0)  << FIXP16_SHIFT)/dyr;
                    dudyr = ((tu2 - tu0) << FIXP16_SHIFT)/dyr;  
                    dvdyr = ((tv2 - tv0) << FIXP16_SHIFT)/dyr;   
                    dzdyr = ((tz2 - tz0) << FIXP16_SHIFT)/dyr;   		

                    // compute overclip
                    dyr = (min_clip_y - y0);
                    dyl = (min_clip_y - y1);

                    // computer new LHS starting values
                    xl = dxdyl*dyl + (x1  << FIXP16_SHIFT);
                    ul = dudyl*dyl + (tu1 << FIXP16_SHIFT);
                    vl = dvdyl*dyl + (tv1 << FIXP16_SHIFT);
                    zl = dzdyl*dyl + (tz1 << FIXP16_SHIFT);

                    // compute new RHS starting values
                    xr = dxdyr*dyr + (x0  << FIXP16_SHIFT);
                    ur = dudyr*dyr + (tu0 << FIXP16_SHIFT);
                    vr = dvdyr*dyr + (tv0 << FIXP16_SHIFT);
                    zr = dzdyr*dyr + (tz0 << FIXP16_SHIFT);

                    // compute new starting y
                    ystart = min_clip_y;

                    // test if we need swap to keep rendering left to right
                    if (dxdyr > dxdyl)
                    {
                        SWAP(dxdyl,dxdyr,temp);
                        SWAP(dudyl,dudyr,temp);
                        SWAP(dvdyl,dvdyr,temp);
                        SWAP(dzdyl,dzdyr,temp);
                        SWAP(xl,xr,temp);
                        SWAP(ul,ur,temp);
                        SWAP(vl,vr,temp);
                        SWAP(zl,zr,temp);
                        SWAP(x1,x2,temp);
                        SWAP(y1,y2,temp);
                        SWAP(tu1,tu2,temp);
                        SWAP(tv1,tv2,temp);
                        SWAP(tz1,tz2,temp);

                        // set interpolation restart
                        irestart = INTERP_RHS;

                    } // end if

                } // end if
                else
                    if (y0 < min_clip_y)
                    {
                        // compute all deltas
                        // LHS
                        dyl = (y1 - y0);

                        dxdyl = ((x1  - x0)  << FIXP16_SHIFT)/dyl;
                        dudyl = ((tu1 - tu0) << FIXP16_SHIFT)/dyl;  
                        dvdyl = ((tv1 - tv0) << FIXP16_SHIFT)/dyl;    
                        dzdyl = ((tz1 - tz0) << FIXP16_SHIFT)/dyl;    

                        // RHS
                        dyr = (y2 - y0);	

                        dxdyr = ((x2  - x0)  << FIXP16_SHIFT)/dyr;
                        dudyr = ((tu2 - tu0) << FIXP16_SHIFT)/dyr;  
                        dvdyr = ((tv2 - tv0) << FIXP16_SHIFT)/dyr;   
                        dzdyr = ((tz2 - tz0) << FIXP16_SHIFT)/dyr;   

                        // compute overclip
                        dy = (min_clip_y - y0);

                        // computer new LHS starting values
                        xl = dxdyl*dy + (x0  << FIXP16_SHIFT);
                        ul = dudyl*dy + (tu0 << FIXP16_SHIFT);
                        vl = dvdyl*dy + (tv0 << FIXP16_SHIFT);
                        zl = dzdyl*dy + (tz0 << FIXP16_SHIFT);

                        // compute new RHS starting values
                        xr = dxdyr*dy + (x0  << FIXP16_SHIFT);
                        ur = dudyr*dy + (tu0 << FIXP16_SHIFT);
                        vr = dvdyr*dy + (tv0 << FIXP16_SHIFT);
                        zr = dzdyr*dy + (tz0 << FIXP16_SHIFT);

                        // compute new starting y
                        ystart = min_clip_y;

                        // test if we need swap to keep rendering left to right
                        if (dxdyr < dxdyl)
                        {
                            SWAP(dxdyl,dxdyr,temp);
                            SWAP(dudyl,dudyr,temp);
                            SWAP(dvdyl,dvdyr,temp);
                            SWAP(dzdyl,dzdyr,temp);
                            SWAP(xl,xr,temp);
                            SWAP(ul,ur,temp);
                            SWAP(vl,vr,temp);
                            SWAP(zl,zr,temp);
                            SWAP(x1,x2,temp);
                            SWAP(y1,y2,temp);
                            SWAP(tu1,tu2,temp);
                            SWAP(tv1,tv2,temp);
                            SWAP(tz1,tz2,temp);

                            // set interpolation restart
                            irestart = INTERP_RHS;

                        } // end if

                    } // end if
                    else
                    {
                        // no initial y clipping

                        // compute all deltas
                        // LHS
                        dyl = (y1 - y0);

                        dxdyl = ((x1  - x0)  << FIXP16_SHIFT)/dyl;
                        dudyl = ((tu1 - tu0) << FIXP16_SHIFT)/dyl;  
                        dvdyl = ((tv1 - tv0) << FIXP16_SHIFT)/dyl;    
                        dzdyl = ((tz1 - tz0) << FIXP16_SHIFT)/dyl;    

                        // RHS
                        dyr = (y2 - y0);	

                        dxdyr = ((x2 - x0)   << FIXP16_SHIFT)/dyr;
                        dudyr = ((tu2 - tu0) << FIXP16_SHIFT)/dyr;  
                        dvdyr = ((tv2 - tv0) << FIXP16_SHIFT)/dyr;   		
                        dzdyr = ((tz2 - tz0) << FIXP16_SHIFT)/dyr;   		

                        // no clipping y

                        // set starting values
                        xl = (x0 << FIXP16_SHIFT);
                        xr = (x0 << FIXP16_SHIFT);

                        ul = (tu0 << FIXP16_SHIFT);
                        vl = (tv0 << FIXP16_SHIFT);
                        zl = (tz0 << FIXP16_SHIFT);

                        ur = (tu0 << FIXP16_SHIFT);
                        vr = (tv0 << FIXP16_SHIFT);
                        zr = (tz0 << FIXP16_SHIFT);

                        // set starting y
                        ystart = y0;

                        // test if we need swap to keep rendering left to right
                        if (dxdyr < dxdyl)
                        {
                            SWAP(dxdyl,dxdyr,temp);
                            SWAP(dudyl,dudyr,temp);
                            SWAP(dvdyl,dvdyr,temp);
                            SWAP(dzdyl,dzdyr,temp);
                            SWAP(xl,xr,temp);
                            SWAP(ul,ur,temp);
                            SWAP(vl,vr,temp);
                            SWAP(zl,zr,temp);
                            SWAP(x1,x2,temp);
                            SWAP(y1,y2,temp);
                            SWAP(tu1,tu2,temp);
                            SWAP(tv1,tv2,temp);
                            SWAP(tz1,tz2,temp);

                            // set interpolation restart
                            irestart = INTERP_RHS;

                        } // end if

                    } // end else


                    // test for horizontal clipping
                    if ((x0 < min_clip_x) || (x0 > max_clip_x) ||
                        (x1 < min_clip_x) || (x1 > max_clip_x) ||
                        (x2 < min_clip_x) || (x2 > max_clip_x))
                    {
                        // clip version
                        // x clipping	

                        // point screen ptr to starting line
                        screen_ptr = dest_buffer + (ystart * mem_pitch);

                        // point zbuffer to starting line
                        z_ptr = zbuffer + (ystart * zpitch);

                        for (yi = ystart; yi < yend; yi++)
                        {
                            // compute span endpoints
                            xstart = ((xl + FIXP16_ROUND_UP) >> FIXP16_SHIFT);
                            xend   = ((xr + FIXP16_ROUND_UP) >> FIXP16_SHIFT);

                            // compute starting points for u,v interpolants
                            ui = ul + FIXP16_ROUND_UP;
                            vi = vl + FIXP16_ROUND_UP;
                            zi = zl + FIXP16_ROUND_UP;

                            // compute u,v interpolants
                            if ((dx = (xend - xstart))>0)
                            {
                                du = (ur - ul)/dx;
                                dv = (vr - vl)/dx;
                                dz = (zr - zl)/dx;
                            } // end if
                            else
                            {
                                du = (ur - ul);
                                dv = (vr - vl);
                                dz = (zr - zl);
                            } // end else

                            ///////////////////////////////////////////////////////////////////////

                            // test for x clipping, LHS
                            if (xstart < min_clip_x)
                            {
                                // compute x overlap
                                dx = min_clip_x - xstart;

                                // slide interpolants over
                                ui+=dx*du;
                                vi+=dx*dv;
                                zi+=dx*dz;

                                // set x to left clip edge
                                xstart = min_clip_x;

                            } // end if

                            // test for x clipping RHS
                            if (xend > max_clip_x)
                                xend = max_clip_x;

                            ///////////////////////////////////////////////////////////////////////

                            // draw span
                            for (xi=xstart; xi < xend; xi++)
                            {
                                // write thru the z buffer always
                                // write textel
                                // test for transparent pixel
                                textel = textmap[(ui >> FIXP16_SHIFT) + ((vi >> FIXP16_SHIFT) << texture_shift2)];

                                if (textel)
                                {
                                    // extract rgb components
                                    r_textel  = ((textel >> 11)       ); 
                                    g_textel  = ((textel >> 5)  & 0x3f); 
                                    b_textel =   (textel        & 0x1f);

                                    // modulate textel with lit background color
                                    r_textel*=r_base; 
                                    g_textel*=g_base;
                                    b_textel*=b_base;

                                    // finally write pixel, note that we did the math such that the results are r*32, g*64, b*32
                                    // hence we need to divide the results by 32,64,32 respetively, BUT since we need to shift
                                    // the results to fit into the destination 5.6.5 word, we can take advantage of the shifts
                                    // and they all cancel out for the most part, but we will need logical anding, we will do
                                    // it later when we optimize more...
                                    screen_ptr[xi] = ((b_textel >> 5) + ((g_textel >> 6) << 5) + ((r_textel >> 5) << 11));

                                    // update z-buffer
                                    z_ptr[xi] = zi;   
                                } // end if                  


                                // interpolate u,v
                                ui+=du;
                                vi+=dv;
                                zi+=dz;
                            } // end for xi

                            // interpolate u,v,x along right and left edge
                            xl+=dxdyl;
                            ul+=dudyl;
                            vl+=dvdyl;
                            zl+=dzdyl;

                            xr+=dxdyr;
                            ur+=dudyr;
                            vr+=dvdyr;
                            zr+=dzdyr;

                            // advance screen ptr
                            screen_ptr+=mem_pitch;

                            // advance zbuffer ptr
                            z_ptr+=zpitch;

                            // test for yi hitting second region, if so change interpolant
                            if (yi==yrestart)
                            {
                                // test interpolation side change flag

                                if (irestart == INTERP_LHS)
                                {
                                    // LHS
                                    dyl = (y2 - y1);	

                                    dxdyl = ((x2 - x1)   << FIXP16_SHIFT)/dyl;
                                    dudyl = ((tu2 - tu1) << FIXP16_SHIFT)/dyl;  
                                    dvdyl = ((tv2 - tv1) << FIXP16_SHIFT)/dyl;   		
                                    dzdyl = ((tz2 - tz1) << FIXP16_SHIFT)/dyl;  

                                    // set starting values
                                    xl = (x1  << FIXP16_SHIFT);
                                    ul = (tu1 << FIXP16_SHIFT);
                                    vl = (tv1 << FIXP16_SHIFT);
                                    zl = (tz1 << FIXP16_SHIFT);

                                    // interpolate down on LHS to even up
                                    xl+=dxdyl;
                                    ul+=dudyl;
                                    vl+=dvdyl;
                                    zl+=dzdyl;
                                } // end if
                                else
                                {
                                    // RHS
                                    dyr = (y1 - y2);	

                                    dxdyr = ((x1 - x2)   << FIXP16_SHIFT)/dyr;
                                    dudyr = ((tu1 - tu2) << FIXP16_SHIFT)/dyr;  
                                    dvdyr = ((tv1 - tv2) << FIXP16_SHIFT)/dyr;   		
                                    dzdyr = ((tz1 - tz2) << FIXP16_SHIFT)/dyr;   

                                    // set starting values
                                    xr = (x2  << FIXP16_SHIFT);
                                    ur = (tu2 << FIXP16_SHIFT);
                                    vr = (tv2 << FIXP16_SHIFT);
                                    zr = (tz2 << FIXP16_SHIFT);

                                    // interpolate down on RHS to even up
                                    xr+=dxdyr;
                                    ur+=dudyr;
                                    vr+=dvdyr;
                                    zr+=dzdyr;

                                } // end else


                            } // end if

                        } // end for y

                    } // end if
                    else
                    {
                        // no x clipping
                        // point screen ptr to starting line
                        screen_ptr = dest_buffer + (ystart * mem_pitch);

                        // point zbuffer to starting line
                        z_ptr = zbuffer + (ystart * zpitch);

                        for (yi = ystart; yi < yend; yi++)
                        {
                            // compute span endpoints
                            xstart = ((xl + FIXP16_ROUND_UP) >> FIXP16_SHIFT);
                            xend   = ((xr + FIXP16_ROUND_UP) >> FIXP16_SHIFT);

                            // compute starting points for u,v,z interpolants
                            ui = ul + FIXP16_ROUND_UP;
                            vi = vl + FIXP16_ROUND_UP;
                            zi = zl + FIXP16_ROUND_UP;

                            // compute u,v interpolants
                            if ((dx = (xend - xstart))>0)

                            {
                                du = (ur - ul)/dx;
                                dv = (vr - vl)/dx;
                                dz = (zr - zl)/dx;
                            } // end if
                            else
                            {
                                du = (ur - ul);
                                dv = (vr - vl);
                                dz = (zr - zl);
                            } // end else

                            // draw span
                            for (xi=xstart; xi < xend; xi++)
                            {
                                // write thru the z buffer always
                                // write textel
                                // test for transparent pixel
                                textel = textmap[(ui >> FIXP16_SHIFT) + ((vi >> FIXP16_SHIFT) << texture_shift2)];

                                if (textel)
                                {
                                    // extract rgb components
                                    r_textel  = ((textel >> 11)       ); 
                                    g_textel  = ((textel >> 5)  & 0x3f); 
                                    b_textel =   (textel        & 0x1f);

                                    // modulate textel with lit background color
                                    r_textel*=r_base; 
                                    g_textel*=g_base;
                                    b_textel*=b_base;

                                    // finally write pixel, note that we did the math such that the results are r*32, g*64, b*32
                                    // hence we need to divide the results by 32,64,32 respetively, BUT since we need to shift
                                    // the results to fit into the destination 5.6.5 word, we can take advantage of the shifts
                                    // and they all cancel out for the most part, but we will need logical anding, we will do
                                    // it later when we optimize more...
                                    screen_ptr[xi] = ((b_textel >> 5) + ((g_textel >> 6) << 5) + ((r_textel >> 5) << 11));

                                    // update z-buffer
                                    z_ptr[xi] = zi;   
                                } // end if    


                                // interpolate u,v,z
                                ui+=du;
                                vi+=dv;
                                zi+=dz;
                            } // end for xi

                            // interpolate u,v,x along right and left edge
                            xl+=dxdyl;
                            ul+=dudyl;
                            vl+=dvdyl;
                            zl+=dzdyl;

                            xr+=dxdyr;
                            ur+=dudyr;
                            vr+=dvdyr;
                            zr+=dzdyr;

                            // advance screen ptr
                            screen_ptr+=mem_pitch;

                            // advance zbuffer ptr
                            z_ptr+=zpitch;

                            // test for yi hitting second region, if so change interpolant
                            if (yi==yrestart)
                            {
                                // test interpolation side change flag

                                if (irestart == INTERP_LHS)
                                {
                                    // LHS
                                    dyl = (y2 - y1);	

                                    dxdyl = ((x2 - x1)   << FIXP16_SHIFT)/dyl;
                                    dudyl = ((tu2 - tu1) << FIXP16_SHIFT)/dyl;  
                                    dvdyl = ((tv2 - tv1) << FIXP16_SHIFT)/dyl;   		
                                    dzdyl = ((tz2 - tz1) << FIXP16_SHIFT)/dyl; 

                                    // set starting values
                                    xl = (x1  << FIXP16_SHIFT);
                                    ul = (tu1 << FIXP16_SHIFT);
                                    vl = (tv1 << FIXP16_SHIFT);
                                    zl = (tz1 << FIXP16_SHIFT);

                                    // interpolate down on LHS to even up
                                    xl+=dxdyl;
                                    ul+=dudyl;
                                    vl+=dvdyl;
                                    zl+=dzdyl;
                                } // end if
                                else
                                {
                                    // RHS
                                    dyr = (y1 - y2);	

                                    dxdyr = ((x1 - x2)   << FIXP16_SHIFT)/dyr;
                                    dudyr = ((tu1 - tu2) << FIXP16_SHIFT)/dyr;  
                                    dvdyr = ((tv1 - tv2) << FIXP16_SHIFT)/dyr;   		
                                    dzdyr = ((tz1 - tz2) << FIXP16_SHIFT)/dyr;   

                                    // set starting values
                                    xr = (x2  << FIXP16_SHIFT);
                                    ur = (tu2 << FIXP16_SHIFT);
                                    vr = (tv2 << FIXP16_SHIFT);
                                    zr = (tz2 << FIXP16_SHIFT);

                                    // interpolate down on RHS to even up
                                    xr+=dxdyr;
                                    ur+=dudyr;
                                    vr+=dvdyr;
                                    zr+=dzdyr;

                                } // end else

                            } // end if

                        } // end for y

                    } // end else	

            } // end if

} // end Draw_Textured_TriangleFSWTZB3_16

//////////////////////////////////////////////////////////////////////////////

void Draw_Textured_TriangleGSWTZB2_16(POLYF4DV2_PTR face,   // ptr to face
                                      UCHAR *_dest_buffer, // pointer to video buffer
                                      int mem_pitch,       // bytes per line, 320, 640 etc.
                                      UCHAR *_zbuffer,       // pointer to z-buffer
                                      int zpitch)          // bytes per line of zbuffer

{
    // this function draws a textured gouraud shaded polygon, and z bufferedbased on the affine texture mapper, 
    // we simply interpolate the (R,G,B) values across the polygons along with the texture coordinates
    // and then modulate to get the final color 

    int v0=0,
        v1=1,
        v2=2,
        temp=0,
        tri_type = TRI_TYPE_NONE,
        irestart = INTERP_LHS;


    int dx,dy,dyl,dyr,      // general deltas
        u,v,w,z, s,t,
        du,dv,dw,dz, ds, dt, 
        xi,yi,             // the current interpolated x,y
        ui,vi,wi,zi, si, ti,    // the current interpolated u,v
        index_x,index_y,    // looping vars
        x,y,                // hold general x,y
        xstart,
        xend,
        ystart,
        yrestart,
        yend,
        xl,                 
        dxdyl,              
        xr,
        dxdyr,             
        dudyl,    
        ul,
        dvdyl,   
        vl,
        dwdyl,   
        wl,
        dzdyl,   
        zl,
        dsdyl,    
        sl,
        dtdyl,   
        tl,
        dudyr,
        ur,
        dvdyr,
        vr,
        dwdyr,
        wr,
        dzdyr,
        zr,
        dsdyr,
        sr,
        dtdyr,
        tr;

    int x0,y0,tu0,tv0,tw0, tz0, ts0,tt0,    // cached vertices
        x1,y1,tu1,tv1,tw1, tz1, ts1,tt1,
        x2,y2,tu2,tv2,tw2, tz2, ts2,tt2;

    int r_base0, g_base0, b_base0,
        r_base1, g_base1, b_base1,
        r_base2, g_base2, b_base2;


    UINT r_textel, g_textel, b_textel;
    USHORT textel;

    USHORT *screen_ptr  = NULL,
        *screen_line = NULL,
        *textmap     = NULL,
        *dest_buffer = (USHORT *)_dest_buffer;

    UINT  *z_ptr = NULL,
        *zbuffer = (UINT *)_zbuffer;

#ifdef DEBUG_ON
    // track rendering stats
    debug_polys_rendered_per_frame++;
#endif


    // extract texture map
    textmap = (USHORT *)face->texture->buffer;

    // extract base 2 of texture width
    int texture_shift2 = logbase2ofx[face->texture->width];

    // adjust memory pitch to words, divide by 2
    mem_pitch >>=1;

    // adjust zbuffer pitch for 32 bit alignment
    zpitch >>= 2;

    // apply fill convention to coordinates
    face->tvlist[0].x = (int)(face->tvlist[0].x+0.0);
    face->tvlist[0].y = (int)(face->tvlist[0].y+0.0);

    face->tvlist[1].x = (int)(face->tvlist[1].x+0.0);
    face->tvlist[1].y = (int)(face->tvlist[1].y+0.0);

    face->tvlist[2].x = (int)(face->tvlist[2].x+0.0);
    face->tvlist[2].y = (int)(face->tvlist[2].y+0.0);

    // first trivial clipping rejection tests 
    if (((face->tvlist[0].y < min_clip_y)  && 
        (face->tvlist[1].y < min_clip_y)  &&
        (face->tvlist[2].y < min_clip_y)) ||

        ((face->tvlist[0].y > max_clip_y)  && 
        (face->tvlist[1].y > max_clip_y)  &&
        (face->tvlist[2].y > max_clip_y)) ||

        ((face->tvlist[0].x < min_clip_x)  && 
        (face->tvlist[1].x < min_clip_x)  &&
        (face->tvlist[2].x < min_clip_x)) ||

        ((face->tvlist[0].x > max_clip_x)  && 
        (face->tvlist[1].x > max_clip_x)  &&
        (face->tvlist[2].x > max_clip_x)))
        return;

    // sort vertices
    if (face->tvlist[v1].y < face->tvlist[v0].y) 
    {SWAP(v0,v1,temp);} 

    if (face->tvlist[v2].y < face->tvlist[v0].y) 
    {SWAP(v0,v2,temp);}

    if (face->tvlist[v2].y < face->tvlist[v1].y) 
    {SWAP(v1,v2,temp);}

    // now test for trivial flat sided cases
    if (FCMP(face->tvlist[v0].y, face->tvlist[v1].y) )
    { 
        // set triangle type
        tri_type = TRI_TYPE_FLAT_TOP;

        // sort vertices left to right
        if (face->tvlist[v1].x < face->tvlist[v0].x) 
        {SWAP(v0,v1,temp);}

    } // end if
    else
        // now test for trivial flat sided cases
        if (FCMP(face->tvlist[v1].y, face->tvlist[v2].y) )
        { 
            // set triangle type
            tri_type = TRI_TYPE_FLAT_BOTTOM;

            // sort vertices left to right
            if (face->tvlist[v2].x < face->tvlist[v1].x) 
            {SWAP(v1,v2,temp);}

        } // end if
        else
        {
            // must be a general triangle
            tri_type = TRI_TYPE_GENERAL;

        } // end else

        // assume 5.6.5 format -- sorry!
        // we can't afford a function call in the inner loops, so we must write 
        // two hard coded versions, if we want support for both 5.6.5, and 5.5.5
        _RGB565FROM16BIT(face->lit_color[v0], &r_base0, &g_base0, &b_base0);
        _RGB565FROM16BIT(face->lit_color[v1], &r_base1, &g_base1, &b_base1);
        _RGB565FROM16BIT(face->lit_color[v2], &r_base2, &g_base2, &b_base2);

        // scale to 8 bit 
        r_base0 <<= 3;
        g_base0 <<= 2;
        b_base0 <<= 3;

        // scale to 8 bit 
        r_base1 <<= 3;
        g_base1 <<= 2;
        b_base1 <<= 3;

        // scale to 8 bit 
        r_base2 <<= 3;
        g_base2 <<= 2;
        b_base2 <<= 3;

        // extract vertices for processing, now that we have order
        x0  = (int)(face->tvlist[v0].x+0.0);
        y0  = (int)(face->tvlist[v0].y+0.0);

        tz0 = (int)(face->tvlist[v0].z+0.5);
        ts0 = (int)(face->tvlist[v0].u0);
        tt0 = (int)(face->tvlist[v0].v0);

        tu0 = r_base0;
        tv0 = g_base0; 
        tw0 = b_base0; 

        x1  = (int)(face->tvlist[v1].x+0.0);
        y1  = (int)(face->tvlist[v1].y+0.0);

        tz1 = (int)(face->tvlist[v1].z+0.5);
        ts1 = (int)(face->tvlist[v1].u0);
        tt1 = (int)(face->tvlist[v1].v0);

        tu1 = r_base1;
        tv1 = g_base1; 
        tw1 = b_base1; 

        x2  = (int)(face->tvlist[v2].x+0.0);
        y2  = (int)(face->tvlist[v2].y+0.0);

        tz2 = (int)(face->tvlist[v2].z+0.5);
        ts2 = (int)(face->tvlist[v2].u0);
        tt2 = (int)(face->tvlist[v2].v0);

        tu2 = r_base2; 
        tv2 = g_base2; 
        tw2 = b_base2; 

        // degenerate triangle
        if ( ((x0 == x1) && (x1 == x2)) || ((y0 ==  y1) && (y1 == y2)))
            return;

        // set interpolation restart value
        yrestart = y1;

        // what kind of triangle
        if (tri_type & TRI_TYPE_FLAT_MASK)
        {

            if (tri_type == TRI_TYPE_FLAT_TOP)
            {
                // compute all deltas
                dy = (y2 - y0);

                dxdyl = ((x2 - x0)   << FIXP16_SHIFT)/dy;
                dudyl = ((tu2 - tu0) << FIXP16_SHIFT)/dy;  
                dvdyl = ((tv2 - tv0) << FIXP16_SHIFT)/dy;    
                dwdyl = ((tw2 - tw0) << FIXP16_SHIFT)/dy;  
                dzdyl = ((tz2 - tz0) << FIXP16_SHIFT)/dy; 

                dsdyl = ((ts2 - ts0) << FIXP16_SHIFT)/dy;    
                dtdyl = ((tt2 - tt0) << FIXP16_SHIFT)/dy;  

                dxdyr = ((x2 - x1)   << FIXP16_SHIFT)/dy;
                dudyr = ((tu2 - tu1) << FIXP16_SHIFT)/dy;  
                dvdyr = ((tv2 - tv1) << FIXP16_SHIFT)/dy;   
                dwdyr = ((tw2 - tw1) << FIXP16_SHIFT)/dy;   
                dzdyr = ((tz2 - tz1) << FIXP16_SHIFT)/dy;   

                dsdyr = ((ts2 - ts1) << FIXP16_SHIFT)/dy;   
                dtdyr = ((tt2 - tt1) << FIXP16_SHIFT)/dy;   

                // test for y clipping
                if (y0 < min_clip_y)
                {
                    // compute overclip
                    dy = (min_clip_y - y0);

                    // computer new LHS starting values
                    xl = dxdyl*dy + (x0  << FIXP16_SHIFT);
                    ul = dudyl*dy + (tu0 << FIXP16_SHIFT);
                    vl = dvdyl*dy + (tv0 << FIXP16_SHIFT);
                    wl = dwdyl*dy + (tw0 << FIXP16_SHIFT);
                    zl = dzdyl*dy + (tz0 << FIXP16_SHIFT);

                    sl = dsdyl*dy + (ts0 << FIXP16_SHIFT);
                    tl = dtdyl*dy + (tt0 << FIXP16_SHIFT);

                    // compute new RHS starting values
                    xr = dxdyr*dy + (x1  << FIXP16_SHIFT);
                    ur = dudyr*dy + (tu1 << FIXP16_SHIFT);
                    vr = dvdyr*dy + (tv1 << FIXP16_SHIFT);
                    wr = dwdyr*dy + (tw1 << FIXP16_SHIFT);
                    zr = dzdyr*dy + (tz1 << FIXP16_SHIFT);

                    sr = dsdyr*dy + (ts1 << FIXP16_SHIFT);
                    tr = dtdyr*dy + (tt1 << FIXP16_SHIFT);

                    // compute new starting y
                    ystart = min_clip_y;

                } // end if
                else
                {
                    // no clipping

                    // set starting values
                    xl = (x0 << FIXP16_SHIFT);
                    xr = (x1 << FIXP16_SHIFT);

                    ul = (tu0 << FIXP16_SHIFT);
                    vl = (tv0 << FIXP16_SHIFT);
                    wl = (tw0 << FIXP16_SHIFT);
                    zl = (tz0 << FIXP16_SHIFT);

                    sl = (ts0 << FIXP16_SHIFT);
                    tl = (tt0 << FIXP16_SHIFT);


                    ur = (tu1 << FIXP16_SHIFT);
                    vr = (tv1 << FIXP16_SHIFT);
                    wr = (tw1 << FIXP16_SHIFT);
                    zr = (tz1 << FIXP16_SHIFT);

                    sr = (ts1 << FIXP16_SHIFT);
                    tr = (tt1 << FIXP16_SHIFT);

                    // set starting y
                    ystart = y0;

                } // end else

            } // end if flat top
            else
            {
                // must be flat bottom

                // compute all deltas
                dy = (y1 - y0);

                dxdyl = ((x1 - x0)   << FIXP16_SHIFT)/dy;
                dudyl = ((tu1 - tu0) << FIXP16_SHIFT)/dy;  
                dvdyl = ((tv1 - tv0) << FIXP16_SHIFT)/dy;    
                dwdyl = ((tw1 - tw0) << FIXP16_SHIFT)/dy; 
                dzdyl = ((tz1 - tz0) << FIXP16_SHIFT)/dy; 

                dsdyl = ((ts1 - ts0) << FIXP16_SHIFT)/dy;    
                dtdyl = ((tt1 - tt0) << FIXP16_SHIFT)/dy; 

                dxdyr = ((x2 - x0)   << FIXP16_SHIFT)/dy;
                dudyr = ((tu2 - tu0) << FIXP16_SHIFT)/dy;  
                dvdyr = ((tv2 - tv0) << FIXP16_SHIFT)/dy;   
                dwdyr = ((tw2 - tw0) << FIXP16_SHIFT)/dy;   
                dzdyr = ((tz2 - tz0) << FIXP16_SHIFT)/dy;   

                dsdyr = ((ts2 - ts0) << FIXP16_SHIFT)/dy;   
                dtdyr = ((tt2 - tt0) << FIXP16_SHIFT)/dy;   


                // test for y clipping
                if (y0 < min_clip_y)
                {
                    // compute overclip
                    dy = (min_clip_y - y0);

                    // computer new LHS starting values
                    xl = dxdyl*dy + (x0  << FIXP16_SHIFT);
                    ul = dudyl*dy + (tu0 << FIXP16_SHIFT);
                    vl = dvdyl*dy + (tv0 << FIXP16_SHIFT);
                    wl = dwdyl*dy + (tw0 << FIXP16_SHIFT);
                    zl = dzdyl*dy + (tz0 << FIXP16_SHIFT);

                    sl = dsdyl*dy + (ts0 << FIXP16_SHIFT);
                    tl = dtdyl*dy + (tt0 << FIXP16_SHIFT);

                    // compute new RHS starting values
                    xr = dxdyr*dy + (x0  << FIXP16_SHIFT);
                    ur = dudyr*dy + (tu0 << FIXP16_SHIFT);
                    vr = dvdyr*dy + (tv0 << FIXP16_SHIFT);
                    wr = dwdyr*dy + (tw0 << FIXP16_SHIFT);
                    zr = dzdyr*dy + (tz0 << FIXP16_SHIFT);

                    sr = dsdyr*dy + (ts0 << FIXP16_SHIFT);
                    tr = dtdyr*dy + (tt0 << FIXP16_SHIFT);


                    // compute new starting y
                    ystart = min_clip_y;

                } // end if
                else
                {
                    // no clipping

                    // set starting values
                    xl = (x0 << FIXP16_SHIFT);
                    xr = (x0 << FIXP16_SHIFT);

                    ul = (tu0 << FIXP16_SHIFT);
                    vl = (tv0 << FIXP16_SHIFT);
                    wl = (tw0 << FIXP16_SHIFT);
                    zl = (tz0 << FIXP16_SHIFT);

                    sl = (ts0 << FIXP16_SHIFT);
                    tl = (tt0 << FIXP16_SHIFT);


                    ur = (tu0 << FIXP16_SHIFT);
                    vr = (tv0 << FIXP16_SHIFT);
                    wr = (tw0 << FIXP16_SHIFT);
                    zr = (tz0 << FIXP16_SHIFT);

                    sr = (ts0 << FIXP16_SHIFT);
                    tr = (tt0 << FIXP16_SHIFT);


                    // set starting y
                    ystart = y0;

                } // end else	

            } // end else flat bottom

            // test for bottom clip, always
            if ((yend = y2) > max_clip_y)
                yend = max_clip_y;

            // test for horizontal clipping
            if ((x0 < min_clip_x) || (x0 > max_clip_x) ||
                (x1 < min_clip_x) || (x1 > max_clip_x) ||
                (x2 < min_clip_x) || (x2 > max_clip_x))
            {
                // clip version

                // point screen ptr to starting line
                screen_ptr = dest_buffer + (ystart * mem_pitch);

                // point zbuffer to starting line
                z_ptr = zbuffer + (ystart * zpitch);

                for (yi = ystart; yi < yend; yi++)
                {
                    // compute span endpoints
                    xstart = ((xl + FIXP16_ROUND_UP) >> FIXP16_SHIFT);
                    xend   = ((xr + FIXP16_ROUND_UP) >> FIXP16_SHIFT);

                    // compute starting points for u,v,w interpolants
                    ui = ul + FIXP16_ROUND_UP;
                    vi = vl + FIXP16_ROUND_UP;
                    wi = wl + FIXP16_ROUND_UP;
                    zi = zl + FIXP16_ROUND_UP;

                    si = sl + FIXP16_ROUND_UP;
                    ti = tl + FIXP16_ROUND_UP;

                    // compute u,v interpolants
                    if ((dx = (xend - xstart))>0)
                    {
                        du = (ur - ul)/dx;
                        dv = (vr - vl)/dx;
                        dw = (wr - wl)/dx;
                        dz = (zr - zl)/dx;

                        ds = (sr - sl)/dx;
                        dt = (tr - tl)/dx;

                    } // end if
                    else
                    {
                        du = (ur - ul);
                        dv = (vr - vl);
                        dw = (wr - wl);
                        dz = (zr - zl);

                        ds = (sr - sl);
                        dt = (tr - tl);

                    } // end else

                    ///////////////////////////////////////////////////////////////////////

                    // test for x clipping, LHS
                    if (xstart < min_clip_x)
                    {
                        // compute x overlap
                        dx = min_clip_x - xstart;

                        // slide interpolants over
                        ui+=dx*du;
                        vi+=dx*dv;
                        wi+=dx*dw;
                        zi+=dx*dz;

                        si+=dx*ds;
                        ti+=dx*dt;

                        // reset vars
                        xstart = min_clip_x;

                    } // end if

                    // test for x clipping RHS
                    if (xend > max_clip_x)
                        xend = max_clip_x;

                    ///////////////////////////////////////////////////////////////////////

                    // draw span
                    for (xi=xstart; xi < xend; xi++)
                    {
                        // write textel assume 5.6.5
                        // write thru z buffer always
                        // get textel first
                        textel = textmap[(si >> FIXP16_SHIFT) + ((ti >> FIXP16_SHIFT) << texture_shift2)];

                        if (textel)
                        {

                            // extract rgb components
                            r_textel  = ((textel >> 11)       ); 
                            g_textel  = ((textel >> 5)  & 0x3f); 
                            b_textel =   (textel        & 0x1f);

                            // modulate textel with gouraud shading
                            r_textel*=ui; 
                            g_textel*=vi;
                            b_textel*=wi;

                            // finally write pixel, note that we did the math such that the results are r*32, g*64, b*32
                            // hence we need to divide the results by 32,64,32 respetively, BUT since we need to shift
                            // the results to fit into the destination 5.6.5 word, we can take advantage of the shifts
                            // and they all cancel out for the most part, but we will need logical anding, we will do
                            // it later when we optimize more...
                            screen_ptr[xi] = ((b_textel >> (FIXP16_SHIFT+8)) + 
                                ((g_textel >> (FIXP16_SHIFT+8)) << 5) + 
                                ((r_textel >> (FIXP16_SHIFT+8)) << 11));

                            // update z-buffer
                            z_ptr[xi] = zi;   
                        } // end if


                        // interpolate u,v
                        ui+=du;
                        vi+=dv;
                        wi+=dw;
                        zi+=dz;

                        si+=ds;
                        ti+=dt;

                    } // end for xi

                    // interpolate u,v,w,x along right and left edge
                    xl+=dxdyl;
                    ul+=dudyl;
                    vl+=dvdyl;
                    wl+=dwdyl;
                    zl+=dzdyl;

                    sl+=dsdyl;
                    tl+=dtdyl;

                    xr+=dxdyr;
                    ur+=dudyr;
                    vr+=dvdyr;
                    wr+=dwdyr;
                    zr+=dzdyr;

                    sr+=dsdyr;
                    tr+=dtdyr;

                    // advance screen ptr
                    screen_ptr+=mem_pitch;

                    // advance zbuffer ptr
                    z_ptr+=zpitch;

                } // end for y

            } // end if clip
            else
            {
                // non-clip version

                // point screen ptr to starting line
                screen_ptr = dest_buffer + (ystart * mem_pitch);

                // point zbuffer to starting line
                z_ptr = zbuffer + (ystart * zpitch);

                for (yi = ystart; yi < yend; yi++)
                {
                    // compute span endpoints
                    xstart = ((xl + FIXP16_ROUND_UP) >> FIXP16_SHIFT);
                    xend   = ((xr + FIXP16_ROUND_UP) >> FIXP16_SHIFT);

                    // compute starting points for u,v,w interpolants
                    ui = ul + FIXP16_ROUND_UP;
                    vi = vl + FIXP16_ROUND_UP;
                    wi = wl + FIXP16_ROUND_UP;
                    zi = zl + FIXP16_ROUND_UP;

                    si = sl + FIXP16_ROUND_UP;
                    ti = tl + FIXP16_ROUND_UP;

                    // compute u,v interpolants
                    if ((dx = (xend - xstart))>0)
                    {
                        du = (ur - ul)/dx;
                        dv = (vr - vl)/dx;
                        dw = (wr - wl)/dx;
                        dz = (zr - zl)/dx;

                        ds = (sr - sl)/dx;
                        dt = (tr - tl)/dx;

                    } // end if
                    else
                    {
                        du = (ur - ul);
                        dv = (vr - vl);
                        dw = (wr - wl);
                        dz = (zr - zl);

                        ds = (sr - sl);
                        dt = (tr - tl);

                    } // end else

                    // draw span
                    for (xi=xstart; xi < xend; xi++)
                    {
                        // write textel assume 5.6.5
                        // write thru z buffer always
                        // get textel first
                        textel = textmap[(si >> FIXP16_SHIFT) + ((ti >> FIXP16_SHIFT) << texture_shift2)];

                        if (textel)
                        {

                            // extract rgb components
                            r_textel  = ((textel >> 11)       ); 
                            g_textel  = ((textel >> 5)  & 0x3f); 
                            b_textel =   (textel        & 0x1f);

                            // modulate textel with gouraud shading
                            r_textel*=ui; 
                            g_textel*=vi;
                            b_textel*=wi;

                            // finally write pixel, note that we did the math such that the results are r*32, g*64, b*32
                            // hence we need to divide the results by 32,64,32 respetively, BUT since we need to shift
                            // the results to fit into the destination 5.6.5 word, we can take advantage of the shifts
                            // and they all cancel out for the most part, but we will need logical anding, we will do
                            // it later when we optimize more...
                            screen_ptr[xi] = ((b_textel >> (FIXP16_SHIFT+8)) + 
                                ((g_textel >> (FIXP16_SHIFT+8)) << 5) + 
                                ((r_textel >> (FIXP16_SHIFT+8)) << 11));

                            // update z-buffer
                            z_ptr[xi] = zi;   
                        } // end if

                        // interpolate u,v
                        ui+=du;
                        vi+=dv;
                        wi+=dw;
                        zi+=dz;

                        si+=ds;
                        ti+=dt;

                    } // end for xi

                    // interpolate u,v,w,x along right and left edge
                    xl+=dxdyl;
                    ul+=dudyl;
                    vl+=dvdyl;
                    wl+=dwdyl;
                    zl+=dzdyl;

                    sl+=dsdyl;
                    tl+=dtdyl;

                    xr+=dxdyr;
                    ur+=dudyr;
                    vr+=dvdyr;
                    wr+=dwdyr;
                    zr+=dzdyr;

                    sr+=dsdyr;
                    tr+=dtdyr;

                    // advance screen ptr
                    screen_ptr+=mem_pitch;

                    // advance zbuffer ptr
                    z_ptr+=zpitch;

                } // end for y

            } // end if non-clipped

        } // end if
        else
            if (tri_type==TRI_TYPE_GENERAL)
            {

                // first test for bottom clip, always
                if ((yend = y2) > max_clip_y)
                    yend = max_clip_y;

                // pre-test y clipping status
                if (y1 < min_clip_y)
                {
                    // compute all deltas
                    // LHS
                    dyl = (y2 - y1);

                    dxdyl = ((x2  - x1)  << FIXP16_SHIFT)/dyl;
                    dudyl = ((tu2 - tu1) << FIXP16_SHIFT)/dyl;  
                    dvdyl = ((tv2 - tv1) << FIXP16_SHIFT)/dyl;    
                    dwdyl = ((tw2 - tw1) << FIXP16_SHIFT)/dyl;  
                    dzdyl = ((tz2 - tz1) << FIXP16_SHIFT)/dyl;  

                    dsdyl = ((ts2 - ts1) << FIXP16_SHIFT)/dyl;    
                    dtdyl = ((tt2 - tt1) << FIXP16_SHIFT)/dyl;  

                    // RHS
                    dyr = (y2 - y0);	

                    dxdyr = ((x2  - x0)  << FIXP16_SHIFT)/dyr;
                    dudyr = ((tu2 - tu0) << FIXP16_SHIFT)/dyr;  
                    dvdyr = ((tv2 - tv0) << FIXP16_SHIFT)/dyr;   
                    dwdyr = ((tw2 - tw0) << FIXP16_SHIFT)/dyr;   
                    dzdyr = ((tz2 - tz0) << FIXP16_SHIFT)/dyr;   

                    dsdyr = ((ts2 - ts0) << FIXP16_SHIFT)/dyr;   
                    dtdyr = ((tt2 - tt0) << FIXP16_SHIFT)/dyr;  

                    // compute overclip
                    dyr = (min_clip_y - y0);
                    dyl = (min_clip_y - y1);

                    // computer new LHS starting values
                    xl = dxdyl*dyl + (x1  << FIXP16_SHIFT);

                    ul = dudyl*dyl + (tu1 << FIXP16_SHIFT);
                    vl = dvdyl*dyl + (tv1 << FIXP16_SHIFT);
                    wl = dwdyl*dyl + (tw1 << FIXP16_SHIFT);
                    zl = dzdyl*dyl + (tz1 << FIXP16_SHIFT);

                    sl = dsdyl*dyl + (ts1 << FIXP16_SHIFT);
                    tl = dtdyl*dyl + (tt1 << FIXP16_SHIFT);

                    // compute new RHS starting values
                    xr = dxdyr*dyr + (x0  << FIXP16_SHIFT);

                    ur = dudyr*dyr + (tu0 << FIXP16_SHIFT);
                    vr = dvdyr*dyr + (tv0 << FIXP16_SHIFT);
                    wr = dwdyr*dyr + (tw0 << FIXP16_SHIFT);
                    zr = dzdyr*dyr + (tz0 << FIXP16_SHIFT);

                    sr = dsdyr*dyr + (ts0 << FIXP16_SHIFT);
                    tr = dtdyr*dyr + (tt0 << FIXP16_SHIFT);

                    // compute new starting y
                    ystart = min_clip_y;

                    // test if we need swap to keep rendering left to right
                    if (dxdyr > dxdyl)
                    {
                        SWAP(dxdyl,dxdyr,temp);
                        SWAP(dudyl,dudyr,temp);
                        SWAP(dvdyl,dvdyr,temp);
                        SWAP(dwdyl,dwdyr,temp);
                        SWAP(dzdyl,dzdyr,temp);

                        SWAP(dsdyl,dsdyr,temp);
                        SWAP(dtdyl,dtdyr,temp);

                        SWAP(xl,xr,temp);
                        SWAP(ul,ur,temp);
                        SWAP(vl,vr,temp);
                        SWAP(wl,wr,temp);
                        SWAP(zl,zr,temp);

                        SWAP(sl,sr,temp);
                        SWAP(tl,tr,temp);

                        SWAP(x1,x2,temp);
                        SWAP(y1,y2,temp);
                        SWAP(tu1,tu2,temp);
                        SWAP(tv1,tv2,temp);
                        SWAP(tw1,tw2,temp);
                        SWAP(tz1,tz2,temp);

                        SWAP(ts1,ts2,temp);
                        SWAP(tt1,tt2,temp);

                        // set interpolation restart
                        irestart = INTERP_RHS;

                    } // end if

                } // end if
                else
                    if (y0 < min_clip_y)
                    {
                        // compute all deltas
                        // LHS
                        dyl = (y1 - y0);

                        dxdyl = ((x1  - x0)  << FIXP16_SHIFT)/dyl;
                        dudyl = ((tu1 - tu0) << FIXP16_SHIFT)/dyl;  
                        dvdyl = ((tv1 - tv0) << FIXP16_SHIFT)/dyl;    
                        dwdyl = ((tw1 - tw0) << FIXP16_SHIFT)/dyl; 
                        dzdyl = ((tz1 - tz0) << FIXP16_SHIFT)/dyl; 

                        dsdyl = ((ts1 - ts0) << FIXP16_SHIFT)/dyl;    
                        dtdyl = ((tt1 - tt0) << FIXP16_SHIFT)/dyl; 

                        // RHS
                        dyr = (y2 - y0);	

                        dxdyr = ((x2  - x0)  << FIXP16_SHIFT)/dyr;
                        dudyr = ((tu2 - tu0) << FIXP16_SHIFT)/dyr;  
                        dvdyr = ((tv2 - tv0) << FIXP16_SHIFT)/dyr;   
                        dwdyr = ((tw2 - tw0) << FIXP16_SHIFT)/dyr;   
                        dzdyr = ((tz2 - tz0) << FIXP16_SHIFT)/dyr;   

                        dsdyr = ((ts2 - ts0) << FIXP16_SHIFT)/dyr;   
                        dtdyr = ((tt2 - tt0) << FIXP16_SHIFT)/dyr;   

                        // compute overclip
                        dy = (min_clip_y - y0);

                        // computer new LHS starting values
                        xl = dxdyl*dy + (x0  << FIXP16_SHIFT);
                        ul = dudyl*dy + (tu0 << FIXP16_SHIFT);
                        vl = dvdyl*dy + (tv0 << FIXP16_SHIFT);
                        wl = dwdyl*dy + (tw0 << FIXP16_SHIFT);
                        zl = dzdyl*dy + (tz0 << FIXP16_SHIFT);

                        sl = dsdyl*dy + (ts0 << FIXP16_SHIFT);
                        tl = dtdyl*dy + (tt0 << FIXP16_SHIFT);

                        // compute new RHS starting values
                        xr = dxdyr*dy + (x0  << FIXP16_SHIFT);
                        ur = dudyr*dy + (tu0 << FIXP16_SHIFT);
                        vr = dvdyr*dy + (tv0 << FIXP16_SHIFT);
                        wr = dwdyr*dy + (tw0 << FIXP16_SHIFT);
                        zr = dzdyr*dy + (tz0 << FIXP16_SHIFT);

                        sr = dsdyr*dy + (ts0 << FIXP16_SHIFT);
                        tr = dtdyr*dy + (tt0 << FIXP16_SHIFT);

                        // compute new starting y
                        ystart = min_clip_y;

                        // test if we need swap to keep rendering left to right
                        if (dxdyr < dxdyl)
                        {
                            SWAP(dxdyl,dxdyr,temp);
                            SWAP(dudyl,dudyr,temp);
                            SWAP(dvdyl,dvdyr,temp);
                            SWAP(dwdyl,dwdyr,temp);
                            SWAP(dzdyl,dzdyr,temp);

                            SWAP(dsdyl,dsdyr,temp);
                            SWAP(dtdyl,dtdyr,temp);

                            SWAP(xl,xr,temp);
                            SWAP(ul,ur,temp);
                            SWAP(vl,vr,temp);
                            SWAP(wl,wr,temp);
                            SWAP(zl,zr,temp);

                            SWAP(sl,sr,temp);
                            SWAP(tl,tr,temp);

                            SWAP(x1,x2,temp);
                            SWAP(y1,y2,temp);
                            SWAP(tu1,tu2,temp);
                            SWAP(tv1,tv2,temp);
                            SWAP(tw1,tw2,temp);
                            SWAP(tz1,tz2,temp);

                            SWAP(ts1,ts2,temp);
                            SWAP(tt1,tt2,temp);

                            // set interpolation restart
                            irestart = INTERP_RHS;

                        } // end if

                    } // end if
                    else
                    {
                        // no initial y clipping

                        // compute all deltas
                        // LHS
                        dyl = (y1 - y0);

                        dxdyl = ((x1  - x0)  << FIXP16_SHIFT)/dyl;
                        dudyl = ((tu1 - tu0) << FIXP16_SHIFT)/dyl;  
                        dvdyl = ((tv1 - tv0) << FIXP16_SHIFT)/dyl;    
                        dwdyl = ((tw1 - tw0) << FIXP16_SHIFT)/dyl;   
                        dzdyl = ((tz1 - tz0) << FIXP16_SHIFT)/dyl;  

                        dsdyl = ((ts1 - ts0) << FIXP16_SHIFT)/dyl;    
                        dtdyl = ((tt1 - tt0) << FIXP16_SHIFT)/dyl;   

                        // RHS
                        dyr = (y2 - y0);	

                        dxdyr = ((x2 - x0)   << FIXP16_SHIFT)/dyr;
                        dudyr = ((tu2 - tu0) << FIXP16_SHIFT)/dyr;  
                        dvdyr = ((tv2 - tv0) << FIXP16_SHIFT)/dyr;   		
                        dwdyr = ((tw2 - tw0) << FIXP16_SHIFT)/dyr;
                        dzdyr = ((tz2 - tz0) << FIXP16_SHIFT)/dyr;

                        dsdyr = ((ts2 - ts0) << FIXP16_SHIFT)/dyr;   		
                        dtdyr = ((tt2 - tt0) << FIXP16_SHIFT)/dyr;

                        // no clipping y

                        // set starting values
                        xl = (x0 << FIXP16_SHIFT);
                        xr = (x0 << FIXP16_SHIFT);

                        ul = (tu0 << FIXP16_SHIFT);
                        vl = (tv0 << FIXP16_SHIFT);
                        wl = (tw0 << FIXP16_SHIFT);
                        zl = (tz0 << FIXP16_SHIFT);

                        sl = (ts0 << FIXP16_SHIFT);
                        tl = (tt0 << FIXP16_SHIFT);

                        ur = (tu0 << FIXP16_SHIFT);
                        vr = (tv0 << FIXP16_SHIFT);
                        wr = (tw0 << FIXP16_SHIFT);
                        zr = (tz0 << FIXP16_SHIFT);

                        sr = (ts0 << FIXP16_SHIFT);
                        tr = (tt0 << FIXP16_SHIFT);

                        // set starting y
                        ystart = y0;

                        // test if we need swap to keep rendering left to right
                        if (dxdyr < dxdyl)
                        {
                            SWAP(dxdyl,dxdyr,temp);
                            SWAP(dudyl,dudyr,temp);
                            SWAP(dvdyl,dvdyr,temp);
                            SWAP(dwdyl,dwdyr,temp);
                            SWAP(dzdyl,dzdyr,temp);

                            SWAP(dsdyl,dsdyr,temp);
                            SWAP(dtdyl,dtdyr,temp);


                            SWAP(xl,xr,temp);
                            SWAP(ul,ur,temp);
                            SWAP(vl,vr,temp);
                            SWAP(wl,wr,temp);
                            SWAP(zl,zr,temp);

                            SWAP(sl,sr,temp);
                            SWAP(tl,tr,temp);


                            SWAP(x1,x2,temp);
                            SWAP(y1,y2,temp);
                            SWAP(tu1,tu2,temp);
                            SWAP(tv1,tv2,temp);
                            SWAP(tw1,tw2,temp);
                            SWAP(tz1,tz2,temp);

                            SWAP(ts1,ts2,temp);
                            SWAP(tt1,tt2,temp);

                            // set interpolation restart
                            irestart = INTERP_RHS;

                        } // end if

                    } // end else

                    // test for horizontal clipping
                    if ((x0 < min_clip_x) || (x0 > max_clip_x) ||
                        (x1 < min_clip_x) || (x1 > max_clip_x) ||
                        (x2 < min_clip_x) || (x2 > max_clip_x))
                    {
                        // clip version
                        // x clipping	

                        // point screen ptr to starting line
                        screen_ptr = dest_buffer + (ystart * mem_pitch);

                        // point zbuffer to starting line
                        z_ptr = zbuffer + (ystart * zpitch);

                        for (yi = ystart; yi < yend; yi++)
                        {
                            // compute span endpoints
                            xstart = ((xl + FIXP16_ROUND_UP) >> FIXP16_SHIFT);
                            xend   = ((xr + FIXP16_ROUND_UP) >> FIXP16_SHIFT);

                            // compute starting points for u,v,w interpolants
                            ui = ul + FIXP16_ROUND_UP;
                            vi = vl + FIXP16_ROUND_UP;
                            wi = wl + FIXP16_ROUND_UP;
                            zi = zl + FIXP16_ROUND_UP;

                            si = sl + FIXP16_ROUND_UP;
                            ti = tl + FIXP16_ROUND_UP;

                            // compute u,v interpolants
                            if ((dx = (xend - xstart))>0)
                            {
                                du = (ur - ul)/dx;
                                dv = (vr - vl)/dx;
                                dw = (wr - wl)/dx;
                                dz = (zr - zl)/dx;

                                ds = (sr - sl)/dx;
                                dt = (tr - tl)/dx;

                            } // end if
                            else
                            {
                                du = (ur - ul);
                                dv = (vr - vl);
                                dw = (wr - wl);
                                dz = (zr - zl);

                                ds = (sr - sl);
                                dt = (tr - tl);

                            } // end else

                            ///////////////////////////////////////////////////////////////////////

                            // test for x clipping, LHS
                            if (xstart < min_clip_x)
                            {
                                // compute x overlap
                                dx = min_clip_x - xstart;

                                // slide interpolants over
                                ui+=dx*du;
                                vi+=dx*dv;
                                wi+=dx*dw;
                                zi+=dx*dz;

                                si+=dx*ds;
                                ti+=dx*dt;

                                // set x to left clip edge
                                xstart = min_clip_x;

                            } // end if

                            // test for x clipping RHS
                            if (xend > max_clip_x)
                                xend = max_clip_x;

                            ///////////////////////////////////////////////////////////////////////

                            // draw span
                            for (xi=xstart; xi < xend; xi++)
                            {
                                // write textel assume 5.6.5
                                // write thru z buffer always
                                // get textel first
                                textel = textmap[(si >> FIXP16_SHIFT) + ((ti >> FIXP16_SHIFT) << texture_shift2)];

                                if (textel)
                                {

                                    // extract rgb components
                                    r_textel  = ((textel >> 11)       ); 
                                    g_textel  = ((textel >> 5)  & 0x3f); 
                                    b_textel =   (textel        & 0x1f);

                                    // modulate textel with gouraud shading
                                    r_textel*=ui; 
                                    g_textel*=vi;
                                    b_textel*=wi;

                                    // finally write pixel, note that we did the math such that the results are r*32, g*64, b*32
                                    // hence we need to divide the results by 32,64,32 respetively, BUT since we need to shift
                                    // the results to fit into the destination 5.6.5 word, we can take advantage of the shifts
                                    // and they all cancel out for the most part, but we will need logical anding, we will do
                                    // it later when we optimize more...
                                    screen_ptr[xi] = ((b_textel >> (FIXP16_SHIFT+8)) + 
                                        ((g_textel >> (FIXP16_SHIFT+8)) << 5) + 
                                        ((r_textel >> (FIXP16_SHIFT+8)) << 11));

                                    // update z-buffer
                                    z_ptr[xi] = zi;   
                                } // end if

                                // interpolate u,v
                                ui+=du;
                                vi+=dv;
                                wi+=dw;
                                zi+=dz;

                                si+=ds;
                                ti+=dt;

                            } // end for xi

                            // interpolate u,v,w,x along right and left edge
                            xl+=dxdyl;
                            ul+=dudyl;
                            vl+=dvdyl;
                            wl+=dwdyl;
                            zl+=dzdyl;

                            sl+=dsdyl;
                            tl+=dtdyl;

                            xr+=dxdyr;
                            ur+=dudyr;
                            vr+=dvdyr;
                            wr+=dwdyr;
                            zr+=dzdyr;

                            sr+=dsdyr;
                            tr+=dtdyr;

                            // advance screen ptr
                            screen_ptr+=mem_pitch;

                            // advance zbuffer ptr
                            z_ptr+=zpitch;

                            // test for yi hitting second region, if so change interpolant
                            if (yi==yrestart)
                            {
                                // test interpolation side change flag

                                if (irestart == INTERP_LHS)
                                {
                                    // LHS
                                    dyl = (y2 - y1);	

                                    dxdyl = ((x2 - x1)   << FIXP16_SHIFT)/dyl;
                                    dudyl = ((tu2 - tu1) << FIXP16_SHIFT)/dyl;  
                                    dvdyl = ((tv2 - tv1) << FIXP16_SHIFT)/dyl;   		
                                    dwdyl = ((tw2 - tw1) << FIXP16_SHIFT)/dyl;  
                                    dzdyl = ((tz2 - tz1) << FIXP16_SHIFT)/dyl;  

                                    dsdyl = ((ts2 - ts1) << FIXP16_SHIFT)/dyl;   		
                                    dtdyl = ((tt2 - tt1) << FIXP16_SHIFT)/dyl;  


                                    // set starting values
                                    xl = (x1  << FIXP16_SHIFT);
                                    ul = (tu1 << FIXP16_SHIFT);
                                    vl = (tv1 << FIXP16_SHIFT);
                                    wl = (tw1 << FIXP16_SHIFT);
                                    zl = (tz1 << FIXP16_SHIFT);

                                    sl = (ts1 << FIXP16_SHIFT);
                                    tl = (tt1 << FIXP16_SHIFT);

                                    // interpolate down on LHS to even up
                                    xl+=dxdyl;
                                    ul+=dudyl;
                                    vl+=dvdyl;
                                    wl+=dwdyl;
                                    zl+=dzdyl;

                                    sl+=dsdyl;
                                    tl+=dtdyl;

                                } // end if
                                else
                                {
                                    // RHS
                                    dyr = (y1 - y2);	

                                    dxdyr = ((x1 - x2)   << FIXP16_SHIFT)/dyr;
                                    dudyr = ((tu1 - tu2) << FIXP16_SHIFT)/dyr;  
                                    dvdyr = ((tv1 - tv2) << FIXP16_SHIFT)/dyr;   		
                                    dwdyr = ((tw1 - tw2) << FIXP16_SHIFT)/dyr;   		
                                    dzdyr = ((tz1 - tz2) << FIXP16_SHIFT)/dyr;   

                                    dsdyr = ((ts1 - ts2) << FIXP16_SHIFT)/dyr;   		
                                    dtdyr = ((tt1 - tt2) << FIXP16_SHIFT)/dyr;  

                                    // set starting values
                                    xr = (x2  << FIXP16_SHIFT);
                                    ur = (tu2 << FIXP16_SHIFT);
                                    vr = (tv2 << FIXP16_SHIFT);
                                    wr = (tw2 << FIXP16_SHIFT);
                                    zr = (tz2 << FIXP16_SHIFT);

                                    sr = (ts2 << FIXP16_SHIFT);
                                    tr = (tt2 << FIXP16_SHIFT);

                                    // interpolate down on RHS to even up
                                    xr+=dxdyr;
                                    ur+=dudyr;
                                    vr+=dvdyr;
                                    wr+=dwdyr;
                                    zr+=dzdyr;

                                    sr+=dsdyr;
                                    tr+=dtdyr;
                                } // end else

                            } // end if

                        } // end for y

                    } // end if
                    else
                    {
                        // no x clipping
                        // point screen ptr to starting line
                        screen_ptr = dest_buffer + (ystart * mem_pitch);

                        // point zbuffer to starting line
                        z_ptr = zbuffer + (ystart * zpitch);

                        for (yi = ystart; yi < yend; yi++)
                        {
                            // compute span endpoints
                            xstart = ((xl + FIXP16_ROUND_UP) >> FIXP16_SHIFT);
                            xend   = ((xr + FIXP16_ROUND_UP) >> FIXP16_SHIFT);

                            // compute starting points for u,v,w interpolants
                            ui = ul + FIXP16_ROUND_UP;
                            vi = vl + FIXP16_ROUND_UP;
                            wi = wl + FIXP16_ROUND_UP;
                            zi = zl + FIXP16_ROUND_UP;

                            si = sl + FIXP16_ROUND_UP;
                            ti = tl + FIXP16_ROUND_UP;

                            // compute u,v interpolants
                            if ((dx = (xend - xstart))>0)
                            {
                                du = (ur - ul)/dx;
                                dv = (vr - vl)/dx;
                                dw = (wr - wl)/dx;
                                dz = (zr - zl)/dx;

                                ds = (sr - sl)/dx;
                                dt = (tr - tl)/dx;

                            } // end if
                            else
                            {
                                du = (ur - ul);
                                dv = (vr - vl);
                                dw = (wr - wl);
                                dz = (zr - zl);

                                ds = (sr - sl);
                                dt = (tr - tl);

                            } // end else

                            // draw span
                            for (xi=xstart; xi < xend; xi++)
                            {
                                // write textel assume 5.6.5
                                // write thru z buffer always
                                // get textel first
                                textel = textmap[(si >> FIXP16_SHIFT) + ((ti >> FIXP16_SHIFT) << texture_shift2)];

                                if (textel)
                                {

                                    // extract rgb components
                                    r_textel  = ((textel >> 11)       ); 
                                    g_textel  = ((textel >> 5)  & 0x3f); 
                                    b_textel =   (textel        & 0x1f);

                                    // modulate textel with gouraud shading
                                    r_textel*=ui; 
                                    g_textel*=vi;
                                    b_textel*=wi;

                                    // finally write pixel, note that we did the math such that the results are r*32, g*64, b*32
                                    // hence we need to divide the results by 32,64,32 respetively, BUT since we need to shift
                                    // the results to fit into the destination 5.6.5 word, we can take advantage of the shifts
                                    // and they all cancel out for the most part, but we will need logical anding, we will do
                                    // it later when we optimize more...
                                    screen_ptr[xi] = ((b_textel >> (FIXP16_SHIFT+8)) + 
                                        ((g_textel >> (FIXP16_SHIFT+8)) << 5) + 
                                        ((r_textel >> (FIXP16_SHIFT+8)) << 11));

                                    // update z-buffer
                                    z_ptr[xi] = zi;   
                                } // end if

                                // interpolate u,v
                                ui+=du;
                                vi+=dv;
                                wi+=dw;
                                zi+=dz;

                                si+=ds;
                                ti+=dt;
                            } // end for xi

                            // interpolate u,v,w,x along right and left edge
                            xl+=dxdyl;
                            ul+=dudyl;
                            vl+=dvdyl;
                            wl+=dwdyl;
                            zl+=dzdyl;

                            sl+=dsdyl;
                            tl+=dtdyl;

                            xr+=dxdyr;
                            ur+=dudyr;
                            vr+=dvdyr;
                            wr+=dwdyr;
                            zr+=dzdyr;

                            sr+=dsdyr;
                            tr+=dtdyr;

                            // advance screen ptr
                            screen_ptr+=mem_pitch;

                            // advance zbuffer ptr
                            z_ptr+=zpitch;

                            // test for yi hitting second region, if so change interpolant
                            if (yi==yrestart)
                            {
                                // test interpolation side change flag

                                if (irestart == INTERP_LHS)
                                {
                                    // LHS
                                    dyl = (y2 - y1);	

                                    dxdyl = ((x2 - x1)   << FIXP16_SHIFT)/dyl;
                                    dudyl = ((tu2 - tu1) << FIXP16_SHIFT)/dyl;  
                                    dvdyl = ((tv2 - tv1) << FIXP16_SHIFT)/dyl;   		
                                    dwdyl = ((tw2 - tw1) << FIXP16_SHIFT)/dyl;   
                                    dzdyl = ((tz2 - tz1) << FIXP16_SHIFT)/dyl;  

                                    dsdyl = ((ts2 - ts1) << FIXP16_SHIFT)/dyl;   		
                                    dtdyl = ((tt2 - tt1) << FIXP16_SHIFT)/dyl;   

                                    // set starting values
                                    xl = (x1  << FIXP16_SHIFT);
                                    ul = (tu1 << FIXP16_SHIFT);
                                    vl = (tv1 << FIXP16_SHIFT);
                                    wl = (tw1 << FIXP16_SHIFT);
                                    zl = (tz1 << FIXP16_SHIFT);

                                    sl = (ts1 << FIXP16_SHIFT);
                                    tl = (tt1 << FIXP16_SHIFT);

                                    // interpolate down on LHS to even up
                                    xl+=dxdyl;
                                    ul+=dudyl;
                                    vl+=dvdyl;
                                    wl+=dwdyl;
                                    zl+=dzdyl;

                                    sl+=dsdyl;
                                    tl+=dtdyl;

                                } // end if
                                else
                                {
                                    // RHS
                                    dyr = (y1 - y2);	

                                    dxdyr = ((x1 - x2)   << FIXP16_SHIFT)/dyr;
                                    dudyr = ((tu1 - tu2) << FIXP16_SHIFT)/dyr;  
                                    dvdyr = ((tv1 - tv2) << FIXP16_SHIFT)/dyr;   		
                                    dwdyr = ((tw1 - tw2) << FIXP16_SHIFT)/dyr;   
                                    dzdyr = ((tz1 - tz2) << FIXP16_SHIFT)/dyr;   

                                    dsdyr = ((ts1 - ts2) << FIXP16_SHIFT)/dyr;   		
                                    dtdyr = ((tt1 - tt2) << FIXP16_SHIFT)/dyr;   

                                    // set starting values
                                    xr = (x2  << FIXP16_SHIFT);
                                    ur = (tu2 << FIXP16_SHIFT);
                                    vr = (tv2 << FIXP16_SHIFT);
                                    wr = (tw2 << FIXP16_SHIFT);
                                    zr = (tz2 << FIXP16_SHIFT);

                                    sr = (ts2 << FIXP16_SHIFT);
                                    tr = (tt2 << FIXP16_SHIFT);

                                    // interpolate down on RHS to even up
                                    xr+=dxdyr;
                                    ur+=dudyr;
                                    vr+=dvdyr;
                                    wr+=dwdyr;
                                    zr+=dzdyr;

                                    sr+=dsdyr;
                                    tr+=dtdyr;

                                } // end else

                            } // end if

                        } // end for y

                    } // end else	

            } // end if

} // end Draw_Textured_TriangleGSWTZB2_16

///////////////////////////////////////////////////////////////////////////////////////////////////

void Draw_Textured_Triangle_Alpha16_2(POLYF4DV2_PTR face,   // ptr to face
                                      UCHAR *_dest_buffer,      // pointer to video buffer
                                      int mem_pitch, int alpha) // bytes per line, 320, 640 etc.
{
    // this function draws a textured triangle in 16-bit mode

    int v0=0,
        v1=1,
        v2=2,
        temp=0,
        tri_type = TRI_TYPE_NONE,
        irestart = INTERP_LHS;

    int dx,dy,dyl,dyr,      // general deltas
        u,v,
        du,dv,
        xi,yi,              // the current interpolated x,y
        ui,vi,              // the current interpolated u,v
        index_x,index_y,    // looping vars
        x,y,                // hold general x,y
        xstart,
        xend,
        ystart,
        yrestart,
        yend,
        xl,                 
        dxdyl,              
        xr,
        dxdyr,             
        dudyl,    
        ul,
        dvdyl,   
        vl,
        dudyr,
        ur,
        dvdyr,
        vr;

    int x0,y0,tu0,tv0,    // cached vertices
        x1,y1,tu1,tv1,
        x2,y2,tu2,tv2;

    USHORT *screen_ptr  = NULL,
        *screen_line = NULL,
        *textmap     = NULL,
        *dest_buffer = (USHORT *)_dest_buffer,
        textel;

#ifdef DEBUG_ON
    // track rendering stats
    debug_polys_rendered_per_frame++;
#endif

    // extract texture map
    textmap = (USHORT *)face->texture->buffer;

    // extract base 2 of texture width
    int texture_shift2 = logbase2ofx[face->texture->width];

    // adjust memory pitch to words, divide by 2
    mem_pitch >>=1;

    // apply fill convention to coordinates
    face->tvlist[0].x = (int)(face->tvlist[0].x+0.5);
    face->tvlist[0].y = (int)(face->tvlist[0].y+0.5);

    face->tvlist[1].x = (int)(face->tvlist[1].x+0.5);
    face->tvlist[1].y = (int)(face->tvlist[1].y+0.5);

    face->tvlist[2].x = (int)(face->tvlist[2].x+0.5);
    face->tvlist[2].y = (int)(face->tvlist[2].y+0.5);

    // first trivial clipping rejection tests 
    if (((face->tvlist[0].y < min_clip_y)  && 
        (face->tvlist[1].y < min_clip_y)  &&
        (face->tvlist[2].y < min_clip_y)) ||

        ((face->tvlist[0].y > max_clip_y)  && 
        (face->tvlist[1].y > max_clip_y)  &&
        (face->tvlist[2].y > max_clip_y)) ||

        ((face->tvlist[0].x < min_clip_x)  && 
        (face->tvlist[1].x < min_clip_x)  &&
        (face->tvlist[2].x < min_clip_x)) ||

        ((face->tvlist[0].x > max_clip_x)  && 
        (face->tvlist[1].x > max_clip_x)  &&
        (face->tvlist[2].x > max_clip_x)))
        return;

    // sort vertices
    if (face->tvlist[v1].y < face->tvlist[v0].y) 
    {SWAP(v0,v1,temp);} 

    if (face->tvlist[v2].y < face->tvlist[v0].y) 
    {SWAP(v0,v2,temp);}

    if (face->tvlist[v2].y < face->tvlist[v1].y) 
    {SWAP(v1,v2,temp);}

    // now test for trivial flat sided cases
    if (FCMP(face->tvlist[v0].y, face->tvlist[v1].y) )
    { 
        // set triangle type
        tri_type = TRI_TYPE_FLAT_TOP;

        // sort vertices left to right
        if (face->tvlist[v1].x < face->tvlist[v0].x) 
        {SWAP(v0,v1,temp);}

    } // end if
    else
        // now test for trivial flat sided cases
        if (FCMP(face->tvlist[v1].y, face->tvlist[v2].y) )
        { 
            // set triangle type
            tri_type = TRI_TYPE_FLAT_BOTTOM;

            // sort vertices left to right
            if (face->tvlist[v2].x < face->tvlist[v1].x) 
            {SWAP(v1,v2,temp);}

        } // end if
        else
        {
            // must be a general triangle
            tri_type = TRI_TYPE_GENERAL;

        } // end else

        // extract vertices for processing, now that we have order
        x0  = (int)(face->tvlist[v0].x+0.0);
        y0  = (int)(face->tvlist[v0].y+0.0);
        tu0 = (int)(face->tvlist[v0].u0);
        tv0 = (int)(face->tvlist[v0].v0);

        x1  = (int)(face->tvlist[v1].x+0.0);
        y1  = (int)(face->tvlist[v1].y+0.0);
        tu1 = (int)(face->tvlist[v1].u0);
        tv1 = (int)(face->tvlist[v1].v0);

        x2  = (int)(face->tvlist[v2].x+0.0);
        y2  = (int)(face->tvlist[v2].y+0.0);
        tu2 = (int)(face->tvlist[v2].u0);
        tv2 = (int)(face->tvlist[v2].v0);


        // degenerate triangle
        if ( ((x0 == x1) && (x1 == x2)) || ((y0 ==  y1) && (y1 == y2)))
            return;

        // assign both source1 and source2 alpha tables based on polygon alpha level
        USHORT *alpha_table_src1 = (USHORT *)&rgb_alpha_table[(NUM_ALPHA_LEVELS-1) - alpha][0];
        USHORT *alpha_table_src2 = (USHORT *)&rgb_alpha_table[alpha][0];

        // set interpolation restart value
        yrestart = y1;

        // what kind of triangle
        if (tri_type & TRI_TYPE_FLAT_MASK)
        {

            if (tri_type == TRI_TYPE_FLAT_TOP)
            {
                // compute all deltas
                dy = (y2 - y0);

                dxdyl = ((x2 - x0)   << FIXP16_SHIFT)/dy;
                dudyl = ((tu2 - tu0) << FIXP16_SHIFT)/dy;  
                dvdyl = ((tv2 - tv0) << FIXP16_SHIFT)/dy;    

                dxdyr = ((x2 - x1)   << FIXP16_SHIFT)/dy;
                dudyr = ((tu2 - tu1) << FIXP16_SHIFT)/dy;  
                dvdyr = ((tv2 - tv1) << FIXP16_SHIFT)/dy;   

                // test for y clipping
                if (y0 < min_clip_y)
                {
                    // compute overclip
                    dy = (min_clip_y - y0);

                    // computer new LHS starting values
                    xl = dxdyl*dy + (x0  << FIXP16_SHIFT);
                    ul = dudyl*dy + (tu0 << FIXP16_SHIFT);
                    vl = dvdyl*dy + (tv0 << FIXP16_SHIFT);

                    // compute new RHS starting values
                    xr = dxdyr*dy + (x1  << FIXP16_SHIFT);
                    ur = dudyr*dy + (tu1 << FIXP16_SHIFT);
                    vr = dvdyr*dy + (tv1 << FIXP16_SHIFT);

                    // compute new starting y
                    ystart = min_clip_y;

                } // end if
                else
                {
                    // no clipping

                    // set starting values
                    xl = (x0 << FIXP16_SHIFT);
                    xr = (x1 << FIXP16_SHIFT);

                    ul = (tu0 << FIXP16_SHIFT);
                    vl = (tv0 << FIXP16_SHIFT);

                    ur = (tu1 << FIXP16_SHIFT);
                    vr = (tv1 << FIXP16_SHIFT);

                    // set starting y
                    ystart = y0;

                } // end else

            } // end if flat top
            else
            {
                // must be flat bottom

                // compute all deltas
                dy = (y1 - y0);

                dxdyl = ((x1 - x0)   << FIXP16_SHIFT)/dy;
                dudyl = ((tu1 - tu0) << FIXP16_SHIFT)/dy;  
                dvdyl = ((tv1 - tv0) << FIXP16_SHIFT)/dy;    

                dxdyr = ((x2 - x0)   << FIXP16_SHIFT)/dy;
                dudyr = ((tu2 - tu0) << FIXP16_SHIFT)/dy;  
                dvdyr = ((tv2 - tv0) << FIXP16_SHIFT)/dy;   

                // test for y clipping
                if (y0 < min_clip_y)
                {
                    // compute overclip
                    dy = (min_clip_y - y0);

                    // computer new LHS starting values
                    xl = dxdyl*dy + (x0  << FIXP16_SHIFT);
                    ul = dudyl*dy + (tu0 << FIXP16_SHIFT);
                    vl = dvdyl*dy + (tv0 << FIXP16_SHIFT);

                    // compute new RHS starting values
                    xr = dxdyr*dy + (x0  << FIXP16_SHIFT);
                    ur = dudyr*dy + (tu0 << FIXP16_SHIFT);
                    vr = dvdyr*dy + (tv0 << FIXP16_SHIFT);

                    // compute new starting y
                    ystart = min_clip_y;

                } // end if
                else
                {
                    // no clipping

                    // set starting values
                    xl = (x0 << FIXP16_SHIFT);
                    xr = (x0 << FIXP16_SHIFT);

                    ul = (tu0 << FIXP16_SHIFT);
                    vl = (tv0 << FIXP16_SHIFT);

                    ur = (tu0 << FIXP16_SHIFT);
                    vr = (tv0 << FIXP16_SHIFT);

                    // set starting y
                    ystart = y0;

                } // end else	

            } // end else flat bottom

            // test for bottom clip, always
            if ((yend = y2) > max_clip_y)
                yend = max_clip_y;

            // test for horizontal clipping
            if ((x0 < min_clip_x) || (x0 > max_clip_x) ||
                (x1 < min_clip_x) || (x1 > max_clip_x) ||
                (x2 < min_clip_x) || (x2 > max_clip_x))
            {
                // clip version

                // point screen ptr to starting line
                screen_ptr = dest_buffer + (ystart * mem_pitch);

                for (yi = ystart; yi < yend; yi++)
                {
                    // compute span endpoints
                    xstart = ((xl + FIXP16_ROUND_UP) >> FIXP16_SHIFT);
                    xend   = ((xr + FIXP16_ROUND_UP) >> FIXP16_SHIFT);

                    // compute starting points for u,v interpolants
                    ui = ul + FIXP16_ROUND_UP;
                    vi = vl + FIXP16_ROUND_UP;

                    // compute u,v interpolants
                    if ((dx = (xend - xstart))>0)
                    {
                        du = (ur - ul)/dx;
                        dv = (vr - vl)/dx;
                    } // end if
                    else
                    {
                        du = (ur - ul);
                        dv = (vr - vl);
                    } // end else

                    ///////////////////////////////////////////////////////////////////////

                    // test for x clipping, LHS
                    if (xstart < min_clip_x)
                    {
                        // compute x overlap
                        dx = min_clip_x - xstart;

                        // slide interpolants over
                        ui+=dx*du;
                        vi+=dx*dv;

                        // reset vars
                        xstart = min_clip_x;

                    } // end if

                    // test for x clipping RHS
                    if (xend > max_clip_x)
                        xend = max_clip_x;

                    ///////////////////////////////////////////////////////////////////////

                    // draw span
                    for (xi=xstart; xi < xend; xi++)
                    {
                        // write textel
                        // check for transparency
                        textel = textmap[(ui >> FIXP16_SHIFT) + ((vi >> FIXP16_SHIFT) << texture_shift2)];

                        if (textel)
                        {
                            screen_ptr[xi] = alpha_table_src1[screen_ptr[xi]] + alpha_table_src2[textel];
                        } // end if


                        // interpolate u,v
                        ui+=du;
                        vi+=dv;
                    } // end for xi

                    // interpolate u,v,x along right and left edge
                    xl+=dxdyl;
                    ul+=dudyl;
                    vl+=dvdyl;

                    xr+=dxdyr;
                    ur+=dudyr;
                    vr+=dvdyr;

                    // advance screen ptr
                    screen_ptr+=mem_pitch;

                } // end for y

            } // end if clip
            else
            {
                // non-clip version

                // point screen ptr to starting line
                screen_ptr = dest_buffer + (ystart * mem_pitch);

                for (yi = ystart; yi < yend; yi++)
                {
                    // compute span endpoints
                    xstart = ((xl + FIXP16_ROUND_UP) >> FIXP16_SHIFT);
                    xend   = ((xr + FIXP16_ROUND_UP) >> FIXP16_SHIFT);

                    // compute starting points for u,v interpolants
                    ui = ul + FIXP16_ROUND_UP;
                    vi = vl + FIXP16_ROUND_UP;

                    // compute u,v interpolants
                    if ((dx = (xend - xstart))>0)
                    {
                        du = (ur - ul)/dx;
                        dv = (vr - vl)/dx;
                    } // end if
                    else
                    {
                        du = (ur - ul);
                        dv = (vr - vl);
                    } // end else

                    // draw span
                    for (xi=xstart; xi < xend; xi++)
                    {
                        // write textel
                        // check for transparency
                        textel = textmap[(ui >> FIXP16_SHIFT) + ((vi >> FIXP16_SHIFT) << texture_shift2)];

                        if (textel)
                        {
                            screen_ptr[xi] = alpha_table_src1[screen_ptr[xi]] + alpha_table_src2[textel];
                        } // end if

                        // interpolate u,v
                        ui+=du;
                        vi+=dv;
                    } // end for xi

                    // interpolate u,v,x along right and left edge
                    xl+=dxdyl;
                    ul+=dudyl;
                    vl+=dvdyl;

                    xr+=dxdyr;
                    ur+=dudyr;
                    vr+=dvdyr;

                    // advance screen ptr
                    screen_ptr+=mem_pitch;

                } // end for y

            } // end if non-clipped

        } // end if
        else
            if (tri_type==TRI_TYPE_GENERAL)
            {

                // first test for bottom clip, always
                if ((yend = y2) > max_clip_y)
                    yend = max_clip_y;

                // pre-test y clipping status
                if (y1 < min_clip_y)
                {
                    // compute all deltas
                    // LHS
                    dyl = (y2 - y1);

                    dxdyl = ((x2  - x1)  << FIXP16_SHIFT)/dyl;
                    dudyl = ((tu2 - tu1) << FIXP16_SHIFT)/dyl;  
                    dvdyl = ((tv2 - tv1) << FIXP16_SHIFT)/dyl;    

                    // RHS
                    dyr = (y2 - y0);	

                    dxdyr = ((x2  - x0)  << FIXP16_SHIFT)/dyr;
                    dudyr = ((tu2 - tu0) << FIXP16_SHIFT)/dyr;  
                    dvdyr = ((tv2 - tv0) << FIXP16_SHIFT)/dyr;   

                    // compute overclip
                    dyr = (min_clip_y - y0);
                    dyl = (min_clip_y - y1);

                    // computer new LHS starting values
                    xl = dxdyl*dyl + (x1  << FIXP16_SHIFT);
                    ul = dudyl*dyl + (tu1 << FIXP16_SHIFT);
                    vl = dvdyl*dyl + (tv1 << FIXP16_SHIFT);

                    // compute new RHS starting values
                    xr = dxdyr*dyr + (x0  << FIXP16_SHIFT);
                    ur = dudyr*dyr + (tu0 << FIXP16_SHIFT);
                    vr = dvdyr*dyr + (tv0 << FIXP16_SHIFT);

                    // compute new starting y
                    ystart = min_clip_y;

                    // test if we need swap to keep rendering left to right
                    if (dxdyr > dxdyl)
                    {
                        SWAP(dxdyl,dxdyr,temp);
                        SWAP(dudyl,dudyr,temp);
                        SWAP(dvdyl,dvdyr,temp);
                        SWAP(xl,xr,temp);
                        SWAP(ul,ur,temp);
                        SWAP(vl,vr,temp);
                        SWAP(x1,x2,temp);
                        SWAP(y1,y2,temp);
                        SWAP(tu1,tu2,temp);
                        SWAP(tv1,tv2,temp);

                        // set interpolation restart
                        irestart = INTERP_RHS;

                    } // end if

                } // end if
                else
                    if (y0 < min_clip_y)
                    {
                        // compute all deltas
                        // LHS
                        dyl = (y1 - y0);

                        dxdyl = ((x1  - x0)  << FIXP16_SHIFT)/dyl;
                        dudyl = ((tu1 - tu0) << FIXP16_SHIFT)/dyl;  
                        dvdyl = ((tv1 - tv0) << FIXP16_SHIFT)/dyl;    

                        // RHS
                        dyr = (y2 - y0);	

                        dxdyr = ((x2  - x0)  << FIXP16_SHIFT)/dyr;
                        dudyr = ((tu2 - tu0) << FIXP16_SHIFT)/dyr;  
                        dvdyr = ((tv2 - tv0) << FIXP16_SHIFT)/dyr;   

                        // compute overclip
                        dy = (min_clip_y - y0);

                        // computer new LHS starting values
                        xl = dxdyl*dy + (x0  << FIXP16_SHIFT);
                        ul = dudyl*dy + (tu0 << FIXP16_SHIFT);
                        vl = dvdyl*dy + (tv0 << FIXP16_SHIFT);

                        // compute new RHS starting values
                        xr = dxdyr*dy + (x0  << FIXP16_SHIFT);
                        ur = dudyr*dy + (tu0 << FIXP16_SHIFT);
                        vr = dvdyr*dy + (tv0 << FIXP16_SHIFT);

                        // compute new starting y
                        ystart = min_clip_y;

                        // test if we need swap to keep rendering left to right
                        if (dxdyr < dxdyl)
                        {
                            SWAP(dxdyl,dxdyr,temp);
                            SWAP(dudyl,dudyr,temp);
                            SWAP(dvdyl,dvdyr,temp);
                            SWAP(xl,xr,temp);
                            SWAP(ul,ur,temp);
                            SWAP(vl,vr,temp);
                            SWAP(x1,x2,temp);
                            SWAP(y1,y2,temp);
                            SWAP(tu1,tu2,temp);
                            SWAP(tv1,tv2,temp);

                            // set interpolation restart
                            irestart = INTERP_RHS;

                        } // end if

                    } // end if
                    else
                    {
                        // no initial y clipping

                        // compute all deltas
                        // LHS
                        dyl = (y1 - y0);

                        dxdyl = ((x1  - x0)  << FIXP16_SHIFT)/dyl;
                        dudyl = ((tu1 - tu0) << FIXP16_SHIFT)/dyl;  
                        dvdyl = ((tv1 - tv0) << FIXP16_SHIFT)/dyl;    

                        // RHS
                        dyr = (y2 - y0);	

                        dxdyr = ((x2 - x0)   << FIXP16_SHIFT)/dyr;
                        dudyr = ((tu2 - tu0) << FIXP16_SHIFT)/dyr;  
                        dvdyr = ((tv2 - tv0) << FIXP16_SHIFT)/dyr;   		

                        // no clipping y

                        // set starting values
                        xl = (x0 << FIXP16_SHIFT);
                        xr = (x0 << FIXP16_SHIFT);

                        ul = (tu0 << FIXP16_SHIFT);
                        vl = (tv0 << FIXP16_SHIFT);

                        ur = (tu0 << FIXP16_SHIFT);
                        vr = (tv0 << FIXP16_SHIFT);

                        // set starting y
                        ystart = y0;

                        // test if we need swap to keep rendering left to right
                        if (dxdyr < dxdyl)
                        {
                            SWAP(dxdyl,dxdyr,temp);
                            SWAP(dudyl,dudyr,temp);
                            SWAP(dvdyl,dvdyr,temp);
                            SWAP(xl,xr,temp);
                            SWAP(ul,ur,temp);
                            SWAP(vl,vr,temp);
                            SWAP(x1,x2,temp);
                            SWAP(y1,y2,temp);
                            SWAP(tu1,tu2,temp);
                            SWAP(tv1,tv2,temp);

                            // set interpolation restart
                            irestart = INTERP_RHS;

                        } // end if

                    } // end else


                    // test for horizontal clipping
                    if ((x0 < min_clip_x) || (x0 > max_clip_x) ||
                        (x1 < min_clip_x) || (x1 > max_clip_x) ||
                        (x2 < min_clip_x) || (x2 > max_clip_x))
                    {
                        // clip version
                        // x clipping	

                        // point screen ptr to starting line
                        screen_ptr = dest_buffer + (ystart * mem_pitch);

                        for (yi = ystart; yi < yend; yi++)
                        {
                            // compute span endpoints
                            xstart = ((xl + FIXP16_ROUND_UP) >> FIXP16_SHIFT);
                            xend   = ((xr + FIXP16_ROUND_UP) >> FIXP16_SHIFT);

                            // compute starting points for u,v interpolants
                            ui = ul + FIXP16_ROUND_UP;
                            vi = vl + FIXP16_ROUND_UP;

                            // compute u,v interpolants
                            if ((dx = (xend - xstart))>0)
                            {
                                du = (ur - ul)/dx;
                                dv = (vr - vl)/dx;
                            } // end if
                            else
                            {
                                du = (ur - ul);
                                dv = (vr - vl);
                            } // end else

                            ///////////////////////////////////////////////////////////////////////

                            // test for x clipping, LHS
                            if (xstart < min_clip_x)
                            {
                                // compute x overlap
                                dx = min_clip_x - xstart;

                                // slide interpolants over
                                ui+=dx*du;
                                vi+=dx*dv;

                                // set x to left clip edge
                                xstart = min_clip_x;

                            } // end if

                            // test for x clipping RHS
                            if (xend > max_clip_x)
                                xend = max_clip_x;

                            ///////////////////////////////////////////////////////////////////////

                            // draw span
                            for (xi=xstart; xi < xend; xi++)
                            {
                                // write textel
                                // check for transparency
                                textel = textmap[(ui >> FIXP16_SHIFT) + ((vi >> FIXP16_SHIFT) << texture_shift2)];

                                if (textel)
                                {
                                    screen_ptr[xi] = alpha_table_src1[screen_ptr[xi]] + alpha_table_src2[textel];
                                } // end if


                                // interpolate u,v
                                ui+=du;
                                vi+=dv;
                            } // end for xi

                            // interpolate u,v,x along right and left edge
                            xl+=dxdyl;
                            ul+=dudyl;
                            vl+=dvdyl;

                            xr+=dxdyr;
                            ur+=dudyr;
                            vr+=dvdyr;

                            // advance screen ptr
                            screen_ptr+=mem_pitch;

                            // test for yi hitting second region, if so change interpolant
                            if (yi==yrestart)
                            {

                                // test interpolation side change flag

                                if (irestart == INTERP_LHS)
                                {
                                    // LHS
                                    dyl = (y2 - y1);	

                                    dxdyl = ((x2 - x1)   << FIXP16_SHIFT)/dyl;
                                    dudyl = ((tu2 - tu1) << FIXP16_SHIFT)/dyl;  
                                    dvdyl = ((tv2 - tv1) << FIXP16_SHIFT)/dyl;   		

                                    // set starting values
                                    xl = (x1  << FIXP16_SHIFT);
                                    ul = (tu1 << FIXP16_SHIFT);
                                    vl = (tv1 << FIXP16_SHIFT);

                                    // interpolate down on LHS to even up
                                    xl+=dxdyl;
                                    ul+=dudyl;
                                    vl+=dvdyl;
                                } // end if
                                else
                                {
                                    // RHS
                                    dyr = (y1 - y2);	

                                    dxdyr = ((x1 - x2)   << FIXP16_SHIFT)/dyr;
                                    dudyr = ((tu1 - tu2) << FIXP16_SHIFT)/dyr;  
                                    dvdyr = ((tv1 - tv2) << FIXP16_SHIFT)/dyr;   		

                                    // set starting values
                                    xr = (x2  << FIXP16_SHIFT);
                                    ur = (tu2 << FIXP16_SHIFT);
                                    vr = (tv2 << FIXP16_SHIFT);

                                    // interpolate down on RHS to even up
                                    xr+=dxdyr;
                                    ur+=dudyr;
                                    vr+=dvdyr;

                                } // end else


                            } // end if

                        } // end for y

                    } // end if
                    else
                    {
                        // no x clipping
                        // point screen ptr to starting line
                        screen_ptr = dest_buffer + (ystart * mem_pitch);

                        for (yi = ystart; yi < yend; yi++)
                        {
                            // compute span endpoints
                            xstart = ((xl + FIXP16_ROUND_UP) >> FIXP16_SHIFT);
                            xend   = ((xr + FIXP16_ROUND_UP) >> FIXP16_SHIFT);

                            // compute starting points for u,v interpolants
                            ui = ul + FIXP16_ROUND_UP;
                            vi = vl + FIXP16_ROUND_UP;

                            // compute u,v interpolants
                            if ((dx = (xend - xstart))>0)

                            {
                                du = (ur - ul)/dx;
                                dv = (vr - vl)/dx;
                            } // end if
                            else
                            {
                                du = (ur - ul);
                                dv = (vr - vl);
                            } // end else

                            // draw span
                            for (xi=xstart; xi < xend; xi++)
                            {
                                // write textel
                                // check for transparency
                                textel = textmap[(ui >> FIXP16_SHIFT) + ((vi >> FIXP16_SHIFT) << texture_shift2)];

                                if (textel)
                                {
                                    screen_ptr[xi] = alpha_table_src1[screen_ptr[xi]] + alpha_table_src2[textel];
                                } // end if


                                // interpolate u,v
                                ui+=du;
                                vi+=dv;
                            } // end for xi

                            // interpolate u,v,x along right and left edge
                            xl+=dxdyl;
                            ul+=dudyl;
                            vl+=dvdyl;

                            xr+=dxdyr;
                            ur+=dudyr;
                            vr+=dvdyr;

                            // advance screen ptr
                            screen_ptr+=mem_pitch;

                            // test for yi hitting second region, if so change interpolant
                            if (yi==yrestart)
                            {
                                // test interpolation side change flag

                                if (irestart == INTERP_LHS)
                                {
                                    // LHS
                                    dyl = (y2 - y1);	

                                    dxdyl = ((x2 - x1)   << FIXP16_SHIFT)/dyl;
                                    dudyl = ((tu2 - tu1) << FIXP16_SHIFT)/dyl;  
                                    dvdyl = ((tv2 - tv1) << FIXP16_SHIFT)/dyl;   		

                                    // set starting values
                                    xl = (x1  << FIXP16_SHIFT);
                                    ul = (tu1 << FIXP16_SHIFT);
                                    vl = (tv1 << FIXP16_SHIFT);

                                    // interpolate down on LHS to even up
                                    xl+=dxdyl;
                                    ul+=dudyl;
                                    vl+=dvdyl;
                                } // end if
                                else
                                {
                                    // RHS
                                    dyr = (y1 - y2);	

                                    dxdyr = ((x1 - x2)   << FIXP16_SHIFT)/dyr;
                                    dudyr = ((tu1 - tu2) << FIXP16_SHIFT)/dyr;  
                                    dvdyr = ((tv1 - tv2) << FIXP16_SHIFT)/dyr;   		

                                    // set starting values
                                    xr = (x2  << FIXP16_SHIFT);
                                    ur = (tu2 << FIXP16_SHIFT);
                                    vr = (tv2 << FIXP16_SHIFT);

                                    // interpolate down on RHS to even up
                                    xr+=dxdyr;
                                    ur+=dudyr;
                                    vr+=dvdyr;

                                } // end else

                            } // end if

                        } // end for y

                    } // end else	

            } // end if

} // end Draw_Textured_Triangle_Alpha16_2

///////////////////////////////////////////////////////////////////////////////

void Draw_Textured_TriangleZB_Alpha16_2(POLYF4DV2_PTR face,  // ptr to face
                                        UCHAR *_dest_buffer,  // pointer to video buffer
                                        int mem_pitch,        // bytes per line, 320, 640 etc.
                                        UCHAR *_zbuffer,       // pointer to z-buffer
                                        int zpitch,          // bytes per line of zbuffer
                                        int alpha)
{
    // this function draws a textured triangle in 16-bit mode

    int v0=0,
        v1=1,
        v2=2,
        temp=0,
        tri_type = TRI_TYPE_NONE,
        irestart = INTERP_LHS;

    int dx,dy,dyl,dyr,      // general deltas
        u,v,z,
        du,dv,dz,
        xi,yi,              // the current interpolated x,y
        ui,vi,zi,           // the current interpolated u,v,z
        index_x,index_y,    // looping vars
        x,y,                // hold general x,y
        xstart,
        xend,
        ystart,
        yrestart,
        yend,
        xl,                 
        dxdyl,              
        xr,
        dxdyr,             
        dudyl,    
        ul,
        dvdyl,   
        vl,
        dzdyl,   
        zl,
        dudyr,
        ur,
        dvdyr,
        vr,
        dzdyr,
        zr;

    int x0,y0,tu0,tv0,tz0,    // cached vertices
        x1,y1,tu1,tv1,tz1,
        x2,y2,tu2,tv2,tz2;

    USHORT *screen_ptr  = NULL,
        *screen_line = NULL,
        *textmap     = NULL,
        *dest_buffer = (USHORT *)_dest_buffer,
        textel; 

    UINT  *z_ptr = NULL,
        *zbuffer = (UINT *)_zbuffer;

#ifdef DEBUG_ON
    // track rendering stats
    debug_polys_rendered_per_frame++;
#endif

    // extract texture map
    textmap = (USHORT *)face->texture->buffer;

    // extract base 2 of texture width
    int texture_shift2 = logbase2ofx[face->texture->width];

    // adjust memory pitch to words, divide by 2
    mem_pitch >>=1;

    // adjust zbuffer pitch for 32 bit alignment
    zpitch >>= 2;

    // apply fill convention to coordinates
    face->tvlist[0].x = (int)(face->tvlist[0].x+0.5);
    face->tvlist[0].y = (int)(face->tvlist[0].y+0.5);

    face->tvlist[1].x = (int)(face->tvlist[1].x+0.5);
    face->tvlist[1].y = (int)(face->tvlist[1].y+0.5);

    face->tvlist[2].x = (int)(face->tvlist[2].x+0.5);
    face->tvlist[2].y = (int)(face->tvlist[2].y+0.5);

    // first trivial clipping rejection tests 
    if (((face->tvlist[0].y < min_clip_y)  && 
        (face->tvlist[1].y < min_clip_y)  &&
        (face->tvlist[2].y < min_clip_y)) ||

        ((face->tvlist[0].y > max_clip_y)  && 
        (face->tvlist[1].y > max_clip_y)  &&
        (face->tvlist[2].y > max_clip_y)) ||

        ((face->tvlist[0].x < min_clip_x)  && 
        (face->tvlist[1].x < min_clip_x)  &&
        (face->tvlist[2].x < min_clip_x)) ||

        ((face->tvlist[0].x > max_clip_x)  && 
        (face->tvlist[1].x > max_clip_x)  &&
        (face->tvlist[2].x > max_clip_x)))
        return;

    // sort vertices
    if (face->tvlist[v1].y < face->tvlist[v0].y) 
    {SWAP(v0,v1,temp);} 

    if (face->tvlist[v2].y < face->tvlist[v0].y) 
    {SWAP(v0,v2,temp);}

    if (face->tvlist[v2].y < face->tvlist[v1].y) 
    {SWAP(v1,v2,temp);}

    // now test for trivial flat sided cases
    if (FCMP(face->tvlist[v0].y, face->tvlist[v1].y) )
    { 
        // set triangle type
        tri_type = TRI_TYPE_FLAT_TOP;

        // sort vertices left to right
        if (face->tvlist[v1].x < face->tvlist[v0].x) 
        {SWAP(v0,v1,temp);}

    } // end if
    else
        // now test for trivial flat sided cases
        if (FCMP(face->tvlist[v1].y ,face->tvlist[v2].y))
        { 
            // set triangle type
            tri_type = TRI_TYPE_FLAT_BOTTOM;

            // sort vertices left to right
            if (face->tvlist[v2].x < face->tvlist[v1].x) 
            {SWAP(v1,v2,temp);}

        } // end if
        else
        {
            // must be a general triangle
            tri_type = TRI_TYPE_GENERAL;

        } // end else

        // extract vertices for processing, now that we have order
        x0  = (int)(face->tvlist[v0].x+0.0);
        y0  = (int)(face->tvlist[v0].y+0.0);
        tu0 = (int)(face->tvlist[v0].u0);
        tv0 = (int)(face->tvlist[v0].v0);
        tz0 = (int)(face->tvlist[v0].z+0.5);

        x1  = (int)(face->tvlist[v1].x+0.0);
        y1  = (int)(face->tvlist[v1].y+0.0);
        tu1 = (int)(face->tvlist[v1].u0);
        tv1 = (int)(face->tvlist[v1].v0);
        tz1 = (int)(face->tvlist[v1].z+0.5);

        x2  = (int)(face->tvlist[v2].x+0.0);
        y2  = (int)(face->tvlist[v2].y+0.0);
        tu2 = (int)(face->tvlist[v2].u0);
        tv2 = (int)(face->tvlist[v2].v0);
        tz2 = (int)(face->tvlist[v2].z+0.5);


        // degenerate triangle
        if ( ((x0 == x1) && (x1 == x2)) || ((y0 ==  y1) && (y1 == y2)))
            return;

        // assign both source1 and source2 alpha tables based on polygon alpha level
        USHORT *alpha_table_src1 = (USHORT *)&rgb_alpha_table[(NUM_ALPHA_LEVELS-1) - alpha][0];
        USHORT *alpha_table_src2 = (USHORT *)&rgb_alpha_table[alpha][0];

        // set interpolation restart value
        yrestart = y1;

        // what kind of triangle
        if (tri_type & TRI_TYPE_FLAT_MASK)
        {

            if (tri_type == TRI_TYPE_FLAT_TOP)
            {
                // compute all deltas
                dy = (y2 - y0);

                dxdyl = ((x2 - x0)   << FIXP16_SHIFT)/dy;
                dudyl = ((tu2 - tu0) << FIXP16_SHIFT)/dy;  
                dvdyl = ((tv2 - tv0) << FIXP16_SHIFT)/dy;    
                dzdyl = ((tz2 - tz0) << FIXP16_SHIFT)/dy;    

                dxdyr = ((x2 - x1)   << FIXP16_SHIFT)/dy;
                dudyr = ((tu2 - tu1) << FIXP16_SHIFT)/dy;  
                dvdyr = ((tv2 - tv1) << FIXP16_SHIFT)/dy;   
                dzdyr = ((tz2 - tz1) << FIXP16_SHIFT)/dy;  

                // test for y clipping
                if (y0 < min_clip_y)
                {
                    // compute overclip
                    dy = (min_clip_y - y0);

                    // computer new LHS starting values
                    xl = dxdyl*dy + (x0  << FIXP16_SHIFT);
                    ul = dudyl*dy + (tu0 << FIXP16_SHIFT);
                    vl = dvdyl*dy + (tv0 << FIXP16_SHIFT);
                    zl = dzdyl*dy + (tz0 << FIXP16_SHIFT);

                    // compute new RHS starting values
                    xr = dxdyr*dy + (x1  << FIXP16_SHIFT);
                    ur = dudyr*dy + (tu1 << FIXP16_SHIFT);
                    vr = dvdyr*dy + (tv1 << FIXP16_SHIFT);
                    zr = dzdyr*dy + (tz1 << FIXP16_SHIFT);

                    // compute new starting y
                    ystart = min_clip_y;

                } // end if
                else
                {
                    // no clipping

                    // set starting values
                    xl = (x0 << FIXP16_SHIFT);
                    xr = (x1 << FIXP16_SHIFT);

                    ul = (tu0 << FIXP16_SHIFT);
                    vl = (tv0 << FIXP16_SHIFT);
                    zl = (tz0 << FIXP16_SHIFT);

                    ur = (tu1 << FIXP16_SHIFT);
                    vr = (tv1 << FIXP16_SHIFT);
                    zr = (tz1 << FIXP16_SHIFT);

                    // set starting y
                    ystart = y0;

                } // end else

            } // end if flat top
            else
            {
                // must be flat bottom

                // compute all deltas
                dy = (y1 - y0);

                dxdyl = ((x1 - x0)   << FIXP16_SHIFT)/dy;
                dudyl = ((tu1 - tu0) << FIXP16_SHIFT)/dy;  
                dvdyl = ((tv1 - tv0) << FIXP16_SHIFT)/dy;    
                dzdyl = ((tz1 - tz0) << FIXP16_SHIFT)/dy;   

                dxdyr = ((x2 - x0)   << FIXP16_SHIFT)/dy;
                dudyr = ((tu2 - tu0) << FIXP16_SHIFT)/dy;  
                dvdyr = ((tv2 - tv0) << FIXP16_SHIFT)/dy;   
                dzdyr = ((tz2 - tz0) << FIXP16_SHIFT)/dy;   

                // test for y clipping
                if (y0 < min_clip_y)
                {
                    // compute overclip
                    dy = (min_clip_y - y0);

                    // computer new LHS starting values
                    xl = dxdyl*dy + (x0  << FIXP16_SHIFT);
                    ul = dudyl*dy + (tu0 << FIXP16_SHIFT);
                    vl = dvdyl*dy + (tv0 << FIXP16_SHIFT);
                    zl = dzdyl*dy + (tz0 << FIXP16_SHIFT);

                    // compute new RHS starting values
                    xr = dxdyr*dy + (x0  << FIXP16_SHIFT);
                    ur = dudyr*dy + (tu0 << FIXP16_SHIFT);
                    vr = dvdyr*dy + (tv0 << FIXP16_SHIFT);
                    zr = dzdyr*dy + (tz0 << FIXP16_SHIFT);

                    // compute new starting y
                    ystart = min_clip_y;

                } // end if
                else
                {
                    // no clipping

                    // set starting values
                    xl = (x0 << FIXP16_SHIFT);
                    xr = (x0 << FIXP16_SHIFT);

                    ul = (tu0 << FIXP16_SHIFT);
                    vl = (tv0 << FIXP16_SHIFT);
                    zl = (tz0 << FIXP16_SHIFT);

                    ur = (tu0 << FIXP16_SHIFT);
                    vr = (tv0 << FIXP16_SHIFT);
                    zr = (tz0 << FIXP16_SHIFT);

                    // set starting y
                    ystart = y0;

                } // end else	

            } // end else flat bottom

            // test for bottom clip, always
            if ((yend = y2) > max_clip_y)
                yend = max_clip_y;

            // test for horizontal clipping
            if ((x0 < min_clip_x) || (x0 > max_clip_x) ||
                (x1 < min_clip_x) || (x1 > max_clip_x) ||
                (x2 < min_clip_x) || (x2 > max_clip_x))
            {
                // clip version

                // point screen ptr to starting line
                screen_ptr = dest_buffer + (ystart * mem_pitch);

                // point zbuffer to starting line
                z_ptr = zbuffer + (ystart * zpitch);

                for (yi = ystart; yi < yend; yi++)
                {
                    // compute span endpoints
                    xstart = ((xl + FIXP16_ROUND_UP) >> FIXP16_SHIFT);
                    xend   = ((xr + FIXP16_ROUND_UP) >> FIXP16_SHIFT);

                    // compute starting points for u,v interpolants
                    ui = ul + FIXP16_ROUND_UP;
                    vi = vl + FIXP16_ROUND_UP;
                    zi = zl + FIXP16_ROUND_UP;

                    // compute u,v interpolants
                    if ((dx = (xend - xstart))>0)
                    {
                        du = (ur - ul)/dx;
                        dv = (vr - vl)/dx;
                        dz = (zr - zl)/dx;
                    } // end if
                    else
                    {
                        du = (ur - ul);
                        dv = (vr - vl);
                        dz = (zr - zl);
                    } // end else

                    ///////////////////////////////////////////////////////////////////////

                    // test for x clipping, LHS
                    if (xstart < min_clip_x)
                    {
                        // compute x overlap
                        dx = min_clip_x - xstart;

                        // slide interpolants over
                        ui+=dx*du;
                        vi+=dx*dv;
                        zi+=dx*dz;

                        // reset vars
                        xstart = min_clip_x;

                    } // end if

                    // test for x clipping RHS
                    if (xend > max_clip_x)
                        xend = max_clip_x;

                    ///////////////////////////////////////////////////////////////////////

                    // draw span
                    for (xi=xstart; xi < xend; xi++)
                    {

                        // get textel
                        textel = textmap[(ui >> FIXP16_SHIFT) + ((vi >> FIXP16_SHIFT) << texture_shift2)];

                        if (textel)
                        {
                            // test if z of current pixel is nearer than current z buffer value
                            if (zi < z_ptr[xi])
                            {
                                // write textel
                                screen_ptr[xi] = alpha_table_src1[screen_ptr[xi]] + 
                                    alpha_table_src2[textel];


                                // update z-buffer
                                z_ptr[xi] = zi;           
                            } // end if

                        } // end if			


                        // interpolate u,v,z
                        ui+=du;
                        vi+=dv;
                        zi+=dz;
                    } // end for xi

                    // interpolate u,v,x along right and left edge
                    xl+=dxdyl;
                    ul+=dudyl;
                    vl+=dvdyl;
                    zl+=dzdyl;

                    xr+=dxdyr;
                    ur+=dudyr;
                    vr+=dvdyr;
                    zr+=dzdyr;

                    // advance screen ptr
                    screen_ptr+=mem_pitch;

                    // advance zbuffer ptr
                    z_ptr+=zpitch;

                } // end for y

            } // end if clip
            else
            {
                // non-clip version

                // point screen ptr to starting line
                screen_ptr = dest_buffer + (ystart * mem_pitch);

                // point zbuffer to starting line
                z_ptr = zbuffer + (ystart * zpitch);

                for (yi = ystart; yi < yend; yi++)
                {
                    // compute span endpoints
                    xstart = ((xl + FIXP16_ROUND_UP) >> FIXP16_SHIFT);
                    xend   = ((xr + FIXP16_ROUND_UP) >> FIXP16_SHIFT);

                    // compute starting points for u,v interpolants
                    ui = ul + FIXP16_ROUND_UP;
                    vi = vl + FIXP16_ROUND_UP;
                    zi = zl + FIXP16_ROUND_UP;

                    // compute u,v interpolants
                    if ((dx = (xend - xstart))>0)
                    {
                        du = (ur - ul)/dx;
                        dv = (vr - vl)/dx;
                        dz = (zr - zl)/dx;
                    } // end if
                    else
                    {
                        du = (ur - ul);
                        dv = (vr - vl);
                        dz = (zr - zl);
                    } // end else

                    // draw span
                    for (xi=xstart; xi < xend; xi++)
                    {
                        // get textel
                        textel = textmap[(ui >> FIXP16_SHIFT) + ((vi >> FIXP16_SHIFT) << texture_shift2)];

                        if (textel)
                        {
                            // test if z of current pixel is nearer than current z buffer value
                            if (zi < z_ptr[xi])
                            {
                                // write textel
                                screen_ptr[xi] = alpha_table_src1[screen_ptr[xi]] + 
                                    alpha_table_src2[textel];


                                // update z-buffer
                                z_ptr[xi] = zi;           
                            } // end if

                        } // end if		

                        // interpolate u,v,z
                        ui+=du;
                        vi+=dv;
                        zi+=dz;
                    } // end for xi

                    // interpolate u,v,x along right and left edge
                    xl+=dxdyl;
                    ul+=dudyl;
                    vl+=dvdyl;
                    zl+=dzdyl;

                    xr+=dxdyr;
                    ur+=dudyr;
                    vr+=dvdyr;
                    zr+=dzdyr;

                    // advance screen ptr
                    screen_ptr+=mem_pitch;

                    // advance zbuffer ptr
                    z_ptr+=zpitch;

                } // end for y

            } // end if non-clipped

        } // end if
        else
            if (tri_type==TRI_TYPE_GENERAL)
            {

                // first test for bottom clip, always
                if ((yend = y2) > max_clip_y)
                    yend = max_clip_y;

                // pre-test y clipping status
                if (y1 < min_clip_y)
                {
                    // compute all deltas
                    // LHS
                    dyl = (y2 - y1);

                    dxdyl = ((x2  - x1)  << FIXP16_SHIFT)/dyl;
                    dudyl = ((tu2 - tu1) << FIXP16_SHIFT)/dyl;  
                    dvdyl = ((tv2 - tv1) << FIXP16_SHIFT)/dyl;    
                    dzdyl = ((tz2 - tz1) << FIXP16_SHIFT)/dyl;  

                    // RHS
                    dyr = (y2 - y0);	

                    dxdyr = ((x2  - x0)  << FIXP16_SHIFT)/dyr;
                    dudyr = ((tu2 - tu0) << FIXP16_SHIFT)/dyr;  
                    dvdyr = ((tv2 - tv0) << FIXP16_SHIFT)/dyr;   
                    dzdyr = ((tz2 - tz0) << FIXP16_SHIFT)/dyr;   

                    // compute overclip
                    dyr = (min_clip_y - y0);
                    dyl = (min_clip_y - y1);

                    // computer new LHS starting values
                    xl = dxdyl*dyl + (x1  << FIXP16_SHIFT);
                    ul = dudyl*dyl + (tu1 << FIXP16_SHIFT);
                    vl = dvdyl*dyl + (tv1 << FIXP16_SHIFT);
                    zl = dzdyl*dyl + (tz1 << FIXP16_SHIFT);

                    // compute new RHS starting values
                    xr = dxdyr*dyr + (x0  << FIXP16_SHIFT);
                    ur = dudyr*dyr + (tu0 << FIXP16_SHIFT);
                    vr = dvdyr*dyr + (tv0 << FIXP16_SHIFT);
                    zr = dzdyr*dyr + (tz0 << FIXP16_SHIFT);

                    // compute new starting y
                    ystart = min_clip_y;

                    // test if we need swap to keep rendering left to right
                    if (dxdyr > dxdyl)
                    {
                        SWAP(dxdyl,dxdyr,temp);
                        SWAP(dudyl,dudyr,temp);
                        SWAP(dvdyl,dvdyr,temp);
                        SWAP(dzdyl,dzdyr,temp);
                        SWAP(xl,xr,temp);
                        SWAP(ul,ur,temp);
                        SWAP(vl,vr,temp);
                        SWAP(zl,zr,temp);
                        SWAP(x1,x2,temp);
                        SWAP(y1,y2,temp);
                        SWAP(tu1,tu2,temp);
                        SWAP(tv1,tv2,temp);
                        SWAP(tz1,tz2,temp);

                        // set interpolation restart
                        irestart = INTERP_RHS;

                    } // end if

                } // end if
                else
                    if (y0 < min_clip_y)
                    {
                        // compute all deltas
                        // LHS
                        dyl = (y1 - y0);

                        dxdyl = ((x1  - x0)  << FIXP16_SHIFT)/dyl;
                        dudyl = ((tu1 - tu0) << FIXP16_SHIFT)/dyl;  
                        dvdyl = ((tv1 - tv0) << FIXP16_SHIFT)/dyl;    
                        dzdyl = ((tz1 - tz0) << FIXP16_SHIFT)/dyl;  

                        // RHS
                        dyr = (y2 - y0);	

                        dxdyr = ((x2  - x0)  << FIXP16_SHIFT)/dyr;
                        dudyr = ((tu2 - tu0) << FIXP16_SHIFT)/dyr;  
                        dvdyr = ((tv2 - tv0) << FIXP16_SHIFT)/dyr;   
                        dzdyr = ((tz2 - tz0) << FIXP16_SHIFT)/dyr;   

                        // compute overclip
                        dy = (min_clip_y - y0);

                        // computer new LHS starting values
                        xl = dxdyl*dy + (x0  << FIXP16_SHIFT);
                        ul = dudyl*dy + (tu0 << FIXP16_SHIFT);
                        vl = dvdyl*dy + (tv0 << FIXP16_SHIFT);
                        zl = dzdyl*dy + (tz0 << FIXP16_SHIFT);

                        // compute new RHS starting values
                        xr = dxdyr*dy + (x0  << FIXP16_SHIFT);
                        ur = dudyr*dy + (tu0 << FIXP16_SHIFT);
                        vr = dvdyr*dy + (tv0 << FIXP16_SHIFT);
                        zr = dzdyr*dy + (tz0 << FIXP16_SHIFT);

                        // compute new starting y
                        ystart = min_clip_y;

                        // test if we need swap to keep rendering left to right
                        if (dxdyr < dxdyl)
                        {
                            SWAP(dxdyl,dxdyr,temp);
                            SWAP(dudyl,dudyr,temp);
                            SWAP(dvdyl,dvdyr,temp);
                            SWAP(dzdyl,dzdyr,temp);
                            SWAP(xl,xr,temp);
                            SWAP(ul,ur,temp);
                            SWAP(vl,vr,temp);
                            SWAP(zl,zr,temp);
                            SWAP(x1,x2,temp);
                            SWAP(y1,y2,temp);
                            SWAP(tu1,tu2,temp);
                            SWAP(tv1,tv2,temp);
                            SWAP(tz1,tz2,temp);

                            // set interpolation restart
                            irestart = INTERP_RHS;

                        } // end if

                    } // end if
                    else
                    {
                        // no initial y clipping

                        // compute all deltas
                        // LHS
                        dyl = (y1 - y0);

                        dxdyl = ((x1  - x0)  << FIXP16_SHIFT)/dyl;
                        dudyl = ((tu1 - tu0) << FIXP16_SHIFT)/dyl;  
                        dvdyl = ((tv1 - tv0) << FIXP16_SHIFT)/dyl;    
                        dzdyl = ((tz1 - tz0) << FIXP16_SHIFT)/dyl;   

                        // RHS
                        dyr = (y2 - y0);	

                        dxdyr = ((x2 - x0)   << FIXP16_SHIFT)/dyr;
                        dudyr = ((tu2 - tu0) << FIXP16_SHIFT)/dyr;  
                        dvdyr = ((tv2 - tv0) << FIXP16_SHIFT)/dyr;   		
                        dzdyr = ((tz2 - tz0) << FIXP16_SHIFT)/dyr;  

                        // no clipping y

                        // set starting values
                        xl = (x0 << FIXP16_SHIFT);
                        xr = (x0 << FIXP16_SHIFT);

                        ul = (tu0 << FIXP16_SHIFT);
                        vl = (tv0 << FIXP16_SHIFT);
                        zl = (tz0 << FIXP16_SHIFT);

                        ur = (tu0 << FIXP16_SHIFT);
                        vr = (tv0 << FIXP16_SHIFT);
                        zr = (tz0 << FIXP16_SHIFT);

                        // set starting y
                        ystart = y0;

                        // test if we need swap to keep rendering left to right
                        if (dxdyr < dxdyl)
                        {
                            SWAP(dxdyl,dxdyr,temp);
                            SWAP(dudyl,dudyr,temp);
                            SWAP(dvdyl,dvdyr,temp);
                            SWAP(dzdyl,dzdyr,temp);
                            SWAP(xl,xr,temp);
                            SWAP(ul,ur,temp);
                            SWAP(vl,vr,temp);
                            SWAP(zl,zr,temp);
                            SWAP(x1,x2,temp);
                            SWAP(y1,y2,temp);
                            SWAP(tu1,tu2,temp);
                            SWAP(tv1,tv2,temp);
                            SWAP(tz1,tz2,temp);

                            // set interpolation restart
                            irestart = INTERP_RHS;

                        } // end if

                    } // end else

                    // test for horizontal clipping
                    if ((x0 < min_clip_x) || (x0 > max_clip_x) ||
                        (x1 < min_clip_x) || (x1 > max_clip_x) ||
                        (x2 < min_clip_x) || (x2 > max_clip_x))
                    {
                        // clip version
                        // x clipping	

                        // point screen ptr to starting line
                        screen_ptr = dest_buffer + (ystart * mem_pitch);

                        // point zbuffer to starting line
                        z_ptr = zbuffer + (ystart * zpitch);

                        for (yi = ystart; yi < yend; yi++)
                        {
                            // compute span endpoints
                            xstart = ((xl + FIXP16_ROUND_UP) >> FIXP16_SHIFT);
                            xend   = ((xr + FIXP16_ROUND_UP) >> FIXP16_SHIFT);

                            // compute starting points for u,v interpolants
                            ui = ul + FIXP16_ROUND_UP;
                            vi = vl + FIXP16_ROUND_UP;
                            zi = zl + FIXP16_ROUND_UP;

                            // compute u,v interpolants
                            if ((dx = (xend - xstart))>0)
                            {
                                du = (ur - ul)/dx;
                                dv = (vr - vl)/dx;
                                dz = (zr - zl)/dx;
                            } // end if
                            else
                            {
                                du = (ur - ul);
                                dv = (vr - vl);
                                dz = (zr - zl);
                            } // end else

                            ///////////////////////////////////////////////////////////////////////

                            // test for x clipping, LHS
                            if (xstart < min_clip_x)
                            {
                                // compute x overlap
                                dx = min_clip_x - xstart;

                                // slide interpolants over
                                ui+=dx*du;
                                vi+=dx*dv;
                                zi+=dx*dz;

                                // set x to left clip edge
                                xstart = min_clip_x;

                            } // end if

                            // test for x clipping RHS
                            if (xend > max_clip_x)
                                xend = max_clip_x;

                            ///////////////////////////////////////////////////////////////////////

                            // draw span
                            for (xi=xstart; xi < xend; xi++)
                            {
                                // get textel
                                textel = textmap[(ui >> FIXP16_SHIFT) + ((vi >> FIXP16_SHIFT) << texture_shift2)];

                                if (textel)
                                {
                                    // test if z of current pixel is nearer than current z buffer value
                                    if (zi < z_ptr[xi])
                                    {
                                        // write textel
                                        screen_ptr[xi] = alpha_table_src1[screen_ptr[xi]] + 
                                            alpha_table_src2[textel];


                                        // update z-buffer
                                        z_ptr[xi] = zi;           
                                    } // end if

                                } // end if		

                                // interpolate u,v,z
                                ui+=du;
                                vi+=dv;
                                zi+=dz;
                            } // end for xi

                            // interpolate u,v,x along right and left edge
                            xl+=dxdyl;
                            ul+=dudyl;
                            vl+=dvdyl;
                            zl+=dzdyl;

                            xr+=dxdyr;
                            ur+=dudyr;
                            vr+=dvdyr;
                            zr+=dzdyr;

                            // advance screen ptr
                            screen_ptr+=mem_pitch;

                            // advance zbuffer ptr
                            z_ptr+=zpitch;

                            // test for yi hitting second region, if so change interpolant
                            if (yi==yrestart)
                            {
                                // test interpolation side change flag

                                if (irestart == INTERP_LHS)
                                {
                                    // LHS
                                    dyl = (y2 - y1);	

                                    dxdyl = ((x2 - x1)   << FIXP16_SHIFT)/dyl;
                                    dudyl = ((tu2 - tu1) << FIXP16_SHIFT)/dyl;  
                                    dvdyl = ((tv2 - tv1) << FIXP16_SHIFT)/dyl;   		
                                    dzdyl = ((tz2 - tz1) << FIXP16_SHIFT)/dyl;   

                                    // set starting values
                                    xl = (x1  << FIXP16_SHIFT);
                                    ul = (tu1 << FIXP16_SHIFT);
                                    vl = (tv1 << FIXP16_SHIFT);
                                    zl = (tz1 << FIXP16_SHIFT);

                                    // interpolate down on LHS to even up
                                    xl+=dxdyl;
                                    ul+=dudyl;
                                    vl+=dvdyl;
                                    zl+=dzdyl;
                                } // end if
                                else
                                {
                                    // RHS
                                    dyr = (y1 - y2);	

                                    dxdyr = ((x1 - x2)   << FIXP16_SHIFT)/dyr;
                                    dudyr = ((tu1 - tu2) << FIXP16_SHIFT)/dyr;  
                                    dvdyr = ((tv1 - tv2) << FIXP16_SHIFT)/dyr;   		
                                    dzdyr = ((tz1 - tz2) << FIXP16_SHIFT)/dyr;  

                                    // set starting values
                                    xr = (x2  << FIXP16_SHIFT);
                                    ur = (tu2 << FIXP16_SHIFT);
                                    vr = (tv2 << FIXP16_SHIFT);
                                    zr = (tz2 << FIXP16_SHIFT);

                                    // interpolate down on RHS to even up
                                    xr+=dxdyr;
                                    ur+=dudyr;
                                    vr+=dvdyr;
                                    zr+=dzdyr;

                                } // end else

                            } // end if

                        } // end for y

                    } // end if
                    else
                    {
                        // no x clipping
                        // point screen ptr to starting line
                        screen_ptr = dest_buffer + (ystart * mem_pitch);

                        // point zbuffer to starting line
                        z_ptr = zbuffer + (ystart * zpitch);

                        for (yi = ystart; yi < yend; yi++)
                        {
                            // compute span endpoints
                            xstart = ((xl + FIXP16_ROUND_UP) >> FIXP16_SHIFT);
                            xend   = ((xr + FIXP16_ROUND_UP) >> FIXP16_SHIFT);

                            // compute starting points for u,v interpolants
                            ui = ul + FIXP16_ROUND_UP;
                            vi = vl + FIXP16_ROUND_UP;
                            zi = zl + FIXP16_ROUND_UP;

                            // compute u,v interpolants
                            if ((dx = (xend - xstart))>0)
                            {
                                du = (ur - ul)/dx;
                                dv = (vr - vl)/dx;
                                dz = (zr - zl)/dx;
                            } // end if
                            else
                            {
                                du = (ur - ul);
                                dv = (vr - vl);
                                dz = (zr - zl);
                            } // end else

                            // draw span
                            for (xi=xstart; xi < xend; xi++)
                            {
                                // test if z of current pixel is nearer than current z buffer value
                                // get textel
                                textel = textmap[(ui >> FIXP16_SHIFT) + ((vi >> FIXP16_SHIFT) << texture_shift2)];

                                if (textel)
                                {
                                    // test if z of current pixel is nearer than current z buffer value
                                    if (zi < z_ptr[xi])
                                    {
                                        // write textel
                                        screen_ptr[xi] = alpha_table_src1[screen_ptr[xi]] + 
                                            alpha_table_src2[textel];


                                        // update z-buffer
                                        z_ptr[xi] = zi;           
                                    } // end if

                                } // end if		

                                // interpolate u,v
                                ui+=du;
                                vi+=dv;
                                zi+=dz;
                            } // end for xi

                            // interpolate u,v,x along right and left edge
                            xl+=dxdyl;
                            ul+=dudyl;
                            vl+=dvdyl;
                            zl+=dzdyl;

                            xr+=dxdyr;
                            ur+=dudyr;
                            vr+=dvdyr;
                            zr+=dzdyr;

                            // advance screen ptr
                            screen_ptr+=mem_pitch;

                            // advance zbuffer ptr
                            z_ptr+=zpitch;

                            // test for yi hitting second region, if so change interpolant
                            if (yi==yrestart)
                            {
                                // test interpolation side change flag

                                if (irestart == INTERP_LHS)
                                {
                                    // LHS
                                    dyl = (y2 - y1);	

                                    dxdyl = ((x2 - x1)   << FIXP16_SHIFT)/dyl;
                                    dudyl = ((tu2 - tu1) << FIXP16_SHIFT)/dyl;  
                                    dvdyl = ((tv2 - tv1) << FIXP16_SHIFT)/dyl;   		
                                    dzdyl = ((tz2 - tz1) << FIXP16_SHIFT)/dyl;   

                                    // set starting values
                                    xl = (x1  << FIXP16_SHIFT);
                                    ul = (tu1 << FIXP16_SHIFT);
                                    vl = (tv1 << FIXP16_SHIFT);
                                    zl = (tz1 << FIXP16_SHIFT);

                                    // interpolate down on LHS to even up
                                    xl+=dxdyl;
                                    ul+=dudyl;
                                    vl+=dvdyl;
                                    zl+=dzdyl;
                                } // end if
                                else
                                {
                                    // RHS
                                    dyr = (y1 - y2);	

                                    dxdyr = ((x1 - x2)   << FIXP16_SHIFT)/dyr;
                                    dudyr = ((tu1 - tu2) << FIXP16_SHIFT)/dyr;  
                                    dvdyr = ((tv1 - tv2) << FIXP16_SHIFT)/dyr;   		
                                    dzdyr = ((tz1 - tz2) << FIXP16_SHIFT)/dyr; 

                                    // set starting values
                                    xr = (x2  << FIXP16_SHIFT);
                                    ur = (tu2 << FIXP16_SHIFT);
                                    vr = (tv2 << FIXP16_SHIFT);
                                    zr = (tz2 << FIXP16_SHIFT);

                                    // interpolate down on RHS to even up
                                    xr+=dxdyr;
                                    ur+=dudyr;
                                    vr+=dvdyr;
                                    zr+=dzdyr;

                                } // end else

                            } // end if

                        } // end for y

                    } // end else	

            } // end if

} // end Draw_Textured_TriangleZB_Alpha16_2

///////////////////////////////////////////////////////////////////////////////

void Draw_Textured_TriangleWTZB_Alpha16_2(POLYF4DV2_PTR face,  // ptr to face
                                          UCHAR *_dest_buffer,  // pointer to video buffer
                                          int mem_pitch,        // bytes per line, 320, 640 etc.
                                          UCHAR *_zbuffer,       // pointer to z-buffer
                                          int zpitch,          // bytes per line of zbuffer
                                          int alpha)
{
    // this function draws a textured triangle in 16-bit mode

    int v0=0,
        v1=1,
        v2=2,
        temp=0,
        tri_type = TRI_TYPE_NONE,
        irestart = INTERP_LHS;

    int dx,dy,dyl,dyr,      // general deltas
        u,v,z,
        du,dv,dz,
        xi,yi,              // the current interpolated x,y
        ui,vi,zi,           // the current interpolated u,v,z
        index_x,index_y,    // looping vars
        x,y,                // hold general x,y
        xstart,
        xend,
        ystart,
        yrestart,
        yend,
        xl,                 
        dxdyl,              
        xr,
        dxdyr,             
        dudyl,    
        ul,
        dvdyl,   
        vl,
        dzdyl,   
        zl,
        dudyr,
        ur,
        dvdyr,
        vr,
        dzdyr,
        zr;

    int x0,y0,tu0,tv0,tz0,    // cached vertices
        x1,y1,tu1,tv1,tz1,
        x2,y2,tu2,tv2,tz2;

    USHORT *screen_ptr  = NULL,
        *screen_line = NULL,
        *textmap     = NULL,
        *dest_buffer = (USHORT *)_dest_buffer,
        textel; 

    UINT  *z_ptr = NULL,
        *zbuffer = (UINT *)_zbuffer;

#ifdef DEBUG_ON
    // track rendering stats
    debug_polys_rendered_per_frame++;
#endif

    // extract texture map
    textmap = (USHORT *)face->texture->buffer;

    // extract base 2 of texture width
    int texture_shift2 = logbase2ofx[face->texture->width];

    // adjust memory pitch to words, divide by 2
    mem_pitch >>=1;

    // adjust zbuffer pitch for 32 bit alignment
    zpitch >>= 2;

    // apply fill convention to coordinates
    face->tvlist[0].x = (int)(face->tvlist[0].x+0.5);
    face->tvlist[0].y = (int)(face->tvlist[0].y+0.5);

    face->tvlist[1].x = (int)(face->tvlist[1].x+0.5);
    face->tvlist[1].y = (int)(face->tvlist[1].y+0.5);

    face->tvlist[2].x = (int)(face->tvlist[2].x+0.5);
    face->tvlist[2].y = (int)(face->tvlist[2].y+0.5);

    // first trivial clipping rejection tests 
    if (((face->tvlist[0].y < min_clip_y)  && 
        (face->tvlist[1].y < min_clip_y)  &&
        (face->tvlist[2].y < min_clip_y)) ||

        ((face->tvlist[0].y > max_clip_y)  && 
        (face->tvlist[1].y > max_clip_y)  &&
        (face->tvlist[2].y > max_clip_y)) ||

        ((face->tvlist[0].x < min_clip_x)  && 
        (face->tvlist[1].x < min_clip_x)  &&
        (face->tvlist[2].x < min_clip_x)) ||

        ((face->tvlist[0].x > max_clip_x)  && 
        (face->tvlist[1].x > max_clip_x)  &&
        (face->tvlist[2].x > max_clip_x)))
        return;

    // sort vertices
    if (face->tvlist[v1].y < face->tvlist[v0].y) 
    {SWAP(v0,v1,temp);} 

    if (face->tvlist[v2].y < face->tvlist[v0].y) 
    {SWAP(v0,v2,temp);}

    if (face->tvlist[v2].y < face->tvlist[v1].y) 
    {SWAP(v1,v2,temp);}

    // now test for trivial flat sided cases
    if (FCMP(face->tvlist[v0].y, face->tvlist[v1].y) )
    { 
        // set triangle type
        tri_type = TRI_TYPE_FLAT_TOP;

        // sort vertices left to right
        if (face->tvlist[v1].x < face->tvlist[v0].x) 
        {SWAP(v0,v1,temp);}

    } // end if
    else
        // now test for trivial flat sided cases
        if (FCMP(face->tvlist[v1].y ,face->tvlist[v2].y))
        { 
            // set triangle type
            tri_type = TRI_TYPE_FLAT_BOTTOM;

            // sort vertices left to right
            if (face->tvlist[v2].x < face->tvlist[v1].x) 
            {SWAP(v1,v2,temp);}

        } // end if
        else
        {
            // must be a general triangle
            tri_type = TRI_TYPE_GENERAL;

        } // end else

        // extract vertices for processing, now that we have order
        x0  = (int)(face->tvlist[v0].x+0.0);
        y0  = (int)(face->tvlist[v0].y+0.0);
        tu0 = (int)(face->tvlist[v0].u0);
        tv0 = (int)(face->tvlist[v0].v0);
        tz0 = (int)(face->tvlist[v0].z+0.5);

        x1  = (int)(face->tvlist[v1].x+0.0);
        y1  = (int)(face->tvlist[v1].y+0.0);
        tu1 = (int)(face->tvlist[v1].u0);
        tv1 = (int)(face->tvlist[v1].v0);
        tz1 = (int)(face->tvlist[v1].z+0.5);

        x2  = (int)(face->tvlist[v2].x+0.0);
        y2  = (int)(face->tvlist[v2].y+0.0);
        tu2 = (int)(face->tvlist[v2].u0);
        tv2 = (int)(face->tvlist[v2].v0);
        tz2 = (int)(face->tvlist[v2].z+0.5);


        // degenerate triangle
        if ( ((x0 == x1) && (x1 == x2)) || ((y0 ==  y1) && (y1 == y2)))
            return;

        // assign both source1 and source2 alpha tables based on polygon alpha level
        USHORT *alpha_table_src1 = (USHORT *)&rgb_alpha_table[(NUM_ALPHA_LEVELS-1) - alpha][0];
        USHORT *alpha_table_src2 = (USHORT *)&rgb_alpha_table[alpha][0];

        // set interpolation restart value
        yrestart = y1;

        // what kind of triangle
        if (tri_type & TRI_TYPE_FLAT_MASK)
        {

            if (tri_type == TRI_TYPE_FLAT_TOP)
            {
                // compute all deltas
                dy = (y2 - y0);

                dxdyl = ((x2 - x0)   << FIXP16_SHIFT)/dy;
                dudyl = ((tu2 - tu0) << FIXP16_SHIFT)/dy;  
                dvdyl = ((tv2 - tv0) << FIXP16_SHIFT)/dy;    
                dzdyl = ((tz2 - tz0) << FIXP16_SHIFT)/dy;    

                dxdyr = ((x2 - x1)   << FIXP16_SHIFT)/dy;
                dudyr = ((tu2 - tu1) << FIXP16_SHIFT)/dy;  
                dvdyr = ((tv2 - tv1) << FIXP16_SHIFT)/dy;   
                dzdyr = ((tz2 - tz1) << FIXP16_SHIFT)/dy;  

                // test for y clipping
                if (y0 < min_clip_y)
                {
                    // compute overclip
                    dy = (min_clip_y - y0);

                    // computer new LHS starting values
                    xl = dxdyl*dy + (x0  << FIXP16_SHIFT);
                    ul = dudyl*dy + (tu0 << FIXP16_SHIFT);
                    vl = dvdyl*dy + (tv0 << FIXP16_SHIFT);
                    zl = dzdyl*dy + (tz0 << FIXP16_SHIFT);

                    // compute new RHS starting values
                    xr = dxdyr*dy + (x1  << FIXP16_SHIFT);
                    ur = dudyr*dy + (tu1 << FIXP16_SHIFT);
                    vr = dvdyr*dy + (tv1 << FIXP16_SHIFT);
                    zr = dzdyr*dy + (tz1 << FIXP16_SHIFT);

                    // compute new starting y
                    ystart = min_clip_y;

                } // end if
                else
                {
                    // no clipping

                    // set starting values
                    xl = (x0 << FIXP16_SHIFT);
                    xr = (x1 << FIXP16_SHIFT);

                    ul = (tu0 << FIXP16_SHIFT);
                    vl = (tv0 << FIXP16_SHIFT);
                    zl = (tz0 << FIXP16_SHIFT);

                    ur = (tu1 << FIXP16_SHIFT);
                    vr = (tv1 << FIXP16_SHIFT);
                    zr = (tz1 << FIXP16_SHIFT);

                    // set starting y
                    ystart = y0;

                } // end else

            } // end if flat top
            else
            {
                // must be flat bottom

                // compute all deltas
                dy = (y1 - y0);

                dxdyl = ((x1 - x0)   << FIXP16_SHIFT)/dy;
                dudyl = ((tu1 - tu0) << FIXP16_SHIFT)/dy;  
                dvdyl = ((tv1 - tv0) << FIXP16_SHIFT)/dy;    
                dzdyl = ((tz1 - tz0) << FIXP16_SHIFT)/dy;   

                dxdyr = ((x2 - x0)   << FIXP16_SHIFT)/dy;
                dudyr = ((tu2 - tu0) << FIXP16_SHIFT)/dy;  
                dvdyr = ((tv2 - tv0) << FIXP16_SHIFT)/dy;   
                dzdyr = ((tz2 - tz0) << FIXP16_SHIFT)/dy;   

                // test for y clipping
                if (y0 < min_clip_y)
                {
                    // compute overclip
                    dy = (min_clip_y - y0);

                    // computer new LHS starting values
                    xl = dxdyl*dy + (x0  << FIXP16_SHIFT);
                    ul = dudyl*dy + (tu0 << FIXP16_SHIFT);
                    vl = dvdyl*dy + (tv0 << FIXP16_SHIFT);
                    zl = dzdyl*dy + (tz0 << FIXP16_SHIFT);

                    // compute new RHS starting values
                    xr = dxdyr*dy + (x0  << FIXP16_SHIFT);
                    ur = dudyr*dy + (tu0 << FIXP16_SHIFT);
                    vr = dvdyr*dy + (tv0 << FIXP16_SHIFT);
                    zr = dzdyr*dy + (tz0 << FIXP16_SHIFT);

                    // compute new starting y
                    ystart = min_clip_y;

                } // end if
                else
                {
                    // no clipping

                    // set starting values
                    xl = (x0 << FIXP16_SHIFT);
                    xr = (x0 << FIXP16_SHIFT);

                    ul = (tu0 << FIXP16_SHIFT);
                    vl = (tv0 << FIXP16_SHIFT);
                    zl = (tz0 << FIXP16_SHIFT);

                    ur = (tu0 << FIXP16_SHIFT);
                    vr = (tv0 << FIXP16_SHIFT);
                    zr = (tz0 << FIXP16_SHIFT);

                    // set starting y
                    ystart = y0;

                } // end else	

            } // end else flat bottom

            // test for bottom clip, always
            if ((yend = y2) > max_clip_y)
                yend = max_clip_y;

            // test for horizontal clipping
            if ((x0 < min_clip_x) || (x0 > max_clip_x) ||
                (x1 < min_clip_x) || (x1 > max_clip_x) ||
                (x2 < min_clip_x) || (x2 > max_clip_x))
            {
                // clip version

                // point screen ptr to starting line
                screen_ptr = dest_buffer + (ystart * mem_pitch);

                // point zbuffer to starting line
                z_ptr = zbuffer + (ystart * zpitch);

                for (yi = ystart; yi < yend; yi++)
                {
                    // compute span endpoints
                    xstart = ((xl + FIXP16_ROUND_UP) >> FIXP16_SHIFT);
                    xend   = ((xr + FIXP16_ROUND_UP) >> FIXP16_SHIFT);

                    // compute starting points for u,v interpolants
                    ui = ul + FIXP16_ROUND_UP;
                    vi = vl + FIXP16_ROUND_UP;
                    zi = zl + FIXP16_ROUND_UP;

                    // compute u,v interpolants
                    if ((dx = (xend - xstart))>0)
                    {
                        du = (ur - ul)/dx;
                        dv = (vr - vl)/dx;
                        dz = (zr - zl)/dx;
                    } // end if
                    else
                    {
                        du = (ur - ul);
                        dv = (vr - vl);
                        dz = (zr - zl);
                    } // end else

                    ///////////////////////////////////////////////////////////////////////

                    // test for x clipping, LHS
                    if (xstart < min_clip_x)
                    {
                        // compute x overlap
                        dx = min_clip_x - xstart;

                        // slide interpolants over
                        ui+=dx*du;
                        vi+=dx*dv;
                        zi+=dx*dz;

                        // reset vars
                        xstart = min_clip_x;

                    } // end if

                    // test for x clipping RHS
                    if (xend > max_clip_x)
                        xend = max_clip_x;

                    ///////////////////////////////////////////////////////////////////////

                    // draw span
                    for (xi=xstart; xi < xend; xi++)
                    {

                        // get textel
                        textel = textmap[(ui >> FIXP16_SHIFT) + ((vi >> FIXP16_SHIFT) << texture_shift2)];

                        if (textel)
                        {
                            // write textel
                            screen_ptr[xi] = alpha_table_src1[screen_ptr[xi]] + 
                                alpha_table_src2[textel];


                            // update z-buffer, write thru
                            z_ptr[xi] = zi;           

                        } // end if			


                        // interpolate u,v,z
                        ui+=du;
                        vi+=dv;
                        zi+=dz;
                    } // end for xi

                    // interpolate u,v,x along right and left edge
                    xl+=dxdyl;
                    ul+=dudyl;
                    vl+=dvdyl;
                    zl+=dzdyl;

                    xr+=dxdyr;
                    ur+=dudyr;
                    vr+=dvdyr;
                    zr+=dzdyr;

                    // advance screen ptr
                    screen_ptr+=mem_pitch;

                    // advance zbuffer ptr
                    z_ptr+=zpitch;

                } // end for y

            } // end if clip
            else
            {
                // non-clip version

                // point screen ptr to starting line
                screen_ptr = dest_buffer + (ystart * mem_pitch);

                // point zbuffer to starting line
                z_ptr = zbuffer + (ystart * zpitch);

                for (yi = ystart; yi < yend; yi++)
                {
                    // compute span endpoints
                    xstart = ((xl + FIXP16_ROUND_UP) >> FIXP16_SHIFT);
                    xend   = ((xr + FIXP16_ROUND_UP) >> FIXP16_SHIFT);

                    // compute starting points for u,v interpolants
                    ui = ul + FIXP16_ROUND_UP;
                    vi = vl + FIXP16_ROUND_UP;
                    zi = zl + FIXP16_ROUND_UP;

                    // compute u,v interpolants
                    if ((dx = (xend - xstart))>0)
                    {
                        du = (ur - ul)/dx;
                        dv = (vr - vl)/dx;
                        dz = (zr - zl)/dx;
                    } // end if
                    else
                    {
                        du = (ur - ul);
                        dv = (vr - vl);
                        dz = (zr - zl);
                    } // end else

                    // draw span
                    for (xi=xstart; xi < xend; xi++)
                    {
                        // test if z of current pixel is nearer than current z buffer value
                        textel = textmap[(ui >> FIXP16_SHIFT) + ((vi >> FIXP16_SHIFT) << texture_shift2)];

                        if (textel)
                        {
                            // write textel
                            screen_ptr[xi] = alpha_table_src1[screen_ptr[xi]] + 
                                alpha_table_src2[textel];


                            // update z-buffer, write thru
                            z_ptr[xi] = zi;           

                        } // end if			

                        // interpolate u,v,z
                        ui+=du;
                        vi+=dv;
                        zi+=dz;
                    } // end for xi

                    // interpolate u,v,x along right and left edge
                    xl+=dxdyl;
                    ul+=dudyl;
                    vl+=dvdyl;
                    zl+=dzdyl;

                    xr+=dxdyr;
                    ur+=dudyr;
                    vr+=dvdyr;
                    zr+=dzdyr;

                    // advance screen ptr
                    screen_ptr+=mem_pitch;

                    // advance zbuffer ptr
                    z_ptr+=zpitch;

                } // end for y

            } // end if non-clipped

        } // end if
        else
            if (tri_type==TRI_TYPE_GENERAL)
            {

                // first test for bottom clip, always
                if ((yend = y2) > max_clip_y)
                    yend = max_clip_y;

                // pre-test y clipping status
                if (y1 < min_clip_y)
                {
                    // compute all deltas
                    // LHS
                    dyl = (y2 - y1);

                    dxdyl = ((x2  - x1)  << FIXP16_SHIFT)/dyl;
                    dudyl = ((tu2 - tu1) << FIXP16_SHIFT)/dyl;  
                    dvdyl = ((tv2 - tv1) << FIXP16_SHIFT)/dyl;    
                    dzdyl = ((tz2 - tz1) << FIXP16_SHIFT)/dyl;  

                    // RHS
                    dyr = (y2 - y0);	

                    dxdyr = ((x2  - x0)  << FIXP16_SHIFT)/dyr;
                    dudyr = ((tu2 - tu0) << FIXP16_SHIFT)/dyr;  
                    dvdyr = ((tv2 - tv0) << FIXP16_SHIFT)/dyr;   
                    dzdyr = ((tz2 - tz0) << FIXP16_SHIFT)/dyr;   

                    // compute overclip
                    dyr = (min_clip_y - y0);
                    dyl = (min_clip_y - y1);

                    // computer new LHS starting values
                    xl = dxdyl*dyl + (x1  << FIXP16_SHIFT);
                    ul = dudyl*dyl + (tu1 << FIXP16_SHIFT);
                    vl = dvdyl*dyl + (tv1 << FIXP16_SHIFT);
                    zl = dzdyl*dyl + (tz1 << FIXP16_SHIFT);

                    // compute new RHS starting values
                    xr = dxdyr*dyr + (x0  << FIXP16_SHIFT);
                    ur = dudyr*dyr + (tu0 << FIXP16_SHIFT);
                    vr = dvdyr*dyr + (tv0 << FIXP16_SHIFT);
                    zr = dzdyr*dyr + (tz0 << FIXP16_SHIFT);

                    // compute new starting y
                    ystart = min_clip_y;

                    // test if we need swap to keep rendering left to right
                    if (dxdyr > dxdyl)
                    {
                        SWAP(dxdyl,dxdyr,temp);
                        SWAP(dudyl,dudyr,temp);
                        SWAP(dvdyl,dvdyr,temp);
                        SWAP(dzdyl,dzdyr,temp);
                        SWAP(xl,xr,temp);
                        SWAP(ul,ur,temp);
                        SWAP(vl,vr,temp);
                        SWAP(zl,zr,temp);
                        SWAP(x1,x2,temp);
                        SWAP(y1,y2,temp);
                        SWAP(tu1,tu2,temp);
                        SWAP(tv1,tv2,temp);
                        SWAP(tz1,tz2,temp);

                        // set interpolation restart
                        irestart = INTERP_RHS;

                    } // end if

                } // end if
                else
                    if (y0 < min_clip_y)
                    {
                        // compute all deltas
                        // LHS
                        dyl = (y1 - y0);

                        dxdyl = ((x1  - x0)  << FIXP16_SHIFT)/dyl;
                        dudyl = ((tu1 - tu0) << FIXP16_SHIFT)/dyl;  
                        dvdyl = ((tv1 - tv0) << FIXP16_SHIFT)/dyl;    
                        dzdyl = ((tz1 - tz0) << FIXP16_SHIFT)/dyl;  

                        // RHS
                        dyr = (y2 - y0);	

                        dxdyr = ((x2  - x0)  << FIXP16_SHIFT)/dyr;
                        dudyr = ((tu2 - tu0) << FIXP16_SHIFT)/dyr;  
                        dvdyr = ((tv2 - tv0) << FIXP16_SHIFT)/dyr;   
                        dzdyr = ((tz2 - tz0) << FIXP16_SHIFT)/dyr;   

                        // compute overclip
                        dy = (min_clip_y - y0);

                        // computer new LHS starting values
                        xl = dxdyl*dy + (x0  << FIXP16_SHIFT);
                        ul = dudyl*dy + (tu0 << FIXP16_SHIFT);
                        vl = dvdyl*dy + (tv0 << FIXP16_SHIFT);
                        zl = dzdyl*dy + (tz0 << FIXP16_SHIFT);

                        // compute new RHS starting values
                        xr = dxdyr*dy + (x0  << FIXP16_SHIFT);
                        ur = dudyr*dy + (tu0 << FIXP16_SHIFT);
                        vr = dvdyr*dy + (tv0 << FIXP16_SHIFT);
                        zr = dzdyr*dy + (tz0 << FIXP16_SHIFT);

                        // compute new starting y
                        ystart = min_clip_y;

                        // test if we need swap to keep rendering left to right
                        if (dxdyr < dxdyl)
                        {
                            SWAP(dxdyl,dxdyr,temp);
                            SWAP(dudyl,dudyr,temp);
                            SWAP(dvdyl,dvdyr,temp);
                            SWAP(dzdyl,dzdyr,temp);
                            SWAP(xl,xr,temp);
                            SWAP(ul,ur,temp);
                            SWAP(vl,vr,temp);
                            SWAP(zl,zr,temp);
                            SWAP(x1,x2,temp);
                            SWAP(y1,y2,temp);
                            SWAP(tu1,tu2,temp);
                            SWAP(tv1,tv2,temp);
                            SWAP(tz1,tz2,temp);

                            // set interpolation restart
                            irestart = INTERP_RHS;

                        } // end if

                    } // end if
                    else
                    {
                        // no initial y clipping

                        // compute all deltas
                        // LHS
                        dyl = (y1 - y0);

                        dxdyl = ((x1  - x0)  << FIXP16_SHIFT)/dyl;
                        dudyl = ((tu1 - tu0) << FIXP16_SHIFT)/dyl;  
                        dvdyl = ((tv1 - tv0) << FIXP16_SHIFT)/dyl;    
                        dzdyl = ((tz1 - tz0) << FIXP16_SHIFT)/dyl;   

                        // RHS
                        dyr = (y2 - y0);	

                        dxdyr = ((x2 - x0)   << FIXP16_SHIFT)/dyr;
                        dudyr = ((tu2 - tu0) << FIXP16_SHIFT)/dyr;  
                        dvdyr = ((tv2 - tv0) << FIXP16_SHIFT)/dyr;   		
                        dzdyr = ((tz2 - tz0) << FIXP16_SHIFT)/dyr;  

                        // no clipping y

                        // set starting values
                        xl = (x0 << FIXP16_SHIFT);
                        xr = (x0 << FIXP16_SHIFT);

                        ul = (tu0 << FIXP16_SHIFT);
                        vl = (tv0 << FIXP16_SHIFT);
                        zl = (tz0 << FIXP16_SHIFT);

                        ur = (tu0 << FIXP16_SHIFT);
                        vr = (tv0 << FIXP16_SHIFT);
                        zr = (tz0 << FIXP16_SHIFT);

                        // set starting y
                        ystart = y0;

                        // test if we need swap to keep rendering left to right
                        if (dxdyr < dxdyl)
                        {
                            SWAP(dxdyl,dxdyr,temp);
                            SWAP(dudyl,dudyr,temp);
                            SWAP(dvdyl,dvdyr,temp);
                            SWAP(dzdyl,dzdyr,temp);
                            SWAP(xl,xr,temp);
                            SWAP(ul,ur,temp);
                            SWAP(vl,vr,temp);
                            SWAP(zl,zr,temp);
                            SWAP(x1,x2,temp);
                            SWAP(y1,y2,temp);
                            SWAP(tu1,tu2,temp);
                            SWAP(tv1,tv2,temp);
                            SWAP(tz1,tz2,temp);

                            // set interpolation restart
                            irestart = INTERP_RHS;

                        } // end if

                    } // end else

                    // test for horizontal clipping
                    if ((x0 < min_clip_x) || (x0 > max_clip_x) ||
                        (x1 < min_clip_x) || (x1 > max_clip_x) ||
                        (x2 < min_clip_x) || (x2 > max_clip_x))
                    {
                        // clip version
                        // x clipping	

                        // point screen ptr to starting line
                        screen_ptr = dest_buffer + (ystart * mem_pitch);

                        // point zbuffer to starting line
                        z_ptr = zbuffer + (ystart * zpitch);

                        for (yi = ystart; yi < yend; yi++)
                        {
                            // compute span endpoints
                            xstart = ((xl + FIXP16_ROUND_UP) >> FIXP16_SHIFT);
                            xend   = ((xr + FIXP16_ROUND_UP) >> FIXP16_SHIFT);

                            // compute starting points for u,v interpolants
                            ui = ul + FIXP16_ROUND_UP;
                            vi = vl + FIXP16_ROUND_UP;
                            zi = zl + FIXP16_ROUND_UP;

                            // compute u,v interpolants
                            if ((dx = (xend - xstart))>0)
                            {
                                du = (ur - ul)/dx;
                                dv = (vr - vl)/dx;
                                dz = (zr - zl)/dx;
                            } // end if
                            else
                            {
                                du = (ur - ul);
                                dv = (vr - vl);
                                dz = (zr - zl);
                            } // end else

                            ///////////////////////////////////////////////////////////////////////

                            // test for x clipping, LHS
                            if (xstart < min_clip_x)
                            {
                                // compute x overlap
                                dx = min_clip_x - xstart;

                                // slide interpolants over
                                ui+=dx*du;
                                vi+=dx*dv;
                                zi+=dx*dz;

                                // set x to left clip edge
                                xstart = min_clip_x;

                            } // end if

                            // test for x clipping RHS
                            if (xend > max_clip_x)
                                xend = max_clip_x;

                            ///////////////////////////////////////////////////////////////////////

                            // draw span
                            for (xi=xstart; xi < xend; xi++)
                            {
                                // get textel
                                textel = textmap[(ui >> FIXP16_SHIFT) + ((vi >> FIXP16_SHIFT) << texture_shift2)];

                                if (textel)
                                {
                                    // write textel
                                    screen_ptr[xi] = alpha_table_src1[screen_ptr[xi]] + 
                                        alpha_table_src2[textel];


                                    // update z-buffer, write thru
                                    z_ptr[xi] = zi;           

                                } // end if		


                                // interpolate u,v,z
                                ui+=du;
                                vi+=dv;
                                zi+=dz;
                            } // end for xi

                            // interpolate u,v,x along right and left edge
                            xl+=dxdyl;
                            ul+=dudyl;
                            vl+=dvdyl;
                            zl+=dzdyl;

                            xr+=dxdyr;
                            ur+=dudyr;
                            vr+=dvdyr;
                            zr+=dzdyr;

                            // advance screen ptr
                            screen_ptr+=mem_pitch;

                            // advance zbuffer ptr
                            z_ptr+=zpitch;

                            // test for yi hitting second region, if so change interpolant
                            if (yi==yrestart)
                            {
                                // test interpolation side change flag

                                if (irestart == INTERP_LHS)
                                {
                                    // LHS
                                    dyl = (y2 - y1);	

                                    dxdyl = ((x2 - x1)   << FIXP16_SHIFT)/dyl;
                                    dudyl = ((tu2 - tu1) << FIXP16_SHIFT)/dyl;  
                                    dvdyl = ((tv2 - tv1) << FIXP16_SHIFT)/dyl;   		
                                    dzdyl = ((tz2 - tz1) << FIXP16_SHIFT)/dyl;   

                                    // set starting values
                                    xl = (x1  << FIXP16_SHIFT);
                                    ul = (tu1 << FIXP16_SHIFT);
                                    vl = (tv1 << FIXP16_SHIFT);
                                    zl = (tz1 << FIXP16_SHIFT);

                                    // interpolate down on LHS to even up
                                    xl+=dxdyl;
                                    ul+=dudyl;
                                    vl+=dvdyl;
                                    zl+=dzdyl;
                                } // end if
                                else
                                {
                                    // RHS
                                    dyr = (y1 - y2);	

                                    dxdyr = ((x1 - x2)   << FIXP16_SHIFT)/dyr;
                                    dudyr = ((tu1 - tu2) << FIXP16_SHIFT)/dyr;  
                                    dvdyr = ((tv1 - tv2) << FIXP16_SHIFT)/dyr;   		
                                    dzdyr = ((tz1 - tz2) << FIXP16_SHIFT)/dyr;  

                                    // set starting values
                                    xr = (x2  << FIXP16_SHIFT);
                                    ur = (tu2 << FIXP16_SHIFT);
                                    vr = (tv2 << FIXP16_SHIFT);
                                    zr = (tz2 << FIXP16_SHIFT);

                                    // interpolate down on RHS to even up
                                    xr+=dxdyr;
                                    ur+=dudyr;
                                    vr+=dvdyr;
                                    zr+=dzdyr;

                                } // end else

                            } // end if

                        } // end for y

                    } // end if
                    else
                    {
                        // no x clipping
                        // point screen ptr to starting line
                        screen_ptr = dest_buffer + (ystart * mem_pitch);

                        // point zbuffer to starting line
                        z_ptr = zbuffer + (ystart * zpitch);

                        for (yi = ystart; yi < yend; yi++)
                        {
                            // compute span endpoints
                            xstart = ((xl + FIXP16_ROUND_UP) >> FIXP16_SHIFT);
                            xend   = ((xr + FIXP16_ROUND_UP) >> FIXP16_SHIFT);

                            // compute starting points for u,v interpolants
                            ui = ul + FIXP16_ROUND_UP;
                            vi = vl + FIXP16_ROUND_UP;
                            zi = zl + FIXP16_ROUND_UP;

                            // compute u,v interpolants
                            if ((dx = (xend - xstart))>0)
                            {
                                du = (ur - ul)/dx;
                                dv = (vr - vl)/dx;
                                dz = (zr - zl)/dx;
                            } // end if
                            else
                            {
                                du = (ur - ul);
                                dv = (vr - vl);
                                dz = (zr - zl);
                            } // end else

                            // draw span
                            for (xi=xstart; xi < xend; xi++)
                            {
                                // test if z of current pixel is nearer than current z buffer value
                                // get textel
                                textel = textmap[(ui >> FIXP16_SHIFT) + ((vi >> FIXP16_SHIFT) << texture_shift2)];

                                if (textel)
                                {
                                    // write textel
                                    screen_ptr[xi] = alpha_table_src1[screen_ptr[xi]] + 
                                        alpha_table_src2[textel];


                                    // update z-buffer, write thru
                                    z_ptr[xi] = zi;           

                                } // end if		

                                // interpolate u,v
                                ui+=du;
                                vi+=dv;
                                zi+=dz;
                            } // end for xi

                            // interpolate u,v,x along right and left edge
                            xl+=dxdyl;
                            ul+=dudyl;
                            vl+=dvdyl;
                            zl+=dzdyl;

                            xr+=dxdyr;
                            ur+=dudyr;
                            vr+=dvdyr;
                            zr+=dzdyr;

                            // advance screen ptr
                            screen_ptr+=mem_pitch;

                            // advance zbuffer ptr
                            z_ptr+=zpitch;

                            // test for yi hitting second region, if so change interpolant
                            if (yi==yrestart)
                            {
                                // test interpolation side change flag

                                if (irestart == INTERP_LHS)
                                {
                                    // LHS
                                    dyl = (y2 - y1);	

                                    dxdyl = ((x2 - x1)   << FIXP16_SHIFT)/dyl;
                                    dudyl = ((tu2 - tu1) << FIXP16_SHIFT)/dyl;  
                                    dvdyl = ((tv2 - tv1) << FIXP16_SHIFT)/dyl;   		
                                    dzdyl = ((tz2 - tz1) << FIXP16_SHIFT)/dyl;   

                                    // set starting values
                                    xl = (x1  << FIXP16_SHIFT);
                                    ul = (tu1 << FIXP16_SHIFT);
                                    vl = (tv1 << FIXP16_SHIFT);
                                    zl = (tz1 << FIXP16_SHIFT);

                                    // interpolate down on LHS to even up
                                    xl+=dxdyl;
                                    ul+=dudyl;
                                    vl+=dvdyl;
                                    zl+=dzdyl;
                                } // end if
                                else
                                {
                                    // RHS
                                    dyr = (y1 - y2);	

                                    dxdyr = ((x1 - x2)   << FIXP16_SHIFT)/dyr;
                                    dudyr = ((tu1 - tu2) << FIXP16_SHIFT)/dyr;  
                                    dvdyr = ((tv1 - tv2) << FIXP16_SHIFT)/dyr;   		
                                    dzdyr = ((tz1 - tz2) << FIXP16_SHIFT)/dyr; 

                                    // set starting values
                                    xr = (x2  << FIXP16_SHIFT);
                                    ur = (tu2 << FIXP16_SHIFT);
                                    vr = (tv2 << FIXP16_SHIFT);
                                    zr = (tz2 << FIXP16_SHIFT);

                                    // interpolate down on RHS to even up
                                    xr+=dxdyr;
                                    ur+=dudyr;
                                    vr+=dvdyr;
                                    zr+=dzdyr;

                                } // end else

                            } // end if

                        } // end for y

                    } // end else	

            } // end if

} // end Draw_Textured_TriangleWTZB_Alpha16_2

///////////////////////////////////////////////////////////////////////////////

void Draw_Textured_TriangleZB3_16(POLYF4DV2_PTR face,  // ptr to face
                                  UCHAR *_dest_buffer,  // pointer to video buffer
                                  int mem_pitch,        // bytes per line, 320, 640 etc.
                                  UCHAR *_zbuffer,       // pointer to z-buffer
                                  int zpitch)          // bytes per line of zbuffer
{
    // this function draws a textured triangle in 16-bit mode

    int v0=0,
        v1=1,
        v2=2,
        temp=0,
        tri_type = TRI_TYPE_NONE,
        irestart = INTERP_LHS;

    int dx,dy,dyl,dyr,      // general deltas
        u,v,z,
        du,dv,dz,
        xi,yi,              // the current interpolated x,y
        ui,vi,zi,           // the current interpolated u,v,z
        index_x,index_y,    // looping vars
        x,y,                // hold general x,y
        xstart,
        xend,
        ystart,
        yrestart,
        yend,
        xl,                 
        dxdyl,              
        xr,
        dxdyr,             
        dudyl,    
        ul,
        dvdyl,   
        vl,
        dzdyl,   
        zl,
        dudyr,
        ur,
        dvdyr,
        vr,
        dzdyr,
        zr;

    int x0,y0,tu0,tv0,tz0,    // cached vertices
        x1,y1,tu1,tv1,tz1,
        x2,y2,tu2,tv2,tz2;

    USHORT *screen_ptr  = NULL,
        *screen_line = NULL,
        *textmap     = NULL,
        *dest_buffer = (USHORT *)_dest_buffer,
        textel;                       

    UINT  *z_ptr = NULL,
        *zbuffer = (UINT *)_zbuffer;

#ifdef DEBUG_ON
    // track rendering stats
    debug_polys_rendered_per_frame++;
#endif

    // extract texture map
    textmap = (USHORT *)face->texture->buffer;

    // extract base 2 of texture width
    int texture_shift2 = logbase2ofx[face->texture->width];

    // adjust memory pitch to words, divide by 2
    mem_pitch >>=1;

    // adjust zbuffer pitch for 32 bit alignment
    zpitch >>= 2;

    // apply fill convention to coordinates
    face->tvlist[0].x = (int)(face->tvlist[0].x+0.5);
    face->tvlist[0].y = (int)(face->tvlist[0].y+0.5);

    face->tvlist[1].x = (int)(face->tvlist[1].x+0.5);
    face->tvlist[1].y = (int)(face->tvlist[1].y+0.5);

    face->tvlist[2].x = (int)(face->tvlist[2].x+0.5);
    face->tvlist[2].y = (int)(face->tvlist[2].y+0.5);

    // first trivial clipping rejection tests 
    if (((face->tvlist[0].y < min_clip_y)  && 
        (face->tvlist[1].y < min_clip_y)  &&
        (face->tvlist[2].y < min_clip_y)) ||

        ((face->tvlist[0].y > max_clip_y)  && 
        (face->tvlist[1].y > max_clip_y)  &&
        (face->tvlist[2].y > max_clip_y)) ||

        ((face->tvlist[0].x < min_clip_x)  && 
        (face->tvlist[1].x < min_clip_x)  &&
        (face->tvlist[2].x < min_clip_x)) ||

        ((face->tvlist[0].x > max_clip_x)  && 
        (face->tvlist[1].x > max_clip_x)  &&
        (face->tvlist[2].x > max_clip_x)))
        return;

    // sort vertices
    if (face->tvlist[v1].y < face->tvlist[v0].y) 
    {SWAP(v0,v1,temp);} 

    if (face->tvlist[v2].y < face->tvlist[v0].y) 
    {SWAP(v0,v2,temp);}

    if (face->tvlist[v2].y < face->tvlist[v1].y) 
    {SWAP(v1,v2,temp);}

    // now test for trivial flat sided cases
    if (FCMP(face->tvlist[v0].y, face->tvlist[v1].y) )
    { 
        // set triangle type
        tri_type = TRI_TYPE_FLAT_TOP;

        // sort vertices left to right
        if (face->tvlist[v1].x < face->tvlist[v0].x) 
        {SWAP(v0,v1,temp);}

    } // end if
    else
        // now test for trivial flat sided cases
        if (FCMP(face->tvlist[v1].y ,face->tvlist[v2].y))
        { 
            // set triangle type
            tri_type = TRI_TYPE_FLAT_BOTTOM;

            // sort vertices left to right
            if (face->tvlist[v2].x < face->tvlist[v1].x) 
            {SWAP(v1,v2,temp);}

        } // end if
        else
        {
            // must be a general triangle
            tri_type = TRI_TYPE_GENERAL;

        } // end else

        // extract vertices for processing, now that we have order
        x0  = (int)(face->tvlist[v0].x+0.0);
        y0  = (int)(face->tvlist[v0].y+0.0);
        tu0 = (int)(face->tvlist[v0].u0);
        tv0 = (int)(face->tvlist[v0].v0);
        tz0 = (int)(face->tvlist[v0].z+0.5);

        x1  = (int)(face->tvlist[v1].x+0.0);
        y1  = (int)(face->tvlist[v1].y+0.0);
        tu1 = (int)(face->tvlist[v1].u0);
        tv1 = (int)(face->tvlist[v1].v0);
        tz1 = (int)(face->tvlist[v1].z+0.5);

        x2  = (int)(face->tvlist[v2].x+0.0);
        y2  = (int)(face->tvlist[v2].y+0.0);
        tu2 = (int)(face->tvlist[v2].u0);
        tv2 = (int)(face->tvlist[v2].v0);
        tz2 = (int)(face->tvlist[v2].z+0.5);


        // degenerate triangle
        if ( ((x0 == x1) && (x1 == x2)) || ((y0 ==  y1) && (y1 == y2)))
            return;

        // set interpolation restart value
        yrestart = y1;

        // what kind of triangle
        if (tri_type & TRI_TYPE_FLAT_MASK)
        {

            if (tri_type == TRI_TYPE_FLAT_TOP)
            {
                // compute all deltas
                dy = (y2 - y0);

                dxdyl = ((x2 - x0)   << FIXP16_SHIFT)/dy;
                dudyl = ((tu2 - tu0) << FIXP16_SHIFT)/dy;  
                dvdyl = ((tv2 - tv0) << FIXP16_SHIFT)/dy;    
                dzdyl = ((tz2 - tz0) << FIXP16_SHIFT)/dy;    

                dxdyr = ((x2 - x1)   << FIXP16_SHIFT)/dy;
                dudyr = ((tu2 - tu1) << FIXP16_SHIFT)/dy;  
                dvdyr = ((tv2 - tv1) << FIXP16_SHIFT)/dy;   
                dzdyr = ((tz2 - tz1) << FIXP16_SHIFT)/dy;  

                // test for y clipping
                if (y0 < min_clip_y)
                {
                    // compute overclip
                    dy = (min_clip_y - y0);

                    // computer new LHS starting values
                    xl = dxdyl*dy + (x0  << FIXP16_SHIFT);
                    ul = dudyl*dy + (tu0 << FIXP16_SHIFT);
                    vl = dvdyl*dy + (tv0 << FIXP16_SHIFT);
                    zl = dzdyl*dy + (tz0 << FIXP16_SHIFT);

                    // compute new RHS starting values
                    xr = dxdyr*dy + (x1  << FIXP16_SHIFT);
                    ur = dudyr*dy + (tu1 << FIXP16_SHIFT);
                    vr = dvdyr*dy + (tv1 << FIXP16_SHIFT);
                    zr = dzdyr*dy + (tz1 << FIXP16_SHIFT);

                    // compute new starting y
                    ystart = min_clip_y;

                } // end if
                else
                {
                    // no clipping

                    // set starting values
                    xl = (x0 << FIXP16_SHIFT);
                    xr = (x1 << FIXP16_SHIFT);

                    ul = (tu0 << FIXP16_SHIFT);
                    vl = (tv0 << FIXP16_SHIFT);
                    zl = (tz0 << FIXP16_SHIFT);

                    ur = (tu1 << FIXP16_SHIFT);
                    vr = (tv1 << FIXP16_SHIFT);
                    zr = (tz1 << FIXP16_SHIFT);

                    // set starting y
                    ystart = y0;

                } // end else

            } // end if flat top
            else
            {
                // must be flat bottom

                // compute all deltas
                dy = (y1 - y0);

                dxdyl = ((x1 - x0)   << FIXP16_SHIFT)/dy;
                dudyl = ((tu1 - tu0) << FIXP16_SHIFT)/dy;  
                dvdyl = ((tv1 - tv0) << FIXP16_SHIFT)/dy;    
                dzdyl = ((tz1 - tz0) << FIXP16_SHIFT)/dy;   

                dxdyr = ((x2 - x0)   << FIXP16_SHIFT)/dy;
                dudyr = ((tu2 - tu0) << FIXP16_SHIFT)/dy;  
                dvdyr = ((tv2 - tv0) << FIXP16_SHIFT)/dy;   
                dzdyr = ((tz2 - tz0) << FIXP16_SHIFT)/dy;   

                // test for y clipping
                if (y0 < min_clip_y)
                {
                    // compute overclip
                    dy = (min_clip_y - y0);

                    // computer new LHS starting values
                    xl = dxdyl*dy + (x0  << FIXP16_SHIFT);
                    ul = dudyl*dy + (tu0 << FIXP16_SHIFT);
                    vl = dvdyl*dy + (tv0 << FIXP16_SHIFT);
                    zl = dzdyl*dy + (tz0 << FIXP16_SHIFT);

                    // compute new RHS starting values
                    xr = dxdyr*dy + (x0  << FIXP16_SHIFT);
                    ur = dudyr*dy + (tu0 << FIXP16_SHIFT);
                    vr = dvdyr*dy + (tv0 << FIXP16_SHIFT);
                    zr = dzdyr*dy + (tz0 << FIXP16_SHIFT);

                    // compute new starting y
                    ystart = min_clip_y;

                } // end if
                else
                {
                    // no clipping

                    // set starting values
                    xl = (x0 << FIXP16_SHIFT);
                    xr = (x0 << FIXP16_SHIFT);

                    ul = (tu0 << FIXP16_SHIFT);
                    vl = (tv0 << FIXP16_SHIFT);
                    zl = (tz0 << FIXP16_SHIFT);

                    ur = (tu0 << FIXP16_SHIFT);
                    vr = (tv0 << FIXP16_SHIFT);
                    zr = (tz0 << FIXP16_SHIFT);

                    // set starting y
                    ystart = y0;

                } // end else	

            } // end else flat bottom

            // test for bottom clip, always
            if ((yend = y2) > max_clip_y)
                yend = max_clip_y;

            // test for horizontal clipping
            if ((x0 < min_clip_x) || (x0 > max_clip_x) ||
                (x1 < min_clip_x) || (x1 > max_clip_x) ||
                (x2 < min_clip_x) || (x2 > max_clip_x))
            {
                // clip version

                // point screen ptr to starting line
                screen_ptr = dest_buffer + (ystart * mem_pitch);

                // point zbuffer to starting line
                z_ptr = zbuffer + (ystart * zpitch);

                for (yi = ystart; yi < yend; yi++)
                {
                    // compute span endpoints
                    xstart = ((xl + FIXP16_ROUND_UP) >> FIXP16_SHIFT);
                    xend   = ((xr + FIXP16_ROUND_UP) >> FIXP16_SHIFT);

                    // compute starting points for u,v interpolants
                    ui = ul + FIXP16_ROUND_UP;
                    vi = vl + FIXP16_ROUND_UP;
                    zi = zl + FIXP16_ROUND_UP;

                    // compute u,v interpolants
                    if ((dx = (xend - xstart))>0)
                    {
                        du = (ur - ul)/dx;
                        dv = (vr - vl)/dx;
                        dz = (zr - zl)/dx;
                    } // end if
                    else
                    {
                        du = (ur - ul);
                        dv = (vr - vl);
                        dz = (zr - zl);
                    } // end else

                    ///////////////////////////////////////////////////////////////////////

                    // test for x clipping, LHS
                    if (xstart < min_clip_x)
                    {
                        // compute x overlap
                        dx = min_clip_x - xstart;

                        // slide interpolants over
                        ui+=dx*du;
                        vi+=dx*dv;
                        zi+=dx*dz;

                        // reset vars
                        xstart = min_clip_x;

                    } // end if

                    // test for x clipping RHS
                    if (xend > max_clip_x)
                        xend = max_clip_x;

                    ///////////////////////////////////////////////////////////////////////

                    // draw span
                    for (xi=xstart; xi < xend; xi++)
                    {
                        textel = textmap[(ui >> FIXP16_SHIFT) + ((vi >> FIXP16_SHIFT) << texture_shift2)];

                        if (textel)
                        {
                            // test if z of current pixel is nearer than current z buffer value
                            if (zi < z_ptr[xi])
                            {
                                // write textel
                                screen_ptr[xi] = textel;

                                // update z-buffer
                                z_ptr[xi] = zi;           
                            } // end if
                        } // end if



                        // interpolate u,v,z
                        ui+=du;
                        vi+=dv;
                        zi+=dz;
                    } // end for xi

                    // interpolate u,v,x along right and left edge
                    xl+=dxdyl;
                    ul+=dudyl;
                    vl+=dvdyl;
                    zl+=dzdyl;

                    xr+=dxdyr;
                    ur+=dudyr;
                    vr+=dvdyr;
                    zr+=dzdyr;

                    // advance screen ptr
                    screen_ptr+=mem_pitch;

                    // advance zbuffer ptr
                    z_ptr+=zpitch;

                } // end for y

            } // end if clip
            else
            {
                // non-clip version

                // point screen ptr to starting line
                screen_ptr = dest_buffer + (ystart * mem_pitch);

                // point zbuffer to starting line
                z_ptr = zbuffer + (ystart * zpitch);

                for (yi = ystart; yi < yend; yi++)
                {
                    // compute span endpoints
                    xstart = ((xl + FIXP16_ROUND_UP) >> FIXP16_SHIFT);
                    xend   = ((xr + FIXP16_ROUND_UP) >> FIXP16_SHIFT);

                    // compute starting points for u,v interpolants
                    ui = ul + FIXP16_ROUND_UP;
                    vi = vl + FIXP16_ROUND_UP;
                    zi = zl + FIXP16_ROUND_UP;

                    // compute u,v interpolants
                    if ((dx = (xend - xstart))>0)
                    {
                        du = (ur - ul)/dx;
                        dv = (vr - vl)/dx;
                        dz = (zr - zl)/dx;
                    } // end if
                    else
                    {
                        du = (ur - ul);
                        dv = (vr - vl);
                        dz = (zr - zl);
                    } // end else

                    // draw span
                    for (xi=xstart; xi < xend; xi++)
                    {
                        textel = textmap[(ui >> FIXP16_SHIFT) + ((vi >> FIXP16_SHIFT) << texture_shift2)];

                        if (textel)
                        {
                            // test if z of current pixel is nearer than current z buffer value
                            if (zi < z_ptr[xi])
                            {
                                // write textel
                                screen_ptr[xi] = textel;

                                // update z-buffer
                                z_ptr[xi] = zi;           
                            } // end if
                        } // end if

                        // interpolate u,v,z
                        ui+=du;
                        vi+=dv;
                        zi+=dz;
                    } // end for xi

                    // interpolate u,v,x along right and left edge
                    xl+=dxdyl;
                    ul+=dudyl;
                    vl+=dvdyl;
                    zl+=dzdyl;

                    xr+=dxdyr;
                    ur+=dudyr;
                    vr+=dvdyr;
                    zr+=dzdyr;

                    // advance screen ptr
                    screen_ptr+=mem_pitch;

                    // advance zbuffer ptr
                    z_ptr+=zpitch;

                } // end for y

            } // end if non-clipped

        } // end if
        else
            if (tri_type==TRI_TYPE_GENERAL)
            {

                // first test for bottom clip, always
                if ((yend = y2) > max_clip_y)
                    yend = max_clip_y;

                // pre-test y clipping status
                if (y1 < min_clip_y)
                {
                    // compute all deltas
                    // LHS
                    dyl = (y2 - y1);

                    dxdyl = ((x2  - x1)  << FIXP16_SHIFT)/dyl;
                    dudyl = ((tu2 - tu1) << FIXP16_SHIFT)/dyl;  
                    dvdyl = ((tv2 - tv1) << FIXP16_SHIFT)/dyl;    
                    dzdyl = ((tz2 - tz1) << FIXP16_SHIFT)/dyl;  

                    // RHS
                    dyr = (y2 - y0);	

                    dxdyr = ((x2  - x0)  << FIXP16_SHIFT)/dyr;
                    dudyr = ((tu2 - tu0) << FIXP16_SHIFT)/dyr;  
                    dvdyr = ((tv2 - tv0) << FIXP16_SHIFT)/dyr;   
                    dzdyr = ((tz2 - tz0) << FIXP16_SHIFT)/dyr;   

                    // compute overclip
                    dyr = (min_clip_y - y0);
                    dyl = (min_clip_y - y1);

                    // computer new LHS starting values
                    xl = dxdyl*dyl + (x1  << FIXP16_SHIFT);
                    ul = dudyl*dyl + (tu1 << FIXP16_SHIFT);
                    vl = dvdyl*dyl + (tv1 << FIXP16_SHIFT);
                    zl = dzdyl*dyl + (tz1 << FIXP16_SHIFT);

                    // compute new RHS starting values
                    xr = dxdyr*dyr + (x0  << FIXP16_SHIFT);
                    ur = dudyr*dyr + (tu0 << FIXP16_SHIFT);
                    vr = dvdyr*dyr + (tv0 << FIXP16_SHIFT);
                    zr = dzdyr*dyr + (tz0 << FIXP16_SHIFT);

                    // compute new starting y
                    ystart = min_clip_y;

                    // test if we need swap to keep rendering left to right
                    if (dxdyr > dxdyl)
                    {
                        SWAP(dxdyl,dxdyr,temp);
                        SWAP(dudyl,dudyr,temp);
                        SWAP(dvdyl,dvdyr,temp);
                        SWAP(dzdyl,dzdyr,temp);
                        SWAP(xl,xr,temp);
                        SWAP(ul,ur,temp);
                        SWAP(vl,vr,temp);
                        SWAP(zl,zr,temp);
                        SWAP(x1,x2,temp);
                        SWAP(y1,y2,temp);
                        SWAP(tu1,tu2,temp);
                        SWAP(tv1,tv2,temp);
                        SWAP(tz1,tz2,temp);

                        // set interpolation restart
                        irestart = INTERP_RHS;

                    } // end if

                } // end if
                else
                    if (y0 < min_clip_y)
                    {
                        // compute all deltas
                        // LHS
                        dyl = (y1 - y0);

                        dxdyl = ((x1  - x0)  << FIXP16_SHIFT)/dyl;
                        dudyl = ((tu1 - tu0) << FIXP16_SHIFT)/dyl;  
                        dvdyl = ((tv1 - tv0) << FIXP16_SHIFT)/dyl;    
                        dzdyl = ((tz1 - tz0) << FIXP16_SHIFT)/dyl;  

                        // RHS
                        dyr = (y2 - y0);	

                        dxdyr = ((x2  - x0)  << FIXP16_SHIFT)/dyr;
                        dudyr = ((tu2 - tu0) << FIXP16_SHIFT)/dyr;  
                        dvdyr = ((tv2 - tv0) << FIXP16_SHIFT)/dyr;   
                        dzdyr = ((tz2 - tz0) << FIXP16_SHIFT)/dyr;   

                        // compute overclip
                        dy = (min_clip_y - y0);

                        // computer new LHS starting values
                        xl = dxdyl*dy + (x0  << FIXP16_SHIFT);
                        ul = dudyl*dy + (tu0 << FIXP16_SHIFT);
                        vl = dvdyl*dy + (tv0 << FIXP16_SHIFT);
                        zl = dzdyl*dy + (tz0 << FIXP16_SHIFT);

                        // compute new RHS starting values
                        xr = dxdyr*dy + (x0  << FIXP16_SHIFT);
                        ur = dudyr*dy + (tu0 << FIXP16_SHIFT);
                        vr = dvdyr*dy + (tv0 << FIXP16_SHIFT);
                        zr = dzdyr*dy + (tz0 << FIXP16_SHIFT);

                        // compute new starting y
                        ystart = min_clip_y;

                        // test if we need swap to keep rendering left to right
                        if (dxdyr < dxdyl)
                        {
                            SWAP(dxdyl,dxdyr,temp);
                            SWAP(dudyl,dudyr,temp);
                            SWAP(dvdyl,dvdyr,temp);
                            SWAP(dzdyl,dzdyr,temp);
                            SWAP(xl,xr,temp);
                            SWAP(ul,ur,temp);
                            SWAP(vl,vr,temp);
                            SWAP(zl,zr,temp);
                            SWAP(x1,x2,temp);
                            SWAP(y1,y2,temp);
                            SWAP(tu1,tu2,temp);
                            SWAP(tv1,tv2,temp);
                            SWAP(tz1,tz2,temp);

                            // set interpolation restart
                            irestart = INTERP_RHS;

                        } // end if

                    } // end if
                    else
                    {
                        // no initial y clipping

                        // compute all deltas
                        // LHS
                        dyl = (y1 - y0);

                        dxdyl = ((x1  - x0)  << FIXP16_SHIFT)/dyl;
                        dudyl = ((tu1 - tu0) << FIXP16_SHIFT)/dyl;  
                        dvdyl = ((tv1 - tv0) << FIXP16_SHIFT)/dyl;    
                        dzdyl = ((tz1 - tz0) << FIXP16_SHIFT)/dyl;   

                        // RHS
                        dyr = (y2 - y0);	

                        dxdyr = ((x2 - x0)   << FIXP16_SHIFT)/dyr;
                        dudyr = ((tu2 - tu0) << FIXP16_SHIFT)/dyr;  
                        dvdyr = ((tv2 - tv0) << FIXP16_SHIFT)/dyr;   		
                        dzdyr = ((tz2 - tz0) << FIXP16_SHIFT)/dyr;  

                        // no clipping y

                        // set starting values
                        xl = (x0 << FIXP16_SHIFT);
                        xr = (x0 << FIXP16_SHIFT);

                        ul = (tu0 << FIXP16_SHIFT);
                        vl = (tv0 << FIXP16_SHIFT);
                        zl = (tz0 << FIXP16_SHIFT);

                        ur = (tu0 << FIXP16_SHIFT);
                        vr = (tv0 << FIXP16_SHIFT);
                        zr = (tz0 << FIXP16_SHIFT);

                        // set starting y
                        ystart = y0;

                        // test if we need swap to keep rendering left to right
                        if (dxdyr < dxdyl)
                        {
                            SWAP(dxdyl,dxdyr,temp);
                            SWAP(dudyl,dudyr,temp);
                            SWAP(dvdyl,dvdyr,temp);
                            SWAP(dzdyl,dzdyr,temp);
                            SWAP(xl,xr,temp);
                            SWAP(ul,ur,temp);
                            SWAP(vl,vr,temp);
                            SWAP(zl,zr,temp);
                            SWAP(x1,x2,temp);
                            SWAP(y1,y2,temp);
                            SWAP(tu1,tu2,temp);
                            SWAP(tv1,tv2,temp);
                            SWAP(tz1,tz2,temp);

                            // set interpolation restart
                            irestart = INTERP_RHS;

                        } // end if

                    } // end else

                    // test for horizontal clipping
                    if ((x0 < min_clip_x) || (x0 > max_clip_x) ||
                        (x1 < min_clip_x) || (x1 > max_clip_x) ||
                        (x2 < min_clip_x) || (x2 > max_clip_x))
                    {
                        // clip version
                        // x clipping	

                        // point screen ptr to starting line
                        screen_ptr = dest_buffer + (ystart * mem_pitch);

                        // point zbuffer to starting line
                        z_ptr = zbuffer + (ystart * zpitch);

                        for (yi = ystart; yi < yend; yi++)
                        {
                            // compute span endpoints
                            xstart = ((xl + FIXP16_ROUND_UP) >> FIXP16_SHIFT);
                            xend   = ((xr + FIXP16_ROUND_UP) >> FIXP16_SHIFT);

                            // compute starting points for u,v interpolants
                            ui = ul + FIXP16_ROUND_UP;
                            vi = vl + FIXP16_ROUND_UP;
                            zi = zl + FIXP16_ROUND_UP;

                            // compute u,v interpolants
                            if ((dx = (xend - xstart))>0)
                            {
                                du = (ur - ul)/dx;
                                dv = (vr - vl)/dx;
                                dz = (zr - zl)/dx;
                            } // end if
                            else
                            {
                                du = (ur - ul);
                                dv = (vr - vl);
                                dz = (zr - zl);
                            } // end else

                            ///////////////////////////////////////////////////////////////////////

                            // test for x clipping, LHS
                            if (xstart < min_clip_x)
                            {
                                // compute x overlap
                                dx = min_clip_x - xstart;

                                // slide interpolants over
                                ui+=dx*du;
                                vi+=dx*dv;
                                zi+=dx*dz;

                                // set x to left clip edge
                                xstart = min_clip_x;

                            } // end if

                            // test for x clipping RHS
                            if (xend > max_clip_x)
                                xend = max_clip_x;

                            ///////////////////////////////////////////////////////////////////////

                            // draw span
                            for (xi=xstart; xi < xend; xi++)
                            {
                                textel = textmap[(ui >> FIXP16_SHIFT) + ((vi >> FIXP16_SHIFT) << texture_shift2)];

                                if (textel)
                                {
                                    // test if z of current pixel is nearer than current z buffer value
                                    if (zi < z_ptr[xi])
                                    {
                                        // write textel
                                        screen_ptr[xi] = textel;

                                        // update z-buffer
                                        z_ptr[xi] = zi;           
                                    } // end if
                                } // end if

                                // interpolate u,v,z
                                ui+=du;
                                vi+=dv;
                                zi+=dz;
                            } // end for xi

                            // interpolate u,v,x along right and left edge
                            xl+=dxdyl;
                            ul+=dudyl;
                            vl+=dvdyl;
                            zl+=dzdyl;

                            xr+=dxdyr;
                            ur+=dudyr;
                            vr+=dvdyr;
                            zr+=dzdyr;

                            // advance screen ptr
                            screen_ptr+=mem_pitch;

                            // advance zbuffer ptr
                            z_ptr+=zpitch;

                            // test for yi hitting second region, if so change interpolant
                            if (yi==yrestart)
                            {
                                // test interpolation side change flag

                                if (irestart == INTERP_LHS)
                                {
                                    // LHS
                                    dyl = (y2 - y1);	

                                    dxdyl = ((x2 - x1)   << FIXP16_SHIFT)/dyl;
                                    dudyl = ((tu2 - tu1) << FIXP16_SHIFT)/dyl;  
                                    dvdyl = ((tv2 - tv1) << FIXP16_SHIFT)/dyl;   		
                                    dzdyl = ((tz2 - tz1) << FIXP16_SHIFT)/dyl;   

                                    // set starting values
                                    xl = (x1  << FIXP16_SHIFT);
                                    ul = (tu1 << FIXP16_SHIFT);
                                    vl = (tv1 << FIXP16_SHIFT);
                                    zl = (tz1 << FIXP16_SHIFT);

                                    // interpolate down on LHS to even up
                                    xl+=dxdyl;
                                    ul+=dudyl;
                                    vl+=dvdyl;
                                    zl+=dzdyl;
                                } // end if
                                else
                                {
                                    // RHS
                                    dyr = (y1 - y2);	

                                    dxdyr = ((x1 - x2)   << FIXP16_SHIFT)/dyr;
                                    dudyr = ((tu1 - tu2) << FIXP16_SHIFT)/dyr;  
                                    dvdyr = ((tv1 - tv2) << FIXP16_SHIFT)/dyr;   		
                                    dzdyr = ((tz1 - tz2) << FIXP16_SHIFT)/dyr;  

                                    // set starting values
                                    xr = (x2  << FIXP16_SHIFT);
                                    ur = (tu2 << FIXP16_SHIFT);
                                    vr = (tv2 << FIXP16_SHIFT);
                                    zr = (tz2 << FIXP16_SHIFT);

                                    // interpolate down on RHS to even up
                                    xr+=dxdyr;
                                    ur+=dudyr;
                                    vr+=dvdyr;
                                    zr+=dzdyr;

                                } // end else

                            } // end if

                        } // end for y

                    } // end if
                    else
                    {
                        // no x clipping
                        // point screen ptr to starting line
                        screen_ptr = dest_buffer + (ystart * mem_pitch);

                        // point zbuffer to starting line
                        z_ptr = zbuffer + (ystart * zpitch);

                        for (yi = ystart; yi < yend; yi++)
                        {
                            // compute span endpoints
                            xstart = ((xl + FIXP16_ROUND_UP) >> FIXP16_SHIFT);
                            xend   = ((xr + FIXP16_ROUND_UP) >> FIXP16_SHIFT);

                            // compute starting points for u,v interpolants
                            ui = ul + FIXP16_ROUND_UP;
                            vi = vl + FIXP16_ROUND_UP;
                            zi = zl + FIXP16_ROUND_UP;

                            // compute u,v interpolants
                            if ((dx = (xend - xstart))>0)
                            {
                                du = (ur - ul)/dx;
                                dv = (vr - vl)/dx;
                                dz = (zr - zl)/dx;
                            } // end if
                            else
                            {
                                du = (ur - ul);
                                dv = (vr - vl);
                                dz = (zr - zl);
                            } // end else

                            // draw span
                            for (xi=xstart; xi < xend; xi++)
                            {
                                textel = textmap[(ui >> FIXP16_SHIFT) + ((vi >> FIXP16_SHIFT) << texture_shift2)];

                                if (textel)
                                {
                                    // test if z of current pixel is nearer than current z buffer value
                                    if (zi < z_ptr[xi])
                                    {
                                        // write textel
                                        screen_ptr[xi] = textel;

                                        // update z-buffer
                                        z_ptr[xi] = zi;           
                                    } // end if
                                } // end if


                                // interpolate u,v
                                ui+=du;
                                vi+=dv;
                                zi+=dz;
                            } // end for xi

                            // interpolate u,v,x along right and left edge
                            xl+=dxdyl;
                            ul+=dudyl;
                            vl+=dvdyl;
                            zl+=dzdyl;

                            xr+=dxdyr;
                            ur+=dudyr;
                            vr+=dvdyr;
                            zr+=dzdyr;

                            // advance screen ptr
                            screen_ptr+=mem_pitch;

                            // advance zbuffer ptr
                            z_ptr+=zpitch;

                            // test for yi hitting second region, if so change interpolant
                            if (yi==yrestart)
                            {
                                // test interpolation side change flag

                                if (irestart == INTERP_LHS)
                                {
                                    // LHS
                                    dyl = (y2 - y1);	

                                    dxdyl = ((x2 - x1)   << FIXP16_SHIFT)/dyl;
                                    dudyl = ((tu2 - tu1) << FIXP16_SHIFT)/dyl;  
                                    dvdyl = ((tv2 - tv1) << FIXP16_SHIFT)/dyl;   		
                                    dzdyl = ((tz2 - tz1) << FIXP16_SHIFT)/dyl;   

                                    // set starting values
                                    xl = (x1  << FIXP16_SHIFT);
                                    ul = (tu1 << FIXP16_SHIFT);
                                    vl = (tv1 << FIXP16_SHIFT);
                                    zl = (tz1 << FIXP16_SHIFT);

                                    // interpolate down on LHS to even up
                                    xl+=dxdyl;
                                    ul+=dudyl;
                                    vl+=dvdyl;
                                    zl+=dzdyl;
                                } // end if
                                else
                                {
                                    // RHS
                                    dyr = (y1 - y2);	

                                    dxdyr = ((x1 - x2)   << FIXP16_SHIFT)/dyr;
                                    dudyr = ((tu1 - tu2) << FIXP16_SHIFT)/dyr;  
                                    dvdyr = ((tv1 - tv2) << FIXP16_SHIFT)/dyr;   		
                                    dzdyr = ((tz1 - tz2) << FIXP16_SHIFT)/dyr; 

                                    // set starting values
                                    xr = (x2  << FIXP16_SHIFT);
                                    ur = (tu2 << FIXP16_SHIFT);
                                    vr = (tv2 << FIXP16_SHIFT);
                                    zr = (tz2 << FIXP16_SHIFT);

                                    // interpolate down on RHS to even up
                                    xr+=dxdyr;
                                    ur+=dudyr;
                                    vr+=dvdyr;
                                    zr+=dzdyr;

                                } // end else

                            } // end if

                        } // end for y

                    } // end else	

            } // end if

} // end Draw_Textured_TriangleZB3_16
