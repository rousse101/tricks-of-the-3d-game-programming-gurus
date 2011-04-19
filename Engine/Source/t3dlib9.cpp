// T3DLIB9.CPP - z buffering

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


// DEFINES //////////////////////////////////////////////////////////////////

// GLOBALS //////////////////////////////////////////////////////////////////


// FUNCTIONS ////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////

void Draw_Triangle_2DZB_16(POLYF4DV2_PTR face,   // ptr to face
                           UCHAR *_dest_buffer,   // pointer to video buffer
                           int mem_pitch,         // bytes per line, 320, 640 etc.
                           UCHAR *_zbuffer,       // pointer to z-buffer
                           int zpitch)            // bytes per line of zbuffer
{
    // this function draws a flat shaded polygon with zbuffering

    int v0=0,
        v1=1,
        v2=2,
        temp=0,
        tri_type = TRI_TYPE_NONE,
        irestart = INTERP_LHS;

    int dx,dy,dyl,dyr,      // general deltas
        z,
        dz,
        xi,yi,              // the current interpolated x,y
        zi,                 // the current interpolated z
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
        dzdyl,   
        zl,
        dzdyr,
        zr;

    int x0,y0,tz0,    // cached vertices
        x1,y1,tz1,
        x2,y2,tz2;

    USHORT *screen_ptr  = NULL,
        *screen_line = NULL,
        *textmap     = NULL,
        *dest_buffer = (USHORT *)_dest_buffer;

    UINT  *z_ptr = NULL,
        *zbuffer = (UINT *)_zbuffer;

    USHORT color;    // polygon color

#ifdef DEBUG_ON
    // track rendering stats
    debug_polys_rendered_per_frame++;
#endif

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

        tz0 = (int)(face->tvlist[v0].z+0.5);

        x1  = (int)(face->tvlist[v1].x+0.0);
        y1  = (int)(face->tvlist[v1].y+0.0);

        tz1 = (int)(face->tvlist[v1].z+0.5);

        x2  = (int)(face->tvlist[v2].x+0.0);
        y2  = (int)(face->tvlist[v2].y+0.0);

        tz2 = (int)(face->tvlist[v2].z+0.5);

        // degenerate triangle
        if ( ((x0 == x1) && (x1 == x2)) || ((y0 ==  y1) && (y1 == y2)))
            return;

        // extract constant color
        color = face->lit_color[0];

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
                dzdyl = ((tz2 - tz0) << FIXP16_SHIFT)/dy; 

                dxdyr = ((x2 - x1)   << FIXP16_SHIFT)/dy;
                dzdyr = ((tz2 - tz1) << FIXP16_SHIFT)/dy;   

                // test for y clipping
                if (y0 < min_clip_y)
                {
                    // compute overclip
                    dy = (min_clip_y - y0);

                    // computer new LHS starting values
                    xl = dxdyl*dy + (x0  << FIXP16_SHIFT);
                    zl = dzdyl*dy + (tz0 << FIXP16_SHIFT);

                    // compute new RHS starting values
                    xr = dxdyr*dy + (x1  << FIXP16_SHIFT);
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

                    zl = (tz0 << FIXP16_SHIFT);
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
                dzdyl = ((tz1 - tz0) << FIXP16_SHIFT)/dy; 

                dxdyr = ((x2 - x0)   << FIXP16_SHIFT)/dy;
                dzdyr = ((tz2 - tz0) << FIXP16_SHIFT)/dy;   

                // test for y clipping
                if (y0 < min_clip_y)
                {
                    // compute overclip
                    dy = (min_clip_y - y0);

                    // computer new LHS starting values
                    xl = dxdyl*dy + (x0  << FIXP16_SHIFT);
                    zl = dzdyl*dy + (tz0 << FIXP16_SHIFT);

                    // compute new RHS starting values
                    xr = dxdyr*dy + (x0  << FIXP16_SHIFT);
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

                    zl = (tz0 << FIXP16_SHIFT);
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

                for (yi = ystart; yi<=yend; yi++)
                {
                    // compute span endpoints
                    xstart = ((xl + FIXP16_ROUND_UP) >> FIXP16_SHIFT);
                    xend   = ((xr + FIXP16_ROUND_UP) >> FIXP16_SHIFT);

                    // compute starting points for u,v,w interpolants
                    zi = zl + FIXP16_ROUND_UP;

                    // compute u,v interpolants
                    if ((dx = (xend - xstart))>0)
                    {
                        dz = (zr - zl)/dx;
                    } // end if
                    else
                    {
                        dz = (zr - zl);
                    } // end else

                    ///////////////////////////////////////////////////////////////////////

                    // test for x clipping, LHS
                    if (xstart < min_clip_x)
                    {
                        // compute x overlap
                        dx = min_clip_x - xstart;

                        // slide interpolants over
                        zi+=dx*dz;

                        // reset vars
                        xstart = min_clip_x;

                    } // end if

                    // test for x clipping RHS
                    if (xend > max_clip_x)
                        xend = max_clip_x;

                    ///////////////////////////////////////////////////////////////////////

                    // draw span
                    for (xi=xstart; xi<=xend; xi++)
                    {
                        // test if z of current pixel is nearer than current z buffer value
                        if (zi < z_ptr[xi])
                        {
                            // write textel assume 5.6.5
                            screen_ptr[xi] = color;

                            // update z-buffer
                            z_ptr[xi] = zi;           
                        } // end if

                        // interpolate u,v,w,z
                        zi+=dz;
                    } // end for xi

                    // interpolate z,x along right and left edge
                    xl+=dxdyl;
                    zl+=dzdyl;

                    xr+=dxdyr;
                    zr+=dzdyr;

                    // advance screen ptr
                    screen_ptr+=mem_pitch;

                    // advance z-buffer ptr
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

                for (yi = ystart; yi<=yend; yi++)
                {
                    // compute span endpoints
                    xstart = ((xl + FIXP16_ROUND_UP) >> FIXP16_SHIFT);
                    xend   = ((xr + FIXP16_ROUND_UP) >> FIXP16_SHIFT);

                    // compute starting points for u,v,w interpolants
                    zi = zl + FIXP16_ROUND_UP;

                    // compute u,v interpolants
                    if ((dx = (xend - xstart))>0)
                    {
                        dz = (zr - zl)/dx;
                    } // end if
                    else
                    {
                        dz = (zr - zl);
                    } // end else

                    // draw span
                    for (xi=xstart; xi<=xend; xi++)
                    {
                        // test if z of current pixel is nearer than current z buffer value
                        if (zi < z_ptr[xi])
                        {
                            // write textel 5.6.5
                            screen_ptr[xi] = color;

                            // update z-buffer
                            z_ptr[xi] = zi;           
                        } // end if


                        // interpolate z
                        zi+=dz;
                    } // end for xi

                    // interpolate x,z along right and left edge
                    xl+=dxdyl;
                    zl+=dzdyl;

                    xr+=dxdyr;
                    zr+=dzdyr;

                    // advance screen ptr
                    screen_ptr+=mem_pitch;

                    // advance z-buffer ptr
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
                    dzdyl = ((tz2 - tz1) << FIXP16_SHIFT)/dyl; 

                    // RHS
                    dyr = (y2 - y0);	

                    dxdyr = ((x2  - x0)  << FIXP16_SHIFT)/dyr;
                    dzdyr = ((tz2 - tz0) << FIXP16_SHIFT)/dyr;  

                    // compute overclip
                    dyr = (min_clip_y - y0);
                    dyl = (min_clip_y - y1);

                    // computer new LHS starting values
                    xl = dxdyl*dyl + (x1  << FIXP16_SHIFT);
                    zl = dzdyl*dyl + (tz1 << FIXP16_SHIFT);

                    // compute new RHS starting values
                    xr = dxdyr*dyr + (x0  << FIXP16_SHIFT);
                    zr = dzdyr*dyr + (tz0 << FIXP16_SHIFT);

                    // compute new starting y
                    ystart = min_clip_y;

                    // test if we need swap to keep rendering left to right
                    if (dxdyr > dxdyl)
                    {
                        SWAP(dxdyl,dxdyr,temp);
                        SWAP(dzdyl,dzdyr,temp);
                        SWAP(xl,xr,temp);
                        SWAP(zl,zr,temp);
                        SWAP(x1,x2,temp);
                        SWAP(y1,y2,temp);
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
                        dzdyl = ((tz1 - tz0) << FIXP16_SHIFT)/dyl; 

                        // RHS
                        dyr = (y2 - y0);	

                        dxdyr = ((x2  - x0)  << FIXP16_SHIFT)/dyr;
                        dzdyr = ((tz2 - tz0) << FIXP16_SHIFT)/dyr;  

                        // compute overclip
                        dy = (min_clip_y - y0);

                        // computer new LHS starting values
                        xl = dxdyl*dy + (x0  << FIXP16_SHIFT);
                        zl = dzdyl*dy + (tz0 << FIXP16_SHIFT);

                        // compute new RHS starting values
                        xr = dxdyr*dy + (x0  << FIXP16_SHIFT);
                        zr = dzdyr*dy + (tz0 << FIXP16_SHIFT);

                        // compute new starting y
                        ystart = min_clip_y;

                        // test if we need swap to keep rendering left to right
                        if (dxdyr < dxdyl)
                        {
                            SWAP(dxdyl,dxdyr,temp);
                            SWAP(dzdyl,dzdyr,temp);
                            SWAP(xl,xr,temp);
                            SWAP(zl,zr,temp);
                            SWAP(x1,x2,temp);
                            SWAP(y1,y2,temp);
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
                        dzdyl = ((tz1 - tz0) << FIXP16_SHIFT)/dyl; 

                        // RHS
                        dyr = (y2 - y0);	

                        dxdyr = ((x2 - x0)   << FIXP16_SHIFT)/dyr;
                        dzdyr = ((tz2 - tz0) << FIXP16_SHIFT)/dyr;

                        // no clipping y

                        // set starting values
                        xl = (x0 << FIXP16_SHIFT);
                        xr = (x0 << FIXP16_SHIFT);

                        zl = (tz0 << FIXP16_SHIFT);

                        zr = (tz0 << FIXP16_SHIFT);

                        // set starting y
                        ystart = y0;

                        // test if we need swap to keep rendering left to right
                        if (dxdyr < dxdyl)
                        {
                            SWAP(dxdyl,dxdyr,temp);
                            SWAP(dzdyl,dzdyr,temp);
                            SWAP(xl,xr,temp);
                            SWAP(zl,zr,temp);
                            SWAP(x1,x2,temp);
                            SWAP(y1,y2,temp);
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

                        for (yi = ystart; yi<=yend; yi++)
                        {
                            // compute span endpoints
                            xstart = ((xl + FIXP16_ROUND_UP) >> FIXP16_SHIFT);
                            xend   = ((xr + FIXP16_ROUND_UP) >> FIXP16_SHIFT);

                            // compute starting points for z interpolants
                            zi = zl + FIXP16_ROUND_UP;

                            // compute z interpolants
                            if ((dx = (xend - xstart))>0)
                            {
                                dz = (zr - zl)/dx;
                            } // end if
                            else
                            {
                                dz = (zr - zl);
                            } // end else

                            ///////////////////////////////////////////////////////////////////////

                            // test for x clipping, LHS
                            if (xstart < min_clip_x)
                            {
                                // compute x overlap
                                dx = min_clip_x - xstart;

                                // slide interpolants over
                                zi+=dx*dz;

                                // set x to left clip edge
                                xstart = min_clip_x;

                            } // end if

                            // test for x clipping RHS
                            if (xend > max_clip_x)
                                xend = max_clip_x;

                            ///////////////////////////////////////////////////////////////////////

                            // draw span
                            for (xi=xstart; xi<=xend; xi++)
                            {
                                // test if z of current pixel is nearer than current z buffer value
                                if (zi < z_ptr[xi])
                                {
                                    // write textel assume 5.6.5
                                    screen_ptr[xi] = color;

                                    // update z-buffer
                                    z_ptr[xi] = zi;           
                                } // end if

                                // interpolate z
                                zi+=dz;
                            } // end for xi

                            // interpolate z,x along right and left edge
                            xl+=dxdyl;
                            zl+=dzdyl;

                            xr+=dxdyr;
                            zr+=dzdyr;

                            // advance screen ptr
                            screen_ptr+=mem_pitch;

                            // advance z-buffer ptr
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
                                    dzdyl = ((tz2 - tz1) << FIXP16_SHIFT)/dyl;  

                                    // set starting values
                                    xl = (x1  << FIXP16_SHIFT);
                                    zl = (tz1 << FIXP16_SHIFT);

                                    // interpolate down on LHS to even up
                                    xl+=dxdyl;
                                    zl+=dzdyl;
                                } // end if
                                else
                                {
                                    // RHS
                                    dyr = (y1 - y2);	

                                    dxdyr = ((x1 - x2)   << FIXP16_SHIFT)/dyr;
                                    dzdyr = ((tz1 - tz2) << FIXP16_SHIFT)/dyr;   

                                    // set starting values
                                    xr = (x2  << FIXP16_SHIFT);
                                    zr = (tz2 << FIXP16_SHIFT);

                                    // interpolate down on RHS to even up
                                    xr+=dxdyr;
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

                        for (yi = ystart; yi<=yend; yi++)
                        {
                            // compute span endpoints
                            xstart = ((xl + FIXP16_ROUND_UP) >> FIXP16_SHIFT);
                            xend   = ((xr + FIXP16_ROUND_UP) >> FIXP16_SHIFT);

                            // compute starting points for u,v,w,z interpolants
                            zi = zl + FIXP16_ROUND_UP;

                            // compute u,v interpolants
                            if ((dx = (xend - xstart))>0)
                            {
                                dz = (zr - zl)/dx;
                            } // end if
                            else
                            {
                                dz = (zr - zl);
                            } // end else

                            // draw span
                            for (xi=xstart; xi<=xend; xi++)
                            {
                                // test if z of current pixel is nearer than current z buffer value
                                if (zi < z_ptr[xi])
                                {
                                    // write textel assume 5.6.5
                                    screen_ptr[xi] = color;

                                    // update z-buffer
                                    z_ptr[xi] = zi;           
                                } // end if

                                // interpolate z
                                zi+=dz;
                            } // end for xi

                            // interpolate x,z along right and left edge
                            xl+=dxdyl;
                            zl+=dzdyl;

                            xr+=dxdyr;
                            zr+=dzdyr;

                            // advance screen ptr
                            screen_ptr+=mem_pitch;

                            // advance z-buffer ptr
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
                                    dzdyl = ((tz2 - tz1) << FIXP16_SHIFT)/dyl;   

                                    // set starting values
                                    xl = (x1  << FIXP16_SHIFT);
                                    zl = (tz1 << FIXP16_SHIFT);

                                    // interpolate down on LHS to even up
                                    xl+=dxdyl;
                                    zl+=dzdyl;
                                } // end if
                                else
                                {
                                    // RHS
                                    dyr = (y1 - y2);	

                                    dxdyr = ((x1 - x2)   << FIXP16_SHIFT)/dyr;
                                    dzdyr = ((tz1 - tz2) << FIXP16_SHIFT)/dyr;   

                                    // set starting values
                                    xr = (x2  << FIXP16_SHIFT);
                                    zr = (tz2 << FIXP16_SHIFT);

                                    // interpolate down on RHS to even up
                                    xr+=dxdyr;
                                    zr+=dzdyr;
                                } // end else

                            } // end if

                        } // end for y

                    } // end else	

            } // end if

} // end Draw_Triangle_2DZB_16

///////////////////////////////////////////////////////////////////////////////

void Draw_Textured_TriangleZB16(POLYF4DV2_PTR face,  // ptr to face
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

                for (yi = ystart; yi<=yend; yi++)
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
                    for (xi=xstart; xi<=xend; xi++)
                    {
                        // test if z of current pixel is nearer than current z buffer value
                        if (zi < z_ptr[xi])
                        {
                            // write textel
                            screen_ptr[xi] = textmap[(ui >> FIXP16_SHIFT) + ((vi >> FIXP16_SHIFT) << texture_shift2)];

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

                for (yi = ystart; yi<=yend; yi++)
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
                    for (xi=xstart; xi<=xend; xi++)
                    {
                        // test if z of current pixel is nearer than current z buffer value
                        if (zi < z_ptr[xi])
                        {
                            // write textel
                            screen_ptr[xi] = textmap[(ui >> FIXP16_SHIFT) + ((vi >> FIXP16_SHIFT) << texture_shift2)];

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

                        for (yi = ystart; yi<=yend; yi++)
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
                            for (xi=xstart; xi<=xend; xi++)
                            {
                                // test if z of current pixel is nearer than current z buffer value
                                if (zi < z_ptr[xi])
                                {
                                    // write textel
                                    screen_ptr[xi] = textmap[(ui >> FIXP16_SHIFT) + ((vi >> FIXP16_SHIFT) << texture_shift2)];

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

                    } // end if
                    else
                    {
                        // no x clipping
                        // point screen ptr to starting line
                        screen_ptr = dest_buffer + (ystart * mem_pitch);

                        // point zbuffer to starting line
                        z_ptr = zbuffer + (ystart * zpitch);

                        for (yi = ystart; yi<=yend; yi++)
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
                            for (xi=xstart; xi<=xend; xi++)
                            {
                                // test if z of current pixel is nearer than current z buffer value
                                if (zi < z_ptr[xi])
                                {
                                    // write textel
                                    screen_ptr[xi] = textmap[(ui >> FIXP16_SHIFT) + ((vi >> FIXP16_SHIFT) << texture_shift2)];

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

                    } // end else	

            } // end if

} // end Draw_Textured_TriangleZB16

///////////////////////////////////////////////////////////////////////////////

void Draw_Textured_TriangleFSZB16(POLYF4DV2_PTR face, // ptr to face
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

                for (yi = ystart; yi<=yend; yi++)
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
                    for (xi=xstart; xi<=xend; xi++)
                    {
                        // test if z of current pixel is nearer than current z buffer value
                        if (zi < z_ptr[xi])
                        {
                            // write textel
                            // get textel first
                            textel = textmap[(ui >> FIXP16_SHIFT) + ((vi >> FIXP16_SHIFT) << texture_shift2)];

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

                for (yi = ystart; yi<=yend; yi++)
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
                    for (xi=xstart; xi<=xend; xi++)
                    {
                        // test if z of current pixel is nearer than current z buffer value
                        if (zi < z_ptr[xi])
                        {
                            // write textel
                            // get textel first
                            textel = textmap[(ui >> FIXP16_SHIFT) + ((vi >> FIXP16_SHIFT) << texture_shift2)];

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

                //Write_Error("\nin tri general: x0=%d, y0=%d,  x1=%d, y1=%d,  x2=%d, y2=%d", x0, y0, x1, y1, x2, y2);

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

                        for (yi = ystart; yi<=yend; yi++)
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
                            for (xi=xstart; xi<=xend; xi++)
                            {
                                // test if z of current pixel is nearer than current z buffer value
                                if (zi < z_ptr[xi])
                                {
                                    // write textel
                                    // get textel first
                                    textel = textmap[(ui >> FIXP16_SHIFT) + ((vi >> FIXP16_SHIFT) << texture_shift2)];

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

                        for (yi = ystart; yi<=yend; yi++)
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
                            for (xi=xstart; xi<=xend; xi++)
                            {
                                // test if z of current pixel is nearer than current z buffer value
                                if (zi < z_ptr[xi])
                                {
                                    // write textel
                                    // get textel first
                                    textel = textmap[(ui >> FIXP16_SHIFT) + ((vi >> FIXP16_SHIFT) << texture_shift2)];

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

} // end Draw_Textured_TriangleFSZB16

///////////////////////////////////////////////////////////////////////////////

void Draw_Gouraud_TriangleZB16(POLYF4DV2_PTR face,   // ptr to face
                               UCHAR *_dest_buffer,   // pointer to video buffer
                               int mem_pitch,         // bytes per line, 320, 640 etc.
                               UCHAR *_zbuffer,       // pointer to z-buffer
                               int zpitch)            // bytes per line of zbuffer
{
    // this function draws a gouraud shaded polygon, based on the affine texture mapper, instead
    // of interpolating the texture coordinates, we simply interpolate the (R,G,B) values across
    // the polygons, I simply needed at another interpolant, I have mapped u->red, v->green, w->blue
    // also a new interpolant for z buffering has been added

    int v0=0,
        v1=1,
        v2=2,
        temp=0,
        tri_type = TRI_TYPE_NONE,
        irestart = INTERP_LHS;

    int dx,dy,dyl,dyr,      // general deltas
        u,v,w,z,
        du,dv,dw,dz,
        xi,yi,              // the current interpolated x,y
        ui,vi,wi,zi,        // the current interpolated u,v,w,z
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
        dudyr,
        ur,
        dvdyr,
        vr,
        dwdyr,
        wr,
        dzdyr,
        zr;

    int x0,y0,tu0,tv0,tw0,tz0,    // cached vertices
        x1,y1,tu1,tv1,tw1,tz1,
        x2,y2,tu2,tv2,tw2,tz2;

    int r_base0, g_base0, b_base0,
        r_base1, g_base1, b_base1,
        r_base2, g_base2, b_base2;

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
    if (FCMP(face->tvlist[v0].y, face->tvlist[v1].y))
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
        tu0 = r_base0;
        tv0 = g_base0; 
        tw0 = b_base0; 

        x1  = (int)(face->tvlist[v1].x+0.0);
        y1  = (int)(face->tvlist[v1].y+0.0);

        tz1 = (int)(face->tvlist[v1].z+0.5);
        tu1 = r_base1;
        tv1 = g_base1; 
        tw1 = b_base1; 

        x2  = (int)(face->tvlist[v2].x+0.0);
        y2  = (int)(face->tvlist[v2].y+0.0);

        tz2 = (int)(face->tvlist[v2].z+0.5);
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

                dxdyr = ((x2 - x1)   << FIXP16_SHIFT)/dy;
                dudyr = ((tu2 - tu1) << FIXP16_SHIFT)/dy;  
                dvdyr = ((tv2 - tv1) << FIXP16_SHIFT)/dy;   
                dwdyr = ((tw2 - tw1) << FIXP16_SHIFT)/dy;   
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
                    wl = dwdyl*dy + (tw0 << FIXP16_SHIFT);
                    zl = dzdyl*dy + (tz0 << FIXP16_SHIFT);

                    // compute new RHS starting values
                    xr = dxdyr*dy + (x1  << FIXP16_SHIFT);
                    ur = dudyr*dy + (tu1 << FIXP16_SHIFT);
                    vr = dvdyr*dy + (tv1 << FIXP16_SHIFT);
                    wr = dwdyr*dy + (tw1 << FIXP16_SHIFT);
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
                    wl = (tw0 << FIXP16_SHIFT);
                    zl = (tz0 << FIXP16_SHIFT);

                    ur = (tu1 << FIXP16_SHIFT);
                    vr = (tv1 << FIXP16_SHIFT);
                    wr = (tw1 << FIXP16_SHIFT);
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
                dwdyl = ((tw1 - tw0) << FIXP16_SHIFT)/dy; 
                dzdyl = ((tz1 - tz0) << FIXP16_SHIFT)/dy; 

                dxdyr = ((x2 - x0)   << FIXP16_SHIFT)/dy;
                dudyr = ((tu2 - tu0) << FIXP16_SHIFT)/dy;  
                dvdyr = ((tv2 - tv0) << FIXP16_SHIFT)/dy;   
                dwdyr = ((tw2 - tw0) << FIXP16_SHIFT)/dy;   
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
                    wl = dwdyl*dy + (tw0 << FIXP16_SHIFT);
                    zl = dzdyl*dy + (tz0 << FIXP16_SHIFT);

                    // compute new RHS starting values
                    xr = dxdyr*dy + (x0  << FIXP16_SHIFT);
                    ur = dudyr*dy + (tu0 << FIXP16_SHIFT);
                    vr = dvdyr*dy + (tv0 << FIXP16_SHIFT);
                    wr = dwdyr*dy + (tw0 << FIXP16_SHIFT);
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
                    wl = (tw0 << FIXP16_SHIFT);
                    zl = (tz0 << FIXP16_SHIFT);

                    ur = (tu0 << FIXP16_SHIFT);
                    vr = (tv0 << FIXP16_SHIFT);
                    wr = (tw0 << FIXP16_SHIFT);
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

                for (yi = ystart; yi<=yend; yi++)
                {
                    // compute span endpoints
                    xstart = ((xl + FIXP16_ROUND_UP) >> FIXP16_SHIFT);
                    xend   = ((xr + FIXP16_ROUND_UP) >> FIXP16_SHIFT);

                    // compute starting points for u,v,w interpolants
                    ui = ul + FIXP16_ROUND_UP;
                    vi = vl + FIXP16_ROUND_UP;
                    wi = wl + FIXP16_ROUND_UP;
                    zi = zl + FIXP16_ROUND_UP;

                    // compute u,v interpolants
                    if ((dx = (xend - xstart))>0)
                    {
                        du = (ur - ul)/dx;
                        dv = (vr - vl)/dx;
                        dw = (wr - wl)/dx;
                        dz = (zr - zl)/dx;
                    } // end if
                    else
                    {
                        du = (ur - ul);
                        dv = (vr - vl);
                        dw = (wr - wl);
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
                        wi+=dx*dw;
                        zi+=dx*dz;

                        // reset vars
                        xstart = min_clip_x;

                    } // end if

                    // test for x clipping RHS
                    if (xend > max_clip_x)
                        xend = max_clip_x;

                    ///////////////////////////////////////////////////////////////////////

                    // draw span
                    for (xi=xstart; xi<=xend; xi++)
                    {
                        // test if z of current pixel is nearer than current z buffer value
                        if (zi < z_ptr[xi])
                        {
                            // write textel assume 5.6.5
                            screen_ptr[xi] = ((ui >> (FIXP16_SHIFT+3)) << 11) + 
                                ((vi >> (FIXP16_SHIFT+2)) << 5) + 
                                (wi >> (FIXP16_SHIFT+3));   

                            // update z-buffer
                            z_ptr[xi] = zi;           
                        } // end if

                        // interpolate u,v,w,z
                        ui+=du;
                        vi+=dv;
                        wi+=dw;
                        zi+=dz;
                    } // end for xi

                    // interpolate u,v,w,z,x along right and left edge
                    xl+=dxdyl;
                    ul+=dudyl;
                    vl+=dvdyl;
                    wl+=dwdyl;
                    zl+=dzdyl;

                    xr+=dxdyr;
                    ur+=dudyr;
                    vr+=dvdyr;
                    wr+=dwdyr;
                    zr+=dzdyr;

                    // advance screen ptr
                    screen_ptr+=mem_pitch;

                    // advance z-buffer ptr
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

                for (yi = ystart; yi<=yend; yi++)
                {
                    // compute span endpoints
                    xstart = ((xl + FIXP16_ROUND_UP) >> FIXP16_SHIFT);
                    xend   = ((xr + FIXP16_ROUND_UP) >> FIXP16_SHIFT);

                    // compute starting points for u,v,w interpolants
                    ui = ul + FIXP16_ROUND_UP;
                    vi = vl + FIXP16_ROUND_UP;
                    wi = wl + FIXP16_ROUND_UP;
                    zi = zl + FIXP16_ROUND_UP;

                    // compute u,v interpolants
                    if ((dx = (xend - xstart))>0)
                    {
                        du = (ur - ul)/dx;
                        dv = (vr - vl)/dx;
                        dw = (wr - wl)/dx;
                        dz = (zr - zl)/dx;
                    } // end if
                    else
                    {
                        du = (ur - ul);
                        dv = (vr - vl);
                        dw = (wr - wl);
                        dz = (zr - zl);
                    } // end else

                    // draw span
                    for (xi=xstart; xi<=xend; xi++)
                    {
                        // test if z of current pixel is nearer than current z buffer value
                        if (zi < z_ptr[xi])
                        {
                            // write textel 5.6.5
                            screen_ptr[xi] = ((ui >> (FIXP16_SHIFT+3)) << 11) + 
                                ((vi >> (FIXP16_SHIFT+2)) << 5) + 
                                (wi >> (FIXP16_SHIFT+3));   

                            // update z-buffer
                            z_ptr[xi] = zi;           
                        } // end if


                        // interpolate u,v,w,z
                        ui+=du;
                        vi+=dv;
                        wi+=dw;
                        zi+=dz;
                    } // end for xi

                    // interpolate u,v,w,x along right and left edge
                    xl+=dxdyl;
                    ul+=dudyl;
                    vl+=dvdyl;
                    wl+=dwdyl;
                    zl+=dzdyl;

                    xr+=dxdyr;
                    ur+=dudyr;
                    vr+=dvdyr;
                    wr+=dwdyr;
                    zr+=dzdyr;

                    // advance screen ptr
                    screen_ptr+=mem_pitch;

                    // advance z-buffer ptr
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

                    // RHS
                    dyr = (y2 - y0);	

                    dxdyr = ((x2  - x0)  << FIXP16_SHIFT)/dyr;
                    dudyr = ((tu2 - tu0) << FIXP16_SHIFT)/dyr;  
                    dvdyr = ((tv2 - tv0) << FIXP16_SHIFT)/dyr;   
                    dwdyr = ((tw2 - tw0) << FIXP16_SHIFT)/dyr;   
                    dzdyr = ((tz2 - tz0) << FIXP16_SHIFT)/dyr;  

                    // compute overclip
                    dyr = (min_clip_y - y0);
                    dyl = (min_clip_y - y1);

                    // computer new LHS starting values
                    xl = dxdyl*dyl + (x1  << FIXP16_SHIFT);

                    ul = dudyl*dyl + (tu1 << FIXP16_SHIFT);
                    vl = dvdyl*dyl + (tv1 << FIXP16_SHIFT);
                    wl = dwdyl*dyl + (tw1 << FIXP16_SHIFT);
                    zl = dzdyl*dyl + (tz1 << FIXP16_SHIFT);

                    // compute new RHS starting values
                    xr = dxdyr*dyr + (x0  << FIXP16_SHIFT);

                    ur = dudyr*dyr + (tu0 << FIXP16_SHIFT);
                    vr = dvdyr*dyr + (tv0 << FIXP16_SHIFT);
                    wr = dwdyr*dyr + (tw0 << FIXP16_SHIFT);
                    zr = dzdyr*dyr + (tz0 << FIXP16_SHIFT);

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
                        SWAP(xl,xr,temp);
                        SWAP(ul,ur,temp);
                        SWAP(vl,vr,temp);
                        SWAP(wl,wr,temp);
                        SWAP(zl,zr,temp);
                        SWAP(x1,x2,temp);
                        SWAP(y1,y2,temp);
                        SWAP(tu1,tu2,temp);
                        SWAP(tv1,tv2,temp);
                        SWAP(tw1,tw2,temp);
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
                        dwdyl = ((tw1 - tw0) << FIXP16_SHIFT)/dyl; 
                        dzdyl = ((tz1 - tz0) << FIXP16_SHIFT)/dyl; 

                        // RHS
                        dyr = (y2 - y0);	

                        dxdyr = ((x2  - x0)  << FIXP16_SHIFT)/dyr;
                        dudyr = ((tu2 - tu0) << FIXP16_SHIFT)/dyr;  
                        dvdyr = ((tv2 - tv0) << FIXP16_SHIFT)/dyr;   
                        dwdyr = ((tw2 - tw0) << FIXP16_SHIFT)/dyr;   
                        dzdyr = ((tz2 - tz0) << FIXP16_SHIFT)/dyr;  

                        // compute overclip
                        dy = (min_clip_y - y0);

                        // computer new LHS starting values
                        xl = dxdyl*dy + (x0  << FIXP16_SHIFT);
                        ul = dudyl*dy + (tu0 << FIXP16_SHIFT);
                        vl = dvdyl*dy + (tv0 << FIXP16_SHIFT);
                        wl = dwdyl*dy + (tw0 << FIXP16_SHIFT);
                        zl = dzdyl*dy + (tz0 << FIXP16_SHIFT);

                        // compute new RHS starting values
                        xr = dxdyr*dy + (x0  << FIXP16_SHIFT);
                        ur = dudyr*dy + (tu0 << FIXP16_SHIFT);
                        vr = dvdyr*dy + (tv0 << FIXP16_SHIFT);
                        wr = dwdyr*dy + (tw0 << FIXP16_SHIFT);
                        zr = dzdyr*dy + (tz0 << FIXP16_SHIFT);

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
                            SWAP(xl,xr,temp);
                            SWAP(ul,ur,temp);
                            SWAP(vl,vr,temp);
                            SWAP(wl,wr,temp);
                            SWAP(zl,zr,temp);
                            SWAP(x1,x2,temp);
                            SWAP(y1,y2,temp);
                            SWAP(tu1,tu2,temp);
                            SWAP(tv1,tv2,temp);
                            SWAP(tw1,tw2,temp);
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
                        dwdyl = ((tw1 - tw0) << FIXP16_SHIFT)/dyl;   
                        dzdyl = ((tz1 - tz0) << FIXP16_SHIFT)/dyl; 

                        // RHS
                        dyr = (y2 - y0);	

                        dxdyr = ((x2 - x0)   << FIXP16_SHIFT)/dyr;
                        dudyr = ((tu2 - tu0) << FIXP16_SHIFT)/dyr;  
                        dvdyr = ((tv2 - tv0) << FIXP16_SHIFT)/dyr;   		
                        dwdyr = ((tw2 - tw0) << FIXP16_SHIFT)/dyr;
                        dzdyr = ((tz2 - tz0) << FIXP16_SHIFT)/dyr;

                        // no clipping y

                        // set starting values
                        xl = (x0 << FIXP16_SHIFT);
                        xr = (x0 << FIXP16_SHIFT);

                        ul = (tu0 << FIXP16_SHIFT);
                        vl = (tv0 << FIXP16_SHIFT);
                        wl = (tw0 << FIXP16_SHIFT);
                        zl = (tz0 << FIXP16_SHIFT);

                        ur = (tu0 << FIXP16_SHIFT);
                        vr = (tv0 << FIXP16_SHIFT);
                        wr = (tw0 << FIXP16_SHIFT);
                        zr = (tz0 << FIXP16_SHIFT);

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
                            SWAP(xl,xr,temp);
                            SWAP(ul,ur,temp);
                            SWAP(vl,vr,temp);
                            SWAP(wl,wr,temp);
                            SWAP(zl,zr,temp);
                            SWAP(x1,x2,temp);
                            SWAP(y1,y2,temp);
                            SWAP(tu1,tu2,temp);
                            SWAP(tv1,tv2,temp);
                            SWAP(tw1,tw2,temp);
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

                        for (yi = ystart; yi<=yend; yi++)
                        {
                            // compute span endpoints
                            xstart = ((xl + FIXP16_ROUND_UP) >> FIXP16_SHIFT);
                            xend   = ((xr + FIXP16_ROUND_UP) >> FIXP16_SHIFT);

                            // compute starting points for u,v,w interpolants
                            ui = ul + FIXP16_ROUND_UP;
                            vi = vl + FIXP16_ROUND_UP;
                            wi = wl + FIXP16_ROUND_UP;
                            zi = zl + FIXP16_ROUND_UP;

                            // compute u,v interpolants
                            if ((dx = (xend - xstart))>0)
                            {
                                du = (ur - ul)/dx;
                                dv = (vr - vl)/dx;
                                dw = (wr - wl)/dx;
                                dz = (zr - zl)/dx;
                            } // end if
                            else
                            {
                                du = (ur - ul);
                                dv = (vr - vl);
                                dw = (wr - wl);
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
                                wi+=dx*dw;
                                zi+=dx*dz;

                                // set x to left clip edge
                                xstart = min_clip_x;

                            } // end if

                            // test for x clipping RHS
                            if (xend > max_clip_x)
                                xend = max_clip_x;

                            ///////////////////////////////////////////////////////////////////////

                            // draw span
                            for (xi=xstart; xi<=xend; xi++)
                            {
                                // test if z of current pixel is nearer than current z buffer value
                                if (zi < z_ptr[xi])
                                {
                                    // write textel assume 5.6.5
                                    screen_ptr[xi] = ((ui >> (FIXP16_SHIFT+3)) << 11) + 
                                        ((vi >> (FIXP16_SHIFT+2)) << 5) + 
                                        (wi >> (FIXP16_SHIFT+3));   

                                    // update z-buffer
                                    z_ptr[xi] = zi;           
                                } // end if

                                // interpolate u,v,w,z
                                ui+=du;
                                vi+=dv;
                                wi+=dw;
                                zi+=dz;
                            } // end for xi

                            // interpolate u,v,w,z,x along right and left edge
                            xl+=dxdyl;
                            ul+=dudyl;
                            vl+=dvdyl;
                            wl+=dwdyl;
                            zl+=dzdyl;

                            xr+=dxdyr;
                            ur+=dudyr;
                            vr+=dvdyr;
                            wr+=dwdyr;
                            zr+=dzdyr;

                            // advance screen ptr
                            screen_ptr+=mem_pitch;

                            // advance z-buffer ptr
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

                                    // set starting values
                                    xl = (x1  << FIXP16_SHIFT);
                                    ul = (tu1 << FIXP16_SHIFT);
                                    vl = (tv1 << FIXP16_SHIFT);
                                    wl = (tw1 << FIXP16_SHIFT);
                                    zl = (tz1 << FIXP16_SHIFT);

                                    // interpolate down on LHS to even up
                                    xl+=dxdyl;
                                    ul+=dudyl;
                                    vl+=dvdyl;
                                    wl+=dwdyl;
                                    zl+=dzdyl;
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

                                    // set starting values
                                    xr = (x2  << FIXP16_SHIFT);
                                    ur = (tu2 << FIXP16_SHIFT);
                                    vr = (tv2 << FIXP16_SHIFT);
                                    wr = (tw2 << FIXP16_SHIFT);
                                    zr = (tz2 << FIXP16_SHIFT);

                                    // interpolate down on RHS to even up
                                    xr+=dxdyr;
                                    ur+=dudyr;
                                    vr+=dvdyr;
                                    wr+=dwdyr;
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

                        for (yi = ystart; yi<=yend; yi++)
                        {
                            // compute span endpoints
                            xstart = ((xl + FIXP16_ROUND_UP) >> FIXP16_SHIFT);
                            xend   = ((xr + FIXP16_ROUND_UP) >> FIXP16_SHIFT);

                            // compute starting points for u,v,w,z interpolants
                            ui = ul + FIXP16_ROUND_UP;
                            vi = vl + FIXP16_ROUND_UP;
                            wi = wl + FIXP16_ROUND_UP;
                            zi = zl + FIXP16_ROUND_UP;

                            // compute u,v interpolants
                            if ((dx = (xend - xstart))>0)
                            {
                                du = (ur - ul)/dx;
                                dv = (vr - vl)/dx;
                                dw = (wr - wl)/dx;
                                dz = (zr - zl)/dx;
                            } // end if
                            else
                            {
                                du = (ur - ul);
                                dv = (vr - vl);
                                dw = (wr - wl);
                                dz = (zr - zl);
                            } // end else

                            // draw span
                            for (xi=xstart; xi<=xend; xi++)
                            {
                                // test if z of current pixel is nearer than current z buffer value
                                if (zi < z_ptr[xi])
                                {
                                    // write textel assume 5.6.5
                                    screen_ptr[xi] = ((ui >> (FIXP16_SHIFT+3)) << 11) + 
                                        ((vi >> (FIXP16_SHIFT+2)) << 5) + 
                                        (wi >> (FIXP16_SHIFT+3));   

                                    // update z-buffer
                                    z_ptr[xi] = zi;           
                                } // end if

                                // interpolate u,v,w,z
                                ui+=du;
                                vi+=dv;
                                wi+=dw;
                                zi+=dz;
                            } // end for xi

                            // interpolate u,v,w,x,z along right and left edge
                            xl+=dxdyl;
                            ul+=dudyl;
                            vl+=dvdyl;
                            wl+=dwdyl;
                            zl+=dzdyl;

                            xr+=dxdyr;
                            ur+=dudyr;
                            vr+=dvdyr;
                            wr+=dwdyr;
                            zr+=dzdyr;

                            // advance screen ptr
                            screen_ptr+=mem_pitch;

                            // advance z-buffer ptr
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

                                    // set starting values
                                    xl = (x1  << FIXP16_SHIFT);
                                    ul = (tu1 << FIXP16_SHIFT);
                                    vl = (tv1 << FIXP16_SHIFT);
                                    wl = (tw1 << FIXP16_SHIFT);
                                    zl = (tz1 << FIXP16_SHIFT);

                                    // interpolate down on LHS to even up
                                    xl+=dxdyl;
                                    ul+=dudyl;
                                    vl+=dvdyl;
                                    wl+=dwdyl;
                                    zl+=dzdyl;
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

                                    // set starting values
                                    xr = (x2  << FIXP16_SHIFT);
                                    ur = (tu2 << FIXP16_SHIFT);
                                    vr = (tv2 << FIXP16_SHIFT);
                                    wr = (tw2 << FIXP16_SHIFT);
                                    zr = (tz2 << FIXP16_SHIFT);

                                    // interpolate down on RHS to even up
                                    xr+=dxdyr;
                                    ur+=dudyr;
                                    vr+=dvdyr;
                                    wr+=dwdyr;
                                    zr+=dzdyr;
                                } // end else

                            } // end if

                        } // end for y

                    } // end else	

            } // end if

} // end Draw_Gouraud_TriangleZB16

///////////////////////////////////////////////////////////////////////////////

void Draw_RENDERLIST4DV2_SolidZB16(RENDERLIST4DV2_PTR rend_list, 
                                   UCHAR *video_buffer, 
                                   int lpitch,
                                   UCHAR *zbuffer,
                                   int zpitch)
{
    // 16-bit version
    // this function "executes" the render list or in other words
    // draws all the faces in the list, the function will call the 
    // proper rasterizer based on the lighting model of the polygons


    POLYF4DV2 face; // temp face used to render polygon

    // at this point, all we have is a list of polygons and it's time
    // to draw them
    for (int poly=0; poly < rend_list->num_polys; poly++)
    {
        // render this polygon if and only if it's not clipped, not culled,
        // active, and visible, note however the concecpt of "backface" is 
        // irrelevant in a wire frame engine though
        if (!(rend_list->poly_ptrs[poly]->state & POLY4DV2_STATE_ACTIVE) ||
            (rend_list->poly_ptrs[poly]->state & POLY4DV2_STATE_CLIPPED ) ||
            (rend_list->poly_ptrs[poly]->state & POLY4DV2_STATE_BACKFACE) )
            continue; // move onto next poly

        // need to test for textured first, since a textured poly can either
        // be emissive, or flat shaded, hence we need to call different
        // rasterizers    
        if (rend_list->poly_ptrs[poly]->attr & POLY4DV2_ATTR_SHADE_MODE_TEXTURE)
        {

            // set the vertices
            face.tvlist[0].x = (float)rend_list->poly_ptrs[poly]->tvlist[0].x;
            face.tvlist[0].y = (float)rend_list->poly_ptrs[poly]->tvlist[0].y;
            face.tvlist[0].z  = (float)rend_list->poly_ptrs[poly]->tvlist[0].z;
            face.tvlist[0].u0 = (float)rend_list->poly_ptrs[poly]->tvlist[0].u0;
            face.tvlist[0].v0 = (float)rend_list->poly_ptrs[poly]->tvlist[0].v0;

            face.tvlist[1].x = (float)rend_list->poly_ptrs[poly]->tvlist[1].x;
            face.tvlist[1].y = (float)rend_list->poly_ptrs[poly]->tvlist[1].y;
            face.tvlist[1].z  = (float)rend_list->poly_ptrs[poly]->tvlist[1].z;
            face.tvlist[1].u0 = (float)rend_list->poly_ptrs[poly]->tvlist[1].u0;
            face.tvlist[1].v0 = (float)rend_list->poly_ptrs[poly]->tvlist[1].v0;

            face.tvlist[2].x = (float)rend_list->poly_ptrs[poly]->tvlist[2].x;
            face.tvlist[2].y = (float)rend_list->poly_ptrs[poly]->tvlist[2].y;
            face.tvlist[2].z  = (float)rend_list->poly_ptrs[poly]->tvlist[2].z;
            face.tvlist[2].u0 = (float)rend_list->poly_ptrs[poly]->tvlist[2].u0;
            face.tvlist[2].v0 = (float)rend_list->poly_ptrs[poly]->tvlist[2].v0;


            // assign the texture
            face.texture = rend_list->poly_ptrs[poly]->texture;

            // is this a plain emissive texture?
            if (rend_list->poly_ptrs[poly]->attr & POLY4DV2_ATTR_SHADE_MODE_CONSTANT)
            {
                // draw the textured triangle as emissive
                Draw_Textured_TriangleZB16(&face, video_buffer, lpitch,zbuffer,zpitch);
            } // end if
            else
            {
                // draw as flat shaded
                face.lit_color[0] = rend_list->poly_ptrs[poly]->lit_color[0];
                Draw_Textured_TriangleFSZB16(&face, video_buffer, lpitch,zbuffer,zpitch);
            } // end else

        } // end if      
        else
            if ((rend_list->poly_ptrs[poly]->attr & POLY4DV2_ATTR_SHADE_MODE_FLAT) || 
                (rend_list->poly_ptrs[poly]->attr & POLY4DV2_ATTR_SHADE_MODE_CONSTANT) )
            {
                // draw as constant shaded
                face.lit_color[0] = rend_list->poly_ptrs[poly]->lit_color[0];

                // set the vertices
                face.tvlist[0].x = (float)rend_list->poly_ptrs[poly]->tvlist[0].x;
                face.tvlist[0].y = (float)rend_list->poly_ptrs[poly]->tvlist[0].y;
                face.tvlist[0].z  = (float)rend_list->poly_ptrs[poly]->tvlist[0].z;

                face.tvlist[1].x = (float)rend_list->poly_ptrs[poly]->tvlist[1].x;
                face.tvlist[1].y = (float)rend_list->poly_ptrs[poly]->tvlist[1].y;
                face.tvlist[1].z  = (float)rend_list->poly_ptrs[poly]->tvlist[1].z;

                face.tvlist[2].x = (float)rend_list->poly_ptrs[poly]->tvlist[2].x;
                face.tvlist[2].y = (float)rend_list->poly_ptrs[poly]->tvlist[2].y;
                face.tvlist[2].z  = (float)rend_list->poly_ptrs[poly]->tvlist[2].z;

                // draw the triangle with basic flat rasterizer
                Draw_Triangle_2DZB_16(&face, video_buffer, lpitch,zbuffer,zpitch);

            } // end if
            else
                if (rend_list->poly_ptrs[poly]->attr & POLY4DV2_ATTR_SHADE_MODE_GOURAUD)
                {
                    // {andre take advantage of the data structures later..}
                    // set the vertices
                    face.tvlist[0].x  = (float)rend_list->poly_ptrs[poly]->tvlist[0].x;
                    face.tvlist[0].y  = (float)rend_list->poly_ptrs[poly]->tvlist[0].y;
                    face.tvlist[0].z  = (float)rend_list->poly_ptrs[poly]->tvlist[0].z;
                    face.lit_color[0] = rend_list->poly_ptrs[poly]->lit_color[0];

                    face.tvlist[1].x  = (float)rend_list->poly_ptrs[poly]->tvlist[1].x;
                    face.tvlist[1].y  = (float)rend_list->poly_ptrs[poly]->tvlist[1].y;
                    face.tvlist[1].z  = (float)rend_list->poly_ptrs[poly]->tvlist[1].z;
                    face.lit_color[1] = rend_list->poly_ptrs[poly]->lit_color[1];

                    face.tvlist[2].x  = (float)rend_list->poly_ptrs[poly]->tvlist[2].x;
                    face.tvlist[2].y  = (float)rend_list->poly_ptrs[poly]->tvlist[2].y;
                    face.tvlist[2].z  = (float)rend_list->poly_ptrs[poly]->tvlist[2].z;
                    face.lit_color[2] = rend_list->poly_ptrs[poly]->lit_color[2];

                    // draw the gouraud shaded triangle
                    Draw_Gouraud_TriangleZB16(&face, video_buffer, lpitch,zbuffer,zpitch);
                } // end if gouraud

    } // end for poly

} // end Draw_RENDERLIST4DV2_SolidZB16

///////////////////////////////////////////////////////////////

int Create_Zbuffer(ZBUFFERV1_PTR zb, // pointer to a zbuffer object
                   int width,      // width 
                   int height,     // height
                   int attr)       // attributes of zbuffer
{
    // this function creates a zbuffer with the sent width, height, 
    // and bytes per word, in fact the the zbuffer is totally linear
    // but this function is nice so we can create it with some
    // intelligence

    // is this a valid object
    if (!zb) 
        return(0);

    // is there any memory already allocated
    if (zb->zbuffer)
        free(zb->zbuffer);

    // set fields
    zb->width  = width;
    zb->height = height;
    zb->attr   = attr;

    // what size zbuffer 16/32 bit?
    if (attr & ZBUFFER_ATTR_16BIT)
    {
        // compute size in quads
        zb->sizeq = width*height/2;

        // allocate memory
        if ((zb->zbuffer = (UCHAR *)malloc(width * height * sizeof(SHORT))))
            return(1);
        else
            return(0);

    } // end if
    else
        if (attr & ZBUFFER_ATTR_32BIT)
        {
            // compute size in quads
            zb->sizeq = width*height;

            // allocate memory
            if ((zb->zbuffer = (UCHAR *)malloc(width * height * sizeof(INT))))
                return(1);
            else
                return(0);
        } // end if
        else
            return(0);

} // end Create_Zbuffer

////////////////////////////////////////////////////////////////

int Delete_Zbuffer(ZBUFFERV1_PTR zb)
{
    // this function deletes and frees the memory of the zbuffer

    // test for valid object
    if (zb)
    {
        // delete memory and zero object
        if (zb->zbuffer)
            free(zb->zbuffer);

        // clear memory
        memset((void *)zb,0, sizeof(ZBUFFERV1));

        return(1);

    } // end if
    else
        return(0);

} // end Delete_Zbuffer

/////////////////////////////////////////////////////////////////

void Clear_Zbuffer(ZBUFFERV1_PTR zb, UINT data)
{
    // this function clears/sets the zbuffer to a value, the filling
    // is ALWAYS performed in QUADS, thus if your zbuffer is a 16-bit
    // zbuffer then you must build a quad that has two values each
    // the 16-bit value you want to fill with, otherwise just send 
    // the fill value casted to a UINT

    Mem_Set_QUAD((void *)zb->zbuffer, data, zb->sizeq); 

} // end Clear_Zbuffer