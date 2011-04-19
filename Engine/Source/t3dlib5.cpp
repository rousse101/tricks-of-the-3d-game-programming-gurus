// T3DLIB5.CPP

#define STANDALONE 0

// INCLUDES ///////////////////////////////////////////////////

#define DEBUG_ON

#define WIN32_LEAN_AND_MEAN  

#if (STANDALONE==1)
#define INITGUID       // you need this or DXGUID.LIB
#endif

#include "CommonHeader.h"

#include <ddraw.h>      // needed for defs in T3DLIB1.H 
#include "T3DLIB1.H"    // T3DLIB4 is based on some defs in this 
#include "T3DLIB4.H"
#include "T3DLIB5.H"

// CLASSES ////////////////////////////////////////////////////

// PROTOTYPES /////////////////////////////////////////////////

// GLOBALS ////////////////////////////////////////////////////

#if (STANDALONE==1)
HWND main_window_handle           = NULL; // save the window handle
HINSTANCE main_instance           = NULL; // save the instance
char buffer[256];                         // used to print text
#endif

// FUNCTIONS //////////////////////////////////////////////////

char *Get_Line_PLG(char *buffer, int maxlength, FILE *fp)
{
    // this little helper function simply read past comments 
    // and blank lines in a PLG file and always returns full 
    // lines with something on them on NULL if the file is empty

    int index = 0;  // general index
    int length = 0; // general length

    // enter into parsing loop
    while(1)
    {
        // read the next line
        if (!fgets(buffer, maxlength, fp))
            return(NULL);

        // kill the whitespace
        for (length = strlen(buffer), index = 0; isspace(buffer[index]); index++);

        // test if this was a blank line or a comment
        if (index >= length || buffer[index]=='#') 
            continue;

        // at this point we have a good line
        return(&buffer[index]);
    } // end while

} // end Get_Line_PLG

///////////////////////////////////////////////////////////////

float Compute_OBJECT4DV1_Radius(OBJECT4DV1_PTR obj)
{
    // this function computes the average and maximum radius for 
    // sent object and opdates the object data

    // reset incase there's any residue
    obj->avg_radius = 0;
    obj->max_radius = 0;

    // loop thru and compute radius
    for (int vertex = 0; vertex < obj->num_vertices; vertex++)
    {
        // update the average and maximum radius
        float dist_to_vertex = 
            sqrt(obj->vlist_local[vertex].x*obj->vlist_local[vertex].x +
            obj->vlist_local[vertex].y*obj->vlist_local[vertex].y +
            obj->vlist_local[vertex].z*obj->vlist_local[vertex].z);

        // accumulate total radius
        obj->avg_radius+=dist_to_vertex;

        // update maximum radius   
        if (dist_to_vertex > obj->max_radius)
            obj->max_radius = dist_to_vertex; 

    } // end for vertex

    // finallize average radius computation
    obj->avg_radius/=obj->num_vertices;

    // return max radius
    return(obj->max_radius);

} // end Compute_OBJECT4DV1_Radius

///////////////////////////////////////////////////////////////

int Load_OBJECT4DV1_PLG(OBJECT4DV1_PTR obj, // pointer to object
                        char *filename,     // filename of plg file
                        VECTOR4D_PTR scale,     // initial scaling factors
                        VECTOR4D_PTR pos,       // initial position
                        VECTOR4D_PTR rot)       // initial rotations
{
    // this function loads a plg object in off disk, additionally
    // it allows the caller to scale, position, and rotate the object
    // to save extra calls later for non-dynamic objects

    FILE *fp;          // file pointer
    char buffer[256];  // working buffer

    char *token_string;  // pointer to actual token text, ready for parsing

    // file format review, note types at end of each description
    // # this is a comment

    // # object descriptor
    // object_name_string num_verts_int num_polys_int

    // # vertex list
    // x0_float y0_float z0_float
    // x1_float y1_float z1_float
    // x2_float y2_float z2_float
    // .
    // .
    // xn_float yn_float zn_float
    //
    // # polygon list
    // surface_description_ushort num_verts_int v0_index_int v1_index_int ..  vn_index_int
    // .
    // .
    // surface_description_ushort num_verts_int v0_index_int v1_index_int ..  vn_index_int

    // lets keep it simple and assume one element per line
    // hence we have to find the object descriptor, read it in, then the
    // vertex list and read it in, and finally the polygon list -- simple :)

    // Step 1: clear out the object and initialize it a bit
    memset(obj, 0, sizeof(OBJECT4DV1));

    // set state of object to active and visible
    obj->state = OBJECT4DV1_STATE_ACTIVE | OBJECT4DV1_STATE_VISIBLE;

    // set position of object
    obj->world_pos.x = pos->x;
    obj->world_pos.y = pos->y;
    obj->world_pos.z = pos->z;
    obj->world_pos.w = pos->w;

    // Step 2: open the file for reading
    if (!(fp = fopen(filename, "r")))
    {
        Write_Error("Couldn't open PLG file %s.", filename);
        return(0);
    } // end if

    // Step 3: get the first token string which should be the object descriptor
    if (!(token_string = Get_Line_PLG(buffer, 255, fp)))
    {
        Write_Error("PLG file error with file %s (object descriptor invalid).",filename);
        return(0);
    } // end if

    Write_Error("Object Descriptor: %s", token_string);

    // parse out the info object
    sscanf(token_string, "%s %d %d", obj->name, &obj->num_vertices, &obj->num_polys);

    // Step 4: load the vertex list
    for (int vertex = 0; vertex < obj->num_vertices; vertex++)
    {
        // get the next vertex
        if (!(token_string = Get_Line_PLG(buffer, 255, fp)))
        {
            Write_Error("PLG file error with file %s (vertex list invalid).",filename);
            return(0);
        } // end if

        // parse out vertex
        sscanf(token_string, "%f %f %f", &obj->vlist_local[vertex].x, 
            &obj->vlist_local[vertex].y, 
            &obj->vlist_local[vertex].z);
        obj->vlist_local[vertex].w = 1;    

        // scale vertices
        obj->vlist_local[vertex].x*=scale->x;
        obj->vlist_local[vertex].y*=scale->y;
        obj->vlist_local[vertex].z*=scale->z;

        Write_Error("\nVertex %d = %f, %f, %f, %f", vertex,
            obj->vlist_local[vertex].x, 
            obj->vlist_local[vertex].y, 
            obj->vlist_local[vertex].z,
            obj->vlist_local[vertex].w);

    } // end for vertex

    // compute average and max radius
    Compute_OBJECT4DV1_Radius(obj);

    Write_Error("\nObject average radius = %f, max radius = %f", 
        obj->avg_radius, obj->max_radius);

    int poly_surface_desc = 0; // PLG/PLX surface descriptor
    int poly_num_verts    = 0; // number of vertices for current poly (always 3)
    char tmp_string[8];        // temp string to hold surface descriptor in and
    // test if it need to be converted from hex

    // Step 5: load the polygon list
    for (int poly=0; poly < obj->num_polys; poly++)
    {
        // get the next polygon descriptor
        if (!(token_string = Get_Line_PLG(buffer, 255, fp)))
        {
            Write_Error("PLG file error with file %s (polygon descriptor invalid).",filename);
            return(0);
        } // end if

        Write_Error("\nPolygon %d:", poly);

        // each vertex list MUST have 3 vertices since we made this a rule that all models
        // must be constructed of triangles
        // read in surface descriptor, number of vertices, and vertex list
        sscanf(token_string, "%s %d %d %d %d", tmp_string,
            &poly_num_verts, // should always be 3 
            &obj->plist[poly].vert[0],
            &obj->plist[poly].vert[1],
            &obj->plist[poly].vert[2]);


        // since we are allowing the surface descriptor to be in hex format
        // with a leading "0x" we need to test for it
        if (tmp_string[0] == '0' && toupper(tmp_string[1]) == 'X')
            sscanf(tmp_string,"%x", &poly_surface_desc);
        else
            poly_surface_desc = atoi(tmp_string);

        // point polygon vertex list to object's vertex list
        // note that this is redundant since the polylist is contained
        // within the object in this case and its up to the user to select
        // whether the local or transformed vertex list is used when building up
        // polygon geometry, might be a better idea to set to NULL in the context
        // of polygons that are part of an object
        obj->plist[poly].vlist = obj->vlist_local; 

        Write_Error("\nSurface Desc = 0x%.4x, num_verts = %d, vert_indices [%d, %d, %d]", 
            poly_surface_desc, 
            poly_num_verts, 
            obj->plist[poly].vert[0],
            obj->plist[poly].vert[1],
            obj->plist[poly].vert[2]);

        // now we that we have the vertex list and we have entered the polygon
        // vertex index data into the polygon itself, now let's analyze the surface
        // descriptor and set the fields for the polygon based on the description

        // extract out each field of data from the surface descriptor
        // first let's get the single/double sided stuff out of the way
        if ((poly_surface_desc & PLX_2SIDED_FLAG))
        {
            SET_BIT(obj->plist[poly].attr, POLY4DV1_ATTR_2SIDED);
            Write_Error("\n2 sided.");
        } // end if
        else
        {
            // one sided
            Write_Error("\n1 sided.");
        } // end else

        // now let's set the color type and color
        if ((poly_surface_desc & PLX_COLOR_MODE_RGB_FLAG)) 
        {
            // this is an RGB 4.4.4 surface
            SET_BIT(obj->plist[poly].attr,POLY4DV1_ATTR_RGB16);

            // now extract color and copy into polygon color
            // field in proper 16-bit format 
            // 0x0RGB is the format, 4 bits per pixel 
            int red   = ((poly_surface_desc & 0x0f00) >> 8);
            int green = ((poly_surface_desc & 0x00f0) >> 4);
            int blue  = (poly_surface_desc & 0x000f);

            // although the data is always in 4.4.4 format, the graphics card
            // is either 5.5.5 or 5.6.5, but our virtual color system translates
            // 8.8.8 into 5.5.5 or 5.6.5 for us, but we have to first scale all
            // these 4.4.4 values into 8.8.8
            obj->plist[poly].color = RGB16Bit(red*16, green*16, blue*16);
            Write_Error("\nRGB color = [%d, %d, %d]", red, green, blue);
        } // end if
        else
        {
            // this is an 8-bit color indexed surface
            SET_BIT(obj->plist[poly].attr,POLY4DV1_ATTR_8BITCOLOR);

            // and simple extract the last 8 bits and that's the color index
            obj->plist[poly].color = (poly_surface_desc & 0x00ff);

            Write_Error("\n8-bit color index = %d", obj->plist[poly].color);

        } // end else

        // handle shading mode
        int shade_mode = (poly_surface_desc & PLX_SHADE_MODE_MASK);

        // set polygon shading mode
        switch(shade_mode)
        {
        case PLX_SHADE_MODE_PURE_FLAG: {
            SET_BIT(obj->plist[poly].attr, POLY4DV1_ATTR_SHADE_MODE_PURE);
            Write_Error("\nShade mode = pure");
                                       } break;

        case PLX_SHADE_MODE_FLAT_FLAG: {
            SET_BIT(obj->plist[poly].attr, POLY4DV1_ATTR_SHADE_MODE_FLAT);
            Write_Error("\nShade mode = flat");

                                       } break;

        case PLX_SHADE_MODE_GOURAUD_FLAG: {
            SET_BIT(obj->plist[poly].attr, POLY4DV1_ATTR_SHADE_MODE_GOURAUD);
            Write_Error("\nShade mode = gouraud");
                                          } break;

        case PLX_SHADE_MODE_PHONG_FLAG: {
            SET_BIT(obj->plist[poly].attr, POLY4DV1_ATTR_SHADE_MODE_PHONG);
            Write_Error("\nShade mode = phong");
                                        } break;

        default: break;
        } // end switch

        // finally set the polygon to active
        obj->plist[poly].state = POLY4DV1_STATE_ACTIVE;    

    } // end for poly

    // close the file
    fclose(fp);

    // return success
    return(1);

} // end Load_OBJECT4DV1_PLG

//////////////////////////////////////////////////////////////

void Translate_OBJECT4DV1(OBJECT4DV1_PTR obj, VECTOR4D_PTR vt) 
{
    // NOTE: Not matrix based
    // this function translates an object without matrices,
    // simply updates the world_pos
    VECTOR4D_Add(&obj->world_pos, vt, &obj->world_pos);

} // end Translate_OBJECT4DV1

/////////////////////////////////////////////////////////////

void Scale_OBJECT4DV1(OBJECT4DV1_PTR obj, VECTOR4D_PTR vs)
{
    // NOTE: Not matrix based
    // this function scales and object without matrices 
    // modifies the object's local vertex list 
    // additionally the radii is updated for the object

    // for each vertex in the mesh scale the local coordinates by
    // vs on a componentwise basis, that is, sx, sy, sz
    for (int vertex=0; vertex < obj->num_vertices; vertex++)
    {
        obj->vlist_local[vertex].x*=vs->x;
        obj->vlist_local[vertex].y*=vs->y;
        obj->vlist_local[vertex].z*=vs->z;
        // leave w unchanged, always equal to 1

    } // end for vertex

    // now since the object is scaled we have to do something with 
    // the radii calculation, but we don't know how the scaling
    // factors relate to the original major axis of the object,
    // therefore for scaling factors all ==1 we will simple multiply
    // which is correct, but for scaling factors not equal to 1, we
    // must take the largest scaling factor and use it to scale the
    // radii with since it's the worst case scenario of the new max and
    // average radii

    // find max scaling factor
    float scale = MAX(vs->x, vs->y);
    scale = MAX(scale, vs->z);

    // now scale
    obj->max_radius*=scale;
    obj->avg_radius*=scale;

} // end Scale_OBJECT4DV1

/////////////////////////////////////////////////////////////

void Build_XYZ_Rotation_MATRIX4X4(float theta_x, // euler angles
                                  float theta_y, 
                                  float theta_z,
                                  MATRIX4X4_PTR mrot) // output 
{
    // this helper function takes a set if euler angles and computes
    // a rotation matrix from them, usefull for object and camera
    // work, also  we will do a little testing in the function to determine
    // the rotations that need to be performed, since there's no
    // reason to perform extra matrix multiplies if the angles are
    // zero!

    MATRIX4X4 mx, my, mz, mtmp;       // working matrices
    float sin_theta=0, cos_theta=0;   // used to initialize matrices
    int rot_seq = 0;                  // 1 for x, 2 for y, 4 for z

    // step 0: fill in with identity matrix
    MAT_IDENTITY_4X4(mrot);

    // step 1: based on zero and non-zero rotation angles, determine
    // rotation sequence
    if (fabs(theta_x) > EPSILON_E5) // x
        rot_seq = rot_seq | 1;

    if (fabs(theta_y) > EPSILON_E5) // y
        rot_seq = rot_seq | 2;

    if (fabs(theta_z) > EPSILON_E5) // z
        rot_seq = rot_seq | 4;

    // now case on sequence
    switch(rot_seq)
    {
    case 0: // no rotation
        {
            // what a waste!
            return;
        } break;

    case 1: // x rotation
        {
            // compute the sine and cosine of the angle
            cos_theta = Fast_Cos(theta_x);
            sin_theta = Fast_Sin(theta_x);

            // set the matrix up 
            Mat_Init_4X4(&mx, 1,    0,          0,         0,
                0,    cos_theta,  sin_theta, 0,
                0,   -sin_theta, cos_theta, 0,
                0,    0,          0,         1);

            // that's it, copy to output matrix
            MAT_COPY_4X4(&mx, mrot);
            return;

        } break;

    case 2: // y rotation
        {
            // compute the sine and cosine of the angle
            cos_theta = Fast_Cos(theta_y);
            sin_theta = Fast_Sin(theta_y);

            // set the matrix up 
            Mat_Init_4X4(&my,cos_theta, 0, -sin_theta, 0,  
                0,         1,  0,         0,
                sin_theta, 0, cos_theta,  0,
                0,         0, 0,          1);


            // that's it, copy to output matrix
            MAT_COPY_4X4(&my, mrot);
            return;

        } break;

    case 3: // xy rotation
        {
            // compute the sine and cosine of the angle for x
            cos_theta = Fast_Cos(theta_x);
            sin_theta = Fast_Sin(theta_x);

            // set the matrix up 
            Mat_Init_4X4(&mx, 1,    0,          0,         0,
                0,    cos_theta,  sin_theta, 0,
                0,   -sin_theta, cos_theta, 0,
                0,    0,          0,         1);

            // compute the sine and cosine of the angle for y
            cos_theta = Fast_Cos(theta_y);
            sin_theta = Fast_Sin(theta_y);

            // set the matrix up 
            Mat_Init_4X4(&my,cos_theta, 0, -sin_theta, 0,  
                0,         1,  0,         0,
                sin_theta, 0, cos_theta,  0,
                0,         0, 0,          1);

            // concatenate matrices 
            Mat_Mul_4X4(&mx, &my, mrot);
            return;

        } break;

    case 4: // z rotation
        {
            // compute the sine and cosine of the angle
            cos_theta = Fast_Cos(theta_z);
            sin_theta = Fast_Sin(theta_z);

            // set the matrix up 
            Mat_Init_4X4(&mz, cos_theta, sin_theta, 0, 0,  
                -sin_theta, cos_theta, 0, 0,
                0,         0,         1, 0,
                0,         0,         0, 1);


            // that's it, copy to output matrix
            MAT_COPY_4X4(&mz, mrot);
            return;

        } break;

    case 5: // xz rotation
        {
            // compute the sine and cosine of the angle x
            cos_theta = Fast_Cos(theta_x);
            sin_theta = Fast_Sin(theta_x);

            // set the matrix up 
            Mat_Init_4X4(&mx, 1,    0,          0,         0,
                0,    cos_theta,  sin_theta, 0,
                0,   -sin_theta, cos_theta, 0,
                0,    0,          0,         1);

            // compute the sine and cosine of the angle z
            cos_theta = Fast_Cos(theta_z);
            sin_theta = Fast_Sin(theta_z);

            // set the matrix up 
            Mat_Init_4X4(&mz, cos_theta, sin_theta, 0, 0,  
                -sin_theta, cos_theta, 0, 0,
                0,         0,         1, 0,
                0,         0,         0, 1);

            // concatenate matrices 
            Mat_Mul_4X4(&mx, &mz, mrot);
            return;

        } break;

    case 6: // yz rotation
        {
            // compute the sine and cosine of the angle y
            cos_theta = Fast_Cos(theta_y);
            sin_theta = Fast_Sin(theta_y);

            // set the matrix up 
            Mat_Init_4X4(&my,cos_theta, 0, -sin_theta, 0,  
                0,         1,  0,         0,
                sin_theta, 0, cos_theta,  0,
                0,         0, 0,          1);

            // compute the sine and cosine of the angle z
            cos_theta = Fast_Cos(theta_z);
            sin_theta = Fast_Sin(theta_z);

            // set the matrix up 
            Mat_Init_4X4(&mz, cos_theta, sin_theta, 0, 0,  
                -sin_theta, cos_theta, 0, 0,
                0,         0,         1, 0,
                0,         0,         0, 1);

            // concatenate matrices 
            Mat_Mul_4X4(&my, &mz, mrot);
            return;

        } break;

    case 7: // xyz rotation
        {
            // compute the sine and cosine of the angle x
            cos_theta = Fast_Cos(theta_x);
            sin_theta = Fast_Sin(theta_x);

            // set the matrix up 
            Mat_Init_4X4(&mx, 1,    0,         0,         0,
                0,    cos_theta, sin_theta, 0,
                0,   -sin_theta, cos_theta, 0,
                0,    0,         0,         1);

            // compute the sine and cosine of the angle y
            cos_theta = Fast_Cos(theta_y);
            sin_theta = Fast_Sin(theta_y);

            // set the matrix up 
            Mat_Init_4X4(&my,cos_theta, 0, -sin_theta, 0,  
                0,         1,  0,         0,
                sin_theta, 0,  cos_theta,  0,
                0,         0,  0,          1);

            // compute the sine and cosine of the angle z
            cos_theta = Fast_Cos(theta_z);
            sin_theta = Fast_Sin(theta_z);

            // set the matrix up 
            Mat_Init_4X4(&mz, cos_theta, sin_theta, 0, 0,  
                -sin_theta, cos_theta, 0, 0,
                0,         0,         1, 0,
                0,         0,         0, 1);

            // concatenate matrices, watch order!
            Mat_Mul_4X4(&mx, &my, &mtmp);
            Mat_Mul_4X4(&mtmp, &mz, mrot);

        } break;

    default: break;

    } // end switch

} // end Build_XYZ_Rotation_MATRIX4X4                                    

///////////////////////////////////////////////////////////

void Build_Model_To_World_MATRIX4X4(VECTOR4D_PTR vpos, MATRIX4X4_PTR m)
{
    // this function builds up a general local to world 
    // transformation matrix that is really nothing more than a translation
    // of the origin by the amount specified in vpos

    Mat_Init_4X4(m, 1,       0,       0,       0, 
        0,       1,       0,       0, 
        0,       0,       1,       0,
        vpos->x, vpos->y, vpos->z, 1 );

} // end Build_Model_To_World_MATRIX4X4

//////////////////////////////////////////////////////////

void Build_Camera_To_Perspective_MATRIX4X4(CAM4DV1_PTR cam, MATRIX4X4_PTR m)
{
    // this function builds up a camera to perspective transformation
    // matrix, in most cases the camera would have a 2x2 normalized
    // view plane with a 90 degree FOV, since the point of the having
    // this matrix must be to also have a perspective to screen (viewport)
    // matrix that scales the normalized coordinates, also the matrix
    // assumes that you are working in 4D homogenous coordinates and at 
    // some point there will be a 4D->3D conversion, it might be immediately
    // after this transform is applied to vertices, or after the perspective
    // to screen transform

    Mat_Init_4X4(m, cam->view_dist, 0,                                0, 0, 
        0,              cam->view_dist*cam->aspect_ratio, 0, 0, 
        0,              0,                                1, 1,
        0,              0,                                0, 0);

} // end Build_Camera_To_Perspective_MATRIX4X4

///////////////////////////////////////////////////////////

void Build_Perspective_To_Screen_4D_MATRIX4X4(CAM4DV1_PTR cam, MATRIX4X4_PTR m)
{
    // this function builds up a perspective to screen transformation
    // matrix, the function assumes that you want to perform the
    // transform in homogeneous coordinates and at raster time there will be 
    // a 4D->3D homogenous conversion and of course only the x,y points
    // will be considered for the 2D rendering, thus you would use this
    // function's matrix is your perspective coordinates were still 
    // in homgeneous form whene this matrix was applied, additionally
    // the point of this matrix to to scale and translate the perspective
    // coordinates to screen coordinates, thus the matrix is built up
    // assuming that the perspective coordinates are in normalized form for
    // a (2x2)/aspect_ratio viewplane, that is, x: -1 to 1, y:-1/aspect_ratio to 1/aspect_ratio

    float alpha = (0.5*cam->viewport_width-0.5);
    float beta  = (0.5*cam->viewport_height-0.5);

    Mat_Init_4X4(m, alpha,   0,     0,    0, 
        0,      -beta,  0,    0, 
        alpha,   beta,  1,    0,
        0,       0,     0,    1);

} // end Build_Perspective_To_Screen_4D_MATRIX4X4()

//////////////////////////////////////////////////////////

void Build_Perspective_To_Screen_MATRIX4X4(CAM4DV1_PTR cam, MATRIX4X4_PTR m)
{
    // this function builds up a perspective to screen transformation
    // matrix, the function assumes that you want to perform the
    // transform in 2D/3D coordinates, that is, you have already converted
    // the perspective coordinates from homogenous 4D to 3D before applying
    // this matrix, additionally
    // the point of this matrix to to scale and translate the perspective
    // coordinates to screen coordinates, thus the matrix is built up
    // assuming that the perspective coordinates are in normalized form for
    // a 2x2 viewplane, that is, x: -1 to 1, y:-1 to 1 
    // the only difference between this function and the version that
    // assumes the coordinates are still in homogenous format is the
    // last column doesn't force w=z, in fact the z, and w results
    // are irrelevent since we assume that BEFORE this matrix is applied
    // all points are already converted from 4D->3D

    float alpha = (0.5*cam->viewport_width-0.5);
    float beta  = (0.5*cam->viewport_height-0.5);

    Mat_Init_4X4(m, alpha,   0,     0,    0, 
        0,      -beta,  0,    0, 
        alpha,   beta,  1,    0,
        0,       0,     0,    1);

} // end Build_Perspective_To_Screen_MATRIX4X4()

///////////////////////////////////////////////////////////

void Build_Camera_To_Screen_MATRIX4X4(CAM4DV1_PTR cam, MATRIX4X4_PTR m)
{
    // this function creates a single matrix that performs the
    // entire camera->perspective->screen transform, the only
    // important thing is that the camera must be created with
    // a viewplane specified to be the size of the viewport
    // furthermore, after this transform is applied the the vertex
    // must be converted from 4D homogeneous to 3D, technically
    // the z is irrelevant since the data would be used for the
    // screen, but still the division by w is needed no matter
    // what

    float alpha = (0.5*cam->viewport_width-0.5);
    float beta  = (0.5*cam->viewport_height-0.5);

    Mat_Init_4X4(m, cam->view_dist,    0,               0,    0, 
        0,                -cam->view_dist,  0,    0, 
        alpha,             beta,            1,    1,
        0,                 0,               0,    0);

} // end Build_Camera_To_Screen_MATRIX4X4()

///////////////////////////////////////////////////////////

void Transform_OBJECT4DV1(OBJECT4DV1_PTR obj, // object to transform
                          MATRIX4X4_PTR mt,   // transformation matrix
                          int coord_select,   // selects coords to transform
                          int transform_basis) // flags if vector orientation
                          // should be transformed too
{
    // this function simply transforms all of the vertices in the local or trans
    // array by the sent matrix

    // what coordinates should be transformed?
    switch(coord_select)
    {
    case TRANSFORM_LOCAL_ONLY:
        {
            // transform each local/model vertex of the object mesh in place
            for (int vertex=0; vertex < obj->num_vertices; vertex++)
            {
                POINT4D presult; // hold result of each transformation

                // transform point
                Mat_Mul_VECTOR4D_4X4(&obj->vlist_local[vertex], mt, &presult);

                // store result back
                VECTOR4D_COPY(&obj->vlist_local[vertex], &presult); 
            } // end for index
        } break;

    case TRANSFORM_TRANS_ONLY:
        {
            // transform each "transformed" vertex of the object mesh in place
            // remember, the idea of the vlist_trans[] array is to accumulate
            // transformations
            for (int vertex=0; vertex < obj->num_vertices; vertex++)
            {
                POINT4D presult; // hold result of each transformation

                // transform point
                Mat_Mul_VECTOR4D_4X4(&obj->vlist_trans[vertex], mt, &presult);

                // store result back
                VECTOR4D_COPY(&obj->vlist_trans[vertex], &presult); 
            } // end for index

        } break;

    case TRANSFORM_LOCAL_TO_TRANS:
        {
            // transform each local/model vertex of the object mesh and store result
            // in "transformed" vertex list
            for (int vertex=0; vertex < obj->num_vertices; vertex++)
            {
                POINT4D presult; // hold result of each transformation

                // transform point
                Mat_Mul_VECTOR4D_4X4(&obj->vlist_local[vertex], mt, &obj->vlist_trans[vertex]);

            } // end for index
        } break;

    default: break;

    } // end switch

    // finally, test if transform should be applied to orientation basis
    // hopefully this is a rotation, otherwise the basis will get corrupted
    if (transform_basis)
    {
        // now rotate orientation basis for object
        VECTOR4D vresult; // use to rotate each orientation vector axis

        // rotate ux of basis
        Mat_Mul_VECTOR4D_4X4(&obj->ux, mt, &vresult);
        VECTOR4D_COPY(&obj->ux, &vresult); 

        // rotate uy of basis
        Mat_Mul_VECTOR4D_4X4(&obj->uy, mt, &vresult);
        VECTOR4D_COPY(&obj->uy, &vresult); 

        // rotate uz of basis
        Mat_Mul_VECTOR4D_4X4(&obj->uz, mt, &vresult);
        VECTOR4D_COPY(&obj->uz, &vresult); 
    } // end if

} // end Transform_OBJECT4DV1

///////////////////////////////////////////////////////////

void Rotate_XYZ_OBJECT4DV1(OBJECT4DV1_PTR obj, // object to rotate
                           float theta_x,      // euler angles
                           float theta_y, 
                           float theta_z)
{
    // this function rotates and object parallel to the
    // XYZ axes in that order or a subset thereof, without
    // matrices (at least externally sent)
    // modifies the object's local vertex list 
    // additionally it rotates the unit directional vectors
    // that track the objects orientation, also note that each
    // time this function is called it calls the rotation generation
    // function, this is wastefull if a number of object are being rotated
    // by the same matrix, therefore, if that's the case, then generate the
    // rotation matrix, store it, and call the general Transform_OBJECT4DV1()
    // with the matrix

    MATRIX4X4 mrot; // used to store generated rotation matrix

    // generate rotation matrix, no way to avoid rotation with a matrix
    // too much math to do manually!
    Build_XYZ_Rotation_MATRIX4X4(theta_x, theta_y, theta_z, &mrot);

    // now simply rotate each point of the mesh in local/model coordinates
    for (int vertex=0; vertex < obj->num_vertices; vertex++)
    {
        POINT4D presult; // hold result of each transformation

        // transform point
        Mat_Mul_VECTOR4D_4X4(&obj->vlist_local[vertex], &mrot, &presult);

        // store result back
        VECTOR4D_COPY(&obj->vlist_local[vertex], &presult); 

    } // end for index

    // now rotate orientation basis for object
    VECTOR4D vresult; // use to rotate each orientation vector axis

    // rotate ux of basis
    Mat_Mul_VECTOR4D_4X4(&obj->ux, &mrot, &vresult);
    VECTOR4D_COPY(&obj->ux, &vresult); 

    // rotate uy of basis
    Mat_Mul_VECTOR4D_4X4(&obj->uy, &mrot, &vresult);
    VECTOR4D_COPY(&obj->uy, &vresult); 

    // rotate uz of basis
    Mat_Mul_VECTOR4D_4X4(&obj->uz, &mrot, &vresult);
    VECTOR4D_COPY(&obj->uz, &vresult); 

} // end Rotate_XYZ_OBJECT4DV1

////////////////////////////////////////////////////////////

void Model_To_World_OBJECT4DV1(OBJECT4DV1_PTR obj, int coord_select)
{
    // NOTE: Not matrix based
    // this function converts the local model coordinates of the
    // sent object into world coordinates, the results are stored
    // in the transformed vertex list (vlist_trans) within the object

    // interate thru vertex list and transform all the model/local 
    // coords to world coords by translating the vertex list by
    // the amount world_pos and storing the results in vlist_trans[]

    if (coord_select == TRANSFORM_LOCAL_TO_TRANS)
    {
        for (int vertex=0; vertex < obj->num_vertices; vertex++)
        {
            // translate vertex
            VECTOR4D_Add(&obj->vlist_local[vertex], &obj->world_pos, &obj->vlist_trans[vertex]);
        } // end for vertex
    } // end if local
    else
    { // TRANSFORM_TRANS_ONLY
        for (int vertex=0; vertex < obj->num_vertices; vertex++)
        {
            // translate vertex
            VECTOR4D_Add(&obj->vlist_trans[vertex], &obj->world_pos, &obj->vlist_trans[vertex]);
        } // end for vertex
    } // end else trans

} // end Model_To_World_OBJECT4DV1

////////////////////////////////////////////////////////////

int Cull_OBJECT4DV1(OBJECT4DV1_PTR obj,  // object to cull
                    CAM4DV1_PTR cam,     // camera to cull relative to
                    int cull_flags)     // clipping planes to consider
{
    // NOTE: is matrix based
    // this function culls an entire object from the viewing
    // frustrum by using the sent camera information and object
    // the cull_flags determine what axes culling should take place
    // x, y, z or all which is controlled by ORing the flags
    // together
    // if the object is culled its state is modified thats all
    // this function assumes that both the camera and the object
    // are valid!

    // step 1: transform the center of the object's bounding
    // sphere into camera space

    POINT4D sphere_pos; // hold result of transforming center of bounding sphere

    // transform point
    Mat_Mul_VECTOR4D_4X4(&obj->world_pos, &cam->mcam, &sphere_pos);

    // step 2:  based on culling flags remove the object
    if (cull_flags & CULL_OBJECT_Z_PLANE)
    {
        // cull only based on z clipping planes

        // test far plane
        if ( ((sphere_pos.z - obj->max_radius) > cam->far_clip_z) ||
            ((sphere_pos.z + obj->max_radius) < cam->near_clip_z) )
        { 
            SET_BIT(obj->state, OBJECT4DV1_STATE_CULLED);
            return(1);
        } // end if

    } // end if

    if (cull_flags & CULL_OBJECT_X_PLANE)
    {
        // cull only based on x clipping planes
        // we could use plane equations, but simple similar triangles
        // is easier since this is really a 2D problem
        // if the view volume is 90 degrees the the problem is trivial
        // buts lets assume its not

        // test the the right and left clipping planes against the leftmost and rightmost
        // points of the bounding sphere
        float z_test = (0.5)*cam->viewplane_width*sphere_pos.z/cam->view_dist;

        if ( ((sphere_pos.x-obj->max_radius) > z_test)  || // right side
            ((sphere_pos.x+obj->max_radius) < -z_test) )  // left side, note sign change
        { 
            SET_BIT(obj->state, OBJECT4DV1_STATE_CULLED);
            return(1);
        } // end if
    } // end if

    if (cull_flags & CULL_OBJECT_Y_PLANE)
    {
        // cull only based on y clipping planes
        // we could use plane equations, but simple similar triangles
        // is easier since this is really a 2D problem
        // if the view volume is 90 degrees the the problem is trivial
        // buts lets assume its not

        // test the the top and bottom clipping planes against the bottommost and topmost
        // points of the bounding sphere
        float z_test = (0.5)*cam->viewplane_height*sphere_pos.z/cam->view_dist;

        if ( ((sphere_pos.y-obj->max_radius) > z_test)  || // top side
            ((sphere_pos.y+obj->max_radius) < -z_test) )  // bottom side, note sign change
        { 
            SET_BIT(obj->state, OBJECT4DV1_STATE_CULLED);
            return(1);
        } // end if

    } // end if

    // return failure to cull
    return(0);

} // end Cull_OBJECT4DV1

////////////////////////////////////////////////////////////

void Remove_Backfaces_OBJECT4DV1(OBJECT4DV1_PTR obj, CAM4DV1_PTR cam)
{
    // NOTE: this is not a matrix based function
    // this function removes the backfaces from an object's
    // polygon mesh, the function does this based on the vertex
    // data in vlist_trans along with the camera position (only)
    // note that only the backface state is set in each polygon

    // test if the object is culled
    if (obj->state & OBJECT4DV1_STATE_CULLED)
        return;

    // process each poly in mesh
    for (int poly=0; poly < obj->num_polys; poly++)
    {
        // acquire polygon
        POLY4DV1_PTR curr_poly = &obj->plist[poly];

        // is this polygon valid?
        // test this polygon if and only if it's not clipped, not culled,
        // active, and visible and not 2 sided. Note we test for backface in the event that
        // a previous call might have already determined this, so why work
        // harder!
        if (!(curr_poly->state & POLY4DV1_STATE_ACTIVE) ||
            (curr_poly->state & POLY4DV1_STATE_CLIPPED ) ||
            (curr_poly->attr  & POLY4DV1_ATTR_2SIDED)    ||
            (curr_poly->state & POLY4DV1_STATE_BACKFACE) )
            continue; // move onto next poly

        // extract vertex indices into master list, rember the polygons are 
        // NOT self contained, but based on the vertex list stored in the object
        // itself
        int vindex_0 = curr_poly->vert[0];
        int vindex_1 = curr_poly->vert[1];
        int vindex_2 = curr_poly->vert[2];

        // we will use the transformed polygon vertex list since the backface removal
        // only makes sense at the world coord stage further of the pipeline 

        // we need to compute the normal of this polygon face, and recall
        // that the vertices are in cw order, u = p0->p1, v=p0->p2, n=uxv
        VECTOR4D u, v, n;

        // build u, v
        VECTOR4D_Build(&obj->vlist_trans[ vindex_0 ], &obj->vlist_trans[ vindex_1 ], &u);
        VECTOR4D_Build(&obj->vlist_trans[ vindex_0 ], &obj->vlist_trans[ vindex_2 ], &v);

        // compute cross product
        VECTOR4D_Cross(&u, &v, &n);

        // now create eye vector to viewpoint
        VECTOR4D view;
        VECTOR4D_Build(&obj->vlist_trans[ vindex_0 ], &cam->pos, &view); 

        // and finally, compute the dot product
        float dp = VECTOR4D_Dot(&n, &view);

        // if the sign is > 0 then visible, 0 = scathing, < 0 invisible
        if (dp <= 0.0 )
            SET_BIT(curr_poly->state, POLY4DV1_STATE_BACKFACE);

    } // end for poly

} // end Remove_Backfaces_OBJECT4DV1

////////////////////////////////////////////////////////////

void Remove_Backfaces_RENDERLIST4DV1(RENDERLIST4DV1_PTR rend_list, CAM4DV1_PTR cam)
{
    // NOTE: this is not a matrix based function
    // this function removes the backfaces from polygon list
    // the function does this based on the polygon list data
    // tvlist along with the camera position (only)
    // note that only the backface state is set in each polygon

    for (int poly = 0; poly < rend_list->num_polys; poly++)
    {
        // acquire current polygon
        POLYF4DV1_PTR curr_poly = rend_list->poly_ptrs[poly];

        // is this polygon valid?
        // test this polygon if and only if it's not clipped, not culled,
        // active, and visible and not 2 sided. Note we test for backface in the event that
        // a previous call might have already determined this, so why work
        // harder!
        if ((curr_poly==NULL) || !(curr_poly->state & POLY4DV1_STATE_ACTIVE) ||
            (curr_poly->state & POLY4DV1_STATE_CLIPPED ) || 
            (curr_poly->attr  & POLY4DV1_ATTR_2SIDED)    ||
            (curr_poly->state & POLY4DV1_STATE_BACKFACE) )
            continue; // move onto next poly

        // we need to compute the normal of this polygon face, and recall
        // that the vertices are in cw order, u = p0->p1, v=p0->p2, n=uxv
        VECTOR4D u, v, n;

        // build u, v
        VECTOR4D_Build(&curr_poly->tvlist[0], &curr_poly->tvlist[1], &u);
        VECTOR4D_Build(&curr_poly->tvlist[0], &curr_poly->tvlist[2], &v);

        // compute cross product
        VECTOR4D_Cross(&u, &v, &n);

        // now create eye vector to viewpoint
        VECTOR4D view;
        VECTOR4D_Build(&curr_poly->tvlist[0], &cam->pos, &view); 

        // and finally, compute the dot product
        float dp = VECTOR4D_Dot(&n, &view);

        // if the sign is > 0 then visible, 0 = scathing, < 0 invisible
        if (dp <= 0.0 )
            SET_BIT(curr_poly->state, POLY4DV1_STATE_BACKFACE);

    } // end for poly

} // end Remove_Backfaces_RENDERLIST4DV1

////////////////////////////////////////////////////////////

void World_To_Camera_OBJECT4DV1(OBJECT4DV1_PTR obj, CAM4DV1_PTR cam)
{
    // NOTE: this is a matrix based function
    // this function transforms the world coordinates of an object
    // into camera coordinates, based on the sent camera matrix
    // but it totally disregards the polygons themselves,
    // it only works on the vertices in the vlist_trans[] list
    // this is one way to do it, you might instead transform
    // the global list of polygons in the render list since you 
    // are guaranteed that those polys represent geometry that 
    // has passed thru backfaces culling (if any)

    // transform each vertex in the object to camera coordinates
    // assumes the object has already been transformed to world
    // coordinates and the result is in vlist_trans[]
    for (int vertex = 0; vertex < obj->num_vertices; vertex++)
    {
        // transform the vertex by the mcam matrix within the camera
        // it better be valid!
        POINT4D presult; // hold result of each transformation

        // transform point
        Mat_Mul_VECTOR4D_4X4(&obj->vlist_trans[vertex], &cam->mcam, &presult);

        // store result back
        VECTOR4D_COPY(&obj->vlist_trans[vertex], &presult); 
    } // end for vertex

} // end World_To_Camera_OBJECT4DV1

////////////////////////////////////////////////////////////

void Camera_To_Perspective_OBJECT4DV1(OBJECT4DV1_PTR obj, CAM4DV1_PTR cam)
{
    // NOTE: this is not a matrix based function
    // this function transforms the camera coordinates of an object
    // into perspective coordinates, based on the 
    // sent camera object, but it totally disregards the polygons themselves,
    // it only works on the vertices in the vlist_trans[] list
    // this is one way to do it, you might instead transform
    // the global list of polygons in the render list since you 
    // are guaranteed that those polys represent geometry that 
    // has passed thru backfaces culling (if any)
    // finally this function is really for experimental reasons only
    // you would probably never let an object stay intact this far down
    // the pipeline, since it's probably that there's only a single polygon
    // that is visible! But this function has to transform the whole mesh!

    // transform each vertex in the object to perspective coordinates
    // assumes the object has already been transformed to camera
    // coordinates and the result is in vlist_trans[]
    for (int vertex = 0; vertex < obj->num_vertices; vertex++)
    {
        float z = obj->vlist_trans[vertex].z;

        // transform the vertex by the view parameters in the camera
        obj->vlist_trans[vertex].x = cam->view_dist*obj->vlist_trans[vertex].x/z;
        obj->vlist_trans[vertex].y = cam->view_dist*obj->vlist_trans[vertex].y*cam->aspect_ratio/z;
        // z = z, so no change

        // not that we are NOT dividing by the homogenous w coordinate since
        // we are not using a matrix operation for this version of the function 

    } // end for vertex

} // end Camera_To_Perspective_OBJECT4DV1

//////////////////////////////////////////////////////////////

void Camera_To_Perspective_Screen_OBJECT4DV1(OBJECT4DV1_PTR obj, CAM4DV1_PTR cam)
{
    // NOTE: this is not a matrix based function
    // this function transforms the camera coordinates of an object
    // into Screen scaled perspective coordinates, based on the 
    // sent camera object, that is, view_dist_h and view_dist_v 
    // should be set to cause the desired (width X height)
    // projection of the vertices, but the function totally 
    // disregards the polygons themselves,
    // it only works on the vertices in the vlist_trans[] list
    // this is one way to do it, you might instead transform
    // the global list of polygons in the render list since you 
    // are guaranteed that those polys represent geometry that 
    // has passed thru backfaces culling (if any)
    // finally this function is really for experimental reasons only
    // you would probably never let an object stay intact this far down
    // the pipeline, since it's probably that there's only a single polygon
    // that is visible! But this function has to transform the whole mesh!
    // finally, the function also inverts the y axis, so the coordinates
    // generated from this function ARE screen coordinates and ready for
    // rendering

    float alpha = (0.5*cam->viewport_width-0.5);
    float beta  = (0.5*cam->viewport_height-0.5);

    // transform each vertex in the object to perspective screen coordinates
    // assumes the object has already been transformed to camera
    // coordinates and the result is in vlist_trans[]
    for (int vertex = 0; vertex < obj->num_vertices; vertex++)
    {
        float z = obj->vlist_trans[vertex].z;

        // transform the vertex by the view parameters in the camera
        obj->vlist_trans[vertex].x = cam->view_dist*obj->vlist_trans[vertex].x/z;
        obj->vlist_trans[vertex].y = cam->view_dist*obj->vlist_trans[vertex].y/z;
        // z = z, so no change

        // not that we are NOT dividing by the homogenous w coordinate since
        // we are not using a matrix operation for this version of the function 

        // now the coordinates are in the range x:(-viewport_width/2 to viewport_width/2)
        // and y:(-viewport_height/2 to viewport_height/2), thus we need a translation and
        // since the y-axis is inverted, we need to invert y to complete the screen 
        // transform:
        obj->vlist_trans[vertex].x =  obj->vlist_trans[vertex].x + alpha;
        obj->vlist_trans[vertex].y = -obj->vlist_trans[vertex].y + beta;

    } // end for vertex

} // end Camera_To_Perspective_Screen_OBJECT4DV1

//////////////////////////////////////////////////////////////

void Perspective_To_Screen_OBJECT4DV1(OBJECT4DV1_PTR obj, CAM4DV1_PTR cam)
{
    // NOTE: this is not a matrix based function
    // this function transforms the perspective coordinates of an object
    // into screen coordinates, based on the sent viewport info
    // but it totally disregards the polygons themselves,
    // it only works on the vertices in the vlist_trans[] list
    // this is one way to do it, you might instead transform
    // the global list of polygons in the render list since you 
    // are guaranteed that those polys represent geometry that 
    // has passed thru backfaces culling (if any)
    // finally this function is really for experimental reasons only
    // you would probably never let an object stay intact this far down
    // the pipeline, since it's probably that there's only a single polygon
    // that is visible! But this function has to transform the whole mesh!
    // this function would be called after a perspective
    // projection was performed on the object

    // transform each vertex in the object to screen coordinates
    // assumes the object has already been transformed to perspective
    // coordinates and the result is in vlist_trans[]

    float alpha = (0.5*cam->viewport_width-0.5);
    float beta  = (0.5*cam->viewport_height-0.5);

    for (int vertex = 0; vertex < obj->num_vertices; vertex++)
    {
        // assumes the vertex is in perspective normalized coords from -1 to 1
        // on each axis, simple scale them to viewport and invert y axis and project
        // to screen

        // transform the vertex by the view parameters in the camera
        obj->vlist_trans[vertex].x = alpha + alpha*obj->vlist_trans[vertex].x;
        obj->vlist_trans[vertex].y = beta  - beta *obj->vlist_trans[vertex].y;

    } // end for vertex

} // end Perspective_To_Screen_OBJECT4DV1

/////////////////////////////////////////////////////////////

void Convert_From_Homogeneous4D_OBJECT4DV1(OBJECT4DV1_PTR obj)
{
    // this function convertes all vertices in the transformed
    // vertex list from 4D homogeneous coordinates to normal 3D coordinates
    // by dividing each x,y,z component by w

    for (int vertex = 0; vertex < obj->num_vertices; vertex++)
    {
        // convert to non-homogenous coords
        VECTOR4D_DIV_BY_W(&obj->vlist_trans[vertex]);     
    } // end for vertex

} // end Convert_From_Homogeneous4D_OBJECT4DV1

/////////////////////////////////////////////////////////////

void Transform_RENDERLIST4DV1(RENDERLIST4DV1_PTR rend_list, // render list to transform
                              MATRIX4X4_PTR mt,   // transformation matrix
                              int coord_select)   // selects coords to transform
{
    // this function simply transforms all of the polygons vertices in the local or trans
    // array of the render list by the sent matrix

    // what coordinates should be transformed?
    switch(coord_select)
    {
    case TRANSFORM_LOCAL_ONLY:
        {
            for (int poly = 0; poly < rend_list->num_polys; poly++)
            {
                // acquire current polygon
                POLYF4DV1_PTR curr_poly = rend_list->poly_ptrs[poly];

                // is this polygon valid?
                // transform this polygon if and only if it's not clipped, not culled,
                // active, and visible, note however the concept of "backface" is 
                // irrelevant in a wire frame engine though
                if ((curr_poly==NULL) || !(curr_poly->state & POLY4DV1_STATE_ACTIVE) ||
                    (curr_poly->state & POLY4DV1_STATE_CLIPPED ) ||
                    (curr_poly->state & POLY4DV1_STATE_BACKFACE) )
                    continue; // move onto next poly

                // all good, let's transform 
                for (int vertex = 0; vertex < 3; vertex++)
                {
                    // transform the vertex by mt
                    POINT4D presult; // hold result of each transformation

                    // transform point
                    Mat_Mul_VECTOR4D_4X4(&curr_poly->vlist[vertex], mt, &presult);

                    // store result back
                    VECTOR4D_COPY(&curr_poly->vlist[vertex], &presult); 
                } // end for vertex

            } // end for poly

        } break;

    case TRANSFORM_TRANS_ONLY:
        {
            // transform each "transformed" vertex of the render list
            // remember, the idea of the tvlist[] array is to accumulate
            // transformations
            for (int poly = 0; poly < rend_list->num_polys; poly++)
            {
                // acquire current polygon
                POLYF4DV1_PTR curr_poly = rend_list->poly_ptrs[poly];

                // is this polygon valid?
                // transform this polygon if and only if it's not clipped, not culled,
                // active, and visible, note however the concept of "backface" is 
                // irrelevant in a wire frame engine though
                if ((curr_poly==NULL) || !(curr_poly->state & POLY4DV1_STATE_ACTIVE) ||
                    (curr_poly->state & POLY4DV1_STATE_CLIPPED ) ||
                    (curr_poly->state & POLY4DV1_STATE_BACKFACE) )
                    continue; // move onto next poly

                // all good, let's transform 
                for (int vertex = 0; vertex < 3; vertex++)
                {
                    // transform the vertex by mt
                    POINT4D presult; // hold result of each transformation

                    // transform point
                    Mat_Mul_VECTOR4D_4X4(&curr_poly->tvlist[vertex], mt, &presult);

                    // store result back
                    VECTOR4D_COPY(&curr_poly->tvlist[vertex], &presult); 
                } // end for vertex

            } // end for poly

        } break;

    case TRANSFORM_LOCAL_TO_TRANS:
        {
            // transform each local/model vertex of the render list and store result
            // in "transformed" vertex list
            for (int poly = 0; poly < rend_list->num_polys; poly++)
            {
                // acquire current polygon
                POLYF4DV1_PTR curr_poly = rend_list->poly_ptrs[poly];

                // is this polygon valid?
                // transform this polygon if and only if it's not clipped, not culled,
                // active, and visible, note however the concept of "backface" is 
                // irrelevant in a wire frame engine though
                if ((curr_poly==NULL) || !(curr_poly->state & POLY4DV1_STATE_ACTIVE) ||
                    (curr_poly->state & POLY4DV1_STATE_CLIPPED ) ||
                    (curr_poly->state & POLY4DV1_STATE_BACKFACE) )
                    continue; // move onto next poly

                // all good, let's transform 
                for (int vertex = 0; vertex < 3; vertex++)
                {
                    // transform the vertex by mt
                    Mat_Mul_VECTOR4D_4X4(&curr_poly->vlist[vertex], mt, &curr_poly->tvlist[vertex]);
                } // end for vertex

            } // end for poly

        } break;

    default: break;

    } // end switch

} // end Transform_RENDERLIST4DV1

/////////////////////////////////////////////////////////////////////////

void Model_To_World_RENDERLIST4DV1(RENDERLIST4DV1_PTR rend_list, 
                                   POINT4D_PTR world_pos, 
                                   int coord_select)
{
    // NOTE: Not matrix based
    // this function converts the local model coordinates of the
    // sent render list into world coordinates, the results are stored
    // in the transformed vertex list (tvlist) within the renderlist

    // interate thru vertex list and transform all the model/local 
    // coords to world coords by translating the vertex list by
    // the amount world_pos and storing the results in tvlist[]
    // is this polygon valid?

    if (coord_select == TRANSFORM_LOCAL_TO_TRANS)
    {
        for (int poly = 0; poly < rend_list->num_polys; poly++)
        {
            // acquire current polygon
            POLYF4DV1_PTR curr_poly = rend_list->poly_ptrs[poly];

            // transform this polygon if and only if it's not clipped, not culled,
            // active, and visible, note however the concept of "backface" is 
            // irrelevant in a wire frame engine though
            if ((curr_poly==NULL) || !(curr_poly->state & POLY4DV1_STATE_ACTIVE) ||
                (curr_poly->state & POLY4DV1_STATE_CLIPPED ) ||
                (curr_poly->state & POLY4DV1_STATE_BACKFACE) )
                continue; // move onto next poly

            // all good, let's transform 
            for (int vertex = 0; vertex < 3; vertex++)
            {
                // translate vertex
                VECTOR4D_Add(&curr_poly->vlist[vertex], world_pos, &curr_poly->tvlist[vertex]);
            } // end for vertex

        } // end for poly
    } // end if local
    else // TRANSFORM_TRANS_ONLY
    {
        for (int poly = 0; poly < rend_list->num_polys; poly++)
        {
            // acquire current polygon
            POLYF4DV1_PTR curr_poly = rend_list->poly_ptrs[poly];

            // transform this polygon if and only if it's not clipped, not culled,
            // active, and visible, note however the concept of "backface" is 
            // irrelevant in a wire frame engine though
            if ((curr_poly==NULL) || !(curr_poly->state & POLY4DV1_STATE_ACTIVE) ||
                (curr_poly->state & POLY4DV1_STATE_CLIPPED ) ||
                (curr_poly->state & POLY4DV1_STATE_BACKFACE) )
                continue; // move onto next poly

            for (int vertex = 0; vertex < 3; vertex++)
            {
                // translate vertex
                VECTOR4D_Add(&curr_poly->tvlist[vertex], world_pos, &curr_poly->tvlist[vertex]);
            } // end for vertex

        } // end for poly

    } // end else

} // end Model_To_World_RENDERLIST4DV1

////////////////////////////////////////////////////////////

void Convert_From_Homogeneous4D_RENDERLIST4DV1(RENDERLIST4DV1_PTR rend_list)
{
    // this function convertes all valid polygons vertices in the transformed
    // vertex list from 4D homogeneous coordinates to normal 3D coordinates
    // by dividing each x,y,z component by w

    for (int poly = 0; poly < rend_list->num_polys; poly++)
    {
        // acquire current polygon
        POLYF4DV1_PTR curr_poly = rend_list->poly_ptrs[poly];

        // is this polygon valid?
        // transform this polygon if and only if it's not clipped, not culled,
        // active, and visible, note however the concept of "backface" is 
        // irrelevant in a wire frame engine though
        if ((curr_poly==NULL) || !(curr_poly->state & POLY4DV1_STATE_ACTIVE) ||
            (curr_poly->state & POLY4DV1_STATE_CLIPPED ) ||
            (curr_poly->state & POLY4DV1_STATE_BACKFACE) )
            continue; // move onto next poly

        // all good, let's transform 
        for (int vertex = 0; vertex < 3; vertex++)
        {
            // convert to non-homogenous coords
            VECTOR4D_DIV_BY_W(&curr_poly->tvlist[vertex]); 
        } // end for vertex

    } // end for poly

} // end Convert_From_Homogeneous4D_RENDERLIST4DV1

/////////////////////////////////////////////////////////////////////////

void World_To_Camera_RENDERLIST4DV1(RENDERLIST4DV1_PTR rend_list, 
                                    CAM4DV1_PTR cam)
{
    // NOTE: this is a matrix based function
    // this function transforms each polygon in the global render list
    // to camera coordinates based on the sent camera transform matrix
    // you would use this function instead of the object based function
    // if you decided earlier in the pipeline to turn each object into 
    // a list of polygons and then add them to the global render list
    // the conversion of an object into polygons probably would have
    // happened after object culling, local transforms, local to world
    // and backface culling, so the minimum number of polygons from
    // each object are in the list, note that the function assumes
    // that at LEAST the local to world transform has been called
    // and the polygon data is in the transformed list tvlist of
    // the POLYF4DV1 object

    // transform each polygon in the render list into camera coordinates
    // assumes the render list has already been transformed to world
    // coordinates and the result is in tvlist[] of each polygon object

    for (int poly = 0; poly < rend_list->num_polys; poly++)
    {
        // acquire current polygon
        POLYF4DV1_PTR curr_poly = rend_list->poly_ptrs[poly];

        // is this polygon valid?
        // transform this polygon if and only if it's not clipped, not culled,
        // active, and visible, note however the concept of "backface" is 
        // irrelevant in a wire frame engine though
        if ((curr_poly==NULL) || !(curr_poly->state & POLY4DV1_STATE_ACTIVE) ||
            (curr_poly->state & POLY4DV1_STATE_CLIPPED ) ||
            (curr_poly->state & POLY4DV1_STATE_BACKFACE) )
            continue; // move onto next poly

        // all good, let's transform 
        for (int vertex = 0; vertex < 3; vertex++)
        {
            // transform the vertex by the mcam matrix within the camera
            // it better be valid!
            POINT4D presult; // hold result of each transformation

            // transform point
            Mat_Mul_VECTOR4D_4X4(&curr_poly->tvlist[vertex], &cam->mcam, &presult);

            // store result back
            VECTOR4D_COPY(&curr_poly->tvlist[vertex], &presult); 
        } // end for vertex

    } // end for poly

} // end World_To_Camera_RENDERLIST4DV1

///////////////////////////////////////////////////////////////

void Camera_To_Perspective_RENDERLIST4DV1(RENDERLIST4DV1_PTR rend_list, 
                                          CAM4DV1_PTR cam)
{
    // NOTE: this is not a matrix based function
    // this function transforms each polygon in the global render list
    // into perspective coordinates, based on the 
    // sent camera object, 
    // you would use this function instead of the object based function
    // if you decided earlier in the pipeline to turn each object into 
    // a list of polygons and then add them to the global render list

    // transform each polygon in the render list into camera coordinates
    // assumes the render list has already been transformed to world
    // coordinates and the result is in tvlist[] of each polygon object

    for (int poly = 0; poly < rend_list->num_polys; poly++)
    {
        // acquire current polygon
        POLYF4DV1_PTR curr_poly = rend_list->poly_ptrs[poly];

        // is this polygon valid?
        // transform this polygon if and only if it's not clipped, not culled,
        // active, and visible, note however the concept of "backface" is 
        // irrelevant in a wire frame engine though
        if ((curr_poly==NULL) || !(curr_poly->state & POLY4DV1_STATE_ACTIVE) ||
            (curr_poly->state & POLY4DV1_STATE_CLIPPED ) ||
            (curr_poly->state & POLY4DV1_STATE_BACKFACE) )
            continue; // move onto next poly

        // all good, let's transform 
        for (int vertex = 0; vertex < 3; vertex++)
        {
            float z = curr_poly->tvlist[vertex].z;

            // transform the vertex by the view parameters in the camera
            curr_poly->tvlist[vertex].x = cam->view_dist*curr_poly->tvlist[vertex].x/z;
            curr_poly->tvlist[vertex].y = cam->view_dist*curr_poly->tvlist[vertex].y*cam->aspect_ratio/z;
            // z = z, so no change

            // not that we are NOT dividing by the homogenous w coordinate since
            // we are not using a matrix operation for this version of the function 

        } // end for vertex

    } // end for poly

} // end Camera_To_Perspective_RENDERLIST4DV1

////////////////////////////////////////////////////////////////

void Camera_To_Perspective_Screen_RENDERLIST4DV1(RENDERLIST4DV1_PTR rend_list, 
                                                 CAM4DV1_PTR cam)
{
    // NOTE: this is not a matrix based function
    // this function transforms the camera coordinates of an object
    // into Screen scaled perspective coordinates, based on the 
    // sent camera object, that is, view_dist_h and view_dist_v 
    // should be set to cause the desired (viewport_width X viewport_height)
    // it only works on the vertices in the tvlist[] list
    // finally, the function also inverts the y axis, so the coordinates
    // generated from this function ARE screen coordinates and ready for
    // rendering

    // transform each polygon in the render list to perspective screen 
    // coordinates assumes the render list has already been transformed 
    // to camera coordinates and the result is in tvlist[]
    for (int poly = 0; poly < rend_list->num_polys; poly++)
    {
        // acquire current polygon
        POLYF4DV1_PTR curr_poly = rend_list->poly_ptrs[poly];

        // is this polygon valid?
        // transform this polygon if and only if it's not clipped, not culled,
        // active, and visible, note however the concept of "backface" is 
        // irrelevant in a wire frame engine though
        if ((curr_poly==NULL) || !(curr_poly->state & POLY4DV1_STATE_ACTIVE) ||
            (curr_poly->state & POLY4DV1_STATE_CLIPPED ) ||
            (curr_poly->state & POLY4DV1_STATE_BACKFACE) )
            continue; // move onto next poly

        float alpha = (0.5*cam->viewport_width-0.5);
        float beta  = (0.5*cam->viewport_height-0.5);

        // all good, let's transform 
        for (int vertex = 0; vertex < 3; vertex++)
        {
            float z = curr_poly->tvlist[vertex].z;

            // transform the vertex by the view parameters in the camera
            curr_poly->tvlist[vertex].x = cam->view_dist*curr_poly->tvlist[vertex].x/z;
            curr_poly->tvlist[vertex].y = cam->view_dist*curr_poly->tvlist[vertex].y/z;
            // z = z, so no change

            // not that we are NOT dividing by the homogenous w coordinate since
            // we are not using a matrix operation for this version of the function 

            // now the coordinates are in the range x:(-viewport_width/2 to viewport_width/2)
            // and y:(-viewport_height/2 to viewport_height/2), thus we need a translation and
            // since the y-axis is inverted, we need to invert y to complete the screen 
            // transform:
            curr_poly->tvlist[vertex].x =  curr_poly->tvlist[vertex].x + alpha; 
            curr_poly->tvlist[vertex].y = -curr_poly->tvlist[vertex].y + beta;

        } // end for vertex

    } // end for poly

} // end Camera_To_Perspective_Screen_RENDERLIST4DV1

//////////////////////////////////////////////////////////////

void Perspective_To_Screen_RENDERLIST4DV1(RENDERLIST4DV1_PTR rend_list, 
                                          CAM4DV1_PTR cam)
{
    // NOTE: this is not a matrix based function
    // this function transforms the perspective coordinates of the render
    // list into screen coordinates, based on the sent viewport in the camera
    // assuming that the viewplane coordinates were normalized
    // you would use this function instead of the object based function
    // if you decided earlier in the pipeline to turn each object into 
    // a list of polygons and then add them to the global render list
    // you would only call this function if you previously performed
    // a normalized perspective transform

    // transform each polygon in the render list from perspective to screen 
    // coordinates assumes the render list has already been transformed 
    // to normalized perspective coordinates and the result is in tvlist[]
    for (int poly = 0; poly < rend_list->num_polys; poly++)
    {
        // acquire current polygon
        POLYF4DV1_PTR curr_poly = rend_list->poly_ptrs[poly];

        // is this polygon valid?
        // transform this polygon if and only if it's not clipped, not culled,
        // active, and visible, note however the concept of "backface" is 
        // irrelevant in a wire frame engine though
        if ((curr_poly==NULL) || !(curr_poly->state & POLY4DV1_STATE_ACTIVE) ||
            (curr_poly->state & POLY4DV1_STATE_CLIPPED ) ||
            (curr_poly->state & POLY4DV1_STATE_BACKFACE) )
            continue; // move onto next poly

        float alpha = (0.5*cam->viewport_width-0.5);
        float beta  = (0.5*cam->viewport_height-0.5);

        // all good, let's transform 
        for (int vertex = 0; vertex < 3; vertex++)
        {
            // the vertex is in perspective normalized coords from -1 to 1
            // on each axis, simple scale them and invert y axis and project
            // to screen

            // transform the vertex by the view parameters in the camera
            curr_poly->tvlist[vertex].x = alpha + alpha*curr_poly->tvlist[vertex].x;
            curr_poly->tvlist[vertex].y = beta  - beta *curr_poly->tvlist[vertex].y;
        } // end for vertex

    } // end for poly

} // end Perspective_To_Screen_RENDERLIST4DV1

///////////////////////////////////////////////////////////////

void Reset_RENDERLIST4DV1(RENDERLIST4DV1_PTR rend_list)
{
    // this function intializes and resets the sent render list and
    // redies it for polygons/faces to be inserted into it
    // note that the render list in this version is composed
    // of an array FACE4DV1 pointer objects, you call this
    // function each frame

    // since we are tracking the number of polys in the
    // list via num_polys we can set it to 0
    // but later we will want a more robust scheme if
    // we generalize the linked list more and disconnect
    // it from the polygon pointer list
    rend_list->num_polys = 0; // that was hard!

}  // Reset_RENDERLIST4DV1

////////////////////////////////////////////////////////////////

void Reset_OBJECT4DV1(OBJECT4DV1_PTR obj)
{
    // this function resets the sent object and redies it for 
    // transformations, basically just resets the culled, clipped and
    // backface flags, but here's where you would add stuff
    // to ready any object for the pipeline
    // the object is valid, let's rip it apart polygon by polygon

    // reset object's culled flag
    RESET_BIT(obj->state, OBJECT4DV1_STATE_CULLED);

    // now the clipped and backface flags for the polygons 
    for (int poly = 0; poly < obj->num_polys; poly++)
    {
        // acquire polygon
        POLY4DV1_PTR curr_poly = &obj->plist[poly];

        // first is this polygon even visible?
        if (!(curr_poly->state & POLY4DV1_STATE_ACTIVE))
            continue; // move onto next poly

        // reset clipped and backface flags
        RESET_BIT(curr_poly->state, POLY4DV1_STATE_CLIPPED);
        RESET_BIT(curr_poly->state, POLY4DV1_STATE_BACKFACE);

    } // end for poly

} // end Reset_OBJECT4DV1

///////////////////////////////////////////////////////////////

int Insert_POLY4DV1_RENDERLIST4DV1(RENDERLIST4DV1_PTR rend_list, 
                                   POLY4DV1_PTR poly)
{
    // converts the sent POLY4DV1 into a FACE4DV1 and inserts it
    // into the render list

    // step 0: are we full?
    if (rend_list->num_polys >= RENDERLIST4DV1_MAX_POLYS)
        return(0);

    // step 1: copy polygon into next opening in polygon render list

    // point pointer to polygon structure
    rend_list->poly_ptrs[rend_list->num_polys] = &rend_list->poly_data[rend_list->num_polys];

    // copy fields
    rend_list->poly_data[rend_list->num_polys].state = poly->state;
    rend_list->poly_data[rend_list->num_polys].attr  = poly->attr;
    rend_list->poly_data[rend_list->num_polys].color = poly->color;

    // now copy vertices, be careful! later put a loop, but for now
    // know there are 3 vertices always!
    VECTOR4D_COPY(&rend_list->poly_data[rend_list->num_polys].tvlist[0],
        &poly->vlist[poly->vert[0]]);

    VECTOR4D_COPY(&rend_list->poly_data[rend_list->num_polys].tvlist[1],
        &poly->vlist[poly->vert[1]]);

    VECTOR4D_COPY(&rend_list->poly_data[rend_list->num_polys].tvlist[2],
        &poly->vlist[poly->vert[2]]);

    // and copy into local vertices too
    VECTOR4D_COPY(&rend_list->poly_data[rend_list->num_polys].vlist[0],
        &poly->vlist[poly->vert[0]]);

    VECTOR4D_COPY(&rend_list->poly_data[rend_list->num_polys].vlist[1],
        &poly->vlist[poly->vert[1]]);

    VECTOR4D_COPY(&rend_list->poly_data[rend_list->num_polys].vlist[2],
        &poly->vlist[poly->vert[2]]);

    // now the polygon is loaded into the next free array position, but
    // we need to fix up the links

    // test if this is the first entry
    if (rend_list->num_polys == 0)
    {
        // set pointers to null, could loop them around though to self
        rend_list->poly_data[0].next = NULL;
        rend_list->poly_data[0].prev = NULL;
    } // end if
    else
    {
        // first set this node to point to previous node and next node (null)
        rend_list->poly_data[rend_list->num_polys].next = NULL;
        rend_list->poly_data[rend_list->num_polys].prev = 
            &rend_list->poly_data[rend_list->num_polys-1];

        // now set previous node to point to this node
        rend_list->poly_data[rend_list->num_polys-1].next = 
            &rend_list->poly_data[rend_list->num_polys];
    } // end else

    // increment number of polys in list
    rend_list->num_polys++;

    // return successful insertion
    return(1);

} // end Insert_POLY4DV1_RENDERLIST4DV1

//////////////////////////////////////////////////////////////

int Insert_POLYF4DV1_RENDERLIST4DV1(RENDERLIST4DV1_PTR rend_list, 
                                    POLYF4DV1_PTR poly)
{
    // inserts the sent polyface POLYF4DV1 into the render list

    // step 0: are we full?
    if (rend_list->num_polys >= RENDERLIST4DV1_MAX_POLYS)
        return(0);

    // step 1: copy polygon into next opening in polygon render list

    // point pointer to polygon structure
    rend_list->poly_ptrs[rend_list->num_polys] = &rend_list->poly_data[rend_list->num_polys];

    // copy face right into array, thats it
    memcpy((void *)&rend_list->poly_data[rend_list->num_polys],(void *)poly, sizeof(POLYF4DV1));

    // now the polygon is loaded into the next free array position, but
    // we need to fix up the links
    // test if this is the first entry
    if (rend_list->num_polys == 0)
    {
        // set pointers to null, could loop them around though to self
        rend_list->poly_data[0].next = NULL;
        rend_list->poly_data[0].prev = NULL;
    } // end if
    else
    {
        // first set this node to point to previous node and next node (null)
        rend_list->poly_data[rend_list->num_polys].next = NULL;
        rend_list->poly_data[rend_list->num_polys].prev = 
            &rend_list->poly_data[rend_list->num_polys-1];

        // now set previous node to point to this node
        rend_list->poly_data[rend_list->num_polys-1].next = 
            &rend_list->poly_data[rend_list->num_polys];
    } // end else

    // increment number of polys in list
    rend_list->num_polys++;

    // return successful insertion
    return(1);

} // end Insert_POLYF4DV1_RENDERLIST4DV1

//////////////////////////////////////////////////////////////

int Insert_OBJECT4DV1_RENDERLIST4DV1(RENDERLIST4DV1_PTR rend_list, 
                                     OBJECT4DV1_PTR obj,
                                     int insert_local)
{
    // converts the entire object into a face list and then inserts
    // the visible, active, non-clipped, non-culled polygons into
    // the render list, also note the flag insert_local control 
    // whether or not the vlist_local or vlist_trans vertex list
    // is used, thus you can insert an object "raw" totally untranformed
    // if you set insert_local to 1, default is 0, that is you would
    // only insert an object after at least the local to world transform

    // is this objective inactive or culled or invisible?
    if (!(obj->state & OBJECT4DV1_STATE_ACTIVE) ||
        (obj->state & OBJECT4DV1_STATE_CULLED) ||
        !(obj->state & OBJECT4DV1_STATE_VISIBLE))
        return(0); 

    // the object is valid, let's rip it apart polygon by polygon
    for (int poly = 0; poly < obj->num_polys; poly++)
    {
        // acquire polygon
        POLY4DV1_PTR curr_poly = &obj->plist[poly];

        // first is this polygon even visible?
        if (!(curr_poly->state & POLY4DV1_STATE_ACTIVE) ||
            (curr_poly->state & POLY4DV1_STATE_CLIPPED ) ||
            (curr_poly->state & POLY4DV1_STATE_BACKFACE) )
            continue; // move onto next poly

        // override vertex list polygon refers to
        // the case that you want the local coords used
        // first save old pointer
        POINT4D_PTR vlist_old = curr_poly->vlist;

        if (insert_local)
            curr_poly->vlist = obj->vlist_local;
        else
            curr_poly->vlist = obj->vlist_trans;

        // now insert this polygon
        if (!Insert_POLY4DV1_RENDERLIST4DV1(rend_list, curr_poly))
        {
            // fix vertex list pointer
            curr_poly->vlist = vlist_old;

            // the whole object didn't fit!
            return(0);
        } // end if

        // fix vertex list pointer
        curr_poly->vlist = vlist_old;

    } // end for

    // return success
    return(1);

} // end Insert_OBJECT4DV1_RENDERLIST4DV1

//////////////////////////////////////////////////////////////

void Draw_OBJECT4DV1_Wire(OBJECT4DV1_PTR obj, 
                          UCHAR *video_buffer, int lpitch)

{
    // this function renders an object to the screen in wireframe, 
    // 8 bit mode, it has no regard at all about hidden surface removal, 
    // etc. the function only exists as an easy way to render an object 
    // without converting it into polygons, the function assumes all 
    // coordinates are screen coordinates, but will perform 2D clipping

    // iterate thru the poly list of the object and simply draw
    // each polygon
    for (int poly=0; poly < obj->num_polys; poly++)
    {
        // render this polygon if and only if it's not clipped, not culled,
        // active, and visible, note however the concecpt of "backface" is 
        // irrelevant in a wire frame engine though
        if (!(obj->plist[poly].state & POLY4DV1_STATE_ACTIVE) ||
            (obj->plist[poly].state & POLY4DV1_STATE_CLIPPED ) ||
            (obj->plist[poly].state & POLY4DV1_STATE_BACKFACE) )
            continue; // move onto next poly

        // extract vertex indices into master list, rember the polygons are 
        // NOT self contained, but based on the vertex list stored in the object
        // itself
        int vindex_0 = obj->plist[poly].vert[0];
        int vindex_1 = obj->plist[poly].vert[1];
        int vindex_2 = obj->plist[poly].vert[2];

        // draw the lines now
        Draw_Clip_Line(obj->vlist_trans[ vindex_0 ].x, obj->vlist_trans[ vindex_0 ].y, 
            obj->vlist_trans[ vindex_1 ].x, obj->vlist_trans[ vindex_1 ].y, 
            obj->plist[poly].color,
            video_buffer, lpitch);

        Draw_Clip_Line(obj->vlist_trans[ vindex_1 ].x, obj->vlist_trans[ vindex_1 ].y, 
            obj->vlist_trans[ vindex_2 ].x, obj->vlist_trans[ vindex_2 ].y, 
            obj->plist[poly].color,
            video_buffer, lpitch);

        Draw_Clip_Line(obj->vlist_trans[ vindex_2 ].x, obj->vlist_trans[ vindex_2 ].y, 
            obj->vlist_trans[ vindex_0 ].x, obj->vlist_trans[ vindex_0 ].y, 
            obj->plist[poly].color,
            video_buffer, lpitch);

        // track rendering stats
#ifdef DEBUG_ON
        debug_polys_rendered_per_frame++;
#endif

    } // end for poly

} // end Draw_OBJECT4DV1_Wire

///////////////////////////////////////////////////////////////

void Draw_RENDERLIST4DV1_Wire(RENDERLIST4DV1_PTR rend_list, 
                              UCHAR *video_buffer, int lpitch)
{
    // this function "executes" the render list or in other words
    // draws all the faces in the list in wire frame 8bit mode
    // note there is no need to sort wire frame polygons, but 
    // later we will need to, so hidden surfaces stay hidden
    // also, we leave it to the function to determine the bitdepth
    // and call the correct rasterizer

    // at this point, all we have is a list of polygons and it's time
    // to draw them
    for (int poly=0; poly < rend_list->num_polys; poly++)
    {
        // render this polygon if and only if it's not clipped, not culled,
        // active, and visible, note however the concecpt of "backface" is 
        // irrelevant in a wire frame engine though
        if (!(rend_list->poly_ptrs[poly]->state & POLY4DV1_STATE_ACTIVE) ||
            (rend_list->poly_ptrs[poly]->state & POLY4DV1_STATE_CLIPPED ) ||
            (rend_list->poly_ptrs[poly]->state & POLY4DV1_STATE_BACKFACE) )
            continue; // move onto next poly

        // draw the triangle edge one, note that clipping was already set up
        // by 2D initialization, so line clipper will clip all polys out
        // of the 2D screen/window boundary
        Draw_Clip_Line(rend_list->poly_ptrs[poly]->tvlist[0].x, 
            rend_list->poly_ptrs[poly]->tvlist[0].y,
            rend_list->poly_ptrs[poly]->tvlist[1].x, 
            rend_list->poly_ptrs[poly]->tvlist[1].y,
            rend_list->poly_ptrs[poly]->color,
            video_buffer, lpitch);

        Draw_Clip_Line(rend_list->poly_ptrs[poly]->tvlist[1].x, 
            rend_list->poly_ptrs[poly]->tvlist[1].y,
            rend_list->poly_ptrs[poly]->tvlist[2].x, 
            rend_list->poly_ptrs[poly]->tvlist[2].y,
            rend_list->poly_ptrs[poly]->color,
            video_buffer, lpitch);

        Draw_Clip_Line(rend_list->poly_ptrs[poly]->tvlist[2].x, 
            rend_list->poly_ptrs[poly]->tvlist[2].y,
            rend_list->poly_ptrs[poly]->tvlist[0].x, 
            rend_list->poly_ptrs[poly]->tvlist[0].y,
            rend_list->poly_ptrs[poly]->color,
            video_buffer, lpitch);
        // track rendering stats
#ifdef DEBUG_ON
        debug_polys_rendered_per_frame++;
#endif

    } // end for poly

} // end Draw_RENDERLIST4DV1_Wire

/////////////////////////////////////////////////////////////

void Draw_OBJECT4DV1_Wire16(OBJECT4DV1_PTR obj, 
                            UCHAR *video_buffer, int lpitch)

{
    // this function renders an object to the screen in wireframe, 
    // 16 bit mode, it has no regard at all about hidden surface removal, 
    // etc. the function only exists as an easy way to render an object 
    // without converting it into polygons, the function assumes all 
    // coordinates are screen coordinates, but will perform 2D clipping

    // iterate thru the poly list of the object and simply draw
    // each polygon
    for (int poly=0; poly < obj->num_polys; poly++)
    {
        // render this polygon if and only if it's not clipped, not culled,
        // active, and visible, note however the concecpt of "backface" is 
        // irrelevant in a wire frame engine though
        if (!(obj->plist[poly].state & POLY4DV1_STATE_ACTIVE) ||
            (obj->plist[poly].state & POLY4DV1_STATE_CLIPPED ) ||
            (obj->plist[poly].state & POLY4DV1_STATE_BACKFACE) )
            continue; // move onto next poly

        // extract vertex indices into master list, rember the polygons are 
        // NOT self contained, but based on the vertex list stored in the object
        // itself
        int vindex_0 = obj->plist[poly].vert[0];
        int vindex_1 = obj->plist[poly].vert[1];
        int vindex_2 = obj->plist[poly].vert[2];

        // draw the lines now
        Draw_Clip_Line16(obj->vlist_trans[ vindex_0 ].x, obj->vlist_trans[ vindex_0 ].y, 
            obj->vlist_trans[ vindex_1 ].x, obj->vlist_trans[ vindex_1 ].y, 
            obj->plist[poly].color,
            video_buffer, lpitch);

        Draw_Clip_Line16(obj->vlist_trans[ vindex_1 ].x, obj->vlist_trans[ vindex_1 ].y, 
            obj->vlist_trans[ vindex_2 ].x, obj->vlist_trans[ vindex_2 ].y, 
            obj->plist[poly].color,
            video_buffer, lpitch);

        Draw_Clip_Line16(obj->vlist_trans[ vindex_2 ].x, obj->vlist_trans[ vindex_2 ].y, 
            obj->vlist_trans[ vindex_0 ].x, obj->vlist_trans[ vindex_0 ].y, 
            obj->plist[poly].color,
            video_buffer, lpitch);

        // track rendering stats
#ifdef DEBUG_ON
        debug_polys_rendered_per_frame++;
#endif


    } // end for poly

} // end Draw_OBJECT4DV1_Wire16

///////////////////////////////////////////////////////////////

void Draw_RENDERLIST4DV1_Wire16(RENDERLIST4DV1_PTR rend_list, 
                                UCHAR *video_buffer, int lpitch)
{
    // this function "executes" the render list or in other words
    // draws all the faces in the list in wire frame 16bit mode
    // note there is no need to sort wire frame polygons, but 
    // later we will need to, so hidden surfaces stay hidden
    // also, we leave it to the function to determine the bitdepth
    // and call the correct rasterizer

    // at this point, all we have is a list of polygons and it's time
    // to draw them
    for (int poly=0; poly < rend_list->num_polys; poly++)
    {
        // render this polygon if and only if it's not clipped, not culled,
        // active, and visible, note however the concecpt of "backface" is 
        // irrelevant in a wire frame engine though
        if (!(rend_list->poly_ptrs[poly]->state & POLY4DV1_STATE_ACTIVE) ||
            (rend_list->poly_ptrs[poly]->state & POLY4DV1_STATE_CLIPPED ) ||
            (rend_list->poly_ptrs[poly]->state & POLY4DV1_STATE_BACKFACE) )
            continue; // move onto next poly

        // draw the triangle edge one, note that clipping was already set up
        // by 2D initialization, so line clipper will clip all polys out
        // of the 2D screen/window boundary
        Draw_Clip_Line16(rend_list->poly_ptrs[poly]->tvlist[0].x, 
            rend_list->poly_ptrs[poly]->tvlist[0].y,
            rend_list->poly_ptrs[poly]->tvlist[1].x, 
            rend_list->poly_ptrs[poly]->tvlist[1].y,
            rend_list->poly_ptrs[poly]->color,
            video_buffer, lpitch);

        Draw_Clip_Line16(rend_list->poly_ptrs[poly]->tvlist[1].x, 
            rend_list->poly_ptrs[poly]->tvlist[1].y,
            rend_list->poly_ptrs[poly]->tvlist[2].x, 
            rend_list->poly_ptrs[poly]->tvlist[2].y,
            rend_list->poly_ptrs[poly]->color,
            video_buffer, lpitch);

        Draw_Clip_Line16(rend_list->poly_ptrs[poly]->tvlist[2].x, 
            rend_list->poly_ptrs[poly]->tvlist[2].y,
            rend_list->poly_ptrs[poly]->tvlist[0].x, 
            rend_list->poly_ptrs[poly]->tvlist[0].y,
            rend_list->poly_ptrs[poly]->color,
            video_buffer, lpitch);

        // track rendering stats
#ifdef DEBUG_ON
        debug_polys_rendered_per_frame++;
#endif

    } // end for poly

} // end Draw_RENDERLIST4DV1_Wire

/////////////////////////////////////////////////////////////

void Build_CAM4DV1_Matrix_Euler(CAM4DV1_PTR cam, int cam_rot_seq)
{
    // this creates a camera matrix based on Euler angles 
    // and stores it in the sent camera object
    // if you recall from chapter 6 to create the camera matrix
    // we need to create a transformation matrix that looks like:

    // Mcam = mt(-1) * my(-1) * mx(-1) * mz(-1)
    // that is the inverse of the camera translation matrix mutilplied
    // by the inverses of yxz, in that order, however, the order of
    // the rotation matrices is really up to you, so we aren't going
    // to force any order, thus its programmable based on the value
    // of cam_rot_seq which can be any value CAM_ROT_SEQ_XYZ where 
    // XYZ can be in any order, YXZ, ZXY, etc.

    MATRIX4X4 mt_inv,  // inverse camera translation matrix
        mx_inv,  // inverse camera x axis rotation matrix
        my_inv,  // inverse camera y axis rotation matrix
        mz_inv,  // inverse camera z axis rotation matrix
        mrot,    // concatenated inverse rotation matrices
        mtmp;    // temporary working matrix


    // step 1: create the inverse translation matrix for the camera
    // position
    Mat_Init_4X4(&mt_inv, 1,    0,     0,     0,
        0,    1,     0,     0,
        0,    0,     1,     0,
        -cam->pos.x, -cam->pos.y, -cam->pos.z, 1);

    // step 2: create the inverse rotation sequence for the camera
    // rember either the transpose of the normal rotation matrix or
    // plugging negative values into each of the rotations will result
    // in an inverse matrix

    // first compute all 3 rotation matrices

    // extract out euler angles
    float theta_x = cam->dir.x;
    float theta_y = cam->dir.y;
    float theta_z = cam->dir.z;

    // compute the sine and cosine of the angle x
    float cos_theta = Fast_Cos(theta_x);  // no change since cos(-x) = cos(x)
    float sin_theta = -Fast_Sin(theta_x); // sin(-x) = -sin(x)

    // set the matrix up 
    Mat_Init_4X4(&mx_inv, 1,    0,         0,         0,
        0,    cos_theta, sin_theta, 0,
        0,   -sin_theta, cos_theta, 0,
        0,    0,         0,         1);

    // compute the sine and cosine of the angle y
    cos_theta = Fast_Cos(theta_y);  // no change since cos(-x) = cos(x)
    sin_theta = -Fast_Sin(theta_y); // sin(-x) = -sin(x)

    // set the matrix up 
    Mat_Init_4X4(&my_inv,cos_theta, 0, -sin_theta, 0,  
        0,         1,  0,         0,
        sin_theta, 0,  cos_theta,  0,
        0,         0,  0,          1);

    // compute the sine and cosine of the angle z
    cos_theta = Fast_Cos(theta_z);  // no change since cos(-x) = cos(x)
    sin_theta = -Fast_Sin(theta_z); // sin(-x) = -sin(x)

    // set the matrix up 
    Mat_Init_4X4(&mz_inv, cos_theta, sin_theta, 0, 0,  
        -sin_theta, cos_theta, 0, 0,
        0,         0,         1, 0,
        0,         0,         0, 1);

    // now compute inverse camera rotation sequence
    switch(cam_rot_seq)
    {
    case CAM_ROT_SEQ_XYZ:
        {
            Mat_Mul_4X4(&mx_inv, &my_inv, &mtmp);
            Mat_Mul_4X4(&mtmp, &mz_inv, &mrot);
        } break;

    case CAM_ROT_SEQ_YXZ:
        {
            Mat_Mul_4X4(&my_inv, &mx_inv, &mtmp);
            Mat_Mul_4X4(&mtmp, &mz_inv, &mrot);
        } break;

    case CAM_ROT_SEQ_XZY:
        {
            Mat_Mul_4X4(&mx_inv, &mz_inv, &mtmp);
            Mat_Mul_4X4(&mtmp, &my_inv, &mrot);
        } break;

    case CAM_ROT_SEQ_YZX:
        {
            Mat_Mul_4X4(&my_inv, &mz_inv, &mtmp);
            Mat_Mul_4X4(&mtmp, &mx_inv, &mrot);
        } break;

    case CAM_ROT_SEQ_ZYX:
        {
            Mat_Mul_4X4(&mz_inv, &my_inv, &mtmp);
            Mat_Mul_4X4(&mtmp, &mx_inv, &mrot);
        } break;

    case CAM_ROT_SEQ_ZXY:
        {
            Mat_Mul_4X4(&mz_inv, &mx_inv, &mtmp);
            Mat_Mul_4X4(&mtmp, &my_inv, &mrot);

        } break;

    default: break;
    } // end switch

    // now mrot holds the concatenated product of inverse rotation matrices
    // multiply the inverse translation matrix against it and store in the 
    // camera objects' camera transform matrix we are done!
    Mat_Mul_4X4(&mt_inv, &mrot, &cam->mcam);

} // end Build_CAM4DV1_Matrix_Euler

/////////////////////////////////////////////////////////////

void Build_CAM4DV1_Matrix_UVN(CAM4DV1_PTR cam, int mode)
{
    // this creates a camera matrix based on a look at vector n,
    // look up vector v, and a look right (or left) u
    // and stores it in the sent camera object, all values are
    // extracted out of the camera object itself
    // mode selects how uvn is computed
    // UVN_MODE_SIMPLE - low level simple model, use the target and view reference point
    // UVN_MODE_SPHERICAL - spherical mode, the x,y components will be used as the
    //     elevation and heading of the view vector respectively
    //     along with the view reference point as the position
    //     as usual

    MATRIX4X4 mt_inv,  // inverse camera translation matrix
        mt_uvn,  // the final uvn matrix
        mtmp;    // temporary working matrix

    // step 1: create the inverse translation matrix for the camera
    // position
    Mat_Init_4X4(&mt_inv, 1,    0,     0,     0,
        0,    1,     0,     0,
        0,    0,     1,     0,
        -cam->pos.x, -cam->pos.y, -cam->pos.z, 1);


    // step 2: determine how the target point will be computed
    if (mode == UVN_MODE_SPHERICAL)
    {
        // use spherical construction
        // target needs to be recomputed

        // extract elevation and heading 
        float phi   = cam->dir.x; // elevation
        float theta = cam->dir.y; // heading

        // compute trig functions once
        float sin_phi = Fast_Sin(phi);
        float cos_phi = Fast_Cos(phi);

        float sin_theta = Fast_Sin(theta);
        float cos_theta = Fast_Cos(theta);

        // now compute the target point on a unit sphere x,y,z
        cam->target.x = -1*sin_phi*sin_theta;
        cam->target.y =  1*cos_phi;
        cam->target.z =  1*sin_phi*cos_theta;
    } // end else

    // at this point, we have the view reference point, the target and that's
    // all we need to recompute u,v,n
    // Step 1: n = <target position - view reference point>
    VECTOR4D_Build(&cam->pos, &cam->target, &cam->n);

    // Step 2: Let v = <0,1,0>
    VECTOR4D_INITXYZ(&cam->v,0,1,0);

    // Step 3: u = (v x n)
    VECTOR4D_Cross(&cam->v,&cam->n,&cam->u);

    // Step 4: v = (n x u)
    VECTOR4D_Cross(&cam->n,&cam->u,&cam->v);

    // Step 5: normalize all vectors
    VECTOR4D_Normalize(&cam->u);
    VECTOR4D_Normalize(&cam->v);
    VECTOR4D_Normalize(&cam->n);


    // build the UVN matrix by placing u,v,n as the columns of the matrix
    Mat_Init_4X4(&mt_uvn, cam->u.x,    cam->v.x,     cam->n.x,     0,
        cam->u.y,    cam->v.y,     cam->n.y,     0,
        cam->u.z,    cam->v.z,     cam->n.z,     0,
        0,           0,            0,            1);

    // now multiply the translation matrix and the uvn matrix and store in the 
    // final camera matrix mcam
    Mat_Mul_4X4(&mt_inv, &mt_uvn, &cam->mcam);

} // end Build_CAM4DV1_Matrix_UVN

/////////////////////////////////////////////////////////////

void Init_CAM4DV1(CAM4DV1_PTR cam,       // the camera object
                  int cam_attr,          // attributes
                  POINT4D_PTR cam_pos,   // initial camera position
                  VECTOR4D_PTR cam_dir,  // initial camera angles
                  POINT4D_PTR cam_target, // UVN target
                  float near_clip_z,     // near and far clipping planes
                  float far_clip_z,
                  float fov,             // field of view in degrees
                  float viewport_width,  // size of final screen viewport
                  float viewport_height)
{
    // this function initializes the camera object cam, the function
    // doesn't do a lot of error checking or sanity checking since 
    // I want to allow you to create projections as you wish, also 
    // I tried to minimize the number of parameters the functions needs

    // first set up parms that are no brainers
    cam->attr = cam_attr;              // camera attributes

    VECTOR4D_COPY(&cam->pos, cam_pos); // positions
    VECTOR4D_COPY(&cam->dir, cam_dir); // direction vector or angles for
    // euler camera
    // for UVN camera
    VECTOR4D_INITXYZ(&cam->u, 1,0,0);  // set to +x
    VECTOR4D_INITXYZ(&cam->v, 0,1,0);  // set to +y
    VECTOR4D_INITXYZ(&cam->n, 0,0,1);  // set to +z        

    if (cam_target!=NULL)
        VECTOR4D_COPY(&cam->target, cam_target); // UVN target
    else
        VECTOR4D_ZERO(&cam->target);

    cam->near_clip_z = near_clip_z;     // near z=constant clipping plane
    cam->far_clip_z  = far_clip_z;      // far z=constant clipping plane

    cam->viewport_width  = viewport_width;   // dimensions of viewport
    cam->viewport_height = viewport_height;

    cam->viewport_center_x = (viewport_width-1)/2; // center of viewport
    cam->viewport_center_y = (viewport_height-1)/2;

    cam->aspect_ratio = (float)viewport_width/(float)viewport_height;

    // set all camera matrices to identity matrix
    MAT_IDENTITY_4X4(&cam->mcam);
    MAT_IDENTITY_4X4(&cam->mper);
    MAT_IDENTITY_4X4(&cam->mscr);

    // set independent vars
    cam->fov              = fov;

    // set the viewplane dimensions up, they will be 2 x (2/ar)
    cam->viewplane_width  = 2.0;
    cam->viewplane_height = 2.0/cam->aspect_ratio;

    // now we know fov and we know the viewplane dimensions plug into formula and
    // solve for view distance parameters
    float tan_fov_div2 = tan(DEG_TO_RAD(fov/2));

    cam->view_dist = (0.5)*(cam->viewplane_width)*tan_fov_div2;

    // test for 90 fov first since it's easy :)
    if (fov == 90.0)
    {
        // set up the clipping planes -- easy for 90 degrees!
        POINT3D pt_origin; // point on the plane
        VECTOR3D_INITXYZ(&pt_origin,0,0,0);

        VECTOR3D vn; // normal to plane

        // right clipping plane 
        VECTOR3D_INITXYZ(&vn,1,0,-1); // x=z plane
        PLANE3D_Init(&cam->rt_clip_plane, &pt_origin,  &vn, 1);

        // left clipping plane
        VECTOR3D_INITXYZ(&vn,-1,0,-1); // -x=z plane
        PLANE3D_Init(&cam->lt_clip_plane, &pt_origin,  &vn, 1);

        // top clipping plane
        VECTOR3D_INITXYZ(&vn,0,1,-1); // y=z plane
        PLANE3D_Init(&cam->tp_clip_plane, &pt_origin,  &vn, 1);

        // bottom clipping plane
        VECTOR3D_INITXYZ(&vn,0,-1,-1); // -y=z plane
        PLANE3D_Init(&cam->bt_clip_plane, &pt_origin,  &vn, 1);
    } // end if d=1
    else 
    {
        // now compute clipping planes yuck!
        POINT3D pt_origin; // point on the plane
        VECTOR3D_INITXYZ(&pt_origin,0,0,0);

        VECTOR3D vn; // normal to plane

        // since we don't have a 90 fov, computing the normals
        // are a bit tricky, there are a number of geometric constructions
        // that solve the problem, but I'm going to solve for the
        // vectors that represent the 2D projections of the frustrum planes
        // on the x-z and y-z planes and then find perpendiculars to them

        // right clipping plane, check the math on graph paper 
        VECTOR3D_INITXYZ(&vn,cam->view_dist,0,-cam->viewplane_width/2.0); 
        PLANE3D_Init(&cam->rt_clip_plane, &pt_origin,  &vn, 1);

        // left clipping plane, we can simply reflect the right normal about
        // the z axis since the planes are symetric about the z axis
        // thus invert x only
        VECTOR3D_INITXYZ(&vn,-cam->view_dist,0,-cam->viewplane_width/2.0); 
        PLANE3D_Init(&cam->lt_clip_plane, &pt_origin,  &vn, 1);

        // top clipping plane, same construction
        VECTOR3D_INITXYZ(&vn,0,cam->view_dist,-cam->viewplane_width/2.0); 
        PLANE3D_Init(&cam->tp_clip_plane, &pt_origin,  &vn, 1);

        // bottom clipping plane, same inversion
        VECTOR3D_INITXYZ(&vn,0,-cam->view_dist,-cam->viewplane_width/2.0); 
        PLANE3D_Init(&cam->bt_clip_plane, &pt_origin,  &vn, 1);
    } // end else

} // end Init_CAM4DV1

// MAIN //////////////////////////////////////////////////////

#if (STANDALONE==1)
void main()
{

    static VECTOR4D vscale={1,1,1,1}, vpos = {0,0,0,1}, vrot = {0,0,0,1};
    static OBJECT4DV1 obj;
    static CAM4DV1 cam;
    static RENDERLIST4DV1 rend_list;
    static POLYF4DV1 poly1;


    Open_Error_File("ERROR.TXT", stdout);

    // set function pointer to something
    RGB16Bit = RGB16Bit565;

    // initialize math engine
    Build_Sin_Cos_Tables();

    // initialize the renderlist
    Init_RENDERLIST4DV1(&rend_list);

    // initialize single polygon
    poly1.state  = POLY4DV1_STATE_ACTIVE;
    poly1.attr   =  0; 
    poly1.color = RGB16Bit(255,255,255);

    poly1.vlist[0].x = 0;
    poly1.vlist[0].y = 50;
    poly1.vlist[0].z = 100;
    poly1.vlist[0].w = 1;

    poly1.vlist[1].x = 50;
    poly1.vlist[1].y = 0;
    poly1.vlist[1].z = 100;
    poly1.vlist[1].w = 1;

    poly1.vlist[2].x = -50;
    poly1.vlist[2].y = 50;
    poly1.vlist[2].z = 100;
    poly1.vlist[2].w = 1;

    poly1.tvlist[0].x = 0;
    poly1.tvlist[0].y = 50;
    poly1.tvlist[0].z = 100;
    poly1.tvlist[0].w = 1;

    poly1.tvlist[1].x = 50;
    poly1.tvlist[1].y = 0;
    poly1.tvlist[1].z = 100;
    poly1.tvlist[1].w = 1;

    poly1.tvlist[2].x = -50;
    poly1.tvlist[2].y = 50;
    poly1.tvlist[2].z = 100;
    poly1.tvlist[2].w = 1;

    poly1.next = poly1.prev = NULL;

    // insert the polygon in the renderlist
    Insert_POLYF4DV1_RENDERLIST4DV1(&rend_list, &poly1);

    Write_Error("\nPolygon Initial");
    VECTOR4D_Print(&rend_list.poly_ptrs[0]->tvlist[0], "v0");
    VECTOR4D_Print(&rend_list.poly_ptrs[0]->tvlist[1], "v1");
    VECTOR4D_Print(&rend_list.poly_ptrs[0]->tvlist[2], "v2");


    // initialize camera 
    static POINT4D cam_pos = {0,0,-500};
    static VECTOR4D cam_dir = {0,0,0};

    Init_CAM4DV1(&cam,      // the camera object
        &cam_pos,  // initial camera position
        &cam_dir,  // initial camera angles
        10.0,      // near and far clipping planes
        1000.0,
        90.0,      // field of view in degrees
        1.0,       // viewing distance
        0.0,       // size of viewplane
        0.0,
        640,   // size of final screen viewport
        480,
        ((float)640/(float)480));

    // generate camera matrix
    Build_CAM4DV1_Matrix_Euler(&cam, CAM_ROT_SEQ_XYZ);

    // apply world to camera transformation
    Camera_To_Perspective_Norm_RENDERLIST4DV1(&rend_list, &cam);

    Write_Error("\nPolygon Perspective");
    VECTOR4D_Print(&rend_list.poly_ptrs[0]->tvlist[0], "v0");
    VECTOR4D_Print(&rend_list.poly_ptrs[0]->tvlist[1], "v1");
    VECTOR4D_Print(&rend_list.poly_ptrs[0]->tvlist[2], "v2");

    // apply screen transform
    Perspective_Norm_To_Screen_RENDERLIST4DV1(&rend_list, &cam);

    Write_Error("\nPolygon Screen");
    VECTOR4D_Print(&rend_list.poly_ptrs[0]->tvlist[0], "v0");
    VECTOR4D_Print(&rend_list.poly_ptrs[0]->tvlist[1], "v1");
    VECTOR4D_Print(&rend_list.poly_ptrs[0]->tvlist[2], "v2");


    //Load_OBJECT4DV1_PLG(&obj, "test.plg",vscale, vpos, vrot);

    Close_Error_File();

} // end main
#endif