// T3DLIB11.CPP -

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


// DEFINES //////////////////////////////////////////////////////////////////



// GLOBALS //////////////////////////////////////////////////////////////////

// dot product look up table
float dp_inverse_cos[360+2]; // 0 to 180 in .5 degree steps
// the +2 for padding

int bhv_nodes_visited;       // used to track how many nodes where visited

// MACROS ////////////////////////////////////////////////

// FUNCTIONS ////////////////////////////////////////////////////////////////


void Intersect_Lines(float x0,float y0,float x1,float y1,
                     float x2,float y2,float x3,float y3,
                     float *xi,float *yi)
{
    // this function computes the intersection of the sent lines
    // and returns the intersection point, note that the function assumes
    // the lines intersect. the function can handle vertical as well
    // as horizontal lines. note the function isn't very clever, it simply applies
    // the math, but we don't need speed since this is a pre-processing step
    // we could have used parametric lines if we wanted, but this is more straight
    // forward for this use, I want the intersection of 2 infinite lines
    // rather than line segments as the parametric system defines

    float a1,b1,c1, // constants of linear equations
        a2,b2,c2,
        det_inv,  // the inverse of the determinant of the coefficient matrix
        m1,m2;    // the slopes of each line

    // compute slopes, note the cludge for infinity, however, this will
    // be close enough
    if ((x1-x0)!=0)
        m1 = (y1-y0) / (x1-x0);
    else
        m1 = (float)1.0E+20;   // close enough to infinity

    if ((x3-x2)!=0)
        m2 = (y3-y2) / (x3-x2);
    else
        m2 = (float)1.0E+20;   // close enough to infinity

    // compute constants 
    a1 = m1;
    a2 = m2;

    b1 = -1;
    b2 = -1;

    c1 = (y0-m1*x0);
    c2 = (y2-m2*x2);

    // compute the inverse of the determinate
    det_inv = 1 / (a1*b2 - a2*b1);

    // use cramers rule to compute xi and yi
    *xi=((b1*c2 - b2*c1)*det_inv);
    *yi=((a2*c1 - a1*c2)*det_inv);

} // end Intersect_Lines

/////////////////////////////////////////////////////////////////////////////

void Bsp_Build_Tree(BSPNODEV1_PTR root)
{
    // this function recursively builds the bsp tree from the root of the wall list
    // the function works by taking the wall list which begins as a linked list
    // of walls in no particular order, then it uses the wall at the TOP of the list
    // as the separating plane and then divides space with that wall computing
    // the walls on the front and back of the separating plane. The walls that are
    // determined to be on the front and back, are once again linked lists, that 
    // are each processed recursively in the same manner. When the process is 
    // complete the entire BSP tree is built with exactly one node/leaf per wall

    static BSPNODEV1_PTR next_wall,  // pointer to next wall to be processed
        front_wall,    // the front wall
        back_wall,     // the back wall
        temp_wall;     // a temporary wall

    static float dot_wall_1,              // dot products for test wall
        dot_wall_2,
        wall_x0,wall_y0,wall_z0, // working vars for test wall
        wall_x1,wall_y1,wall_z1,
        pp_x0,pp_y0,pp_z0,       // working vars for partitioning plane
        pp_x1,pp_y1,pp_z1,
        xi,zi;                   // points of intersection when the partioning
    // plane cuts a wall in two

    static VECTOR4D test_vector_1,  // test vectors from the partioning plane
        test_vector_2;  // to the test wall to test the side
    // of the partioning plane the test wall
    // lies on

    static int front_flag = 0,      // flags if a wall is on the front or back
        back_flag  = 0,      // of the partioning plane
        index;               // looping index

    // SECTION 1 ////////////////////////////////////////////////////////////////

    // test if this tree is complete
    if (root==NULL)
        return;

    // the root is the partitioning plane, partition the polygons using it
    next_wall  = root->link;
    root->link = NULL;

    // extract top two vertices of partioning plane wall for ease of calculations
    pp_x0 = root->wall.vlist[0].x;
    pp_y0 = root->wall.vlist[0].y;
    pp_z0 = root->wall.vlist[0].z;

    pp_x1 = root->wall.vlist[1].x;
    pp_y1 = root->wall.vlist[1].y;
    pp_z1 = root->wall.vlist[1].z;

    // SECTION 2  ////////////////////////////////////////////////////////////////

    // test if all walls have been partitioned
    while(next_wall)
    {
        // test which side test wall is relative to partioning plane
        // defined by root

        // first compute vectors from point on partioning plane to points on
        // test wall
        VECTOR4D_Build(&root->wall.vlist[0].v, 
            &next_wall->wall.vlist[0].v, 
            &test_vector_1);

        VECTOR4D_Build(&root->wall.vlist[0].v,
            &next_wall->wall.vlist[1].v,
            &test_vector_2);

        // now dot each test vector with the surface normal and analyze signs
        // to determine half space
        dot_wall_1 = VECTOR4D_Dot(&test_vector_1, &root->wall.normal);
        dot_wall_2 = VECTOR4D_Dot(&test_vector_2, &root->wall.normal);

        // SECTION 3  ////////////////////////////////////////////////////////////////

        // perform the tests

        // case 0, the partioning plane and the test wall have a point in common
        // this is a special case and must be accounted for, shorten the code
        // we will set a pair of flags and then the next case will handle
        // the actual insertion of the wall into BSP

        // reset flags
        front_flag = back_flag = 0;

        // determine if wall is tangent to endpoints of partitioning wall
        if (VECTOR4D_Equal(&root->wall.vlist[0].v ,&next_wall->wall.vlist[0].v) )
        {
            // p0 of partioning plane is the same at p0 of test wall
            // we only need to see what side p1 of test wall in on
            if (dot_wall_2 > 0)
                front_flag = 1;
            else
                back_flag = 1;

        } // end if
        else
            if (VECTOR4D_Equal(&root->wall.vlist[0].v, &next_wall->wall.vlist[1].v) )
            {
                // p0 of partioning plane is the same at p1 of test wall
                // we only need to see what side p0 of test wall in on
                if (dot_wall_1 > 0)
                    front_flag = 1;
                else
                    back_flag = 1;

            } // end if
            else
                if (VECTOR4D_Equal(&root->wall.vlist[1].v, &next_wall->wall.vlist[0].v) )
                {
                    // p1 of partioning plane is the same at p0 of test wall
                    // we only need to see what side p1 of test wall in on

                    if (dot_wall_2 > 0)
                        front_flag = 1;
                    else
                        back_flag = 1;

                } // end if
                else
                    if (VECTOR4D_Equal(&root->wall.vlist[1].v, &next_wall->wall.vlist[1].v) )
                    {
                        // p1 of partioning plane is the same at p1 of test wall
                        // we only need to see what side p0 of test wall in on

                        if (dot_wall_1 > 0)
                            front_flag = 1;
                        else
                            back_flag = 1;

                    } // end if

                    // SECTION 4  ////////////////////////////////////////////////////////////////

                    // case 1 both signs are the same or the front or back flag has been set
                    if ( (dot_wall_1 >= 0 && dot_wall_2 >= 0) || front_flag )
                    {
                        // place this wall on the front list
                        if (root->front==NULL)
                        {
                            // this is the first node
                            root->front      = next_wall;
                            next_wall        = next_wall->link;
                            front_wall       = root->front;
                            front_wall->link = NULL;

                        } // end if
                        else
                        {
                            // this is the nth node
                            front_wall->link = next_wall;
                            next_wall        = next_wall->link;
                            front_wall       = front_wall->link;
                            front_wall->link = NULL;

                        } // end else

                    } // end if both positive

                    // SECTION  5 ////////////////////////////////////////////////////////////////

                    else // back side sub-case
                        if ( (dot_wall_1 < 0 && dot_wall_2 < 0) || back_flag)
                        {
                            // place this wall on the back list
                            if (root->back==NULL)
                            {
                                // this is the first node
                                root->back      = next_wall;
                                next_wall       = next_wall->link;
                                back_wall       = root->back;
                                back_wall->link = NULL;

                            } // end if
                            else
                            {
                                // this is the nth node
                                back_wall->link = next_wall;
                                next_wall       = next_wall->link;
                                back_wall       = back_wall->link;
                                back_wall->link = NULL;

                            } // end else

                        } // end if both negative

                        // case 2 both signs are different, we must partition the wall

                        // SECTION 6  ////////////////////////////////////////////////////////////////

                        else
                            if ( (dot_wall_1 < 0 && dot_wall_2 >= 0) ||
                                (dot_wall_1 >= 0 && dot_wall_2 < 0))
                            {
                                // the partioning plane cuts the wall in half, the wall
                                // must be split into two walls

                                // extract top two vertices of test wall for ease of calculations
                                wall_x0 = next_wall->wall.vlist[0].x;
                                wall_y0 = next_wall->wall.vlist[0].y;
                                wall_z0 = next_wall->wall.vlist[0].z;

                                wall_x1 = next_wall->wall.vlist[1].x;
                                wall_y1 = next_wall->wall.vlist[1].y;
                                wall_z1 = next_wall->wall.vlist[1].z;

                                // compute the point of intersection between the walls
                                // note that the intersection takes place in the x-z plane 
                                Intersect_Lines(wall_x0, wall_z0, wall_x1, wall_z1,
                                    pp_x0,   pp_z0,   pp_x1,   pp_z1,
                                    &xi, &zi);

                                // here comes the tricky part, we need to split the wall in half and
                                // create two new walls. We'll do this by creating two new walls,
                                // placing them on the appropriate front and back lists and
                                // then deleting the original wall (hold your breath)...

                                // process first wall...

                                // allocate the memory for the wall
                                temp_wall = (BSPNODEV1_PTR)malloc(sizeof(BSPNODEV1));

                                // set links
                                temp_wall->front = NULL;
                                temp_wall->back  = NULL;
                                temp_wall->link  = NULL;

                                // poly normal is the same
                                temp_wall->wall.normal  = next_wall->wall.normal;
                                temp_wall->wall.nlength = next_wall->wall.nlength;

                                // poly color is the same
                                temp_wall->wall.color = next_wall->wall.color;

                                // poly material is the same
                                temp_wall->wall.mati = next_wall->wall.mati;

                                // poly texture is the same
                                temp_wall->wall.texture = next_wall->wall.texture;

                                // poly attributes are the same
                                temp_wall->wall.attr = next_wall->wall.attr;

                                // poly states are the same
                                temp_wall->wall.state = next_wall->wall.state;

                                temp_wall->id          = next_wall->id + WALL_SPLIT_ID; // add factor denote a split

                                // compute wall vertices
                                for (index = 0; index < 4; index++)
                                {
                                    temp_wall->wall.vlist[index].x = next_wall->wall.vlist[index].x;
                                    temp_wall->wall.vlist[index].y = next_wall->wall.vlist[index].y;
                                    temp_wall->wall.vlist[index].z = next_wall->wall.vlist[index].z;
                                    temp_wall->wall.vlist[index].w = 1;

                                    // copy vertex attributes, texture coordinates, normal
                                    temp_wall->wall.vlist[index].attr = next_wall->wall.vlist[index].attr;
                                    temp_wall->wall.vlist[index].n    = next_wall->wall.vlist[index].n;
                                    temp_wall->wall.vlist[index].t    = next_wall->wall.vlist[index].t;
                                } // end for index

                                // now modify vertices 1 and 2 to reflect intersection point
                                // but leave y alone since it's invariant for the wall spliting
                                temp_wall->wall.vlist[1].x = xi;
                                temp_wall->wall.vlist[1].z = zi;

                                temp_wall->wall.vlist[2].x = xi;
                                temp_wall->wall.vlist[2].z = zi;

                                // SECTION 7  ////////////////////////////////////////////////////////////////

                                // insert new wall into front or back of root
                                if (dot_wall_1 >= 0)
                                {
                                    // place this wall on the front list
                                    if (root->front==NULL)
                                    {
                                        // this is the first node
                                        root->front      = temp_wall;
                                        front_wall       = root->front;
                                        front_wall->link = NULL;

                                    } // end if
                                    else
                                    {
                                        // this is the nth node
                                        front_wall->link = temp_wall;
                                        front_wall       = front_wall->link;
                                        front_wall->link = NULL;

                                    } // end else

                                } // end if positive
                                else
                                    if (dot_wall_1 < 0)
                                    {
                                        // place this wall on the back list
                                        if (root->back==NULL)
                                        {
                                            // this is the first node
                                            root->back      = temp_wall;
                                            back_wall       = root->back;
                                            back_wall->link = NULL;

                                        } // end if
                                        else
                                        {
                                            // this is the nth node
                                            back_wall->link = temp_wall;
                                            back_wall       = back_wall->link;
                                            back_wall->link = NULL;

                                        } // end else

                                    } // end if negative

                                    // SECTION  8 ////////////////////////////////////////////////////////////////

                                    // process second wall...

                                    // allocate the memory for the wall
                                    temp_wall = (BSPNODEV1_PTR)malloc(sizeof(BSPNODEV1));

                                    // set links
                                    temp_wall->front = NULL;
                                    temp_wall->back  = NULL;
                                    temp_wall->link  = NULL;

                                    // poly normal is the same
                                    temp_wall->wall.normal  = next_wall->wall.normal;
                                    temp_wall->wall.nlength = next_wall->wall.nlength;

                                    // poly color is the same
                                    temp_wall->wall.color = next_wall->wall.color;

                                    // poly material is the same
                                    temp_wall->wall.mati = next_wall->wall.mati;

                                    // poly texture is the same
                                    temp_wall->wall.texture = next_wall->wall.texture;

                                    // poly attributes are the same
                                    temp_wall->wall.attr = next_wall->wall.attr;

                                    // poly states are the same
                                    temp_wall->wall.state = next_wall->wall.state;

                                    temp_wall->id          = next_wall->id + WALL_SPLIT_ID; // add factor denote a split

                                    // compute wall vertices
                                    for (index=0; index < 4; index++)
                                    {
                                        temp_wall->wall.vlist[index].x = next_wall->wall.vlist[index].x;
                                        temp_wall->wall.vlist[index].y = next_wall->wall.vlist[index].y;
                                        temp_wall->wall.vlist[index].z = next_wall->wall.vlist[index].z;
                                        temp_wall->wall.vlist[index].w = 1;

                                        // copy vertex attributes, texture coordinates, normal
                                        temp_wall->wall.vlist[index].attr = next_wall->wall.vlist[index].attr;
                                        temp_wall->wall.vlist[index].n    = next_wall->wall.vlist[index].n;
                                        temp_wall->wall.vlist[index].t    = next_wall->wall.vlist[index].t;

                                    } // end for index

                                    // now modify vertices 0 and 3 to reflect intersection point
                                    // but leave y alone since it's invariant for the wall spliting
                                    temp_wall->wall.vlist[0].x = xi;
                                    temp_wall->wall.vlist[0].z = zi;

                                    temp_wall->wall.vlist[3].x = xi;
                                    temp_wall->wall.vlist[3].z = zi;

                                    // insert new wall into front or back of root
                                    if (dot_wall_2 >= 0)
                                    {
                                        // place this wall on the front list
                                        if (root->front==NULL)
                                        {
                                            // this is the first node
                                            root->front      = temp_wall;
                                            front_wall       = root->front;
                                            front_wall->link = NULL;

                                        } // end if
                                        else
                                        {
                                            // this is the nth node
                                            front_wall->link = temp_wall;
                                            front_wall       = front_wall->link;
                                            front_wall->link = NULL;

                                        } // end else


                                    } // end if positive
                                    else
                                        if (dot_wall_2 < 0)
                                        {
                                            // place this wall on the back list
                                            if (root->back==NULL)
                                            {
                                                // this is the first node
                                                root->back      = temp_wall;
                                                back_wall       = root->back;
                                                back_wall->link = NULL;

                                            } // end if
                                            else
                                            {
                                                // this is the nth node
                                                back_wall->link = temp_wall;
                                                back_wall       = back_wall->link;
                                                back_wall->link = NULL;

                                            } // end else

                                        } // end if negative

                                        // SECTION  9  ////////////////////////////////////////////////////////////////

                                        // we are now done splitting the wall, so we can delete it
                                        temp_wall = next_wall;
                                        next_wall = next_wall->link;

                                        // release the memory
                                        free(temp_wall);

                            } // end else

    } // end while

    // SECTION  10 ////////////////////////////////////////////////////////////////

    // recursively process front and back walls/sub-trees
    Bsp_Build_Tree(root->front);

    Bsp_Build_Tree(root->back);

} // end Bsp_Build_Tree

//////////////////////////////////////////////////////////////////////////////

void Bsp_Print(BSPNODEV1_PTR root)
{
    // this function performs a recursive in-order traversal of the BSP tree and
    // prints the results out to the file opened with fp_out as the handle

    // test if this child is null
    if (root==NULL)
    {
        Write_Error("\nReached NULL node returning...");
        return;
    } // end if

    // search left tree (back walls)
    Write_Error("\nTraversing back sub-tree...");

    // call recursively
    Bsp_Print(root->back);

    // visit node
    Write_Error("\n\n\nWall ID #%d",root->id);

    Write_Error("\nstate   = %d", root->wall.state);      // state information
    Write_Error("\nattr    = %d", root->wall.attr);       // physical attributes of polygon
    Write_Error("\ncolor   = %d", root->wall.color);      // color of polygon
    Write_Error("\ntexture = %x", root->wall.texture);    // pointer to the texture information for simple texture mapping
    Write_Error("\nmati    = %d", root->wall.mati);       // material index (-1) for no material  (new)

    Write_Error("\nVertex 0: (%f,%f,%f,%f)",root->wall.vlist[0].x,
        root->wall.vlist[0].y,
        root->wall.vlist[0].z,
        root->wall.vlist[0].w);

    Write_Error("\nVertex 1: (%f,%f,%f, %f)",root->wall.vlist[1].x,
        root->wall.vlist[1].y,
        root->wall.vlist[1].z,
        root->wall.vlist[1].w);

    Write_Error("\nVertex 2: (%f,%f,%f, %f)",root->wall.vlist[2].x,
        root->wall.vlist[2].y,
        root->wall.vlist[2].z,
        root->wall.vlist[2].w);

    Write_Error("\nVertex 3: (%f,%f,%f, %f)",root->wall.vlist[3].x,
        root->wall.vlist[3].y,
        root->wall.vlist[3].z,
        root->wall.vlist[3].w);

    Write_Error("\nNormal (%f,%f,%f, %f), length=%f",root->wall.normal.x,
        root->wall.normal.y,
        root->wall.normal.z,
        root->wall.nlength);

    Write_Error("\nTextCoords (%f,%f)",root->wall.vlist[1].u0,
        root->wall.vlist[1].v0);

    Write_Error("\nEnd wall data\n");

    // search right tree (front walls)
    Write_Error("\nTraversing front sub-tree..");

    Bsp_Print(root->front);

} // end Bsp_Print

//////////////////////////////////////////////////////////////////////////////

void Bsp_Delete(BSPNODEV1_PTR root)
{
    // this function recursively deletes all the nodes in the bsp tree and free's
    // the memory back to the OS.

    BSPNODEV1_PTR temp_wall; // a temporary wall

    // test if we have hit a dead end
    if (root==NULL)
        return;

    // delete back sub tree
    Bsp_Delete(root->back);

    // delete this node, but first save the front sub-tree
    temp_wall = root->front;

    // delete the memory
    free(root);

    // assign the root to the saved front most sub-tree
    root = temp_wall;

    // delete front sub tree
    Bsp_Delete(root);

} // end Bsp_Delete

///////////////////////////////////////////////////////////////////////////////

void Bsp_Insertion_Traversal_RENDERLIST4DV2(RENDERLIST4DV2_PTR rend_list, 
                                            BSPNODEV1_PTR root,
                                            CAM4DV1_PTR cam,
                                            int insert_local=0)

{
    // converts the entire bsp tree into a face list and then inserts
    // the visible, active, non-clipped, non-culled polygons into
    // the render list, also note the flag insert_local control 
    // whether or not the vlist_local or vlist_trans vertex list
    // is used, thus you can insert a bsp tree "raw" totally untranformed
    // if you set insert_local to 1, default is 0, that is you would
    // only insert an object after at least the local to world transform

    // the functions walks the tree recursively in back to front order
    // relative to the viewpoint sent in cam, the bsp must be in world coordinates
    // for this to work correctly

    // this function works be testing the viewpoint against the current wall
    // in the bsp, then depending on the side the viewpoint is the algorithm
    // proceeds. the search takes place as the rest in an "inorder" method
    // with hooks to process and add each node into the polygon list at the
    // right time

    static VECTOR4D test_vector;
    static float dot_wall;

    // SECTION 1  ////////////////////////////////////////////////////////////////

    //Write_Error("\nEntering Bsp_Insertion_Traversal_RENDERLIST4DV2()...");

    //Write_Error("\nTesting root...");

    // is this a dead end?
    if (root==NULL)
    {
        //Write_Error("\nRoot was null...");
        return;
    } // end if

    //Write_Error("\nRoot was valid...");

    // test which side viewpoint is on relative to the current wall
    VECTOR4D_Build(&root->wall.vlist[0].v, &cam->pos, &test_vector);

    // now dot test vector with the surface normal and analyze signs
    dot_wall = VECTOR4D_Dot(&test_vector, &root->wall.normal);

    //Write_Error("\nTesting dot product...");

    // SECTION 2  ////////////////////////////////////////////////////////////////

    // if the sign of the dot product is positive then the viewer on on the
    // front side of current wall, so recursively process the walls behind then
    // in front of this wall, else do the opposite
    if (dot_wall > 0)
    {
        // viewer is in front of this wall
        //Write_Error("\nDot > 0, front side...");

        // process the back wall sub tree
        Bsp_Insertion_Traversal_RENDERLIST4DV2(rend_list, root->back, cam, insert_local);

        // split quad into (2) triangles for insertion
        POLYF4DV2 poly1, poly2;

        // the only difference from the POLYF4DV2 and the POLYF4DV2Q is that
        // the later has (4) vertices rather than (3), thus we are going to
        // create (2) triangles from the single quad :)
        // copy fields that are important
        poly1.state   = root->wall.state;      // state information
        poly1.attr    = root->wall.attr;       // physical attributes of polygon
        poly1.color   = root->wall.color;      // color of polygon
        poly1.texture = root->wall.texture;    // pointer to the texture information for simple texture mapping
        poly1.mati    = root->wall.mati;       // material index (-1) for no material  (new)
        poly1.nlength = root->wall.nlength;    // length of the polygon normal if not normalized (new)
        poly1.normal  = root->wall.normal;     // the general polygon normal (new)

        poly2.state   = root->wall.state;      // state information
        poly2.attr    = root->wall.attr;       // physical attributes of polygon
        poly2.color   = root->wall.color;      // color of polygon
        poly2.texture = root->wall.texture;    // pointer to the texture information for simple texture mapping
        poly2.mati    = root->wall.mati;       // material index (-1) for no material  (new)
        poly2.nlength = root->wall.nlength;    // length of the polygon normal if not normalized (new)
        poly2.normal  = root->wall.normal;     // the general polygon normal (new)

        // now the vertices, they currently look like this
        // v0      v1
        //
        //
        // v3      v2
        // we want to create (2) triangles that look like this
        //    poly 1           poly2
        // v0      v1                v1  
        // 
        //        
        //
        // v3                v3      v2        
        // 
        // where the winding order of poly 1 is v0,v1,v3 and the winding order
        // of poly 2 is v1, v2, v3 to keep with our clockwise conventions
        if (insert_local==1)
        {
            // polygon 1
            poly1.vlist[0]  = root->wall.vlist[0];  // the vertices of this triangle 
            poly1.tvlist[0] = root->wall.vlist[0];  // the vertices of this triangle    

            poly1.vlist[1]  = root->wall.vlist[1];  // the vertices of this triangle 
            poly1.tvlist[1] = root->wall.vlist[1];  // the vertices of this triangle  

            poly1.vlist[2]  = root->wall.vlist[3];  // the vertices of this triangle 
            poly1.tvlist[2] = root->wall.vlist[3];  // the vertices of this triangle  

            // polygon 2
            poly2.vlist[0]  = root->wall.vlist[1];  // the vertices of this triangle 
            poly2.tvlist[0] = root->wall.vlist[1];  // the vertices of this triangle    

            poly2.vlist[1]  = root->wall.vlist[2];  // the vertices of this triangle 
            poly2.tvlist[1] = root->wall.vlist[2];  // the vertices of this triangle  

            poly2.vlist[2]  = root->wall.vlist[3];  // the vertices of this triangle 
            poly2.tvlist[2] = root->wall.vlist[3];  // the vertices of this triangle  
        } // end if
        else
        {
            // polygon 1
            poly1.vlist[0]  = root->wall.vlist[0];   // the vertices of this triangle 
            poly1.tvlist[0] = root->wall.tvlist[0];  // the vertices of this triangle    

            poly1.vlist[1]  = root->wall.vlist[1];   // the vertices of this triangle 
            poly1.tvlist[1] = root->wall.tvlist[1];  // the vertices of this triangle  

            poly1.vlist[2]  = root->wall.vlist[3];   // the vertices of this triangle 
            poly1.tvlist[2] = root->wall.tvlist[3];  // the vertices of this triangle  

            // polygon 2
            poly2.vlist[0]  = root->wall.vlist[1];   // the vertices of this triangle 
            poly2.tvlist[0] = root->wall.tvlist[1];  // the vertices of this triangle    

            poly2.vlist[1]  = root->wall.vlist[2];   // the vertices of this triangle 
            poly2.tvlist[1] = root->wall.tvlist[2];  // the vertices of this triangle  

            poly2.vlist[2]  = root->wall.vlist[3];   // the vertices of this triangle 
            poly2.tvlist[2] = root->wall.tvlist[3];  // the vertices of this triangle  
        } // end if

        //Write_Error("\nInserting polygons...");

        // insert polygons into rendering list
        Insert_POLYF4DV2_RENDERLIST4DV2(rend_list, &poly1);
        Insert_POLYF4DV2_RENDERLIST4DV2(rend_list, &poly2);

        // now process the front walls sub tree
        Bsp_Insertion_Traversal_RENDERLIST4DV2(rend_list, root->front, cam, insert_local);

    } // end if

    // SECTION 3 ////////////////////////////////////////////////////////////////

    else
    {
        // viewer is behind this wall
        //Write_Error("\nDot < 0, back side...");

        // process the back wall sub tree
        Bsp_Insertion_Traversal_RENDERLIST4DV2(rend_list, root->front, cam, insert_local);

        // split quad into (2) triangles for insertion
        POLYF4DV2 poly1, poly2;

        // the only difference from the POLYF4DV2 and the POLYF4DV2Q is that
        // the later has (4) vertices rather than (3), thus we are going to
        // create (2) triangles from the single quad :)
        // copy fields that are important
        poly1.state   = root->wall.state;      // state information
        poly1.attr    = root->wall.attr;       // physical attributes of polygon
        poly1.color   = root->wall.color;      // color of polygon
        poly1.texture = root->wall.texture;    // pointer to the texture information for simple texture mapping
        poly1.mati    = root->wall.mati;       // material index (-1) for no material  (new)
        poly1.nlength = root->wall.nlength;    // length of the polygon normal if not normalized (new)
        poly1.normal  = root->wall.normal;     // the general polygon normal (new)

        poly2.state   = root->wall.state;      // state information
        poly2.attr    = root->wall.attr;       // physical attributes of polygon
        poly2.color   = root->wall.color;      // color of polygon
        poly2.texture = root->wall.texture;    // pointer to the texture information for simple texture mapping
        poly2.mati    = root->wall.mati;       // material index (-1) for no material  (new)
        poly2.nlength = root->wall.nlength;    // length of the polygon normal if not normalized (new)
        poly2.normal  = root->wall.normal;     // the general polygon normal (new)

        // now the vertices, they currently look like this
        // v0      v1
        //
        //
        // v3      v2
        // we want to create (2) triangles that look like this
        //    poly 1           poly2
        // v0      v1                v1  
        // 
        //        
        //
        // v3                v3      v2        
        // 
        // where the winding order of poly 1 is v0,v1,v3 and the winding order
        // of poly 2 is v1, v2, v3 to keep with our clockwise conventions
        if (insert_local==1)
        {
            // polygon 1
            poly1.vlist[0]  = root->wall.vlist[0];  // the vertices of this triangle 
            poly1.tvlist[0] = root->wall.vlist[0];  // the vertices of this triangle    

            poly1.vlist[1]  = root->wall.vlist[1];  // the vertices of this triangle 
            poly1.tvlist[1] = root->wall.vlist[1];  // the vertices of this triangle  

            poly1.vlist[2]  = root->wall.vlist[3];  // the vertices of this triangle 
            poly1.tvlist[2] = root->wall.vlist[3];  // the vertices of this triangle  

            // polygon 2
            poly2.vlist[0]  = root->wall.vlist[1];  // the vertices of this triangle 
            poly2.tvlist[0] = root->wall.vlist[1];  // the vertices of this triangle    

            poly2.vlist[1]  = root->wall.vlist[2];  // the vertices of this triangle 
            poly2.tvlist[1] = root->wall.vlist[2];  // the vertices of this triangle  

            poly2.vlist[2]  = root->wall.vlist[3];  // the vertices of this triangle 
            poly2.tvlist[2] = root->wall.vlist[3];  // the vertices of this triangle  
        } // end if
        else
        {
            // polygon 1
            poly1.vlist[0]  = root->wall.vlist[0];   // the vertices of this triangle 
            poly1.tvlist[0] = root->wall.tvlist[0];  // the vertices of this triangle    

            poly1.vlist[1]  = root->wall.vlist[1];   // the vertices of this triangle 
            poly1.tvlist[1] = root->wall.tvlist[1];  // the vertices of this triangle  

            poly1.vlist[2]  = root->wall.vlist[3];   // the vertices of this triangle 
            poly1.tvlist[2] = root->wall.tvlist[3];  // the vertices of this triangle  

            // polygon 2
            poly2.vlist[0]  = root->wall.vlist[1];   // the vertices of this triangle 
            poly2.tvlist[0] = root->wall.tvlist[1];  // the vertices of this triangle    

            poly2.vlist[1]  = root->wall.vlist[2];   // the vertices of this triangle 
            poly2.tvlist[1] = root->wall.tvlist[2];  // the vertices of this triangle  

            poly2.vlist[2]  = root->wall.vlist[3];   // the vertices of this triangle 
            poly2.tvlist[2] = root->wall.tvlist[3];  // the vertices of this triangle  
        } // end if

        //Write_Error("\nInserting polygons...");

        // insert polygons into rendering list
        Insert_POLYF4DV2_RENDERLIST4DV2(rend_list, &poly1);
        Insert_POLYF4DV2_RENDERLIST4DV2(rend_list, &poly2);

        // now process the front walls sub tree
        Bsp_Insertion_Traversal_RENDERLIST4DV2(rend_list, root->back, cam, insert_local);

    } // end else

    //Write_Error("\nExiting Bsp_Insertion_Traversal_RENDERLIST4DV2()...");

} // end Bsp_Insertion_Traversal_RENDERLIST4DV2

///////////////////////////////////////////////////////////////////////////////

void Bsp_Insertion_Traversal_RemoveBF_RENDERLIST4DV2(RENDERLIST4DV2_PTR rend_list, 
                                                     BSPNODEV1_PTR root,
                                                     CAM4DV1_PTR cam,
                                                     int insert_local=0)

{
    // converts the entire bsp tree into a face list and then inserts
    // the visible, active, non-clipped, non-culled polygons into
    // the render list, also note the flag insert_local control 
    // whether or not the vlist_local or vlist_trans vertex list
    // is used, thus you can insert a bsp tree "raw" totally untranformed
    // if you set insert_local to 1, default is 0, that is you would
    // only insert an object after at least the local to world transform

    // the functions walks the tree recursively in back to front order
    // relative to the viewpoint sent in cam, the bsp must be in world coordinates
    // for this to work correctly

    // this function works be testing the viewpoint against the current wall
    // in the bsp, then depending on the side the viewpoint is the algorithm
    // proceeds. the search takes place as the rest in an "inorder" method
    // with hooks to process and add each node into the polygon list at the
    // right time
    // additionally the function tests for backfaces on the fly and only inserts
    // polygons that are not backfacing the viewpoint
    // also, it's up to the caller to make sure that cam->n has the valid look at 
    // view vector for euler or UVN model

    static VECTOR4D test_vector;
    static float dot_wall;

    // SECTION 1  ////////////////////////////////////////////////////////////////

    //Write_Error("\nEntering Bsp_Insertion_Traversal_RENDERLIST4DV2()...");

    //Write_Error("\nTesting root...");

    // is this a dead end?
    if (root==NULL)
    {
        //Write_Error("\nRoot was null...");
        return;
    } // end if

    //Write_Error("\nRoot was valid...");

    // test which side viewpoint is on relative to the current wall
    VECTOR4D_Build(&root->wall.vlist[0].v, &cam->pos, &test_vector);

    // now dot test vector with the surface normal and analyze signs
    dot_wall = VECTOR4D_Dot(&test_vector, &root->wall.normal);

    //Write_Error("\nTesting dot product...");

    // SECTION 2  ////////////////////////////////////////////////////////////////

    // if the sign of the dot product is positive then the viewer on on the
    // front side of current wall, so recursively process the walls behind then
    // in front of this wall, else do the opposite
    if (dot_wall > 0)
    {
        // viewer is in front of this wall
        //Write_Error("\nDot > 0, front side...");

        // process the back wall sub tree
        Bsp_Insertion_Traversal_RemoveBF_RENDERLIST4DV2(rend_list, root->back, cam, insert_local);

        // we want to cull polygons that can't be seen from the current viewpoint
        // based not only on the view position, but the viewing angle, hence
        // we first determine if we are on the front or back side of the partitioning
        // plane, then we compute the angle theta from the partitioning plane
        // normal to the viewing direction the player is currently pointing
        // then we ask the question given the field of view and the current
        // viewing direction, can the player see this plane? the answer boils down
        // to the following tests
        // front side test:
        // if theta > (90 + fov/2)
        // and for back side:
        // if theta < (90 - fov/2)
        // to compute theta we need this
        // u . v = |u| * |v| * cos theta
        // since |u| = |v| = 1, we can write
        // u . v = cos theta, hence using the inverse cosine we can get the angle
        // theta = arccosine(u . v)
        // we use the lookup table created with the value of u . v : [-1,1] scaled to
        // 0  to 180 (or 360 for .5 degree accuracy) to compute the angle which is 
        // ALWAYS from 0 - 180, the 360 table just has more accurate entries.
        float dp = VECTOR4D_Dot(&cam->n, &root->wall.normal);  

        // polygon is visible if this is true
        if (FAST_INV_COS(dp) > (90 - cam->fov/2) )
        {
            //////////////////////////////////////////////////////////////////////////// 
            // split quad into (2) triangles for insertion
            POLYF4DV2 poly1, poly2;

            // the only difference from the POLYF4DV2 and the POLYF4DV2Q is that
            // the later has (4) vertices rather than (3), thus we are going to
            // create (2) triangles from the single quad :)
            // copy fields that are important
            poly1.state   = root->wall.state;      // state information
            poly1.attr    = root->wall.attr;       // physical attributes of polygon
            poly1.color   = root->wall.color;      // color of polygon
            poly1.texture = root->wall.texture;    // pointer to the texture information for simple texture mapping
            poly1.mati    = root->wall.mati;       // material index (-1) for no material  (new)
            poly1.nlength = root->wall.nlength;    // length of the polygon normal if not normalized (new)
            poly1.normal  = root->wall.normal;     // the general polygon normal (new)

            poly2.state   = root->wall.state;      // state information
            poly2.attr    = root->wall.attr;       // physical attributes of polygon
            poly2.color   = root->wall.color;      // color of polygon
            poly2.texture = root->wall.texture;    // pointer to the texture information for simple texture mapping
            poly2.mati    = root->wall.mati;       // material index (-1) for no material  (new)
            poly2.nlength = root->wall.nlength;    // length of the polygon normal if not normalized (new)
            poly2.normal  = root->wall.normal;     // the general polygon normal (new)

            // now the vertices, they currently look like this
            // v0      v1
            //
            //
            // v3      v2
            // we want to create (2) triangles that look like this
            //    poly 1           poly2
            // v0      v1                v1  
            // 
            //        
            //
            // v3                v3      v2        
            // 
            // where the winding order of poly 1 is v0,v1,v3 and the winding order
            // of poly 2 is v1, v2, v3 to keep with our clockwise conventions
            if (insert_local==1)
            {
                // polygon 1
                poly1.vlist[0]  = root->wall.vlist[0];  // the vertices of this triangle 
                poly1.tvlist[0] = root->wall.vlist[0];  // the vertices of this triangle    

                poly1.vlist[1]  = root->wall.vlist[1];  // the vertices of this triangle 
                poly1.tvlist[1] = root->wall.vlist[1];  // the vertices of this triangle  

                poly1.vlist[2]  = root->wall.vlist[3];  // the vertices of this triangle 
                poly1.tvlist[2] = root->wall.vlist[3];  // the vertices of this triangle  

                // polygon 2
                poly2.vlist[0]  = root->wall.vlist[1];  // the vertices of this triangle 
                poly2.tvlist[0] = root->wall.vlist[1];  // the vertices of this triangle    

                poly2.vlist[1]  = root->wall.vlist[2];  // the vertices of this triangle 
                poly2.tvlist[1] = root->wall.vlist[2];  // the vertices of this triangle  

                poly2.vlist[2]  = root->wall.vlist[3];  // the vertices of this triangle 
                poly2.tvlist[2] = root->wall.vlist[3];  // the vertices of this triangle  
            } // end if
            else
            {
                // polygon 1
                poly1.vlist[0]  = root->wall.vlist[0];   // the vertices of this triangle 
                poly1.tvlist[0] = root->wall.tvlist[0];  // the vertices of this triangle    

                poly1.vlist[1]  = root->wall.vlist[1];   // the vertices of this triangle 
                poly1.tvlist[1] = root->wall.tvlist[1];  // the vertices of this triangle  

                poly1.vlist[2]  = root->wall.vlist[3];   // the vertices of this triangle 
                poly1.tvlist[2] = root->wall.tvlist[3];  // the vertices of this triangle  

                // polygon 2
                poly2.vlist[0]  = root->wall.vlist[1];   // the vertices of this triangle 
                poly2.tvlist[0] = root->wall.tvlist[1];  // the vertices of this triangle    

                poly2.vlist[1]  = root->wall.vlist[2];   // the vertices of this triangle 
                poly2.tvlist[1] = root->wall.tvlist[2];  // the vertices of this triangle  

                poly2.vlist[2]  = root->wall.vlist[3];   // the vertices of this triangle 
                poly2.tvlist[2] = root->wall.tvlist[3];  // the vertices of this triangle  
            } // end if

            //Write_Error("\nInserting polygons...");

            // insert polygons into rendering list
            Insert_POLYF4DV2_RENDERLIST4DV2(rend_list, &poly1);
            Insert_POLYF4DV2_RENDERLIST4DV2(rend_list, &poly2);
            ////////////////////////////////////////////////////////////////////////////
        } // end if visible

        // now process the front walls sub tree
        Bsp_Insertion_Traversal_RemoveBF_RENDERLIST4DV2(rend_list, root->front, cam, insert_local);

    } // end if

    // SECTION 3 ////////////////////////////////////////////////////////////////

    else
    {
        // viewer is behind this wall
        //Write_Error("\nDot < 0, back side...");

        // process the back wall sub tree
        Bsp_Insertion_Traversal_RemoveBF_RENDERLIST4DV2(rend_list, root->front, cam, insert_local);

        // review explanation above...
        float dp = VECTOR4D_Dot(&cam->n, &root->wall.normal);  

        // polygon is visible if this is true
        if (FAST_INV_COS(dp) < (90 + cam->fov/2) )
        {
            //////////////////////////////////////////////////////////////////////////// 

            // split quad into (2) triangles for insertion
            POLYF4DV2 poly1, poly2;

            // the only difference from the POLYF4DV2 and the POLYF4DV2Q is that
            // the later has (4) vertices rather than (3), thus we are going to
            // create (2) triangles from the single quad :)
            // copy fields that are important
            poly1.state   = root->wall.state;      // state information
            poly1.attr    = root->wall.attr;       // physical attributes of polygon
            poly1.color   = root->wall.color;      // color of polygon
            poly1.texture = root->wall.texture;    // pointer to the texture information for simple texture mapping
            poly1.mati    = root->wall.mati;       // material index (-1) for no material  (new)
            poly1.nlength = root->wall.nlength;    // length of the polygon normal if not normalized (new)
            poly1.normal  = root->wall.normal;     // the general polygon normal (new)

            poly2.state   = root->wall.state;      // state information
            poly2.attr    = root->wall.attr;       // physical attributes of polygon
            poly2.color   = root->wall.color;      // color of polygon
            poly2.texture = root->wall.texture;    // pointer to the texture information for simple texture mapping
            poly2.mati    = root->wall.mati;       // material index (-1) for no material  (new)
            poly2.nlength = root->wall.nlength;    // length of the polygon normal if not normalized (new)
            poly2.normal  = root->wall.normal;     // the general polygon normal (new)

            // now the vertices, they currently look like this
            // v0      v1
            //
            //
            // v3      v2
            // we want to create (2) triangles that look like this
            //    poly 1           poly2
            // v0      v1                v1  
            // 
            //        
            //
            // v3                v3      v2        
            // 
            // where the winding order of poly 1 is v0,v1,v3 and the winding order
            // of poly 2 is v1, v2, v3 to keep with our clockwise conventions
            if (insert_local==1)
            {
                // polygon 1
                poly1.vlist[0]  = root->wall.vlist[0];  // the vertices of this triangle 
                poly1.tvlist[0] = root->wall.vlist[0];  // the vertices of this triangle    

                poly1.vlist[1]  = root->wall.vlist[1];  // the vertices of this triangle 
                poly1.tvlist[1] = root->wall.vlist[1];  // the vertices of this triangle  

                poly1.vlist[2]  = root->wall.vlist[3];  // the vertices of this triangle 
                poly1.tvlist[2] = root->wall.vlist[3];  // the vertices of this triangle  

                // polygon 2
                poly2.vlist[0]  = root->wall.vlist[1];  // the vertices of this triangle 
                poly2.tvlist[0] = root->wall.vlist[1];  // the vertices of this triangle    

                poly2.vlist[1]  = root->wall.vlist[2];  // the vertices of this triangle 
                poly2.tvlist[1] = root->wall.vlist[2];  // the vertices of this triangle  

                poly2.vlist[2]  = root->wall.vlist[3];  // the vertices of this triangle 
                poly2.tvlist[2] = root->wall.vlist[3];  // the vertices of this triangle  
            } // end if
            else
            {
                // polygon 1
                poly1.vlist[0]  = root->wall.vlist[0];   // the vertices of this triangle 
                poly1.tvlist[0] = root->wall.tvlist[0];  // the vertices of this triangle    

                poly1.vlist[1]  = root->wall.vlist[1];   // the vertices of this triangle 
                poly1.tvlist[1] = root->wall.tvlist[1];  // the vertices of this triangle  

                poly1.vlist[2]  = root->wall.vlist[3];   // the vertices of this triangle 
                poly1.tvlist[2] = root->wall.tvlist[3];  // the vertices of this triangle  

                // polygon 2
                poly2.vlist[0]  = root->wall.vlist[1];   // the vertices of this triangle 
                poly2.tvlist[0] = root->wall.tvlist[1];  // the vertices of this triangle    

                poly2.vlist[1]  = root->wall.vlist[2];   // the vertices of this triangle 
                poly2.tvlist[1] = root->wall.tvlist[2];  // the vertices of this triangle  

                poly2.vlist[2]  = root->wall.vlist[3];   // the vertices of this triangle 
                poly2.tvlist[2] = root->wall.tvlist[3];  // the vertices of this triangle  
            } // end if

            //Write_Error("\nInserting polygons...");

            // insert polygons into rendering list
            Insert_POLYF4DV2_RENDERLIST4DV2(rend_list, &poly1);
            Insert_POLYF4DV2_RENDERLIST4DV2(rend_list, &poly2);
            ////////////////////////////////////////////////////////////////////////////
        } // end if visible

        // now process the front walls sub tree
        Bsp_Insertion_Traversal_RemoveBF_RENDERLIST4DV2(rend_list, root->back, cam, insert_local);

    } // end else

    //Write_Error("\nExiting Bsp_Insertion_Traversal_RENDERLIST4DV2()...");

} // end Bsp_Insertion_Traversal_RemoveBF_RENDERLIST4DV2

/////////////////////////////////////////////////////////////////////

void Bsp_Insertion_Traversal_FrustrumCull_RENDERLIST4DV2(RENDERLIST4DV2_PTR rend_list, 
                                                         BSPNODEV1_PTR root,
                                                         CAM4DV1_PTR cam,
                                                         int insert_local=0)

{
    // converts the entire bsp tree into a face list and then inserts
    // the visible, active, non-clipped, non-culled polygons into
    // the render list, also note the flag insert_local control 
    // whether or not the vlist_local or vlist_trans vertex list
    // is used, thus you can insert a bsp tree "raw" totally untranformed
    // if you set insert_local to 1, default is 0, that is you would
    // only insert an object after at least the local to world transform

    // the functions walks the tree recursively in back to front order
    // relative to the viewpoint sent in cam, the bsp must be in world coordinates
    // for this to work correctly

    // this function works be testing the viewpoint against the current wall
    // in the bsp, then depending on the side the viewpoint is the algorithm
    // proceeds. the search takes place as the rest in an "inorder" method
    // with hooks to process and add each node into the polygon list at the
    // right time
    // additionally the function cull the BSP on the fly and only inserts
    // polygons that are not backfacing the viewpoint
    // also, it's up to the caller to make sure that cam->n has the valid look at 
    // view vector for euler or UVN model

    static VECTOR4D test_vector;
    static float dot_wall;

    // SECTION 1  ////////////////////////////////////////////////////////////////

    //Write_Error("\nEntering Bsp_Insertion_Traversal_RENDERLIST4DV2()...");

    //Write_Error("\nTesting root...");

    // is this a dead end?
    if (root==NULL)
    {
        //Write_Error("\nRoot was null...");
        return;
    } // end if

    //Write_Error("\nRoot was valid...");

    // test which side viewpoint is on relative to the current wall
    VECTOR4D_Build(&root->wall.vlist[0].v, &cam->pos, &test_vector);

    // now dot test vector with the surface normal and analyze signs
    dot_wall = VECTOR4D_Dot(&test_vector, &root->wall.normal);

    //Write_Error("\nTesting dot product...");

    // SECTION 2  ////////////////////////////////////////////////////////////////

    // if the sign of the dot product is positive then the viewer on on the
    // front side of current wall, so recursively process the walls behind then
    // in front of this wall, else do the opposite
    if (dot_wall > 0)
    {
        // viewer is in front of this wall
        //Write_Error("\nDot > 0, front side...");

        // we want to cull sub spaces that can't be seen from the current viewpoint
        // based not only on the view position, but the viewing angle, hence
        // we first determine if we are on the front or back side of the partitioning
        // plane, then we compute the angle theta from the partitioning plane
        // normal to the viewing direction the player is currently pointing
        // then we ask the question given the field of view and the current
        // viewing direction, can the player see this plane? the answer boils down
        // to the following tests
        // front side test:
        // if theta > (90 + fov/2)
        // and for back side:
        // if theta < (90 - fov/2)
        // to compute theta we need this
        // u . v = |u| * |v| * cos theta
        // since |u| = |v| = 1, we can write
        // u . v = cos theta, hence using the inverse cosine we can get the angle
        // theta = arccosine(u . v)
        // we use the lookup table created with the value of u . v : [-1,1] scaled to
        // 0  to 180 (or 360 for .5 degree accuracy) to compute the angle which is 
        // ALWAYS from 0 - 180, the 360 table just has more accurate entries.
        float dp = VECTOR4D_Dot(&cam->n, &root->wall.normal);  

        // polygon is visible if this is true
        if (FAST_INV_COS(dp) > (90 - cam->fov/2) )
        {
            // process the back wall sub tree
            Bsp_Insertion_Traversal_FrustrumCull_RENDERLIST4DV2(rend_list, root->back, cam, insert_local);

            //////////////////////////////////////////////////////////////////////////// 
            // split quad into (2) triangles for insertion
            POLYF4DV2 poly1, poly2;

            // the only difference from the POLYF4DV2 and the POLYF4DV2Q is that
            // the later has (4) vertices rather than (3), thus we are going to
            // create (2) triangles from the single quad :)
            // copy fields that are important
            poly1.state   = root->wall.state;      // state information
            poly1.attr    = root->wall.attr;       // physical attributes of polygon
            poly1.color   = root->wall.color;      // color of polygon
            poly1.texture = root->wall.texture;    // pointer to the texture information for simple texture mapping
            poly1.mati    = root->wall.mati;       // material index (-1) for no material  (new)
            poly1.nlength = root->wall.nlength;    // length of the polygon normal if not normalized (new)
            poly1.normal  = root->wall.normal;     // the general polygon normal (new)

            poly2.state   = root->wall.state;      // state information
            poly2.attr    = root->wall.attr;       // physical attributes of polygon
            poly2.color   = root->wall.color;      // color of polygon
            poly2.texture = root->wall.texture;    // pointer to the texture information for simple texture mapping
            poly2.mati    = root->wall.mati;       // material index (-1) for no material  (new)
            poly2.nlength = root->wall.nlength;    // length of the polygon normal if not normalized (new)
            poly2.normal  = root->wall.normal;     // the general polygon normal (new)

            // now the vertices, they currently look like this
            // v0      v1
            //
            //
            // v3      v2
            // we want to create (2) triangles that look like this
            //    poly 1           poly2
            // v0      v1                v1  
            // 
            //        
            //
            // v3                v3      v2        
            // 
            // where the winding order of poly 1 is v0,v1,v3 and the winding order
            // of poly 2 is v1, v2, v3 to keep with our clockwise conventions
            if (insert_local==1)
            {
                // polygon 1
                poly1.vlist[0]  = root->wall.vlist[0];  // the vertices of this triangle 
                poly1.tvlist[0] = root->wall.vlist[0];  // the vertices of this triangle    

                poly1.vlist[1]  = root->wall.vlist[1];  // the vertices of this triangle 
                poly1.tvlist[1] = root->wall.vlist[1];  // the vertices of this triangle  

                poly1.vlist[2]  = root->wall.vlist[3];  // the vertices of this triangle 
                poly1.tvlist[2] = root->wall.vlist[3];  // the vertices of this triangle  

                // polygon 2
                poly2.vlist[0]  = root->wall.vlist[1];  // the vertices of this triangle 
                poly2.tvlist[0] = root->wall.vlist[1];  // the vertices of this triangle    

                poly2.vlist[1]  = root->wall.vlist[2];  // the vertices of this triangle 
                poly2.tvlist[1] = root->wall.vlist[2];  // the vertices of this triangle  

                poly2.vlist[2]  = root->wall.vlist[3];  // the vertices of this triangle 
                poly2.tvlist[2] = root->wall.vlist[3];  // the vertices of this triangle  
            } // end if
            else
            {
                // polygon 1
                poly1.vlist[0]  = root->wall.vlist[0];   // the vertices of this triangle 
                poly1.tvlist[0] = root->wall.tvlist[0];  // the vertices of this triangle    

                poly1.vlist[1]  = root->wall.vlist[1];   // the vertices of this triangle 
                poly1.tvlist[1] = root->wall.tvlist[1];  // the vertices of this triangle  

                poly1.vlist[2]  = root->wall.vlist[3];   // the vertices of this triangle 
                poly1.tvlist[2] = root->wall.tvlist[3];  // the vertices of this triangle  

                // polygon 2
                poly2.vlist[0]  = root->wall.vlist[1];   // the vertices of this triangle 
                poly2.tvlist[0] = root->wall.tvlist[1];  // the vertices of this triangle    

                poly2.vlist[1]  = root->wall.vlist[2];   // the vertices of this triangle 
                poly2.tvlist[1] = root->wall.tvlist[2];  // the vertices of this triangle  

                poly2.vlist[2]  = root->wall.vlist[3];   // the vertices of this triangle 
                poly2.tvlist[2] = root->wall.tvlist[3];  // the vertices of this triangle  
            } // end if

            //Write_Error("\nInserting polygons...");

            // insert polygons into rendering list
            Insert_POLYF4DV2_RENDERLIST4DV2(rend_list, &poly1);
            Insert_POLYF4DV2_RENDERLIST4DV2(rend_list, &poly2);
            ////////////////////////////////////////////////////////////////////////////
        } // end if visible

        // now process the front walls sub tree
        Bsp_Insertion_Traversal_FrustrumCull_RENDERLIST4DV2(rend_list, root->front, cam, insert_local);

    } // end if

    // SECTION 3 ////////////////////////////////////////////////////////////////

    else
    {
        // viewer is behind this wall
        //Write_Error("\nDot < 0, back side...");


        // review explanation above...
        float dp = VECTOR4D_Dot(&cam->n, &root->wall.normal);  

        // polygon is visible if this is true
        if (FAST_INV_COS(dp) < (90 + cam->fov/2) )
        {
            // process the back wall sub tree
            Bsp_Insertion_Traversal_FrustrumCull_RENDERLIST4DV2(rend_list, root->front, cam, insert_local);

            //////////////////////////////////////////////////////////////////////////// 

            // split quad into (2) triangles for insertion
            POLYF4DV2 poly1, poly2;

            // the only difference from the POLYF4DV2 and the POLYF4DV2Q is that
            // the later has (4) vertices rather than (3), thus we are going to
            // create (2) triangles from the single quad :)
            // copy fields that are important
            poly1.state   = root->wall.state;      // state information
            poly1.attr    = root->wall.attr;       // physical attributes of polygon
            poly1.color   = root->wall.color;      // color of polygon
            poly1.texture = root->wall.texture;    // pointer to the texture information for simple texture mapping
            poly1.mati    = root->wall.mati;       // material index (-1) for no material  (new)
            poly1.nlength = root->wall.nlength;    // length of the polygon normal if not normalized (new)
            poly1.normal  = root->wall.normal;     // the general polygon normal (new)

            poly2.state   = root->wall.state;      // state information
            poly2.attr    = root->wall.attr;       // physical attributes of polygon
            poly2.color   = root->wall.color;      // color of polygon
            poly2.texture = root->wall.texture;    // pointer to the texture information for simple texture mapping
            poly2.mati    = root->wall.mati;       // material index (-1) for no material  (new)
            poly2.nlength = root->wall.nlength;    // length of the polygon normal if not normalized (new)
            poly2.normal  = root->wall.normal;     // the general polygon normal (new)

            // now the vertices, they currently look like this
            // v0      v1
            //
            //
            // v3      v2
            // we want to create (2) triangles that look like this
            //    poly 1           poly2
            // v0      v1                v1  
            // 
            //        
            //
            // v3                v3      v2        
            // 
            // where the winding order of poly 1 is v0,v1,v3 and the winding order
            // of poly 2 is v1, v2, v3 to keep with our clockwise conventions
            if (insert_local==1)
            {
                // polygon 1
                poly1.vlist[0]  = root->wall.vlist[0];  // the vertices of this triangle 
                poly1.tvlist[0] = root->wall.vlist[0];  // the vertices of this triangle    

                poly1.vlist[1]  = root->wall.vlist[1];  // the vertices of this triangle 
                poly1.tvlist[1] = root->wall.vlist[1];  // the vertices of this triangle  

                poly1.vlist[2]  = root->wall.vlist[3];  // the vertices of this triangle 
                poly1.tvlist[2] = root->wall.vlist[3];  // the vertices of this triangle  

                // polygon 2
                poly2.vlist[0]  = root->wall.vlist[1];  // the vertices of this triangle 
                poly2.tvlist[0] = root->wall.vlist[1];  // the vertices of this triangle    

                poly2.vlist[1]  = root->wall.vlist[2];  // the vertices of this triangle 
                poly2.tvlist[1] = root->wall.vlist[2];  // the vertices of this triangle  

                poly2.vlist[2]  = root->wall.vlist[3];  // the vertices of this triangle 
                poly2.tvlist[2] = root->wall.vlist[3];  // the vertices of this triangle  
            } // end if
            else
            {
                // polygon 1
                poly1.vlist[0]  = root->wall.vlist[0];   // the vertices of this triangle 
                poly1.tvlist[0] = root->wall.tvlist[0];  // the vertices of this triangle    

                poly1.vlist[1]  = root->wall.vlist[1];   // the vertices of this triangle 
                poly1.tvlist[1] = root->wall.tvlist[1];  // the vertices of this triangle  

                poly1.vlist[2]  = root->wall.vlist[3];   // the vertices of this triangle 
                poly1.tvlist[2] = root->wall.tvlist[3];  // the vertices of this triangle  

                // polygon 2
                poly2.vlist[0]  = root->wall.vlist[1];   // the vertices of this triangle 
                poly2.tvlist[0] = root->wall.tvlist[1];  // the vertices of this triangle    

                poly2.vlist[1]  = root->wall.vlist[2];   // the vertices of this triangle 
                poly2.tvlist[1] = root->wall.tvlist[2];  // the vertices of this triangle  

                poly2.vlist[2]  = root->wall.vlist[3];   // the vertices of this triangle 
                poly2.tvlist[2] = root->wall.tvlist[3];  // the vertices of this triangle  
            } // end if

            //Write_Error("\nInserting polygons...");

            // insert polygons into rendering list
            Insert_POLYF4DV2_RENDERLIST4DV2(rend_list, &poly1);
            Insert_POLYF4DV2_RENDERLIST4DV2(rend_list, &poly2);
            ////////////////////////////////////////////////////////////////////////////
        } // end if visible

        // now process the front walls sub tree
        Bsp_Insertion_Traversal_FrustrumCull_RENDERLIST4DV2(rend_list, root->back, cam, insert_local);

    } // end else

    //Write_Error("\nExiting Bsp_Insertion_Traversal_RENDERLIST4DV2()...");

} // end Bsp_Insertion_Traversal_FrustrumCull_RENDERLIST4DV2

/////////////////////////////////////////////////////////////////////

void Bsp_Transform(BSPNODEV1_PTR root,  // root of bsp tree
                   MATRIX4X4_PTR mt,    // transformation matrix
                   int coord_select)    // selects coords to transform)
{
    // this function traverses the bsp tree and applies the transformation
    // matrix to each node, the function is of course recursive and uses 
    // and inorder traversal, other traversals such as preorder and postorder 
    // will would just as well...

    // test if we have hit a dead end
    if (root==NULL)
        return;

    // transform back most sub-tree
    Bsp_Transform(root->back, mt, coord_select);

    // iterate thru all vertices of current wall and transform them into
    // camera coordinates

    // what coordinates should be transformed?
    switch(coord_select)
    {
    case TRANSFORM_LOCAL_ONLY:
        {
            // transform each local/model vertex of the object mesh in place
            for (int vertex = 0; vertex < 4; vertex++)
            {
                POINT4D presult; // hold result of each transformation

                // transform point
                Mat_Mul_VECTOR4D_4X4(&root->wall.vlist[vertex].v, mt, &presult);

                // store result back
                VECTOR4D_COPY(&root->wall.vlist[vertex].v, &presult); 

                // transform vertex normal if needed
                if (root->wall.vlist[vertex].attr & VERTEX4DTV1_ATTR_NORMAL)
                {
                    // transform normal
                    Mat_Mul_VECTOR4D_4X4(&root->wall.vlist[vertex].n, mt, &presult);

                    // store result back
                    VECTOR4D_COPY(&root->wall.vlist[vertex].n, &presult); 
                } // end if
            } // end for index
        } break;

    case TRANSFORM_TRANS_ONLY:
        {
            // transform each "transformed" vertex of the object mesh in place
            // remember, the idea of the vlist_trans[] array is to accumulate
            // transformations
            for (int vertex = 0; vertex < 4; vertex++)
            {
                POINT4D presult; // hold result of each transformation

                // transform point
                Mat_Mul_VECTOR4D_4X4(&root->wall.tvlist[vertex].v, mt, &presult);

                // store result back
                VECTOR4D_COPY(&root->wall.tvlist[vertex].v, &presult); 

                // transform vertex normal if needed
                if (root->wall.tvlist[vertex].attr & VERTEX4DTV1_ATTR_NORMAL)
                {
                    // transform normal
                    Mat_Mul_VECTOR4D_4X4(&root->wall.tvlist[vertex].n, mt, &presult);

                    // store result back
                    VECTOR4D_COPY(&root->wall.tvlist[vertex].n, &presult); 
                } // end if
            } // end for index

        } break;

    case TRANSFORM_LOCAL_TO_TRANS:
        {
            // transform each local/model vertex of the object mesh and store result
            // in "transformed" vertex list
            for (int vertex=0; vertex < 4; vertex++)
            {
                POINT4D presult; // hold result of each transformation

                // transform point
                Mat_Mul_VECTOR4D_4X4(&root->wall.vlist[vertex].v, mt, &root->wall.tvlist[vertex].v);

                // transform vertex normal if needed
                if (root->wall.tvlist[vertex].attr & VERTEX4DTV1_ATTR_NORMAL)
                {
                    // transform point
                    Mat_Mul_VECTOR4D_4X4(&root->wall.vlist[vertex].n, mt, &root->wall.tvlist[vertex].n);
                } // end if

            } // end for index
        } break;

    default: break;

    } // end switch

    // transform front most sub-tree
    Bsp_Transform(root->front, mt, coord_select);

} // end Bsp_Insertion_Traversal_RENDERLIST4DV2

////////////////////////////////////////////////////////////////////////

void Bsp_Translate(BSPNODEV1_PTR root, VECTOR4D_PTR trans)
{
    // this function translates all the walls that make up the bsp world
    // note function is recursive, we don't really need this function, but
    // it's a good example of how we might perform transformations on the BSP
    // tree and similar tree like structures using recursion
    // note the translation is perform from local to world coords

    static int index; // looping variable

    // test if we have hit a dead end
    if (root==NULL)
        return;

    // translate back most sub-tree
    Bsp_Translate(root->back, trans);

    // iterate thru all vertices of current wall and translate them
    for (index=0; index < 4; index++)
    {
        // perform translation
        root->wall.tvlist[index].x = root->wall.vlist[index].x + trans->x;
        root->wall.tvlist[index].y = root->wall.vlist[index].y + trans->y;
        root->wall.tvlist[index].z = root->wall.vlist[index].x + trans->z;
    } // end for index

    // translate front most sub-tree
    Bsp_Translate(root->front, trans);

} // end Bsp_Translate

///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////

void Build_Inverse_Cos_Table(float *invcos,     // storage for table
                             int   range_scale) // range for table to span

{
    // this function builds an inverse cosine table used to help with
    // dot product calculations where the angle is needed rather than 
    // the value -1 to 1, the function maps the values [-1, 1] to
    // [0, range] and then breaks that interval up into n intervals
    // where the interval size is 180/range then the function computes
    // the arccos for each value -1 to 1 scaled and mapped as 
    // referred to above and places the values in the table
    // for example if the range is 360 then the interval will be
    // [0, 360] therefore, since the inverse cosine must
    // always be from 0 - 180, we are going to map 0 - 180 to 0 - 360
    // or in other words, the array elements will each represent .5
    // degree increments
    // the table must be large enough to hold the results which will
    // be = (range_scale+1) * sizeof(float)

    // to use the table, the user must scale the value of [-1, 1] to the 
    // exact table size, for example say you made a table with 0..180 
    // the to access it with values from x = [-1, 1] would be:
    // invcos[ (x+1)*180 ], the result would be the angle in degrees
    // to a 1.0 degree accuracy.


    float val = -1; // starting value

    // create table
    int index = 0;
    for (index = 0; index <= range_scale; index++)
    {
        // insert next element in table in degrees
        val = (val > 1) ? 1 : val;
        invcos[index] = RAD_TO_DEG(acos(val));
        //Write_Error("\ninvcos[%d] = %f, val = %f", index, invcos[index], val);

        // increment val by interval
        val += ((float)1/(float)(range_scale/2));

    } // end for index

    // insert one more element for padding, so that durring access if there is
    // an overflow of 1, we won't go out of bounds and we can save the clamp in the
    // floating point logic
    invcos[index] = invcos[index-1];

} // end Build_Inverse_Cos_Table

///////////////////////////////////////////////////////////////////////////////


void Draw_RENDERLIST4DV2_RENDERCONTEXTV1_16_2(RENDERCONTEXTV1_PTR rc)
{
    // this function renders the rendering list, it's based on the new
    // rendering context data structure which is container for everything
    // we need to consider when rendering, z, 1/z buffering, alpha, mipmapping,
    // perspective, bilerp, etc. the function is basically a merge of all the functions
    // we have written thus far, so its rather long, but better than having 
    // 20-30 rendering functions for all possible permutations!
    // this new version _2 supports the new RENDER_ATTR_WRITETHRUZBUFFER 
    // functionality in a very limited manner, it supports mip mapping in general
    // no alpha blending, and only supports the following types:
    // constant shaded, flat shaded, gouraud shaded 
    // affine only: constant textured, flat textured, and gouraud textured
    // when not using the RENDER_ATTR_WRITETHRUZBUFFER support, the 
    // function is identical to the previous version

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
                            Draw_Textured_Triangle_Alpha16(&face, rc->video_buffer, rc->lpitch, alpha);
                        } // end if
                        else
                            if (rc->attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_CORRECT)
                            {
                                // not supported yet!
                                Draw_Textured_Triangle_Alpha16(&face, rc->video_buffer, rc->lpitch, alpha);
                            } // end if
                            else
                                if (rc->attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_LINEAR)
                                {
                                    // not supported yet
                                    Draw_Textured_Triangle_Alpha16(&face, rc->video_buffer, rc->lpitch, alpha);
                                } // end if
                                else
                                    if (rc->attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_HYBRID1)
                                    {
                                        // test z distance again perspective transition gate
                                        if (rc->rend_list->poly_ptrs[poly]->tvlist[0].z > rc-> texture_dist)
                                        {
                                            // default back to affine
                                            Draw_Textured_Triangle_Alpha16(&face, rc->video_buffer, rc->lpitch, alpha);
                                        } // end if
                                        else
                                        {
                                            // use perspective linear
                                            // not supported yet
                                            Draw_Textured_Triangle_Alpha16(&face, rc->video_buffer, rc->lpitch, alpha);
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
                                Draw_Textured_TriangleZB_Alpha16(&face, rc->video_buffer, rc->lpitch,rc->zbuffer,rc->zpitch, alpha);
                            } // end if
                            else
                                if (rc->attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_CORRECT)
                                {
                                    // not supported yet!
                                    Draw_Textured_TriangleZB_Alpha16(&face, rc->video_buffer, rc->lpitch,rc->zbuffer,rc->zpitch, alpha);
                                } // end if
                                else
                                    if (rc->attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_LINEAR)
                                    {
                                        // not supported yet
                                        Draw_Textured_TriangleZB_Alpha16(&face, rc->video_buffer, rc->lpitch,rc->zbuffer,rc->zpitch, alpha);
                                    } // end if
                                    else
                                        if (rc->attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_HYBRID1)
                                        {
                                            // test z distance again perspective transition gate
                                            if (rc->rend_list->poly_ptrs[poly]->tvlist[0].z > rc-> texture_dist)
                                            {
                                                // default back to affine
                                                Draw_Textured_TriangleZB_Alpha16(&face, rc->video_buffer, rc->lpitch,rc->zbuffer,rc->zpitch, alpha);
                                            } // end if
                                            else
                                            {
                                                // use perspective linear
                                                // not supported yet
                                                Draw_Textured_TriangleZB_Alpha16(&face, rc->video_buffer, rc->lpitch,rc->zbuffer,rc->zpitch, alpha);
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
                                    Draw_Textured_TriangleZB2_16(&face, rc->video_buffer, rc->lpitch,rc->zbuffer,rc->zpitch);

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
                                        Draw_Textured_TriangleZB_Alpha16(&face, rc->video_buffer, rc->lpitch,rc->zbuffer,rc->zpitch, alpha);
                                    } // end if
                                    else
                                        if (rc->attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_CORRECT)
                                        {
                                            // not supported yet!
                                            Draw_Textured_TriangleZB_Alpha16(&face, rc->video_buffer, rc->lpitch,rc->zbuffer,rc->zpitch, alpha);
                                        } // end if
                                        else
                                            if (rc->attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_LINEAR)
                                            {
                                                // not supported yet
                                                Draw_Textured_TriangleZB_Alpha16(&face, rc->video_buffer, rc->lpitch,rc->zbuffer,rc->zpitch, alpha);
                                            } // end if
                                            else
                                                if (rc->attr & RENDER_ATTR_TEXTURE_PERSPECTIVE_HYBRID1)
                                                {
                                                    // test z distance again perspective transition gate
                                                    if (rc->rend_list->poly_ptrs[poly]->tvlist[0].z > rc-> texture_dist)
                                                    {
                                                        // default back to affine
                                                        Draw_Textured_TriangleZB_Alpha16(&face, rc->video_buffer, rc->lpitch,rc->zbuffer,rc->zpitch, alpha);
                                                    } // end if
                                                    else
                                                    {
                                                        // use perspective linear
                                                        // not supported yet
                                                        Draw_Textured_TriangleZB_Alpha16(&face, rc->video_buffer, rc->lpitch,rc->zbuffer,rc->zpitch, alpha);
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
                                            Draw_Textured_TriangleWTZB2_16(&face, rc->video_buffer, rc->lpitch,rc->zbuffer,rc->zpitch);

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
                                            Draw_Textured_TriangleFSWTZB2_16(&face, rc->video_buffer, rc->lpitch,rc->zbuffer,rc->zpitch);
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
                                        Draw_Gouraud_TriangleWTZB2_16(&face, rc->video_buffer, rc->lpitch,rc->zbuffer,rc->zpitch);
                                    } // end if

                                } // end if gouraud

                    } // end for poly

                } // end if RENDER_ATTR_ZBUFFER

} // end Draw_RENDERLIST4DV2_RENDERCONTEXTV1_16_2

////////////////////////////////////////////////////////////////////////////////////////

void Draw_Textured_TriangleWTZB2_16(POLYF4DV2_PTR face,  // ptr to face
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
                        screen_ptr[xi] = textmap[(ui >> FIXP16_SHIFT) + ((vi >> FIXP16_SHIFT) << texture_shift2)];

                        // update z-buffer
                        z_ptr[xi] = zi;           


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
                        screen_ptr[xi] = textmap[(ui >> FIXP16_SHIFT) + ((vi >> FIXP16_SHIFT) << texture_shift2)];

                        // update z-buffer
                        z_ptr[xi] = zi;   

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
                                screen_ptr[xi] = textmap[(ui >> FIXP16_SHIFT) + ((vi >> FIXP16_SHIFT) << texture_shift2)];

                                // update z-buffer
                                z_ptr[xi] = zi;   

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
                                screen_ptr[xi] = textmap[(ui >> FIXP16_SHIFT) + ((vi >> FIXP16_SHIFT) << texture_shift2)];

                                // update z-buffer
                                z_ptr[xi] = zi;   

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

} // end Draw_Textured_TriangleWTZB2_16

///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////

void Draw_Textured_TriangleFSWTZB2_16(POLYF4DV2_PTR face, // ptr to face
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

} // end Draw_Textured_TriangleFSWTZB2_16

//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////

void Draw_Textured_TriangleGSWTZB_16(POLYF4DV2_PTR face,   // ptr to face
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

} // end Draw_Textured_TriangleGSWTZB_16

///////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////

void Draw_Triangle_2DWTZB_16(POLYF4DV2_PTR face,   // ptr to face
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
                        // write thru z buffer 
                        // write textel assume 5.6.5
                        screen_ptr[xi] = color;

                        // update z-buffer
                        z_ptr[xi] = zi;           


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
                        // write thru z buffer 
                        // write textel assume 5.6.5
                        screen_ptr[xi] = color;

                        // update z-buffer
                        z_ptr[xi] = zi;     


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
                                // write thru z buffer 
                                // write textel assume 5.6.5
                                screen_ptr[xi] = color;

                                // update z-buffer
                                z_ptr[xi] = zi;     

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
                                // write thru z buffer 
                                // write textel assume 5.6.5
                                screen_ptr[xi] = color;

                                // update z-buffer
                                z_ptr[xi] = zi;     

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

} // end Draw_Triangle_2DWTZB_16

///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////

void Draw_Gouraud_TriangleWTZB2_16(POLYF4DV2_PTR face,   // ptr to face
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
                    for (xi=xstart; xi < xend; xi++)
                    {
                        // write thru z buffer always
                        // write textel assume 5.6.5
                        screen_ptr[xi] = ((ui >> (FIXP16_SHIFT+3)) << 11) + 
                            ((vi >> (FIXP16_SHIFT+2)) << 5) + 
                            (wi >> (FIXP16_SHIFT+3));   

                        // update z-buffer
                        z_ptr[xi] = zi;           

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
                    for (xi=xstart; xi < xend; xi++)
                    {
                        // write thru z buffer always
                        // write textel assume 5.6.5
                        screen_ptr[xi] = ((ui >> (FIXP16_SHIFT+3)) << 11) + 
                            ((vi >> (FIXP16_SHIFT+2)) << 5) + 
                            (wi >> (FIXP16_SHIFT+3));   

                        // update z-buffer
                        z_ptr[xi] = zi;           


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
                            for (xi=xstart; xi < xend; xi++)
                            {
                                // write thru z buffer always
                                // write textel assume 5.6.5
                                screen_ptr[xi] = ((ui >> (FIXP16_SHIFT+3)) << 11) + 
                                    ((vi >> (FIXP16_SHIFT+2)) << 5) + 
                                    (wi >> (FIXP16_SHIFT+3));   

                                // update z-buffer
                                z_ptr[xi] = zi;           


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

                        for (yi = ystart; yi < yend; yi++)
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
                            for (xi=xstart; xi < xend; xi++)
                            {
                                // write thru z buffer always
                                // write textel assume 5.6.5
                                screen_ptr[xi] = ((ui >> (FIXP16_SHIFT+3)) << 11) + 
                                    ((vi >> (FIXP16_SHIFT+2)) << 5) + 
                                    (wi >> (FIXP16_SHIFT+3));   

                                // update z-buffer
                                z_ptr[xi] = zi;           


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

} // end Draw_Gouraud_TriangleWTZB2_16

///////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////

void BHV_Reset_Tree(BHV_NODEV1_PTR bhv_tree)
{
    // this function simply resets all of the culled flags in the root of the
    // tree which will enable the entire tree as visible
    for (int iobject = 0; iobject < bhv_tree->num_objects; iobject++)
    {
        // reset the culled flag
        RESET_BIT(bhv_tree->objects[iobject]->state, OBJECT4DV2_STATE_CULLED);
    } // end if

} // end BHV_Reset_Tree

//////////////////////////////////////////////////////////////////////////////

int BHV_FrustrumCull(BHV_NODEV1_PTR bhv_tree, // the root of the BHV  
                     CAM4DV1_PTR cam,         // camera to cull relative to
                     int cull_flags)          // clipping planes to consider
{
    // NOTE: is matrix based
    // this function culls the BHV from the viewing
    // frustrum by using the sent camera information and
    // the cull_flags determine what axes culling should take place
    // x, y, z or all which is controlled by ORing the flags
    // together. as the BHV is culled the state information in each node is 
    // modified, so rendering functions can refer to it

    // test for valid BHV and camera
    if (!bhv_tree || !cam)
        return(0);

    // we need to walk the tree from top to bottom culling

    // step 1: transform the center of the nodes bounding
    // sphere into camera space

    POINT4D sphere_pos; // hold result of transforming center of bounding sphere

    // transform point
    Mat_Mul_VECTOR4D_4X4(&bhv_tree->pos, &cam->mcam, &sphere_pos);

    // step 2:  based on culling flags remove the object
    if (cull_flags & CULL_OBJECT_Z_PLANE)
    {
        // cull only based on z clipping planes

        // test far plane
        if ( ((sphere_pos.z - bhv_tree->radius.z) > cam->far_clip_z) ||
            ((sphere_pos.z + bhv_tree->radius.z) < cam->near_clip_z) )
        { 
            // this entire node is culled, so we need to set the culled flag
            // for every object
            for (int iobject = 0; iobject < bhv_tree->num_objects; iobject++)
            {
                SET_BIT(bhv_tree->objects[iobject]->state, OBJECT4DV2_STATE_CULLED);
            } // end for iobject
#if 0
            Write_Error("\n[ZBHV p(%f, %f, %f) r(%f) #objs(%d)]", bhv_tree->pos.x, 
                bhv_tree->pos.y,
                bhv_tree->pos.z,
                bhv_tree->radius.x, 
                bhv_tree->num_objects);
#endif


            // this node was visited and culled
            bhv_nodes_visited++;

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

        if ( ((sphere_pos.x - bhv_tree->radius.x) > z_test)  || // right side
            ((sphere_pos.x + bhv_tree->radius.x) < -z_test) )  // left side, note sign change
        { 
            // this entire node is culled, so we need to set the culled flag
            // for every object
            for (int iobject = 0; iobject < bhv_tree->num_objects; iobject++)
            {
                SET_BIT(bhv_tree->objects[iobject]->state, OBJECT4DV2_STATE_CULLED);
            } // end for iobject
#if 0
            Write_Error("\n[XBHV p(%f, %f, %f) r(%f) #objs(%d)]", bhv_tree->pos.x, 
                bhv_tree->pos.y,
                bhv_tree->pos.z,
                bhv_tree->radius.x, bhv_tree->num_objects);

#endif

            // this node was visited and culled
            bhv_nodes_visited++;

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

        if ( ((sphere_pos.y - bhv_tree->radius.y) > z_test)  || // top side
            ((sphere_pos.y + bhv_tree->radius.y) < -z_test) )  // bottom side, note sign change
        { 
            // this entire node is culled, so we need to set the culled flag
            // for every object
            for (int iobject = 0; iobject < bhv_tree->num_objects; iobject++)
            {
                SET_BIT(bhv_tree->objects[iobject]->state, OBJECT4DV2_STATE_CULLED);
            } // end for iobject

#if 0
            Write_Error("\n[YBHV p(%f, %f, %f) r(%f) #objs(%d)]", bhv_tree->pos.x, 
                bhv_tree->pos.y,
                bhv_tree->pos.z,
                bhv_tree->radius.x, 
                bhv_tree->num_objects);
#endif

            // this node was visited and culled
            bhv_nodes_visited++;

            return(1);
        } // end if

    } // end if

    // at this point, we have concluded that this BHV node is too large
    // to cull, so we need to traverse the children and see if we can cull them
    for (int ichild = 0; ichild < bhv_tree->num_children; ichild++)
    {
        // recursively call..
        BHV_FrustrumCull(bhv_tree->links[ichild], cam, cull_flags);

        // here's where we can optimize by tracking the total number 
        // of objects culled and we can exit early if all the objects
        // are already culled...

    } // end ichild

    // return failure to cull anything!
    return(0);

} // end BHV_FrustrumCull

//////////////////////////////////////////////////////////////////////////////

void BHV_Build_Tree(BHV_NODEV1_PTR bhv_tree,         // tree to build
                    OBJ_CONTAINERV1_PTR bhv_objects, // ptr to all objects in intial scene
                    int num_objects,                 // number of objects in initial scene
                    int level,                       // recursion level
                    int num_divisions,               // number of division per level
                    int universe_radius)             // initial size of universe to enclose
{
    // this function builds the BHV tree using a divide and conquer
    // divisioning algorithm to cluster the objects together

    Write_Error("\nEntering BHV function...");

    // are we building root?
    if (level == 0)
    {
        Write_Error("\nlevel = 0");

        // position at (0,0,0)
        bhv_tree->pos.x = 0;
        bhv_tree->pos.y = 0;
        bhv_tree->pos.z = 0;
        bhv_tree->pos.w = 1;

        // set radius of node to maximum
        bhv_tree->radius.x = universe_radius;
        bhv_tree->radius.y = universe_radius;
        bhv_tree->radius.z = universe_radius;
        bhv_tree->radius.w = 1;

        Write_Error("\nnode pos[%f, %f, %f], r[%f, %f, %f]", 
            bhv_tree->pos.x,bhv_tree->pos.y,bhv_tree->pos.z,
            bhv_tree->radius.x,bhv_tree->radius.y,bhv_tree->radius.z);

        // build root, simply add all objects to root
        for (int index = 0; index < num_objects; index++)
        {
            // make sure object is not culled 
            if (!(bhv_objects[index].state & OBJECT4DV2_STATE_CULLED))
            {
                bhv_tree->objects[bhv_tree->num_objects++] = (OBJ_CONTAINERV1_PTR)&bhv_objects[index];
            } // end if

        } // end for index

        // at this point all the objects have been inserted into the root node
        // and num_objects is set to the number of objects
        // enable the node
        bhv_tree->state = 1;
        bhv_tree->attr  = 0;

        // and set all links to NULL
        for (int ilink = 0; ilink < MAX_BHV_PER_NODE; ilink++)
            bhv_tree->links[ilink] = NULL;

        // set the number of objects
        bhv_tree->num_objects = num_objects;

        Write_Error("\nInserted %d objects into root node", bhv_tree->num_objects);
        Write_Error("\nMaking recursive call with root node...");

        // done, so now allow recursion to build the rest of the tree
        BHV_Build_Tree(bhv_tree, 
            bhv_objects, 
            num_objects, 
            1,
            num_divisions,
            universe_radius);
    } // end if
    else
    {
        Write_Error("\nEntering Level = %d > 0 block, number of objects = %d",level, bhv_tree->num_objects);

        // test for exit state
        if (bhv_tree->num_objects <= MIN_OBJECTS_PER_BHV_CELL)
            return;

        // building a child node (hard part)
        // we must take the current node and split it into a number of children nodes, and then
        // create a bvh for each child and then insert all the objects into each child
        // the for each child call the recursion again....

        // create 3D cell temp storage to track cells
        BHV_CELL cells[MAX_BHV_CELL_DIVISIONS][MAX_BHV_CELL_DIVISIONS][MAX_BHV_CELL_DIVISIONS];

        // find origin of bounding volume based on radius and center
        int x0 = bhv_tree->pos.x - bhv_tree->radius.x;
        int y0 = bhv_tree->pos.y - bhv_tree->radius.y;
        int z0 = bhv_tree->pos.z - bhv_tree->radius.z;

        // compute cell sizes on x,y,z axis
        float cell_size_x = 2*bhv_tree->radius.x / (float)num_divisions;
        float cell_size_y = 2*bhv_tree->radius.y / (float)num_divisions;
        float cell_size_z = 2*bhv_tree->radius.z / (float)num_divisions;

        Write_Error("\ncell pos=(%d, %d, %d) size=(%f, %f, %f)", x0,y0,z0, cell_size_x, cell_size_y, cell_size_z);

        int cell_x, cell_y, cell_z; // used to locate cell in 3D matrix

        // clear cell memory out
        memset(cells, 0, sizeof(cells));

        // now partition space up into num_divisions (must be < MAX_BHV_CELL_DIVISIONS)
        // and then sort each object's center into each cell of the 3D sorting matrix
        for (int obj_index = 0; obj_index < bhv_tree->num_objects; obj_index++)
        {
            // compute cell position in temp sorting matrix
            cell_x = (bhv_tree->objects[obj_index]->pos.x - x0)/cell_size_x;
            cell_y = (bhv_tree->objects[obj_index]->pos.y - y0)/cell_size_y;
            cell_z = (bhv_tree->objects[obj_index]->pos.z - z0)/cell_size_z;

            // insert this object into list
            cells[cell_x][cell_y][cell_z].obj_list[ cells[cell_x][cell_y][cell_z].num_objects ] = bhv_tree->objects[obj_index];

            Write_Error("\ninserting object %d located at (%f, %f, %f) into cell (%d, %d, %d)", obj_index, 
                bhv_tree->objects[obj_index]->pos.x,
                bhv_tree->objects[obj_index]->pos.y,
                bhv_tree->objects[obj_index]->pos.z,
                cell_x, cell_y, cell_z);

            // increment number of objects in this cell
            if (++cells[cell_x][cell_y][cell_z].num_objects >= MAX_OBJECTS_PER_BHV_CELL)
                cells[cell_x][cell_y][cell_z].num_objects  = MAX_OBJECTS_PER_BHV_CELL-1;   

        } // end for obj_index

        Write_Error("\nEntering sorting section...");

        // now the 3D sorting matrix has all the information we need, the next step
        // is to create a BHV node for each non-empty

        for (int icell_x = 0; icell_x < num_divisions; icell_x++)
        {
            for (int icell_y = 0; icell_y < num_divisions; icell_y++)
            {
                for (int icell_z = 0; icell_z < num_divisions; icell_z++)
                {
                    // are there any objects in this node?
                    if ( cells[icell_x][icell_y][icell_z].num_objects > 0)
                    {
                        Write_Error("\nCell %d, %d, %d contains %d objects", icell_x, icell_y, icell_z, 
                            cells[icell_x][icell_y][icell_z].num_objects);

                        Write_Error("\nCreating child node...");

                        // create a node and set the link to it
                        bhv_tree->links[ bhv_tree->num_children ] = (BHV_NODEV1_PTR)malloc(sizeof(BHV_NODEV1));

                        // zero node out
                        memset(bhv_tree->links[ bhv_tree->num_children ], 0, sizeof(BHV_NODEV1) );

                        // set the node up
                        BHV_NODEV1_PTR curr_node = bhv_tree->links[ bhv_tree->num_children ];

                        // position
                        curr_node->pos.x = (icell_x*cell_size_x + cell_size_x/2) + x0;
                        curr_node->pos.y = (icell_y*cell_size_y + cell_size_y/2) + y0;
                        curr_node->pos.z = (icell_z*cell_size_z + cell_size_z/2) + z0;
                        curr_node->pos.w = 1;

                        // radius is cell_size / 2
                        curr_node->radius.x = cell_size_x/2;
                        curr_node->radius.y = cell_size_y/2;
                        curr_node->radius.z = cell_size_z/2;
                        curr_node->radius.w = 1;

                        // set number of objects
                        curr_node->num_objects = cells[icell_x][icell_y][icell_z].num_objects;

                        // set num children
                        curr_node->num_children = 0;

                        // set state and attr
                        curr_node->state        = 1; // enable node               
                        curr_node->attr         = 0;

                        // now insert each object into this node's object list
                        for (int icell_index = 0; icell_index < curr_node->num_objects; icell_index++)
                        {
                            curr_node->objects[icell_index] = cells[icell_x][icell_y][icell_z].obj_list[icell_index];    
                        } // end for icell_index

                        Write_Error("\nChild node pos=(%f, %f, %f), r=(%f, %f, %f)", 
                            curr_node->pos.x,curr_node->pos.y,curr_node->pos.z,
                            curr_node->radius.x,curr_node->radius.y,curr_node->radius.z);

                        // increment number of children of parent
                        bhv_tree->num_children++;

                    } // end if

                } // end for icell_z

            } // end for icell_y

        } // end for icell_x

        Write_Error("\nParent has %d children..", bhv_tree->num_children);

        // now for each child build a BHV
        for (int inode = 0; inode < bhv_tree->num_children; inode++)
        {
            Write_Error("\nfor Level %d, creating child %d", level, inode);

            BHV_Build_Tree(bhv_tree->links[inode], 
                NULL, // unused now
                NULL, // unused now
                level+1, 
                num_divisions,
                universe_radius);

        } // end if     

    } // end else level > 0

    Write_Error("\nExiting BHV...level = %d", level);

} // end BHV_Build_Tree
