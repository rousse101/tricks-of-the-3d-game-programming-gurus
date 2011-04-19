// DEMOII8_7.CPP - Parser demo
// READ THIS!
// To compile make sure to include DDRAW.LIB, DSOUND.LIB,
// DINPUT.LIB, DINPUT8.LIB, WINMM.LIB in the project link list, and of course 
// the C++ source modules T3DLIB1,4,5,6.CPP and the headers T3DLIB1,4,5,6.H
// be in the working directory of the compiler
// compile as a win32 CONSOLE APPLICATION not a WIN32 .EXE!!!

// I N C L U D E S ///////////////////////////////////////////////////////////

#define DEBUG_ON

#define INITGUID

#define WIN32_LEAN_AND_MEAN  

#include <windows.h>   // include important windows stuff
#include <windowsx.h> 
#include <mmsystem.h>
#include <objbase.h>
#include <iostream.h> // include important C/C++ stuff
#include <conio.h>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <math.h>
#include <io.h>
#include <fcntl.h>
#include <direct.h>
#include <wchar.h>
#include <limits.h>
#include <float.h>

#include <ddraw.h>      // needed for defs in T3DLIB1.H 
#include "T3DLIB1.H"
#include "T3DLIB4.H"
#include "T3DLIB5.H"
#include "T3DLIB6.H"

// GLOBALS //////////////////////////////////////////////////////////////////

HWND main_window_handle           = NULL; // save the window handle
HINSTANCE main_instance           = NULL; // save the instance

// MAIN //////////////////////////////////////////////////////////////////

void main()
{
CPARSERV1 parser;     // create a parser
char filename[80];    // holds fileaname
char pattern[80];     // holds pattern
int sel;              // user input

// enter main loop
while(1)
     {
     // display menu
     printf("\nParser Menu:\n");
     printf("\n1. Enter in a filename to load.");
     printf("\n2. Display file untouched.");     
     printf("\n3. Display file with all comments removed (comments begin with #).");
     printf("\n4. Display file with all blank lines and comments removed.");
     printf("\n5. Search file for pattern.");
     printf("\n6. Exit program.\n");
     printf("\nPlease make a selection?");

     // get user selection
     scanf("%d", &sel);

     // what to do?
     switch(sel)
           {
           case 1: // enter filename
                {
                printf("\nEnter filename?");
                scanf("%s", filename);
                } break;
        
           case 2: // display file untouched
                {
                // open the file
                if (!parser.Open(filename))
                   { printf("\nFile not found!"); break; }

                int line = 0;

                // loop until no more lines
                while(parser.Getline(0))
                     printf("line %d: %s", line++, parser.buffer);                                     
 
                parser.Close();

                } break;

           case 3: // display file with comments removed
                {
                // open the file
                if (!parser.Open(filename))
                   { printf("\nFile not found!"); break; }

                int line = 0;

                // loop until no more lines
                while(parser.Getline(PARSER_STRIP_COMMENTS))
                     printf("line %d: %s", line++, parser.buffer);                                     
 
                parser.Close();
                } break;

           case 4: // Display file with all blank lines and comments removed
                {
                // open the file
                if (!parser.Open(filename))
                   { printf("\nFile not found!"); break; }

                int line = 0;
       
                // loop until no more lines
                while(parser.Getline(PARSER_STRIP_COMMENTS | PARSER_STRIP_EMPTY_LINES))
                     printf("line %d: %s", line++, parser.buffer);                                     
 
                parser.Close();
                } break;

           case 5: // Search file for pattern
                {
                printf("\nEnter pattern to match?");
                pattern[0] = 80;
                _cgets(pattern);

                // open the file
                if (!parser.Open(filename))
                   { printf("\nFile not found!"); break; }

                // loop until no more lines
                while(parser.Getline(PARSER_STRIP_COMMENTS | PARSER_STRIP_EMPTY_LINES))
                     {
                     // does this line have the pattern?
                     if (parser.Pattern_Match(parser.buffer, &pattern[2]))
                        {
                        printf("\nPattern: \"%s\" matched, stats:", &pattern[2]); 

                        printf("\n%d ints matched", parser.num_pints);
                        for (int i=0; i < parser.num_pints; i++)
                            printf("\nInt[%d]: %i", i, parser.pints[i]);
        
                        printf("\n%d floats matched", parser.num_pfloats);
                        for (int f=0; f < parser.num_pfloats; f++)
                            printf("\nFloat[%d]: %f", f, parser.pfloats[f]);

                        printf("\n%d strings matched", parser.num_pstrings);
                        for (int s=0; s < parser.num_pstrings; s++)
                            printf("\nString[%d]: %s", s, parser.pstrings[s]);

                        } // end if
     
                } // end while

                parser.Close();
                } break;


           case 6: // exit program
                {
                exit(0);
                } break;
        
     } // end switch

    } // end while

} // end main