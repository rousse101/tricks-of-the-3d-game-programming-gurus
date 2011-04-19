// T3DLIB6.CPP - low level line and triangle rendering code

// I N C L U D E S ///////////////////////////////////////////////////////////

#define DEBUG_ON

#define WIN32_LEAN_AND_MEAN  

#include "CommonHeader.h"

#include <ddraw.h>      // needed for defs in T3DLIB1.H 
#include "T3DLIB1.H"
#include "T3DLIB4.H"
#include "T3DLIB5.H"
#include "T3DLIB6.H"

// GLOBALS //////////////////////////////////////////////////////////////////

MATV1 materials[MAX_MATERIALS]; // materials in system
int num_materials;              // current number of materials

LIGHTV1 lights[MAX_LIGHTS];  // lights in system
int num_lights;              // current number of lights

// these look up tables are used by the 8-bit lighting engine
// the first one holds a color translation table in the form of each
// row is a color 0..255, and each row consists of 256 shades of that color
// the data in each row is the color/intensity indices and the resulting value
// is an 8-bit index into the real color lookup that should be used as the color

// the second table works by each index being a compressed 16bit RGB value
// the data indexed by that RGB value IS the index 0..255 of the real
// color lookup that matches the desired color the closest

UCHAR rgbilookup[256][256];         // intensity RGB 8-bit lookup storage
UCHAR rgblookup[65536];             // RGB 8-bit color lookup

// FUNCTIONS ////////////////////////////////////////////////////////////////

void Draw_OBJECT4DV1_Solid(OBJECT4DV1_PTR obj, 
                           UCHAR *video_buffer, int lpitch)

{
    // this function renders an object to the screen in solid, 
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

        // draw the triangle
        Draw_Triangle_2D(obj->vlist_trans[ vindex_0 ].x, obj->vlist_trans[ vindex_0 ].y,
            obj->vlist_trans[ vindex_1 ].x, obj->vlist_trans[ vindex_1 ].y,
            obj->vlist_trans[ vindex_2 ].x, obj->vlist_trans[ vindex_2 ].y,
            obj->plist[poly].color, video_buffer, lpitch);

    } // end for poly

} // end Draw_OBJECT4DV1_Solid

///////////////////////////////////////////////////////////////

void Draw_RENDERLIST4DV1_Solid(RENDERLIST4DV1_PTR rend_list, 
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

        // draw the triangle
        Draw_Triangle_2D(rend_list->poly_ptrs[poly]->tvlist[0].x, rend_list->poly_ptrs[poly]->tvlist[0].y,
            rend_list->poly_ptrs[poly]->tvlist[1].x, rend_list->poly_ptrs[poly]->tvlist[1].y,
            rend_list->poly_ptrs[poly]->tvlist[2].x, rend_list->poly_ptrs[poly]->tvlist[2].y,
            rend_list->poly_ptrs[poly]->color, video_buffer, lpitch);

    } // end for poly

} // end Draw_RENDERLIST4DV1_Solid

/////////////////////////////////////////////////////////////

void Draw_OBJECT4DV1_Solid16(OBJECT4DV1_PTR obj, 
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

        // draw the triangle
        Draw_Triangle_2D16(obj->vlist_trans[ vindex_0 ].x, obj->vlist_trans[ vindex_0 ].y,
            obj->vlist_trans[ vindex_1 ].x, obj->vlist_trans[ vindex_1 ].y,
            obj->vlist_trans[ vindex_2 ].x, obj->vlist_trans[ vindex_2 ].y,
            obj->plist[poly].color, video_buffer, lpitch);

    } // end for poly

} // end Draw_OBJECT4DV1_Solid16

///////////////////////////////////////////////////////////////

void Draw_RENDERLIST4DV1_Solid16(RENDERLIST4DV1_PTR rend_list, 
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

        // draw the triangle
        Draw_Triangle_2D16(rend_list->poly_ptrs[poly]->tvlist[0].x, rend_list->poly_ptrs[poly]->tvlist[0].y,
            rend_list->poly_ptrs[poly]->tvlist[1].x, rend_list->poly_ptrs[poly]->tvlist[1].y,
            rend_list->poly_ptrs[poly]->tvlist[2].x, rend_list->poly_ptrs[poly]->tvlist[2].y,
            rend_list->poly_ptrs[poly]->color, video_buffer, lpitch);

    } // end for poly

} // end Draw_RENDERLIST4DV1_Solid16

// CLASS CPARSERV1 IMPLEMENTATION /////////////////////////////////////////

// constructor /////////////////////////////////////////////////

CPARSERV1::CPARSERV1()
{
#ifdef PARSER_DEBUG_ON
    printf("\nEntering CPARSERV1() constructor.");
#endif

    // reset file system
    fstream = NULL;
    Reset();

} // end constructor

// destructor ///////////////////////////////////////////////////

CPARSERV1::~CPARSERV1() 
{ 
#ifdef PARSER_DEBUG_ON
    printf("\nEntering ~CPARSERV1() destructor.");
#endif
    Reset();

} // end destructor

// reset file system ////////////////////////////////////////////
int CPARSERV1::Reset()
{
#ifdef PARSER_DEBUG_ON
    printf("\nEntering Reset().");
#endif

    // reset file buffer
    if (fstream)
        fclose(fstream);

    fstream = NULL;

    // clear and reset buffer
    memset(buffer, 0, sizeof(buffer));
    length    = 0;
    num_lines = 0;

    // set comment
    strcpy(comment, PARSER_DEFAULT_COMMENT);

    return(1);

} // end Reset

// open file /////////////////////////////////////////////////////

int CPARSERV1::Open(char *filename)
{
#ifdef PARSER_DEBUG_ON
    printf("\nEntering Open().");
#endif

    // reset file system
    Reset();

    // opens a file
    if ((fstream = fopen(filename, "r"))!=NULL) 
    {
#ifdef PARSER_DEBUG_ON
        printf("\nOpening file: %s", filename);
#endif   
        return(1);
    } // end if
    else
    {
#ifdef PARSER_DEBUG_ON
        printf("\nCouldn't open file: %s", filename);
#endif
        return(0);
    } // end else

} // end Open

// close file ////////////////////////////////////////////////////
int CPARSERV1::Close()
{
    return(Reset());
} // end Close

// get line //////////////////////////////////////////////////////

char *CPARSERV1::Getline(int mode)
{
#ifdef PARSER_DEBUG_ON
    printf("\nEntering Getline().");
#endif

    char *string;

    // gets a single line from the stream
    if (fstream)
    {
        // check translation mode
        if (mode & PARSER_STRIP_EMPTY_LINES)
        {
            // get lines until we get a real one with data on it
            while(1)
            {      
                // have we went to the end of the file without getting anything?
                if ((string = fgets(buffer,PARSER_BUFFER_SIZE, fstream))==NULL)
                    break;

                // we have something, strip ws from it
                int slength = strlen(string);
                int sindex = 0;

                // eat up space
                while(isspace(string[sindex]))
                    sindex++;

                // is there anything left?
                if ((slength - sindex) > 0)
                {
                    // copy the string into place
                    memmove((void *)buffer, (void *)&string[sindex],  (slength - sindex)+1 );
                    string = buffer;
                    slength = strlen(string); 

                    // strip comments also?
                    if (mode & PARSER_STRIP_COMMENTS)
                    {
                        // does this begin with a comment or end with a comment?
                        char *comment_string = strstr(string, comment);

                        // 3 cases, no comment, comment at beginning, comment at end
                        if (comment_string == NULL)
                            break; // line is valid exit with line

                        // compute index into string from beginning where comment begins
                        int cindex = (int)(comment_string - string);

                        // comment at beginning then continue

                        if (cindex == 0)
                            continue; // this line is a comment, ignore completely, get another
                        else
                        {
                            // comment at end, strip it, insert null where it begins
                            comment_string[0] = 0;
                            break;
                        } // end else

                    } // end if 

                    // exit loop, we have something :)
                    break;
                } // end if

            } // end while

        } // end if strip mode
        else
        { 
            // just get the next line, don't worry about stripping anything
            string = fgets(buffer,PARSER_BUFFER_SIZE, fstream);
        } // end else 


        // was the line valid?
        if (string)
        {
            // increment line count
            num_lines++;

            // final stripping of whitspace
            if (mode & PARSER_STRIP_WS_ENDS)
            {
                StringLtrim(buffer);
                StringRtrim(buffer);
            } // end if         

            // compute line length
            length = strlen(buffer);

#ifdef PARSER_DEBUG_ON
            printf("\nString[%d]:%s", length, string);
#endif
            // return the pointer, copy of data already in buffer
            return(string);     

        } // end if
        else
        {
#ifdef PARSER_DEBUG_ON
            printf("\nEOF");
#endif
            return(NULL);
        }

    } // end if
    else
    {
#ifdef PARSER_DEBUG_ON
        printf("\nFstream NULL.");
#endif
        return(NULL);
    } // end else

} // end Getline

// sets the comment string ///////////////////////////////////////

int CPARSERV1::SetComment(char *string)
{
    // sets the comment string
    if (strlen(string) < PARSER_MAX_COMMENT)
    {
        strcpy(comment, string);
        return(1);
    } // end if
    else
        return(0);

} // end SetComment

// find pattern in line //////////////////////////////////////////

int CPARSERV1::Pattern_Match(char *string, char *pattern, ...)
{
    // this function tries to match the pattern sent in pattern with
    // the sent string, the results are sent back to the sender in the
    // variable arguments as well as stored in the parameter passing area

    // string literal                        = ['string']
    // floating point number                 = [f]
    // integer number                        = [i]
    // match a string exactly ddd chars      = [s=ddd] 
    // match a string less than ddd chars    = [s<ddd] 
    // match a string greater than ddd chars = [s>ddd]
    // for example to match "vertex: 34.234 56.34 12.4
    // ['vertex'] [f] [f] [f]

    char token_type[PATTERN_MAX_ARGS];         // type of token, f,i,s,l
    char token_string[PATTERN_MAX_ARGS][PATTERN_BUFFER_SIZE];   // for literal strings this holds them
    char token_operator[PATTERN_MAX_ARGS];     // holds and operators for the token, >, <, =, etc.
    int  token_numeric[PATTERN_MAX_ARGS];      // holds any numeric data to qualify the token

    char buffer[PARSER_BUFFER_SIZE]; // working buffer

    // a little error testing
    if ( (!string || strlen(string)==0) || (!pattern || strlen(pattern)==0) )
        return(0);

    // copy line into working area
    strcpy(buffer, string);

    int tok_start  = 0, 
        tok_end    = 0,
        tok_restart = 0,
        tok_first_pass = 0,
        num_tokens = 0;

    // step 1: extract token list
    while(1)
    {
        // eat whitepace
        while(isspace(pattern[tok_start]) )
            tok_start++;

        // end of line?
        if (tok_start >= strlen(pattern))
            break;    

        // look for beginning of token '['
        if (pattern[tok_start] == '[')
        {
            // now look for token code
            switch(pattern[tok_start+1])
            {
            case PATTERN_TOKEN_FLOAT:  // float
                {
                    // make sure token is well formed
                    if (pattern[tok_start+2]!=']')
                        return(0); // error

                    // advance token scanner
                    tok_start+=3;

                    // insert a float into pattern
                    token_type[num_tokens] = PATTERN_TOKEN_FLOAT;  // type of token, f,i,s,l
                    strcpy(token_string[num_tokens],"");           // for literal strings this holds them
                    token_operator[num_tokens] = 0;                // holds and operators for the token, >, <, =, etc.
                    token_numeric[num_tokens]  = 0; 

                    // increment number of tokens
                    num_tokens++;

#ifdef PARSER_DEBUG_ON
                    printf("\nFound Float token");
#endif
                } break;

            case PATTERN_TOKEN_INT:  // integer
                {
                    // make sure token is well formed
                    if (pattern[tok_start+2]!=']')
                        return(0); // error

                    // advance token scanner
                    tok_start+=3;

                    // insert a int into pattern
                    token_type[num_tokens] = PATTERN_TOKEN_INT;  // type of token, f,i,s,l
                    strcpy(token_string[num_tokens],"");         // for literal strings this holds them
                    token_operator[num_tokens] = 0;              // holds and operators for the token, >, <, =, etc.
                    token_numeric[num_tokens]  = 0; 

                    // increment number of tokens
                    num_tokens++;

#ifdef PARSER_DEBUG_ON
                    printf("\nFound Int token");
#endif

                } break;

            case PATTERN_TOKEN_LITERAL: // literal string
                {
                    // advance token scanner to begining literal string
                    tok_start+=2;
                    tok_end = tok_start;

                    // eat up string
                    while(pattern[tok_end]!=PATTERN_TOKEN_LITERAL)
                        tok_end++;

                    // make sure string is well formed
                    if (pattern[tok_end+1]!=']')
                        return(0);

                    // insert a string into pattern              

                    // literal string lies from (tok_start - (tok_end-1)
                    memcpy(token_string[num_tokens], &pattern[tok_start], (tok_end - tok_start) );
                    token_string[num_tokens][(tok_end - tok_start)] = 0; // null terminate

                    token_type[num_tokens] = PATTERN_TOKEN_LITERAL;  // type of token, f,i,s,'
                    token_operator[num_tokens] = 0;                 // holds and operators for the token, >, <, =, etc.
                    token_numeric[num_tokens]  = 0; 

#ifdef PARSER_DEBUG_ON
                    printf("\nFound Literal token = %s",token_string[num_tokens]);
#endif

                    // advance token scanner
                    tok_start = tok_end+2;

                    // increment number of tokens
                    num_tokens++;

                } break;

            case PATTERN_TOKEN_STRING: // ascii string varying length
                {
                    // look for comparator
                    if (pattern[tok_start+2] == '=' || 
                        pattern[tok_start+2] == '>' ||
                        pattern[tok_start+2] == '<' )
                    {
                        // extract the number
                        tok_end = tok_start+3;

                        while( isdigit(pattern[tok_end] ) )
                            tok_end++;

                        // check for well formed
                        if (pattern[tok_end]!=']')
                            return(0);

                        // copy number in ascii to string and convert to real number
                        memcpy(buffer, &pattern[tok_start+3], (tok_end - tok_start) );
                        buffer[tok_end - tok_start] = 0;

                        // insert a string into pattern
                        token_type[num_tokens] = PATTERN_TOKEN_STRING;     // type of token, f,i,s,l
                        strcpy(token_string[num_tokens],"");               // for literal strings this holds them
                        token_operator[num_tokens] = pattern[tok_start+2]; // holds and operators for the token, >, <, =, etc.
                        token_numeric[num_tokens]  = atoi(buffer);

                    } // end if
                    else
                        return(0); // not well formed

#ifdef PARSER_DEBUG_ON
                    printf("\nFound String token, comparator: %c, characters: %d", token_operator[num_tokens], token_numeric[num_tokens]);
#endif
                    // advance token scanner
                    tok_start = tok_end+1; 

                    // increment number of tokens
                    num_tokens++;

                } break;             

            default: break;

            } // end switch

        } // end if

        // end of line?
        if (tok_start >= strlen(pattern))
            break;    

    } // end while

#ifdef PARSER_DEBUG_ON
    printf("\nstring to parse: %s", string);
    printf("\nPattern to scan for: %s", pattern);
    printf("\nnumber of tokens found %d", num_tokens);
#endif

    // at this point we have the pattern we need to look for, so look for it
    int pattern_state = PATTERN_STATE_INIT; // initial state for pattern recognizer
    int curr_tok = 0;                 // test for num_tokens
    char token[PATTERN_BUFFER_SIZE];  // token under consideration

    // enter scan state machine
    while(1)
    {
        switch(pattern_state)
        {
        case PATTERN_STATE_INIT:
            {
                // initial state for pattern
                strcpy(buffer, string);

                tok_start      = 0; 
                tok_end        = 0;
                tok_restart    = 0;
                tok_first_pass = 1;
                curr_tok       = 0;

                // reset output arrays
                num_pints = num_pfloats = num_pstrings = 0;

                // transition to restart         
                pattern_state = PATTERN_STATE_RESTART;

            } break;

        case PATTERN_STATE_RESTART:
            {
                // pattern may still be here?
                curr_tok       = 0;
                tok_first_pass = 1;

                // error detection
                if (tok_end >= strlen(buffer))
                    return(0);

                // restart scanner after first token from last pass
                tok_start = tok_end = tok_restart;

                // start validating tokens
                pattern_state = PATTERN_STATE_NEXT;

            } break;

        case PATTERN_STATE_NEXT:
            {
                // have we matched pattern yet?
                if (curr_tok >= num_tokens)
                {   
                    pattern_state = PATTERN_STATE_MATCH;       
                }
                else
                {
                    // get next token
                    if (tok_end >= strlen(buffer))
                        return(0);

                    tok_start = tok_end;
                    while(isspace(buffer[tok_start])) tok_start++;
                    tok_end = tok_start;

                    while(!isspace(buffer[tok_end]) && tok_end < strlen(buffer)) tok_end++;

                    // copy token
                    memcpy(token, &buffer[tok_start], tok_end - tok_start);
                    token[tok_end - tok_start] = 0;

                    // check for error
                    if (strlen(token)==0) 
                        return(0);

                    // remember position of first token, so we can restart after it on next pass
                    // if need
                    if (tok_first_pass)
                    {
                        tok_first_pass = 0; 
                        tok_restart = tok_end;
                    } // end if

                    // we have the token, set state to check for that token
                    switch(token_type[curr_tok])
                    {
                    case PATTERN_TOKEN_FLOAT:
                        {
                            pattern_state = PATTERN_STATE_FLOAT;
                        } break;
                    case PATTERN_TOKEN_INT:    
                        {
                            pattern_state = PATTERN_STATE_INT;
                        } break;
                    case PATTERN_TOKEN_STRING: 
                        {
                            pattern_state = PATTERN_STATE_STRING;
                        } break;
                    case PATTERN_TOKEN_LITERAL:
                        {
                            pattern_state = PATTERN_STATE_LITERAL;
                        } break;

                    default: break;

                    } // end switch

                } // end else

            } break;

        case PATTERN_STATE_FLOAT:
            {
                // simply validate this token as a float
                float f = IsFloat(token);

                if (f!=FLT_MIN)
                {
                    pfloats[num_pfloats++] = f;

                    // get next token
                    curr_tok++;
                    pattern_state = PATTERN_STATE_NEXT;
                } // end if                
                else
                {
                    // error pattern doesn't match, restart
                    pattern_state = PATTERN_STATE_RESTART;
                } // end else

            } break;

        case PATTERN_STATE_INT:
            {
                // simply validate this token as a int
                int i = IsInt(token);

                if (i!=INT_MIN)
                {
                    pints[num_pints++] = i;

                    // get next token
                    curr_tok++;
                    pattern_state = PATTERN_STATE_NEXT;
                } // end if                
                else
                {
                    // error pattern doesn't match, restart
                    pattern_state = PATTERN_STATE_RESTART;
                } // end else

            } break;

        case PATTERN_STATE_LITERAL:
            {
                // simply validate this token by comparing to data in table
                if (strcmp(token, token_string[curr_tok]) == 0)
                {
                    // increment number of pstrings found and insert into table
                    strcpy(pstrings[num_pstrings++], token);

                    // get next token
                    curr_tok++;
                    pattern_state = PATTERN_STATE_NEXT;
                } // end if                
                else
                {
                    // error pattern doesn't match, restart
                    pattern_state = PATTERN_STATE_RESTART;
                } // end else

            } break;

        case PATTERN_STATE_STRING:
            {
                // need to test for non-space chars 
                // get comparator

                switch(token_operator[curr_tok])
                {
                case '=':
                    {
                        // we need exactly
                        if (strlen(token) == token_numeric[curr_tok])
                        {
                            // put this string into table
                            strcpy(pstrings[num_pstrings++], token);

                            // get next token
                            curr_tok++;
                            pattern_state = PATTERN_STATE_NEXT;
                        } // end if    
                        else
                        {
                            // error pattern doesn't match, restart
                            pattern_state = PATTERN_STATE_RESTART;
                        } // end else

                    } break; 

                case '>':
                    {
                        // we need greater than
                        if (strlen(token) > token_numeric[curr_tok])
                        {
                            // put this string into table
                            strcpy(pstrings[num_pstrings++], token);

                            // get next token
                            curr_tok++;
                            pattern_state = PATTERN_STATE_NEXT;
                        } // end if  
                        else
                        {
                            // error pattern doesn't match, restart
                            pattern_state = PATTERN_STATE_RESTART;
                        } // end else

                    } break; 

                case '<':
                    {
                        // we need less than
                        if (strlen(token) < token_numeric[curr_tok])
                        {
                            // put this string into table
                            strcpy(pstrings[num_pstrings++], token);

                            // get next token
                            curr_tok++;
                            pattern_state = PATTERN_STATE_NEXT;
                        } // end if    
                        else
                        {
                            // error pattern doesn't match, restart
                            pattern_state = PATTERN_STATE_RESTART;
                        } // end else

                    } break; 

                default: break;

                } // end switch

            } break;

        case PATTERN_STATE_MATCH:
            {
                // we have matched the string, output vars into variable arg list

#ifdef PARSER_DEBUG_ON
                printf("\nPattern: %s matched!", pattern);
#endif

                return(1);
            } break;

        case PATTERN_STATE_END: { } break;

        default: break;

        } // end switch            

    } // end while

} // end Pattern_Match

// END IMPLEMENTATION OF CPARSERV1 CLASS ///////////////////////////////////

int ReplaceChars(char *string_in, char *string_out, char *replace_chars, char rep_char, int case_on)
{
    // this function simply replaces the characters from the input string that
    // are listed in replace with the replace char, the results are stored in 
    // string_out, string_in and isn't touched, the number of replacments is 
    // returned. if case_on = 1 then case is checked, other it's case insensitive

    int num_replacements = 0,  // tracks number of characters replaced
        index_in     = 0,      // curr index into input
        index_out    = 0,      // curr index into output
        sindex,                // loop var into strip array
        slength = strlen(replace_chars); // length of strip string

    // do some error checking
    if (!string_in || !string_out || strlen(string_in) == 0)
        return(0);

    // nothing to replace
    if (!replace_chars || strlen(replace_chars)==0)
    {
        strcpy(string_out, string_in);
        return(0);
    } // end if

    // determine if case is important
    if (case_on==1)
    {
        // perform char by char copy
        while(string_in[index_in])
        {
            for (sindex = 0; sindex < slength; sindex++)
                if (string_in[index_in] == replace_chars[sindex])
                {
                    // replace it
                    string_out[index_out++] = rep_char;
                    index_in++;
                    num_replacements++;
                    break;
                } // end if

                // was a replacement performed?, no just copy then
                if (sindex >= slength)
                    string_out[index_out++] = string_in[index_in++];

        } // end while
    } // end if case_on
    else
    {
        // perform char by char copy with case insensitivity
        while(string_in[index_in])
        {
            for (sindex = 0; sindex < slength; sindex++)
                if (toupper(string_in[index_in]) == toupper(replace_chars[sindex]))
                {
                    // replace it
                    string_out[index_out++] = rep_char;
                    index_in++;
                    num_replacements++;
                    break;
                } // end if

                // was a strip char found?
                if (sindex >= slength)
                    string_out[index_out++] = string_in[index_in++];

        } // end while
    } // end if case_off

    // terminate output string
    string_out[index_out] = 0;

    // return extracts
    return(num_replacements);

} // end ReplaceChars

//////////////////////////////////////////////////////////////////////////////////

int StripChars(char *string_in, char *string_out, char *strip_chars, int case_on)
{
    // this function simply strips/extracts the characters from the input string that
    // are listed in strip, the results are stored in string_out, string_in
    // isn't touched, the number of extractions or returned
    // if case_on = 1 then case is checked, other it's case insensitive

    int num_extracts = 0,  // tracks number of characters extracted
        index_in     = 0,  // curr index into input
        index_out    = 0,  // curr index into output
        sindex,            // loop var into strip array
        slength = strlen(strip_chars); // length of strip string

    // do some error checking
    if (!string_in || !string_out || strlen(string_in) == 0)
        return(0);

    // nothing to replace
    if (!strip_chars || strlen(strip_chars)==0)
    {
        strcpy(string_out, string_in);
        return(0);
    } // end if

    // determine if case is importants
    if (case_on==1)
    {
        // perform char by char copy
        while(string_in[index_in])
        {
            for (sindex = 0; sindex < slength; sindex++)
                if (string_in[index_in] == strip_chars[sindex])
                {
                    // jump over input char, it's stripped
                    index_in++;
                    num_extracts++;
                    break;
                } // end if

                // was a strip char found?
                if (sindex >= slength)
                    string_out[index_out++] = string_in[index_in++];

        } // end while
    } // end if case_on
    else
    {
        // perform char by char copy with case insensitivity
        while(string_in[index_in])
        {
            for (sindex = 0; sindex < slength; sindex++)
                if (toupper(string_in[index_in]) == toupper(strip_chars[sindex]))
                {
                    // jump over input char, it's stripped
                    index_in++;
                    num_extracts++;
                    break;
                } // end if

                // was a strip char found?
                if (sindex >= slength)
                    string_out[index_out++] = string_in[index_in++];

        } // end while
    } // end if case_off

    // terminate output string
    string_out[index_out] = 0;

    // return extracts
    return(num_extracts);

} // end StripChars

////////////////////////////////////////////////////////////////////////////

int IsInt(char *istring)
{
    // validates the sent string as a int and converts it, if it's not valid
    // the function sends back INT_MIN, the chances of this being the number
    // validated is slim
    // [whitespace] [sign]digits

    char *string = istring;

    // must be of the form
    // [whitespace] 
    while(isspace(*string)) string++;

    // [sign] 
    if (*string=='+' || *string=='-') string++;

    // [digits] 
    while(isdigit(*string)) string++;

    // the string better be the same size as the other one
    if (strlen(istring) == (int)(string - istring))
        return(atoi(istring));
    else
        return(INT_MIN);

} // end IsInt

//////////////////////////////////////////////////////////////////////////////

float IsFloat(char *fstring)
{
    // validates the sent string as a float and converts it, if it's not valid
    // the function sends back FLT_MIN, the chances of this being the number
    // validated is slim
    // [whitespace] [sign] [digits] [.digits] [ {d | D | e | E }[sign]digits]

    char *string = fstring;

    // must be of the form
    // [whitespace] 
    while(isspace(*string)) string++;

    // [sign] 
    if (*string=='+' || *string=='-') string++;

    // [digits] 
    while(isdigit(*string)) string++;

    // [.digits] 
    if (*string =='.') 
    {
        string++;
        while(isdigit(*string)) string++;
    }

    // [ {d | D | e | E }[sign]digits]
    if (*string =='e' || *string == 'E' || *string =='d' || *string == 'D')
    {
        string++;

        // [sign] 
        if (*string=='+' || *string=='-') string++;

        // [digits] 
        while(isdigit(*string)) string++;
    } 

    // the string better be the same size as the other one
    if (strlen(fstring) == (int)(string - fstring))
        return(atof(fstring));
    else
        return(FLT_MIN);

} // end IsFloat

////////////////////////////////////////////////////////////////////////////

char *StringRtrim(char *string)
{
    // trims whitespace from right side, note is destructive
    int sindex = 0;

    int slength = strlen(string);

    if (!string || slength == 0) return(string);

    // index to end of string
    sindex = slength - 1;

    // trim whitespace by overwriting nulls
    while( isspace(string[sindex]) && sindex >= 0)
        string[sindex--] = 0;

    // string doens't need to be moved, so simply return pointer
    return(string);

} // end StringRtrim

////////////////////////////////////////////////////////////////////////////

char *StringLtrim(char *string)
{
    // trims whitespace from left side, note is destructive
    int sindex = 0;

    int slength = strlen(string);

    if (!string || slength == 0) return(string);

    // trim whitespace by advancing pointer
    while(isspace(string[sindex]) && sindex < slength)
        string[sindex++] = 0; // not needed actually

    // copy string to left
    memmove((void *)string, (void *)&string[sindex], (slength - sindex)+1);

    // now return pointer
    return(string);

} // end StringLtrim

////////////////////////////////////////////////////////////////////////////

int Convert_Bitmap_8_16(BITMAP_FILE_PTR bitmap)
{
    // function converts a bitmap from 8 bit to 16 bit based on the palette
    // and the current bitformat

    // is this a valid bitmap file?
    if (!bitmap || !bitmap->buffer)
        return(0);

    // cache vars
    int bwidth = bitmap->bitmapinfoheader.biWidth;
    int bheight = bitmap->bitmapinfoheader.biHeight;

    // allocate temporary buffer
    USHORT *buffer16 = (USHORT *)malloc(bwidth * bheight * 2);

    // for each pixel in the bitmap convert it to a 16 bit value
    for (int pixel = 0; pixel < bwidth*bheight; pixel++)
    {
        // get 8-bit color index
        int cindex = bitmap->buffer[pixel];

        // build 16-bit pixel with extractd RGB values for the pixel in palette
        buffer16[pixel] = RGB16Bit(bitmap->palette[cindex].peRed, 
            bitmap->palette[cindex].peGreen,
            bitmap->palette[cindex].peBlue);
    } // end for

    // now change file information, actually not much has changed, but what the heck
    bitmap->bitmapinfoheader.biBitCount     = 16; 
    bitmap->bitmapinfoheader.biSizeImage    = bwidth*bheight*2;
    bitmap->bitmapinfoheader.biWidth        = bwidth;
    bitmap->bitmapinfoheader.biHeight       = bheight;
    bitmap->bitmapinfoheader.biClrUsed      = 0;
    bitmap->bitmapinfoheader.biClrImportant = 0;

    // finally copy temp buffer
    free( bitmap->buffer );

    // link new buffer
    bitmap->buffer = (UCHAR *)buffer16;

#if 1
    // write the file info out 
    printf("\nsize=%d \nwidth=%d \nheight=%d \nbitsperpixel=%d \ncolors=%d \nimpcolors=%d",
        bitmap->bitmapinfoheader.biSizeImage,
        bitmap->bitmapinfoheader.biWidth,
        bitmap->bitmapinfoheader.biHeight,
        bitmap->bitmapinfoheader.biBitCount,
        bitmap->bitmapinfoheader.biClrUsed,
        bitmap->bitmapinfoheader.biClrImportant);
#endif

    // return success
    return(1);

} // end Convert_Bitmap_8_16

///////////////////////////////////////////////////////////////////////////////

int Load_OBJECT4DV1_3DSASC(OBJECT4DV1_PTR obj,   // pointer to object
                           char *filename,       // filename of ASC file
                           VECTOR4D_PTR scale,   // initial scaling factors
                           VECTOR4D_PTR pos,     // initial position
                           VECTOR4D_PTR rot,     // initial rotations
                           int vertex_flags)     // flags to re-order vertices
{
    // this function loads a 3D Studi .ASC file object in off disk, additionally
    // it allows the caller to scale, position, and rotate the object
    // to save extra calls later for non-dynamic objects
    // create a parser object
    CPARSERV1 parser; 

    char seps[16];          // seperators for token scanning
    char token_buffer[256]; // used as working buffer for token
    char *token;            // pointer to next token

    int r,g,b;              // working colors


    // Step 1: clear out the object and initialize it a bit
    memset(obj, 0, sizeof(OBJECT4DV1));

    // set state of object to active and visible
    obj->state = OBJECT4DV1_STATE_ACTIVE | OBJECT4DV1_STATE_VISIBLE;

    // set position of object is caller requested position
    if (pos)
    {
        // set position of object
        obj->world_pos.x = pos->x;
        obj->world_pos.y = pos->y;
        obj->world_pos.z = pos->z;
        obj->world_pos.w = pos->w;
    } // end 
    else
    {
        // set it to (0,0,0,1)
        obj->world_pos.x = 0;
        obj->world_pos.y = 0;
        obj->world_pos.z = 0;
        obj->world_pos.w = 1;
    } // end else

    // Step 2: open the file for reading using the parser
    if (!parser.Open(filename))
    {
        Write_Error("Couldn't open .ASC file %s.", filename);
        return(0);
    } // end if

    // Step 3: 

    // lets find the name of the object first 
    while(1)
    {
        // get the next line, we are looking for "Named object:"
        if (!parser.Getline(PARSER_STRIP_EMPTY_LINES | PARSER_STRIP_WS_ENDS))
        {
            Write_Error("Image 'name' not found in .ASC file %s.", filename);
            return(0);
        } // end if

        // check for pattern?  
        if (parser.Pattern_Match(parser.buffer, "['Named'] ['object:']"))
        {
            // at this point we have the string with the name in it, parse it out by finding 
            // name between quotes "name...."
            strcpy(token_buffer, parser.buffer);
            strcpy(seps, "\"");        
            strtok( token_buffer, seps );

            // this will be the token between the quotes
            token = strtok(NULL, seps);

            // copy name into structure
            strcpy(obj->name, token);          
            Write_Error("\nASC Reader Object Name: %s", obj->name);

            break;    
        } // end if

    } // end while

    // step 4: get number of vertices and polys in object
    while(1)
    {
        // get the next line, we are looking for "Tri-mesh, Vertices:" 
        if (!parser.Getline(PARSER_STRIP_EMPTY_LINES | PARSER_STRIP_WS_ENDS))
        {
            Write_Error("'Tri-mesh' line not found in .ASC file %s.", filename);
            return(0);
        } // end if

        // check for pattern?  
        if (parser.Pattern_Match(parser.buffer, "['Tri-mesh,'] ['Vertices:'] [i] ['Faces:'] [i]"))
        {
            // simply extract the number of vertices and polygons from the pattern matching 
            // output arrays
            obj->num_vertices = parser.pints[0];
            obj->num_polys    = parser.pints[1];

            Write_Error("\nASC Reader Num Vertices: %d, Num Polys: %d", 
                obj->num_vertices, obj->num_polys);
            break;    

        } // end if

    } // end while

    // Step 5: load the vertex list

    // advance parser to vertex list denoted by:
    // "Vertex list:"
    while(1)
    {
        // get next line
        if (!parser.Getline(PARSER_STRIP_EMPTY_LINES | PARSER_STRIP_WS_ENDS))
        {
            Write_Error("\n'Vertex list:' line not found in .ASC file %s.", filename);
            return(0);
        } // end if

        // check for pattern?  
        if (parser.Pattern_Match(parser.buffer, "['Vertex'] ['list:']"))
        {
            Write_Error("\nASC Reader found vertex list in .ASC file %s.", filename);
            break;
        } // end if
    } // end while

    // now read in vertex list, format:
    // "Vertex: d  X:d.d Y:d.d  Z:d.d"
    for (int vertex = 0; vertex < obj->num_vertices; vertex++)
    {
        // hunt for vertex
        while(1)
        {
            // get the next vertex
            if (!parser.Getline(PARSER_STRIP_EMPTY_LINES | PARSER_STRIP_WS_ENDS))
            {
                Write_Error("\nVertex list ended abruptly! in .ASC file %s.", filename);
                return(0);
            } // end if

            // strip all ":XYZ", make this easier, note use of input and output as same var, this is legal
            // since the output is guaranteed to be the same length or shorter as the input :)
            StripChars(parser.buffer, parser.buffer, ":XYZ");

            // check for pattern?  
            if (parser.Pattern_Match(parser.buffer, "['Vertex'] [i] [f] [f] [f]"))
            {
                // at this point we have the x,y,z in the the pfloats array locations 0,1,2
                obj->vlist_local[vertex].x = parser.pfloats[0];
                obj->vlist_local[vertex].y = parser.pfloats[1];
                obj->vlist_local[vertex].z = parser.pfloats[2];
                obj->vlist_local[vertex].w = 1;

                // do vertex swapping right here, allow muliple swaps, why not!
                // defines for vertex re-ordering flags

                //#define VERTEX_FLAGS_INVERT_X   1    // inverts the Z-coordinates
                //#define VERTEX_FLAGS_INVERT_Y   2    // inverts the Z-coordinates
                //#define VERTEX_FLAGS_INVERT_Z   4    // inverts the Z-coordinates
                //#define VERTEX_FLAGS_SWAP_YZ    8    // transforms a RHS model to a LHS model
                //#define VERTEX_FLAGS_SWAP_XZ    16   // ???
                //#define VERTEX_FLAGS_SWAP_XY    32
                //#define VERTEX_FLAGS_INVERT_WINDING_ORDER 64  // invert winding order from cw to ccw or ccw to cc

                float temp_f; // used for swapping

                // invert signs?
                if (vertex_flags & VERTEX_FLAGS_INVERT_X)
                    obj->vlist_local[vertex].x=-obj->vlist_local[vertex].x;

                if (vertex_flags & VERTEX_FLAGS_INVERT_Y)
                    obj->vlist_local[vertex].y=-obj->vlist_local[vertex].y;

                if (vertex_flags & VERTEX_FLAGS_INVERT_Z)
                    obj->vlist_local[vertex].z=-obj->vlist_local[vertex].z;

                // swap any axes?
                if (vertex_flags & VERTEX_FLAGS_SWAP_YZ)
                    SWAP(obj->vlist_local[vertex].y, obj->vlist_local[vertex].z, temp_f);

                if (vertex_flags & VERTEX_FLAGS_SWAP_XZ)
                    SWAP(obj->vlist_local[vertex].x, obj->vlist_local[vertex].z, temp_f);

                if (vertex_flags & VERTEX_FLAGS_SWAP_XY)
                    SWAP(obj->vlist_local[vertex].x, obj->vlist_local[vertex].y, temp_f);

                Write_Error("\nVertex %d = %f, %f, %f, %f", vertex,
                    obj->vlist_local[vertex].x, 
                    obj->vlist_local[vertex].y, 
                    obj->vlist_local[vertex].z,
                    obj->vlist_local[vertex].w);

                // scale vertices
                if (scale)
                {
                    obj->vlist_local[vertex].x*=scale->x;
                    obj->vlist_local[vertex].y*=scale->y;
                    obj->vlist_local[vertex].z*=scale->z;
                } // end if

                // found vertex, break out of while for next pass
                break;

            } // end if

        } // end while

    } // end for vertex

    // compute average and max radius
    Compute_OBJECT4DV1_Radius(obj);

    Write_Error("\nObject average radius = %f, max radius = %f", 
        obj->avg_radius, obj->max_radius);

    // step 6: load in the polygons
    // poly list starts off with:
    // "Face list:"
    while(1)
    {
        // get next line
        if (!parser.Getline(PARSER_STRIP_EMPTY_LINES | PARSER_STRIP_WS_ENDS))
        {
            Write_Error("\n'Face list:' line not found in .ASC file %s.", filename);
            return(0);
        } // end if

        // check for pattern?  
        if (parser.Pattern_Match(parser.buffer, "['Face'] ['list:']"))
        {
            Write_Error("\nASC Reader found face list in .ASC file %s.", filename);
            break;
        } // end if
    } // end while

    // now read each face in format:
    // Face ddd:    A:ddd B:ddd C:ddd AB:1|0 BC:1|0 CA:1|
    // Material:"rdddgdddbddda0"
    // the A, B, C part is vertex 0,1,2 but the AB, BC, CA part
    // has to do with the edges and the vertex ordering
    // the material indicates the color, and has an 'a0' tacked on the end???

    int  poly_surface_desc = 0; // ASC surface descriptor/material in this case
    int  poly_num_verts    = 0; // number of vertices for current poly (always 3)
    char tmp_string[8];         // temp string to hold surface descriptor in and
    // test if it need to be converted from hex

    for (int poly=0; poly < obj->num_polys; poly++)
    {
        // hunt until next face is found
        while(1)
        {
            // get the next polygon face
            if (!parser.Getline(PARSER_STRIP_EMPTY_LINES | PARSER_STRIP_WS_ENDS))
            {
                Write_Error("\nface list ended abruptly! in .ASC file %s.", filename);
                return(0);
            } // end if

            // strip all ":ABC", make this easier, note use of input and output as same var, this is legal
            // since the output is guaranteed to be the same length or shorter as the input :)
            StripChars(parser.buffer, parser.buffer, ":ABC");

            // check for pattern?  
            if (parser.Pattern_Match(parser.buffer, "['Face'] [i] [i] [i] [i]"))
            {
                // at this point we have the vertex indices in the the pints array locations 1,2,3, 
                // 0 contains the face number

                // insert polygon, check for winding order invert
                if (vertex_flags & VERTEX_FLAGS_INVERT_WINDING_ORDER)
                {     
                    poly_num_verts           = 3;
                    obj->plist[poly].vert[0] = parser.pints[3];
                    obj->plist[poly].vert[1] = parser.pints[2];
                    obj->plist[poly].vert[2] = parser.pints[1];
                } // end if
                else
                { // leave winding order alone
                    poly_num_verts           = 3;
                    obj->plist[poly].vert[0] = parser.pints[1];
                    obj->plist[poly].vert[1] = parser.pints[2];
                    obj->plist[poly].vert[2] = parser.pints[3];
                } // end else

                // point polygon vertex list to object's vertex list
                // note that this is redundant since the polylist is contained
                // within the object in this case and its up to the user to select
                // whether the local or transformed vertex list is used when building up
                // polygon geometry, might be a better idea to set to NULL in the context
                // of polygons that are part of an object
                obj->plist[poly].vlist = obj->vlist_local; 

                // found the face, break out of while for another pass
                break;

            } // end if

        } // end while      

        // hunt until next material for face is found
        while(1)
        {
            // get the next polygon material (the "page xxx" breaks mess everything up!!!)
            if (!parser.Getline(PARSER_STRIP_EMPTY_LINES | PARSER_STRIP_WS_ENDS))
            {
                Write_Error("\nmaterial list ended abruptly! in .ASC file %s.", filename);
                return(0);
            } // end if

            // Material:"rdddgdddbddda0"
            // replace all ':"rgba', make this easier, note use of input and output as same var, this is legal
            // since the output is guaranteed to be the same length or shorter as the input :)
            // the result will look like:
            // "M t ri l   ddd ddd ddd 0" 
            // which we can parse!
            ReplaceChars(parser.buffer, parser.buffer, ":\"rgba", ' ');

            // check for pattern?  
            if (parser.Pattern_Match(parser.buffer, "[i] [i] [i]"))
            {
                // at this point we have the red, green, and blue components in the the pints array locations 0,1,2, 
                r = parser.pints[0];
                g = parser.pints[1];
                b = parser.pints[2];

                // set all the attributes of polygon as best we can with this format
                // SET_BIT(obj->plist[poly].attr, POLY4DV1_ATTR_2SIDED);

                // we need to know what color depth we are dealing with, so check
                // the bits per pixel, this assumes that the system has already
                // made the call to DDraw_Init() or set the bit depth
                if (screen_bpp==16)
                {
                    // cool, 16 bit mode
                    SET_BIT(obj->plist[poly].attr,POLY4DV1_ATTR_RGB16);
                    obj->plist[poly].color = RGB16Bit(r, g, b);
                    Write_Error("\nPolygon 16-bit");
                } // end if 
                else
                {
                    // 8 bit mode
                    SET_BIT(obj->plist[poly].attr,POLY4DV1_ATTR_8BITCOLOR);
                    obj->plist[poly].color = RGBto8BitIndex(r,g,b, palette, 0);
                    Write_Error("\nPolygon 8-bit, index=%d", obj->plist[poly].color);
                } // end else

                // for now manually set shading mode
                //SET_BIT(obj->plist[poly].attr, POLY4DV1_ATTR_SHADE_MODE_PURE);
                //SET_BIT(obj->plist[poly].attr, POLY4DV1_ATTR_SHADE_MODE_GOURAUD);
                //SET_BIT(obj->plist[poly].attr, POLY4DV1_ATTR_SHADE_MODE_PHONG);
                SET_BIT(obj->plist[poly].attr, POLY4DV1_ATTR_SHADE_MODE_FLAT);

                // set polygon to active
                obj->plist[poly].state = POLY4DV1_STATE_ACTIVE;    

                // found the material, break out of while for another pass
                break;

            } // end if

        } // end while      

        Write_Error("\nPolygon %d:", poly);
        Write_Error("\nSurface Desc = [RGB]=[%d, %d, %d], vert_indices [%d, %d, %d]", 
            r,g,b,
            obj->plist[poly].vert[0],
            obj->plist[poly].vert[1],
            obj->plist[poly].vert[2]);

    } // end for poly

    // return success
    return(1);

} // end Load_OBJECT4DV1_3DASC

//////////////////////////////////////////////////////////////////////////////

int Load_OBJECT4DV1_COB(OBJECT4DV1_PTR obj,   // pointer to object
                        char *filename,       // filename of Caligari COB file
                        VECTOR4D_PTR scale,   // initial scaling factors
                        VECTOR4D_PTR pos,     // initial position
                        VECTOR4D_PTR rot,     // initial rotations
                        int vertex_flags)     // flags to re-order vertices 
                        // and perform transforms
{
    // this function loads a Caligari TrueSpace .COB file object in off disk, additionally
    // it allows the caller to scale, position, and rotate the object
    // to save extra calls later for non-dynamic objects, note that this function 
    // works with a OBJECT4DV1 which has no support for textures, or materials, etc, however we will
    // still parse them and get them ready for the next incarnation objects, so we can
    // re-use this code to support those features

    // create a parser object
    CPARSERV1 parser; 

    char seps[16];          // seperators for token scanning
    char token_buffer[256]; // used as working buffer for token
    char *token;            // pointer to next token

    int r,g,b;              // working colors

    // cache for texture vertices
    VERTEX2DF texture_vertices[1024];

    int num_texture_vertices = 0;

    MATRIX4X4 mat_local,  // storage for local transform if user requests it in cob format
        mat_world;  // "   " for local to world " "

    // initialize matrices
    MAT_IDENTITY_4X4(&mat_local);
    MAT_IDENTITY_4X4(&mat_world);

    // Step 1: clear out the object and initialize it a bit
    memset(obj, 0, sizeof(OBJECT4DV1));

    // set state of object to active and visible
    obj->state = OBJECT4DV1_STATE_ACTIVE | OBJECT4DV1_STATE_VISIBLE;

    // set position of object is caller requested position
    if (pos)
    {
        // set position of object
        obj->world_pos.x = pos->x;
        obj->world_pos.y = pos->y;
        obj->world_pos.z = pos->z;
        obj->world_pos.w = pos->w;
    } // end 
    else
    {
        // set it to (0,0,0,1)
        obj->world_pos.x = 0;
        obj->world_pos.y = 0;
        obj->world_pos.z = 0;
        obj->world_pos.w = 1;
    } // end else

    // Step 2: open the file for reading using the parser
    if (!parser.Open(filename))
    {
        Write_Error("Couldn't open .COB file %s.", filename);
        return(0);
    } // end if

    // Step 3: 

    // lets find the name of the object first 
    while(1)
    {
        // get the next line, we are looking for "Name"
        if (!parser.Getline(PARSER_STRIP_EMPTY_LINES | PARSER_STRIP_WS_ENDS))
        {
            Write_Error("Image 'name' not found in .COB file %s.", filename);
            return(0);
        } // end if

        // check for pattern?  
        if ( parser.Pattern_Match(parser.buffer, "['Name'] [s>0]") )
        {
            // name should be in second string variable, index 1
            strcpy(obj->name, parser.pstrings[1]);          
            Write_Error("\nCOB Reader Object Name: %s", obj->name);

            break;    
        } // end if

    } // end while


    // step 4: get local and world transforms and store them

    // center 0 0 0
    // x axis 1 0 0
    // y axis 0 1 0
    // z axis 0 0 1

    while(1)
    {
        // get the next line, we are looking for "center"
        if (!parser.Getline(PARSER_STRIP_EMPTY_LINES | PARSER_STRIP_WS_ENDS))
        {
            Write_Error("Center not found in .COB file %s.", filename);
            return(0);
        } // end if

        // check for pattern?  
        if ( parser.Pattern_Match(parser.buffer, "['center'] [f] [f] [f]") )
        {
            // the "center" holds the translation factors, so place in
            // last row of homogeneous matrix, note that these are row vectors
            // that we need to drop in each column of matrix
            mat_local.M[3][0] = -parser.pfloats[0]; // center x
            mat_local.M[3][1] = -parser.pfloats[1]; // center y
            mat_local.M[3][2] = -parser.pfloats[2]; // center z

            // ok now, the next 3 lines should be the x,y,z transform vectors
            // so build up   

            // "x axis" 
            parser.Getline(PARSER_STRIP_EMPTY_LINES | PARSER_STRIP_WS_ENDS);
            parser.Pattern_Match(parser.buffer, "['x'] ['axis'] [f] [f] [f]");

            // place row in x column of transform matrix
            mat_local.M[0][0] = parser.pfloats[0]; // rxx
            mat_local.M[1][0] = parser.pfloats[1]; // rxy
            mat_local.M[2][0] = parser.pfloats[2]; // rxz

            // "y axis" 
            parser.Getline(PARSER_STRIP_EMPTY_LINES | PARSER_STRIP_WS_ENDS);
            parser.Pattern_Match(parser.buffer, "['y'] ['axis'] [f] [f] [f]");

            // place row in y column of transform matrix
            mat_local.M[0][1] = parser.pfloats[0]; // ryx
            mat_local.M[1][1] = parser.pfloats[1]; // ryy
            mat_local.M[2][1] = parser.pfloats[2]; // ryz

            // "z axis" 
            parser.Getline(PARSER_STRIP_EMPTY_LINES | PARSER_STRIP_WS_ENDS);
            parser.Pattern_Match(parser.buffer, "['z'] ['axis'] [f] [f] [f]");

            // place row in z column of transform matrix
            mat_local.M[0][2] = parser.pfloats[0]; // rzx
            mat_local.M[1][2] = parser.pfloats[1]; // rzy
            mat_local.M[2][2] = parser.pfloats[2]; // rzz

            Print_Mat_4X4(&mat_local, "Local COB Matrix:");

            break;    
        } // end if

    } // end while

    // now "Transform"
    while(1)
    {
        // get the next line, we are looking for "Transform"
        if (!parser.Getline(PARSER_STRIP_EMPTY_LINES | PARSER_STRIP_WS_ENDS))
        {
            Write_Error("Transform not found in .COB file %s.", filename);
            return(0);
        } // end if

        // check for pattern?  
        if ( parser.Pattern_Match(parser.buffer, "['Transform']") )
        {

            // "x axis" 
            parser.Getline(PARSER_STRIP_EMPTY_LINES | PARSER_STRIP_WS_ENDS);
            parser.Pattern_Match(parser.buffer, "[f] [f] [f]");

            // place row in x column of transform matrix
            mat_world.M[0][0] = parser.pfloats[0]; // rxx
            mat_world.M[1][0] = parser.pfloats[1]; // rxy
            mat_world.M[2][0] = parser.pfloats[2]; // rxz

            // "y axis" 
            parser.Getline(PARSER_STRIP_EMPTY_LINES | PARSER_STRIP_WS_ENDS);
            parser.Pattern_Match(parser.buffer, "[f] [f] [f]");

            // place row in y column of transform matrix
            mat_world.M[0][1] = parser.pfloats[0]; // ryx
            mat_world.M[1][1] = parser.pfloats[1]; // ryy
            mat_world.M[2][1] = parser.pfloats[2]; // ryz

            // "z axis" 
            parser.Getline(PARSER_STRIP_EMPTY_LINES | PARSER_STRIP_WS_ENDS);
            parser.Pattern_Match(parser.buffer, "[f] [f] [f]");

            // place row in z column of transform matrix
            mat_world.M[0][2] = parser.pfloats[0]; // rzx
            mat_world.M[1][2] = parser.pfloats[1]; // rzy
            mat_world.M[2][2] = parser.pfloats[2]; // rzz

            Print_Mat_4X4(&mat_world, "World COB Matrix:");

            // no need to read in last row, since it's always 0,0,0,1 and we don't use it anyway
            break;    

        } // end if

    } // end while

    // step 6: get number of vertices and polys in object
    while(1)
    {
        // get the next line, we are looking for "World Vertices" 
        if (!parser.Getline(PARSER_STRIP_EMPTY_LINES | PARSER_STRIP_WS_ENDS))
        {
            Write_Error("'World Vertices' line not found in .COB file %s.", filename);
            return(0);
        } // end if

        // check for pattern?  
        if (parser.Pattern_Match(parser.buffer, "['World'] ['Vertices'] [i]") )
        {
            // simply extract the number of vertices from the pattern matching 
            // output arrays
            obj->num_vertices = parser.pints[0];

            Write_Error("\nCOB Reader Num Vertices: %d", obj->num_vertices);
            break;    

        } // end if

    } // end while

    // Step 7: load the vertex list
    // now read in vertex list, format:
    // "d.d d.d d.d"
    for (int vertex = 0; vertex < obj->num_vertices; vertex++)
    {
        // hunt for vertex
        while(1)
        {
            // get the next vertex
            if (!parser.Getline(PARSER_STRIP_EMPTY_LINES | PARSER_STRIP_WS_ENDS))
            {
                Write_Error("\nVertex list ended abruptly! in .COB file %s.", filename);
                return(0);
            } // end if

            // check for pattern?  
            if (parser.Pattern_Match(parser.buffer, "[f] [f] [f]"))
            {
                // at this point we have the x,y,z in the the pfloats array locations 0,1,2
                obj->vlist_local[vertex].x = parser.pfloats[0];
                obj->vlist_local[vertex].y = parser.pfloats[1];
                obj->vlist_local[vertex].z = parser.pfloats[2];
                obj->vlist_local[vertex].w = 1;

                // do vertex swapping right here, allow muliple swaps, why not!
                // defines for vertex re-ordering flags

                //#define VERTEX_FLAGS_INVERT_X   1    // inverts the Z-coordinates
                //#define VERTEX_FLAGS_INVERT_Y   2    // inverts the Z-coordinates
                //#define VERTEX_FLAGS_INVERT_Z   4    // inverts the Z-coordinates
                //#define VERTEX_FLAGS_SWAP_YZ    8    // transforms a RHS model to a LHS model
                //#define VERTEX_FLAGS_SWAP_XZ    16   // ???
                //#define VERTEX_FLAGS_SWAP_XY    32
                //#define VERTEX_FLAGS_INVERT_WINDING_ORDER 64  // invert winding order from cw to ccw or ccw to cc
                //#define VERTEX_FLAGS_TRANSFORM_LOCAL         512   // if file format has local transform then do it!
                //#define VERTEX_FLAGS_TRANSFORM_LOCAL_WORLD  1024  // if file format has local to world then do it!

                VECTOR4D temp_vector; // temp for calculations

                // now apply local and world transformations encoded in COB format
                if (vertex_flags & VERTEX_FLAGS_TRANSFORM_LOCAL )
                {
                    Mat_Mul_VECTOR4D_4X4(&obj->vlist_local[vertex], &mat_local, &temp_vector);
                    VECTOR4D_COPY(&obj->vlist_local[vertex], &temp_vector); 
                } // end if 

                if (vertex_flags & VERTEX_FLAGS_TRANSFORM_LOCAL_WORLD )
                {
                    Mat_Mul_VECTOR4D_4X4(&obj->vlist_local[vertex], &mat_world, &temp_vector);
                    VECTOR4D_COPY(&obj->vlist_local[vertex], &temp_vector); 
                } // end if 

                float temp_f; // used for swapping

                // invert signs?
                if (vertex_flags & VERTEX_FLAGS_INVERT_X)
                    obj->vlist_local[vertex].x=-obj->vlist_local[vertex].x;

                if (vertex_flags & VERTEX_FLAGS_INVERT_Y)
                    obj->vlist_local[vertex].y=-obj->vlist_local[vertex].y;

                if (vertex_flags & VERTEX_FLAGS_INVERT_Z)
                    obj->vlist_local[vertex].z=-obj->vlist_local[vertex].z;

                // swap any axes?
                if (vertex_flags & VERTEX_FLAGS_SWAP_YZ)
                    SWAP(obj->vlist_local[vertex].y, obj->vlist_local[vertex].z, temp_f);

                if (vertex_flags & VERTEX_FLAGS_SWAP_XZ)
                    SWAP(obj->vlist_local[vertex].x, obj->vlist_local[vertex].z, temp_f);

                if (vertex_flags & VERTEX_FLAGS_SWAP_XY)
                    SWAP(obj->vlist_local[vertex].x, obj->vlist_local[vertex].y, temp_f);

                // scale vertices
                if (scale)
                {
                    obj->vlist_local[vertex].x*=scale->x;
                    obj->vlist_local[vertex].y*=scale->y;
                    obj->vlist_local[vertex].z*=scale->z;
                } // end if

                Write_Error("\nVertex %d = %f, %f, %f, %f", vertex,
                    obj->vlist_local[vertex].x, 
                    obj->vlist_local[vertex].y, 
                    obj->vlist_local[vertex].z,
                    obj->vlist_local[vertex].w);

                // found vertex, break out of while for next pass
                break;

            } // end if

        } // end while

    } // end for vertex

    // compute average and max radius
    Compute_OBJECT4DV1_Radius(obj);

    Write_Error("\nObject average radius = %f, max radius = %f", 
        obj->avg_radius, obj->max_radius);


    // step 8: get number of texture vertices
    while(1)
    {
        // get the next line, we are looking for "Texture Vertices ddd" 
        if (!parser.Getline(PARSER_STRIP_EMPTY_LINES | PARSER_STRIP_WS_ENDS))
        {
            Write_Error("'Texture Vertices' line not found in .COB file %s.", filename);
            return(0);
        } // end if

        // check for pattern?  
        if (parser.Pattern_Match(parser.buffer, "['Texture'] ['Vertices'] [i]") )
        {
            // simply extract the number of texture vertices from the pattern matching 
            // output arrays
            num_texture_vertices = parser.pints[0];

            Write_Error("\nCOB Reader Texture Vertices: %d", num_texture_vertices);
            break;    

        } // end if

    } // end while

    // Step 9: load the texture vertex list in format "U V"
    // "d.d d.d"
    for (int tvertex = 0; tvertex < num_texture_vertices; tvertex++)
    {
        // hunt for texture
        while(1)
        {
            // get the next vertex
            if (!parser.Getline(PARSER_STRIP_EMPTY_LINES | PARSER_STRIP_WS_ENDS))
            {
                Write_Error("\nTexture Vertex list ended abruptly! in .COB file %s.", filename);
                return(0);
            } // end if

            // check for pattern?  
            if (parser.Pattern_Match(parser.buffer, "[f] [f]"))
            {
                // at this point we have the U V in the the pfloats array locations 0,1 for this 
                // texture vertex, although we do nothing with them at this point with this parser
                texture_vertices[tvertex].x = parser.pfloats[0];
                texture_vertices[tvertex].y = parser.pfloats[1];

                Write_Error("\nTexture Vertex %d: U=%f, V=%f", tvertex,
                    texture_vertices[tvertex].x, 
                    texture_vertices[tvertex].y );

                // found vertex, break out of while for next pass
                break;

            } // end if

        } // end while

    } // end for

    int poly_material[OBJECT4DV1_MAX_POLYS]; // this holds the material index for each polygon
    // we need these indices since when reading the file
    // we read the polygons BEFORE the materials, so we need
    // this data, so we can go back later and extract the material
    // that each poly WAS assigned and get the colors out, since
    // objects and polygons do not currenlty support materials


    int material_index_referenced[MAX_MATERIALS];   // used to track if an index has been used yet as a material 
    // reference. since we don't know how many materials, we need
    // a way to count them up, but if we have seen a material reference
    // more than once then we don't increment the total number of materials
    // this array is for this

    // clear out reference array
    memset(material_index_referenced,0, sizeof(material_index_referenced));


    // step 10: load in the polygons
    // poly list starts off with:
    // "Faces ddd:"
    while(1)
    {
        // get next line
        if (!parser.Getline(PARSER_STRIP_EMPTY_LINES | PARSER_STRIP_WS_ENDS))
        {
            Write_Error("\n'Faces' line not found in .ASC file %s.", filename);
            return(0);
        } // end if

        // check for pattern?  
        if (parser.Pattern_Match(parser.buffer, "['Faces'] [i]"))
        {
            Write_Error("\nCOB Reader found face list in .COB file %s.", filename);

            // finally set number of polys
            obj->num_polys = parser.pints[0];

            break;
        } // end if
    } // end while

    // now read each face in format:
    // Face verts nn flags ff mat mm
    // the nn is the number of vertices, always 3
    // the ff is the flags, unused for now, has to do with holes
    // the mm is the material index number 


    int poly_surface_desc    = 0; // ASC surface descriptor/material in this case
    int poly_num_verts       = 0; // number of vertices for current poly (always 3)
    int num_materials_object = 0; // number of materials for this object

    for (int poly=0; poly < obj->num_polys; poly++)
    {
        // hunt until next face is found
        while(1)
        {
            // get the next polygon face
            if (!parser.Getline(PARSER_STRIP_EMPTY_LINES | PARSER_STRIP_WS_ENDS))
            {
                Write_Error("\nface list ended abruptly! in .COB file %s.", filename);
                return(0);
            } // end if

            // check for pattern?  
            if (parser.Pattern_Match(parser.buffer, "['Face'] ['verts'] [i] ['flags'] [i] ['mat'] [i]"))
            {
                // at this point we have the number of vertices for the polygon, the flags, and it's material index
                // in the integer output array locations 0,1,2

                // store the material index for this polygon for retrieval later, but make sure adjust the 
                // the index to take into consideration that the data in parser.pints[2] is 0 based, and we need
                // an index relative to the entire library, so we simply need to add num_materials to offset the 
                // index properly, but we will leave this reference zero based for now... and fix up later
                poly_material[poly] = parser.pints[2];

                // update the reference array
                if (material_index_referenced[ poly_material[poly] ] == 0)
                {
                    // mark as referenced
                    material_index_referenced[ poly_material[poly] ] = 1;

                    // increment total number of materials for this object
                    num_materials_object++;
                } // end if        


                // test if number of vertices is 3
                if (parser.pints[0]!=3)
                {
                    Write_Error("\nface not a triangle! in .COB file %s.", filename);
                    return(0);
                } // end if

                // now read out the vertex indices and texture indices format:
                // <vindex0, tindex0>  <vindex1, tindex1> <vindex1, tindex1> 
                if (!parser.Getline(PARSER_STRIP_EMPTY_LINES | PARSER_STRIP_WS_ENDS))
                {
                    Write_Error("\nface list ended abruptly! in .COB file %s.", filename);
                    return(0);
                } // end if

                // lets replace ",<>" with ' ' to make extraction easy
                ReplaceChars(parser.buffer, parser.buffer, ",<>",' ');      
                parser.Pattern_Match(parser.buffer, "[i] [i] [i] [i] [i] [i]");

                // 0,2,4 holds vertex indices
                // 1,3,5 holds texture indices -- unused for now, no place to put them!

                // insert polygon, check for winding order invert
                if (vertex_flags & VERTEX_FLAGS_INVERT_WINDING_ORDER)
                {     
                    poly_num_verts           = 3;
                    obj->plist[poly].vert[0] = parser.pints[4];
                    obj->plist[poly].vert[1] = parser.pints[2];
                    obj->plist[poly].vert[2] = parser.pints[0];
                } // end if
                else
                { // leave winding order alone
                    poly_num_verts           = 3;
                    obj->plist[poly].vert[0] = parser.pints[0];
                    obj->plist[poly].vert[1] = parser.pints[2];
                    obj->plist[poly].vert[2] = parser.pints[4];
                } // end else

                // point polygon vertex list to object's vertex list
                // note that this is redundant since the polylist is contained
                // within the object in this case and its up to the user to select
                // whether the local or transformed vertex list is used when building up
                // polygon geometry, might be a better idea to set to NULL in the context
                // of polygons that are part of an object
                obj->plist[poly].vlist = obj->vlist_local; 


                // set polygon to active
                obj->plist[poly].state = POLY4DV1_STATE_ACTIVE;    

                // found the face, break out of while for another pass
                break;

            } // end if

        } // end while      

        Write_Error("\nPolygon %d:", poly);
        Write_Error("\nLocal material Index=%d, total materials for object = %d, vert_indices [%d, %d, %d]", 
            poly_material[poly],
            num_materials_object,
            obj->plist[poly].vert[0],
            obj->plist[poly].vert[1],
            obj->plist[poly].vert[2]);       
    } // end for poly

    // now find materials!!! and we are out of here!
    int curr_material = 0;
    for (curr_material = 0; curr_material < num_materials_object; curr_material++)
    {
        // hunt for the material header "mat# ddd"
        while(1)
        {
            // get the next polygon material 
            if (!parser.Getline(PARSER_STRIP_EMPTY_LINES | PARSER_STRIP_WS_ENDS))
            {
                Write_Error("\nmaterial list ended abruptly! in .COB file %s.", filename);
                return(0);
            } // end if

            // check for pattern?  
            if (parser.Pattern_Match(parser.buffer, "['mat#'] [i]") )
            {
                // extract the material that is being defined 
                int material_index = parser.pints[0];

                // get color of polygon, although it might be irrelevant for a textured surface
                while(1)
                {
                    // get the next line
                    if (!parser.Getline(PARSER_STRIP_EMPTY_LINES | PARSER_STRIP_WS_ENDS))
                    {
                        Write_Error("\nRGB color ended abruptly! in .COB file %s.", filename);
                        return(0);
                    } // end if

                    // replace the , comma's if there are any with spaces
                    ReplaceChars(parser.buffer, parser.buffer, ",", ' ', 1);

                    // look for "rgb float,float,float"
                    if (parser.Pattern_Match(parser.buffer, "['rgb'] [f] [f] [f]") )
                    {
                        // extract data and store color in material libary
                        // pfloats[] 0,1,2,3, has data
                        materials[material_index + num_materials].color.r = (int)(parser.pfloats[0]*255 + 0.5);
                        materials[material_index + num_materials].color.g = (int)(parser.pfloats[1]*255 + 0.5);
                        materials[material_index + num_materials].color.b = (int)(parser.pfloats[2]*255 + 0.5);

                        break; // while looking for rgb
                    } // end if

                } // end while    

                // extract out lighting constants for the heck of it, they are on a line like this:
                // "alpha float ka float ks float exp float ior float"
                // alpha is transparency           0 - 1
                // ka is ambient coefficient       0 - 1
                // ks is specular coefficient      0 - 1
                // exp is highlight power exponent 0 - 1
                // ior is index of refraction (unused)

                // although our engine will have minimal support for these, we might as well get them
                while(1)
                {
                    // get the next line
                    if (!parser.Getline(PARSER_STRIP_EMPTY_LINES | PARSER_STRIP_WS_ENDS))
                    {
                        Write_Error("\nmaterial properties ended abruptly! in .COB file %s.", filename);
                        return(0);
                    } // end if

                    // look for "alpha float ka float ks float exp float ior float"
                    if (parser.Pattern_Match(parser.buffer, "['alpha'] [f] ['ka'] [f] ['ks'] [f] ['exp'] [f]") )
                    {
                        // extract data and store in material libary
                        // pfloats[] 0,1,2,3, has data
                        materials[material_index + num_materials].color.a  = (UCHAR)(parser.pfloats[0]*255 + 0.5);
                        materials[material_index + num_materials].ka       = parser.pfloats[1];
                        materials[material_index + num_materials].kd       = 1; // hard code for now
                        materials[material_index + num_materials].ks       = parser.pfloats[2];
                        materials[material_index + num_materials].power    = parser.pfloats[3];

                        // compute material reflectivities in pre-multiplied format to help engine
                        for (int rgb_index=0; rgb_index < 3; rgb_index++)
                        {
                            // ambient reflectivity
                            materials[material_index + num_materials].ra.rgba_M[rgb_index] = 
                                ( (UCHAR)(materials[material_index + num_materials].ka * 
                                (float)materials[material_index + num_materials].color.rgba_M[rgb_index] + 0.5) );


                            // diffuse reflectivity
                            materials[material_index + num_materials].rd.rgba_M[rgb_index] = 
                                ( (UCHAR)(materials[material_index + num_materials].kd * 
                                (float)materials[material_index + num_materials].color.rgba_M[rgb_index] + 0.5) );


                            // specular reflectivity
                            materials[material_index + num_materials].rs.rgba_M[rgb_index] = 
                                ( (UCHAR)(materials[material_index + num_materials].ks * 
                                (float)materials[material_index + num_materials].color.rgba_M[rgb_index] + 0.5) );

                        } // end for rgb_index

                        break;
                    } // end if

                } // end while    

                // now we need to know the shading model, it's a bit tricky, we need to look for the lines
                // "Shader class: color" first, then after this line is:
                // "Shader name: "xxxxxx" (xxxxxx) "
                // where the xxxxx part will be "plain color" and "plain" for colored polys 
                // or "texture map" and "caligari texture"  for textures
                // THEN based on that we hunt for "Shader class: reflectance" which is where the type
                // of shading is encoded, we look for the "Shader name: "xxxxxx" (xxxxxx) " again, 
                // and based on it's value we map it to our shading system as follows:
                // "constant" -> MATV1_ATTR_SHADE_MODE_CONSTANT 
                // "matte"    -> MATV1_ATTR_SHADE_MODE_FLAT
                // "plastic"  -> MATV1_ATTR_SHADE_MODE_GOURAUD
                // "phong"    -> MATV1_ATTR_SHADE_MODE_FASTPHONG 
                // and in the case that in the "color" class, we found a "texture map" then the "shading mode" is
                // "texture map" -> MATV1_ATTR_SHADE_MODE_TEXTURE 
                // which must be logically or'ed with the other previous modes

                //  look for the "shader class: color"
                while(1)
                {
                    // get the next line
                    if (!parser.Getline(PARSER_STRIP_EMPTY_LINES | PARSER_STRIP_WS_ENDS))
                    {
                        Write_Error("\nshader class ended abruptly! in .COB file %s.", filename);
                        return(0);
                    } // end if

                    if (parser.Pattern_Match(parser.buffer, "['Shader'] ['class:'] ['color']") )
                    {
                        break;
                    } // end if

                } // end while

                // now look for the shader name for this class
                // Shader name: "plain color" or Shader name: "texture map"
                while(1)
                {
                    // get the next line
                    if (!parser.Getline(PARSER_STRIP_EMPTY_LINES | PARSER_STRIP_WS_ENDS))
                    {
                        Write_Error("\nshader name ended abruptly! in .COB file %s.", filename);
                        return(0);
                    } // end if

                    // replace the " with spaces
                    ReplaceChars(parser.buffer, parser.buffer, "\"", ' ', 1);

                    // is this a "plain color" poly?
                    if (parser.Pattern_Match(parser.buffer, "['Shader'] ['name:'] ['plain'] ['color']") )
                    {
                        // not much to do this is default, we need to wait for the reflectance type
                        // to tell us the shading mode

                        break;
                    } // end if

                    // is this a "texture map" poly?
                    if (parser.Pattern_Match(parser.buffer, "['Shader'] ['name:'] ['texture'] ['map']") )
                    {
                        // set the texture mapping flag in material
                        SET_BIT(materials[material_index + num_materials].attr, MATV1_ATTR_SHADE_MODE_TEXTURE);

                        // almost done, we need the file name of the darn texture map, its in this format:
                        // file name: string "D:\Source\models\textures\wall01.bmp"

                        // of course the filename in the quotes will change
                        // so lets hunt until we find it...
                        while(1)
                        {
                            // get the next line
                            if (!parser.Getline(PARSER_STRIP_EMPTY_LINES | PARSER_STRIP_WS_ENDS))
                            {
                                Write_Error("\ncouldnt find texture name! in .COB file %s.", filename);
                                return(0);
                            } // end if

                            // replace the " with spaces
                            ReplaceChars(parser.buffer, parser.buffer, "\"", ' ', 1);

                            // is this the file name?
                            if (parser.Pattern_Match(parser.buffer, "['file'] ['name:'] ['string'] [s>0]") )
                            {
                                // ok, simply convert to a real file name by changing the slashes
                                ReplaceChars(parser.pstrings[3], parser.pstrings[3], "\\", '/',1);

                                // and save the filename
                                strcpy(materials[material_index + num_materials].texture_file, parser.pstrings[3]);

                                break;
                            } // end if

                        } // end while

                        break;
                    } // end if

                } // end while 

                // alright, finally! Now we need to know what the actual shader type, now in the COB format
                // I have decided that in the "reflectance" class that's where we will look at what kind
                // of shader is supposed to be used on the polygon

                //  look for the "Shader class: reflectance"
                while(1)
                {
                    // get the next line
                    if (!parser.Getline(PARSER_STRIP_EMPTY_LINES | PARSER_STRIP_WS_ENDS))
                    {
                        Write_Error("\nshader reflectance class not found in .COB file %s.", filename);
                        return(0);
                    } // end if

                    // look for "Shader class: reflectance"
                    if (parser.Pattern_Match(parser.buffer, "['Shader'] ['class:'] ['reflectance']") )
                    {
                        // now we know the next "shader name" is what we are looking for so, break

                        break;
                    } // end if

                } // end while    

                // looking for "Shader name: "xxxxxx" (xxxxxx) " again, 
                // and based on it's value we map it to our shading system as follows:
                // "constant" -> MATV1_ATTR_SHADE_MODE_CONSTANT 
                // "matte"    -> MATV1_ATTR_SHADE_MODE_FLAT
                // "plastic"  -> MATV1_ATTR_SHADE_MODE_GOURAUD
                // "phong"    -> MATV1_ATTR_SHADE_MODE_FASTPHONG 
                // and in the case that in the "color" class, we found a "texture map" then the "shading mode" is
                // "texture map" -> MATV1_ATTR_SHADE_MODE_TEXTURE 
                // which must be logically or'ed with the other previous modes
                while(1)
                {
                    // get the next line
                    if (!parser.Getline(PARSER_STRIP_EMPTY_LINES | PARSER_STRIP_WS_ENDS))
                    {
                        Write_Error("\nshader name ended abruptly! in .COB file %s.", filename);
                        return(0);
                    } // end if

                    // get rid of those quotes
                    ReplaceChars(parser.buffer, parser.buffer, "\"",' ',1);

                    // did we find the name?
                    if (parser.Pattern_Match(parser.buffer, "['Shader'] ['name:'] [s>0]" ) )
                    {
                        // figure out which shader to use
                        if (strcmp(parser.pstrings[2], "constant") == 0)
                        {
                            // set the shading mode flag in material
                            SET_BIT(materials[material_index + num_materials].attr, MATV1_ATTR_SHADE_MODE_CONSTANT);
                        } // end if
                        else
                            if (strcmp(parser.pstrings[2], "matte") == 0)
                            {
                                // set the shading mode flag in material
                                SET_BIT(materials[material_index + num_materials].attr, MATV1_ATTR_SHADE_MODE_FLAT);
                            } // end if
                            else
                                if (strcmp(parser.pstrings[2], "plastic") == 0)
                                {
                                    // set the shading mode flag in material
                                    SET_BIT(materials[curr_material + num_materials].attr, MATV1_ATTR_SHADE_MODE_GOURAUD);
                                } // end if
                                else
                                    if (strcmp(parser.pstrings[2], "phong") == 0)
                                    {
                                        // set the shading mode flag in material
                                        SET_BIT(materials[material_index + num_materials].attr, MATV1_ATTR_SHADE_MODE_FASTPHONG);
                                    } // end if
                                    else
                                    {
                                        // set the shading mode flag in material
                                        SET_BIT(materials[material_index + num_materials].attr, MATV1_ATTR_SHADE_MODE_FLAT);
                                    } // end else

                                    break;
                    } // end if

                } // end while

                // found the material, break out of while for another pass
                break;

            } // end if found material

        } // end while looking for mat#1

    } // end for curr_material

    // at this point poly_material[] holds all the indices for the polygon materials (zero based, so they need fix up)
    // and we must access the materials array to fill in each polygon with the polygon color, etc.
    // now that we finally have the material libary loaded
    for (int curr_poly = 0; curr_poly < obj->num_polys; curr_poly++)
    {
        Write_Error("\nfixing poly material %d from index %d to index %d", curr_poly, 
            poly_material[curr_poly],
            poly_material[curr_poly] + num_materials  );
        // fix up offset
        poly_material[curr_poly]  = poly_material[curr_poly] + num_materials;

        // we need to know what color depth we are dealing with, so check
        // the bits per pixel, this assumes that the system has already
        // made the call to DDraw_Init() or set the bit depth
        if (screen_bpp == 16)
        {
            // cool, 16 bit mode
            SET_BIT(obj->plist[curr_poly].attr,POLY4DV1_ATTR_RGB16);
            obj->plist[curr_poly].color = RGB16Bit(materials[ poly_material[curr_poly] ].color.r, 
                materials[ poly_material[curr_poly] ].color.g, 
                materials[ poly_material[curr_poly] ].color.b);
            Write_Error("\nPolygon 16-bit");
        } // end
        else
        {
            // 8 bit mode
            SET_BIT(obj->plist[curr_poly].attr,POLY4DV1_ATTR_8BITCOLOR);
            obj->plist[curr_poly].color = RGBto8BitIndex(materials[ poly_material[curr_poly] ].color.r,
                materials[ poly_material[curr_poly] ].color.g,
                materials[ poly_material[curr_poly] ].color.b,
                palette, 0);

            Write_Error("\nPolygon 8-bit, index=%d", obj->plist[curr_poly].color);
        } // end else

        // now set all the shading flags
        // figure out which shader to use
        if (materials[ poly_material[curr_poly] ].attr & MATV1_ATTR_SHADE_MODE_CONSTANT)
        {
            // set shading mode
            SET_BIT(obj->plist[curr_poly].attr, POLY4DV1_ATTR_SHADE_MODE_CONSTANT);
        } // end if
        else
            if (materials[ poly_material[curr_poly] ].attr & MATV1_ATTR_SHADE_MODE_FLAT)
            {
                // set shading mode
                SET_BIT(obj->plist[curr_poly].attr, POLY4DV1_ATTR_SHADE_MODE_FLAT);
            } // end if
            else
                if (materials[ poly_material[curr_poly] ].attr & MATV1_ATTR_SHADE_MODE_GOURAUD)
                {
                    // set shading mode
                    SET_BIT(obj->plist[curr_poly].attr, POLY4DV1_ATTR_SHADE_MODE_GOURAUD);
                } // end if
                else
                    if (materials[ poly_material[curr_poly] ].attr & MATV1_ATTR_SHADE_MODE_FASTPHONG)
                    {
                        // set shading mode
                        SET_BIT(obj->plist[curr_poly].attr, POLY4DV1_ATTR_SHADE_MODE_FASTPHONG);
                    } // end if
                    else
                    {
                        // set shading mode
                        SET_BIT(obj->plist[curr_poly].attr, POLY4DV1_ATTR_SHADE_MODE_GOURAUD);
                    } // end if

                    if (materials[ poly_material[curr_poly] ].attr & MATV1_ATTR_SHADE_MODE_TEXTURE)
                    {
                        // set shading mode
                        SET_BIT(obj->plist[curr_poly].attr, POLY4DV1_ATTR_SHADE_MODE_TEXTURE);
                    } // end if

    } // end for curr_poly

    // local object materials have been added to database, update total materials in system
    num_materials+=num_materials_object;

#ifdef DEBUG_ON
    for (curr_material = 0; curr_material < num_materials; curr_material++)
    {
        Write_Error("\nMaterial %d", curr_material);

        Write_Error("\nint  state    = %d", materials[curr_material].state);
        Write_Error("\nint  id       = %d", materials[curr_material].id);
        Write_Error("\nchar name[64] = %s", materials[curr_material].name);
        Write_Error("\nint  attr     = %d", materials[curr_material].attr); 
        Write_Error("\nint r         = %d", materials[curr_material].color.r); 
        Write_Error("\nint g         = %d", materials[curr_material].color.g); 
        Write_Error("\nint b         = %d", materials[curr_material].color.b); 
        Write_Error("\nint alpha     = %d", materials[curr_material].color.a);
        Write_Error("\nint color     = %d", materials[curr_material].attr); 
        Write_Error("\nfloat ka      = %f", materials[curr_material].ka); 
        Write_Error("\nkd            = %f", materials[curr_material].kd); 
        Write_Error("\nks            = %f", materials[curr_material].ks); 
        Write_Error("\npower         = %f", materials[curr_material].power);
        Write_Error("\nchar texture_file = %s\n", materials[curr_material].texture_file);
    } // end for curr_material
#endif

    // return success
    return(1);

} // end Load_OBJECT4DV1_COB

///////////////////////////////////////////////////////////////////////////////

int RGBto8BitIndex(UCHAR r, UCHAR g, UCHAR b, LPPALETTEENTRY palette, int flush_cache=0)
{
    // this function hunts thru the loaded 8-bit palette and tries to find the 
    // best match to the sent rgb color, the 8-bit index is returned. The algorithm
    // performings a least squares match on the values in the CLUT, also to speed up
    // the process, the last few translated colored are stored in a stack in the format
    // rgbi, so when a new rgb comes in, it is compared against the rgb entries in the
    // table, if found then that index is used, else, the rgb is translated and added to
    // the table, and the table is shifted one slot, so the last element is thrown away,
    // hence the table is FIFO in as much as the first discarded value will be the first
    // this way the system keeps previously translated colors cached, so the fairly long
    // least squared scan doesn't take forever!
    // also note the compression of the RGBI data, a compare is performed on the upper 24 bits only
    // also, if flush_cache = 1 then the local cache is flushed, for example a new palette is loaded

#define COLOR_CACHE_SIZE 16 // 16 entries should do for now

    typedef struct 
    { 
        UCHAR r,g,b;    // the rgb value of this translated color
        UCHAR index;    // the color index that matched is most closely
    } RGBINDEX, *RGBINDEX_PTR;

    static RGBINDEX color_cache[COLOR_CACHE_SIZE];  // the color cache
    static int cache_entries=0;                     // number of entries in the cache

    // test for flush cache command, new palette coming in...
    if (flush_cache==1)
        cache_entries = 0;

    // test if the color is in the cache
    for (int cache_index=0; cache_index < cache_entries; cache_index++)
    {
        // is this a match?
        if (r==color_cache[cache_index].r &&
            g==color_cache[cache_index].g &&
            b==color_cache[cache_index].b )
            return(color_cache[cache_index].index);

    } // end for

    // if we get here then we had no luck, so least sqaures scan for best match
    // and make sure to add results to cache

    int  curr_index  = -1;        // current color index of best match
    long curr_error = INT_MAX;    // distance in color space to nearest match or "error"

    for (int color_index = 0; color_index < 256; color_index++)
    {
        // compute distance to color from target
        long delta_red   = abs(palette[color_index].peRed   - r);
        long delta_green = abs(palette[color_index].peGreen - g);
        long delta_blue  = abs(palette[color_index].peBlue  - b);

        long error = (delta_red*delta_red) + (delta_green*delta_green) + (delta_blue*delta_blue);

        // is this color a better match?
        if (error < curr_error)
        { 
            curr_index = color_index;
            curr_error = error;   
        } // end if

    } // end for color_index

    // at this point we have the new color, insert it into cache
    // shift cache over one entry, copy elements [0 - (n-1)] -> [1 - n]
    memmove((void *)&color_cache[1], (void *)&color_cache[0],  COLOR_CACHE_SIZE*sizeof(RGBINDEX) - sizeof(RGBINDEX) );

    // now insert the new element
    color_cache[0].r     = r;
    color_cache[0].b     = b;
    color_cache[0].g     = g;
    color_cache[0].index = curr_index; 

    // increment number of elements in the cache until saturation
    if (++cache_entries > COLOR_CACHE_SIZE)
        cache_entries = COLOR_CACHE_SIZE;

    // return results
    return(curr_index);

} // end RGBto8BitIndex

//////////////////////////////////////////////////////////////////////////////

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
                       float        _pf)         // power factor/falloff for spot lights
{
    // this function initializes a light based on the flags sent in _attr, values that
    // aren't needed are set to 0 by caller

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
        VECTOR4D_COPY(&lights[index].pos, _pos);  // position of light

    if (_dir)
    {
        VECTOR4D_COPY(&lights[index].dir, _dir);  // direction of light
        // normalize it
        VECTOR4D_Normalize(&lights[index].dir);

    } // end if

    lights[index].spot_inner  = _spot_inner; // inner angle for spot light
    lights[index].spot_outer  = _spot_outer; // outer angle for spot light
    lights[index].pf          = _pf;         // power factor/falloff for spot lights

    // return light index as success
    return(index);

} // end Create_Light_LIGHTV1

//////////////////////////////////////////////////////////////////////////////

int Reset_Lights_LIGHTV1(void)
{
    // this function simply resets all lights in the system
    static int first_time = 1;

    memset(lights, 0, MAX_LIGHTS*sizeof(LIGHTV1));

    // reset number of lights
    num_lights = 0;

    // reset first time
    first_time = 0;

    // return success
    return(1);

} // end Reset_Lights_LIGHTV1

//////////////////////////////////////////////////////////////////////////////

int Reset_Materials_MATV1(void)
{
    // this function resets all the materials
    static int first_time = 1;

    // if this is the first time then zero EVERYTHING out
    if (first_time)
    {
        memset(materials, 0, MAX_MATERIALS*sizeof(MATV1));
        first_time = 0;
    } // end if

    // scan thru materials and release all textures, if any?
    for (int curr_matt = 0; curr_matt < MAX_MATERIALS; curr_matt++)
    {
        // regardless if the material is active check to see if there is a 
        // dangling texture map
        Destroy_Bitmap(&materials[curr_matt].texture);

        // now it's safe to zero memory out
        memset(&materials[curr_matt], 0, sizeof(MATV1));
    } // end if

    return(1);

} // end Reset_Materials_MATV1

//////////////////////////////////////////////////////////////////////////////

int RGB_16_8_IndexedRGB_Table_Builder(int rgb_format,             // format we want to build table for
                                      // 99/100 565
                                      LPPALETTEENTRY src_palette, // source palette
                                      UCHAR *rgblookup)           // lookup table
{
    // this function takes as input the rgb format that it should generate the lookup table
    // for;  dd_pixel_format = DD_PIXEL_FORMAT565, or DD_PIXEL_FORMAT555
    // notice in 5.5.5 format the input has only 32K possible colors and the high most
    // bit will be disregarded, thus the look up table will only need to be 32K
    // in either case, it's up to the caller to send in the rgblookup table pre-allocated
    // the function doesn't allocate memory for the caller
    // the function uses a simple least squares scan for all possible RGB colors in the
    // 16-bit color space that map to the discrete RGB space in the 8-bit palette 

    // first check the pointers
    if (!src_palette || !rgblookup)
        return(-1);

    // what is the color depth we are building a table for?
    if (rgb_format==DD_PIXEL_FORMAT565)
    {
        // there are a total of 64k entries, perform a loop and look them up, do the least 
        // amount of work, even with a pentium, there are 65536*256 interations here!
        for (int rgbindex = 0; rgbindex < 65536; rgbindex++)
        {
            int  curr_index  = -1;        // current color index of best match
            long curr_error  = INT_MAX;    // distance in color space to nearest match or "error"

            for (int color_index = 0; color_index < 256; color_index++)
            {
                // extract r,g,b from rgbindex, assuming an encoding of 5.6.5, then scale to 8.8.8 since 
                // palette is in that format always
                int r = (rgbindex >> 11) << 3;;
                int g = ((rgbindex >> 5) & 0x3f) << 2;
                int b = (rgbindex & 0x1f) << 3;

                // compute distance to color from target
                long delta_red   = abs(src_palette[color_index].peRed   - r);
                long delta_green = abs(src_palette[color_index].peGreen - g);
                long delta_blue  = abs(src_palette[color_index].peBlue  - b);
                long error = (delta_red*delta_red) + (delta_green*delta_green) + (delta_blue*delta_blue);

                // is this color a better match?
                if (error < curr_error)
                { 
                    curr_index = color_index;
                    curr_error = error;   
                } // end if

            } // end for color_index

            // best match has been found, enter it into table
            rgblookup[rgbindex] = curr_index;             

        } // end for rgbindex

    } // end if
    else 
        if (rgb_format==DD_PIXEL_FORMAT555)
        {
            // there are a total of 32k entries, perform a loop and look them up, do the least 
            // amount of work, even with a pentium, there are 32768*256 interations here!
            for (int rgbindex = 0; rgbindex < 32768; rgbindex++)
            {
                int  curr_index  = -1;        // current color index of best match
                long curr_error  = INT_MAX;    // distance in color space to nearest match or "error"

                for (int color_index = 0; color_index < 256; color_index++)
                {
                    // extract r,g,b from rgbindex, assuming an encoding of 5.6.5, then scale to 8.8.8 since 
                    // palette is in that format always
                    int r =  (rgbindex >> 10) << 3;;
                    int g = ((rgbindex >> 5) & 0x1f) << 3;
                    int b =  (rgbindex & 0x1f) << 3;

                    // compute distance to color from target
                    long delta_red   = abs(src_palette[color_index].peRed   - r);
                    long delta_green = abs(src_palette[color_index].peGreen - g);
                    long delta_blue  = abs(src_palette[color_index].peBlue  - b);
                    long error = (delta_red*delta_red) + (delta_green*delta_green) + (delta_blue*delta_blue);

                    // is this color a better match?
                    if (error < curr_error)
                    { 
                        curr_index = color_index;
                        curr_error = error;   
                    } // end if
                } // end for color_index

                // best match has been found, enter it into table
                rgblookup[rgbindex] = curr_index;             

            } // end for rgbindex

        } // end if
        else
            return(-1); // serious problem! unsupported format, what are you doing to me!!!!

    // return success
    return(1);

} // end RGB_16_8_IndexedRGB_Table_Builder

//////////////////////////////////////////////////////////////////////////////

int RGB_16_8_Indexed_Intensity_Table_Builder(LPPALETTEENTRY src_palette,  // source palette
                                             UCHAR rgbilookup[256][256],  // lookup table
                                             int intensity_normalization)
{
    // this function takes the source palette to compute the intensity shading table with
    // the table will be formatted such that each row is a color index, and each column
    // is the shade 0..255 desired, the output is a single byte index
    // in either case, it's up to the caller to send in the rgbilookup table pre-allocated
    // 64k buffer byte [256][256]the function doesn't allocate memory for the caller
    // the function builds the table by looping thru each color in the color palette and then
    // for each color, it scales the color to maximum intensity without overflow the RGB channels
    // and then uses this as the 100% intensity value of the color, then the algorithm computes
    // the 256 shades of the color, and then uses the standard least squares scan the find the 
    // colors in the palette and stores them in the row of the current color under intensity 
    // translation, sounds diabolical huh? Note: if you set intensity normalization to 0
    // the the maximization step isn't performed.

    int ri,gi,bi;        // initial color 
    int rw,gw,bw;        // current working color
    float ratio;         // scaling ratio
    float dl,dr,db,dg;   // intensity gradients for 256 shades

    // first check the pointers
    if (!src_palette || !rgbilookup)
        return(-1);

    // for each color in the palette, compute maximum intensity value then scan
    // for 256 shades of it
    for (int col_index = 0; col_index < 256; col_index++)
    {
        // extract color from palette
        ri = src_palette[col_index].peRed;
        gi = src_palette[col_index].peGreen;
        bi = src_palette[col_index].peBlue;

        // find largest channel then max it out and scale other
        // channels based on ratio
        if (intensity_normalization==1)
        {
            // red largest?
            if (ri >= gi && ri >= bi)
            {
                // compute scaling ratio
                ratio = (float)255/(float)ri;

                // max colors out
                ri = 255;
                gi = (int)((float)gi * ratio + 0.5);
                bi = (int)((float)bi * ratio + 0.5);

            } // end if
            else // green largest?
                if (gi >= ri && gi >= bi)
                {
                    // compute scaling ratio
                    ratio = (float)255/(float)gi;

                    // max colors out
                    gi = 255;
                    ri = (int)((float)ri * ratio + 0.5);
                    bi = (int)((float)bi * ratio + 0.5);

                } // end if
                else // blue is largest
                {
                    // compute scaling ratio
                    ratio = (float)255/(float)bi;

                    // max colors out
                    bi = 255;
                    ri = (int)((float)ri * ratio + 0.5);
                    gi = (int)((float)gi * ratio + 0.5);
                } // end if

        } // end if

        // at this point, we need to compute the intensity gradients for this color,
        // so we can compute the RGB values for 256 shades of the current color
        dl = sqrt((float)(ri*ri + gi*gi + bi*bi))/(float)256;
        dr = ri/dl,
            db = gi/dl,
            dg = bi/dl;

        // initialize working color
        rw = 0;
        gw = 0;
        bw = 0;

        // at this point rw,gw,bw, is the color that we need to compute the 256 intensities for to 
        // enter into the col_index (th) row of the table
        for (int intensity_index = 0; intensity_index < 256; intensity_index++)
        {
            int  curr_index  = -1;        // current color index of best match
            long curr_error  = INT_MAX;    // distance in color space to nearest match or "error"

            for (int color_index = 0; color_index < 256; color_index++)
            {
                // compute distance to color from target
                long delta_red   = abs(src_palette[color_index].peRed   - rw);
                long delta_green = abs(src_palette[color_index].peGreen - gw);
                long delta_blue  = abs(src_palette[color_index].peBlue  - bw);
                long error = (delta_red*delta_red) + (delta_green*delta_green) + (delta_blue*delta_blue);

                // is this color a better match?
                if (error < curr_error)
                { 
                    curr_index = color_index;
                    curr_error = error;   
                } // end if

            } // end for color_index

            // best match has been found, enter it into table
            rgbilookup[col_index][intensity_index] = curr_index;             

            // compute next intensity level (test for overflow, shouldn't happen, but never know)
            if (rw+=dr > 255) rw=255;
            if (gw+=dg > 255) gw=255;
            if (bw+=db > 255) bw=255;

        } // end for intensity_index

    } // end for c_index

    // return success
    return(1);

} // end RGB_16_8_Indexed_Intensity_Table_Builder

///////////////////////////////////////////////////////////////////////////////

int Insert_OBJECT4DV1_RENDERLIST4DV12(RENDERLIST4DV1_PTR rend_list, 
                                      OBJECT4DV1_PTR obj,
                                      int insert_local,
                                      int lighting_on) 
{
    // converts the entire object into a face list and then inserts
    // the visible, active, non-clipped, non-culled polygons into
    // the render list, also note the flag insert_local control 
    // whether or not the vlist_local or vlist_trans vertex list
    // is used, thus you can insert an object "raw" totally untranformed
    // if you set insert_local to 1, default is 0, that is you would
    // only insert an object after at least the local to world transform
    // the last parameter is used to control if their has been
    // a lighting step that has generated a light value stored
    // in the upper 16-bits of color, if lighting_on = 1 then
    // this value is used to overwrite the base color of the 
    // polygon when its sent to the rendering list

    unsigned int base_color; // save base color of polygon

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

        // test if we should overwrite color with upper 16-bits
        if (lighting_on==1)
        {
            // save color for a sec
            base_color = (unsigned int)(curr_poly->color);
            curr_poly->color = (int)(base_color >> 16);
        } // end if

        // now insert this polygon
        if (!Insert_POLY4DV1_RENDERLIST4DV1(rend_list, curr_poly))
        {
            // fix vertex list pointer
            curr_poly->vlist = vlist_old;

            // the whole object didn't fit!
            return(0);
        } // end if

        // test if we should overwrite color with upper 16-bits
        if (lighting_on==1)
        {
            // fix color upc
            curr_poly->color = (int)(base_color & 0xffff);
        } // end if

        // fix vertex list pointer
        curr_poly->vlist = vlist_old;

    } // end for

    // return success
    return(1);

} // end Insert_OBJECT4DV1_RENDERLIST4DV12

///////////////////////////////////////////////////////////////////////////////

int Light_OBJECT4DV1_World16(OBJECT4DV1_PTR obj,  // object to process
                             CAM4DV1_PTR cam,     // camera position
                             LIGHTV1_PTR lights,  // light list (might have more than one)
                             int max_lights)      // maximum lights in list
{
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


    unsigned int r_base, g_base, b_base,  // base color being lit
        r_sum,  g_sum,  b_sum,   // sum of lighting process over all lights
        shaded_color;            // final color

    float dp,     // dot product 
        dist,   // distance from light to surface
        i,      // general intensities
        nl,     // length of normal
        atten;  // attenuation computations

    // test if the object is culled
    if (!(obj->state & OBJECT4DV1_STATE_ACTIVE) ||
        (obj->state & OBJECT4DV1_STATE_CULLED) ||
        !(obj->state & OBJECT4DV1_STATE_VISIBLE))
        return(0); 

    // process each poly in mesh
    for (int poly=0; poly < obj->num_polys; poly++)
    {
        // acquire polygon
        POLY4DV1_PTR curr_poly = &obj->plist[poly];

        // is this polygon valid?
        // test this polygon if and only if it's not clipped, not culled,
        // active, and visible. Note we test for backface in the event that
        // a previous call might have already determined this, so why work
        // harder!
        if (!(curr_poly->state & POLY4DV1_STATE_ACTIVE) ||
            (curr_poly->state & POLY4DV1_STATE_CLIPPED ) ||
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

        // test the lighting mode of the polygon (use flat for flat, gouraud))
        if (curr_poly->attr & POLY4DV1_ATTR_SHADE_MODE_FLAT || curr_poly->attr & POLY4DV1_ATTR_SHADE_MODE_GOURAUD)
        {
            // step 1: extract the base color out in RGB mode
            if (dd_pixel_format == DD_PIXEL_FORMAT565)
            {
                _RGB565FROM16BIT(curr_poly->color, &r_base, &g_base, &b_base);

                // scale to 8 bit 
                r_base <<= 3;
                g_base <<= 2;
                b_base <<= 3;
            } // end if
            else
            {
                _RGB555FROM16BIT(curr_poly->color, &r_base, &g_base, &b_base);

                // scale to 8 bit 
                r_base <<= 3;
                g_base <<= 3;
                b_base <<= 3;
            } // end if

            // initialize color sum
            r_sum  = 0;
            g_sum  = 0;
            b_sum  = 0;

            // loop thru lights
            for (int curr_light = 0; curr_light < max_lights; curr_light++)
            {
                // is this light active
                if (lights[curr_light].state==LIGHTV1_STATE_OFF)
                    continue;

                // what kind of light are we dealing with
                if (lights[curr_light].attr & LIGHTV1_ATTR_AMBIENT)
                {
                    // simply multiply each channel against the color of the 
                    // polygon then divide by 256 to scale back to 0..255
                    // use a shift in real life!!! >> 8
                    r_sum+= ((lights[curr_light].c_ambient.r * r_base) / 256);
                    g_sum+= ((lights[curr_light].c_ambient.g * g_base) / 256);
                    b_sum+= ((lights[curr_light].c_ambient.b * b_base) / 256);

                    // there better only be one ambient light!

                } // end if
                else
                    if (lights[curr_light].attr & LIGHTV1_ATTR_INFINITE)
                    {
                        // infinite lighting, we need the surface normal, and the direction
                        // of the light source

                        // we need to compute the normal of this polygon face, and recall
                        // that the vertices are in cw order, u=p0->p1, v=p0->p2, n=uxv
                        VECTOR4D u, v, n;

                        // build u, v
                        VECTOR4D_Build(&obj->vlist_trans[ vindex_0 ], &obj->vlist_trans[ vindex_1 ], &u);
                        VECTOR4D_Build(&obj->vlist_trans[ vindex_0 ], &obj->vlist_trans[ vindex_2 ], &v);

                        // compute cross product
                        VECTOR4D_Cross(&u, &v, &n);

                        // at this point, we are almost ready, but we have to normalize the normal vector!
                        // this is a key optimization we can make later, we can pre-compute the length of all polygon
                        // normals, so this step can be optimized
                        // compute length of normal
                        nl = VECTOR4D_Length_Fast(&n);

                        // ok, recalling the lighting model for infinite lights
                        // I(d)dir = I0dir * Cldir
                        // and for the diffuse model
                        // Itotald =   Rsdiffuse*Idiffuse * (n . l)
                        // so we basically need to multiple it all together
                        // notice the scaling by 128, I want to avoid floating point calculations, not because they 
                        // are slower, but the conversion to and from cost cycles

                        dp = VECTOR4D_Dot(&n, &lights[curr_light].dir);

                        // only add light if dp > 0
                        if (dp > 0)
                        { 
                            i = 128*dp/nl; 
                            r_sum+= (lights[curr_light].c_diffuse.r * r_base * i) / (256*128);
                            g_sum+= (lights[curr_light].c_diffuse.g * g_base * i) / (256*128);
                            b_sum+= (lights[curr_light].c_diffuse.b * b_base * i) / (256*128);
                        } // end if

                    } // end if infinite light
                    else
                        if (lights[curr_light].attr & LIGHTV1_ATTR_POINT)
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

                            // we need to compute the normal of this polygon face, and recall
                            // that the vertices are in cw order, u=p0->p1, v=p0->p2, n=uxv
                            VECTOR4D u, v, n, l;

                            // build u, v
                            VECTOR4D_Build(&obj->vlist_trans[ vindex_0 ], &obj->vlist_trans[ vindex_1 ], &u);
                            VECTOR4D_Build(&obj->vlist_trans[ vindex_0 ], &obj->vlist_trans[ vindex_2 ], &v);

                            // compute cross product
                            VECTOR4D_Cross(&u, &v, &n);

                            // at this point, we are almost ready, but we have to normalize the normal vector!
                            // this is a key optimization we can make later, we can pre-compute the length of all polygon
                            // normals, so this step can be optimized
                            // compute length of normal
                            nl = VECTOR4D_Length_Fast(&n);

                            // compute vector from surface to light
                            VECTOR4D_Build(&obj->vlist_trans[ vindex_0 ], &lights[curr_light].pos, &l);

                            // compute distance and attenuation
                            dist = VECTOR4D_Length_Fast(&l);  

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

                        } // end if point
                        else
                            if (lights[curr_light].attr & LIGHTV1_ATTR_SPOTLIGHT1)
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

                                // we need to compute the normal of this polygon face, and recall
                                // that the vertices are in cw order, u=p0->p1, v=p0->p2, n=uxv
                                VECTOR4D u, v, n, l;

                                // build u, v
                                VECTOR4D_Build(&obj->vlist_trans[ vindex_0 ], &obj->vlist_trans[ vindex_1 ], &u);
                                VECTOR4D_Build(&obj->vlist_trans[ vindex_0 ], &obj->vlist_trans[ vindex_2 ], &v);

                                // compute cross product (we need -n, so do vxu)
                                VECTOR4D_Cross(&v, &u, &n);

                                // at this point, we are almost ready, but we have to normalize the normal vector!
                                // this is a key optimization we can make later, we can pre-compute the length of all polygon
                                // normals, so this step can be optimized
                                // compute length of normal
                                nl = VECTOR4D_Length_Fast(&n);

                                // compute vector from surface to light
                                VECTOR4D_Build(&obj->vlist_trans[ vindex_0 ], &lights[curr_light].pos, &l);

                                // compute distance and attenuation
                                dist = VECTOR4D_Length_Fast(&l);  

                                // and for the diffuse model
                                // Itotald =   Rsdiffuse*Idiffuse * (n . l)
                                // so we basically need to multiple it all together
                                // notice the scaling by 128, I want to avoid floating point calculations, not because they 
                                // are slower, but the conversion to and from cost cycles

                                // note that I use the direction of the light here rather than a the vector to the light
                                // thus we are taking orientation into account which is similar to the spotlight model
                                dp = VECTOR4D_Dot(&n, &lights[curr_light].dir);

                                // only add light if dp > 0
                                if (dp > 0)
                                { 
                                    atten =  (lights[curr_light].kc + lights[curr_light].kl*dist + lights[curr_light].kq*dist*dist);    

                                    i = 128*dp / (nl * atten ); 

                                    r_sum += (lights[curr_light].c_diffuse.r * r_base * i) / (256*128);
                                    g_sum += (lights[curr_light].c_diffuse.g * g_base * i) / (256*128);
                                    b_sum += (lights[curr_light].c_diffuse.b * b_base * i) / (256*128);
                                } // end if

                            } // end if spotlight1
                            else
                                if (lights[curr_light].attr & LIGHTV1_ATTR_SPOTLIGHT2) // simple version
                                {
                                    // perform spot light computations
                                    // light model for spot light simple version is once again:
                                    //         	     I0spotlight * Clspotlight * MAX( (l . s), 0)^pf                     
                                    // I(d)spotlight = __________________________________________      
                                    //               		 kc + kl*d + kq*d2        
                                    // Where d = |p - s|, and pf = power factor

                                    // thus it's almost identical to the point, but has the extra term in the numerator
                                    // relating the angle between the light source and the point on the surface

                                    // we need to compute the normal of this polygon face, and recall
                                    // that the vertices are in cw order, u=p0->p1, v=p0->p2, n=uxv
                                    VECTOR4D u, v, n, d, s;

                                    // build u, v
                                    VECTOR4D_Build(&obj->vlist_trans[ vindex_0 ], &obj->vlist_trans[ vindex_1 ], &u);
                                    VECTOR4D_Build(&obj->vlist_trans[ vindex_0 ], &obj->vlist_trans[ vindex_2 ], &v);

                                    // compute cross product (v x u, to invert n)
                                    VECTOR4D_Cross(&v, &u, &n);

                                    // at this point, we are almost ready, but we have to normalize the normal vector!
                                    // this is a key optimization we can make later, we can pre-compute the length of all polygon
                                    // normals, so this step can be optimized
                                    // compute length of normal
                                    nl = VECTOR4D_Length_Fast(&n);

                                    // and for the diffuse model
                                    // Itotald =   Rsdiffuse*Idiffuse * (n . l)
                                    // so we basically need to multiple it all together
                                    // notice the scaling by 128, I want to avoid floating point calculations, not because they 
                                    // are slower, but the conversion to and from cost cycles
                                    dp = VECTOR4D_Dot(&n, &lights[curr_light].dir);

                                    // only add light if dp > 0
                                    if (dp > 0)
                                    { 
                                        // compute vector from light to surface (different from l which IS the light dir)
                                        VECTOR4D_Build( &lights[curr_light].pos, &obj->vlist_trans[ vindex_0 ], &s);

                                        // compute length of s (distance to light source) to normalize s for lighting calc
                                        dist = VECTOR4D_Length_Fast(&s);  

                                        // compute spot light term (s . l)
                                        float dpsl = VECTOR4D_Dot(&s, &lights[curr_light].dir)/dist;

                                        // proceed only if term is positive
                                        if (dpsl > 0) 
                                        {
                                            // compute attenuation
                                            atten = (lights[curr_light].kc + lights[curr_light].kl*dist + lights[curr_light].kq*dist*dist);    

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

                                } // end if spot light

            } // end for light

            // make sure colors aren't out of range
            if (r_sum  > 255) r_sum = 255;
            if (g_sum  > 255) g_sum = 255;
            if (b_sum  > 255) b_sum = 255;

            // write the color
            shaded_color = RGB16Bit(r_sum, g_sum, b_sum);
            curr_poly->color = (int)((shaded_color << 16) | curr_poly->color);

        } // end if
        else // assume POLY4DV1_ATTR_SHADE_MODE_CONSTANT
        {
            // emmisive shading only, copy base color into upper 16-bits
            // without any change
            curr_poly->color = (int)((curr_poly->color << 16) | curr_poly->color);
        } // end if

    } // end for poly

    // return success
    return(1);

} // end Light_OBJECT4DV1_World16

///////////////////////////////////////////////////////////////////////////////

int Light_OBJECT4DV1_World(OBJECT4DV1_PTR obj,  // object to process
                           CAM4DV1_PTR cam,     // camera position
                           LIGHTV1_PTR lights,  // light list (might have more than one)
                           int max_lights)      // maximum lights in list
{
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

    unsigned int r_base, g_base, b_base,  // base color being lit
        r_sum,  g_sum,  b_sum,   // sum of lighting process over all lights
        shaded_color;            // final color

    float dp,     // dot product 
        dist,   // distance from light to surface
        i,      // general intensities
        nl,     // length of normal
        atten;  // attenuation computations

    // test if the object is culled
    if (!(obj->state & OBJECT4DV1_STATE_ACTIVE) ||
        (obj->state & OBJECT4DV1_STATE_CULLED) ||
        !(obj->state & OBJECT4DV1_STATE_VISIBLE))
        return(0); 

    // process each poly in mesh
    for (int poly=0; poly < obj->num_polys; poly++)
    {
        // acquire polygon
        POLY4DV1_PTR curr_poly = &obj->plist[poly];

        // is this polygon valid?
        // test this polygon if and only if it's not clipped, not culled,
        // active, and visible. Note we test for backface in the event that
        // a previous call might have already determined this, so why work
        // harder!
        if (!(curr_poly->state & POLY4DV1_STATE_ACTIVE) ||
            (curr_poly->state & POLY4DV1_STATE_CLIPPED ) ||
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

        // test the lighting mode of the polygon (use flat for flat, gouraud))
        if (curr_poly->attr & POLY4DV1_ATTR_SHADE_MODE_FLAT || curr_poly->attr & POLY4DV1_ATTR_SHADE_MODE_GOURAUD)
        {
            // step 1: extract the base color out in RGB mode (it's already in 8 bits per channel)
            r_base = palette[curr_poly->color].peRed;
            g_base = palette[curr_poly->color].peGreen;
            b_base = palette[curr_poly->color].peBlue;

            // initialize color sum
            r_sum  = 0;
            g_sum  = 0;
            b_sum  = 0;

            // loop thru lights
            for (int curr_light = 0; curr_light < max_lights; curr_light++)
            {
                // is this light active
                if (lights[curr_light].state==LIGHTV1_STATE_OFF)
                    continue;

                // what kind of light are we dealing with
                if (lights[curr_light].attr & LIGHTV1_ATTR_AMBIENT)
                {
                    // simply multiply each channel against the color of the 
                    // polygon then divide by 256 to scale back to 0..255
                    // use a shift in real life!!! >> 8
                    r_sum+= ((lights[curr_light].c_ambient.r * r_base) / 256);
                    g_sum+= ((lights[curr_light].c_ambient.g * g_base) / 256);
                    b_sum+= ((lights[curr_light].c_ambient.b * b_base) / 256);

                    // there better only be one ambient light!

                } // end if
                else
                    if (lights[curr_light].attr & LIGHTV1_ATTR_INFINITE)
                    {
                        // infinite lighting, we need the surface normal, and the direction
                        // of the light source

                        // we need to compute the normal of this polygon face, and recall
                        // that the vertices are in cw order, u=p0->p1, v=p0->p2, n=uxv
                        VECTOR4D u, v, n;

                        // build u, v
                        VECTOR4D_Build(&obj->vlist_trans[ vindex_0 ], &obj->vlist_trans[ vindex_1 ], &u);
                        VECTOR4D_Build(&obj->vlist_trans[ vindex_0 ], &obj->vlist_trans[ vindex_2 ], &v);

                        // compute cross product
                        VECTOR4D_Cross(&u, &v, &n);

                        // at this point, we are almost ready, but we have to normalize the normal vector!
                        // this is a key optimization we can make later, we can pre-compute the length of all polygon
                        // normals, so this step can be optimized
                        // compute length of normal
                        nl = VECTOR4D_Length_Fast(&n);

                        // ok, recalling the lighting model for infinite lights
                        // I(d)dir = I0dir * Cldir
                        // and for the diffuse model
                        // Itotald =   Rsdiffuse*Idiffuse * (n . l)
                        // so we basically need to multiple it all together
                        // notice the scaling by 128, I want to avoid floating point calculations, not because they 
                        // are slower, but the conversion to and from cost cycles

                        dp = VECTOR4D_Dot(&n, &lights[curr_light].dir);

                        // only add light if dp > 0
                        if (dp > 0)
                        { 
                            i = 128*dp/nl; 
                            r_sum+= (lights[curr_light].c_diffuse.r * r_base * i) / (256*128);
                            g_sum+= (lights[curr_light].c_diffuse.g * g_base * i) / (256*128);
                            b_sum+= (lights[curr_light].c_diffuse.b * b_base * i) / (256*128);
                        } // end if

                    } // end if infinite light
                    else
                        if (lights[curr_light].attr & LIGHTV1_ATTR_POINT)
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

                            // we need to compute the normal of this polygon face, and recall
                            // that the vertices are in cw order, u=p0->p1, v=p0->p2, n=uxv
                            VECTOR4D u, v, n, l;

                            // build u, v
                            VECTOR4D_Build(&obj->vlist_trans[ vindex_0 ], &obj->vlist_trans[ vindex_1 ], &u);
                            VECTOR4D_Build(&obj->vlist_trans[ vindex_0 ], &obj->vlist_trans[ vindex_2 ], &v);

                            // compute cross product
                            VECTOR4D_Cross(&u, &v, &n);

                            // at this point, we are almost ready, but we have to normalize the normal vector!
                            // this is a key optimization we can make later, we can pre-compute the length of all polygon
                            // normals, so this step can be optimized
                            // compute length of normal
                            nl = VECTOR4D_Length_Fast(&n);

                            // compute vector from surface to light
                            VECTOR4D_Build(&obj->vlist_trans[ vindex_0 ], &lights[curr_light].pos, &l);

                            // compute distance and attenuation
                            dist = VECTOR4D_Length_Fast(&l);  

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

                        } // end if point
                        else
                            if (lights[curr_light].attr & LIGHTV1_ATTR_SPOTLIGHT1)
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

                                // we need to compute the normal of this polygon face, and recall
                                // that the vertices are in cw order, u=p0->p1, v=p0->p2, n=uxv
                                VECTOR4D u, v, n, l;

                                // build u, v
                                VECTOR4D_Build(&obj->vlist_trans[ vindex_0 ], &obj->vlist_trans[ vindex_1 ], &u);
                                VECTOR4D_Build(&obj->vlist_trans[ vindex_0 ], &obj->vlist_trans[ vindex_2 ], &v);

                                // compute cross product (we need -n, so do vxu)
                                VECTOR4D_Cross(&v, &u, &n);

                                // at this point, we are almost ready, but we have to normalize the normal vector!
                                // this is a key optimization we can make later, we can pre-compute the length of all polygon
                                // normals, so this step can be optimized
                                // compute length of normal
                                nl = VECTOR4D_Length_Fast(&n);

                                // compute vector from surface to light
                                VECTOR4D_Build(&obj->vlist_trans[ vindex_0 ], &lights[curr_light].pos, &l);

                                // compute distance and attenuation
                                dist = VECTOR4D_Length_Fast(&l);  

                                // and for the diffuse model
                                // Itotald =   Rsdiffuse*Idiffuse * (n . l)
                                // so we basically need to multiple it all together
                                // notice the scaling by 128, I want to avoid floating point calculations, not because they 
                                // are slower, but the conversion to and from cost cycles

                                // note that I use the direction of the light here rather than a the vector to the light
                                // thus we are taking orientation into account which is similar to the spotlight model
                                dp = VECTOR4D_Dot(&n, &lights[curr_light].dir);

                                // only add light if dp > 0
                                if (dp > 0)
                                { 
                                    atten =  (lights[curr_light].kc + lights[curr_light].kl*dist + lights[curr_light].kq*dist*dist);    

                                    i = 128*dp / (nl * atten ); 

                                    r_sum += (lights[curr_light].c_diffuse.r * r_base * i) / (256*128);
                                    g_sum += (lights[curr_light].c_diffuse.g * g_base * i) / (256*128);
                                    b_sum += (lights[curr_light].c_diffuse.b * b_base * i) / (256*128);
                                } // end if

                            } // end if spotlight1
                            else
                                if (lights[curr_light].attr & LIGHTV1_ATTR_SPOTLIGHT2) // simple version
                                {
                                    // perform spot light computations
                                    // light model for spot light simple version is once again:
                                    //         	     I0spotlight * Clspotlight * MAX( (l . s), 0)^pf                     
                                    // I(d)spotlight = __________________________________________      
                                    //               		 kc + kl*d + kq*d2        
                                    // Where d = |p - s|, and pf = power factor

                                    // thus it's almost identical to the point, but has the extra term in the numerator
                                    // relating the angle between the light source and the point on the surface

                                    // we need to compute the normal of this polygon face, and recall
                                    // that the vertices are in cw order, u=p0->p1, v=p0->p2, n=uxv
                                    VECTOR4D u, v, n, d, s;

                                    // build u, v
                                    VECTOR4D_Build(&obj->vlist_trans[ vindex_0 ], &obj->vlist_trans[ vindex_1 ], &u);
                                    VECTOR4D_Build(&obj->vlist_trans[ vindex_0 ], &obj->vlist_trans[ vindex_2 ], &v);

                                    // compute cross product (v x u, to invert n)
                                    VECTOR4D_Cross(&v, &u, &n);

                                    // at this point, we are almost ready, but we have to normalize the normal vector!
                                    // this is a key optimization we can make later, we can pre-compute the length of all polygon
                                    // normals, so this step can be optimized
                                    // compute length of normal
                                    nl = VECTOR4D_Length_Fast(&n);

                                    // and for the diffuse model
                                    // Itotald =   Rsdiffuse*Idiffuse * (n . l)
                                    // so we basically need to multiple it all together
                                    // notice the scaling by 128, I want to avoid floating point calculations, not because they 
                                    // are slower, but the conversion to and from cost cycles
                                    dp = VECTOR4D_Dot(&n, &lights[curr_light].dir);

                                    // only add light if dp > 0
                                    if (dp > 0)
                                    { 
                                        // compute vector from light to surface (different from l which IS the light dir)
                                        VECTOR4D_Build( &lights[curr_light].pos, &obj->vlist_trans[ vindex_0 ], &s);

                                        // compute length of s (distance to light source) to normalize s for lighting calc
                                        dist = VECTOR4D_Length_Fast(&s);  

                                        // compute spot light term (s . l)
                                        float dpsl = VECTOR4D_Dot(&s, &lights[curr_light].dir)/dist;

                                        // proceed only if term is positive
                                        if (dpsl > 0) 
                                        {
                                            // compute attenuation
                                            atten = (lights[curr_light].kc + lights[curr_light].kl*dist + lights[curr_light].kq*dist*dist);    

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

                                } // end if spot light

            } // end for light

            // make sure colors aren't out of range
            if (r_sum  > 255) r_sum = 255;
            if (g_sum  > 255) g_sum = 255;
            if (b_sum  > 255) b_sum = 255;

            // use straght rgb look up, assume 565 format, more color space
            int rgbindex = RGB16Bit565(r_sum, g_sum, b_sum);          
            shaded_color = rgblookup[rgbindex];

            // and now insert color index into upper 16 bits
            curr_poly->color = (int)((shaded_color << 16) | curr_poly->color);

        } // end if
        else // assume POLY4DV1_ATTR_SHADE_MODE_CONSTANT
        {
            // emmisive shading only, copy base color into upper 16-bits
            // without any change
            curr_poly->color = (int)((curr_poly->color << 16) | curr_poly->color);
        } // end if

    } // end for poly

    // return success
    return(1);

} // end Light_OBJECT4DV1_World

//////////////////////////////////////////////////////////////////////////////

int Light_RENDERLIST4DV1_World16(RENDERLIST4DV1_PTR rend_list,  // list to process
                                 CAM4DV1_PTR cam,     // camera position
                                 LIGHTV1_PTR lights,  // light list (might have more than one)
                                 int max_lights)      // maximum lights in list
{
    // 16-bit version of function
    // function lights the enture rendering list based on the sent lights and camera. the function supports
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

    unsigned int r_base, g_base, b_base,  // base color being lit
        r_sum,  g_sum,  b_sum,   // sum of lighting process over all lights
        shaded_color;            // final color

    float dp,     // dot product 
        dist,   // distance from light to surface
        i,      // general intensities
        nl,     // length of normal
        atten;  // attenuation computations


    // for each valid poly, light it...
    for (int poly=0; poly < rend_list->num_polys; poly++)
    {
        // acquire polygon
        POLYF4DV1_PTR curr_poly = rend_list->poly_ptrs[poly];

        // light this polygon if and only if it's not clipped, not culled,
        // active, and visible
        if (!(curr_poly->state & POLY4DV1_STATE_ACTIVE) ||
            (curr_poly->state & POLY4DV1_STATE_CLIPPED ) ||
            (curr_poly->state & POLY4DV1_STATE_BACKFACE) )
            continue; // move onto next poly


        // we will use the transformed polygon vertex list since the backface removal
        // only makes sense at the world coord stage further of the pipeline 

        // test the lighting mode of the polygon (use flat for flat, gouraud))
        if (curr_poly->attr & POLY4DV1_ATTR_SHADE_MODE_FLAT || curr_poly->attr & POLY4DV1_ATTR_SHADE_MODE_GOURAUD)
        {
            // step 1: extract the base color out in RGB mode
            if (dd_pixel_format == DD_PIXEL_FORMAT565)
            {
                _RGB565FROM16BIT(curr_poly->color, &r_base, &g_base, &b_base);

                // scale to 8 bit 
                r_base <<= 3;
                g_base <<= 2;
                b_base <<= 3;
            } // end if
            else
            {
                _RGB555FROM16BIT(curr_poly->color, &r_base, &g_base, &b_base);

                // scale to 8 bit 
                r_base <<= 3;
                g_base <<= 3;
                b_base <<= 3;
            } // end if

            // initialize color sum
            r_sum  = 0;
            g_sum  = 0;
            b_sum  = 0;

            // loop thru lights
            for (int curr_light = 0; curr_light < max_lights; curr_light++)
            {
                // is this light active
                if (lights[curr_light].state==LIGHTV1_STATE_OFF)
                    continue;

                // what kind of light are we dealing with
                if (lights[curr_light].attr & LIGHTV1_ATTR_AMBIENT)
                {
                    // simply multiply each channel against the color of the 
                    // polygon then divide by 256 to scale back to 0..255
                    // use a shift in real life!!! >> 8
                    r_sum+= ((lights[curr_light].c_ambient.r * r_base) / 256);
                    g_sum+= ((lights[curr_light].c_ambient.g * g_base) / 256);
                    b_sum+= ((lights[curr_light].c_ambient.b * b_base) / 256);

                    // there better only be one ambient light!

                } // end if
                else
                    if (lights[curr_light].attr & LIGHTV1_ATTR_INFINITE)
                    {
                        // infinite lighting, we need the surface normal, and the direction
                        // of the light source

                        // we need to compute the normal of this polygon face, and recall
                        // that the vertices are in cw order, u=p0->p1, v=p0->p2, n=uxv
                        VECTOR4D u, v, n;

                        // build u, v
                        VECTOR4D_Build(&curr_poly->tvlist[0], &curr_poly->tvlist[1], &u);
                        VECTOR4D_Build(&curr_poly->tvlist[0], &curr_poly->tvlist[2], &v);

                        // compute cross product
                        VECTOR4D_Cross(&u, &v, &n);

                        // at this point, we are almost ready, but we have to normalize the normal vector!
                        // this is a key optimization we can make later, we can pre-compute the length of all polygon
                        // normals, so this step can be optimized
                        // compute length of normal
                        nl = VECTOR4D_Length_Fast(&n);

                        // ok, recalling the lighting model for infinite lights
                        // I(d)dir = I0dir * Cldir
                        // and for the diffuse model
                        // Itotald =   Rsdiffuse*Idiffuse * (n . l)
                        // so we basically need to multiple it all together
                        // notice the scaling by 128, I want to avoid floating point calculations, not because they 
                        // are slower, but the conversion to and from cost cycles

                        dp = VECTOR4D_Dot(&n, &lights[curr_light].dir);

                        // only add light if dp > 0
                        if (dp > 0)
                        { 
                            i = 128*dp/nl; 
                            r_sum+= (lights[curr_light].c_diffuse.r * r_base * i) / (256*128);
                            g_sum+= (lights[curr_light].c_diffuse.g * g_base * i) / (256*128);
                            b_sum+= (lights[curr_light].c_diffuse.b * b_base * i) / (256*128);
                        } // end if

                    } // end if infinite light
                    else
                        if (lights[curr_light].attr & LIGHTV1_ATTR_POINT)
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

                            // we need to compute the normal of this polygon face, and recall
                            // that the vertices are in cw order, u=p0->p1, v=p0->p2, n=uxv
                            VECTOR4D u, v, n, l;

                            // build u, v
                            VECTOR4D_Build(&curr_poly->tvlist[0], &curr_poly->tvlist[1], &u);
                            VECTOR4D_Build(&curr_poly->tvlist[0], &curr_poly->tvlist[2], &v);

                            // compute cross product
                            VECTOR4D_Cross(&u, &v, &n);

                            // at this point, we are almost ready, but we have to normalize the normal vector!
                            // this is a key optimization we can make later, we can pre-compute the length of all polygon
                            // normals, so this step can be optimized
                            // compute length of normal
                            nl = VECTOR4D_Length_Fast(&n);

                            // compute vector from surface to light
                            VECTOR4D_Build(&curr_poly->tvlist[0], &lights[curr_light].pos, &l);

                            // compute distance and attenuation
                            dist = VECTOR4D_Length_Fast(&l);  

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

                        } // end if point
                        else
                            if (lights[curr_light].attr & LIGHTV1_ATTR_SPOTLIGHT1)
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

                                // we need to compute the normal of this polygon face, and recall
                                // that the vertices are in cw order, u=p0->p1, v=p0->p2, n=uxv
                                VECTOR4D u, v, n, l;

                                // build u, v
                                VECTOR4D_Build(&curr_poly->tvlist[0], &curr_poly->tvlist[1], &u);
                                VECTOR4D_Build(&curr_poly->tvlist[0], &curr_poly->tvlist[2], &v);

                                // compute cross product (we need -n, so do vxu)
                                VECTOR4D_Cross(&v, &u, &n);

                                // at this point, we are almost ready, but we have to normalize the normal vector!
                                // this is a key optimization we can make later, we can pre-compute the length of all polygon
                                // normals, so this step can be optimized
                                // compute length of normal
                                nl = VECTOR4D_Length_Fast(&n);

                                // compute vector from surface to light
                                VECTOR4D_Build(&curr_poly->tvlist[0], &lights[curr_light].pos, &l);

                                // compute distance and attenuation
                                dist = VECTOR4D_Length_Fast(&l);  

                                // and for the diffuse model
                                // Itotald =   Rsdiffuse*Idiffuse * (n . l)
                                // so we basically need to multiple it all together
                                // notice the scaling by 128, I want to avoid floating point calculations, not because they 
                                // are slower, but the conversion to and from cost cycles

                                // note that I use the direction of the light here rather than a the vector to the light
                                // thus we are taking orientation into account which is similar to the spotlight model
                                dp = VECTOR4D_Dot(&n, &lights[curr_light].dir);

                                // only add light if dp > 0
                                if (dp > 0)
                                { 
                                    atten =  (lights[curr_light].kc + lights[curr_light].kl*dist + lights[curr_light].kq*dist*dist);    

                                    i = 128*dp / (nl * atten ); 

                                    r_sum += (lights[curr_light].c_diffuse.r * r_base * i) / (256*128);
                                    g_sum += (lights[curr_light].c_diffuse.g * g_base * i) / (256*128);
                                    b_sum += (lights[curr_light].c_diffuse.b * b_base * i) / (256*128);
                                } // end if

                            } // end if spotlight1
                            else
                                if (lights[curr_light].attr & LIGHTV1_ATTR_SPOTLIGHT2) // simple version
                                {
                                    // perform spot light computations
                                    // light model for spot light simple version is once again:
                                    //         	     I0spotlight * Clspotlight * MAX( (l . s), 0)^pf                     
                                    // I(d)spotlight = __________________________________________      
                                    //               		 kc + kl*d + kq*d2        
                                    // Where d = |p - s|, and pf = power factor

                                    // thus it's almost identical to the point, but has the extra term in the numerator
                                    // relating the angle between the light source and the point on the surface

                                    // we need to compute the normal of this polygon face, and recall
                                    // that the vertices are in cw order, u=p0->p1, v=p0->p2, n=uxv
                                    VECTOR4D u, v, n, d, s;

                                    // build u, v
                                    VECTOR4D_Build(&curr_poly->tvlist[0], &curr_poly->tvlist[1], &u);
                                    VECTOR4D_Build(&curr_poly->tvlist[0], &curr_poly->tvlist[2], &v);

                                    // compute cross product (v x u, to invert n)
                                    VECTOR4D_Cross(&v, &u, &n);

                                    // at this point, we are almost ready, but we have to normalize the normal vector!
                                    // this is a key optimization we can make later, we can pre-compute the length of all polygon
                                    // normals, so this step can be optimized
                                    // compute length of normal
                                    nl = VECTOR4D_Length_Fast(&n);

                                    // and for the diffuse model
                                    // Itotald =   Rsdiffuse*Idiffuse * (n . l)
                                    // so we basically need to multiple it all together
                                    // notice the scaling by 128, I want to avoid floating point calculations, not because they 
                                    // are slower, but the conversion to and from cost cycles
                                    dp = VECTOR4D_Dot(&n, &lights[curr_light].dir);

                                    // only add light if dp > 0
                                    if (dp > 0)
                                    { 
                                        // compute vector from light to surface (different from l which IS the light dir)
                                        VECTOR4D_Build( &lights[curr_light].pos, &curr_poly->tvlist[0], &s);

                                        // compute length of s (distance to light source) to normalize s for lighting calc
                                        dist = VECTOR4D_Length_Fast(&s);  

                                        // compute spot light term (s . l)
                                        float dpsl = VECTOR4D_Dot(&s, &lights[curr_light].dir)/dist;

                                        // proceed only if term is positive
                                        if (dpsl > 0) 
                                        {
                                            // compute attenuation
                                            atten = (lights[curr_light].kc + lights[curr_light].kl*dist + lights[curr_light].kq*dist*dist);    

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

                                } // end if spot light

            } // end for light

            // make sure colors aren't out of range
            if (r_sum  > 255) r_sum = 255;
            if (g_sum  > 255) g_sum = 255;
            if (b_sum  > 255) b_sum = 255;

            // write the color over current color
            curr_poly->color = RGB16Bit(r_sum, g_sum, b_sum);

        } // end if
        else // assume POLY4DV1_ATTR_SHADE_MODE_CONSTANT
        {
            // emmisive shading only, do nothing
            // ...
        } // end if

    } // end for poly

    // return success
    return(1);

} // end Light_RENDERLIST4DV1_World16

//////////////////////////////////////////////////////////////////////////////

int Light_RENDERLIST4DV1_World(RENDERLIST4DV1_PTR rend_list,  // list to process
                               CAM4DV1_PTR cam,     // camera position
                               LIGHTV1_PTR lights,  // light list (might have more than one)
                               int max_lights)      // maximum lights in list
{
    // 8-bit version of function
    // function lights the enture rendering list based on the sent lights and camera. the function supports
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

    // also note since we are dealing with a rendering list and not object, the final lit color is
    // immediately written over the real color

    unsigned int r_base, g_base, b_base,  // base color being lit
        r_sum,  g_sum,  b_sum,   // sum of lighting process over all lights
        shaded_color;            // final color

    float dp,     // dot product 
        dist,   // distance from light to surface
        i,      // general intensities
        nl,     // length of normal
        atten;  // attenuation computations


    // for each valid poly, light it...
    for (int poly=0; poly < rend_list->num_polys; poly++)
    {
        // acquire polygon
        POLYF4DV1_PTR curr_poly = rend_list->poly_ptrs[poly];

        // light this polygon if and only if it's not clipped, not culled,
        // active, and visible
        if (!(curr_poly->state & POLY4DV1_STATE_ACTIVE) ||
            (curr_poly->state & POLY4DV1_STATE_CLIPPED ) ||
            (curr_poly->state & POLY4DV1_STATE_BACKFACE) )
            continue; // move onto next poly


        // we will use the transformed polygon vertex list since the backface removal
        // only makes sense at the world coord stage further of the pipeline 

        // test the lighting mode of the polygon (use flat for flat, gouraud))
        if (curr_poly->attr & POLY4DV1_ATTR_SHADE_MODE_FLAT || curr_poly->attr & POLY4DV1_ATTR_SHADE_MODE_GOURAUD)
        {
            // step 1: extract the base color out in RGB mode (it's already in 8 bits per channel)
            r_base = palette[curr_poly->color].peRed;
            g_base = palette[curr_poly->color].peGreen;
            b_base = palette[curr_poly->color].peBlue;


            // initialize color sum
            r_sum  = 0;
            g_sum  = 0;
            b_sum  = 0;

            // loop thru lights
            for (int curr_light = 0; curr_light < max_lights; curr_light++)
            {
                // is this light active
                if (lights[curr_light].state==LIGHTV1_STATE_OFF)
                    continue;

                // what kind of light are we dealing with
                if (lights[curr_light].attr & LIGHTV1_ATTR_AMBIENT)
                {
                    // simply multiply each channel against the color of the 
                    // polygon then divide by 256 to scale back to 0..255
                    // use a shift in real life!!! >> 8
                    r_sum+= ((lights[curr_light].c_ambient.r * r_base) / 256);
                    g_sum+= ((lights[curr_light].c_ambient.g * g_base) / 256);
                    b_sum+= ((lights[curr_light].c_ambient.b * b_base) / 256);

                    // there better only be one ambient light!

                } // end if
                else
                    if (lights[curr_light].attr & LIGHTV1_ATTR_INFINITE)
                    {
                        // infinite lighting, we need the surface normal, and the direction
                        // of the light source

                        // we need to compute the normal of this polygon face, and recall
                        // that the vertices are in cw order, u=p0->p1, v=p0->p2, n=uxv
                        VECTOR4D u, v, n;

                        // build u, v
                        VECTOR4D_Build(&curr_poly->tvlist[0], &curr_poly->tvlist[1], &u);
                        VECTOR4D_Build(&curr_poly->tvlist[0], &curr_poly->tvlist[2], &v);

                        // compute cross product
                        VECTOR4D_Cross(&u, &v, &n);

                        // at this point, we are almost ready, but we have to normalize the normal vector!
                        // this is a key optimization we can make later, we can pre-compute the length of all polygon
                        // normals, so this step can be optimized
                        // compute length of normal
                        nl = VECTOR4D_Length_Fast(&n);

                        // ok, recalling the lighting model for infinite lights
                        // I(d)dir = I0dir * Cldir
                        // and for the diffuse model
                        // Itotald =   Rsdiffuse*Idiffuse * (n . l)
                        // so we basically need to multiple it all together
                        // notice the scaling by 128, I want to avoid floating point calculations, not because they 
                        // are slower, but the conversion to and from cost cycles

                        dp = VECTOR4D_Dot(&n, &lights[curr_light].dir);

                        // only add light if dp > 0
                        if (dp > 0)
                        { 
                            i = 128*dp/nl; 
                            r_sum+= (lights[curr_light].c_diffuse.r * r_base * i) / (256*128);
                            g_sum+= (lights[curr_light].c_diffuse.g * g_base * i) / (256*128);
                            b_sum+= (lights[curr_light].c_diffuse.b * b_base * i) / (256*128);
                        } // end if

                    } // end if infinite light
                    else
                        if (lights[curr_light].attr & LIGHTV1_ATTR_POINT)
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

                            // we need to compute the normal of this polygon face, and recall
                            // that the vertices are in cw order, u=p0->p1, v=p0->p2, n=uxv
                            VECTOR4D u, v, n, l;

                            // build u, v
                            VECTOR4D_Build(&curr_poly->tvlist[0], &curr_poly->tvlist[1], &u);
                            VECTOR4D_Build(&curr_poly->tvlist[0], &curr_poly->tvlist[2], &v);

                            // compute cross product
                            VECTOR4D_Cross(&u, &v, &n);

                            // at this point, we are almost ready, but we have to normalize the normal vector!
                            // this is a key optimization we can make later, we can pre-compute the length of all polygon
                            // normals, so this step can be optimized
                            // compute length of normal
                            nl = VECTOR4D_Length_Fast(&n);

                            // compute vector from surface to light
                            VECTOR4D_Build(&curr_poly->tvlist[0], &lights[curr_light].pos, &l);

                            // compute distance and attenuation
                            dist = VECTOR4D_Length_Fast(&l);  

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

                        } // end if point
                        else
                            if (lights[curr_light].attr & LIGHTV1_ATTR_SPOTLIGHT1)
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

                                // we need to compute the normal of this polygon face, and recall
                                // that the vertices are in cw order, u=p0->p1, v=p0->p2, n=uxv
                                VECTOR4D u, v, n, l;

                                // build u, v
                                VECTOR4D_Build(&curr_poly->tvlist[0], &curr_poly->tvlist[1], &u);
                                VECTOR4D_Build(&curr_poly->tvlist[0], &curr_poly->tvlist[2], &v);

                                // compute cross product (we need -n, so do vxu)
                                VECTOR4D_Cross(&v, &u, &n);

                                // at this point, we are almost ready, but we have to normalize the normal vector!
                                // this is a key optimization we can make later, we can pre-compute the length of all polygon
                                // normals, so this step can be optimized
                                // compute length of normal
                                nl = VECTOR4D_Length_Fast(&n);

                                // compute vector from surface to light
                                VECTOR4D_Build(&curr_poly->tvlist[0], &lights[curr_light].pos, &l);

                                // compute distance and attenuation
                                dist = VECTOR4D_Length_Fast(&l);  

                                // and for the diffuse model
                                // Itotald =   Rsdiffuse*Idiffuse * (n . l)
                                // so we basically need to multiple it all together
                                // notice the scaling by 128, I want to avoid floating point calculations, not because they 
                                // are slower, but the conversion to and from cost cycles

                                // note that I use the direction of the light here rather than a the vector to the light
                                // thus we are taking orientation into account which is similar to the spotlight model
                                dp = VECTOR4D_Dot(&n, &lights[curr_light].dir);

                                // only add light if dp > 0
                                if (dp > 0)
                                { 
                                    atten =  (lights[curr_light].kc + lights[curr_light].kl*dist + lights[curr_light].kq*dist*dist);    

                                    i = 128*dp / (nl * atten ); 

                                    r_sum += (lights[curr_light].c_diffuse.r * r_base * i) / (256*128);
                                    g_sum += (lights[curr_light].c_diffuse.g * g_base * i) / (256*128);
                                    b_sum += (lights[curr_light].c_diffuse.b * b_base * i) / (256*128);
                                } // end if

                            } // end if spotlight1
                            else
                                if (lights[curr_light].attr & LIGHTV1_ATTR_SPOTLIGHT2) // simple version
                                {
                                    // perform spot light computations
                                    // light model for spot light simple version is once again:
                                    //         	     I0spotlight * Clspotlight * MAX( (l . s), 0)^pf                     
                                    // I(d)spotlight = __________________________________________      
                                    //               		 kc + kl*d + kq*d2        
                                    // Where d = |p - s|, and pf = power factor

                                    // thus it's almost identical to the point, but has the extra term in the numerator
                                    // relating the angle between the light source and the point on the surface

                                    // we need to compute the normal of this polygon face, and recall
                                    // that the vertices are in cw order, u=p0->p1, v=p0->p2, n=uxv
                                    VECTOR4D u, v, n, d, s;

                                    // build u, v
                                    VECTOR4D_Build(&curr_poly->tvlist[0], &curr_poly->tvlist[1], &u);
                                    VECTOR4D_Build(&curr_poly->tvlist[0], &curr_poly->tvlist[2], &v);

                                    // compute cross product (v x u, to invert n)
                                    VECTOR4D_Cross(&v, &u, &n);

                                    // at this point, we are almost ready, but we have to normalize the normal vector!
                                    // this is a key optimization we can make later, we can pre-compute the length of all polygon
                                    // normals, so this step can be optimized
                                    // compute length of normal
                                    nl = VECTOR4D_Length_Fast(&n);

                                    // and for the diffuse model
                                    // Itotald =   Rsdiffuse*Idiffuse * (n . l)
                                    // so we basically need to multiple it all together
                                    // notice the scaling by 128, I want to avoid floating point calculations, not because they 
                                    // are slower, but the conversion to and from cost cycles
                                    dp = VECTOR4D_Dot(&n, &lights[curr_light].dir);

                                    // only add light if dp > 0
                                    if (dp > 0)
                                    { 
                                        // compute vector from light to surface (different from l which IS the light dir)
                                        VECTOR4D_Build( &lights[curr_light].pos, &curr_poly->tvlist[0], &s);

                                        // compute length of s (distance to light source) to normalize s for lighting calc
                                        dist = VECTOR4D_Length_Fast(&s);  

                                        // compute spot light term (s . l)
                                        float dpsl = VECTOR4D_Dot(&s, &lights[curr_light].dir)/dist;

                                        // proceed only if term is positive
                                        if (dpsl > 0) 
                                        {
                                            // compute attenuation
                                            atten = (lights[curr_light].kc + lights[curr_light].kl*dist + lights[curr_light].kq*dist*dist);    

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

                                } // end if spot light

            } // end for light

            // make sure colors aren't out of range
            if (r_sum  > 255) r_sum = 255;
            if (g_sum  > 255) g_sum = 255;
            if (b_sum  > 255) b_sum = 255;

            // look color up
            int rgbindex = RGB16Bit565(r_sum, g_sum, b_sum);          

            // write the color over current color
            curr_poly->color = rgblookup[rgbindex];

        } // end if
        else // assume POLY4DV1_ATTR_SHADE_MODE_CONSTANT
        {
            // emmisive shading only, do nothing
            // ...
        } // end if

    } // end for poly

    // return success
    return(1);

} // end Light_RENDERLIST4DV1_World

///////////////////////////////////////////////////////////////////////////////

void Sort_RENDERLIST4DV1(RENDERLIST4DV1_PTR rend_list, int sort_method)
{
    // this function sorts the rendering list based on the polygon z-values 
    // the specific sorting method is controlled by sending in control flags
    // #define SORT_POLYLIST_AVGZ  0 - sorts on average of all vertices
    // #define SORT_POLYLIST_NEARZ 1 - sorts on closest z vertex of each poly
    // #define SORT_POLYLIST_FARZ  2 - sorts on farthest z vertex of each poly

    switch(sort_method)
    {
    case SORT_POLYLIST_AVGZ:  //  - sorts on average of all vertices
        {
            qsort((void *)rend_list->poly_ptrs, rend_list->num_polys, sizeof(POLYF4DV1_PTR), Compare_AvgZ_POLYF4DV1);
        } break;

    case SORT_POLYLIST_NEARZ: // - sorts on closest z vertex of each poly
        {
            qsort((void *)rend_list->poly_ptrs, rend_list->num_polys, sizeof(POLYF4DV1_PTR), Compare_NearZ_POLYF4DV1);
        } break;

    case SORT_POLYLIST_FARZ:  //  - sorts on farthest z vertex of each poly
        {
            qsort((void *)rend_list->poly_ptrs, rend_list->num_polys, sizeof(POLYF4DV1_PTR), Compare_FarZ_POLYF4DV1);
        } break;

    default: break;
    } // end switch

} // end Sort_RENDERLIST4DV1

////////////////////////////////////////////////////////////////////////////////

int Compare_AvgZ_POLYF4DV1(const void *arg1, const void *arg2)
{
    // this function comapares the average z's of two polygons and is used by the
    // depth sort surface ordering algorithm

    float z1, z2;

    POLYF4DV1_PTR poly_1, poly_2;

    // dereference the poly pointers
    poly_1 = *((POLYF4DV1_PTR *)(arg1));
    poly_2 = *((POLYF4DV1_PTR *)(arg2));

    // compute z average of each polygon
    z1 = (float)0.33333*(poly_1->tvlist[0].z + poly_1->tvlist[1].z + poly_1->tvlist[2].z);

    // now polygon 2
    z2 = (float)0.33333*(poly_2->tvlist[0].z + poly_2->tvlist[1].z + poly_2->tvlist[2].z);

    // compare z1 and z2, such that polys' will be sorted in descending Z order
    if (z1 > z2)
        return(-1);
    else
        if (z1 < z2)
            return(1);
        else
            return(0);

} // end Compare_AvgZ_POLYF4DV1

////////////////////////////////////////////////////////////////////////////////

int Compare_NearZ_POLYF4DV1(const void *arg1, const void *arg2)
{
    // this function comapares the closest z's of two polygons and is used by the
    // depth sort surface ordering algorithm

    float z1, z2;

    POLYF4DV1_PTR poly_1, poly_2;

    // dereference the poly pointers
    poly_1 = *((POLYF4DV1_PTR *)(arg1));
    poly_2 = *((POLYF4DV1_PTR *)(arg2));

    // compute the near z of each polygon
    z1 = MIN(poly_1->tvlist[0].z, poly_1->tvlist[1].z);
    z1 = MIN(z1, poly_1->tvlist[2].z);

    z2 = MIN(poly_2->tvlist[0].z, poly_2->tvlist[1].z);
    z2 = MIN(z2, poly_2->tvlist[2].z);

    // compare z1 and z2, such that polys' will be sorted in descending Z order
    if (z1 > z2)
        return(-1);
    else
        if (z1 < z2)
            return(1);
        else
            return(0);

} // end Compare_NearZ_POLYF4DV1

////////////////////////////////////////////////////////////////////////////////

int Compare_FarZ_POLYF4DV1(const void *arg1, const void *arg2)
{
    // this function comapares the farthest z's of two polygons and is used by the
    // depth sort surface ordering algorithm

    float z1, z2;

    POLYF4DV1_PTR poly_1, poly_2;

    // dereference the poly pointers
    poly_1 = *((POLYF4DV1_PTR *)(arg1));
    poly_2 = *((POLYF4DV1_PTR *)(arg2));

    // compute the near z of each polygon
    z1 = MAX(poly_1->tvlist[0].z, poly_1->tvlist[1].z);
    z1 = MAX(z1, poly_1->tvlist[2].z);

    z2 = MAX(poly_2->tvlist[0].z, poly_2->tvlist[1].z);
    z2 = MAX(z2, poly_2->tvlist[2].z);

    // compare z1 and z2, such that polys' will be sorted in descending Z order
    if (z1 > z2)
        return(-1);
    else
        if (z1 < z2)
            return(1);
        else
            return(0);

} // end Compare_FarZ_POLYF4DV1