
/***********************************************************************
 *   Code is stol^H^H^H^Hbased on wmppp, wmload, and wmtme  
 *
 *   Author: Ben Cohen (buddog@aztec.asu.edu)
 *
 *   This program is distributed under the GPL license --
 *   !! Except for those parts ( face images ) which are certaintly 
 *   owned by ID Software. !!
 *
 ***********************************************************************/


    


#include <Xlib.h>
#include <Xutil.h>
#include <xpm.h>
#include <extensions/shape.h>
#include <keysym.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>


/* XPM files */

#include "backdrop.xpm"
/* #include "buttons.xpm" */
#include "face_0c.xpm"
#include "face_0r.xpm"
#include "face_0l.xpm"
#include "face_1c.xpm"
#include "face_1r.xpm"
#include "face_1l.xpm"
#include "face_2c.xpm"
#include "face_2r.xpm"
#include "face_2l.xpm"
#include "face_3c.xpm"
#include "face_3r.xpm"
#include "face_3l.xpm"
#include "face_4c.xpm"
#include "face_4r.xpm"
#include "face_4l.xpm"
#include "face_god.xpm"

#include "mask.xbm"

/***********************************************************************
 *         Globals..   
 ***********************************************************************/

#define VERSION "wmdl 1.3.7"
#define LEVELS 5

typedef enum { YES, NO } yesno_type;
typedef enum { CPU, LOAD, COMMAND } mode_type;

typedef struct {
    Pixmap         pixmap;
    Pixmap         mask;
    XpmAttributes  attributes;
} XpmIcon_type;


typedef struct {
    char     mode_str[32];
    mode_t   mode;
    char     command_str[256];
    int      index;
} command_type;


Display * display;
int screen;
Window rootwin, win, iconwin;
GC gc;
int depth;
Pixel bg_pixel, fg_pixel;

Pixmap pixmask;

XpmIcon_type template, visible;
XpmIcon_type face[16];

int Verbose = 0;


/***********************************************************************
 *         Function Prototypes
 ***********************************************************************/

void redraw( int level, int dirrection );
void getPixmaps();
int whichButton( int x, int y );
int flush_expose(Window w);
void show_usage();
float get_cpu( float scale_factor );
float get_load( float scale_factor );
float get_command( command_type command, float scale_factor );
void new_dirrection( int *dirrection );
void refresh( command_type command, float scale_factor, int dirrection );






/***********************************************************************
 *         main
 ***********************************************************************/

int main( int argc, char ** argv )
{
    XEvent report;
    XGCValues xgcValues;    

    XSizeHints xsizehints;
    XWMHints * xwmhints;
    XClassHint xclasshint;

    XTextProperty app_name_atom;


    char * app_name = "wmdl";

    int dirrection = 0;
    long update_miliseconds = 999; 

    char window_state_str[32] = "withdrawn";
    unsigned int window_state = WithdrawnState;

    char is_shaped_str[32] = "yes";
    yesno_type is_shaped = YES;

    float scale_factor = 1.0;
    yesno_type user_selected_f = NO;

    command_type command;
    
    int border = 5;
    int dummy = 0;
    int i;

    char Geometry_str[64] = "64x64+5+5";
    char Display_str[64] = "";


    strcpy (command.mode_str, "load");
    command.mode = LOAD;
    strcpy(command.command_str, "");
    command.index = 0;

    /* Parse Command Line Arguments */

    for ( i=1; i<argc; i++ ) {
        if ( *argv[i] == '-' ) {
            switch ( *(argv[i]+1) ) {
                case 'v':
                    Verbose = 1;
                    break;
                case 's':
                    if ( ++i >= argc ) show_usage();
                    strcpy( is_shaped_str, argv[i] );
                    if ( strcmp(is_shaped_str,"yes") == 0 ) {
                        is_shaped = YES;
                    }
                    if ( strcmp(is_shaped_str,"no") == 0 ) {
                        is_shaped = NO;
                    }
                    break;
                case 'w':
                    if ( ++i >= argc ) show_usage();
                    strcpy( window_state_str, argv[i] );
                    if ( strcmp(window_state_str,"iconic") == 0 ) {
                            window_state = IconicState;
                    }
                    if ( strcmp(window_state_str,"normal") == 0 ) {
                            window_state = NormalState;
                    }
                    if ( strcmp(window_state_str,"withdrawn") == 0 ) {
                            window_state = WithdrawnState;
                    }
                    break;
                case 'g':
                    if ( ++i >= argc ) show_usage();
                    strcpy( Geometry_str, argv[i] );
                    break;
                case 'd':
                    if ( ++i >= argc ) show_usage();
                    strcpy( Display_str, argv[i] );
                    break;
                case 'u':
                    if ( ++i >= argc ) show_usage();
                    update_miliseconds = atol(argv[i]);
                    break;
                case 'm':
                    if ( ++i >= argc ) show_usage();
                    strcpy( command.mode_str, argv[i] );
                    if ( strcmp(command.mode_str,"load") == 0 ) {
                        command.mode = LOAD;
                    }
                    if ( strcmp(command.mode_str,"cpu") == 0 ) {
                        command.mode = CPU;
                    }
                    if ( strcmp(command.mode_str,"command") == 0 ) {
                        command.mode = COMMAND;
                        if ( ++i >= argc ) show_usage();
                        strcpy(command.command_str, argv[i]);
                        if ( ++i >= argc ) show_usage();
                        command.index = atoi(argv[i]);
                    }
                    break;
                case 'f':
                    if ( ++i >= argc ) show_usage();
                    scale_factor = atof(argv[i]);
                    user_selected_f = YES;
                    break;
                    
                case 'h':
                    show_usage();
                    break;
            }
        }
    }




    /* Default scale factor values unless user overrides on command line */

    if ( user_selected_f == NO ) {
        if ( command.mode == CPU ) {
            scale_factor = 1.0;
        }
        if ( command.mode == LOAD ||
             command.mode == COMMAND ) {
            scale_factor = 2.0;
        }
    }




    if ( Verbose ) {
        printf("Command Line Arguments:\n");
        printf("-m mode         = %s ", command.mode_str );
        if ( strcmp(command.mode_str,"command") == 0 ) 
            printf("'%s' %d", command.command_str, command.index);
        printf("\n");
        printf("-f scale factor = %f\n", scale_factor ); 
        printf("-u update speed = %ld\n", update_miliseconds );
        printf("-w window_state = %s\n", window_state_str );
        printf("-s shaped       = %s\n", is_shaped_str );
        printf("-g geometry     = %s\n", Geometry_str );
        printf("-d display      = %s\n", Display_str );
        printf("\n\n");

    }

    if ( (display = XOpenDisplay(Display_str)) == NULL ) {
        fprintf(stderr,"Fail: XOpenDisplay for %s\n", Display_str);    
        exit(-1);
    }


    screen = DefaultScreen(display);
    rootwin = RootWindow(display,screen);
    depth = DefaultDepth(display, screen);

    bg_pixel = WhitePixel(display, screen ); 
    fg_pixel = BlackPixel(display, screen ); 

    if ( Verbose ) {
       printf ("Default Screen = %d\n", screen );
       printf ("Color Depth    = %d\n", depth );
    }


    xsizehints.flags = USSize | USPosition;
    xsizehints.width = 64;
    xsizehints.height = 64;


    /* Parse Geometry string and fill in sizehints fields */

    XWMGeometry(display, screen, 
                Geometry_str, 
                NULL,     
                border, 
                &xsizehints,
                &xsizehints.x, 
                &xsizehints.y,
                &xsizehints.width,
                &xsizehints.height, 
                &dummy);


    if ( (win = XCreateSimpleWindow( 
        display,
        rootwin,
        xsizehints.x,
        xsizehints.y,
        xsizehints.width,
        xsizehints.height,
        border,
        fg_pixel, bg_pixel) ) == 0 ) {
            fprintf(stderr,"Fail: XCreateSimpleWindow\n");    
            exit(-1);
    }

    if ( (iconwin = XCreateSimpleWindow( 
        display,
        win,
        xsizehints.x,
        xsizehints.y,
        xsizehints.width,
        xsizehints.height,
        border,
        fg_pixel, bg_pixel) ) == 0 ) {
            fprintf(stderr,"Fail: XCreateSimpleWindow\n");    
            exit(-1);
    }




    /* Set up shaped windows                                      */
    /* Gives the appicon a border so you can grab and move it.     */

    if ( is_shaped == YES ) {
        if ( ( pixmask = XCreateBitmapFromData(display, win,
                            mask_bits, mask_width, mask_height) ) == 0 ) {
                fprintf(stderr,"Fail: XCreateBitmapFromData\n");
        }

        XShapeCombineMask(display,win,    ShapeBounding,0,0, pixmask,ShapeSet);
        XShapeCombineMask(display,iconwin,ShapeBounding,0,0, pixmask,ShapeSet);
    }



    /* Convert in pixmaps from .xpm includes. */
    getPixmaps();

    


    /* Interclient Communication stuff */
    /* Appicons don't work with out this stuff */
    
    xwmhints = XAllocWMHints();

    xwmhints->flags = WindowGroupHint | IconWindowHint | StateHint;    
    xwmhints->icon_window = iconwin;
    xwmhints->window_group = win;
    xwmhints->initial_state = window_state;
    XSetWMHints( display, win, xwmhints );
    
    xclasshint.res_name = "wmdl";
    xclasshint.res_class = "WMdl";
    XSetClassHint( display, win, &xclasshint );

    XSetWMNormalHints( display, win, &xsizehints );





    /* Tell window manager what the title bar name is. We never see        */
    /* this anyways in the WithdrawnState                                 */

    if ( XStringListToTextProperty(&app_name, 1, &app_name_atom) == 0 ) {
        fprintf(stderr,"%s: Can't set up window name\n", app_name);
        exit(-1);
    }
    XSetWMName( display, win, &app_name_atom );
            


    /* Create Graphic Context */
    
    if ( (gc = XCreateGC( 
        display,
        win,
        (GCForeground | GCBackground),
        &xgcValues)) == NULL ) {
            fprintf(stderr,"Fail: XCreateGC\n");    
            exit(-1);
    }





    /* XEvent Masks. We want both window to process X events */

    XSelectInput(
        display,
        win,
        ExposureMask | 
        ButtonPressMask | 
        PointerMotionMask |
        StructureNotifyMask );

    XSelectInput(
        display,
        iconwin,
        ExposureMask | 
        ButtonPressMask | 
        PointerMotionMask |
        StructureNotifyMask );



    /* Store the 'state' of the application for restarting */

    XSetCommand( display, win, argv, argc );    


    /* Window won't ever show up until it is mapped.. then drawn after a     */
    /* ConfigureNotify                                                         */

    XMapWindow( display, win );



    /* X Event Loop */

    while ( 1 ) {

        usleep( 1000 * update_miliseconds );

        new_dirrection( & dirrection );
        refresh( command, scale_factor, dirrection );

        while ( XPending(display) ) {

            XNextEvent(display, &report );

            switch (report.type) {
                case Expose:
                    if (report.xexpose.count != 0) {    
                        break;
                    }
                    if ( Verbose ) 
                        fprintf(stdout,"Event: Expose\n");    
                    refresh( command, scale_factor, dirrection );
                    break;
    
    
                case ConfigureNotify:
                    if ( Verbose )     
                        fprintf(stdout,"Event: ConfigureNotify\n");    
                    refresh( command, scale_factor, dirrection );
                    break;
    
    
                case ButtonPress:
    
                    switch (report.xbutton.button) {
                        case Button1:
                        case Button2:
                        case Button3:
                            refresh( command, scale_factor, dirrection );
                            break;
    
                    }
                    if ( Verbose ) 
                        fprintf(stdout,"Button Press: x=%d y=%d \n", 
                            report.xbutton.x, report.xbutton.y );
    
                    break;
    
    
                case DestroyNotify:
                    XFreeGC(display, gc);
                    XDestroyWindow(display,win);
                    XDestroyWindow(display,iconwin);
                    XCloseDisplay(display);
                    exit(0);
                    break;

             }
        }
    }


return (0);
}


/***********************************************************************
 *    new_dirrection
 *
 *    Randomly choose to move head left, right or not at all.
 *
 ***********************************************************************/

void new_dirrection( int *dirrection )
{
    int random_dirrection;

    /* Random walk in range: -1, 0, 1 */
    random_dirrection = 
            (int) floor( ( rand() / (float) RAND_MAX ) * (3.0-.001) ) -1 ;


    (*dirrection) += random_dirrection;

    if ( *dirrection > 2 )
        *dirrection = 2;

    if ( *dirrection < 0 )
        *dirrection = 0;

}


/***********************************************************************
 *   refresh
 *
 *    Get scaled LOAD or CPU percentage and call redraw()
 *
 ***********************************************************************/

void refresh( command_type command, float scale_factor, int dirrection ) 
{
    float percent;
    int level;

    switch ( command.mode ) {
        case CPU:
            percent = get_cpu( scale_factor );
            break;
        case LOAD:
            percent = get_load( scale_factor );
            break;
        case COMMAND:
            percent = get_command( command, scale_factor );
            break;
    }

    level = LEVELS * ( percent );

    if ( Verbose )
        printf("%f%% scaled --> face %d\n", percent, level );

    redraw( level , dirrection );
}




/***********************************************************************
 *   redraw
 *
 *    Draw windows from appropriate pixmap images.
 *
 ***********************************************************************/

void redraw( int level, int dirrection )
{
    int index;

    /* Face image indexed by 2d array [level][dirrection] */
    index = 3 * level + dirrection;



    XCopyArea(     display, 
            template.pixmap, visible.pixmap, 
            gc, 
            0, 0,
            template.attributes.width, 
            template.attributes.height,
            0, 0 ); 

    XCopyArea(     display, 
            face[index].pixmap, visible.pixmap, 
            gc, 
            0, 0,
            template.attributes.width, 
            template.attributes.height,
            6, 5 ); 
 
    flush_expose( win ); 
    XCopyArea(     display, 
            visible.pixmap, win, gc, 
            0, 0,
            visible.attributes.width, 
            visible.attributes.height,
            0, 0 ); 

    flush_expose( iconwin ); 
    XCopyArea(     display, 
            visible.pixmap, iconwin, gc, 
            0, 0,
            visible.attributes.width, 
            visible.attributes.height,
            0, 0 );

}







/***********************************************************************
 *   getPixmaps
 *    
 *   Load XPM data into X Pixmaps.
 *  
 *   Pixmap template contains the window backdrop image -- never modified.
 *   Pixmap visible is the template pixmap. Appropriate face pixmap is copied
 *     onto it.
 *   Pixmap face_xy are the various face pixmaps. 
 *  
 ***********************************************************************/
void getPixmaps()
{
    int i;
	char message_buffer[256];

  /* Note:  FROM:  Motif FAQ (Part 5 of 9)
  If you are allocating 242 colors in an 8 bit display, then you are likely to
  run out of colors.  If you carefully read the Xpm manual, you will notice that
  one of the Xpm values that you can modify is the "closeness".  This value will
  control the actual closness of the colors allocated by the Xpm library.
  According to the Xpm manual:
           
         * The "closeness" field(s) (when set) control if and how colors
         are found when they failed to be allocated.  If the color cannot
         be allocated, Xpm looks in the colormap for a color that matches
         the desired closeness.
         
         * The value is an integer in the range 0 (black) - 65535 (white)
         
         * A closeness of less than 10000 will cause only "close" colors to
         match.
         
         * A cliseness of more than 50000 will allow quite disimilar colors
         to match.
         
         * A closeness of more than 65535 will allow any color to match.
         
         * A value of 40000 seems reasonable for many situations requiring
         reasonable but not perfect color matches.
  */
        
	char ** face_name[16] = { 
               face_0l, face_0c, face_0r,
               face_1l, face_1c, face_1r,
               face_2l, face_2c, face_2r,
               face_3l, face_3c, face_3r, 
               face_4l, face_4c, face_4r, face_god };

    for (i=0; i<16; i++ ) {
        face[i].attributes.valuemask |=
                (XpmReturnPixels | XpmReturnExtensions | XpmCloseness);
        face[i].attributes.closeness = 50001;

        if ( XpmCreatePixmapFromData( display, rootwin, face_name[i],
        &face[i].pixmap,&face[i].mask,&face[i].attributes) != XpmSuccess ) {
			sprintf (message_buffer, "Can't Create face #%d", i );
            perror(message_buffer);
            exit(1);
        }
    }

    /* Template Pixmap. Never Drawn To. */

    template.attributes.valuemask |=
            (XpmReturnPixels | XpmReturnExtensions | XpmCloseness);
    template.attributes.closeness = 50001;

    if ( XpmCreatePixmapFromData(    
                    display,
                    rootwin,
                    backdrop_xpm,
                    &template.pixmap,
                    &template.mask,
                    &template.attributes) != XpmSuccess ) {
           fprintf(stderr, "Can't Create 'template' Pixmap");
           exit(1);
    }



    /* Visible Pixmap. Copied from template Pixmap and then drawn to. */

    visible.attributes.valuemask |=
            (XpmReturnPixels | XpmReturnExtensions | XpmCloseness);
    visible.attributes.closeness = 50001;

    if ( XpmCreatePixmapFromData(    
                    display,
                    rootwin,
                    backdrop_xpm,
                    &visible.pixmap,
                    &visible.mask,
                    &visible.attributes) != XpmSuccess ) {
           fprintf(stderr, "Can't Create 'visible' Pixmap");
           exit(1);
    }

}





/***********************************************************************
 *   flush_expose
 *
 *   Everyone else has one of these... Can't hurt to throw it in.
 *
 ***********************************************************************/
int flush_expose(Window w) {

    XEvent      dummy;
    int         i=0;

    while (XCheckTypedWindowEvent(display, w, Expose, &dummy))
        i++;

    return i;
}



/***********************************************************************
 * get_cpu
 *            Uses Linux style /proc file system
 ***********************************************************************/

float get_cpu( float scale_factor )
{
    FILE * fin;
    char buffer[256];
    char tokens[4] = " \t\n";

    static long last_user = 0;
    static long last_nice = 0;
    static long last_sys = 0;
    static long last_idle = 0;

    long user, nice, sys, idle, total;
    long d_user, d_nice, d_sys, d_idle;
    float cpu_use;
    float percent;

    if ( (fin = fopen("/proc/stat", "r")) == NULL ) {
        return(0);
    }
    
    while ( fgets(buffer, 256, fin) ) {
        if ( strstr(buffer,"cpu") != NULL ) {
            strtok(buffer,tokens);
            user = atol(strtok(NULL,tokens));
            nice = atol(strtok(NULL,tokens));
            sys =  atol(strtok(NULL,tokens));
            idle = atol(strtok(NULL,tokens));
        }
    }
    
    fclose (fin);


    d_user = user - last_user;
    d_nice = nice - last_nice;
    d_sys  = sys  - last_sys;
    d_idle = idle - last_idle;

    last_user = user;
    last_nice = nice;
    last_sys  = sys;
    last_idle = idle;

    total = d_user + d_sys + d_nice + d_idle;

    if ( total < 1 ) 
        total = 1.0;

    cpu_use = 1.0 - ( (float)  d_idle  / (float) total );

    if ( Verbose ) {
        printf("   CPU (/proc/stat): %5.2f  ", cpu_use);
        printf(" (jiffies: user:%ld nice:%ld system:%ld idle:%ld total:%ld)\n", 
            d_user, d_nice, d_sys, d_idle, total  );
    }

    percent = cpu_use / scale_factor;
    if ( percent > .999999 ) 
        percent = .999999;

    return ( percent );    

}


/***********************************************************************
 * get_load
 *            Uses Linux style /proc file system
 ***********************************************************************/

float get_load( float scale_factor )
{
    FILE * fin;
    char buffer[256];
    char * strtok_ptr;
    char tokens[4] = " \t\n";
    float load = 0.0;
    float percent;

    if ( (fin = fopen("/proc/loadavg", "r")) == NULL ) {
        return(0);
    }
    
    while ( fgets(buffer, 256, fin) ) { 

        strtok_ptr = strtok(buffer,tokens);

        if ( strtok_ptr == NULL ) {
            load = 0;
        } else {
            load = atof(strtok_ptr);
        }
    }
 
    fclose (fin);

    if ( Verbose ) 
        printf("   LOAD (/proc/loadavg): %f \n", load ); 

    percent = load / scale_factor;
    if ( percent > .999999 ) 
        percent = .999999;

    return ( percent );    
}




/***********************************************************************
 * get_command
 *            do a popen(command.command_str ... )
 *          Use the command.index 'th token.
 ***********************************************************************/

float get_command( command_type command, float scale_factor )
{
    FILE * fin;
    char buffer[256];
    char * token_ptr;
    int i;

    char tokens[5] = ", \t\n";
    float load = 0.0;
    float percent;

    if ( Verbose )
        printf ("   Command = \"%s\" index = %d\n", 
           command.command_str, command.index);

    if ( (fin = popen(command.command_str, "r")) == NULL ) {
        fprintf (stderr, "Can not popen(%s)\n", command.command_str); 
        exit(-1);
    }

        
    if ( fgets(buffer, 256, fin) == NULL ) {
        fprintf (stderr, "popen(%s) returns nothing.\n", command.command_str); 
        exit(-1);
    }

    pclose (fin);

    if ( Verbose )    printf ("   Returns \"%s\"\n", buffer ); 

    if ( (token_ptr = strtok(buffer,tokens)) == NULL ) {
        fprintf(stderr,"No tokens found in %s.\n", command.command_str);
        exit(-1);
    }

    if ( Verbose ) printf ("(1,'%s')", token_ptr);

    for ( i=1; i < command.index; i++ ) {
        if ( (token_ptr = strtok(NULL,tokens)) == NULL ) {
            fprintf(stderr,"There are less than %d tokens returned by %s.\n",
                command.index, command.command_str);
            exit(-1);
        }
        if ( Verbose ) printf ("(%d,'%s')", i+1, token_ptr);

    }

    if ( Verbose ) printf ("\n");

    load = atof( token_ptr );

    if ( Verbose ) printf ("   Using \"%s\" -->  %f\n", token_ptr, load ); 
    
    percent = load / scale_factor;
    if ( percent > .999999 ) 
        percent = .999999;

    return ( percent );    
}



/***********************************************************************
 *         show_usage
 *
 ***********************************************************************/

void show_usage()
{

printf("
%s
usage: [-v] [-h] 
       [-s yes/no] [-w withdrawn/iconic/normal] 
       [-m cpu/load/uptime] 
       [-f scale_factor ] [-u miliseconds] 
       [-d dislay ] [-g geometry ]


-g                        geometry:  ie: 64x64+10+10
-d                             dpy:  Display. ie: 127.0.0.1:0.0
-v                                :  Verbose.
-h                                :  Help. This screen.
-w         withdrawn/iconic/normal:  Windows: Iconic, Normal, or Withdrawn
-s                          yes/no:  Shaped window: y or n
-m cpu/load/command[command index]:  Use load, cpu, or external command
-f                    scale_factor:  Scale: floating point number = 100%% bloody
-u                     miliseconds:  Update period in miliseconds

Defaults:
   -g 64x64+10+10
   -d NULL
   -w withdrawn
   -s yes
   -m load
   -f 1.0 for CPU / -f 2.0 for LOAD
   -u 999


", VERSION );

exit(-1);

}
    


